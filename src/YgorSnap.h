//YgorSnap.h
//
// This file defines a fixed-precision "snap" floating-point type that approximates
// floating point arithmetic using an integer representation scaled by a configurable
// factor. This helps avoid floating-point precision issues in algorithms that require
// deterministic and consistent rounding behavior.
//
// The snap_fp class stores values internally as integers, where the stored value is
// the floating point value multiplied by a scaling factor (template parameter).
// Binary operations between snap_fp objects require matching scaling factors.
//
// Example:
//   snap_fp<double, 1000000> a(1.234567);  // Internal: 1234567
//   snap_fp<double, 1000000> b(0.000001);  // Internal: 1
//   auto c = a + b;  // c.to_fp() == 1.234568
//

#ifndef YGOR_SNAP_H_
#define YGOR_SNAP_H_

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <type_traits>
#include <string>
#include <stdexcept>

// Forward declarations for friend functions.
template <class T, int64_t ScaleFactor> class snap_fp;

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> abs(const snap_fp<T, ScaleFactor> &);

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> sqrt(const snap_fp<T, ScaleFactor> &);

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> floor(const snap_fp<T, ScaleFactor> &);

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> ceil(const snap_fp<T, ScaleFactor> &);

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> round(const snap_fp<T, ScaleFactor> &);

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> fmod(const snap_fp<T, ScaleFactor> &, const snap_fp<T, ScaleFactor> &);

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> min(const snap_fp<T, ScaleFactor> &, const snap_fp<T, ScaleFactor> &);

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> max(const snap_fp<T, ScaleFactor> &, const snap_fp<T, ScaleFactor> &);

template <class Y, int64_t S>
std::ostream & operator<<(std::ostream &, const snap_fp<Y, S> &);

template <class Y, int64_t S>
std::istream & operator>>(std::istream &, snap_fp<Y, S> &);

//---------------------------------------------------------------------------------------------------------------------------
//-------------------------------- snap_fp: Fixed-precision snap type for floating point -----------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//
// This class provides a drop-in replacement for floating point types (float/double) that
// "snaps" values to a fixed-precision grid determined by the ScaleFactor template parameter.
//
// The internal representation is an integer (int64_t) equal to the floating point value
// multiplied by ScaleFactor. This ensures deterministic rounding and avoids floating point
// precision issues in algorithms that are sensitive to small numerical errors.
//
// Template parameters:
//   T           - The underlying floating point type (float or double) for conversions.
//   ScaleFactor - The precision factor. A value of 1000000 means precision to 1e-6.
//                 Must be a positive integer. Higher values = more precision but
//                 smaller range.
//
// Notes:
// - Binary operations between snap_fp types with different ScaleFactors are not allowed
//   and will result in a compile-time error.
// - Operations with raw floating point values are supported via operator overloads.
// - The range of representable values is approximately:
//   [-INT64_MAX/ScaleFactor, INT64_MAX/ScaleFactor]
//
template <class T, int64_t ScaleFactor>
class snap_fp {
    static_assert(std::is_floating_point<T>::value, "snap_fp requires a floating point type");
    static_assert(ScaleFactor > 0, "ScaleFactor must be positive");

    public:
        using value_type = T;
        using internal_type = int64_t;
        static constexpr int64_t scale_factor = ScaleFactor;

    private:
        internal_type m_value;  // The scaled integer representation.

        // Private constructor from raw internal value (for internal use).
        struct raw_tag {};
        explicit constexpr snap_fp(raw_tag, internal_type raw_value) : m_value(raw_value) {}

    public:
        //---------------------------
        // Constructors
        //---------------------------

        // Default constructor - initializes to zero.
        constexpr snap_fp() : m_value(0) {}

        // Constructor from floating point value.
        // Rounds the floating point value to the nearest representable value.
        explicit snap_fp(T value)
            : m_value(static_cast<internal_type>(std::round(value * static_cast<T>(ScaleFactor)))) {}

        // Copy constructor.
        constexpr snap_fp(const snap_fp &other) = default;

