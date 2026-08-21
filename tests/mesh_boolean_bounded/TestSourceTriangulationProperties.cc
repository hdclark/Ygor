#include "SourceTriangulationFixtures.h"
#include <cmath>
using namespace source_triangulation_tests;
void test_source_triangulation_properties() {
  for (int iteration = 0; iteration < 8; ++iteration) {
    auto mesh = box<double, std::uint32_t>();
    const double scale = std::ldexp(1.0, iteration - 4);
    for (auto &vertex : mesh.vertices) {
      vertex.x = vertex.x * scale + double(iteration * 4);
      vertex.y = vertex.y * scale + double(iteration * 4);
      vertex.z = vertex.z * scale + double(iteration * 4);
    }
    auto fixture = make_fixture(mesh);
    auto result = triangulate(fixture);
    require(result.has_value(), "power-of-two scale and translation property");
    require(bounded::verify_source_triangle_complex(**result.value(), *fixture.operand,
                                                     *fixture.precision),
            "property artifact independently verifies");
  }
}
