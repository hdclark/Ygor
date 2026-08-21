#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::qualification {

class ExactUnsignedInteger {
public:
    using limb_type = std::uint32_t;

    ExactUnsignedInteger() = default;
    ExactUnsignedInteger(std::uint64_t value);

    static ExactUnsignedInteger from_limbs(std::vector<limb_type> limbs);

    bool is_zero() const noexcept { return limbs_.empty(); }
    std::size_t limb_count() const noexcept { return limbs_.size(); }
    std::size_t bit_length() const noexcept;
    const std::vector<limb_type>& limbs() const noexcept { return limbs_; }

    int compare(const ExactUnsignedInteger& other) const noexcept;
    ExactUnsignedInteger shifted_left(std::size_t bits) const;
    ExactUnsignedInteger shifted_right(std::size_t bits) const;
    std::pair<ExactUnsignedInteger, ExactUnsignedInteger>
    div_mod(const ExactUnsignedInteger& divisor) const;
    std::vector<std::uint8_t> canonical_bytes() const;

    ExactUnsignedInteger& operator+=(const ExactUnsignedInteger& other);
    ExactUnsignedInteger& operator-=(const ExactUnsignedInteger& other);
    ExactUnsignedInteger& operator*=(const ExactUnsignedInteger& other);
    ExactUnsignedInteger& operator<<=(std::size_t bits);
    ExactUnsignedInteger& operator>>=(std::size_t bits);

    friend bool operator==(const ExactUnsignedInteger& a,
                           const ExactUnsignedInteger& b) noexcept {
        return a.limbs_ == b.limbs_;
    }
    friend bool operator!=(const ExactUnsignedInteger& a,
                           const ExactUnsignedInteger& b) noexcept { return !(a == b); }
    friend bool operator<(const ExactUnsignedInteger& a,
                          const ExactUnsignedInteger& b) noexcept { return a.compare(b) < 0; }
    friend bool operator>(const ExactUnsignedInteger& a,
                          const ExactUnsignedInteger& b) noexcept { return b < a; }
    friend bool operator<=(const ExactUnsignedInteger& a,
                           const ExactUnsignedInteger& b) noexcept { return !(b < a); }
    friend bool operator>=(const ExactUnsignedInteger& a,
                           const ExactUnsignedInteger& b) noexcept { return !(a < b); }

private:
    explicit ExactUnsignedInteger(std::vector<limb_type> limbs, int);
    void normalize() noexcept;

    std::vector<limb_type> limbs_;
};

ExactUnsignedInteger operator+(ExactUnsignedInteger a, const ExactUnsignedInteger& b);
ExactUnsignedInteger operator-(ExactUnsignedInteger a, const ExactUnsignedInteger& b);
ExactUnsignedInteger operator*(ExactUnsignedInteger a, const ExactUnsignedInteger& b);
ExactUnsignedInteger operator/(const ExactUnsignedInteger& a, const ExactUnsignedInteger& b);
ExactUnsignedInteger operator%(const ExactUnsignedInteger& a, const ExactUnsignedInteger& b);
ExactUnsignedInteger operator<<(ExactUnsignedInteger a, std::size_t bits);
ExactUnsignedInteger operator>>(ExactUnsignedInteger a, std::size_t bits);
ExactUnsignedInteger gcd(ExactUnsignedInteger a, ExactUnsignedInteger b);

} // namespace ygor::mesh_boolean::qualification