        // Move constructor.
        constexpr snap_fp(snap_fp &&other) noexcept = default;

        //---------------------------
        // Assignment operators
        //---------------------------

        // Copy assignment.
        snap_fp& operator=(const snap_fp &other) = default;

        // Move assignment.
        snap_fp& operator=(snap_fp &&other) noexcept = default;

        // Assignment from floating point value.
        snap_fp& operator=(T value) {
            m_value = static_cast<internal_type>(std::round(value * static_cast<T>(ScaleFactor)));
            return *this;
        }

        //---------------------------
        // Conversion to floating point
        //---------------------------

        // Convert to floating point.
        T to_fp() const {
            return static_cast<T>(m_value) / static_cast<T>(ScaleFactor);
        }

        // Explicit conversion operator.
        explicit operator T() const {
            return to_fp();
        }

        //---------------------------
        // Access to internal representation
        //---------------------------

        // Get the raw internal value (for advanced use cases).
        constexpr internal_type raw() const { return m_value; }

        // Create from raw internal value (for advanced use cases).
        static constexpr snap_fp from_raw(internal_type raw_value) {
            return snap_fp(raw_tag{}, raw_value);
        }

        //---------------------------
        // Unary operators
        //---------------------------

        // Unary plus.
        constexpr snap_fp operator+() const {
            return *this;
        }

        // Unary minus (negation).
        constexpr snap_fp operator-() const {
            return snap_fp(raw_tag{}, -m_value);
        }

        //---------------------------
        // Binary arithmetic operators with same snap_fp type
        //---------------------------

    private:
        // Helper function to add with overflow checking.
        // Returns the sum if no overflow occurs, otherwise throws.
        static internal_type checked_add(internal_type a, internal_type b) {
            if (b > 0 && a > std::numeric_limits<internal_type>::max() - b) {
                throw std::overflow_error("snap_fp: addition overflow");
            }
            if (b < 0 && a < std::numeric_limits<internal_type>::min() - b) {
                throw std::overflow_error("snap_fp: addition underflow");
            }
            return a + b;
        }

        // Helper function to subtract with overflow checking.
        // Returns the difference if no overflow occurs, otherwise throws.
        static internal_type checked_sub(internal_type a, internal_type b) {
            if (b < 0 && a > std::numeric_limits<internal_type>::max() + b) {
                throw std::overflow_error("snap_fp: subtraction overflow");
            }
            if (b > 0 && a < std::numeric_limits<internal_type>::min() + b) {
                throw std::overflow_error("snap_fp: subtraction underflow");
            }
            return a - b;
        }

#if !defined(__SIZEOF_INT128__)
        // Integer-based multiplication without 128-bit integers.
        // Uses the decomposition: a*b = (a_hi*2^32 + a_lo) * (b_hi*2^32 + b_lo)
        // Result is computed with overflow detection and proper scaling.
        static internal_type multiply_scaled(internal_type a, internal_type b, internal_type scale) {
            // Handle signs separately to simplify the logic.
            const bool negative = (a < 0) != (b < 0);
            uint64_t abs_a = static_cast<uint64_t>(a < 0 ? -a : a);
            uint64_t abs_b = static_cast<uint64_t>(b < 0 ? -b : b);
            uint64_t abs_scale = static_cast<uint64_t>(scale);

            // Split into high and low 32-bit parts.
            const uint64_t MASK32 = 0xFFFFFFFFULL;
            uint64_t a_lo = abs_a & MASK32;
            uint64_t a_hi = abs_a >> 32;
            uint64_t b_lo = abs_b & MASK32;
            uint64_t b_hi = abs_b >> 32;

            // Compute partial products (each fits in 64 bits).
            uint64_t lo_lo = a_lo * b_lo;
            uint64_t lo_hi = a_lo * b_hi;
            uint64_t hi_lo = a_hi * b_lo;
            uint64_t hi_hi = a_hi * b_hi;

            // Combine: result = hi_hi*2^64 + (lo_hi + hi_lo)*2^32 + lo_lo
            // Then divide by scale.
            // For simplicity, use long double for the final combination and division.
            constexpr long double TWO_POW_64 = 18446744073709551616.0L; // 2^64, equivalent to std::ldexp(1.0L, 64)
            constexpr long double TWO_POW_32 = 4294967296.0L;          // 2^32, equivalent to std::ldexp(1.0L, 32)
            long double result = static_cast<long double>(hi_hi) * TWO_POW_64;
            result += static_cast<long double>(lo_hi + hi_lo) * TWO_POW_32;
            result += static_cast<long double>(lo_lo);
            result /= static_cast<long double>(abs_scale);

            // Check for overflow.
            const long double max_val = static_cast<long double>(std::numeric_limits<internal_type>::max());
            if (result > max_val) {
                throw std::overflow_error("snap_fp: multiplication overflow");
            }

            internal_type int_result = static_cast<internal_type>(result);
            return negative ? -int_result : int_result;
        }

