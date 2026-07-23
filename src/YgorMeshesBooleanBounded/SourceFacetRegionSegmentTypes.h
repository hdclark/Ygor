#pragma once

#include "SourceFacetRegionKernel.h"
#include "CanonicalBytes.h"
#include "Sha256.h"

#include <cmath>

namespace ygor::mesh_boolean::bounded {

namespace source_facet_region_detail {
template <class T>
bool interval_valid(const finite_interval<T> &interval) noexcept {
  return finite_bits(interval.lower()) && finite_bits(interval.upper()) &&
         !finite_numeric_less(interval.upper(), interval.lower());
}

template <class T>
bool interval_equal_bits(const finite_interval<T> &a,
                         const finite_interval<T> &b) noexcept {
  return to_bits(a.lower()) == to_bits(b.lower()) &&
         to_bits(a.upper()) == to_bits(b.upper());
}

template <class T>
bool unit_parameter(const finite_interval<T> &parameter, T nominal) noexcept {
  return interval_valid(parameter) && finite_bits(nominal) &&
         parameter.contains(nominal) &&
         !finite_numeric_less(parameter.lower(), T(0)) &&
         !finite_numeric_less(T(1), parameter.upper());
}

template <class T>
bool definitely_before(const finite_interval<T> &a,
                       const finite_interval<T> &b) noexcept {
  return finite_numeric_less(a.upper(), b.lower());
}

template <class T>
bool intervals_overlap(const finite_interval<T> &a,
                       const finite_interval<T> &b) noexcept {
  return !definitely_before(a, b) && !definitely_before(b, a);
}

template <class T>
bool point_strictly_between(const finite_interval<T> &parameter,
                            const finite_interval<T> &left,
                            const finite_interval<T> &right) noexcept {
  return finite_numeric_less(left.upper(), parameter.lower()) &&
         finite_numeric_less(parameter.upper(), right.lower());
}

template <class T>
bool parameter_inside_open_interval(const finite_interval<T> &parameter,
                                    const finite_interval<T> &first,
                                    const finite_interval<T> &second) noexcept {
  return finite_numeric_less(first.upper(), parameter.lower()) &&
         finite_numeric_less(parameter.upper(), second.lower());
}

template <class T>
void encode_interval(canonical_writer &writer,
                     const finite_interval<T> &interval) {
  writer.floating(interval.lower());
  writer.floating(interval.upper());
}

template <class T>
void encode_projected_point(canonical_writer &writer,
                            const projected_source_point<T> &point) {
  writer.u64(point.source_vertex);
  writer.u64(point.source_corner);
  for (std::size_t axis = 0; axis < 2; ++axis) {
    writer.floating(point.nominal[axis]);
    encode_interval(writer, point.enclosure[axis]);
  }
}

inline void encode_edge_owner(canonical_writer &writer,
                              const source_facet_boundary_edge_owner &owner) {
  writer.u64(owner.edge_ordinal);
  writer.u64(owner.origin_source_vertex);
  writer.u64(owner.destination_source_vertex);
}

template <class T>
boolean_outcome<projected_source_point<T>>
interpolate_projected_segment(const projected_source_point<T> &a,
                              const projected_source_point<T> &b,
                              T nominal_parameter,
                              const finite_interval<T> &parameter) {
  if (!valid_projected_point(a) || !valid_projected_point(b) ||
      !unit_parameter(parameter, nominal_parameter))
    return boolean_outcome<projected_source_point<T>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_malformed,
            "Component 07 projected segment interpolation input is malformed"));

