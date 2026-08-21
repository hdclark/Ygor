#pragma once

#include "BoundedSourcePolygonKernel.h"
#include "CanonicalHalfedgeOperand.h"
#include "ClassificationTypes.h"
#include "ExactFloatExpansion.h"

#include <array>
#include <cstdint>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

// Canonical primitive integer directions for bounded shell queries. Each
// direction is a nonzero triple of -1/0/1 with the first nonzero component
// positive (so opposite directions are never both present). Directions are
// ordered by squared length, then lexicographically.
struct primitive_direction final {
  std::int8_t x = 0;
  std::int8_t y = 0;
  std::int8_t z = 0;

  friend bool operator==(const primitive_direction &a,
                         const primitive_direction &b) noexcept {
    return std::tie(a.x, a.y, a.z) == std::tie(b.x, b.y, b.z);
  }
  friend bool operator!=(const primitive_direction &a,
                         const primitive_direction &b) noexcept {
    return !(a == b);
  }
  friend bool operator<(const primitive_direction &a,
                        const primitive_direction &b) noexcept {
    return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
  }
};

inline const std::array<primitive_direction, 13> &
canonical_primitive_directions() noexcept;

// A bounded witness point for shell queries: nominal coordinates with a
// conservative finite enclosure inherited from the producing construction.
template <class T> struct shell_query_point final {
  std::array<T, 3> nominal{};
  std::array<finite_interval<T>, 3> enclosure{};
  std::uint64_t provenance_ordinal = 0;
};

enum class shell_query_disposition : std::uint8_t {
  definite = 1,
  boundary_touched = 2,
  origin_on_plane = 3,
  parallel_ambiguous = 4,
  non_finite = 5,
};

enum class shell_hit_kind : std::uint8_t {
  proper = 1,
  boundary_edge = 2,
  boundary_vertex = 3,
  tangent = 4,
  coplanar = 5,
  behind = 6,
};

template <class T> struct shell_query_hit final {
  std::uint64_t triangle = 0;
  std::uint64_t shell = 0;
  shell_hit_kind kind = shell_hit_kind::behind;
  std::int32_t contribution = 0;
};

template <class T> struct shell_query_result final {
  shell_query_disposition disposition = shell_query_disposition::definite;
  std::int64_t total_winding = 0;
  // Sorted sparse (shell ordinal, winding contribution) pairs.
  std::vector<std::pair<std::uint64_t, std::int64_t>> shell_winding;
  std::vector<shell_query_hit<T>> hits;
};

