#pragma once

#include "SourceEdgeRelationTypes.h"

namespace ygor::mesh_boolean::bounded {

namespace source_edge_relation_detail {

template <class T>
bool valid_interval(const finite_interval<T> &value) noexcept {
  return finite_bits(value.lower()) && finite_bits(value.upper()) &&
         !finite_numeric_less(value.upper(), value.lower());
}

template <class T>
bool valid_scalar(const bounded_scalar<T> &value,
                  const context_owner_token &owner) noexcept {
  return bounded_operations_detail::bounded_scalar_valid(value) &&
         value.identity.owner.same_owner(owner);
}

template <class T>
bool valid_point(const bounded_point3<T> &point,
                 const context_owner_token &owner) noexcept {
  if (!point.owner.same_owner(owner) ||
      !point.coordinates.owner.same_owner(owner))
    return false;
  for (const auto &component : point.coordinates.components)
    if (!valid_scalar(component, owner))
      return false;
  return finite_bits(point.coordinates.radial_error_upper) &&
         point.coordinates.radial_error_upper >= T(0);
}

template <class T>
source_edge_geometry_snapshot<T>
snapshot(const bounded_point3<T> &point) noexcept {
  source_edge_geometry_snapshot<T> result;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    result.rounded_nominal[axis] =
        point.coordinates.components[axis].rounded_nominal;
    result.enclosure[axis] =
        point.coordinates.components[axis].uncertainty_enclosure;
  }
  result.provenance = point.provenance.ordinal();
  result.lineage = point.lineage.ordinal();
  return result;
}

template <class T>
std::array<T, 3> nominal(const bounded_point3<T> &point) noexcept {
  return {{
      point.coordinates.components[0].rounded_nominal,
      point.coordinates.components[1].rounded_nominal,
      point.coordinates.components[2].rounded_nominal,
  }};
}

template <class T>
bool valid_snapshot(const source_edge_geometry_snapshot<T> &point) noexcept {
  for (std::size_t axis = 0; axis < 3; ++axis)
    if (!finite_bits(point.rounded_nominal[axis]) ||
        !valid_interval(point.enclosure[axis]) ||
        !point.enclosure[axis].contains(point.rounded_nominal[axis]))
      return false;
  return true;
}

inline bool valid_exact_status(exact_relation_status value) noexcept {
  switch (value) {
  case exact_relation_status::exact_negative:
  case exact_relation_status::exact_zero:
  case exact_relation_status::exact_positive:
  case exact_relation_status::unavailable:
    return true;
  case exact_relation_status::invalid:
    return false;
  }
  return false;
}

template <class T>
T minimum_absolute(const finite_interval<T> &value) noexcept {
  if (value.lower() > T(0))
    return value.lower();
  if (value.upper() < T(0))
    return -value.upper();
  return T(0);
}

template <class T>
boolean_outcome<std::uint8_t>
select_vector_axis(const bounded_vec3<T> &value,
                   relation_subcode failure_subcode,
                   const char *failure_summary) {
  std::uint8_t selected = 3;
  T selected_square = T(0);
  for (std::uint8_t axis = 0; axis < 3; ++axis) {
    const T lower_absolute =
        minimum_absolute(value.components[axis].uncertainty_enclosure);
    if (!(lower_absolute > T(0)))
      continue;
    const auto square = directed_multiply(lower_absolute, lower_absolute);
    if (!square || !(square.value.lower > T(0)))
      return boolean_outcome<std::uint8_t>::failure(
          source_edge_relation_error(
              failure_subcode, failure_summary));
    if (selected == 3 ||
        finite_numeric_less(selected_square, square.value.lower)) {
      selected = axis;
      selected_square = square.value.lower;
    }
  }
  if (selected == 3)
    return boolean_outcome<std::uint8_t>::failure(
        source_edge_relation_error(failure_subcode, failure_summary));
  return boolean_outcome<std::uint8_t>::success(selected);
}

template <class T>
bool exact_zero(exact_relation_status value) noexcept {
  return value == exact_relation_status::exact_zero;
}

template <class T>
boolean_outcome<source_edge_parameter_evidence<T>>
parameter_evidence(const bounded_scalar<T> &value,
                   exact_relation_status zero_relation,
                   exact_relation_status one_relation,
                   const context_owner_token &owner) {
  if (!valid_scalar(value, owner) || !valid_exact_status(zero_relation) ||
      !valid_exact_status(one_relation) ||
      (zero_relation == exact_relation_status::exact_zero &&
       one_relation == exact_relation_status::exact_zero))
    return boolean_outcome<source_edge_parameter_evidence<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_parameter_unresolved,
            "Component 07 source-edge parameter evidence is malformed"));

  source_edge_parameter_evidence<T> result;
  result.rounded_nominal = value.rounded_nominal;
  result.enclosure = value.uncertainty_enclosure;
  result.exact_zero = zero_relation;
  result.exact_one = one_relation;
  result.contributors = value.contributors;
  result.trace_root = value.identity.trace_root;

  const bool at_zero = zero_relation == exact_relation_status::exact_zero;
  const bool at_one = one_relation == exact_relation_status::exact_zero;
  if (at_zero || at_one) {
    const T endpoint = at_zero ? T(0) : T(1);
    if (!result.enclosure.contains(endpoint))
      return boolean_outcome<source_edge_parameter_evidence<T>>::failure(
          source_edge_relation_error(
              relation_subcode::source_edge_parameter_unresolved,
              "Component 07 exact endpoint parameter escaped its enclosure"));
    result.domain = parameter_domain_status::stable_endpoint;
    result.domain_margin = T(0);
    return boolean_outcome<source_edge_parameter_evidence<T>>::success(
        std::move(result));
  }

  if (result.enclosure.lower() > T(0) &&
      result.enclosure.upper() < T(1)) {
    result.domain = parameter_domain_status::stable_interior;
    const T lower_margin = result.enclosure.lower();
    const auto upper_margin =
        directed_subtract(T(1), result.enclosure.upper());
    if (!upper_margin)
      return boolean_outcome<source_edge_parameter_evidence<T>>::failure(
          source_edge_relation_error(
              relation_subcode::source_edge_parameter_unresolved,
              "Component 07 source-edge parameter margin is unavailable"));
    result.domain_margin =
        finite_numeric_less(lower_margin, upper_margin.value.lower)
            ? lower_margin
            : upper_margin.value.lower;
    return boolean_outcome<source_edge_parameter_evidence<T>>::success(
        std::move(result));
  }

  if (result.enclosure.upper() < T(0)) {
    result.domain = parameter_domain_status::outside;
    result.domain_margin = -result.enclosure.upper();
    return boolean_outcome<source_edge_parameter_evidence<T>>::success(
        std::move(result));
  }
  if (result.enclosure.lower() > T(1)) {
    result.domain = parameter_domain_status::outside;
    const auto margin = directed_subtract(result.enclosure.lower(), T(1));
    if (!margin)
      return boolean_outcome<source_edge_parameter_evidence<T>>::failure(
          source_edge_relation_error(
              relation_subcode::source_edge_parameter_unresolved,
              "Component 07 source-edge outside margin is unavailable"));
    result.domain_margin = margin.value.lower;
    return boolean_outcome<source_edge_parameter_evidence<T>>::success(
        std::move(result));
  }

  return boolean_outcome<source_edge_parameter_evidence<T>>::failure(
      source_edge_relation_error(
          relation_subcode::source_edge_parameter_unresolved,
          "Component 07 source-edge parameter overlaps a topology boundary"));
}

