#pragma once

#include "CandidateSourceEdgeRelations.h"
#include "CoplanarRelationOverlay.h"
#include "FacetFacetRelations.h"
#include "FloatingBits.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace ygor::mesh_boolean::bounded {
namespace relation_construction_policy_detail {

inline constexpr std::uint64_t construction_use_tag(
    std::uint8_t category) noexcept {
  return (std::uint64_t{10} << 56U) |
         (static_cast<std::uint64_t>(category) << 48U);
}

inline relation_request_key construction_key(
    const relation_request_key &source, std::uint8_t category,
    std::uint32_t occurrence, const relation_feature_key *first = nullptr,
    const relation_feature_key *second = nullptr) noexcept {
  relation_request_key out = source;
  out.family = relation_request_family::authoritative_construction;
  out.directed_use = construction_use_tag(category);
  out.occurrence_discriminator = occurrence;
  out.formula_version = contract_versions::exact_relation_formulas;
  out.policy_version =
      contract_versions::relation_construction_registry_policy;
  out.reserved = 0;
  if (first)
    out.first = *first;
  if (second)
    out.second = *second;
  return out;
}

template <class T> struct geometry_snapshot final {
  relation_construction_kind kind = relation_construction_kind::bounded_point;
  relation_construction_coordinate_space coordinate_space =
      relation_construction_coordinate_space::world_3d;
  std::uint8_t component_count = 0;
  std::uint8_t projection_axis = 3;
  std::array<T, 6> nominal{};
  std::array<T, 6> lower{};
  std::array<T, 6> upper{};
  std::uint64_t provenance = 0;
  std::uint64_t lineage = 0;
  bool accepted_source_vertex = false;
  bool finite = false;
  bool tolerance_compatible = false;
};

template <class T> struct authority final {
  relation_request_key key{};
  relation_request_key source_relation{};
  relation_construction_precedence precedence =
      relation_construction_precedence::verification_witness;
  relation_feature_key source_feature{};
  geometry_snapshot<T> geometry{};
  std::uint32_t source_occurrence = 0;
};

template <class T>
inline bool finite_ordered_component(T nominal, T lower, T upper) noexcept {
  return finite_bits(nominal) && finite_bits(lower) && finite_bits(upper) &&
         !finite_numeric_less(upper, lower) &&
         !finite_numeric_less(nominal, lower) &&
         !finite_numeric_less(upper, nominal);
}

template <class T>
inline bool valid_geometry(const geometry_snapshot<T> &geometry) noexcept {
  const bool valid_count =
      (geometry.kind == relation_construction_kind::bounded_point &&
       (geometry.component_count == 2 || geometry.component_count == 3)) ||
      (geometry.kind == relation_construction_kind::bounded_carrier &&
       geometry.component_count == 6);
  if (!valid_count || !geometry.finite || !geometry.tolerance_compatible)
    return false;
  if (geometry.coordinate_space ==
          relation_construction_coordinate_space::source_facet_projection) {
    if (geometry.component_count != 2 || geometry.projection_axis > 2)
      return false;
  } else if (geometry.coordinate_space ==
             relation_construction_coordinate_space::world_3d) {
    if (geometry.component_count != 3 && geometry.component_count != 6)
      return false;
    if (geometry.projection_axis != 3)
      return false;
  } else {
    return false;
  }
  for (std::size_t i = 0; i < geometry.component_count; ++i)
    if (!finite_ordered_component(geometry.nominal[i], geometry.lower[i],
                                  geometry.upper[i]))
      return false;
  return true;
}

template <class T>
inline geometry_snapshot<T> geometry_from_point(
    const source_edge_geometry_snapshot<T> &point,
    bool accepted_source_vertex, bool tolerance_compatible) noexcept {
  geometry_snapshot<T> out;
  out.kind = relation_construction_kind::bounded_point;
  out.coordinate_space = relation_construction_coordinate_space::world_3d;
  out.component_count = 3;
  out.projection_axis = 3;
  out.provenance = point.provenance;
  out.lineage = point.lineage;
  out.accepted_source_vertex = accepted_source_vertex;
  out.finite = true;
  out.tolerance_compatible = tolerance_compatible;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    out.nominal[axis] = point.rounded_nominal[axis];
    out.lower[axis] = point.enclosure[axis].lower();
    out.upper[axis] = point.enclosure[axis].upper();
  }
  return out;
}

