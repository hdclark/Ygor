#include "BroadPhaseExactOracle.h"

namespace broad_phase_tests {

std::vector<bounded::canonical_candidate_key>
all_pairs_keys(const bounded::canonical_candidate_stream<scalar, index_type> &artifact) {
  std::vector<bounded::canonical_candidate_key> result;
  const auto collect = [&](bounded::directed_candidate_role role) {
    const auto &edges = artifact.primitive_table(bounded::role_edge_operand(role)).edges;
    const auto &triangles =
        artifact.primitive_table(bounded::role_triangle_operand(role)).triangles;
    for (const auto &edge : edges) {
      for (const auto &triangle : triangles) {
        if (bounded::classify_closed_bound_relation(edge.bound, triangle.bound)
                .definitely_separated)
          continue;
        bounded::canonical_candidate_key key;
        key.role = role;
        key.family =
            bounded::broad_phase_relation_family::canonical_edge_source_triangle;
        key.edge = edge.semantic_key;
        key.triangle = triangle.semantic_key;
        key.edge_class = edge.edge_class;
        key.domain_policy_version = artifact.domain_policy_version();
        result.push_back(std::move(key));
      }
    }
  };
  collect(bounded::directed_candidate_role::a_edge_b_triangle);
  collect(bounded::directed_candidate_role::b_edge_a_triangle);
  std::sort(result.begin(), result.end());
  return result;
}

} // namespace broad_phase_tests
