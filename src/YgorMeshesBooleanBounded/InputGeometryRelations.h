#pragma once

#include "ExactFloatExpansion.h"
#include "FiniteInterval.h"
#include "InputValidationTypes.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T> using validation_point3 = std::array<T, 3>;
template <class T> struct validation_triangle final {
  validation_point3<T> a{}, b{}, c{};
  std::uint64_t facet = 0, local = 0, shell = 0;
  std::array<std::uint64_t, 3> vertices{};
};
template <class T> struct validation_aabb final {
  std::array<T, 3> low{}, high{};
};

inline int exact_sign(const exact_relation_record &r) noexcept {
  return r.status == exact_relation_status::exact_negative
             ? -1
             : r.status == exact_relation_status::exact_positive
                   ? 1
                   : r.status == exact_relation_status::exact_zero ? 0 : 2;
}
template <class T>
validation_aabb<T> triangle_bounds(const validation_triangle<T> &t,
                                   T inflate) {
  validation_aabb<T> b;
  for (std::size_t k = 0; k < 3; ++k) {
    const T low = std::min({t.a[k], t.b[k], t.c[k]});
    const T high = std::max({t.a[k], t.b[k], t.c[k]});
    const auto lower = directed_subtract(low, inflate);
    const auto upper = directed_add(high, inflate);
    b.low[k] = lower ? lower.value.lower : -std::numeric_limits<T>::max();
    b.high[k] = upper ? upper.value.upper : std::numeric_limits<T>::max();
  }
  return b;
}
template <class T>
bool bounds_overlap(const validation_aabb<T> &a, const validation_aabb<T> &b) {
  for (std::size_t k = 0; k < 3; ++k)
    if (a.high[k] < b.low[k] || b.high[k] < a.low[k])
      return false;
  return true;
}

namespace input_relation_detail {
template <class T>
std::optional<finite_interval<T>> coordinate_interval(T value, T radius) {
  if (!finite_bits(value) || !finite_bits(radius) || radius < T(0))
    return std::nullopt;
  const auto lower = directed_subtract(value, radius);
  const auto upper = directed_add(value, radius);
  if (!lower || !upper)
    return std::nullopt;
  return finite_interval<T>::create(lower.value.lower, upper.value.upper);
}

template <class T>
std::optional<finite_interval<T>>
bounded_orient3_interval(const validation_point3<T> &a,
                         const validation_point3<T> &b,
                         const validation_point3<T> &c,
                         const validation_point3<T> &d, T radius) {
  std::array<finite_interval<T>, 9> matrix;
  const std::array<validation_point3<T>, 3> rows{a, b, c};
  for (std::size_t row = 0; row < 3; ++row)
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto lhs = coordinate_interval(rows[row][axis], radius);
      const auto rhs = coordinate_interval(d[axis], radius);
      if (!lhs || !rhs)
        return std::nullopt;
      const auto difference = interval_subtract(*lhs, *rhs);
      if (!difference)
        return std::nullopt;
      matrix[row * 3 + axis] = *difference.value;
    }
  const auto minor = [&](std::size_t a0, std::size_t a1, std::size_t b0,
                         std::size_t b1) {
    const auto p = interval_multiply(matrix[a0], matrix[a1]);
    const auto q = interval_multiply(matrix[b0], matrix[b1]);
    return p && q ? interval_subtract(*p.value, *q.value)
                  : interval_operation_result<T>{numeric_status::invalid_interval,
                                                 std::nullopt};
  };
  const auto m0 = minor(4, 8, 5, 7);
  const auto m1 = minor(3, 8, 5, 6);
  const auto m2 = minor(3, 7, 4, 6);
  if (!m0 || !m1 || !m2)
    return std::nullopt;
  const auto t0 = interval_multiply(matrix[0], *m0.value);
  const auto t1 = interval_multiply(matrix[1], *m1.value);
  const auto t2 = interval_multiply(matrix[2], *m2.value);
  if (!t0 || !t1 || !t2)
    return std::nullopt;
  const auto difference = interval_subtract(*t0.value, *t1.value);
  if (!difference)
    return std::nullopt;
  const auto determinant = interval_add(*difference.value, *t2.value);
  if (!determinant)
    return std::nullopt;
  return determinant.value;
}

