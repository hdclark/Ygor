#pragma once

#include "CoplanarRelationOverlay.h"
#include "ContextVerifier.h"
#include "PrecisionContext.h"
#include "RelationPreflight.h"
#include "RelationArtifactAssembly.h"
#include "RelationVerifier.h"
#include "Transaction.h"

#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <utility>

namespace ygor::mesh_boolean::bounded {
namespace relation_build_detail {

inline void bind_relation_error(bounded_boolean_error &error,
                                const bounded_boolean_digest &context_digest,
                                const bounded_boolean_digest &replay_digest) noexcept {
  error.context_digest = context_digest;
  error.replay_digest = replay_digest;
}

inline bool checked_accumulate_relation_bytes(std::uint64_t value,
                                              std::uint64_t &total) noexcept {
  return checked_add<std::uint64_t>(total, value, total);
}

template <class U>
inline bool relation_vector_storage_bytes(const std::vector<U> &values,
                                          std::uint64_t &bytes) noexcept {
  return checked_multiply<std::uint64_t>(
      static_cast<std::uint64_t>(values.size()),
      static_cast<std::uint64_t>(sizeof(U)), bytes);
}

inline bool relation_graph_storage_bytes(const relation_request_graph &graph,
                                         std::uint64_t &bytes) noexcept {
  bytes = sizeof(relation_request_graph);
  const auto add = [&](const auto &values) {
    std::uint64_t contribution = 0;
    return relation_vector_storage_bytes(values, contribution) &&
           checked_accumulate_relation_bytes(contribution, bytes);
  };
  return add(graph.requests) && add(graph.dependencies) &&
         add(graph.reverse_consumers) && add(graph.candidate_witnesses);
}

template <class T, class I>
bool estimate_relation_persistent_bytes(
    const signed_feature_relations<T, I> &artifact,
    std::uint64_t &bytes) {
  bytes = sizeof(signed_feature_relations<T, I>);
  const auto add_vector = [&](const auto &values) {
    std::uint64_t contribution = 0;
    return relation_vector_storage_bytes(values, contribution) &&
           checked_accumulate_relation_bytes(contribution, bytes);
  };
  std::uint64_t graph_bytes = 0;
  if (!relation_graph_storage_bytes(artifact.request_graph(), graph_bytes) ||
      !checked_accumulate_relation_bytes(graph_bytes, bytes) ||
      !add_vector(artifact.imported_geometry()) ||
      !add_vector(artifact.bounded_primitives()) ||
      !add_vector(artifact.exact_relations()) ||
      !add_vector(artifact.truth_lineage()) ||
      !add_vector(artifact.truth_records()) ||
      !add_vector(artifact.relations()) ||
      !add_vector(artifact.constructions()) ||
      !add_vector(artifact.coplanar_event_nodes()) ||
      !add_vector(artifact.coplanar_oriented_arcs()) ||
      !add_vector(artifact.coplanar_overlap_components()) ||
      !add_vector(artifact.symbolic_eligibility()) ||
      !add_vector(artifact.symbolic_decisions()) ||
      !add_vector(artifact.crossings()) ||
      !add_vector(artifact.event_seeds()) ||
      !add_vector(artifact.event_seed_incidence()) ||
      !add_vector(artifact.candidate_dispositions()) ||
      !add_vector(artifact.canonical_bytes()))
    return false;

  for (const auto &node : artifact.coplanar_event_nodes()) {
    if (!add_vector(node.occurrences))
      return false;
    for (const auto &occurrence : node.occurrences)
      if (!add_vector(occurrence.event_lineages))
        return false;
  }
  for (const auto &arc : artifact.coplanar_oriented_arcs())
    if (!add_vector(arc.occurrences) || !add_vector(arc.overlap_lineages))
      return false;
  for (const auto &component : artifact.coplanar_overlap_components())
    if (!add_vector(component.node_ids) || !add_vector(component.arc_ids))
      return false;

  // Detailed relation stages contain nested variable-length records. Their
  // owner-free semantic encodings are a deterministic, architecture-independent
  // accounting projection of that persistent content; the stage objects
  // themselves are charged separately.
  const auto add_stage = [&](const auto &stage, const auto &encoder) {
    if (!stage)
      return true;
    const auto semantic = encoder(*stage);
    return checked_accumulate_relation_bytes(
               static_cast<std::uint64_t>(sizeof(*stage)), bytes) &&
           checked_accumulate_relation_bytes(
               static_cast<std::uint64_t>(semantic.size()), bytes);
  };
  return add_stage(artifact.source_edge_stage(),
                   [](const auto &stage) {
                     return encode_candidate_source_edge_relation_semantics(stage);
                   }) &&
         add_stage(artifact.source_edge_facet_stage(),
                   [](const auto &stage) {
                     return encode_candidate_source_edge_facet_relation_semantics(stage);
                   }) &&
         add_stage(artifact.source_facet_stage(),
                   [](const auto &stage) {
                     return encode_candidate_source_facet_relation_semantics(stage);
                   }) &&
         add_stage(artifact.coplanar_overlay_stage(),
                   [](const auto &stage) {
                     return encode_candidate_coplanar_overlay_semantics(stage);
                   });
}

} // namespace relation_build_detail

template <class T, class I> class relation_builder final {
public:
  relation_builder(
      const boolean_context<T, I> &context,
      const precision_context<T> &precision,
      std::shared_ptr<const canonical_candidate_stream<T, I>> candidates,
      relation_capabilities capabilities)
      : context_(context), precision_(precision),
        candidates_(std::move(candidates)), capabilities_(std::move(capabilities)) {}

  boolean_outcome<std::shared_ptr<const signed_feature_relations<T, I>>> run() {
    try {
      if (!validate_contracts() || !preflight_and_reserve() ||
          !build_candidate_edge_relations() ||
          !build_candidate_edge_facet_relations() ||
          !build_candidate_facet_relations() ||
          !build_candidate_coplanar_overlays() ||
          !build_final_artifact() || !encode_and_verify())
        return failure();
      auto published =
          std::shared_ptr<const signed_feature_relations<T, I>>(
              std::move(artifact_));
      if (!commit_resources() || !transaction_.begin_join() ||
          !transaction_.begin_verify() || !transaction_.ready() ||
          !transaction_.commit()) {
        fail(relation_subcode::transaction_failure,
             bounded_boolean_error_category::internal_invariant_error,
             "Component 07 transaction could not commit",
             relation_checkpoint::transaction_commit);
        return failure();
      }
      return outcome_type::success(std::move(published));
    } catch (const std::bad_alloc &) {
      fail(relation_subcode::resource_preflight,
           bounded_boolean_error_category::resource_limit,
           "Component 07 allocation failed",
           relation_checkpoint::discovery_resource_reservation);
      return failure();
    } catch (...) {
      fail(relation_subcode::internal_invariant,
           bounded_boolean_error_category::internal_invariant_error,
           "Component 07 construction raised an unexpected exception",
           relation_checkpoint::transaction_commit);
      return failure();
    }
  }

private:
  using artifact_type = signed_feature_relations<T, I>;
  using outcome_type =
      boolean_outcome<std::shared_ptr<const signed_feature_relations<T, I>>>;
  using edge_stage_type = candidate_source_edge_relation_stage<T>;
  using edge_facet_stage_type = candidate_source_edge_facet_relation_stage<T>;
  using facet_stage_type = candidate_source_facet_relation_stage<T>;
  using overlay_stage_type = candidate_coplanar_overlay_stage<T>;

  outcome_type failure() {
    relation_build_detail::bind_relation_error(
        error_, context_.context_digest, context_.replay_digest);
    return outcome_type::failure(error_);
  }

  bool fail(relation_subcode subcode,
            bounded_boolean_error_category category, const char *summary,
            relation_checkpoint checkpoint) {
    if (error_.subcode == 0)
      error_ = relation_error(subcode, category, summary, checkpoint);
    return false;
  }

  bool check_cancel(relation_checkpoint checkpoint) {
    return !relation_cancelled(capabilities_) ||
           fail(relation_subcode::cancelled,
                bounded_boolean_error_category::cancelled,
                "Component 07 construction cancelled", checkpoint);
  }

  bool validate_contracts() {
    if (!transaction_.open())
      return fail(relation_subcode::transaction_failure,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 transaction did not open",
                  relation_checkpoint::context_policy_capability_validation);
    if (!candidates_ || !verify_context(context_) ||
        !context_.owner.same_owner(capabilities_.owner) ||
        !precision_.owner().same_owner(capabilities_.owner) ||
        !candidates_->owner().same_owner(capabilities_.owner) ||
        precision_.boolean_context_digest() != context_.context_digest ||
        candidates_->precision_digest() != precision_.digest() ||
        candidates_->verification() !=
            broad_phase_verification_disposition::independently_verified ||
        !precision_.ordinary_success_eligible())
      return fail(relation_subcode::wrong_owner,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 context, precision, or candidate handshake failed",
                  relation_checkpoint::context_policy_capability_validation);
    if (capabilities_.provider_version != contract_versions::relation_provider ||
        capabilities_.graph_policy_version !=
            contract_versions::relation_graph_policy ||
        capabilities_.truth_policy_version !=
            contract_versions::relation_truth_policy ||
        capabilities_.codec_version != contract_versions::relation_codec ||
        capabilities_.verifier_version != contract_versions::relation_verifier ||
        capabilities_.reserved != 0)
      return fail(relation_subcode::unsupported_version,
                  bounded_boolean_error_category::input_contract_error,
                  "Component 07 capability version is unsupported",
                  relation_checkpoint::context_policy_capability_validation);
    if (!capabilities_.resources)
      return fail(relation_subcode::resource_preflight,
                  bounded_boolean_error_category::input_contract_error,
                  "Component 07 resource manager is required",
                  relation_checkpoint::discovery_resource_reservation);
    return check_cancel(
        relation_checkpoint::context_policy_capability_validation);
  }

  bool preflight_and_reserve() {
    if (!preflight_relation_foundation(*candidates_, capabilities_, preflight_,
                                       error_))
      return false;

    persistent_reservation_ = capabilities_.resources->reserve(
        resource_kind::persistent_bytes, preflight_.fixed_persistent_bytes);
    temporary_reservation_ = capabilities_.resources->reserve(
        resource_kind::temporary_bytes, preflight_.fixed_temporary_bytes);
    work_reservation_ = capabilities_.resources->reserve(
        resource_kind::work_units, preflight_.fixed_work_units);
    if (!persistent_reservation_ || !temporary_reservation_ ||
        !work_reservation_)
      return fail(relation_subcode::resource_preflight,
                  bounded_boolean_error_category::resource_limit,
                  "Component 07 resource reservation failed",
                  relation_checkpoint::discovery_resource_reservation);
    return transaction_.register_work() ||
           fail(relation_subcode::transaction_failure,
                bounded_boolean_error_category::internal_invariant_error,
                "Component 07 transaction could not register work",
                relation_checkpoint::discovery_resource_reservation);
  }

  bool build_candidate_edge_relations() {
    if (!check_cancel(relation_checkpoint::candidate_scan))
      return false;
    auto stage = build_candidate_source_edge_relations(
        *candidates_, context_.context_digest, precision_.tolerance(),
        capabilities_);
    if (!stage.has_value()) {
      error_ = *stage.error();
      return false;
    }
    edge_stage_.emplace(std::move(*stage.value()));
    return true;
  }

  bool build_candidate_edge_facet_relations() {
    if (!check_cancel(relation_checkpoint::edge_facet_evaluation))
      return false;
    if (!edge_stage_)
      return fail(relation_subcode::source_edge_facet_invariant,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 edge/facet stage is missing edge dependencies",
                  relation_checkpoint::edge_facet_evaluation);
    auto stage = build_candidate_source_edge_facet_relations(
        *candidates_, *edge_stage_, context_.context_digest,
        precision_.tolerance(), capabilities_);
    if (!stage.has_value()) {
      error_ = *stage.error();
      return false;
    }
    edge_facet_stage_.emplace(std::move(*stage.value()));
    return true;
  }

  bool build_candidate_facet_relations() {
    if (!check_cancel(relation_checkpoint::facet_facet_evaluation))
      return false;
    if (!edge_facet_stage_)
      return fail(relation_subcode::source_facet_relation_invariant,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 facet/facet stage is missing edge/facet consumers",
                  relation_checkpoint::facet_facet_evaluation);
    auto stage = build_candidate_source_facet_relations(
        *candidates_, *edge_facet_stage_, context_.context_digest,
        precision_.tolerance(), capabilities_);
    if (!stage.has_value()) {
      error_ = *stage.error();
      return false;
    }
    facet_stage_.emplace(std::move(*stage.value()));
    return true;
  }

  bool build_candidate_coplanar_overlays() {
    if (!check_cancel(relation_checkpoint::coplanar_overlay_evaluation))
      return false;
    if (!edge_stage_ || !facet_stage_)
      return fail(relation_subcode::coplanar_overlay_invariant,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 coplanar overlay stage is missing predecessor relations",
                  relation_checkpoint::coplanar_overlay_evaluation);
    auto stage = ygor::mesh_boolean::bounded::build_candidate_coplanar_overlays(
        *candidates_, *edge_stage_, *facet_stage_, capabilities_);
    if (!stage.has_value()) {
      error_ = *stage.error();
      return false;
    }
    overlay_stage_.emplace(std::move(*stage.value()));
    return true;
  }

  bool build_final_artifact() {
    if (!check_cancel(relation_checkpoint::initial_request_grouping))
      return false;
    if (!edge_stage_ || !edge_facet_stage_ || !facet_stage_ || !overlay_stage_)
      return fail(relation_subcode::internal_invariant,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 final assembly is missing a verified predecessor stage",
                  relation_checkpoint::predecessor_validation);

    auto edge = std::make_shared<const edge_stage_type>(
        std::move(*edge_stage_));
    auto edge_facet = std::make_shared<const edge_facet_stage_type>(
        std::move(*edge_facet_stage_));
    auto facet = std::make_shared<const facet_stage_type>(
        std::move(*facet_stage_));
    auto overlay = std::make_shared<const overlay_stage_type>(
        std::move(*overlay_stage_));
    artifact_ = std::make_unique<artifact_type>();
    relation_artifact_assembler<T, I> assembler(
        context_, precision_, candidates_, std::move(edge),
        std::move(edge_facet), std::move(facet), std::move(overlay),
        capabilities_);
    return assembler.assemble(*artifact_, error_);
  }

  bool encode_and_verify() {
    if (!check_cancel(relation_checkpoint::canonical_encoding))
      return false;
    artifact_->statistics_.persistent_bytes = 0;
    artifact_->canonical_bytes_ = encode_signed_feature_relations(*artifact_);
    if (artifact_->canonical_bytes_.size() >
        capabilities_.maximum_canonical_bytes)
      return fail(relation_subcode::resource_preflight,
                  bounded_boolean_error_category::resource_limit,
                  "Component 07 canonical bytes exceed the configured limit",
                  relation_checkpoint::canonical_encoding);

    std::uint64_t persistent = 0;
    if (!relation_build_detail::estimate_relation_persistent_bytes(*artifact_,
                                                                   persistent))
      return fail(relation_subcode::byte_count_overflow,
                  bounded_boolean_error_category::index_overflow,
                  "Component 07 persistent byte count overflow",
                  relation_checkpoint::resource_reconciliation);
    artifact_->statistics_.persistent_bytes = persistent;
    artifact_->canonical_bytes_ = encode_signed_feature_relations(*artifact_);
    artifact_->digest_ = sha256::digest(artifact_->canonical_bytes_);

    const std::uint64_t covered = persistent_reservation_->amount();
    const std::uint64_t extra = persistent > covered ? persistent - covered : 0;
    final_persistent_reservation_ = capabilities_.resources->reserve(
        resource_kind::persistent_bytes, extra);
    codec_reservation_ = capabilities_.resources->reserve(
        resource_kind::replay_bytes, artifact_->canonical_bytes_.size());
    if (!final_persistent_reservation_ || !codec_reservation_)
      return fail(relation_subcode::resource_preflight,
                  bounded_boolean_error_category::resource_limit,
                  "Component 07 final artifact reservation failed",
                  relation_checkpoint::resource_reconciliation);

    bounded_boolean_error verification_error;
    if (!verify_signed_feature_relations(*artifact_, verification_error)) {
      error_ = verification_error;
      return false;
    }

    std::uint64_t actual_work = artifact_->request_graph_.proposal_count;
    const auto add_work = [&](std::uint64_t value) {
      return checked_add<std::uint64_t>(actual_work, value, actual_work);
    };
    if (!add_work(artifact_->request_graph_.requests.size()) ||
        !add_work(artifact_->request_graph_.dependencies.size()) ||
        !add_work(artifact_->request_graph_.reverse_consumers.size()) ||
        !add_work(artifact_->request_graph_.candidate_witnesses.size()) ||
        !add_work(artifact_->imported_geometry_.size()) ||
        !add_work(artifact_->bounded_primitives_.size()) ||
        !add_work(artifact_->exact_relations_.size()) ||
        !add_work(artifact_->truth_lineage_.size()) ||
        !add_work(artifact_->truth_records_.size()) ||
        !add_work(artifact_->relations_.size()) ||
        !add_work(artifact_->constructions_.size()) ||
        !add_work(artifact_->symbolic_eligibility_.size()) ||
        !add_work(artifact_->symbolic_decisions_.size()) ||
        !add_work(artifact_->crossings_.size()) ||
        !add_work(artifact_->event_seeds_.size()) ||
        !add_work(artifact_->event_seed_incidence_.size()) ||
        !add_work(artifact_->candidate_dispositions_.size()) ||
        !add_work(artifact_->statistics_.verifier_work_units) ||
        actual_work > work_reservation_->amount())
      return fail(relation_subcode::resource_preflight,
                  bounded_boolean_error_category::resource_limit,
                  "Component 07 actual work exceeds reserved work",
                  relation_checkpoint::resource_reconciliation);
    persistent_used_ = persistent;
    replay_used_ = artifact_->canonical_bytes_.size();
    work_used_ = actual_work;
    return true;
  }

  bool commit_resources() {
    const auto fixed_used =
        std::min(persistent_reservation_->amount(), persistent_used_);
    const auto final_used = persistent_used_ - fixed_used;
    return persistent_reservation_->commit(fixed_used) &&
           final_persistent_reservation_->commit(final_used) &&
           codec_reservation_->commit(replay_used_) &&
           temporary_reservation_->commit(0) &&
           work_reservation_->commit(work_used_);
  }

  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  std::shared_ptr<const canonical_candidate_stream<T, I>> candidates_;
  relation_capabilities capabilities_;
  relation_preflight_plan preflight_{};
  std::optional<edge_stage_type> edge_stage_;
  std::optional<edge_facet_stage_type> edge_facet_stage_;
  std::optional<facet_stage_type> facet_stage_;
  std::optional<overlay_stage_type> overlay_stage_;
  std::unique_ptr<artifact_type> artifact_;
  stage_transaction transaction_;
  std::optional<resource_reservation> persistent_reservation_;
  std::optional<resource_reservation> final_persistent_reservation_;
  std::optional<resource_reservation> codec_reservation_;
  std::optional<resource_reservation> temporary_reservation_;
  std::optional<resource_reservation> work_reservation_;
  std::uint64_t persistent_used_ = 0;
  std::uint64_t replay_used_ = 0;
  std::uint64_t work_used_ = 0;
  bounded_boolean_error error_{};
};

template <class T, class I>
boolean_outcome<std::shared_ptr<const signed_feature_relations<T, I>>>
build_signed_feature_relations(
    const boolean_context<T, I> &context,
    const precision_context<T> &precision,
    std::shared_ptr<const canonical_candidate_stream<T, I>> candidates,
    relation_capabilities capabilities) {
  return relation_builder<T, I>(context, precision, std::move(candidates),
                                std::move(capabilities))
      .run();
}

template <class T, class I>
boolean_outcome<std::shared_ptr<const signed_feature_relations<T, I>>>
decode_signed_feature_relations(
    const std::vector<std::uint8_t> &bytes,
    const boolean_context<T, I> &context,
    const precision_context<T> &precision,
    std::shared_ptr<const canonical_candidate_stream<T, I>> candidates,
    relation_capabilities capabilities) {
  using outcome_type =
      boolean_outcome<std::shared_ptr<const signed_feature_relations<T, I>>>;
  relation_artifact_envelope<T> envelope;
  bounded_boolean_error error;
  if (!parse_relation_artifact_envelope(bytes, capabilities, envelope, error)) {
    relation_build_detail::bind_relation_error(
        error, context.context_digest, context.replay_digest);
    return outcome_type::failure(std::move(error));
  }
  const auto fail = [&](relation_subcode subcode, const char *summary) {
    auto failure = relation_error(
        subcode, bounded_boolean_error_category::input_contract_error, summary,
        relation_checkpoint::canonical_encoding);
    relation_build_detail::bind_relation_error(
        failure, context.context_digest, context.replay_digest);
    return outcome_type::failure(std::move(failure));
  };
  if (!candidates || !context.owner.same_owner(capabilities.owner) ||
      !precision.owner().same_owner(capabilities.owner) ||
      !candidates->owner().same_owner(capabilities.owner))
    return fail(relation_subcode::wrong_owner,
                "Component 07 decode owner handshake failed");
  if (envelope.context_digest != context.context_digest ||
      envelope.precision_digest != precision.digest() ||
      envelope.candidate_digest != candidates->candidate_digest() ||
      envelope.operation != context.operation ||
      to_bits(envelope.residual_boundary) != to_bits(precision.tolerance()) ||
      envelope.symbolic_policy_digest != context.symbolic.digest ||
      envelope.statistics.candidate_count != candidates->candidates().size())
    return fail(relation_subcode::predecessor_mismatch,
                "Component 07 encoded predecessor or policy handshake failed");
  for (const bool present : envelope.detailed_stage_present)
    if (!present)
      return fail(relation_subcode::codec_error,
                  "Component 07 encoded artifact omits a required detailed stage");

  auto rebuilt = build_signed_feature_relations(
      context, precision, std::move(candidates), std::move(capabilities));
  if (!rebuilt.has_value())
    return rebuilt;
  if ((*rebuilt.value())->canonical_bytes() != bytes ||
      (*rebuilt.value())->graph_digest() != envelope.graph_digest)
    return fail(relation_subcode::codec_error,
                "Component 07 encoded artifact is not canonical");
  return rebuilt;
}

} // namespace ygor::mesh_boolean::bounded
