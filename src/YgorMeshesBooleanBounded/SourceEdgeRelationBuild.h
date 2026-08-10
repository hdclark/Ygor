#pragma once

#include "SourceEdgeRelationValidation.h"

namespace ygor::mesh_boolean::bounded {

template <class T>
boolean_outcome<source_edge_relation_record<T>>
classify_source_edge_relation(
    const source_edge_relation_input<T> &first,
    const source_edge_relation_input<T> &second,
    const context_owner_token &owner, T residual_boundary) {
  using namespace source_edge_relation_detail;
  static_assert(supported_precision_scalar_v<T>);

  if (!owner.anchor || !finite_bits(residual_boundary) ||
      residual_boundary < T(0) || !first.original_source_edge ||
      !second.original_source_edge || first.reserved != 0 ||
      second.reserved != 0 ||
      !valid_relation_feature_key(first.feature) ||
      !valid_relation_feature_key(second.feature) ||
      first.feature.kind != relation_feature_kind::source_edge ||
      second.feature.kind != relation_feature_kind::source_edge ||
      first.feature.operand == second.feature.operand ||
      !(first.feature < second.feature) ||
      !valid_point(first.start, owner) || !valid_point(first.end, owner) ||
      !valid_point(second.start, owner) || !valid_point(second.end, owner))
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_relation_malformed,
            "Component 07 source-edge relation input is malformed"));

  source_edge_relation_record<T> result;
  result.owner = owner;
  result.first_feature = first.feature;
  result.second_feature = second.feature;
  result.first_start = snapshot(first.start);
  result.first_end = snapshot(first.end);
  result.second_start = snapshot(second.start);
  result.second_end = snapshot(second.end);
  result.residual_boundary = residual_boundary;

  auto u = bounded_vector_subtract(first.end.coordinates,
                                   first.start.coordinates);
  auto v = bounded_vector_subtract(second.end.coordinates,
                                   second.start.coordinates);
  auto w = bounded_vector_subtract(second.start.coordinates,
                                   first.start.coordinates);
  if (!u.has_value() || !v.has_value() || !w.has_value())
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_relation_malformed,
            "Component 07 source-edge direction construction failed"));

  auto u_norm = bounded_squared_norm(*u.value());
  auto v_norm = bounded_squared_norm(*v.value());
  if (!u_norm.has_value() || !v_norm.has_value() ||
      !definite_positive(*u_norm.value()) ||
      !definite_positive(*v_norm.value()))
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_direction_degenerate,
            "Component 07 source-edge direction is not definitely nondegenerate"));

  auto normal = bounded_cross3(*u.value(), *v.value());
  if (!normal.has_value())
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_support_unresolved,
            "Component 07 source-edge support cross product failed"));
  auto parallel_measure = bounded_squared_norm(*normal.value());
  if (!parallel_measure.has_value())
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_support_unresolved,
            "Component 07 source-edge parallel measure failed"));

  const auto a0 = nominal(first.start);
  const auto a1 = nominal(first.end);
  const auto b0 = nominal(second.start);
  const auto b1 = nominal(second.end);
  const auto exact_parallel =
      exact_segment_directions_parallel_3d(a0, a1, b0, b1);
  auto parallel_truth =
      make_truth(std::move(*parallel_measure.value()), exact_parallel,
                 rounded_operation_code::squared_norm);
  if (!parallel_truth.has_value())
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        *parallel_truth.error());
  result.parallel_truth = std::move(*parallel_truth.value());

  if (accepted_nonzero<T>(result.parallel_truth)) {
    auto coplanarity_measure = bounded_dot3(*w.value(), *normal.value());
    if (!coplanarity_measure.has_value())
      return boolean_outcome<source_edge_relation_record<T>>::failure(
          source_edge_relation_error(
              relation_subcode::source_edge_support_unresolved,
              "Component 07 source-edge coplanarity measure failed"));
    const auto exact_coplanarity =
        exact_coplanarity_3d(a0, a1, b0, b1);
    auto truth =
        make_truth(std::move(*coplanarity_measure.value()),
                   exact_coplanarity, rounded_operation_code::dot3);
    if (!truth.has_value())
      return boolean_outcome<source_edge_relation_record<T>>::failure(
          *truth.error());
    result.coplanarity_truth = std::move(*truth.value());
    result.has_coplanarity_truth = true;

    if (accepted_nonzero<T>(result.coplanarity_truth)) {
      result.support = source_edge_support_class::skew_separated;
      result.contact = source_edge_contact_class::none;
      result.semantic_digest =
          sha256::digest(encode_source_edge_relation_semantics(result));
      if (!valid_source_edge_relation_record(result))
        return boolean_outcome<source_edge_relation_record<T>>::failure(
            source_edge_relation_error(
                relation_subcode::source_edge_relation_invariant,
                "Component 07 skew source-edge relation failed validation"));
      return boolean_outcome<source_edge_relation_record<T>>::success(
          std::move(result));
    }
    if (!zero_tie<T>(result.coplanarity_truth))
      return boolean_outcome<source_edge_relation_record<T>>::failure(
          source_edge_relation_error(
              relation_subcode::source_edge_support_unresolved,
              "Component 07 source-edge coplanarity remains unresolved"));

    result.support = source_edge_support_class::nonparallel_coplanar;
    auto minor_axis =
        select_vector_axis(*normal.value(),
                           relation_subcode::source_edge_parameter_unresolved,
                           "Component 07 has no definitely invertible source-edge minor");
    if (!minor_axis.has_value())
      return boolean_outcome<source_edge_relation_record<T>>::failure(
          *minor_axis.error());
    result.selected_minor_axis = *minor_axis.value();
    constexpr std::array<std::array<std::size_t, 2>, 3> projected_axes{{
        {{1, 2}},
        {{2, 0}},
        {{0, 1}},
    }};
    const auto axes = projected_axes[result.selected_minor_axis];

    auto denominator = bounded_determinant2(
        u.value()->components[axes[0]], u.value()->components[axes[1]],
        v.value()->components[axes[0]], v.value()->components[axes[1]]);
    auto first_numerator = bounded_determinant2(
        w.value()->components[axes[0]], w.value()->components[axes[1]],
        v.value()->components[axes[0]], v.value()->components[axes[1]]);
    auto second_numerator = bounded_determinant2(
        w.value()->components[axes[0]], w.value()->components[axes[1]],
        u.value()->components[axes[0]], u.value()->components[axes[1]]);
    if (!denominator.has_value() || !first_numerator.has_value() ||
        !second_numerator.has_value() ||
        !definite_nonzero(*denominator.value()))
      return boolean_outcome<source_edge_relation_record<T>>::failure(
          source_edge_relation_error(
              relation_subcode::source_edge_parameter_unresolved,
              "Component 07 selected source-edge minor is not invertible"));

    auto first_scalar =
        bounded_divide(*first_numerator.value(), *denominator.value());
    auto second_scalar =
        bounded_divide(*second_numerator.value(), *denominator.value());
    if (!first_scalar.has_value() || !second_scalar.has_value())
      return boolean_outcome<source_edge_relation_record<T>>::failure(
          source_edge_relation_error(
              relation_subcode::source_edge_parameter_unresolved,
              "Component 07 source-edge parameter division failed"));

    const auto pa0 = project(a0, axes[0], axes[1]);
    const auto pa1 = project(a1, axes[0], axes[1]);
    const auto pb0 = project(b0, axes[0], axes[1]);
    const auto pb1 = project(b1, axes[0], axes[1]);
    auto first_parameter = make_parameter(
        std::move(*first_scalar.value()),
        exact_orient_2d(pa0, pb0, pb1),
        exact_orient_2d(pa1, pb0, pb1), owner);
    auto second_parameter = make_parameter(
        std::move(*second_scalar.value()),
        exact_orient_2d(pa0, pb0, pa1),
        exact_orient_2d(pa0, pb1, pa1), owner);
    if (!first_parameter.has_value() || !second_parameter.has_value())
      return boolean_outcome<source_edge_relation_record<T>>::failure(
          source_edge_relation_error(
              relation_subcode::source_edge_parameter_unresolved,
              "Component 07 source-edge parameter classification failed"));

    result.parameter_count = 1;
    result.first_parameters[0] = first_parameter.value()->evidence;
    result.second_parameters[0] = second_parameter.value()->evidence;
    if (first_parameter.value()->evidence.domain ==
            parameter_domain_status::outside ||
        second_parameter.value()->evidence.domain ==
            parameter_domain_status::outside) {
      result.contact = source_edge_contact_class::none;
      result.semantic_digest =
          sha256::digest(encode_source_edge_relation_semantics(result));
      if (!valid_source_edge_relation_record(result))
        return boolean_outcome<source_edge_relation_record<T>>::failure(
            source_edge_relation_error(
                relation_subcode::source_edge_relation_invariant,
                "Component 07 non-contact source-edge solve failed validation"));
      return boolean_outcome<source_edge_relation_record<T>>::success(
          std::move(result));
    }

    const std::uint8_t first_endpoint =
        endpoint_mask(first_parameter.value()->evidence);
    const std::uint8_t second_endpoint =
        endpoint_mask(second_parameter.value()->evidence);
    bounded_point3<T> candidate;
    bool accepted_source_vertex = true;
    if (first_endpoint == 1)
      candidate = first.start;
    else if (first_endpoint == 2)
      candidate = first.end;
    else if (second_endpoint == 1)
      candidate = second.start;
    else if (second_endpoint == 2)
      candidate = second.end;
    else {
      accepted_source_vertex = false;
      const auto first_bounded =
          as_bounded_parameter(*first_parameter.value(), owner);
      auto point =
          bounded_interpolate_from_a(first.start, first.end, first_bounded);
      if (!point.has_value())
        return boolean_outcome<source_edge_relation_record<T>>::failure(
            source_edge_relation_error(
                relation_subcode::source_edge_residual_rejected,
                "Component 07 source-edge point construction failed"));
      candidate = std::move(*point.value());
    }
    auto construction = point_construction(
        candidate, first, second, *first_parameter.value(),
        *second_parameter.value(), residual_boundary, accepted_source_vertex,
        first_endpoint, second_endpoint, owner);
    if (!construction.has_value())
      return boolean_outcome<source_edge_relation_record<T>>::failure(
          *construction.error());
    result.points[0] = std::move(*construction.value());
    result.point_count = 1;
    result.contact =
        first_parameter.value()->evidence.domain ==
                    parameter_domain_status::stable_interior &&
                second_parameter.value()->evidence.domain ==
                    parameter_domain_status::stable_interior
            ? source_edge_contact_class::proper_crossing
            : source_edge_contact_class::endpoint_contact;
    result.semantic_digest =
        sha256::digest(encode_source_edge_relation_semantics(result));
    if (!valid_source_edge_relation_record(result))
      return boolean_outcome<source_edge_relation_record<T>>::failure(
          source_edge_relation_error(
              relation_subcode::source_edge_relation_invariant,
              "Component 07 nonparallel source-edge relation failed validation"));
    return boolean_outcome<source_edge_relation_record<T>>::success(
        std::move(result));
  }

  if (!zero_tie<T>(result.parallel_truth))
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_support_unresolved,
            "Component 07 source-edge parallel support remains unresolved"));

  auto offset_cross = bounded_cross3(*w.value(), *u.value());
  if (!offset_cross.has_value())
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_support_unresolved,
            "Component 07 source-edge collinearity cross product failed"));
  auto collinearity_measure = bounded_squared_norm(*offset_cross.value());
  if (!collinearity_measure.has_value())
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_support_unresolved,
            "Component 07 source-edge collinearity measure failed"));
  const auto exact_collinearity = exact_collinearity_3d(a0, a1, b0);
  auto collinearity_truth =
      make_truth(std::move(*collinearity_measure.value()),
                 exact_collinearity, rounded_operation_code::squared_norm);
  if (!collinearity_truth.has_value())
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        *collinearity_truth.error());
  result.collinearity_truth = std::move(*collinearity_truth.value());
  result.has_collinearity_truth = true;

  if (accepted_nonzero<T>(result.collinearity_truth)) {
    result.support = source_edge_support_class::parallel_separated;
    result.contact = source_edge_contact_class::none;
    result.semantic_digest =
        sha256::digest(encode_source_edge_relation_semantics(result));
    if (!valid_source_edge_relation_record(result))
      return boolean_outcome<source_edge_relation_record<T>>::failure(
          source_edge_relation_error(
              relation_subcode::source_edge_relation_invariant,
              "Component 07 parallel source-edge relation failed validation"));
    return boolean_outcome<source_edge_relation_record<T>>::success(
        std::move(result));
  }
  if (!zero_tie<T>(result.collinearity_truth))
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_support_unresolved,
            "Component 07 source-edge collinearity remains unresolved"));

  result.support = source_edge_support_class::collinear;
  auto axis = select_vector_axis(
      *u.value(), relation_subcode::source_edge_parameter_unresolved,
      "Component 07 has no stable collinear source-edge carrier axis");
  if (!axis.has_value())
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        *axis.error());
  result.selected_collinear_axis = *axis.value();
  const std::size_t selected = result.selected_collinear_axis;
  if (!definite_nonzero(v.value()->components[selected]))
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_parameter_unresolved,
            "Component 07 opposite collinear edge axis is not stable"));

  auto q0_numerator = bounded_subtract(
      second.start.coordinates.components[selected],
      first.start.coordinates.components[selected]);
  auto q1_numerator = bounded_subtract(
      second.end.coordinates.components[selected],
      first.start.coordinates.components[selected]);
  auto r0_numerator = bounded_subtract(
      first.start.coordinates.components[selected],
      second.start.coordinates.components[selected]);
  auto r1_numerator = bounded_subtract(
      first.end.coordinates.components[selected],
      second.start.coordinates.components[selected]);
  if (!q0_numerator.has_value() || !q1_numerator.has_value() ||
      !r0_numerator.has_value() || !r1_numerator.has_value())
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_parameter_unresolved,
            "Component 07 collinear source-edge numerator failed"));

  auto q0_scalar =
      bounded_divide(*q0_numerator.value(), u.value()->components[selected]);
  auto q1_scalar =
      bounded_divide(*q1_numerator.value(), u.value()->components[selected]);
  auto r0_scalar =
      bounded_divide(*r0_numerator.value(), v.value()->components[selected]);
  auto r1_scalar =
      bounded_divide(*r1_numerator.value(), v.value()->components[selected]);
  if (!q0_scalar.has_value() || !q1_scalar.has_value() ||
      !r0_scalar.has_value() || !r1_scalar.has_value())
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_parameter_unresolved,
            "Component 07 collinear source-edge parameter division failed"));

  auto q0 = make_parameter(
      std::move(*q0_scalar.value()),
      exact_scalar_comparison(b0[selected], a0[selected]),
      exact_scalar_comparison(b0[selected], a1[selected]), owner);
  auto q1 = make_parameter(
      std::move(*q1_scalar.value()),
      exact_scalar_comparison(b1[selected], a0[selected]),
      exact_scalar_comparison(b1[selected], a1[selected]), owner);
  auto r0 = make_parameter(
      std::move(*r0_scalar.value()),
      exact_scalar_comparison(a0[selected], b0[selected]),
      exact_scalar_comparison(a0[selected], b1[selected]), owner);
  auto r1 = make_parameter(
      std::move(*r1_scalar.value()),
      exact_scalar_comparison(a1[selected], b0[selected]),
      exact_scalar_comparison(a1[selected], b1[selected]), owner);
  if (!q0.has_value() || !q1.has_value() || !r0.has_value() ||
      !r1.has_value())
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_parameter_unresolved,
            "Component 07 collinear source-edge parameter classification failed"));

  const auto &u_axis = u.value()->components[selected].uncertainty_enclosure;
  const auto &v_axis = v.value()->components[selected].uncertainty_enclosure;
  const bool u_positive = u_axis.lower() > T(0);
  const bool u_negative = u_axis.upper() < T(0);
  const bool v_positive = v_axis.lower() > T(0);
  const bool v_negative = v_axis.upper() < T(0);
  if ((!u_positive && !u_negative) || (!v_positive && !v_negative))
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_parameter_unresolved,
            "Component 07 collinear edge orientation is unresolved"));
  const bool same_orientation =
      (u_positive && v_positive) || (u_negative && v_negative);
  result.orientation = same_orientation
                           ? source_edge_orientation_relation::same
                           : source_edge_orientation_relation::opposite;

  parameter_work<T> *b_min = same_orientation ? q0.value() : q1.value();
  parameter_work<T> *b_max = same_orientation ? q1.value() : q0.value();
  const bounded_point3<T> *b_min_point =
      same_orientation ? &second.start : &second.end;
  const bounded_point3<T> *b_max_point =
      same_orientation ? &second.end : &second.start;

  if (!definitely_before(b_min->evidence, b_max->evidence))
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_parameter_unresolved,
            "Component 07 collinear endpoint order is unresolved"));

  auto zero = constant_parameter(owner, T(0));
  auto one = constant_parameter(owner, T(1));
  if (!zero.has_value() || !one.has_value())
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_parameter_unresolved,
            "Component 07 endpoint parameter construction failed"));

  result.parameter_count = 2;
  result.first_parameters[0] = q0.value()->evidence;
  result.second_parameters[0] = zero.value()->evidence;
  result.first_parameters[1] = q1.value()->evidence;
  result.second_parameters[1] = one.value()->evidence;

  if (b_max->evidence.enclosure.upper() < T(0) ||
      b_min->evidence.enclosure.lower() > T(1)) {
    result.contact = source_edge_contact_class::none;
    result.semantic_digest =
        sha256::digest(encode_source_edge_relation_semantics(result));
    if (!valid_source_edge_relation_record(result))
      return boolean_outcome<source_edge_relation_record<T>>::failure(
          source_edge_relation_error(
              relation_subcode::source_edge_relation_invariant,
              "Component 07 disjoint collinear relation failed validation"));
    return boolean_outcome<source_edge_relation_record<T>>::success(
        std::move(result));
  }

  paired_parameter_source<T> start_source;
  paired_parameter_source<T> end_source;
  if (exact_parameter_at(b_min->evidence, T(0)) ||
      b_min->evidence.enclosure.upper() < T(0)) {
    start_source.first = *zero.value();
    start_source.second = *r0.value();
    start_source.point = &first.start;
    start_source.accepted_source_vertex = true;
  } else if (b_min->evidence.enclosure.lower() > T(0)) {
    start_source.first = *b_min;
    start_source.second =
        same_orientation ? *zero.value() : *one.value();
    start_source.point = b_min_point;
    start_source.accepted_source_vertex = true;
  } else {
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_parameter_unresolved,
            "Component 07 collinear overlap start is unresolved"));
  }

  if (exact_parameter_at(b_max->evidence, T(1)) ||
      b_max->evidence.enclosure.lower() > T(1)) {
    end_source.first = *one.value();
    end_source.second = *r1.value();
    end_source.point = &first.end;
    end_source.accepted_source_vertex = true;
  } else if (b_max->evidence.enclosure.upper() < T(1)) {
    end_source.first = *b_max;
    end_source.second =
        same_orientation ? *one.value() : *zero.value();
    end_source.point = b_max_point;
    end_source.accepted_source_vertex = true;
  } else {
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_parameter_unresolved,
            "Component 07 collinear overlap end is unresolved"));
  }

  const bool point_at_zero =
      exact_parameter_at(start_source.first.evidence, T(0)) &&
      exact_parameter_at(end_source.first.evidence, T(0));
  const bool point_at_one =
      exact_parameter_at(start_source.first.evidence, T(1)) &&
      exact_parameter_at(end_source.first.evidence, T(1));
  if (point_at_zero || point_at_one) {
    const auto &source = point_at_zero ? start_source : end_source;
    result.parameter_count = 1;
    result.first_parameters[0] = source.first.evidence;
    result.second_parameters[0] = source.second.evidence;
    auto construction = point_construction(
        *source.point, first, second, source.first, source.second,
        residual_boundary, true, endpoint_mask(source.first.evidence),
        endpoint_mask(source.second.evidence), owner);
    if (!construction.has_value())
      return boolean_outcome<source_edge_relation_record<T>>::failure(
          *construction.error());
    result.points[0] = std::move(*construction.value());
    result.point_count = 1;
    result.contact = source_edge_contact_class::point_contact;
  } else {
    if (!definitely_before(start_source.first.evidence,
                           end_source.first.evidence))
      return boolean_outcome<source_edge_relation_record<T>>::failure(
          source_edge_relation_error(
              relation_subcode::source_edge_parameter_unresolved,
              "Component 07 collinear overlap interval order is unresolved"));
    result.first_parameters[0] = start_source.first.evidence;
    result.second_parameters[0] = start_source.second.evidence;
    result.first_parameters[1] = end_source.first.evidence;
    result.second_parameters[1] = end_source.second.evidence;
    auto start_construction = point_construction(
        *start_source.point, first, second, start_source.first,
        start_source.second, residual_boundary, true,
        endpoint_mask(start_source.first.evidence),
        endpoint_mask(start_source.second.evidence), owner);
    auto end_construction = point_construction(
        *end_source.point, first, second, end_source.first,
        end_source.second, residual_boundary, true,
        endpoint_mask(end_source.first.evidence),
        endpoint_mask(end_source.second.evidence), owner);
    if (!start_construction.has_value() || !end_construction.has_value())
      return boolean_outcome<source_edge_relation_record<T>>::failure(
          source_edge_relation_error(
              relation_subcode::source_edge_residual_rejected,
              "Component 07 collinear overlap endpoint validation failed"));
    result.points[0] = std::move(*start_construction.value());
    result.points[1] = std::move(*end_construction.value());
    result.point_count = 2;

    const bool b_min_at_zero =
        exact_parameter_at(b_min->evidence, T(0));
    const bool b_max_at_one =
        exact_parameter_at(b_max->evidence, T(1));
    if (b_min_at_zero && b_max_at_one)
      result.contact = source_edge_contact_class::equal;
    else if (b_min->evidence.enclosure.lower() > T(0) &&
             b_max->evidence.enclosure.upper() < T(1))
      result.contact = source_edge_contact_class::first_contains_second;
    else if (b_min->evidence.enclosure.upper() < T(0) &&
             b_max->evidence.enclosure.lower() > T(1))
      result.contact = source_edge_contact_class::second_contains_first;
    else
      result.contact = source_edge_contact_class::partial_overlap;
  }

  result.semantic_digest =
      sha256::digest(encode_source_edge_relation_semantics(result));
  if (!valid_source_edge_relation_record(result))
    return boolean_outcome<source_edge_relation_record<T>>::failure(
        source_edge_relation_error(
            relation_subcode::source_edge_relation_invariant,
            "Component 07 collinear source-edge relation failed validation"));
  return boolean_outcome<source_edge_relation_record<T>>::success(
      std::move(result));
}

} // namespace ygor::mesh_boolean::bounded
