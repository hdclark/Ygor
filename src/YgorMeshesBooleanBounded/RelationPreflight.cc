#include "StrictFloatingBuild.h"
#include "RelationPreflight.h"

#include <algorithm>
#include <limits>

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
bool preflight_relation_foundation(
    const canonical_candidate_stream<T, I> &candidates,
    const relation_capabilities &capabilities, relation_preflight_plan &plan,
    bounded_boolean_error &error) {
  plan = {};
  plan.candidate_count = candidates.candidates().size();
  const auto &manifolds = candidates.manifolds();
  if (!manifolds || !manifolds->a() || !manifolds->b() ||
      !manifolds->owner().same_owner(candidates.owner())) {
    error = relation_error(
        relation_subcode::source_edge_facet_malformed,
        bounded_boolean_error_category::internal_invariant_error,
        "Component 07 preflight source-manifold handshake failed",
        relation_checkpoint::count_representability_preflight);
    return false;
  }

  std::uint64_t maximum_facet_boundary = 0;
  const auto include_operand = [&](const auto &operand) {
    for (const auto &facet : operand->facet_groups()) {
      if (facet.source_vertices.size() < 3 ||
          facet.boundary_halfedges.size() != facet.source_vertices.size())
        return false;
      maximum_facet_boundary = std::max(
          maximum_facet_boundary,
          static_cast<std::uint64_t>(facet.boundary_halfedges.size()));
    }
    return true;
  };
  if (!include_operand(manifolds->a()) || !include_operand(manifolds->b()) ||
      (plan.candidate_count != 0 && maximum_facet_boundary < 3)) {
    error = relation_error(
        relation_subcode::source_edge_facet_malformed,
        bounded_boolean_error_category::internal_invariant_error,
        "Component 07 preflight found an incomplete source-facet boundary",
        relation_checkpoint::count_representability_preflight);
    return false;
  }

  std::uint64_t boundary_pair_requests = 0;
  std::uint64_t initial_requests_per_candidate = 0;
  // Each original candidate edge has two incident source facets. Closing a
  // candidate-derived facet pair requires every original boundary edge of an
  // incident facet against every original boundary edge of the opposite facet.
  // The remaining three requests cover one edge/facet composite and the two
  // incident facet/facet support requests.
  if (!checked_multiply<std::uint64_t>(maximum_facet_boundary,
                                       maximum_facet_boundary,
                                       boundary_pair_requests) ||
      !checked_multiply<std::uint64_t>(boundary_pair_requests,
                                       std::uint64_t{2},
                                       boundary_pair_requests) ||
      !checked_add<std::uint64_t>(boundary_pair_requests, std::uint64_t{3},
                                  initial_requests_per_candidate) ||
      !checked_multiply<std::uint64_t>(plan.candidate_count,
                                       initial_requests_per_candidate,
                                       plan.initial_request_upper_bound)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 candidate-derived request count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }
  plan.relation_upper_bound = plan.initial_request_upper_bound;

  // A simple source polygon has at most O(m^2) coplanar event nodes under the
  // frozen pairwise-boundary policy. The following closed formulas cover every
  // final-family producer without relying on observed output size:
  //   * two point constructions per edge/edge relation;
  //   * one event construction per possible boundary-pair event plus carriers;
  //   * two symbolic requests per lower-dimensional occurrence;
  //   * one event seed per point occurrence; and
  //   * one disposition per candidate.
  std::uint64_t event_occurrences_per_candidate = 0;
  std::uint64_t linear_events = 0;
  if (!checked_multiply<std::uint64_t>(maximum_facet_boundary,
                                       std::uint64_t{4}, linear_events) ||
      !checked_add<std::uint64_t>(boundary_pair_requests, linear_events,
                                  event_occurrences_per_candidate) ||
      !checked_add<std::uint64_t>(event_occurrences_per_candidate,
                                  std::uint64_t{8},
                                  event_occurrences_per_candidate) ||
      !checked_multiply<std::uint64_t>(plan.candidate_count,
                                       event_occurrences_per_candidate,
                                       plan.construction_upper_bound) ||
      !checked_multiply<std::uint64_t>(plan.construction_upper_bound,
                                       std::uint64_t{2},
                                       plan.construction_ledger_upper_bound) ||
      !checked_multiply<std::uint64_t>(plan.construction_upper_bound,
                                       std::uint64_t{2},
                                       plan.symbolic_upper_bound)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 derived relation count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }
  plan.event_seed_upper_bound = plan.construction_upper_bound;
  plan.disposition_upper_bound = plan.candidate_count;

  // Family 04 publishes every accepted parameter, residual/conditioning
  // interval, complete point/facet region classification, and every
  // partition breakpoint/interval witness. Pairwise source-boundary closure
  // dominates these tables. The coefficients deliberately cover both
  // directed incident facets, all three residual axes, complete polygon
  // traversal evidence, and triangle-local reconciliation witnesses.
  std::uint64_t family04_pair_evidence = 0;
  std::uint64_t family04_linear_evidence = 0;
  std::uint64_t family04_per_candidate = 0;
  if (!checked_multiply<std::uint64_t>(boundary_pair_requests,
                                       std::uint64_t{64},
                                       family04_pair_evidence) ||
      !checked_multiply<std::uint64_t>(maximum_facet_boundary,
                                       std::uint64_t{128},
                                       family04_linear_evidence) ||
      !checked_add<std::uint64_t>(family04_pair_evidence,
                                  family04_linear_evidence,
                                  family04_per_candidate) ||
      !checked_add<std::uint64_t>(family04_per_candidate,
                                  std::uint64_t{256},
                                  family04_per_candidate) ||
      !checked_multiply<std::uint64_t>(plan.candidate_count,
                                       family04_per_candidate,
                                       plan.interval_evidence_upper_bound) ||
      !checked_multiply<std::uint64_t>(plan.candidate_count,
                                       family04_per_candidate,
                                       plan.region_record_upper_bound)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 family-04 evidence count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }

  std::uint64_t multiplicity_upper_bound = plan.candidate_count;
  std::uint64_t primitive_support_upper_bound = 0;
  // Each authoritative base relation contributes at most two imported-source
  // requests, three bounded primitive producers, and three exact producers.
  // Canonical grouping only reduces this deliberately proposal-safe bound.
  if (!checked_multiply<std::uint64_t>(plan.initial_request_upper_bound,
                                       std::uint64_t{8},
                                       primitive_support_upper_bound) ||
      !checked_add<std::uint64_t>(plan.initial_request_upper_bound,
                                  primitive_support_upper_bound,
                                  plan.request_upper_bound) ||
      !checked_add<std::uint64_t>(plan.request_upper_bound,
                                  plan.interval_evidence_upper_bound,
                                  plan.request_upper_bound) ||
      !checked_add<std::uint64_t>(plan.request_upper_bound,
                                  plan.region_record_upper_bound,
                                  plan.request_upper_bound) ||
      !checked_add<std::uint64_t>(plan.request_upper_bound,
                                  plan.construction_upper_bound,
                                  plan.request_upper_bound) ||
      !checked_add<std::uint64_t>(plan.request_upper_bound,
                                  multiplicity_upper_bound,
                                  plan.request_upper_bound) ||
      !checked_add<std::uint64_t>(plan.request_upper_bound,
                                  plan.symbolic_upper_bound,
                                  plan.request_upper_bound) ||
      !checked_add<std::uint64_t>(plan.request_upper_bound,
                                  plan.event_seed_upper_bound,
                                  plan.request_upper_bound) ||
      !checked_add<std::uint64_t>(plan.request_upper_bound,
                                  plan.disposition_upper_bound,
                                  plan.request_upper_bound)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 final request count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }

  std::uint64_t initial_dependencies_per_candidate = 0;
  std::uint64_t initial_dependency_upper_bound = 0;
  std::uint64_t derived_dependency_upper_bound = 0;
  if (!checked_add<std::uint64_t>(maximum_facet_boundary,
                                  boundary_pair_requests,
                                  initial_dependencies_per_candidate) ||
      !checked_multiply<std::uint64_t>(plan.candidate_count,
                                       initial_dependencies_per_candidate,
                                       initial_dependency_upper_bound) ||
      !checked_multiply<std::uint64_t>(
          plan.request_upper_bound - plan.initial_request_upper_bound,
          std::uint64_t{4}, derived_dependency_upper_bound) ||
      !checked_add<std::uint64_t>(initial_dependency_upper_bound,
                                  derived_dependency_upper_bound,
                                  plan.dependency_upper_bound) ||
      !checked_add<std::uint64_t>(plan.dependency_upper_bound,
                                  plan.initial_request_upper_bound,
                                  plan.dependency_upper_bound) ||
      !checked_multiply<std::uint64_t>(plan.initial_request_upper_bound,
                                       std::uint64_t{18},
                                       primitive_support_upper_bound) ||
      !checked_add<std::uint64_t>(plan.dependency_upper_bound,
                                  primitive_support_upper_bound,
                                  plan.dependency_upper_bound) ||
      !checked_add<std::uint64_t>(plan.interval_evidence_upper_bound,
                                  plan.region_record_upper_bound,
                                  primitive_support_upper_bound) ||
      !checked_multiply<std::uint64_t>(primitive_support_upper_bound,
                                       std::uint64_t{10},
                                       primitive_support_upper_bound) ||
      !checked_add<std::uint64_t>(plan.dependency_upper_bound,
                                  primitive_support_upper_bound,
                                  plan.dependency_upper_bound)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 dependency count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }
  // Every proposal is candidate-derived. Canonical grouping can only reduce the
  // number of distinct candidate witnesses, never increase it.
  plan.witness_upper_bound = plan.request_upper_bound;
  plan.event_seed_incidence_upper_bound = plan.witness_upper_bound;
  plan.candidate_coverage_upper_bound = plan.witness_upper_bound;
  plan.candidate_partition_upper_bound = candidates.partitions().size();

  std::uint64_t boundary_work = 0;
  std::uint64_t work_per_candidate = 0;
  std::uint64_t candidate_work = 0;
  std::uint64_t final_family_work = 0;
  if (!checked_multiply<std::uint64_t>(boundary_pair_requests,
                                       std::uint64_t{128}, boundary_work) ||
      !checked_add<std::uint64_t>(boundary_work, std::uint64_t{384},
                                  work_per_candidate) ||
      !checked_multiply<std::uint64_t>(plan.candidate_count,
                                       work_per_candidate, candidate_work) ||
      !checked_multiply<std::uint64_t>(plan.request_upper_bound,
                                       std::uint64_t{32}, final_family_work) ||
      !checked_add<std::uint64_t>(candidate_work, final_family_work,
                                  plan.fixed_work_units) ||
      !checked_add<std::uint64_t>(plan.fixed_work_units, std::uint64_t{1},
                                  plan.fixed_work_units) ||
      !checked_add<std::uint64_t>(
          plan.fixed_work_units,
          std::uint64_t{4 + 17},
          plan.fixed_work_units)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 work count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }

  std::uint64_t boundary_bytes = 0;
  std::uint64_t bytes_per_candidate = 0;
  std::uint64_t candidate_bytes = 0;
  std::uint64_t graph_bytes = 0;
  std::uint64_t dependency_bytes = 0;
  std::uint64_t witness_bytes = 0;
  std::uint64_t table_bytes = 0;
  if (!checked_multiply<std::uint64_t>(boundary_pair_requests,
                                       std::uint64_t{4096}, boundary_bytes) ||
      !checked_add<std::uint64_t>(boundary_bytes, std::uint64_t{49152},
                                  bytes_per_candidate) ||
      !checked_multiply<std::uint64_t>(plan.candidate_count,
                                       bytes_per_candidate, candidate_bytes) ||
      !checked_add<std::uint64_t>(candidate_bytes, std::uint64_t{4096},
                                  plan.fixed_temporary_bytes) ||
      !checked_multiply<std::uint64_t>(plan.request_upper_bound,
          static_cast<std::uint64_t>(sizeof(canonical_relation_request)),
          graph_bytes) ||
      !checked_multiply<std::uint64_t>(plan.dependency_upper_bound,
          static_cast<std::uint64_t>(sizeof(canonical_relation_dependency)),
          dependency_bytes) ||
      !checked_multiply<std::uint64_t>(plan.witness_upper_bound,
          static_cast<std::uint64_t>(sizeof(candidate_id) +
                                     sizeof(relation_request_id)),
          witness_bytes) ||
      !checked_multiply<std::uint64_t>(plan.request_upper_bound,
                                       std::uint64_t{512}, table_bytes) ||
      !checked_add<std::uint64_t>(graph_bytes, dependency_bytes, graph_bytes) ||
      !checked_add<std::uint64_t>(graph_bytes, witness_bytes, graph_bytes) ||
      !checked_add<std::uint64_t>(graph_bytes, table_bytes, graph_bytes) ||
      !checked_add<std::uint64_t>(graph_bytes,
          static_cast<std::uint64_t>(sizeof(relation_request_graph)) +
              std::uint64_t{65536},
          plan.fixed_persistent_bytes)) {
    error = relation_error(relation_subcode::byte_count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 preflight byte count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }

  if (plan.request_upper_bound >
          static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 request count is not addressable",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }
  if (plan.request_upper_bound > capabilities.maximum_requests ||
      plan.relation_upper_bound > capabilities.maximum_relations ||
      plan.interval_evidence_upper_bound >
          capabilities.maximum_interval_evidence ||
      plan.region_record_upper_bound > capabilities.maximum_region_records ||
      plan.construction_upper_bound > capabilities.maximum_constructions ||
      plan.construction_ledger_upper_bound >
          capabilities.maximum_construction_ledger ||
      plan.symbolic_upper_bound > capabilities.maximum_symbolic_decisions ||
      plan.event_seed_upper_bound > capabilities.maximum_event_seeds ||
      plan.event_seed_incidence_upper_bound >
          capabilities.maximum_event_seed_incidence ||
      plan.candidate_coverage_upper_bound >
          capabilities.maximum_candidate_coverage ||
      plan.dependency_upper_bound > capabilities.maximum_dependencies ||
      plan.witness_upper_bound > capabilities.maximum_consumers ||
      capabilities.maximum_diagnostics < 4 ||
      capabilities.maximum_replay_checkpoints < 17 ||
      plan.fixed_work_units > capabilities.maximum_work_units) {
    error = relation_error(relation_subcode::work_limit,
                           bounded_boolean_error_category::resource_limit,
                           "Component 07 preflight limit exceeded",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }
  return true;
}

template bool preflight_relation_foundation<float, std::uint32_t>(
    const canonical_candidate_stream<float, std::uint32_t> &,
    const relation_capabilities &, relation_preflight_plan &,
    bounded_boolean_error &);
template bool preflight_relation_foundation<float, std::uint64_t>(
    const canonical_candidate_stream<float, std::uint64_t> &,
    const relation_capabilities &, relation_preflight_plan &,
    bounded_boolean_error &);
template bool preflight_relation_foundation<double, std::uint32_t>(
    const canonical_candidate_stream<double, std::uint32_t> &,
    const relation_capabilities &, relation_preflight_plan &,
    bounded_boolean_error &);
template bool preflight_relation_foundation<double, std::uint64_t>(
    const canonical_candidate_stream<double, std::uint64_t> &,
    const relation_capabilities &, relation_preflight_plan &,
    bounded_boolean_error &);

} // namespace ygor::mesh_boolean::bounded
