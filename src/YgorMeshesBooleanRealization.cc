#include "YgorMeshesBooleanRealization.h"
#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <tuple>

#if defined(__FAST_MATH__)
#error "Component 11 requires strict floating-point compilation"
#endif
#if defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__
#error "Component 11 must not assume finite-only arithmetic"
#endif

namespace ygor {
namespace mesh_boolean {
namespace {

template <class T, class I> std::uint64_t type_tag() {
  return realized_boundary_type_tag +
         (static_cast<std::uint64_t>(std::is_same<T, double>::value
                                         ? coordinate_tag::binary64
                                         : coordinate_tag::binary32)
          << 8) +
         static_cast<std::uint64_t>(std::is_same<I, std::uint64_t>::value
                                        ? index_tag::uint64
                                        : index_tag::uint32);
}

template <class Id> void ids(canonical_encoder &e, const std::vector<Id> &v) {
  e.u64(v.size());
  for (const auto x : v)
    e.id(x);
}

void point(canonical_encoder &e, const exact_point3 &p) {
  encode(e, p.x);
  encode(e, p.y);
  encode(e, p.z);
}

template <class T> void bits(canonical_encoder &e, coordinate_bits<T> b) {
  if constexpr (sizeof(T) == 4)
    e.u32(b.bits);
  else
    e.u64(b.bits);
}

digest exact_point_digest(const exact_point3 &p) {
  canonical_encoder e;
  point(e, p);
  return domain_digest({{'Y', 'G', 'B', 'P', 'N', 'T', '1', '1'}}, e.bytes());
}

std::vector<std::uint8_t> owner_free_vertex_key(const exact_point3 &p) {
  canonical_encoder e;
  const char tag[] = "YGBVTX11";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(1);
  point(e, p);
  return e.bytes();
}

digest realization_policy_digest(const realization_policy &p) {
  canonical_encoder e;
  e.u16(p.schema);
  e.u16(p.solver_version);
  e.byte(static_cast<std::uint8_t>(p.strategy));
  e.byte(static_cast<std::uint8_t>(p.original_coordinates));
  e.byte(static_cast<std::uint8_t>(p.topology));
  e.byte(static_cast<std::uint8_t>(p.pair_certification));
  e.byte(static_cast<std::uint8_t>(p.certificate_level));
  e.u32(p.neighboring_value_radius);
  return domain_digest({{'Y', 'G', 'B', 'P', 'O', 'L', '1', '1'}}, e.bytes());
}

template <class T, class I>
std::vector<std::uint8_t>
owner_free_triangle_key(const realized_boundary<T, I> &a,
                        const realization_triangle &triangle) {
  const auto &selected = *a.selected->payload;
  const auto &patch = selected.patches[triangle.patch.value_for_debug()];
  canonical_encoder e;
  const char tag[] = "YGBTRI11";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(1);
  e.byte(static_cast<std::uint8_t>(patch.orientation));
  e.u64(patch.cycles.size());
  for (auto cycle_id : patch.cycles) {
    const auto &cycle = selected.cycles[cycle_id.value_for_debug()];
    e.boolean(cycle.hole);
    e.u64(cycle.halfedges.size());
    for (auto halfedge_id : cycle.halfedges) {
      const auto vertex =
          selected.halfedges[halfedge_id.value_for_debug()].origin;
      e.byte_string(
          a.vertices[vertex.value_for_debug()].owner_free_semantic_key);
    }
  }
  for (auto vertex : triangle.vertices)
    e.byte_string(a.vertices[vertex.value_for_debug()].owner_free_semantic_key);
  e.byte(static_cast<std::uint8_t>(triangle.projection));
  for (auto halfedge : triangle.halfedges)
    e.byte(static_cast<std::uint8_t>(
        a.halfedges[halfedge.value_for_debug()].role));
  return e.bytes();
}

template <class T>
exact_point3 decoded_point(const std::array<coordinate_bits<T>, 3> &b) {
  auto x = decode_coordinate<T>(b[0], boolean_stage::geometry_realization);
  auto y = decode_coordinate<T>(b[1], boolean_stage::geometry_realization);
  auto z = decode_coordinate<T>(b[2], boolean_stage::geometry_realization);
  if (!x.has_value() || !y.has_value() || !z.has_value())
    throw std::logic_error("nonfinite realization coordinate");
  return {x.value().value, y.value().value, z.value().value};
}

template <class T>
bool same_bits(const std::array<coordinate_bits<T>, 3> &a,
               const std::array<coordinate_bits<T>, 3> &b) {
  return a[0].bits == b[0].bits && a[1].bits == b[1].bits &&
         a[2].bits == b[2].bits;
}

template <class T>
std::vector<realization_axis_candidate<T>>
axis_candidates(const exact_scalar &target, const realization_policy &policy,
                const std::optional<coordinate_bits<T>> &fixed) {
  std::vector<std::pair<coordinate_bits<T>, std::uint32_t>> raw;
  if (fixed) {
    raw.push_back({*fixed, 0});
  } else {
    const auto nearest = round_binary_nearest_even<T>(target);
    if (!nearest)
      return {};
    raw.push_back({*nearest, 0});
    if (policy.strategy == realization_strategy::neighboring_values) {
      auto p = *nearest;
      auto s = *nearest;
      for (std::uint32_t i = 0; i < policy.neighboring_value_radius; ++i) {
        const auto next = predecessor_bits<T>(p);
        if (!next)
          break;
        raw.push_back({*next, i + 1});
        p = *next;
      }
      for (std::uint32_t i = 0; i < policy.neighboring_value_radius; ++i) {
        const auto next = successor_bits<T>(s);
        if (!next)
          break;
        raw.push_back({*next, i + 1});
        s = *next;
      }
    }
  }
  std::sort(raw.begin(), raw.end(), [](const auto &a, const auto &b) {
    return a.first.bits < b.first.bits;
  });
  raw.erase(std::unique(raw.begin(), raw.end(),
                        [](const auto &a, const auto &b) {
                          return a.first.bits == b.first.bits;
                        }),
            raw.end());
  struct ranked {
    coordinate_bits<T> bits;
    exact_scalar error;
    std::uint32_t distance;
  };
  std::vector<ranked> ranked_values;
  for (const auto &entry : raw) {
    const auto b = entry.first;
    auto d = decode_coordinate<T>(b, boolean_stage::geometry_realization);
    if (!d.has_value())
      continue;
    ranked_values.push_back(
        {b, (d.value().value - target).abs(), entry.second});
  }
  std::sort(ranked_values.begin(), ranked_values.end(),
            [](const auto &a, const auto &b) {
              if (a.distance != b.distance)
                return a.distance < b.distance;
              if (a.error != b.error)
                return a.error < b.error;
              const auto av = decode_coordinate<T>(
                  a.bits, boolean_stage::geometry_realization);
              const auto bv = decode_coordinate<T>(
                  b.bits, boolean_stage::geometry_realization);
              if (av.value().value != bv.value().value)
                return av.value().value < bv.value().value;
              return a.bits.bits < b.bits.bits;
            });
  std::vector<realization_axis_candidate<T>> out;
  for (std::size_t i = 0; i < ranked_values.size(); ++i)
    out.push_back({candidate_value_id::from_canonical_value(i),
                   ranked_values[i].bits, ranked_values[i].distance,
                   static_cast<std::uint32_t>(i), ranked_values[i].error});
  return out;
}

template <class T> struct point_candidate {
  std::array<coordinate_bits<T>, 3> bits;
  std::array<std::uint32_t, 3> axis_rank;
  std::uint32_t max_step = 0, sum_step = 0;
  exact_scalar squared_error;
  std::uint64_t rank = 0;
};

template <class T>
std::vector<point_candidate<T>>
point_candidates(const realization_axis_domain<T> &x,
                 const realization_axis_domain<T> &y,
                 const realization_axis_domain<T> &z) {
  std::vector<point_candidate<T>> out;
  out.reserve(x.values.size() * y.values.size() * z.values.size());
  for (const auto &a : x.values)
    for (const auto &b : y.values)
      for (const auto &c : z.values) {
        point_candidate<T> p;
        p.bits = {{a.bits, b.bits, c.bits}};
        p.axis_rank = {{a.rank, b.rank, c.rank}};
        p.max_step =
            std::max({a.step_distance, b.step_distance, c.step_distance});
        p.sum_step = a.step_distance + b.step_distance + c.step_distance;
        p.squared_error = a.absolute_error.pow(2) + b.absolute_error.pow(2) +
                          c.absolute_error.pow(2);
        out.push_back(std::move(p));
      }
  std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) {
    if (a.max_step != b.max_step)
      return a.max_step < b.max_step;
    if (a.sum_step != b.sum_step)
      return a.sum_step < b.sum_step;
    if (a.squared_error != b.squared_error)
      return a.squared_error < b.squared_error;
    if (a.axis_rank != b.axis_rank)
      return a.axis_rank < b.axis_rank;
    return std::make_tuple(a.bits[0].bits, a.bits[1].bits, a.bits[2].bits) <
           std::make_tuple(b.bits[0].bits, b.bits[1].bits, b.bits[2].bits);
  });
  for (std::size_t i = 0; i < out.size(); ++i)
    out[i].rank = i;
  return out;
}

template <class T, class I>
std::vector<std::size_t> solver_vertex_order(const realized_boundary<T, I> &a) {
  std::vector<std::size_t> order(a.vertices.size()), degree(a.vertices.size());
  for (std::size_t i = 0; i < order.size(); ++i)
    order[i] = i;
  for (const auto &t : a.triangles)
    for (auto v : t.vertices)
      degree[v.value_for_debug()]++;
  for (const auto &e : a.selected->payload->edges) {
    degree[e.lower.value_for_debug()]++;
    degree[e.upper.value_for_debug()]++;
  }
  std::sort(order.begin(), order.end(), [&](auto x, auto y) {
    const auto xs = a.axis_domains[3 * x].values.size() *
                    a.axis_domains[3 * x + 1].values.size() *
                    a.axis_domains[3 * x + 2].values.size();
    const auto ys = a.axis_domains[3 * y].values.size() *
                    a.axis_domains[3 * y + 1].values.size() *
                    a.axis_domains[3 * y + 2].values.size();
    if (xs != ys)
      return xs < ys;
    if (degree[x] != degree[y])
      return degree[x] > degree[y];
    return x < y;
  });
  return order;
}

exact_scalar polygon_double_area(const std::vector<exact_point2> &ring) {
  exact_scalar area(0);
  for (std::size_t i = 0; i < ring.size(); ++i) {
    const auto &a = ring[i];
    const auto &b = ring[(i + 1) % ring.size()];
    area = area + a.x * b.y - a.y * b.x;
  }
  return area;
}

exact_sign polygon_sign(const std::vector<exact_point2> &ring) {
  return polygon_double_area(ring).sign();
}

bool point_in_triangle_or_boundary(const exact_point2 &p, const exact_point2 &a,
                                   const exact_point2 &b, const exact_point2 &c,
                                   exact_sign sign) {
  const auto x = orient2d(a, b, p), y = orient2d(b, c, p),
             z = orient2d(c, a, p);
  if (sign == exact_sign::positive)
    return x != exact_sign::negative && y != exact_sign::negative &&
           z != exact_sign::negative;
  return x != exact_sign::positive && y != exact_sign::positive &&
         z != exact_sign::positive;
}

struct triangulation_node {
  realization_vertex_id vertex;
  exact_point2 point;
};

struct triangulation_ring {
  std::vector<triangulation_node> nodes;
  bool hole = false;
};

struct patch_triangulation {
  std::vector<std::array<realization_vertex_id, 3>> triangles;
  std::set<std::pair<realization_vertex_id, realization_vertex_id>> bridges;
};

std::pair<realization_vertex_id, realization_vertex_id>
edge_key(realization_vertex_id a, realization_vertex_id b) {
  return std::minmax(a, b);
}

std::tuple<selected_patch_id, realization_vertex_id, realization_vertex_id>
bridge_key(selected_patch_id patch, realization_vertex_id a,
           realization_vertex_id b) {
  const auto edge = edge_key(a, b);
  return {patch, edge.first, edge.second};
}

bool endpoint_only_intersection(const exact_segment2 &candidate,
                                const exact_segment2 &edge) {
  const auto relation = relate_segments(candidate, edge);
  if (relation.dimension == intersection_dimension::empty)
    return true;
  if (relation.dimension != intersection_dimension::point || !relation.point)
    return false;
  const auto &p = *relation.point;
  return (p == candidate.origin || p == candidate.destination) &&
         (p == edge.origin || p == edge.destination);
}

bool visible_bridge(const triangulation_node &a, const triangulation_node &b,
                    const std::vector<triangulation_node> &boundary,
                    const std::vector<triangulation_ring> &rings) {
  if (a.point == b.point)
    return false;
  const exact_segment2 candidate{a.point, b.point};
  for (const auto &ring : rings)
    for (std::size_t i = 0; i < ring.nodes.size(); ++i)
      if (!endpoint_only_intersection(
              candidate, {ring.nodes[i].point,
                          ring.nodes[(i + 1) % ring.nodes.size()].point}))
        return false;
  for (std::size_t i = 0; i < boundary.size(); ++i)
    if (!endpoint_only_intersection(
            candidate,
            {boundary[i].point, boundary[(i + 1) % boundary.size()].point}))
      return false;

  const exact_point2 midpoint{(a.point.x + b.point.x) / exact_scalar(2),
                              (a.point.y + b.point.y) / exact_scalar(2)};
  auto outer = classify_point_polygon(midpoint, [&] {
    std::vector<exact_point2> p;
    for (const auto &n : rings.front().nodes)
      p.push_back(n.point);
    return p;
  }());
  if (!outer.has_value() ||
      outer.value().kind != point_region_kind::open_interior)
    return false;
  for (std::size_t i = 1; i < rings.size(); ++i) {
    std::vector<exact_point2> p;
    for (const auto &n : rings[i].nodes)
      p.push_back(n.point);
    auto hole = classify_point_polygon(midpoint, p);
    if (!hole.has_value() || hole.value().kind != point_region_kind::outside)
      return false;
  }
  return true;
}

bool valid_ear_diagonal(const std::vector<triangulation_node> &nodes,
                        std::size_t previous, std::size_t next) {
  const exact_segment2 diagonal{nodes[previous].point, nodes[next].point};
  if (diagonal.origin == diagonal.destination)
    return false;
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    const auto j = (i + 1) % nodes.size();
    if (i == previous || j == previous || i == next || j == next)
      continue;
    const exact_segment2 edge{nodes[i].point, nodes[j].point};
    const auto relation = relate_segments(diagonal, edge);
    if (relation.dimension == intersection_dimension::empty)
      continue;
    if (relation.dimension == intersection_dimension::segment &&
        ((diagonal.origin == edge.origin &&
          diagonal.destination == edge.destination) ||
         (diagonal.origin == edge.destination &&
          diagonal.destination == edge.origin)))
      continue;
    if (relation.dimension != intersection_dimension::point ||
        !relation.point ||
        !(*relation.point == diagonal.origin ||
          *relation.point == diagonal.destination) ||
        !(*relation.point == edge.origin ||
          *relation.point == edge.destination))
      return false;
  }
  return true;
}

