#pragma once

#include "DirectedRounding.h"
#include "FloatingBits.h"
#include "PrecisionTypes.h"

#include <algorithm>
#include <optional>

namespace ygor::mesh_boolean::bounded {

template<class T>
class finite_interval final {
  public:
    finite_interval() noexcept
        : lower_(from_bits<T>(floating_bits_traits<T>::sign_mask)), upper_(T(0)) {}

    static std::optional<finite_interval> create(T lower, T upper) noexcept {
        if (!finite_bits(lower) || !finite_bits(upper) || finite_numeric_less(upper, lower)) {
            return std::nullopt;
        }
        if (lower == T(0)) lower = from_bits<T>(floating_bits_traits<T>::sign_mask);
        if (upper == T(0)) upper = T(0);
        return finite_interval(lower, upper);
    }

    // Untrusted callers should use checked_singleton(). The direct API remains
    // conservative for compatibility and maps nonfinite input to the full finite range.
    static finite_interval singleton(T value) noexcept {
        if (!finite_bits(value)) {
            return finite_interval(-std::numeric_limits<T>::max(),
                                   std::numeric_limits<T>::max());
        }
        if (value == T(0)) {
            return finite_interval(from_bits<T>(floating_bits_traits<T>::sign_mask), T(0));
        }
        return finite_interval(value, value);
    }

    static std::optional<finite_interval> checked_singleton(T value) noexcept {
        return finite_bits(value) ? std::optional<finite_interval>(singleton(value)) : std::nullopt;
    }

    T lower() const noexcept { return lower_; }
    T upper() const noexcept { return upper_; }
    bool contains_zero() const noexcept { return !finite_numeric_less(T(0), lower_) &&
                                                !finite_numeric_less(upper_, T(0)); }
    bool contains(T value) const noexcept {
        return finite_bits(value) && !finite_numeric_less(value, lower_) &&
               !finite_numeric_less(upper_, value);
    }
    bool is_singleton() const noexcept {
        return lower_ == upper_ && !(lower_ == T(0) && to_bits(lower_) != to_bits(upper_));
    }

  private:
    finite_interval(T lower, T upper) noexcept : lower_(lower), upper_(upper) {}
    T lower_;
    T upper_;
};

template<class T>
struct interval_operation_result final {
    numeric_status status = numeric_status::invalid_argument;
    std::optional<finite_interval<T>> value;

    explicit operator bool() const noexcept { return status == numeric_status::success && value.has_value(); }
};

template<class T>
finite_interval<T> interval_hull(const finite_interval<T> &left,
                                 const finite_interval<T> &right) noexcept {
    const T lower = finite_numeric_less(right.lower(), left.lower()) ? right.lower() : left.lower();
    const T upper = finite_numeric_less(left.upper(), right.upper()) ? right.upper() : left.upper();
    const auto result = finite_interval<T>::create(lower, upper);
    return result ? *result
                  : finite_interval<T>::singleton(std::numeric_limits<T>::infinity());
}

template<class T>
class interval_intersection_evidence final {
  public:
    interval_intersection_evidence(const interval_intersection_evidence &) = default;
    interval_intersection_evidence &operator=(const interval_intersection_evidence &) = default;

  private:
    interval_intersection_evidence(const finite_interval<T> &left,
                                   const finite_interval<T> &right,
                                   const interval_intersection_proof &proof) noexcept
        : left_lower_(to_bits(left.lower())), left_upper_(to_bits(left.upper())),
          right_lower_(to_bits(right.lower())), right_upper_(to_bits(right.upper())),
          left_parent_(proof.left_parent), right_parent_(proof.right_parent),
          constraint_(proof.constraint), guarantee_source_(proof.guarantee_source),
          verifier_path_(proof.verifier_path) {}

    floating_uint_t<T> left_lower_ = 0;
    floating_uint_t<T> left_upper_ = 0;
    floating_uint_t<T> right_lower_ = 0;
    floating_uint_t<T> right_upper_ = 0;
    std::uint64_t left_parent_ = 0;
    std::uint64_t right_parent_ = 0;
    interval_constraint_kind constraint_ = interval_constraint_kind::exact_identity;
    std::uint64_t guarantee_source_ = 0;
    std::uint64_t verifier_path_ = 0;