template <class T>
int bounded_orient3(const validation_point3<T> &a,
                    const validation_point3<T> &b,
                    const validation_point3<T> &c,
                    const validation_point3<T> &d, T radius) {
  const auto determinant = bounded_orient3_interval(a, b, c, d, radius);
  if (!determinant)
    return 2;
  if (determinant->lower() > T(0))
    return 1;
  if (determinant->upper() < T(0))
    return -1;
  return 0;
}

template <class T>
int bounded_polygon_area_sign(const std::vector<std::array<T, 2>> &polygon,
                              T radius) {
  if (polygon.size() < 3)
    return 2;
  auto total = finite_interval<T>::checked_singleton(T(0));
  if (!total)
    return 2;
  for (std::size_t i = 1; i + 1 < polygon.size(); ++i) {
    std::array<finite_interval<T>, 4> differences;
    for (std::size_t axis = 0; axis < 2; ++axis) {
      const auto origin = coordinate_interval(polygon[0][axis], radius);
      const auto first = coordinate_interval(polygon[i][axis], radius);
      const auto second = coordinate_interval(polygon[i + 1][axis], radius);
      if (!origin || !first || !second)
        return 2;
      const auto u = interval_subtract(*first, *origin);
      const auto v = interval_subtract(*second, *origin);
      if (!u || !v)
        return 2;
      differences[axis] = *u.value;
      differences[axis + 2] = *v.value;
    }
    const auto positive = interval_multiply(differences[0], differences[3]);
    const auto negative = interval_multiply(differences[1], differences[2]);
    if (!positive || !negative)
      return 2;
    const auto determinant = interval_subtract(*positive.value, *negative.value);
    if (!determinant)
      return 2;
    const auto sum = interval_add(*total, *determinant.value);
    if (!sum)
      return 2;
    total = sum.value;
  }
  if (total->lower() > T(0))
    return 1;
  if (total->upper() < T(0))
    return -1;
  return 0;
}

