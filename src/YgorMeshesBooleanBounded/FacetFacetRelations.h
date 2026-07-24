#pragma once

#include "EdgeFacetRelations.h"
#include "ExactGeometryRelations.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

enum class source_facet_support_relation_class : std::uint8_t {
  transverse = 1,
  parallel_separated = 2,
  coplanar_same_orientation = 3,
  coplanar_opposite_orientation = 4,
};

template <class T> struct bounded_geometry_snapshot1 final {
  T rounded = T(0);
  T lower = T(0);
  T upper = T(0);
};

template <class T> struct bounded_geometry_snapshot3 final {
  std::array<T, 3> rounded{};
  std::array<T, 3> lower{};
  std::array<T, 3> upper{};
};

template <class T> struct source_facet_support_plane_snapshot final {
  bounded_geometry_snapshot3<T> normal{};
  bounded_geometry_snapshot1<T> offset{};
  bounded_geometry_snapshot1<T> normal_squared{};
};

template <class T> struct source_facet_transverse_carrier final {
  bounded_geometry_snapshot3<T> point{};
  bounded_geometry_snapshot3<T> direction{};
  finite_interval<T> direction_squared = finite_interval<T>::singleton(T(0));
  std::array<finite_interval<T>, 2> point_plane_residuals{
      finite_interval<T>::singleton(T(0)), finite_interval<T>::singleton(T(0))};
  std::array<finite_interval<T>, 2> direction_plane_residuals{
      finite_interval<T>::singleton(T(0)), finite_interval<T>::singleton(T(0))};
  bool residuals_accepted = false;
  std::uint8_t reserved8 = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

template <class T> struct source_facet_support_input final {
  relation_feature_key feature{};
  std::uint64_t source_facet = 0;
  std::uint64_t ring = 0;
  std::uint64_t shell = 0;
  occupied_side material_side = occupied_side::negative;
  std::uint8_t dropped_axis = 0;
  std::array<bounded_point3<T>, 3> support{};
  bounded_boolean_digest basis_digest{};
  std::uint32_t reserved = 0;
};

template <class T> struct source_facet_source_facet_relation_record final {
  std::uint16_t schema_version =
      contract_versions::relation_source_facet_facet_schema;
  std::uint16_t policy_version =
      contract_versions::relation_source_facet_facet_policy;
  context_owner_token owner{};
  relation_feature_key first_feature{};
  relation_feature_key second_feature{};
  std::array<std::uint64_t, 2> source_facets{};
  std::array<std::uint64_t, 2> rings{};
  std::array<std::uint64_t, 2> shells{};
  std::array<occupied_side, 2> material_sides{
      occupied_side::negative, occupied_side::negative};
  std::array<std::uint8_t, 2> dropped_axes{};
  std::array<bounded_boolean_digest, 2> basis_digests{};
  std::array<std::array<bounded_geometry_snapshot3<T>, 3>, 2> support_points{};
  std::array<source_facet_support_plane_snapshot<T>, 2> support_planes{};
  source_facet_support_relation_class classification =
      source_facet_support_relation_class::transverse;
  relation_truth_record parallelism_truth{};
  relation_truth_record coplanarity_truth{};
  relation_truth_record orientation_truth{};
  bool has_coplanarity_truth = false;
  bool has_orientation_truth = false;
  bool has_transverse_carrier = false;
  std::uint8_t reserved8 = 0;
  source_facet_transverse_carrier<T> transverse_carrier{};
  std::vector<relation_request_id> edge_facet_consumers;
  T residual_boundary = T(0);
  std::uint32_t reserved32 = 0;
  bounded_boolean_digest semantic_digest{};
};

struct candidate_source_facet_relation_range final {
  candidate_id candidate{0};
  std::uint64_t begin = 0;
  std::uint64_t count = 0;
  std::uint32_t reserved = 0;
};

template <class T> struct candidate_source_facet_relation_stage final {
  std::uint16_t schema_version =
      contract_versions::relation_source_facet_facet_stage_schema;
  std::uint16_t policy_version =
      contract_versions::relation_source_facet_facet_stage_policy;
  context_owner_token owner{};
  relation_request_graph request_graph{};
  std::vector<source_facet_source_facet_relation_record<T>> relations;
  std::vector<relation_request_id> candidate_relations;
  std::vector<candidate_source_facet_relation_range> candidate_ranges;
  std::uint64_t evaluation_count = 0;
  std::uint32_t reserved = 0;
  bounded_boolean_digest semantic_digest{};
};

inline bounded_boolean_error source_facet_relation_error(
    relation_subcode subcode, const char *summary,
    relation_checkpoint checkpoint = relation_checkpoint::facet_facet_evaluation,
    bounded_boolean_error_category category =
        bounded_boolean_error_category::internal_invariant_error) {
  return relation_error(subcode, category, summary, checkpoint);
}

namespace source_facet_relation_detail {

template <class T>
void encode_interval(canonical_writer &writer, const finite_interval<T> &value) {
  writer.floating(value.lower());
  writer.floating(value.upper());
}

inline void encode_truth(canonical_writer &writer,
                         const relation_truth_record &record) {
  writer.u64(record.rounded_nominal_bits);
  writer.u8(static_cast<std::uint8_t>(record.bounded_sign));
  writer.u8(static_cast<std::uint8_t>(record.exact_relation));
  writer.u8(static_cast<std::uint8_t>(record.disposition));
  writer.u16(record.rounded_formula);
  writer.u16(record.exact_formula);
  writer.u32(record.reserved);
}

template <class T>
void encode_snapshot(canonical_writer &writer,
                     const bounded_geometry_snapshot1<T> &value) {
  writer.floating(value.rounded);
  writer.floating(value.lower);
  writer.floating(value.upper);
}

template <class T>
void encode_snapshot(canonical_writer &writer,
                     const bounded_geometry_snapshot3<T> &value) {
  for (const auto component : value.rounded)
    writer.floating(component);
  for (const auto component : value.lower)
    writer.floating(component);
  for (const auto component : value.upper)
    writer.floating(component);
}

template <class T>
void encode_plane(canonical_writer &writer,
                  const source_facet_support_plane_snapshot<T> &value) {
  encode_snapshot(writer, value.normal);
  encode_snapshot(writer, value.offset);
  encode_snapshot(writer, value.normal_squared);
}

inline bool valid_material_side(occupied_side side) noexcept {
  return side == occupied_side::negative || side == occupied_side::positive;
}

template <class T>
bool valid_snapshot(const bounded_geometry_snapshot1<T> &value) noexcept {
  return finite_bits(value.rounded) && finite_bits(value.lower) &&
         finite_bits(value.upper) &&
         !finite_numeric_less(value.upper, value.lower) &&
         !finite_numeric_less(value.rounded, value.lower) &&
         !finite_numeric_less(value.upper, value.rounded);
}

template <class T>
bool valid_snapshot(const bounded_geometry_snapshot3<T> &value) noexcept {
  for (std::size_t axis = 0; axis < 3; ++axis) {
    if (!finite_bits(value.rounded[axis]) || !finite_bits(value.lower[axis]) ||
        !finite_bits(value.upper[axis]) ||
        finite_numeric_less(value.upper[axis], value.lower[axis]) ||
        finite_numeric_less(value.rounded[axis], value.lower[axis]) ||
        finite_numeric_less(value.upper[axis], value.rounded[axis]))
      return false;
  }
  return true;
}

template <class T>
bounded_geometry_snapshot1<T> snapshot(const bounded_scalar<T> &value) {
  return {value.rounded_nominal, value.uncertainty_enclosure.lower(),
          value.uncertainty_enclosure.upper()};
}

template <class T>
bounded_geometry_snapshot3<T> snapshot(const bounded_vec3<T> &value) {
  bounded_geometry_snapshot3<T> out;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    out.rounded[axis] = value.components[axis].rounded_nominal;
    out.lower[axis] = value.components[axis].uncertainty_enclosure.lower();
    out.upper[axis] = value.components[axis].uncertainty_enclosure.upper();
  }
  return out;
}

template <class T>
source_facet_support_plane_snapshot<T> snapshot(
    const bounded_plane3<T> &plane) {
  source_facet_support_plane_snapshot<T> out;
  out.normal = snapshot(plane.normal);
  out.offset = snapshot(plane.offset);
  out.normal_squared = snapshot(plane.normal_sq);
  return out;
}

template <class T>
std::array<T, 3> nominal(const bounded_point3<T> &point) noexcept {
  return {{point.coordinates.components[0].rounded_nominal,
           point.coordinates.components[1].rounded_nominal,
           point.coordinates.components[2].rounded_nominal}};
}

template <class T>
bool valid_support_input(const source_facet_support_input<T> &input,
                         const context_owner_token &owner) noexcept {
  if (!owner.anchor || !valid_relation_feature_key(input.feature) ||
      input.feature.kind != relation_feature_kind::source_facet ||
      input.feature.primary != input.source_facet ||
      input.feature.secondary != input.ring || input.dropped_axis > 2 ||
      !valid_material_side(input.material_side) || input.reserved != 0)
    return false;
  for (const auto &point : input.support)
    if (!source_edge_relation_detail::valid_point(point, owner))
      return false;
  const auto a = nominal(input.support[0]);
  const auto b = nominal(input.support[1]);
  const auto c = nominal(input.support[2]);
  return a != b && a != c && b != c;
}

template <class T>
boolean_outcome<bounded_vec3<T>> bounded_vector_divide(
    const bounded_vec3<T> &value, const bounded_scalar<T> &divisor) {
  if (!bounded_operations_detail::same_bound_owner(value.owner,
                                                    divisor.identity.owner))
    return boolean_outcome<bounded_vec3<T>>::failure(
        source_facet_relation_error(
            relation_subcode::source_facet_carrier_unresolved,
            "Component 07 facet/facet carrier division owner mismatch"));
  bounded_vec3<T> out;
  out.owner = value.owner;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    auto component = bounded_divide(value.components[axis], divisor);
    if (!component.has_value())
      return boolean_outcome<bounded_vec3<T>>::failure(*component.error());
    out.components[axis] = std::move(*component.value());
  }
  if (!bounded_operations_detail::compute_radial_error(out))
    return boolean_outcome<bounded_vec3<T>>::failure(
        source_facet_relation_error(
            relation_subcode::source_facet_carrier_unresolved,
            "Component 07 facet/facet carrier radial bound failed"));
  return boolean_outcome<bounded_vec3<T>>::success(std::move(out));
}

