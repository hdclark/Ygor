#pragma once

#include "FloatingBits.h"
#include "PrecisionTypes.h"
#include "../YgorMeshesExactFloatExpansionCore.h"

#include <cstdint>
#include <limits>
#include <type_traits>

namespace ygor::mesh_boolean::bounded {

template<class T>
struct directed_scalar_result final {
    T rounded = T(0);
    T lower = T(0);
    T upper = T(0);
    rounding_exactness exactness = rounding_exactness::invalid;
    residual_sign direction = residual_sign::unavailable;
    rounded_operation_code operation = rounded_operation_code::invalid;
    std::uint64_t residual_evidence = 0;
};

template<class T>
struct directed_operation_result final {
    numeric_status status = numeric_status::invalid_argument;
    directed_scalar_result<T> value{};

    constexpr explicit operator bool() const noexcept { return status == numeric_status::success; }
};

namespace detail {
template<class T>
directed_operation_result<T> directed_from_residual(T rounded, residual_sign direction,
                                                    rounded_operation_code operation,
                                                    std::uint64_t evidence = 0) noexcept {
    directed_operation_result<T> result;
    result.value.rounded = rounded;
    result.value.lower = rounded;
    result.value.upper = rounded;
    result.value.direction = direction;
    result.value.operation = operation;
    result.value.residual_evidence = evidence;
    if (!finite_bits(rounded)) {
        result.status = numeric_status::non_finite_result;
        return result;
    }
    if (direction == residual_sign::zero) {
        result.value.exactness = rounding_exactness::exact;
        result.status = numeric_status::success;
        return result;
    }
    if (direction == residual_sign::positive) {
        if (!next_up_finite(rounded, result.value.upper)) {
            result.status = numeric_status::outward_step_unavailable;
            return result;
        }
        result.value.exactness = rounding_exactness::inexact_direction_known;
        result.status = numeric_status::success;
        return result;
    }
    if (direction == residual_sign::negative) {
        if (!next_down_finite(rounded, result.value.lower)) {
            result.status = numeric_status::outward_step_unavailable;
            return result;
        }
        result.value.exactness = rounding_exactness::inexact_direction_known;
        result.status = numeric_status::success;
        return result;
    }
    if (!next_down_finite(rounded, result.value.lower) ||
        !next_up_finite(rounded, result.value.upper)) {
        result.status = numeric_status::outward_step_unavailable;
        return result;
    }
    result.value.exactness = rounding_exactness::direction_unavailable;
    result.status = numeric_status::success;
    return result;
}

inline residual_sign residual_from_core(exact_float_expansion_core::sign value) noexcept {
    using core_sign = exact_float_expansion_core::sign;
    return value == core_sign::negative ? residual_sign::negative
         : value == core_sign::zero ? residual_sign::zero
         : value == core_sign::positive ? residual_sign::positive
         : residual_sign::unavailable;
}
} // namespace detail

template<class T>
directed_operation_result<T> directed_add(T a, T b) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    if (!finite_bits(a) || !finite_bits(b)) {
        return {numeric_status::non_finite_input, {}};
    }
    T rounded = T(0), residual = T(0);
    exact_float_expansion_core::two_sum(a, b, rounded, residual);
    if (!finite_bits(rounded)) return {numeric_status::non_finite_result, {}};
    const auto direction = residual < T(0) ? residual_sign::negative
                         : residual > T(0) ? residual_sign::positive : residual_sign::zero;
    return detail::directed_from_residual(rounded, direction, rounded_operation_code::add);
}

template<class T>
directed_operation_result<T> directed_subtract(T a, T b) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    if (!finite_bits(a) || !finite_bits(b)) {
        return {numeric_status::non_finite_input, {}};
    }
    T rounded = T(0), residual = T(0);
    exact_float_expansion_core::two_diff(a, b, rounded, residual);
    if (!finite_bits(rounded)) return {numeric_status::non_finite_result, {}};
    const auto direction = residual < T(0) ? residual_sign::negative
                         : residual > T(0) ? residual_sign::positive : residual_sign::zero;
    return detail::directed_from_residual(rounded, direction, rounded_operation_code::subtract);
}

