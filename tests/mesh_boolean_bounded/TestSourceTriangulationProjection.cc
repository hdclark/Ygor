#include "SourceTriangulationFixtures.h"
using namespace source_triangulation_tests;
void test_source_triangulation_projection() {
  auto fixture = make_fixture(concave_prism<double, std::uint32_t>());
  auto result = triangulate(fixture);
  if (!result.has_value()) throw std::runtime_error(result.error()->summary);
  require((*result.value())->triangles().size() == 20,
          "concave prism projected facets triangulate");
  for (const auto &triangle : (*result.value())->triangles())
    require(triangle.orientation.bounded_sign != bounded::bounded_planar_sign::uncertain,
            "every projected source triangle has definite orientation");
}