template <class T>
bool residual_interval_accepted(const finite_interval<T> &value,
                                T boundary) noexcept {
  return finite_bits(boundary) && boundary >= T(0) &&
         finite_bits(value.lower()) && finite_bits(value.upper()) &&
         value.lower() >= -boundary && value.upper() <= boundary;
}

template <class T>
boolean_outcome<source_facet_transverse_carrier<T>> build_carrier(
    const bounded_plane3<T> &first, const bounded_plane3<T> &second,
    const bounded_vec3<T> &direction, const bounded_scalar<T> &direction_sq,
    T residual_boundary) {
  if (direction_sq.uncertainty_enclosure.lower() <= T(0))
    return boolean_outcome<source_facet_transverse_carrier<T>>::failure(
        source_facet_relation_error(
            relation_subcode::source_facet_carrier_unresolved,
            "Component 07 facet/facet transverse carrier is ill-conditioned",
            relation_checkpoint::construction_validation,
            bounded_boolean_error_category::geometric_condition_exceeds_tolerance));

  auto second_first = bounded_vector_scale(first.normal, second.offset);
  if (!second_first.has_value())
    return boolean_outcome<source_facet_transverse_carrier<T>>::failure(
        *second_first.error());
  auto first_second = bounded_vector_scale(second.normal, first.offset);
  if (!first_second.has_value())
    return boolean_outcome<source_facet_transverse_carrier<T>>::failure(
        *first_second.error());
  auto numerator_base =
      bounded_vector_subtract(*second_first.value(), *first_second.value());
  if (!numerator_base.has_value())
    return boolean_outcome<source_facet_transverse_carrier<T>>::failure(
        *numerator_base.error());
  auto numerator = bounded_cross3(*numerator_base.value(), direction);
  if (!numerator.has_value())
    return boolean_outcome<source_facet_transverse_carrier<T>>::failure(
        *numerator.error());
  auto coordinates = bounded_vector_divide(*numerator.value(), direction_sq);
  if (!coordinates.has_value())
    return boolean_outcome<source_facet_transverse_carrier<T>>::failure(
        *coordinates.error());

  bounded_point3<T> point;
  point.owner = first.owner;
  point.coordinates = *coordinates.value();
  auto first_point = bounded_plane_residual(first, point, residual_boundary);
  auto second_point = bounded_plane_residual(second, point, residual_boundary);
  auto first_direction = bounded_dot3(first.normal, direction);
  auto second_direction = bounded_dot3(second.normal, direction);
  if (!first_point.has_value() || !second_point.has_value() ||
      !first_direction.has_value() || !second_direction.has_value())
    return boolean_outcome<source_facet_transverse_carrier<T>>::failure(
        source_facet_relation_error(
            relation_subcode::source_facet_carrier_unresolved,
            "Component 07 facet/facet carrier residual evaluation failed",
            relation_checkpoint::construction_validation,
            bounded_boolean_error_category::geometric_condition_exceeds_tolerance));

  source_facet_transverse_carrier<T> out;
  out.point = snapshot(point.coordinates);
  out.direction = snapshot(direction);
  out.direction_squared = direction_sq.uncertainty_enclosure;
  out.point_plane_residuals = {
      first_point.value()->value.uncertainty_enclosure,
      second_point.value()->value.uncertainty_enclosure};
  out.direction_plane_residuals = {
      first_direction.value()->uncertainty_enclosure,
      second_direction.value()->uncertainty_enclosure};
  out.residuals_accepted =
      first_point.value()->disposition == residual_disposition::pass &&
      second_point.value()->disposition == residual_disposition::pass &&
      residual_interval_accepted(out.direction_plane_residuals[0],
                                 residual_boundary) &&
      residual_interval_accepted(out.direction_plane_residuals[1],
                                 residual_boundary);
  if (!out.residuals_accepted)
    return boolean_outcome<source_facet_transverse_carrier<T>>::failure(
        source_facet_relation_error(
            relation_subcode::source_facet_residual_rejected,
            "Component 07 facet/facet carrier residual exceeds tolerance",
            relation_checkpoint::construction_validation,
            bounded_boolean_error_category::geometric_condition_exceeds_tolerance));
  return boolean_outcome<source_facet_transverse_carrier<T>>::success(
      std::move(out));
}

} // namespace source_facet_relation_detail

