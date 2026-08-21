#pragma once

#include "ExactUnsignedInteger.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::qualification {

class ExactInteger {
public:
    ExactInteger() = default;
    ExactInteger(std::int64_t value);
    ExactInteger(int sign, ExactUnsignedInteger magnitude);

    int sign() const noexcept { return sign_; }
    bool is_zero() const noexcept { return sign_ == 0; }
    const ExactUnsignedInteger& magnitude() const noexcept { return magnitude_; }
    ExactInteger abs() const;
    ExactInteger operator-() const;
    int compare(const ExactInteger& other) const noexcept;
    std::pair<ExactInteger, ExactInteger> div_mod(const ExactInteger& divisor) const;
    bool is_divisible_by(const ExactInteger& divisor) const;
    std::vector<std::uint8_t> canonical_bytes() const;

    ExactInteger& operator+=(const ExactInteger& other);
    ExactInteger& operator-=(const ExactInteger& other);
    ExactInteger& operator*=(const ExactInteger& other);

    friend bool operator==(const ExactInteger& a, const ExactInteger& b) noexcept {
        return a.sign_ == b.sign_ && a.magnitude_ == b.magnitude_;
    }
    friend bool operator!=(const ExactInteger& a, const ExactInteger& b) noexcept { return !(a == b); }
    friend bool operator<(const ExactInteger& a, const ExactInteger& b) noexcept { return a.compare(b) < 0; }
    friend bool operator>(const ExactInteger& a, const ExactInteger& b) noexcept { return b < a; }
    friend bool operator<=(const ExactInteger& a, const ExactInteger& b) noexcept { return !(b < a); }
    friend bool operator>=(const ExactInteger& a, const ExactInteger& b) noexcept { return !(a < b); }

private:
    void normalize() noexcept;
    int sign_ = 0;
    ExactUnsignedInteger magnitude_;
};

ExactInteger operator+(ExactInteger a, const ExactInteger& b);
ExactInteger operator-(ExactInteger a, const ExactInteger& b);
ExactInteger operator*(ExactInteger a, const ExactInteger& b);
ExactInteger operator/(const ExactInteger& a, const ExactInteger& b);
ExactInteger operator%(const ExactInteger& a, const ExactInteger& b);

} // namespace ygor::mesh_boolean::qualification