status_or<std::vector<std::array<realization_vertex_id, 3>>>
clip_weakly_simple_ring(std::vector<triangulation_node> nodes,
                        exact_sign sign) {
  if (nodes.size() < 3 || sign == exact_sign::zero)
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::geometry_realization,
                      "invalid_selected_cycle");
  std::vector<std::array<realization_vertex_id, 3>> out;
  while (nodes.size() > 3) {
    std::optional<std::size_t> best;
    std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::size_t>
        best_key;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
      const auto p = (i + nodes.size() - 1) % nodes.size();
      const auto r = (i + 1) % nodes.size();
      if (nodes[p].vertex == nodes[i].vertex ||
          nodes[i].vertex == nodes[r].vertex ||
          nodes[p].vertex == nodes[r].vertex ||
          orient2d(nodes[p].point, nodes[i].point, nodes[r].point) != sign ||
          !valid_ear_diagonal(nodes, p, r))
        continue;
      bool contains = false;
      for (std::size_t n = 0; n < nodes.size(); ++n)
        if (n != p && n != i && n != r && !(nodes[n].point == nodes[p].point) &&
            !(nodes[n].point == nodes[i].point) &&
            !(nodes[n].point == nodes[r].point) &&
            point_in_triangle_or_boundary(nodes[n].point, nodes[p].point,
                                          nodes[i].point, nodes[r].point,
                                          sign)) {
          contains = true;
          break;
        }
      if (contains)
        continue;
      const auto key = std::make_tuple(nodes[p].vertex.value_for_debug(),
                                       nodes[i].vertex.value_for_debug(),
                                       nodes[r].vertex.value_for_debug(), i);
      if (!best || key < best_key) {
        best = i;
        best_key = key;
      }
    }
    if (!best)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::geometry_realization,
                        "non_simple_selected_cycle");
    const auto i = *best;
    const auto p = (i + nodes.size() - 1) % nodes.size();
    const auto r = (i + 1) % nodes.size();
    out.push_back({nodes[p].vertex, nodes[i].vertex, nodes[r].vertex});
    nodes.erase(nodes.begin() + static_cast<std::ptrdiff_t>(i));
  }
  if (nodes[0].vertex == nodes[1].vertex ||
      nodes[1].vertex == nodes[2].vertex ||
      nodes[0].vertex == nodes[2].vertex ||
      orient2d(nodes[0].point, nodes[1].point, nodes[2].point) != sign)
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::geometry_realization,
                      "degenerate_selected_cycle_tail");
  out.push_back({nodes[0].vertex, nodes[1].vertex, nodes[2].vertex});
  return out;
}

