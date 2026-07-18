#include "YgorMeshesBooleanGlobalArrangement.h"

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <tuple>

#if defined(__FAST_MATH__)
#error "Component 8 verifier requires strict floating-point compilation"
#endif

namespace ygor { namespace mesh_boolean { namespace {

using local_key = std::pair<facet_id, std::uint64_t>;

struct verifier_indices {
  std::map<symbolic_vertex_id, global_vertex_id> vertex_by_symbolic;
  std::map<local_key, global_vertex_id> vertex_by_local;
  std::map<local_key, global_atomic_edge_id> edge_by_local;
  std::map<local_key, global_halfedge_id> halfedge_by_local;
  std::map<local_key, sheet_use_id> use_by_local_patch;
  std::vector<std::vector<sheet_use_id>> uses_by_edge;
  std::vector<std::vector<sheet_use_id>> uses_by_facet;
};

template<class Id>
bool canonical_id(Id id, std::size_t ordinal) {
  return id.value_for_debug() == ordinal;
}

template<class Id>
bool valid_id(Id id, std::size_t size) {
  return id.value_for_debug() < size;
}

bool equal_vector(const exact_vector3 &a, const exact_vector3 &b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

struct verifier_direction_key {
  std::uint8_t axis = 0;
  bool negative = false;
  std::array<exact_scalar, 3> ratio;
};

struct verifier_direction_entry {
  verifier_direction_key key;
  std::size_t ordinal = 0;
};

bool direction_key_less(const verifier_direction_key &a,
                        const verifier_direction_key &b) {
  if (a.axis != b.axis) return a.axis < b.axis;
  if (a.negative != b.negative) return a.negative < b.negative;
  return a.ratio < b.ratio;
}

bool direction_key_equal(const verifier_direction_key &a,
                         const verifier_direction_key &b) {
  return a.axis == b.axis && a.negative == b.negative && a.ratio == b.ratio;
}

std::uint64_t direction_sort_levels(std::size_t count) {
  std::uint64_t levels = 0;
  for (std::size_t width = 1; width < count;) {
    ++levels;
    if (width > count / 2) break;
    width *= 2;
  }
  return levels;
}

void sort_direction_entries(std::vector<verifier_direction_entry> &entries,
                            std::vector<verifier_direction_entry> &scratch) {
  const auto count = entries.size();
  scratch.resize(count);
  for (std::size_t width = 1; width < count;) {
    for (std::size_t first = 0; first < count;) {
      const auto middle = std::min(first + width, count);
      const auto span = std::min(width, count - middle);
      const auto last = middle + span;
      std::size_t left = first, right = middle, output = first;
      while (left < middle && right < last) {
        performance_count(
            performance_counter::link_direction_sort_comparisons);
        if (direction_key_less(entries[right].key, entries[left].key))
          scratch[output++] = std::move(entries[right++]);
        else
          scratch[output++] = std::move(entries[left++]);
      }
      while (left < middle) scratch[output++] = std::move(entries[left++]);
      while (right < last) scratch[output++] = std::move(entries[right++]);
      first = last;
    }
    entries.swap(scratch);
    if (width > count / 2) break;
    width *= 2;
  }
}

void sort_incident_edges(std::vector<global_atomic_edge_id> &edges,
                         std::vector<global_atomic_edge_id> &scratch) {
  const auto count = edges.size();
  scratch.resize(count);
  for (std::size_t width = 1; width < count;) {
    for (std::size_t first = 0; first < count;) {
      const auto middle = std::min(first + width, count);
      const auto span = std::min(width, count - middle);
      const auto last = middle + span;
      std::size_t left = first, right = middle, output = first;
      while (left < middle && right < last) {
        performance_count(
            performance_counter::link_direction_sort_comparisons);
        if (edges[right] < edges[left])
          scratch[output++] = edges[right++];
        else
          scratch[output++] = edges[left++];
      }
      while (left < middle) scratch[output++] = edges[left++];
      while (right < last) scratch[output++] = edges[right++];
      first = last;
    }
    edges.swap(scratch);
    if (width > count / 2) break;
    width *= 2;
  }
}

bool same_positive_direction(const exact_vector3 &a, const exact_vector3 &b) {
  performance_count(performance_counter::exact_link_direction_tests);
  const auto c = cross(a, b);
  return c.x.sign() == exact_sign::zero &&
         c.y.sign() == exact_sign::zero &&
         c.z.sign() == exact_sign::zero &&
         dot_sign_exact(a, b) == exact_sign::positive;
}

template<class T, class I>
bool check_binding(const arrangement_complex<T, I> &a, const artifact_view &v,
                   const verification_environment_view &env) {
  return a.refined && a.symbolic && a.validated && a.owner == v.owner &&
         a.owner == a.refined->owner && a.owner == a.symbolic->owner &&
         a.owner == a.validated->owner && a.setup_digest == env.setup_digest &&
         a.refined_digest == a.refined->artifact_digest &&
         a.symbolic_digest == a.symbolic->artifact_digest &&
         a.validated_digest == a.validated->artifact_digest &&
         a.refined->payload && a.symbolic->payload && a.validated->payload &&
         a.refined->payload->symbolic.get() == a.symbolic.get() &&
         a.refined->payload->validated.get() == a.validated.get() &&
         a.refined->payload->constructions == a.constructions &&
         a.refined->payload->kernel_policy_digest == a.kernel_policy_digest;
}

bool same_map(const local_entity_image &actual, local_map_kind kind, facet_id facet,
              std::uint64_t local, const std::vector<std::uint64_t> &fragments,
              bool incidence = false) {
  return actual.kind == kind && actual.facet == facet &&
         actual.local_id == local && actual.global_fragments == fragments &&
         actual.retained_incidence_only == incidence;
}

template<class T, class I>
bool check_maps(const arrangement_complex<T, I> &a, verifier_indices &index) {
  const auto &facets = a.refined->payload->facets;
  std::uint64_t local_vertices = 0, local_edges = 0, local_halfedges = 0,
                local_walks = 0, local_faces = 0, local_patches = 0;
  index.uses_by_edge.assign(a.edges.size(), {});
  index.uses_by_facet.assign(a.validated->payload->facets.size(), {});

  for (std::size_t i = 0; i < a.vertices.size(); ++i) {
    const auto &vertex = a.vertices[i];
    if (!canonical_id(vertex.id, i) ||
        !valid_id(vertex.symbolic, a.symbolic->payload->vertices.size()) ||
        !index.vertex_by_symbolic.emplace(vertex.symbolic, vertex.id).second)
      return false;
    for (const auto &local : vertex.local_occurrences)
      if (!index.vertex_by_local
               .emplace(local_key{local.facet, local.id.value_for_debug()},
                        vertex.id)
               .second)
        return false;
  }
  for (std::size_t i = 0; i < a.edges.size(); ++i) {
    const auto &edge = a.edges[i];
    if (!canonical_id(edge.id, i)) return false;
    for (const auto &local : edge.local_occurrences)
      if (!index.edge_by_local
               .emplace(local_key{local.facet, local.id.value_for_debug()}, edge.id)
               .second)
        return false;
  }
  for (std::size_t i = 0; i < a.sheet_uses.size(); ++i) {
    const auto &use = a.sheet_uses[i];
    if (!canonical_id(use.id, i) || !valid_id(use.patch, a.patches.size()) ||
        !valid_id(use.facet, index.uses_by_facet.size()) ||
        !index.use_by_local_patch
             .emplace(local_key{use.local_patch.facet,
                                use.local_patch.id.value_for_debug()}, use.id)
             .second)
      return false;
    index.uses_by_facet[use.facet.value_for_debug()].push_back(use.id);
  }
  for (std::size_t i = 0; i < a.halfedges.size(); ++i) {
    const auto &halfedge = a.halfedges[i];
    if (!canonical_id(halfedge.id, i) || !valid_id(halfedge.edge, a.edges.size()) ||
        !valid_id(halfedge.use, a.sheet_uses.size()) ||
        !index.halfedge_by_local
             .emplace(local_key{halfedge.local.facet,
                                halfedge.local.id.value_for_debug()}, halfedge.id)
             .second)
      return false;
    index.uses_by_edge[halfedge.edge.value_for_debug()].push_back(halfedge.use);
  }
  for (auto &uses : index.uses_by_edge) {
    std::sort(uses.begin(), uses.end());
    uses.erase(std::unique(uses.begin(), uses.end()), uses.end());
  }
  for (auto &uses : index.uses_by_facet)
    std::sort(uses.begin(), uses.end());

  std::size_t cursor = 0;
  auto expect = [&](local_map_kind kind, facet_id facet, std::uint64_t local,
                    std::vector<std::uint64_t> fragments,
                    bool incidence = false) {
    return cursor < a.local_maps.size() &&
           same_map(a.local_maps[cursor++], kind, facet, local, fragments,
                    incidence);
  };
  for (const auto &facet : facets) {
    local_vertices += facet.vertices.size();
    local_edges += facet.edges.size();
    local_halfedges += facet.halfedges.size();
    local_walks += facet.walks.size();
    local_faces += facet.faces.size();
    local_patches += facet.patches.size();
    for (const auto &vertex : facet.vertices) {
      const local_key key{facet.facet, vertex.id.id.value_for_debug()};
      const auto found = index.vertex_by_local.find(key);
      if (found == index.vertex_by_local.end()) return false;
      const std::vector<std::uint64_t> fragments{found->second.value_for_debug()};
      if (!expect(local_map_kind::vertex, facet.facet, key.second, fragments) ||
          !expect(local_map_kind::point_incidence, facet.facet, key.second,
                  fragments))
        return false;
    }
    for (const auto &edge : facet.edges) {
      const local_key key{facet.facet, edge.id.id.value_for_debug()};
      const auto found = index.edge_by_local.find(key);
      if (found == index.edge_by_local.end() ||
          !expect(edge.artificial ? local_map_kind::artificial_cut
                                  : local_map_kind::edge,
                  facet.facet, key.second,
                  {found->second.value_for_debug()}))
        return false;
    }
    for (const auto &halfedge : facet.halfedges) {
      const local_key key{facet.facet, halfedge.id.id.value_for_debug()};
      const auto found = index.halfedge_by_local.find(key);
      if (found != index.halfedge_by_local.end()) {
        if (!expect(local_map_kind::halfedge, facet.facet, key.second,
                    {found->second.value_for_debug()}))
          return false;
      } else {
        const auto edge = index.edge_by_local.find(
            {facet.facet, halfedge.edge.id.value_for_debug()});
        if (edge == index.edge_by_local.end() ||
            !expect(local_map_kind::halfedge, facet.facet, key.second,
                    {edge->second.value_for_debug()}, true))
          return false;
      }
    }
    for (const auto &walk : facet.walks) {
      std::vector<std::uint64_t> fragments;
      for (const auto halfedge : walk.halfedges) {
        const auto found = index.halfedge_by_local.find(
            {facet.facet, halfedge.id.value_for_debug()});
        if (found != index.halfedge_by_local.end())
          fragments.push_back(found->second.value_for_debug());
      }
      const bool incidence = fragments.empty();
      if (!expect(local_map_kind::boundary_walk, facet.facet,
                  walk.id.id.value_for_debug(), std::move(fragments), incidence))
        return false;
    }
    for (const auto &face : facet.faces) {
      std::vector<std::uint64_t> fragments;
      for (const auto &patch : facet.patches)
        if (patch.parent_face.id == face.id.id) {
          const auto use = index.use_by_local_patch.find(
              {facet.facet, patch.id.id.value_for_debug()});
          if (use == index.use_by_local_patch.end()) return false;
          fragments.push_back(use->second.value_for_debug());
        }
      std::sort(fragments.begin(), fragments.end());
      const bool incidence = fragments.empty();
      if (!expect(local_map_kind::face, facet.facet,
                  face.id.id.value_for_debug(), std::move(fragments), incidence))
        return false;
    }
    for (const auto &patch : facet.patches) {
      const auto use = index.use_by_local_patch.find(
          {facet.facet, patch.id.id.value_for_debug()});
      if (use == index.use_by_local_patch.end() ||
          !expect(local_map_kind::patch, facet.facet,
                  patch.id.id.value_for_debug(),
                  {use->second.value_for_debug()}))
        return false;
    }
    for (std::size_t i = 0; i < facet.source_boundary.size(); ++i) {
      std::vector<std::uint64_t> fragments;
      for (const auto edge : facet.source_boundary[i].edges) {
        const auto found = index.edge_by_local.find(
            {facet.facet, edge.id.value_for_debug()});
        if (found == index.edge_by_local.end()) return false;
        fragments.push_back(found->second.value_for_debug());
      }
      if (!expect(local_map_kind::source_chain, facet.facet, i,
                  std::move(fragments)))
        return false;
    }
    for (std::size_t i = 0; i < facet.constraints.size(); ++i) {
      std::vector<std::uint64_t> fragments;
      for (const auto edge : facet.constraints[i].edges) {
        const auto found = index.edge_by_local.find(
            {facet.facet, edge.id.value_for_debug()});
        if (found == index.edge_by_local.end()) return false;
        fragments.push_back(found->second.value_for_debug());
      }
      if (!expect(local_map_kind::constraint_chain, facet.facet, i,
                  std::move(fragments)))
        return false;
    }
  }
  return cursor == a.local_maps.size() &&
         local_vertices == a.certificate.local_vertices &&
         local_edges == a.certificate.local_edges &&
         local_halfedges == a.certificate.local_halfedges &&
         local_walks == a.certificate.local_walks &&
         local_faces == a.certificate.local_faces &&
         local_patches == a.certificate.local_patches;
}

template<class T, class I>
bool check_cycles(const arrangement_complex<T, I> &a) {
  std::uint64_t patch_cycles = 0;
  for (std::size_t i = 0; i < a.patches.size(); ++i) {
    const auto &patch = a.patches[i];
    if (!canonical_id(patch.id, i) || patch.outer.size() < 3 ||
        !(exact_scalar(0) < patch.projected_double_area))
      return false;
    for (auto vertex : patch.outer)
      if (!valid_id(vertex, a.vertices.size())) return false;
    for (const auto &hole : patch.holes) {
      if (hole.size() < 3) return false;
      for (auto vertex : hole)
        if (!valid_id(vertex, a.vertices.size())) return false;
    }
  }
  for (std::size_t i = 0; i < a.sheet_members.size(); ++i)
    if (!canonical_id(a.sheet_members[i].id, i)) return false;
  for (std::size_t i = 0; i < a.sheet_uses.size(); ++i) {
    const auto &use = a.sheet_uses[i];
    if (!canonical_id(use.id, i) || !valid_id(use.member, a.sheet_members.size()) ||
        !valid_id(use.patch, a.patches.size()) || use.boundary.empty())
      return false;
    std::set<global_halfedge_id> seen;
    for (auto start : use.boundary) {
      if (!valid_id(start, a.halfedges.size())) return false;
      if (!seen.insert(start).second) continue;
      ++patch_cycles;
      auto halfedge = start;
      std::size_t steps = 0;
      do {
        if (!valid_id(halfedge, a.halfedges.size()) ||
            ++steps > a.halfedges.size())
          return false;
        const auto &current = a.halfedges[halfedge.value_for_debug()];
        if (current.use != use.id ||
            (!seen.insert(current.next).second && current.next != start))
          return false;
        halfedge = current.next;
      } while (halfedge != start);
    }
    if (seen.size() != use.boundary.size()) return false;
  }
  std::uint64_t mate_pairs = 0;
  for (std::size_t i = 0; i < a.halfedges.size(); ++i) {
    const auto &h = a.halfedges[i];
    if (!canonical_id(h.id, i) || !valid_id(h.next, a.halfedges.size()) ||
        !valid_id(h.previous, a.halfedges.size()) ||
        !valid_id(h.sheet_mate, a.halfedges.size()) ||
        !valid_id(h.edge, a.edges.size()) || !valid_id(h.use, a.sheet_uses.size()) ||
        !valid_id(h.origin, a.vertices.size()) ||
        !valid_id(h.destination, a.vertices.size()) ||
        !valid_id(h.origin_occurrence, a.vertex_occurrences.size()) ||
        !valid_id(h.destination_occurrence, a.vertex_occurrences.size()))
      return false;
    const auto &next = a.halfedges[h.next.value_for_debug()];
    const auto &previous = a.halfedges[h.previous.value_for_debug()];
    const auto &mate = a.halfedges[h.sheet_mate.value_for_debug()];
    if (next.previous != h.id || previous.next != h.id ||
        mate.sheet_mate != h.id || mate.origin != h.destination ||
        mate.destination != h.origin ||
        mate.origin_occurrence != h.destination_occurrence ||
        mate.destination_occurrence != h.origin_occurrence)
      return false;
    if (h.id < h.sheet_mate) ++mate_pairs;
  }
  for (std::size_t i = 0; i < a.edges.size(); ++i) {
    const auto &edge = a.edges[i];
    if (!canonical_id(edge.id, i) || !valid_id(edge.lower, a.vertices.size()) ||
        !valid_id(edge.upper, a.vertices.size()) || !(edge.lower < edge.upper) ||
        static_cast<unsigned>(edge.kind) > 3)
      return false;
  }
  return patch_cycles == a.certificate.patch_cycles &&
         mate_pairs == a.certificate.mate_pairs;
}

template<class T, class I>
bool check_seams(const arrangement_complex<T, I> &a,
                 const verifier_indices &index) {
  std::size_t seam_cursor = 0, seam_sector_cursor = 0, source_cursor = 0;
  for (const auto &edge : a.edges) {
    const auto &uses = index.uses_by_edge[edge.id.value_for_debug()];
    if (uses.size() < 2) continue;
    const auto &p = a.symbolic->payload
                        ->vertices[a.vertices[edge.lower.value_for_debug()]
                                       .symbolic.value_for_debug()].point;
    const auto &q = a.symbolic->payload
                        ->vertices[a.vertices[edge.upper.value_for_debug()]
                                       .symbolic.value_for_debug()].point;
    std::vector<exact_plane3> planes;
    for (auto use : uses) {
      const auto facet = a.sheet_uses[use.value_for_debug()].facet;
      if (!valid_id(facet, a.validated->payload->facets.size())) return false;
      planes.push_back(a.validated->payload->facets[facet.value_for_debug()].plane);
    }
    const auto order = rank_planes_around_carrier(q - p, planes);
    if (!order.has_value()) return false;
    if (edge.kind == global_edge_kind::intersection_seam) {
      if (seam_cursor >= a.seams.size()) return false;
      const auto &seam = a.seams[seam_cursor];
      if (!canonical_id(seam.id, seam_cursor) || seam.edge != edge.id ||
          seam.incident_uses != uses ||
          seam.radial_layers.size() != order.value().layers.size())
        return false;
      for (std::size_t layer = 0; layer < seam.radial_layers.size(); ++layer) {
        std::vector<sheet_use_id> expected;
        for (auto member : order.value().layers[layer].members)
          expected.push_back(uses[member]);
        if (seam.radial_layers[layer].uses != expected) return false;
      }
      const auto expected_sectors = seam.radial_layers.size() > 1
                                        ? seam.radial_layers.size() : 0;
      if (seam.sectors.size() != expected_sectors) return false;
      for (std::size_t layer = 0; layer < expected_sectors; ++layer) {
        if (seam_sector_cursor >= a.seam_sectors.size()) return false;
        const auto &sector = a.seam_sectors[seam_sector_cursor];
        if (!canonical_id(sector.id, seam_sector_cursor) ||
            sector.seam != seam.id || sector.lower_layer != layer ||
            sector.upper_layer != (layer + 1) % expected_sectors ||
            seam.sectors[layer] != sector.id)
          return false;
        ++seam_sector_cursor;
      }
      ++seam_cursor;
    } else if (edge.kind == global_edge_kind::source_edge) {
      for (std::size_t layer = 0; layer < order.value().layers.size(); ++layer) {
        if (source_cursor >= a.source_edge_sectors.size()) return false;
        const auto &sector = a.source_edge_sectors[source_cursor];
        std::vector<sheet_use_id> expected;
        for (auto member : order.value().layers[layer].members)
          expected.push_back(uses[member]);
        const auto upper = (layer + 1) % order.value().layers.size();
        for (auto member : order.value().layers[upper].members)
          expected.push_back(uses[member]);
        std::sort(expected.begin(), expected.end());
        expected.erase(std::unique(expected.begin(), expected.end()),
                       expected.end());
        if (!canonical_id(sector.id, source_cursor) || sector.edge != edge.id ||
            sector.lower_layer != layer || sector.upper_layer != upper ||
            sector.incident_uses != expected)
          return false;
        ++source_cursor;
      }
    }
  }
  return seam_cursor == a.seams.size() &&
         seam_sector_cursor == a.seam_sectors.size() &&
         source_cursor == a.source_edge_sectors.size() &&
         seam_sector_cursor == a.certificate.seam_sectors;
}

template<class T, class I>
bool check_coincidence(const arrangement_complex<T, I> &a) {
  std::size_t cursor = 0;
  std::uint64_t memberships = 0;
  for (const auto &patch : a.patches)
    if (patch.uses.size() > 1) {
      if (cursor >= a.coincident_groups.size()) return false;
      const auto &group = a.coincident_groups[cursor];
      if (!canonical_id(group.id, cursor) || group.patch != patch.id ||
          group.members != patch.uses)
        return false;
      memberships += group.members.size();
      ++cursor;
    }
  return cursor == a.coincident_groups.size() &&
         memberships == a.certificate.coincident_memberships;
}

template<class T, class I>
bool check_side_graph(const arrangement_complex<T, I> &a,
                      const verifier_indices &index) {
  if (a.patch_sides.size() != 2 * a.patches.size()) return false;
  for (std::size_t i = 0; i < a.patch_sides.size(); ++i) {
    const auto &side = a.patch_sides[i];
    if (!canonical_id(side.id, i) || side.patch.value_for_debug() != i / 2 ||
        side.side != static_cast<patch_plane_side>(i % 2) ||
        side.component.value_for_debug() != i)
      return false;
  }
  std::vector<std::optional<coincident_group_id>> groups(a.patches.size());
  for (const auto &group : a.coincident_groups) {
    if (!valid_id(group.patch, groups.size()) ||
        groups[group.patch.value_for_debug()])
      return false;
    groups[group.patch.value_for_debug()] = group.id;
  }
  std::size_t cursor = 0;
  auto preserving = [&](side_transition_kind kind, sheet_use_id use) {
    if (cursor >= a.transitions.size() || !valid_id(use, a.sheet_uses.size()))
      return false;
    const auto &actual = a.transitions[cursor];
    const auto side = patch_side_id::from_canonical_value(
        2 * a.sheet_uses[use.value_for_debug()].patch.value_for_debug());
    return canonical_id(actual.id, cursor++) && actual.kind == kind &&
           actual.from == side && actual.to == side && !actual.region_crossing &&
           actual.uses == std::vector<sheet_use_id>{use} && !actual.coincidence;
  };
  for (const auto &patch : a.patches) {
    if (cursor >= a.transitions.size()) return false;
    const auto &actual = a.transitions[cursor];
    const auto group = groups[patch.id.value_for_debug()];
    const auto kind = group ? side_transition_kind::coincidence_crossing
                            : side_transition_kind::sheet_crossing;
    if (!canonical_id(actual.id, cursor++) || actual.kind != kind ||
        actual.from.value_for_debug() != 2 * patch.id.value_for_debug() ||
        actual.to.value_for_debug() != 2 * patch.id.value_for_debug() + 1 ||
        !actual.region_crossing || actual.uses != patch.uses ||
        actual.coincidence != group)
      return false;
  }
  for (const auto &sector : a.seam_sectors) {
    if (!valid_id(sector.seam, a.seams.size())) return false;
    const auto &uses = a.seams[sector.seam.value_for_debug()].incident_uses;
    if (!uses.empty() &&
        !preserving(side_transition_kind::seam_sector, uses.front()))
      return false;
  }
  for (const auto &sector : a.source_edge_sectors)
    if (!sector.incident_uses.empty() &&
        !preserving(side_transition_kind::source_edge_sector,
                    sector.incident_uses.front()))
      return false;
  for (const auto &sector : a.vertex_sectors) {
    if (!valid_id(sector.vertex, a.vertices.size())) return false;
    const auto &locals = a.vertices[sector.vertex.value_for_debug()].local_occurrences;
    if (!locals.empty()) {
      if (!valid_id(locals.front().facet, index.uses_by_facet.size())) return false;
      const auto &uses = index.uses_by_facet[locals.front().facet.value_for_debug()];
      if (!uses.empty() &&
          !preserving(side_transition_kind::vertex_sector, uses.front()))
        return false;
    }
  }
  return cursor == a.transitions.size() &&
         cursor == a.certificate.side_transitions;
}

void encode_plane(canonical_encoder &e, const exact_plane3 &p) {
  encode(e, p.a); encode(e, p.b); encode(e, p.c); encode(e, p.d);
  e.byte(static_cast<std::uint8_t>(p.oriented));
}
void encode_vector(canonical_encoder &e, const exact_vector3 &v) {
  encode(e, v.x); encode(e, v.y); encode(e, v.z);
}
void encode_point(canonical_encoder &e, const exact_point3 &p) {
  encode(e, p.x); encode(e, p.y); encode(e, p.z);
}
template<class Id>
void encode_ids(canonical_encoder &e, const std::vector<Id> &ids) {
  e.u64(ids.size()); for (auto id : ids) e.id(id);
}

template<class T, class I>
std::vector<std::uint8_t> encode_semantic(const arrangement_complex<T, I> &a,
                                          bool quotient) {
  canonical_encoder e;
  const char *tag = quotient ? "YGBCAN8Q" : "YGBCAN08";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(arrangement_complex_schema);
  e.byte(static_cast<std::uint8_t>(a.classification));
  e.u64(a.vertices.size()); for (const auto &v : a.vertices) { e.id(v.id); e.id(v.symbolic); encode_ids(e,v.incident_edges); encode_ids(e,v.occurrences); if(!quotient){e.u64(v.local_occurrences.size());for(auto r:v.local_occurrences){e.id(r.facet);e.id(r.id);}} }
  e.u64(a.vertex_occurrences.size()); for(const auto&o:a.vertex_occurrences){e.id(o.id);e.id(o.vertex);e.id(o.operand);e.id(o.shell);encode_ids(e,o.incident_halfedges);encode_ids(e,o.link_regions);if(!quotient){e.u64(o.local_germs.size());for(auto r:o.local_germs){e.id(r.facet);e.id(r.id);}}}
  e.u64(a.edges.size()); for(const auto&x:a.edges){e.id(x.id);e.id(x.lower);e.id(x.upper);e.byte(static_cast<std::uint8_t>(x.kind));encode_ids(e,x.curves);if(!quotient){e.u64(x.local_occurrences.size());for(auto r:x.local_occurrences){e.id(r.facet);e.id(r.id);}}}
  e.u64(a.sheet_members.size());for(const auto&m:a.sheet_members){e.id(m.id);e.id(m.operand);e.id(m.shell);encode_ids(e,m.facets);if(!quotient){e.u64(m.local_patches.size());for(auto r:m.local_patches){e.id(r.facet);e.id(r.id);}}}
  e.u64(a.patches.size());for(const auto&p:a.patches){e.id(p.id);encode_plane(e,p.plane);encode(e,p.projected_double_area);encode_ids(e,p.outer);e.u64(p.holes.size());for(const auto&h:p.holes)encode_ids(e,h);encode_ids(e,p.uses);}
  e.u64(a.sheet_uses.size());for(const auto&u:a.sheet_uses){e.id(u.id);e.id(u.member);e.id(u.patch);e.id(u.operand);e.id(u.shell);e.id(u.facet);e.boolean(u.source_plane_agrees);e.byte(static_cast<std::uint8_t>(u.occupied_side));encode_ids(e,u.boundary);if(!quotient){e.id(u.local_patch.facet);e.id(u.local_patch.id);}}
  e.u64(a.halfedges.size());for(const auto&h:a.halfedges){e.id(h.id);e.id(h.next);e.id(h.previous);e.id(h.sheet_mate);e.id(h.use);e.id(h.edge);e.id(h.origin);e.id(h.destination);e.id(h.origin_occurrence);e.id(h.destination_occurrence);if(!quotient){e.id(h.local.facet);e.id(h.local.id);}}
  e.u64(a.seams.size());for(const auto&s:a.seams){e.id(s.id);e.id(s.edge);encode_ids(e,s.incident_uses);e.u64(s.radial_layers.size());for(const auto&l:s.radial_layers)encode_ids(e,l.uses);encode_ids(e,s.sectors);}
  e.u64(a.seam_sectors.size());for(const auto&s:a.seam_sectors){e.id(s.id);e.id(s.seam);e.u64(s.lower_layer);e.u64(s.upper_layer);encode_ids(e,s.incident_sides);}
  e.u64(a.source_edge_sectors.size());for(const auto&s:a.source_edge_sectors){e.id(s.id);e.id(s.edge);e.u64(s.lower_layer);e.u64(s.upper_layer);encode_ids(e,s.incident_uses);}
  e.u64(a.link_rays.size());for(const auto&r:a.link_rays){e.id(r.id);encode_vector(e,r.direction);e.id(r.antipode);}
  e.u64(a.link_arcs.size());for(const auto&r:a.link_arcs){e.id(r.id);e.id(r.occurrence);e.id(r.origin);e.id(r.destination);encode_ids(e,r.layers);}
  e.u64(a.vertex_sectors.size());for(const auto&s:a.vertex_sectors){e.id(s.id);e.id(s.vertex);e.id(s.occurrence);e.id(s.region);e.byte(static_cast<std::uint8_t>(s.germ));encode_ids(e,s.boundary_rays);encode_ids(e,s.boundary_arcs);encode_vector(e,s.witness_direction);e.u64(s.witness_evidence.size());for(auto x:s.witness_evidence)e.byte(static_cast<std::uint8_t>(x));encode_ids(e,s.seam_continuations);encode_ids(e,s.source_edge_continuations);}
  e.u64(a.coincident_groups.size());for(const auto&g:a.coincident_groups){e.id(g.id);e.id(g.patch);encode_ids(e,g.members);}
  e.u64(a.patch_sides.size());for(const auto&s:a.patch_sides){e.id(s.id);e.id(s.patch);e.byte(static_cast<std::uint8_t>(s.side));e.id(s.component);}
  e.u64(a.transitions.size());for(const auto&t:a.transitions){e.id(t.id);e.byte(static_cast<std::uint8_t>(t.kind));e.id(t.from);e.id(t.to);e.boolean(t.region_crossing);encode_ids(e,t.uses);e.boolean(bool(t.coincidence));if(t.coincidence)e.id(*t.coincidence);}
  e.u64(a.probes.size());for(const auto&p:a.probes){e.id(p.side);e.id(p.component);e.byte(static_cast<std::uint8_t>(p.base_kind));e.u64(p.base_id);e.boolean(bool(p.base_vertex));if(p.base_vertex)e.id(*p.base_vertex);e.boolean(bool(p.exact_base));if(p.exact_base)encode_point(e,*p.exact_base);encode_vector(e,p.direction);e.u64(p.constraints.size());for(const auto&c:p.constraints){encode_plane(e,c.plane);e.byte(static_cast<std::uint8_t>(c.required));}e.u64(p.evidence.size());for(auto x:p.evidence)e.byte(static_cast<std::uint8_t>(x));e.u16(p.formula_version);}
  if(!quotient){e.u64(a.local_maps.size());for(const auto&m:a.local_maps){e.byte(static_cast<std::uint8_t>(m.kind));e.id(m.facet);e.u64(m.local_id);e.u64(m.global_fragments.size());for(auto x:m.global_fragments)e.u64(x);e.boolean(m.retained_incidence_only);}}
  return e.bytes();
}

template<class T, class I>
std::vector<std::uint8_t> encode_invocation(const arrangement_complex<T, I> &a) {
  canonical_encoder e; const char tag[] = "YGBARR08";
  e.raw(reinterpret_cast<const std::uint8_t *>(tag), 8);
  e.u16(arrangement_complex_schema);
  e.raw(a.setup_digest.bytes.data(),16); e.raw(a.refined_digest.bytes.data(),16);
  e.raw(a.symbolic_digest.bytes.data(),16); e.raw(a.validated_digest.bytes.data(),16);
  e.raw(a.kernel_policy_digest.bytes.data(),16); e.byte_string(a.canonical_bytes);
  return e.bytes();
}

template<class T, class I>
bool check_canonical_encoding(const arrangement_complex<T, I> &a,
                              const artifact_view &view) {
  if (encode_semantic(a, false) != a.canonical_bytes ||
      encode_semantic(a, true) != a.quotient_bytes ||
      encode_invocation(a) != a.artifact_bytes)
    return false;
  canonical_encoder e;
  e.raw(a.setup_digest.bytes.data(), 16);
  e.byte(static_cast<std::uint8_t>(artifact_slot::arrangement_complex));
  e.byte_string(a.artifact_bytes);
  const auto artifact = domain_digest({{'Y','G','B','A','R','T','0','1'}}, e.bytes());
  return artifact == a.artifact_digest && artifact == view.artifact_digest &&
         a.certificate.semantic_digest ==
             domain_digest({{'Y','G','B','C','A','N','0','8'}}, a.canonical_bytes);
}

template<class T,class I>
bool check_occurrences(const arrangement_complex<T,I>&a){
  using key=std::tuple<global_vertex_id,operand_id,shell_id>;
  std::map<key,std::vector<local_vertex_ref>> expected;
  std::map<symbolic_vertex_id,global_vertex_id> geometric;
  std::vector<std::vector<local_vertex_ref>> locals_by_vertex(a.vertices.size());
  std::vector<std::vector<vertex_occurrence_id>> occurrences_by_vertex(a.vertices.size());
  std::vector<std::vector<global_halfedge_id>> halfedges_by_occurrence(a.vertex_occurrences.size());
  std::vector<std::vector<global_atomic_edge_id>> edges_by_vertex(a.vertices.size());
  for(const auto&v:a.vertices)if(!geometric.emplace(v.symbolic,v.id).second)return false;
  for(const auto&f:a.refined->payload->facets)for(const auto&v:f.vertices){auto g=geometric.find(v.symbolic);if(g==geometric.end()||!valid_id(g->second,a.vertices.size()))return false;expected[{g->second,f.operand,f.shell}].push_back(v.id);locals_by_vertex[g->second.value_for_debug()].push_back(v.id);}
  if(expected.size()!=a.vertex_occurrences.size())return false;
  std::size_t i=0;
  for(auto&entry:expected){auto germs=entry.second;std::sort(germs.begin(),germs.end(),[](const auto&x,const auto&y){return std::tie(x.facet,x.id)<std::tie(y.facet,y.id);});const auto&o=a.vertex_occurrences[i];auto actual=o.local_germs;std::sort(actual.begin(),actual.end(),[](const auto&x,const auto&y){return std::tie(x.facet,x.id)<std::tie(y.facet,y.id);});if(o.id.value_for_debug()!=i||std::tie(o.vertex,o.operand,o.shell)!=entry.first||actual!=germs||o.local_germs.empty()||!valid_id(o.vertex,a.vertices.size()))return false;occurrences_by_vertex[o.vertex.value_for_debug()].push_back(o.id);++i;}
  for(const auto&h:a.halfedges){if(!valid_id(h.origin_occurrence,a.vertex_occurrences.size())||!valid_id(h.destination_occurrence,a.vertex_occurrences.size())||!valid_id(h.sheet_mate,a.halfedges.size())||!valid_id(h.use,a.sheet_uses.size()))return false;const auto&u=a.sheet_uses[h.use.value_for_debug()];const auto&x=a.vertex_occurrences[h.origin_occurrence.value_for_debug()];const auto&y=a.vertex_occurrences[h.destination_occurrence.value_for_debug()];const auto&mate=a.halfedges[h.sheet_mate.value_for_debug()];if(x.vertex!=h.origin||y.vertex!=h.destination||x.operand!=u.operand||y.operand!=u.operand||x.shell!=u.shell||y.shell!=u.shell||mate.sheet_mate!=h.id||mate.origin_occurrence!=h.destination_occurrence||mate.destination_occurrence!=h.origin_occurrence)return false;halfedges_by_occurrence[h.origin_occurrence.value_for_debug()].push_back(h.id);}
  for(const auto&edge:a.edges){if(!valid_id(edge.lower,a.vertices.size())||!valid_id(edge.upper,a.vertices.size()))return false;edges_by_vertex[edge.lower.value_for_debug()].push_back(edge.id);edges_by_vertex[edge.upper.value_for_debug()].push_back(edge.id);}
  for(std::size_t vertex=0;vertex<a.vertices.size();++vertex){if(a.vertices[vertex].local_occurrences!=locals_by_vertex[vertex]||a.vertices[vertex].occurrences!=occurrences_by_vertex[vertex]||a.vertices[vertex].incident_edges!=edges_by_vertex[vertex])return false;}
  for(std::size_t occurrence=0;occurrence<a.vertex_occurrences.size();++occurrence)if(a.vertex_occurrences[occurrence].incident_halfedges!=halfedges_by_occurrence[occurrence])return false;
  return true;
}

template<class T,class I>
bool check_vertex_links(const arrangement_complex<T,I>&a,
                        verification_level level){
  if(a.vertex_sectors.size()!=a.vertex_occurrences.size())return false;
  for(std::size_t i=0;i<a.link_rays.size();++i){const auto&r=a.link_rays[i];if(!canonical_id(r.id,i)||!valid_id(r.antipode,a.link_rays.size()))return false;const auto&q=a.link_rays[r.antipode.value_for_debug()];if(q.antipode!=r.id||q.direction.x!=r.direction.x.negated()||q.direction.y!=r.direction.y.negated()||q.direction.z!=r.direction.z.negated())return false;}
  using seam_incidence=std::pair<global_atomic_edge_id,seam_sector_id>;using source_incidence=std::pair<global_atomic_edge_id,source_edge_sector_id>;
  std::vector<seam_incidence> seam_by_edge;std::vector<source_incidence> source_by_edge;
  for(const auto&x:a.seams){if(!valid_id(x.edge,a.edges.size()))return false;for(auto sector:x.sectors){if(!valid_id(sector,a.seam_sectors.size()))return false;seam_by_edge.push_back({x.edge,sector});}}
  for(const auto&x:a.source_edge_sectors){if(!valid_id(x.edge,a.edges.size()))return false;source_by_edge.push_back({x.edge,x.id});}
  std::sort(seam_by_edge.begin(),seam_by_edge.end());std::sort(source_by_edge.begin(),source_by_edge.end());performance_count(performance_counter::global_incidence_records,seam_by_edge.size()+source_by_edge.size());
  constexpr std::size_t exhaustive_oracle_limit=32;
  std::size_t ray_cursor=0,arc_cursor=0;
  for(std::size_t ordinal=0;ordinal<a.vertex_sectors.size();++ordinal){const auto&s=a.vertex_sectors[ordinal];if(!canonical_id(s.id,ordinal)||!valid_id(s.occurrence,a.vertex_occurrences.size())||!valid_id(s.vertex,a.vertices.size())||a.vertex_occurrences[s.occurrence.value_for_debug()].vertex!=s.vertex||s.region.value_for_debug()!=ordinal)return false;
    std::vector<global_atomic_edge_id>incident,incident_scratch;for(auto h:a.vertex_occurrences[s.occurrence.value_for_debug()].incident_halfedges){if(!valid_id(h,a.halfedges.size()))return false;const auto edge=a.halfedges[h.value_for_debug()].edge;if(!valid_id(edge,a.edges.size()))return false;incident.push_back(edge);}sort_incident_edges(incident,incident_scratch);incident.erase(std::unique(incident.begin(),incident.end()),incident.end());
    const auto expected_germ=incident.empty()?vertex_germ_kind::terminal_contact:incident.size()==1?vertex_germ_kind::semicircle:vertex_germ_kind::wedge;if(s.germ!=expected_germ)return false;
    std::vector<seam_sector_id>seams;std::vector<source_edge_sector_id>sources;for(auto edge:incident){performance_count(performance_counter::global_index_lookups,2);auto sr=std::equal_range(seam_by_edge.begin(),seam_by_edge.end(),seam_incidence{edge,{}},[](const auto&x,const auto&y){return x.first<y.first;});for(auto it=sr.first;it!=sr.second;++it)seams.push_back(it->second);auto rr=std::equal_range(source_by_edge.begin(),source_by_edge.end(),source_incidence{edge,{}},[](const auto&x,const auto&y){return x.first<y.first;});for(auto it=rr.first;it!=rr.second;++it)sources.push_back(it->second);}
    const auto symbolic=a.vertices[s.vertex.value_for_debug()].symbolic;if(!valid_id(symbolic,a.symbolic->payload->vertices.size()))return false;const auto&p=a.symbolic->payload->vertices[symbolic.value_for_debug()].point;std::vector<exact_vector3>candidate_directions;std::vector<verifier_direction_entry>directions,direction_scratch;candidate_directions.reserve(incident.size());directions.reserve(incident.size());performance_count(performance_counter::link_direction_candidates,incident.size());
    for(auto edge:incident){const auto&e=a.edges[edge.value_for_debug()];if(e.lower!=s.vertex&&e.upper!=s.vertex)return false;const auto other=e.lower==s.vertex?e.upper:e.lower;if(!valid_id(other,a.vertices.size()))return false;const auto other_symbolic=a.vertices[other.value_for_debug()].symbolic;if(!valid_id(other_symbolic,a.symbolic->payload->vertices.size()))return false;const auto d=a.symbolic->payload->vertices[other_symbolic.value_for_debug()].point-p;std::array<exact_scalar,3>c{{d.x,d.y,d.z}};std::size_t axis=0;while(axis<c.size()&&c[axis].is_zero())++axis;if(axis==c.size())return false;verifier_direction_key key;key.axis=static_cast<std::uint8_t>(axis);key.negative=c[axis].sign()==exact_sign::negative;for(std::size_t n=0;n<c.size();++n)key.ratio[n]=c[n]/c[axis];directions.push_back({std::move(key),candidate_directions.size()});candidate_directions.push_back(d);}
    sort_direction_entries(directions,direction_scratch);std::vector<bool>representative(candidate_directions.size(),false);for(std::size_t first=0;first<directions.size();){std::size_t last=first+1;while(last<directions.size()&&direction_key_equal(directions[first].key,directions[last].key)){if(!same_positive_direction(candidate_directions[directions[first].ordinal],candidate_directions[directions[last].ordinal]))return false;++last;}std::size_t representative_ordinal=directions[first].ordinal;for(std::size_t i=first+1;i<last;++i)representative_ordinal=std::min(representative_ordinal,directions[i].ordinal);representative[representative_ordinal]=true;first=last;}std::vector<exact_vector3>expected_directions;expected_directions.reserve(candidate_directions.size());for(std::size_t i=0;i<candidate_directions.size();++i)if(representative[i])expected_directions.push_back(candidate_directions[i]);
    if(level==verification_level::exhaustive&&a.vertex_occurrences[s.occurrence.value_for_debug()].incident_halfedges.size()<=exhaustive_oracle_limit&&candidate_directions.size()<=exhaustive_oracle_limit){std::vector<exact_vector3>oracle;for(const auto&d:candidate_directions){bool duplicate=false;for(const auto&prior:oracle)if(same_positive_direction(d,prior)){duplicate=true;break;}if(!duplicate)oracle.push_back(d);}if(oracle.size()!=expected_directions.size())return false;for(std::size_t i=0;i<oracle.size();++i)if(!equal_vector(oracle[i],expected_directions[i]))return false;}
    auto actual_seams=s.seam_continuations;auto actual_sources=s.source_edge_continuations;std::sort(seams.begin(),seams.end());std::sort(sources.begin(),sources.end());std::sort(actual_seams.begin(),actual_seams.end());std::sort(actual_sources.begin(),actual_sources.end());if(seams!=actual_seams||sources!=actual_sources||s.boundary_rays.size()!=expected_directions.size()*2||s.boundary_arcs.size()!=s.boundary_rays.size()||a.vertex_occurrences[s.occurrence.value_for_debug()].link_regions!=std::vector<vertex_sector_id>{s.id})return false;for(std::size_t i=0;i<expected_directions.size();++i){const auto antipode=expected_directions[i]*exact_scalar(-1);if(ray_cursor+1>=a.link_rays.size()||s.boundary_rays[2*i].value_for_debug()!=ray_cursor||s.boundary_rays[2*i+1].value_for_debug()!=ray_cursor+1||!equal_vector(a.link_rays[ray_cursor].direction,expected_directions[i])||!equal_vector(a.link_rays[ray_cursor+1].direction,antipode))return false;ray_cursor+=2;}for(std::size_t i=0;i<s.boundary_rays.size();++i){if(!valid_id(s.boundary_arcs[i],a.link_arcs.size())||s.boundary_arcs[i].value_for_debug()!=arc_cursor)return false;const auto&arc=a.link_arcs[arc_cursor++];if(!canonical_id(arc.id,arc_cursor-1)||arc.occurrence!=s.occurrence||arc.origin!=s.boundary_rays[i]||arc.destination!=s.boundary_rays[(i+1)%s.boundary_rays.size()]||!arc.layers.empty())return false;}
    const auto&occurrence=a.vertex_occurrences[s.occurrence.value_for_debug()];if(occurrence.local_germs.empty()||!valid_id(occurrence.local_germs.front().facet,a.validated->payload->facets.size()))return false;const auto&plane=a.validated->payload->facets[occurrence.local_germs.front().facet.value_for_debug()].plane;exact_vector3 normal{exact_scalar(plane.a,big_uint(1)),exact_scalar(plane.b,big_uint(1)),exact_scalar(plane.c,big_uint(1))};if(plane.oriented==orientation_parity::opposite)normal=normal*exact_scalar(-1);const auto witness=construct_strict_cone_witness({{normal,exact_sign::positive}});if(!witness.has_value()||!equal_vector(s.witness_direction,witness.value().direction)||s.witness_evidence!=witness.value().evaluations)return false;
  }
  return ray_cursor==a.link_rays.size()&&arc_cursor==a.link_arcs.size();
}

template<class T,class I>
bool check_open_probes(const arrangement_complex<T,I>&a,
                       const verification_environment_view&env){
  if(a.classification!=classification_strategy::independent_patch_side_v1||a.probes.size()!=(a.patch_sides.empty()?1:a.patch_sides.size()))return false;
  if(a.patch_sides.empty()){const auto&p=a.probes.front();const exact_vector3 direction{exact_scalar(1),exact_scalar(0),exact_scalar(0)};return p.base_kind==probe_base_stratum_kind::universe&&p.side.value_for_debug()==0&&p.component.value_for_debug()==0&&p.exact_base&&*p.exact_base==exact_point3{exact_scalar(0),exact_scalar(0),exact_scalar(0)}&&equal_vector(p.direction,direction)&&p.constraints.empty()&&p.evidence.empty()&&!p.base_vertex;}
  std::vector<bool>seen(a.patch_sides.size());
  for(const auto&p:a.probes){if(p.base_kind!=probe_base_stratum_kind::patch_side||!valid_id(p.side,a.patch_sides.size())||seen[p.side.value_for_debug()]||!p.exact_base||p.base_vertex||p.base_id!=p.side.value_for_debug()||p.constraints.size()!=1||p.evidence.size()!=1||(env.options&&p.formula_version!=env.options->classification.probe_formula_version))return false;seen[p.side.value_for_debug()]=true;const auto&s=a.patch_sides[p.side.value_for_debug()];if(s.component!=p.component||!valid_id(s.patch,a.patches.size()))return false;const auto&patch=a.patches[s.patch.value_for_debug()];const auto&constraint=p.constraints.front().plane;const bool same_plane=constraint.a==patch.plane.a&&constraint.b==patch.plane.b&&constraint.c==patch.plane.c&&constraint.d==patch.plane.d&&constraint.oriented==patch.plane.oriented;if(plane_side(patch.plane,*p.exact_base,predicate_execution_policy::exact_only)!=exact_sign::zero||!same_plane)return false;const auto axis=dominant_projection(patch.plane);std::vector<exact_point2>ring;for(auto v:patch.outer){if(!valid_id(v,a.vertices.size()))return false;const auto symbolic=a.vertices[v.value_for_debug()].symbolic;if(!valid_id(symbolic,a.symbolic->payload->vertices.size()))return false;ring.push_back(project(a.symbolic->payload->vertices[symbolic.value_for_debug()].point,axis));}auto relation=classify_point_polygon(project(*p.exact_base,axis),ring);if(!relation.has_value()||relation.value().kind!=point_region_kind::open_interior)return false;for(const auto&hole:patch.holes){ring.clear();for(auto v:hole){if(!valid_id(v,a.vertices.size()))return false;const auto symbolic=a.vertices[v.value_for_debug()].symbolic;if(!valid_id(symbolic,a.symbolic->payload->vertices.size()))return false;ring.push_back(project(a.symbolic->payload->vertices[symbolic.value_for_debug()].point,axis));}auto hole_relation=classify_point_polygon(project(*p.exact_base,axis),ring);if(!hole_relation.has_value()||hole_relation.value().kind!=point_region_kind::outside)return false;}exact_vector3 n{exact_scalar(patch.plane.a,big_uint(1)),exact_scalar(patch.plane.b,big_uint(1)),exact_scalar(patch.plane.c,big_uint(1))};if(patch.plane.oriented==orientation_parity::opposite)n=n*exact_scalar(-1);const auto expected=s.side==patch_plane_side::positive?exact_sign::positive:exact_sign::negative;if(dot_sign(n,p.direction,predicate_execution_policy::exact_only)!=expected||p.constraints.front().required!=expected||p.evidence.front()!=expected)return false;}
  for(bool value:seen)if(!value)return false;
  return true;
}

status_or<std::uint64_t> add_count(std::uint64_t total, std::uint64_t value) {
  return checked_add(total, value, boolean_stage::global_arrangement);
}

template<class T,class I>
status_or<std::array<std::uint64_t,2>> verifier_resources(
    const arrangement_complex<T,I>&a,const verification_spec&s){
  std::uint64_t local_records=0, memberships=0, ring_peak=0,direction_peak=0,direction_sort_work=0,direction_oracle_work=0;
  const auto add=[&](std::uint64_t&total,std::uint64_t value)->bool{auto next=add_count(total,value);if(!next.has_value())return false;total=next.value();return true;};
  if(a.refined&&a.refined->payload)for(const auto&f:a.refined->payload->facets){if(!add(local_records,f.vertices.size())||!add(local_records,f.vertices.size())||!add(local_records,f.edges.size())||!add(local_records,f.halfedges.size())||!add(local_records,f.patches.size()))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");}
  for(const auto&v:a.vertices)if(!add(memberships,v.local_occurrences.size())||!add(memberships,v.incident_edges.size())||!add(memberships,v.occurrences.size()))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");
  for(const auto&o:a.vertex_occurrences){if(!add(memberships,o.local_germs.size())||!add(memberships,o.incident_halfedges.size())||!add(memberships,o.link_regions.size()))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");direction_peak=std::max<std::uint64_t>(direction_peak,o.incident_halfedges.size());auto sort=checked_multiply(o.incident_halfedges.size(),direction_sort_levels(o.incident_halfedges.size()),boolean_stage::global_arrangement);if(!sort.has_value()||!add(direction_sort_work,sort.value())||!add(direction_sort_work,sort.value()))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");if(s.level==verification_level::exhaustive&&o.incident_halfedges.size()<=32){auto pairs=checked_multiply(o.incident_halfedges.size(),o.incident_halfedges.size()-static_cast<std::size_t>(!o.incident_halfedges.empty()),boolean_stage::global_arrangement);if(!pairs.has_value()||!add(direction_oracle_work,pairs.value()/2))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");}}
  for(const auto&e:a.edges)if(!add(memberships,e.local_occurrences.size())||!add(memberships,e.curves.size()))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");
  for(const auto&p:a.patches){std::uint64_t ring=p.outer.size();for(const auto&h:p.holes)if(!add(ring,h.size()))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");ring_peak=std::max(ring_peak,ring);if(!add(memberships,ring)||!add(memberships,p.uses.size()))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");}
  for(const auto&u:a.sheet_uses)if(!add(memberships,u.boundary.size()))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");
  for(const auto&seam:a.seams){if(!add(memberships,seam.incident_uses.size())||!add(memberships,seam.sectors.size()))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");for(const auto&layer:seam.radial_layers)if(!add(memberships,layer.uses.size()))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");}
  for(const auto&sector:a.vertex_sectors)if(!add(memberships,sector.boundary_rays.size())||!add(memberships,sector.boundary_arcs.size())||!add(memberships,sector.seam_continuations.size())||!add(memberships,sector.source_edge_continuations.size()))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");
  for(const auto&map:a.local_maps)if(!add(memberships,map.global_fragments.size()))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");
  auto map_bytes=checked_multiply(local_records,sizeof(std::pair<const local_key,std::uint64_t>)+4*sizeof(void*),boolean_stage::global_arrangement);if(!map_bytes.has_value())return map_bytes.error();
  auto membership_bytes=checked_multiply(memberships,std::max({sizeof(local_vertex_ref),sizeof(std::array<exact_scalar,3>),sizeof(std::uint64_t)}),boolean_stage::global_arrangement);if(!membership_bytes.has_value())return membership_bytes.error();
  auto vector_bytes=checked_multiply(a.edges.size()+(a.validated&&a.validated->payload?a.validated->payload->facets.size():0),sizeof(std::vector<sheet_use_id>),boolean_stage::global_arrangement);if(!vector_bytes.has_value())return vector_bytes.error();
  auto ring_bytes=checked_multiply(ring_peak,sizeof(exact_point2),boolean_stage::global_arrangement);if(!ring_bytes.has_value())return ring_bytes.error();
  auto direction_bytes=checked_multiply(direction_peak,2*sizeof(verifier_direction_entry)+3*sizeof(exact_vector3)+2*sizeof(global_atomic_edge_id),boolean_stage::global_arrangement);if(!direction_bytes.has_value())return direction_bytes.error();
  const auto encoding_peak=std::max({a.canonical_bytes.size(),a.quotient_bytes.size(),a.artifact_bytes.size()});
  auto scratch=checked_add(map_bytes.value(),membership_bytes.value(),boolean_stage::global_arrangement);if(!scratch.has_value())return scratch.error();scratch=checked_add(scratch.value(),vector_bytes.value(),boolean_stage::global_arrangement);if(!scratch.has_value())return scratch.error();scratch=checked_add(scratch.value(),ring_bytes.value(),boolean_stage::global_arrangement);if(!scratch.has_value())return scratch.error();scratch=checked_add(scratch.value(),direction_bytes.value(),boolean_stage::global_arrangement);if(!scratch.has_value())return scratch.error();scratch=checked_add(scratch.value(),encoding_peak,boolean_stage::global_arrangement);if(!scratch.has_value())return scratch.error();
  std::uint64_t linear=s.required_invariants.size();for(auto n:{a.vertices.size(),a.vertex_occurrences.size(),a.edges.size(),a.sheet_uses.size(),a.halfedges.size(),a.seams.size(),a.seam_sectors.size(),a.source_edge_sectors.size(),a.link_rays.size(),a.link_arcs.size(),a.vertex_sectors.size(),a.coincident_groups.size(),a.patch_sides.size(),a.transitions.size(),a.probes.size(),a.local_maps.size()})if(!add(linear,n))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");if(!add(linear,memberships)||!add(linear,local_records)||!add(linear,direction_sort_work)||!add(linear,direction_oracle_work))return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_resource_overflow");
  return std::array<std::uint64_t,2>{{scratch.value(),linear}};
}

template<class T,class I>
status_or<verification_report>verify(const artifact_view&v,const verification_spec&s,const verification_environment_view&e)noexcept{
  try{
    if(!v.payload)return make_error(boolean_error_code::internal_invariant_error,boolean_stage::global_arrangement,"arrangement_verifier_null");
    const auto&a=*static_cast<const arrangement_complex<T,I>*>(v.payload);
    if(!e.accountant)return make_error(boolean_error_code::internal_invariant_error,boolean_stage::global_arrangement,"missing_verifier_accountant");
    auto resources=verifier_resources(a,s);if(!resources.has_value())return resources.error();
    auto scratch=e.accountant->reserve_scoped(resource_kind::verifier_scratch_bytes,resources.value()[0],boolean_stage::global_arrangement);if(!scratch.has_value())return scratch.error();
    auto work=e.accountant->reserve_scoped(resource_kind::verifier_work,resources.value()[1],boolean_stage::global_arrangement);if(!work.has_value())return work.error();
    verification_report r;r.checker_version=s.checker_version;r.owner=v.owner;r.stage=boolean_stage::global_arrangement;r.slot=v.slot;r.artifact_type_tag=v.artifact_type_tag;r.artifact_schema=v.artifact_schema;r.setup_digest=e.setup_digest;r.artifact_digest=v.artifact_digest;r.invariant_set_digest=s.invariant_set_digest;r.outcome=verification_outcome::pass;
    verifier_indices index;bool failed=false;
    for(auto code:s.required_invariants){if(e.cancelled&&e.cancelled())return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"cancelled");invariant_result result;result.code=code;result.status=failed?check_status::not_run_due_to_prior_failure:check_status::failed;if(!failed){bool ok=false;switch(code){case invariant_code::arrangement_binding:ok=check_binding(a,v,e);break;case invariant_code::arrangement_maps:ok=check_maps(a,index);break;case invariant_code::arrangement_cycles:ok=check_cycles(a);break;case invariant_code::arrangement_seams:ok=check_seams(a,index);break;case invariant_code::arrangement_coincidence:ok=check_coincidence(a);break;case invariant_code::arrangement_side_graph:ok=check_side_graph(a,index);break;case invariant_code::arrangement_canonical_encoding:ok=check_canonical_encoding(a,v);break;case invariant_code::arrangement_occurrences:ok=check_occurrences(a);break;case invariant_code::arrangement_vertex_links:ok=check_vertex_links(a,s.level);break;case invariant_code::arrangement_open_probes:ok=check_open_probes(a,e);break;default:ok=false;break;}result.status=ok?check_status::passed:check_status::failed;if(!ok){failed=true;r.outcome=verification_outcome::invariant_failure;}}r.results.push_back(std::move(result));}
    auto encoded=encode_verification_report(r);if(!encoded.has_value())return encoded.error();r.report_digest=domain_digest({{'Y','G','B','V','E','R','0','1'}},encoded.value());return r;
  }catch(const std::bad_alloc&){return make_error(boolean_error_code::resource_limit,boolean_stage::global_arrangement,"arrangement_verifier_allocation");}catch(...){return make_error(boolean_error_code::internal_invariant_error,boolean_stage::global_arrangement,"arrangement_verifier_exception");}
}

template<class T,class I>status_or<verification_report>callback(const artifact_view&v,const verification_spec&s,const verification_environment_view&e)noexcept{return verify<T,I>(v,s,e);}

} // namespace

status_or<bool>register_global_arrangement_verifier(verifier_registry&r,coordinate_tag c,index_tag i){verifier_registration x;x.slot=artifact_slot::arrangement_complex;x.artifact_type_tag=arrangement_complex_type_tag+(static_cast<std::uint64_t>(c)<<8)+static_cast<std::uint64_t>(i);x.artifact_schema=arrangement_complex_schema;x.mandatory={invariant_code::arrangement_binding,invariant_code::arrangement_maps,invariant_code::arrangement_cycles,invariant_code::arrangement_seams,invariant_code::arrangement_coincidence,invariant_code::arrangement_side_graph,invariant_code::arrangement_canonical_encoding,invariant_code::arrangement_occurrences,invariant_code::arrangement_vertex_links,invariant_code::arrangement_open_probes};x.exhaustive=x.mandatory;if(c==coordinate_tag::binary32&&i==index_tag::uint32)x.callback=&callback<float,std::uint32_t>;else if(c==coordinate_tag::binary32)x.callback=&callback<float,std::uint64_t>;else if(i==index_tag::uint32)x.callback=&callback<double,std::uint32_t>;else x.callback=&callback<double,std::uint64_t>;return r.register_verifier(std::move(x));}

} }
