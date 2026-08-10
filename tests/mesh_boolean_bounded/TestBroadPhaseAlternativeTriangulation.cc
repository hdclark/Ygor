#include "BroadPhaseFixtures.h"

namespace broad_phase_tests {
void test_alternative_triangulation() {
  auto indexed = build(
      box(), box(0.25, 0.25, 0.25, 1.25, 1.25, 1.25),
      bounded::source_triangulation_provider_kind::indexed_dependency_v1, true);
  auto reference = build(
      box(), box(0.25, 0.25, 0.25, 1.25, 1.25, 1.25),
      bounded::source_triangulation_provider_kind::full_rescan_reference_v1,
      false);
  require(indexed.artifact->candidates().size() ==
              reference.artifact->candidates().size(),
          "alternative source triangulation candidate count");
  for (std::size_t i = 0; i < indexed.artifact->candidates().size(); ++i)
    require(indexed.artifact->candidates()[i].key ==
                reference.artifact->candidates()[i].key,
            "alternative source triangulation semantic candidate set");
}
} // namespace broad_phase_tests