template <class T>
std::vector<std::uint8_t> encode_source_facet_relation_semantics(
    const source_facet_source_facet_relation_record<T> &record) {
  canonical_writer writer;
  writer.u32(0x37464652U); // RFF7
  writer.u16(record.schema_version);
  writer.u16(record.policy_version);
  encode_relation_feature_key(writer, record.first_feature);
  encode_relation_feature_key(writer, record.second_feature);
  for (const auto value : record.source_facets)
    writer.u64(value);
  for (const auto value : record.rings)
    writer.u64(value);
  for (const auto value : record.shells)
    writer.u64(value);
  for (const auto value : record.material_sides)
    writer.u8(static_cast<std::uint8_t>(value));
  for (const auto value : record.dropped_axes)
    writer.u8(value);
  for (const auto &digest : record.basis_digests)
    for (const auto byte : digest.bytes)
      writer.u8(byte);
  for (const auto &facet_support : record.support_points)
    for (const auto &point : facet_support)
      source_facet_relation_detail::encode_snapshot(writer, point);
  for (const auto &plane : record.support_planes)
    source_facet_relation_detail::encode_plane(writer, plane);
  writer.u8(static_cast<std::uint8_t>(record.classification));
  source_facet_relation_detail::encode_truth(writer, record.parallelism_truth);
  source_facet_relation_detail::encode_truth(writer, record.coplanarity_truth);
  source_facet_relation_detail::encode_truth(writer, record.orientation_truth);
  writer.boolean(record.has_coplanarity_truth);
  writer.boolean(record.has_orientation_truth);
  writer.boolean(record.has_transverse_carrier);
  writer.u8(record.reserved8);
  source_facet_relation_detail::encode_snapshot(
      writer, record.transverse_carrier.point);
  source_facet_relation_detail::encode_snapshot(
      writer, record.transverse_carrier.direction);
  source_facet_relation_detail::encode_interval(
      writer, record.transverse_carrier.direction_squared);
  for (const auto &value : record.transverse_carrier.point_plane_residuals)
    source_facet_relation_detail::encode_interval(writer, value);
  for (const auto &value : record.transverse_carrier.direction_plane_residuals)
    source_facet_relation_detail::encode_interval(writer, value);
  writer.boolean(record.transverse_carrier.residuals_accepted);
  writer.u8(record.transverse_carrier.reserved8);
  writer.u16(record.transverse_carrier.reserved16);
  writer.u32(record.transverse_carrier.reserved32);
  writer.u64(record.edge_facet_consumers.size());
  for (const auto consumer : record.edge_facet_consumers)
    writer.u64(consumer.ordinal());
  writer.floating(record.residual_boundary);
  writer.u32(record.reserved32);
  return writer.take();
}

template <class T>
bool valid_source_facet_relation_record(
    const source_facet_source_facet_relation_record<T> &record) noexcept {
  using namespace source_facet_relation_detail;
  if (record.schema_version !=
          contract_versions::relation_source_facet_facet_schema ||
      record.policy_version !=
          contract_versions::relation_source_facet_facet_policy ||
      !record.owner.anchor || !valid_relation_feature_key(record.first_feature) ||
      !valid_relation_feature_key(record.second_feature) ||
      record.first_feature.kind != relation_feature_kind::source_facet ||
      record.second_feature.kind != relation_feature_kind::source_facet ||
      record.first_feature.operand == record.second_feature.operand ||
      record.second_feature < record.first_feature ||
      record.first_feature.primary != record.source_facets[0] ||
      record.second_feature.primary != record.source_facets[1] ||
      record.first_feature.secondary != record.rings[0] ||
      record.second_feature.secondary != record.rings[1] ||
      !valid_material_side(record.material_sides[0]) ||
      !valid_material_side(record.material_sides[1]) ||
      record.dropped_axes[0] > 2 || record.dropped_axes[1] > 2 ||
      !finite_bits(record.residual_boundary) || record.residual_boundary < T(0) ||
      record.reserved8 != 0 || record.reserved32 != 0 ||
      !valid_relation_truth_record(record.parallelism_truth) ||
      record.transverse_carrier.reserved8 != 0 ||
      record.transverse_carrier.reserved16 != 0 ||
      record.transverse_carrier.reserved32 != 0 ||
      !std::is_sorted(record.edge_facet_consumers.begin(),
                      record.edge_facet_consumers.end()) ||
      std::adjacent_find(record.edge_facet_consumers.begin(),
                         record.edge_facet_consumers.end()) !=
          record.edge_facet_consumers.end())
    return false;
  for (const auto &facet_support : record.support_points)
    for (const auto &point : facet_support)
      if (!valid_snapshot(point))
        return false;
  for (const auto &plane : record.support_planes)
    if (!valid_snapshot(plane.normal) || !valid_snapshot(plane.offset) ||
        !valid_snapshot(plane.normal_squared) ||
        plane.normal_squared.lower <= T(0))
      return false;

  const bool transverse = record.classification ==
                          source_facet_support_relation_class::transverse;
  const bool parallel_separated =
      record.classification ==
      source_facet_support_relation_class::parallel_separated;
  const bool coplanar =
      record.classification ==
          source_facet_support_relation_class::coplanar_same_orientation ||
      record.classification ==
          source_facet_support_relation_class::coplanar_opposite_orientation;
  if (!transverse && !parallel_separated && !coplanar)
    return false;
  if (record.has_coplanarity_truth != !transverse ||
      record.has_orientation_truth != coplanar ||
      record.has_transverse_carrier != transverse)
    return false;
  if (record.has_coplanarity_truth &&
      !valid_relation_truth_record(record.coplanarity_truth))
    return false;
  if (record.has_orientation_truth &&
      !valid_relation_truth_record(record.orientation_truth))
    return false;
  if (transverse) {
    if (!valid_snapshot(record.transverse_carrier.point) ||
        !valid_snapshot(record.transverse_carrier.direction) ||
        record.transverse_carrier.direction_squared.lower() <= T(0) ||
        !record.transverse_carrier.residuals_accepted)
      return false;
    for (const auto &value : record.transverse_carrier.point_plane_residuals)
      if (!residual_interval_accepted(value, record.residual_boundary))
        return false;
    for (const auto &value : record.transverse_carrier.direction_plane_residuals)
      if (!residual_interval_accepted(value, record.residual_boundary))
        return false;
  }
  return record.semantic_digest ==
         sha256::digest(encode_source_facet_relation_semantics(record));
}

