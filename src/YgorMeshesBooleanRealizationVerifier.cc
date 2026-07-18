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
bool verify_realization_constraint_evidence(const realized_boundary<T, I> &a) {
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
  auto boxes = a.pair_boxes;
  std::sort(boxes.begin(), boxes.end(), [](const auto &x, const auto &y) {
    if (x.lower.x != y.lower.x)
      return x.lower.x < y.lower.x;
    return x.triangle < y.triangle;
  });
  std::vector<const realization_domain_box *> active;
  std::vector<realization_triangle_pair> pairs;
  std::uint64_t overlap_checks = 0;
  const auto overlaps = [](const exact_scalar &alo, const exact_scalar &ahi,
                           const exact_scalar &blo, const exact_scalar &bhi) {
    return !(ahi < blo) && !(bhi < alo);
  };
  for (const auto &box : boxes) {
    active.erase(std::remove_if(active.begin(), active.end(), [&](const auto *x) {
                   return x->upper.x < box.lower.x;
                 }), active.end());
    for (const auto *other : active) {
      if (overlap_checks == std::numeric_limits<std::uint64_t>::max())
        return false;
      ++overlap_checks;
      if (overlaps(other->lower.y, other->upper.y, box.lower.y, box.upper.y) &&
          overlaps(other->lower.z, other->upper.z, box.lower.z, box.upper.z)) {
        const auto pair = std::minmax(other->triangle, box.triangle);
        pairs.push_back({pair.first, pair.second});
      }
    }
    active.push_back(&box);
  }
  std::sort(pairs.begin(), pairs.end(), [](const auto &x, const auto &y) {
    return std::tie(x.lower, x.upper) < std::tie(y.lower, y.upper);
  });
  pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
  if (pairs != a.pair_candidates ||
      overlap_checks > std::numeric_limits<std::uint64_t>::max() - pairs.size() ||
      a.search.pair_checks != overlap_checks + pairs.size())
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

#define YGOR_INSTANTIATE(T, I)                                                 \
  template bool verify_realization_exact_substitution(                         \
      const realized_boundary<T, I> &);                                        \
  template bool verify_realization_constraint_evidence(                        \
      const realized_boundary<T, I> &)
YGOR_INSTANTIATE(float, std::uint32_t);
YGOR_INSTANTIATE(float, std::uint64_t);
YGOR_INSTANTIATE(double, std::uint32_t);
YGOR_INSTANTIATE(double, std::uint64_t);
#undef YGOR_INSTANTIATE

} // namespace mesh_boolean
} // namespace ygor
