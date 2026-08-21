#pragma once

#include "BoundedOperations.h"
#include "PrecisionLedger.h"

#include <array>
#include <cmath>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct foreign_precision_provenance final {
    std::uint16_t schema_version = 1;
    context_owner_token owner{};
    double prior_output_precision = 0.0;
    double inherited_precision = 0.0;
    double construction_uncertainty = 0.0;
    double cumulative_cleanup_displacement = 0.0;
    double serialization_contribution = 0.0;
    std::vector<std::uint8_t> construction_history_digest;
    std::vector<std::uint8_t> prior_context_digest;
    std::vector<std::uint8_t> replay_lineage;
    std::vector<std::uint8_t> publication_digest;
    std::vector<std::uint8_t> verified_publication_digest;
    std::vector<std::uint8_t> verification_evidence;
    bool publication_verified = false;
};

template<class T>
boolean_outcome<uncertainty_contributors> verify_precision_import(
    const foreign_precision_provenance &record, T current_tolerance) {
    const double values[]{record.prior_output_precision, record.inherited_precision,
                          record.construction_uncertainty, record.cumulative_cleanup_displacement,
                          record.serialization_contribution};
    for (const auto value : values)
        if (!std::isfinite(value) || value < 0.0)
            return boolean_outcome<uncertainty_contributors>::failure(
                bounded_operations_detail::arithmetic_error(32001));
    if (!std::isfinite(current_tolerance) || current_tolerance < T(0) ||
        !bounded_operations_detail::owner_bound(record.owner) || !record.publication_verified ||
        record.construction_history_digest.empty() || record.prior_context_digest.empty() ||
        record.publication_digest.empty() || record.verification_evidence.empty() ||
        record.publication_digest != record.verified_publication_digest ||
        record.prior_output_precision > current_tolerance)
        return boolean_outcome<uncertainty_contributors>::failure(
            bounded_operations_detail::arithmetic_error(32002));
    double reconstructed = 0.0;
    for (const auto contribution : {record.inherited_precision, record.construction_uncertainty,
                                    record.cumulative_cleanup_displacement,
                                    record.serialization_contribution}) {
        const auto sum = directed_add(reconstructed, contribution);
        if (!sum || sum.value.upper > record.prior_output_precision)
            return boolean_outcome<uncertainty_contributors>::failure(
                bounded_operations_detail::arithmetic_error(32003));
        reconstructed = sum.value.upper;
    }
    uncertainty_contributors out;
    out.inherited_a = record.inherited_precision;
    out.construction = record.construction_uncertainty;
    out.prior_cleanup = record.cumulative_cleanup_displacement;
    out.conversion = record.serialization_contribution;
    const auto unexplained = directed_subtract(record.prior_output_precision, reconstructed);
    if (!unexplained || unexplained.value.lower < 0.0)
        return boolean_outcome<uncertainty_contributors>::failure(
            bounded_operations_detail::arithmetic_error(32004));
    out.inherited_b = unexplained.value.upper;
    return boolean_outcome<uncertainty_contributors>::success(out);
}

template<class T>
struct bounded_affine3 final {
    context_owner_token owner{};
    std::array<std::array<bounded_scalar<T>, 3>, 3> linear{};
    std::array<bounded_scalar<T>, 3> translation{};
};

template<class T>
boolean_outcome<bounded_point3<T>> apply_affine(const bounded_affine3<T> &transform,
                                                 const bounded_point3<T> &point) {
    if (!bounded_operations_detail::same_bound_owner(transform.owner, point.owner))
        return boolean_outcome<bounded_point3<T>>::failure(
            bounded_operations_detail::arithmetic_error(32005));
    bounded_point3<T> out;
    out.owner = point.owner;
    out.provenance = point.provenance;
    out.lineage = point.lineage;
    for (unsigned row = 0; row < 3; ++row) {
        auto p0 = bounded_multiply(transform.linear[row][0], point.coordinates.components[0]);
        auto p1 = bounded_multiply(transform.linear[row][1], point.coordinates.components[1]);
        auto p2 = bounded_multiply(transform.linear[row][2], point.coordinates.components[2]);
        if (!p0.has_value()) return boolean_outcome<bounded_point3<T>>::failure(*p0.error());
        if (!p1.has_value()) return boolean_outcome<bounded_point3<T>>::failure(*p1.error());
        if (!p2.has_value()) return boolean_outcome<bounded_point3<T>>::failure(*p2.error());
        auto sum01 = bounded_add(*p0.value(), *p1.value());
        if (!sum01.has_value()) return boolean_outcome<bounded_point3<T>>::failure(*sum01.error());
        auto sum012 = bounded_add(*sum01.value(), *p2.value());
        if (!sum012.has_value()) return boolean_outcome<bounded_point3<T>>::failure(*sum012.error());
        auto translated = bounded_add(*sum012.value(), transform.translation[row]);
        if (!translated.has_value()) return boolean_outcome<bounded_point3<T>>::failure(*translated.error());
        out.coordinates.components[row] = std::move(*translated.value());
    }
    out.coordinates.owner = point.owner;
    if (!bounded_operations_detail::compute_radial_error(out.coordinates))
        return boolean_outcome<bounded_point3<T>>::failure(
            bounded_operations_detail::arithmetic_error(32006));
    return boolean_outcome<bounded_point3<T>>::success(std::move(out));
}

template<class T>
boolean_outcome<bounded_point3<T>> translate_point(
    const bounded_point3<T> &point, const std::array<bounded_scalar<T>, 3> &translation) {
    bounded_point3<T> out = point;
    for (unsigned axis = 0; axis < 3; ++axis) {
        auto value = bounded_add(point.coordinates.components[axis], translation[axis]);
        if (!value.has_value()) return boolean_outcome<bounded_point3<T>>::failure(*value.error());
        out.coordinates.components[axis] = std::move(*value.value());
    }
    if (!bounded_operations_detail::compute_radial_error(out.coordinates))
        return boolean_outcome<bounded_point3<T>>::failure(
            bounded_operations_detail::arithmetic_error(32007));
    return boolean_outcome<bounded_point3<T>>::success(std::move(out));
}

} // namespace ygor::mesh_boolean::bounded
