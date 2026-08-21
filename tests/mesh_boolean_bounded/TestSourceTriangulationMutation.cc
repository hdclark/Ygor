#include "SourceTriangulationFixtures.h"
namespace ygor::mesh_boolean::bounded {
struct source_triangle_complex_test_access {
  template <class T, class I>
  static source_triangle_complex<T, I> copy(const source_triangle_complex<T, I> &a) { return a; }
  template <class T, class I>
  static auto &diagonals(source_triangle_complex<T, I> &a) { return a.diagonals_; }
  template <class T, class I>
  static auto &edge_uses(source_triangle_complex<T, I> &a) { return a.edge_uses_; }
};
}
using namespace source_triangulation_tests;
void test_source_triangulation_mutation() {
  auto fixture = make_fixture(box<double, std::uint32_t>());
  auto result = triangulate(fixture);
  require(result.has_value(), "mutation baseline");
  auto mutated = bounded::source_triangle_complex_test_access::copy(**result.value());
  bounded::source_triangle_complex_test_access::diagonals(mutated)[0].source_edge_candidate_eligible = true;
  require(!bounded::verify_source_triangle_complex(mutated, *fixture.operand, *fixture.precision),
          "verifier rejects internal diagonal source-edge eligibility mutation");
  mutated = bounded::source_triangle_complex_test_access::copy(**result.value());
  bounded::source_triangle_complex_test_access::edge_uses(mutated)[0].destination =
      bounded::source_triangle_complex_test_access::edge_uses(mutated)[0].origin;
  require(!bounded::verify_source_triangle_complex(mutated, *fixture.operand, *fixture.precision),
          "verifier rejects boundary endpoint mutation");
}
