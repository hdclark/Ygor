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
  std::uint64_t requests_per_candidate = 0;
  // Each original candidate edge has two incident source facets. Closing a
  // candidate-derived facet pair requires every original boundary edge of an
  // incident facet against every original boundary edge of the opposite facet.
  // The remaining three proposals cover one edge/facet request and the two
  // incident facet/facet support requests.
  if (!checked_multiply<std::uint64_t>(maximum_facet_boundary,
                                       maximum_facet_boundary,
                                       boundary_pair_requests) ||
      !checked_multiply<std::uint64_t>(boundary_pair_requests,
                                       std::uint64_t{2},
                                       boundary_pair_requests) ||
      !checked_add<std::uint64_t>(boundary_pair_requests, std::uint64_t{3},
                                  requests_per_candidate) ||
      !checked_multiply<std::uint64_t>(plan.candidate_count,
                                       requests_per_candidate,
                                       plan.request_upper_bound)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 candidate-derived request count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }
  std::uint64_t dependencies_per_candidate = 0;
  if (!checked_add<std::uint64_t>(maximum_facet_boundary,
                                  boundary_pair_requests,
                                  dependencies_per_candidate) ||
      !checked_multiply<std::uint64_t>(plan.candidate_count,
                                       dependencies_per_candidate,
                                       plan.dependency_upper_bound)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 edge/facet dependency count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }
  plan.witness_upper_bound = plan.request_upper_bound;

  std::uint64_t boundary_work = 0;
  std::uint64_t work_per_candidate = 0;
  std::uint64_t candidate_work = 0;
  if (!checked_multiply<std::uint64_t>(boundary_pair_requests,
                                       std::uint64_t{128}, boundary_work) ||
      !checked_add<std::uint64_t>(boundary_work, std::uint64_t{384},
                                  work_per_candidate) ||
      !checked_multiply<std::uint64_t>(plan.candidate_count,
                                       work_per_candidate, candidate_work) ||
      !checked_add<std::uint64_t>(candidate_work, std::uint64_t{1},
                                  plan.fixed_work_units)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 source-edge work count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }

  std::uint64_t boundary_bytes = 0;
  std::uint64_t bytes_per_candidate = 0;
  std::uint64_t candidate_bytes = 0;
  if (!checked_multiply<std::uint64_t>(boundary_pair_requests,
                                       std::uint64_t{4096}, boundary_bytes) ||
      !checked_add<std::uint64_t>(boundary_bytes, std::uint64_t{49152},
                                  bytes_per_candidate) ||
      !checked_multiply<std::uint64_t>(plan.candidate_count,
                                       bytes_per_candidate, candidate_bytes) ||
      !checked_add<std::uint64_t>(candidate_bytes, std::uint64_t{4096},
                                  plan.fixed_temporary_bytes) ||
      !checked_add<std::uint64_t>(
          static_cast<std::uint64_t>(sizeof(relation_request_graph)),
          std::uint64_t{4096}, plan.fixed_persistent_bytes)) {
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
      plan.request_upper_bound > capabilities.maximum_relations ||
      plan.dependency_upper_bound > capabilities.maximum_dependencies ||
      plan.witness_upper_bound > capabilities.maximum_consumers ||
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
