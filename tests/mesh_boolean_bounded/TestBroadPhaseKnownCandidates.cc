#include "BroadPhaseFixtures.h"

namespace broad_phase_tests {
void test_known_candidates() {
  auto disjoint = build(box(), box(4, 4, 4, 5, 5, 5));
  require(disjoint.artifact->candidates().empty(),
          "disjoint boxes have no broad-phase candidates");
  auto overlapping = build(box(), box(0.5, 0.5, 0.5, 1.5, 1.5, 1.5));
  require(!overlapping.artifact->candidates().empty(),
          "overlapping boxes have broad-phase candidates");
  bool saw_a = false, saw_b = false;
  for (const auto &candidate : overlapping.artifact->candidates()) {
    saw_a = saw_a || candidate.role ==
                         bounded::directed_candidate_role::a_edge_b_triangle;
    saw_b = saw_b || candidate.role ==
                         bounded::directed_candidate_role::b_edge_a_triangle;
  }
  require(saw_a && saw_b, "both directed broad-phase roles published");
}
} // namespace broad_phase_tests
