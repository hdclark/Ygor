#include "YgorMeshesBooleanRealization.h"
#include <algorithm>
#include <limits>
#include <map>
#include <numeric>
#include <tuple>

namespace ygor {
namespace mesh_boolean {
namespace {

exact_scalar integer_scalar(const big_int &value) {
  return exact_scalar(value, big_uint(1));
}

bool canonical_coordinate_relation(const defining_relation &relation,
                                   const exact_point3 &target,
                                   std::size_t &axis) {
  const std::array<exact_scalar, 3> coordinates{{target.x, target.y, target.z}};
  std::optional<std::size_t> found;
  for (std::size_t i = 0; i < coordinates.size(); ++i) {
    if (relation.coefficients[i] == exact_scalar(1)) {
      if (found)
        return false;
      found = i;
    } else if (!relation.coefficients[i].is_zero()) {
      return false;
    }
  }
  if (!found || relation.coefficients[3] != coordinates[*found].negated())
    return false;
  axis = *found;
  return true;
}

template <class T, class I>
bool canonical_plane_relation(const realized_boundary<T, I> &artifact,
                              const defining_relation &relation) {
  if (!artifact.symbolic || relation.defining_sources.size() != 1)
    return false;
  const auto *facet = std::get_if<facet_id>(&relation.defining_sources.front());
  const auto &validated = *artifact.symbolic->payload->validated->payload;
  if (!facet || facet->value_for_debug() >= validated.facets.size())
    return false;
  const auto &plane = validated.facets[facet->value_for_debug()].plane;
  return relation.coefficients ==
         std::array<exact_scalar, 4>{{integer_scalar(plane.a),
                                      integer_scalar(plane.b),
                                      integer_scalar(plane.c),
                                      integer_scalar(plane.d)}};
}

} // namespace

template <class T, class I>
bool verify_realization_exact_substitution(const realized_boundary<T, I> &a) {
  if (!a.constructions)
    return false;
  for (const auto &vertex : a.vertices) {
    std::array<exact_scalar, 3> decoded;
    for (std::size_t axis = 0; axis < decoded.size(); ++axis) {
      const auto value = decode_coordinate<T>(vertex.accepted_bits[axis],
                                               boolean_stage::geometry_realization);
      if (!value.has_value())
        return false;
      decoded[axis] = value.value().value;
    }
    const exact_point3 point{decoded[0], decoded[1], decoded[2]};
    if (!(point == vertex.exact_coordinate))
      return false;
    for (auto node_id : vertex.derivations) {
      if (!node_id.valid() ||
          node_id.value_for_debug() >= a.constructions->nodes.size())
        return false;
      const auto &node = a.constructions->nodes[node_id.value_for_debug()];
      if (node.id != node_id)
        return false;
      std::array<bool, 3> coordinate_relations{{false, false, false}};
      for (auto relation_id : node.defining_relations) {
        if (!relation_id.valid() ||
            relation_id.value_for_debug() >= a.constructions->relations.size())
          return false;
        const auto &relation =
            a.constructions->relations[relation_id.value_for_debug()];
        if (relation.id != relation_id || relation.construction != node_id ||
            !defining_relation_satisfied(relation, point))
          return false;
        if (relation.kind == defining_relation_kind::coordinate_equality) {
          std::size_t axis = 0;
          if (!canonical_coordinate_relation(relation, vertex.exact_coordinate,
                                             axis) ||
              coordinate_relations[axis])
            return false;
          coordinate_relations[axis] = true;
        } else if (relation.kind == defining_relation_kind::point_on_plane) {
          if (!canonical_plane_relation(a, relation))
            return false;
        } else {
          return false;
        }
      }
      if (!std::all_of(coordinate_relations.begin(), coordinate_relations.end(),
                       [](bool present) { return present; }))
        return false;
    }
  }
  return true;
}

template <class T, class I>
bool verify_realization_constraint_evidence_impl(const realized_boundary<T, I> &a) {
  if (!a.selected || !a.selected->payload || !a.constructions ||
      a.pair_boxes.size() != a.triangles.size() ||
      a.components.size() != a.component_transcripts.size())
    return false;
  if (a.axis_domains.size() != 3 * a.vertices.size() ||
      std::any_of(a.axis_domains.begin(), a.axis_domains.end(),
                  [](const auto &domain) { return domain.values.size() != 1; }))
    return false;
  std::vector<exact_scalar> decoded_domains;
  decoded_domains.reserve(a.axis_domains.size());
  for (std::size_t i = 0; i < a.axis_domains.size(); ++i) {
    const auto &domain = a.axis_domains[i];
    const auto decoded = decode_coordinate<T>(
        domain.values.front().bits, boolean_stage::geometry_realization);
    if (!decoded.has_value() || decoded.value().value != domain.target ||
        domain.vertex.value_for_debug() != i / 3 || domain.axis != i % 3)
      return false;
    decoded_domains.push_back(decoded.value().value);
  }
  for (std::size_t ti = 0; ti < a.triangles.size(); ++ti) {
    const auto &triangle = a.triangles[ti];
    const auto &stored = a.pair_boxes[ti];
    std::array<exact_scalar, 3> lower, upper;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      bool initialized = false;
      for (auto vertex : triangle.vertices) {
        const auto vi = vertex.value_for_debug();
        if (vi >= a.vertices.size())
          return false;
        const auto &value = decoded_domains[3 * vi + axis];
        if (!initialized) {
          lower[axis] = upper[axis] = value;
          initialized = true;
        } else {
          if (value < lower[axis])
            lower[axis] = value;
          if (upper[axis] < value)
            upper[axis] = value;
        }
      }
      if (!initialized)
        return false;
    }
    if (stored.triangle != triangle.id ||
        !(stored.lower == exact_point3{lower[0], lower[1], lower[2]}) ||
        !(stored.upper == exact_point3{upper[0], upper[1], upper[2]}))
      return false;
  }
  std::vector<std::size_t> order(a.pair_boxes.size());
  std::iota(order.begin(), order.end(), std::size_t(0));
  std::sort(order.begin(), order.end(), [&](const auto x, const auto y) {
    const auto &xb = a.pair_boxes[x];
    const auto &yb = a.pair_boxes[y];
    if (xb.lower.x != yb.lower.x)
      return xb.lower.x < yb.lower.x;
    return xb.triangle < yb.triangle;
  });
  std::vector<exact_scalar> y_coordinates;
  y_coordinates.reserve(a.pair_boxes.size());
  for (const auto &box : a.pair_boxes)
    y_coordinates.push_back(box.lower.y);
  std::sort(y_coordinates.begin(), y_coordinates.end());
  y_coordinates.erase(std::unique(y_coordinates.begin(), y_coordinates.end()),
                      y_coordinates.end());
  using interval_key = std::pair<exact_scalar, realization_triangle_id>;
  std::vector<std::map<interval_key, std::size_t>> interval_tree(
      y_coordinates.empty() ? 0 : 4 * y_coordinates.size());
  const auto update_interval = [&](auto &&self, std::size_t node,
                                   std::size_t begin, std::size_t end,
                                   std::size_t position, std::size_t box_index,
                                   bool insert) -> bool {
    const auto &box = a.pair_boxes[box_index];
    const interval_key key{box.upper.y, box.triangle};
    if (insert) {
      if (!interval_tree[node].emplace(key, box_index).second)
        return false;
    } else if (interval_tree[node].erase(key) != 1) {
      return false;
    }
    if (end - begin == 1)
      return true;
    const auto middle = begin + (end - begin) / 2;
    return position < middle
               ? self(self, 2 * node + 1, begin, middle, position, box_index,
                      insert)
               : self(self, 2 * node + 2, middle, end, position, box_index,
                      insert);
  };
  const auto query_intervals = [&](auto &&self, std::size_t node,
                                   std::size_t begin, std::size_t end,
                                   std::size_t prefix,
                                   const realization_domain_box &query,
                                   const auto &consume) -> bool {
    if (begin >= prefix)
      return true;
    if (end <= prefix) {
      const interval_key first{query.lower.y,
                               realization_triangle_id::from_canonical_value(0)};
      for (auto it = interval_tree[node].lower_bound(first);
           it != interval_tree[node].end(); ++it) {
        if (!consume(it->second)) return false;
      }
      return true;
    }
    const auto middle = begin + (end - begin) / 2;
    return self(self, 2 * node + 1, begin, middle, prefix, query, consume) &&
           self(self, 2 * node + 2, middle, end, prefix, query, consume);
  };
  std::map<std::pair<exact_scalar, realization_triangle_id>, std::size_t>
      expiry;
  std::vector<realization_triangle_pair> pairs;
  std::uint64_t x_overlap_pairs = 0, indexed_overlap_checks = 0;
  for (const auto box_index : order) {
    const auto &box = a.pair_boxes[box_index];
    while (!expiry.empty() && expiry.begin()->first.first < box.lower.x) {
      const auto expired = expiry.begin()->second;
      expiry.erase(expiry.begin());
      const auto position = static_cast<std::size_t>(std::lower_bound(
          y_coordinates.begin(), y_coordinates.end(),
          a.pair_boxes[expired].lower.y) - y_coordinates.begin());
      if (!update_interval(update_interval, 0, 0, y_coordinates.size(),
                           position, expired, false))
        return false;
    }
    if (expiry.size() > std::numeric_limits<std::uint64_t>::max() -
                            x_overlap_pairs)
      return false;
    x_overlap_pairs += expiry.size();
    const auto prefix = static_cast<std::size_t>(std::upper_bound(
        y_coordinates.begin(), y_coordinates.end(), box.upper.y) -
        y_coordinates.begin());
    const auto consume = [&](const std::size_t other_index) {
      if (indexed_overlap_checks == std::numeric_limits<std::uint64_t>::max())
        return false;
      ++indexed_overlap_checks;
      const auto &other = a.pair_boxes[other_index];
      if (!(other.upper.z < box.lower.z) && !(box.upper.z < other.lower.z)) {
        const auto pair = std::minmax(other.triangle, box.triangle);
        pairs.push_back({pair.first, pair.second});
      }
      return true;
    };
    if (!query_intervals(query_intervals, 0, 0, y_coordinates.size(), prefix,
                         box, consume))
      return false;
    const auto position = static_cast<std::size_t>(std::lower_bound(
        y_coordinates.begin(), y_coordinates.end(), box.lower.y) -
        y_coordinates.begin());
    if (!update_interval(update_interval, 0, 0, y_coordinates.size(), position,
                         box_index, true) ||
        !expiry.emplace(std::make_pair(box.upper.x, box.triangle), box_index)
             .second)
      return false;
  }
  performance_count(performance_counter::realization_pair_boxes,
                    a.pair_boxes.size());
  performance_count(performance_counter::realization_pair_candidates,
                    pairs.size());
  performance_count(performance_counter::realization_exact_pair_checks,
                    indexed_overlap_checks);
  std::sort(pairs.begin(), pairs.end(), [](const auto &x, const auto &y) {
    return std::tie(x.lower, x.upper) < std::tie(y.lower, y.upper);
  });
  pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
  if (pairs != a.pair_candidates ||
      x_overlap_pairs > std::numeric_limits<std::uint64_t>::max() - pairs.size() ||
      a.search.pair_checks != x_overlap_pairs + pairs.size())
    return false;

