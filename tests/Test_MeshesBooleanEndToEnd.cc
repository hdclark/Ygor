#include "MeshBooleanAnalyticFixtures.h"
#include "MeshBooleanTestHarness.h"

#include <iostream>

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

template <class T, class I> void disjoint_matrix() {
  struct expectation { operation op; std::uint64_t components, vertices, faces; };
  const expectation cases[] = {
      {operation::regularized_union, 2, 16, 24},
      {operation::regularized_intersection, 0, 0, 0},
      {operation::a_minus_b, 1, 8, 12},
      {operation::b_minus_a, 1, 8, 12},
      {operation::symmetric_difference, 2, 16, 24},
  };
  for (const auto &entry : cases) {
    const auto result = run_box_operation<T, I>(T(0), T(1), T(3), T(4), entry.op);
    const auto topology = independently_check_closed_mesh(result->payload->mesh);
    require_equal(topology.components, entry.components, "analytic component count");
    require_equal(topology.vertices, entry.vertices, "analytic vertex count");
    require_equal(topology.faces, entry.faces, "analytic triangle count");
    require_equal(topology.euler_characteristic,
                  static_cast<std::int64_t>(entry.components * 2),
                  "analytic Euler characteristic");
  }
}

int main() {
  harness tests;
  tests.add("C14.E2E.disjoint.float.u32", [] { disjoint_matrix<float, std::uint32_t>(); });
  tests.add("C14.E2E.disjoint.float.u64", [] { disjoint_matrix<float, std::uint64_t>(); });
  tests.add("C14.E2E.disjoint.double.u32", [] { disjoint_matrix<double, std::uint32_t>(); });
  tests.add("C14.E2E.disjoint.double.u64", [] { disjoint_matrix<double, std::uint64_t>(); });
  tests.add("C14.E2E.equal.identities", [] {
    for (const auto op : {operation::regularized_union,
                          operation::regularized_intersection,
                          operation::a_minus_b, operation::b_minus_a,
                          operation::symmetric_difference}) {
      const auto result = run_box_operation<double, std::uint32_t>(0, 1, 0, 1, op);
      const bool retained = op == operation::regularized_union ||
                            op == operation::regularized_intersection;
      require_equal(result->payload->mesh.vertices.size(), retained ? 8U : 0U,
                    "equal-solid vertex identity");
      require_equal(result->payload->mesh.faces.size(), retained ? 12U : 0U,
                    "equal-solid boundary identity");
    }
  });
  return tests.run(std::cout, std::cerr);
}