        // Integer-based division without 128-bit integers.
        // Computes (a * scale) / b with overflow handling.
        static internal_type divide_scaled(internal_type a, internal_type b, internal_type scale) {
            // Handle signs separately.
            const bool negative = (a < 0) != (b < 0);
            uint64_t abs_a = static_cast<uint64_t>(a < 0 ? -a : a);
            uint64_t abs_b = static_cast<uint64_t>(b < 0 ? -b : b);
            uint64_t abs_scale = static_cast<uint64_t>(scale);

            // Use long double for intermediate calculation to handle large values.
            long double dividend = static_cast<long double>(abs_a) * static_cast<long double>(abs_scale);
            long double result = dividend / static_cast<long double>(abs_b);

            // Check for overflow.
            const long double max_val = static_cast<long double>(std::numeric_limits<internal_type>::max());
            if (result > max_val) {
                throw std::overflow_error("snap_fp: division overflow");
            }

            internal_type int_result = static_cast<internal_type>(result);
            return negative ? -int_result : int_result;
        }
#endif

    public:
        snap_fp operator+(const snap_fp &other) const {
            return snap_fp(raw_tag{}, checked_add(m_value, other.m_value));
        }

        snap_fp operator-(const snap_fp &other) const {
            return snap_fp(raw_tag{}, checked_sub(m_value, other.m_value));
        }

        // Multiplication: (a*S) * (b*S) / S = a*b*S
        snap_fp operator*(const snap_fp &other) const {
            // Use 128-bit arithmetic to avoid overflow during multiplication.
            // Result = (m_value * other.m_value) / ScaleFactor
#if defined(__SIZEOF_INT128__)
            __int128 product = static_cast<__int128>(m_value) * static_cast<__int128>(other.m_value);
            return snap_fp(raw_tag{}, static_cast<internal_type>(product / ScaleFactor));
#else
            // Fallback using integer-based algorithm with long double for intermediate values.
            return snap_fp(raw_tag{}, multiply_scaled(m_value, other.m_value, ScaleFactor));
#endif
        }

        // Division: (a*S) / (b*S) * S = (a/b)*S
        snap_fp operator/(const snap_fp &other) const {
            if (other.m_value == 0) {
                throw std::domain_error("snap_fp: division by zero");
            }
#if defined(__SIZEOF_INT128__)
            __int128 dividend = static_cast<__int128>(m_value) * static_cast<__int128>(ScaleFactor);
            return snap_fp(raw_tag{}, static_cast<internal_type>(dividend / other.m_value));
#else
            // Fallback using integer-based algorithm with long double for intermediate values.
            return snap_fp(raw_tag{}, divide_scaled(m_value, other.m_value, ScaleFactor));
#endif
        }

        //---------------------------
        // Compound assignment operators with same snap_fp type
        //---------------------------

        snap_fp& operator+=(const snap_fp &other) {
            m_value = checked_add(m_value, other.m_value);
            return *this;
        }

        snap_fp& operator-=(const snap_fp &other) {
            m_value = checked_sub(m_value, other.m_value);
            return *this;
        }

        snap_fp& operator*=(const snap_fp &other) {
            *this = *this * other;
            return *this;
        }

