#include "BroadPhaseFixtures.h"

namespace broad_phase_tests {
void test_domain() {
  auto fixture = build(box(), box(0.25, 0.25, 0.25, 1.25, 1.25, 1.25));
  const auto &stats = fixture.artifact->statistics();
  require(stats.internal_diagonal_counts[0] != 0 &&
              stats.internal_diagonal_counts[1] != 0,
          "broad-phase V1 must include facet-internal diagonals");
  for (const auto operand : {bounded::operand_id::a, bounded::operand_id::b}) {
    const auto &table = fixture.artifact->primitive_table(operand);
    require(table.edges.size() ==
                fixture.artifact->statistics().edge_counts[bounded::operand_slot(operand)],
            "broad-phase edge-domain count");
    for (const auto &edge : table.edges) {
      require(edge.inclusion == bounded::candidate_domain_disposition::included,
              "broad-phase canonical edge included");
      if (edge.edge_class == bounded::canonical_edge_class::facet_internal_diagonal)
        require(!edge.source_feature_owner && !edge.symbolic_contact_owner &&
                    !edge.classification_barrier_inside_source_facet &&
                    !edge.retained_surface_feature,
                "internal diagonal visibility separation");
    }
  }
}
} // namespace broad_phase_tests
