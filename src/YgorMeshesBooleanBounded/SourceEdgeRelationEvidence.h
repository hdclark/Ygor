#pragma once

#include "SourceEdgeRelationArithmetic.h"

namespace ygor::mesh_boolean::bounded {
namespace source_edge_relation_detail {

inline void encode_truth(canonical_writer &writer,
                         const relation_truth_record &truth) {
  writer.u64(truth.rounded_nominal_bits);
  writer.u8(static_cast<std::uint8_t>(truth.bounded_sign));
  writer.u8(static_cast<std::uint8_t>(truth.exact_relation));
  writer.u8(static_cast<std::uint8_t>(truth.disposition));
  writer.u16(truth.rounded_formula);
  writer.u16(truth.exact_formula);
  writer.u32(truth.reserved);
}

inline void encode_contributors(canonical_writer &writer,
                                const uncertainty_contributors &value) {
  writer.floating(value.inherited_a);
  writer.floating(value.inherited_b);
  writer.floating(value.machine_floor);
  writer.floating(value.construction);
  writer.floating(value.conditioning);
  writer.floating(value.conversion);
  writer.floating(value.prior_cleanup);
  writer.floating(value.current_cleanup);
}

template <class T>
void encode_snapshot(canonical_writer &writer,
                     const source_edge_geometry_snapshot<T> &point) {
  for (std::size_t axis = 0; axis < 3; ++axis) {
    writer.floating(point.rounded_nominal[axis]);
    writer.floating(point.enclosure[axis].lower());
    writer.floating(point.enclosure[axis].upper());
  }
  writer.u64(point.provenance);
  writer.u64(point.lineage);
}

template <class T>
void encode_parameter(canonical_writer &writer,
                      const source_edge_parameter_evidence<T> &parameter) {
  writer.floating(parameter.rounded_nominal);
  writer.floating(parameter.enclosure.lower());
  writer.floating(parameter.enclosure.upper());
  writer.u8(static_cast<std::uint8_t>(parameter.domain));
  writer.floating(parameter.domain_margin);
  writer.u8(static_cast<std::uint8_t>(parameter.exact_zero));
  writer.u8(static_cast<std::uint8_t>(parameter.exact_one));
  encode_contributors(writer, parameter.contributors);
  writer.u64(parameter.trace_root);
}

template <class T>
void encode_point(canonical_writer &writer,
                  const source_edge_point_construction<T> &point) {
  encode_snapshot(writer, point.point);
  for (const auto &residual : point.first_carrier_residual) {
    writer.floating(residual.lower());
    writer.floating(residual.upper());
  }
  for (const auto &residual : point.second_carrier_residual) {
    writer.floating(residual.lower());
    writer.floating(residual.upper());
  }
  writer.u8(point.first_endpoint_owner_mask);
  writer.u8(point.second_endpoint_owner_mask);
  writer.boolean(point.accepted_source_vertex);
  writer.boolean(point.tolerance_compatible);
  writer.u16(point.reserved16);
  writer.u32(point.reserved32);
}

template <class T>
bool parameter_valid(const source_edge_parameter_evidence<T> &parameter) {
  if (!finite_bits(parameter.rounded_nominal) ||
      !valid_interval(parameter.enclosure) ||
      !parameter.enclosure.contains(parameter.rounded_nominal) ||
      !finite_bits(parameter.domain_margin) || parameter.domain_margin < T(0) ||
      !valid_exact_status(parameter.exact_zero) ||
      !valid_exact_status(parameter.exact_one) ||
      (parameter.exact_zero == exact_relation_status::exact_zero &&
       parameter.exact_one == exact_relation_status::exact_zero))
    return false;
  switch (parameter.domain) {
  case parameter_domain_status::stable_interior:
    return parameter.enclosure.lower() > T(0) &&
           parameter.enclosure.upper() < T(1) &&
           parameter.domain_margin > T(0) &&
           parameter.exact_zero != exact_relation_status::exact_zero &&
           parameter.exact_one != exact_relation_status::exact_zero;
  case parameter_domain_status::stable_endpoint:
    return parameter.domain_margin == T(0) &&
           ((parameter.exact_zero == exact_relation_status::exact_zero &&
             parameter.enclosure.contains(T(0))) ||
            (parameter.exact_one == exact_relation_status::exact_zero &&
             parameter.enclosure.contains(T(1))));
  case parameter_domain_status::outside:
    return parameter.domain_margin > T(0) &&
           (parameter.enclosure.upper() < T(0) ||
            parameter.enclosure.lower() > T(1));
  case parameter_domain_status::overlaps_boundary:
  case parameter_domain_status::invalid:
    return false;
  }
  return false;
}

template <class T>
bool point_valid(const source_edge_point_construction<T> &point,
                 T boundary) {
  if (!valid_snapshot(point.point) || !point.tolerance_compatible ||
      point.reserved16 != 0 || point.reserved32 != 0 ||
      point.first_endpoint_owner_mask > 3 ||
      point.second_endpoint_owner_mask > 3)
    return false;
  for (const auto &residual : point.first_carrier_residual)
    if (!residual_accepted(residual, boundary))
      return false;
  for (const auto &residual : point.second_carrier_residual)
    if (!residual_accepted(residual, boundary))
      return false;
  return true;
}

template <class T>
bool exact_parameter_at(const source_edge_parameter_evidence<T> &parameter,
                        T endpoint) noexcept {
  return endpoint == T(0)
             ? parameter.exact_zero == exact_relation_status::exact_zero
             : parameter.exact_one == exact_relation_status::exact_zero;
}

template <class T>
bool definitely_before(const source_edge_parameter_evidence<T> &a,
                       const source_edge_parameter_evidence<T> &b) noexcept {
  return finite_numeric_less(a.enclosure.upper(), b.enclosure.lower());
}

template <class T>
std::uint8_t endpoint_mask(const source_edge_parameter_evidence<T> &parameter) {
  return exact_parameter_at(parameter, T(0))
             ? std::uint8_t{1}
             : exact_parameter_at(parameter, T(1)) ? std::uint8_t{2}
                                                   : std::uint8_t{0};
}

template <class T>
bool definite_positive(const bounded_scalar<T> &value) noexcept {
  return value.uncertainty_enclosure.lower() > T(0);
}

template <class T>
bool definite_nonzero(const bounded_scalar<T> &value) noexcept {
  return value.uncertainty_enclosure.upper() < T(0) ||
         value.uncertainty_enclosure.lower() > T(0);
}

template <class T>
std::array<T, 2> project(const std::array<T, 3> &point,
                         std::size_t first_axis,
                         std::size_t second_axis) noexcept {
  return {{point[first_axis], point[second_axis]}};
}

template <class T>
boolean_outcome<relation_truth_record>
make_truth(bounded_scalar<T> bounded, const exact_relation_record &exact,
           rounded_operation_code rounded_formula) {
  auto result =
      assemble_relation_truth_record(std::move(bounded), exact, rounded_formula);
  if (!result.has_value())
    return boolean_outcome<relation_truth_record>::failure(*result.error());
  return result;
}

template <class T>
bool zero_tie(const relation_truth_record &truth) noexcept {
  return truth.bounded_sign == bounded_sign_status::overlaps_boundary &&
         truth.exact_relation == exact_relation_status::exact_zero &&
         truth.disposition ==
             predicate_disposition::retain_tie_for_consumer_eligibility;
}

template <class T>
bool accepted_nonzero(const relation_truth_record &truth) noexcept {
  return (truth.bounded_sign == bounded_sign_status::definitely_negative ||
          truth.bounded_sign == bounded_sign_status::definitely_positive) &&
         truth.disposition == predicate_disposition::accept_numeric_sign &&
         truth.exact_relation != exact_relation_status::exact_zero &&
         truth.exact_relation != exact_relation_status::invalid;
}

template <class T>
struct paired_parameter_source final {
  parameter_work<T> first{};
  parameter_work<T> second{};
  const bounded_point3<T> *point = nullptr;
  bool accepted_source_vertex = false;
};

} // namespace source_edge_relation_detail
} // namespace ygor::mesh_boolean::bounded