template <class T>
struct parameter_work final {
  bounded_scalar<T> scalar{};
  source_edge_parameter_evidence<T> evidence{};
};

template <class T>
boolean_outcome<parameter_work<T>>
make_parameter(bounded_scalar<T> scalar,
               const exact_relation_record &zero_relation,
               const exact_relation_record &one_relation,
               const context_owner_token &owner) {
  if (zero_relation.evaluation_status != numeric_status::success ||
      one_relation.evaluation_status != numeric_status::success)
    return boolean_outcome<parameter_work<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_parameter_unresolved,
            "Component 07 exact parameter endpoint relation failed"));
  auto evidence = parameter_evidence(
      scalar, zero_relation.status, one_relation.status, owner);
  if (!evidence.has_value())
    return boolean_outcome<parameter_work<T>>::failure(*evidence.error());
  parameter_work<T> result;
  result.scalar = std::move(scalar);
  result.evidence = std::move(*evidence.value());
  return boolean_outcome<parameter_work<T>>::success(std::move(result));
}

template <class T>
boolean_outcome<parameter_work<T>>
constant_parameter(const context_owner_token &owner, T value) {
  auto scalar = checked_bounded_singleton(owner, value);
  if (!scalar.has_value())
    return boolean_outcome<parameter_work<T>>::failure(*scalar.error());
  exact_relation_record zero = exact_scalar_comparison(value, T(0));
  exact_relation_record one = exact_scalar_comparison(value, T(1));
  return make_parameter(std::move(*scalar.value()), zero, one, owner);
}

