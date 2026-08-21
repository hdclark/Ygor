#include "SourceTriangulationFixtures.h"
using namespace source_triangulation_tests;
void test_source_triangulation_coverage() {
  auto fixture = make_fixture(concave_prism<double, std::uint32_t>());
  auto result = triangulate(fixture);
  require(result.has_value(), "coverage fixture triangulates");
  for (const auto &facet : (*result.value())->facets()) {
    require(!facet.producer_witnesses.empty(), "producer witnesses retained");
    require(!facet.verifier_witnesses.empty(), "verifier witnesses retained");
  }
  for (const auto &diagonal : (*result.value())->diagonals()) {
    require(!diagonal.source_feature_eligible &&
                !diagonal.source_edge_candidate_eligible &&
                !diagonal.symbolic_owner_eligible &&
                !diagonal.classification_barrier,
            "internal diagonals are bookkeeping only");
  }
}