  projected_source_point<T> result;
  const auto one = finite_interval<T>::checked_singleton(T(1));
  if (!one)
    return boolean_outcome<projected_source_point<T>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_partition_unresolved,
            "Component 07 could not create the unit parameter interval"));
  const auto one_minus = interval_subtract(*one, parameter);
  if (!one_minus)
    return boolean_outcome<projected_source_point<T>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_partition_unresolved,
            "Component 07 segment witness complement interval is unavailable"));

  for (std::size_t axis = 0; axis < 2; ++axis) {
    // Exact constant-coordinate lineage must remain exact.  Applying the
    // generic interpolation graph to equal singleton endpoints would add a
    // spurious rounding enclosure and can turn a certified boundary carrier
    // into an unresolved near-boundary query.
    if (singleton(a.enclosure[axis]) && singleton(b.enclosure[axis]) &&
        to_bits(a.nominal[axis]) == to_bits(b.nominal[axis]) &&
        to_bits(a.enclosure[axis].lower()) ==
            to_bits(b.enclosure[axis].lower())) {
      result.nominal[axis] = a.nominal[axis];
      result.enclosure[axis] = a.enclosure[axis];
      continue;
    }

    const auto left = interval_multiply(*one_minus.value, a.enclosure[axis]);
    const auto right = interval_multiply(parameter, b.enclosure[axis]);
    if (!left || !right)
      return boolean_outcome<projected_source_point<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_segment_partition_unresolved,
              "Component 07 segment witness interval multiplication failed"));
    const auto enclosure = interval_add(*left.value, *right.value);
    if (!enclosure)
      return boolean_outcome<projected_source_point<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_segment_partition_unresolved,
              "Component 07 segment witness interval addition failed"));
    result.enclosure[axis] = *enclosure.value;

    const auto delta = directed_subtract(b.nominal[axis], a.nominal[axis]);
    if (!delta)
      return boolean_outcome<projected_source_point<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_segment_partition_unresolved,
              "Component 07 segment witness nominal subtraction failed"));
    const auto scaled =
        directed_multiply(nominal_parameter, delta.value.rounded);
    if (!scaled)
      return boolean_outcome<projected_source_point<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_segment_partition_unresolved,
              "Component 07 segment witness nominal multiplication failed"));
    const auto nominal = directed_add(a.nominal[axis], scaled.value.rounded);
    if (!nominal)
      return boolean_outcome<projected_source_point<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_segment_partition_unresolved,
              "Component 07 segment witness nominal addition failed"));
    result.nominal[axis] = nominal.value.rounded;
    if (!result.enclosure[axis].contains(result.nominal[axis]))
      return boolean_outcome<projected_source_point<T>>::failure(
          source_facet_region_error(
              relation_subcode::source_facet_segment_partition_unresolved,
              "Component 07 segment witness nominal escaped its enclosure"));
  }

  if (!valid_projected_point(result))
    return boolean_outcome<projected_source_point<T>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_partition_unresolved,
            "Component 07 segment witness construction is invalid"));
  return boolean_outcome<projected_source_point<T>>::success(
      std::move(result));
}

template <class T>
bool parameter_matches_projected_point(
    const projected_source_point<T> &segment_start,
    const projected_source_point<T> &segment_end,
    T rounded_parameter, const finite_interval<T> &parameter,
    const projected_source_point<T> &point) {
  auto reconstructed = interpolate_projected_segment(
      segment_start, segment_end, rounded_parameter, parameter);
  if (!reconstructed.has_value())
    return false;
  for (std::size_t axis = 0; axis < 2; ++axis)
    if (!intervals_overlap(
            reconstructed.value()->enclosure[axis],
            point.enclosure[axis]))
      return false;
  return true;
}

inline bool nonzero_digest(
    const bounded_boolean_digest &digest) noexcept {
  return std::any_of(digest.bytes.begin(), digest.bytes.end(),
                     [](std::uint8_t byte) { return byte != 0; });
}

template <class T>
boolean_outcome<std::pair<T, finite_interval<T>>>
dyadic_parameter(T left, T right, std::uint32_t depth,
                 std::uint32_t numerator) {
  if (!finite_bits(left) || !finite_bits(right) ||
      !finite_numeric_less(left, right) || depth == 0 || depth > 6 ||
      numerator == 0 || numerator >= (std::uint32_t{1} << depth) ||
      (numerator & 1U) == 0)
    return boolean_outcome<std::pair<T, finite_interval<T>>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_partition_unresolved,
            "Component 07 dyadic witness request is invalid"));

  const T weight =
      std::ldexp(static_cast<T>(numerator), -static_cast<int>(depth));
  const auto left_interval = finite_interval<T>::checked_singleton(left);
  const auto right_interval = finite_interval<T>::checked_singleton(right);
  const auto weight_interval = finite_interval<T>::checked_singleton(weight);
  if (!left_interval || !right_interval || !weight_interval)
    return boolean_outcome<std::pair<T, finite_interval<T>>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_partition_unresolved,
            "Component 07 dyadic witness singleton construction failed"));

  const auto gap = interval_subtract(*right_interval, *left_interval);
  if (!gap)
    return boolean_outcome<std::pair<T, finite_interval<T>>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_partition_unresolved,
            "Component 07 dyadic witness gap interval failed"));
  const auto offset = interval_multiply(*gap.value, *weight_interval);
  if (!offset)
    return boolean_outcome<std::pair<T, finite_interval<T>>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_partition_unresolved,
            "Component 07 dyadic witness offset interval failed"));
  const auto parameter = interval_add(*left_interval, *offset.value);
  if (!parameter)
    return boolean_outcome<std::pair<T, finite_interval<T>>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_partition_unresolved,
            "Component 07 dyadic witness parameter interval failed"));

  const auto nominal_gap = directed_subtract(right, left);
  if (!nominal_gap)
    return boolean_outcome<std::pair<T, finite_interval<T>>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_partition_unresolved,
            "Component 07 dyadic witness nominal gap failed"));
  const auto nominal_offset =
      directed_multiply(nominal_gap.value.rounded, weight);
  if (!nominal_offset)
    return boolean_outcome<std::pair<T, finite_interval<T>>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_partition_unresolved,
            "Component 07 dyadic witness nominal offset failed"));
  const auto nominal = directed_add(left, nominal_offset.value.rounded);
  if (!nominal || !parameter.value->contains(nominal.value.rounded))
    return boolean_outcome<std::pair<T, finite_interval<T>>>::failure(
        source_facet_region_error(
            relation_subcode::source_facet_segment_partition_unresolved,
            "Component 07 dyadic witness nominal is not enclosed"));

  return boolean_outcome<std::pair<T, finite_interval<T>>>::success(
      std::make_pair(nominal.value.rounded, *parameter.value));
}