template <class T>
inline geometry_snapshot<T> geometry_from_carrier(
    const source_facet_transverse_carrier<T> &carrier) noexcept {
  geometry_snapshot<T> out;
  out.kind = relation_construction_kind::bounded_carrier;
  out.coordinate_space = relation_construction_coordinate_space::world_3d;
  out.component_count = 6;
  out.projection_axis = 3;
  out.finite = true;
  out.tolerance_compatible = carrier.residuals_accepted;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    out.nominal[axis] = carrier.point.rounded[axis];
    out.lower[axis] = carrier.point.lower[axis];
    out.upper[axis] = carrier.point.upper[axis];
    out.nominal[axis + 3] = carrier.direction.rounded[axis];
    out.lower[axis + 3] = carrier.direction.lower[axis];
    out.upper[axis + 3] = carrier.direction.upper[axis];
  }
  return out;
}

template <class T>
inline geometry_snapshot<T> geometry_from_projected(
    const projected_source_point<T> &point, std::uint8_t dropped_axis,
    bool accepted_source_vertex = false) noexcept {
  geometry_snapshot<T> out;
  out.kind = relation_construction_kind::bounded_point;
  out.coordinate_space =
      relation_construction_coordinate_space::source_facet_projection;
  out.component_count = 2;
  out.projection_axis = dropped_axis;
  out.accepted_source_vertex = accepted_source_vertex;
  out.finite = true;
  out.tolerance_compatible = true;
  for (std::size_t axis = 0; axis < 2; ++axis) {
    out.nominal[axis] = point.nominal[axis];
    out.lower[axis] = point.enclosure[axis].lower();
    out.upper[axis] = point.enclosure[axis].upper();
  }
  return out;
}

template <class T>
inline bool same_geometry(const geometry_snapshot<T> &a,
                          const geometry_snapshot<T> &b) noexcept {
  if (a.kind != b.kind || a.coordinate_space != b.coordinate_space ||
      a.component_count != b.component_count ||
      a.projection_axis != b.projection_axis ||
      a.provenance != b.provenance || a.lineage != b.lineage ||
      a.accepted_source_vertex != b.accepted_source_vertex ||
      a.finite != b.finite ||
      a.tolerance_compatible != b.tolerance_compatible)
    return false;
  for (std::size_t i = 0; i < a.component_count; ++i)
    if (to_bits(a.nominal[i]) != to_bits(b.nominal[i]) ||
        to_bits(a.lower[i]) != to_bits(b.lower[i]) ||
        to_bits(a.upper[i]) != to_bits(b.upper[i]))
      return false;
  return true;
}

template <class T>
inline bool nominal_in_enclosure(T nominal, T lower, T upper) noexcept {
  return !finite_numeric_less(nominal, lower) &&
         !finite_numeric_less(upper, nominal);
}

template <class T>
inline bool compatible_geometry(const geometry_snapshot<T> &authority_geometry,
                                const geometry_snapshot<T> &witness) noexcept {
  if (!valid_geometry(authority_geometry) || !valid_geometry(witness) ||
      authority_geometry.kind != witness.kind)
    return false;
  if (authority_geometry.coordinate_space == witness.coordinate_space) {
    if (authority_geometry.component_count != witness.component_count ||
        authority_geometry.projection_axis != witness.projection_axis)
      return false;
    for (std::size_t i = 0; i < authority_geometry.component_count; ++i)
      if (!nominal_in_enclosure(authority_geometry.nominal[i], witness.lower[i],
                                witness.upper[i]))
        return false;
    return true;
  }
  if (authority_geometry.coordinate_space !=
          relation_construction_coordinate_space::world_3d ||
      witness.coordinate_space !=
          relation_construction_coordinate_space::source_facet_projection ||
      authority_geometry.component_count != 3 || witness.component_count != 2 ||
      witness.projection_axis > 2)
    return false;
  std::size_t projected = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    if (axis == witness.projection_axis)
      continue;
    if (!nominal_in_enclosure(authority_geometry.nominal[axis],
                              witness.lower[projected],
                              witness.upper[projected]))
      return false;
    ++projected;
  }
  return true;
}

inline relation_feature_key source_vertex_feature(operand_id operand,
                                                   std::uint64_t source_vertex) {
  relation_feature_key feature;
  feature.operand = operand;
  feature.kind = relation_feature_kind::source_vertex;
  feature.primary = source_vertex;
  return feature;
}

