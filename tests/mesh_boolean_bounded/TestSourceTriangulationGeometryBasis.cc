#include "SourceTriangulationFixtures.h"
using namespace source_triangulation_tests;
void test_source_triangulation_geometry_basis() {
  auto fixture = make_fixture(box<double, std::uint32_t>());
  auto result = triangulate(fixture);
  require(result.has_value(), "geometry-basis fixture triangulates");
  for (const auto &triangle : (*result.value())->triangles()) {
    require(triangle.basis.kind == bounded::source_geometry_basis_kind::nominal_embedded,
            "nominal coherent basis retained");
    require(triangle.basis.predecessor_digest == fixture.operand->digest(),
            "basis predecessor digest retained");
    require(triangle.basis.precision_digest == fixture.precision->digest(),
            "basis precision digest retained");
  }
  bounded::source_triangulation_capabilities wrong;
  wrong.owner = bounded::context_owner_token::create();
  wrong.resources = fixture.resources.get();
  auto rejected = bounded::triangulate_source_operand(
      fixture.operand, fixture.context, *fixture.precision, wrong);
  require(!rejected.has_value() &&
              rejected.error()->subcode == static_cast<std::uint32_t>(
                  bounded::source_triangulation_subcode::wrong_owner),
          "cross-owner source triangulation rejected");
}
