#include "YgorMeshesBooleanApproximate.h"

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <tuple>

namespace ygor {
namespace mesh_boolean {
namespace {

struct node {
  selected_vertex_id vertex;
  exact_point2 point;
};

struct ring {
  std::vector<node> nodes;
  bool hole = false;
};

struct topology_budget {
  const std::function<bool(std::uint64_t, std::uint64_t, std::uint64_t,
                           std::uint64_t)> &charge;
  bool failed = false;
  bool spend(std::uint64_t work = 1, std::uint64_t predicates = 0,
             std::uint64_t records = 0, std::uint64_t bytes = 0) {
    if (failed)
      return false;
    failed = charge && !charge(work, predicates, records, bytes);
    return !failed;
  }
};

exact_scalar double_area(const std::vector<node> &nodes, topology_budget &budget) {
  exact_scalar area(0);
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    if (!budget.spend())
      return {};
    const auto &a = nodes[i].point;
    const auto &b = nodes[(i + 1) % nodes.size()].point;
    area = area + a.x * b.y - a.y * b.x;
  }
  return area;
}

bool endpoint_only(const exact_segment2 &candidate, const exact_segment2 &edge,
                   topology_budget &budget) {
  if (!budget.spend(1, 1))
    return false;
  const auto relation = relate_segments(candidate, edge);
  if (relation.dimension == intersection_dimension::empty)
    return true;
  if (relation.dimension != intersection_dimension::point || !relation.point)
    return false;
  return (*relation.point == candidate.origin ||
          *relation.point == candidate.destination) &&
         (*relation.point == edge.origin || *relation.point == edge.destination);
}

bool visible(const node &a, const node &b, const std::vector<node> &boundary,
              const std::vector<ring> &rings, topology_budget &budget) {
  if (!budget.spend())
    return false;
  if (a.point == b.point)
    return false;
  const exact_segment2 candidate{a.point, b.point};
  for (const auto &current : rings)
    for (std::size_t i = 0; i < current.nodes.size(); ++i)
      if (!endpoint_only(candidate,
                          {current.nodes[i].point,
                           current.nodes[(i + 1) % current.nodes.size()].point},
                         budget))
        return false;
  for (std::size_t i = 0; i < boundary.size(); ++i)
    if (!endpoint_only(candidate,
                       {boundary[i].point,
                        boundary[(i + 1) % boundary.size()].point}, budget))
      return false;
  const exact_point2 midpoint{(a.point.x + b.point.x) / exact_scalar(2),
                              (a.point.y + b.point.y) / exact_scalar(2)};
  std::vector<exact_point2> polygon;
  for (const auto &entry : rings.front().nodes)
    polygon.push_back(entry.point);
  if (!budget.spend(1, 1))
    return false;
  auto outer = classify_point_polygon(midpoint, polygon);
  if (!outer.has_value() ||
      outer.value().kind != point_region_kind::open_interior)
    return false;
  for (std::size_t i = 1; i < rings.size(); ++i) {
    polygon.clear();
    for (const auto &entry : rings[i].nodes)
      polygon.push_back(entry.point);
    if (!budget.spend(1, 1))
      return false;
    auto hole = classify_point_polygon(midpoint, polygon);
    if (!hole.has_value() || hole.value().kind != point_region_kind::outside)
      return false;
  }
  return true;
}

bool valid_diagonal(const std::vector<node> &nodes, std::size_t previous,
                     std::size_t next, topology_budget &budget) {
  if (!budget.spend())
    return false;
  const exact_segment2 diagonal{nodes[previous].point, nodes[next].point};
  if (diagonal.origin == diagonal.destination)
    return false;
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    const auto j = (i + 1) % nodes.size();
    if (i == previous || j == previous || i == next || j == next)
      continue;
    if (!endpoint_only(diagonal, {nodes[i].point, nodes[j].point}, budget))
      return false;
  }
  return true;
}

bool inside_triangle(const exact_point2 &p, const exact_point2 &a,
                      const exact_point2 &b, const exact_point2 &c,
                      exact_sign sign, topology_budget &budget) {
  if (!budget.spend(3, 3))
    return false;
  const auto x = orient2d(a, b, p), y = orient2d(b, c, p),
             z = orient2d(c, a, p);
  return sign == exact_sign::positive
             ? x != exact_sign::negative && y != exact_sign::negative &&
                   z != exact_sign::negative
             : x != exact_sign::positive && y != exact_sign::positive &&
                   z != exact_sign::positive;
}

bool clip(std::vector<node> nodes, exact_sign sign,
           std::vector<std::array<selected_vertex_id, 3>> &triangles,
          topology_budget &budget) {
  while (nodes.size() > 3) {
    if (!budget.spend())
      return false;
    std::optional<std::size_t> best;
    std::tuple<std::uint64_t, std::uint64_t, std::uint64_t, std::size_t> key;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
      const auto previous = (i + nodes.size() - 1) % nodes.size();
      const auto next = (i + 1) % nodes.size();
      if (!budget.spend(1, 1) ||
          nodes[previous].vertex == nodes[i].vertex ||
          nodes[i].vertex == nodes[next].vertex ||
          nodes[previous].vertex == nodes[next].vertex ||
          orient2d(nodes[previous].point, nodes[i].point,
                   nodes[next].point) != sign ||
          !valid_diagonal(nodes, previous, next, budget))
        continue;
      bool contains = false;
      for (std::size_t n = 0; n < nodes.size(); ++n)
        if (n != previous && n != i && n != next &&
            !(nodes[n].point == nodes[previous].point) &&
            !(nodes[n].point == nodes[i].point) &&
            !(nodes[n].point == nodes[next].point) &&
            inside_triangle(nodes[n].point, nodes[previous].point,
                             nodes[i].point, nodes[next].point, sign, budget))
          contains = true;
      const auto candidate = std::make_tuple(
          nodes[previous].vertex.value_for_debug(),
          nodes[i].vertex.value_for_debug(), nodes[next].vertex.value_for_debug(),
          i);
      if (!contains && (!best || candidate < key)) {
        best = i;
        key = candidate;
      }
    }
    if (!best)
      return false;
    const auto i = *best;
    const auto previous = (i + nodes.size() - 1) % nodes.size();
    const auto next = (i + 1) % nodes.size();
    if (!budget.spend(1, 0, 1,
                      sizeof(std::array<selected_vertex_id, 3>)))
      return false;
    triangles.push_back(
        {nodes[previous].vertex, nodes[i].vertex, nodes[next].vertex});
    nodes.erase(nodes.begin() + static_cast<std::ptrdiff_t>(i));
  }
  if (!budget.spend(1, 1) || nodes.size() != 3 ||
      orient2d(nodes[0].point, nodes[1].point, nodes[2].point) != sign)
    return false;
  if (!budget.spend(1, 0, 1,
                    sizeof(std::array<selected_vertex_id, 3>)))
    return false;
  triangles.push_back({nodes[0].vertex, nodes[1].vertex, nodes[2].vertex});
  return true;
}

} // namespace

