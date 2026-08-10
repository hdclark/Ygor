#include "SourceTriangulationFixtures.h"
#include "SourceTriangulationExactOracle.h"
using namespace source_triangulation_tests;
void test_source_triangulation_unit() {
  auto fixture = make_fixture(box<double, std::uint32_t>());
  auto result = triangulate(fixture);
  require(result.has_value(), "box source triangulation succeeds");
  const auto &artifact = **result.value();
  require(artifact.facets().size() == 6, "box facet count");
  require(artifact.triangles().size() == 12, "box triangle count");
  require(artifact.diagonals().size() == 6, "box diagonal count");
  require(artifact.edge_uses().size() == 36, "box edge-use count");
  for (const auto &facet : artifact.facets()) {
    require(facet.triangles.size() == exact_expected_triangle_count(facet.source_vertices.size()),
            "per-facet triangle count");
    require(facet.diagonals.size() == exact_expected_diagonal_count(facet.source_vertices.size()),
            "per-facet diagonal count");
  }
  require(bounded::verify_source_triangle_complex(artifact, *fixture.operand,
                                                   *fixture.precision),
          "independent source triangle verifier accepts");
}
