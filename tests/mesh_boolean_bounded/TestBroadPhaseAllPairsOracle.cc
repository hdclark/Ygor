#include "BroadPhaseExactOracle.h"

namespace broad_phase_tests {
void test_all_pairs_oracle() {
  auto fixture = build(box(), box(0.25, 0.25, 0.25, 1.25, 1.25, 1.25));
  auto oracle = all_pairs_keys(*fixture.artifact);
  require(oracle.size() == fixture.artifact->candidates().size(),
          "all-pairs oracle candidate count");
  for (std::size_t i = 0; i < oracle.size(); ++i)
    require(oracle[i] == fixture.artifact->candidates()[i].key,
            "all-pairs oracle exact candidate set");
  const auto &evidence = fixture.artifact->verification_evidence();
  require(evidence.exhaustive_all_pairs_performed &&
              evidence.exhaustive_all_pairs_complete &&
              evidence.exhaustive_candidate_count == oracle.size(),
          "mandatory exhaustive diagnostic evidence");
}
} // namespace broad_phase_tests
