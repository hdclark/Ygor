#pragma once

#include "CandidateSourceEdgeRelations.h"
#include "ContextVerifier.h"
#include "PrecisionContext.h"
#include "RelationPreflight.h"
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
          !build_candidate_edge_relations() || !require_remaining_families() ||
          !build_foundation_artifact() || !encode_and_verify())
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

    std::uint64_t persistent_reserve = 0;
    if (!checked_add<std::uint64_t>(
            static_cast<std::uint64_t>(sizeof(artifact_type)),
            static_cast<std::uint64_t>(4096), persistent_reserve))
      return fail(relation_subcode::byte_count_overflow,
                  bounded_boolean_error_category::index_overflow,
                  "Component 07 persistent reservation overflow",
                  relation_checkpoint::count_representability_preflight);
    persistent_reservation_ = capabilities_.resources->reserve(
        resource_kind::persistent_bytes, persistent_reserve);
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

  bool require_remaining_families() {
    if (preflight_.candidate_count == 0)
      return true;
    return fail(
        relation_subcode::unsupported_relation_kernel,
        bounded_boolean_error_category::result_geometry_not_validated,
        "Component 07 candidate-derived source-edge/source-edge relations are verified; source-edge/source-facet and later relation families are not yet implemented",
        relation_checkpoint::edge_facet_evaluation);
  }

  bool build_foundation_artifact() {
    if (!check_cancel(relation_checkpoint::initial_request_grouping))
      return false;
    auto graph = build_relation_request_graph({}, capabilities_);
    if (!graph.has_value()) {
      error_ = *graph.error();
      return false;
    }
    artifact_ = std::make_unique<artifact_type>();
    artifact_->owner_ = capabilities_.owner;
    artifact_->candidates_ = candidates_;
    artifact_->request_graph_ = std::move(*graph.value());
    artifact_->context_digest_ = context_.context_digest;
    artifact_->precision_digest_ = precision_.digest();
    artifact_->candidate_digest_ = candidates_->candidate_digest();
    artifact_->graph_digest_ = artifact_->request_graph_.semantic_digest;
    artifact_->statistics_.candidate_count = 0;
    artifact_->statistics_.request_proposal_count = 0;
    artifact_->statistics_.unique_request_count = 0;
    artifact_->statistics_.dependency_count = 0;
    artifact_->statistics_.reverse_consumer_count = 0;
    artifact_->statistics_.candidate_witness_count = 0;
    artifact_->verification_evidence_.id = relation_verifier_evidence_id(0);
    artifact_->verification_evidence_.verifier_version =
        contract_versions::relation_verifier;
    artifact_->verification_evidence_.graph_reconstructed = true;
    artifact_->verification_evidence_.owner_exclusion_checked = true;
    artifact_->verification_evidence_.selection_boundary_checked = true;
    artifact_->verification_evidence_.candidate_dispositions_complete = true;
    artifact_->verification_evidence_.verifier_work_units = 1;
    artifact_->verification_evidence_.semantic_digest = artifact_->graph_digest_;
    artifact_->statistics_.verifier_work_units = 1;
    artifact_->verification_ =
        relation_verification_disposition::independently_verified;
    return true;
  }

  bool encode_and_verify() {
    if (!check_cancel(relation_checkpoint::canonical_encoding))
      return false;
    artifact_->canonical_bytes_ = encode_signed_feature_relations(*artifact_);
    std::uint64_t used = 0;
    if (!checked_add<std::uint64_t>(
            static_cast<std::uint64_t>(sizeof(artifact_type)),
            static_cast<std::uint64_t>(artifact_->canonical_bytes_.size()),
            used) ||
        used > capabilities_.maximum_canonical_bytes ||
        used > persistent_reservation_->amount())
      return fail(relation_subcode::byte_count_overflow,
                  bounded_boolean_error_category::resource_limit,
                  "Component 07 canonical artifact exceeds reserved bytes",
                  relation_checkpoint::canonical_encoding);
    artifact_->statistics_.persistent_bytes = used;
    artifact_->canonical_bytes_ = encode_signed_feature_relations(*artifact_);
    artifact_->digest_ = sha256::digest(artifact_->canonical_bytes_);
    bounded_boolean_error verification_error;
    if (!verify_signed_feature_relations(*artifact_, verification_error)) {
      error_ = verification_error;
      return false;
    }
    persistent_used_ = used;
    return true;
  }

  bool commit_resources() {
    return persistent_reservation_->commit(persistent_used_) &&
           temporary_reservation_->commit(0) && work_reservation_->commit(0);
  }

  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  std::shared_ptr<const canonical_candidate_stream<T, I>> candidates_;
  relation_capabilities capabilities_;
  relation_preflight_plan preflight_{};
  std::optional<edge_stage_type> edge_stage_;
  std::unique_ptr<artifact_type> artifact_;
  stage_transaction transaction_;
  std::optional<resource_reservation> persistent_reservation_;
  std::optional<resource_reservation> temporary_reservation_;
  std::optional<resource_reservation> work_reservation_;
  std::uint64_t persistent_used_ = 0;
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

} // namespace ygor::mesh_boolean::bounded