template <class T>
boolean_outcome<source_facet_source_facet_relation_record<T>>
classify_source_facet_support_relation(
    source_facet_support_input<T> first,
    source_facet_support_input<T> second,
    const context_owner_token &owner, T residual_boundary,
    std::vector<relation_request_id> edge_facet_consumers = {}) {
  using record_type = source_facet_source_facet_relation_record<T>;
  using namespace source_facet_relation_detail;
  static_assert(supported_precision_scalar_v<T>);
  if (second.feature < first.feature)
    std::swap(first, second);
  if (!valid_support_input(first, owner) || !valid_support_input(second, owner) ||
      first.feature.operand == second.feature.operand ||
      !finite_bits(residual_boundary) || residual_boundary < T(0))
    return boolean_outcome<record_type>::failure(source_facet_relation_error(
        relation_subcode::source_facet_relation_malformed,
        "Component 07 facet/facet support input is malformed"));

  auto first_plane = bounded_plane_from_points(
      first.support[0], first.support[1], first.support[2]);
  auto second_plane = bounded_plane_from_points(
      second.support[0], second.support[1], second.support[2]);
  if (!first_plane.has_value() || !second_plane.has_value())
    return boolean_outcome<record_type>::failure(source_facet_relation_error(
        relation_subcode::source_facet_support_unresolved,
        "Component 07 facet/facet support plane is unavailable",
        relation_checkpoint::rounded_primitive_evaluation,
        bounded_boolean_error_category::geometric_condition_exceeds_tolerance));

  auto direction = bounded_cross3(first_plane.value()->normal,
                                  second_plane.value()->normal);
  if (!direction.has_value())
    return boolean_outcome<record_type>::failure(*direction.error());
  auto direction_sq = bounded_squared_norm(*direction.value());
  if (!direction_sq.has_value())
    return boolean_outcome<record_type>::failure(*direction_sq.error());

  const auto a0 = nominal(first.support[0]);
  const auto a1 = nominal(first.support[1]);
  const auto a2 = nominal(first.support[2]);
  const auto b0 = nominal(second.support[0]);
  const auto b1 = nominal(second.support[1]);
  const auto b2 = nominal(second.support[2]);
  const auto exact_parallel = exact_plane_normals_parallel_3d(
      a0, a1, a2, b0, b1, b2);
  auto parallel_truth = assemble_relation_truth_record(
      *direction_sq.value(), exact_parallel, rounded_operation_code::squared_norm,
      true);
  if (!parallel_truth.has_value())
    return boolean_outcome<record_type>::failure(*parallel_truth.error());

  record_type record;
  record.owner = owner;
  record.first_feature = first.feature;
  record.second_feature = second.feature;
  record.source_facets = {first.source_facet, second.source_facet};
  record.rings = {first.ring, second.ring};
  record.shells = {first.shell, second.shell};
  record.material_sides = {first.material_side, second.material_side};
  record.dropped_axes = {first.dropped_axis, second.dropped_axis};
  record.basis_digests = {first.basis_digest, second.basis_digest};
  for (std::size_t support = 0; support < 3; ++support) {
    record.support_points[0][support] = snapshot(first.support[support].coordinates);
    record.support_points[1][support] = snapshot(second.support[support].coordinates);
  }
  record.support_planes = {snapshot(*first_plane.value()),
                           snapshot(*second_plane.value())};
  record.parallelism_truth = *parallel_truth.value();
  record.residual_boundary = residual_boundary;
  std::sort(edge_facet_consumers.begin(), edge_facet_consumers.end());
  edge_facet_consumers.erase(
      std::unique(edge_facet_consumers.begin(), edge_facet_consumers.end()),
      edge_facet_consumers.end());
  record.edge_facet_consumers = std::move(edge_facet_consumers);

  if (exact_parallel.status == exact_relation_status::exact_positive) {
    if (direction_sq.value()->uncertainty_enclosure.lower() <= T(0))
      return boolean_outcome<record_type>::failure(source_facet_relation_error(
          relation_subcode::source_facet_support_unresolved,
          "Component 07 facet/facet nonparallel support lacks a positive bounded margin",
          relation_checkpoint::facet_facet_evaluation,
          bounded_boolean_error_category::geometric_condition_exceeds_tolerance));
    auto carrier = build_carrier(*first_plane.value(), *second_plane.value(),
                                 *direction.value(), *direction_sq.value(),
                                 residual_boundary);
    if (!carrier.has_value())
      return boolean_outcome<record_type>::failure(*carrier.error());
    record.classification = source_facet_support_relation_class::transverse;
    record.has_transverse_carrier = true;
    record.transverse_carrier = std::move(*carrier.value());
  } else if (exact_parallel.status == exact_relation_status::exact_zero) {
    auto offset = bounded_plane_residual(*first_plane.value(), second.support[0],
                                         residual_boundary);
    if (!offset.has_value())
      return boolean_outcome<record_type>::failure(*offset.error());
    const auto exact_coplanar = exact_plane_point_residual_3d(a0, a1, a2, b0);
    auto coplanarity_truth = assemble_relation_truth_record(
        offset.value()->value, exact_coplanar,
        rounded_operation_code::plane_residual, true);
    if (!coplanarity_truth.has_value())
      return boolean_outcome<record_type>::failure(*coplanarity_truth.error());
    record.has_coplanarity_truth = true;
    record.coplanarity_truth = *coplanarity_truth.value();

    if (exact_coplanar.status == exact_relation_status::exact_zero) {
      auto orientation = bounded_dot3(first_plane.value()->normal,
                                      second_plane.value()->normal);
      if (!orientation.has_value())
        return boolean_outcome<record_type>::failure(*orientation.error());
      const auto exact_orientation = exact_plane_normal_dot_3d(
          a0, a1, a2, b0, b1, b2);
      if (exact_orientation.status != exact_relation_status::exact_negative &&
          exact_orientation.status != exact_relation_status::exact_positive)
        return boolean_outcome<record_type>::failure(source_facet_relation_error(
            relation_subcode::source_facet_orientation_unresolved,
            "Component 07 coplanar support orientation is unresolved",
            relation_checkpoint::facet_facet_evaluation,
            bounded_boolean_error_category::geometric_condition_exceeds_tolerance));
      auto orientation_truth = assemble_relation_truth_record(
          *orientation.value(), exact_orientation,
          rounded_operation_code::dot3, true);
      if (!orientation_truth.has_value())
        return boolean_outcome<record_type>::failure(*orientation_truth.error());
      record.has_orientation_truth = true;
      record.orientation_truth = *orientation_truth.value();
      record.classification =
          exact_orientation.status == exact_relation_status::exact_positive
              ? source_facet_support_relation_class::coplanar_same_orientation
              : source_facet_support_relation_class::coplanar_opposite_orientation;
    } else if (exact_coplanar.status == exact_relation_status::exact_negative ||
               exact_coplanar.status == exact_relation_status::exact_positive) {
      const auto sign = classify_bounded_sign(
          offset.value()->value.uncertainty_enclosure);
      if (sign == bounded_sign_status::overlaps_boundary ||
          sign == bounded_sign_status::invalid)
        return boolean_outcome<record_type>::failure(source_facet_relation_error(
            relation_subcode::source_facet_offset_unresolved,
            "Component 07 parallel support offset is within the unresolved tolerance band",
            relation_checkpoint::facet_facet_evaluation,
            bounded_boolean_error_category::geometric_condition_exceeds_tolerance));
      record.classification =
          source_facet_support_relation_class::parallel_separated;
    } else {
      return boolean_outcome<record_type>::failure(source_facet_relation_error(
          relation_subcode::source_facet_offset_unresolved,
          "Component 07 parallel support exact offset is unavailable",
          relation_checkpoint::exact_relation_evaluation,
          bounded_boolean_error_category::geometric_condition_exceeds_tolerance));
    }
  } else {
    return boolean_outcome<record_type>::failure(source_facet_relation_error(
        relation_subcode::source_facet_support_unresolved,
        "Component 07 facet/facet exact parallelism is unavailable",
        relation_checkpoint::exact_relation_evaluation,
        bounded_boolean_error_category::geometric_condition_exceeds_tolerance));
  }

  record.semantic_digest =
      sha256::digest(encode_source_facet_relation_semantics(record));
  if (!valid_source_facet_relation_record(record))
    return boolean_outcome<record_type>::failure(source_facet_relation_error(
        relation_subcode::source_facet_relation_invariant,
        "Component 07 facet/facet support record failed producer validation",
        relation_checkpoint::producer_verification));
  return boolean_outcome<record_type>::success(std::move(record));
}