status_or<patch_triangulation>
triangulate_patch(std::vector<triangulation_ring> rings) {
  if (rings.empty() || rings.front().hole || rings.front().nodes.size() < 3)
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::geometry_realization,
                      "invalid_selected_patch_cycles");
  std::vector<exact_point2> outer_points;
  for (const auto &node : rings.front().nodes)
    outer_points.push_back(node.point);
  const auto sign = polygon_sign(outer_points);
  if (sign == exact_sign::zero)
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::geometry_realization,
                      "degenerate_selected_cycle");
  exact_scalar expected_area = polygon_double_area(outer_points);
  std::size_t expected_triangle_count = rings.front().nodes.size() - 2;
  std::map<realization_vertex_id, exact_point2> points_by_vertex;
  for (const auto &node : rings.front().nodes)
    points_by_vertex.emplace(node.vertex, node.point);
  for (std::size_t i = 1; i < rings.size(); ++i) {
    if (!rings[i].hole || rings[i].nodes.size() < 3)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::geometry_realization,
                        "invalid_selected_patch_cycles");
    std::vector<exact_point2> points;
    for (const auto &node : rings[i].nodes)
      points.push_back(node.point);
    if (polygon_sign(points) == exact_sign::zero ||
        polygon_sign(points) == sign)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::geometry_realization,
                        "invalid_selected_hole_orientation");
    expected_area = expected_area + polygon_double_area(points);
    expected_triangle_count += rings[i].nodes.size() + 2;
    for (const auto &node : rings[i].nodes)
      points_by_vertex.emplace(node.vertex, node.point);
  }

  patch_triangulation result;
  auto boundary = rings.front().nodes;
  std::vector<bool> bridged(rings.size());
  bridged.front() = true;
  for (std::size_t remaining = rings.size() - 1; remaining > 0; --remaining) {
    std::optional<std::tuple<std::size_t, std::size_t, std::size_t>> best;
    std::tuple<std::uint64_t, std::uint64_t, std::size_t, std::size_t,
               std::size_t>
        best_key;
    for (std::size_t h = 1; h < rings.size(); ++h) {
      if (bridged[h])
        continue;
      for (std::size_t hi = 0; hi < rings[h].nodes.size(); ++hi)
        for (std::size_t bi = 0; bi < boundary.size(); ++bi) {
          if (!visible_bridge(rings[h].nodes[hi], boundary[bi], boundary,
                              rings))
            continue;
          const auto key =
              std::make_tuple(rings[h].nodes[hi].vertex.value_for_debug(),
                              boundary[bi].vertex.value_for_debug(), h, hi, bi);
          if (!best || key < best_key) {
            best = std::make_tuple(h, hi, bi);
            best_key = key;
          }
        }
    }
    if (!best)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::geometry_realization,
                        "selected_hole_bridge_not_visible");
    const auto h = std::get<0>(*best), hi = std::get<1>(*best),
               bi = std::get<2>(*best);
    const auto hole_vertex = rings[h].nodes[hi];
    const auto boundary_vertex = boundary[bi];
    result.bridges.insert(edge_key(hole_vertex.vertex, boundary_vertex.vertex));
    std::vector<triangulation_node> merged;
    merged.reserve(boundary.size() + rings[h].nodes.size() + 2);
    merged.insert(merged.end(), boundary.begin(), boundary.begin() + bi + 1);
    for (std::size_t i = 0; i < rings[h].nodes.size(); ++i)
      merged.push_back(rings[h].nodes[(hi + i) % rings[h].nodes.size()]);
    merged.push_back(hole_vertex);
    merged.push_back(boundary_vertex);
    merged.insert(merged.end(), boundary.begin() + bi + 1, boundary.end());
    boundary = std::move(merged);
    bridged[h] = true;
  }
  auto triangles = clip_weakly_simple_ring(std::move(boundary), sign);
  if (!triangles.has_value())
    return triangles.error();
  exact_scalar actual_area(0);
  for (const auto &triangle : triangles.value()) {
    const auto &a = points_by_vertex.at(triangle[0]);
    const auto &b = points_by_vertex.at(triangle[1]);
    const auto &c = points_by_vertex.at(triangle[2]);
    actual_area = actual_area + a.x * b.y - a.y * b.x + b.x * c.y - b.y * c.x +
                  c.x * a.y - c.y * a.x;
  }
  if (triangles.value().size() != expected_triangle_count ||
      actual_area != expected_area)
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::geometry_realization,
                      "selected_triangulation_partition");
  result.triangles = std::move(triangles.value());
  return result;
}

template <class T, class I>
status_or<std::vector<triangulation_ring>>
selected_patch_rings(const selected_exact_boundary<T, I> &selected,
                     const realized_boundary<T, I> &realized,
                     const selected_patch &patch, projection_axis projection) {
  if (patch.cycles.empty())
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::geometry_realization,
                      "invalid_selected_patch_cycles");
  std::vector<triangulation_ring> rings;
  for (std::size_t i = 0; i < patch.cycles.size(); ++i) {
    const auto cycle_id = patch.cycles[i];
    if (cycle_id.value_for_debug() >= selected.cycles.size())
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::geometry_realization,
                        "selected_cycle_range");
    const auto &cycle = selected.cycles[cycle_id.value_for_debug()];
    if (cycle.patch != patch.id || cycle.hole != (i != 0))
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::geometry_realization,
                        "invalid_selected_patch_cycles");
    triangulation_ring ring;
    ring.hole = cycle.hole;
    for (const auto halfedge_id : cycle.halfedges) {
      if (halfedge_id.value_for_debug() >= selected.halfedges.size())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::geometry_realization,
                          "selected_halfedge_range");
      const auto selected_vertex =
          selected.halfedges[halfedge_id.value_for_debug()].origin;
      if (selected_vertex.value_for_debug() >= realized.vertices.size())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::geometry_realization,
                          "selected_vertex_range");
      const auto vertex = realization_vertex_id::from_canonical_value(
          selected_vertex.value_for_debug());
      ring.nodes.push_back(
          {vertex,
           project(realized.vertices[vertex.value_for_debug()].exact_coordinate,
                   projection)});
    }
    rings.push_back(std::move(ring));
  }
  return rings;
}

template <class T, class I>
std::vector<std::uint8_t> semantic(const realized_boundary<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBCAN11";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(realized_boundary_schema);
  e.raw(a.selected->payload->certificate.semantic_digest.bytes.data(), 16);
  e.raw(a.kernel_policy_digest.bytes.data(), 16);
  e.raw(a.policy_digest.bytes.data(), 16);
  e.u64(a.vertices.size());
  for (const auto &v : a.vertices) {
    e.id(v.id);
    e.id(v.selected);
    e.id(v.symbolic);
    point(e, v.exact_coordinate);
    e.raw(v.exact_digest.bytes.data(), 16);
    e.byte_string(v.owner_free_semantic_key);
    ids(e, v.derivations);
    ids(e, v.original_sources);
    e.boolean(bool(v.preserved_source));
    if (v.preserved_source)
      e.id(*v.preserved_source);
    e.boolean(bool(v.preserved_source_bits));
    if (v.preserved_source_bits)
      for (auto b : *v.preserved_source_bits)
        bits(e, b);
    for (auto b : v.accepted_bits)
      bits(e, b);
    for (auto r : v.accepted_axis_rank)
      e.u32(r);
    e.u64(v.accepted_point_rank);
    ids(e, v.selected_edges);
    ids(e, v.patches);
    ids(e, v.triangles);
    ids(e, v.obligations);
  }
  e.u64(a.axis_domains.size());
  for (const auto &d : a.axis_domains) {
    e.id(d.vertex);
    e.byte(d.axis);
    encode(e, d.target);
    e.u64(d.values.size());
    for (const auto &v : d.values) {
      e.id(v.id);
      bits(e, v.bits);
      e.u32(v.step_distance);
      e.u32(v.rank);
      encode(e, v.absolute_error);
    }
  }
  e.u64(a.triangles.size());
  for (const auto &t : a.triangles) {
    e.id(t.id);
    e.id(t.patch);
    e.byte_string(t.owner_free_semantic_key);
    for (auto x : t.vertices)
      e.id(x);
    for (auto x : t.halfedges)
      e.id(x);
    e.byte(static_cast<std::uint8_t>(t.projection));
    e.byte(static_cast<std::uint8_t>(t.exact_orientation));
  }
  e.u64(a.halfedges.size());
  for (const auto &h : a.halfedges) {
    e.id(h.id);
    e.id(h.triangle);
    e.id(h.origin);
    e.id(h.destination);
    e.id(h.next);
    e.id(h.previous);
    e.boolean(bool(h.twin));
    if (h.twin)
      e.id(*h.twin);
    e.byte(static_cast<std::uint8_t>(h.role));
    e.boolean(bool(h.selected_edge));
    if (h.selected_edge)
      e.id(*h.selected_edge);
  }
  e.u64(a.obligations.size());
  for (const auto &o : a.obligations) {
    e.id(o.id);
    e.byte(static_cast<std::uint8_t>(o.kind));
    e.u16(o.version);
    ids(e, o.vertices);
    ids(e, o.triangles);
    ids(e, o.selected_edges);
    ids(e, o.selected_patches);
    e.byte(static_cast<std::uint8_t>(o.expected));
    e.byte(static_cast<std::uint8_t>(o.actual));
    e.byte_string(o.witness);
  }
  e.raw(a.search.domain_digest.bytes.data(), 16);
  e.u64(a.search.visited_nodes);
  e.u64(a.search.complete_assignments);
  e.boolean(bool(a.search.accepted_assignment));
  if (a.search.accepted_assignment)
    e.id(*a.search.accepted_assignment);
  e.boolean(a.search.nearest_passed);
  e.boolean(a.search.exhausted);
  const auto &c = a.certificate;
  e.id(c.id);
  e.u16(c.triangulation_version);
  e.u16(c.obligation_version);
  e.u16(c.solver_version);
  e.u64(c.vertices);
  e.u64(c.triangles);
  e.u64(c.halfedges);
  e.u64(c.obligations);
  e.u64(c.witnesses);
  e.raw(a.selected->payload->certificate.semantic_digest.bytes.data(), 16);
  for (const auto *d :
       {&c.kernel_policy_digest, &c.policy_digest, &c.triangulation_digest,
        &c.obligation_digest, &c.assignment_digest})
    e.raw(d->bytes.data(), 16);
  return e.bytes();
}

