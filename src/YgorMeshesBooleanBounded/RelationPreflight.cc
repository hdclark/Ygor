#include "StrictFloatingBuild.h"
#include "RelationPreflight.h"

#include <limits>

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
bool preflight_relation_foundation(
    const canonical_candidate_stream<T, I> &candidates,
    const relation_capabilities &capabilities, relation_preflight_plan &plan,
    bounded_boolean_error &error) {
  plan = {};
  plan.candidate_count = candidates.candidates().size();
  if (!checked_multiply<std::uint64_t>(plan.candidate_count,
                                       std::uint64_t{3},
                                       plan.request_upper_bound)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 source-edge request count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }
  plan.dependency_upper_bound = 0;
  plan.witness_upper_bound = plan.request_upper_bound;

  std::uint64_t candidate_work = 0;
  if (!checked_multiply<std::uint64_t>(plan.candidate_count,
                                       std::uint64_t{128}, candidate_work) ||
      !checked_add<std::uint64_t>(candidate_work, std::uint64_t{1},
                                  plan.fixed_work_units)) {
    error = relation_error(relation_subcode::count_overflow,
                           bounded_boolean_error_category::index_overflow,
                           "Component 07 source-edge work count overflow",
                           relation_checkpoint::count_representability_preflight);
    return false;
  }

  std::uint64_t candidate_bytes = 0;
  if (!checked_multiply<std::uint64_t>(plan.candidate_count,
                                       std::uint64_t{32768}, candidate_bytes) ||
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