template <class T>
std::array<T, 2> project(const validation_point3<T> &p, std::uint8_t axis) {
  return axis == 0 ? std::array<T, 2>{p[1], p[2]}
                   : axis == 1 ? std::array<T, 2>{p[0], p[2]}
                               : std::array<T, 2>{p[0], p[1]};
}
template <class T>
int orient2(const std::array<T, 2> &a, const std::array<T, 2> &b,
            const std::array<T, 2> &c) {
  return exact_sign(exact_orient_2d(a, b, c));
}
template <class T>
int orient3(const validation_point3<T> &a, const validation_point3<T> &b,
            const validation_point3<T> &c, const validation_point3<T> &d) {
  return exact_sign(exact_orient_3d(a, b, c, d));
}
template <class T> bool between(T a, T b, T x) {
  const auto lower = exact_scalar_comparison(x, std::min(a, b));
  const auto upper = exact_scalar_comparison(x, std::max(a, b));
  return exact_sign(lower) >= 0 && exact_sign(lower) != 2 &&
         exact_sign(upper) <= 0;
}
template <class T>
bool point_in_triangle_2d(const std::array<T, 2> &p, const std::array<T, 2> &a,
                          const std::array<T, 2> &b, const std::array<T, 2> &c,
                          bool strict = false) {
  auto x = orient2(a, b, p), y = orient2(b, c, p), z = orient2(c, a, p);
  if (x == 2 || y == 2 || z == 2)
    return false;
  bool positive = x >= 0 && y >= 0 && z >= 0,
       negative = x <= 0 && y <= 0 && z <= 0;
  if (strict)
    return (positive || negative) && x != 0 && y != 0 && z != 0;
  return positive || negative;
}
template <class T>
int segment_relation_2d(const std::array<T, 2> &a, const std::array<T, 2> &b,
                        const std::array<T, 2> &c, const std::array<T, 2> &d) {
  auto x = orient2(a, b, c), y = orient2(a, b, d), z = orient2(c, d, a),
       w = orient2(c, d, b);
  if (x == 2 || y == 2 || z == 2 || w == 2)
    return 4;
  if (x * y < 0 && z * w < 0)
    return 3;
  if (x == 0 && y == 0 && z == 0 && w == 0) {
    const std::size_t axis = exact_sign(exact_scalar_comparison(a[0], b[0])) != 0
                                 ? 0
                                 : 1;
    auto lo = std::max(std::min(a[axis], b[axis]), std::min(c[axis], d[axis])),
         hi = std::min(std::max(a[axis], b[axis]), std::max(c[axis], d[axis]));
    return lo < hi ? 2 : lo == hi ? 1 : 0;
  }
  if ((x == 0 && between(a[0], b[0], c[0]) && between(a[1], b[1], c[1])) ||
      (y == 0 && between(a[0], b[0], d[0]) && between(a[1], b[1], d[1])) ||
      (z == 0 && between(c[0], d[0], a[0]) && between(c[1], d[1], a[1])) ||
      (w == 0 && between(c[0], d[0], b[0]) && between(c[1], d[1], b[1])))
    return 1;
  return 0;
}
template <class T>
std::uint8_t projection_axis(const validation_triangle<T> &t) {
  for (std::uint8_t axis = 0; axis < 3; ++axis)
    if (orient2(project(t.a, axis), project(t.b, axis), project(t.c, axis)) != 0)
      return axis;
  return 3;
}
template <class T>
bool same_patch(const validation_triangle<T> &a,
                 const validation_triangle<T> &b) {
  std::array<validation_point3<T>, 3> x{a.a, a.b, a.c}, y{b.a, b.b, b.c};
  auto less = [](const auto &p, const auto &q) {
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const int sign = exact_sign(exact_scalar_comparison(p[axis], q[axis]));
      if (sign != 0)
        return sign < 0;
    }
    return false;
  };
  std::sort(x.begin(), x.end(), less);
  std::sort(y.begin(), y.end(), less);
  for (std::size_t i = 0; i < 3; ++i)
    for (std::size_t axis = 0; axis < 3; ++axis)
      if (exact_sign(exact_scalar_comparison(x[i][axis], y[i][axis])) != 0)
        return false;
  return true;
}
template <class T>
validation_relation coplanar(const validation_triangle<T> &a,
                             const validation_triangle<T> &b) {
  if (same_patch(a, b))
    return validation_relation::whole_patch_coincidence;
  auto axis = projection_axis(a);
  if (axis > 2)
    return validation_relation::uncertain;
  std::array<std::array<T, 2>, 3> x{project(a.a, axis), project(a.b, axis),
                                    project(a.c, axis)},
      y{project(b.a, axis), project(b.b, axis), project(b.c, axis)};
  int dimension = 0;
  for (std::size_t i = 0; i < 3; ++i)
    for (std::size_t j = 0; j < 3; ++j) {
      auto r = segment_relation_2d(x[i], x[(i + 1) % 3], y[j], y[(j + 1) % 3]);
      if (r == 4)
        return validation_relation::uncertain;
      if (r == 3)
        return validation_relation::coplanar_positive_area_overlap;
      dimension = std::max(dimension, r);
    }
  for (const auto &p : x)
    if (point_in_triangle_2d(p, y[0], y[1], y[2], true))
      return validation_relation::coplanar_positive_area_overlap;
  for (const auto &p : y)
    if (point_in_triangle_2d(p, x[0], x[1], x[2], true))
      return validation_relation::coplanar_positive_area_overlap;
  return dimension == 2
             ? validation_relation::edge_contact
             : dimension == 1 ? validation_relation::point_contact
                              : validation_relation::definitely_disjoint;
}
template <class T> bool has_both_sides(const std::array<int, 3> &s) {
  bool p = false, n = false;
  for (auto x : s) {
    p |= x > 0 && x != 2;
    n |= x < 0;
  }
  return p && n;
}
enum class segment_triangle_hit : std::uint8_t {
  none,
  boundary,
  proper,
  coplanar,
  uncertain
};