        snap_fp& operator/=(const snap_fp &other) {
            *this = *this / other;
            return *this;
        }

        //---------------------------
        // Binary arithmetic operators with floating point
        //---------------------------

        snap_fp operator+(T value) const {
            return *this + snap_fp(value);
        }

        snap_fp operator-(T value) const {
            return *this - snap_fp(value);
        }

        snap_fp operator*(T value) const {
            // Multiply internal value by the floating point value with overflow checking.
            long double product =
                static_cast<long double>(m_value) * static_cast<long double>(value);
            const long double min_val =
                static_cast<long double>(std::numeric_limits<internal_type>::min());
            const long double max_val =
                static_cast<long double>(std::numeric_limits<internal_type>::max());
            if (product < min_val || product > max_val) {
                throw std::overflow_error("snap_fp: multiplication overflow");
            }
            return snap_fp(raw_tag{}, static_cast<internal_type>(std::round(product)));
        }

        snap_fp operator/(T value) const {
            if (value == static_cast<T>(0)) {
                throw std::domain_error("snap_fp: division by zero");
            }
            // Divide internal value by the floating point value with overflow checking.
            long double quotient =
                static_cast<long double>(m_value) / static_cast<long double>(value);
            const long double min_val =
                static_cast<long double>(std::numeric_limits<internal_type>::min());
            const long double max_val =
                static_cast<long double>(std::numeric_limits<internal_type>::max());
            if (quotient < min_val || quotient > max_val) {
                throw std::overflow_error("snap_fp: division overflow");
            }
            return snap_fp(raw_tag{}, static_cast<internal_type>(std::round(quotient)));
        }

        //---------------------------
        // Compound assignment operators with floating point
        //---------------------------

        snap_fp& operator+=(T value) {
            *this = *this + value;
            return *this;
        }

        snap_fp& operator-=(T value) {
            *this = *this - value;
            return *this;
        }

        snap_fp& operator*=(T value) {
            *this = *this * value;
            return *this;
        }

        snap_fp& operator/=(T value) {
            *this = *this / value;
            return *this;
        }

        //---------------------------
        // Comparison operators
        //---------------------------

        constexpr bool operator==(const snap_fp &other) const {
            return m_value == other.m_value;
        }

        constexpr bool operator!=(const snap_fp &other) const {
            return m_value != other.m_value;
        }

        constexpr bool operator<(const snap_fp &other) const {
            return m_value < other.m_value;
        }

        constexpr bool operator<=(const snap_fp &other) const {
            return m_value <= other.m_value;
        }

        constexpr bool operator>(const snap_fp &other) const {
            return m_value > other.m_value;
        }

        constexpr bool operator>=(const snap_fp &other) const {
            return m_value >= other.m_value;
        }

        // Comparison with floating point values.
        bool operator==(T value) const {
            return *this == snap_fp(value);
        }

        bool operator!=(T value) const {
            return *this != snap_fp(value);
        }

        bool operator<(T value) const {
            return *this < snap_fp(value);
        }

        bool operator<=(T value) const {
            return *this <= snap_fp(value);
        }

        bool operator>(T value) const {
            return *this > snap_fp(value);
        }

        bool operator>=(T value) const {
            return *this >= snap_fp(value);
        }

        //---------------------------
        // Utility functions
        //---------------------------

        // Check if value is zero.
        constexpr bool is_zero() const {
            return m_value == 0;
        }

        // Check if value is positive.
        constexpr bool is_positive() const {
            return m_value > 0;
        }

        // Check if value is negative.
        constexpr bool is_negative() const {
            return m_value < 0;
        }

        // Return the sign of the value (-1, 0, or 1).
        constexpr int sign() const {
            return (m_value > 0) - (m_value < 0);
        }

        // String representation.
        std::string to_string() const {
            return std::to_string(to_fp());
        }

        //---------------------------
        // Static utility functions
        //---------------------------

        // Get the minimum representable positive value.
        static constexpr snap_fp epsilon() {
            return snap_fp(raw_tag{}, 1);
        }

