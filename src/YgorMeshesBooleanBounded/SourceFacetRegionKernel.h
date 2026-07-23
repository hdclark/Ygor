#pragma once

#include "BoundedSourcePolygonKernel.h"
#include "Outcome.h"
#include "RelationTypes.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

enum class source_facet_point_region_class : std::uint8_t {
  interior = 1,
  original_edge = 2,
  original_vertex = 3,
  outside = 4,
};

struct source_facet_boundary_edge_owner final {
  std::uint64_t edge_ordinal = 0;
  std::uint64_t origin_source_vertex = 0;
  std::uint64_t destination_source_vertex = 0;

  friend bool operator<(const source_facet_boundary_edge_owner &a,
                        const source_facet_boundary_edge_owner &b) noexcept {
    return std::tie(a.edge_ordinal, a.origin_source_vertex,
                    a.destination_source_vertex) <
           std::tie(b.edge_ordinal, b.origin_source_vertex,
                    b.destination_source_vertex);
  }
  friend bool operator==(const source_facet_boundary_edge_owner &a,
                         const source_facet_boundary_edge_owner &b) noexcept {
    return a.edge_ordinal == b.edge_ordinal &&
           a.origin_source_vertex == b.origin_source_vertex &&
           a.destination_source_vertex == b.destination_source_vertex;
  }
};

template <class T> struct source_facet_point_region_record final {
  std::uint16_t schema_version =
      contract_versions::relation_source_facet_region_schema;
  std::uint16_t policy_version =
      contract_versions::relation_source_facet_region_policy;
  source_facet_point_region_class classification =
      source_facet_point_region_class::outside;
  std::uint64_t source_facet = 0;
  std::uint64_t ring = 0;
  std::uint8_t sweep_axis = 1;
  bool query_source_identity_valid = false;
  bool complete_boundary_traversal = false;
  bool boundary_ownership_resolved = false;
  std::uint64_t boundary_test_count = 0;
  std::uint64_t parity_crossing_count = 0;
  std::vector<std::uint64_t> source_vertex_owners;
  std::vector<source_facet_boundary_edge_owner> source_edge_owners;
  source_orientation_evidence<T> polygon_orientation_evidence{};
  std::vector<source_orientation_evidence<T>> orientation_evidence;
  std::uint32_t reserved = 0;
};

template <class T>
bool valid_source_facet_point_region_record(
    const source_facet_point_region_record<T> &record) noexcept {
  if (record.schema_version !=
          contract_versions::relation_source_facet_region_schema ||
      record.policy_version !=
          contract_versions::relation_source_facet_region_policy ||
      record.sweep_axis != 1 || !record.complete_boundary_traversal ||
      !record.boundary_ownership_resolved || record.reserved != 0 ||
      record.boundary_test_count != record.orientation_evidence.size() ||
      record.polygon_orientation_evidence.formula_version !=
          contract_versions::bounded_source_polygon_kernel ||
      (record.polygon_orientation_evidence.exact_sign != -1 &&
       record.polygon_orientation_evidence.exact_sign != 1))
    return false;

  if (!std::is_sorted(record.source_vertex_owners.begin(),
                      record.source_vertex_owners.end()) ||
      std::adjacent_find(record.source_vertex_owners.begin(),
                         record.source_vertex_owners.end()) !=
          record.source_vertex_owners.end() ||
      !std::is_sorted(record.source_edge_owners.begin(),
                      record.source_edge_owners.end()) ||
      std::adjacent_find(record.source_edge_owners.begin(),
                         record.source_edge_owners.end()) !=
          record.source_edge_owners.end())
    return false;

  for (const auto &edge : record.source_edge_owners) {
    if (edge.edge_ordinal >= record.boundary_test_count ||
        edge.origin_source_vertex == edge.destination_source_vertex)
      return false;
  }

  switch (record.classification) {
  case source_facet_point_region_class::interior:
  case source_facet_point_region_class::outside:
    return record.source_vertex_owners.empty() &&
           record.source_edge_owners.empty();
  case source_facet_point_region_class::original_edge:
    return record.source_vertex_owners.empty() &&
           !record.source_edge_owners.empty();
  case source_facet_point_region_class::original_vertex:
    return record.source_vertex_owners.size() == 1 &&
           record.source_edge_owners.size() >= 2;
  }
  return false;
}