  std::vector<exact_point3> points;
  points.reserve(a.vertices.size());
  for (std::size_t i = 0; i < a.vertices.size(); ++i)
    points.push_back({decoded_domains[3 * i], decoded_domains[3 * i + 1],
                      decoded_domains[3 * i + 2]});

  std::vector<std::vector<realization_obligation_id>> vertex_obligations(
      a.vertices.size());
  std::size_t expected_index = 0;
  const auto add_expected = [&](realization_obligation expected) {
    if (expected_index >= a.obligations.size())
      return false;
    expected.id = realization_obligation_id::from_canonical_value(expected_index);
    expected.witness = {static_cast<std::uint8_t>(expected.actual)};
    const auto &stored = a.obligations[expected_index];
    if (stored.id != expected.id || stored.kind != expected.kind ||
        stored.version != expected.version ||
        stored.vertices != expected.vertices ||
        stored.triangles != expected.triangles ||
        stored.selected_edges != expected.selected_edges ||
        stored.selected_patches != expected.selected_patches ||
        stored.expected != expected.expected || stored.actual != expected.actual ||
        stored.witness != expected.witness ||
        stored.defining_relation != expected.defining_relation)
      return false;
    for (auto vertex : expected.vertices) {
      const auto vi = vertex.value_for_debug();
      if (vi >= vertex_obligations.size())
        return false;
      vertex_obligations[vi].push_back(expected.id);
    }
    ++expected_index;
    return true;
  };
  const auto sign_relation = [](exact_sign sign)
      -> std::optional<realization_relation> {
    if (sign == exact_sign::positive)
      return realization_relation::positive;
    if (sign == exact_sign::negative)
      return realization_relation::negative;
    return std::nullopt;
  };
  const auto intersection_relation = [](polygon_intersection_kind relation) {
    if (relation == polygon_intersection_kind::point)
      return realization_relation::point;
    if (relation == polygon_intersection_kind::segment)
      return realization_relation::segment;
    return realization_relation::disjoint;
  };

