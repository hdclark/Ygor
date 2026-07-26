#pragma once

#include "FacetFacetRelations.h"
#include "SourceEdgeRelationKernel.h"
#include "SourceFacetRegionSegmentBuild.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <tuple>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

enum class coplanar_facet_overlay_class : std::uint8_t {
  disjoint = 1,
  point_contact = 2,
  segment_contact = 3,
  area_overlap = 4,
  first_contains_second = 5,
  second_contains_first = 6,
  equal_same_orientation = 7,
  equal_opposite_orientation = 8,
};

template <class T> struct coplanar_facet_polygon_input final {
  relation_feature_key feature{};
  std::uint64_t source_facet = 0;
  std::uint64_t ring = 0;
  std::uint64_t shell = 0;
  bounded_planar_sign orientation = bounded_planar_sign::uncertain;
  std::uint8_t dropped_axis = 3;
  std::uint16_t reserved16 = 0;
  std::vector<projected_source_point<T>> polygon;
  std::vector<relation_feature_key> boundary_edges;
  std::uint32_t reserved = 0;
};

template <class T> struct coplanar_boundary_relation_input final {
  relation_request_id request{0};
  std::uint64_t first_edge_ordinal = 0;
  std::uint64_t second_edge_ordinal = 0;
  source_edge_relation_record<T> relation{};
  std::uint32_t reserved = 0;

  friend bool operator<(const coplanar_boundary_relation_input &a,
                        const coplanar_boundary_relation_input &b) noexcept {
    return std::tie(a.first_edge_ordinal, a.second_edge_ordinal, a.request) <
           std::tie(b.first_edge_ordinal, b.second_edge_ordinal, b.request);
  }
};

template <class T> struct coplanar_vertex_region_witness final {
  std::uint8_t polygon = 0;
  std::uint64_t vertex_ordinal = 0;
  source_facet_point_region_record<T> region{};
  std::uint32_t reserved = 0;
};

struct coplanar_boundary_contact final {
  relation_request_id request{0};
  std::uint64_t first_edge_ordinal = 0;
  std::uint64_t second_edge_ordinal = 0;
  source_edge_contact_class contact = source_edge_contact_class::none;
  source_edge_orientation_relation orientation =
      source_edge_orientation_relation::not_applicable;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

template <class T> struct coplanar_boundary_partition final {
  std::uint8_t polygon = 0;
  std::uint64_t edge_ordinal = 0;
  source_facet_segment_partition_record<T> partition{};
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;

  friend bool operator<(const coplanar_boundary_partition &a,
                        const coplanar_boundary_partition &b) noexcept {
    return std::tie(a.polygon, a.edge_ordinal) <
           std::tie(b.polygon, b.edge_ordinal);
  }
};

enum class coplanar_overlap_arc_kind : std::uint8_t {
  interior_boundary = 1,
  shared_boundary = 2,
};

enum class coplanar_overlap_component_kind : std::uint8_t {
  isolated_point = 1,
  boundary_segment = 2,
  area_boundary = 3,
  coincident_sheet_boundary = 4,
};

struct coplanar_event_lineage final {
  std::uint64_t contact_lineage = 0;
  std::uint8_t endpoint_role = 0;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;

  friend bool operator<(const coplanar_event_lineage &a,
                        const coplanar_event_lineage &b) noexcept {
    return std::tie(a.contact_lineage, a.endpoint_role) <
           std::tie(b.contact_lineage, b.endpoint_role);
  }

  friend bool operator==(const coplanar_event_lineage &a,
                         const coplanar_event_lineage &b) noexcept {
    return a.contact_lineage == b.contact_lineage &&
           a.endpoint_role == b.endpoint_role &&
           a.reserved8 == b.reserved8 && a.reserved16 == b.reserved16;
  }
};

struct coplanar_breakpoint_occurrence final {
  std::uint8_t polygon = 0;
  std::uint64_t edge_ordinal = 0;
  std::uint64_t breakpoint_ordinal = 0;
  bool query_source_vertex_valid = false;
  std::uint64_t query_source_vertex = 0;
  std::vector<coplanar_event_lineage> event_lineages;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;

  friend bool operator<(const coplanar_breakpoint_occurrence &a,
                        const coplanar_breakpoint_occurrence &b) noexcept {
    return std::tie(a.polygon, a.edge_ordinal, a.breakpoint_ordinal,
                    a.query_source_vertex_valid, a.query_source_vertex,
                    a.event_lineages) <
           std::tie(b.polygon, b.edge_ordinal, b.breakpoint_ordinal,
                    b.query_source_vertex_valid, b.query_source_vertex,
                    b.event_lineages);
  }

  friend bool operator==(const coplanar_breakpoint_occurrence &a,
                         const coplanar_breakpoint_occurrence &b) noexcept {
    return a.polygon == b.polygon &&
           a.edge_ordinal == b.edge_ordinal &&
           a.breakpoint_ordinal == b.breakpoint_ordinal &&
           a.query_source_vertex_valid == b.query_source_vertex_valid &&
           a.query_source_vertex == b.query_source_vertex &&
           a.event_lineages == b.event_lineages &&
           a.reserved8 == b.reserved8 && a.reserved16 == b.reserved16 &&
           a.reserved32 == b.reserved32;
  }
};

template <class T> struct coplanar_overlap_event_node final {
  std::uint64_t id = 0;
  std::vector<coplanar_breakpoint_occurrence> occurrences;
  projected_source_point<T> representative{};
  std::uint32_t reserved = 0;
};

struct coplanar_boundary_arc_occurrence final {
  std::uint8_t polygon = 0;
  std::uint64_t edge_ordinal = 0;
  std::uint64_t interval_ordinal = 0;
  std::uint64_t start_node = 0;
  std::uint64_t end_node = 0;
  bool forward_along_source_edge = true;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;

  friend bool operator<(const coplanar_boundary_arc_occurrence &a,
                        const coplanar_boundary_arc_occurrence &b) noexcept {
    return std::tie(a.polygon, a.edge_ordinal, a.interval_ordinal,
                    a.start_node, a.end_node,
                    a.forward_along_source_edge) <
           std::tie(b.polygon, b.edge_ordinal, b.interval_ordinal,
                    b.start_node, b.end_node,
                    b.forward_along_source_edge);
  }

