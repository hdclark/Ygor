#include "BroadPhaseFixtures.h"

namespace broad_phase_tests {
void test_properties() {
  auto first = build(box(), box(0.5, 0.5, 0.5, 1.5, 1.5, 1.5));
  auto second = build(box(), box(0.5, 0.5, 0.5, 1.5, 1.5, 1.5));
  require(first.artifact->canonical_bytes() == second.artifact->canonical_bytes(),
          "broad-phase repeated construction is byte deterministic");

  auto permuted = build(permuted_box(),
                        permuted_box(0.5, 0.5, 0.5, 1.5, 1.5, 1.5));
  require(first.artifact->candidates().size() ==
              permuted.artifact->candidates().size(),
          "presentation permutation candidate count");
  for (std::size_t i = 0; i < first.artifact->candidates().size(); ++i)
    require(first.artifact->candidates()[i].key ==
                permuted.artifact->candidates()[i].key,
            "presentation permutation candidate semantic order");
}
} // namespace broad_phase_tests
