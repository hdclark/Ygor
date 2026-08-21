#pragma once

#include "CanonicalSourceTopology.h"
#include "CheckedArithmetic.h"
#include "InputGeometryRelations.h"
#include "PrecisionCapabilities.h"
#include "Transaction.h"
#include "ValidatedOperandCodec.h"
#include "ValidatedOperandVerifier.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <set>
#include <tuple>

namespace ygor::mesh_boolean::bounded {

template <class T, class I> class input_validation_builder final {
  using artifact = validated_operand<T, I>;
  const immutable_source_mesh<T, I> &source_;
  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  input_validation_capabilities caps_;
  struct typed_reservation {
    resource_kind kind;
    resource_reservation reservation;
  };
  artifact out_;
  std::vector<std::vector<std::uint64_t>> source_rings_, rings_;
  std::vector<std::uint64_t> s2c_, canonical_to_source_, canonical_to_source_facet_;
  std::vector<validation_point3<T>> points_;
  std::vector<validation_triangle<T>> triangles_;
  std::set<std::pair<std::uint64_t, std::uint64_t>> unresolved_shell_pairs_;
  std::vector<typed_reservation> reservations_;

  auto failure(input_validation_subcode code,
               bounded_boolean_error_category category, const char *summary,
               std::uint32_t checkpoint) const {
    auto error = input_validation_error(source_.operand(), code, category,
                                        summary, checkpoint);
    error.context_digest = precision_.digest();
    error.replay_digest = context_.replay_digest;
    return boolean_outcome<std::shared_ptr<const artifact>>::failure(error);
  }
  bool cancelled() const noexcept {
    return caps_.cancellation && caps_.cancellation->cancellation_requested();
  }
  std::array<std::uint64_t, 3> bits(std::uint64_t v) const {
    std::array<std::uint64_t, 3> b{};
    for (std::size_t k = 0; k < 3; ++k)
      b[k] = source_.coordinate_bits()[v * 3 + k];
    return b;
  }
  validation_point3<T> source_point(std::uint64_t v) const {
    return {from_bits<T>(source_.coordinate_bits()[v * 3]),
            from_bits<T>(source_.coordinate_bits()[v * 3 + 1]),
            from_bits<T>(source_.coordinate_bits()[v * 3 + 2])};
  }
  std::optional<bounded_boolean_error>
  error(input_validation_subcode code, bounded_boolean_error_category category,
        const char *summary, std::uint32_t checkpoint) const {
    return input_validation_error(source_.operand(), code, category, summary,
                                  checkpoint);
  }

  bool reserve(resource_kind kind, std::uint64_t amount) {
    auto reservation = caps_.resources->reserve(kind, amount);
    if (!reservation)
      return false;
    reservations_.push_back({kind, std::move(*reservation)});
    return true;
  }
  bool actual_usage(resource_kind kind, std::uint64_t &usage) const {
    std::uint64_t corners = 0;
    for (const auto &r : rings_)
      if (!checked_add(corners, static_cast<std::uint64_t>(r.size()), corners))
        return false;
    switch (kind) {
    case resource_kind::source_vertices:
      usage = out_.vertices_.size();
      return true;
    case resource_kind::source_faces:
    case resource_kind::source_rings:
      usage = out_.facets_.size();
      return true;
    case resource_kind::source_indices:
      usage = corners;
      return true;
    case resource_kind::relations:
      usage = out_.relations_.size();
      return true;
    case resource_kind::verification_findings:
      usage = 0;
      return true;
    case resource_kind::temporary_bytes:
      usage = 0;
      return true;
    case resource_kind::work_units:
      usage = out_.statistics_.canonical_branches;
      return checked_add(usage, out_.statistics_.relation_calls, usage) &&
             checked_add(usage, corners, usage) &&
             checked_add(usage, static_cast<std::uint64_t>(out_.facets_.size()),
                         usage);
    case resource_kind::persistent_bytes: {
      std::uint64_t bytes = 0;
      auto add = [&](std::uint64_t value) { return checked_add(bytes, value, bytes); };
      auto add_records = [&](std::uint64_t count, std::uint64_t size) {
        std::uint64_t value = 0;
        return checked_multiply(count, size, value) && add(value);
      };
      if (!add(out_.canonical_bytes_.size()) ||
          !add(out_.source_presentation_bytes_.size()) ||
          !add_records(out_.vertices_.size(), sizeof(validated_vertex_record)) ||
          !add_records(out_.bounded_vertices_.size(),
                       sizeof(bounded_source_vertex_evidence<T>)) ||
          !add_records(out_.facets_.size(), sizeof(validated_facet_record)) ||
          !add_records(out_.directed_uses_.size(),
                       sizeof(directed_source_use_record)) ||
          !add_records(out_.edges_.size(), sizeof(undirected_source_edge_record)) ||
          !add_records(out_.vertex_links_.size(),
                       sizeof(validated_vertex_link_record)) ||
          !add_records(out_.shells_.size(), sizeof(validated_shell_record)) ||
          !add_records(out_.relations_.size(), sizeof(geometry_relation_record)) ||
          !add_records(out_.shell_pairs_.size(), sizeof(shell_pair_record)) ||
          !add_records(out_.edge_wedges_.size(), sizeof(edge_wedge_record)) ||
          !add_records(out_.vertex_stars_.size(), sizeof(vertex_star_record)) ||
          !add_records(out_.normalization_.size(),
                       sizeof(normalized_position_record)))
        return false;
      for (const auto &facet : out_.facets_)
        if (!add_records(facet.vertices.size(), sizeof(std::uint64_t)) ||
            !add_records(facet.decomposition.size(),
                         sizeof(std::array<std::uint64_t, 3>)))
          return false;
      for (const auto &link : out_.vertex_links_)
        if (!add_records(link.cyclic_facets.size(), sizeof(std::uint64_t)))
          return false;
      for (const auto &shell : out_.shells_)
        if (!add_records(shell.facets.size(), sizeof(std::uint64_t)))
          return false;
      usage = bytes;
      return true;
    }
    default:
      usage = 0;
      return true;
    }
  }
  bool preflight() {
    std::uint64_t n = source_.indices().size(), pair_work = 0, temp = 0,
                   persistent = 0, work = 0, temporary = 0, relations = 0,
                   findings = 0;
    if (!checked_multiply<std::uint64_t>(n, n, pair_work) ||
        !checked_multiply<std::uint64_t>(n, 256, temp) ||
        !checked_multiply<std::uint64_t>(n, 1024, persistent) ||
        !checked_add(pair_work, source_.face_count(), work) ||
        !checked_add(work, std::uint64_t{1}, work) ||
        !checked_add(temp, std::uint64_t{1024}, temporary) ||
        !checked_add(persistent, std::uint64_t{8192}, persistent) ||
        !checked_add(pair_work / 2, std::uint64_t{1}, relations) ||
        !checked_add(n, std::uint64_t{1}, findings))
      return false;
    return reserve(resource_kind::source_vertices, source_.vertex_count()) &&
           reserve(resource_kind::source_faces, source_.face_count()) &&
           reserve(resource_kind::source_indices, n) &&
           reserve(resource_kind::source_rings, source_.face_count()) &&
            reserve(resource_kind::work_units, work) &&
            reserve(resource_kind::temporary_bytes, temporary) &&
            reserve(resource_kind::persistent_bytes, persistent) &&
            reserve(resource_kind::relations, relations) &&
            reserve(resource_kind::verification_findings, findings);
  }

  std::optional<bounded_boolean_error> normalize() {
    std::vector<bounded_boolean_error> findings;
    auto record = [&](input_validation_subcode code, const char *summary,
                      std::uint32_t checkpoint) {
      findings.push_back(*error(code,
                                bounded_boolean_error_category::input_contract_error,
                                summary, checkpoint));
    };
    source_rings_.resize(source_.face_count());
    for (std::size_t f = 0; f < source_rings_.size(); ++f) {
      auto &r = source_rings_[f];
      std::size_t last_retained_record =
          std::numeric_limits<std::size_t>::max();
      for (auto p = source_.face_offsets()[f];
           p < source_.face_offsets()[f + 1]; ++p) {
        auto v = static_cast<std::uint64_t>(source_.indices()[p]);
        if (v >= source_.vertex_count()) {
          record(input_validation_subcode::out_of_range_index,
                 "source index is out of range", 3);
          continue;
        }
        if (!r.empty() && r.back() == v) {
          out_.normalization_.push_back({p, f, 0,
                                         ring_position_action::consecutive_duplicate,
                                         r.size() - 1});
          continue;
        }
        r.push_back(v);
        last_retained_record = out_.normalization_.size();
        out_.normalization_.push_back(
            {p, f, 0, ring_position_action::retained, r.size() - 1});
      }
      if (r.size() > 1 && r.front() == r.back()) {
        const auto removed_corner = r.size() - 1;
        r.pop_back();
        if (last_retained_record ==
                std::numeric_limits<std::size_t>::max() ||
            last_retained_record >= out_.normalization_.size())
          return error(input_validation_subcode::verifier_rejection,
                       bounded_boolean_error_category::internal_invariant_error,
                       "closing source position was not retained", 4);
        auto &closing = out_.normalization_[last_retained_record];
        closing.action = ring_position_action::duplicate_closure;
        closing.retained_corner = 0;
        for (std::size_t record = last_retained_record + 1;
             record < out_.normalization_.size(); ++record) {
          auto &trailing = out_.normalization_[record];
          if (trailing.source_facet != f)
            break;
          if (trailing.action ==
                  ring_position_action::consecutive_duplicate &&
              trailing.retained_corner == removed_corner)
            trailing.retained_corner = 0;
        }
      }
      if (r.size() < 3)
        record(input_validation_subcode::undersized_ring,
               "normalized facet has fewer than three corners", 4);
      std::set<std::uint64_t> unique(r.begin(), r.end());
      if (unique.size() != r.size())
        record(input_validation_subcode::repeated_ring_vertex,
               "normalized facet repeats a vertex", 4);
    }
    if (!findings.empty()) {
      return *std::min_element(findings.begin(), findings.end(),
                               [](const auto &a, const auto &b) {
                                 return std::tie(a.checkpoint, a.subcode) <
                                        std::tie(b.checkpoint, b.subcode);
                               });
    }
    return std::nullopt;
  }
  std::optional<bounded_boolean_error> temporary_topology() {
    using key = std::pair<std::uint64_t, std::uint64_t>;
    std::map<key, std::vector<std::pair<std::uint64_t, std::uint64_t>>> uses;
    for (std::uint64_t f = 0; f < source_rings_.size(); ++f)
      for (std::uint64_t c = 0; c < source_rings_[f].size(); ++c) {
        auto a = source_rings_[f][c],
             b = source_rings_[f][(c + 1) % source_rings_[f].size()];
        if (a == b)
          return error(input_validation_subcode::self_edge,
                       bounded_boolean_error_category::input_contract_error,
                       "topological self edge", 5);
        uses[{std::min(a, b), std::max(a, b)}].push_back({f, c});
      }
    std::map<std::pair<std::uint64_t, std::uint64_t>,
             std::pair<std::uint64_t, std::uint64_t>>
        reciprocal;
    std::vector<bounded_boolean_error> edge_findings;
    for (const auto &entry : uses) {
      if (entry.second.size() < 2)
        edge_findings.push_back(*error(
            input_validation_subcode::open_boundary,
            bounded_boolean_error_category::input_contract_error,
            "open boundary edge", 5));
      else if (entry.second.size() > 2)
        edge_findings.push_back(*error(
            input_validation_subcode::non_manifold_edge,
            bounded_boolean_error_category::input_contract_error,
            "non-manifold edge", 5));
      if (entry.second.size() != 2)
        continue;
      auto a = entry.second[0], b = entry.second[1];
      if (source_rings_[a.first][a.second] !=
              source_rings_[b.first]
                           [(b.second + 1) % source_rings_[b.first].size()] ||
          source_rings_[a.first]
                       [(a.second + 1) % source_rings_[a.first].size()] !=
              source_rings_[b.first][b.second])
        edge_findings.push_back(*error(
            input_validation_subcode::same_direction_pair,
            bounded_boolean_error_category::input_contract_error,
            "paired uses have the same direction", 5));
      reciprocal[a] = b;
      reciprocal[b] = a;
    }
    if (!edge_findings.empty())
      return *std::min_element(edge_findings.begin(), edge_findings.end(),
                               [](const auto &a, const auto &b) {
                                 return a.subcode < b.subcode;
                               });
    std::map<std::uint64_t,
             std::vector<std::pair<std::uint64_t, std::uint64_t>>>
        corners;
    for (std::uint64_t f = 0; f < source_rings_.size(); ++f)
      for (std::uint64_t c = 0; c < source_rings_[f].size(); ++c)
        corners[source_rings_[f][c]].push_back({f, c});
    std::vector<bounded_boolean_error> link_findings;
    for (const auto &entry : corners) {
      const auto &incident = entry.second;
      std::map<std::uint64_t, std::set<std::uint64_t>> fan_graph;
      for (const auto &corner : incident)
        fan_graph[corner.first];
      for (const auto &edge : uses)
        if (edge.first.first == entry.first ||
            edge.first.second == entry.first) {
          auto f0 = edge.second[0].first, f1 = edge.second[1].first;
          fan_graph[f0].insert(f1);
          fan_graph[f1].insert(f0);
        }
      std::set<std::uint64_t> fan_visited;
      std::vector<std::uint64_t> fan_queue{fan_graph.begin()->first};
      while (!fan_queue.empty()) {
        auto facet = fan_queue.back();
        fan_queue.pop_back();
        if (!fan_visited.insert(facet).second)
          continue;
        for (auto next : fan_graph[facet])
          fan_queue.push_back(next);
      }
      if (fan_visited.size() != fan_graph.size())
        link_findings.push_back(*error(
            input_validation_subcode::vertex_link_multiple_cycles,
            bounded_boolean_error_category::input_contract_error,
            "source vertex has more than one incident fan", 6));
      std::map<std::pair<std::uint64_t, std::uint64_t>,
               std::pair<std::uint64_t, std::uint64_t>>
          successor;
      for (const auto &corner : incident) {
        auto paired = reciprocal.find(corner);
        if (paired == reciprocal.end())
          return error(input_validation_subcode::vertex_link_open_chain,
                       bounded_boolean_error_category::input_contract_error,
                       "temporary vertex link is open", 6);
        auto adjacent = paired->second;
        adjacent.second =
            (adjacent.second + 1) % source_rings_[adjacent.first].size();
        if (source_rings_[adjacent.first][adjacent.second] != entry.first ||
            !successor.emplace(corner, adjacent).second)
          return error(input_validation_subcode::vertex_link_degree_mismatch,
                       bounded_boolean_error_category::input_contract_error,
                       "temporary vertex link successor mismatch", 6);
      }
      std::set<std::pair<std::uint64_t, std::uint64_t>> visited;
      auto current = *std::min_element(incident.begin(), incident.end());
      for (std::size_t i = 0; i < incident.size(); ++i) {
        if (!visited.insert(current).second)
          break;
        auto next = successor.find(current);
        if (next == successor.end())
          break;
        current = next->second;
      }
      if (visited.size() != incident.size() || current != *visited.begin())
        link_findings.push_back(*error(
            input_validation_subcode::vertex_link_multiple_cycles,
            bounded_boolean_error_category::input_contract_error,
            "source vertex has more than one incident fan", 6));
    }
    if (!link_findings.empty())
      return *std::min_element(link_findings.begin(), link_findings.end(),
                               [](const auto &a, const auto &b) {
                                 return a.subcode < b.subcode;
                               });
    return std::nullopt;
  }
  std::optional<bounded_boolean_error> canonicalize() {
    std::vector<std::array<std::uint64_t, 3>> labels(source_.vertex_count());
    for (std::uint64_t v = 0; v < labels.size(); ++v)
      labels[v] = bits(v);
    auto result = canonicalize_source_topology(source_.vertex_count(), labels,
                                               source_rings_);
    s2c_ = std::move(result.source_to_canonical_vertex);
    canonical_to_source_ = std::move(result.canonical_to_source_vertex);
    canonical_to_source_facet_ = std::move(result.canonical_to_source_facet);
    rings_ = std::move(result.rings);
    out_.statistics_.canonical_branches = result.branches;
    std::set<std::uint64_t> published_sources;
    for (std::uint64_t v = 0; v < canonical_to_source_.size(); ++v) {
      auto source_v = canonical_to_source_[v];
      if (source_v >= source_.vertex_count() ||
          !published_sources.insert(source_v).second || s2c_[source_v] != v)
        return error(input_validation_subcode::invalid_automorphism_class,
                     bounded_boolean_error_category::internal_invariant_error,
                     "source vertex maps to multiple canonical vertices", 8);
      out_.vertices_.push_back({v, bits(source_v), source_v});
      points_.push_back(source_point(source_v));
    }
    std::vector<std::uint64_t> source_to_canonical_facet(
        source_rings_.size(), std::numeric_limits<std::uint64_t>::max());
    for (std::uint64_t f = 0; f < canonical_to_source_facet_.size(); ++f) {
      auto source_f = canonical_to_source_facet_[f];
      if (source_f >= source_rings_.size() ||
          source_to_canonical_facet[source_f] !=
              std::numeric_limits<std::uint64_t>::max())
        return error(input_validation_subcode::invalid_automorphism_class,
                     bounded_boolean_error_category::internal_invariant_error,
                     "source facet correspondence is not bijective", 8);
      source_to_canonical_facet[source_f] = f;
    }
    for (auto &position : out_.normalization_) {
      position.canonical_facet = source_to_canonical_facet[position.source_facet];
      const auto source_corner = position.retained_corner;
      const auto source_vertex = source_rings_[position.source_facet][source_corner];
      const auto canonical_vertex = s2c_[source_vertex];
      const auto &ring = rings_[position.canonical_facet];
      auto found = std::find(ring.begin(), ring.end(), canonical_vertex);
      if (found == ring.end())
        return error(input_validation_subcode::invalid_automorphism_class,
                     bounded_boolean_error_category::internal_invariant_error,
                     "source position has no canonical corner", 8);
      position.retained_corner = static_cast<std::uint64_t>(found - ring.begin());
    }
    return std::nullopt;
  }
  std::optional<bounded_boolean_error> incidence_and_links() {
    std::map<std::pair<std::uint64_t, std::uint64_t>,
             std::vector<std::uint64_t>>
        groups;
    for (std::uint64_t f = 0; f < rings_.size(); ++f)
      for (std::uint64_t c = 0; c < rings_[f].size(); ++c) {
        auto a = rings_[f][c], b = rings_[f][(c + 1) % rings_[f].size()],
             u = out_.directed_uses_.size();
        out_.directed_uses_.push_back({a, b, f, c, 0, 0});
        groups[{std::min(a, b), std::max(a, b)}].push_back(u);
      }
    for (const auto &entry : groups) {
      auto id = out_.edges_.size(), a = entry.second[0], b = entry.second[1];
      out_.directed_uses_[a].reciprocal = b;
      out_.directed_uses_[b].reciprocal = a;
      out_.directed_uses_[a].undirected_edge = id;
      out_.directed_uses_[b].undirected_edge = id;
      out_.edges_.push_back(
          {id, entry.first.first, entry.first.second, {a, b}});
    }
    std::vector<std::vector<std::uint64_t>> incident(out_.vertices_.size());
    for (std::uint64_t f = 0; f < rings_.size(); ++f)
      for (auto v : rings_[f])
        incident[v].push_back(f);
    for (std::uint64_t v = 0; v < incident.size(); ++v) {
      std::map<std::uint64_t, std::uint64_t> next;
      for (auto f : incident[v]) {
        const auto &r = rings_[f];
        auto c = std::find(r.begin(), r.end(), v) - r.begin();
        auto u =
            std::find_if(out_.directed_uses_.begin(), out_.directed_uses_.end(),
                         [&](const auto &e) {
                           return e.facet == f &&
                                  e.corner == static_cast<std::uint64_t>(c);
                         }) -
            out_.directed_uses_.begin();
        next[f] = out_.directed_uses_[out_.directed_uses_[u].reciprocal].facet;
      }
      std::vector<std::uint64_t> cycle;
      auto current = *std::min_element(incident[v].begin(), incident[v].end());
      for (std::size_t i = 0; i < incident[v].size(); ++i) {
        if (std::find(cycle.begin(), cycle.end(), current) != cycle.end())
          break;
        cycle.push_back(current);
        current = next[current];
      }
      if (cycle.size() != incident[v].size() || current != cycle.front())
        return error(input_validation_subcode::vertex_link_multiple_cycles,
                     bounded_boolean_error_category::input_contract_error,
                     "vertex link is not one cycle", 6);
      out_.vertex_links_.push_back({v, cycle});
    }
    return std::nullopt;
  }

  static std::uint8_t projection_axis(const validation_point3<T> &a,
                                      const validation_point3<T> &b,
                                      const validation_point3<T> &c) {
    validation_triangle<T> triangle{a, b, c};
    return input_relation_detail::projection_axis(triangle);
  }
  std::optional<bounded_boolean_error> facets() {
    auto imported = import_source_bounded_values(precision_, source_);
    if (!imported.has_value())
      return *imported.error();
    out_.bounded_vertices_.reserve(out_.vertices_.size());
    for (const auto &vertex : out_.vertices_) {
      const auto &point =
          imported.value()->points[vertex.presentation_vertex].value;
      bounded_source_vertex_evidence<T> evidence;
      evidence.vertex = vertex.canonical_id;
      evidence.radial_error = point.coordinates.radial_error_upper;
      for (std::size_t axis = 0; axis < 3; ++axis) {
        evidence.lower[axis] =
            point.coordinates.components[axis].uncertainty_enclosure.lower();
        evidence.upper[axis] =
            point.coordinates.components[axis].uncertainty_enclosure.upper();
      }
      out_.bounded_vertices_.push_back(evidence);
    }
    out_.facets_.resize(rings_.size());
    for (std::uint64_t f = 0; f < rings_.size(); ++f) {
      const auto &r = rings_[f];
      std::array<std::uint64_t, 3> support{};
      bool found = false;
      for (std::size_t i = 0; i + 2 < r.size() && !found; ++i)
        for (std::size_t j = i + 1; j + 1 < r.size() && !found; ++j)
          for (std::size_t k = j + 1; k < r.size() && !found; ++k) {
            auto axis =
                projection_axis(points_[r[i]], points_[r[j]], points_[r[k]]);
            auto p = input_relation_detail::project(points_[r[i]], axis),
                 q = input_relation_detail::project(points_[r[j]], axis),
                 s = input_relation_detail::project(points_[r[k]], axis);
            const int orientation = input_relation_detail::orient2(p, q, s);
            if (orientation == -1 || orientation == 1) {
              support = {r[i], r[j], r[k]};
              found = true;
            }
          }
      if (!found)
        return error(
            input_validation_subcode::support_plane_unavailable,
            bounded_boolean_error_category::input_geometry_not_epsilon_valid,
            "facet support plane unavailable", 11);
      for (auto v : r) {
        const int planarity = input_relation_detail::orient3(
            points_[support[0]], points_[support[1]], points_[support[2]],
            points_[v]);
        if (planarity == 2)
          return error(
              input_validation_subcode::uncertainty_incompatible,
              bounded_boolean_error_category::input_geometry_not_epsilon_valid,
              "facet planarity predicate is unavailable", 11);
        if (planarity != 0)
          return error(
              input_validation_subcode::planarity_exceeded,
              bounded_boolean_error_category::input_geometry_not_epsilon_valid,
              "nominal-only V1 requires exact planarity", 11);
      }
      auto axis = projection_axis(points_[support[0]], points_[support[1]],
                                  points_[support[2]]);
      std::vector<std::array<T, 2>> projected;
      for (auto v : r)
        projected.push_back(input_relation_detail::project(points_[v], axis));
      for (std::size_t i = 0; i < r.size(); ++i)
        for (std::size_t j = i + 1; j < r.size(); ++j) {
          const bool adjacent = j == i + 1 || (i == 0 && j + 1 == r.size());
          auto relation = input_relation_detail::segment_relation_2d(
              projected[i], projected[(i + 1) % r.size()], projected[j],
              projected[(j + 1) % r.size()]);
          if ((adjacent && relation != 1) || (!adjacent && relation != 0))
            return error(input_validation_subcode::projected_crossing,
                         bounded_boolean_error_category::
                             input_geometry_not_epsilon_valid,
                         "facet ring is not simple", 13);
        }
      long double area = 0;
      for (std::size_t i = 0; i < r.size(); ++i)
        area += static_cast<long double>(projected[i][0]) *
                    projected[(i + 1) % r.size()][1] -
                static_cast<long double>(projected[(i + 1) % r.size()][0]) *
                    projected[i][1];
      const int exact_area_sign = exact_sign(exact_polygon_area_2d(projected));
      if (exact_area_sign == 2)
        return error(
            input_validation_subcode::uncertainty_incompatible,
            bounded_boolean_error_category::input_geometry_not_epsilon_valid,
            "facet orientation predicate is unavailable", 13);
      if (exact_area_sign == 0)
        return error(
            input_validation_subcode::facet_orientation_zero,
            bounded_boolean_error_category::input_geometry_not_epsilon_valid,
            "facet has zero area", 13);
      long double area_uncertainty = 0;
      for (std::size_t i = 0; i < r.size(); ++i) {
        auto j = (i + 1) % r.size();
        const long double ri = out_.bounded_vertices_[r[i]].radial_error;
        const long double rj = out_.bounded_vertices_[r[j]].radial_error;
        const long double xi = projected[i][0] - projected[0][0];
        const long double yi = projected[i][1] - projected[0][1];
        const long double xj = projected[j][0] - projected[0][0];
        const long double yj = projected[j][1] - projected[0][1];
        area_uncertainty += std::fabs(xi) * rj + std::fabs(yi) * rj +
                            std::fabs(xj) * ri + std::fabs(yj) * ri +
                            2 * ri * rj;
      }
      T maximum_radius = T(0);
      for (auto v : r)
        maximum_radius = std::max(maximum_radius,
                                  out_.bounded_vertices_[v].radial_error);
      const int bounded_area_sign = input_relation_detail::bounded_polygon_area_sign(
          projected, maximum_radius);
      if (bounded_area_sign == 0 || bounded_area_sign == 2 ||
          bounded_area_sign != exact_area_sign)
        return error(
            input_validation_subcode::uncertainty_incompatible,
            bounded_boolean_error_category::input_geometry_not_epsilon_valid,
            "facet can collapse or invert within input uncertainty", 13);
      auto &facet = out_.facets_[f];
      facet.canonical_id = f;
      facet.presentation_facet = canonical_to_source_facet_[f];
      facet.vertices = r;
      facet.support_vertices = support;
      facet.dropped_axis = axis;
      facet.projected_area = area / 2;
      auto a = points_[support[0]], b = points_[support[1]],
           c = points_[support[2]];
      long double ux = b[0] - a[0], uy = b[1] - a[1], uz = b[2] - a[2],
                  vx = c[0] - a[0], vy = c[1] - a[1], vz = c[2] - a[2],
                  nx = uy * vz - uz * vy, ny = uz * vx - ux * vz,
                  nz = ux * vy - uy * vx;
      facet.support_plane = {nx, ny, nz, -(nx * a[0] + ny * a[1] + nz * a[2])};
      std::vector<std::size_t> polygon(r.size());
      std::iota(polygon.begin(), polygon.end(), 0);
      int sign = exact_area_sign;
      while (polygon.size() > 3) {
        bool ear = false;
        for (std::size_t k = 0; k < polygon.size(); ++k) {
          auto i = polygon[(k + polygon.size() - 1) % polygon.size()],
               j = polygon[k], l = polygon[(k + 1) % polygon.size()];
          if (input_relation_detail::orient2(projected[i], projected[j],
                                             projected[l]) *
                  sign <=
              0)
            continue;
          bool contains = false;
          for (auto x : polygon)
            if (x != i && x != j && x != l &&
                input_relation_detail::point_in_triangle_2d(
                    projected[x], projected[i], projected[j], projected[l])) {
              contains = true;
              break;
            }
          if (contains)
            continue;
          facet.decomposition.push_back({r[i], r[j], r[l]});
          polygon.erase(polygon.begin() + k);
          ear = true;
          break;
        }
        if (!ear)
          return error(
              input_validation_subcode::projected_crossing,
              bounded_boolean_error_category::input_geometry_not_epsilon_valid,
              "facet decomposition failed", 13);
      }
      facet.decomposition.push_back(
          {r[polygon[0]], r[polygon[1]], r[polygon[2]]});
      if (facet.decomposition.size() != r.size() - 2)
        return error(
            input_validation_subcode::projected_crossing,
            bounded_boolean_error_category::input_geometry_not_epsilon_valid,
            "facet decomposition count mismatch", 13);
      std::map<std::pair<std::uint64_t, std::uint64_t>, std::uint64_t>
          decomposition_edges;
      long double triangle_area = 0;
      for (const auto &triangle : facet.decomposition) {
        auto pa = input_relation_detail::project(points_[triangle[0]], axis);
        auto pb = input_relation_detail::project(points_[triangle[1]], axis);
        auto pc = input_relation_detail::project(points_[triangle[2]], axis);
        const int orientation = input_relation_detail::orient2(pa, pb, pc);
        const std::vector<std::array<T, 2>> bounded_triangle{pa, pb, pc};
        if (orientation != sign ||
            input_relation_detail::bounded_polygon_area_sign(
                bounded_triangle, maximum_radius) != sign)
          return error(
              input_validation_subcode::projected_crossing,
              bounded_boolean_error_category::input_geometry_not_epsilon_valid,
              "facet decomposition orientation mismatch", 13);
        triangle_area += static_cast<long double>(orientation * 2);
        for (std::size_t edge = 0; edge < 3; ++edge) {
          auto u = triangle[edge], v = triangle[(edge + 1) % 3];
          ++decomposition_edges[{std::min(u, v), std::max(u, v)}];
        }
      }
      std::set<std::pair<std::uint64_t, std::uint64_t>> boundary;
      for (std::size_t i = 0; i < r.size(); ++i)
        boundary.insert({std::min(r[i], r[(i + 1) % r.size()]),
                         std::max(r[i], r[(i + 1) % r.size()])});
      for (const auto &edge : decomposition_edges)
        if (edge.second != (boundary.count(edge.first) ? 1U : 2U))
          return error(
              input_validation_subcode::projected_crossing,
              bounded_boolean_error_category::input_geometry_not_epsilon_valid,
              "facet decomposition boundary mismatch", 13);
      (void)triangle_area;
    }
    for (const auto &e : out_.edges_) {
      bool separated = false;
      for (std::size_t axis = 0; axis < 3; ++axis)
        separated |= out_.bounded_vertices_[e.low].upper[axis] <
                         out_.bounded_vertices_[e.high].lower[axis] ||
                     out_.bounded_vertices_[e.high].upper[axis] <
                         out_.bounded_vertices_[e.low].lower[axis];
      if (points_[e.low] == points_[e.high] || !separated)
        return error(
            input_validation_subcode::collapsed_geometry,
            bounded_boolean_error_category::input_geometry_not_epsilon_valid,
            "source edge can collapse within input uncertainty", 17);
    }
    return std::nullopt;
  }

  std::optional<bounded_boolean_error> discover_shells() {
    std::vector<std::vector<std::uint64_t>> adj(rings_.size());
    for (const auto &e : out_.edges_) {
      auto a = out_.directed_uses_[e.uses[0]].facet,
           b = out_.directed_uses_[e.uses[1]].facet;
      adj[a].push_back(b);
      adj[b].push_back(a);
    }
    std::vector<std::int64_t> seen(rings_.size(), -1);
    for (std::uint64_t seed = 0; seed < rings_.size(); ++seed)
      if (seen[seed] < 0) {
        auto id = out_.shells_.size();
        validated_shell_record shell;
        shell.canonical_id = id;
        std::queue<std::uint64_t> q;
        q.push(seed);
        seen[seed] = id;
        while (!q.empty()) {
          auto f = q.front();
          q.pop();
          shell.facets.push_back(f);
          out_.facets_[f].shell = id;
          for (auto n : adj[f])
            if (seen[n] < 0) {
              seen[n] = id;
              q.push(n);
            }
        }
        std::sort(shell.facets.begin(), shell.facets.end());
        out_.shells_.push_back(std::move(shell));
      }
    for (auto &shell : out_.shells_) {
      const auto reference_id =
          out_.facets_[shell.facets.front()].vertices.front();
      const auto reference = points_[reference_id];
      const long double reference_error =
          out_.bounded_vertices_[reference_id].radial_error;
      auto bounded_volume = finite_interval<T>::checked_singleton(T(0));
      if (!bounded_volume)
        return error(input_validation_subcode::support_plane_unavailable,
                     bounded_boolean_error_category::input_geometry_not_epsilon_valid,
                     "shell volume interval is unavailable", 14);
      for (auto f : shell.facets)
        for (std::uint64_t local = 0;
             local < out_.facets_[f].decomposition.size(); ++local) {
          auto ids = out_.facets_[f].decomposition[local];
          validation_triangle<T> triangle{points_[ids[0]],
                                          points_[ids[1]],
                                          points_[ids[2]],
                                          f,
                                          local,
                                          shell.canonical_id,
                                          ids};
          triangles_.push_back(triangle);
          std::array<long double, 3> a{}, b{}, c{};
          long double magnitude = 0;
          for (std::size_t axis = 0; axis < 3; ++axis) {
            a[axis] =
                static_cast<long double>(triangle.a[axis]) - reference[axis];
            b[axis] =
                static_cast<long double>(triangle.b[axis]) - reference[axis];
            c[axis] =
                static_cast<long double>(triangle.c[axis]) - reference[axis];
            magnitude = std::max({magnitude, std::fabs(a[axis]),
                                  std::fabs(b[axis]), std::fabs(c[axis])});
          }
          shell.signed_volume += (a[0] * (b[1] * c[2] - b[2] * c[1]) -
                                  a[1] * (b[0] * c[2] - b[2] * c[0]) +
                                  a[2] * (b[0] * c[1] - b[1] * c[0])) /
                                 6;
          const long double radius =
              reference_error +
              std::max({static_cast<long double>(
                            out_.bounded_vertices_[ids[0]].radial_error),
                        static_cast<long double>(
                            out_.bounded_vertices_[ids[1]].radial_error),
                        static_cast<long double>(
                            out_.bounded_vertices_[ids[2]].radial_error)});
          shell.volume_uncertainty += 3 * magnitude * magnitude * radius +
                                      3 * magnitude * radius * radius +
                                       radius * radius * radius;
          const T bounded_radius = static_cast<T>(radius);
          const auto determinant = input_relation_detail::bounded_orient3_interval(
              triangle.a, triangle.b, triangle.c, reference, bounded_radius);
          if (!determinant)
            return error(input_validation_subcode::support_plane_unavailable,
                         bounded_boolean_error_category::input_geometry_not_epsilon_valid,
                         "shell volume interval is unavailable", 14);
          const auto sum = interval_add(*bounded_volume, *determinant);
          if (!sum)
            return error(input_validation_subcode::support_plane_unavailable,
                         bounded_boolean_error_category::input_geometry_not_epsilon_valid,
                         "shell volume interval is unavailable", 14);
          bounded_volume = sum.value;
        }
      if (bounded_volume->contains_zero())
        return error(
            input_validation_subcode::support_plane_unavailable,
            bounded_boolean_error_category::input_geometry_not_epsilon_valid,
            "shell orientation is unresolved within input uncertainty", 14);
      shell.intrinsic_orientation = bounded_volume->lower() > T(0)
                                         ? shell_orientation::outward
                                         : shell_orientation::inward;
    }
    return std::nullopt;
  }

  static std::uint8_t shared_dimension(const validation_triangle<T> &a,
                                       const validation_triangle<T> &b) {
    std::uint8_t count = 0;
    for (auto x : a.vertices)
      for (auto y : b.vertices)
        if (x == y)
          ++count;
    return count >= 2 ? 2 : count == 1 ? 1 : 0;
  }
  std::vector<std::uint64_t>
  shared_vertices(const validation_triangle<T> &a,
                  const validation_triangle<T> &b) const {
    std::vector<std::uint64_t> shared;
    for (auto x : a.vertices)
      for (auto y : b.vertices)
        if (x == y &&
            std::find(shared.begin(), shared.end(), x) == shared.end())
          shared.push_back(x);
    std::sort(shared.begin(), shared.end());
    return shared;
  }
  std::uint8_t facet_shared_dimension(std::uint64_t a, std::uint64_t b) const {
    std::vector<std::uint64_t> shared;
    for (auto x : out_.facets_[a].vertices)
      for (auto y : out_.facets_[b].vertices)
        if (x == y &&
            std::find(shared.begin(), shared.end(), x) == shared.end())
          shared.push_back(x);
    if (shared.size() < 2)
      return shared.empty() ? 0 : 1;
    std::sort(shared.begin(), shared.end());
    return std::any_of(out_.edges_.begin(), out_.edges_.end(),
                       [&](const auto &e) {
                         return e.low == shared[0] && e.high == shared[1];
                       })
               ? 2
               : 1;
  }
  bool confined_to_shared_vertex(const validation_triangle<T> &a,
                                 const validation_triangle<T> &b) const {
    auto shared = shared_vertices(a, b);
    if (shared.size() != 1)
      return false;
    const auto&point=points_[shared[0]];auto inside=[](const auto&p,const auto&t){auto axis=input_relation_detail::projection_axis(t);return input_relation_detail::point_in_triangle_2d(input_relation_detail::project(p,axis),input_relation_detail::project(t.a,axis),input_relation_detail::project(t.b,axis),input_relation_detail::project(t.c,axis));};return inside(point,a)&&inside(point,b);
  }
  bool confined_to_shared_source_edge(const validation_triangle<T> &a,
                                      const validation_triangle<T> &b) const {
    auto shared = shared_vertices(a, b);
    if (shared.size() != 2)
      return false;
    auto found = std::find_if(
        out_.edges_.begin(), out_.edges_.end(), [&](const auto &e) {
          return e.low == shared[0] && e.high == shared[1];
        });
    if (found == out_.edges_.end())
      return false;
    auto f0 = out_.directed_uses_[found->uses[0]].facet,
         f1 = out_.directed_uses_[found->uses[1]].facet;
    return (f0 == a.facet && f1 == b.facet) || (f0 == b.facet && f1 == a.facet);
  }
  static bool opposite_normals(const validation_triangle<T> &a,
                               const validation_triangle<T> &b) {
    const auto axis = input_relation_detail::projection_axis(a);
    if (axis > 2)
      return false;
    const int first = input_relation_detail::orient2(
        input_relation_detail::project(a.a, axis),
        input_relation_detail::project(a.b, axis),
        input_relation_detail::project(a.c, axis));
    const int second = input_relation_detail::orient2(
        input_relation_detail::project(b.a, axis),
        input_relation_detail::project(b.b, axis),
        input_relation_detail::project(b.c, axis));
    return (first == -1 || first == 1) && second == -first;
  }
  std::optional<bounded_boolean_error> interactions() {
    const T uncertainty = static_cast<T>(
        source_.operand() == operand_id::a
            ? precision_.effective_input_precision_a()
            : precision_.effective_input_precision_b());
    const auto inflation = directed_multiply(uncertainty, T(2));
    if (!inflation)
      return error(input_validation_subcode::uncertainty_incompatible,
                   bounded_boolean_error_category::input_geometry_not_epsilon_valid,
                   "triangle uncertainty inflation is unavailable", 17);
    const T relation_radius = inflation.value.upper;
    std::vector<validation_aabb<T>> nominal, inflated;
    for (const auto &t : triangles_) {
      nominal.push_back(triangle_bounds(t, T(0)));
      inflated.push_back(triangle_bounds(t, relation_radius));
    }
    std::vector<std::size_t> order(triangles_.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](auto a, auto b) {
      return inflated[a].low[0] != inflated[b].low[0]
                 ? inflated[a].low[0] < inflated[b].low[0]
                 : a < b;
    });
    for (std::size_t oi = 0; oi < order.size(); ++oi) {
      if (cancelled())
        return error(input_validation_subcode::cancelled,
                     bounded_boolean_error_category::cancelled,
                     "input validation cancelled", 17);
      auto i = order[oi];
      for (std::size_t oj = oi + 1;
           oj < order.size() &&
           inflated[order[oj]].low[0] <= inflated[i].high[0];
           ++oj) {
        auto j = order[oj];
        if (triangles_[i].facet == triangles_[j].facet ||
            !bounds_overlap(inflated[i], inflated[j]))
          continue;
        ++out_.statistics_.triangle_pairs;
        ++out_.statistics_.relation_calls;
        auto relation =
            classify_triangle_relation(triangles_[i], triangles_[j]);
        auto shared = shared_dimension(triangles_[i], triangles_[j]);
        bool uncertainty_separated =
            relation == validation_relation::definitely_disjoint &&
            definitely_separated_under_uncertainty(triangles_[i], triangles_[j],
                                                     relation_radius);
        out_.relations_.push_back({triangles_[i].facet, triangles_[i].local,
                                   triangles_[j].facet, triangles_[j].local,
                                   relation, shared, uncertainty_separated});
        bool same_shell = triangles_[i].shell == triangles_[j].shell;
        if (relation == validation_relation::uncertain)
          return error(
              input_validation_subcode::uncertainty_incompatible,
              bounded_boolean_error_category::input_geometry_not_epsilon_valid,
              "triangle relation is unresolved", 17);
        if (same_shell) {
          if (relation == validation_relation::definitely_disjoint) {
            if (facet_shared_dimension(triangles_[i].facet,
                                       triangles_[j].facet) == 0 &&
                !definitely_separated_under_uncertainty(
                    triangles_[i], triangles_[j], relation_radius))
              return error(input_validation_subcode::uncertainty_incompatible,
                           bounded_boolean_error_category::
                               input_geometry_not_epsilon_valid,
                           "uncertainty spans a non-adjacent interaction", 17);
            continue;
          }
          if ((relation == validation_relation::point_contact &&
               confined_to_shared_vertex(triangles_[i], triangles_[j])) ||
              (relation == validation_relation::edge_contact &&
               confined_to_shared_source_edge(triangles_[i], triangles_[j])))
            continue;
          auto failure = error(
              shared ? input_validation_subcode::adjacency_exceeded
                     : input_validation_subcode::forbidden_interaction,
              bounded_boolean_error_category::input_geometry_not_epsilon_valid,
              "source facets interact outside their shared feature", 17);
          failure->witnesses = {triangles_[i].facet, triangles_[i].local,
                                triangles_[j].facet, triangles_[j].local};
          failure->witness_count = 4;
          return failure;
        } else if (relation == validation_relation::whole_patch_coincidence)
          return error(
              input_validation_subcode::shell_coplanar_overlap,
              bounded_boolean_error_category::ambiguous_shell_semantics,
              "whole-patch shell coincidence is unsupported", 15);
        else if (relation == validation_relation::transverse_intersection)
          return error(
              input_validation_subcode::shell_transverse_intersection,
              bounded_boolean_error_category::input_geometry_not_epsilon_valid,
              "shell boundaries intersect transversely", 15);
        else if ((relation ==
                      validation_relation::coplanar_positive_area_overlap ||
                  relation == validation_relation::whole_patch_coincidence) &&
                 !opposite_normals(triangles_[i], triangles_[j]))
          return error(
              input_validation_subcode::shell_coplanar_overlap,
              bounded_boolean_error_category::ambiguous_shell_semantics,
              "same-facing shell boundaries overlap", 15);
        else if (relation == validation_relation::definitely_disjoint &&
                  !definitely_separated_under_uncertainty(
                      triangles_[i], triangles_[j], relation_radius))
          unresolved_shell_pairs_.insert(
              {std::min(triangles_[i].shell, triangles_[j].shell),
               std::max(triangles_[i].shell, triangles_[j].shell)});
      }
    }
    return std::nullopt;
  }
  std::optional<bounded_boolean_error> local_compatibility() {
    for (const auto &e : out_.edges_) {
      auto f0 = out_.directed_uses_[e.uses[0]].facet,
           f1 = out_.directed_uses_[e.uses[1]].facet;
      bool certified = false;
      for (const auto &r : out_.relations_) {
        if (!((r.facet_a == f0 && r.facet_b == f1) ||
              (r.facet_a == f1 && r.facet_b == f0)) ||
            r.relation != validation_relation::edge_contact)
          continue;
        auto a = std::find_if(
            triangles_.begin(), triangles_.end(), [&](const auto &t) {
              return t.facet == r.facet_a && t.local == r.triangle_a;
            });
        auto b = std::find_if(
            triangles_.begin(), triangles_.end(), [&](const auto &t) {
              return t.facet == r.facet_b && t.local == r.triangle_b;
            });
        if (a == triangles_.end() || b == triangles_.end())
          return error(input_validation_subcode::verifier_rejection,
                       bounded_boolean_error_category::internal_invariant_error,
                       "local relation references a missing triangle", 17);
        certified |= confined_to_shared_source_edge(*a, *b);
      }
      if (!certified)
        return error(
            input_validation_subcode::adjacency_exceeded,
            bounded_boolean_error_category::input_geometry_not_epsilon_valid,
            "source edge wedge lacks bounded shared-edge contact evidence", 17);
      out_.edge_wedges_.push_back({e.canonical_id, {f0, f1}, true});
    }
    for (const auto &link : out_.vertex_links_) {
      bool certified = !link.cyclic_facets.empty();
      for (std::size_t i = 0; i < link.cyclic_facets.size(); ++i) {
        auto a = link.cyclic_facets[i],
             b = link.cyclic_facets[(i + 1) % link.cyclic_facets.size()];
        certified &=
            std::any_of(out_.edge_wedges_.begin(), out_.edge_wedges_.end(),
                        [&](const auto &w) {
                          return (w.facets[0] == a && w.facets[1] == b) ||
                                 (w.facets[0] == b && w.facets[1] == a);
                        });
      }
      if (!certified)
        return error(
            input_validation_subcode::adjacency_exceeded,
            bounded_boolean_error_category::input_geometry_not_epsilon_valid,
            "vertex star lacks a closed bounded wedge cycle", 17);
      out_.vertex_stars_.push_back(
          {link.vertex, link.cyclic_facets.size(), true});
    }
    return std::nullopt;
  }

