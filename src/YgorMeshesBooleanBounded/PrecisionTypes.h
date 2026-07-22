#pragma once

#include <cstddef>
#include <cstdint>

namespace ygor::mesh_boolean::bounded {

inline constexpr std::uint16_t precision_numeric_foundation_version = 1;
inline constexpr std::uint16_t floating_bits_provider_version = 1;
inline constexpr std::uint16_t directed_rounding_provider_version = 1;
inline constexpr std::uint16_t finite_interval_provider_version = 1;
inline constexpr std::uint16_t exact_expansion_provider_version = 1;
inline constexpr std::uint16_t rounded_operation_registry_version = 1;
inline constexpr std::uint16_t exact_formula_registry_version = 1;

enum class numeric_status : std::uint8_t {
    success = 1,
    non_finite_input = 2,
    non_finite_result = 3,
    outward_step_unavailable = 4,
    division_by_zero = 5,
    denominator_contains_zero = 6,
    invalid_interval = 7,
    empty_intersection = 8,
    exact_scaling_unavailable = 9,
    expansion_capacity_exceeded = 10,
    unsupported_formula = 11,
    invalid_argument = 12,
};

enum class rounding_exactness : std::uint8_t {
    exact = 1,
    inexact_direction_known = 2,
    direction_unavailable = 3,
    invalid = 4,
};

enum class residual_sign : std::uint8_t {
    negative = 1,
    zero = 2,
    positive = 3,
    unavailable = 4,
};

enum class rounded_operation_code : std::uint16_t {
    invalid = 0,
    source_import = 1,
    exact_scalar_constant = 2,
    checked_integer_conversion = 3,
    add = 4,
    subtract = 5,
    negate = 6,
    multiply = 7,
    divide = 8,
    interval_hull = 9,
    interval_intersection = 10,
    serialization_conversion = 11,
    prior_result_import = 12,
    multiply_add = 13,
    multiply_subtract = 14,
    vector_add = 15,
    vector_subtract = 16,
    vector_scale = 17,
    interpolate_from_a = 18,
    interpolate_from_b = 19,
    dot2 = 20,
    dot3 = 21,
    cross2 = 22,
    cross3 = 23,
    squared_norm = 24,
    coordinate_radial_derivation = 25,
    affine_map = 26,
    projection = 27,
    plane_construction = 28,
    plane_residual = 29,
    determinant_2x2 = 30,
    determinant_3x3 = 31,
    carrier_parameter = 32,
    aabb_build = 33,
    aabb_inflate = 34,
    aabb_union = 35,
    aabb_intersection = 36,
};

enum class exact_relation_formula_code : std::uint16_t {
    invalid = 0,
    finite_scalar_comparison = 1,
    sum_residual = 2,
    difference_residual = 3,
    product_residual = 4,
    division_quotient_residual_numerator = 5,
    determinant_2x2 = 6,
    determinant_3x3 = 7,
    orient_2d = 8,
    orient_3d = 9,
    plane_numerator = 10,
    endpoint_numerator = 11,
    collinearity_2d = 12,
    collinearity_3d = 13,
    coplanarity_3d = 14,
};

struct rounded_operation_descriptor final {
    rounded_operation_code code = rounded_operation_code::invalid;
    std::uint8_t minimum_arity = 0;
    std::uint8_t maximum_arity = 0;
};

constexpr rounded_operation_descriptor rounded_operation_descriptor_for(
    rounded_operation_code code) noexcept {
    switch (code) {
    case rounded_operation_code::source_import:
    case rounded_operation_code::exact_scalar_constant:
    case rounded_operation_code::prior_result_import: return {code, 0, 0};
    case rounded_operation_code::checked_integer_conversion:
    case rounded_operation_code::negate:
    case rounded_operation_code::serialization_conversion:
    case rounded_operation_code::squared_norm:
    case rounded_operation_code::aabb_build: return {code, 1, 1};
    case rounded_operation_code::add:
    case rounded_operation_code::subtract:
    case rounded_operation_code::multiply:
    case rounded_operation_code::divide:
    case rounded_operation_code::interval_hull:
    case rounded_operation_code::interval_intersection:
    case rounded_operation_code::vector_add:
    case rounded_operation_code::vector_subtract:
    case rounded_operation_code::vector_scale:
    case rounded_operation_code::dot2:
    case rounded_operation_code::dot3:
    case rounded_operation_code::cross2:
    case rounded_operation_code::cross3:
    case rounded_operation_code::affine_map:
    case rounded_operation_code::projection:
    case rounded_operation_code::plane_residual:
    case rounded_operation_code::carrier_parameter:
    case rounded_operation_code::aabb_inflate:
    case rounded_operation_code::aabb_union:
    case rounded_operation_code::aabb_intersection: return {code, 2, 2};
    case rounded_operation_code::multiply_add:
    case rounded_operation_code::multiply_subtract:
    case rounded_operation_code::interpolate_from_a:
    case rounded_operation_code::interpolate_from_b:
    case rounded_operation_code::plane_construction: return {code, 3, 3};
    case rounded_operation_code::coordinate_radial_derivation: return {code, 2, 3};
    case rounded_operation_code::determinant_2x2: return {code, 4, 4};
    case rounded_operation_code::determinant_3x3: return {code, 9, 9};
    default: return {};
    }
}

constexpr bool registered_rounded_operation(rounded_operation_code code) noexcept {
    return rounded_operation_descriptor_for(code).code != rounded_operation_code::invalid;
}

constexpr bool registered_exact_formula(exact_relation_formula_code code) noexcept {
    switch (code) {
    case exact_relation_formula_code::invalid: return false;
    case exact_relation_formula_code::finite_scalar_comparison:
    case exact_relation_formula_code::sum_residual:
    case exact_relation_formula_code::difference_residual:
    case exact_relation_formula_code::product_residual:
    case exact_relation_formula_code::division_quotient_residual_numerator:
    case exact_relation_formula_code::determinant_2x2:
    case exact_relation_formula_code::determinant_3x3:
    case exact_relation_formula_code::orient_2d:
    case exact_relation_formula_code::orient_3d:
    case exact_relation_formula_code::plane_numerator:
    case exact_relation_formula_code::endpoint_numerator:
    case exact_relation_formula_code::collinearity_2d:
    case exact_relation_formula_code::collinearity_3d:
    case exact_relation_formula_code::coplanarity_3d: return true;
    }
    return false;
}

constexpr bool valid_exact_formula_code(std::uint16_t code) noexcept {
    return code == 0 || registered_exact_formula(static_cast<exact_relation_formula_code>(code));
}

enum class exact_relation_status : std::uint8_t {
    exact_negative = 1,
    exact_zero = 2,
    exact_positive = 3,
    unavailable = 4,
    invalid = 5,
};

enum class bounded_sign_status : std::uint8_t {
    definitely_negative = 1,
    overlaps_boundary = 2,
    definitely_positive = 3,
    invalid = 4,
};

enum class predicate_disposition : std::uint8_t {
    accept_numeric_sign = 1,
    retain_tie_for_consumer_eligibility = 2,
    try_permitted_alternate = 3,
    route_coplanar_or_coincident = 4,
    fail_condition_or_tolerance = 5,
    fail_invalid = 6,
};

enum class interval_constraint_kind : std::uint8_t {
    exact_identity = 1,
    algebraic_nonnegative = 2,
    domain_constraint = 3,
    independent_geometric_bound = 4,
};

struct interval_intersection_proof final {
    std::uint64_t left_parent = 0;
    std::uint64_t right_parent = 0;
    interval_constraint_kind constraint = interval_constraint_kind::exact_identity;
    std::uint64_t guarantee_source = 0;
    std::uint64_t verifier_path = 0;
};

enum class interval_guarantee_source : std::uint64_t {
    squared_norm_algebra = 0x030001,
};

enum class interval_verifier_path : std::uint64_t {
    squared_norm_nonnegative_v1 = 0x030001,
};

struct exact_relation_record final {
    exact_relation_formula_code formula = exact_relation_formula_code::invalid;
    exact_relation_status status = exact_relation_status::invalid;
    numeric_status evaluation_status = numeric_status::invalid_argument;
    std::int32_t normalization_exponent = 0;
    std::size_t capacity_used = 0;
    std::size_t capacity_limit = 0;
};

constexpr exact_relation_status exact_status_from_residual(residual_sign sign) noexcept {
    return sign == residual_sign::negative ? exact_relation_status::exact_negative
         : sign == residual_sign::zero ? exact_relation_status::exact_zero
         : sign == residual_sign::positive ? exact_relation_status::exact_positive
         : exact_relation_status::unavailable;
}

} // namespace ygor::mesh_boolean::bounded