  for (std::size_t vi = 0; vi < a.vertices.size(); ++vi) {
    const auto &vertex = a.vertices[vi];
    if (vertex.id.value_for_debug() != vi)
      return false;
    if (!add_expected({{}, realization_obligation_kind::exact_target_equality,
                       1, {vertex.id}, {}, {}, {},
                       realization_relation::exact_equal,
                       points[vi] == vertex.exact_coordinate
                           ? realization_relation::exact_equal
                           : realization_relation::distinct,
                       {}}))
      return false;
    for (auto node_id : vertex.derivations) {
      if (!node_id.valid() ||
          node_id.value_for_debug() >= a.constructions->nodes.size())
        return false;
      const auto &node = a.constructions->nodes[node_id.value_for_debug()];
      if (node.id != node_id)
        return false;
      for (auto relation_id : node.defining_relations) {
        if (!relation_id.valid() ||
            relation_id.value_for_debug() >= a.constructions->relations.size())
          return false;
        const auto &relation =
            a.constructions->relations[relation_id.value_for_debug()];
        realization_obligation expected{
            {}, realization_obligation_kind::defining_relation, 1,
            {vertex.id}, {}, {}, {}, realization_relation::exact_equal,
            defining_relation_satisfied(relation, points[vi])
                ? realization_relation::exact_equal
                : realization_relation::distinct,
            {}};
        expected.defining_relation = relation_id;
        if (relation.id != relation_id || relation.construction != node_id ||
            !add_expected(std::move(expected)))
          return false;
      }
    }
    if (!add_expected({{}, realization_obligation_kind::finite_coordinate, 1,
                       {vertex.id}, {}, {}, {}, realization_relation::finite,
                       realization_relation::finite, {}}))
      return false;
    if (vertex.preserved_source) {
      bool fixed = bool(vertex.preserved_source_bits);
      if (fixed)
        for (std::size_t axis = 0; axis < 3; ++axis)
          fixed = fixed &&
                  vertex.accepted_bits[axis].bits ==
                      (*vertex.preserved_source_bits)[axis].bits;
      if (!add_expected({{},
                         realization_obligation_kind::fixed_original_bits, 1,
                         {vertex.id}, {}, {}, {},
                         realization_relation::equal_bits,
                         fixed ? realization_relation::equal_bits
                               : realization_relation::distinct,
                         {}}))
        return false;
    }
  }