    template<class U>
    friend std::optional<interval_intersection_evidence<U>> verify_interval_intersection(
        const finite_interval<U> &, const finite_interval<U> &,
        const interval_intersection_proof &) noexcept;
    template<class U>
    friend interval_operation_result<U> interval_intersection(
        const finite_interval<U> &, const finite_interval<U> &,
        const interval_intersection_evidence<U> &) noexcept;
};

template<class T>
std::optional<interval_intersection_evidence<T>> verify_interval_intersection(
    const finite_interval<T> &left, const finite_interval<T> &right,
    const interval_intersection_proof &proof) noexcept {
    if (proof.left_parent == 0 || proof.right_parent == 0 ||
        proof.guarantee_source == 0 || proof.verifier_path == 0) return std::nullopt;

    const T lower = finite_numeric_less(left.lower(), right.lower()) ? right.lower() : left.lower();
    const T upper = finite_numeric_less(left.upper(), right.upper()) ? left.upper() : right.upper();
    if (!finite_interval<T>::create(lower, upper)) return std::nullopt;

    bool constraint_valid = false;
    switch (proof.constraint) {
        case interval_constraint_kind::exact_identity:
            constraint_valid = to_bits(left.lower()) == to_bits(right.lower()) &&
                               to_bits(left.upper()) == to_bits(right.upper());
            break;
        case interval_constraint_kind::algebraic_nonnegative: {
            const auto is_nonnegative_constraint = [](const finite_interval<T> &value) {
                return value.lower() == T(0) &&
                       value.upper() == std::numeric_limits<T>::max();
            };
            constraint_valid = is_nonnegative_constraint(left) || is_nonnegative_constraint(right);
            break;
        }
        case interval_constraint_kind::domain_constraint:
            // Bare intervals carry no domain provenance from which to verify this claim.
            constraint_valid = false;
            break;
        case interval_constraint_kind::independent_geometric_bound:
            constraint_valid = proof.left_parent != proof.right_parent;
            break;
        default:
            break;
    }
    if (!constraint_valid) return std::nullopt;
    return interval_intersection_evidence<T>(left, right, proof);
}

template<class T>
interval_operation_result<T> interval_intersection(
    const finite_interval<T> &left, const finite_interval<T> &right,
    const interval_intersection_evidence<T> &evidence) noexcept {
    if (evidence.left_lower_ != to_bits(left.lower()) ||
        evidence.left_upper_ != to_bits(left.upper()) ||
        evidence.right_lower_ != to_bits(right.lower()) ||
        evidence.right_upper_ != to_bits(right.upper()) ||
        evidence.left_parent_ == 0 || evidence.right_parent_ == 0 ||
        evidence.guarantee_source_ == 0 || evidence.verifier_path_ == 0) {
        return {numeric_status::invalid_argument, std::nullopt};
    }
    const T lower = finite_numeric_less(left.lower(), right.lower()) ? right.lower() : left.lower();
    const T upper = finite_numeric_less(left.upper(), right.upper()) ? left.upper() : right.upper();
    auto value = finite_interval<T>::create(lower, upper);
    if (!value) return {numeric_status::empty_intersection, std::nullopt};
    return {numeric_status::success, std::move(value)};
}

template<class T>
interval_operation_result<T> interval_intersection(
    const finite_interval<T> &, const finite_interval<T> &,
    const interval_intersection_proof &) noexcept {
    // Public aggregate proofs are forgeable. Retain the source-compatible overload,
    // but require callers to obtain opaque evidence through the verifier.
    return {numeric_status::invalid_argument, std::nullopt};
}

template<class T>
directed_operation_result<T> interval_width(const finite_interval<T> &value) noexcept {
    return directed_subtract(value.upper(), value.lower());
}

template<class T>
interval_operation_result<T> interval_add(const finite_interval<T> &left,
                                          const finite_interval<T> &right) noexcept {
    const auto lower = directed_add(left.lower(), right.lower());
    if (!lower) return {lower.status, std::nullopt};
    const auto upper = directed_add(left.upper(), right.upper());
    if (!upper) return {upper.status, std::nullopt};
    auto value = finite_interval<T>::create(lower.value.lower, upper.value.upper);
    return value ? interval_operation_result<T>{numeric_status::success, std::move(value)}
                 : interval_operation_result<T>{numeric_status::invalid_interval, std::nullopt};
}

template<class T>
interval_operation_result<T> interval_subtract(const finite_interval<T> &left,
                                               const finite_interval<T> &right) noexcept {
    const auto lower = directed_subtract(left.lower(), right.upper());
    if (!lower) return {lower.status, std::nullopt};
    const auto upper = directed_subtract(left.upper(), right.lower());
    if (!upper) return {upper.status, std::nullopt};
    auto value = finite_interval<T>::create(lower.value.lower, upper.value.upper);
    return value ? interval_operation_result<T>{numeric_status::success, std::move(value)}
                 : interval_operation_result<T>{numeric_status::invalid_interval, std::nullopt};
}

template<class T>
interval_operation_result<T> interval_multiply(const finite_interval<T> &left,
                                               const finite_interval<T> &right) noexcept {
    const T a[2] = {left.lower(), left.upper()};
    const T b[2] = {right.lower(), right.upper()};
    bool initialized = false;
    T lower = T(0), upper = T(0);
    for (T x : a) {
        for (T y : b) {
            const auto product = directed_multiply(x, y);
            if (!product) return {product.status, std::nullopt};
            if (!initialized || finite_numeric_less(product.value.lower, lower)) lower = product.value.lower;
            if (!initialized || finite_numeric_less(upper, product.value.upper)) upper = product.value.upper;
            initialized = true;
        }
    }
    auto value = finite_interval<T>::create(lower, upper);
    return value ? interval_operation_result<T>{numeric_status::success, std::move(value)}
                 : interval_operation_result<T>{numeric_status::invalid_interval, std::nullopt};
}

template<class T>
interval_operation_result<T> interval_divide(const finite_interval<T> &numerator,
                                             const finite_interval<T> &denominator) noexcept {
    if (denominator.contains_zero()) {
        return {numeric_status::denominator_contains_zero, std::nullopt};
    }
    const T a[2] = {numerator.lower(), numerator.upper()};
    const T b[2] = {denominator.lower(), denominator.upper()};
    bool initialized = false;
    T lower = T(0), upper = T(0);
    for (T x : a) {
        for (T y : b) {
            const auto quotient = directed_divide(x, y);
            if (!quotient) return {quotient.status, std::nullopt};
            if (!initialized || finite_numeric_less(quotient.value.lower, lower)) lower = quotient.value.lower;
            if (!initialized || finite_numeric_less(upper, quotient.value.upper)) upper = quotient.value.upper;
            initialized = true;
        }
    }
    auto value = finite_interval<T>::create(lower, upper);
    return value ? interval_operation_result<T>{numeric_status::success, std::move(value)}
                 : interval_operation_result<T>{numeric_status::invalid_interval, std::nullopt};
}

} // namespace ygor::mesh_boolean::bounded
