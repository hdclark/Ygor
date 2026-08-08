#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

#ifdef __FAST_MATH__
#error "Exact float expansion core requires strict floating-point semantics"
#endif

namespace ygor::exact_float_expansion_core {

inline constexpr std::uint16_t provider_version = 1;

enum class status : std::uint8_t {
    success = 1,
    non_finite_input = 2,
    non_finite_result = 3,
    capacity_exceeded = 4,
    exact_scaling_unavailable = 5,
    invalid_argument = 6,
};

enum class sign : std::int8_t { negative = -1, zero = 0, positive = 1, unavailable = 2 };

template<class T>
inline constexpr bool supported_scalar_v =
    (std::is_same_v<T, float> || std::is_same_v<T, double>) &&
    std::numeric_limits<T>::is_iec559 && std::numeric_limits<T>::radix == 2;

template<class T>
constexpr T splitter() noexcept {
    static_assert(supported_scalar_v<T>);
    return std::is_same_v<T, float> ? T(4097) : T(134217729);
}

template<class T>
bool finite(T value) noexcept {
    return std::isfinite(value);
}

template<class T>
void two_sum(T a, T b, T &rounded, T &residual) noexcept {
    static_assert(supported_scalar_v<T>);
    rounded = static_cast<T>(a + b);
    const T b_virtual = static_cast<T>(rounded - a);
    const T a_virtual = static_cast<T>(rounded - b_virtual);
    const T b_roundoff = static_cast<T>(b - b_virtual);
    const T a_roundoff = static_cast<T>(a - a_virtual);
    residual = static_cast<T>(a_roundoff + b_roundoff);
}

template<class T>
void two_diff(T a, T b, T &rounded, T &residual) noexcept {
    static_assert(supported_scalar_v<T>);
    rounded = static_cast<T>(a - b);
    const T b_virtual = static_cast<T>(a - rounded);
    const T a_virtual = static_cast<T>(rounded + b_virtual);
    const T b_roundoff = static_cast<T>(b_virtual - b);
    const T a_roundoff = static_cast<T>(a - a_virtual);
    residual = static_cast<T>(a_roundoff + b_roundoff);
}

template<class T>
void split(T value, T &high, T &low) noexcept {
    const T product = static_cast<T>(splitter<T>() * value);
    const T large = static_cast<T>(product - value);
    high = static_cast<T>(product - large);
    low = static_cast<T>(value - high);
}

template<class T>
status two_product(T a, T b, T &rounded, T &residual) noexcept {
    static_assert(supported_scalar_v<T>);
    if (!finite(a) || !finite(b)) return status::non_finite_input;
    rounded = static_cast<T>(a * b);
    if (!finite(rounded)) return status::non_finite_result;
    if (a == T(0) || b == T(0)) {
        residual = std::copysign(T(0), rounded);
        return status::success;
    }

    // Keep splitter multiplication away from overflow. Scaling is exact and the
    // residual is scaled back only when it remains representable as a T limb.
    int a_exponent = 0;
    int b_exponent = 0;
    (void)std::frexp(a, &a_exponent);
    (void)std::frexp(b, &b_exponent);
    const int target = -2;
    const int a_shift = target - a_exponent;
    const int b_shift = target - b_exponent;
    const T scaled_a = std::scalbn(a, a_shift);
    const T scaled_b = std::scalbn(b, b_shift);
    if (!finite(scaled_a) || !finite(scaled_b) || scaled_a == T(0) || scaled_b == T(0)) {
        return status::exact_scaling_unavailable;
    }
    const T scaled_product = static_cast<T>(scaled_a * scaled_b);
    T a_high = T(0), a_low = T(0), b_high = T(0), b_low = T(0);
    split(scaled_a, a_high, a_low);
    split(scaled_b, b_high, b_low);
    const T error1 = static_cast<T>(scaled_product - static_cast<T>(a_high * b_high));
    const T error2 = static_cast<T>(error1 - static_cast<T>(a_low * b_high));
    const T error3 = static_cast<T>(error2 - static_cast<T>(a_high * b_low));
    const T scaled_residual = static_cast<T>(static_cast<T>(a_low * b_low) - error3);
    const int inverse_shift = -(a_shift + b_shift);
    const T restored_product = std::scalbn(scaled_product, inverse_shift);
    if (!finite(restored_product) || restored_product != rounded) {
        return status::exact_scaling_unavailable;
    }
    residual = std::scalbn(scaled_residual, inverse_shift);
    if (!finite(residual)) return status::non_finite_result;
    if (scaled_residual != T(0) && residual == T(0)) return status::exact_scaling_unavailable;
    if (std::scalbn(residual, -inverse_shift) != scaled_residual) {
        return status::exact_scaling_unavailable;
    }
    return status::success;
}

template<class T, std::size_t Capacity>
struct expansion final {
    std::array<T, Capacity> limbs{};
    std::size_t size = 1;