  for (const auto &triangle : a.triangles) {
    for (auto vertex : triangle.vertices)
      if (vertex.value_for_debug() >= points.size())
        return false;
    const auto actual_sign = orient2d(
        project(points[triangle.vertices[0].value_for_debug()],
                triangle.projection),
        project(points[triangle.vertices[1].value_for_debug()],
                triangle.projection),
        project(points[triangle.vertices[2].value_for_debug()],
                triangle.projection));
    const auto expected_relation = sign_relation(triangle.exact_orientation);
    const auto actual_relation = sign_relation(actual_sign);
    if (!expected_relation || !actual_relation ||
        !add_expected({{}, realization_obligation_kind::triangle_orientation,
                       1,
                       {triangle.vertices[0], triangle.vertices[1],
                        triangle.vertices[2]},
                       {triangle.id}, {}, {triangle.patch}, *expected_relation,
                       *actual_relation, {}}))
      return false;
  }

  for (const auto &edge : a.selected->payload->edges) {
    const auto lower = realization_vertex_id::from_canonical_value(
        edge.lower.value_for_debug());
    const auto upper = realization_vertex_id::from_canonical_value(
        edge.upper.value_for_debug());
    if (lower.value_for_debug() >= points.size() ||
        upper.value_for_debug() >= points.size())
      return false;
    if (!add_expected({{}, realization_obligation_kind::selected_edge_order, 1,
                       {lower, upper}, {}, {edge.id}, {},
                       realization_relation::distinct,
                       points[lower.value_for_debug()] ==
                               points[upper.value_for_debug()]
                           ? realization_relation::exact_equal
                           : realization_relation::distinct,
                       {}}))
      return false;
  }