template <class T>
bool exact_point_on_query_segment(
    const projected_source_point<T> &segment_start,
    const projected_source_point<T> &segment_end,
    const projected_source_point<T> &point) {
  const auto orientation =
      bounded_source_polygon_kernel<T>::orientation(segment_start, segment_end,
                                                    point);
  return valid_source_orientation_evidence(orientation) &&
         orientation.exact_sign == 0 &&
         orientation.determinant.lower() == T(0) &&
         orientation.determinant.upper() == T(0) &&
         point_interval_inside_edge_box(point, segment_start, segment_end);
}

template <class T>
bool owner_subset(const std::vector<std::uint64_t> &expected_vertices,
                  const std::vector<source_facet_boundary_edge_owner> &expected_edges,
                  const source_facet_point_region_record<T> &actual) {
  for (const auto vertex : expected_vertices)
    if (!std::binary_search(actual.source_vertex_owners.begin(),
                            actual.source_vertex_owners.end(), vertex))
      return false;
  for (const auto &edge : expected_edges)
    if (!std::binary_search(actual.source_edge_owners.begin(),
                            actual.source_edge_owners.end(), edge))
      return false;
  return true;
}

template <class T>
bool promote_declared_vertex_owner(
    source_facet_point_region_record<T> &record,
    const projected_source_point<T> &point,
    const std::vector<projected_source_point<T>> &polygon,
    const std::vector<std::uint64_t> &declared_vertices) {
  if (declared_vertices.empty())
    return true;
  if (declared_vertices.size() != 1)
    return false;
  if (record.classification ==
      source_facet_point_region_class::original_vertex)
    return record.source_vertex_owners == declared_vertices;
  if (record.classification !=
          source_facet_point_region_class::original_edge ||
      record.source_edge_owners.size() < 2)
    return false;

  const auto declared = declared_vertices.front();
  const auto iterator =
      std::find_if(polygon.begin(), polygon.end(),
                   [declared](const projected_source_point<T> &candidate) {
                     return candidate.source_vertex == declared;
                   });
  if (iterator == polygon.end() || !same_projected_geometry(point, *iterator))
    return false;

  record.source_vertex_owners = declared_vertices;
  record.classification = source_facet_point_region_class::original_vertex;
  return valid_source_facet_point_region_record(record);
}

} // namespace source_facet_region_detail

// Segment/source-polygon partition records deliberately consume canonical
// boundary-contact constructions.  They do not perform edge-edge arithmetic.
enum class source_facet_segment_contact_kind : std::uint8_t {
  point_contact = 1,
  boundary_overlap = 2,
};

enum class source_facet_segment_interval_class : std::uint8_t {
  interior = 1,
  outside = 2,
  original_edge_overlap = 3,
};

enum class source_facet_triangle_absorption_kind : std::uint8_t {
  public_breakpoint = 1,
  public_interval = 2,
  bookkeeping_only = 3,
};