template <class T, class I>
std::vector<std::uint8_t> invocation(const realized_boundary<T, I> &a) {
  canonical_encoder e;
  const char tag[] = "YGBREA11";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(realized_boundary_schema);
  e.raw(a.setup_digest.bytes.data(), 16);
  e.raw(a.selected_digest.bytes.data(), 16);
  e.raw(a.kernel_policy_digest.bytes.data(), 16);
  e.raw(a.policy_digest.bytes.data(), 16);
  e.byte_string(a.canonical_bytes);
  return e.bytes();
}

template <class T, class I>
digest artifact_digest_for(const realized_boundary<T, I> &a) {
  canonical_encoder e;
  e.raw(a.setup_digest.bytes.data(), 16);
  e.byte(static_cast<std::uint8_t>(artifact_slot::realized_boundary));
  e.byte_string(a.artifact_bytes);
  return domain_digest({{'Y', 'G', 'B', 'A', 'R', 'T', '0', '1'}}, e.bytes());
}

template <class T> exact_point3 accepted_point(const realization_vertex<T> &v) {
  return decoded_point(v.accepted_bits);
}

realization_relation relation_for(polygon_intersection_kind r) {
  if (r == polygon_intersection_kind::point)
    return realization_relation::point;
  if (r == polygon_intersection_kind::segment)
    return realization_relation::segment;
  return realization_relation::disjoint;
}

template <class T, class I>
bool assignment_valid(const realized_boundary<T, I> &a) {
  std::set<std::tuple<decltype(coordinate_bits<T>{}.bits),
                      decltype(coordinate_bits<T>{}.bits),
                      decltype(coordinate_bits<T>{}.bits)>>
      unique;
  std::vector<exact_point3> p;
  for (const auto &v : a.vertices) {
    if (!unique
             .emplace(v.accepted_bits[0].bits, v.accepted_bits[1].bits,
                      v.accepted_bits[2].bits)
             .second)
      return false;
    p.push_back(accepted_point(v));
  }
  for (const auto &t : a.triangles)
    if (orient2d(project(p[t.vertices[0].value_for_debug()], t.projection),
                 project(p[t.vertices[1].value_for_debug()], t.projection),
                 project(p[t.vertices[2].value_for_debug()], t.projection)) !=
        t.exact_orientation)
      return false;
  for (std::size_t i = 0; i < a.triangles.size(); ++i)
    for (std::size_t j = i + 1; j < a.triangles.size(); ++j) {
      const auto &x = a.triangles[i], &y = a.triangles[j];
      std::size_t shared = 0;
      for (auto xv : x.vertices)
        for (auto yv : y.vertices)
          shared += xv == yv;
      const auto relation =
          relate_triangles({p[x.vertices[0].value_for_debug()],
                            p[x.vertices[1].value_for_debug()],
                            p[x.vertices[2].value_for_debug()]},
                           {p[y.vertices[0].value_for_debug()],
                            p[y.vertices[1].value_for_debug()],
                            p[y.vertices[2].value_for_debug()]});
      if ((shared == 0 && relation != polygon_intersection_kind::disjoint) ||
          (shared == 1 && relation != polygon_intersection_kind::point) ||
          (shared == 2 && relation != polygon_intersection_kind::segment) ||
          shared > 2)
        return false;
    }
  return true;
}

template <class T, class I>
void append_obligations(realized_boundary<T, I> &a) {
  auto add = [&](realization_obligation o) {
    o.id =
        realization_obligation_id::from_canonical_value(a.obligations.size());
    for (auto v : o.vertices)
      a.vertices[v.value_for_debug()].obligations.push_back(o.id);
    o.witness = {static_cast<std::uint8_t>(o.actual)};
    a.obligations.push_back(std::move(o));
  };
  for (const auto &v : a.vertices) {
    add({{},
         realization_obligation_kind::finite_coordinate,
         1,
         {v.id},
         {},
         {},
         {},
         realization_relation::finite,
         realization_relation::finite,
         {}});
    if (v.preserved_source)
      add({{},
           realization_obligation_kind::fixed_original_bits,
           1,
           {v.id},
           {},
           {},
           {},
           realization_relation::equal_bits,
           realization_relation::equal_bits,
           {}});
  }
  for (std::size_t i = 0; i < a.vertices.size(); ++i)
    for (std::size_t j = i + 1; j < a.vertices.size(); ++j)
      add({{},
           realization_obligation_kind::distinct_vertices,
           1,
           {a.vertices[i].id, a.vertices[j].id},
           {},
           {},
           {},
           realization_relation::distinct,
           realization_relation::distinct,
           {}});
  for (const auto &t : a.triangles)
    add({{},
         realization_obligation_kind::triangle_orientation,
         1,
         {t.vertices[0], t.vertices[1], t.vertices[2]},
         {t.id},
         {},
         {t.patch},
         t.exact_orientation == exact_sign::positive
             ? realization_relation::positive
             : realization_relation::negative,
         t.exact_orientation == exact_sign::positive
             ? realization_relation::positive
             : realization_relation::negative,
         {}});
  for (const auto &edge : a.selected->payload->edges)
    add({{},
         realization_obligation_kind::selected_edge_order,
         1,
         {realization_vertex_id::from_canonical_value(
              edge.lower.value_for_debug()),
          realization_vertex_id::from_canonical_value(
              edge.upper.value_for_debug())},
         {},
         {edge.id},
         {},
         realization_relation::distinct,
         realization_relation::distinct,
         {}});
  const auto points = [&] {
    std::vector<exact_point3> p;
    for (const auto &v : a.vertices)
      p.push_back(accepted_point(v));
    return p;
  }();
  for (std::size_t i = 0; i < a.triangles.size(); ++i)
    for (std::size_t j = i + 1; j < a.triangles.size(); ++j) {
      const auto &x = a.triangles[i], &y = a.triangles[j];
      std::vector<realization_vertex_id> shared;
      for (auto xv : x.vertices)
        for (auto yv : y.vertices)
          if (xv == yv)
            shared.push_back(xv);
      const auto rel = relation_for(
          relate_triangles({points[x.vertices[0].value_for_debug()],
                            points[x.vertices[1].value_for_debug()],
                            points[x.vertices[2].value_for_debug()]},
                           {points[y.vertices[0].value_for_debug()],
                            points[y.vertices[1].value_for_debug()],
                            points[y.vertices[2].value_for_debug()]}));
      const auto expected =
          shared.empty() ? realization_relation::disjoint
                         : shared.size() == 1 ? realization_relation::point
                                              : realization_relation::segment;
      add({{},
           shared.empty()
               ? realization_obligation_kind::nonadjacent_disjointness
               : shared.size() == 1
                     ? realization_obligation_kind::shared_vertex_relation
                     : realization_obligation_kind::shared_edge_relation,
           1,
           shared,
           {x.id, y.id},
           {},
           {x.patch, y.patch},
           expected,
           rel,
           {}});
    }
  for (const auto &patch : a.selected->payload->patches)
    add({{},
         realization_obligation_kind::patch_embedding,
         1,
         {},
         {},
         {},
         {patch.id},
         realization_relation::embedded,
         realization_relation::embedded,
         {}});
  add({{},
       realization_obligation_kind::global_embedding,
       1,
       {},
       {},
       {},
       {},
       realization_relation::embedded,
       realization_relation::embedded,
       {}});
}