namespace shell_query_detail {

// Two-axis frame derived from the direction's dominant axis. Returns the two
// dropped axes in the fixed cyclic order so the projected orientation agrees
// with the dominant axis normal component.
inline std::pair<int, int> projected_axes(int dominant) noexcept {
  // dominant axis k; the two others are (k+1)%3 and (k+2)%3. This preserves
  // the right-handedness of the world frame for a positive dominant direction.
  return {(dominant + 1) % 3, (dominant + 2) % 3};
}

template <class T>
bool exact_dominant_direction_sign(const std::array<T, 3> &a,
                                   const std::array<T, 3> &b,
                                   const std::array<T, 3> &c,
                                   const primitive_direction &direction,
                                   std::int8_t &out) noexcept {
  // den = dot(cross(b-a, c-a), direction), computed as a 3x3 determinant whose
  // columns are (b-a), (c-a), direction.
  const std::array<T, 9> matrix{{
      b[0] - a[0], c[0] - a[0], static_cast<T>(direction.x),
      b[1] - a[1], c[1] - a[1], static_cast<T>(direction.y),
      b[2] - a[2], c[2] - a[2], static_cast<T>(direction.z),
  }};
  const auto record = exact_determinant_3x3(matrix);
  switch (record.status) {
  case exact_relation_status::exact_negative:
    out = -1;
    return true;
  case exact_relation_status::exact_positive:
    out = 1;
    return true;
  case exact_relation_status::exact_zero:
    out = 0;
    return true;
  default:
    return false;
  }
}

template <class T>
std::int8_t point_in_triangle_parity(const std::array<T, 2> &q,
                                     const std::array<T, 2> &a,
                                     const std::array<T, 2> &b,
                                     const std::array<T, 2> &c,
                                     bool &on_boundary, bool &uncertain) {
  const std::array<std::array<T, 2>, 3> vertices{{a, b, c}};
  on_boundary = false;
  uncertain = false;
  bool inside = false;
  for (std::size_t i = 0; i < 3; ++i) {
    const auto &p = vertices[i];
    const auto &r = vertices[(i + 1) % 3];
    // Half-open +u sweep exactly as Component 07 source-facet classification.
    const auto p_v = p[1];
    const auto r_v = r[1];
    const bool p_below_or_equal = p_v <= q[1];
    const bool r_below_or_equal = r_v <= q[1];
    const bool p_strict_above = p_v > q[1];
    const bool r_strict_above = r_v > q[1];
    const bool upward = p_below_or_equal && r_strict_above;
    const bool downward = r_below_or_equal && p_strict_above;
    if (!upward && !downward)
      continue;
    const auto orient = exact_orient_2d(p, r, q);
    if (orient.status == exact_relation_status::exact_zero) {
      // Collinear with the edge's supporting line. If within the edge box, the
      // point lies on the boundary; otherwise it does not cross.
      const T low_u = p[0] < r[0] ? p[0] : r[0];
      const T high_u = p[0] < r[0] ? r[0] : p[0];
      if (q[0] >= low_u && q[0] <= high_u) {
        on_boundary = true;
      }
      continue;
    }
    if (orient.status != exact_relation_status::exact_negative &&
        orient.status != exact_relation_status::exact_positive) {
      uncertain = true;
      continue;
    }
    const int sign = orient.status == exact_relation_status::exact_positive ? 1
                                                                             : -1;
    const bool crosses = (upward && sign > 0) || (downward && sign < 0);
    if (crosses)
      inside = !inside;
  }
  return inside ? 1 : 0;
}

} // namespace shell_query_detail