bounded_boolean_error source_facet_region_error(
    relation_subcode subcode, const char *summary,
    relation_checkpoint checkpoint =
        relation_checkpoint::source_facet_region_evaluation);

namespace source_facet_region_detail {

enum class interval_position : std::int8_t {
  below = -1,
  equal = 0,
  above = 1,
  uncertain = 2,
};

template <class T>
bool valid_projected_point(const projected_source_point<T> &point) noexcept {
  for (std::size_t axis = 0; axis < 2; ++axis) {
    const auto &interval = point.enclosure[axis];
    if (!finite_bits(point.nominal[axis]) ||
        !finite_bits(interval.lower()) || !finite_bits(interval.upper()) ||
        finite_numeric_less(interval.upper(), interval.lower()) ||
        !interval.contains(point.nominal[axis]))
      return false;
  }
  return true;
}

template <class T>
bool same_projected_geometry(const projected_source_point<T> &a,
                             const projected_source_point<T> &b) noexcept {
  for (std::size_t axis = 0; axis < 2; ++axis) {
    if (to_bits(a.nominal[axis]) != to_bits(b.nominal[axis]) ||
        to_bits(a.enclosure[axis].lower()) !=
            to_bits(b.enclosure[axis].lower()) ||
        to_bits(a.enclosure[axis].upper()) !=
            to_bits(b.enclosure[axis].upper()))
      return false;
  }
  return true;
}

template <class T>
bool singleton(const finite_interval<T> &interval) noexcept {
  return to_bits(interval.lower()) == to_bits(interval.upper());
}

template <class T>
bool singleton(const projected_source_point<T> &point) noexcept {
  return singleton(point.enclosure[0]) && singleton(point.enclosure[1]);
}

template <class T>
interval_position compare_interval_to_query(const finite_interval<T> &value,
                                            const finite_interval<T> &query) {
  if (finite_numeric_less(value.upper(), query.lower()))
    return interval_position::below;
  if (finite_numeric_less(query.upper(), value.lower()))
    return interval_position::above;
  if (singleton(value) && singleton(query) &&
      to_bits(value.lower()) == to_bits(query.lower()))
    return interval_position::equal;
  return interval_position::uncertain;
}

template <class T>
bool point_interval_inside_edge_box(const projected_source_point<T> &point,
                                    const projected_source_point<T> &a,
                                    const projected_source_point<T> &b) {
  for (std::size_t axis = 0; axis < 2; ++axis) {
    const T low = finite_numeric_less(a.enclosure[axis].lower(),
                                     b.enclosure[axis].lower())
                      ? a.enclosure[axis].lower()
                      : b.enclosure[axis].lower();
    const T high = finite_numeric_less(a.enclosure[axis].upper(),
                                      b.enclosure[axis].upper())
                       ? b.enclosure[axis].upper()
                       : a.enclosure[axis].upper();
    if (finite_numeric_less(point.enclosure[axis].lower(), low) ||
        finite_numeric_less(high, point.enclosure[axis].upper()))
      return false;
  }
  return true;
}

template <class T>
bool definite_point_on_edge(const projected_source_point<T> &point,
                            const projected_source_point<T> &a,
                            const projected_source_point<T> &b,
                            const source_orientation_evidence<T> &orientation) {
  return orientation.exact_sign == 0 &&
         orientation.determinant.lower() == T(0) &&
         orientation.determinant.upper() == T(0) &&
         point_interval_inside_edge_box(point, a, b);
}

template <class T>
int certified_orientation_sign(const source_orientation_evidence<T> &evidence,
                               const projected_source_point<T> &a,
                               const projected_source_point<T> &b,
                               const projected_source_point<T> &point) {
  if (evidence.bounded_sign == bounded_planar_sign::negative)
    return -1;
  if (evidence.bounded_sign == bounded_planar_sign::positive)
    return 1;
  if (evidence.exact_sign != 0 && singleton(a) && singleton(b) &&
      singleton(point))
    return evidence.exact_sign;
  return 0;
}

inline void canonicalize_owners(
    std::vector<std::uint64_t> &vertices,
    std::vector<source_facet_boundary_edge_owner> &edges) {
  std::sort(vertices.begin(), vertices.end());
  vertices.erase(std::unique(vertices.begin(), vertices.end()),
                 vertices.end());
  std::sort(edges.begin(), edges.end());
  edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
}

} // namespace source_facet_region_detail