template <class T>
segment_triangle_hit segment_triangle(const validation_point3<T> &p,
                                      const validation_point3<T> &q,
                                      const validation_triangle<T> &t) {
  const int pside = orient3(t.a, t.b, t.c, p);
  const int qside = orient3(t.a, t.b, t.c, q);
  if (pside == 2 || qside == 2)
    return segment_triangle_hit::uncertain;
  if (pside == 0 && qside == 0)
    return segment_triangle_hit::coplanar;
  if ((pside > 0 && qside > 0) || (pside < 0 && qside < 0))
    return segment_triangle_hit::none;
  if (pside == 0 || qside == 0) {
    const auto &endpoint = pside == 0 ? p : q;
    const auto axis = projection_axis(t);
    if (axis > 2)
      return segment_triangle_hit::uncertain;
    return point_in_triangle_2d(project(endpoint, axis), project(t.a, axis),
                                project(t.b, axis), project(t.c, axis))
               ? segment_triangle_hit::boundary
               : segment_triangle_hit::none;
  }
  const std::array<int, 3> edge_signs{
      orient3(p, q, t.a, t.b), orient3(p, q, t.b, t.c),
      orient3(p, q, t.c, t.a)};
  if (std::any_of(edge_signs.begin(), edge_signs.end(),
                  [](int sign) { return sign == 2; }))
    return segment_triangle_hit::uncertain;
  const bool nonnegative =
      std::all_of(edge_signs.begin(), edge_signs.end(),
                  [](int sign) { return sign >= 0; });
  const bool nonpositive =
      std::all_of(edge_signs.begin(), edge_signs.end(),
                  [](int sign) { return sign <= 0; });
  if (!nonnegative && !nonpositive)
    return segment_triangle_hit::none;
  return std::any_of(edge_signs.begin(), edge_signs.end(),
                     [](int sign) { return sign == 0; })
             ? segment_triangle_hit::boundary
             : segment_triangle_hit::proper;
}
} // namespace input_relation_detail

template <class T>
validation_relation
classify_triangle_relation(const validation_triangle<T> &a,
                           const validation_triangle<T> &b) {
  using namespace input_relation_detail;
  std::array<int, 3> sa{orient3(a.a, a.b, a.c, b.a),
                        orient3(a.a, a.b, a.c, b.b),
                        orient3(a.a, a.b, a.c, b.c)},
      sb{orient3(b.a, b.b, b.c, a.a), orient3(b.a, b.b, b.c, a.b),
         orient3(b.a, b.b, b.c, a.c)};
  for (auto x : sa)
    if (x == 2)
      return validation_relation::uncertain;
  for (auto x : sb)
    if (x == 2)
      return validation_relation::uncertain;
  auto one_side = [](const auto &s) {
    return (s[0] > 0 && s[1] > 0 && s[2] > 0) ||
           (s[0] < 0 && s[1] < 0 && s[2] < 0);
  };
  if (one_side(sa) || one_side(sb))
    return validation_relation::definitely_disjoint;
  bool ca = sa[0] == 0 && sa[1] == 0 && sa[2] == 0,
       cb = sb[0] == 0 && sb[1] == 0 && sb[2] == 0;
  if (ca && cb)
    return coplanar(a, b);
  std::array<validation_point3<T>, 3> av{a.a, a.b, a.c}, bv{b.a, b.b, b.c};
  std::size_t common_vertices = 0;
  for (const auto &x : av)
    for (const auto &y : bv) {
      bool equal = true;
      for (std::size_t axis = 0; axis < 3; ++axis)
        equal &= exact_sign(exact_scalar_comparison(x[axis], y[axis])) == 0;
      common_vertices += equal;
    }
  std::size_t proper = 0, boundary = 0;
  for (std::size_t i = 0; i < 3; ++i) {
    const auto ah = segment_triangle(av[i], av[(i + 1) % 3], b);
    const auto bh = segment_triangle(bv[i], bv[(i + 1) % 3], a);
    if (ah == segment_triangle_hit::uncertain ||
        bh == segment_triangle_hit::uncertain)
      return validation_relation::uncertain;
    proper += ah == segment_triangle_hit::proper;
    proper += bh == segment_triangle_hit::proper;
    boundary += ah == segment_triangle_hit::boundary;
    boundary += bh == segment_triangle_hit::boundary;
  }
  if (proper)
    return validation_relation::transverse_intersection;
  if (common_vertices >= 2)
    return validation_relation::edge_contact;
  if (boundary || common_vertices == 1)
    return validation_relation::point_contact;
  return validation_relation::definitely_disjoint;
}
template <class T>
bool definitely_separated_under_uncertainty(const validation_triangle<T> &a,
                                             const validation_triangle<T> &b,
                                             T radius) {
  if (!finite_bits(radius) || radius < T(0))
    return false;
  auto plane_separates = [&](const auto &plane, const auto &other) {
    const std::array<validation_point3<T>, 3> vertices{other.a, other.b,
                                                       other.c};
    int side = 0;
    for (const auto &vertex : vertices) {
      const int current = input_relation_detail::bounded_orient3(
          plane.a, plane.b, plane.c, vertex, radius);
      if (current == 0 || current == 2 || (side && side != current))
        return false;
      side = current;
    }
    return true;
  };
  return plane_separates(a, b) || plane_separates(b, a);
}

