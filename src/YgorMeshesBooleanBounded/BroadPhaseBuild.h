#pragma once

#include "BroadPhaseCodec.h"
#include "BroadPhasePreflight.h"
#include "BroadPhaseVerifier.h"
#include "CandidateCanonicalization.h"
#include "ContextVerifier.h"
#include "Outcome.h"
#include "RankMortonKey.h"
#include "Transaction.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {
namespace broad_phase_build_detail {

inline void bind_error(bounded_boolean_error &error,
                       const bounded_boolean_digest &context_digest,
                       const bounded_boolean_digest &replay_digest) noexcept {
  error.context_digest = context_digest;
  error.replay_digest = replay_digest;
}

inline bool checked_sum(std::uint64_t a, std::uint64_t b,
                        std::uint64_t &out) noexcept {
  return checked_add<std::uint64_t>(a, b, out);
}

inline bool checked_accumulate(std::uint64_t value,
                               std::uint64_t &total) noexcept {
  return checked_add<std::uint64_t>(total, value, total);
}

template <class U>
inline bool vector_storage_bytes(const std::vector<U> &values,
                                 std::uint64_t &bytes) noexcept {
  return checked_multiply<std::uint64_t>(
      static_cast<std::uint64_t>(values.size()),
      static_cast<std::uint64_t>(sizeof(U)), bytes);
}

template <class T, class I>
bool estimate_persistent_bytes(const canonical_candidate_stream<T, I> &artifact,
                               std::uint64_t &bytes) noexcept {
  bytes = sizeof(canonical_candidate_stream<T, I>);
  const auto add_vector = [&](const auto &values) {
    std::uint64_t contribution = 0;
    return vector_storage_bytes(values, contribution) &&
           checked_accumulate(contribution, bytes);
  };
  for (const auto operand : {operand_id::a, operand_id::b}) {
    const auto &table = artifact.primitive_table(operand);
    const auto &hierarchy = artifact.hierarchy(operand);
    if (!add_vector(table.edges) || !add_vector(table.triangles) ||
        !add_vector(hierarchy.spatial_primitives) || !add_vector(hierarchy.nodes))
      return false;
    for (const auto &triangle : table.triangles) {
      std::uint64_t morton_bytes = 0;
      if (!vector_storage_bytes(triangle.morton.words, morton_bytes) ||
          !checked_accumulate(morton_bytes, bytes))
        return false;
    }
  }
  return add_vector(artifact.count_plans()) && add_vector(artifact.witnesses()) &&
         add_vector(artifact.candidates()) && add_vector(artifact.partitions()) &&
         add_vector(artifact.canonical_bytes());
}

inline bool evidence_equal(const broad_phase_verification_evidence &a,
                           const broad_phase_verification_evidence &b) noexcept {
  return a.id == b.id && a.verifier_version == b.verifier_version &&
         a.primitive_reconstruction_complete ==
             b.primitive_reconstruction_complete &&
         a.rank_reconstruction_complete == b.rank_reconstruction_complete &&
         a.hierarchy_reconstruction_complete ==
             b.hierarchy_reconstruction_complete &&
         a.breadth_first_candidate_set_complete ==
             b.breadth_first_candidate_set_complete &&
         a.exhaustive_all_pairs_performed == b.exhaustive_all_pairs_performed &&
         a.exhaustive_all_pairs_complete == b.exhaustive_all_pairs_complete &&
         a.breadth_first_candidate_count == b.breadth_first_candidate_count &&
         a.exhaustive_candidate_count == b.exhaustive_candidate_count &&
         a.verifier_work_units == b.verifier_work_units &&
         a.maximum_queue == b.maximum_queue &&
         a.candidate_set_digest == b.candidate_set_digest &&
         a.reserved == b.reserved;
}

} // namespace broad_phase_build_detail