        // Get the maximum representable value.
        static constexpr snap_fp max_value() {
            return snap_fp(raw_tag{}, std::numeric_limits<internal_type>::max());
        }

        // Get the minimum representable value.
        static constexpr snap_fp min_value() {
            return snap_fp(raw_tag{}, std::numeric_limits<internal_type>::min());
        }

        // Get the lowest representable value (same as min_value for signed types).
        static constexpr snap_fp lowest() {
            return snap_fp(raw_tag{}, std::numeric_limits<internal_type>::lowest());
        }

        // Get a zero value.
        static constexpr snap_fp zero() {
            return snap_fp();
        }

        // Get a value of one.
        static constexpr snap_fp one() {
            return snap_fp(raw_tag{}, ScaleFactor);
        }

        //---------------------------
        // Friend function declarations
        //---------------------------

        friend snap_fp abs<>(const snap_fp &);
        friend snap_fp sqrt<>(const snap_fp &);
        friend snap_fp floor<>(const snap_fp &);
        friend snap_fp ceil<>(const snap_fp &);
        friend snap_fp round<>(const snap_fp &);
        friend snap_fp fmod<>(const snap_fp &, const snap_fp &);
        friend snap_fp min<>(const snap_fp &, const snap_fp &);
        friend snap_fp max<>(const snap_fp &, const snap_fp &);
        friend std::ostream& operator<< <>(std::ostream &, const snap_fp &);
        friend std::istream& operator>> <>(std::istream &, snap_fp &);
};


//---------------------------------------------------------------------------------------------------------------------------
//-------------------------------- Free functions for snap_fp type ---------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

// Absolute value.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> abs(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>::from_raw(x.m_value >= 0 ? x.m_value : -x.m_value);
}

// Square root.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> sqrt(const snap_fp<T, ScaleFactor> &x) {
    if (x.m_value < 0) {
        throw std::domain_error("snap_fp: sqrt of negative number");
    }
    return snap_fp<T, ScaleFactor>(std::sqrt(x.to_fp()));
}

// Floor (round towards negative infinity).
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> floor(const snap_fp<T, ScaleFactor> &x) {
    // Since internal representation is already at snap precision,
    // floor to integer means: floor(value) = floor(raw / ScaleFactor) * ScaleFactor
    typename snap_fp<T, ScaleFactor>::internal_type raw = x.raw();
    typename snap_fp<T, ScaleFactor>::internal_type floored;
    if (raw >= 0) {
        floored = (raw / ScaleFactor) * ScaleFactor;
    } else {
        // For negative values, floor goes more negative.
        floored = ((raw - ScaleFactor + 1) / ScaleFactor) * ScaleFactor;
    }
    return snap_fp<T, ScaleFactor>::from_raw(floored);
}

// Ceiling (round towards positive infinity).
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> ceil(const snap_fp<T, ScaleFactor> &x) {
    typename snap_fp<T, ScaleFactor>::internal_type raw = x.raw();
    typename snap_fp<T, ScaleFactor>::internal_type ceiled;
    if (raw >= 0) {
        ceiled = ((raw + ScaleFactor - 1) / ScaleFactor) * ScaleFactor;
    } else {
        // For negative values, ceil goes towards zero.
        ceiled = (raw / ScaleFactor) * ScaleFactor;
    }
    return snap_fp<T, ScaleFactor>::from_raw(ceiled);
}

// Round to nearest integer value.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> round(const snap_fp<T, ScaleFactor> &x) {
    typename snap_fp<T, ScaleFactor>::internal_type raw = x.raw();
    typename snap_fp<T, ScaleFactor>::internal_type half = ScaleFactor / 2;
    typename snap_fp<T, ScaleFactor>::internal_type rounded;
    if (raw >= 0) {
        rounded = ((raw + half) / ScaleFactor) * ScaleFactor;
    } else {
        rounded = ((raw - half) / ScaleFactor) * ScaleFactor;
    }
    return snap_fp<T, ScaleFactor>::from_raw(rounded);
}