template <class T, class I>
bool valid(const realized_boundary<T, I> &a, const realization_policy &policy) {
  if (!a.selected || !a.symbolic || !a.constructions ||
      a.owner != a.selected->owner || a.owner != a.symbolic->owner ||
      a.selected->payload->arrangement->payload->symbolic.get() !=
          a.symbolic.get() ||
      a.selected_digest != a.selected->artifact_digest ||
      a.kernel_policy_digest != a.symbolic->payload->kernel_policy_digest ||
      a.policy_digest != realization_policy_digest(policy) ||
      a.constructions.get() != a.symbolic->payload->constructions.get())
    return false;
  const auto &s = *a.selected->payload;
  const auto &symbolic = *a.symbolic->payload;
  const auto &validated = *symbolic.validated->payload;
  if (a.constructions->owner != a.owner)
    return false;
  if (a.vertices.size() != s.vertices.size() ||
      a.axis_domains.size() != 3 * a.vertices.size() ||
      a.certificate.id.value_for_debug() != 0 ||
      a.certificate.vertices != a.vertices.size() ||
      a.certificate.triangles != a.triangles.size() ||
      a.certificate.halfedges != a.halfedges.size() ||
      a.certificate.obligations != a.obligations.size() ||
      a.certificate.triangulation_version != 1 ||
      a.certificate.obligation_version != 1 ||
      a.certificate.selected_digest != a.selected_digest ||
      a.certificate.kernel_policy_digest != a.kernel_policy_digest ||
      a.certificate.policy_digest != a.policy_digest ||
      a.certificate.solver_version != policy.solver_version)
    return false;
  for (std::size_t i = 0; i < a.vertices.size(); ++i) {
    const auto &v = a.vertices[i];
    if (v.id.value_for_debug() != i || v.selected.value_for_debug() != i ||
        s.vertices[i].symbolic != v.symbolic ||
        v.symbolic.value_for_debug() >= symbolic.vertices.size())
      return false;
    const auto &sv = symbolic.vertices[v.symbolic.value_for_debug()];
    if (!(v.exact_coordinate == sv.point) ||
        v.exact_digest != exact_point_digest(sv.point) ||
        v.owner_free_semantic_key != owner_free_vertex_key(sv.point) ||
        v.derivations != sv.constructions ||
        v.original_sources != sv.original_vertices)
      return false;
    if (!std::is_sorted(v.derivations.begin(), v.derivations.end()) ||
        std::adjacent_find(v.derivations.begin(), v.derivations.end()) !=
            v.derivations.end())
      return false;
    for (const auto derivation : v.derivations) {
      if (derivation.value_for_debug() >= a.constructions->nodes.size())
        return false;
      const auto &node = a.constructions->nodes[derivation.value_for_debug()];
      if (node.id != derivation ||
          node.kind != construction_kind::exact_relation ||
          node.exact_result.empty())
        return false;
    }
    std::optional<std::array<coordinate_bits<T>, 3>> preserved;
    if (!sv.original_vertices.empty()) {
      const auto oi = sv.original_vertices.front().value_for_debug();
      if (oi >= validated.vertices.size())
        return false;
      preserved = validated.vertices[oi].raw_bits;
      for (auto source : sv.original_vertices)
        if (source.value_for_debug() >= validated.vertices.size() ||
            !(validated.vertices[source.value_for_debug()].exact_coordinate ==
              sv.point))
          return false;
      if (v.preserved_source !=
              std::optional<original_vertex_id>(sv.original_vertices.front()) ||
          !v.preserved_source_bits ||
          !same_bits(*preserved, *v.preserved_source_bits))
        return false;
    } else if (v.preserved_source || v.preserved_source_bits)
      return false;
    const std::array<exact_scalar, 3> target{
        {sv.point.x, sv.point.y, sv.point.z}};
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto &d = a.axis_domains[3 * i + axis];
      auto expected = axis_candidates<T>(
          target[axis], policy,
          preserved ? std::optional<coordinate_bits<T>>((*preserved)[axis])
                    : std::nullopt);
      if (d.vertex != v.id || d.axis != axis || d.target != target[axis] ||
          d.values.size() != expected.size() || d.values.empty() ||
          v.accepted_axis_rank[axis] >= d.values.size() ||
          v.accepted_bits[axis].bits !=
              d.values[v.accepted_axis_rank[axis]].bits.bits)
        return false;
      for (std::size_t n = 0; n < expected.size(); ++n)
        if (d.values[n].id != expected[n].id ||
            d.values[n].bits.bits != expected[n].bits.bits ||
            d.values[n].rank != expected[n].rank ||
            d.values[n].step_distance != expected[n].step_distance ||
            d.values[n].absolute_error != expected[n].absolute_error)
          return false;
    }
    const auto candidates =
        point_candidates(a.axis_domains[3 * i], a.axis_domains[3 * i + 1],
                         a.axis_domains[3 * i + 2]);
    if (v.accepted_point_rank >= candidates.size() ||
        !same_bits(candidates[v.accepted_point_rank].bits, v.accepted_bits) ||
        candidates[v.accepted_point_rank].axis_rank != v.accepted_axis_rank ||
        bits_of(v.coordinate.x).bits != v.accepted_bits[0].bits ||
        bits_of(v.coordinate.y).bits != v.accepted_bits[1].bits ||
        bits_of(v.coordinate.z).bits != v.accepted_bits[2].bits)
      return false;
  }
  std::vector<unsigned> halfedge_use(a.halfedges.size());
  std::vector<std::tuple<selected_patch_id, projection_axis,
                         std::array<realization_vertex_id, 3>>>
      expected_triangles;
  std::set<std::tuple<selected_patch_id, realization_vertex_id,
                      realization_vertex_id>>
      expected_hole_bridges;
  for (const auto &patch : s.patches) {
    if (patch.source.value_for_debug() >=
        s.arrangement->payload->patches.size())
      return false;
    const auto projection = dominant_projection(
        s.arrangement->payload->patches[patch.source.value_for_debug()].plane);
    auto rings = selected_patch_rings(s, a, patch, projection);
    if (!rings.has_value())
      return false;
    auto triangulated = triangulate_patch(std::move(rings.value()));
    if (!triangulated.has_value())
      return false;
    for (const auto &bridge : triangulated.value().bridges)
      expected_hole_bridges.insert(
          bridge_key(patch.id, bridge.first, bridge.second));
    for (const auto &triangle : triangulated.value().triangles)
      expected_triangles.push_back({patch.id, projection, triangle});
  }
  if (expected_triangles.size() != a.triangles.size())
    return false;
  for (std::size_t i = 0; i < a.triangles.size(); ++i) {
    const auto &t = a.triangles[i];
    if (t.id.value_for_debug() != i ||
        t.patch.value_for_debug() >= s.patches.size() ||
        t.patch != std::get<0>(expected_triangles[i]) ||
        t.projection != std::get<1>(expected_triangles[i]) ||
        t.vertices != std::get<2>(expected_triangles[i]))
      return false;
    if (t.owner_free_semantic_key != owner_free_triangle_key(a, t))
      return false;
    const auto exact_orientation = orient2d(
        project(a.vertices[t.vertices[0].value_for_debug()].exact_coordinate,
                t.projection),
        project(a.vertices[t.vertices[1].value_for_debug()].exact_coordinate,
                t.projection),
        project(a.vertices[t.vertices[2].value_for_debug()].exact_coordinate,
                t.projection));
    if (exact_orientation == exact_sign::zero ||
        exact_orientation != t.exact_orientation)
      return false;
    for (std::size_t n = 0; n < t.halfedges.size(); ++n) {
      const auto h = t.halfedges[n];
      if (h.value_for_debug() >= a.halfedges.size())
        return false;
      const auto &halfedge = a.halfedges[h.value_for_debug()];
      if (halfedge.triangle != t.id || halfedge.origin != t.vertices[n] ||
          halfedge.destination != t.vertices[(n + 1) % t.vertices.size()])
        return false;
      ++halfedge_use[h.value_for_debug()];
    }
  }
  std::map<std::pair<realization_vertex_id, realization_vertex_id>,
           realization_halfedge_id>
      directed_halfedges;
  for (const auto &h : a.halfedges)
    if (!directed_halfedges
             .emplace(std::make_pair(h.origin, h.destination), h.id)
             .second)
      return false;
  for (std::size_t i = 0; i < a.halfedges.size(); ++i) {
    const auto &h = a.halfedges[i];
    if (h.id.value_for_debug() != i || halfedge_use[i] != 1 ||
        h.triangle.value_for_debug() >= a.triangles.size() ||
        h.next.value_for_debug() >= a.halfedges.size() ||
        h.previous.value_for_debug() >= a.halfedges.size() ||
        a.halfedges[h.next.value_for_debug()].previous != h.id ||
        a.halfedges[h.previous.value_for_debug()].next != h.id)
      return false;
    const auto selected_edge =
        std::find_if(s.edges.begin(), s.edges.end(), [&](const auto &edge) {
          const auto lo = realization_vertex_id::from_canonical_value(
              edge.lower.value_for_debug());
          const auto hi = realization_vertex_id::from_canonical_value(
              edge.upper.value_for_debug());
          return std::minmax(lo, hi) == std::minmax(h.origin, h.destination);
        });
    const bool hole_bridge =
        expected_hole_bridges.count(
            bridge_key(a.triangles[h.triangle.value_for_debug()].patch,
                       h.origin, h.destination)) != 0;
    if ((selected_edge != s.edges.end()) != bool(h.selected_edge) ||
        (h.selected_edge && (*h.selected_edge != selected_edge->id ||
                             h.role != realization_edge_role::selected_edge)) ||
        (!h.selected_edge &&
         h.role != (hole_bridge
                        ? realization_edge_role::hole_bridge
                        : realization_edge_role::triangulation_diagonal)))
      return false;
    const auto expected_twin =
        directed_halfedges.find({h.destination, h.origin});
    if ((expected_twin != directed_halfedges.end()) != bool(h.twin) ||
        (h.twin && (*h.twin != expected_twin->second ||
                    h.twin->value_for_debug() >= a.halfedges.size() ||
                    a.halfedges[h.twin->value_for_debug()].twin != h.id)))
      return false;
  }
  if (!assignment_valid(a) || !a.search.accepted_assignment ||
      a.search.complete_assignments == 0 || a.search.exhausted)
    return false;
  realized_boundary<T, I> replay = a;
  bool replay_accepted = false;
  std::uint64_t replay_assignment = 0, replay_nodes = 0, replay_complete = 0;
  const auto replay_order = solver_vertex_order(replay);
  std::function<void(std::size_t)> replay_dfs = [&](std::size_t depth) {
    if (replay_accepted)
      return;
    ++replay_nodes;
    if (depth == replay.vertices.size()) {
      ++replay_complete;
      for (auto &v : replay.vertices)
        v.coordinate = {value_of_bits<T>(v.accepted_bits[0]),
                        value_of_bits<T>(v.accepted_bits[1]),
                        value_of_bits<T>(v.accepted_bits[2])};
      if (assignment_valid(replay))
        replay_accepted = true;
      else
        ++replay_assignment;
      return;
    }
    const auto vi = replay_order[depth];
    auto &vertex = replay.vertices[vi];
    const auto candidates = point_candidates(replay.axis_domains[3 * vi],
                                             replay.axis_domains[3 * vi + 1],
                                             replay.axis_domains[3 * vi + 2]);
    for (const auto &candidate : candidates) {
      vertex.accepted_bits = candidate.bits;
      vertex.accepted_axis_rank = candidate.axis_rank;
      vertex.accepted_point_rank = candidate.rank;
      replay_dfs(depth + 1);
      if (replay_accepted)
        return;
    }
  };
  replay_dfs(0);
  if (!replay_accepted ||
      replay_assignment != a.search.accepted_assignment->value_for_debug() ||
      replay_complete != a.search.complete_assignments ||
      replay_nodes != a.search.visited_nodes ||
      a.search.nearest_passed != (replay_assignment == 0))
    return false;
  for (std::size_t i = 0; i < a.vertices.size(); ++i)
    if (!same_bits(replay.vertices[i].accepted_bits,
                   a.vertices[i].accepted_bits))
      return false;
  auto expected_obligations = a;
  expected_obligations.obligations.clear();
  for (auto &v : expected_obligations.vertices)
    v.obligations.clear();
  append_obligations(expected_obligations);
  if (expected_obligations.obligations.size() != a.obligations.size())
    return false;
  for (std::size_t i = 0; i < a.obligations.size(); ++i) {
    const auto &x = a.obligations[i];
    const auto &y = expected_obligations.obligations[i];
    if (x.id.value_for_debug() != i || x.kind != y.kind ||
        x.version != y.version || x.vertices != y.vertices ||
        x.triangles != y.triangles || x.selected_edges != y.selected_edges ||
        x.selected_patches != y.selected_patches || x.expected != y.expected ||
        x.actual != y.actual || x.witness != y.witness)
      return false;
  }
  for (std::size_t i = 0; i < a.vertices.size(); ++i)
    if (a.vertices[i].obligations !=
        expected_obligations.vertices[i].obligations)
      return false;
  std::uint64_t witnesses = 0;
  for (const auto &o : a.obligations)
    witnesses += !o.witness.empty();
  if (a.certificate.witnesses != witnesses)
    return false;
  canonical_encoder domains;
  for (const auto &d : a.axis_domains)
    for (const auto &v : d.values)
      bits(domains, v.bits);
  if (a.search.domain_digest !=
      domain_digest({{'Y', 'G', 'B', 'D', 'O', 'M', '1', '1'}},
                    domains.bytes()))
    return false;
  canonical_encoder tri;
  for (const auto &t : a.triangles) {
    tri.id(t.patch);
    for (auto v : t.vertices)
      tri.id(v);
  }
  canonical_encoder obl;
  for (const auto &o : a.obligations) {
    obl.byte(static_cast<std::uint8_t>(o.kind));
    obl.byte(static_cast<std::uint8_t>(o.actual));
  }
  canonical_encoder assignment;
  for (const auto &v : a.vertices)
    for (auto b : v.accepted_bits)
      bits(assignment, b);
  if (a.certificate.triangulation_digest !=
          domain_digest({{'Y', 'G', 'B', 'T', 'R', 'I', '1', '1'}},
                        tri.bytes()) ||
      a.certificate.obligation_digest !=
          domain_digest({{'Y', 'G', 'B', 'O', 'B', 'L', '1', '1'}},
                        obl.bytes()) ||
      a.certificate.assignment_digest !=
          domain_digest({{'Y', 'G', 'B', 'A', 'S', 'N', '1', '1'}},
                        assignment.bytes()))
    return false;
  return semantic(a) == a.canonical_bytes &&
         invocation(a) == a.artifact_bytes &&
         artifact_digest_for(a) == a.artifact_digest &&
         a.certificate.semantic_digest ==
             domain_digest({{'Y', 'G', 'B', 'C', 'A', 'N', '1', '1'}},
                           a.canonical_bytes);
}