template <class T> struct source_facet_segment_contact_proposal final {
  std::uint16_t schema_version =
      contract_versions::relation_source_facet_segment_schema;
  source_facet_segment_contact_kind kind =
      source_facet_segment_contact_kind::point_contact;
  std::uint64_t lineage = 0;
  T first_rounded_parameter = T(0);
  finite_interval<T> first_parameter{};
  projected_source_point<T> first_point{};
  bool first_point_source_identity_valid = false;
  std::vector<std::uint64_t> first_source_vertex_owners;
  std::vector<source_facet_boundary_edge_owner> first_source_edge_owners;
  T second_rounded_parameter = T(0);
  finite_interval<T> second_parameter{};
  projected_source_point<T> second_point{};
  bool second_point_source_identity_valid = false;
  std::vector<std::uint64_t> second_source_vertex_owners;
  std::vector<source_facet_boundary_edge_owner> second_source_edge_owners;
  std::vector<source_facet_boundary_edge_owner> overlap_source_edge_owners;
  std::uint32_t reserved = 0;
};

template <class T> struct source_facet_segment_breakpoint final {
  finite_interval<T> parameter{};
  T rounded_parameter = T(0);
  projected_source_point<T> point{};
  source_facet_point_region_record<T> region{};
  std::vector<std::uint64_t> contact_lineages;
  std::uint8_t segment_endpoint_mask = 0;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
};

template <class T> struct source_facet_segment_interval final {
  std::uint64_t left_breakpoint = 0;
  std::uint64_t right_breakpoint = 0;
  T rounded_witness_parameter = T(0);
  finite_interval<T> witness_parameter{};
  projected_source_point<T> witness_point{};
  source_facet_point_region_record<T> witness_region{};
  source_facet_segment_interval_class classification =
      source_facet_segment_interval_class::outside;
  std::vector<source_facet_boundary_edge_owner> source_edge_owners;
  std::uint32_t dyadic_attempt_ordinal = 0;
  std::uint32_t reserved = 0;
};

template <class T> struct source_facet_triangle_local_witness final {
  std::uint16_t schema_version =
      contract_versions::relation_triangle_local_reconciliation_schema;
  std::uint64_t triangle = 0;
  std::uint64_t local_witness = 0;
  source_triangle_edge_role local_edge_role =
      source_triangle_edge_role::source_boundary;
  source_facet_triangle_absorption_kind absorption =
      source_facet_triangle_absorption_kind::public_breakpoint;
  std::uint64_t public_index = 0;
  finite_interval<T> parameter{};
  source_facet_point_region_class semantic_classification =
      source_facet_point_region_class::outside;
  std::uint64_t internal_diagonal = 0;
  std::vector<std::uint64_t> source_vertex_owners;
  std::vector<source_facet_boundary_edge_owner> source_edge_owners;
  bounded_boolean_digest exact_triangulation_digest{};
  std::uint32_t reserved = 0;

  friend bool operator<(const source_facet_triangle_local_witness &a,
                        const source_facet_triangle_local_witness &b) noexcept {
    return std::tie(a.triangle, a.local_witness, a.local_edge_role,
                    a.absorption, a.public_index, a.internal_diagonal) <
           std::tie(b.triangle, b.local_witness, b.local_edge_role,
                    b.absorption, b.public_index, b.internal_diagonal);
  }
};

template <class T> struct source_facet_segment_partition_record final {
  std::uint16_t schema_version =
      contract_versions::relation_source_facet_segment_schema;
  std::uint16_t policy_version =
      contract_versions::relation_source_facet_segment_policy;
  std::uint16_t witness_policy_version =
      contract_versions::relation_source_facet_segment_witness_policy;
  std::uint16_t reconciliation_policy_version =
      contract_versions::relation_alternative_triangulation_semantics_policy;
  std::uint64_t source_facet = 0;
  std::uint64_t ring = 0;
  projected_source_point<T> segment_start{};
  projected_source_point<T> segment_end{};
  bool segment_start_source_identity_valid = false;
  bool segment_end_source_identity_valid = false;
  bool complete_boundary_contact_set = false;
  bool triangle_reconciliation_complete = false;
  std::uint64_t boundary_edge_relation_count = 0;
  source_orientation_evidence<T> polygon_orientation_evidence{};
  std::vector<source_facet_segment_contact_proposal<T>> contacts;
  std::vector<source_facet_segment_breakpoint<T>> breakpoints;
  std::vector<source_facet_segment_interval<T>> intervals;
  std::vector<source_facet_triangle_local_witness<T>> triangle_witnesses;
  bounded_boolean_digest semantic_digest{};
  std::uint32_t reserved = 0;
};

} // namespace ygor::mesh_boolean::bounded