  std::vector<validation_triangle<T>>
  shell_triangles(std::uint64_t shell) const {
    std::vector<validation_triangle<T>> out;
    for (const auto &t : triangles_)
      if (t.shell == shell)
        out.push_back(t);
    return out;
  }
  point_shell_result classify_shell_vertex(std::uint64_t source_shell,
                                            std::uint64_t target_shell) const {
    auto target = shell_triangles(target_shell);
    std::set<std::uint64_t> vertices;
    for (auto f : out_.shells_[source_shell].facets)
      vertices.insert(out_.facets_[f].vertices.begin(),
                      out_.facets_[f].vertices.end());
    bool inside = false, outside = false;
    for (auto v : vertices) {
      const T uncertainty = static_cast<T>(
          source_.operand() == operand_id::a
              ? precision_.effective_input_precision_a()
              : precision_.effective_input_precision_b());
      auto result = classify_point_against_triangles(points_[v], target,
                                                     uncertainty);
      if (result == point_shell_result::ambiguous)
        return result;
      if (result == point_shell_result::inside)
        inside = true;
      if (result == point_shell_result::outside)
        outside = true;
      if (inside && outside)
        return point_shell_result::ambiguous;
    }
    return inside ? point_shell_result::inside
                  : outside ? point_shell_result::outside
                            : point_shell_result::boundary;
  }
  std::optional<bounded_boolean_error> shell_semantics() {
    for (std::uint64_t a = 0; a < out_.shells_.size(); ++a)
      for (std::uint64_t b = a + 1; b < out_.shells_.size(); ++b) {
        bool point = false, edge = false, face = false;
        std::uint64_t contact_fa = 0, contact_fb = 0;
        for (const auto &r : out_.relations_) {
          auto sa = out_.facets_[r.facet_a].shell,
               sb = out_.facets_[r.facet_b].shell;
          if (!((sa == a && sb == b) || (sa == b && sb == a)))
            continue;
          point |= r.relation == validation_relation::point_contact;
          edge |= r.relation == validation_relation::edge_contact;
          if (r.relation ==
              validation_relation::coplanar_positive_area_overlap) {
            face = true;
            contact_fa = r.facet_a;
            contact_fb = r.facet_b;
          }
        }
        auto ainb = classify_shell_vertex(a, b),
             bina = classify_shell_vertex(b, a);
        if (ainb == point_shell_result::ambiguous ||
            bina == point_shell_result::ambiguous)
          return error(
              input_validation_subcode::shell_contact_ambiguous,
              bounded_boolean_error_category::ambiguous_shell_semantics,
              "shell containment is ambiguous", 16);
        shell_pair_relation relation;
        const bool contact = point || edge || face;
        if (!contact && unresolved_shell_pairs_.count({a, b}))
          return error(
              input_validation_subcode::uncertainty_incompatible,
              bounded_boolean_error_category::input_geometry_not_epsilon_valid,
              "shell relation is unresolved within input uncertainty", 16);
        if (contact && (ainb == point_shell_result::inside ||
                        bina == point_shell_result::inside)) {
          auto failure = error(
              input_validation_subcode::shell_contact_ambiguous,
              bounded_boolean_error_category::ambiguous_shell_semantics,
              face
                  ? "face-contacting shells cannot establish containment"
                  : edge ? "edge-contacting shells cannot establish containment"
                         : "point-contacting shells cannot establish "
                           "containment",
              16);
          failure->witnesses[0] = a;
          failure->witnesses[1] = b;
          failure->witnesses[2] = contact_fa;
          failure->witnesses[3] = contact_fb;
          failure->witness_count = 4;
          return failure;
        }
        if (ainb == point_shell_result::inside &&
            bina == point_shell_result::inside)
          return error(
              input_validation_subcode::shell_positive_volume_overlap,
              bounded_boolean_error_category::ambiguous_shell_semantics,
              "shells have positive-volume overlap", 16);
        else if (ainb == point_shell_result::inside)
          relation = shell_pair_relation::a_inside_b;
        else if (bina == point_shell_result::inside)
          relation = shell_pair_relation::b_inside_a;
        else if (face)
          relation = shell_pair_relation::authorized_face_contact;
        else if (edge)
          relation = shell_pair_relation::authorized_edge_contact;
        else if (point)
          relation = shell_pair_relation::authorized_point_contact;
        else
          relation = shell_pair_relation::definitely_disjoint;
        out_.shell_pairs_.push_back({a, b, relation});
      }
    for (std::uint64_t child = 0; child < out_.shells_.size(); ++child) {
      std::vector<std::uint64_t> parents;
      for (const auto &p : out_.shell_pairs_) {
        if (p.relation == shell_pair_relation::a_inside_b && p.shell_a == child)
          parents.push_back(p.shell_b);
        if (p.relation == shell_pair_relation::b_inside_a && p.shell_b == child)
          parents.push_back(p.shell_a);
      }
      if (!parents.empty()) {
        auto is_inside = [&](std::uint64_t inner, std::uint64_t outer) {
          for (const auto &p : out_.shell_pairs_) {
            if (p.shell_a == inner && p.shell_b == outer &&
                p.relation == shell_pair_relation::a_inside_b)
              return true;
            if (p.shell_b == inner && p.shell_a == outer &&
                p.relation == shell_pair_relation::b_inside_a)
              return true;
          }
          return false;
        };
        for (std::size_t i = 0; i < parents.size(); ++i)
          for (std::size_t j = i + 1; j < parents.size(); ++j)
            if (!is_inside(parents[i], parents[j]) &&
                !is_inside(parents[j], parents[i]))
              return error(
                  input_validation_subcode::shell_parent_ambiguous,
                  bounded_boolean_error_category::ambiguous_shell_semantics,
                  "candidate parents do not form a strict containment chain",
                  16);
        auto immediate = parents.front();
        for (auto candidate : parents)
          if (is_inside(candidate, immediate))
            immediate = candidate;
        out_.shells_[child].parent = static_cast<std::int64_t>(immediate);
      }
    }
    for (std::uint64_t s = 0; s < out_.shells_.size(); ++s) {
      std::set<std::uint64_t> seen;
      auto current = s;
      std::uint32_t depth = 0;
      while (out_.shells_[current].parent >= 0) {
        if (!seen.insert(current).second)
          return error(
              input_validation_subcode::shell_parent_ambiguous,
              bounded_boolean_error_category::ambiguous_shell_semantics,
              "shell containment cycle", 16);
        current = static_cast<std::uint64_t>(out_.shells_[current].parent);
        ++depth;
      }
      out_.shells_[s].depth = depth;
      auto required =
          depth % 2 ? shell_orientation::inward : shell_orientation::outward;
      if (out_.shells_[s].intrinsic_orientation != required)
        return error(input_validation_subcode::shell_orientation_parity,
                     bounded_boolean_error_category::input_contract_error,
                     "shell orientation conflicts with nesting depth", 16);
    }
    return std::nullopt;
  }

