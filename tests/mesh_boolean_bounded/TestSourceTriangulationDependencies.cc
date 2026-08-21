#include "SourceTriangulationFixtures.h"
using namespace source_triangulation_tests;
void test_source_triangulation_dependencies() {
  auto fixture = make_fixture(concave_prism<double, std::uint32_t>());
  auto indexed = triangulate(fixture, bounded::source_triangulation_provider_kind::indexed_dependency_v1, true);
  require(indexed.has_value(), "indexed dependency provider matches reference");
  require(!(*indexed.value())->trace().empty(), "candidate dependency trace retained");
  for (const auto &step : (*indexed.value())->trace()) {
    require(step.selected_valid && !step.candidates.empty(),
            "every ear step has complete candidate dispositions");
    for (const auto &candidate : step.candidates)
      require(candidate.active_vertices == step.active_ring,
              "full dependency closure records all active points");
  }
}
