#pragma once

#include "BoundedValues.h"
#include "CanonicalBytes.h"
#include "Outcome.h"
#include "PrecisionTrace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace ygor::mesh_boolean::bounded {

namespace bounded_operations_detail {
inline bool owner_bound(const context_owner_token &owner) noexcept {
    return static_cast<bool>(owner.anchor);
}

template<class T> bool finite_interval_valid(const finite_interval<T> &i) noexcept {
    return finite_bits(i.lower()) && finite_bits(i.upper()) && !finite_numeric_less(i.upper(), i.lower());
}

inline bounded_boolean_error arithmetic_error(std::uint32_t code) {
    bounded_boolean_error e;
    e.category = bounded_boolean_error_category::geometric_condition_exceeds_tolerance;
    e.stage = 4; e.checkpoint = 11; e.component = 3; e.subcode = code;
    e.summary = "bounded arithmetic operation failed";
    return e;
}

template<class T> bool bounded_scalar_valid(const bounded_scalar<T> &value) noexcept {
    return finite_bits(value.rounded_nominal) && finite_interval_valid(value.uncertainty_enclosure) &&
           value.uncertainty_enclosure.contains(value.rounded_nominal);
}

inline bool finite_contributors(const uncertainty_contributors &value) noexcept {
    const double fields[]{value.inherited_a, value.inherited_b, value.machine_floor,
                          value.construction, value.conditioning, value.conversion,
                          value.prior_cleanup, value.current_cleanup};
    for (const double field : fields)
        if (!std::isfinite(field) || field < 0.0) return false;
    return true;
}

inline bool same_bound_owner(const context_owner_token &a, const context_owner_token &b) noexcept {
    return owner_bound(a) && owner_bound(b) && a.same_owner(b);
}

inline bool add_contributor(double a, double b, double &out) noexcept {
    const auto sum = directed_add(a, b);
    if (!sum || !std::isfinite(sum.value.upper)) return false;
    out = sum.value.upper;
    return true;
}

inline bool sum_contributors(const uncertainty_contributors &a,
                             const uncertainty_contributors &b,
                             uncertainty_contributors &out) noexcept {
    if (!finite_contributors(a) || !finite_contributors(b)) return false;
    return add_contributor(a.inherited_a, b.inherited_a, out.inherited_a) &&
           add_contributor(a.inherited_b, b.inherited_b, out.inherited_b) &&
           add_contributor(a.machine_floor, b.machine_floor, out.machine_floor) &&
           add_contributor(a.construction, b.construction, out.construction) &&
           add_contributor(a.conditioning, b.conditioning, out.conditioning) &&
           add_contributor(a.conversion, b.conversion, out.conversion) &&
           add_contributor(a.prior_cleanup, b.prior_cleanup, out.prior_cleanup) &&
           add_contributor(a.current_cleanup, b.current_cleanup, out.current_cleanup);
}

inline bool scale_contributor(double value, double scale, double &out) noexcept {
    if (!std::isfinite(value) || value < 0.0 || !std::isfinite(scale) || scale < 0.0) return false;
    const auto product = directed_multiply(value, scale);
    if (!product || !std::isfinite(product.value.upper)) return false;
    out = product.value.upper;
    return true;
}

inline bool weighted_contributors(const uncertainty_contributors &a, double a_scale,
                                  const uncertainty_contributors &b, double b_scale,
                                  uncertainty_contributors &out) noexcept {
    if (!finite_contributors(a) || !finite_contributors(b)) return false;
    const auto combine = [&](double left, double right, double &result) {
        double scaled_left = 0.0, scaled_right = 0.0;
        return scale_contributor(left, a_scale, scaled_left) &&
               scale_contributor(right, b_scale, scaled_right) &&
               add_contributor(scaled_left, scaled_right, result);
    };
    return combine(a.inherited_a, b.inherited_a, out.inherited_a) &&
           combine(a.inherited_b, b.inherited_b, out.inherited_b) &&
           combine(a.machine_floor, b.machine_floor, out.machine_floor) &&
           combine(a.construction, b.construction, out.construction) &&
           combine(a.conditioning, b.conditioning, out.conditioning) &&
           combine(a.conversion, b.conversion, out.conversion) &&
           combine(a.prior_cleanup, b.prior_cleanup, out.prior_cleanup) &&
           combine(a.current_cleanup, b.current_cleanup, out.current_cleanup);
}

inline bool contributor_total(const uncertainty_contributors &value, double &out) noexcept {
    if (!finite_contributors(value)) return false;
    out = 0.0;
    const double fields[]{value.inherited_a, value.inherited_b, value.machine_floor,
                          value.construction, value.conditioning, value.conversion,
                          value.prior_cleanup, value.current_cleanup};
    for (const double field : fields)
        if (!add_contributor(out, field, out)) return false;
    return true;
}

template<class T>
bool enclosure_error_upper(const bounded_scalar<T> &value, double &out) noexcept {
    const auto below = directed_subtract(value.rounded_nominal, value.uncertainty_enclosure.lower());
    const auto above = directed_subtract(value.uncertainty_enclosure.upper(), value.rounded_nominal);
    if (!below || !above) return false;
    const T error = finite_numeric_less(below.value.upper, above.value.upper)
        ? above.value.upper : below.value.upper;
    out = static_cast<double>(error);
    return std::isfinite(out) && out >= 0.0;
}

inline bool cover_enclosure_error(uncertainty_contributors &contributors, double required,
                                  bool denominator_conditioned) noexcept {
    double total = 0.0;
    if (!contributor_total(contributors, total) || !std::isfinite(required) || required < 0.0)
        return false;
    if (total >= required) return true;
    const auto deficit = directed_subtract(required, total);
    if (!deficit || !std::isfinite(deficit.value.upper)) return false;
    double &target = denominator_conditioned ? contributors.conditioning : contributors.machine_floor;
    return add_contributor(target, deficit.value.upper, target);
}

template<class T> T interval_maximum_absolute(const finite_interval<T> &value) noexcept {
    const T lower = std::fabs(value.lower());
    const T upper = std::fabs(value.upper());
    return finite_numeric_less(lower, upper) ? upper : lower;
}

template<class T> T interval_minimum_absolute_nonzero(const finite_interval<T> &value) noexcept {
    return value.lower() > T(0) ? value.lower() : -value.upper();
}

template<class T> bool same_scalar_owner(const bounded_scalar<T> &a,
                                         const bounded_scalar<T> &b) noexcept {
    return same_bound_owner(a.identity.owner, b.identity.owner);
}

template<class T> bool compute_radial_error(bounded_vec3<T> &value) noexcept {
    T radial = T(0);
    for (const auto &component : value.components) {
        if (!bounded_scalar_valid(component) ||
            !same_bound_owner(value.owner, component.identity.owner)) return false;
        const auto below = directed_subtract(component.rounded_nominal,
                                             component.uncertainty_enclosure.lower());
        const auto above = directed_subtract(component.uncertainty_enclosure.upper(),
                                             component.rounded_nominal);
        if (!below || !above) return false;
        const T axis = finite_numeric_less(below.value.upper, above.value.upper)
                         ? above.value.upper : below.value.upper;
        const auto sum = directed_add(radial, axis);
        if (!sum) return false;
        radial = sum.value.upper;
    }
    value.radial_error_upper = radial;
    return finite_bits(radial);
}


template<class T> bool bounded_vec3_valid(const bounded_vec3<T> &value) noexcept {
    for (const auto &component : value.components)
        if (!bounded_scalar_valid(component) || !finite_contributors(component.contributors) ||
            !same_bound_owner(value.owner, component.identity.owner)) return false;
    return finite_bits(value.radial_error_upper) && value.radial_error_upper >= T(0);
}
} // namespace bounded_operations_detail

