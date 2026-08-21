#include "StrictFloatingBuild.h"
#include "RelationPreflight.h"

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

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
  std::uint64_t linear_boundary_evidence = 0;
  std::uint64_t facet_pair_requests = 0;
  // Candidate triangles repeatedly discover the same canonical source-facet
  // pair. Plan 07 closes that pair once and retains candidates as consumers.
  std::vector<std::array<std::uint64_t, 5>> facet_pair_closures;
  const auto boundary_size = [](const auto &operand, std::uint64_t source_facet,
                                std::uint64_t &size) {
    for (const auto &facet : operand->facet_groups())
      if (facet.source_facet == source_facet) {
        size = facet.boundary_halfedges.size();
        return size >= 3;
      }
    return false;
  };
  for (std::size_t ordinal = 0; ordinal < candidates.candidates().size();
       ++ordinal) {
    const auto &candidate = candidates.candidates()[ordinal];
    const bool edge_a = candidate.role == directed_candidate_role::a_edge_b_triangle;
    const auto &edge_table = candidates.primitive_table(
        edge_a ? operand_id::a : operand_id::b);
    const auto &triangle_table = candidates.primitive_table(
        edge_a ? operand_id::b : operand_id::a);
    const auto &edge_operand = edge_a ? manifolds->a() : manifolds->b();
    const auto &triangle_operand = edge_a ? manifolds->b() : manifolds->a();
    if (candidate.edge.ordinal() >= edge_table.edges.size() ||
        candidate.triangle.ordinal() >= triangle_table.triangles.size()) {
      error = relation_error(relation_subcode::malformed_candidate,
                             bounded_boolean_error_category::internal_invariant_error,
                             "Component 07 preflight candidate primitive is out of range",
                             relation_checkpoint::count_representability_preflight);
      error.witnesses[0] = ordinal;
      error.witness_count = 1;
      return false;
    }
    const auto &edge = edge_table.edges[candidate.edge.ordinal()];
    const auto &triangle = triangle_table.triangles[candidate.triangle.ordinal()];
    std::uint64_t opposite_boundary = 0, incident_boundary = 0;
    if (!boundary_size(triangle_operand, triangle.source_facet,
                       opposite_boundary)) {
      error = relation_error(relation_subcode::predecessor_mismatch,
                             bounded_boolean_error_category::internal_invariant_error,
                             "Component 07 candidate opposite facet is absent",
                             relation_checkpoint::count_representability_preflight);
      error.witnesses[0] = ordinal;
      error.witnesses[1] = triangle.source_facet;
      error.witness_count = 2;
      return false;
    }
    std::array<std::uint64_t, 2> incident_facets = edge.source_facets;
    std::size_t incident_count = edge.edge_class == canonical_edge_class::source_edge ? 2 : 1;
    if (edge.edge_class != canonical_edge_class::source_edge)
      incident_facets[0] = edge.source_facet;
    for (std::size_t i = 0; i < incident_count; ++i) {
      std::uint64_t boundary = 0;
      if (!boundary_size(edge_operand, incident_facets[i], boundary) ||
          !checked_add(incident_boundary, boundary, incident_boundary)) {
        error = relation_error(relation_subcode::predecessor_mismatch,
                               bounded_boolean_error_category::internal_invariant_error,
                               "Component 07 candidate incident facet is absent",
                               relation_checkpoint::count_representability_preflight);
        error.witnesses[0] = ordinal;
        error.witnesses[1] = incident_facets[i];
        error.witness_count = 2;
        return false;
      }
      facet_pair_closures.push_back(
          {{edge_a ? 0U : 1U, incident_facets[i], triangle.source_facet,
            boundary, opposite_boundary}});
    }
    std::uint64_t local_pairs = 0, local_linear = 0;
    if (!checked_multiply(opposite_boundary, incident_boundary, local_pairs) ||
        !checked_add(opposite_boundary, incident_boundary, local_linear) ||
        !checked_add(facet_pair_requests,
                     static_cast<std::uint64_t>(incident_count),
                     facet_pair_requests)) {
      error = relation_error(relation_subcode::count_overflow,
                             bounded_boolean_error_category::index_overflow,
                             "Component 07 candidate-local boundary accounting overflow",
                             relation_checkpoint::count_representability_preflight);
      error.witnesses[0] = ordinal;
      error.witness_count = 1;
      return false;
    }
    if (local_pairs > plan.maximum_candidate_boundary_pairs) {
      plan.maximum_candidate_boundary_pairs = local_pairs;
      plan.maximum_candidate_boundary_witness = ordinal;
    }
  }

  std::sort(facet_pair_closures.begin(), facet_pair_closures.end());
  facet_pair_closures.erase(
      std::unique(facet_pair_closures.begin(), facet_pair_closures.end()),
      facet_pair_closures.end());
  facet_pair_requests = facet_pair_closures.size();
  for (const auto &closure : facet_pair_closures) {
    std::uint64_t local_pairs = 0;
    std::uint64_t local_linear = 0;
    if (!checked_multiply(closure[3], closure[4], local_pairs) ||
        !checked_add(closure[3], closure[4], local_linear) ||
        !checked_add(boundary_pair_requests, local_pairs,
                     boundary_pair_requests) ||
        !checked_add(linear_boundary_evidence, local_linear,
                     linear_boundary_evidence)) {
      error = relation_error(
          relation_subcode::count_overflow,
          bounded_boolean_error_category::index_overflow,
          "Component 07 canonical facet-pair accounting overflow",
          relation_checkpoint::count_representability_preflight);
      return false;
    }
  }

  std::uint64_t fixed_candidate_requests = 0;
  if (!checked_multiply(plan.candidate_count, std::uint64_t{4},
                        fixed_candidate_requests) ||
      !checked_add(linear_boundary_evidence, fixed_candidate_requests,
                   plan.vertex_facet_upper_bound) ||
      !checked_multiply(plan.candidate_count, std::uint64_t{3},
                        fixed_candidate_requests) ||
      !checked_add(boundary_pair_requests, plan.vertex_facet_upper_bound,
                   plan.initial_request_upper_bound) ||
      !checked_add(plan.initial_request_upper_bound, fixed_candidate_requests,
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
  std::uint64_t linear_events = 0;
  if (!checked_multiply<std::uint64_t>(linear_boundary_evidence,
                                       std::uint64_t{2}, linear_events) ||
      !checked_add<std::uint64_t>(boundary_pair_requests, linear_events,
                                  plan.construction_upper_bound) ||
      !checked_multiply<std::uint64_t>(plan.candidate_count,
                                       std::uint64_t{8}, linear_events) ||
      !checked_add(plan.construction_upper_bound, linear_events,
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
  plan.transverse_membership_upper_bound = plan.construction_upper_bound;
  plan.disposition_upper_bound = plan.candidate_count;

  // Family 04 publishes every accepted parameter, residual/conditioning
  // interval, complete point/facet region classification, and every
  // partition breakpoint/interval witness. Pairwise source-boundary closure
  // dominates these tables. The coefficients deliberately cover both
  // directed incident facets, all three residual axes, complete polygon
  // traversal evidence, and triangle-local reconciliation witnesses.
  std::uint64_t family04_pair_evidence = 0;
  std::uint64_t family04_linear_evidence = 0;
  std::uint64_t family04_fixed_evidence = 0;
  if (!checked_multiply<std::uint64_t>(boundary_pair_requests,
                                        std::uint64_t{64},
                                        family04_pair_evidence) ||
      !checked_multiply<std::uint64_t>(linear_boundary_evidence,
                                        std::uint64_t{128},
                                        family04_linear_evidence) ||
      !checked_multiply<std::uint64_t>(plan.candidate_count,
                                       std::uint64_t{256},
                                       family04_fixed_evidence) ||
      !checked_add<std::uint64_t>(family04_pair_evidence,
                                  family04_linear_evidence,
                                  plan.interval_evidence_upper_bound) ||
      !checked_add(plan.interval_evidence_upper_bound,
                   family04_fixed_evidence,
                   plan.interval_evidence_upper_bound)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 family-04 evidence count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }
  plan.region_record_upper_bound = plan.interval_evidence_upper_bound;

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

  std::uint64_t initial_dependency_upper_bound = 0;
  std::uint64_t derived_dependency_upper_bound = 0;
  if (!checked_add<std::uint64_t>(linear_boundary_evidence,
                                  boundary_pair_requests,
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
  std::uint64_t candidate_work = 0;
  std::uint64_t final_family_work = 0;
  if (!checked_multiply<std::uint64_t>(boundary_pair_requests,
                                       std::uint64_t{128}, boundary_work) ||
       !checked_multiply<std::uint64_t>(plan.candidate_count,
                                        std::uint64_t{384}, candidate_work) ||
       !checked_add(boundary_work, candidate_work, candidate_work) ||
      !checked_multiply<std::uint64_t>(plan.request_upper_bound,
                                       std::uint64_t{32}, final_family_work) ||
      !checked_add<std::uint64_t>(candidate_work, final_family_work,
                                  plan.fixed_work_units) ||
      !checked_add<std::uint64_t>(plan.fixed_work_units, std::uint64_t{1},
                                  plan.fixed_work_units) ||
       !checked_add<std::uint64_t>(
           plan.fixed_work_units,
           std::uint64_t{4 + 17},
           plan.fixed_work_units) ||
       !checked_multiply<std::uint64_t>(plan.fixed_work_units,
                                        std::uint64_t{4},
                                        plan.fixed_work_units)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 work count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }

  std::uint64_t boundary_bytes = 0;
  std::uint64_t candidate_bytes = 0;
  std::uint64_t graph_bytes = 0;
  std::uint64_t dependency_bytes = 0;
  std::uint64_t witness_bytes = 0;
  std::uint64_t table_bytes = 0;
  if (!checked_multiply<std::uint64_t>(boundary_pair_requests,
                                       std::uint64_t{4096}, boundary_bytes) ||
       !checked_multiply<std::uint64_t>(plan.candidate_count,
                                        std::uint64_t{49152}, candidate_bytes) ||
       !checked_add(boundary_bytes, candidate_bytes, candidate_bytes) ||
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

  plan.domains.requests = plan.request_upper_bound;
  plan.domains.primitives = primitive_support_upper_bound;
  plan.domains.relation_families = plan.relation_upper_bound;
  if (!checked_add(plan.dependency_upper_bound, plan.witness_upper_bound,
                   plan.domains.graph) ||
      !checked_add(plan.interval_evidence_upper_bound,
                   plan.region_record_upper_bound, plan.domains.regions) ||
      !checked_add(plan.domains.regions, plan.relation_upper_bound,
                   plan.domains.numerical_workspaces) ||
      !checked_add(boundary_pair_requests, facet_pair_requests,
                   plan.domains.overlays) ||
      !checked_add(plan.construction_upper_bound,
                   plan.construction_ledger_upper_bound,
                   plan.domains.constructions) ||
      !checked_add(plan.request_upper_bound, plan.dependency_upper_bound,
                   plan.domains.canonical_merge)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 resource-domain count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }
  plan.domains.crossings = multiplicity_upper_bound;
  plan.domains.symbolic = plan.symbolic_upper_bound;
  if (!checked_add(plan.event_seed_upper_bound,
                   plan.event_seed_incidence_upper_bound, plan.domains.seeds) ||
      !checked_add(plan.disposition_upper_bound,
                   plan.candidate_coverage_upper_bound,
                   plan.domains.dispositions) ||
      !checked_add(plan.candidate_count, plan.candidate_partition_upper_bound,
                   plan.domains.private_buffers)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 resource-domain count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }
  plan.domains.codec_replay_diagnostics = 10 + 17 + 7;
  plan.domains.verifier = plan.fixed_work_units;
  plan.domains.persistent_artifact = plan.fixed_persistent_bytes;

  const std::array<std::uint64_t, 17> domain_bounds{{
      plan.domains.requests, plan.domains.primitives,
      plan.domains.relation_families, plan.domains.graph,
      plan.domains.regions, plan.domains.numerical_workspaces,
      plan.domains.overlays, plan.domains.constructions,
      plan.domains.crossings, plan.domains.symbolic, plan.domains.seeds,
      plan.domains.dispositions, plan.domains.canonical_merge,
      plan.domains.private_buffers, plan.fixed_persistent_bytes,
      plan.domains.verifier, plan.domains.persistent_artifact}};
  for (std::size_t domain = 0; domain < domain_bounds.size(); ++domain) {
    if (domain_bounds[domain] <= capabilities.maximum_resource_domains[domain])
      continue;
    error = relation_error(relation_subcode::resource_preflight,
                           bounded_boolean_error_category::resource_limit,
                           "Component 07 resource-domain preflight limit exceeded",
                           relation_checkpoint::count_representability_preflight);
    error.witnesses[0] = domain + 1;
    error.witnesses[1] = domain_bounds[domain];
    error.witnesses[2] = capabilities.maximum_resource_domains[domain];
    error.witnesses[3] = plan.maximum_candidate_boundary_witness;
    error.witness_count = 4;
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
       plan.transverse_membership_upper_bound >
           capabilities.maximum_transverse_memberships ||
      plan.symbolic_upper_bound > capabilities.maximum_symbolic_decisions ||
      plan.event_seed_upper_bound > capabilities.maximum_event_seeds ||
      plan.event_seed_incidence_upper_bound >
          capabilities.maximum_event_seed_incidence ||
      plan.candidate_coverage_upper_bound >
          capabilities.maximum_candidate_coverage ||
      plan.dependency_upper_bound > capabilities.maximum_dependencies ||
      plan.witness_upper_bound > capabilities.maximum_consumers ||
       capabilities.maximum_diagnostics < 7 ||
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
