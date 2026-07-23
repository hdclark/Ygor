#pragma once

#include "BoundedOperations.h"
#include "ContractVersions.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

enum class legacy_predicate_class : std::uint8_t {
    definitely_negative = 1, exact_tie_evidence = 2, definitely_positive = 3, uncertain = 4, invalid = 5
};

struct exact_relation_evidence final {
    std::uint16_t schema_version = 1;
    std::uint16_t formula_code = 0;
    context_owner_token owner{};
    exact_relation_id id{0};
    exact_relation_status status = exact_relation_status::unavailable;
    std::int32_t normalization_exponent = 0;
    std::uint32_t capacity_used = 0;
    std::vector<bounded_value_id> ordered_inputs;
};

template<class T>
struct predicate_result final {
    std::uint16_t schema_version = 1;
    context_owner_token owner{};
    bounded_scalar<T> rounded_and_bounded{};
    bounded_sign_status bounded_sign = bounded_sign_status::invalid;
    exact_relation_evidence exact_relation{};
    predicate_disposition disposition = predicate_disposition::fail_invalid;
    T separation_margin{};
    T uncertainty_width{};
    uncertainty_contributors contributors{};
    std::uint64_t trace_root = 0;
};

template<class T>
bounded_sign_status classify_bounded_sign(const finite_interval<T> &interval) noexcept {
    if (!finite_bits(interval.lower()) || !finite_bits(interval.upper()) ||
        finite_numeric_less(interval.upper(), interval.lower()))
        return bounded_sign_status::invalid;
    if (interval.upper() < T(0)) return bounded_sign_status::definitely_negative;
    if (interval.lower() > T(0)) return bounded_sign_status::definitely_positive;
    return bounded_sign_status::overlaps_boundary;
}

predicate_disposition assemble_predicate_disposition(
    bounded_sign_status bounded, exact_relation_status exact, bool alternate_available = false) noexcept;

legacy_predicate_class legacy_classification(
    bounded_sign_status bounded, exact_relation_status exact) noexcept;

template<class T>
boolean_outcome<predicate_result<T>> assemble_predicate_result(
    bounded_scalar<T> value, exact_relation_evidence exact, bool alternate_available = false) {
    if (!bounded_operations_detail::bounded_scalar_valid(value) ||
        !bounded_operations_detail::finite_contributors(value.contributors) ||
        !bounded_operations_detail::same_bound_owner(value.identity.owner, exact.owner))
        return boolean_outcome<predicate_result<T>>::failure(
            bounded_operations_detail::arithmetic_error(31201));
    predicate_result<T> out;
    out.owner = value.identity.owner;
    out.rounded_and_bounded = std::move(value);
    out.bounded_sign = classify_bounded_sign(out.rounded_and_bounded.uncertainty_enclosure);
    out.exact_relation = std::move(exact);
    out.disposition = assemble_predicate_disposition(out.bounded_sign,
                                                     out.exact_relation.status,
                                                     alternate_available);
    const auto &i = out.rounded_and_bounded.uncertainty_enclosure;
    const auto width = directed_subtract(i.upper(), i.lower());
    if (!width || !finite_bits(width.value.upper) ||
        out.disposition == predicate_disposition::fail_invalid)
        return boolean_outcome<predicate_result<T>>::failure(
            bounded_operations_detail::arithmetic_error(width ? 31203 : 31202));
    out.uncertainty_width = width.value.upper;
    if (out.bounded_sign == bounded_sign_status::definitely_negative) out.separation_margin = -i.upper();
    if (out.bounded_sign == bounded_sign_status::definitely_positive) out.separation_margin = i.lower();
    out.contributors = out.rounded_and_bounded.contributors;
    out.trace_root = out.rounded_and_bounded.identity.trace_root;
    return boolean_outcome<predicate_result<T>>::success(std::move(out));
}

namespace predicate_results_detail {
inline bool same_uncertainty_contributors(
    const uncertainty_contributors &a,
    const uncertainty_contributors &b) noexcept {
    return to_bits(a.inherited_a) == to_bits(b.inherited_a) &&
           to_bits(a.inherited_b) == to_bits(b.inherited_b) &&
           to_bits(a.machine_floor) == to_bits(b.machine_floor) &&
           to_bits(a.construction) == to_bits(b.construction) &&
           to_bits(a.conditioning) == to_bits(b.conditioning) &&
           to_bits(a.conversion) == to_bits(b.conversion) &&
           to_bits(a.prior_cleanup) == to_bits(b.prior_cleanup) &&
           to_bits(a.current_cleanup) == to_bits(b.current_cleanup);
}
} // namespace predicate_results_detail

template<class T>
bool valid_predicate_result(const predicate_result<T> &value) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    if (value.schema_version != contract_versions::predicate_truth_layers ||
        value.exact_relation.schema_version != contract_versions::predicate_truth_layers ||
        !bounded_operations_detail::bounded_scalar_valid(value.rounded_and_bounded) ||
        !bounded_operations_detail::finite_contributors(value.rounded_and_bounded.contributors) ||
        !bounded_operations_detail::same_bound_owner(value.owner,
                                                      value.rounded_and_bounded.identity.owner) ||
        !bounded_operations_detail::same_bound_owner(value.owner,
                                                      value.exact_relation.owner) ||
        value.exact_relation.status == exact_relation_status::invalid)
        return false;

    const auto classified =
        classify_bounded_sign(value.rounded_and_bounded.uncertainty_enclosure);
    if (classified == bounded_sign_status::invalid ||
        classified != value.bounded_sign)
        return false;

    const bool alternate_available =
        value.disposition == predicate_disposition::try_permitted_alternate;
    if (assemble_predicate_disposition(value.bounded_sign,
                                       value.exact_relation.status,
                                       alternate_available) != value.disposition)
        return false;

    const auto &interval = value.rounded_and_bounded.uncertainty_enclosure;
    const auto width = directed_subtract(interval.upper(), interval.lower());
    if (!width || to_bits(width.value.upper) != to_bits(value.uncertainty_width))
        return false;

    T expected_margin = T(0);
    if (value.bounded_sign == bounded_sign_status::definitely_negative)
        expected_margin = -interval.upper();
    if (value.bounded_sign == bounded_sign_status::definitely_positive)
        expected_margin = interval.lower();
    return to_bits(expected_margin) == to_bits(value.separation_margin) &&
           predicate_results_detail::same_uncertainty_contributors(
               value.contributors, value.rounded_and_bounded.contributors) &&
           value.trace_root == value.rounded_and_bounded.identity.trace_root;
}

} // namespace ygor::mesh_boolean::bounded