template<class T>
boolean_outcome<bounded_scalar<T>> checked_bounded_singleton(const context_owner_token &owner, T value) {
    using namespace bounded_operations_detail;
    if (!owner_bound(owner) || !finite_bits(value))
        return boolean_outcome<bounded_scalar<T>>::failure(arithmetic_error(31100));
    bounded_scalar<T> out;
    out.rounded_nominal = value;
    out.uncertainty_enclosure = *finite_interval<T>::checked_singleton(value);
    out.identity.owner = owner;
    return boolean_outcome<bounded_scalar<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_singleton(const context_owner_token &owner, T value) {
    return checked_bounded_singleton(owner, value);
}

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_add(const bounded_scalar<T> &a, const bounded_scalar<T> &b) {
    using namespace bounded_operations_detail;
    if (!bounded_scalar_valid(a) || !bounded_scalar_valid(b) || !same_scalar_owner(a, b))
        return boolean_outcome<bounded_scalar<T>>::failure(arithmetic_error(31101));
    const auto nominal = directed_add(a.rounded_nominal, b.rounded_nominal);
    const auto enclosure = interval_add(a.uncertainty_enclosure, b.uncertainty_enclosure);
    if (!nominal || !enclosure)
        return boolean_outcome<bounded_scalar<T>>::failure(arithmetic_error(31102));
    bounded_scalar<T> out;
    out.rounded_nominal = nominal.value.rounded;
    out.uncertainty_enclosure = *enclosure.value;
    out.identity.owner = a.identity.owner;
    if (!sum_contributors(a.contributors, b.contributors, out.contributors) ||
        !bounded_scalar_valid(out))
        return boolean_outcome<bounded_scalar<T>>::failure(arithmetic_error(31110));
    return boolean_outcome<bounded_scalar<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_negate(const bounded_scalar<T> &a) {
    using namespace bounded_operations_detail;
    if (!bounded_scalar_valid(a) || !finite_contributors(a.contributors))
        return boolean_outcome<bounded_scalar<T>>::failure(arithmetic_error(31103));
    bounded_scalar<T> out;
    out.rounded_nominal = -a.rounded_nominal;
    out.uncertainty_enclosure = *finite_interval<T>::create(-a.uncertainty_enclosure.upper(),
                                                            -a.uncertainty_enclosure.lower());
    out.identity.owner = a.identity.owner;
    out.contributors = a.contributors;
    if (!bounded_scalar_valid(out))
        return boolean_outcome<bounded_scalar<T>>::failure(arithmetic_error(31111));
    return boolean_outcome<bounded_scalar<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_subtract(const bounded_scalar<T> &a, const bounded_scalar<T> &b) {
    auto negated = bounded_negate(b);
    if (!negated.has_value()) return boolean_outcome<bounded_scalar<T>>::failure(*negated.error());
    return bounded_add(a, *negated.value());
}

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_multiply(const bounded_scalar<T> &a, const bounded_scalar<T> &b) {
    using namespace bounded_operations_detail;
    if (!bounded_scalar_valid(a) || !bounded_scalar_valid(b) || !same_scalar_owner(a, b))
        return boolean_outcome<bounded_scalar<T>>::failure(arithmetic_error(31104));
    const auto nominal = directed_multiply(a.rounded_nominal, b.rounded_nominal);
    const auto enclosure = interval_multiply(a.uncertainty_enclosure, b.uncertainty_enclosure);
    if (!nominal || !enclosure)
        return boolean_outcome<bounded_scalar<T>>::failure(arithmetic_error(31105));
    bounded_scalar<T> out;
    out.rounded_nominal = nominal.value.rounded;
    out.uncertainty_enclosure = *enclosure.value;
    out.identity.owner = a.identity.owner;
    const double a_scale = static_cast<double>(interval_maximum_absolute(b.uncertainty_enclosure));
    const double b_scale = static_cast<double>(interval_maximum_absolute(a.uncertainty_enclosure));
    double result_error = 0.0;
    if (!weighted_contributors(a.contributors, a_scale, b.contributors, b_scale,
                               out.contributors) ||
        !enclosure_error_upper(out, result_error) ||
        !cover_enclosure_error(out.contributors, result_error, false) ||
        !bounded_scalar_valid(out))
        return boolean_outcome<bounded_scalar<T>>::failure(arithmetic_error(31112));
    return boolean_outcome<bounded_scalar<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_divide(const bounded_scalar<T> &a, const bounded_scalar<T> &b) {
    using namespace bounded_operations_detail;
    if (!bounded_scalar_valid(a) || !bounded_scalar_valid(b) || !same_scalar_owner(a, b) ||
        b.uncertainty_enclosure.contains_zero())
        return boolean_outcome<bounded_scalar<T>>::failure(arithmetic_error(31106));
    const auto nominal = directed_divide(a.rounded_nominal, b.rounded_nominal);
    const auto enclosure = interval_divide(a.uncertainty_enclosure, b.uncertainty_enclosure);
    if (!nominal || !enclosure)
        return boolean_outcome<bounded_scalar<T>>::failure(arithmetic_error(31107));
    bounded_scalar<T> out;
    out.rounded_nominal = nominal.value.rounded;
    out.uncertainty_enclosure = *enclosure.value;
    out.identity.owner = a.identity.owner;
    const double denominator_min = static_cast<double>(
        interval_minimum_absolute_nonzero(b.uncertainty_enclosure));
    const double numerator_max = static_cast<double>(
        interval_maximum_absolute(a.uncertainty_enclosure));
    const auto numerator_scale = directed_divide(1.0, denominator_min);
    const auto denominator_ratio = directed_divide(numerator_max, denominator_min);
    const auto denominator_scale = denominator_ratio
        ? directed_divide(denominator_ratio.value.upper, denominator_min)
        : directed_operation_result<double>{numeric_status::non_finite_result, {}};
    double result_error = 0.0;
    if (!numerator_scale || !denominator_scale ||
        !weighted_contributors(a.contributors, numerator_scale.value.upper,
                               b.contributors, denominator_scale.value.upper,
                               out.contributors) ||
        !enclosure_error_upper(out, result_error) ||
        !cover_enclosure_error(out.contributors, result_error, true) ||
        !bounded_scalar_valid(out))
        return boolean_outcome<bounded_scalar<T>>::failure(arithmetic_error(31113));
    return boolean_outcome<bounded_scalar<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_dot3(const bounded_vec3<T> &a, const bounded_vec3<T> &b) {
    if (!bounded_operations_detail::bounded_vec3_valid(a) ||
        !bounded_operations_detail::bounded_vec3_valid(b) ||
        !bounded_operations_detail::same_bound_owner(a.owner, b.owner))
        return boolean_outcome<bounded_scalar<T>>::failure(
            bounded_operations_detail::arithmetic_error(31114));
    auto p0 = bounded_multiply(a.components[0], b.components[0]);
    auto p1 = bounded_multiply(a.components[1], b.components[1]);
    auto p2 = bounded_multiply(a.components[2], b.components[2]);
    if (!p0.has_value()) return boolean_outcome<bounded_scalar<T>>::failure(*p0.error());
    if (!p1.has_value()) return boolean_outcome<bounded_scalar<T>>::failure(*p1.error());
    if (!p2.has_value()) return boolean_outcome<bounded_scalar<T>>::failure(*p2.error());
    auto partial = bounded_add(*p0.value(), *p1.value());
    if (!partial.has_value()) return boolean_outcome<bounded_scalar<T>>::failure(*partial.error());
    return bounded_add(*partial.value(), *p2.value());
}

template<class T>
boolean_outcome<bounded_vec3<T>> bounded_cross3(const bounded_vec3<T> &a, const bounded_vec3<T> &b) {
    bounded_vec3<T> out;
    if (!bounded_operations_detail::bounded_vec3_valid(a) ||
        !bounded_operations_detail::bounded_vec3_valid(b) ||
        !bounded_operations_detail::same_bound_owner(a.owner, b.owner))
        return boolean_outcome<bounded_vec3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31114));
    out.owner = a.owner;
    constexpr std::array<std::array<unsigned, 4>, 3> indices{{{{1,2,2,1}},{{2,0,0,2}},{{0,1,1,0}}}};
    for (unsigned i = 0; i < 3; ++i) {
        auto lhs = bounded_multiply(a.components[indices[i][0]], b.components[indices[i][1]]);
        auto rhs = bounded_multiply(a.components[indices[i][2]], b.components[indices[i][3]]);
        if (!lhs.has_value()) return boolean_outcome<bounded_vec3<T>>::failure(*lhs.error());
        if (!rhs.has_value()) return boolean_outcome<bounded_vec3<T>>::failure(*rhs.error());
        auto component = bounded_subtract(*lhs.value(), *rhs.value());
        if (!component.has_value()) return boolean_outcome<bounded_vec3<T>>::failure(*component.error());
        out.components[i] = std::move(*component.value());
    }
    if (!bounded_operations_detail::compute_radial_error(out))
        return boolean_outcome<bounded_vec3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31115));
    return boolean_outcome<bounded_vec3<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<bounded_vec3<T>> bounded_vector_add(const bounded_vec3<T> &a,
                                                    const bounded_vec3<T> &b) {
    bounded_vec3<T> out;
    if (!bounded_operations_detail::bounded_vec3_valid(a) ||
        !bounded_operations_detail::bounded_vec3_valid(b) ||
        !bounded_operations_detail::same_bound_owner(a.owner, b.owner))
        return boolean_outcome<bounded_vec3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31116));
    out.owner = a.owner;
    for (unsigned axis = 0; axis < 3; ++axis) {
        auto component = bounded_add(a.components[axis], b.components[axis]);
        if (!component.has_value()) return boolean_outcome<bounded_vec3<T>>::failure(*component.error());
        out.components[axis] = std::move(*component.value());
    }
    if (!bounded_operations_detail::compute_radial_error(out))
        return boolean_outcome<bounded_vec3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31117));
    return boolean_outcome<bounded_vec3<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<bounded_vec3<T>> bounded_vector_subtract(const bounded_vec3<T> &a,
                                                         const bounded_vec3<T> &b) {
    bounded_vec3<T> out;
    if (!bounded_operations_detail::bounded_vec3_valid(a) ||
        !bounded_operations_detail::bounded_vec3_valid(b) ||
        !bounded_operations_detail::same_bound_owner(a.owner, b.owner))
        return boolean_outcome<bounded_vec3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31118));
    out.owner = a.owner;
    for (unsigned axis = 0; axis < 3; ++axis) {
        auto component = bounded_subtract(a.components[axis], b.components[axis]);
        if (!component.has_value()) return boolean_outcome<bounded_vec3<T>>::failure(*component.error());
        out.components[axis] = std::move(*component.value());
    }
    if (!bounded_operations_detail::compute_radial_error(out))
        return boolean_outcome<bounded_vec3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31119));
    return boolean_outcome<bounded_vec3<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<bounded_vec3<T>> bounded_vector_scale(const bounded_vec3<T> &value,
                                                      const bounded_scalar<T> &scale) {
    bounded_vec3<T> out;
    if (!bounded_operations_detail::bounded_vec3_valid(value) ||
        !bounded_operations_detail::same_bound_owner(value.owner, scale.identity.owner))
        return boolean_outcome<bounded_vec3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31120));
    out.owner = value.owner;
    for (unsigned axis = 0; axis < 3; ++axis) {
        auto component = bounded_multiply(value.components[axis], scale);
        if (!component.has_value()) return boolean_outcome<bounded_vec3<T>>::failure(*component.error());
        out.components[axis] = std::move(*component.value());
    }
    if (!bounded_operations_detail::compute_radial_error(out))
        return boolean_outcome<bounded_vec3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31121));
    return boolean_outcome<bounded_vec3<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<bounded_point3<T>> bounded_interpolate_from_a(const bounded_point3<T> &a,
                                                              const bounded_point3<T> &b,
                                                              const bounded_parameter<T> &parameter) {
    if (!bounded_operations_detail::same_bound_owner(a.owner, a.coordinates.owner) ||
        !bounded_operations_detail::same_bound_owner(b.owner, b.coordinates.owner) ||
        !bounded_operations_detail::same_bound_owner(parameter.owner, parameter.value.identity.owner) ||
        !bounded_operations_detail::same_bound_owner(a.owner, b.owner) ||
        !bounded_operations_detail::same_bound_owner(a.owner, parameter.owner))
        return boolean_outcome<bounded_point3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31122));
    auto delta = bounded_vector_subtract(b.coordinates, a.coordinates);
    if (!delta.has_value()) return boolean_outcome<bounded_point3<T>>::failure(*delta.error());
    auto scaled = bounded_vector_scale(*delta.value(), parameter.value);
    if (!scaled.has_value()) return boolean_outcome<bounded_point3<T>>::failure(*scaled.error());
    auto coordinates = bounded_vector_add(a.coordinates, *scaled.value());
    if (!coordinates.has_value()) return boolean_outcome<bounded_point3<T>>::failure(*coordinates.error());
    bounded_point3<T> out;
    out.owner = a.owner;
    out.coordinates = std::move(*coordinates.value());
    out.provenance = a.provenance;
    out.lineage = a.lineage;
    return boolean_outcome<bounded_point3<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_squared_norm(const bounded_vec3<T> &value) {
    auto out = bounded_dot3(value, value);
    if (!out.has_value()) return out;
    if (out.value()->uncertainty_enclosure.lower() < T(0)) {
        interval_intersection_proof proof{
            out.value()->identity.trace_root + 1,
            std::numeric_limits<std::uint64_t>::max(),
            interval_constraint_kind::algebraic_nonnegative,
            static_cast<std::uint64_t>(interval_guarantee_source::squared_norm_algebra),
            static_cast<std::uint64_t>(interval_verifier_path::squared_norm_nonnegative_v1)};
        const auto nonnegative = *finite_interval<T>::create(T(0), std::numeric_limits<T>::max());
        const auto evidence = verify_interval_intersection(
            out.value()->uncertainty_enclosure, nonnegative, proof);
        if (!evidence) return boolean_outcome<bounded_scalar<T>>::failure(
            bounded_operations_detail::arithmetic_error(31108));
        const auto narrowed = interval_intersection(
            out.value()->uncertainty_enclosure, nonnegative, *evidence);
        if (!narrowed) return boolean_outcome<bounded_scalar<T>>::failure(
            bounded_operations_detail::arithmetic_error(31108));
        out.value()->uncertainty_enclosure = *narrowed.value;
    }
    return out;
}

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_determinant2(const bounded_scalar<T> &a,
                                                        const bounded_scalar<T> &b,
                                                        const bounded_scalar<T> &c,
                                                        const bounded_scalar<T> &d) {
    auto ad = bounded_multiply(a, d); auto bc = bounded_multiply(b, c);
    if (!ad.has_value()) return boolean_outcome<bounded_scalar<T>>::failure(*ad.error());
    if (!bc.has_value()) return boolean_outcome<bounded_scalar<T>>::failure(*bc.error());
    return bounded_subtract(*ad.value(), *bc.value());
}

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_determinant3(
    const std::array<bounded_scalar<T>, 9> &m) {
    auto minor0 = bounded_determinant2(m[4], m[5], m[7], m[8]);
    auto minor1 = bounded_determinant2(m[3], m[5], m[6], m[8]);
    auto minor2 = bounded_determinant2(m[3], m[4], m[6], m[7]);
    if (!minor0.has_value()) return boolean_outcome<bounded_scalar<T>>::failure(*minor0.error());
    if (!minor1.has_value()) return boolean_outcome<bounded_scalar<T>>::failure(*minor1.error());
    if (!minor2.has_value()) return boolean_outcome<bounded_scalar<T>>::failure(*minor2.error());
    auto term0 = bounded_multiply(m[0], *minor0.value());
    auto term1 = bounded_multiply(m[1], *minor1.value());
    auto term2 = bounded_multiply(m[2], *minor2.value());
    if (!term0.has_value()) return boolean_outcome<bounded_scalar<T>>::failure(*term0.error());
    if (!term1.has_value()) return boolean_outcome<bounded_scalar<T>>::failure(*term1.error());
    if (!term2.has_value()) return boolean_outcome<bounded_scalar<T>>::failure(*term2.error());
    auto difference = bounded_subtract(*term0.value(), *term1.value());
    if (!difference.has_value()) return boolean_outcome<bounded_scalar<T>>::failure(*difference.error());
    return bounded_add(*difference.value(), *term2.value());
}

