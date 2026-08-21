#include "YgorMeshesAdaptivePredicates.h"
#include "YgorMeshesExactFloatExpansionCore.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace core = ygor::exact_float_expansion_core;

namespace {

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

template<class T>
int scalar_sign(T value) {
    return value < T(0) ? -1 : value > T(0) ? 1 : 0;
}

int core_sign(core::sign value) {
    if (value == core::sign::negative) return -1;
    if (value == core::sign::positive) return 1;
    if (value == core::sign::zero) return 0;
    throw std::runtime_error("strict core returned unavailable sign");
}

template<class T, std::size_t Capacity>
core::expansion<T, Capacity> make_expansion(const T *values, int size) {
    require(size > 0 && static_cast<std::size_t>(size) <= Capacity,
            "valid expansion size");
    core::expansion<T, Capacity> result;
    result.size = static_cast<std::size_t>(size);
    for (int i = 0; i < size; ++i) result.limbs[static_cast<std::size_t>(i)] = values[i];
    return result;
}

template<class T, std::size_t Capacity>
void require_same_value_and_sign(const T *legacy, int legacy_size,
                                 const core::expansion<T, Capacity> &strict,
                                 const char *message) {
    const T legacy_estimate = adaptive_predicate::estimate(legacy_size, legacy);
    T strict_estimate = T(0);
    for (std::size_t i = 0; i < strict.size; ++i) strict_estimate += strict.limbs[i];
    require(legacy_estimate == strict_estimate, message);

    T compacted[Capacity]{};
    const int compacted_size = adaptive_predicate::compress(legacy_size, legacy, compacted);
    require(scalar_sign(compacted[compacted_size - 1]) ==
                core_sign(core::expansion_sign(strict)),
            message);
}

template<class T>
void test_primitives() {
    const T epsilon = std::numeric_limits<T>::epsilon();

    T legacy_hi = T(0), legacy_lo = T(0);
    T strict_hi = T(0), strict_lo = T(0);
    adaptive_predicate::two_sum(T(1), epsilon / T(2), legacy_hi, legacy_lo);
    core::two_sum(T(1), epsilon / T(2), strict_hi, strict_lo);
    require(legacy_hi == strict_hi && legacy_lo == strict_lo,
            "two_sum agrees with strict core");
    require(static_cast<long double>(legacy_hi) + static_cast<long double>(legacy_lo) ==
                static_cast<long double>(T(1)) + static_cast<long double>(epsilon / T(2)),
            "two_sum is an exact decomposition");

    const T factor = T(1) + epsilon;
    adaptive_predicate::two_product(factor, factor, legacy_hi, legacy_lo);
    const auto product_status = core::two_product(factor, factor, strict_hi, strict_lo);
    require(product_status == core::status::success, "strict two_product succeeds");
    require(legacy_hi == strict_hi && legacy_lo == strict_lo,
            "two_product agrees with strict core");
    require(legacy_hi == T(1) + T(2) * epsilon && legacy_lo == epsilon * epsilon,
            "two_product is an exact decomposition");

    adaptive_predicate::two_product(T(-0.75), T(1.5), legacy_hi, legacy_lo);
    require(core::two_product(T(-0.75), T(1.5), strict_hi, strict_lo) ==
                core::status::success &&
                legacy_hi == strict_hi && legacy_lo == strict_lo &&
                legacy_hi + legacy_lo == T(-1.125),
            "two_product handles representative signed finite values");
}

template<class T>
void test_expansions() {
    constexpr std::size_t capacity = 16;
    const T epsilon = std::numeric_limits<T>::epsilon();
    const T e[] = {epsilon / T(2), T(1)};
    const T f[] = {epsilon / T(4), T(-1)};
    const auto strict_e = make_expansion<T, capacity>(e, 2);
    const auto strict_f = make_expansion<T, capacity>(f, 2);

    T legacy[capacity]{};
    int legacy_size = adaptive_predicate::grow_expansion(2, e, T(-1), legacy);
    core::expansion<T, capacity> strict;
    require(core::grow(strict_e, T(-1), strict) == core::status::success,
            "strict grow succeeds");
    require_same_value_and_sign(legacy, legacy_size, strict,
                                "grow_expansion agrees with strict core");

    legacy_size = adaptive_predicate::expansion_sum(2, e, 2, f, legacy);
    require(core::sum(strict_e, strict_f, strict) == core::status::success,
            "strict expansion sum succeeds");
    require_same_value_and_sign(legacy, legacy_size, strict,
                                "expansion_sum agrees with strict core");

    legacy_size = adaptive_predicate::scale_expansion(2, e, T(3), legacy);
    require(core::scale(strict_e, T(3), strict) == core::status::success,
            "strict expansion scale succeeds");
    require_same_value_and_sign(legacy, legacy_size, strict,
                                "scale_expansion agrees with strict core");

    const T sparse[] = {T(0), -epsilon, T(0), T(2), T(0)};
    T legacy_compact[capacity]{};
    const int compact_size = adaptive_predicate::compress(5, sparse, legacy_compact);
    const auto strict_sparse = make_expansion<T, capacity>(sparse, 5);
    require(core::compress(strict_sparse, strict) == core::status::success,
            "strict compress succeeds");
    require(compact_size == 2 && strict.size == 2 &&
                legacy_compact[0] == strict.limbs[0] &&
                legacy_compact[1] == strict.limbs[1],
            "compress agrees with strict core");

    const T estimate_terms[] = {epsilon / T(2), T(1), T(2)};
    T expected = T(0);
    for (T term : estimate_terms) expected += term;
    require(adaptive_predicate::estimate(3, estimate_terms) == expected,
            "estimate preserves legacy ordered sum");
}

template<class T>
core::sign orient_core(const std::array<T, 3> &a, const std::array<T, 3> &b,
                       const std::array<T, 3> &c, const std::array<T, 3> &d) {
    std::array<T, 9> matrix{};
    const std::array<std::array<T, 3>, 3> points = {a, b, c};
    for (std::size_t row = 0; row < 3; ++row) {
        for (std::size_t column = 0; column < 3; ++column) {
            matrix[row * 3 + column] = points[row][column] - d[column];
        }
    }
    core::sign result = core::sign::unavailable;
    std::size_t used = 0;
    int exponent = 0;
    require(core::determinant_sign<T, 3>(matrix, result, used, exponent) ==
                core::status::success,
            "strict orientation determinant succeeds");
    return result;
}

template<class T>
core::sign insphere_core(const std::array<T, 3> &a, const std::array<T, 3> &b,
                         const std::array<T, 3> &c, const std::array<T, 3> &d,
                         const std::array<T, 3> &e) {
    const std::array<std::array<T, 3>, 4> points = {a, b, c, d};
    std::array<T, 16> matrix{};
    for (std::size_t row = 0; row < 4; ++row) {
        T lift = T(0);
        for (std::size_t column = 0; column < 3; ++column) {
            const T translated = points[row][column] - e[column];
            matrix[row * 4 + column] = translated;
            lift += translated * translated;
        }
        matrix[row * 4 + 3] = lift;
    }
    core::sign result = core::sign::unavailable;
    std::size_t used = 0;
    int exponent = 0;
    require(core::determinant_sign<T, 4>(matrix, result, used, exponent) ==
                core::status::success,
            "strict insphere determinant succeeds");
    return result;
}

template<class T>
void test_orient3d() {
    const T delta = std::numeric_limits<T>::epsilon();
    const std::array<T, 3> a = {T(1), T(0), T(0)};
    const std::array<T, 3> b = {T(0), T(1), T(0)};
    const std::array<T, 3> d = {T(0), T(0), T(0)};

    for (T height : {delta, -delta, T(0)}) {
        const std::array<T, 3> c = {T(1), T(1), height};
        const int expected = core_sign(orient_core(a, b, c, d));
        require(scalar_sign(adaptive_predicate::orient3d_adaptive(
                    a.data(), b.data(), c.data(), d.data())) == expected,
                "orient3d_adaptive sign agrees with strict core");
        require(scalar_sign(adaptive_predicate::orient3d(
                    a.data(), b.data(), c.data(), d.data())) == expected,
                "orient3d sign agrees with strict core");
        require(expected == scalar_sign(height), "orientation sign and coplanarity");
    }
}

template<class T>
void test_insphere() {
    const std::array<T, 3> a = {T(1), T(0), T(0)};
    const std::array<T, 3> b = {T(0), T(1), T(0)};
    const std::array<T, 3> c = {T(0), T(0), T(1)};
    const std::array<T, 3> d = {T(-1), T(0), T(0)};
    const std::array<T, 3> inside = {T(0), T(0), T(0)};
    const std::array<T, 3> outside = {T(2), T(0), T(0)};
    const std::array<T, 3> cospherical = {T(0), T(-1), T(0)};

    int inside_sign = 0;
    int outside_sign = 0;
    for (const auto &query : {inside, outside, cospherical}) {
        const int expected = core_sign(insphere_core(a, b, c, d, query));
        require(scalar_sign(adaptive_predicate::insphere_adaptive(
                    a.data(), b.data(), c.data(), d.data(), query.data())) == expected,
                "insphere_adaptive sign agrees with strict core");
        require(scalar_sign(adaptive_predicate::insphere(
                    a.data(), b.data(), c.data(), d.data(), query.data())) == expected,
                "insphere sign agrees with strict core");
        if (query == inside) inside_sign = expected;
        if (query == outside) outside_sign = expected;
        if (query == cospherical) require(expected == 0, "cospherical case is exact zero");
    }
    require(inside_sign != 0 && outside_sign == -inside_sign,
            "inside and outside insphere signs are opposite");
}

template<class T>
void run_for_type() {
    static_assert(std::is_same_v<T, float> || std::is_same_v<T, double>);
    test_primitives<T>();
    test_expansions<T>();
    test_orient3d<T>();
    test_insphere<T>();
}

} // namespace

int main() {
    try {
        run_for_type<float>();
        run_for_type<double>();
        std::cout << "Adaptive predicate compatibility tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
