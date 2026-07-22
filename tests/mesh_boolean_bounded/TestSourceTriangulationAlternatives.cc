#include "SourceTriangulationFixtures.h"
using namespace source_triangulation_tests;
void test_source_triangulation_alternatives() {
  auto fixture = make_fixture(box<double, std::uint32_t>());
  auto baseline = triangulate(fixture);
  require(baseline.has_value(), "alternative baseline");
  std::vector<std::vector<std::array<std::uint64_t, 3>>> alternatives;
  alternatives.resize(fixture.operand->facets().size());
  for (const auto &facet : fixture.operand->facets()) {
    require(facet.vertices.size() == 4, "box facet is a quad");
    alternatives[facet.canonical_id] = {
        {facet.vertices[0], facet.vertices[1], facet.vertices[2]},
        {facet.vertices[0], facet.vertices[2], facet.vertices[3]}};
  }
  bounded::source_triangulation_capabilities capabilities;
  capabilities.owner = fixture.context.owner;
  capabilities.resources = fixture.resources.get();
  auto alternative = bounded::assemble_source_triangle_complex_from_triangles(
      fixture.operand, fixture.context, *fixture.precision, capabilities,
      alternatives);
  require(alternative.has_value(), "legal alternative triangulation accepted");
  require((*alternative.value())->source_semantic_digest() ==
              (*baseline.value())->source_semantic_digest(),
          "alternative layout preserves source semantic digest");
  bool same_layout = (*alternative.value())->triangles().size() ==
                     (*baseline.value())->triangles().size();
  if (same_layout)
    for (std::size_t i = 0; i < (*alternative.value())->triangles().size(); ++i)
      same_layout = same_layout &&
                    (*alternative.value())->triangles()[i].vertices ==
                        (*baseline.value())->triangles()[i].vertices;
  require((*alternative.value())->exact_triangulation_digest() !=
              (*baseline.value())->exact_triangulation_digest() || same_layout,
          "different legal layout changes only exact digest");
}