namespace candidate_source_facet_relation_detail {

using candidate_source_edge_relation_detail::candidate_operands;
using candidate_source_edge_relation_detail::operand_topology;
using candidate_source_edge_relation_detail::valid_original_edge_primitive;
using candidate_source_edge_facet_detail::source_facet_feature;

inline relation_request_key facet_pair_key(
    const bounded_boolean_digest &semantic_namespace,
    relation_feature_key first, relation_feature_key second) {
  relation_request_key key;
  key.semantic_namespace = semantic_namespace;
  key.family = relation_request_family::source_facet_source_facet;
  key.scope = relation_record_scope::public_source_feature;
  key.first = first;
  key.second = second;
  if (key.second < key.first)
    std::swap(key.first, key.second);
  key.formula_version = contract_versions::exact_relation_formulas;
  key.policy_version = contract_versions::relation_request_key_schema;
  return key;
}

template <class T, class I>
bool source_facet_feature_from_topology(
    const canonical_candidate_stream<T, I> &candidates, operand_id operand,
    std::uint64_t source_facet, relation_feature_key &feature) {
  const auto *topology = operand_topology(candidates, operand);
  if (!topology || source_facet >= topology->source_facet_to_group().size())
    return false;
  const auto group_ordinal = topology->source_facet_to_group()[source_facet];
  if (group_ordinal >= topology->facet_groups().size())
    return false;
  const auto &group = topology->facet_groups()[group_ordinal];
  if (group.canonical_id != group_ordinal ||
      group.source_facet != source_facet)
    return false;
  feature = source_facet_feature(operand, source_facet, group.ring);
  return valid_relation_feature_key(feature);
}

template <class T, class I>
bool append_candidate_proposals(
    const canonical_candidate_stream<T, I> &candidates,
    const bounded_boolean_digest &semantic_namespace,
    std::vector<relation_request_proposal> &proposals,
    const relation_capabilities &capabilities,
    bounded_boolean_error &error) {
  for (std::size_t ordinal = 0; ordinal < candidates.candidates().size();
       ++ordinal) {
    if (relation_cancelled(capabilities)) {
      error = source_facet_relation_error(
          relation_subcode::cancelled,
          "Component 07 facet/facet request discovery cancelled",
          relation_checkpoint::candidate_scan,
          bounded_boolean_error_category::cancelled);
      return false;
    }
    const auto &candidate = candidates.candidates()[ordinal];
    operand_id edge_operand = operand_id::a;
    operand_id triangle_operand = operand_id::b;
    if (candidate.id.ordinal() != ordinal || candidate.ordinal != ordinal ||
        candidate.family !=
            broad_phase_relation_family::canonical_edge_source_triangle ||
        candidate.filter_reason != topological_filter_reason::not_filtered ||
        candidate.reserved != 0 ||
        !candidate_operands<T, I>(candidate, edge_operand, triangle_operand)) {
      error = source_facet_relation_error(
          relation_subcode::source_facet_relation_malformed,
          "Component 07 facet/facet candidate is malformed",
          relation_checkpoint::candidate_scan);
      return false;
    }
    const auto &edge_table = candidates.primitive_table(edge_operand);
    const auto &triangle_table = candidates.primitive_table(triangle_operand);
    if (candidate.edge.ordinal() >= edge_table.edges.size() ||
        candidate.triangle.ordinal() >= triangle_table.triangles.size()) {
      error = source_facet_relation_error(
          relation_subcode::source_facet_relation_malformed,
          "Component 07 facet/facet candidate primitive is out of range",
          relation_checkpoint::candidate_scan);
      return false;
    }
    const auto &edge = edge_table.edges[candidate.edge.ordinal()];
    const auto &triangle = triangle_table.triangles[candidate.triangle.ordinal()];
    if (edge.edge_class == canonical_edge_class::facet_internal_diagonal)
      continue;
    if (!valid_original_edge_primitive(edge) ||
        triangle.source_facet == broad_phase_invalid_ordinal) {
      error = source_facet_relation_error(
          relation_subcode::source_facet_relation_malformed,
          "Component 07 facet/facet public feature handshake failed",
          relation_checkpoint::candidate_scan);
      return false;
    }
    relation_feature_key opposite_feature;
    if (!source_facet_feature_from_topology(
            candidates, triangle_operand, triangle.source_facet,
            opposite_feature)) {
      error = source_facet_relation_error(
          relation_subcode::source_facet_relation_malformed,
          "Component 07 opposite source facet is unavailable",
          relation_checkpoint::candidate_scan);
      return false;
    }
    std::array<std::uint64_t, 2> incident = edge.source_facets;
    std::sort(incident.begin(), incident.end());
    for (std::size_t index = 0; index < incident.size(); ++index) {
      if (index != 0 && incident[index] == incident[index - 1])
        continue;
      relation_feature_key incident_feature;
      if (!source_facet_feature_from_topology(
              candidates, edge_operand, incident[index], incident_feature)) {
        error = source_facet_relation_error(
            relation_subcode::source_facet_relation_malformed,
            "Component 07 incident source facet is unavailable",
            relation_checkpoint::candidate_scan);
        return false;
      }
      relation_request_proposal proposal;
      proposal.key = facet_pair_key(semantic_namespace, incident_feature,
                                    opposite_feature);
      proposal.candidate_witnesses.push_back(candidate.id);
      if (!valid_relation_request_key(proposal.key)) {
        error = source_facet_relation_error(
            relation_subcode::source_facet_relation_malformed,
            "Component 07 generated an invalid facet/facet request key",
            relation_checkpoint::candidate_scan);
        return false;
      }
      if (proposals.size() >= capabilities.maximum_requests) {
        error = source_facet_relation_error(
            relation_subcode::work_limit,
            "Component 07 facet/facet request limit exceeded",
            relation_checkpoint::count_representability_preflight,
            bounded_boolean_error_category::resource_limit);
        return false;
      }
      proposals.push_back(std::move(proposal));
    }
  }
  return true;
}

template <class T, class I>
bool make_support_input(const canonical_candidate_stream<T, I> &candidates,
                        const relation_feature_key &feature,
                        source_facet_support_input<T> &out) {
  if (!valid_relation_feature_key(feature) ||
      feature.kind != relation_feature_kind::source_facet)
    return false;
  const auto *topology = operand_topology(candidates, feature.operand);
  if (!topology || !topology->owner().same_owner(candidates.owner()) ||
      feature.primary >= topology->source_facet_to_group().size())
    return false;
  const auto group_ordinal = topology->source_facet_to_group()[feature.primary];
  if (group_ordinal >= topology->facet_groups().size())
    return false;
  const auto &group = topology->facet_groups()[group_ordinal];
  if (group.canonical_id != group_ordinal ||
      group.source_facet != feature.primary || group.ring != feature.secondary ||
      group.basis.dropped_axis > 2 ||
      group.shell >= topology->source_shell_to_group().size())
    return false;
  const auto shell_ordinal = topology->source_shell_to_group()[group.shell];
  if (shell_ordinal >= topology->shell_groups().size())
    return false;
  const auto &shell = topology->shell_groups()[shell_ordinal];
  if (shell.canonical_id != shell_ordinal ||
      shell.source_shell != group.shell)
    return false;

  out = source_facet_support_input<T>{};
  out.feature = feature;
  out.source_facet = group.source_facet;
  out.ring = group.ring;
  out.shell = group.shell;
  out.material_side = shell.material_side;
  out.dropped_axis = group.basis.dropped_axis;
  out.basis_digest = group.basis.basis_digest;
  for (std::size_t support = 0; support < 3; ++support) {
    const auto source_vertex = group.basis.support_vertices[support];
    if (source_vertex >= topology->source_vertex_to_vertex().size())
      return false;
    const auto vertex_ordinal = topology->source_vertex_to_vertex()[source_vertex];
    if (vertex_ordinal >= topology->vertices().size() ||
        !candidate_source_edge_relation_detail::import_vertex_point(
            topology->vertices()[vertex_ordinal], feature.operand,
            candidates.owner(), out.support[support]))
      return false;
  }
  return source_facet_relation_detail::valid_support_input(out,
                                                           candidates.owner());
}

template <class T, class I>
bool edge_incident_to_facet(
    const canonical_candidate_stream<T, I> &candidates,
    const relation_feature_key &edge_feature,
    const relation_feature_key &facet_feature) {
  if (edge_feature.kind != relation_feature_kind::source_edge ||
      facet_feature.kind != relation_feature_kind::source_facet ||
      edge_feature.operand != facet_feature.operand)
    return false;
  const auto &table = candidates.primitive_table(edge_feature.operand);
  const broad_phase_edge_primitive<T> *match = nullptr;
  for (const auto &edge : table.edges) {
    if (!valid_original_edge_primitive(edge) ||
        candidate_source_edge_relation_detail::source_edge_feature(edge) !=
            edge_feature)
      continue;
    if (match)
      return false;
    match = &edge;
  }
  return match &&
         (match->source_facets[0] == facet_feature.primary ||
          match->source_facets[1] == facet_feature.primary);
}

template <class T, class I>
std::vector<relation_request_id> edge_facet_consumers(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_facet_relation_stage<T> &edge_facet_stage,
    const relation_feature_key &first, const relation_feature_key &second) {
  std::vector<relation_request_id> result;
  for (const auto &request : edge_facet_stage.request_graph.requests) {
    if (request.key.family !=
            relation_request_family::source_edge_source_facet ||
        request.key.first.kind != relation_feature_kind::source_edge ||
        request.key.second.kind != relation_feature_kind::source_facet)
      continue;
    const bool consumes_first =
        request.key.second == first &&
        edge_incident_to_facet(candidates, request.key.first, second);
    const bool consumes_second =
        request.key.second == second &&
        edge_incident_to_facet(candidates, request.key.first, first);
    if (consumes_first || consumes_second)
      result.push_back(request.id);
  }
  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());
  return result;
}

