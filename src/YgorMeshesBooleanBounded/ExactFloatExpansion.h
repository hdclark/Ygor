#pragma once

#include "FloatingBits.h"
#include "PrecisionTypes.h"
#include "../YgorMeshesExactFloatExpansionCore.h"

#include <array>
#include <cstddef>

namespace ygor::mesh_boolean::bounded {

inline constexpr std::size_t exact_relation_default_capacity = 512;

namespace detail {
inline numeric_status numeric_status_from_core(exact_float_expansion_core::status value) noexcept {
    using core_status = exact_float_expansion_core::status;
    return value == core_status::success ? numeric_status::success
         : value == core_status::non_finite_input ? numeric_status::non_finite_input
         : value == core_status::non_finite_result ? numeric_status::non_finite_result
         : value == core_status::capacity_exceeded ? numeric_status::expansion_capacity_exceeded
         : value == core_status::exact_scaling_unavailable ? numeric_status::exact_scaling_unavailable
         : numeric_status::invalid_argument;
}

inline exact_relation_status exact_status_from_core(exact_float_expansion_core::sign value) noexcept {
    using core_sign = exact_float_expansion_core::sign;
    return value == core_sign::negative ? exact_relation_status::exact_negative
         : value == core_sign::zero ? exact_relation_status::exact_zero
         : value == core_sign::positive ? exact_relation_status::exact_positive
         : exact_relation_status::unavailable;
}

inline exact_relation_record relation_failure(exact_relation_formula_code formula,
                                              numeric_status status,
                                              std::size_t limit) noexcept {
    exact_relation_record record;
    record.formula = formula;
    record.status = status == numeric_status::non_finite_input
                  ? exact_relation_status::invalid : exact_relation_status::unavailable;
    record.evaluation_status = status;
    record.capacity_limit = limit;
    return record;
}
} // namespace detail

template<class T>
exact_relation_record exact_scalar_comparison(T left, T right) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    if (!finite_bits(left) || !finite_bits(right)) {
        return detail::relation_failure(exact_relation_formula_code::finite_scalar_comparison,
                                        numeric_status::non_finite_input, 1);
    }
    exact_relation_record record;
    record.formula = exact_relation_formula_code::finite_scalar_comparison;
    record.status = left < right ? exact_relation_status::exact_negative
                  : left > right ? exact_relation_status::exact_positive
                                 : exact_relation_status::exact_zero;
    record.evaluation_status = numeric_status::success;
    record.capacity_used = 1;
    record.capacity_limit = 1;
    return record;
}

template<class T>
exact_relation_record exact_sum_residual(T a, T b, T rounded) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    if (!finite_bits(a) || !finite_bits(b) || !finite_bits(rounded)) {
        return detail::relation_failure(exact_relation_formula_code::sum_residual,
                                        numeric_status::non_finite_input, 2);
    }
    T sum = T(0), residual = T(0);
    exact_float_expansion_core::two_sum(a, b, sum, residual);
    T difference = T(0), difference_residual = T(0);
    exact_float_expansion_core::two_diff(sum, rounded, difference, difference_residual);
    exact_float_expansion_core::expansion<T, 8> relation;
    relation.size = 1;
    relation.limbs[0] = difference;
    exact_float_expansion_core::expansion<T, 8> next;
    auto core_status = exact_float_expansion_core::grow(relation, difference_residual, next);
    if (core_status == exact_float_expansion_core::status::success) {
        core_status = exact_float_expansion_core::grow(next, residual, relation);
    }
    if (core_status != exact_float_expansion_core::status::success) {
        return detail::relation_failure(exact_relation_formula_code::sum_residual,
                                        detail::numeric_status_from_core(core_status), 8);
    }
    exact_relation_record record;
    record.formula = exact_relation_formula_code::sum_residual;
    record.status = detail::exact_status_from_core(exact_float_expansion_core::expansion_sign(relation));
    record.evaluation_status = numeric_status::success;
    record.capacity_used = relation.size;
    record.capacity_limit = 8;
    return record;
}

template<class T>
exact_relation_record exact_difference_residual(T a, T b, T rounded) noexcept {
    auto record = exact_sum_residual(a, -b, rounded);
    record.formula = exact_relation_formula_code::difference_residual;
    return record;
}