    static constexpr std::size_t capacity() noexcept { return Capacity; }
};

template<class T, std::size_t Capacity>
status grow(const expansion<T, Capacity> &input, T value,
            expansion<T, Capacity> &output) noexcept {
    if (!finite(value) || input.size == 0 || input.size > Capacity) return status::invalid_argument;
    if (input.size == Capacity) return status::capacity_exceeded;
    T accumulator = value;
    std::size_t used = 0;
    for (std::size_t i = 0; i < input.size; ++i) {
        T next = T(0), residual = T(0);
        two_sum(accumulator, input.limbs[i], next, residual);
        if (residual != T(0)) output.limbs[used++] = residual;
        accumulator = next;
    }
    if (accumulator != T(0) || used == 0) output.limbs[used++] = accumulator;
    output.size = used;
    return status::success;
}

template<class T, std::size_t Capacity>
status sum(const expansion<T, Capacity> &left, const expansion<T, Capacity> &right,
           expansion<T, Capacity> &output) noexcept {
    if (left.size == 0 || right.size == 0 || left.size > Capacity || right.size > Capacity) {
        return status::invalid_argument;
    }
    expansion<T, Capacity> current = left;
    for (std::size_t i = 0; i < right.size; ++i) {
        expansion<T, Capacity> next;
        const auto result = grow(current, right.limbs[i], next);
        if (result != status::success) return result;
        current = next;
    }
    output = current;
    return status::success;
}

template<class T, std::size_t Capacity>
status scale(const expansion<T, Capacity> &input, T factor,
             expansion<T, Capacity> &output) noexcept {
    if (!finite(factor) || input.size == 0 || input.size > Capacity) return status::invalid_argument;
    expansion<T, Capacity> total;
    total.limbs[0] = T(0);
    total.size = 1;
    for (std::size_t i = 0; i < input.size; ++i) {
        T product = T(0), residual = T(0);
        const auto product_status = two_product(input.limbs[i], factor, product, residual);
        if (product_status != status::success) return product_status;
        expansion<T, Capacity> term;
        term.size = residual == T(0) ? 1 : 2;
        term.limbs[0] = residual == T(0) ? product : residual;
        if (residual != T(0)) term.limbs[1] = product;
        expansion<T, Capacity> next;
        const auto sum_status = sum(total, term, next);
        if (sum_status != status::success) return sum_status;
        total = next;
    }
    output = total;
    return status::success;
}

template<class T, std::size_t Capacity>
status compress(const expansion<T, Capacity> &input,
                expansion<T, Capacity> &output) noexcept {
    if (input.size == 0 || input.size > Capacity) return status::invalid_argument;
    std::size_t used = 0;
    for (std::size_t i = 0; i < input.size; ++i) {
        if (!finite(input.limbs[i])) return status::non_finite_input;
        if (input.limbs[i] != T(0)) output.limbs[used++] = input.limbs[i];
    }
    if (used == 0) output.limbs[used++] = T(0);
    output.size = used;
    return status::success;
}

template<class T, std::size_t Capacity>
sign expansion_sign(const expansion<T, Capacity> &value) noexcept {
    if (value.size == 0 || value.size > Capacity) return sign::unavailable;
    for (std::size_t i = value.size; i != 0; --i) {
        const T limb = value.limbs[i - 1];
        if (!finite(limb)) return sign::unavailable;
        if (limb < T(0)) return sign::negative;
        if (limb > T(0)) return sign::positive;
    }
    return sign::zero;
}

template<class T>
status product_difference_sign(T a, T b, T c, sign &result,
                               std::size_t &capacity_used) noexcept {
    static_assert(supported_scalar_v<T>);
    if (!finite(a) || !finite(b) || !finite(c)) return status::non_finite_input;
    if (b == T(0) || c == T(0)) {
        result = a < T(0) ? sign::negative : a > T(0) ? sign::positive : sign::zero;
        capacity_used = 1;
        return status::success;
    }
    int b_exp = 0, c_exp = 0;
    (void)std::frexp(b, &b_exp);
    (void)std::frexp(c, &c_exp);
    const int b_shift = -2 - b_exp;
    const int c_shift = -2 - c_exp;
    const T scaled_b = std::scalbn(b, b_shift);
    const T scaled_c = std::scalbn(c, c_shift);
    const T scaled_a = std::scalbn(a, b_shift + c_shift);
    if (!finite(scaled_a) || (a != T(0) && scaled_a == T(0))) {
        return status::exact_scaling_unavailable;
    }
    T product = T(0), product_residual = T(0);
    const auto product_status = two_product(scaled_b, scaled_c, product, product_residual);
    if (product_status != status::success) return product_status;
    T difference = T(0), difference_residual = T(0);
    two_diff(scaled_a, product, difference, difference_residual);
    expansion<T, 8> relation;
    relation.size = 1;
    relation.limbs[0] = difference;
    expansion<T, 8> next;
    auto relation_status = grow(relation, difference_residual, next);
    if (relation_status != status::success) return relation_status;
    relation_status = grow(next, -product_residual, relation);
    if (relation_status != status::success) return relation_status;
    capacity_used = relation.size;
    result = expansion_sign(relation);
    return result == sign::unavailable ? status::invalid_argument : status::success;
}

namespace detail {
template<class T, std::size_t N, std::size_t Capacity>
status determinant_recursive(const std::array<T, N * N> &matrix, std::size_t dimension,
                             expansion<T, Capacity> &result) noexcept {
    if (dimension == 1) {
        result.size = 1;
        result.limbs[0] = matrix[0];
        return status::success;
    }
    expansion<T, Capacity> total;
    total.size = 1;
    total.limbs[0] = T(0);
    for (std::size_t column = 0; column < dimension; ++column) {
        std::array<T, N * N> minor{};
        std::size_t out = 0;
        for (std::size_t row = 1; row < dimension; ++row) {
            for (std::size_t inner_column = 0; inner_column < dimension; ++inner_column) {
                if (inner_column != column) minor[out++] = matrix[row * N + inner_column];
            }
        }
        // Pack the smaller matrix at its compile-time stride.
        std::array<T, N * N> packed{};
        for (std::size_t row = 0; row + 1 < dimension; ++row) {
            for (std::size_t inner_column = 0; inner_column + 1 < dimension; ++inner_column) {
                packed[row * N + inner_column] = minor[row * (dimension - 1) + inner_column];
            }
        }
        expansion<T, Capacity> cofactor;
        auto operation_status = determinant_recursive<T, N, Capacity>(packed, dimension - 1, cofactor);
        if (operation_status != status::success) return operation_status;
        const T factor = column % 2 == 0 ? matrix[column] : -matrix[column];
        expansion<T, Capacity> term;
        operation_status = scale(cofactor, factor, term);
        if (operation_status != status::success) return operation_status;
        expansion<T, Capacity> next;
        operation_status = sum(total, term, next);
        if (operation_status != status::success) return operation_status;
        total = next;
    }
    result = total;
    return status::success;
}
} // namespace detail

template<class T, std::size_t N, std::size_t Capacity = 512>
status determinant_sign(const std::array<T, N * N> &input, sign &result,
                        std::size_t &capacity_used, int &normalization_exponent) noexcept {
    static_assert(supported_scalar_v<T>);
    static_assert(N >= 2 && N <= 4);
    std::array<T, N * N> matrix = input;
    normalization_exponent = 0;
    for (T value : input) if (!finite(value)) return status::non_finite_input;

    // A positive power-of-two factor is applied independently to each column.
    // Determinant sign and zero are invariant under these exact column scalings.
    for (std::size_t column = 0; column < N; ++column) {
        int maximum_exponent = std::numeric_limits<int>::min();
        for (std::size_t row = 0; row < N; ++row) {
            const T value = matrix[row * N + column];
            if (value != T(0)) {
                int exponent = 0;
                (void)std::frexp(value, &exponent);
                maximum_exponent = std::max(maximum_exponent, exponent);
            }
        }
        if (maximum_exponent == std::numeric_limits<int>::min()) {
            result = sign::zero;
            capacity_used = 1;
            return status::success;
        }
        const int shift = -2 - maximum_exponent;
        normalization_exponent += shift;
        for (std::size_t row = 0; row < N; ++row) {
            const T original = matrix[row * N + column];
            const T scaled = std::scalbn(original, shift);
            if (!finite(scaled) || (original != T(0) && scaled == T(0)) ||
                std::scalbn(scaled, -shift) != original) {
                return status::exact_scaling_unavailable;
            }
            matrix[row * N + column] = scaled;
        }
    }
    expansion<T, Capacity> determinant;
    const auto operation_status = detail::determinant_recursive<T, N, Capacity>(matrix, N, determinant);
    if (operation_status != status::success) return operation_status;
    expansion<T, Capacity> compact;
    const auto compact_status = compress(determinant, compact);
    if (compact_status != status::success) return compact_status;
    capacity_used = compact.size;
    result = expansion_sign(compact);
    return result == sign::unavailable ? status::invalid_argument : status::success;
}

} // namespace ygor::exact_float_expansion_core