// Evaluate one bounded shell query: cast a ray from `origin` along `direction`
// through the opposite operand's triangles and accumulate the signed winding
// per source shell. The half-open sweep rule assigns shared edges and vertices
// exactly once. Exact ties are reported as `boundary_touched` so the caller may
// retry a different canonical direction rather than guess.
template <class T, class I>
shell_query_result<T> bounded_shell_query(
    const canonical_halfedge_operand<T, I> &opposite,
    const shell_query_point<T> &origin, const primitive_direction &direction) {
  shell_query_result<T> result;
  int dominant = 0;
  {
    int magnitude = direction.x != 0 ? (direction.x < 0 ? -direction.x
                                                        : direction.x)
                                     : 0;
    for (int axis = 1; axis < 3; ++axis) {
      const std::int8_t component =
          axis == 1 ? direction.y : direction.z;
      const int m = component < 0 ? -component : component;
      if (m > magnitude) {
        magnitude = m;
        dominant = axis;
      }
    }
    if (magnitude == 0) {
      result.disposition = shell_query_disposition::non_finite;
      return result;
    }
  }
  const auto axes = shell_query_detail::projected_axes(dominant);
  const int u = axes.first;
  const int v = axes.second;

  const std::array<T, 2> q{{origin.nominal[u], origin.nominal[v]}};
  bool boundary_touched = false;
  bool uncertain = false;

  const auto &triangles = opposite.triangles();
  const auto &vertices = opposite.vertices();
  for (const auto &triangle : triangles) {
    if (triangle.vertices[0] >= vertices.size() ||
        triangle.vertices[1] >= vertices.size() ||
        triangle.vertices[2] >= vertices.size()) {
      result.disposition = shell_query_disposition::non_finite;
      return result;
    }
    const auto &pa = vertices[triangle.vertices[0]].committed_point;
    const auto &pb = vertices[triangle.vertices[1]].committed_point;
    const auto &pc = vertices[triangle.vertices[2]].committed_point;

    // Depth/normal test using exact 3D determinants.
    std::int8_t den_sign = 0;
    if (!shell_query_detail::exact_dominant_direction_sign(
            pa, pb, pc, direction, den_sign)) {
      result.disposition = shell_query_disposition::non_finite;
      return result;
    }
    const auto orient3 = exact_orient_3d(pa, pb, pc, origin.nominal);
    if (orient3.status != exact_relation_status::exact_negative &&
        orient3.status != exact_relation_status::exact_positive &&
        orient3.status != exact_relation_status::exact_zero) {
      result.disposition = shell_query_disposition::non_finite;
      return result;
    }
    if (den_sign == 0) {
      // Ray parallel to the triangle plane.
      if (orient3.status == exact_relation_status::exact_zero) {
        // Ray lies in the triangle plane: coplanar, retry.
        result.hits.push_back({triangle.canonical_id, triangle.shell,
                               shell_hit_kind::coplanar, 0});
        result.disposition = shell_query_disposition::origin_on_plane;
        return result;
      }
      continue;
    }
    if (orient3.status == exact_relation_status::exact_zero) {
      // The origin lies exactly on the triangle's supporting plane (the ray
      // only touches that plane at t = 0). It is a boundary hit only when the
      // origin projects inside the triangle; otherwise the ray crosses the
      // plane outside the triangle and contributes nothing.
      const std::array<T, 2> a2{{pa[u], pa[v]}};
      const std::array<T, 2> b2{{pb[u], pb[v]}};
      const std::array<T, 2> c2{{pc[u], pc[v]}};
      bool on_boundary = false;
      bool triangle_uncertain = false;
      const int inside = shell_query_detail::point_in_triangle_parity(
          q, a2, b2, c2, on_boundary, triangle_uncertain);
      if (triangle_uncertain) {
        uncertain = true;
        continue;
      }
      if (inside || on_boundary) {
        boundary_touched = true;
        result.hits.push_back({triangle.canonical_id, triangle.shell,
                               shell_hit_kind::tangent, 0});
      }
      continue;
    }
    // num = dot(normal, a - origin) = -orient3(a,b,c,origin). t = num/den.
    const std::int8_t num_sign =
        orient3.status == exact_relation_status::exact_negative ? 1 : -1;
    const bool front = num_sign == den_sign;
    if (!front) {
      result.hits.push_back({triangle.canonical_id, triangle.shell,
                             shell_hit_kind::behind, 0});
      continue;
    }

    const std::array<T, 2> a2{{pa[u], pa[v]}};
    const std::array<T, 2> b2{{pb[u], pb[v]}};
    const std::array<T, 2> c2{{pc[u], pc[v]}};
    bool on_boundary = false;
    bool triangle_uncertain = false;
    const int inside = shell_query_detail::point_in_triangle_parity(
        q, a2, b2, c2, on_boundary, triangle_uncertain);
    if (triangle_uncertain) {
      uncertain = true;
      continue;
    }
    if (on_boundary) {
      boundary_touched = true;
      result.hits.push_back({triangle.canonical_id, triangle.shell,
                             shell_hit_kind::boundary_edge, 0});
      continue;
    }
    if (inside) {
      // Winding number contribution: a front-facing crossing with the surface
      // normal pointing against the ray direction enters the solid (+1).
      const std::int32_t contribution = -den_sign;
      result.total_winding += contribution;
      auto it = result.shell_winding.begin();
      while (it != result.shell_winding.end() &&
             it->first < triangle.shell)
        ++it;
      if (it != result.shell_winding.end() && it->first == triangle.shell)
        it->second += contribution;
      else
        result.shell_winding.insert(
            it, {triangle.shell, static_cast<std::int64_t>(contribution)});
      result.hits.push_back({triangle.canonical_id, triangle.shell,
                             shell_hit_kind::proper, contribution});
    }
  }

  if (uncertain) {
    result.disposition = shell_query_disposition::parallel_ambiguous;
    return result;
  }
  if (boundary_touched) {
    result.disposition = shell_query_disposition::boundary_touched;
    return result;
  }
  result.disposition = shell_query_disposition::definite;
  return result;
}

inline const std::array<primitive_direction, 13> &
canonical_primitive_directions() noexcept {
  static const std::array<primitive_direction, 13> directions{{
      {1, 0, 0},  {0, 1, 0},  {0, 0, 1},   {1, 1, 0},
      {1, -1, 0}, {1, 0, 1},  {1, 0, -1},  {0, 1, 1},
      {0, 1, -1}, {1, 1, 1},  {1, 1, -1},  {1, -1, 1},
      {1, -1, -1},
  }};
  return directions;
}

} // namespace ygor::mesh_boolean::bounded
