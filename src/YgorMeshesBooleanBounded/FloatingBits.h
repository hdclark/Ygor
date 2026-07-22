#pragma once

#include "PrecisionTypes.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

namespace ygor::mesh_boolean::bounded {

template<class T>
struct floating_bits_traits;

template<>
struct floating_bits_traits<float> final {
    using uint_type = std::uint32_t;
    static constexpr uint_type sign_mask = UINT32_C(0x80000000);
    static constexpr uint_type exponent_mask = UINT32_C(0x7f800000);
    static constexpr uint_type fraction_mask = UINT32_C(0x007fffff);
    static constexpr int exponent_bias = 127;
};

template<>
struct floating_bits_traits<double> final {
    using uint_type = std::uint64_t;
    static constexpr uint_type sign_mask = UINT64_C(0x8000000000000000);
    static constexpr uint_type exponent_mask = UINT64_C(0x7ff0000000000000);
    static constexpr uint_type fraction_mask = UINT64_C(0x000fffffffffffff);
    static constexpr int exponent_bias = 1023;
};

template<class T>
inline constexpr bool supported_precision_scalar_v =
    (std::is_same_v<T, float> || std::is_same_v<T, double>) &&
    std::numeric_limits<T>::is_iec559 && std::numeric_limits<T>::radix == 2;

template<class T>
using floating_uint_t = typename floating_bits_traits<T>::uint_type;

template<class T>
floating_uint_t<T> to_bits(T value) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    floating_uint_t<T> result = 0;
    static_assert(sizeof(result) == sizeof(value));
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

template<class T>
T from_bits(floating_uint_t<T> bits) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    T result = T(0);
    std::memcpy(&result, &bits, sizeof(result));
    return result;
}

template<class T>
bool finite_bits(T value) noexcept {
    using traits = floating_bits_traits<T>;
    return (to_bits(value) & traits::exponent_mask) != traits::exponent_mask;
}

template<class T>
bool negative_zero(T value) noexcept {
    using traits = floating_bits_traits<T>;
    return to_bits(value) == traits::sign_mask;
}

template<class T>
floating_uint_t<T> finite_total_order_key(T value) noexcept {
    using traits = floating_bits_traits<T>;
    const auto bits = to_bits(value);
    return (bits & traits::sign_mask) != 0 ? static_cast<floating_uint_t<T>>(~bits)
                                           : static_cast<floating_uint_t<T>>(bits | traits::sign_mask);
}

template<class T>
bool finite_total_less(T a, T b) noexcept {
    return finite_total_order_key(a) < finite_total_order_key(b);
}

template<class T>
bool finite_numeric_less(T a, T b) noexcept {
    if (a == T(0) && b == T(0)) return false;
    return finite_total_less(a, b);
}

inline constexpr bool strict_floating_build_enabled() noexcept {
#if defined(YGOR_MESH_BOOLEAN_STRICT_FP_BUILD) && YGOR_MESH_BOOLEAN_STRICT_FP_BUILD == 1 && !defined(__FAST_MATH__)
    return true;
#else
    return false;
#endif
}

// Exercise properties that compiler flags and a caller-provided capability bit cannot prove.
template<class T>
bool runtime_floating_profile_qualified() noexcept {
    static_assert(supported_precision_scalar_v<T>);
    if (!strict_floating_build_enabled() || std::numeric_limits<T>::has_denorm != std::denorm_present) {
        return false;
    }

    using traits = floating_bits_traits<T>;
    volatile T negative_zero = from_bits<T>(traits::sign_mask);
    volatile T zero_sum = static_cast<T>(negative_zero + negative_zero);
    if (to_bits(static_cast<T>(zero_sum)) != traits::sign_mask) return false;

    volatile T denormal = std::numeric_limits<T>::denorm_min();
    volatile T doubled = static_cast<T>(denormal + denormal);
    if (to_bits(static_cast<T>(doubled)) != floating_uint_t<T>(2)) return false;

    const int half_digits = std::numeric_limits<T>::digits / 2 + 1;
    volatile T delta = std::scalbn(T(1), -half_digits);
    volatile T one_plus = static_cast<T>(T(1) + delta);
    volatile T one_minus = static_cast<T>(T(1) - delta);
    volatile T minus_one = T(-1);
    volatile T uncontracted = static_cast<T>(one_plus * one_minus + minus_one);
    return to_bits(static_cast<T>(uncontracted)) == floating_uint_t<T>(0);
}

template<class T>
bool next_up_finite(T value, T &result) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    using traits = floating_bits_traits<T>;
    using U = floating_uint_t<T>;
    if (!finite_bits(value) || value == std::numeric_limits<T>::max()) return false;
    U bits = to_bits(value);
    if ((bits & ~traits::sign_mask) == U(0)) {
        result = from_bits<T>(U(1));
    } else if ((bits & traits::sign_mask) != 0) {
        result = from_bits<T>(static_cast<U>(bits - U(1)));
    } else {
        result = from_bits<T>(static_cast<U>(bits + U(1)));
    }
    return true;
}

template<class T>
bool next_down_finite(T value, T &result) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    using traits = floating_bits_traits<T>;
    using U = floating_uint_t<T>;
    if (!finite_bits(value) || value == -std::numeric_limits<T>::max()) return false;
    U bits = to_bits(value);
    if ((bits & ~traits::sign_mask) == U(0)) {
        result = from_bits<T>(static_cast<U>(traits::sign_mask | U(1)));
    } else if ((bits & traits::sign_mask) != 0) {
        result = from_bits<T>(static_cast<U>(bits + U(1)));
    } else {
        result = from_bits<T>(static_cast<U>(bits - U(1)));
    }
    return true;
}

template<class T>
int unbiased_exponent(T value) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    using traits = floating_bits_traits<T>;
    const auto bits = to_bits(value);
    const auto exponent = static_cast<unsigned>((bits & traits::exponent_mask) >>
                                                 std::numeric_limits<T>::digits - 1);
    if (exponent != 0) return static_cast<int>(exponent) - traits::exponent_bias;
    const auto fraction = bits & traits::fraction_mask;
    if (fraction == 0) return std::numeric_limits<int>::min();
    int highest = 0;
    auto scan = fraction;
    while ((scan >>= 1) != 0) ++highest;
    return std::numeric_limits<T>::min_exponent - std::numeric_limits<T>::digits + highest;
}

template<class T>
numeric_status scale_power_of_two_exact(T value, int exponent, T &result) noexcept {
    static_assert(supported_precision_scalar_v<T>);
    if (!finite_bits(value)) return numeric_status::non_finite_input;
    if (value == T(0) || exponent == 0) {
        result = value;
        return numeric_status::success;
    }
    if (exponent == std::numeric_limits<int>::min()) {
        return numeric_status::exact_scaling_unavailable;
    }
    const T scaled = std::scalbn(value, exponent);
    if (!finite_bits(scaled) || scaled == T(0)) return numeric_status::exact_scaling_unavailable;
    const T restored = std::scalbn(scaled, -exponent);
    if (!finite_bits(restored) || to_bits(restored) != to_bits(value)) {
        return numeric_status::exact_scaling_unavailable;
    }
    result = scaled;
    return numeric_status::success;
}

} // namespace ygor::mesh_boolean::bounded
