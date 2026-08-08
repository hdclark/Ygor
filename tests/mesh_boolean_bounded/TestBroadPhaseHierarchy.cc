#include "BroadPhaseFixtures.h"

#include <numeric>

namespace broad_phase_tests {
void test_hierarchy() {
  std::vector<bounded::broad_phase_triangle_primitive<scalar>> triangles(17);
  std::vector<std::uint64_t> order(triangles.size());
  std::iota(order.begin(), order.end(), std::uint64_t{0});
  for (std::uint64_t i = 0; i < triangles.size(); ++i) {
    auto &triangle = triangles[i];
    triangle.id = bounded::broad_phase_triangle_primitive_id{i};
    triangle.ordinal = i;
    triangle.operand = bounded::operand_id::a;
    triangle.spatial_ordinal = i;
    triangle.bound.axes[0] = *bounded::finite_interval<scalar>::create(i, i);
    triangle.bound.axes[1] = *bounded::finite_interval<scalar>::create(0, 0);
    triangle.bound.axes[2] = *bounded::finite_interval<scalar>::create(0, 0);
  }
  bounded::triangle_aabb_hierarchy<scalar> hierarchy;
  bounded_boolean_error error;
  require(bounded::build_triangle_aabb_hierarchy(
              bounded::operand_id::a, triangles, order, hierarchy, error),
          "triangle hierarchy construction");
  require(hierarchy.leaf_count == 3 && hierarchy.nodes.size() == 5,
          "triangle hierarchy frozen odd-carry layout");
  require(hierarchy.root == 4 && hierarchy.nodes[hierarchy.root].subtree_primitive_count == 17,
          "triangle hierarchy root coverage");
  for (std::uint64_t i = 0; i < hierarchy.leaf_count; ++i)
    require(hierarchy.nodes[i].kind == bounded::hierarchy_node_kind::leaf &&
                hierarchy.nodes[i].id.ordinal() == i,
            "triangle hierarchy leaves-first IDs");
  require(bounded::verify_triangle_aabb_hierarchy_producer(
              triangles, hierarchy, error),
          "triangle hierarchy producer verification");
}
} // namespace broad_phase_tests
