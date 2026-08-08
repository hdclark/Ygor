#include "BroadPhaseFixtures.h"
#include "GoldenBroadPhaseV1.h"

namespace broad_phase_tests {
namespace {
bounded::canonical_bound3<scalar> point_bound(scalar x, scalar y, scalar z) {
  bounded::canonical_bound3<scalar> result;
  result.axes[0] = *bounded::finite_interval<scalar>::create(x, x);
  result.axes[1] = *bounded::finite_interval<scalar>::create(y, y);
  result.axes[2] = *bounded::finite_interval<scalar>::create(z, z);
  return result;
}
}
void test_rank_morton() {
  auto one = bounded::make_rank_morton_key({1, 0, 0}, 1);
  require(one.words.size() == 1 && one.words[0] == morton_100_v1,
          "rank-Morton X/Y/Z bit order");
  auto known = bounded::make_rank_morton_key({1, 2, 3}, 2);
  require(known.words.size() == 1 && known.words[0] == morton_011101_v1,
          "rank-Morton known answer");
  auto zero = bounded::make_rank_morton_key({0, 0, 0}, 0);
  require(zero.words.empty(), "rank-Morton zero-width encoding");

  std::vector<bounded::broad_phase_triangle_primitive<scalar>> triangles(4);
  const scalar points[4][3]{{2, 0, 0}, {0, 0, 0}, {2, 0, 0}, {1, 1, 0}};
  for (std::uint64_t i = 0; i < triangles.size(); ++i) {
    auto &triangle = triangles[i];
    triangle.id = bounded::broad_phase_triangle_primitive_id{i};
    triangle.ordinal = i;
    triangle.operand = bounded::operand_id::a;
    triangle.semantic_key.operand = bounded::operand_id::a;
    triangle.semantic_key.source_triangle = i;
    triangle.bound = point_bound(points[i][0], points[i][1], points[i][2]);
    for (std::size_t axis = 0; axis < 3; ++axis) {
      triangle.axis_keys[axis].lower = bounded::finite_total_order_key(
          triangle.bound.axes[axis].lower());
      triangle.axis_keys[axis].upper = bounded::finite_total_order_key(
          triangle.bound.axes[axis].upper());
    }
  }
  bounded::rank_morton_build_result result;
  bounded_boolean_error error;
  require(bounded::assign_rank_morton_order(triangles, result, error),
          "rank-Morton build");
  require(triangles[0].dense_ranks == triangles[2].dense_ranks,
          "equal endpoint pairs share dense ranks");
  std::vector<std::uint8_t> seen(triangles.size(), 0);
  for (const auto primitive : result.spatial_order) {
    require(primitive < seen.size() && !seen[primitive],
            "rank-Morton order permutation");
    seen[primitive] = 1;
  }
}
} // namespace broad_phase_tests
