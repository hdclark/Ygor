//YgorMultiprecision.h - Exact integer/rational helpers for Ygor BSP code.
#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <iosfwd>
#include <limits>
#include <tuple>
#include <type_traits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "YgorMath.h"

class ExactInt {
public:
    ExactInt();
    ExactInt(int64_t v);

    int sign() const;
    bool is_zero() const;
    ExactInt abs() const;

    ExactInt& operator+=(const ExactInt &rhs);
    ExactInt& operator-=(const ExactInt &rhs);
    ExactInt& operator*=(const ExactInt &rhs);
    ExactInt& operator/=(const ExactInt &rhs);
    ExactInt& operator%=(const ExactInt &rhs);
    ExactInt& operator<<=(int bits);

    template <class T>
    T convert_to() const { return static_cast<T>(to_long_double()); }

    long double to_long_double() const;

    friend ExactInt operator-(ExactInt v);
    friend ExactInt operator+(ExactInt lhs, const ExactInt &rhs) { lhs += rhs; return lhs; }
    friend ExactInt operator-(ExactInt lhs, const ExactInt &rhs) { lhs -= rhs; return lhs; }
    friend ExactInt operator*(ExactInt lhs, const ExactInt &rhs) { lhs *= rhs; return lhs; }
    friend ExactInt operator/(ExactInt lhs, const ExactInt &rhs) { lhs /= rhs; return lhs; }
    friend ExactInt operator%(ExactInt lhs, const ExactInt &rhs) { lhs %= rhs; return lhs; }
    friend ExactInt operator<<(ExactInt lhs, int bits) { lhs <<= bits; return lhs; }

    friend bool operator<(const ExactInt &lhs, const ExactInt &rhs);
    friend bool operator==(const ExactInt &lhs, const ExactInt &rhs);

private:
    static constexpr uint32_t base = 1000000000U;
    int sign_ = 0;
    std::vector<uint32_t> limbs_;

    void normalize();
    static int abs_compare(const ExactInt &lhs, const ExactInt &rhs);
    static ExactInt abs_add(const ExactInt &lhs, const ExactInt &rhs);
    static ExactInt abs_sub(const ExactInt &lhs, const ExactInt &rhs);
    static void div_mod_abs(const ExactInt &a, const ExactInt &b, ExactInt &q, ExactInt &r);
    void mul_small(uint32_t m);
    uint32_t div_small(uint32_t d);
};

bool operator!=(const ExactInt &lhs, const ExactInt &rhs);
bool operator>(const ExactInt &lhs, const ExactInt &rhs);
bool operator<=(const ExactInt &lhs, const ExactInt &rhs);
bool operator>=(const ExactInt &lhs, const ExactInt &rhs);
ExactInt abs_exact_int(ExactInt v);
ExactInt gcd_exact_int(ExactInt a, ExactInt b);

struct ExactScalar {
    ExactInt n = 0;
    ExactInt d = 1;

    ExactScalar() = default;
    ExactScalar(int64_t v) : n(v), d(1) {}
    ExactScalar(ExactInt num, ExactInt den);

    template <class T>
    static ExactScalar from_binary(T value) {
        static_assert(std::numeric_limits<T>::is_iec559, "ExactScalar requires IEC 559 floating point input.");
        if(!std::isfinite(value)) throw std::invalid_argument("ExactScalar::from_binary: non-finite input.");
        if(value == static_cast<T>(0)) return ExactScalar();
        int exp = 0;
        const T frac = std::frexp(value, &exp);
        const int digits = std::numeric_limits<T>::digits;
        const T scaled = std::ldexp(frac, digits);
        const auto mantissa = static_cast<int64_t>(scaled);
        ExactInt num = mantissa;
        ExactInt den = 1;
        const int pow2 = exp - digits;
        if(pow2 >= 0) num <<= pow2; else den <<= -pow2;
        return ExactScalar(std::move(num), std::move(den));
    }

    void normalize();
    int sign() const;
    bool is_zero() const;
};

ExactScalar operator-(const ExactScalar &a);
ExactScalar operator+(const ExactScalar &a, const ExactScalar &b);
ExactScalar operator-(const ExactScalar &a, const ExactScalar &b);
ExactScalar operator*(const ExactScalar &a, const ExactScalar &b);
ExactScalar operator/(const ExactScalar &a, const ExactScalar &b);
bool operator==(const ExactScalar &a, const ExactScalar &b);
bool operator!=(const ExactScalar &a, const ExactScalar &b);
bool operator<(const ExactScalar &a, const ExactScalar &b);
bool operator<=(const ExactScalar &a, const ExactScalar &b);
bool operator>(const ExactScalar &a, const ExactScalar &b);
bool operator>=(const ExactScalar &a, const ExactScalar &b);

enum class ExactVertexKind : uint8_t {
    OriginalVertex = 0,
    OriginalEdgeSplit = 1,
    PlaneTriple = 2,
    Coordinate = 3
};

struct ExactVertexSymbol {
    ExactVertexKind kind = ExactVertexKind::Coordinate;
    std::array<uint64_t, 3> ids = {{0, 0, 0}};
};

struct ExactPoint2 { ExactScalar x; ExactScalar y; };
ExactPoint2 operator+(const ExactPoint2 &a, const ExactPoint2 &b);
ExactPoint2 operator-(const ExactPoint2 &a, const ExactPoint2 &b);
ExactPoint2 operator*(const ExactPoint2 &p, const ExactScalar &s);
bool operator==(const ExactPoint2 &a, const ExactPoint2 &b);
bool exact_point2_less(const ExactPoint2 &a, const ExactPoint2 &b);
ExactScalar cross_exact_2d(const ExactPoint2 &a, const ExactPoint2 &b);
ExactScalar orient_exact_2d(const ExactPoint2 &a, const ExactPoint2 &b, const ExactPoint2 &c);

struct ExactPoint3 {
    ExactScalar x; ExactScalar y; ExactScalar z;
    template <class T>
    static ExactPoint3 from_vec3(const vec3<T> &v) {
        return { ExactScalar::from_binary(v.x), ExactScalar::from_binary(v.y), ExactScalar::from_binary(v.z) };
    }
};
ExactPoint3 operator+(const ExactPoint3 &a, const ExactPoint3 &b);
ExactPoint3 operator-(const ExactPoint3 &a, const ExactPoint3 &b);
ExactPoint3 operator*(const ExactPoint3 &p, const ExactScalar &s);
bool operator==(const ExactPoint3 &a, const ExactPoint3 &b);
bool exact_point_less(const ExactPoint3 &a, const ExactPoint3 &b);
ExactScalar dot_exact(const ExactPoint3 &a, const ExactPoint3 &b);
ExactPoint3 cross_exact(const ExactPoint3 &a, const ExactPoint3 &b);

struct ExactPlane3 {
    ExactScalar a; ExactScalar b; ExactScalar c; ExactScalar d;
    static ExactPlane3 from_points(const ExactPoint3 &p0, const ExactPoint3 &p1, const ExactPoint3 &p2);
    ExactScalar eval(const ExactPoint3 &p) const;
};
struct ExactLine3 { ExactPoint3 p; ExactPoint3 dir; };

bool ygor_multiprecision_self_test();
