#pragma once

#include "RelationSemanticProjection.h"

#include "CoplanarRelationOverlay.h"
#include "ContextVerifier.h"
#include "PrecisionContext.h"
#include "RelationPreflight.h"
#include "RelationPredecessorCommitments.h"
#include "RelationReplay.h"
#include "RelationArtifactAssembly.h"
#include "RelationExecutionAuthority.h"
#include "RelationVerifier.h"
#include "Transaction.h"

#include <cstdint>
#include <array>
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
  error.replay_digest = relation_failure_replay_digest(replay_digest, error);
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

template <class T, class I, class EdgeStage, class EdgeFacetStage,
          class FacetStage, class OverlayStage>
bool estimate_relation_persistent_bytes(
    const signed_feature_relations<T, I> &artifact,
    const EdgeStage &edge_stage, const EdgeFacetStage &edge_facet_stage,
    const FacetStage &facet_stage, const OverlayStage &overlay_stage,
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
      !add_vector(artifact.interval_evidence()) ||
      !add_vector(artifact.source_facet_regions()) ||
      !add_vector(artifact.truth_records()) ||
      !add_vector(artifact.relations()) ||
      !add_vector(artifact.constructions()) ||
      !add_vector(artifact.construction_ledger()) ||
      !add_vector(artifact.coplanar_event_nodes()) ||
      !add_vector(artifact.coplanar_oriented_arcs()) ||
      !add_vector(artifact.coplanar_overlap_components()) ||
      !add_vector(artifact.symbolic_eligibility()) ||
      !add_vector(artifact.symbolic_decisions()) ||
      !add_vector(artifact.crossings()) ||
      !add_vector(artifact.event_seeds()) ||
      !add_vector(artifact.event_seed_incidence()) ||
       !add_vector(artifact.event_seed_candidate_incidence()) ||
       !add_vector(artifact.triangle_local_reconciliation()) ||
       !add_vector(artifact.transverse_carrier_memberships()) ||
      !add_vector(artifact.candidate_dispositions()) ||
      !add_vector(artifact.candidate_relation_coverage()) ||
      !add_vector(artifact.candidate_event_seed_coverage()) ||
      !add_vector(artifact.candidate_partitions()) ||
      !add_vector(artifact.diagnostics()) ||
      !add_vector(artifact.resource_evidence()) ||
      !add_vector(artifact.replay_checkpoints()) ||
      !add_vector(artifact.canonical_bytes()))
    return false;

  for (const auto &record : artifact.source_facet_regions()) {
    if (!add_vector(record.region.source_vertex_owners) ||
        !add_vector(record.region.source_edge_owners) ||
        !add_vector(record.region.orientation_evidence))
      return false;
  }
  for (const auto &record : artifact.interval_evidence())
    if (!add_vector(record.issued_parent_values) ||
        !add_vector(record.issued_parent_trace_roots) ||
        !add_vector(record.issued_parent_ledger_entries) ||
        !add_vector(record.issued_operation_evidence))
      return false;
  for (const auto &record : artifact.constructions())
    if (!add_vector(record.ordered_bounded_inputs) ||
        !add_vector(record.certificate_evidence))
      return false;
  for (const auto &record : artifact.construction_ledger())
    if (!add_vector(record.ordered_bounded_inputs) ||
        !add_vector(record.certificate_evidence))
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
  for (const auto &topology : artifact.source_topology()) {
    if (!add_vector(topology.source_edges) || !add_vector(topology.vertex_fans) ||
        !add_vector(topology.edge_adjacencies))
      return false;
    for (const auto &fan : topology.vertex_fans)
      if (!add_vector(fan.ordered_facets))
        return false;
  }
  if (!add_vector(artifact.transverse_carrier_supports()))
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
  return add_stage(edge_stage,
                   [](const auto &stage) {
                     return encode_candidate_source_edge_relation_semantics(stage);
                   }) &&
         add_stage(edge_facet_stage,
                   [](const auto &stage) {
                     return encode_candidate_source_edge_facet_relation_semantics(stage);
                   }) &&
         add_stage(facet_stage,
                   [](const auto &stage) {
                     return encode_candidate_source_facet_relation_semantics(stage);
                   }) &&
         add_stage(overlay_stage,
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
          !build_execution_authority() || !reserve_closed_authority_domains() ||
          !build_source_vertex_facet_relations() ||
          !build_candidate_edge_relations() ||
          !build_candidate_edge_facet_relations() ||
          !build_candidate_facet_relations() ||
          !build_transverse_relations() ||
          !build_candidate_coplanar_overlays() ||
          !build_final_artifact() || !encode_and_verify())
        return failure();
      auto published =
          std::shared_ptr<const signed_feature_relations<T, I>>(
              std::move(artifact_));
      if (!check_cancel(relation_checkpoint::transaction_commit) ||
          !commit_resources() || !transaction_.begin_join() ||
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
  using vertex_facet_stage_type = source_vertex_facet_evaluated_stage<T>;
  using edge_facet_stage_type = candidate_source_edge_facet_relation_stage<T>;
  using facet_stage_type = candidate_source_facet_relation_stage<T>;
  using overlay_stage_type = candidate_coplanar_overlay_stage<T>;
  using transverse_stage_type = transverse_relation_evaluated_stage<T>;

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
    return !relation_cancelled(capabilities_, checkpoint) ||
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
    if (!candidates_ || !context_.owner.same_owner(capabilities_.owner) ||
        !precision_.owner().same_owner(capabilities_.owner) ||
        !candidates_->owner().same_owner(capabilities_.owner))
      return fail(relation_subcode::wrong_owner,
                  bounded_boolean_error_category::internal_invariant_error,
                   "Component 07 context, precision, or candidate handshake failed",
                   relation_checkpoint::context_policy_capability_validation);
    if (!validate_relation_predecessors_before_work(
            context_, precision_, *candidates_, predecessor_commitments_, error_))
      return false;
    if (capabilities_.provider_version != contract_versions::relation_provider ||
        capabilities_.graph_policy_version !=
            contract_versions::relation_graph_policy ||
        capabilities_.truth_policy_version !=
            contract_versions::relation_truth_policy ||
        capabilities_.codec_version != contract_versions::relation_codec ||
        capabilities_.verifier_version != contract_versions::relation_verifier ||
        capabilities_.reserved != 0 ||
        (capabilities_.cancellation_observer &&
         (capabilities_.cancellation_observer->version !=
              contract_versions::relation_cancellation_observer ||
          capabilities_.cancellation_observer->reserved16 != 0 ||
          capabilities_.cancellation_observer->reserved32 != 0 ||
          !capabilities_.cancellation_observer->poll)))
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
    if (!check_cancel(relation_checkpoint::count_representability_preflight) ||
        !preflight_relation_foundation(*candidates_, capabilities_, preflight_,
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
    const auto reserve_domain = [&](std::size_t index, resource_kind kind,
                                    std::uint64_t amount) {
      discovery_domain_reservations_[index] =
          capabilities_.resources->reserve(kind, amount);
      if (discovery_domain_reservations_[index])
        return true;
      const auto counters = capabilities_.resources->snapshot();
      error_ = relation_error(relation_subcode::resource_preflight,
                              bounded_boolean_error_category::resource_limit,
                              "Component 07 discovery resource domain limit exceeded",
                              relation_checkpoint::discovery_resource_reservation);
      error_.witnesses[0] = index + 1;
      error_.witnesses[1] = amount;
      error_.witnesses[2] = counters[static_cast<std::size_t>(kind)].hard;
      error_.witness_count = 3;
      return false;
    };
    if (!reserve_domain(0, resource_kind::relation_request_records,
                        preflight_.initial_request_upper_bound) ||
        !reserve_domain(3, resource_kind::relation_graph_edges,
                        preflight_.dependency_upper_bound) ||
        !reserve_domain(11, resource_kind::relation_disposition_records,
                        preflight_.disposition_upper_bound) ||
        !reserve_domain(13, resource_kind::relation_private_buffers,
                        preflight_.domains.private_buffers) ||
        !check_cancel(relation_checkpoint::discovery_resource_reservation))
      return false;
    return transaction_.register_work() ||
           fail(relation_subcode::transaction_failure,
                bounded_boolean_error_category::internal_invariant_error,
                "Component 07 transaction could not register work",
                relation_checkpoint::discovery_resource_reservation);
  }

  bool reserve_closed_authority_domains() {
    if (!execution_authority_)
      return fail(relation_subcode::internal_invariant,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 closed authority is absent",
                  relation_checkpoint::discovery_resource_reservation);
    const std::uint64_t exact_requests =
        execution_authority_->graph.requests.size();
    std::uint64_t exact_graph = 0;
    if (!checked_add<std::uint64_t>(
            execution_authority_->graph.dependencies.size(),
            execution_authority_->graph.reverse_consumers.size(), exact_graph) ||
        !checked_add<std::uint64_t>(
            exact_graph, execution_authority_->graph.candidate_witnesses.size(),
            exact_graph))
      return fail(relation_subcode::count_overflow,
                  bounded_boolean_error_category::index_overflow,
                  "Component 07 closed-authority resource count overflow",
                  relation_checkpoint::discovery_resource_reservation);
    if ((discovery_domain_reservations_[0] &&
         discovery_domain_reservations_[0]->amount() > exact_requests &&
         !discovery_domain_reservations_[0]->shrink(exact_requests)) ||
        (discovery_domain_reservations_[3] &&
         discovery_domain_reservations_[3]->amount() > exact_graph &&
         !discovery_domain_reservations_[3]->shrink(exact_graph)))
      return fail(relation_subcode::resource_preflight,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 closed-authority reservation tightening failed",
                  relation_checkpoint::discovery_resource_reservation);
    const std::array<resource_kind, 17> kinds{{
        resource_kind::relation_request_records,
        resource_kind::relation_primitive_records,
        resource_kind::relation_family_records,
        resource_kind::relation_graph_edges,
        resource_kind::relation_region_workspace,
        resource_kind::relation_numerical_workspace,
        resource_kind::relation_overlay_records,
        resource_kind::relation_construction_records,
        resource_kind::relation_crossing_records,
        resource_kind::relation_symbolic_records,
        resource_kind::relation_seed_records,
        resource_kind::relation_disposition_records,
        resource_kind::relation_canonical_workspace,
        resource_kind::relation_private_buffers,
        resource_kind::relation_codec_evidence,
        resource_kind::relation_verifier_evidence,
        resource_kind::relation_persistent_artifact}};
    const std::array<std::uint64_t, 17> amounts{{
        exact_requests, preflight_.domains.primitives,
        preflight_.domains.relation_families, exact_graph,
        preflight_.domains.regions, preflight_.domains.numerical_workspaces,
        preflight_.domains.overlays, preflight_.domains.constructions,
        preflight_.domains.crossings, preflight_.domains.symbolic,
        preflight_.domains.seeds, preflight_.domains.dispositions,
        preflight_.domains.canonical_merge, preflight_.domains.private_buffers,
        preflight_.fixed_persistent_bytes, preflight_.domains.verifier,
        preflight_.domains.persistent_artifact}};
    for (std::size_t i = 0; i < amounts.size(); ++i) {
      const std::uint64_t already = discovery_domain_reservations_[i]
                                        ? discovery_domain_reservations_[i]->amount()
                                        : 0;
      const std::uint64_t additional = amounts[i] > already ? amounts[i] - already : 0;
      closed_domain_reservations_[i] =
          capabilities_.resources->reserve(kinds[i], additional);
      if (!closed_domain_reservations_[i]) {
        const auto counters = capabilities_.resources->snapshot();
        error_ = relation_error(relation_subcode::resource_preflight,
                                bounded_boolean_error_category::resource_limit,
                                "Component 07 closed-authority resource domain limit exceeded",
                                relation_checkpoint::discovery_resource_reservation);
        error_.witnesses[0] = i + 1;
        error_.witnesses[1] = amounts[i];
        error_.witnesses[2] = counters[static_cast<std::size_t>(kinds[i])].hard;
        error_.witnesses[3] = preflight_.maximum_candidate_boundary_witness;
        error_.witness_count = 4;
        return false;
      }
    }
    return true;
  }

  bool build_candidate_edge_relations() {
    if (!check_cancel(relation_checkpoint::candidate_scan) ||
        !check_cancel(relation_checkpoint::edge_edge_evaluation))
      return false;
    auto stage = build_candidate_source_edge_relations(
        *candidates_, context_.context_digest, precision_.tolerance(),
        capabilities_);
    if (!stage.has_value()) {
      error_ = *stage.error();
      return false;
    }
    edge_stage_.emplace(std::move(*stage.value()));
    if (!execution_authorizes(*execution_authority_,
                              edge_stage_->request_graph))
      return fail(relation_subcode::unclosed_dependency,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 edge evaluation escaped its frozen authority",
                  relation_checkpoint::edge_edge_evaluation);
    return true;
  }

  bool build_source_vertex_facet_relations() {
    if (!execution_authority_ ||
        !check_cancel(relation_checkpoint::source_facet_region_evaluation))
      return false;
    auto stage = build_source_vertex_facet_evaluated_stage(
        *candidates_, *execution_authority_, context_.context_digest,
        capabilities_, precision_.tolerance());
    if (!stage.has_value()) {
      error_ = *stage.error();
      return false;
    }
    vertex_facet_stage_.emplace(std::move(*stage.value()));
    return true;
  }

  bool build_execution_authority() {
    if (!check_cancel(relation_checkpoint::candidate_scan) ||
        !check_cancel(relation_checkpoint::initial_request_grouping) ||
        !check_cancel(relation_checkpoint::dependency_closure) ||
        !check_cancel(relation_checkpoint::graph_finalization))
      return false;
    auto authority = ygor::mesh_boolean::bounded::build_relation_execution_authority(
        *candidates_, context_.context_digest, capabilities_);
    if (!authority.has_value()) {
      error_ = *authority.error();
      return false;
    }
    execution_authority_.emplace(std::move(*authority.value()));
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
    if (!vertex_facet_stage_)
      return fail(relation_subcode::source_edge_facet_invariant,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 edge/facet stage is missing vertex/facet authority",
                  relation_checkpoint::edge_facet_evaluation);
    auto stage = build_candidate_source_edge_facet_relations(
        *candidates_, *edge_stage_, context_.context_digest,
        precision_.tolerance(), capabilities_, &*vertex_facet_stage_);
    if (!stage.has_value()) {
      error_ = *stage.error();
      return false;
    }
    edge_facet_stage_.emplace(std::move(*stage.value()));
    if (!execution_authorizes(*execution_authority_,
                              edge_facet_stage_->request_graph))
      return fail(relation_subcode::unclosed_dependency,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 edge/facet evaluation escaped its frozen authority",
                  relation_checkpoint::edge_facet_evaluation);
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
    if (!execution_authorizes(*execution_authority_,
                              facet_stage_->request_graph))
      return fail(relation_subcode::unclosed_dependency,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 facet evaluation escaped its frozen authority",
                  relation_checkpoint::facet_facet_evaluation);
    return true;
  }

  bool build_candidate_coplanar_overlays() {
    if (!check_cancel(relation_checkpoint::coplanar_overlay_evaluation))
      return false;
    if (!vertex_facet_stage_ || !edge_stage_ || !facet_stage_)
      return fail(relation_subcode::coplanar_overlay_invariant,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 coplanar overlay stage is missing predecessor relations",
                  relation_checkpoint::coplanar_overlay_evaluation);
    auto stage = ygor::mesh_boolean::bounded::build_candidate_coplanar_overlays(
        *candidates_, *edge_stage_, *facet_stage_, capabilities_,
        &*vertex_facet_stage_, &context_.context_digest);
    if (!stage.has_value()) {
      error_ = *stage.error();
      return false;
    }
    overlay_stage_.emplace(std::move(*stage.value()));
    return true;
  }

  bool build_transverse_relations() {
    if (!check_cancel(relation_checkpoint::construction_validation))
      return false;
    if (!execution_authority_ || !edge_facet_stage_ || !facet_stage_)
      return fail(relation_subcode::internal_invariant,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 transverse evaluation is missing predecessor relations",
                  relation_checkpoint::predecessor_validation);
    auto stage = build_transverse_relation_evaluated_stage(
        *candidates_, *execution_authority_, *edge_facet_stage_, *facet_stage_,
        capabilities_, precision_.tolerance());
    if (!stage.has_value()) {
      error_ = *stage.error();
      return false;
    }
    transverse_stage_.emplace(std::move(*stage.value()));
    return true;
  }

  bool build_final_artifact() {
    if (!check_cancel(relation_checkpoint::initial_request_grouping))
      return false;
    if (!execution_authority_ || !vertex_facet_stage_ || !edge_stage_ || !edge_facet_stage_ ||
        !facet_stage_ || !overlay_stage_ || !transverse_stage_)
      return fail(relation_subcode::internal_invariant,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 final assembly is missing a verified predecessor stage",
                  relation_checkpoint::predecessor_validation);

    auto edge = std::make_shared<const edge_stage_type>(
        std::move(*edge_stage_));
    auto vertex_facet = std::make_shared<const vertex_facet_stage_type>(
        std::move(*vertex_facet_stage_));
    auto edge_facet = std::make_shared<const edge_facet_stage_type>(
        std::move(*edge_facet_stage_));
    auto facet = std::make_shared<const facet_stage_type>(
        std::move(*facet_stage_));
    auto overlay = std::make_shared<const overlay_stage_type>(
        std::move(*overlay_stage_));
    auto transverse = std::make_shared<const transverse_stage_type>(
        std::move(*transverse_stage_));
    artifact_ = std::make_unique<artifact_type>();
    relation_artifact_assembler<T, I> assembler(
        context_, precision_, candidates_, std::move(vertex_facet), std::move(edge),
        std::move(edge_facet), std::move(facet), std::move(overlay),
        std::move(transverse),
        std::make_shared<const relation_execution_authority>(
            std::move(*execution_authority_)), capabilities_);
    if (!assembler.assemble(*artifact_, error_))
      return false;
    return populate_resource_evidence();
  }

  bool populate_resource_evidence() {
    std::array<std::uint64_t, 17> used{};
    const auto add = [](std::uint64_t value, std::uint64_t &target) {
      return checked_add(target, value, target);
    };
    used[0] = artifact_->execution_authority_.graph.requests.size();
    if (!add(artifact_->imported_geometry_.size(), used[1]) ||
        !add(artifact_->bounded_primitives_.size(), used[1]) ||
        !add(artifact_->exact_relations_.size(), used[1]) ||
        !add(artifact_->truth_lineage_.size(), used[1]) ||
        !add(artifact_->relations_.size(), used[2]) ||
        !add(artifact_->execution_authority_.graph.dependencies.size(), used[3]) ||
        !add(artifact_->execution_authority_.graph.reverse_consumers.size(), used[3]) ||
        !add(artifact_->execution_authority_.graph.candidate_witnesses.size(), used[3]) ||
        !add(artifact_->interval_evidence_.size(), used[4]) ||
        !add(artifact_->source_facet_regions_.size(), used[4]) ||
        !add(artifact_->truth_records_.size(), used[5]) ||
        !add(artifact_->coplanar_event_nodes_.size(), used[6]) ||
        !add(artifact_->coplanar_oriented_arcs_.size(), used[6]) ||
        !add(artifact_->coplanar_overlap_components_.size(), used[6]) ||
        !add(artifact_->constructions_.size(), used[7]) ||
        !add(artifact_->construction_ledger_.size(), used[7]) ||
        !add(artifact_->crossings_.size(), used[8]) ||
        !add(artifact_->symbolic_eligibility_.size(), used[9]) ||
        !add(artifact_->symbolic_decisions_.size(), used[9]) ||
        !add(artifact_->event_seeds_.size(), used[10]) ||
        !add(artifact_->event_seed_incidence_.size(), used[10]) ||
        !add(artifact_->event_seed_candidate_incidence_.size(), used[10]) ||
        !add(artifact_->candidate_dispositions_.size(), used[11]) ||
        !add(artifact_->candidate_relation_coverage_.size(), used[11]) ||
        !add(artifact_->candidate_event_seed_coverage_.size(), used[11]))
      return fail(relation_subcode::count_overflow,
                  bounded_boolean_error_category::index_overflow,
                  "Component 07 resource evidence count overflow",
                  relation_checkpoint::resource_reconciliation);
    used[12] = artifact_->request_graph_.requests.size() +
               artifact_->request_graph_.dependencies.size();
    used[13] = 0;
    used[14] = artifact_->diagnostics_.size() + artifact_->replay_checkpoints_.size();
    used[15] = artifact_->statistics_.verifier_work_units;

    const std::array<relation_resource_domain, 17> domains{{
        relation_resource_domain::requests,
        relation_resource_domain::primitives,
        relation_resource_domain::relation_families,
        relation_resource_domain::graph, relation_resource_domain::regions,
        relation_resource_domain::numerical_workspaces,
        relation_resource_domain::overlays,
        relation_resource_domain::constructions,
        relation_resource_domain::crossings,
        relation_resource_domain::symbolic, relation_resource_domain::seeds,
        relation_resource_domain::dispositions,
        relation_resource_domain::canonical_merge,
        relation_resource_domain::private_buffers,
        relation_resource_domain::codec_replay_diagnostics,
        relation_resource_domain::verifier,
        relation_resource_domain::persistent_artifact}};
    const std::array<resource_kind, 17> kinds{{
        resource_kind::relation_request_records,
        resource_kind::relation_primitive_records,
        resource_kind::relation_family_records,
        resource_kind::relation_graph_edges,
        resource_kind::relation_region_workspace,
        resource_kind::relation_numerical_workspace,
        resource_kind::relation_overlay_records,
        resource_kind::relation_construction_records,
        resource_kind::relation_crossing_records,
        resource_kind::relation_symbolic_records,
        resource_kind::relation_seed_records,
        resource_kind::relation_disposition_records,
        resource_kind::relation_canonical_workspace,
        resource_kind::relation_private_buffers,
        resource_kind::relation_codec_evidence,
        resource_kind::relation_verifier_evidence,
        resource_kind::relation_persistent_artifact}};
    const std::array<std::uint64_t, 17> bounds{{
        preflight_.domains.requests, preflight_.domains.primitives,
        preflight_.domains.relation_families, preflight_.domains.graph,
        preflight_.domains.regions, preflight_.domains.numerical_workspaces,
        preflight_.domains.overlays, preflight_.domains.constructions,
        preflight_.domains.crossings, preflight_.domains.symbolic,
        preflight_.domains.seeds, preflight_.domains.dispositions,
        preflight_.domains.canonical_merge, preflight_.domains.private_buffers,
        preflight_.fixed_persistent_bytes, preflight_.domains.verifier,
        preflight_.domains.persistent_artifact}};
    artifact_->resource_evidence_.clear();
    artifact_->resource_evidence_.reserve(domains.size());
    for (std::size_t i = 0; i < domains.size(); ++i) {
      relation_resource_evidence_record record;
      record.id = relation_resource_evidence_id(i);
      record.domain = domains[i];
      record.resource = kinds[i];
      record.required_limit = bounds[i];
      record.preflight_reserved = bounds[i];
      record.closed_authority_bound = (i == 0 || i == 3) ? used[i] : bounds[i];
      record.reconciled_used = used[i];
      record.witness_ordinal =
          i == 6 ? preflight_.maximum_candidate_boundary_witness
                 : relation_invalid_ordinal;
      record.closed_authority_exact = i == 0 || i == 3;
      artifact_->resource_evidence_.push_back(record);
    }
    domain_used_ = used;
    return true;
  }

  bool encode_and_verify() {
    if (!check_cancel(relation_checkpoint::canonical_encoding))
      return false;
    artifact_->statistics_.persistent_bytes = 0;
    if (!refresh_relation_section_digests(*artifact_))
      return fail(relation_subcode::digest_mismatch,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 section digest construction failed",
                  relation_checkpoint::canonical_encoding);
    if (!build_relation_replay_bundle(*artifact_, capabilities_, error_))
      return false;
    if (!refresh_relation_section_digests(*artifact_))
      return fail(relation_subcode::digest_mismatch,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 section digest reconstruction failed",
                  relation_checkpoint::canonical_encoding);
    artifact_->canonical_bytes_ = encode_signed_feature_relations(*artifact_);
    if (artifact_->canonical_bytes_.size() >
        capabilities_.maximum_canonical_bytes)
      return fail(relation_subcode::resource_preflight,
                  bounded_boolean_error_category::resource_limit,
                  "Component 07 canonical bytes exceed the configured limit",
                   relation_checkpoint::canonical_encoding);
    if (artifact_->resource_evidence_.size() == 17) {
      std::uint64_t codec_evidence = artifact_->canonical_bytes_.size();
      if (!checked_add(codec_evidence,
                       encode_relation_diagnostic_semantics(
                           artifact_->diagnostics_).size(),
                       codec_evidence) ||
          !checked_add(codec_evidence,
                       encode_relation_replay_checkpoint_semantics(
                           artifact_->replay_checkpoints_).size(),
                       codec_evidence))
        return fail(relation_subcode::byte_count_overflow,
                    bounded_boolean_error_category::index_overflow,
                    "Component 07 codec evidence byte count overflow",
                    relation_checkpoint::resource_reconciliation);
      artifact_->resource_evidence_[14].reconciled_used = codec_evidence;
      domain_used_[14] = codec_evidence;
    }

    std::uint64_t persistent = 0;
    if (!relation_build_detail::estimate_relation_persistent_bytes(
            *artifact_, edge_stage_, edge_facet_stage_, facet_stage_,
            overlay_stage_, persistent))
      return fail(relation_subcode::byte_count_overflow,
                  bounded_boolean_error_category::index_overflow,
                  "Component 07 persistent byte count overflow",
                  relation_checkpoint::resource_reconciliation);
    artifact_->statistics_.persistent_bytes = persistent;
    if (!artifact_->resource_evidence_.empty()) {
      artifact_->resource_evidence_.back().reconciled_used = persistent;
      artifact_->resource_evidence_.back().closed_authority_bound = persistent;
      artifact_->resource_evidence_.back().closed_authority_exact = true;
      domain_used_[16] = persistent;
    }
    if (!refresh_relation_section_digests(*artifact_))
      return fail(relation_subcode::digest_mismatch,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 reconciled section digest construction failed",
                  relation_checkpoint::canonical_encoding);
    // Rebuild after final-size reconciliation. Record sizes are fixed, so this
    // changes only canonical evidence values and cannot change persistent use.
    if (!build_relation_replay_bundle(*artifact_, capabilities_, error_))
      return false;
    if (!refresh_relation_section_digests(*artifact_))
      return fail(relation_subcode::digest_mismatch,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 final section digest construction failed",
                  relation_checkpoint::canonical_encoding);
    std::uint64_t reconciled_persistent = 0;
    if (!relation_build_detail::estimate_relation_persistent_bytes(
            *artifact_, edge_stage_, edge_facet_stage_, facet_stage_,
            overlay_stage_, reconciled_persistent) ||
        reconciled_persistent != persistent)
      return fail(relation_subcode::internal_invariant,
                  bounded_boolean_error_category::internal_invariant_error,
                  "Component 07 replay evidence changed persistent size",
                  relation_checkpoint::resource_reconciliation);
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

    if (!check_cancel(relation_checkpoint::independent_verification))
      return false;
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
        !add_work(artifact_->interval_evidence_.size()) ||
        !add_work(artifact_->source_facet_regions_.size()) ||
        !add_work(artifact_->truth_records_.size()) ||
        !add_work(artifact_->relations_.size()) ||
        !add_work(artifact_->constructions_.size()) ||
         !add_work(artifact_->construction_ledger_.size()) ||
         !add_work(artifact_->transverse_carrier_memberships_.size()) ||
        !add_work(artifact_->symbolic_eligibility_.size()) ||
        !add_work(artifact_->symbolic_decisions_.size()) ||
        !add_work(artifact_->crossings_.size()) ||
        !add_work(artifact_->event_seeds_.size()) ||
        !add_work(artifact_->event_seed_incidence_.size()) ||
         !add_work(artifact_->event_seed_candidate_incidence_.size()) ||
         !add_work(artifact_->triangle_local_reconciliation_.size()) ||
        !add_work(artifact_->candidate_dispositions_.size()) ||
        !add_work(artifact_->candidate_relation_coverage_.size()) ||
        !add_work(artifact_->candidate_event_seed_coverage_.size()) ||
        !add_work(artifact_->candidate_partitions_.size()) ||
        !add_work(artifact_->diagnostics_.size()) ||
        !add_work(artifact_->replay_checkpoints_.size()) ||
        !add_work(artifact_->statistics_.verifier_work_units) ||
        actual_work > work_reservation_->amount())
      return fail(relation_subcode::resource_preflight,
                  bounded_boolean_error_category::resource_limit,
                  "Component 07 actual work exceeds reserved work",
                  relation_checkpoint::resource_reconciliation);
    if (!check_cancel(relation_checkpoint::resource_reconciliation))
      return false;
    persistent_used_ = persistent;
    replay_used_ = artifact_->canonical_bytes_.size();
    work_used_ = actual_work;
    return true;
  }

  bool commit_resources() {
    const auto fixed_used =
        std::min(persistent_reservation_->amount(), persistent_used_);
    const auto final_used = persistent_used_ - fixed_used;
    if (!(persistent_reservation_->commit(fixed_used) &&
           final_persistent_reservation_->commit(final_used) &&
           codec_reservation_->commit(replay_used_) &&
           temporary_reservation_->commit(0) &&
            work_reservation_->commit(work_used_)))
      return false;
    for (std::size_t i = 0; i < domain_used_.size(); ++i) {
      const auto first_amount = discovery_domain_reservations_[i]
                                    ? discovery_domain_reservations_[i]->amount()
                                    : 0;
      const auto first_used = std::min(first_amount, domain_used_[i]);
      const auto second_used = domain_used_[i] - first_used;
      if ((discovery_domain_reservations_[i] &&
           !discovery_domain_reservations_[i]->commit(first_used)) ||
          (closed_domain_reservations_[i] &&
           !closed_domain_reservations_[i]->commit(second_used)))
        return false;
    }
    return true;
  }

  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  std::shared_ptr<const canonical_candidate_stream<T, I>> candidates_;
  relation_capabilities capabilities_;
  relation_preflight_plan preflight_{};
  std::array<relation_predecessor_commitment_record, 6>
      predecessor_commitments_{};
  std::optional<edge_stage_type> edge_stage_;
  std::optional<vertex_facet_stage_type> vertex_facet_stage_;
  std::optional<edge_facet_stage_type> edge_facet_stage_;
  std::optional<facet_stage_type> facet_stage_;
  std::optional<overlay_stage_type> overlay_stage_;
  std::optional<transverse_stage_type> transverse_stage_;
  std::optional<relation_execution_authority> execution_authority_;
  std::unique_ptr<artifact_type> artifact_;
  stage_transaction transaction_;
  std::optional<resource_reservation> persistent_reservation_;
  std::optional<resource_reservation> final_persistent_reservation_;
  std::optional<resource_reservation> codec_reservation_;
  std::optional<resource_reservation> temporary_reservation_;
  std::optional<resource_reservation> work_reservation_;
  std::array<std::optional<resource_reservation>, 17>
      discovery_domain_reservations_{};
  std::array<std::optional<resource_reservation>, 17>
      closed_domain_reservations_{};
  std::array<std::uint64_t, 17> domain_used_{};
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
  std::array<relation_predecessor_commitment_record, 6>
      expected_commitments{};
  bounded_boolean_error predecessor_error;
  if (!validate_relation_predecessors_before_work(
          context, precision, *candidates, expected_commitments,
          predecessor_error)) {
    relation_build_detail::bind_relation_error(
        predecessor_error, context.context_digest, context.replay_digest);
    return outcome_type::failure(std::move(predecessor_error));
  }
  for (std::size_t i = 0; i < expected_commitments.size(); ++i)
    if (!same_relation_predecessor_commitment(
            envelope.predecessor_commitments[i], expected_commitments[i]))
      return fail(relation_subcode::predecessor_mismatch,
                  "Component 07 decoded predecessor commitment mismatch");
  if (envelope.context_digest != context.context_digest ||
      envelope.precision_digest !=
          relation_precision_semantic_digest(precision) ||
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
      (*rebuilt.value())->graph_digest() != envelope.graph_digest ||
      (*rebuilt.value())->section_digests() != envelope.section_digests)
    return fail(relation_subcode::codec_error,
                "Component 07 encoded artifact is not canonical");
  return rebuilt;
}

} // namespace ygor::mesh_boolean::bounded
