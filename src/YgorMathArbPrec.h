#pragma once
#ifndef YGOR_MATH_ARB_PREC_H_
#define YGOR_MATH_ARB_PREC_H_

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <limits>
#include <string>
#include <vector>

class ArbPrec {
    public:
        using limb_type = uint32_t;

        ArbPrec();
        ArbPrec(int value);
        ArbPrec(unsigned int value);
        ArbPrec(long value);
        ArbPrec(unsigned long value);
        ArbPrec(long long value);
        ArbPrec(unsigned long long value);
        ArbPrec(float value);
        ArbPrec(double value);
        ArbPrec(long double value);
        ArbPrec(float value, std::size_t precision_bits);
        ArbPrec(double value, std::size_t precision_bits);
        ArbPrec(long double value, std::size_t precision_bits);

        static void set_default_precision_bits(std::size_t bits) noexcept;
        static std::size_t default_precision_bits() noexcept;

        std::size_t precision_bits() const noexcept;
        void set_precision_bits(std::size_t bits);
        ArbPrec with_precision_bits(std::size_t bits) const;

        bool is_zero() const noexcept;
        bool is_negative() const noexcept;
        std::string to_hex_string() const;

        explicit operator float() const;
        explicit operator double() const;
        explicit operator long double() const;

        ArbPrec operator+() const;
        ArbPrec operator-() const;

        ArbPrec &operator+=(const ArbPrec &rhs);
        ArbPrec &operator-=(const ArbPrec &rhs);
        ArbPrec &operator*=(const ArbPrec &rhs);
        ArbPrec &operator/=(const ArbPrec &rhs);

        friend ArbPrec operator+(ArbPrec lhs, const ArbPrec &rhs){ lhs += rhs; return lhs; }
        friend ArbPrec operator-(ArbPrec lhs, const ArbPrec &rhs){ lhs -= rhs; return lhs; }
        friend ArbPrec operator*(ArbPrec lhs, const ArbPrec &rhs){ lhs *= rhs; return lhs; }
        friend ArbPrec operator/(ArbPrec lhs, const ArbPrec &rhs){ lhs /= rhs; return lhs; }

        friend bool operator==(const ArbPrec &lhs, const ArbPrec &rhs) noexcept;
        friend bool operator!=(const ArbPrec &lhs, const ArbPrec &rhs) noexcept;
        friend bool operator<(const ArbPrec &lhs, const ArbPrec &rhs);
        friend bool operator<=(const ArbPrec &lhs, const ArbPrec &rhs);
        friend bool operator>(const ArbPrec &lhs, const ArbPrec &rhs);
        friend bool operator>=(const ArbPrec &lhs, const ArbPrec &rhs);

        friend std::ostream &operator<<(std::ostream &out, const ArbPrec &value);
        friend std::istream &operator>>(std::istream &in, ArbPrec &value);

        static ArbPrec epsilon_value(std::size_t precision_bits = 0);
        static ArbPrec min_value(std::size_t precision_bits = 0);
        static ArbPrec max_value(std::size_t precision_bits = 0);
        static ArbPrec lowest_value(std::size_t precision_bits = 0);
        static ArbPrec power_of_two(std::int64_t exponent, std::size_t precision_bits = 0);

    private:
        int m_sign = 0;
        std::int64_t m_exponent = 0;
        std::size_t m_precision_bits = 0;
        std::vector<limb_type> m_limbs;

        ArbPrec(int sign,
                std::int64_t exponent,
                std::vector<limb_type> limbs,
                std::size_t precision_bits);

        static std::size_t clamp_precision_bits(std::size_t bits) noexcept;
        void normalize();

        friend ArbPrec abs(const ArbPrec &value);
        friend ArbPrec fabs(const ArbPrec &value);
        friend ArbPrec sqrt(const ArbPrec &value);
        friend bool isfinite(const ArbPrec &value) noexcept;

        template <class Float>
        void assign_from_floating_value(Float value, std::size_t precision_bits);

        void assign_from_signed_integer(long long value, std::size_t precision_bits);
        void assign_from_unsigned_integer(unsigned long long value, std::size_t precision_bits);
        void assign_from_hex_string(const std::string &repr, std::size_t precision_bits);
};

ArbPrec abs(const ArbPrec &value);
ArbPrec fabs(const ArbPrec &value);
ArbPrec sqrt(const ArbPrec &value);
bool isfinite(const ArbPrec &value) noexcept;

namespace std {

template <>
class numeric_limits<ArbPrec> {
    public:
        static constexpr bool is_specialized = true;
        static constexpr bool is_signed = true;
        static constexpr bool is_integer = false;
        static constexpr bool is_exact = false;
        static constexpr bool has_infinity = false;
        static constexpr bool has_quiet_NaN = false;
        static constexpr bool has_signaling_NaN = false;
        static constexpr float_denorm_style has_denorm = denorm_absent;
        static constexpr bool has_denorm_loss = false;
        static constexpr rounding_style round_style = round_to_nearest;
        static constexpr bool is_iec559 = false;
        static constexpr bool is_bounded = false;
        static constexpr bool is_modulo = false;
        static constexpr int digits = 256;
        static constexpr int digits10 = 76;
        static constexpr int max_digits10 = 80;
        static constexpr int radix = 2;
        static constexpr int min_exponent = std::numeric_limits<int>::min();
        static constexpr int min_exponent10 = std::numeric_limits<int>::min();
        static constexpr int max_exponent = std::numeric_limits<int>::max();
        static constexpr int max_exponent10 = std::numeric_limits<int>::max();
        static constexpr bool traps = false;
        static constexpr bool tinyness_before = false;

        static ArbPrec min() noexcept { return ArbPrec::min_value(); }
        static ArbPrec lowest() noexcept { return ArbPrec::lowest_value(); }
        static ArbPrec max() noexcept { return ArbPrec::max_value(); }
        static ArbPrec epsilon() noexcept { return ArbPrec::epsilon_value(); }
        static ArbPrec round_error() noexcept { return ArbPrec(0.5); }
        static ArbPrec infinity() noexcept { return ArbPrec::max_value(); }
        static ArbPrec quiet_NaN() noexcept { return ArbPrec(); }
        static ArbPrec signaling_NaN() noexcept { return ArbPrec(); }
        static ArbPrec denorm_min() noexcept { return ArbPrec::min_value(); }
};

} // namespace std

#endif // YGOR_MATH_ARB_PREC_H_