template <class T, class I>
inline bool source_vertex_geometry(
    const canonical_candidate_stream<T, I> &candidates, operand_id operand,
    std::uint64_t source_vertex, geometry_snapshot<T> &geometry) noexcept {
  const auto &manifolds = candidates.manifolds();
  if (!manifolds)
    return false;
  const auto selected = operand == operand_id::a ? manifolds->a() : manifolds->b();
  if (!selected || source_vertex >= selected->source_vertex_to_vertex().size())
    return false;
  const auto dense = selected->source_vertex_to_vertex()[source_vertex];
  if (dense >= selected->vertices().size())
    return false;
  const auto &vertex = selected->vertices()[dense];
  if (vertex.source_vertex != source_vertex)
    return false;
  geometry = geometry_snapshot<T>{};
  geometry.kind = relation_construction_kind::bounded_point;
  geometry.coordinate_space = relation_construction_coordinate_space::world_3d;
  geometry.component_count = 3;
  geometry.projection_axis = 3;
  geometry.provenance = source_vertex + 1;
  geometry.lineage = (static_cast<std::uint64_t>(operand) << 63U) |
                     (source_vertex + 1);
  geometry.accepted_source_vertex = true;
  geometry.finite = true;
  geometry.tolerance_compatible = true;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    geometry.nominal[axis] = vertex.committed_point[axis];
    geometry.lower[axis] = vertex.lower[axis];
    geometry.upper[axis] = vertex.upper[axis];
  }
  return valid_geometry(geometry);
}

template <class T, class I>
inline bool endpoint_vertex(
    const canonical_candidate_stream<T, I> &candidates,
    const relation_feature_key &edge, std::uint8_t mask,
    operand_id &operand, std::uint64_t &source_vertex) noexcept {
  if (!valid_relation_feature_key(edge) ||
      edge.kind != relation_feature_kind::source_edge || mask == 0 || mask > 2)
    return false;
  const auto *topology =
      candidate_source_edge_relation_detail::operand_topology(candidates,
                                                               edge.operand);
  if (!topology || !topology->owner().same_owner(candidates.owner()))
    return false;
  const auto &table = candidates.primitive_table(edge.operand);
  const broad_phase_edge_primitive<T> *primitive = nullptr;
  for (const auto &candidate : table.edges) {
    if (!candidate_source_edge_relation_detail::valid_original_edge_primitive(
            candidate) ||
        candidate_source_edge_relation_detail::source_edge_feature(candidate) !=
            edge)
      continue;
    if (primitive)
      return false;
    primitive = &candidate;
  }
  if (!primitive)
    return false;
  const auto endpoint = primitive->endpoints[mask - 1];
  if (endpoint.ordinal() >= topology->vertices().size())
    return false;
  const auto *vertex = topology->vertex(endpoint, candidates.owner());
  if (!vertex)
    return false;
  operand = edge.operand;
  source_vertex = vertex->source_vertex;
  return true;
}

template <class T, class I>
inline bool edge_point_authority(
    const canonical_candidate_stream<T, I> &candidates,
    const relation_request_key &source_key,
    const source_edge_relation_record<T> &source, std::uint32_t point_ordinal,
    authority<T> &out) noexcept {
  if (point_ordinal >= source.point_count || point_ordinal >= source.points.size())
    return false;
  const auto &point = source.points[point_ordinal];
  out = authority<T>{};
  out.source_relation = source_key;
  out.source_occurrence = point_ordinal;
  std::uint64_t vertex = 0;
  operand_id vertex_operand = operand_id::a;
  bool has_vertex = false;
  if (point.first_endpoint_owner_mask != 0) {
    if (!endpoint_vertex(candidates, source_key.first,
                         point.first_endpoint_owner_mask, vertex_operand,
                         vertex))
      return false;
    has_vertex = true;
  } else if (point.second_endpoint_owner_mask != 0) {
    if (!endpoint_vertex(candidates, source_key.second,
                         point.second_endpoint_owner_mask, vertex_operand,
                         vertex))
      return false;
    has_vertex = true;
  }
  if (point.accepted_source_vertex) {
    if (!has_vertex ||
        !source_vertex_geometry(candidates, vertex_operand, vertex, out.geometry))
      return false;
    out.precedence = relation_construction_precedence::accepted_source_vertex;
    out.source_feature = source_vertex_feature(vertex_operand, vertex);
    out.key = construction_key(source_key, 1, 0, &out.source_feature);
    relation_feature_key none{};
    none.operand = vertex_operand;
    out.key.second = none;
    out.key.scope = relation_record_scope::public_source_feature;
    return valid_relation_request_key(out.key);
  }
  out.precedence =
      relation_construction_precedence::source_edge_source_edge_point;
  out.source_feature = source_key.first;
  out.geometry = geometry_from_point(point.point, false,
                                     point.tolerance_compatible);
  out.key = construction_key(source_key, 2, point_ordinal);
  return valid_relation_request_key(out.key) && valid_geometry(out.geometry);
}