template<class T>
boolean_outcome<bounded_residual<T>> bounded_plane_residual(const bounded_plane3<T> &plane,
                                                            const bounded_point3<T> &point,
                                                            T comparison_boundary) {
    if (!bounded_operations_detail::same_bound_owner(plane.owner, point.owner) ||
        !bounded_operations_detail::same_bound_owner(plane.owner, plane.normal.owner) ||
        !bounded_operations_detail::same_bound_owner(point.owner, point.coordinates.owner) ||
        !finite_bits(comparison_boundary) || comparison_boundary < T(0))
        return boolean_outcome<bounded_residual<T>>::failure(
            bounded_operations_detail::arithmetic_error(31124));
    auto dot = bounded_dot3(plane.normal, point.coordinates);
    if (!dot.has_value()) return boolean_outcome<bounded_residual<T>>::failure(*dot.error());
    auto value = bounded_add(*dot.value(), plane.offset);
    if (!value.has_value()) return boolean_outcome<bounded_residual<T>>::failure(*value.error());
    bounded_residual<T> out;
    out.owner = plane.owner;
    out.value = std::move(*value.value());
    out.scale = plane.normal_sq.rounded_nominal;
    out.comparison_boundary = comparison_boundary;
    const auto lower = out.value.uncertainty_enclosure.lower();
    const auto upper = out.value.uncertainty_enclosure.upper();
    out.disposition = lower >= -comparison_boundary && upper <= comparison_boundary
        ? residual_disposition::pass
        : (upper < -comparison_boundary || lower > comparison_boundary
            ? residual_disposition::fail : residual_disposition::uncertain);
    out.contributors = out.value.contributors;
    return boolean_outcome<bounded_residual<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<bounded_plane3<T>> bounded_plane_from_points(const bounded_point3<T> &a,
                                                             const bounded_point3<T> &b,
                                                             const bounded_point3<T> &c) {
    if (!bounded_operations_detail::same_bound_owner(a.owner, a.coordinates.owner) ||
        !bounded_operations_detail::same_bound_owner(b.owner, b.coordinates.owner) ||
        !bounded_operations_detail::same_bound_owner(c.owner, c.coordinates.owner) ||
        !bounded_operations_detail::same_bound_owner(a.owner, b.owner) ||
        !bounded_operations_detail::same_bound_owner(a.owner, c.owner))
        return boolean_outcome<bounded_plane3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31123));
    auto u = bounded_vector_subtract(b.coordinates, a.coordinates);
    auto v = bounded_vector_subtract(c.coordinates, a.coordinates);
    if (!u.has_value()) return boolean_outcome<bounded_plane3<T>>::failure(*u.error());
    if (!v.has_value()) return boolean_outcome<bounded_plane3<T>>::failure(*v.error());
    auto normal = bounded_cross3(*u.value(), *v.value());
    if (!normal.has_value()) return boolean_outcome<bounded_plane3<T>>::failure(*normal.error());
    auto normal_sq = bounded_squared_norm(*normal.value());
    if (!normal_sq.has_value()) return boolean_outcome<bounded_plane3<T>>::failure(*normal_sq.error());
    if (normal_sq.value()->uncertainty_enclosure.lower() <= T(0))
        return boolean_outcome<bounded_plane3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31109));
    auto dot = bounded_dot3(*normal.value(), a.coordinates);
    if (!dot.has_value()) return boolean_outcome<bounded_plane3<T>>::failure(*dot.error());
    auto offset = bounded_negate(*dot.value());
    if (!offset.has_value()) return boolean_outcome<bounded_plane3<T>>::failure(*offset.error());
    bounded_plane3<T> out;
    out.owner = a.owner;
    out.normal = std::move(*normal.value());
    out.offset = std::move(*offset.value());
    out.anchor = a;
    out.normal_sq = std::move(*normal_sq.value());
    out.provenance = a.provenance;
    return boolean_outcome<bounded_plane3<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<bounded_point3<T>> bounded_project_onto_plane(const bounded_plane3<T> &plane,
                                                              const bounded_point3<T> &point) {
    if (!bounded_operations_detail::same_bound_owner(plane.owner, point.owner))
        return boolean_outcome<bounded_point3<T>>::failure(
            bounded_operations_detail::arithmetic_error(31125));
    auto residual = bounded_plane_residual(plane, point, T(0));
    if (!residual.has_value()) return boolean_outcome<bounded_point3<T>>::failure(*residual.error());
    auto parameter = bounded_divide(residual.value()->value, plane.normal_sq);
    if (!parameter.has_value()) return boolean_outcome<bounded_point3<T>>::failure(*parameter.error());
    auto displacement = bounded_vector_scale(plane.normal, *parameter.value());
    if (!displacement.has_value()) return boolean_outcome<bounded_point3<T>>::failure(*displacement.error());
    auto coordinates = bounded_vector_subtract(point.coordinates, *displacement.value());
    if (!coordinates.has_value()) return boolean_outcome<bounded_point3<T>>::failure(*coordinates.error());
    bounded_point3<T> out = point;
    out.coordinates = std::move(*coordinates.value());
    return boolean_outcome<bounded_point3<T>>::success(std::move(out));
}