// Floating-point modulo (remainder after division).
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> fmod(const snap_fp<T, ScaleFactor> &x, const snap_fp<T, ScaleFactor> &y) {
    if (y.raw() == 0) {
        throw std::domain_error("snap_fp: fmod by zero");
    }
    return snap_fp<T, ScaleFactor>::from_raw(x.raw() % y.raw());
}

// Minimum of two values.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> min(const snap_fp<T, ScaleFactor> &a, const snap_fp<T, ScaleFactor> &b) {
    return a.raw() <= b.raw() ? a : b;
}

// Maximum of two values.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> max(const snap_fp<T, ScaleFactor> &a, const snap_fp<T, ScaleFactor> &b) {
    return a.raw() >= b.raw() ? a : b;
}

// Power function.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> pow(const snap_fp<T, ScaleFactor> &base, const snap_fp<T, ScaleFactor> &exp) {
    return snap_fp<T, ScaleFactor>(std::pow(base.to_fp(), exp.to_fp()));
}

// Power function with integer exponent.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> pow(const snap_fp<T, ScaleFactor> &base, int exp) {
    return snap_fp<T, ScaleFactor>(std::pow(base.to_fp(), exp));
}

// Trigonometric functions (operate through floating point conversion).
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> sin(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::sin(x.to_fp()));
}

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> cos(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::cos(x.to_fp()));
}

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> tan(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::tan(x.to_fp()));
}

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> asin(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::asin(x.to_fp()));
}

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> acos(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::acos(x.to_fp()));
}

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> atan(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::atan(x.to_fp()));
}

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> atan2(const snap_fp<T, ScaleFactor> &y, const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::atan2(y.to_fp(), x.to_fp()));
}

// Exponential and logarithmic functions.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> exp(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::exp(x.to_fp()));
}

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> log(const snap_fp<T, ScaleFactor> &x) {
    if (x.raw() <= 0) {
        throw std::domain_error("snap_fp: log of non-positive number");
    }
    return snap_fp<T, ScaleFactor>(std::log(x.to_fp()));
}

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> log10(const snap_fp<T, ScaleFactor> &x) {
    if (x.raw() <= 0) {
        throw std::domain_error("snap_fp: log10 of non-positive number");
    }
    return snap_fp<T, ScaleFactor>(std::log10(x.to_fp()));
}

// Natural logarithm of (1 + x), more accurate for small x.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> log1p(const snap_fp<T, ScaleFactor> &x) {
    if (x.raw() <= -ScaleFactor) {
        throw std::domain_error("snap_fp: log1p argument must be > -1");
    }
    return snap_fp<T, ScaleFactor>(std::log1p(x.to_fp()));
}

// Logarithm base 2.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> log2(const snap_fp<T, ScaleFactor> &x) {
    if (x.raw() <= 0) {
        throw std::domain_error("snap_fp: log2 of non-positive number");
    }
    return snap_fp<T, ScaleFactor>(std::log2(x.to_fp()));
}

// Exponential minus 1, more accurate for small x.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> expm1(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::expm1(x.to_fp()));
}

// Exponential base 2.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> exp2(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::exp2(x.to_fp()));
}

// Cube root.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> cbrt(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::cbrt(x.to_fp()));
}

// Hypotenuse: sqrt(x^2 + y^2).
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> hypot(const snap_fp<T, ScaleFactor> &x, const snap_fp<T, ScaleFactor> &y) {
    return snap_fp<T, ScaleFactor>(std::hypot(x.to_fp(), y.to_fp()));
}

// Hyperbolic functions.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> sinh(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::sinh(x.to_fp()));
}

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> cosh(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::cosh(x.to_fp()));
}

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> tanh(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::tanh(x.to_fp()));
}

// Inverse hyperbolic functions.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> asinh(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::asinh(x.to_fp()));
}

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> acosh(const snap_fp<T, ScaleFactor> &x) {
    if (x.to_fp() < static_cast<T>(1)) {
        throw std::domain_error("snap_fp: acosh argument must be >= 1");
    }
    return snap_fp<T, ScaleFactor>(std::acosh(x.to_fp()));
}

