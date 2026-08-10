#pragma once

#include "BoundedOperations.h"
#include "PredicateResults.h"

#include <cstdint>

namespace ygor::mesh_boolean::bounded {

struct construction_tag;
using construction_id = strong_id<construction_tag>;

enum class construction_kind : std::uint8_t {
    edge_plane = 1, edge_face = 2, carrier = 3, interpolation = 4, projection = 5
};
enum class construction_category : std::uint8_t {
    stable_interior = 1,
    stable_endpoint = 2,
    exact_stored_coordinate_tie = 3,
    coplanar_or_coincident = 4,
    near_parallel_bounded = 5,
    ill_conditioned = 6,
    invalid = 7
};

template<class T>
struct construction_conditioning final {
    std::uint16_t schema_version = 1;
    context_owner_token owner{};
    construction_id id{0};
    construction_kind kind = construction_kind::edge_plane;
    std::uint16_t rounded_graph_code = 0;
    std::uint16_t exact_formula_code = 0;
    bounded_parameter<T> parameter{};
    finite_interval<T> denominator{};
    exact_relation_evidence denominator_relation{};
    bounded_residual<T> carrier_residual{};
    bounded_residual<T> support_residual{};
    bounded_point3<T> constructed_point{};
    bounded_residual<T> residual_after{};
    bool has_constructed_point = false;
    uncertainty_contributors contributors{};
    T available_tolerance{};
    T required_precision{};
    T amplification_upper{};
    bool cancellation_detected = false;
    construction_category category = construction_category::invalid;
    std::uint64_t trace_root = 0;
};

template<class T>
construction_category classify_construction(const bounded_parameter<T> &parameter,
                                             const finite_interval<T> &denominator,
                                             exact_relation_status endpoint_relation,
                                             exact_relation_status coplanar_relation,
                                             T required_precision,
                                             T available_tolerance) noexcept {
    if (!std::isfinite(required_precision) || !std::isfinite(available_tolerance) ||
        required_precision < T(0) || available_tolerance < T(0) ||
        finite_numeric_less(denominator.upper(), denominator.lower()))
        return construction_category::invalid;
    if (coplanar_relation == exact_relation_status::exact_zero)
        return construction_category::coplanar_or_coincident;
    if (denominator.contains_zero())
        return endpoint_relation == exact_relation_status::exact_zero
            ? construction_category::exact_stored_coordinate_tie
            : construction_category::ill_conditioned;
    if (required_precision > available_tolerance) return construction_category::ill_conditioned;
    if (parameter.value.uncertainty_enclosure.lower() > T(0) &&
        parameter.value.uncertainty_enclosure.upper() < T(1))
        return construction_category::stable_interior;
    if (endpoint_relation == exact_relation_status::exact_zero)
        return construction_category::stable_endpoint;
    if (parameter.value.uncertainty_enclosure.lower() >= T(0) &&
        parameter.value.uncertainty_enclosure.upper() <= T(1))
        return construction_category::near_parallel_bounded;
    return construction_category::ill_conditioned;
}

template<class T>
boolean_outcome<construction_conditioning<T>> condition_edge_plane(
    const bounded_residual<T> &at_a,
    const bounded_residual<T> &at_b,
    exact_relation_evidence denominator_relation,
    exact_relation_evidence endpoint_relation,
    exact_relation_evidence coplanar_relation,
    bounded_residual<T> carrier_residual,
    bounded_residual<T> support_residual,
    T required_precision,
    T available_tolerance,
    construction_id id = construction_id(0)) {
    using namespace bounded_operations_detail;
    const auto &owner = at_a.owner;
    if (!owner_bound(owner) || !same_bound_owner(owner, at_b.owner) ||
        !same_bound_owner(owner, denominator_relation.owner) ||
        !same_bound_owner(owner, endpoint_relation.owner) ||
        !same_bound_owner(owner, coplanar_relation.owner) ||
        !same_bound_owner(owner, carrier_residual.owner) ||
        !same_bound_owner(owner, support_residual.owner) ||
        !same_bound_owner(owner, at_a.value.identity.owner) ||
        !same_bound_owner(owner, at_b.value.identity.owner) ||
        !same_bound_owner(owner, carrier_residual.value.identity.owner) ||
        !same_bound_owner(owner, support_residual.value.identity.owner) ||
        !bounded_scalar_valid(at_a.value) || !bounded_scalar_valid(at_b.value) ||
        !bounded_scalar_valid(carrier_residual.value) || !bounded_scalar_valid(support_residual.value) ||
        !finite_bits(at_a.scale) || !finite_bits(at_b.scale) ||
        !finite_bits(at_a.comparison_boundary) || !finite_bits(at_b.comparison_boundary) ||
        !finite_bits(carrier_residual.scale) || !finite_bits(support_residual.scale) ||
        !finite_bits(carrier_residual.comparison_boundary) ||
        !finite_bits(support_residual.comparison_boundary) ||
        !finite_bits(required_precision) || !finite_bits(available_tolerance) ||
        required_precision < T(0) || available_tolerance < T(0) ||
        endpoint_relation.status == exact_relation_status::invalid ||
        coplanar_relation.status == exact_relation_status::invalid ||
        carrier_residual.disposition != residual_disposition::pass ||
        support_residual.disposition != residual_disposition::pass)
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31301));
    auto denominator = bounded_subtract(at_a.value, at_b.value);
    if (!denominator.has_value())
        return boolean_outcome<construction_conditioning<T>>::failure(*denominator.error());
    const auto denominator_sign = classify_bounded_sign(denominator.value()->uncertainty_enclosure);
    if ((denominator_sign == bounded_sign_status::definitely_negative &&
         (denominator_relation.status == exact_relation_status::exact_positive ||
          denominator_relation.status == exact_relation_status::exact_zero)) ||
        (denominator_sign == bounded_sign_status::definitely_positive &&
         (denominator_relation.status == exact_relation_status::exact_negative ||
          denominator_relation.status == exact_relation_status::exact_zero)) ||
        denominator_relation.status == exact_relation_status::invalid)
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31302));
    if ((endpoint_relation.status == exact_relation_status::exact_zero &&
         !at_a.value.uncertainty_enclosure.contains_zero() &&
         !at_b.value.uncertainty_enclosure.contains_zero()) ||
        (coplanar_relation.status == exact_relation_status::exact_zero &&
         (!at_a.value.uncertainty_enclosure.contains_zero() ||
          !at_b.value.uncertainty_enclosure.contains_zero())))
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31302));
    construction_conditioning<T> out;
    out.owner = owner;
    out.id = id;
    out.rounded_graph_code = 1;
    out.exact_formula_code = denominator_relation.formula_code;
    out.denominator = denominator.value()->uncertainty_enclosure;
    out.denominator_relation = std::move(denominator_relation);
    out.carrier_residual = std::move(carrier_residual);
    out.support_residual = std::move(support_residual);
    out.required_precision = required_precision;
    out.available_tolerance = available_tolerance;
    out.parameter.owner = owner;
    if (coplanar_relation.status == exact_relation_status::exact_zero ||
        endpoint_relation.status == exact_relation_status::exact_zero) {
        const T endpoint = at_a.value.uncertainty_enclosure.contains_zero() ? T(0) : T(1);
        auto endpoint_value = checked_bounded_singleton(owner, endpoint);
        if (!endpoint_value.has_value())
            return boolean_outcome<construction_conditioning<T>>::failure(*endpoint_value.error());
        out.parameter.value = std::move(*endpoint_value.value());
        out.parameter.domain = endpoint_relation.status == exact_relation_status::exact_zero
            ? parameter_domain_status::stable_endpoint : parameter_domain_status::overlaps_boundary;
        out.cancellation_detected = out.denominator.contains_zero();
        out.category = coplanar_relation.status == exact_relation_status::exact_zero
            ? construction_category::coplanar_or_coincident
            : construction_category::exact_stored_coordinate_tie;
        if (!sum_contributors(at_a.contributors, at_b.contributors, out.contributors) ||
            !sum_contributors(out.contributors, out.carrier_residual.contributors, out.contributors) ||
            !sum_contributors(out.contributors, out.support_residual.contributors, out.contributors))
            return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31304));
        return boolean_outcome<construction_conditioning<T>>::success(std::move(out));
    }
    auto parameter_value = bounded_divide(at_a.value, *denominator.value());
    if (!parameter_value.has_value())
        return boolean_outcome<construction_conditioning<T>>::failure(*parameter_value.error());
    out.parameter.value = std::move(*parameter_value.value());
    const auto &i = out.parameter.value.uncertainty_enclosure;
    out.parameter.domain = i.lower() > T(0) && i.upper() < T(1)
        ? parameter_domain_status::stable_interior
        : (i.lower() >= T(0) && i.upper() <= T(1)
            ? (endpoint_relation.status == exact_relation_status::exact_zero
                ? parameter_domain_status::stable_endpoint
                : parameter_domain_status::overlaps_boundary)
            : parameter_domain_status::outside);
    out.parameter.domain_margin = i.lower() > T(0) && i.upper() < T(1)
        ? std::min(i.lower(), T(1) - i.upper()) : T(0);
    if (out.parameter.domain == parameter_domain_status::outside ||
        out.parameter.domain == parameter_domain_status::overlaps_boundary)
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31303));
    if (!sum_contributors(at_a.contributors, at_b.contributors, out.contributors) ||
        !sum_contributors(out.contributors, out.carrier_residual.contributors, out.contributors) ||
        !sum_contributors(out.contributors, out.support_residual.contributors, out.contributors))
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31304));
    const T denominator_distance = denominator_sign == bounded_sign_status::definitely_positive
        ? out.denominator.lower() : -out.denominator.upper();
    const auto amplification = directed_divide(T(1), denominator_distance);
    if (!amplification)
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31305));
    out.amplification_upper = amplification.value.upper;
    out.cancellation_detected = out.denominator.contains_zero();
    out.category = classify_construction(out.parameter, out.denominator,
                                         endpoint_relation.status, coplanar_relation.status,
                                         required_precision, available_tolerance);
    if (out.category == construction_category::invalid)
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31306));
    return boolean_outcome<construction_conditioning<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<construction_conditioning<T>> construct_edge_plane(
    const bounded_point3<T> &a, const bounded_point3<T> &b, const bounded_plane3<T> &plane,
    exact_relation_evidence denominator_relation, exact_relation_evidence endpoint_relation,
    exact_relation_evidence coplanar_relation, T required_precision, T available_tolerance,
    construction_id id = construction_id(0)) {
    auto at_a = bounded_plane_residual(plane, a, available_tolerance);
    auto at_b = bounded_plane_residual(plane, b, available_tolerance);
    if (!at_a.has_value()) return boolean_outcome<construction_conditioning<T>>::failure(*at_a.error());
    if (!at_b.has_value()) return boolean_outcome<construction_conditioning<T>>::failure(*at_b.error());
    auto carrier = *at_a.value(); auto support = *at_b.value();
    carrier.comparison_boundary = std::max(std::fabs(carrier.value.uncertainty_enclosure.lower()),
                                           std::fabs(carrier.value.uncertainty_enclosure.upper()));
    support.comparison_boundary = std::max(std::fabs(support.value.uncertainty_enclosure.lower()),
                                           std::fabs(support.value.uncertainty_enclosure.upper()));
    carrier.disposition = residual_disposition::pass; support.disposition = residual_disposition::pass;
    auto conditioned = condition_edge_plane(*at_a.value(), *at_b.value(),
        std::move(denominator_relation), std::move(endpoint_relation), std::move(coplanar_relation),
        std::move(carrier), std::move(support), required_precision, available_tolerance, id);
    if (!conditioned.has_value()) return conditioned;
    if (conditioned.value()->category == construction_category::coplanar_or_coincident)
        return conditioned;
    auto point = bounded_interpolate_from_a(a, b, conditioned.value()->parameter);
    if (!point.has_value()) return boolean_outcome<construction_conditioning<T>>::failure(*point.error());
    auto after = bounded_plane_residual(plane, *point.value(), available_tolerance);
    if (!after.has_value() || after.value()->disposition != residual_disposition::pass)
        return boolean_outcome<construction_conditioning<T>>::failure(
            bounded_operations_detail::arithmetic_error(31307));
    conditioned.value()->constructed_point = std::move(*point.value());
    conditioned.value()->residual_after = std::move(*after.value());
    conditioned.value()->carrier_residual = conditioned.value()->residual_after;
    conditioned.value()->support_residual = conditioned.value()->residual_after;
    conditioned.value()->has_constructed_point = true;
    return conditioned;
}