template <class T>
bool relation_semantics_equal(
    const source_facet_source_facet_relation_record<T> &first,
    const source_facet_source_facet_relation_record<T> &second) {
  return encode_source_facet_relation_semantics(first) ==
         encode_source_facet_relation_semantics(second);
}

} // namespace candidate_source_facet_relation_detail

template <class T>
std::vector<std::uint8_t> encode_candidate_source_facet_relation_semantics(
    const candidate_source_facet_relation_stage<T> &stage) {
  canonical_writer writer;
  writer.u32(0x37464643U); // CFF7
  writer.u16(stage.schema_version);
  writer.u16(stage.policy_version);
  writer.sized_bytes(encode_relation_request_graph_semantics(stage.request_graph));
  writer.u64(stage.relations.size());
  for (const auto &relation : stage.relations)
    writer.sized_bytes(encode_source_facet_relation_semantics(relation));
  writer.u64(stage.candidate_relations.size());
  for (const auto request : stage.candidate_relations)
    writer.u64(request.ordinal());
  writer.u64(stage.candidate_ranges.size());
  for (const auto &range : stage.candidate_ranges) {
    writer.u64(range.candidate.ordinal());
    writer.u64(range.begin);
    writer.u64(range.count);
    writer.u32(range.reserved);
  }
  writer.u64(stage.evaluation_count);
  writer.u32(stage.reserved);
  return writer.take();
}

