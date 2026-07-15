#include "YgorMeshesBooleanRealization.h"
#include <algorithm>
#include <map>
#include <numeric>

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
  if (a.pair_boxes.size() != a.triangles.size() ||
      a.components.size() != a.component_transcripts.size())
    return false;
  if (a.axis_domains.size() != 3 * a.vertices.size())
    return false;
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
        const auto &domain = a.axis_domains[3 * vi + axis];
        for (const auto &candidate : domain.values) {
          const auto decoded = decode_coordinate<T>(
              candidate.bits, boolean_stage::geometry_realization);
          if (!decoded.has_value())
            return false;
          const auto &value = decoded.value().value;
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
      a.search.pair_checks != overlap_checks + pairs.size())
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