template <class T, class I>
status_or<verification_report>
verify_typed(const artifact_view &v, const verification_spec &s,
             const verification_environment_view &e) noexcept {
  try {
    const auto &a = *static_cast<const realized_boundary<T, I> *>(v.payload);
    verification_report r;
    r.checker_version = s.checker_version;
    r.owner = v.owner;
    r.stage = boolean_stage::geometry_realization;
    r.slot = v.slot;
    r.artifact_type_tag = v.artifact_type_tag;
    r.artifact_schema = v.artifact_schema;
    r.setup_digest = e.setup_digest;
    r.artifact_digest = v.artifact_digest;
    r.invariant_set_digest = s.invariant_set_digest;
    const bool ok = e.options && v.payload && v.owner == e.owner &&
                    v.slot == artifact_slot::realized_boundary &&
                    v.artifact_type_tag == type_tag<T, I>() &&
                    v.artifact_schema == realized_boundary_schema &&
                    v.artifact_digest == a.artifact_digest &&
                    a.owner == e.owner && a.setup_digest == e.setup_digest &&
                    e.coordinate == (std::is_same<T, double>::value
                                         ? coordinate_tag::binary64
                                         : coordinate_tag::binary32) &&
                    e.index == (std::is_same<I, std::uint64_t>::value
                                    ? index_tag::uint64
                                    : index_tag::uint32) &&
                    valid(a, e.options->realization);
    r.outcome = ok ? verification_outcome::pass
                   : verification_outcome::invariant_failure;
    bool failed = false;
    for (auto c : s.required_invariants) {
      const auto st = ok ? check_status::passed
                         : failed ? check_status::not_run_due_to_prior_failure
                                  : check_status::failed;
      r.results.push_back({c, st, {}, 0});
      failed |= st == check_status::failed;
    }
    r.dependency_digests = {a.selected_digest, a.kernel_policy_digest,
                            a.policy_digest};
    if (a.symbolic)
      r.dependency_digests.insert(r.dependency_digests.begin() + 1,
                                  a.symbolic->artifact_digest);
    auto bytes = encode_verification_report(r);
    if (!bytes.has_value())
      return bytes.error();
    r.report_digest = domain_digest({{'Y', 'G', 'B', 'V', 'E', 'R', '0', '1'}},
                                    bytes.value());
    return r;
  } catch (...) {
    return make_error(boolean_error_code::internal_invariant_error,
                      boolean_stage::geometry_realization,
                      "realization_verifier_exception");
  }
}

template <class T, class I>
status_or<verification_report>
callback(const artifact_view &v, const verification_spec &s,
         const verification_environment_view &e) noexcept {
  return verify_typed<T, I>(v, s, e);
}

} // namespace