product_status_or<std::vector<approximate_triangle_evidence>>
reconstruct_authorized_approximate_triangulation(
    const exact_stratified_boundary &boundary,
    const std::function<bool(std::uint64_t, std::uint64_t, std::uint64_t,
                             std::uint64_t)> &charge) noexcept {
  try {
    topology_budget budget{charge};
    std::map<std::uint64_t, exact_plane3> planes;
    for (const auto &geometry : boundary.patch_geometry)
      if (!budget.spend())
        return make_product_error(product_error_code::resource_limit,
                                  "approximate_topology.budget");
      else
      planes.emplace(geometry.patch.value_for_debug(), geometry.plane);
    std::vector<approximate_triangle_evidence> output;
    for (const auto &patch : boundary.patches) {
      if (!budget.spend())
        return make_product_error(product_error_code::resource_limit,
                                  "approximate_topology.budget");
      const auto plane = planes.find(patch.id.value_for_debug());
      if (plane == planes.end() || patch.cycles.empty())
        return make_product_error(product_error_code::verifier_disagreement,
                                  "approximate_topology.patch_plane");
      const auto projection = dominant_projection(plane->second);
      std::vector<ring> rings;
      for (std::size_t cycle_index = 0; cycle_index < patch.cycles.size();
           ++cycle_index) {
        const auto cycle_id = patch.cycles[cycle_index];
        if (cycle_id.value_for_debug() >= boundary.cycles.size())
          return make_product_error(product_error_code::verifier_disagreement,
                                    "approximate_topology.cycle_range");
        const auto &cycle = boundary.cycles[cycle_id.value_for_debug()];
        if (cycle.patch != patch.id || cycle.hole != (cycle_index != 0))
          return make_product_error(product_error_code::verifier_disagreement,
                                    "approximate_topology.cycle_binding");
        ring current;
        current.hole = cycle.hole;
        for (const auto halfedge_id : cycle.halfedges) {
          if (!budget.spend())
            return make_product_error(product_error_code::resource_limit,
                                      "approximate_topology.budget");
          if (halfedge_id.value_for_debug() >= boundary.halfedges.size())
            return make_product_error(product_error_code::verifier_disagreement,
                                      "approximate_topology.halfedge_range");
          const auto vertex =
              boundary.halfedges[halfedge_id.value_for_debug()].origin;
          if (vertex.value_for_debug() >= boundary.vertices.size())
            return make_product_error(product_error_code::verifier_disagreement,
                                      "approximate_topology.vertex_range");
          if (!budget.spend(1, 0, 1, sizeof(node)))
            return make_product_error(product_error_code::resource_limit,
                                      "approximate_topology.budget");
          current.nodes.push_back(
              {vertex, project(boundary.vertices[vertex.value_for_debug()].coordinate,
                               projection)});
        }
        if (!budget.spend(1, 0, 1, sizeof(ring)))
          return make_product_error(product_error_code::resource_limit,
                                    "approximate_topology.budget");
        rings.push_back(std::move(current));
      }
      if (rings.front().nodes.size() < 3)
        return make_product_error(product_error_code::verifier_disagreement,
                                  "approximate_topology.outer_ring");
      const auto sign = double_area(rings.front().nodes, budget).sign();
      if (budget.failed)
        return make_product_error(product_error_code::resource_limit,
                                  "approximate_topology.budget");
      if (sign == exact_sign::zero)
        return make_product_error(product_error_code::verifier_disagreement,
                                  "approximate_topology.zero_area");
      auto merged = rings.front().nodes;
      std::vector<bool> bridged(rings.size());
      bridged.front() = true;
      for (std::size_t remaining = rings.size() - 1; remaining > 0; --remaining) {
        std::optional<std::tuple<std::size_t, std::size_t, std::size_t>> best;
        std::tuple<std::uint64_t, std::uint64_t, std::size_t, std::size_t,
                   std::size_t>
            key;
        for (std::size_t h = 1; h < rings.size(); ++h)
          if (!bridged[h])
            for (std::size_t hi = 0; hi < rings[h].nodes.size(); ++hi)
              for (std::size_t bi = 0; bi < merged.size(); ++bi) {
                if (!visible(rings[h].nodes[hi], merged[bi], merged, rings,
                             budget))
                  continue;
                const auto candidate = std::make_tuple(
                    rings[h].nodes[hi].vertex.value_for_debug(),
                    merged[bi].vertex.value_for_debug(), h, hi, bi);
                if (!best || candidate < key) {
                  best = {h, hi, bi};
                  key = candidate;
                }
              }
        if (!best) {
          if (budget.failed)
            return make_product_error(product_error_code::resource_limit,
                                      "approximate_topology.budget");
          else
            return make_product_error(product_error_code::verifier_disagreement,
                                      "approximate_topology.bridge");
        }
        const auto h = std::get<0>(*best), hi = std::get<1>(*best),
                   bi = std::get<2>(*best);
        std::vector<node> next;
        for (std::size_t i = 0;
             i < merged.size() + rings[h].nodes.size() + 2; ++i)
          if (!budget.spend(1, 0, 1, sizeof(node)))
            return make_product_error(product_error_code::resource_limit,
                                      "approximate_topology.budget");
        next.insert(next.end(), merged.begin(), merged.begin() + bi + 1);
        for (std::size_t i = 0; i < rings[h].nodes.size(); ++i)
          next.push_back(rings[h].nodes[(hi + i) % rings[h].nodes.size()]);
        next.push_back(rings[h].nodes[hi]);
        next.push_back(merged[bi]);
        next.insert(next.end(), merged.begin() + bi + 1, merged.end());
        merged = std::move(next);
        bridged[h] = true;
      }
      std::vector<std::array<selected_vertex_id, 3>> triangles;
      if (!clip(std::move(merged), sign, triangles, budget)) {
        if (budget.failed)
          return make_product_error(product_error_code::resource_limit,
                                    "approximate_topology.budget");
        else
          return make_product_error(product_error_code::verifier_disagreement,
                                    "approximate_topology.triangulation");
      }
      for (const auto &vertices : triangles) {
        if (!budget.spend(1, 0, 1, sizeof(approximate_triangle_evidence)))
          return make_product_error(product_error_code::resource_limit,
                                    "approximate_topology.budget");
        approximate_triangle_evidence triangle;
        triangle.triangle = realization_triangle_id::from_canonical_value(
            output.size());
        triangle.patch = patch.id;
        triangle.vertices = vertices;
        triangle.projection = projection;
        if (!budget.spend(1, 1))
          return make_product_error(product_error_code::resource_limit,
                                    "approximate_topology.budget");
        triangle.exact_orientation = orient2d(
            project(boundary.vertices[vertices[0].value_for_debug()].coordinate,
                    projection),
            project(boundary.vertices[vertices[1].value_for_debug()].coordinate,
                    projection),
            project(boundary.vertices[vertices[2].value_for_debug()].coordinate,
                    projection));
        output.push_back(std::move(triangle));
      }
    }
    return output;
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "approximate_topology.allocation");
  } catch (...) {
    return make_product_error(product_error_code::verifier_disagreement,
                              "approximate_topology.exception");
  }
}

} // namespace mesh_boolean
} // namespace ygor
