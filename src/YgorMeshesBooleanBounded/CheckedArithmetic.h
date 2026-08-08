#pragma once

#include <cstddef>
#include <limits>
#include <type_traits>

namespace ygor::mesh_boolean::bounded {
template<class U>
constexpr bool checked_add(U a, U b, U &out) noexcept {
    static_assert(std::is_unsigned_v<U>);
    if (b > std::numeric_limits<U>::max() - a) return false;
    out = static_cast<U>(a + b);
    return true;
}
template<class U>
constexpr bool checked_multiply(U a, U b, U &out) noexcept {
    static_assert(std::is_unsigned_v<U>);
    if ((a != 0) && (b > std::numeric_limits<U>::max() / a)) return false;
    out = static_cast<U>(a * b);
    return true;
}
template<class To, class From>
constexpr bool checked_narrow(From value, To &out) noexcept {
    static_assert(std::is_integral_v<To> && std::is_integral_v<From>);
    if constexpr (std::is_signed_v<From> == std::is_signed_v<To>) {
        if (value < static_cast<From>(std::numeric_limits<To>::min()) ||
            value > static_cast<From>(std::numeric_limits<To>::max())) return false;
    } else if constexpr (std::is_signed_v<From>) {
        if (value < 0 || static_cast<std::make_unsigned_t<From>>(value) > std::numeric_limits<To>::max()) return false;
    } else if (value > static_cast<std::make_unsigned_t<To>>(std::numeric_limits<To>::max())) return false;
    out = static_cast<To>(value);
    return true;
}
}