template <class T, class I>
bool verify_candidate_source_facet_relation_stage(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_facet_relation_stage<T> &edge_facet_stage,
    const bounded_boolean_digest &semantic_namespace, T residual_boundary,
    const relation_capabilities &capabilities,
    const candidate_source_facet_relation_stage<T> &stage,
    bounded_boolean_error &error) {
  using namespace candidate_source_facet_relation_detail;
  auto fail = [&](relation_subcode subcode, const char *summary) {
    error = source_facet_relation_error(subcode, summary,
                                        relation_checkpoint::independent_verification);
    return false;
  };
  if (stage.schema_version !=
          contract_versions::relation_source_facet_facet_stage_schema ||
      stage.policy_version !=
          contract_versions::relation_source_facet_facet_stage_policy ||
      !stage.owner.same_owner(capabilities.owner) || stage.reserved != 0 ||
      stage.evaluation_count != stage.relations.size() ||
      stage.relations.size() != stage.request_graph.requests.size() ||
      stage.candidate_ranges.size() != candidates.candidates().size())
    return fail(relation_subcode::source_facet_relation_invariant,
                "Component 07 facet/facet stage header is malformed");

  std::vector<relation_request_proposal> proposals;
  bounded_boolean_error proposal_error;
  if (!append_candidate_proposals(candidates, semantic_namespace, proposals,
                                  capabilities, proposal_error)) {
    error = proposal_error;
    return false;
  }
  auto expected_graph = build_relation_request_graph(std::move(proposals),
                                                      capabilities);
  if (!expected_graph.has_value()) {
    error = *expected_graph.error();
    return false;
  }
  if (encode_relation_request_graph_semantics(*expected_graph.value()) !=
      encode_relation_request_graph_semantics(stage.request_graph))
    return fail(relation_subcode::source_facet_relation_invariant,
                "Component 07 facet/facet request graph did not reconstruct");

  for (std::size_t index = 0; index < stage.relations.size(); ++index) {
    const auto &request = stage.request_graph.requests[index];
    source_facet_support_input<T> first;
    source_facet_support_input<T> second;
    if (request.id.ordinal() != index ||
        request.key.family !=
            relation_request_family::source_facet_source_facet ||
        !make_support_input(candidates, request.key.first, first) ||
        !make_support_input(candidates, request.key.second, second))
      return fail(relation_subcode::source_facet_relation_invariant,
                  "Component 07 facet/facet verifier input reconstruction failed");
    auto reconstructed = classify_source_facet_support_relation(
        first, second, capabilities.owner, residual_boundary,
        edge_facet_consumers(candidates, edge_facet_stage, request.key.first,
                             request.key.second));
    if (!reconstructed.has_value()) {
      error = *reconstructed.error();
      return false;
    }
    if (!valid_source_facet_relation_record(stage.relations[index]) ||
        !relation_semantics_equal(stage.relations[index],
                                  *reconstructed.value()))
      return fail(relation_subcode::source_facet_relation_invariant,
                  "Component 07 facet/facet numerical record did not reconstruct");
  }

  std::vector<std::vector<relation_request_id>> expected_links(
      candidates.candidates().size());
  for (const auto &request : stage.request_graph.requests) {
    if (request.witness_begin > stage.request_graph.candidate_witnesses.size() ||
        request.witness_count > stage.request_graph.candidate_witnesses.size() -
                                    request.witness_begin)
      return fail(relation_subcode::source_facet_relation_invariant,
                  "Component 07 facet/facet witness range is malformed");
    for (std::uint64_t offset = 0; offset < request.witness_count; ++offset) {
      const auto witness = stage.request_graph.candidate_witnesses[
          request.witness_begin + offset];
      if (witness.ordinal() >= expected_links.size())
        return fail(relation_subcode::source_facet_relation_invariant,
                    "Component 07 facet/facet witness is out of range");
      expected_links[witness.ordinal()].push_back(request.id);
    }
  }
  std::uint64_t expected_begin = 0;
  for (std::size_t candidate = 0; candidate < expected_links.size(); ++candidate) {
    auto &links = expected_links[candidate];
    std::sort(links.begin(), links.end());
    links.erase(std::unique(links.begin(), links.end()), links.end());
    const auto &range = stage.candidate_ranges[candidate];
    if (range.candidate.ordinal() != candidate || range.begin != expected_begin ||
        range.count != links.size() || range.reserved != 0 ||
        range.begin > stage.candidate_relations.size() ||
        range.count > stage.candidate_relations.size() - range.begin)
      return fail(relation_subcode::source_facet_relation_invariant,
                  "Component 07 facet/facet candidate range is malformed");
    for (std::size_t offset = 0; offset < links.size(); ++offset)
      if (stage.candidate_relations[range.begin + offset] != links[offset])
        return fail(relation_subcode::source_facet_relation_invariant,
                    "Component 07 facet/facet candidate links are incomplete");
    expected_begin += links.size();
  }
  if (expected_begin != stage.candidate_relations.size())
    return fail(relation_subcode::source_facet_relation_invariant,
                "Component 07 facet/facet candidate links contain trailing data");
  if (stage.semantic_digest != sha256::digest(
                                   encode_candidate_source_facet_relation_semantics(
                                       stage)))
    return fail(relation_subcode::digest_mismatch,
                "Component 07 facet/facet integration digest mismatch");
  auto owner_changed = stage;
  owner_changed.owner = context_owner_token::create();
  owner_changed.request_graph.owner = owner_changed.owner;
  if (encode_candidate_source_facet_relation_semantics(owner_changed) !=
      encode_candidate_source_facet_relation_semantics(stage))
    return fail(relation_subcode::owner_in_semantics,
                "Component 07 facet/facet semantics encoded a runtime owner");
  return true;
}