template <class T>
bounded_parameter<T>
as_bounded_parameter(const parameter_work<T> &parameter,
                     const context_owner_token &owner) {
  bounded_parameter<T> result;
  result.owner = owner;
  result.value = parameter.scalar;
  result.carrier = parameter_carrier::edge;
  result.endpoints = endpoint_convention::closed;
  result.domain = parameter.evidence.domain;
  result.domain_margin = parameter.evidence.domain_margin;
  return result;
}

template <class T>
bool residual_accepted(const finite_interval<T> &residual,
                       T boundary) noexcept {
  return valid_interval(residual) && residual.contains(T(0)) &&
         !finite_numeric_less(residual.lower(), -boundary) &&
         !finite_numeric_less(boundary, residual.upper());
}

template <class T>
boolean_outcome<source_edge_point_construction<T>>
point_construction(const bounded_point3<T> &candidate,
                   const source_edge_relation_input<T> &first,
                   const source_edge_relation_input<T> &second,
                   const parameter_work<T> &first_parameter,
                   const parameter_work<T> &second_parameter,
                   T residual_boundary, bool accepted_source_vertex,
                   std::uint8_t first_endpoint_mask,
                   std::uint8_t second_endpoint_mask,
                   const context_owner_token &owner) {
  const auto first_bounded = as_bounded_parameter(first_parameter, owner);
  const auto second_bounded = as_bounded_parameter(second_parameter, owner);
  auto first_reconstructed =
      bounded_interpolate_from_a(first.start, first.end, first_bounded);
  auto second_reconstructed =
      bounded_interpolate_from_a(second.start, second.end, second_bounded);
  if (!first_reconstructed.has_value() || !second_reconstructed.has_value())
    return boolean_outcome<source_edge_point_construction<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_residual_rejected,
            "Component 07 source-edge carrier reconstruction failed"));

  auto first_residual = bounded_vector_subtract(
      candidate.coordinates, first_reconstructed.value()->coordinates);
  auto second_residual = bounded_vector_subtract(
      candidate.coordinates, second_reconstructed.value()->coordinates);
  if (!first_residual.has_value() || !second_residual.has_value())
    return boolean_outcome<source_edge_point_construction<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_residual_rejected,
            "Component 07 source-edge carrier residual failed"));

  source_edge_point_construction<T> result;
  result.point = snapshot(candidate);
  result.accepted_source_vertex = accepted_source_vertex;
  result.first_endpoint_owner_mask = first_endpoint_mask;
  result.second_endpoint_owner_mask = second_endpoint_mask;
  result.tolerance_compatible = true;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    result.first_carrier_residual[axis] =
        first_residual.value()->components[axis].uncertainty_enclosure;
    result.second_carrier_residual[axis] =
        second_residual.value()->components[axis].uncertainty_enclosure;
    if (!residual_accepted(result.first_carrier_residual[axis],
                           residual_boundary) ||
        !residual_accepted(result.second_carrier_residual[axis],
                           residual_boundary))
      return boolean_outcome<source_edge_point_construction<T>>::failure(
          source_edge_relation_error(
              relation_subcode::source_edge_residual_rejected,
              "Component 07 source-edge construction residual exceeds tolerance"));
  }
  return boolean_outcome<source_edge_point_construction<T>>::success(
      std::move(result));
}

} // namespace source_edge_relation_detail

} // namespace ygor::mesh_boolean::bounded