template <class T>
inline bool same_point_lineage(const source_edge_geometry_snapshot<T> &a,
                               const source_edge_geometry_snapshot<T> &b) noexcept {
  return a.provenance != 0 && a.lineage != 0 &&
         a.provenance == b.provenance && a.lineage == b.lineage;
}

template <class T, class I>
inline bool edge_relation_point_authority(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_relation_stage<T> &edge_stage,
    relation_request_id request, const source_edge_geometry_snapshot<T> &witness,
    authority<T> &out) noexcept {
  if (request.ordinal() >= edge_stage.relations.size() ||
      request.ordinal() >= edge_stage.request_graph.requests.size())
    return false;
  const auto &relation = edge_stage.relations[request.ordinal()];
  std::uint32_t match = std::numeric_limits<std::uint32_t>::max();
  for (std::uint32_t point = 0; point < relation.point_count; ++point) {
    if (!same_point_lineage(relation.points[point].point, witness))
      continue;
    if (match != std::numeric_limits<std::uint32_t>::max())
      return false;
    match = point;
  }
  if (match == std::numeric_limits<std::uint32_t>::max())
    return false;
  const auto &source_key =
      edge_stage.request_graph.requests[request.ordinal()].key;
  return edge_point_authority(candidates, source_key, relation, match, out);
}

template <class T, class I>
inline bool edge_facet_event_authority(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_relation_stage<T> &edge_stage,
    const relation_request_key &source_key,
    const source_edge_facet_relation_record<T> &source,
    const source_edge_facet_event_record<T> &event, std::uint32_t occurrence,
    authority<T> &out) noexcept {
  out = authority<T>{};
  std::uint64_t vertex = 0;
  operand_id vertex_operand = source_key.first.operand;
  bool has_vertex = false;
  if (event.construction.edge_endpoint_owner_mask != 0) {
    if (!endpoint_vertex(candidates, source_key.first,
                         event.construction.edge_endpoint_owner_mask,
                         vertex_operand, vertex))
      return false;
    has_vertex = true;
  }
  if (!has_vertex && event.region.classification ==
                         source_facet_point_region_class::original_vertex) {
    if (event.region.source_vertex_owners.size() != 1)
      return false;
    vertex = event.region.source_vertex_owners.front();
    vertex_operand = source_key.second.operand;
    has_vertex = true;
  }
  if (event.construction.accepted_source_vertex || has_vertex) {
    if (!has_vertex ||
        !source_vertex_geometry(candidates, vertex_operand, vertex, out.geometry))
      return false;
    out.key = construction_key(source_key, 1, 0);
    out.source_feature = source_vertex_feature(vertex_operand, vertex);
    out.key.first = out.source_feature;
    out.key.second = relation_feature_key{};
    out.key.second.operand = vertex_operand;
    out.key.scope = relation_record_scope::public_source_feature;
    out.source_relation = source_key;
    out.precedence = relation_construction_precedence::accepted_source_vertex;
    return valid_relation_request_key(out.key);
  }
  if (event.region.classification ==
      source_facet_point_region_class::original_edge) {
    for (const auto request : source.boundary_relation_requests)
      if (edge_relation_point_authority(candidates, edge_stage, request,
                                        event.construction.point, out))
        return true;
    return false;
  }
  out.key = construction_key(source_key, 3, occurrence);
  out.source_relation = source_key;
  out.precedence =
      relation_construction_precedence::source_edge_source_facet_point;
  out.source_feature = source_key.first;
  out.geometry = geometry_from_point(event.construction.point, false,
                                     event.construction.tolerance_compatible);
  out.source_occurrence = occurrence;
  return valid_relation_request_key(out.key) && valid_geometry(out.geometry);
}