  for (const auto &pair : a.pair_candidates) {
    const auto xi = pair.lower.value_for_debug();
    const auto yi = pair.upper.value_for_debug();
    if (xi >= a.triangles.size() || yi >= a.triangles.size())
      return false;
    const auto &x = a.triangles[xi];
    const auto &y = a.triangles[yi];
    std::vector<realization_vertex_id> shared;
    for (auto xv : x.vertices)
      for (auto yv : y.vertices)
        if (xv == yv)
          shared.push_back(xv);
    std::vector<realization_vertex_id> participants(x.vertices.begin(),
                                                     x.vertices.end());
    participants.insert(participants.end(), y.vertices.begin(), y.vertices.end());
    std::sort(participants.begin(), participants.end());
    participants.erase(std::unique(participants.begin(), participants.end()),
                       participants.end());
    for (auto vertex : participants)
      if (vertex.value_for_debug() >= points.size())
        return false;
    const auto actual = intersection_relation(relate_triangles(
        {points[x.vertices[0].value_for_debug()],
         points[x.vertices[1].value_for_debug()],
         points[x.vertices[2].value_for_debug()]},
        {points[y.vertices[0].value_for_debug()],
         points[y.vertices[1].value_for_debug()],
         points[y.vertices[2].value_for_debug()]}));
    const auto expected = shared.empty()
                              ? realization_relation::disjoint
                              : shared.size() == 1
                                    ? realization_relation::point
                                    : realization_relation::segment;
    const auto kind = shared.empty()
                          ? realization_obligation_kind::nonadjacent_disjointness
                          : shared.size() == 1
                                ? realization_obligation_kind::shared_vertex_relation
                                : realization_obligation_kind::shared_edge_relation;
    if (!add_expected({{}, kind, 1, participants, {x.id, y.id}, {},
                       {x.patch, y.patch}, expected, actual, {}}))
      return false;
  }

  for (const auto &patch : a.selected->payload->patches)
    if (!add_expected({{}, realization_obligation_kind::patch_embedding, 1, {},
                       {}, {}, {patch.id}, realization_relation::embedded,
                       realization_relation::embedded, {}}))
      return false;
  if (!add_expected({{}, realization_obligation_kind::global_embedding, 1, {},
                     {}, {}, {}, realization_relation::embedded,
                     realization_relation::embedded, {}}) ||
      expected_index != a.obligations.size())
    return false;
  for (std::size_t vi = 0; vi < a.vertices.size(); ++vi)
    if (a.vertices[vi].obligations != vertex_obligations[vi])
      return false;

