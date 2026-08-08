#pragma once

#include "CanonicalSourceTopologyVerifier.h"
#include "InputGeometryVerifier.h"
#include "Sha256.h"
#include "ValidatedOperand.h"
#include "ValidatedOperandCodec.h"

#include <algorithm>
#include <map>
#include <set>

namespace ygor::mesh_boolean::bounded {

template <class T>
validation_point3<T> verifier_point(const validated_vertex_record &v) {
  return {from_bits<T>(static_cast<scalar_bits_t<T>>(v.coordinate_bits[0])),
          from_bits<T>(static_cast<scalar_bits_t<T>>(v.coordinate_bits[1])),
          from_bits<T>(static_cast<scalar_bits_t<T>>(v.coordinate_bits[2]))};
}

template <class T, class I>
bool verify_validated_operand(const validated_operand<T, I> &a,
                              const immutable_source_mesh<T, I> &source,
                              const precision_context<T> &precision) {
  if (a.schema_version_ != contract_versions::validated_operand ||
      a.operand_ != source.operand() ||
      !a.owner_.same_owner(precision.owner()) ||
       a.source_digest_ != source.digest() ||
       a.precision_digest_ != precision.digest() ||
       a.context_digest_ != precision.boolean_context_digest() ||
       a.certificate_ != input_certificate_disposition::nominal_embedded ||
       sha256::digest(a.canonical_bytes_) != a.digest_ ||
       sha256::digest(a.source_presentation_bytes_) !=
           a.source_presentation_digest_)
    return false;
  if (a.canonical_bytes_ != encode_validated_operand_semantic(a))
    return false;
  if (a.source_presentation_bytes_ !=
      encode_validated_operand_source_presentation(a))
    return false;
  if (!independently_verify_canonical_minimum(a, source))
    return false;
  std::uint64_t persistent_bytes = 0;
  auto add = [&](std::uint64_t value) {
    return checked_add(persistent_bytes, value, persistent_bytes);
  };
  auto add_records = [&](std::uint64_t count, std::uint64_t size) {
    std::uint64_t value = 0;
    return checked_multiply(count, size, value) && add(value);
  };
  if (!add(a.canonical_bytes_.size()) ||
      !add(a.source_presentation_bytes_.size()) ||
      !add_records(a.vertices_.size(), sizeof(validated_vertex_record)) ||
      !add_records(a.bounded_vertices_.size(),
                   sizeof(bounded_source_vertex_evidence<T>)) ||
      !add_records(a.facets_.size(), sizeof(validated_facet_record)) ||
      !add_records(a.directed_uses_.size(),
                   sizeof(directed_source_use_record)) ||
      !add_records(a.edges_.size(), sizeof(undirected_source_edge_record)) ||
      !add_records(a.vertex_links_.size(),
                   sizeof(validated_vertex_link_record)) ||
      !add_records(a.shells_.size(), sizeof(validated_shell_record)) ||
      !add_records(a.relations_.size(), sizeof(geometry_relation_record)) ||
      !add_records(a.shell_pairs_.size(), sizeof(shell_pair_record)) ||
      !add_records(a.edge_wedges_.size(), sizeof(edge_wedge_record)) ||
      !add_records(a.vertex_stars_.size(), sizeof(vertex_star_record)) ||
      !add_records(a.normalization_.size(), sizeof(normalized_position_record)))
    return false;
  for (const auto &facet : a.facets_)
    if (!add_records(facet.vertices.size(), sizeof(std::uint64_t)) ||
        !add_records(facet.decomposition.size(),
                     sizeof(std::array<std::uint64_t, 3>)))
      return false;
  for (const auto &link : a.vertex_links_)
    if (!add_records(link.cyclic_facets.size(), sizeof(std::uint64_t)))
      return false;
  for (const auto &shell : a.shells_)
    if (!add_records(shell.facets.size(), sizeof(std::uint64_t)))
      return false;
  if (persistent_bytes != a.statistics_.persistent_bytes)
    return false;
  std::set<std::uint64_t> ids;
  std::vector<validation_point3<T>> points(a.vertices_.size());
  for (std::uint64_t ordinal = 0; ordinal < a.vertices_.size(); ++ordinal) {
    const auto &v = a.vertices_[ordinal];
    if (v.canonical_id != ordinal ||
        !ids.insert(v.canonical_id).second ||
        v.presentation_vertex >= source.vertex_count())
      return false;
    for (std::size_t k = 0; k < 3; ++k)
      if (v.coordinate_bits[k] !=
          source.coordinate_bits()[v.presentation_vertex * 3 + k])
        return false;
    points[v.canonical_id] = verifier_point<T>(v);
  }
  std::vector<std::uint64_t> source_to_canonical(
      source.vertex_count(), std::numeric_limits<std::uint64_t>::max());
  for (const auto &v : a.vertices_)
    if (source_to_canonical[v.presentation_vertex] !=
            std::numeric_limits<std::uint64_t>::max())
      return false;
    else
      source_to_canonical[v.presentation_vertex] = v.canonical_id;

  if (a.facets_.size() != source.face_count())
    return false;
  std::vector<std::uint64_t> source_to_canonical_facet(
      source.face_count(), std::numeric_limits<std::uint64_t>::max());
  for (std::uint64_t f = 0; f < a.facets_.size(); ++f) {
    const auto &facet = a.facets_[f];
    if (facet.canonical_id != f || facet.presentation_facet >= source.face_count() ||
        source_to_canonical_facet[facet.presentation_facet] !=
            std::numeric_limits<std::uint64_t>::max())
      return false;
    source_to_canonical_facet[facet.presentation_facet] = f;
    std::vector<std::uint64_t> source_ring;
    for (auto p = source.face_offsets()[facet.presentation_facet];
         p < source.face_offsets()[facet.presentation_facet + 1]; ++p) {
      auto vertex = static_cast<std::uint64_t>(source.indices()[p]);
      if (source_ring.empty() || source_ring.back() != vertex)
        source_ring.push_back(vertex);
    }
    if (source_ring.size() > 1 && source_ring.front() == source_ring.back())
      source_ring.pop_back();
    if (source_ring.size() != facet.vertices.size())
      return false;
    std::vector<std::uint64_t> mapped;
    for (auto source_vertex : source_ring) {
      if (source_vertex >= source_to_canonical.size() ||
          source_to_canonical[source_vertex] ==
              std::numeric_limits<std::uint64_t>::max())
        return false;
      mapped.push_back(source_to_canonical[source_vertex]);
    }
    bool rotation = false;
    for (std::size_t shift = 0; shift < mapped.size(); ++shift) {
      bool equal = true;
      for (std::size_t i = 0; i < mapped.size(); ++i)
        equal &= mapped[(shift + i) % mapped.size()] == facet.vertices[i];
      rotation |= equal;
    }
    if (!rotation)
      return false;
  }

  if (a.normalization_.size() != source.indices().size())
    return false;
  std::vector<const normalized_position_record *> positions(
      source.indices().size(), nullptr);
  for (const auto &position : a.normalization_) {
    if (position.source_position >= positions.size() ||
        positions[position.source_position] ||
        position.source_facet >= source.face_count() ||
        position.canonical_facet >= a.facets_.size() ||
        source_to_canonical_facet[position.source_facet] !=
            position.canonical_facet ||
        position.retained_corner >=
            a.facets_[position.canonical_facet].vertices.size())
      return false;
    positions[position.source_position] = &position;
  }
  for (std::uint64_t f = 0; f < source.face_count(); ++f) {
    const auto begin = source.face_offsets()[f];
    const auto end = source.face_offsets()[f + 1];
    const auto count = static_cast<std::size_t>(end - begin);
    std::vector<ring_position_action> expected_actions(
        count, ring_position_action::retained);
    std::vector<std::uint64_t> normalized;
    std::size_t last_retained = std::numeric_limits<std::size_t>::max();
    for (auto p = begin; p < end; ++p) {
      const auto local = static_cast<std::size_t>(p - begin);
      const auto source_vertex =
          static_cast<std::uint64_t>(source.indices()[p]);
      if (!normalized.empty() && normalized.back() == source_vertex) {
        expected_actions[local] = ring_position_action::consecutive_duplicate;
        continue;
      }
      normalized.push_back(source_vertex);
      last_retained = local;
    }
    if (normalized.size() > 1 && normalized.front() == normalized.back()) {
      normalized.pop_back();
      if (last_retained == std::numeric_limits<std::size_t>::max())
        return false;
      expected_actions[last_retained] = ring_position_action::duplicate_closure;
    }
    const auto canonical_facet = source_to_canonical_facet[f];
    if (canonical_facet >= a.facets_.size() ||
        normalized.size() != a.facets_[canonical_facet].vertices.size())
      return false;
    for (std::size_t local = 0; local < count; ++local) {
      const auto p = begin + local;
      const auto *position = positions[p];
      if (!position)
        return false;
      const auto source_vertex =
          static_cast<std::uint64_t>(source.indices()[p]);
      if (source_vertex >= source_to_canonical.size())
        return false;
      const auto canonical_vertex = source_to_canonical[source_vertex];
      if (canonical_vertex == std::numeric_limits<std::uint64_t>::max())
        return false;
      const auto &ring = a.facets_[canonical_facet].vertices;
      const auto corner =
          std::find(ring.begin(), ring.end(), canonical_vertex);
      if (corner == ring.end() || position->source_facet != f ||
          position->canonical_facet != canonical_facet ||
          position->action != expected_actions[local] ||
          position->retained_corner !=
              static_cast<std::uint64_t>(corner - ring.begin()))
        return false;
    }
  }
  auto imported = import_source_bounded_values(precision, source);
  if (!imported.has_value() || a.bounded_vertices_.size() != a.vertices_.size())
    return false;
  for (std::uint64_t ordinal = 0; ordinal < a.bounded_vertices_.size(); ++ordinal) {
    const auto &e = a.bounded_vertices_[ordinal];
    if (e.vertex != ordinal || e.vertex >= a.vertices_.size())
      return false;
    const auto &point = imported.value()
                            ->points[a.vertices_[e.vertex].presentation_vertex]
                            .value;
    if (e.radial_error != point.coordinates.radial_error_upper)
      return false;
    for (std::size_t axis = 0; axis < 3; ++axis)
      if (e.lower[axis] != point.coordinates.components[axis]
                               .uncertainty_enclosure.lower() ||
          e.upper[axis] !=
              point.coordinates.components[axis].uncertainty_enclosure.upper())
        return false;
  }
  std::map<std::pair<std::uint64_t, std::uint64_t>, std::vector<std::uint64_t>>
      edges;
  for (std::uint64_t u = 0; u < a.directed_uses_.size(); ++u) {
    const auto &e = a.directed_uses_[u];
    if (e.origin >= a.vertices_.size() || e.destination >= a.vertices_.size() ||
        e.origin == e.destination || e.facet >= a.facets_.size() ||
        e.reciprocal >= a.directed_uses_.size() ||
        e.undirected_edge >= a.edges_.size())
      return false;
    const auto &r = a.directed_uses_[e.reciprocal];
    if (r.reciprocal != u || r.origin != e.destination ||
        r.destination != e.origin)
      return false;
    if (e.corner >= a.facets_[e.facet].vertices.size() ||
        a.facets_[e.facet].vertices[e.corner] != e.origin ||
        a.facets_[e.facet].vertices[(e.corner + 1) %
                                    a.facets_[e.facet].vertices.size()] !=
            e.destination)
      return false;
    edges[{std::min(e.origin, e.destination),
           std::max(e.origin, e.destination)}]
        .push_back(u);
  }
  if (edges.size() != a.edges_.size())
    return false;
  for (const auto &e : edges)
    if (e.second.size() != 2)
      return false;
  for (std::uint64_t ordinal = 0; ordinal < a.edges_.size(); ++ordinal) {
    const auto &e = a.edges_[ordinal];
    if (e.canonical_id != ordinal || e.low >= e.high)
      return false;
    auto found = edges.find({e.low, e.high});
    if (found == edges.end())
      return false;
    auto expected = found->second,
         actual = std::vector<std::uint64_t>{e.uses[0], e.uses[1]};
    std::sort(expected.begin(), expected.end());
    std::sort(actual.begin(), actual.end());
    if (expected != actual)
      return false;
  }
  for (const auto &l : a.vertex_links_) {
    if (l.vertex >= a.vertices_.size() || l.cyclic_facets.empty())
      return false;
    std::set<std::uint64_t> unique(l.cyclic_facets.begin(),
                                   l.cyclic_facets.end());
    if (unique.size() != l.cyclic_facets.size())
      return false;
    for (auto f : l.cyclic_facets)
      if (f >= a.facets_.size() ||
          std::find(a.facets_[f].vertices.begin(), a.facets_[f].vertices.end(),
                    l.vertex) == a.facets_[f].vertices.end())
        return false;
  }
  if (a.vertex_links_.size() != a.vertices_.size())
    return false;
  for (std::uint64_t vertex = 0; vertex < a.vertices_.size(); ++vertex) {
    std::map<std::uint64_t, std::set<std::uint64_t>> graph;
    for (const auto &e : a.edges_)
      if (e.low == vertex || e.high == vertex) {
        auto f0 = a.directed_uses_[e.uses[0]].facet,
             f1 = a.directed_uses_[e.uses[1]].facet;
        graph[f0].insert(f1);
        graph[f1].insert(f0);
      }
    if (graph.empty())
      return false;
    for (const auto &node : graph)
      if (node.second.size() != 2)
        return false;
    std::set<std::uint64_t> visited;
    std::vector<std::uint64_t> stack{graph.begin()->first};
    while (!stack.empty()) {
      auto f = stack.back();
      stack.pop_back();
      if (!visited.insert(f).second)
        continue;
      for (auto n : graph[f])
        stack.push_back(n);
    }
    auto record =
        std::find_if(a.vertex_links_.begin(), a.vertex_links_.end(),
                     [&](const auto &l) { return l.vertex == vertex; });
    if (record == a.vertex_links_.end() || visited.size() != graph.size() ||
        std::set<std::uint64_t>(record->cyclic_facets.begin(),
                                record->cyclic_facets.end()) != visited)
      return false;
  }
  if (a.edge_wedges_.size() != a.edges_.size() ||
      a.vertex_stars_.size() != a.vertex_links_.size())
    return false;
  for (const auto &w : a.edge_wedges_) {
    if (w.edge >= a.edges_.size() || !w.locally_embedded)
      return false;
    const auto &e = a.edges_[w.edge];
    std::array<std::uint64_t, 2> facets{
        {a.directed_uses_[e.uses[0]].facet, a.directed_uses_[e.uses[1]].facet}};
    if (w.facets != facets)
      return false;
  }
  for (const auto &s : a.vertex_stars_) {
    if (s.vertex >= a.vertices_.size() || !s.closed_disk)
      return false;
    auto found =
        std::find_if(a.vertex_links_.begin(), a.vertex_links_.end(),
                     [&](const auto &l) { return l.vertex == s.vertex; });
    if (found == a.vertex_links_.end() ||
        s.incident_facets != found->cyclic_facets.size())
      return false;
  }
  std::vector<std::uint64_t> membership(a.facets_.size(), 0);
  std::vector<validation_triangle<T>> triangles;
  for (const auto &facet : a.facets_) {
    if (facet.vertices.size() < 3 ||
        facet.decomposition.size() != facet.vertices.size() - 2 ||
        facet.dropped_axis > 2 || facet.shell >= a.shells_.size())
      return false;
    const auto support = facet.support_vertices;
    if (std::any_of(support.begin(), support.end(), [&](auto vertex) {
          return vertex >= points.size() ||
                 std::find(facet.vertices.begin(), facet.vertices.end(), vertex) ==
                     facet.vertices.end();
        }))
      return false;
    validation_triangle<T> support_triangle{points[support[0]], points[support[1]],
                                            points[support[2]]};
    const std::uint8_t expected_axis =
        input_relation_detail::projection_axis(support_triangle);
    if (facet.dropped_axis != expected_axis)
      return false;
    const auto &pa3 = support_triangle.a;
    const auto &pb3 = support_triangle.b;
    const auto &pc3 = support_triangle.c;
    const long double ux = static_cast<long double>(pb3[0]) - pa3[0];
    const long double uy = static_cast<long double>(pb3[1]) - pa3[1];
    const long double uz = static_cast<long double>(pb3[2]) - pa3[2];
    const long double vx = static_cast<long double>(pc3[0]) - pa3[0];
    const long double vy = static_cast<long double>(pc3[1]) - pa3[1];
    const long double vz = static_cast<long double>(pc3[2]) - pa3[2];
    std::array<long double, 4> diagnostic_plane{
        uy * vz - uz * vy, uz * vx - ux * vz, ux * vy - uy * vx, 0};
    diagnostic_plane[3] =
        -(diagnostic_plane[0] * pa3[0] + diagnostic_plane[1] * pa3[1] +
          diagnostic_plane[2] * pa3[2]);
    if (diagnostic_plane != facet.support_plane)
      return false;
    std::vector<std::array<T, 2>> projected;
    for (auto vertex : facet.vertices) {
      if (vertex >= points.size())
        return false;
      const auto &p = points[vertex];
      if (input_relation_detail::orient3(points[support[0]], points[support[1]],
                                         points[support[2]], p) != 0)
        return false;
      projected.push_back(input_relation_detail::project(p, facet.dropped_axis));
    }
    for (std::size_t i = 0; i < projected.size(); ++i)
      for (std::size_t j = i + 1; j < projected.size(); ++j) {
        const bool adjacent = j == i + 1 ||
                              (i == 0 && j + 1 == projected.size());
        const auto relation = input_relation_detail::segment_relation_2d(
            projected[i], projected[(i + 1) % projected.size()], projected[j],
            projected[(j + 1) % projected.size()]);
        if ((adjacent && relation != 1) || (!adjacent && relation != 0))
          return false;
      }
    const int polygon_sign = exact_sign(exact_polygon_area_2d(projected));
    long double projected_area = 0;
    for (std::size_t i = 0; i < projected.size(); ++i)
      projected_area += static_cast<long double>(projected[i][0]) *
                            projected[(i + 1) % projected.size()][1] -
                        static_cast<long double>(projected[(i + 1) %
                                                           projected.size()][0]) *
                            projected[i][1];
    if (facet.projected_area != projected_area / 2)
      return false;
    T maximum_radius = T(0);
    for (auto vertex : facet.vertices)
      maximum_radius =
          std::max(maximum_radius, a.bounded_vertices_[vertex].radial_error);
    if ((polygon_sign != -1 && polygon_sign != 1) ||
        input_relation_detail::bounded_polygon_area_sign(projected,
                                                         maximum_radius) !=
            polygon_sign)
      return false;
    std::map<std::pair<std::uint64_t, std::uint64_t>, std::uint64_t> uses;
    std::set<std::pair<std::uint64_t, std::uint64_t>> boundary;
    for (std::size_t i = 0; i < facet.vertices.size(); ++i)
      boundary.insert(
          {std::min(facet.vertices[i],
                    facet.vertices[(i + 1) % facet.vertices.size()]),
           std::max(facet.vertices[i],
                    facet.vertices[(i + 1) % facet.vertices.size()])});
    for (const auto &t : facet.decomposition) {
      for (auto v : t)
        if (v >= points.size() ||
            std::find(facet.vertices.begin(), facet.vertices.end(), v) ==
                facet.vertices.end())
          return false;
      auto pa =
               input_relation_detail::project(points[t[0]], facet.dropped_axis),
           pb =
               input_relation_detail::project(points[t[1]], facet.dropped_axis),
           pc =
               input_relation_detail::project(points[t[2]], facet.dropped_axis);
      const int triangle_sign = input_relation_detail::orient2(pa, pb, pc);
      const std::vector<std::array<T, 2>> bounded_triangle{pa, pb, pc};
      if (triangle_sign != polygon_sign ||
          input_relation_detail::bounded_polygon_area_sign(
              bounded_triangle, maximum_radius) != polygon_sign)
        return false;
      for (std::size_t e = 0; e < 3; ++e) {
        auto u = t[e], v = t[(e + 1) % 3];
        ++uses[{std::min(u, v), std::max(u, v)}];
      }
    }
    for (const auto &e : uses)
      if (e.second != (boundary.count(e.first) ? 1U : 2U))
        return false;
  }
  for (std::uint64_t ordinal = 0; ordinal < a.shells_.size(); ++ordinal) {
    const auto &s = a.shells_[ordinal];
    if (s.canonical_id != ordinal ||
        s.material_side != occupied_side::negative ||
        s.empty_side != occupied_side::positive)
      return false;
    if (s.facets.empty() ||
        std::any_of(s.facets.begin(), s.facets.end(),
                    [&](auto f) { return f >= a.facets_.size(); }))
      return false;
    if (s.facets.empty())
      return false;
    const auto reference_id = a.facets_[s.facets.front()].vertices.front();
    const auto reference = points[reference_id];
    const long double reference_error =
        a.bounded_vertices_[reference_id].radial_error;
    auto bounded_volume = finite_interval<T>::checked_singleton(T(0));
    if (!bounded_volume || !std::isfinite(s.signed_volume) ||
        !std::isfinite(s.volume_uncertainty) || s.volume_uncertainty < 0)
      return false;
    for (auto f : s.facets) {
      if (f >= a.facets_.size() || a.facets_[f].shell != s.canonical_id)
        return false;
      ++membership[f];
      for (std::uint64_t local = 0; local < a.facets_[f].decomposition.size();
           ++local) {
        auto ids3 = a.facets_[f].decomposition[local];
        for (auto v : ids3)
          if (v >= points.size())
            return false;
        validation_triangle<T> t{points[ids3[0]],
                                 points[ids3[1]],
                                 points[ids3[2]],
                                 f,
                                 local,
                                 s.canonical_id,
                                 ids3};
        triangles.push_back(t);
        const long double radius =
            reference_error +
            std::max({static_cast<long double>(
                          a.bounded_vertices_[ids3[0]].radial_error),
                      static_cast<long double>(
                          a.bounded_vertices_[ids3[1]].radial_error),
                      static_cast<long double>(
                          a.bounded_vertices_[ids3[2]].radial_error)});
        if (radius > static_cast<long double>(std::numeric_limits<T>::max()))
          return false;
        const auto determinant = input_relation_detail::bounded_orient3_interval(
            t.a, t.b, t.c, reference, static_cast<T>(radius));
        if (!determinant)
          return false;
        const auto sum = interval_add(*bounded_volume, *determinant);
        if (!sum)
          return false;
        bounded_volume = sum.value;
      }
    }
    if (bounded_volume->contains_zero())
      return false;
    const auto orientation = bounded_volume->lower() > T(0)
                                 ? shell_orientation::outward
                                 : shell_orientation::inward;
    if (orientation != s.intrinsic_orientation)
      return false;
  }
  if (!std::all_of(membership.begin(), membership.end(),
                   [](auto n) { return n == 1; }))
    return false;
  std::vector<std::vector<std::uint64_t>> facet_adjacency(a.facets_.size());
  for (const auto &e : a.edges_) {
    auto f0 = a.directed_uses_[e.uses[0]].facet,
         f1 = a.directed_uses_[e.uses[1]].facet;
    facet_adjacency[f0].push_back(f1);
    facet_adjacency[f1].push_back(f0);
  }
  std::vector<bool> shell_seen(a.facets_.size(), false);
  std::vector<std::set<std::uint64_t>> reconstructed_shells;
  for (std::uint64_t seed = 0; seed < a.facets_.size(); ++seed)
    if (!shell_seen[seed]) {
      std::set<std::uint64_t> component;
      std::vector<std::uint64_t> queue{seed};
      shell_seen[seed] = true;
      for (std::size_t q = 0; q < queue.size(); ++q) {
        auto f = queue[q];
        component.insert(f);
        for (auto n : facet_adjacency[f])
          if (!shell_seen[n]) {
            shell_seen[n] = true;
            queue.push_back(n);
          }
      }
      reconstructed_shells.push_back(std::move(component));
    }
  std::vector<std::set<std::uint64_t>> published_shells;
  for (const auto &s : a.shells_)
    published_shells.emplace_back(s.facets.begin(), s.facets.end());
  std::sort(reconstructed_shells.begin(), reconstructed_shells.end());
  std::sort(published_shells.begin(), published_shells.end());
  if (reconstructed_shells != published_shells)
    return false;
  const T uncertainty = source.operand() == operand_id::a
                            ? precision.effective_input_precision_a()
                            : precision.effective_input_precision_b();
  const auto inflation = directed_multiply(uncertainty, T(2));
  if (!inflation)
    return false;
  const T relation_radius = inflation.value.upper;
  using relation_key = std::array<std::uint64_t, 4>;
  std::map<relation_key, const geometry_relation_record *> published;
  for (const auto &r : a.relations_) {
    relation_key key{r.facet_a, r.triangle_a, r.facet_b, r.triangle_b};
    if (!published.emplace(key, &r).second)
      return false;
  }
  std::uint64_t candidates = 0;
  for (std::size_t i = 0; i < triangles.size(); ++i)
    for (std::size_t j = i + 1; j < triangles.size(); ++j) {
      if (triangles[i].facet == triangles[j].facet)
        continue;
      if (!bounds_overlap(triangle_bounds(triangles[i], relation_radius),
                          triangle_bounds(triangles[j], relation_radius)))
        continue;
      ++candidates;
      relation_key key{triangles[i].facet, triangles[i].local,
                       triangles[j].facet, triangles[j].local};
      auto found = published.find(key);
      if (found == published.end()) {
        key = {triangles[j].facet, triangles[j].local, triangles[i].facet,
               triangles[i].local};
        found = published.find(key);
      }
      if (found == published.end() ||
           found->second->relation != input_geometry_verifier::triangle_relation(
                                          triangles[i], triangles[j]))
        return false;
      const bool uncertainty_separated =
           found->second->relation == validation_relation::definitely_disjoint &&
           definitely_separated_under_uncertainty(triangles[i], triangles[j],
                                                   relation_radius);
      if (found->second->uncertainty_separated != uncertainty_separated)
        return false;
      if (triangles[i].shell == triangles[j].shell) {
        std::vector<std::uint64_t> shared;
        for (auto x : triangles[i].vertices)
          for (auto y : triangles[j].vertices)
            if (x == y &&
                std::find(shared.begin(), shared.end(), x) == shared.end())
              shared.push_back(x);
        auto relation = found->second->relation;
        if (relation == validation_relation::point_contact &&
            shared.size() != 1)
          return false;
        if (relation == validation_relation::edge_contact) {
          if (shared.size() != 2)
            return false;
          std::sort(shared.begin(), shared.end());
          auto edge = edges.find({shared[0], shared[1]});
          if (edge == edges.end())
            return false;
          std::set<std::uint64_t> incident;
          for (auto use : edge->second)
            incident.insert(a.directed_uses_[use].facet);
          if (incident !=
              std::set<std::uint64_t>{triangles[i].facet, triangles[j].facet})
            return false;
        }
        if (relation == validation_relation::definitely_disjoint &&
            shared.empty()) {
          bool facets_share = false;
          for (auto x : a.facets_[triangles[i].facet].vertices)
            for (auto y : a.facets_[triangles[j].facet].vertices)
              facets_share |= x == y;
           if (!facets_share && !definitely_separated_under_uncertainty(
                                   triangles[i], triangles[j], relation_radius))
            return false;
        }
      }
    }
  if (candidates != a.statistics_.triangle_pairs ||
      a.statistics_.relation_calls != candidates ||
      published.size() != candidates)
    return false;
  for (const auto &w : a.edge_wedges_) {
    const auto &e = a.edges_[w.edge];
    bool certified = false;
    for (const auto &r : a.relations_) {
      if (r.relation != validation_relation::edge_contact ||
          !((r.facet_a == w.facets[0] && r.facet_b == w.facets[1]) ||
            (r.facet_a == w.facets[1] && r.facet_b == w.facets[0])))
        continue;
      auto ta = std::find_if(triangles.begin(), triangles.end(),
                             [&](const auto &t) {
                               return t.facet == r.facet_a &&
                                      t.local == r.triangle_a;
                             }),
           tb = std::find_if(
               triangles.begin(), triangles.end(), [&](const auto &t) {
                 return t.facet == r.facet_b && t.local == r.triangle_b;
               });
      if (ta == triangles.end() || tb == triangles.end())
        return false;
      std::vector<std::uint64_t> shared;
      for (auto x : ta->vertices)
        for (auto y : tb->vertices)
          if (x == y &&
              std::find(shared.begin(), shared.end(), x) == shared.end())
            shared.push_back(x);
      std::sort(shared.begin(), shared.end());
      certified |= shared == std::vector<std::uint64_t>{e.low, e.high};
    }
    if (!certified)
      return false;
  }
  for (const auto &s : a.vertex_stars_) {
    auto link =
        std::find_if(a.vertex_links_.begin(), a.vertex_links_.end(),
                     [&](const auto &l) { return l.vertex == s.vertex; });
    if (link == a.vertex_links_.end())
      return false;
    for (std::size_t i = 0; i < link->cyclic_facets.size(); ++i) {
      auto f0 = link->cyclic_facets[i],
           f1 = link->cyclic_facets[(i + 1) % link->cyclic_facets.size()];
      if (!std::any_of(a.edge_wedges_.begin(), a.edge_wedges_.end(),
                       [&](const auto &w) {
                         return (w.facets[0] == f0 && w.facets[1] == f1) ||
                                (w.facets[0] == f1 && w.facets[1] == f0);
                       }))
        return false;
    }
  }
  const std::uint64_t expected_pairs =
      a.shells_.size() < 2 ? 0 : a.shells_.size() * (a.shells_.size() - 1) / 2;
  if (a.shell_pairs_.size() != expected_pairs)
    return false;
  std::set<std::pair<std::uint64_t, std::uint64_t>> shell_keys;
  for (const auto &p : a.shell_pairs_) {
    if (p.shell_a >= a.shells_.size() || p.shell_b >= a.shells_.size() ||
        p.shell_a >= p.shell_b ||
        !shell_keys.insert({p.shell_a, p.shell_b}).second)
      return false;
  }
  auto triangles_for_shell = [&](std::uint64_t shell) {
    std::vector<validation_triangle<T>> result;
    for (const auto &triangle : triangles)
      if (triangle.shell == shell)
        result.push_back(triangle);
    return result;
  };
  auto classify_shell_vertex = [&](std::uint64_t source_shell,
                                   std::uint64_t target_shell) {
    auto target = triangles_for_shell(target_shell);
    std::set<std::uint64_t> shell_vertices;
    for (auto facet : a.shells_[source_shell].facets)
      shell_vertices.insert(a.facets_[facet].vertices.begin(),
                             a.facets_[facet].vertices.end());
    bool inside = false, outside = false;
    for (auto vertex : shell_vertices) {
      const T radius = static_cast<T>(uncertainty);
      auto result = input_geometry_verifier::point_against_triangles(
          points[vertex], target, radius);
      if (result == point_shell_result::ambiguous)
        return result;
      inside |= result == point_shell_result::inside;
      outside |= result == point_shell_result::outside;
      if (inside && outside)
        return point_shell_result::ambiguous;
    }
    return inside ? point_shell_result::inside
                  : outside ? point_shell_result::outside
                            : point_shell_result::boundary;
  };
  for (const auto &pair : a.shell_pairs_) {
    bool point = false, edge = false, face = false;
    for (const auto &relation : a.relations_) {
      auto first = a.facets_[relation.facet_a].shell;
      auto second = a.facets_[relation.facet_b].shell;
      if (!((first == pair.shell_a && second == pair.shell_b) ||
            (first == pair.shell_b && second == pair.shell_a)))
        continue;
      point |= relation.relation == validation_relation::point_contact;
      edge |= relation.relation == validation_relation::edge_contact;
      if (relation.relation == validation_relation::whole_patch_coincidence)
        return false;
      if (relation.relation ==
          validation_relation::coplanar_positive_area_overlap) {
        auto ta = std::find_if(triangles.begin(), triangles.end(),
                               [&](const auto &t) {
                                 return t.facet == relation.facet_a &&
                                        t.local == relation.triangle_a;
                               }),
             tb = std::find_if(triangles.begin(), triangles.end(),
                               [&](const auto &t) {
                                 return t.facet == relation.facet_b &&
                                        t.local == relation.triangle_b;
                               });
        if (ta == triangles.end() || tb == triangles.end())
          return false;
        auto normal = [](const auto &t) {
          std::array<long double, 3> u{t.b[0] - t.a[0], t.b[1] - t.a[1],
                                       t.b[2] - t.a[2]},
              v{t.c[0] - t.a[0], t.c[1] - t.a[1], t.c[2] - t.a[2]};
          return std::array<long double, 3>{u[1] * v[2] - u[2] * v[1],
                                            u[2] * v[0] - u[0] * v[2],
                                            u[0] * v[1] - u[1] * v[0]};
        };
        auto n0 = normal(*ta), n1 = normal(*tb);
        if (n0[0] * n1[0] + n0[1] * n1[1] + n0[2] * n1[2] >= 0)
          return false;
      }
      face |= relation.relation ==
              validation_relation::coplanar_positive_area_overlap;
    }
    auto a_in_b = classify_shell_vertex(pair.shell_a, pair.shell_b);
    auto b_in_a = classify_shell_vertex(pair.shell_b, pair.shell_a);
    shell_pair_relation expected = shell_pair_relation::definitely_disjoint;
    const bool contact = point || edge || face;
    if (contact && (a_in_b == point_shell_result::inside ||
                    b_in_a == point_shell_result::inside))
      return false;
    if (a_in_b == point_shell_result::inside &&
        b_in_a == point_shell_result::inside)
      return false;
    if (a_in_b == point_shell_result::inside)
      expected = shell_pair_relation::a_inside_b;
    else if (b_in_a == point_shell_result::inside)
      expected = shell_pair_relation::b_inside_a;
    else if (face)
      expected = shell_pair_relation::authorized_face_contact;
    else if (edge)
      expected = shell_pair_relation::authorized_edge_contact;
    else if (point)
      expected = shell_pair_relation::authorized_point_contact;
    if (pair.relation != expected)
      return false;
  }
  auto is_inside = [&](std::uint64_t inner, std::uint64_t outer) {
    for (const auto &p : a.shell_pairs_) {
      if (p.shell_a == inner && p.shell_b == outer &&
          p.relation == shell_pair_relation::a_inside_b)
        return true;
      if (p.shell_b == inner && p.shell_a == outer &&
          p.relation == shell_pair_relation::b_inside_a)
        return true;
    }
    return false;
  };
  for (std::uint64_t child = 0; child < a.shells_.size(); ++child) {
    std::vector<std::uint64_t> parents;
    for (const auto &p : a.shell_pairs_) {
      if (p.shell_a == child && p.relation == shell_pair_relation::a_inside_b)
        parents.push_back(p.shell_b);
      if (p.shell_b == child && p.relation == shell_pair_relation::b_inside_a)
        parents.push_back(p.shell_a);
    }
    for (std::size_t i = 0; i < parents.size(); ++i)
      for (std::size_t j = i + 1; j < parents.size(); ++j)
        if (!is_inside(parents[i], parents[j]) &&
            !is_inside(parents[j], parents[i]))
          return false;
    std::int64_t expected = -1;
    if (!parents.empty()) {
      auto immediate = parents.front();
      for (auto candidate : parents)
        if (is_inside(candidate, immediate))
          immediate = candidate;
      expected = static_cast<std::int64_t>(immediate);
    }
    if (a.shells_[child].parent != expected)
      return false;
  }
  for (std::uint64_t s = 0; s < a.shells_.size(); ++s) {
    const auto &shell = a.shells_[s];
    if (shell.parent >= static_cast<std::int64_t>(a.shells_.size()) ||
        shell.parent == static_cast<std::int64_t>(s))
      return false;
    std::set<std::uint64_t> seen;
    std::uint32_t depth = 0;
    auto current = s;
    while (a.shells_[current].parent >= 0) {
      if (!seen.insert(current).second)
        return false;
      current = static_cast<std::uint64_t>(a.shells_[current].parent);
      ++depth;
    }
    if (depth != shell.depth ||
        shell.intrinsic_orientation != (depth % 2 ? shell_orientation::inward
                                                  : shell_orientation::outward))
      return false;
  }
  return true;
}

struct validated_operand_test_access final {
  template <class T, class I>
  static void refresh_encoding(validated_operand<T, I> &a) {
    a.canonical_bytes_ = encode_validated_operand_semantic(a);
    a.digest_ = sha256::digest(a.canonical_bytes_);
    a.source_presentation_bytes_ =
        encode_validated_operand_source_presentation(a);
    a.source_presentation_digest_ =
        sha256::digest(a.source_presentation_bytes_);
  }
  template <class T, class I>
  static void set_directed_origin(validated_operand<T, I> &a, std::size_t use,
                                  std::uint64_t value) {
    a.directed_uses_.at(use).origin = value;
    refresh_encoding(a);
  }
  template <class T, class I>
  static void set_edge_low(validated_operand<T, I> &a, std::size_t edge,
                           std::uint64_t value) {
    a.edges_.at(edge).low = value;
    refresh_encoding(a);
  }
  template <class T, class I>
  static void clear_vertex_link(validated_operand<T, I> &a, std::size_t link) {
    a.vertex_links_.at(link).cyclic_facets.clear();
    refresh_encoding(a);
  }
  template <class T, class I>
  static void clear_shell_facets(validated_operand<T, I> &a,
                                 std::size_t shell) {
    a.shells_.at(shell).facets.clear();
    refresh_encoding(a);
  }
  template <class T, class I>
  static void drop_decomposition_triangle(validated_operand<T, I> &a,
                                          std::size_t facet) {
    a.facets_.at(facet).decomposition.pop_back();
    refresh_encoding(a);
  }
  template <class T, class I>
  static void set_link_facet(validated_operand<T, I> &a, std::size_t link,
                             std::size_t position, std::uint64_t value) {
    a.vertex_links_.at(link).cyclic_facets.at(position) = value;
    refresh_encoding(a);
  }
  template <class T, class I>
  static void set_shell_facet(validated_operand<T, I> &a, std::size_t shell,
                              std::size_t position, std::uint64_t value) {
    a.shells_.at(shell).facets.at(position) = value;
    refresh_encoding(a);
  }
  template <class T, class I>
  static void collapse_decomposition_triangle(validated_operand<T, I> &a,
                                              std::size_t facet,
                                              std::size_t triangle) {
    a.facets_.at(facet).decomposition.at(triangle)[1] =
        a.facets_.at(facet).decomposition.at(triangle)[0];
    refresh_encoding(a);
  }
  template <class T, class I>
  static void set_shell_depth(validated_operand<T, I> &a, std::size_t shell,
                              std::uint32_t depth) {
    a.shells_.at(shell).depth = depth;
  }
  template <class T, class I>
  static void set_relation(validated_operand<T, I> &a, std::size_t relation,
                           validation_relation value) {
    a.relations_.at(relation).relation = value;
  }
  template <class T, class I>
  static void set_occupied_side(validated_operand<T, I> &a, std::size_t shell,
                                occupied_side value) {
    a.shells_.at(shell).material_side = value;
  }
  template <class T, class I>
  static void set_wedge_embedded(validated_operand<T, I> &a, std::size_t edge,
                                 bool value) {
    a.edge_wedges_.at(edge).locally_embedded = value;
  }
  template <class T, class I>
  static void set_star_degree(validated_operand<T, I> &a, std::size_t vertex,
                              std::uint64_t value) {
    a.vertex_stars_.at(vertex).incident_facets = value;
  }
  template <class T, class I>
  static void set_wedge_facets(validated_operand<T, I> &a, std::size_t edge,
                               std::array<std::uint64_t, 2> value) {
    a.edge_wedges_.at(edge).facets = value;
  }
  template <class T, class I>
  static void set_persistent_bytes(validated_operand<T, I> &a,
                                   std::uint64_t value) {
    a.statistics_.persistent_bytes = value;
  }
  template <class T, class I>
  static void set_vertex_id(validated_operand<T, I> &a, std::size_t vertex,
                            std::uint64_t value) {
    a.vertices_.at(vertex).canonical_id = value;
    refresh_encoding(a);
  }
  template <class T, class I>
  static void set_facet_id(validated_operand<T, I> &a, std::size_t facet,
                           std::uint64_t value) {
    a.facets_.at(facet).canonical_id = value;
    refresh_encoding(a);
  }
  template <class T, class I>
  static void set_presentation_facet(validated_operand<T, I> &a,
                                     std::size_t facet, std::uint64_t value) {
    a.facets_.at(facet).presentation_facet = value;
    refresh_encoding(a);
  }
  template <class T, class I>
  static void set_normalization_corner(validated_operand<T, I> &a,
                                       std::size_t position,
                                       std::uint64_t value) {
    a.normalization_.at(position).retained_corner = value;
    refresh_encoding(a);
  }
  template <class T, class I>
  static void set_support_plane(validated_operand<T, I> &a, std::size_t facet,
                                std::size_t coefficient, long double value) {
    a.facets_.at(facet).support_plane.at(coefficient) = value;
    refresh_encoding(a);
  }
  template <class T, class I>
  static void set_projection_axis(validated_operand<T, I> &a,
                                  std::size_t facet, std::uint8_t value) {
    a.facets_.at(facet).dropped_axis = value;
    refresh_encoding(a);
  }
  template <class T, class I>
  static void set_projected_area(validated_operand<T, I> &a,
                                 std::size_t facet, long double value) {
    a.facets_.at(facet).projected_area = value;
    refresh_encoding(a);
  }
  template <class T, class I>
  static void set_uncertainty_separated(validated_operand<T, I> &a,
                                        std::size_t relation, bool value) {
    a.relations_.at(relation).uncertainty_separated = value;
    refresh_encoding(a);
  }
  template <class T, class I>
  static void clear_context_digest(validated_operand<T, I> &a) {
    a.context_digest_ = {};
    refresh_encoding(a);
  }
  template <class T, class I>
  static void clear_source_digest(validated_operand<T, I> &a) {
    a.source_digest_ = {};
    refresh_encoding(a);
  }
  template <class T, class I>
  static void clear_precision_digest(validated_operand<T, I> &a) {
    a.precision_digest_ = {};
    refresh_encoding(a);
  }
  template <class T, class I>
  static void set_edge_id(validated_operand<T, I> &a, std::size_t edge,
                          std::uint64_t value) {
    a.edges_.at(edge).canonical_id = value;
    refresh_encoding(a);
  }
  template <class T, class I>
  static void set_shell_id(validated_operand<T, I> &a, std::size_t shell,
                           std::uint64_t value) {
    a.shells_.at(shell).canonical_id = value;
    refresh_encoding(a);
  }
};

} // namespace ygor::mesh_boolean::bounded
