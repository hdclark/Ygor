#include "SourceTriangulationFixtures.h"
using namespace source_triangulation_tests;
void test_source_triangulation_canonicalization() {
  auto first_fixture = make_fixture(box<double, std::uint32_t>());
  auto first = triangulate(first_fixture);
  require(first.has_value(), "canonical baseline");
  auto mesh = box<double, std::uint32_t>();
  for (auto &face : mesh.faces)
    std::rotate(face.begin(), face.begin() + 1, face.end());
  std::reverse(mesh.faces.begin(), mesh.faces.end());
  auto second_fixture = make_fixture(mesh);
  auto second = triangulate(second_fixture);
  require(second.has_value(), "canonical permutation");
  require((*first.value())->source_semantic_digest() ==
              (*second.value())->source_semantic_digest(),
          "presentation permutations preserve source semantics");
  require((*first.value())->exact_triangulation_digest() ==
              (*second.value())->exact_triangulation_digest(),
          "presentation permutations preserve exact V1 triangulation layout");
}