template <class T>
inline bool carrier_authority(
    const relation_request_key &source_key,
    const source_facet_transverse_carrier<T> &carrier,
    authority<T> &out) noexcept {
  out = authority<T>{};
  out.key = construction_key(source_key, 5, 0);
  out.source_relation = source_key;
  out.precedence =
      relation_construction_precedence::source_facet_source_facet_carrier;
  out.source_feature = source_key.first;
  out.geometry = geometry_from_carrier(carrier);
  return valid_relation_request_key(out.key) && valid_geometry(out.geometry);
}

template <class T, class I>
inline bool overlay_node_authority(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_relation_stage<T> &edge_stage,
    const relation_request_key &source_key,
    const source_facet_coplanar_overlay_record<T> &source,
    const coplanar_overlap_event_node<T> &node, authority<T> &out) noexcept {
  bool has_vertex = false;
  bool ambiguous_vertex = false;
  operand_id vertex_operand = operand_id::a;
  std::uint64_t vertex = 0;
  for (const auto &occurrence : node.occurrences) {
    if (!occurrence.query_source_vertex_valid)
      continue;
    if (occurrence.polygon > 1)
      return false;
    const auto operand = source.facets[occurrence.polygon].feature.operand;
    if (has_vertex && (operand != vertex_operand ||
                       occurrence.query_source_vertex != vertex)) {
      ambiguous_vertex = true;
      continue;
    }
    has_vertex = true;
    vertex_operand = operand;
    vertex = occurrence.query_source_vertex;
  }
  if (ambiguous_vertex)
    has_vertex = false;
  if (has_vertex) {
    out = authority<T>{};
    out.source_feature = source_vertex_feature(vertex_operand, vertex);
    out.key = construction_key(source_key, 1, 0, &out.source_feature);
    out.key.second = relation_feature_key{};
    out.key.second.operand = vertex_operand;
    out.key.scope = relation_record_scope::public_source_feature;
    out.source_relation = source_key;
    out.precedence = relation_construction_precedence::accepted_source_vertex;
    if (!source_vertex_geometry(candidates, vertex_operand, vertex, out.geometry))
      return false;
    return valid_relation_request_key(out.key);
  }
  relation_request_id lineage_request{0};
  std::uint8_t endpoint_role = 0;
  bool has_lineage = false;
  bool ambiguous_lineage = false;
  for (const auto &occurrence : node.occurrences)
    for (const auto &lineage : occurrence.event_lineages) {
      relation_request_id request{0};
      if (!coplanar_relation_overlay_detail::decode_contact_lineage(
              lineage.contact_lineage, request))
        return false;
      if (!has_lineage) {
        lineage_request = request;
        endpoint_role = lineage.endpoint_role;
        has_lineage = true;
      } else if (request != lineage_request ||
                 lineage.endpoint_role != endpoint_role) {
        ambiguous_lineage = true;
      }
    }
  if (ambiguous_lineage)
    has_lineage = false;
  if (has_lineage && lineage_request.ordinal() < edge_stage.relations.size() &&
      lineage_request.ordinal() < edge_stage.request_graph.requests.size()) {
    const auto &relation = edge_stage.relations[lineage_request.ordinal()];
    const std::uint32_t point = endpoint_role;
    if (point < relation.point_count) {
      const auto &key = edge_stage.request_graph.requests[lineage_request.ordinal()].key;
      out = authority<T>{};
      out.key = construction_key(key, 2, point);
      out.source_relation = key;
      out.precedence =
          relation_construction_precedence::source_edge_source_edge_point;
      out.source_feature = key.first;
      out.geometry = geometry_from_point(relation.points[point].point,
                                         relation.points[point].accepted_source_vertex,
                                         relation.points[point].tolerance_compatible);
      out.source_occurrence = point;
      if (valid_relation_request_key(out.key) && valid_geometry(out.geometry))
        return true;
    }
  }
  out = authority<T>{};
  out.key = construction_key(source_key, 4,
                             static_cast<std::uint32_t>(node.id));
  out.source_relation = source_key;
  out.precedence = relation_construction_precedence::coplanar_overlap_endpoint;
  out.source_feature = source_key.first;
  out.geometry = geometry_from_projected(node.representative,
                                         source.facets[0].dropped_axis);
  out.source_occurrence = static_cast<std::uint32_t>(node.id);
  return valid_relation_request_key(out.key) && valid_geometry(out.geometry);
}

} // namespace relation_construction_policy_detail
} // namespace ygor::mesh_boolean::bounded
