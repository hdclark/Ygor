#include "SourceTriangulationFixtures.h"
using namespace source_triangulation_tests;
void test_source_triangulation_boundary_sharing() {
  auto fixture = make_fixture(box<double, std::uint32_t>());
  auto result = triangulate(fixture);
  require(result.has_value(), "boundary fixture triangulates");
  const auto &artifact = **result.value();
  require(artifact.source_directed_use_to_edge_use().size() == fixture.operand->directed_uses().size(),
          "every source directed use maps to one triangle use");
  for (const auto &edge : fixture.operand->edges()) {
    const auto a = artifact.source_directed_use_to_edge_use()[edge.uses[0]];
    const auto b = artifact.source_directed_use_to_edge_use()[edge.uses[1]];
    require(a < artifact.edge_uses().size() && b < artifact.edge_uses().size(),
            "paired source uses mapped");
    require(artifact.edge_uses()[a].origin == artifact.edge_uses()[b].destination &&
                artifact.edge_uses()[a].destination == artifact.edge_uses()[b].origin,
            "shared source edge directions remain opposite");
  }
}