  friend bool operator==(const coplanar_boundary_arc_occurrence &a,
                         const coplanar_boundary_arc_occurrence &b) noexcept {
    return a.polygon == b.polygon &&
           a.edge_ordinal == b.edge_ordinal &&
           a.interval_ordinal == b.interval_ordinal &&
           a.start_node == b.start_node && a.end_node == b.end_node &&
           a.forward_along_source_edge == b.forward_along_source_edge &&
           a.reserved8 == b.reserved8 && a.reserved16 == b.reserved16 &&
           a.reserved32 == b.reserved32;
  }
};

struct coplanar_oriented_boundary_arc final {
  std::uint64_t id = 0;
  coplanar_overlap_arc_kind kind =
      coplanar_overlap_arc_kind::interior_boundary;
  std::uint64_t start_node = 0;
  std::uint64_t end_node = 0;
  std::vector<coplanar_boundary_arc_occurrence> occurrences;
  std::vector<relation_request_id> overlap_lineages;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct coplanar_overlap_component final {
  std::uint64_t id = 0;
  coplanar_overlap_component_kind kind =
      coplanar_overlap_component_kind::isolated_point;
  std::vector<std::uint64_t> node_ids;
  std::vector<std::uint64_t> arc_ids;
  std::uint8_t sheet_mask = 0;
  bool closed = false;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

template <class T> struct source_facet_coplanar_overlay_record final {
  std::uint16_t schema_version =
      contract_versions::relation_coplanar_overlay_schema;
  std::uint16_t policy_version =
      contract_versions::relation_coplanar_overlay_policy;
  context_owner_token owner{};
  source_facet_source_facet_relation_record<T> support_relation{};
  std::array<coplanar_facet_polygon_input<T>, 2> facets{};
  std::vector<coplanar_boundary_relation_input<T>> boundary_relations;
  std::vector<coplanar_vertex_region_witness<T>> vertex_regions;
  std::vector<coplanar_boundary_contact> boundary_contacts;
  std::vector<coplanar_boundary_partition<T>> boundary_partitions;
  std::vector<coplanar_overlap_event_node<T>> event_nodes;
  std::vector<coplanar_oriented_boundary_arc> oriented_arcs;
  std::vector<coplanar_overlap_component> overlap_components;
  coplanar_facet_overlay_class classification =
      coplanar_facet_overlay_class::disjoint;
  bool complete_boundary_pair_coverage = false;
  bool complete_vertex_coverage = false;
  bool complete_boundary_partition_coverage = false;
  bool complete_event_lineage = false;
  bool complete_authorized_arc_coverage = false;
  bool complete_overlap_component_assembly = false;
  bool distinct_sheet_occurrences = false;
  std::uint8_t reserved8 = 0;
  std::uint64_t proper_crossing_count = 0;
  std::uint64_t overlapping_edge_pair_count = 0;
  std::uint32_t reserved32 = 0;
  bounded_boolean_digest semantic_digest{};
};

inline bounded_boolean_error coplanar_overlay_error(
    relation_subcode subcode, const char *summary,
    bounded_boolean_error_category category =
        bounded_boolean_error_category::internal_invariant_error) {
  return relation_error(subcode, category, summary,
                        relation_checkpoint::coplanar_overlay_evaluation);
}

namespace coplanar_relation_overlay_detail {

template <class T>
bool numeric_equal(T a, T b) noexcept {
  return !finite_numeric_less(a, b) && !finite_numeric_less(b, a);
}

inline bool is_boundary(source_facet_point_region_class value) noexcept {
  return value == source_facet_point_region_class::original_edge ||
         value == source_facet_point_region_class::original_vertex;
}

inline bool is_overlap(source_edge_contact_class value) noexcept {
  return value == source_edge_contact_class::partial_overlap ||
         value == source_edge_contact_class::first_contains_second ||
         value == source_edge_contact_class::second_contains_first ||
         value == source_edge_contact_class::equal;
}

template <class T>
void encode_projected_point(canonical_writer &writer,
                            const projected_source_point<T> &point) {
  writer.u64(point.source_vertex);
  writer.u64(point.source_corner);
  for (const auto value : point.nominal)
    writer.floating(value);
  for (const auto &value : point.enclosure) {
    writer.floating(value.lower());
    writer.floating(value.upper());
  }
}

template <class T>
void encode_orientation(canonical_writer &writer,
                        const source_orientation_evidence<T> &value) {
  writer.floating(value.determinant.lower());
  writer.floating(value.determinant.upper());
  writer.u8(static_cast<std::uint8_t>(value.exact_sign));
  writer.u8(static_cast<std::uint8_t>(value.bounded_sign));
  writer.u16(value.formula_version);
}

template <class T>
void encode_region(canonical_writer &writer,
                   const source_facet_point_region_record<T> &value) {
  writer.u16(value.schema_version);
  writer.u16(value.policy_version);
  writer.u8(static_cast<std::uint8_t>(value.classification));
  writer.u64(value.source_facet);
  writer.u64(value.ring);
  writer.u8(value.sweep_axis);
  writer.boolean(value.query_source_identity_valid);
  writer.boolean(value.complete_boundary_traversal);
  writer.boolean(value.boundary_ownership_resolved);
  writer.u64(value.boundary_test_count);
  writer.u64(value.parity_crossing_count);
  writer.u64(static_cast<std::uint64_t>(value.source_vertex_owners.size()));
  for (const auto owner : value.source_vertex_owners)
    writer.u64(owner);
  writer.u64(static_cast<std::uint64_t>(value.source_edge_owners.size()));
  for (const auto &owner : value.source_edge_owners) {
    writer.u64(owner.edge_ordinal);
    writer.u64(owner.origin_source_vertex);
    writer.u64(owner.destination_source_vertex);
  }
  encode_orientation(writer, value.polygon_orientation_evidence);
  writer.u64(static_cast<std::uint64_t>(value.orientation_evidence.size()));
  for (const auto &evidence : value.orientation_evidence)
    encode_orientation(writer, evidence);
  writer.u32(value.reserved);
}

template <class T>
void encode_facet(canonical_writer &writer,
                  const coplanar_facet_polygon_input<T> &facet) {
  encode_relation_feature_key(writer, facet.feature);
  writer.u64(facet.source_facet);
  writer.u64(facet.ring);
  writer.u64(facet.shell);
  writer.u8(static_cast<std::uint8_t>(facet.orientation));
  writer.u8(facet.dropped_axis);
  writer.u16(facet.reserved16);
  writer.u64(static_cast<std::uint64_t>(facet.polygon.size()));
  for (const auto &point : facet.polygon)
    encode_projected_point(writer, point);
  writer.u64(static_cast<std::uint64_t>(facet.boundary_edges.size()));
  for (const auto &edge : facet.boundary_edges)
    encode_relation_feature_key(writer, edge);
  writer.u32(facet.reserved);
}

template <class T>
bool valid_facet(const coplanar_facet_polygon_input<T> &facet) {
  if (!valid_relation_feature_key(facet.feature) ||
      facet.feature.kind != relation_feature_kind::source_facet ||
      facet.feature.primary != facet.source_facet ||
      facet.feature.secondary != facet.ring || facet.polygon.size() < 3 ||
      facet.polygon.size() != facet.boundary_edges.size() ||
      facet.orientation == bounded_planar_sign::uncertain ||
      facet.dropped_axis > 2 || facet.reserved16 != 0 ||
      facet.reserved != 0)
    return false;
  std::vector<std::uint64_t> vertices;
  vertices.reserve(facet.polygon.size());
  for (std::size_t i = 0; i < facet.polygon.size(); ++i) {
    const auto &point = facet.polygon[i];
    const auto &edge = facet.boundary_edges[i];
    if (!source_facet_region_detail::valid_projected_point(point) ||
        !valid_relation_feature_key(edge) ||
        edge.kind != relation_feature_kind::source_edge ||
        edge.operand != facet.feature.operand)
      return false;
    vertices.push_back(point.source_vertex);
  }
  std::sort(vertices.begin(), vertices.end());
  if (std::adjacent_find(vertices.begin(), vertices.end()) != vertices.end())
    return false;
  const auto orientation =
      bounded_source_polygon_kernel<T>::polygon_orientation(facet.polygon);
  return orientation && valid_source_orientation_evidence(*orientation) &&
         orientation->bounded_sign == facet.orientation &&
         orientation->exact_sign == static_cast<int>(facet.orientation);
}

inline void encode_breakpoint_occurrence(
    canonical_writer &writer, const coplanar_breakpoint_occurrence &value) {
  writer.u8(value.polygon);
  writer.u64(value.edge_ordinal);
  writer.u64(value.breakpoint_ordinal);
  writer.boolean(value.query_source_vertex_valid);
  writer.u64(value.query_source_vertex);
  writer.u64(static_cast<std::uint64_t>(value.event_lineages.size()));
  for (const auto &lineage : value.event_lineages) {
    writer.u64(lineage.contact_lineage);
    writer.u8(lineage.endpoint_role);
    writer.u8(lineage.reserved8);
    writer.u16(lineage.reserved16);
  }
  writer.u8(value.reserved8);
  writer.u16(value.reserved16);
  writer.u32(value.reserved32);
}

template <class T>
void encode_event_node(canonical_writer &writer,
                       const coplanar_overlap_event_node<T> &value) {
  writer.u64(value.id);
  writer.u64(static_cast<std::uint64_t>(value.occurrences.size()));
  for (const auto &occurrence : value.occurrences)
    encode_breakpoint_occurrence(writer, occurrence);
  encode_projected_point(writer, value.representative);
  writer.u32(value.reserved);
}

inline void encode_arc_occurrence(
    canonical_writer &writer,
    const coplanar_boundary_arc_occurrence &value) {
  writer.u8(value.polygon);
  writer.u64(value.edge_ordinal);
  writer.u64(value.interval_ordinal);
  writer.u64(value.start_node);
  writer.u64(value.end_node);
  writer.boolean(value.forward_along_source_edge);
  writer.u8(value.reserved8);
  writer.u16(value.reserved16);
  writer.u32(value.reserved32);
}

inline void encode_boundary_arc(canonical_writer &writer,
                                const coplanar_oriented_boundary_arc &value) {
  writer.u64(value.id);
  writer.u8(static_cast<std::uint8_t>(value.kind));
  writer.u64(value.start_node);
  writer.u64(value.end_node);
  writer.u64(static_cast<std::uint64_t>(value.occurrences.size()));
  for (const auto &occurrence : value.occurrences)
    encode_arc_occurrence(writer, occurrence);
  writer.u64(static_cast<std::uint64_t>(value.overlap_lineages.size()));
  for (const auto lineage : value.overlap_lineages)
    writer.u64(lineage.ordinal());
  writer.u8(value.reserved8);
  writer.u16(value.reserved16);
  writer.u32(value.reserved32);
}

inline void encode_overlap_component(
    canonical_writer &writer, const coplanar_overlap_component &value) {
  writer.u64(value.id);
  writer.u8(static_cast<std::uint8_t>(value.kind));
  writer.u64(static_cast<std::uint64_t>(value.node_ids.size()));
  for (const auto node : value.node_ids)
    writer.u64(node);
  writer.u64(static_cast<std::uint64_t>(value.arc_ids.size()));
  for (const auto arc : value.arc_ids)
    writer.u64(arc);
  writer.u8(value.sheet_mask);
  writer.boolean(value.closed);
  writer.u16(value.reserved16);
  writer.u32(value.reserved32);
}

template <class T>
std::vector<std::uint8_t> encode_semantics(
    const source_facet_coplanar_overlay_record<T> &record) {
  canonical_writer writer;
  writer.u32(0x374F5043U); // CPO7
  writer.u16(record.schema_version);
  writer.u16(record.policy_version);
  const auto support =
      encode_source_facet_relation_semantics(record.support_relation);
  writer.sized_bytes(support);
  encode_facet(writer, record.facets[0]);
  encode_facet(writer, record.facets[1]);
  writer.u64(static_cast<std::uint64_t>(record.boundary_relations.size()));
  for (const auto &boundary : record.boundary_relations) {
    writer.u64(boundary.request.ordinal());
    writer.u64(boundary.first_edge_ordinal);
    writer.u64(boundary.second_edge_ordinal);
    const auto relation = encode_source_edge_relation_semantics(boundary.relation);
    writer.sized_bytes(relation);
    writer.u32(boundary.reserved);
  }
  writer.u64(static_cast<std::uint64_t>(record.vertex_regions.size()));
  for (const auto &vertex : record.vertex_regions) {
    writer.u8(vertex.polygon);
    writer.u64(vertex.vertex_ordinal);
    encode_region(writer, vertex.region);
    writer.u32(vertex.reserved);
  }
  writer.u64(static_cast<std::uint64_t>(record.boundary_contacts.size()));
  for (const auto &contact : record.boundary_contacts) {
    writer.u64(contact.request.ordinal());
    writer.u64(contact.first_edge_ordinal);
    writer.u64(contact.second_edge_ordinal);
    writer.u8(static_cast<std::uint8_t>(contact.contact));
    writer.u8(static_cast<std::uint8_t>(contact.orientation));
    writer.u16(contact.reserved16);
    writer.u32(contact.reserved32);
  }
  writer.u64(static_cast<std::uint64_t>(record.boundary_partitions.size()));
  for (const auto &partition : record.boundary_partitions) {
    writer.u8(partition.polygon);
    writer.u64(partition.edge_ordinal);
    writer.sized_bytes(
        source_facet_region_detail::semantic_bytes(partition.partition));
    writer.u8(partition.reserved8);
    writer.u16(partition.reserved16);
    writer.u32(partition.reserved32);
  }
  writer.u64(static_cast<std::uint64_t>(record.event_nodes.size()));
  for (const auto &node : record.event_nodes)
    encode_event_node(writer, node);
  writer.u64(static_cast<std::uint64_t>(record.oriented_arcs.size()));
  for (const auto &arc : record.oriented_arcs)
    encode_boundary_arc(writer, arc);
  writer.u64(static_cast<std::uint64_t>(record.overlap_components.size()));
  for (const auto &component : record.overlap_components)
    encode_overlap_component(writer, component);
  writer.u8(static_cast<std::uint8_t>(record.classification));
  writer.boolean(record.complete_boundary_pair_coverage);
  writer.boolean(record.complete_vertex_coverage);
  writer.boolean(record.complete_boundary_partition_coverage);
  writer.boolean(record.complete_event_lineage);
  writer.boolean(record.complete_authorized_arc_coverage);
  writer.boolean(record.complete_overlap_component_assembly);
  writer.boolean(record.distinct_sheet_occurrences);
  writer.u8(record.reserved8);
  writer.u64(record.proper_crossing_count);
  writer.u64(record.overlapping_edge_pair_count);
  writer.u32(record.reserved32);
  return writer.take();
}

template <class T>
bool matching_boundary_relation(
    const coplanar_facet_polygon_input<T> &first,
    const coplanar_facet_polygon_input<T> &second,
    const coplanar_boundary_relation_input<T> &input,
    const context_owner_token &owner) {
  if (input.first_edge_ordinal >= first.boundary_edges.size() ||
      input.second_edge_ordinal >= second.boundary_edges.size() ||
      input.reserved != 0 || !valid_source_edge_relation_record(input.relation) ||
      !input.relation.owner.same_owner(owner))
    return false;
  auto first_feature = first.boundary_edges[input.first_edge_ordinal];
  auto second_feature = second.boundary_edges[input.second_edge_ordinal];
  if (second_feature < first_feature)
    std::swap(first_feature, second_feature);
  return input.relation.first_feature == first_feature &&
         input.relation.second_feature == second_feature;
}

template <class T>
bool singleton_parameter(const source_edge_parameter_evidence<T> &parameter,
                         T &value) {
  if (!numeric_equal(parameter.enclosure.lower(), parameter.enclosure.upper()))
    return false;
  value = parameter.enclosure.lower();
  return !finite_numeric_less(value, T(0)) &&
         !finite_numeric_less(T(1), value);
}

template <class T>
bool edge_fully_covered(
    std::uint8_t polygon, std::uint64_t edge_ordinal,
    const std::vector<coplanar_boundary_relation_input<T>> &relations) {
  std::vector<std::pair<T, T>> intervals;
  for (const auto &input : relations) {
    if (!is_overlap(input.relation.contact))
      continue;
    if ((polygon == 0 && input.first_edge_ordinal != edge_ordinal) ||
        (polygon == 1 && input.second_edge_ordinal != edge_ordinal))
      continue;
    const auto &edge_feature = polygon == 0
                                   ? input.relation.first_feature
                                   : input.relation.second_feature;
    const bool relation_first =
        edge_feature == input.relation.first_feature;
    const auto &parameters = relation_first ? input.relation.first_parameters
                                            : input.relation.second_parameters;
    T a = T(0);
    T b = T(0);
    if (!singleton_parameter(parameters[0], a) ||
        !singleton_parameter(parameters[1], b))
      return false;
    if (finite_numeric_less(b, a))
      std::swap(a, b);
    intervals.emplace_back(a, b);
  }
  if (intervals.empty())
    return false;
  std::sort(intervals.begin(), intervals.end(),
            [](const auto &a, const auto &b) {
              return finite_numeric_less(a.first, b.first) ||
                     (numeric_equal(a.first, b.first) &&
                      finite_numeric_less(a.second, b.second));
            });
  if (!numeric_equal(intervals.front().first, T(0)))
    return false;
  T covered = intervals.front().second;
  for (std::size_t i = 1; i < intervals.size(); ++i) {
    if (finite_numeric_less(covered, intervals[i].first))
      return false;
    if (finite_numeric_less(covered, intervals[i].second))
      covered = intervals[i].second;
  }
  return numeric_equal(covered, T(1));
}

template <class T>
bool complete_boundary_coverage(
    const std::array<coplanar_facet_polygon_input<T>, 2> &facets,
    const std::vector<coplanar_boundary_relation_input<T>> &relations) {
  for (std::uint8_t polygon = 0; polygon < 2; ++polygon)
    for (std::uint64_t edge = 0; edge < facets[polygon].boundary_edges.size();
         ++edge)
      if (!edge_fully_covered(polygon, edge, relations))
        return false;
  return true;
}

template <class T>
boolean_outcome<coplanar_boundary_partition<T>> build_boundary_partition(
    std::uint8_t polygon, std::uint64_t edge_ordinal,
    const std::array<coplanar_facet_polygon_input<T>, 2> &facets,
    const std::vector<coplanar_boundary_relation_input<T>> &relations) {
  if (polygon > 1 || edge_ordinal >= facets[polygon].boundary_edges.size())
    return boolean_outcome<coplanar_boundary_partition<T>>::failure(
        coplanar_overlay_error(
            relation_subcode::coplanar_overlay_malformed,
            "Component 07 coplanar boundary partition key is malformed"));
  const auto other = static_cast<std::uint8_t>(1 - polygon);
  const auto &query_facet = facets[polygon];
  const auto &other_facet = facets[other];

  source_edge_facet_input<T> input;
  input.edge.feature = query_facet.boundary_edges[edge_ordinal];
  input.facet_feature = other_facet.feature;
  input.source_facet = other_facet.source_facet;
  input.ring = other_facet.ring;
  input.shell = other_facet.shell;
  input.dropped_axis = query_facet.dropped_axis;
  input.polygon = other_facet.polygon;
  input.polygon_orientation = other_facet.orientation;

  std::vector<source_facet_segment_contact_proposal<T>> contacts;
  contacts.reserve(other_facet.boundary_edges.size());
  for (const auto &relation : relations) {
    const bool matches =
        polygon == 0 ? relation.first_edge_ordinal == edge_ordinal
                     : relation.second_edge_ordinal == edge_ordinal;
    if (!matches)
      continue;
    const auto opposite_edge = polygon == 0 ? relation.second_edge_ordinal
                                             : relation.first_edge_ordinal;
    if (opposite_edge >= other_facet.boundary_edges.size())
      return boolean_outcome<coplanar_boundary_partition<T>>::failure(
          coplanar_overlay_error(
              relation_subcode::coplanar_overlay_dependency_missing,
              "Component 07 coplanar boundary partition relation is out of range"));
    source_edge_facet_boundary_relation<T> boundary;
    boundary.binding.request = relation.request;
    boundary.binding.feature = other_facet.boundary_edges[opposite_edge];
    boundary.binding.owner = {
        opposite_edge, other_facet.polygon[opposite_edge].source_vertex,
        other_facet.polygon[(opposite_edge + 1) %
                            other_facet.polygon.size()].source_vertex};
    boundary.binding.parameter_source_vertices = {
        boundary.binding.owner.origin_source_vertex,
        boundary.binding.owner.destination_source_vertex};
    boundary.relation = relation.relation;
    const std::array<source_edge_parameter_evidence<T>, 2> *query_parameters =
        nullptr;
    const std::array<source_edge_parameter_evidence<T>, 2> *facet_parameters =
        nullptr;
    if (!source_edge_facet_detail::relation_parameter_views(
            input, boundary, query_parameters, facet_parameters))
      return boolean_outcome<coplanar_boundary_partition<T>>::failure(
          coplanar_overlay_error(
              relation_subcode::coplanar_overlay_dependency_missing,
              "Component 07 coplanar boundary partition feature binding is inconsistent"));
    const auto prior_size = contacts.size();
    bounded_boolean_error error;
    if (!source_edge_facet_detail::append_coplanar_contacts(
            input, boundary, contacts, error))
      return boolean_outcome<coplanar_boundary_partition<T>>::failure(
          std::move(error));
    if (contacts.size() == prior_size)
      continue;
    if (contacts.size() != prior_size + 1)
      return boolean_outcome<coplanar_boundary_partition<T>>::failure(
          coplanar_overlay_error(
              relation_subcode::coplanar_overlay_invariant,
              "Component 07 coplanar boundary relation emitted an invalid contact count"));

    auto &contact = contacts.back();
    const auto canonicalize_query_endpoint = [&](
        const source_edge_parameter_evidence<T> &parameter,
        T &rounded, finite_interval<T> &enclosure,
        projected_source_point<T> &point, bool &identity_valid) -> bool {
      const auto mask = source_edge_relation_detail::endpoint_mask(parameter);
      if (mask != 1 && mask != 2)
        return true;
      const auto endpoint = mask == 1 ? std::size_t(0) : std::size_t(1);
      const T value = endpoint == 0 ? T(0) : T(1);
      const auto singleton = finite_interval<T>::checked_singleton(value);
      if (!singleton)
        return false;
      rounded = value;
      enclosure = *singleton;
      point = query_facet.polygon[
          (edge_ordinal + endpoint) % query_facet.polygon.size()];
      // Source identities are operand-local. Opposite-operand query endpoint
      // identity cannot be used as target-polygon ownership evidence.
      identity_valid = false;
      return true;
    };
    if (relation.relation.contact == source_edge_contact_class::proper_crossing ||
        relation.relation.contact == source_edge_contact_class::endpoint_contact ||
        relation.relation.contact == source_edge_contact_class::point_contact) {
      if (!canonicalize_query_endpoint(
              (*query_parameters)[0], contact.first_rounded_parameter,
              contact.first_parameter, contact.first_point,
              contact.first_point_source_identity_valid))
        return boolean_outcome<coplanar_boundary_partition<T>>::failure(
            coplanar_overlay_error(
                relation_subcode::coplanar_overlay_invariant,
                "Component 07 could not canonicalize a boundary endpoint contact"));
      contact.second_rounded_parameter = contact.first_rounded_parameter;
      contact.second_parameter = contact.first_parameter;
      contact.second_point = contact.first_point;
      contact.second_point_source_identity_valid =
          contact.first_point_source_identity_valid;
    } else {
      std::size_t first_parameter = 0;
      std::size_t second_parameter = 1;
      if (source_edge_facet_detail::parameter_definitely_before(
              (*query_parameters)[1], (*query_parameters)[0]))
        std::swap(first_parameter, second_parameter);
      if (!canonicalize_query_endpoint(
              (*query_parameters)[first_parameter],
              contact.first_rounded_parameter, contact.first_parameter,
              contact.first_point, contact.first_point_source_identity_valid) ||
          !canonicalize_query_endpoint(
              (*query_parameters)[second_parameter],
              contact.second_rounded_parameter, contact.second_parameter,
              contact.second_point, contact.second_point_source_identity_valid))
        return boolean_outcome<coplanar_boundary_partition<T>>::failure(
            coplanar_overlay_error(
                relation_subcode::coplanar_overlay_invariant,
                "Component 07 could not canonicalize a boundary overlap endpoint"));
    }
  }

  if (contacts.size() > other_facet.boundary_edges.size())
    return boolean_outcome<coplanar_boundary_partition<T>>::failure(
        coplanar_overlay_error(
            relation_subcode::coplanar_overlay_boundary_incomplete,
            "Component 07 coplanar boundary partition has excess contacts"));
  auto partition = partition_source_facet_segment(
      other_facet.source_facet, other_facet.ring,
      query_facet.polygon[edge_ordinal], false,
      query_facet.polygon[(edge_ordinal + 1) % query_facet.polygon.size()],
      false, other_facet.polygon, other_facet.orientation,
      other_facet.boundary_edges.size(), true, std::move(contacts));
  if (!partition.has_value())
    return boolean_outcome<coplanar_boundary_partition<T>>::failure(
        *partition.error());
  auto reconciled = reconcile_source_facet_triangle_local_witnesses(
      std::move(*partition.value()), {});
  if (!reconciled.has_value())
    return boolean_outcome<coplanar_boundary_partition<T>>::failure(
        *reconciled.error());

  coplanar_boundary_partition<T> result;
  result.polygon = polygon;
  result.edge_ordinal = edge_ordinal;
  result.partition = std::move(*reconciled.value());
  return boolean_outcome<coplanar_boundary_partition<T>>::success(
      std::move(result));
}

template <class T>
bool projected_points_compatible(const projected_source_point<T> &a,
                                 const projected_source_point<T> &b) {
  if (!source_facet_region_detail::valid_projected_point(a) ||
      !source_facet_region_detail::valid_projected_point(b))
    return false;
  for (std::size_t axis = 0; axis < 2; ++axis)
    if (!source_facet_region_detail::intervals_overlap(a.enclosure[axis],
                                                       b.enclosure[axis]))
      return false;
  return true;
}

inline bool authorized_boundary_interval(
    source_facet_segment_interval_class classification) noexcept {
  return classification == source_facet_segment_interval_class::interior ||
         classification ==
             source_facet_segment_interval_class::original_edge_overlap;
}

inline bool decode_contact_lineage(std::uint64_t lineage,
                                   relation_request_id &request) noexcept {
  if (lineage == 0 || ((lineage - 1) & 1U) != 0)
    return false;
  request = relation_request_id((lineage - 1) / 2);
  return true;
}

template <class T>
const coplanar_boundary_relation_input<T> *find_boundary_relation(
    const std::vector<coplanar_boundary_relation_input<T>> &relations,
    relation_request_id request) {
  const auto iterator = std::find_if(
      relations.begin(), relations.end(),
      [request](const coplanar_boundary_relation_input<T> &candidate) {
        return candidate.request == request;
      });
  return iterator == relations.end() ? nullptr : &*iterator;
}

template <class T>
bool parameter_matches_evidence(
    const finite_interval<T> &parameter,
    const source_edge_parameter_evidence<T> &evidence) {
  if (source_facet_region_detail::interval_equal_bits(parameter,
                                                       evidence.enclosure))
    return true;
  if (!source_facet_region_detail::singleton(parameter))
    return false;
  const auto mask = source_edge_relation_detail::endpoint_mask(evidence);
  return (numeric_equal(parameter.lower(), T(0)) && mask == 1) ||
         (numeric_equal(parameter.lower(), T(1)) && mask == 2);
}

template <class T>
bool canonical_event_endpoint_role(
    const coplanar_boundary_relation_input<T> &boundary,
    const relation_feature_key &query_edge,
    source_facet_segment_contact_kind kind,
    const finite_interval<T> &parameter, std::uint8_t &role) {
  const std::array<source_edge_parameter_evidence<T>, 2> *query = nullptr;
  if (boundary.relation.first_feature == query_edge)
    query = &boundary.relation.first_parameters;
  else if (boundary.relation.second_feature == query_edge)
    query = &boundary.relation.second_parameters;
  else
    return false;
  if (kind == source_facet_segment_contact_kind::point_contact) {
    if (boundary.relation.point_count != 1 ||
        !parameter_matches_evidence(parameter, (*query)[0]))
      return false;
    role = 0;
    return true;
  }
  if (kind != source_facet_segment_contact_kind::boundary_overlap ||
      boundary.relation.point_count != 2)
    return false;
  std::uint8_t match = 2;
  for (std::uint8_t point = 0; point < 2; ++point)
    if (parameter_matches_evidence(parameter, (*query)[point])) {
      if (match != 2)
        return false;
      match = point;
    }
  if (match == 2)
    return false;
  role = static_cast<std::uint8_t>(match + 1);
  return true;
}

template <class T>
bool breakpoint_event_lineages(
    const source_facet_coplanar_overlay_record<T> &record,
    const coplanar_boundary_partition<T> &entry,
    const source_facet_segment_breakpoint<T> &breakpoint,
    std::vector<coplanar_event_lineage> &lineages) {
  lineages.clear();
  const auto &query = record.facets[entry.polygon];
  if (entry.edge_ordinal >= query.boundary_edges.size())
    return false;
  for (const auto &contact : entry.partition.contacts) {
    if (!std::binary_search(breakpoint.contact_lineages.begin(),
                            breakpoint.contact_lineages.end(),
                            contact.lineage))
      continue;
    relation_request_id request{0};
    if (!decode_contact_lineage(contact.lineage, request))
      return false;
    const auto *boundary =
        find_boundary_relation(record.boundary_relations, request);
    if (!boundary)
      return false;
    const auto add = [&](const finite_interval<T> &parameter,
                         const projected_source_point<T> &point) {
      if (!source_facet_region_detail::interval_equal_bits(
              breakpoint.parameter, parameter) ||
          !source_facet_region_detail::same_projected_geometry(
              breakpoint.point, point))
        return true;
      std::uint8_t role = 0;
      if (!canonical_event_endpoint_role(
              *boundary, query.boundary_edges[entry.edge_ordinal],
              contact.kind, parameter, role))
        return false;
      lineages.push_back({contact.lineage, role, 0, 0});
      return true;
    };
    if (!add(contact.first_parameter, contact.first_point) ||
        !add(contact.second_parameter, contact.second_point))
      return false;
  }
  std::sort(lineages.begin(), lineages.end());
  lineages.erase(std::unique(lineages.begin(), lineages.end()),
                 lineages.end());
  if (lineages.size() != breakpoint.contact_lineages.size())
    return false;
  for (std::size_t index = 0; index < lineages.size(); ++index)
    if (lineages[index].contact_lineage !=
        breakpoint.contact_lineages[index])
      return false;
  return true;
}

class deterministic_union_find final {
public:
  explicit deterministic_union_find(std::size_t count) : parent_(count) {
    for (std::size_t index = 0; index < count; ++index)
      parent_[index] = index;
  }

  std::size_t find(std::size_t value) {
    while (parent_[value] != value) {
      parent_[value] = parent_[parent_[value]];
      value = parent_[value];
    }
    return value;
  }

  void unite(std::size_t first, std::size_t second) {
    first = find(first);
    second = find(second);
    if (first == second)
      return;
    if (second < first)
      std::swap(first, second);
    parent_[second] = first;
  }

private:
  std::vector<std::size_t> parent_;
};

template <class T> struct breakpoint_candidate final {
  coplanar_breakpoint_occurrence occurrence{};
  projected_source_point<T> point{};
  std::size_t partition_index = 0;
  std::size_t breakpoint_index = 0;
};

template <class T>
bool share_event_lineage(const breakpoint_candidate<T> &a,
                         const breakpoint_candidate<T> &b) {
  std::size_t first = 0;
  std::size_t second = 0;
  while (first < a.occurrence.event_lineages.size() &&
         second < b.occurrence.event_lineages.size()) {
    if (a.occurrence.event_lineages[first] ==
        b.occurrence.event_lineages[second])
      return true;
    if (a.occurrence.event_lineages[first] <
        b.occurrence.event_lineages[second])
      ++first;
    else
      ++second;
  }
  return false;
}

template <class T>
bool relevant_breakpoint(const coplanar_boundary_partition<T> &entry,
                         std::size_t breakpoint_index) {
  if (breakpoint_index >= entry.partition.breakpoints.size())
    return false;
  if (!entry.partition.breakpoints[breakpoint_index].contact_lineages.empty())
    return true;
  for (const auto &interval : entry.partition.intervals)
    if (authorized_boundary_interval(interval.classification) &&
        (interval.left_breakpoint == breakpoint_index ||
         interval.right_breakpoint == breakpoint_index))
      return true;
  return false;
}

template <class T>
bool build_event_nodes(
    source_facet_coplanar_overlay_record<T> &record,
    std::vector<std::vector<std::size_t>> &node_by_breakpoint) {
  constexpr auto missing = std::numeric_limits<std::size_t>::max();
  std::vector<breakpoint_candidate<T>> candidates;
  node_by_breakpoint.clear();
  node_by_breakpoint.resize(record.boundary_partitions.size());
  for (std::size_t partition_index = 0;
       partition_index < record.boundary_partitions.size(); ++partition_index) {
    const auto &entry = record.boundary_partitions[partition_index];
    if (entry.polygon > 1 ||
        entry.edge_ordinal >= record.facets[entry.polygon].polygon.size())
      return false;
    node_by_breakpoint[partition_index].assign(
        entry.partition.breakpoints.size(), missing);
    for (std::size_t breakpoint_index = 0;
         breakpoint_index < entry.partition.breakpoints.size();
         ++breakpoint_index) {
      if (!relevant_breakpoint(entry, breakpoint_index))
        continue;
      const auto &breakpoint =
          entry.partition.breakpoints[breakpoint_index];
      breakpoint_candidate<T> candidate;
      candidate.occurrence.polygon = entry.polygon;
      candidate.occurrence.edge_ordinal = entry.edge_ordinal;
      candidate.occurrence.breakpoint_ordinal = breakpoint_index;
      const auto mask = breakpoint.segment_endpoint_mask;
      if (mask != 0) {
        if (mask != 1 && mask != 2)
          return false;
        const auto vertex_index =
            mask == 1 ? entry.edge_ordinal
                      : (entry.edge_ordinal + 1) %
                            record.facets[entry.polygon].polygon.size();
        candidate.occurrence.query_source_vertex_valid = true;
        candidate.occurrence.query_source_vertex =
            record.facets[entry.polygon].polygon[vertex_index].source_vertex;
      }
      if (!breakpoint_event_lineages(record, entry, breakpoint,
                                     candidate.occurrence.event_lineages))
        return false;
      candidate.point = breakpoint.point;
      candidate.partition_index = partition_index;
      candidate.breakpoint_index = breakpoint_index;
      candidates.push_back(std::move(candidate));
    }
  }
  std::sort(candidates.begin(), candidates.end(),
            [](const auto &a, const auto &b) {
              return a.occurrence < b.occurrence;
            });
  for (std::size_t index = 1; index < candidates.size(); ++index)
    if (!(candidates[index - 1].occurrence < candidates[index].occurrence))
      return false;

  deterministic_union_find equivalence(candidates.size());
  for (std::size_t first = 0; first < candidates.size(); ++first)
    for (std::size_t second = first + 1; second < candidates.size(); ++second) {
      const bool same_vertex =
          candidates[first].occurrence.query_source_vertex_valid &&
          candidates[second].occurrence.query_source_vertex_valid &&
          candidates[first].occurrence.polygon ==
              candidates[second].occurrence.polygon &&
          candidates[first].occurrence.query_source_vertex ==
              candidates[second].occurrence.query_source_vertex;
      if (!same_vertex && !share_event_lineage(candidates[first],
                                               candidates[second]))
        continue;
      if (!projected_points_compatible(candidates[first].point,
                                       candidates[second].point))
        return false;
      equivalence.unite(first, second);
    }

  struct event_group final {
    std::vector<std::size_t> candidates;
    std::vector<coplanar_breakpoint_occurrence> occurrences;
    projected_source_point<T> representative{};
  };
  std::vector<event_group> groups;
  std::vector<std::size_t> root_to_group(candidates.size(), missing);
  for (std::size_t index = 0; index < candidates.size(); ++index) {
    const auto root = equivalence.find(index);
    if (root_to_group[root] == missing) {
      root_to_group[root] = groups.size();
      groups.emplace_back();
    }
    groups[root_to_group[root]].candidates.push_back(index);
  }
  for (auto &group : groups) {
    for (std::size_t first = 0; first < group.candidates.size(); ++first)
      for (std::size_t second = first + 1;
           second < group.candidates.size(); ++second)
        if (!projected_points_compatible(
                candidates[group.candidates[first]].point,
                candidates[group.candidates[second]].point))
          return false;
    for (const auto index : group.candidates)
      group.occurrences.push_back(candidates[index].occurrence);
    std::sort(group.occurrences.begin(), group.occurrences.end());
    if (group.occurrences.empty() ||
        std::adjacent_find(group.occurrences.begin(),
                           group.occurrences.end()) !=
            group.occurrences.end())
      return false;
    const auto representative = *std::min_element(
        group.candidates.begin(), group.candidates.end(),
        [&](std::size_t a, std::size_t b) {
          return candidates[a].occurrence < candidates[b].occurrence;
        });
    group.representative = candidates[representative].point;
  }
  std::sort(groups.begin(), groups.end(),
            [](const auto &a, const auto &b) {
              return a.occurrences < b.occurrences;
            });

  record.event_nodes.clear();
  record.event_nodes.reserve(groups.size());
  for (std::size_t node = 0; node < groups.size(); ++node) {
    coplanar_overlap_event_node<T> event;
    event.id = node;
    event.occurrences = groups[node].occurrences;
    event.representative = groups[node].representative;
    record.event_nodes.push_back(std::move(event));
    for (const auto candidate_index : groups[node].candidates) {
      const auto &candidate = candidates[candidate_index];
      auto &slot = node_by_breakpoint[candidate.partition_index]
                                     [candidate.breakpoint_index];
      if (slot != missing)
        return false;
      slot = node;
    }
  }
  record.complete_event_lineage = true;
  return true;
}

inline bool lineage_intersection(
    const std::vector<relation_request_id> &a,
    const std::vector<relation_request_id> &b,
    std::vector<relation_request_id> *out = nullptr) {
  std::size_t first = 0;
  std::size_t second = 0;
  bool found = false;
  if (out)
    out->clear();
  while (first < a.size() && second < b.size()) {
    if (a[first] == b[second]) {
      found = true;
      if (out)
        out->push_back(a[first]);
      ++first;
      ++second;
    } else if (a[first] < b[second]) {
      ++first;
    } else {
      ++second;
    }
  }
  return found;
}

template <class T>
bool interval_overlap_lineages(
    const source_facet_coplanar_overlay_record<T> &record,
    const coplanar_boundary_partition<T> &entry,
    const source_facet_segment_interval<T> &interval,
    std::vector<relation_request_id> &lineages) {
  lineages.clear();
  if (interval.left_breakpoint >= entry.partition.breakpoints.size() ||
      interval.right_breakpoint >= entry.partition.breakpoints.size())
    return false;
  std::vector<coplanar_event_lineage> left;
  std::vector<coplanar_event_lineage> right;
  if (!breakpoint_event_lineages(
          record, entry,
          entry.partition.breakpoints[interval.left_breakpoint], left) ||
      !breakpoint_event_lineages(
          record, entry,
          entry.partition.breakpoints[interval.right_breakpoint], right))
    return false;
  for (const auto &a : left)
    for (const auto &b : right) {
      if (a.contact_lineage != b.contact_lineage ||
          !((a.endpoint_role == 1 && b.endpoint_role == 2) ||
            (a.endpoint_role == 2 && b.endpoint_role == 1)))
        continue;
      relation_request_id request{0};
      if (!decode_contact_lineage(a.contact_lineage, request))
        return false;
      const auto *relation =
          find_boundary_relation(record.boundary_relations, request);
      if (!relation || !is_overlap(relation->relation.contact))
        return false;
      lineages.push_back(request);
    }
  std::sort(lineages.begin(), lineages.end());
  lineages.erase(std::unique(lineages.begin(), lineages.end()),
                 lineages.end());
  return !lineages.empty();
}

struct pending_boundary_arc final {
  coplanar_overlap_arc_kind kind =
      coplanar_overlap_arc_kind::interior_boundary;
  std::uint64_t start_node = 0;
  std::uint64_t end_node = 0;
  coplanar_boundary_arc_occurrence occurrence{};
  std::vector<relation_request_id> overlap_lineages;
};

template <class T>
bool build_boundary_arcs(
    source_facet_coplanar_overlay_record<T> &record,
    const std::vector<std::vector<std::size_t>> &node_by_breakpoint) {
  constexpr auto missing = std::numeric_limits<std::size_t>::max();
  if (node_by_breakpoint.size() != record.boundary_partitions.size())
    return false;
  std::vector<pending_boundary_arc> pending;
  std::uint64_t authorized_count = 0;
  for (std::size_t partition_index = 0;
       partition_index < record.boundary_partitions.size(); ++partition_index) {
    const auto &entry = record.boundary_partitions[partition_index];
    if (node_by_breakpoint[partition_index].size() !=
        entry.partition.breakpoints.size())
      return false;
    const bool forward = record.facets[entry.polygon].orientation ==
                         bounded_planar_sign::positive;
    for (std::size_t interval_index = 0;
         interval_index < entry.partition.intervals.size(); ++interval_index) {
      const auto &interval = entry.partition.intervals[interval_index];
      if (!authorized_boundary_interval(interval.classification))
        continue;
      ++authorized_count;
      if (interval.left_breakpoint >=
              node_by_breakpoint[partition_index].size() ||
          interval.right_breakpoint >=
              node_by_breakpoint[partition_index].size())
        return false;
      const auto left =
          node_by_breakpoint[partition_index][interval.left_breakpoint];
      const auto right =
          node_by_breakpoint[partition_index][interval.right_breakpoint];
      if (left == missing || right == missing || left == right ||
          left >= record.event_nodes.size() || right >= record.event_nodes.size())
        return false;
      pending_boundary_arc arc;
      arc.kind = interval.classification ==
                         source_facet_segment_interval_class::interior
                     ? coplanar_overlap_arc_kind::interior_boundary
                     : coplanar_overlap_arc_kind::shared_boundary;
      arc.occurrence.polygon = entry.polygon;
      arc.occurrence.edge_ordinal = entry.edge_ordinal;
      arc.occurrence.interval_ordinal = interval_index;
      arc.occurrence.forward_along_source_edge = forward;
      arc.occurrence.start_node = forward ? left : right;
      arc.occurrence.end_node = forward ? right : left;
      if (arc.kind == coplanar_overlap_arc_kind::interior_boundary) {
        arc.start_node = arc.occurrence.start_node;
        arc.end_node = arc.occurrence.end_node;
      } else {
        arc.start_node = std::min(left, right);
        arc.end_node = std::max(left, right);
        if (!interval_overlap_lineages(record, entry, interval,
                                       arc.overlap_lineages))
          return false;
      }
      pending.push_back(std::move(arc));
    }
  }

  std::vector<coplanar_oriented_boundary_arc> arcs;
  for (const auto &candidate : pending) {
    if (candidate.kind == coplanar_overlap_arc_kind::interior_boundary) {
      coplanar_oriented_boundary_arc arc;
      arc.kind = candidate.kind;
      arc.start_node = candidate.start_node;
      arc.end_node = candidate.end_node;
      arc.occurrences = {candidate.occurrence};
      arcs.push_back(std::move(arc));
    }
  }

  std::vector<std::size_t> shared;
  for (std::size_t index = 0; index < pending.size(); ++index)
    if (pending[index].kind == coplanar_overlap_arc_kind::shared_boundary)
      shared.push_back(index);
  deterministic_union_find shared_groups(shared.size());
  for (std::size_t first = 0; first < shared.size(); ++first)
    for (std::size_t second = first + 1; second < shared.size(); ++second) {
      const auto &a = pending[shared[first]];
      const auto &b = pending[shared[second]];
      if (a.start_node == b.start_node && a.end_node == b.end_node &&
          lineage_intersection(a.overlap_lineages, b.overlap_lineages))
        shared_groups.unite(first, second);
    }
  std::vector<std::vector<std::size_t>> grouped(shared.size());
  for (std::size_t index = 0; index < shared.size(); ++index)
    grouped[shared_groups.find(index)].push_back(shared[index]);
  for (auto &group : grouped) {
    if (group.empty())
      continue;
    coplanar_oriented_boundary_arc arc;
    arc.kind = coplanar_overlap_arc_kind::shared_boundary;
    arc.start_node = pending[group.front()].start_node;
    arc.end_node = pending[group.front()].end_node;
    arc.overlap_lineages = pending[group.front()].overlap_lineages;
    for (const auto pending_index : group) {
      const auto &candidate = pending[pending_index];
      if (candidate.start_node != arc.start_node ||
          candidate.end_node != arc.end_node)
        return false;
      std::vector<relation_request_id> common;
      if (!lineage_intersection(arc.overlap_lineages,
                                candidate.overlap_lineages, &common))
        return false;
      arc.overlap_lineages = std::move(common);
      arc.occurrences.push_back(candidate.occurrence);
    }
    std::sort(arc.occurrences.begin(), arc.occurrences.end());
    if (arc.occurrences.empty() ||
        std::adjacent_find(arc.occurrences.begin(), arc.occurrences.end()) !=
            arc.occurrences.end())
      return false;
    arcs.push_back(std::move(arc));
  }

  const auto arc_less = [](const auto &a, const auto &b) {
    return std::tie(a.kind, a.start_node, a.end_node, a.occurrences,
                    a.overlap_lineages) <
           std::tie(b.kind, b.start_node, b.end_node, b.occurrences,
                    b.overlap_lineages);
  };
  std::sort(arcs.begin(), arcs.end(), arc_less);
  std::uint64_t occurrence_count = 0;
  for (std::size_t index = 0; index < arcs.size(); ++index) {
    arcs[index].id = index;
    occurrence_count += arcs[index].occurrences.size();
  }
  if (occurrence_count != authorized_count)
    return false;
  record.oriented_arcs = std::move(arcs);
  record.complete_authorized_arc_coverage = true;
  return true;
}

inline bool valid_overlay_classification(
    coplanar_facet_overlay_class classification) noexcept {
  return classification >= coplanar_facet_overlay_class::disjoint &&
         classification <=
             coplanar_facet_overlay_class::equal_opposite_orientation;
}

template <class T>
bool build_overlap_components(
    source_facet_coplanar_overlay_record<T> &record) {
  if (!valid_overlay_classification(record.classification))
    return false;
  deterministic_union_find connectivity(record.event_nodes.size());
  for (const auto &arc : record.oriented_arcs) {
    if (arc.start_node >= record.event_nodes.size() ||
        arc.end_node >= record.event_nodes.size() ||
        arc.start_node == arc.end_node)
      return false;
    connectivity.unite(arc.start_node, arc.end_node);
  }
  struct pending_component final {
    std::vector<std::uint64_t> node_ids;
    std::vector<std::uint64_t> arc_ids;
    std::uint8_t sheet_mask = 0;
    bool closed = false;
    coplanar_overlap_component_kind kind =
        coplanar_overlap_component_kind::isolated_point;
  };
  std::vector<pending_component> pending(record.event_nodes.size());
  std::vector<bool> used(record.event_nodes.size(), false);
  for (std::size_t node = 0; node < record.event_nodes.size(); ++node) {
    const auto root = connectivity.find(node);
    used[root] = true;
    pending[root].node_ids.push_back(node);
    for (const auto &occurrence : record.event_nodes[node].occurrences) {
      if (occurrence.polygon > 1)
        return false;
      pending[root].sheet_mask |=
          static_cast<std::uint8_t>(1U << occurrence.polygon);
    }
  }
  for (const auto &arc : record.oriented_arcs) {
    const auto root = connectivity.find(arc.start_node);
    pending[root].arc_ids.push_back(arc.id);
    for (const auto &occurrence : arc.occurrences) {
      if (occurrence.polygon > 1)
        return false;
      pending[root].sheet_mask |=
          static_cast<std::uint8_t>(1U << occurrence.polygon);
    }
  }

  std::vector<coplanar_overlap_component> components;
  for (std::size_t root = 0; root < pending.size(); ++root) {
    if (!used[root])
      continue;
    auto &candidate = pending[root];
    std::sort(candidate.node_ids.begin(), candidate.node_ids.end());
    std::sort(candidate.arc_ids.begin(), candidate.arc_ids.end());
    std::vector<std::uint64_t> degree(record.event_nodes.size(), 0);
    for (const auto arc_id : candidate.arc_ids) {
      if (arc_id >= record.oriented_arcs.size())
        return false;
      const auto &arc = record.oriented_arcs[arc_id];
      ++degree[arc.start_node];
      ++degree[arc.end_node];
    }
    candidate.closed = !candidate.arc_ids.empty();
    for (const auto node : candidate.node_ids)
      if (degree[node] != 2)
        candidate.closed = false;

    if (candidate.arc_ids.empty()) {
      candidate.kind = coplanar_overlap_component_kind::isolated_point;
    } else if (record.classification ==
                   coplanar_facet_overlay_class::segment_contact) {
      candidate.kind = coplanar_overlap_component_kind::boundary_segment;
      if (candidate.closed)
        return false;
    } else if (record.classification ==
                   coplanar_facet_overlay_class::equal_same_orientation ||
               record.classification ==
                   coplanar_facet_overlay_class::equal_opposite_orientation) {
      candidate.kind =
          coplanar_overlap_component_kind::coincident_sheet_boundary;
      if (!candidate.closed || candidate.sheet_mask != 3)
        return false;
    } else if (record.classification ==
                   coplanar_facet_overlay_class::area_overlap ||
               record.classification ==
                   coplanar_facet_overlay_class::first_contains_second ||
               record.classification ==
                   coplanar_facet_overlay_class::second_contains_first) {
      candidate.kind = coplanar_overlap_component_kind::area_boundary;
      if (!candidate.closed)
        return false;
    } else {
      return false;
    }
    coplanar_overlap_component component;
    component.kind = candidate.kind;
    component.node_ids = std::move(candidate.node_ids);
    component.arc_ids = std::move(candidate.arc_ids);
    component.sheet_mask = candidate.sheet_mask;
    component.closed = candidate.closed;
    components.push_back(std::move(component));
  }
  if (record.classification == coplanar_facet_overlay_class::disjoint &&
      !components.empty())
    return false;
  if (record.classification == coplanar_facet_overlay_class::point_contact &&
      (components.empty() ||
       std::find_if(components.begin(), components.end(), [](const auto &value) {
         return value.kind !=
                coplanar_overlap_component_kind::isolated_point;
       }) != components.end()))
    return false;

  const auto component_less = [](const auto &a, const auto &b) {
    return std::tie(a.kind, a.node_ids, a.arc_ids, a.sheet_mask, a.closed) <
           std::tie(b.kind, b.node_ids, b.arc_ids, b.sheet_mask, b.closed);
  };
  std::sort(components.begin(), components.end(), component_less);
  for (std::size_t index = 0; index < components.size(); ++index)
    components[index].id = index;
  record.overlap_components = std::move(components);
  record.complete_overlap_component_assembly = true;
  return true;
}

template <class T>
bool build_coplanar_event_topology(
    source_facet_coplanar_overlay_record<T> &record) {
  record.event_nodes.clear();
  record.oriented_arcs.clear();
  record.overlap_components.clear();
  record.complete_event_lineage = false;
  record.complete_authorized_arc_coverage = false;
  record.complete_overlap_component_assembly = false;
  std::vector<std::vector<std::size_t>> node_by_breakpoint;
  return build_event_nodes(record, node_by_breakpoint) &&
         build_boundary_arcs(record, node_by_breakpoint) &&
         build_overlap_components(record);
}

template <class T>
std::vector<std::uint8_t> encode_event_topology(
    const source_facet_coplanar_overlay_record<T> &record) {
  canonical_writer writer;
  writer.u32(0x37545043U); // CPT7
  writer.u64(static_cast<std::uint64_t>(record.event_nodes.size()));
  for (const auto &node : record.event_nodes)
    encode_event_node(writer, node);
  writer.u64(static_cast<std::uint64_t>(record.oriented_arcs.size()));
  for (const auto &arc : record.oriented_arcs)
    encode_boundary_arc(writer, arc);
  writer.u64(static_cast<std::uint64_t>(record.overlap_components.size()));
  for (const auto &component : record.overlap_components)
    encode_overlap_component(writer, component);
  writer.boolean(record.complete_event_lineage);
  writer.boolean(record.complete_authorized_arc_coverage);
  writer.boolean(record.complete_overlap_component_assembly);
  return writer.take();
}

template <class T>
bool validate_coplanar_event_topology(
    const source_facet_coplanar_overlay_record<T> &record) {
  if (!record.complete_event_lineage ||
      !record.complete_authorized_arc_coverage ||
      !record.complete_overlap_component_assembly)
    return false;
  auto expected = record;
  if (!build_coplanar_event_topology(expected))
    return false;
  return encode_event_topology(expected) == encode_event_topology(record);
}

} // namespace coplanar_relation_overlay_detail

template <class T>
std::vector<std::uint8_t> encode_coplanar_overlay_semantics(
    const source_facet_coplanar_overlay_record<T> &record) {
  return coplanar_relation_overlay_detail::encode_semantics(record);
}

template <class T>
bool valid_coplanar_overlay_record(
    const source_facet_coplanar_overlay_record<T> &record) {
  using namespace coplanar_relation_overlay_detail;
  if (record.schema_version !=
          contract_versions::relation_coplanar_overlay_schema ||
      record.policy_version !=
          contract_versions::relation_coplanar_overlay_policy ||
      !record.owner.anchor || !valid_source_facet_relation_record(
                                  record.support_relation) ||
      !record.support_relation.owner.same_owner(record.owner) ||
      !valid_facet(record.facets[0]) || !valid_facet(record.facets[1]) ||
      !(record.facets[0].feature < record.facets[1].feature) ||
      record.support_relation.first_feature != record.facets[0].feature ||
      record.support_relation.second_feature != record.facets[1].feature ||
      (record.support_relation.classification !=
           source_facet_support_relation_class::coplanar_same_orientation &&
       record.support_relation.classification !=
           source_facet_support_relation_class::coplanar_opposite_orientation) ||
      !record.complete_boundary_pair_coverage ||
      !record.complete_vertex_coverage ||
      !record.complete_boundary_partition_coverage ||
      !record.complete_event_lineage ||
      !record.complete_authorized_arc_coverage ||
      !record.complete_overlap_component_assembly ||
      !record.distinct_sheet_occurrences ||
      record.reserved8 != 0 || record.reserved32 != 0 ||
      record.boundary_relations.size() !=
          record.facets[0].boundary_edges.size() *
              record.facets[1].boundary_edges.size() ||
      record.vertex_regions.size() !=
          record.facets[0].polygon.size() + record.facets[1].polygon.size() ||
      record.boundary_partitions.size() !=
          record.facets[0].boundary_edges.size() +
              record.facets[1].boundary_edges.size() ||
      record.facets[0].dropped_axis != record.facets[1].dropped_axis ||
      record.facets[0].dropped_axis !=
          record.support_relation.dropped_axes[0] ||
      !std::is_sorted(record.boundary_relations.begin(),
                      record.boundary_relations.end()))
    return false;
  for (std::size_t i = 0; i < record.boundary_relations.size(); ++i) {
    const auto &boundary = record.boundary_relations[i];
    if (!matching_boundary_relation(record.facets[0], record.facets[1],
                                    boundary, record.owner) ||
        (i != 0 &&
         record.boundary_relations[i - 1].first_edge_ordinal ==
             boundary.first_edge_ordinal &&
         record.boundary_relations[i - 1].second_edge_ordinal ==
             boundary.second_edge_ordinal))
      return false;
  }
  for (std::size_t i = 0; i < record.vertex_regions.size(); ++i) {
    const auto &vertex = record.vertex_regions[i];
    const auto other = static_cast<std::uint8_t>(1 - vertex.polygon);
    if (vertex.polygon > 1 || vertex.reserved != 0 ||
        vertex.vertex_ordinal >= record.facets[vertex.polygon].polygon.size() ||
        !valid_source_facet_point_region_record(vertex.region) ||
        vertex.region.query_source_identity_valid ||
        vertex.region.source_facet != record.facets[other].source_facet ||
        vertex.region.ring != record.facets[other].ring ||
        vertex.region.boundary_test_count !=
            record.facets[other].polygon.size())
      return false;
    if (i != 0) {
      const auto &prior = record.vertex_regions[i - 1];
      if (std::tie(vertex.polygon, vertex.vertex_ordinal) <=
          std::tie(prior.polygon, prior.vertex_ordinal))
        return false;
    }
  }
  std::uint64_t crossings = 0;
  std::uint64_t overlaps = 0;
  for (const auto &boundary : record.boundary_relations) {
    if (boundary.relation.contact == source_edge_contact_class::proper_crossing)
      ++crossings;
    if (is_overlap(boundary.relation.contact))
      ++overlaps;
  }
  if (crossings != record.proper_crossing_count ||
      overlaps != record.overlapping_edge_pair_count)
    return false;
  std::size_t contact_index = 0;
  for (const auto &boundary_relation : record.boundary_relations) {
    if (boundary_relation.relation.contact == source_edge_contact_class::none)
      continue;
    if (contact_index >= record.boundary_contacts.size())
      return false;
    const auto &contact = record.boundary_contacts[contact_index++];
    if (contact.request != boundary_relation.request ||
        contact.first_edge_ordinal != boundary_relation.first_edge_ordinal ||
        contact.second_edge_ordinal != boundary_relation.second_edge_ordinal ||
        contact.contact != boundary_relation.relation.contact ||
        contact.orientation != boundary_relation.relation.orientation ||
        contact.first_edge_ordinal >= record.facets[0].boundary_edges.size() ||
        contact.second_edge_ordinal >= record.facets[1].boundary_edges.size() ||
        contact.reserved16 != 0 || contact.reserved32 != 0)
      return false;
  }
  if (contact_index != record.boundary_contacts.size() ||
      !std::is_sorted(record.boundary_partitions.begin(),
                      record.boundary_partitions.end()))
    return false;
  for (std::size_t i = 0; i < record.boundary_partitions.size(); ++i) {
    const auto &entry = record.boundary_partitions[i];
    if (entry.polygon > 1 || entry.reserved8 != 0 ||
        entry.reserved16 != 0 || entry.reserved32 != 0 ||
        entry.edge_ordinal >=
            record.facets[entry.polygon].boundary_edges.size() ||
        !valid_source_facet_segment_partition_record(entry.partition) ||
        !entry.partition.triangle_reconciliation_complete)
      return false;
    const auto other = static_cast<std::uint8_t>(1 - entry.polygon);
    const auto &query = record.facets[entry.polygon];
    const auto &target = record.facets[other];
    if (entry.partition.source_facet != target.source_facet ||
        entry.partition.ring != target.ring ||
        entry.partition.boundary_edge_relation_count !=
            target.boundary_edges.size() ||
        entry.partition.segment_start_source_identity_valid ||
        entry.partition.segment_end_source_identity_valid ||
        !source_facet_region_detail::same_projected_geometry(
            entry.partition.segment_start,
            query.polygon[entry.edge_ordinal]) ||
        !source_facet_region_detail::same_projected_geometry(
            entry.partition.segment_end,
            query.polygon[(entry.edge_ordinal + 1) % query.polygon.size()]) ||
        (i != 0 &&
         std::tie(record.boundary_partitions[i - 1].polygon,
                  record.boundary_partitions[i - 1].edge_ordinal) >=
             std::tie(entry.polygon, entry.edge_ordinal)))
      return false;
  }
  if (!validate_coplanar_event_topology(record))
    return false;
  return record.semantic_digest ==
         sha256::digest(encode_coplanar_overlay_semantics(record));
}

template <class T>
boolean_outcome<source_facet_coplanar_overlay_record<T>>
classify_coplanar_facet_overlay(
    coplanar_facet_polygon_input<T> first,
    coplanar_facet_polygon_input<T> second,
    source_facet_source_facet_relation_record<T> support_relation,
    std::vector<coplanar_boundary_relation_input<T>> boundary_relations,
    const context_owner_token &owner) {
  using record_type = source_facet_coplanar_overlay_record<T>;
  using namespace coplanar_relation_overlay_detail;
  static_assert(supported_precision_scalar_v<T>);
  try {
    if (second.feature < first.feature)
      std::swap(first, second);
    if (!valid_facet(first) || !valid_facet(second) ||
        first.feature.operand == second.feature.operand ||
        !valid_source_facet_relation_record(support_relation) ||
        !support_relation.owner.same_owner(owner) ||
        support_relation.first_feature != first.feature ||
        support_relation.second_feature != second.feature ||
        (support_relation.classification !=
             source_facet_support_relation_class::coplanar_same_orientation &&
         support_relation.classification !=
             source_facet_support_relation_class::coplanar_opposite_orientation))
      return boolean_outcome<record_type>::failure(coplanar_overlay_error(
          relation_subcode::coplanar_overlay_malformed,
          "Component 07 coplanar overlay input is malformed"));

    std::sort(boundary_relations.begin(), boundary_relations.end());
    const std::uint64_t expected =
        static_cast<std::uint64_t>(first.boundary_edges.size()) *
        static_cast<std::uint64_t>(second.boundary_edges.size());
    if (boundary_relations.size() != expected)
      return boolean_outcome<record_type>::failure(coplanar_overlay_error(
          relation_subcode::coplanar_overlay_dependency_missing,
          "Component 07 coplanar overlay lacks complete boundary-pair relations"));
    for (std::size_t i = 0; i < boundary_relations.size(); ++i) {
      const auto &boundary = boundary_relations[i];
      if (!matching_boundary_relation(first, second, boundary, owner) ||
          (i != 0 &&
           boundary_relations[i - 1].first_edge_ordinal ==
               boundary.first_edge_ordinal &&
           boundary_relations[i - 1].second_edge_ordinal ==
               boundary.second_edge_ordinal))
        return boolean_outcome<record_type>::failure(coplanar_overlay_error(
            relation_subcode::coplanar_overlay_dependency_missing,
            "Component 07 coplanar overlay boundary dependency is malformed or duplicated"));
    }

    record_type record;
    record.owner = owner;
    record.support_relation = std::move(support_relation);
    record.facets = {std::move(first), std::move(second)};
    record.boundary_relations = std::move(boundary_relations);
    record.complete_boundary_pair_coverage = true;
    record.distinct_sheet_occurrences = true;

    for (std::uint8_t polygon = 0; polygon < 2; ++polygon) {
      const auto other = static_cast<std::uint8_t>(1 - polygon);
      for (std::uint64_t vertex = 0;
           vertex < record.facets[polygon].polygon.size(); ++vertex) {
        auto region = classify_source_facet_point(
            record.facets[other].source_facet, record.facets[other].ring,
            record.facets[polygon].polygon[vertex], false,
            record.facets[other].polygon, record.facets[other].orientation);
        if (!region.has_value())
          return boolean_outcome<record_type>::failure(coplanar_overlay_error(
              relation_subcode::coplanar_overlay_region_unresolved,
              "Component 07 coplanar overlay vertex classification is unresolved",
              bounded_boolean_error_category::geometric_condition_exceeds_tolerance));
        record.vertex_regions.push_back(
            {polygon, vertex, std::move(*region.value()), 0});
      }
    }
    record.complete_vertex_coverage = true;

    for (std::uint8_t polygon = 0; polygon < 2; ++polygon) {
      for (std::uint64_t edge = 0;
           edge < record.facets[polygon].boundary_edges.size(); ++edge) {
        auto partition = build_boundary_partition(
            polygon, edge, record.facets, record.boundary_relations);
        if (!partition.has_value())
          return boolean_outcome<record_type>::failure(*partition.error());
        record.boundary_partitions.push_back(
            std::move(*partition.value()));
      }
    }
    record.complete_boundary_partition_coverage = true;

    bool has_point_contact = false;
    bool has_segment_contact = false;
    for (const auto &boundary : record.boundary_relations) {
      const auto contact = boundary.relation.contact;
      if (contact == source_edge_contact_class::proper_crossing)
        ++record.proper_crossing_count;
      if (is_overlap(contact)) {
        ++record.overlapping_edge_pair_count;
        has_segment_contact = true;
      } else if (contact == source_edge_contact_class::endpoint_contact ||
                 contact == source_edge_contact_class::point_contact) {
        has_point_contact = true;
      }
      if (contact != source_edge_contact_class::none)
        record.boundary_contacts.push_back(
            {boundary.request, boundary.first_edge_ordinal,
             boundary.second_edge_ordinal, contact,
             boundary.relation.orientation, 0, 0});
    }

    std::array<std::uint64_t, 2> interior{};
    std::array<std::uint64_t, 2> outside{};
    std::array<std::uint64_t, 2> boundary{};
    for (const auto &witness : record.vertex_regions) {
      switch (witness.region.classification) {
      case source_facet_point_region_class::interior:
        ++interior[witness.polygon];
        break;
      case source_facet_point_region_class::outside:
        ++outside[witness.polygon];
        break;
      case source_facet_point_region_class::original_edge:
      case source_facet_point_region_class::original_vertex:
        ++boundary[witness.polygon];
        break;
      }
    }

    const bool all_first_boundary =
        boundary[0] == record.facets[0].polygon.size();
    const bool all_second_boundary =
        boundary[1] == record.facets[1].polygon.size();
    if (all_first_boundary && all_second_boundary) {
      if (!complete_boundary_coverage(record.facets,
                                      record.boundary_relations))
        return boolean_outcome<record_type>::failure(coplanar_overlay_error(
            relation_subcode::coplanar_overlay_boundary_incomplete,
            "Component 07 coplanar equality boundary coverage is incomplete",
            bounded_boolean_error_category::result_geometry_not_validated));
      record.classification =
          record.support_relation.classification ==
                  source_facet_support_relation_class::coplanar_same_orientation
              ? coplanar_facet_overlay_class::equal_same_orientation
              : coplanar_facet_overlay_class::equal_opposite_orientation;
    } else if (record.proper_crossing_count != 0 ||
               (interior[0] != 0 && interior[1] != 0)) {
      record.classification = coplanar_facet_overlay_class::area_overlap;
    } else {
      const bool first_in_second = outside[0] == 0 && interior[0] != 0;
      const bool second_in_first = outside[1] == 0 && interior[1] != 0;
      if (first_in_second && !second_in_first)
        record.classification =
            coplanar_facet_overlay_class::second_contains_first;
      else if (second_in_first && !first_in_second)
        record.classification =
            coplanar_facet_overlay_class::first_contains_second;
      else if (interior[0] != 0 || interior[1] != 0)
        return boolean_outcome<record_type>::failure(coplanar_overlay_error(
            relation_subcode::coplanar_overlay_boundary_incomplete,
            "Component 07 coplanar overlap topology is inconsistent without a certified crossing",
            bounded_boolean_error_category::result_geometry_not_validated));
      else if (has_segment_contact)
        record.classification = coplanar_facet_overlay_class::segment_contact;
      else if (has_point_contact)
        record.classification = coplanar_facet_overlay_class::point_contact;
      else
        record.classification = coplanar_facet_overlay_class::disjoint;
    }

    if (!build_coplanar_event_topology(record))
      return boolean_outcome<record_type>::failure(coplanar_overlay_error(
          relation_subcode::coplanar_overlay_invariant,
          "Component 07 coplanar overlay event topology is inconsistent",
          bounded_boolean_error_category::result_geometry_not_validated));

    record.semantic_digest =
        sha256::digest(encode_coplanar_overlay_semantics(record));
    if (!valid_coplanar_overlay_record(record))
      return boolean_outcome<record_type>::failure(coplanar_overlay_error(
          relation_subcode::coplanar_overlay_invariant,
          "Component 07 coplanar overlay record failed independent invariants"));
    return boolean_outcome<record_type>::success(std::move(record));
  } catch (const std::bad_alloc &) {
    return boolean_outcome<record_type>::failure(coplanar_overlay_error(
        relation_subcode::resource_preflight,
        "Component 07 coplanar overlay allocation failed",
        bounded_boolean_error_category::resource_limit));
  } catch (...) {
    return boolean_outcome<record_type>::failure(coplanar_overlay_error(
        relation_subcode::coplanar_overlay_invariant,
        "Component 07 coplanar overlay raised an unexpected exception"));
  }
}

template <class T>
bool verify_coplanar_overlay_record(
    const source_facet_coplanar_overlay_record<T> &record) {
  if (!valid_coplanar_overlay_record(record))
    return false;
  auto rebuilt = classify_coplanar_facet_overlay(
      record.facets[0], record.facets[1], record.support_relation,
      record.boundary_relations, record.owner);
  return rebuilt.has_value() &&
         encode_coplanar_overlay_semantics(*rebuilt.value()) ==
             encode_coplanar_overlay_semantics(record);
}


struct candidate_coplanar_overlay_link final {
  relation_request_id support_relation{0};
  std::uint64_t overlay_ordinal = 0;
  std::uint32_t reserved = 0;
};

template <class T> struct candidate_coplanar_overlay_stage final {
  std::uint16_t schema_version =
      contract_versions::relation_coplanar_overlay_stage_schema;
  std::uint16_t policy_version =
      contract_versions::relation_coplanar_overlay_stage_policy;
  context_owner_token owner{};
  std::vector<source_facet_coplanar_overlay_record<T>> overlays;
  std::vector<candidate_coplanar_overlay_link> links;
  std::uint64_t evaluation_count = 0;
  std::uint32_t reserved = 0;
  bounded_boolean_digest semantic_digest{};
};

namespace candidate_coplanar_overlay_detail {

template <class T, class I>
bool make_facet_polygon(
    const canonical_candidate_stream<T, I> &candidates,
    const relation_feature_key &feature, std::uint8_t dropped_axis,
    coplanar_facet_polygon_input<T> &out) {
  if (!valid_relation_feature_key(feature) ||
      feature.kind != relation_feature_kind::source_facet || dropped_axis > 2)
    return false;
  const auto *topology =
      candidate_source_edge_relation_detail::operand_topology(candidates,
                                                               feature.operand);
  if (!topology || !topology->owner().same_owner(candidates.owner()) ||
      feature.primary >= topology->source_facet_to_group().size())
    return false;
  const auto group_ordinal = topology->source_facet_to_group()[feature.primary];
  if (group_ordinal >= topology->facet_groups().size())
    return false;
  const auto &group = topology->facet_groups()[group_ordinal];
  if (group.canonical_id != group_ordinal ||
      group.source_facet != feature.primary || group.ring != feature.secondary ||
      group.source_vertices.size() < 3 ||
      group.boundary_halfedges.size() != group.source_vertices.size())
    return false;

  out = coplanar_facet_polygon_input<T>{};
  out.feature = feature;
  out.source_facet = group.source_facet;
  out.ring = group.ring;
  out.shell = group.shell;
  out.dropped_axis = dropped_axis;
  out.polygon.reserve(group.source_vertices.size());
  out.boundary_edges.reserve(group.source_vertices.size());
  std::vector<bool> consumed(group.boundary_halfedges.size(), false);
  const auto &primitive_table = candidates.primitive_table(feature.operand);
  for (std::size_t corner = 0; corner < group.source_vertices.size(); ++corner) {
    const auto source_vertex = group.source_vertices[corner];
    const auto source_destination =
        group.source_vertices[(corner + 1) % group.source_vertices.size()];
    if (source_vertex >= topology->source_vertex_to_vertex().size())
      return false;
    const auto vertex_ordinal = topology->source_vertex_to_vertex()[source_vertex];
    if (vertex_ordinal >= topology->vertices().size())
      return false;
    bounded_point3<T> point;
    if (!candidate_source_edge_relation_detail::import_vertex_point(
            topology->vertices()[vertex_ordinal], feature.operand,
            candidates.owner(), point))
      return false;
    out.polygon.push_back(source_edge_facet_detail::project_point(
        point, dropped_axis, source_vertex, corner));

    const canonical_manifold_halfedge_record *ordered = nullptr;
    std::size_t ordered_index = 0;
    for (std::size_t i = 0; i < group.boundary_halfedges.size(); ++i) {
      const auto halfedge_ordinal = group.boundary_halfedges[i];
      if (halfedge_ordinal >= topology->halfedges().size())
        return false;
      const auto &halfedge = topology->halfedges()[halfedge_ordinal];
      if (halfedge.canonical_id != halfedge_ordinal ||
          halfedge.source_facet != group.source_facet ||
          halfedge.source_origin != source_vertex ||
          halfedge.source_destination != source_destination)
        continue;
      if (ordered || consumed[i])
        return false;
      ordered = &halfedge;
      ordered_index = i;
    }
    if (!ordered || ordered->edge >= topology->edges().size())
      return false;
    consumed[ordered_index] = true;
    const auto &edge = topology->edges()[ordered->edge];
    if (edge.canonical_id != ordered->edge ||
        edge.edge_class != canonical_edge_class::source_edge ||
        !edge.source_feature_owner)
      return false;
    const auto *primitive =
        candidate_source_edge_relation_detail::find_original_edge_primitive(
            primitive_table, manifold_edge_id(edge.canonical_id));
    if (!primitive)
      return false;
    out.boundary_edges.push_back(
        candidate_source_edge_relation_detail::source_edge_feature(*primitive));
  }
  if (std::find(consumed.begin(), consumed.end(), false) != consumed.end())
    return false;
  const auto orientation =
      bounded_source_polygon_kernel<T>::polygon_orientation(out.polygon);
  if (!orientation || !valid_source_orientation_evidence(*orientation) ||
      orientation->bounded_sign == bounded_planar_sign::uncertain ||
      orientation->exact_sign != static_cast<int>(orientation->bounded_sign))
    return false;
  out.orientation = orientation->bounded_sign;
  return coplanar_relation_overlay_detail::valid_facet(out);
}

template <class T>
const canonical_relation_request *find_edge_relation(
    const candidate_source_edge_relation_stage<T> &stage,
    relation_feature_key first, relation_feature_key second,
    const source_edge_relation_record<T> *&relation) {
  if (second < first)
    std::swap(first, second);
  const canonical_relation_request *request = nullptr;
  relation = nullptr;
  for (std::size_t i = 0; i < stage.request_graph.requests.size(); ++i) {
    const auto &candidate = stage.request_graph.requests[i];
    if (candidate.key.family !=
            relation_request_family::source_edge_source_edge ||
        candidate.key.first != first || candidate.key.second != second)
      continue;
    if (request || i >= stage.relations.size())
      return nullptr;
    request = &candidate;
    relation = &stage.relations[i];
  }
  return request;
}

template <class T, class I>
bool reconstruct_overlay(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_relation_stage<T> &edge_stage,
    const source_facet_source_facet_relation_record<T> &support,
    source_facet_coplanar_overlay_record<T> &out,
    bounded_boolean_error &error) {
  std::array<coplanar_facet_polygon_input<T>, 2> facets;
  if (!make_facet_polygon(candidates, support.first_feature,
                          support.dropped_axes[0], facets[0]) ||
      !make_facet_polygon(candidates, support.second_feature,
                          support.dropped_axes[0], facets[1])) {
    error = coplanar_overlay_error(
        relation_subcode::coplanar_overlay_malformed,
        "Component 07 could not reconstruct a complete coplanar source boundary");
    return false;
  }
  std::vector<coplanar_boundary_relation_input<T>> dependencies;
  const auto first_count = facets[0].boundary_edges.size();
  const auto second_count = facets[1].boundary_edges.size();
  if (first_count != 0 &&
      second_count > std::numeric_limits<std::size_t>::max() / first_count) {
    error = coplanar_overlay_error(
        relation_subcode::byte_count_overflow,
        "Component 07 coplanar boundary-pair count overflow",
        bounded_boolean_error_category::index_overflow);
    return false;
  }
  dependencies.reserve(first_count * second_count);
  for (std::size_t first = 0; first < first_count; ++first)
    for (std::size_t second = 0; second < second_count; ++second) {
      const source_edge_relation_record<T> *relation = nullptr;
      const auto *request = find_edge_relation(
          edge_stage, facets[0].boundary_edges[first],
          facets[1].boundary_edges[second], relation);
      if (!request || !relation) {
        error = coplanar_overlay_error(
            relation_subcode::coplanar_overlay_dependency_missing,
            "Component 07 candidate-derived coplanar overlay lacks a boundary relation");
        return false;
      }
      dependencies.push_back(
          {request->id, first, second, *relation, 0});
    }
  auto classified = classify_coplanar_facet_overlay(
      std::move(facets[0]), std::move(facets[1]), support,
      std::move(dependencies), candidates.owner());
  if (!classified.has_value()) {
    error = *classified.error();
    return false;
  }
  out = std::move(*classified.value());
  return true;
}

} // namespace candidate_coplanar_overlay_detail

template <class T>
std::vector<std::uint8_t> encode_candidate_coplanar_overlay_semantics(
    const candidate_coplanar_overlay_stage<T> &stage) {
  canonical_writer writer;
  writer.u32(0x374F4343U); // CCO7
  writer.u16(stage.schema_version);
  writer.u16(stage.policy_version);
  writer.u64(static_cast<std::uint64_t>(stage.overlays.size()));
  for (const auto &overlay : stage.overlays)
    writer.sized_bytes(encode_coplanar_overlay_semantics(overlay));
  writer.u64(static_cast<std::uint64_t>(stage.links.size()));
  for (const auto &link : stage.links) {
    writer.u64(link.support_relation.ordinal());
    writer.u64(link.overlay_ordinal);
    writer.u32(link.reserved);
  }
  writer.u64(stage.evaluation_count);
  writer.u32(stage.reserved);
  return writer.take();
}

template <class T, class I>
bool verify_candidate_coplanar_overlay_stage(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_relation_stage<T> &edge_stage,
    const candidate_source_facet_relation_stage<T> &facet_stage,
    const candidate_coplanar_overlay_stage<T> &stage,
    bounded_boolean_error &error) {
  using namespace candidate_coplanar_overlay_detail;
  const auto fail = [&](relation_subcode subcode, const char *summary) {
    error = coplanar_overlay_error(subcode, summary);
    return false;
  };
  if (stage.schema_version !=
          contract_versions::relation_coplanar_overlay_stage_schema ||
      stage.policy_version !=
          contract_versions::relation_coplanar_overlay_stage_policy ||
      edge_stage.schema_version !=
          contract_versions::relation_source_edge_edge_schema ||
      edge_stage.policy_version !=
          contract_versions::relation_source_edge_edge_policy ||
      facet_stage.schema_version !=
          contract_versions::relation_source_facet_facet_stage_schema ||
      facet_stage.policy_version !=
          contract_versions::relation_source_facet_facet_stage_policy ||
      !stage.owner.same_owner(candidates.owner()) ||
      !stage.owner.same_owner(edge_stage.owner) ||
      !stage.owner.same_owner(facet_stage.owner) ||
      !edge_stage.request_graph.owner.same_owner(stage.owner) ||
      !facet_stage.request_graph.owner.same_owner(stage.owner) ||
      edge_stage.relations.size() != edge_stage.request_graph.requests.size() ||
      facet_stage.relations.size() !=
          facet_stage.request_graph.requests.size() ||
      stage.reserved != 0 || stage.evaluation_count != stage.overlays.size() ||
      stage.links.size() != stage.overlays.size())
    return fail(relation_subcode::coplanar_overlay_invariant,
                "Component 07 candidate coplanar overlay header is malformed");

  std::size_t overlay = 0;
  for (std::size_t relation = 0; relation < facet_stage.relations.size();
       ++relation) {
    const auto &support = facet_stage.relations[relation];
    const bool coplanar =
        support.classification ==
            source_facet_support_relation_class::coplanar_same_orientation ||
        support.classification ==
            source_facet_support_relation_class::coplanar_opposite_orientation;
    if (!coplanar)
      continue;
    if (relation >= facet_stage.request_graph.requests.size() ||
        overlay >= stage.overlays.size())
      return fail(relation_subcode::coplanar_overlay_invariant,
                  "Component 07 candidate coplanar overlay links are incomplete");
    const auto &support_request =
        facet_stage.request_graph.requests[relation];
    const auto &link = stage.links[overlay];
    if (support_request.id.ordinal() != relation ||
        support_request.key.family !=
            relation_request_family::source_facet_source_facet ||
        support_request.key.scope !=
            relation_record_scope::public_source_feature ||
        !valid_source_facet_relation_record(support) ||
        link.support_relation != support_request.id ||
        link.overlay_ordinal != overlay || link.reserved != 0 ||
        !stage.overlays[overlay].owner.same_owner(stage.owner) ||
        !verify_coplanar_overlay_record(stage.overlays[overlay]))
      return fail(relation_subcode::coplanar_overlay_invariant,
                  "Component 07 candidate coplanar overlay link is malformed");
    source_facet_coplanar_overlay_record<T> reconstructed;
    if (!reconstruct_overlay(candidates, edge_stage, support, reconstructed,
                             error))
      return false;
    if (encode_coplanar_overlay_semantics(reconstructed) !=
        encode_coplanar_overlay_semantics(stage.overlays[overlay]))
      return fail(relation_subcode::coplanar_overlay_invariant,
                  "Component 07 candidate coplanar overlay did not reconstruct");
    ++overlay;
  }
  if (overlay != stage.overlays.size())
    return fail(relation_subcode::coplanar_overlay_invariant,
                "Component 07 candidate coplanar overlay has trailing records");
  if (stage.semantic_digest != sha256::digest(
          encode_candidate_coplanar_overlay_semantics(stage)))
    return fail(relation_subcode::digest_mismatch,
                "Component 07 candidate coplanar overlay digest mismatch");
  auto owner_changed = stage;
  owner_changed.owner = context_owner_token::create();
  if (encode_candidate_coplanar_overlay_semantics(owner_changed) !=
      encode_candidate_coplanar_overlay_semantics(stage))
    return fail(relation_subcode::owner_in_semantics,
                "Component 07 candidate coplanar overlay encoded a runtime owner");
  return true;
}

template <class T, class I>
boolean_outcome<candidate_coplanar_overlay_stage<T>>
build_candidate_coplanar_overlays(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_relation_stage<T> &edge_stage,
    const candidate_source_facet_relation_stage<T> &facet_stage,
    const relation_capabilities &capabilities) {
  using stage_type = candidate_coplanar_overlay_stage<T>;
  using namespace candidate_coplanar_overlay_detail;
  try {
    if (!capabilities.owner.anchor ||
        !capabilities.owner.same_owner(candidates.owner()) ||
        !capabilities.owner.same_owner(edge_stage.owner) ||
        !capabilities.owner.same_owner(facet_stage.owner) ||
        edge_stage.relations.size() != edge_stage.request_graph.requests.size() ||
        facet_stage.relations.size() !=
            facet_stage.request_graph.requests.size())
      return boolean_outcome<stage_type>::failure(coplanar_overlay_error(
          relation_subcode::coplanar_overlay_malformed,
          "Component 07 candidate coplanar overlay handshake failed"));
    stage_type stage;
    stage.owner = capabilities.owner;
    for (std::size_t relation = 0; relation < facet_stage.relations.size();
         ++relation) {
      if (relation_cancelled(capabilities))
        return boolean_outcome<stage_type>::failure(coplanar_overlay_error(
            relation_subcode::cancelled,
            "Component 07 candidate coplanar overlay cancelled",
            bounded_boolean_error_category::cancelled));
      const auto &support = facet_stage.relations[relation];
      if (relation >= facet_stage.request_graph.requests.size() ||
          facet_stage.request_graph.requests[relation].id.ordinal() != relation ||
          facet_stage.request_graph.requests[relation].key.family !=
              relation_request_family::source_facet_source_facet ||
          !valid_source_facet_relation_record(support))
        return boolean_outcome<stage_type>::failure(coplanar_overlay_error(
            relation_subcode::coplanar_overlay_malformed,
            "Component 07 candidate coplanar support producer is malformed"));
      if (support.classification !=
              source_facet_support_relation_class::coplanar_same_orientation &&
          support.classification !=
              source_facet_support_relation_class::coplanar_opposite_orientation)
        continue;
      if (stage.overlays.size() >= capabilities.maximum_relations)
        return boolean_outcome<stage_type>::failure(coplanar_overlay_error(
            relation_subcode::work_limit,
            "Component 07 candidate coplanar overlay relation limit exceeded",
            bounded_boolean_error_category::resource_limit));
      source_facet_coplanar_overlay_record<T> overlay;
      bounded_boolean_error build_error;
      if (!reconstruct_overlay(candidates, edge_stage, support, overlay,
                               build_error))
        return boolean_outcome<stage_type>::failure(build_error);
      stage.links.push_back(
          {facet_stage.request_graph.requests[relation].id,
           static_cast<std::uint64_t>(stage.overlays.size()), 0});
      stage.overlays.push_back(std::move(overlay));
      ++stage.evaluation_count;
    }
    const auto bytes = encode_candidate_coplanar_overlay_semantics(stage);
    if (bytes.size() > capabilities.maximum_canonical_bytes)
      return boolean_outcome<stage_type>::failure(coplanar_overlay_error(
          relation_subcode::byte_count_overflow,
          "Component 07 candidate coplanar overlay bytes exceed capabilities",
          bounded_boolean_error_category::resource_limit));
    stage.semantic_digest = sha256::digest(bytes);
    bounded_boolean_error verification_error;
    if (!verify_candidate_coplanar_overlay_stage(
            candidates, edge_stage, facet_stage, stage, verification_error))
      return boolean_outcome<stage_type>::failure(verification_error);
    return boolean_outcome<stage_type>::success(std::move(stage));
  } catch (const std::bad_alloc &) {
    return boolean_outcome<stage_type>::failure(coplanar_overlay_error(
        relation_subcode::resource_preflight,
        "Component 07 candidate coplanar overlay allocation failed",
        bounded_boolean_error_category::resource_limit));
  } catch (...) {
    return boolean_outcome<stage_type>::failure(coplanar_overlay_error(
        relation_subcode::coplanar_overlay_invariant,
        "Component 07 candidate coplanar overlay raised an unexpected exception"));
  }
}

} // namespace ygor::mesh_boolean::bounded