template<class T>
exact_relation_record exact_product_residual(T a, T b, T rounded) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    exact_float_expansion_core::sign core_sign = exact_float_expansion_core::sign::unavailable;
    std::size_t used = 0;
    const auto core_status = exact_float_expansion_core::product_difference_sign(
        rounded, a, b, core_sign, used);
    if (core_status != exact_float_expansion_core::status::success) {
        return detail::relation_failure(exact_relation_formula_code::product_residual,
                                        detail::numeric_status_from_core(core_status), 8);
    }
    // Convert rounded-a*b to a*b-rounded.
    if (core_sign == exact_float_expansion_core::sign::negative) {
        core_sign = exact_float_expansion_core::sign::positive;
    } else if (core_sign == exact_float_expansion_core::sign::positive) {
        core_sign = exact_float_expansion_core::sign::negative;
    }
    exact_relation_record record;
    record.formula = exact_relation_formula_code::product_residual;
    record.status = detail::exact_status_from_core(core_sign);
    record.evaluation_status = numeric_status::success;
    record.capacity_used = used;
    record.capacity_limit = 8;
    return record;
}

template<class T>
exact_relation_record exact_division_residual_numerator(T numerator, T quotient,
                                                       T denominator) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    exact_float_expansion_core::sign core_sign = exact_float_expansion_core::sign::unavailable;
    std::size_t used = 0;
    const auto core_status = exact_float_expansion_core::product_difference_sign(
        numerator, quotient, denominator, core_sign, used);
    if (core_status != exact_float_expansion_core::status::success) {
        return detail::relation_failure(
            exact_relation_formula_code::division_quotient_residual_numerator,
            detail::numeric_status_from_core(core_status), 8);
    }
    exact_relation_record record;
    record.formula = exact_relation_formula_code::division_quotient_residual_numerator;
    record.status = detail::exact_status_from_core(core_sign);
    record.evaluation_status = numeric_status::success;
    record.capacity_used = used;
    record.capacity_limit = 8;
    return record;
}

template<class T, std::size_t N>
exact_relation_record exact_determinant(const std::array<T, N * N> &matrix,
                                        exact_relation_formula_code formula) noexcept {
    static_assert(N == 2 || N == 3 || N == 4);
    exact_float_expansion_core::sign core_sign = exact_float_expansion_core::sign::unavailable;
    std::size_t used = 0;
    int exponent = 0;
    const auto core_status = exact_float_expansion_core::determinant_sign<T, N,
        exact_relation_default_capacity>(matrix, core_sign, used, exponent);
    if (core_status != exact_float_expansion_core::status::success) {
        return detail::relation_failure(formula, detail::numeric_status_from_core(core_status),
                                        exact_relation_default_capacity);
    }
    exact_relation_record record;
    record.formula = formula;
    record.status = detail::exact_status_from_core(core_sign);
    record.evaluation_status = numeric_status::success;
    record.normalization_exponent = exponent;
    record.capacity_used = used;
    record.capacity_limit = exact_relation_default_capacity;
    return record;
}

template<class T>
exact_relation_record exact_determinant_2x2(const std::array<T, 4> &matrix) noexcept {
    return exact_determinant<T, 2>(matrix, exact_relation_formula_code::determinant_2x2);
}

template<class T>
exact_relation_record exact_determinant_3x3(const std::array<T, 9> &matrix) noexcept {
    return exact_determinant<T, 3>(matrix, exact_relation_formula_code::determinant_3x3);
}

template<class T>
exact_relation_record exact_orient_2d(const std::array<T, 2> &a,
                                     const std::array<T, 2> &b,
                                     const std::array<T, 2> &c) noexcept {
    const std::array<T, 9> matrix{{a[0], a[1], T(1),
                                   b[0], b[1], T(1),
                                   c[0], c[1], T(1)}};
    return exact_determinant<T, 3>(matrix, exact_relation_formula_code::orient_2d);
}

template<class T>
exact_relation_record exact_orient_3d(const std::array<T, 3> &a,
                                     const std::array<T, 3> &b,
                                     const std::array<T, 3> &c,
                                     const std::array<T, 3> &d) noexcept {
    const std::array<T, 16> matrix{{a[0], a[1], a[2], T(1),
                                    b[0], b[1], b[2], T(1),
                                    c[0], c[1], c[2], T(1),
                                    d[0], d[1], d[2], T(1)}};
    return exact_determinant<T, 4>(matrix, exact_relation_formula_code::orient_3d);
}

} // namespace ygor::mesh_boolean::bounded
