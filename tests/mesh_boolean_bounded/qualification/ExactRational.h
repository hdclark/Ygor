#pragma once

#include "ExactInteger.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::qualification {

class ExactRational {
public:
    ExactRational();
    ExactRational(std::int64_t value);
    ExactRational(ExactInteger numerator, ExactUnsignedInteger denominator);

    static ExactRational dyadic(ExactInteger significand, int exponent);

    const ExactInteger& numerator() const noexcept { return numerator_; }
    const ExactUnsignedInteger& denominator() const noexcept { return denominator_; }
    int sign() const noexcept { return numerator_.sign(); }
    bool is_zero() const noexcept { return numerator_.is_zero(); }
    int compare(const ExactRational& other) const;
    ExactRational abs() const;
    ExactRational operator-() const;
    std::vector<std::uint8_t> canonical_bytes() const;

    ExactRational& operator+=(const ExactRational& other);
    ExactRational& operator-=(const ExactRational& other);
    ExactRational& operator*=(const ExactRational& other);
    ExactRational& operator/=(const ExactRational& other);

    friend bool operator==(const ExactRational& a, const ExactRational& b) noexcept {
        return a.numerator_ == b.numerator_ && a.denominator_ == b.denominator_;
    }
    friend bool operator!=(const ExactRational& a, const ExactRational& b) noexcept { return !(a == b); }
    friend bool operator<(const ExactRational& a, const ExactRational& b) { return a.compare(b) < 0; }
    friend bool operator>(const ExactRational& a, const ExactRational& b) { return b < a; }
    friend bool operator<=(const ExactRational& a, const ExactRational& b) { return !(b < a); }
    friend bool operator>=(const ExactRational& a, const ExactRational& b) { return !(a < b); }

private:
    void normalize();
    ExactInteger numerator_;
    ExactUnsignedInteger denominator_{1};
};

ExactRational operator+(ExactRational a, const ExactRational& b);
ExactRational operator-(ExactRational a, const ExactRational& b);
ExactRational operator*(ExactRational a, const ExactRational& b);
ExactRational operator/(ExactRational a, const ExactRational& b);

} // namespace ygor::mesh_boolean::qualification