enum class point_shell_result : std::uint8_t {
  outside = 0,
  inside = 1,
  boundary = 2,
  ambiguous = 3
};
template <class T>
point_shell_result classify_point_against_triangles(
    const validation_point3<T> &point,
    const std::vector<validation_triangle<T>> &triangles,
    T uncertainty_radius = T(0)) {
  if (triangles.empty())
    return point_shell_result::outside;
  std::array<T, 3> low{{point[0], point[1], point[2]}}, high = low;
  for (const auto &triangle : triangles)
    for (const auto &vertex : {triangle.a, triangle.b, triangle.c})
      for (std::size_t axis = 0; axis < 3; ++axis) {
        low[axis] = std::min(low[axis], vertex[axis]);
        high[axis] = std::max(high[axis], vertex[axis]);
      }
  std::array<validation_point3<T>, 6> endpoints{};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto span = directed_subtract(high[axis], low[axis]);
    if (!span)
      continue;
    const T margin = std::max(T(1), span.value.upper);
    endpoints[axis * 2] = point;
    endpoints[axis * 2 + 1] = point;
    const auto upper = directed_add(high[axis], margin);
    const auto lower = directed_subtract(low[axis], margin);
    endpoints[axis * 2][axis] =
        upper ? upper.value.upper : std::numeric_limits<T>::infinity();
    endpoints[axis * 2 + 1][axis] =
        lower ? lower.value.lower : -std::numeric_limits<T>::infinity();
  }
  for (const auto &endpoint : endpoints) {
    bool finite = endpoint != point;
    for (auto coordinate : endpoint)
      finite &= finite_bits(coordinate);
    if (!finite)
      continue;
    std::uint64_t count = 0;
    bool retry = false;
    for (const auto &triangle : triangles) {
       const int source_side = uncertainty_radius == T(0)
                                   ? input_relation_detail::orient3(
                                         triangle.a, triangle.b, triangle.c,
                                         point)
                                   : input_relation_detail::bounded_orient3(
                                         triangle.a, triangle.b, triangle.c,
                                         point, uncertainty_radius);
       const int endpoint_side = uncertainty_radius == T(0)
                                     ? input_relation_detail::orient3(
                                           triangle.a, triangle.b, triangle.c,
                                           endpoint)
                                     : input_relation_detail::bounded_orient3(
                                           triangle.a, triangle.b, triangle.c,
                                           endpoint, uncertainty_radius);
      if (source_side == 2 || endpoint_side == 2)
        return point_shell_result::ambiguous;
      if (source_side == 0) {
        const auto axis = input_relation_detail::projection_axis(triangle);
        if (input_relation_detail::point_in_triangle_2d(
                input_relation_detail::project(point, axis),
                input_relation_detail::project(triangle.a, axis),
                input_relation_detail::project(triangle.b, axis),
                input_relation_detail::project(triangle.c, axis)))
          return point_shell_result::boundary;
      }
      if (source_side == 0 || endpoint_side == 0 ||
          source_side == endpoint_side)
        continue;
       const auto ray_side = [&](const auto &a, const auto &b) {
         return uncertainty_radius == T(0)
                    ? input_relation_detail::orient3(point, endpoint, a, b)
                    : input_relation_detail::bounded_orient3(
                          point, endpoint, a, b, uncertainty_radius);
       };
       const int e0 = ray_side(triangle.a, triangle.b);
       const int e1 = ray_side(triangle.b, triangle.c);
       const int e2 = ray_side(triangle.c, triangle.a);
      if (e0 == 2 || e1 == 2 || e2 == 2)
        return point_shell_result::ambiguous;
      if (e0 == 0 || e1 == 0 || e2 == 0) {
        retry = true;
        break;
      }
      if ((e0 > 0 && e1 > 0 && e2 > 0) || (e0 < 0 && e1 < 0 && e2 < 0))
        ++count;
    }
    if (!retry)
      return (count & 1) ? point_shell_result::inside
                         : point_shell_result::outside;
  }
  return point_shell_result::ambiguous;
}

} // namespace ygor::mesh_boolean::bounded