template <class T>
boolean_outcome<source_facet_point_region_record<T>>
classify_source_facet_point(
    std::uint64_t source_facet, std::uint64_t ring,
    const projected_source_point<T> &query,
    bool query_source_identity_valid,
    const std::vector<projected_source_point<T>> &polygon,
    bounded_planar_sign polygon_orientation) {
  static_assert(supported_precision_scalar_v<T>);
  using namespace source_facet_region_detail;

  if (polygon.size() < 3 ||
      polygon_orientation == bounded_planar_sign::uncertain ||
      !valid_projected_point(query))
    return boolean_outcome<source_facet_point_region_record<T>>::failure(
        source_facet_region_error(
            relation_subcode::malformed_source_polygon,
            "Component 07 source-facet point query or polygon is malformed"));

  std::vector<std::uint64_t> polygon_vertices;
  polygon_vertices.reserve(polygon.size());
  for (const auto &point : polygon) {
    if (!valid_projected_point(point))
      return boolean_outcome<source_facet_point_region_record<T>>::failure(
          source_facet_region_error(
              relation_subcode::malformed_source_polygon,
              "Component 07 source-facet boundary contains an invalid projected point"));
    polygon_vertices.push_back(point.source_vertex);
  }
  std::sort(polygon_vertices.begin(), polygon_vertices.end());
  if (std::adjacent_find(polygon_vertices.begin(), polygon_vertices.end()) !=
      polygon_vertices.end())
    return boolean_outcome<source_facet_point_region_record<T>>::failure(
        source_facet_region_error(
            relation_subcode::malformed_source_polygon,
            "Component 07 source-facet boundary repeats a source vertex identity"));

  const auto orientation =
      bounded_source_polygon_kernel<T>::polygon_orientation(polygon);
  const int expected_orientation = static_cast<int>(polygon_orientation);
  if (!orientation || orientation->exact_sign != expected_orientation ||
      (orientation->bounded_sign != bounded_planar_sign::uncertain &&
       orientation->bounded_sign != polygon_orientation))
    return boolean_outcome<source_facet_point_region_record<T>>::failure(
        source_facet_region_error(
            relation_subcode::malformed_source_polygon,
            "Component 07 frozen facet orientation disagrees with the complete polygon"));

  source_facet_point_region_record<T> result;
  result.source_facet = source_facet;
  result.ring = ring;
  result.query_source_identity_valid = query_source_identity_valid;
  result.polygon_orientation_evidence = *orientation;
  result.orientation_evidence.reserve(polygon.size());

  std::uint64_t identity_matches = 0;
  bool unresolved_boundary = false;
  for (std::size_t i = 0; i < polygon.size(); ++i) {
    const auto &a = polygon[i];
    const auto &b = polygon[(i + 1) % polygon.size()];
    if (!valid_projected_point(a) || !valid_projected_point(b) ||
        a.source_vertex == b.source_vertex)
      return boolean_outcome<source_facet_point_region_record<T>>::failure(
          source_facet_region_error(
              relation_subcode::malformed_source_polygon,
              "Component 07 source-facet boundary contains invalid points or a zero-length topological edge"));

    if (query_source_identity_valid &&
        query.source_vertex == a.source_vertex) {
      if (!same_projected_geometry(query, a))
        return boolean_outcome<source_facet_point_region_record<T>>::failure(
            source_facet_region_error(
                relation_subcode::source_facet_boundary_ownership,
                "Component 07 source vertex identity disagrees with projected geometry"));
      ++identity_matches;
      result.source_vertex_owners.push_back(a.source_vertex);
    }

    const auto evidence =
        bounded_source_polygon_kernel<T>::orientation(a, b, query);
    result.orientation_evidence.push_back(evidence);
    ++result.boundary_test_count;

    const bool identity_owned =
        query_source_identity_valid &&
        (query.source_vertex == a.source_vertex ||
         query.source_vertex == b.source_vertex);
    const bool geometric_owned =
        definite_point_on_edge(query, a, b, evidence);
    if (identity_owned || geometric_owned) {
      result.source_edge_owners.push_back(
          {static_cast<std::uint64_t>(i), a.source_vertex,
           b.source_vertex});
    } else if (evidence.exact_sign == 0 &&
               !source_polygon_kernel_detail::point_box_disjoint(query, a,
                                                                  b)) {
      unresolved_boundary = true;
    }
  }

  if (identity_matches > 1)
    return boolean_outcome<source_facet_point_region_record<T>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_boundary_ownership,
            "Component 07 source vertex identity occurs more than once in one facet ring"));

  canonicalize_owners(result.source_vertex_owners, result.source_edge_owners);
  result.complete_boundary_traversal = true;
  if (!result.source_vertex_owners.empty()) {
    result.classification = source_facet_point_region_class::original_vertex;
    result.boundary_ownership_resolved = true;
    if (!valid_source_facet_point_region_record(result))
      return boolean_outcome<source_facet_point_region_record<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_boundary_ownership,
              "Component 07 source-vertex region record is inconsistent"));
    return boolean_outcome<source_facet_point_region_record<T>>::success(
        std::move(result));
  }
  if (!result.source_edge_owners.empty()) {
    result.classification = source_facet_point_region_class::original_edge;
    result.boundary_ownership_resolved = true;
    if (!valid_source_facet_point_region_record(result))
      return boolean_outcome<source_facet_point_region_record<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_boundary_ownership,
              "Component 07 source-edge region record is inconsistent"));
    return boolean_outcome<source_facet_point_region_record<T>>::success(
        std::move(result));
  }
  if (unresolved_boundary)
    return boolean_outcome<source_facet_point_region_record<T>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_region_unresolved,
            "Component 07 source-facet boundary ownership is unresolved"));

  bool inside = false;
  for (std::size_t i = 0; i < polygon.size(); ++i) {
    const auto &a = polygon[i];
    const auto &b = polygon[(i + 1) % polygon.size()];
    const auto a_position =
        compare_interval_to_query(a.enclosure[1], query.enclosure[1]);
    const auto b_position =
        compare_interval_to_query(b.enclosure[1], query.enclosure[1]);

    if (a_position == interval_position::uncertain ||
        b_position == interval_position::uncertain)
      return boolean_outcome<source_facet_point_region_record<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_region_unresolved,
              "Component 07 half-open source-facet sweep ordering is unresolved"));

    const bool upward =
        (a_position == interval_position::below ||
         a_position == interval_position::equal) &&
        b_position == interval_position::above;
    const bool downward =
        (b_position == interval_position::below ||
         b_position == interval_position::equal) &&
        a_position == interval_position::above;
    if (!upward && !downward)
      continue;

    const int orientation_sign = certified_orientation_sign(
        result.orientation_evidence[i], a, b, query);
    if (orientation_sign == 0)
      return boolean_outcome<source_facet_point_region_record<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_region_unresolved,
              "Component 07 source-facet sweep crossing orientation is unresolved"));

    const bool crosses_positive_ray =
        (upward && orientation_sign > 0) ||
        (downward && orientation_sign < 0);
    if (crosses_positive_ray) {
      inside = !inside;
      ++result.parity_crossing_count;
    }
  }

  result.classification = inside
                              ? source_facet_point_region_class::interior
                              : source_facet_point_region_class::outside;
  result.boundary_ownership_resolved = true;
  if (!valid_source_facet_point_region_record(result))
    return boolean_outcome<source_facet_point_region_record<T>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_region_unresolved,
            "Component 07 interior/exterior region record is inconsistent"));
  return boolean_outcome<source_facet_point_region_record<T>>::success(
      std::move(result));
}

} // namespace ygor::mesh_boolean::bounded