status_or<bool> register_geometry_realization_verifier(verifier_registry &r,
                                                       coordinate_tag c,
                                                       index_tag i) {
  verifier_registration x;
  x.slot = artifact_slot::realized_boundary;
  x.artifact_type_tag = realized_boundary_type_tag +
                        (static_cast<std::uint64_t>(c) << 8) +
                        static_cast<std::uint64_t>(i);
  x.artifact_schema = realized_boundary_schema;
  x.mandatory = {invariant_code::realization_binding,
                 invariant_code::realization_coordinates,
                 invariant_code::realization_triangulation,
                 invariant_code::realization_domains,
                 invariant_code::realization_obligations,
                 invariant_code::realization_embedding,
                 invariant_code::realization_search,
                 invariant_code::realization_canonical_encoding};
  x.exhaustive = x.mandatory;
  if (c == coordinate_tag::binary32 && i == index_tag::uint32)
    x.callback = &callback<float, std::uint32_t>;
  else if (c == coordinate_tag::binary32)
    x.callback = &callback<float, std::uint64_t>;
  else if (i == index_tag::uint32)
    x.callback = &callback<double, std::uint32_t>;
  else
    x.callback = &callback<double, std::uint64_t>;
  return r.register_verifier(std::move(x));
}

template <class T, class I>
status_or<std::shared_ptr<const published_artifact<realized_boundary<T, I>>>>
realize_selected_boundary(boolean_context<T, I> &ctx) {
  try {
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::geometry_realization, "cancelled");
    if (auto old = ctx.artifacts().latest(artifact_slot::realized_boundary))
      return std::static_pointer_cast<
          const published_artifact<realized_boundary<T, I>>>(old);
    auto selected_result = select_boolean_boundary(ctx);
    if (!selected_result.has_value())
      return selected_result.error();
    auto selected = selected_result.value();
    if (ctx.artifacts().latest_generation(
            artifact_slot::selected_exact_boundary) != selected->generation)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::geometry_realization, "stale_selection");
    const auto &s = *selected->payload;
    auto symbolic = s.arrangement->payload->symbolic;
    if (!symbolic ||
        s.constructions.get() != symbolic->payload->constructions.get())
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::geometry_realization,
                        "dependency_binding");
    const auto &svs = symbolic->payload->vertices;
    const auto &validated = *symbolic->payload->validated->payload;
    realized_boundary<T, I> a;
    a.owner = ctx.owner();
    a.setup_digest = ctx.replay().setup;
    a.selected_digest = selected->artifact_digest;
    a.kernel_policy_digest = symbolic->payload->kernel_policy_digest;
    a.policy_digest = realization_policy_digest(ctx.options().realization);
    a.selected = selected;
    a.symbolic = symbolic;
    a.constructions = s.constructions;
    if (a.constructions->owner != ctx.owner())
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::geometry_realization,
                        "construction_owner");
    for (const auto &source : s.vertices) {
      if (source.symbolic.value_for_debug() >= svs.size())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::geometry_realization,
                          "selected_symbolic_range");
      const auto &sv = svs[source.symbolic.value_for_debug()];
      realization_vertex<T> v;
      v.id = realization_vertex_id::from_canonical_value(a.vertices.size());
      v.selected = source.id;
      v.symbolic = source.symbolic;
      v.exact_coordinate = sv.point;
      v.exact_digest = exact_point_digest(sv.point);
      v.owner_free_semantic_key = owner_free_vertex_key(sv.point);
      v.derivations = sv.constructions;
      v.original_sources = sv.original_vertices;
      if (!std::is_sorted(v.derivations.begin(), v.derivations.end()) ||
          std::adjacent_find(v.derivations.begin(), v.derivations.end()) !=
              v.derivations.end())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::geometry_realization,
                          "construction_order");
      for (auto id : v.derivations) {
        if (id.value_for_debug() >= a.constructions->nodes.size())
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::geometry_realization,
                            "construction_range");
        const auto &node = a.constructions->nodes[id.value_for_debug()];
        if (node.id != id || node.kind != construction_kind::exact_relation ||
            node.exact_result.empty())
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::geometry_realization,
                            "construction_evidence");
      }
      if (!sv.original_vertices.empty()) {
        const auto oi = sv.original_vertices.front().value_for_debug();
        if (oi >= validated.vertices.size())
          return make_error(boolean_error_code::internal_invariant_error,
                            boolean_stage::geometry_realization,
                            "original_source_range");
        v.preserved_source = sv.original_vertices.front();
        v.preserved_source_bits = validated.vertices[oi].raw_bits;
        for (auto id : sv.original_vertices)
          if (id.value_for_debug() >= validated.vertices.size() ||
              !(validated.vertices[id.value_for_debug()].exact_coordinate ==
                sv.point))
            return make_error(boolean_error_code::internal_invariant_error,
                              boolean_stage::geometry_realization,
                              "conflicting_original_derivation");
      }
      a.vertices.push_back(std::move(v));
    }
    for (const auto &edge : s.edges) {
      a.vertices[edge.lower.value_for_debug()].selected_edges.push_back(
          edge.id);
      a.vertices[edge.upper.value_for_debug()].selected_edges.push_back(
          edge.id);
    }
    std::set<std::tuple<selected_patch_id, realization_vertex_id,
                        realization_vertex_id>>
        hole_bridges;
    for (const auto &patch : s.patches) {
      if (patch.source.value_for_debug() >=
          s.arrangement->payload->patches.size())
        return make_error(boolean_error_code::internal_invariant_error,
                          boolean_stage::geometry_realization,
                          "selected_patch_source_range");
      const auto projection = dominant_projection(
          s.arrangement->payload->patches[patch.source.value_for_debug()]
              .plane);
      auto rings = selected_patch_rings(s, a, patch, projection);
      if (!rings.has_value())
        return rings.error();
      for (const auto &ring : rings.value())
        for (const auto &node : ring.nodes)
          a.vertices[node.vertex.value_for_debug()].patches.push_back(patch.id);
      auto triangulated = triangulate_patch(std::move(rings.value()));
      if (!triangulated.has_value())
        return triangulated.error();
      for (const auto &bridge : triangulated.value().bridges)
        hole_bridges.insert(bridge_key(patch.id, bridge.first, bridge.second));
      for (const auto &vertices : triangulated.value().triangles) {
        realization_triangle t;
        t.id =
            realization_triangle_id::from_canonical_value(a.triangles.size());
        t.patch = patch.id;
        t.vertices = vertices;
        t.projection = projection;
        t.exact_orientation = orient2d(
            project(a.vertices[vertices[0].value_for_debug()].exact_coordinate,
                    projection),
            project(a.vertices[vertices[1].value_for_debug()].exact_coordinate,
                    projection),
            project(a.vertices[vertices[2].value_for_debug()].exact_coordinate,
                    projection));
        for (auto v : vertices)
          a.vertices[v.value_for_debug()].triangles.push_back(t.id);
        a.triangles.push_back(t);
      }
    }
    std::map<std::pair<realization_vertex_id, realization_vertex_id>,
             selected_edge_id>
        selected_edges;
    for (const auto &edge : s.edges)
      selected_edges[std::minmax(realization_vertex_id::from_canonical_value(
                                     edge.lower.value_for_debug()),
                                 realization_vertex_id::from_canonical_value(
                                     edge.upper.value_for_debug()))] = edge.id;
    std::map<std::pair<realization_vertex_id, realization_vertex_id>,
             realization_halfedge_id>
        directed;
    for (auto &t : a.triangles) {
      for (std::size_t n = 0; n < 3; ++n) {
        realization_halfedge h;
        h.id =
            realization_halfedge_id::from_canonical_value(a.halfedges.size());
        h.triangle = t.id;
        h.origin = t.vertices[n];
        h.destination = t.vertices[(n + 1) % 3];
        auto se = selected_edges.find(edge_key(h.origin, h.destination));
        if (se != selected_edges.end()) {
          h.role = realization_edge_role::selected_edge;
          h.selected_edge = se->second;
        } else if (hole_bridges.count(
                       bridge_key(t.patch, h.origin, h.destination)))
          h.role = realization_edge_role::hole_bridge;
        t.halfedges[n] = h.id;
        a.halfedges.push_back(h);
      }
      for (std::size_t n = 0; n < 3; ++n) {
        auto &h = a.halfedges[t.halfedges[n].value_for_debug()];
        h.next = t.halfedges[(n + 1) % 3];
        h.previous = t.halfedges[(n + 2) % 3];
        directed[{h.origin, h.destination}] = h.id;
      }
    }
    for (auto &h : a.halfedges) {
      auto twin = directed.find({h.destination, h.origin});
      if (twin != directed.end())
        h.twin = twin->second;
    }
    for (auto &triangle : a.triangles)
      triangle.owner_free_semantic_key = owner_free_triangle_key(a, triangle);
    std::vector<resource_reservation> realization_charges;
    std::uint64_t candidate_count = 0;
    for (auto &v : a.vertices) {
      const std::array<exact_scalar, 3> target{
          {v.exact_coordinate.x, v.exact_coordinate.y, v.exact_coordinate.z}};
      for (std::size_t axis = 0; axis < 3; ++axis) {
        realization_axis_domain<T> d;
        d.vertex = v.id;
        d.axis = axis;
        d.target = target[axis];
        d.values = axis_candidates<T>(target[axis], ctx.options().realization,
                                      v.preserved_source_bits
                                          ? std::optional<coordinate_bits<T>>((
                                                *v.preserved_source_bits)[axis])
                                          : std::nullopt);
        if (d.values.empty())
          return make_error(boolean_error_code::output_not_representable,
                            boolean_stage::geometry_realization,
                            "coordinate_out_of_range");
        auto sum = checked_add(candidate_count, d.values.size(),
                               boolean_stage::geometry_realization);
        if (!sum.has_value())
          return sum.error();
        candidate_count = sum.value();
        a.axis_domains.push_back(std::move(d));
      }
    }
    for (std::size_t i = 0; i < a.vertices.size(); ++i) {
      auto xy = checked_multiply(a.axis_domains[3 * i].values.size(),
                                 a.axis_domains[3 * i + 1].values.size(),
                                 boolean_stage::geometry_realization);
      if (!xy.has_value())
        return xy.error();
      auto xyz =
          checked_multiply(xy.value(), a.axis_domains[3 * i + 2].values.size(),
                           boolean_stage::geometry_realization);
      if (!xyz.has_value())
        return xyz.error();
      auto sum = checked_add(candidate_count, xyz.value(),
                             boolean_stage::geometry_realization);
      if (!sum.has_value())
        return sum.error();
      candidate_count = sum.value();
    }
    auto candidate_charge = ctx.accountant().reserve_scoped(
        resource_kind::candidates, candidate_count,
        boolean_stage::geometry_realization);
    if (!candidate_charge.has_value())
      return candidate_charge.error();
    realization_charges.push_back(std::move(candidate_charge.value()));
    canonical_encoder domains;
    for (const auto &d : a.axis_domains)
      for (const auto &v : d.values)
        bits(domains, v.bits);
    a.search.domain_digest = domain_digest(
        {{'Y', 'G', 'B', 'D', 'O', 'M', '1', '1'}}, domains.bytes());
    bool accepted = false, cancelled = false, limited = false;
    std::uint64_t assignment_number = 0;
    const auto variable_order = solver_vertex_order(a);
    std::function<void(std::size_t)> dfs = [&](std::size_t depth) {
      if (accepted || cancelled || limited)
        return;
      if (ctx.cancelled()) {
        cancelled = true;
        return;
      }
      auto node = ctx.accountant().reserve_scoped(
          resource_kind::realization_attempts, 1,
          boolean_stage::geometry_realization);
      if (!node.has_value()) {
        limited = true;
        return;
      }
      realization_charges.push_back(std::move(node.value()));
      ++a.search.visited_nodes;
      if (depth == a.vertices.size()) {
        ++a.search.complete_assignments;
        for (auto &v : a.vertices)
          v.coordinate = {value_of_bits<T>(v.accepted_bits[0]),
                          value_of_bits<T>(v.accepted_bits[1]),
                          value_of_bits<T>(v.accepted_bits[2])};
        if (assignment_valid(a)) {
          accepted = true;
          a.search.accepted_assignment =
              candidate_assignment_id::from_canonical_value(assignment_number);
          a.search.nearest_passed = assignment_number == 0;
        }
        ++assignment_number;
        return;
      }
      const auto vi = variable_order[depth];
      auto &v = a.vertices[vi];
      const auto candidates =
          point_candidates(a.axis_domains[3 * vi], a.axis_domains[3 * vi + 1],
                           a.axis_domains[3 * vi + 2]);
      for (const auto &candidate : candidates) {
        v.accepted_bits = candidate.bits;
        v.accepted_axis_rank = candidate.axis_rank;
        v.accepted_point_rank = candidate.rank;
        dfs(depth + 1);
        if (accepted || cancelled || limited)
          return;
      }
    };
    dfs(0);
    if (cancelled)
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::geometry_realization, "cancelled");
    if (limited)
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::geometry_realization,
                        "realization_search_limit");
    if (!accepted) {
      a.search.exhausted = true;
      return make_error(boolean_error_code::output_not_representable,
                        boolean_stage::geometry_realization,
                        "candidate_domain_exhausted");
    }
    append_obligations(a);
    a.certificate.id = realization_certificate_id::from_canonical_value(0);
    a.certificate.solver_version = ctx.options().realization.solver_version;
    a.certificate.vertices = a.vertices.size();
    a.certificate.triangles = a.triangles.size();
    a.certificate.halfedges = a.halfedges.size();
    a.certificate.obligations = a.obligations.size();
    for (const auto &o : a.obligations)
      a.certificate.witnesses += !o.witness.empty();
    a.certificate.selected_digest = a.selected_digest;
    a.certificate.kernel_policy_digest = a.kernel_policy_digest;
    a.certificate.policy_digest = a.policy_digest;
    canonical_encoder tri;
    for (const auto &t : a.triangles) {
      tri.id(t.patch);
      for (auto v : t.vertices)
        tri.id(v);
    }
    a.certificate.triangulation_digest =
        domain_digest({{'Y', 'G', 'B', 'T', 'R', 'I', '1', '1'}}, tri.bytes());
    canonical_encoder obl;
    for (const auto &o : a.obligations) {
      obl.byte(static_cast<std::uint8_t>(o.kind));
      obl.byte(static_cast<std::uint8_t>(o.actual));
    }
    a.certificate.obligation_digest =
        domain_digest({{'Y', 'G', 'B', 'O', 'B', 'L', '1', '1'}}, obl.bytes());
    canonical_encoder assignment;
    for (const auto &v : a.vertices)
      for (auto b : v.accepted_bits)
        bits(assignment, b);
    a.certificate.assignment_digest = domain_digest(
        {{'Y', 'G', 'B', 'A', 'S', 'N', '1', '1'}}, assignment.bytes());
    a.canonical_bytes = semantic(a);
    a.certificate.semantic_digest = domain_digest(
        {{'Y', 'G', 'B', 'C', 'A', 'N', '1', '1'}}, a.canonical_bytes);
    a.artifact_bytes = invocation(a);
    a.artifact_digest = artifact_digest_for(a);
    auto evidence_charge = ctx.accountant().reserve_scoped(
        resource_kind::evidence_records, a.obligations.size(),
        boolean_stage::geometry_realization);
    if (!evidence_charge.has_value())
      return evidence_charge.error();
    realization_charges.push_back(std::move(evidence_charge.value()));
    auto authoritative_charge = ctx.accountant().reserve_scoped(
        resource_kind::authoritative_bytes, a.artifact_bytes.size(),
        boolean_stage::geometry_realization);
    if (!authoritative_charge.has_value())
      return authoritative_charge.error();
    realization_charges.push_back(std::move(authoritative_charge.value()));
    auto ptr = std::make_shared<const realized_boundary<T, I>>(std::move(a));
    auto registry = dynamic_cast<const verifier_registry *>(&ctx.verifiers());
    if (!registry)
      return make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::geometry_realization,
                        "verifier_registry_required");
    auto spec = registry->specification(
        artifact_slot::realized_boundary, type_tag<T, I>(),
        realized_boundary_schema, ctx.options().verification);
    if (!spec.has_value())
      return spec.error();
    artifact_view view{ctx.owner(),
                       artifact_slot::realized_boundary,
                       type_tag<T, I>(),
                       realized_boundary_schema,
                       1,
                       ptr->artifact_digest,
                       ptr,
                       ptr.get()};
    verification_environment_view env{ctx.owner(),
                                      ctx.replay().setup,
                                      ctx.contract().selected_operation(),
                                      &ctx.options(),
                                      ctx.platform().coordinate,
                                      ctx.platform().index,
                                      &ctx.kernel(),
                                      {},
                                      &ctx.accountant(),
                                      [&] { return ctx.cancelled(); }};
    stage_transaction<realized_boundary<T, I>, realized_boundary<T, I>> tx(
        ctx.owner(), boolean_stage::geometry_realization,
        artifact_slot::realized_boundary,
        std::make_unique<realized_boundary<T, I>>());
    for (auto &charge : realization_charges)
      tx.stage_reservation(std::move(charge));
    auto ok = tx.verify(ptr, view, spec.value(), env, ctx.verifiers());
    if (!ok.has_value())
      return ok.error();
    if (ctx.cancelled())
      return make_error(boolean_error_code::resource_limit,
                        boolean_stage::geometry_realization, "cancelled");
    return tx.compare_and_publish(ctx.artifacts(), 0);
  } catch (const std::bad_alloc &) {
    return make_error(boolean_error_code::resource_limit,
                      boolean_stage::geometry_realization,
                      "realization_allocation");
  } catch (const std::exception &e) {
    auto x = make_error(boolean_error_code::internal_invariant_error,
                        boolean_stage::geometry_realization,
                        "realization_exception");
    x.detail = e.what();
    return x;
  }
}

#define INST(T, I)                                                             \
  template status_or<                                                          \
      std::shared_ptr<const published_artifact<realized_boundary<T, I>>>>      \
  realize_selected_boundary(boolean_context<T, I> &)
INST(float, std::uint32_t);
INST(float, std::uint64_t);
INST(double, std::uint32_t);
INST(double, std::uint64_t);
#undef INST

} // namespace mesh_boolean
} // namespace ygor