  void encode() {
    out_.canonical_bytes_ = encode_validated_operand_semantic(out_);
    out_.digest_ = sha256::digest(out_.canonical_bytes_);
    out_.source_presentation_bytes_ =
        encode_validated_operand_source_presentation(out_);
    out_.source_presentation_digest_ =
        sha256::digest(out_.source_presentation_bytes_);
    std::uint64_t persistent = 0;
    if (actual_usage(resource_kind::persistent_bytes, persistent))
      out_.statistics_.persistent_bytes = persistent;
  }

  class publication final : public transaction_participant {
  public:
    publication(std::shared_ptr<const artifact> candidate,
                const immutable_source_mesh<T, I> &source,
                const precision_context<T> &precision)
        : candidate_(std::move(candidate)), source_(source),
          precision_(precision) {}
    bool prepare() override {
      return candidate_ &&
             verify_validated_operand(*candidate_, source_, precision_);
    }
    void commit() noexcept override { published_ = candidate_; }
    void rollback() noexcept override {
      published_.reset();
      candidate_.reset();
    }
    std::shared_ptr<const artifact> published_;

  private:
    std::shared_ptr<const artifact> candidate_;
    const immutable_source_mesh<T, I> &source_;
    const precision_context<T> &precision_;
  };

public:
  input_validation_builder(const immutable_source_mesh<T, I> &source,
                           const boolean_context<T, I> &context,
                           const precision_context<T> &precision,
                           input_validation_capabilities caps)
      : source_(source), context_(context), precision_(precision),
        caps_(std::move(caps)) {
    out_.operand_ = source.operand();
    out_.owner_ = context.owner;
    out_.source_digest_ = source.digest();
    out_.precision_digest_ = precision.digest();
    out_.context_digest_ = context.context_digest;
  }
  boolean_outcome<std::shared_ptr<const artifact>> run() {
    if (caps_.version != contract_versions::input_validation_provider ||
        caps_.reserved || !caps_.owner.same_owner(context_.owner) ||
        !caps_.resources || !precision_.owned_by(context_.owner) ||
        !context_.sources || !verify_source(source_) ||
        !precision_.ordinary_success_eligible())
      return failure(input_validation_subcode::wrong_capability,
                     bounded_boolean_error_category::internal_invariant_error,
                     "input validation capability mismatch", 1);
    if (cancelled())
      return failure(input_validation_subcode::cancelled,
                     bounded_boolean_error_category::cancelled,
                     "input validation cancelled", 1);
    if (!preflight())
      return failure(cancelled() ? input_validation_subcode::cancelled
                                 : input_validation_subcode::resource_preflight,
                     cancelled()
                         ? bounded_boolean_error_category::cancelled
                         : bounded_boolean_error_category::resource_limit,
                     "input validation preflight failed", 2);
    for (auto b : source_.coordinate_bits())
      if (!finite_bits(from_bits<T>(b)))
        return failure(input_validation_subcode::non_finite_coordinate,
                       bounded_boolean_error_category::input_contract_error,
                       "source coordinate is not finite", 3);
    if (auto e = normalize())
      return boolean_outcome<std::shared_ptr<const artifact>>::failure(*e);
    if (auto e = temporary_topology())
      return boolean_outcome<std::shared_ptr<const artifact>>::failure(*e);
    if (cancelled())
      return failure(input_validation_subcode::cancelled,
                     bounded_boolean_error_category::cancelled,
                     "input validation cancelled", 7);
    if (auto e = canonicalize())
      return boolean_outcome<std::shared_ptr<const artifact>>::failure(*e);
    if (auto e = incidence_and_links())
      return boolean_outcome<std::shared_ptr<const artifact>>::failure(*e);
    if (auto e = facets())
      return boolean_outcome<std::shared_ptr<const artifact>>::failure(*e);
    if (auto e = discover_shells())
      return boolean_outcome<std::shared_ptr<const artifact>>::failure(*e);
    if (auto e = interactions())
      return boolean_outcome<std::shared_ptr<const artifact>>::failure(*e);
    if (auto e = local_compatibility())
      return boolean_outcome<std::shared_ptr<const artifact>>::failure(*e);
    if (auto e = shell_semantics())
      return boolean_outcome<std::shared_ptr<const artifact>>::failure(*e);
    out_.certificate_ = input_certificate_disposition::nominal_embedded;
    encode();
    if (cancelled())
      return failure(input_validation_subcode::cancelled,
                     bounded_boolean_error_category::cancelled,
                     "input validation cancelled", 18);
    std::vector<std::uint64_t> usage;
    usage.reserve(reservations_.size());
    for (const auto &reservation : reservations_) {
      std::uint64_t actual = 0;
      if (!actual_usage(reservation.kind, actual) ||
          actual > reservation.reservation.amount())
        return failure(input_validation_subcode::resource_preflight,
                       bounded_boolean_error_category::resource_limit,
                       "resource finalization exceeded reservation", 20);
      usage.push_back(actual);
    }
    auto candidate = std::make_shared<const artifact>(std::move(out_));
    publication participant(candidate, source_, precision_);
    stage_transaction transaction;
    if (!transaction.open() || !transaction.enlist(participant) ||
        !transaction.begin_join() || !transaction.begin_verify() ||
        !transaction.ready())
      return failure(input_validation_subcode::verifier_rejection,
                     bounded_boolean_error_category::internal_invariant_error,
                     "validated operand verifier rejected proposal", 19);
    if (cancelled()) {
      transaction.rollback();
      return failure(input_validation_subcode::cancelled,
                     bounded_boolean_error_category::cancelled,
                     "input validation cancelled", 21);
    }
    for (std::size_t i = 0; i < reservations_.size(); ++i)
      if (!reservations_[i].reservation.commit(usage[i])) {
        transaction.rollback();
        return failure(input_validation_subcode::verifier_rejection,
                       bounded_boolean_error_category::internal_invariant_error,
                       "resource finalization failed", 20);
      }
    if (!transaction.commit() || !participant.published_)
      return failure(input_validation_subcode::verifier_rejection,
                     bounded_boolean_error_category::internal_invariant_error,
                     "validated operand publication failed", 21);
    return boolean_outcome<std::shared_ptr<const artifact>>::success(
        std::move(participant.published_));
  }
};

template <class T, class I>
boolean_outcome<std::shared_ptr<const validated_operand<T, I>>>
validate_operand(operand_id operand, const immutable_source_mesh<T, I> &source,
                 const boolean_context<T, I> &context,
                 const precision_context<T> &precision,
                 input_validation_capabilities caps) {
  if (source.operand() != operand)
    return boolean_outcome<std::shared_ptr<const validated_operand<T, I>>>::
        failure(input_validation_error(
            operand, input_validation_subcode::wrong_capability,
            bounded_boolean_error_category::internal_invariant_error,
            "operand/source mismatch", 1));
  return input_validation_builder<T, I>(source, context, precision,
                                        std::move(caps))
      .run();
}

} // namespace ygor::mesh_boolean::bounded