  std::vector<std::size_t> parent(a.vertices.size());
  std::iota(parent.begin(), parent.end(), std::size_t(0));
  const auto root = [&](std::size_t x) {
    while (parent[x] != x)
      x = parent[x];
    return x;
  };
  for (const auto &obligation : a.obligations) {
    for (auto vertex : obligation.vertices)
      if (vertex.value_for_debug() >= parent.size())
        return false;
    for (std::size_t i = 1; i < obligation.vertices.size(); ++i) {
      auto x = root(obligation.vertices[0].value_for_debug());
      auto y = root(obligation.vertices[i].value_for_debug());
      if (x != y)
        parent[std::max(x, y)] = std::min(x, y);
    }
  }
  std::map<std::size_t, std::vector<realization_vertex_id>> variables;
  std::map<std::size_t, std::vector<realization_obligation_id>> obligations;
  for (const auto &vertex : a.vertices)
    variables[root(vertex.id.value_for_debug())].push_back(vertex.id);
  for (const auto &obligation : a.obligations)
    if (!obligation.vertices.empty())
      obligations[root(obligation.vertices.front().value_for_debug())].push_back(
          obligation.id);
  if (variables.size() != a.components.size())
    return false;
  std::size_t component_index = 0;
  std::uint64_t nodes = 0, complete = 0;
  for (const auto &entry : variables) {
    const auto &component = a.components[component_index];
    const auto &transcript = a.component_transcripts[component_index];
    if (component.id.value_for_debug() != component_index ||
        component.variables != entry.second ||
        component.obligations != obligations[entry.first] ||
        transcript.component != component.id ||
        transcript.accepted_ranks.size() != component.variables.size() ||
        !transcript.rejected_prefix_witnesses.empty() ||
        transcript.visited_nodes != component.variables.size() + 1 ||
        transcript.complete_assignments != 1)
      return false;
    for (std::size_t i = 0; i < component.variables.size(); ++i)
      if (transcript.accepted_ranks[i] != 0 ||
          transcript.accepted_ranks[i] !=
          a.vertices[component.variables[i].value_for_debug()].accepted_point_rank)
        return false;
    for (auto witness : transcript.rejected_prefix_witnesses)
      if (!std::binary_search(component.obligations.begin(),
                              component.obligations.end(), witness))
        return false;
    nodes += transcript.visited_nodes;
    complete += transcript.complete_assignments;
    ++component_index;
  }
  return nodes == a.search.visited_nodes &&
         complete == a.search.complete_assignments;
}

struct realization_sweep_metrics {
  std::uint64_t candidates = 0;
  std::uint64_t peak_index_entries = 0;
};

realization_sweep_metrics realization_count_indexed_candidates(
    const std::vector<realization_domain_box> &boxes) {
  realization_sweep_metrics result;
  std::vector<std::size_t> order(boxes.size());
  std::iota(order.begin(), order.end(), std::size_t(0));
  std::sort(order.begin(), order.end(), [&](std::size_t x, std::size_t y) {
    return boxes[x].lower.x == boxes[y].lower.x
               ? boxes[x].triangle < boxes[y].triangle
               : boxes[x].lower.x < boxes[y].lower.x;
  });
  std::vector<exact_scalar> coordinates;
  coordinates.reserve(boxes.size());
  for (const auto &box : boxes) coordinates.push_back(box.lower.y);
  std::sort(coordinates.begin(), coordinates.end());
  coordinates.erase(std::unique(coordinates.begin(), coordinates.end()),
                    coordinates.end());
  using key = std::pair<exact_scalar, realization_triangle_id>;
  std::vector<std::map<key, std::size_t>> tree(
      coordinates.empty() ? 0 : 4 * coordinates.size());
  std::uint64_t live_entries = 0;
  const auto update = [&](auto &&self, std::size_t node, std::size_t begin,
                          std::size_t end, std::size_t position,
                          std::size_t box_index, bool insert) -> bool {
    const key entry{boxes[box_index].upper.y, boxes[box_index].triangle};
    if (insert) {
      if (!tree[node].emplace(entry, box_index).second) return false;
      ++live_entries;
      result.peak_index_entries =
          std::max(result.peak_index_entries, live_entries);
    } else {
      if (tree[node].erase(entry) != 1) return false;
      --live_entries;
    }
    if (end - begin == 1) return true;
    const auto middle = begin + (end - begin) / 2;
    return position < middle
               ? self(self, 2 * node + 1, begin, middle, position, box_index,
                      insert)
               : self(self, 2 * node + 2, middle, end, position, box_index,
                      insert);
  };
  const auto query = [&](auto &&self, std::size_t node, std::size_t begin,
                         std::size_t end, std::size_t prefix,
                         const realization_domain_box &box) -> bool {
    if (begin >= prefix) return true;
    if (end <= prefix) {
      const key first{box.lower.y,
                      realization_triangle_id::from_canonical_value(0)};
      const auto count = static_cast<std::uint64_t>(
          std::distance(tree[node].lower_bound(first), tree[node].end()));
      if (result.candidates >
          std::numeric_limits<std::uint64_t>::max() - count)
        return false;
      result.candidates += count;
      return true;
    }
    const auto middle = begin + (end - begin) / 2;
    return self(self, 2 * node + 1, begin, middle, prefix, box) &&
           self(self, 2 * node + 2, middle, end, prefix, box);
  };
  std::map<std::pair<exact_scalar, realization_triangle_id>, std::size_t> expiry;
  for (const auto box_index : order) {
    const auto &box = boxes[box_index];
    while (!expiry.empty() && expiry.begin()->first.first < box.lower.x) {
      const auto expired = expiry.begin()->second;
      expiry.erase(expiry.begin());
      const auto position = static_cast<std::size_t>(std::lower_bound(
          coordinates.begin(), coordinates.end(), boxes[expired].lower.y) -
                                                     coordinates.begin());
      if (!update(update, 0, 0, coordinates.size(), position, expired, false))
        throw std::logic_error("realization count index removal");
    }
    const auto prefix = static_cast<std::size_t>(std::upper_bound(
        coordinates.begin(), coordinates.end(), box.upper.y) -
                                                 coordinates.begin());
    if (!query(query, 0, 0, coordinates.size(), prefix, box))
      throw std::overflow_error("realization indexed candidates");
    const auto position = static_cast<std::size_t>(std::lower_bound(
        coordinates.begin(), coordinates.end(), box.lower.y) -
                                                 coordinates.begin());
    if (!update(update, 0, 0, coordinates.size(), position, box_index, true) ||
        !expiry.emplace(std::make_pair(box.upper.x, box.triangle), box_index)
             .second)
      throw std::logic_error("realization count index insertion");
  }
  return result;
}

template <class T, class I>
status_or<std::array<std::uint64_t, 2>> realization_verifier_resources(
    const realized_boundary<T, I> &a) {
  const auto n = static_cast<std::uint64_t>(a.pair_boxes.size());
  const auto metrics = realization_count_indexed_candidates(a.pair_boxes);
  auto index_bytes = checked_multiply(
      metrics.peak_index_entries,
      sizeof(std::pair<const std::pair<exact_scalar, realization_triangle_id>,
                        std::size_t>) + 4 * sizeof(void *),
      boolean_stage::geometry_realization);
  if (!index_bytes.has_value())
    return index_bytes.error();
  auto pair_bytes = checked_multiply(a.pair_candidates.size(),
                                     sizeof(realization_triangle_pair),
                                     boolean_stage::geometry_realization);
  if (!pair_bytes.has_value())
    return pair_bytes.error();
  auto scratch = checked_add(index_bytes.value(), pair_bytes.value(),
                              boolean_stage::geometry_realization);
  if (!scratch.has_value())
    return scratch.error();
  const auto add_scratch = [&](std::uint64_t count, std::uint64_t size) {
    auto bytes = checked_multiply(count, size,
                                  boolean_stage::geometry_realization);
    if (!bytes.has_value()) return status_or<bool>(bytes.error());
    auto total = checked_add(scratch.value(), bytes.value(),
                             boolean_stage::geometry_realization);
    if (!total.has_value()) return status_or<bool>(total.error());
    scratch = total;
    return status_or<bool>(true);
  };
  std::uint64_t obligation_vertices = 0;
  for (const auto &obligation : a.obligations)
    obligation_vertices += obligation.vertices.size();
  for (const auto item : std::array<std::pair<std::uint64_t, std::uint64_t>, 11>{{
           {a.axis_domains.size(), sizeof(exact_scalar)},
           {n, sizeof(std::size_t) + sizeof(exact_scalar) +
                   4 * sizeof(std::map<std::pair<exact_scalar,
                                                realization_triangle_id>,
                                       std::size_t>)},
           {n, sizeof(std::pair<const std::pair<exact_scalar,
                                               realization_triangle_id>,
                                      std::size_t>) + 4 * sizeof(void *)},
           {a.vertices.size(), sizeof(exact_point3)},
           {a.vertices.size(), sizeof(std::vector<realization_obligation_id>) +
                                   sizeof(std::size_t)},
           {obligation_vertices, sizeof(realization_obligation_id)},
           {a.vertices.size(), sizeof(realization_vertex_id)},
           {a.obligations.size(), sizeof(realization_obligation_id)},
           {a.components.size(),
            2 * sizeof(std::map<std::size_t, std::vector<std::size_t>>)},
           {a.pair_candidates.size(), sizeof(realization_triangle_pair)},
           {n, sizeof(std::size_t)}}}) {
    auto added = add_scratch(item.first, item.second);
    if (!added.has_value()) return added.error();
  }
  auto linear = checked_add(a.vertices.size(), a.obligations.size(),
                             boolean_stage::geometry_realization);
  if (!linear.has_value())
    return linear.error();
  auto work = checked_add(metrics.candidates, linear.value(),
                           boolean_stage::geometry_realization);
  if (!work.has_value())
    return work.error();
  return std::array<std::uint64_t, 2>{{scratch.value(), work.value()}};
}

template <class T, class I>
status_or<bool> verify_realization_constraint_evidence_checked(
    const realized_boundary<T, I> &a, resource_accountant *accountant) {
  std::optional<resource_reservation> scratch_charge, work_charge;
  if (accountant) {
    auto resources = realization_verifier_resources(a);
    if (!resources.has_value())
      return resources.error();
    auto scratch = accountant->reserve_scoped(
        resource_kind::verifier_scratch_bytes, resources.value()[0],
        boolean_stage::geometry_realization);
    if (!scratch.has_value())
      return scratch.error();
    scratch_charge.emplace(std::move(scratch.value()));
    auto work = accountant->reserve_scoped(resource_kind::verifier_work,
                                           resources.value()[1],
                                           boolean_stage::geometry_realization);
    if (!work.has_value())
      return work.error();
    work_charge.emplace(std::move(work.value()));
  }
  return verify_realization_constraint_evidence_impl(a);
}

template <class T, class I>
bool verify_realization_constraint_evidence(const realized_boundary<T, I> &a) {
  const auto checked = verify_realization_constraint_evidence_checked(a, nullptr);
  return checked.has_value() && checked.value();
}

#define YGOR_INSTANTIATE(T, I)                                                 \
  template bool verify_realization_exact_substitution(                         \
      const realized_boundary<T, I> &);                                        \
  template bool verify_realization_constraint_evidence(                        \
      const realized_boundary<T, I> &);                                        \
  template status_or<bool> verify_realization_constraint_evidence_checked(     \
      const realized_boundary<T, I> &, resource_accountant *)
YGOR_INSTANTIATE(float, std::uint32_t);
YGOR_INSTANTIATE(float, std::uint64_t);
YGOR_INSTANTIATE(double, std::uint32_t);
YGOR_INSTANTIATE(double, std::uint64_t);
#undef YGOR_INSTANTIATE

} // namespace mesh_boolean
} // namespace ygor