template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> atanh(const snap_fp<T, ScaleFactor> &x) {
    T val = x.to_fp();
    if (val <= static_cast<T>(-1) || val >= static_cast<T>(1)) {
        throw std::domain_error("snap_fp: atanh argument must be in (-1, 1)");
    }
    return snap_fp<T, ScaleFactor>(std::atanh(val));
}

// Error function.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> erf(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::erf(x.to_fp()));
}

// Complementary error function.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> erfc(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::erfc(x.to_fp()));
}

// Gamma function.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> tgamma(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::tgamma(x.to_fp()));
}

// Log-gamma function.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> lgamma(const snap_fp<T, ScaleFactor> &x) {
    return snap_fp<T, ScaleFactor>(std::lgamma(x.to_fp()));
}

// Truncate toward zero.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> trunc(const snap_fp<T, ScaleFactor> &x) {
    typename snap_fp<T, ScaleFactor>::internal_type raw = x.raw();
    typename snap_fp<T, ScaleFactor>::internal_type truncated = (raw / ScaleFactor) * ScaleFactor;
    return snap_fp<T, ScaleFactor>::from_raw(truncated);
}

// Copy sign: returns a value with magnitude of x and sign of y.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> copysign(const snap_fp<T, ScaleFactor> &x, const snap_fp<T, ScaleFactor> &y) {
    auto abs_x = abs(x);
    return y.raw() >= 0 ? abs_x : -abs_x;
}

// Fused multiply-add: returns x * y + z with a single rounding.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> fma(const snap_fp<T, ScaleFactor> &x, 
                            const snap_fp<T, ScaleFactor> &y,
                            const snap_fp<T, ScaleFactor> &z) {
    return snap_fp<T, ScaleFactor>(std::fma(x.to_fp(), y.to_fp(), z.to_fp()));
}

// Remainder after division (IEEE 754).
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> remainder(const snap_fp<T, ScaleFactor> &x, const snap_fp<T, ScaleFactor> &y) {
    if (y.raw() == 0) {
        throw std::domain_error("snap_fp: remainder by zero");
    }
    return snap_fp<T, ScaleFactor>(std::remainder(x.to_fp(), y.to_fp()));
}

// Linear interpolation: returns (1-t)*a + t*b.
// Uses the numerically stable form that guarantees lerp(a, b, 1) == b.
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> lerp(const snap_fp<T, ScaleFactor> &a, 
                              const snap_fp<T, ScaleFactor> &b,
                              const snap_fp<T, ScaleFactor> &t) {
    // Use the stable form: (1-t)*a + t*b, computed via floating point for accuracy.
    T t_fp = t.to_fp();
    T result = (static_cast<T>(1) - t_fp) * a.to_fp() + t_fp * b.to_fp();
    return snap_fp<T, ScaleFactor>(result);
}

// Clamp value to range [lo, hi].
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> clamp(const snap_fp<T, ScaleFactor> &v,
                               const snap_fp<T, ScaleFactor> &lo,
                               const snap_fp<T, ScaleFactor> &hi) {
    return min(max(v, lo), hi);
}

// Check if value is finite (not infinite or NaN).
// Note: snap_fp uses integer internally, so it's always "finite" in the FP sense,
// but we provide this for API compatibility.
template <class T, int64_t ScaleFactor>
bool isfinite(const snap_fp<T, ScaleFactor> &) {
    return true;  // snap_fp values are always finite (integer-backed).
}

// Check if value is NaN. Always false for snap_fp.
template <class T, int64_t ScaleFactor>
bool isnan(const snap_fp<T, ScaleFactor> &) {
    return false;  // snap_fp cannot represent NaN.
}

// Check if value is infinite. Always false for snap_fp.
template <class T, int64_t ScaleFactor>
bool isinf(const snap_fp<T, ScaleFactor> &) {
    return false;  // snap_fp cannot represent infinity.
}

// Sign bit check (true if negative).
template <class T, int64_t ScaleFactor>
bool signbit(const snap_fp<T, ScaleFactor> &x) {
    return x.raw() < 0;
}