template<class T>
directed_operation_result<T> directed_multiply(T a, T b) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    if (!finite_bits(a) || !finite_bits(b)) {
        return {numeric_status::non_finite_input, {}};
    }
    const T rounded = static_cast<T>(a * b);
    if (!finite_bits(rounded)) return {numeric_status::non_finite_result, {}};
    exact_float_expansion_core::sign exact_sign = exact_float_expansion_core::sign::unavailable;
    std::size_t used = 0;
    const auto exact_status = exact_float_expansion_core::product_difference_sign(
        rounded, a, b, exact_sign, used);
    residual_sign direction = residual_sign::unavailable;
    if (exact_status == exact_float_expansion_core::status::success) {
        // product_difference_sign evaluated rounded - a*b.
        direction = detail::residual_from_core(exact_sign);
        if (direction == residual_sign::negative) direction = residual_sign::positive;
        else if (direction == residual_sign::positive) direction = residual_sign::negative;
    }
    return detail::directed_from_residual(rounded, direction, rounded_operation_code::multiply,
                                          static_cast<std::uint64_t>(used));
}

template<class T>
directed_operation_result<T> directed_divide(T numerator, T denominator) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    if (!finite_bits(numerator) || !finite_bits(denominator)) {
        return {numeric_status::non_finite_input, {}};
    }
    if (denominator == T(0)) return {numeric_status::division_by_zero, {}};
    const T rounded = static_cast<T>(numerator / denominator);
    if (!finite_bits(rounded)) return {numeric_status::non_finite_result, {}};
    exact_float_expansion_core::sign numerator_residual = exact_float_expansion_core::sign::unavailable;
    std::size_t used = 0;
    const auto exact_status = exact_float_expansion_core::product_difference_sign(
        numerator, rounded, denominator, numerator_residual, used);
    residual_sign direction = residual_sign::unavailable;
    if (exact_status == exact_float_expansion_core::status::success) {
        direction = detail::residual_from_core(numerator_residual);
        if (denominator < T(0)) {
            if (direction == residual_sign::negative) direction = residual_sign::positive;
            else if (direction == residual_sign::positive) direction = residual_sign::negative;
        }
    }
    return detail::directed_from_residual(rounded, direction, rounded_operation_code::divide,
                                          static_cast<std::uint64_t>(used));
}

template<class T, class I>
directed_operation_result<T> directed_integer_conversion(I value) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    static_assert(std::is_integral_v<I>);
    const T rounded = static_cast<T>(value);
    if (!finite_bits(rounded)) return {numeric_status::non_finite_result, {}};
    using U = std::make_unsigned_t<I>;
    const bool negative = std::is_signed_v<I> && value < I(0);
    const U encoded = static_cast<U>(value);
    const U magnitude = negative ? static_cast<U>(U(0) - encoded) : encoded;
    if (magnitude == U(0)) {
        return detail::directed_from_residual(rounded, residual_sign::zero,
                                              rounded_operation_code::checked_integer_conversion);
    }
    int bit_count = 0;
    for (U scan = magnitude; scan != U(0); scan >>= 1) ++bit_count;
    const int discarded_count = bit_count - std::numeric_limits<T>::digits;
    if (discarded_count <= 0) {
        return detail::directed_from_residual(rounded, residual_sign::zero,
                                              rounded_operation_code::checked_integer_conversion);
    }
    const U discarded_mask = static_cast<U>((U(1) << discarded_count) - U(1));
    const U discarded = static_cast<U>(magnitude & discarded_mask);
    if (discarded == U(0)) {
        return detail::directed_from_residual(rounded, residual_sign::zero,
                                              rounded_operation_code::checked_integer_conversion);
    }
    const U retained = static_cast<U>(magnitude >> discarded_count);
    const U halfway = static_cast<U>(U(1) << (discarded_count - 1));
    const bool rounded_magnitude_up = discarded > halfway ||
                                      (discarded == halfway && (retained & U(1)) != U(0));
    residual_sign direction = rounded_magnitude_up ? residual_sign::negative
                                                   : residual_sign::positive;
    if (negative) {
        direction = direction == residual_sign::negative ? residual_sign::positive
                                                         : residual_sign::negative;
    }
    return detail::directed_from_residual(
        rounded, direction,
        rounded_operation_code::checked_integer_conversion);
}

} // namespace ygor::mesh_boolean::bounded