template <class T, class I>
boolean_outcome<candidate_source_facet_relation_stage<T>>
build_candidate_source_facet_relations(
    const canonical_candidate_stream<T, I> &candidates,
    const candidate_source_edge_facet_relation_stage<T> &edge_facet_stage,
    const bounded_boolean_digest &semantic_namespace, T residual_boundary,
    const relation_capabilities &capabilities) {
  using stage_type = candidate_source_facet_relation_stage<T>;
  using namespace candidate_source_facet_relation_detail;
  try {
    if (!capabilities.owner.anchor ||
        !capabilities.owner.same_owner(candidates.owner()) ||
        !capabilities.owner.same_owner(edge_facet_stage.owner) ||
        !finite_bits(residual_boundary) || residual_boundary < T(0))
      return boolean_outcome<stage_type>::failure(source_facet_relation_error(
          relation_subcode::source_facet_relation_malformed,
          "Component 07 facet/facet integration handshake failed",
          relation_checkpoint::predecessor_validation));

    std::vector<relation_request_proposal> proposals;
    const auto candidate_count =
        static_cast<std::uint64_t>(candidates.candidates().size());
    const auto proposal_reserve =
        candidate_count > capabilities.maximum_requests / 2
            ? capabilities.maximum_requests
            : std::min<std::uint64_t>(candidate_count * 2,
                                      capabilities.maximum_requests);
    if (proposal_reserve <=
        static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()))
      proposals.reserve(static_cast<std::size_t>(proposal_reserve));
    bounded_boolean_error discovery_error;
    if (!append_candidate_proposals(candidates, semantic_namespace, proposals,
                                    capabilities, discovery_error))
      return boolean_outcome<stage_type>::failure(discovery_error);
    auto graph = build_relation_request_graph(std::move(proposals), capabilities);
    if (!graph.has_value())
      return boolean_outcome<stage_type>::failure(*graph.error());
    if (graph.value()->requests.size() > capabilities.maximum_relations)
      return boolean_outcome<stage_type>::failure(source_facet_relation_error(
          relation_subcode::work_limit,
          "Component 07 facet/facet relation limit exceeded",
          relation_checkpoint::count_representability_preflight,
          bounded_boolean_error_category::resource_limit));

    stage_type stage;
    stage.owner = capabilities.owner;
    stage.request_graph = std::move(*graph.value());
    stage.relations.reserve(stage.request_graph.requests.size());
    for (const auto &request : stage.request_graph.requests) {
      if (relation_cancelled(capabilities))
        return boolean_outcome<stage_type>::failure(source_facet_relation_error(
            relation_subcode::cancelled,
            "Component 07 facet/facet evaluation cancelled",
            relation_checkpoint::facet_facet_evaluation,
            bounded_boolean_error_category::cancelled));
      source_facet_support_input<T> first;
      source_facet_support_input<T> second;
      if (!make_support_input(candidates, request.key.first, first) ||
          !make_support_input(candidates, request.key.second, second))
        return boolean_outcome<stage_type>::failure(source_facet_relation_error(
            relation_subcode::source_facet_relation_malformed,
            "Component 07 facet/facet producer could not reconstruct source supports"));
      auto relation = classify_source_facet_support_relation(
          first, second, capabilities.owner, residual_boundary,
          edge_facet_consumers(candidates, edge_facet_stage, request.key.first,
                               request.key.second));
      if (!relation.has_value())
        return boolean_outcome<stage_type>::failure(*relation.error());
      stage.relations.push_back(std::move(*relation.value()));
      ++stage.evaluation_count;
    }

    std::vector<std::vector<relation_request_id>> candidate_links(
        candidates.candidates().size());
    for (const auto &request : stage.request_graph.requests)
      for (std::uint64_t offset = 0; offset < request.witness_count; ++offset) {
        const auto witness = stage.request_graph.candidate_witnesses[
            request.witness_begin + offset];
        if (witness.ordinal() >= candidate_links.size())
          return boolean_outcome<stage_type>::failure(source_facet_relation_error(
              relation_subcode::source_facet_relation_invariant,
              "Component 07 facet/facet witness is out of range",
              relation_checkpoint::event_seed_and_disposition_reconciliation));
        candidate_links[witness.ordinal()].push_back(request.id);
      }
    stage.candidate_ranges.reserve(candidate_links.size());
    for (std::size_t candidate = 0; candidate < candidate_links.size();
         ++candidate) {
      auto &links = candidate_links[candidate];
      std::sort(links.begin(), links.end());
      links.erase(std::unique(links.begin(), links.end()), links.end());
      candidate_source_facet_relation_range range;
      range.candidate = candidate_id(candidate);
      range.begin = stage.candidate_relations.size();
      range.count = links.size();
      stage.candidate_relations.insert(stage.candidate_relations.end(),
                                       links.begin(), links.end());
      stage.candidate_ranges.push_back(range);
    }
    const auto semantic_bytes =
        encode_candidate_source_facet_relation_semantics(stage);
    if (semantic_bytes.size() > capabilities.maximum_canonical_bytes)
      return boolean_outcome<stage_type>::failure(source_facet_relation_error(
          relation_subcode::byte_count_overflow,
          "Component 07 facet/facet integration bytes exceed capabilities",
          relation_checkpoint::canonical_encoding,
          bounded_boolean_error_category::resource_limit));
    stage.semantic_digest = sha256::digest(semantic_bytes);
    bounded_boolean_error verification_error;
    if (!verify_candidate_source_facet_relation_stage(
            candidates, edge_facet_stage, semantic_namespace, residual_boundary,
            capabilities, stage, verification_error))
      return boolean_outcome<stage_type>::failure(verification_error);
    return boolean_outcome<stage_type>::success(std::move(stage));
  } catch (const std::bad_alloc &) {
    return boolean_outcome<stage_type>::failure(source_facet_relation_error(
        relation_subcode::resource_preflight,
        "Component 07 facet/facet integration allocation failed",
        relation_checkpoint::discovery_resource_reservation,
        bounded_boolean_error_category::resource_limit));
  } catch (...) {
    return boolean_outcome<stage_type>::failure(source_facet_relation_error(
        relation_subcode::internal_invariant,
        "Component 07 facet/facet integration raised an unexpected exception",
        relation_checkpoint::producer_verification));
  }
}

} // namespace ygor::mesh_boolean::bounded
