#pragma once

#include "InputGeometryRelations.h"

#include <optional>

namespace ygor::mesh_boolean::bounded::input_geometry_verifier {

template <class T>
validation_relation triangle_relation(const validation_triangle<T> &left,
                                      const validation_triangle<T> &right) {
  using namespace input_relation_detail;
  const std::array<validation_point3<T>, 3> l{left.a, left.b, left.c};
  const std::array<validation_point3<T>, 3> r{right.a, right.b, right.c};
  std::array<int, 3> left_sides{}, right_sides{};
  for (std::size_t i = 0; i < 3; ++i) {
    left_sides[i] = orient3(right.a, right.b, right.c, l[i]);
    right_sides[i] = orient3(left.a, left.b, left.c, r[i]);
    if (left_sides[i] == 2 || right_sides[i] == 2)
      return validation_relation::uncertain;
  }
  const auto strictly_one_side = [](const auto &sides) {
    return std::all_of(sides.begin(), sides.end(),
                       [](int side) { return side > 0; }) ||
           std::all_of(sides.begin(), sides.end(),
                       [](int side) { return side < 0; });
  };
  if (strictly_one_side(left_sides) || strictly_one_side(right_sides))
    return validation_relation::definitely_disjoint;

  const bool coplanar =
      std::all_of(left_sides.begin(), left_sides.end(),
                  [](int side) { return side == 0; }) &&
      std::all_of(right_sides.begin(), right_sides.end(),
                  [](int side) { return side == 0; });
  if (coplanar) {
    if (same_patch(left, right))
      return validation_relation::whole_patch_coincidence;
    const auto axis = projection_axis(right);
    if (axis > 2)
      return validation_relation::uncertain;
    std::array<std::array<T, 2>, 3> lp{}, rp{};
    for (std::size_t i = 0; i < 3; ++i) {
      lp[i] = project(l[i], axis);
      rp[i] = project(r[i], axis);
    }
    int dimension = 0;
    for (std::size_t j = 0; j < 3; ++j)
      for (std::size_t i = 0; i < 3; ++i) {
        const int relation = segment_relation_2d(
            rp[j], rp[(j + 1) % 3], lp[i], lp[(i + 1) % 3]);
        if (relation == 4)
          return validation_relation::uncertain;
        if (relation == 3)
          return validation_relation::coplanar_positive_area_overlap;
        dimension = std::max(dimension, relation);
      }
    for (std::size_t i = 0; i < 3; ++i)
      if (point_in_triangle_2d(lp[i], rp[0], rp[1], rp[2], true) ||
          point_in_triangle_2d(rp[i], lp[0], lp[1], lp[2], true))
        return validation_relation::coplanar_positive_area_overlap;
    return dimension == 2
               ? validation_relation::edge_contact
               : dimension == 1 ? validation_relation::point_contact
                                : validation_relation::definitely_disjoint;
  }

  std::size_t proper = 0, boundary = 0, common = 0;
  for (const auto &a : l)
    for (const auto &b : r) {
      bool equal = true;
      for (std::size_t axis = 0; axis < 3; ++axis)
        equal &= exact_sign(exact_scalar_comparison(a[axis], b[axis])) == 0;
      common += equal;
    }
  for (std::size_t i = 3; i-- > 0;) {
    const auto right_hit = segment_triangle(r[i], r[(i + 1) % 3], left);
    const auto left_hit = segment_triangle(l[i], l[(i + 1) % 3], right);
    if (right_hit == segment_triangle_hit::uncertain ||
        left_hit == segment_triangle_hit::uncertain)
      return validation_relation::uncertain;
    proper += right_hit == segment_triangle_hit::proper;
    proper += left_hit == segment_triangle_hit::proper;
    boundary += right_hit == segment_triangle_hit::boundary;
    boundary += left_hit == segment_triangle_hit::boundary;
  }
  if (proper)
    return validation_relation::transverse_intersection;
  if (common >= 2)
    return validation_relation::edge_contact;
  if (boundary || common == 1)
    return validation_relation::point_contact;
  return validation_relation::definitely_disjoint;
}

template <class T>
point_shell_result point_against_triangles(
    const validation_point3<T> &point,
    const std::vector<validation_triangle<T>> &triangles, T radius) {
  using namespace input_relation_detail;
  if (triangles.empty())
    return point_shell_result::outside;
  std::array<T, 3> low = point, high = point;
  for (auto triangle = triangles.rbegin(); triangle != triangles.rend();
       ++triangle)
    for (const auto &vertex : {triangle->a, triangle->b, triangle->c})
      for (std::size_t axis = 0; axis < 3; ++axis) {
        low[axis] = std::min(low[axis], vertex[axis]);
        high[axis] = std::max(high[axis], vertex[axis]);
      }

  std::optional<bool> consensus;
  for (std::size_t direction = 0; direction < 6; ++direction) {
    const std::size_t axis = direction / 2;
    const T span = high[axis] - low[axis];
    const T margin = std::max(T(1), span);
    validation_point3<T> endpoint = point;
    endpoint[axis] = direction % 2 ? low[axis] - margin : high[axis] + margin;
    if (!finite_bits(endpoint[axis]) || endpoint == point)
      continue;
    std::uint64_t crossings = 0;
    bool degenerate = false;
    for (auto triangle = triangles.rbegin(); triangle != triangles.rend();
         ++triangle) {
      const int point_side = radius == T(0)
                                 ? orient3(triangle->a, triangle->b,
                                           triangle->c, point)
                                 : bounded_orient3(triangle->a, triangle->b,
                                                   triangle->c, point, radius);
      const int endpoint_side = radius == T(0)
                                    ? orient3(triangle->a, triangle->b,
                                              triangle->c, endpoint)
                                    : bounded_orient3(
                                          triangle->a, triangle->b,
                                          triangle->c, endpoint, radius);
      if (point_side == 2 || endpoint_side == 2)
        return point_shell_result::ambiguous;
      if (point_side == 0) {
        const auto projection = projection_axis(*triangle);
        if (projection > 2)
          return point_shell_result::ambiguous;
        if (point_in_triangle_2d(project(point, projection),
                                 project(triangle->a, projection),
                                 project(triangle->b, projection),
                                 project(triangle->c, projection)))
          return radius == T(0) ? point_shell_result::boundary
                                : point_shell_result::ambiguous;
      }
      if (point_side == 0 || endpoint_side == 0 ||
          point_side == endpoint_side)
        continue;
      const std::array<int, 3> edge_sides{
          radius == T(0) ? orient3(point, endpoint, triangle->a, triangle->b)
                         : bounded_orient3(point, endpoint, triangle->a,
                                           triangle->b, radius),
          radius == T(0) ? orient3(point, endpoint, triangle->b, triangle->c)
                         : bounded_orient3(point, endpoint, triangle->b,
                                           triangle->c, radius),
          radius == T(0) ? orient3(point, endpoint, triangle->c, triangle->a)
                         : bounded_orient3(point, endpoint, triangle->c,
                                           triangle->a, radius)};
      if (std::any_of(edge_sides.begin(), edge_sides.end(),
                      [](int side) { return side == 2; }))
        return point_shell_result::ambiguous;
      if (std::any_of(edge_sides.begin(), edge_sides.end(),
                      [](int side) { return side == 0; })) {
        degenerate = true;
        break;
      }
      const bool positive = std::all_of(edge_sides.begin(), edge_sides.end(),
                                        [](int side) { return side > 0; });
      const bool negative = std::all_of(edge_sides.begin(), edge_sides.end(),
                                        [](int side) { return side < 0; });
      crossings += positive || negative;
    }
    if (degenerate)
      continue;
    const bool inside = (crossings & 1U) != 0;
    if (consensus && *consensus != inside)
      return point_shell_result::ambiguous;
    consensus = inside;
  }
  if (!consensus)
    return point_shell_result::ambiguous;
  return *consensus ? point_shell_result::inside : point_shell_result::outside;
}

} // namespace ygor::mesh_boolean::bounded::input_geometry_verifier