namespace bounded_operations_detail {

template<class T>
boolean_outcome<bounded_scalar<T>> attach_scalar_trace(
    local_precision_trace &trace, rounded_operation_code operation,
    bounded_scalar<T> value, const std::vector<std::uint64_t> &parents,
    exact_relation_formula_code formula = exact_relation_formula_code::invalid) {
    if (!same_bound_owner(trace.owner(), value.identity.owner))
        return boolean_outcome<bounded_scalar<T>>::failure(arithmetic_error(31126));
    precision_trace_key key;
    key.operation_code = static_cast<std::uint16_t>(operation);
    key.exact_formula_code = formula == exact_relation_formula_code::invalid
        ? 0 : static_cast<std::uint16_t>(formula);
    key.parents = parents;
    if (value.identity.provenance.ordinal() != 0)
        key.provenance.push_back(value.identity.provenance.ordinal());
    canonical_writer result;
    result.floating(value.rounded_nominal);
    result.floating(value.uncertainty_enclosure.lower());
    result.floating(value.uncertainty_enclosure.upper());
    key.result_bytes = result.take();
    const auto node = trace.append(std::move(key), value.contributors);
    value.identity.trace_root = node.ordinal;
    value.identity.publication = bounded_publication_state::transaction_local;
    return boolean_outcome<bounded_scalar<T>>::success(std::move(value));
}

template<class T>
void inherit_binary_identity(bounded_scalar<T> &out, const bounded_scalar<T> &a,
                             const bounded_scalar<T> &b) noexcept {
    if (a.identity.provenance == b.identity.provenance) out.identity.provenance = a.identity.provenance;
    if (a.identity.lineage == b.identity.lineage) out.identity.lineage = a.identity.lineage;
}

} // namespace bounded_operations_detail

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_add(local_precision_trace &trace,
                                                const bounded_scalar<T> &a,
                                                const bounded_scalar<T> &b) {
    auto out = bounded_add(a, b);
    if (!out.has_value()) return out;
    bounded_operations_detail::inherit_binary_identity(*out.value(), a, b);
    return bounded_operations_detail::attach_scalar_trace(
        trace, rounded_operation_code::add, std::move(*out.value()),
        {a.identity.trace_root, b.identity.trace_root});
}

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_subtract(local_precision_trace &trace,
                                                     const bounded_scalar<T> &a,
                                                     const bounded_scalar<T> &b) {
    auto out = bounded_subtract(a, b);
    if (!out.has_value()) return out;
    bounded_operations_detail::inherit_binary_identity(*out.value(), a, b);
    return bounded_operations_detail::attach_scalar_trace(
        trace, rounded_operation_code::subtract, std::move(*out.value()),
        {a.identity.trace_root, b.identity.trace_root});
}

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_multiply(local_precision_trace &trace,
                                                     const bounded_scalar<T> &a,
                                                     const bounded_scalar<T> &b) {
    auto out = bounded_multiply(a, b);
    if (!out.has_value()) return out;
    bounded_operations_detail::inherit_binary_identity(*out.value(), a, b);
    return bounded_operations_detail::attach_scalar_trace(
        trace, rounded_operation_code::multiply, std::move(*out.value()),
        {a.identity.trace_root, b.identity.trace_root});
}

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_divide(local_precision_trace &trace,
                                                   const bounded_scalar<T> &a,
                                                   const bounded_scalar<T> &b) {
    auto out = bounded_divide(a, b);
    if (!out.has_value()) return out;
    bounded_operations_detail::inherit_binary_identity(*out.value(), a, b);
    return bounded_operations_detail::attach_scalar_trace(
        trace, rounded_operation_code::divide, std::move(*out.value()),
        {a.identity.trace_root, b.identity.trace_root},
        exact_relation_formula_code::division_quotient_residual_numerator);
}

template<class T>
boolean_outcome<bounded_scalar<T>> bounded_negate(local_precision_trace &trace,
                                                   const bounded_scalar<T> &a) {
    auto out = bounded_negate(a);
    if (!out.has_value()) return out;
    out.value()->identity.provenance = a.identity.provenance;
    out.value()->identity.lineage = a.identity.lineage;
    return bounded_operations_detail::attach_scalar_trace(
        trace, rounded_operation_code::negate, std::move(*out.value()),
        {a.identity.trace_root});
}

} // namespace ygor::mesh_boolean::bounded