template<class T>
boolean_outcome<construction_conditioning<T>> condition_edge_plane(
    const bounded_point3<T> &a, const bounded_point3<T> &b, const bounded_plane3<T> &plane,
    exact_relation_evidence denominator_relation, exact_relation_evidence endpoint_relation,
    exact_relation_evidence coplanar_relation, T required_precision, T available_tolerance,
    construction_id id = construction_id(0)) {
    return construct_edge_plane(a, b, plane, std::move(denominator_relation),
        std::move(endpoint_relation), std::move(coplanar_relation), required_precision,
        available_tolerance, id);
}

template<class T>
boolean_outcome<construction_conditioning<T>> construct_edge_face(
    const bounded_point3<T> &a, const bounded_point3<T> &b,
    const bounded_point3<T> &face_a, const bounded_point3<T> &face_b,
    const bounded_point3<T> &face_c, exact_relation_evidence denominator_relation,
    exact_relation_evidence endpoint_relation, exact_relation_evidence coplanar_relation,
    T required_precision, T available_tolerance, construction_id id = construction_id(0)) {
    auto plane = bounded_plane_from_points(face_a, face_b, face_c);
    if (!plane.has_value()) return boolean_outcome<construction_conditioning<T>>::failure(*plane.error());
    auto result = construct_edge_plane(a, b, *plane.value(), std::move(denominator_relation),
        std::move(endpoint_relation), std::move(coplanar_relation), required_precision,
        available_tolerance, id);
    if (result.has_value()) result.value()->kind = construction_kind::edge_face;
    return result;
}

