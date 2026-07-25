#pragma once

#include "FacetFacetRelations.h"
#include "SourceEdgeRelationKernel.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
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
  coplanar_facet_overlay_class classification =
      coplanar_facet_overlay_class::disjoint;
  bool complete_boundary_pair_coverage = false;
  bool complete_vertex_coverage = false;
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
  writer.u8(static_cast<std::uint8_t>(record.classification));
  writer.boolean(record.complete_boundary_pair_coverage);
  writer.boolean(record.complete_vertex_coverage);
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
      !record.complete_vertex_coverage || !record.distinct_sheet_occurrences ||
      record.reserved8 != 0 || record.reserved32 != 0 ||
      record.boundary_relations.size() !=
          record.facets[0].boundary_edges.size() *
              record.facets[1].boundary_edges.size() ||
      record.vertex_regions.size() !=
          record.facets[0].polygon.size() + record.facets[1].polygon.size() ||
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
  if (contact_index != record.boundary_contacts.size())
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

} // namespace ygor::mesh_boolean::bounded