//---------------------------------------------------------------------------------------------------------------------------
//-------------------------------- Stream operators for snap_fp type -------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

template <class T, int64_t ScaleFactor>
std::ostream& operator<<(std::ostream &os, const snap_fp<T, ScaleFactor> &x) {
    os << x.to_fp();
    return os;
}

template <class T, int64_t ScaleFactor>
std::istream& operator>>(std::istream &is, snap_fp<T, ScaleFactor> &x) {
    T value;
    is >> value;
    if (is) {
        x = snap_fp<T, ScaleFactor>(value);
    }
    return is;
}

//---------------------------------------------------------------------------------------------------------------------------
//-------------------------------- Symmetric operators for snap_fp type (floating point on left side) ----------------------
//---------------------------------------------------------------------------------------------------------------------------

// Floating point + snap_fp
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> operator+(T lhs, const snap_fp<T, ScaleFactor> &rhs) {
    return snap_fp<T, ScaleFactor>(lhs) + rhs;
}

// Floating point - snap_fp
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> operator-(T lhs, const snap_fp<T, ScaleFactor> &rhs) {
    return snap_fp<T, ScaleFactor>(lhs) - rhs;
}

// Floating point * snap_fp
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> operator*(T lhs, const snap_fp<T, ScaleFactor> &rhs) {
    return rhs * lhs;  // Commutative, use member operator.
}

// Floating point / snap_fp
template <class T, int64_t ScaleFactor>
snap_fp<T, ScaleFactor> operator/(T lhs, const snap_fp<T, ScaleFactor> &rhs) {
    return snap_fp<T, ScaleFactor>(lhs) / rhs;
}

// Floating point comparisons (left side).
template <class T, int64_t ScaleFactor>
bool operator==(T lhs, const snap_fp<T, ScaleFactor> &rhs) {
    return snap_fp<T, ScaleFactor>(lhs) == rhs;
}

template <class T, int64_t ScaleFactor>
bool operator!=(T lhs, const snap_fp<T, ScaleFactor> &rhs) {
    return snap_fp<T, ScaleFactor>(lhs) != rhs;
}

template <class T, int64_t ScaleFactor>
bool operator<(T lhs, const snap_fp<T, ScaleFactor> &rhs) {
    return snap_fp<T, ScaleFactor>(lhs) < rhs;
}

template <class T, int64_t ScaleFactor>
bool operator<=(T lhs, const snap_fp<T, ScaleFactor> &rhs) {
    return snap_fp<T, ScaleFactor>(lhs) <= rhs;
}

template <class T, int64_t ScaleFactor>
bool operator>(T lhs, const snap_fp<T, ScaleFactor> &rhs) {
    return snap_fp<T, ScaleFactor>(lhs) > rhs;
}

template <class T, int64_t ScaleFactor>
bool operator>=(T lhs, const snap_fp<T, ScaleFactor> &rhs) {
    return snap_fp<T, ScaleFactor>(lhs) >= rhs;
}


//---------------------------------------------------------------------------------------------------------------------------
//-------------------------------- Common type aliases for convenience -----------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

// High precision (1e-9).
using snap_double_nano = snap_fp<double, 1000000000LL>;
// Note: use double for nanoscale precision; float does not have enough significant digits for a 1e9 scale.
using snap_float_nano = snap_fp<double, 1000000000LL>;

// Standard precision (1e-6).
using snap_double_micro = snap_fp<double, 1000000LL>;
// Note: we intentionally use double here to preserve micro-precision across
// a wider dynamic range; using float with a 1e6 scale can lose precision
// for |value| > ~100.
using snap_float_micro = snap_fp<double, 1000000LL>;

// Low precision (1e-3).
using snap_double_milli = snap_fp<double, 1000LL>;
using snap_float_milli = snap_fp<float, 1000LL>;

// Integer precision (1e0).
using snap_double_unit = snap_fp<double, 1LL>;
using snap_float_unit = snap_fp<float, 1LL>;

#endif // YGOR_SNAP_H_
