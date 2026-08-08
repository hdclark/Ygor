#include "BroadPhaseFixtures.h"

namespace broad_phase_tests {
void test_structural_performance() {
  auto fixture = build(box(), box(0.5, 0.5, 0.5, 1.5, 1.5, 1.5));
  const auto &stats = fixture.artifact->statistics();
  for (std::size_t slot = 0; slot < 2; ++slot) {
    require(stats.leaf_counts[slot] ==
                (stats.triangle_counts[slot] +
                 bounded::broad_phase_leaf_capacity_v1 - 1) /
                    bounded::broad_phase_leaf_capacity_v1,
            "structural leaf-count profile");
    require(stats.node_counts[slot] <=
                (stats.leaf_counts[slot] == 0
                     ? 0
                     : 2 * stats.leaf_counts[slot] - 1),
            "structural node-count bound");
  }
  require(stats.maximum_producer_stack != 0 &&
              stats.maximum_verifier_queue != 0 &&
              stats.producer_work_units != 0 && stats.verifier_work_units != 0,
          "structural traversal evidence counters");
  require(stats.candidate_count == fixture.artifact->candidates().size() &&
              stats.partition_count == fixture.artifact->partitions().size() &&
              stats.canonical_bytes == fixture.artifact->canonical_bytes().size(),
          "structural artifact counters");
  const auto &evidence = fixture.artifact->verification_evidence();
  require(evidence.primitive_reconstruction_complete &&
              evidence.rank_reconstruction_complete &&
              evidence.hierarchy_reconstruction_complete &&
              evidence.breadth_first_candidate_set_complete &&
              fixture.artifact->verification() ==
                  bounded::broad_phase_verification_disposition::independently_verified,
          "structural independent verifier completion");
}
} // namespace broad_phase_tests