template<class T>
boolean_outcome<construction_conditioning<T>> condition_edge_face(
    const bounded_point3<T> &a, const bounded_point3<T> &b,
    const bounded_point3<T> &face_a, const bounded_point3<T> &face_b,
    const bounded_point3<T> &face_c, exact_relation_evidence denominator_relation,
    exact_relation_evidence endpoint_relation, exact_relation_evidence coplanar_relation,
    T required_precision, T available_tolerance, construction_id id = construction_id(0)) {
    return construct_edge_face(a, b, face_a, face_b, face_c, std::move(denominator_relation),
        std::move(endpoint_relation), std::move(coplanar_relation), required_precision,
        available_tolerance, id);
}

template<class T>
boolean_outcome<construction_conditioning<T>> condition_projection(
    const bounded_plane3<T> &plane, const bounded_point3<T> &point,
    T required_precision, T available_tolerance, construction_id id = construction_id(0)) {
    using namespace bounded_operations_detail;
    if (!same_bound_owner(plane.owner, point.owner) || !finite_bits(required_precision) ||
        !finite_bits(available_tolerance) || required_precision < T(0) || available_tolerance < T(0))
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31308));
    auto before = bounded_plane_residual(plane, point, available_tolerance);
    auto projected = bounded_project_onto_plane(plane, point);
    if (!before.has_value()) return boolean_outcome<construction_conditioning<T>>::failure(*before.error());
    if (!projected.has_value()) return boolean_outcome<construction_conditioning<T>>::failure(*projected.error());
    auto projection_parameter = bounded_divide(before.value()->value, plane.normal_sq);
    if (!projection_parameter.has_value())
        return boolean_outcome<construction_conditioning<T>>::failure(*projection_parameter.error());
    auto after = bounded_plane_residual(plane, *projected.value(), available_tolerance);
    if (!after.has_value() || after.value()->disposition != residual_disposition::pass)
        return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31309));
    construction_conditioning<T> out;
    out.owner = plane.owner; out.id = id; out.kind = construction_kind::projection;
    out.rounded_graph_code = 2; out.denominator = plane.normal_sq.uncertainty_enclosure;
    out.parameter.owner = plane.owner; out.parameter.value = std::move(*projection_parameter.value());
    out.parameter.carrier = parameter_carrier::face;
    out.parameter.domain = parameter_domain_status::stable_interior;
    out.denominator_relation.owner = plane.owner;
    out.denominator_relation.status = exact_relation_status::unavailable;
    out.carrier_residual = *before.value(); out.support_residual = *after.value();
    out.residual_after = *after.value(); out.constructed_point = std::move(*projected.value());
    out.has_constructed_point = true; out.required_precision = required_precision;
    out.available_tolerance = available_tolerance; out.category = required_precision <= available_tolerance
        ? construction_category::stable_interior : construction_category::ill_conditioned;
    out.contributors = out.constructed_point.coordinates.components[0].contributors;
    for (unsigned axis = 1; axis < 3; ++axis)
        if (!sum_contributors(out.contributors,
                out.constructed_point.coordinates.components[axis].contributors, out.contributors))
            return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31310));
    const auto amplification = directed_divide(T(1), out.denominator.lower());
    if (!amplification) return boolean_outcome<construction_conditioning<T>>::failure(arithmetic_error(31311));
    out.amplification_upper = amplification.value.upper;
    return boolean_outcome<construction_conditioning<T>>::success(std::move(out));
}

} // namespace ygor::mesh_boolean::bounded
