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
  coplanar_facet_overlay_class classification =
      coplanar_facet_overlay_class::disjoint;
  bool complete_boundary_pair_coverage = false;
  bool complete_vertex_coverage = false;
  bool complete_boundary_partition_coverage = false;
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
  writer.u8(static_cast<std::uint8_t>(record.classification));
  writer.boolean(record.complete_boundary_pair_coverage);
  writer.boolean(record.complete_vertex_coverage);
  writer.boolean(record.complete_boundary_partition_coverage);
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