template <class T, class I> class broad_phase_builder final {
public:
  broad_phase_builder(
      const boolean_context<T, I> &context,
      const precision_context<T> &precision,
      std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
      broad_phase_capabilities capabilities)
      : context_(context), precision_(precision), manifolds_(std::move(manifolds)),
        capabilities_(std::move(capabilities)) {}

  boolean_outcome<std::shared_ptr<const canonical_candidate_stream<T, I>>>
  run() {
    try {
      if (!validate_contracts() || !preflight_and_reserve() ||
          !build_primitives_and_hierarchies() || !count_and_materialize() ||
          !canonicalize_and_verify())
        return failure();
      // Construct the immutable publication handle before the no-fail commit
      // boundary. Moving a shared_ptr after commit is non-allocating.
      auto published = std::shared_ptr<const canonical_candidate_stream<T, I>>(
          std::move(artifact_));
      if (!transaction_.begin_join() || !transaction_.begin_verify() ||
          !transaction_.ready() || !transaction_.commit()) {
        fail(broad_phase_subcode::transaction_failure,
             bounded_boolean_error_category::internal_invariant_error,
             "broad-phase transaction could not commit",
             broad_phase_checkpoint::transaction_commit);
        return failure();
      }
      return boolean_outcome<
          std::shared_ptr<const canonical_candidate_stream<T, I>>>::success(
          std::move(published));
    } catch (const std::bad_alloc &) {
      fail(broad_phase_subcode::resource_preflight,
           bounded_boolean_error_category::resource_limit,
           "broad-phase allocation failed",
           broad_phase_checkpoint::fixed_resource_reservation);
      return failure();
    } catch (...) {
      fail(broad_phase_subcode::internal_invariant,
           bounded_boolean_error_category::internal_invariant_error,
           "broad-phase construction raised an unexpected exception",
           broad_phase_checkpoint::transaction_commit);
      return failure();
    }
  }

private:
  using artifact_type = canonical_candidate_stream<T, I>;
  using outcome_type =
      boolean_outcome<std::shared_ptr<const canonical_candidate_stream<T, I>>>;

  outcome_type failure() {
    broad_phase_build_detail::bind_error(error_, context_.context_digest,
                                         context_.replay_digest);
    return outcome_type::failure(error_);
  }

  bool fail(broad_phase_subcode subcode,
            bounded_boolean_error_category category, const char *summary,
            broad_phase_checkpoint checkpoint) {
    if (error_.subcode == 0)
      error_ = broad_phase_error(subcode, category, summary, checkpoint);
    return false;
  }

  bool check_cancel(broad_phase_checkpoint checkpoint) {
    return !broad_phase_cancelled(capabilities_) ||
           fail(broad_phase_subcode::cancelled,
                bounded_boolean_error_category::cancelled,
                "broad-phase construction cancelled", checkpoint);
  }

  bool validate_contracts() {
    if (!transaction_.open())
      return fail(broad_phase_subcode::transaction_failure,
                  bounded_boolean_error_category::internal_invariant_error,
                  "broad-phase transaction did not open",
                  broad_phase_checkpoint::context_capability_validation);
    if (!manifolds_ || !verify_context(context_) ||
        !context_.owner.same_owner(capabilities_.owner) ||
        !precision_.owner().same_owner(capabilities_.owner) ||
        !manifolds_->owner().same_owner(capabilities_.owner) ||
        precision_.boolean_context_digest() != context_.context_digest ||
        !precision_.ordinary_success_eligible()) {
      return fail(broad_phase_subcode::wrong_owner,
                  bounded_boolean_error_category::internal_invariant_error,
                  "broad-phase context, precision, or predecessor handshake failed",
                  broad_phase_checkpoint::context_capability_validation);
    }
    if (!check_cancel(broad_phase_checkpoint::context_capability_validation))
      return false;
    artifact_ = std::make_unique<artifact_type>();
    artifact_->owner_ = capabilities_.owner;
    artifact_->manifolds_ = manifolds_;
    artifact_->precision_digest_ = precision_.digest();
    artifact_->predecessor_digest_ = manifolds_->digest();
    return true;
  }

  bool preflight_and_reserve() {
    if (!preflight_broad_phase(*manifolds_, capabilities_, preflight_, error_))
      return false;
    if (!capabilities_.resources)
      return fail(broad_phase_subcode::resource_preflight,
                  bounded_boolean_error_category::input_contract_error,
                  "broad-phase resource manager is required",
                  broad_phase_checkpoint::fixed_resource_reservation);

    std::uint64_t node_total = 0;
    std::uint64_t edge_total = 0;
    std::uint64_t node_edge_work_a = 0;
    std::uint64_t node_edge_work_b = 0;
    std::uint64_t traversal_work = preflight_.fixed_work_units;
    if (!broad_phase_build_detail::checked_sum(preflight_.node_upper_bounds[0],
                                               preflight_.node_upper_bounds[1],
                                               node_total) ||
        !broad_phase_build_detail::checked_sum(preflight_.edges[0],
                                               preflight_.edges[1], edge_total) ||
        !checked_multiply<std::uint64_t>(preflight_.edges[0],
                                         preflight_.node_upper_bounds[1],
                                         node_edge_work_a) ||
        !checked_multiply<std::uint64_t>(preflight_.edges[1],
                                         preflight_.node_upper_bounds[0],
                                         node_edge_work_b) ||
        !broad_phase_build_detail::checked_accumulate(node_edge_work_a, traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(node_edge_work_a, traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(node_edge_work_a, traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(node_edge_work_a, traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(node_edge_work_b, traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(node_edge_work_b, traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(node_edge_work_b, traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(node_edge_work_b, traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(preflight_.pair_products[0], traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(preflight_.pair_products[0], traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(preflight_.pair_products[0], traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(preflight_.pair_products[0], traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(preflight_.pair_products[0], traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(preflight_.pair_products[0], traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(preflight_.pair_products[1], traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(preflight_.pair_products[1], traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(preflight_.pair_products[1], traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(preflight_.pair_products[1], traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(preflight_.pair_products[1], traversal_work) ||
        !broad_phase_build_detail::checked_accumulate(preflight_.pair_products[1], traversal_work) ||
        traversal_work > capabilities_.maximum_work_units) {
      return fail(broad_phase_subcode::traversal_limit,
                  bounded_boolean_error_category::resource_limit,
                  "broad-phase worst-case traversal work exceeds configured limit",
                  broad_phase_checkpoint::representability_preflight);
    }

    node_reservation_ = capabilities_.resources->reserve(
        resource_kind::broad_phase_nodes, node_total);
    fixed_persistent_reservation_ = capabilities_.resources->reserve(
        resource_kind::persistent_bytes, preflight_.fixed_persistent_bytes);
    fixed_temporary_reservation_ = capabilities_.resources->reserve(
        resource_kind::temporary_bytes, preflight_.fixed_temporary_bytes);
    work_reservation_ = capabilities_.resources->reserve(resource_kind::work_units,
                                                         traversal_work);
    if (!node_reservation_ || !fixed_persistent_reservation_ ||
        !fixed_temporary_reservation_ || !work_reservation_)
      return fail(broad_phase_subcode::resource_preflight,
                  bounded_boolean_error_category::resource_limit,
                  "broad-phase fixed resource reservation failed",
                  broad_phase_checkpoint::fixed_resource_reservation);
    return transaction_.register_work() ||
           fail(broad_phase_subcode::transaction_failure,
                bounded_boolean_error_category::internal_invariant_error,
                "broad-phase transaction could not register work",
                broad_phase_checkpoint::fixed_resource_reservation);
  }

  bool build_primitives_and_hierarchies() {
    const canonical_halfedge_operand<T, I> *sources[2] = {
        manifolds_->a().get(), manifolds_->b().get()};
    for (std::size_t slot = 0; slot < 2; ++slot) {
      if (!check_cancel(slot == 0
                            ? broad_phase_checkpoint::operand_a_primitive_construction
                            : broad_phase_checkpoint::operand_b_primitive_construction))
        return false;
      if (!build_broad_phase_primitive_table(
              *sources[slot], capabilities_.owner,
              artifact_->primitive_tables_[slot], error_))
        return false;
      rank_morton_build_result ranks;
      if (!assign_rank_morton_order(artifact_->primitive_tables_[slot].triangles,
                                    ranks, error_))
        return false;
      rank_results_[slot] = std::move(ranks);
      if (!build_triangle_aabb_hierarchy(
              sources[slot]->operand(),
              artifact_->primitive_tables_[slot].triangles,
              rank_results_[slot].spatial_order,
              artifact_->hierarchies_[slot], error_) ||
          !verify_triangle_aabb_hierarchy_producer(
              artifact_->primitive_tables_[slot].triangles,
              artifact_->hierarchies_[slot], error_))
        return false;

      artifact_->statistics_.edge_counts[slot] = preflight_.edges[slot];
      artifact_->statistics_.source_edge_counts[slot] =
          preflight_.source_edges[slot];
      artifact_->statistics_.internal_diagonal_counts[slot] =
          preflight_.internal_diagonals[slot];
      artifact_->statistics_.triangle_counts[slot] = preflight_.triangles[slot];
      artifact_->statistics_.distinct_rank_counts_x[slot] =
          rank_results_[slot].distinct_rank_counts[0];
      artifact_->statistics_.distinct_rank_counts_y[slot] =
          rank_results_[slot].distinct_rank_counts[1];
      artifact_->statistics_.distinct_rank_counts_z[slot] =
          rank_results_[slot].distinct_rank_counts[2];
      artifact_->statistics_.leaf_counts[slot] =
          artifact_->hierarchies_[slot].leaf_count;
      artifact_->statistics_.node_counts[slot] =
          artifact_->hierarchies_[slot].nodes.size();
      artifact_->statistics_.hierarchy_heights[slot] =
          artifact_->hierarchies_[slot].height;
      if (!artifact_->hierarchies_[slot].empty())
        artifact_->statistics_.maximum_producer_stack = std::max(
            artifact_->statistics_.maximum_producer_stack,
            artifact_->hierarchies_[slot].height + 1);
    }
    return true;
  }

  bool count_and_materialize() {
    if (!count_directed_candidates(
            directed_candidate_role::a_edge_b_triangle,
            artifact_->primitive_tables_[0].edges,
            artifact_->primitive_tables_[1].triangles,
            artifact_->hierarchies_[1], capabilities_, artifact_->count_plans_,
            artifact_->statistics_, error_) ||
        !count_directed_candidates(
            directed_candidate_role::b_edge_a_triangle,
            artifact_->primitive_tables_[1].edges,
            artifact_->primitive_tables_[0].triangles,
            artifact_->hierarchies_[0], capabilities_, artifact_->count_plans_,
            artifact_->statistics_, error_))
      return false;

    std::uint64_t total = 0;
    for (auto &plan : artifact_->count_plans_) {
      plan.output_prefix = total;
      if (!broad_phase_build_detail::checked_accumulate(plan.candidate_count,
                                                        total) ||
          total > capabilities_.maximum_candidates)
        return fail(broad_phase_subcode::count_overflow,
                    bounded_boolean_error_category::resource_limit,
                    "broad-phase candidate count exceeds configured limit",
                    broad_phase_checkpoint::prefix_and_candidate_reservation);
    }

    std::uint64_t discovery_bytes = 0;
    std::uint64_t persistent_candidate_bytes = 0;
    if (!checked_multiply<std::uint64_t>(
            total, sizeof(broad_phase_candidate_discovery<T>), discovery_bytes) ||
        !checked_multiply<std::uint64_t>(
            total,
            sizeof(canonical_candidate_record<T>) +
                sizeof(broad_phase_overlap_witness<T>),
            persistent_candidate_bytes))
      return fail(broad_phase_subcode::byte_count_overflow,
                  bounded_boolean_error_category::index_overflow,
                  "broad-phase candidate byte count overflow",
                  broad_phase_checkpoint::prefix_and_candidate_reservation);

    candidate_reservation_ = capabilities_.resources->reserve(
        resource_kind::broad_phase_candidates, total);
    discovery_reservation_ = capabilities_.resources->reserve(
        resource_kind::temporary_bytes, discovery_bytes);
    candidate_bytes_reservation_ = capabilities_.resources->reserve(
        resource_kind::persistent_bytes, persistent_candidate_bytes);
    if (!candidate_reservation_ || !discovery_reservation_ ||
        !candidate_bytes_reservation_)
      return fail(broad_phase_subcode::resource_preflight,
                  bounded_boolean_error_category::resource_limit,
                  "broad-phase candidate reservation failed",
                  broad_phase_checkpoint::prefix_and_candidate_reservation);

    std::vector<broad_phase_candidate_discovery<T>> discoveries;
    discoveries.resize(static_cast<std::size_t>(total));
    const auto b_plan_begin = artifact_->primitive_tables_[0].edges.size();
    if (!emit_directed_candidates(
            directed_candidate_role::a_edge_b_triangle,
            artifact_->primitive_tables_[0].edges,
            artifact_->primitive_tables_[1].triangles,
            artifact_->hierarchies_[1], capabilities_, artifact_->count_plans_, 0,
            discoveries, artifact_->statistics_, error_) ||
        !emit_directed_candidates(
            directed_candidate_role::b_edge_a_triangle,
            artifact_->primitive_tables_[1].edges,
            artifact_->primitive_tables_[0].triangles,
            artifact_->hierarchies_[0], capabilities_, artifact_->count_plans_,
            b_plan_begin, discoveries, artifact_->statistics_, error_))
      return false;

    return canonicalize_broad_phase_candidates(
        std::move(discoveries), artifact_->primitive_tables_,
        artifact_->hierarchies_, capabilities_, artifact_->witnesses_,
        artifact_->candidates_, artifact_->partitions_, artifact_->statistics_,
        error_);
  }

  bool canonicalize_and_verify() {
    broad_phase_verification_evidence evidence;
    if (!verify_canonical_candidate_stream(*artifact_, context_, precision_,
                                           capabilities_, evidence, error_, false))
      return false;
    artifact_->statistics_.verifier_work_units = evidence.verifier_work_units;
    artifact_->statistics_.maximum_verifier_queue = evidence.maximum_queue;
    broad_phase_verifier_access::set_evidence(*artifact_, evidence);

    std::uint64_t persistent = 0;
    artifact_->statistics_.persistent_bytes = 0;
    artifact_->statistics_.canonical_bytes = 0;
    if (!broad_phase_codec_access::refresh(*artifact_, error_) ||
        !broad_phase_build_detail::estimate_persistent_bytes(*artifact_, persistent))
      return fail(broad_phase_subcode::byte_count_overflow,
                  bounded_boolean_error_category::index_overflow,
                  "broad-phase persistent byte count overflow",
                  broad_phase_checkpoint::codec_digest_resource_reconciliation);
    artifact_->statistics_.persistent_bytes = persistent;
    if (!broad_phase_codec_access::refresh(*artifact_, error_))
      return false;
    if (artifact_->canonical_bytes_.size() >
        capabilities_.maximum_canonical_bytes)
      return fail(broad_phase_subcode::resource_preflight,
                  bounded_boolean_error_category::resource_limit,
                  "broad-phase canonical stream exceeds configured byte limit",
                  broad_phase_checkpoint::canonical_encoding);

    std::uint64_t covered_persistent = 0;
    if (!checked_add<std::uint64_t>(fixed_persistent_reservation_->amount(),
                                    candidate_bytes_reservation_->amount(),
                                    covered_persistent))
      return fail(broad_phase_subcode::byte_count_overflow,
                  bounded_boolean_error_category::index_overflow,
                  "broad-phase persistent reservation sum overflow",
                  broad_phase_checkpoint::codec_digest_resource_reconciliation);
    const std::uint64_t extra_persistent =
        persistent > covered_persistent ? persistent - covered_persistent : 0;
    final_persistent_reservation_ = capabilities_.resources->reserve(
        resource_kind::persistent_bytes, extra_persistent);
    codec_reservation_ = capabilities_.resources->reserve(
        resource_kind::replay_bytes, artifact_->canonical_bytes_.size());
    if (!final_persistent_reservation_ || !codec_reservation_)
      return fail(broad_phase_subcode::resource_preflight,
                  bounded_boolean_error_category::resource_limit,
                  "broad-phase final artifact reservation failed",
                  broad_phase_checkpoint::codec_digest_resource_reconciliation);

    broad_phase_verification_evidence final_evidence;
    if (!verify_canonical_candidate_stream(*artifact_, context_, precision_,
                                           capabilities_, final_evidence, error_, true) ||
        !broad_phase_build_detail::evidence_equal(evidence, final_evidence))
      return fail(broad_phase_subcode::counter_mismatch,
                  bounded_boolean_error_category::internal_invariant_error,
                  "broad-phase verification evidence changed after encoding",
                  broad_phase_checkpoint::codec_digest_resource_reconciliation);

    std::uint64_t actual_work = artifact_->statistics_.producer_work_units;
    if (!broad_phase_build_detail::checked_accumulate(
            evidence.verifier_work_units, actual_work) ||
        !broad_phase_build_detail::checked_accumulate(
            final_evidence.verifier_work_units, actual_work) ||
        !work_reservation_ || actual_work > work_reservation_->amount())
      return fail(broad_phase_subcode::resource_reconciliation,
                  bounded_boolean_error_category::resource_limit,
                  "broad-phase actual work exceeds reserved work",
                  broad_phase_checkpoint::codec_digest_resource_reconciliation);

    if (!check_cancel(broad_phase_checkpoint::transaction_commit))
      return false;
    const std::uint64_t fixed_used =
        std::min(fixed_persistent_reservation_->amount(), persistent);
    const std::uint64_t after_fixed = persistent - fixed_used;
    const std::uint64_t candidate_used =
        std::min(candidate_bytes_reservation_->amount(), after_fixed);
    const std::uint64_t final_used = after_fixed - candidate_used;
    std::uint64_t node_count = 0;
    if (!checked_add<std::uint64_t>(artifact_->hierarchies_[0].nodes.size(),
                                    artifact_->hierarchies_[1].nodes.size(),
                                    node_count) ||
        !node_reservation_->commit(node_count) ||
        !candidate_reservation_->commit(artifact_->candidates_.size()) ||
        !fixed_persistent_reservation_->commit(fixed_used) ||
        !candidate_bytes_reservation_->commit(candidate_used) ||
        !final_persistent_reservation_->commit(final_used) ||
        !codec_reservation_->commit(artifact_->canonical_bytes_.size()) ||
        !work_reservation_->commit(actual_work))
      return fail(broad_phase_subcode::resource_reconciliation,
                  bounded_boolean_error_category::internal_invariant_error,
                  "broad-phase resource commit failed",
                  broad_phase_checkpoint::codec_digest_resource_reconciliation);
    fixed_temporary_reservation_->release();
    discovery_reservation_->release();
    return true;
  }

  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds_;
  broad_phase_capabilities capabilities_{};
  broad_phase_preflight_counts preflight_{};
  std::array<rank_morton_build_result, 2> rank_results_{};
  std::unique_ptr<artifact_type> artifact_;
  stage_transaction transaction_;
  std::optional<resource_reservation> node_reservation_;
  std::optional<resource_reservation> candidate_reservation_;
  std::optional<resource_reservation> fixed_persistent_reservation_;
  std::optional<resource_reservation> candidate_bytes_reservation_;
  std::optional<resource_reservation> final_persistent_reservation_;
  std::optional<resource_reservation> fixed_temporary_reservation_;
  std::optional<resource_reservation> discovery_reservation_;
  std::optional<resource_reservation> codec_reservation_;
  std::optional<resource_reservation> work_reservation_;
  bounded_boolean_error error_{};
};

template <class T, class I>
boolean_outcome<std::shared_ptr<const canonical_candidate_stream<T, I>>>
build_canonical_candidate_stream(
    const boolean_context<T, I> &context,
    const precision_context<T> &precision,
    std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
    broad_phase_capabilities capabilities = {}) {
  broad_phase_builder<T, I> builder(context, precision, std::move(manifolds),
                                     std::move(capabilities));
  return builder.run();
}

template <class T, class I>
boolean_outcome<std::shared_ptr<const canonical_candidate_stream<T, I>>>
decode_canonical_candidate_stream(
    const std::vector<std::uint8_t> &bytes,
    const boolean_context<T, I> &context,
    const precision_context<T> &precision,
    std::shared_ptr<const canonical_source_manifolds<T, I>> manifolds,
    broad_phase_capabilities capabilities = {}) {
  canonical_candidate_stream_envelope envelope;
  bounded_boolean_error error;
  if (!parse_canonical_candidate_stream_envelope(
          bytes, capabilities.maximum_canonical_bytes, envelope, error)) {
    broad_phase_build_detail::bind_error(error, context.context_digest,
                                         context.replay_digest);
    return boolean_outcome<
        std::shared_ptr<const canonical_candidate_stream<T, I>>>::failure(
        std::move(error));
  }
  if (!manifolds || envelope.predecessor_digest != manifolds->digest() ||
      envelope.precision_digest != precision.digest()) {
    error = broad_phase_error(broad_phase_subcode::predecessor_digest_mismatch,
                              bounded_boolean_error_category::input_contract_error,
                              "broad-phase encoded predecessor digest mismatch",
                              broad_phase_checkpoint::predecessor_validation);
    broad_phase_build_detail::bind_error(error, context.context_digest,
                                         context.replay_digest);
    return boolean_outcome<
        std::shared_ptr<const canonical_candidate_stream<T, I>>>::failure(
        std::move(error));
  }
  auto rebuilt = build_canonical_candidate_stream(
      context, precision, std::move(manifolds), std::move(capabilities));
  if (!rebuilt.has_value())
    return rebuilt;
  if ((*rebuilt.value())->canonical_bytes() != bytes) {
    error = broad_phase_error(broad_phase_subcode::codec_error,
                              bounded_boolean_error_category::input_contract_error,
                              "broad-phase encoded artifact is not canonical",
                              broad_phase_checkpoint::canonical_encoding);
    broad_phase_build_detail::bind_error(error, context.context_digest,
                                         context.replay_digest);
    return boolean_outcome<
        std::shared_ptr<const canonical_candidate_stream<T, I>>>::failure(
        std::move(error));
  }
  return rebuilt;
}

} // namespace ygor::mesh_boolean::bounded
