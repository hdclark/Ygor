#include "ExactUnsignedInteger.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace ygor::mesh_boolean::qualification {
namespace {
constexpr std::uint64_t limb_base = std::uint64_t{1} << 32;

unsigned leading_zeroes(std::uint32_t value) noexcept {
    unsigned count = 0;
    if (value == 0) return 32;
    while ((value & 0x80000000U) == 0) {
        value <<= 1;
        ++count;
    }
    return count;
}
} // namespace

ExactUnsignedInteger::ExactUnsignedInteger(std::uint64_t value) {
    if (value != 0) {
        limbs_.push_back(static_cast<limb_type>(value));
        const auto high = static_cast<limb_type>(value >> 32);
        if (high != 0) limbs_.push_back(high);
    }
}

ExactUnsignedInteger::ExactUnsignedInteger(std::vector<limb_type> limbs, int)
    : limbs_(std::move(limbs)) {
    normalize();
}

ExactUnsignedInteger ExactUnsignedInteger::from_limbs(std::vector<limb_type> limbs) {
    return ExactUnsignedInteger(std::move(limbs), 0);
}

void ExactUnsignedInteger::normalize() noexcept {
    while (!limbs_.empty() && limbs_.back() == 0) limbs_.pop_back();
}

std::size_t ExactUnsignedInteger::bit_length() const noexcept {
    if (is_zero()) return 0;
    return (limbs_.size() - 1) * 32 + (32 - leading_zeroes(limbs_.back()));
}

int ExactUnsignedInteger::compare(const ExactUnsignedInteger& other) const noexcept {
    if (limbs_.size() != other.limbs_.size())
        return limbs_.size() < other.limbs_.size() ? -1 : 1;
    for (std::size_t i = limbs_.size(); i-- > 0;) {
        if (limbs_[i] != other.limbs_[i]) return limbs_[i] < other.limbs_[i] ? -1 : 1;
    }
    return 0;
}

ExactUnsignedInteger& ExactUnsignedInteger::operator+=(const ExactUnsignedInteger& other) {
    const std::size_t n = std::max(limbs_.size(), other.limbs_.size());
    limbs_.resize(n, 0);
    std::uint64_t carry = 0;
    for (std::size_t i = 0; i < n; ++i) {
        const std::uint64_t sum = std::uint64_t(limbs_[i]) +
                                  (i < other.limbs_.size() ? other.limbs_[i] : 0) + carry;
        limbs_[i] = static_cast<limb_type>(sum);
        carry = sum >> 32;
    }
    if (carry != 0) limbs_.push_back(static_cast<limb_type>(carry));
    return *this;
}

ExactUnsignedInteger& ExactUnsignedInteger::operator-=(const ExactUnsignedInteger& other) {
    if (*this < other) throw std::domain_error("ExactUnsignedInteger subtraction underflow");
    std::uint64_t borrow = 0;
    for (std::size_t i = 0; i < limbs_.size(); ++i) {
        const std::uint64_t sub = (i < other.limbs_.size() ? other.limbs_[i] : 0) + borrow;
        const std::uint64_t current = limbs_[i];
        limbs_[i] = static_cast<limb_type>(current - sub);
        borrow = current < sub ? 1 : 0;
    }
    normalize();
    return *this;
}

ExactUnsignedInteger& ExactUnsignedInteger::operator*=(const ExactUnsignedInteger& other) {
    if (is_zero() || other.is_zero()) {
        limbs_.clear();
        return *this;
    }
    if (other.limbs_.size() > std::numeric_limits<std::size_t>::max() - limbs_.size())
        throw std::length_error("ExactUnsignedInteger limb limit");
    std::vector<limb_type> product(limbs_.size() + other.limbs_.size(), 0);
    for (std::size_t i = 0; i < limbs_.size(); ++i) {
        std::uint64_t carry = 0;
        for (std::size_t j = 0; j < other.limbs_.size(); ++j) {
            const std::uint64_t value = std::uint64_t(limbs_[i]) * other.limbs_[j] +
                                        product[i + j] + carry;
            product[i + j] = static_cast<limb_type>(value);
            carry = value >> 32;
        }
        std::size_t k = i + other.limbs_.size();
        while (carry != 0) {
            const std::uint64_t value = std::uint64_t(product[k]) + carry;
            product[k] = static_cast<limb_type>(value);
            carry = value >> 32;
            ++k;
        }
    }
    limbs_ = std::move(product);
    normalize();
    return *this;
}

ExactUnsignedInteger ExactUnsignedInteger::shifted_left(std::size_t bits) const {
    if (is_zero()) return {};
    const std::size_t words = bits / 32;
    if (words > std::numeric_limits<std::size_t>::max() - limbs_.size() - 1)
        throw std::length_error("ExactUnsignedInteger shift limit");
    std::vector<limb_type> result(words + limbs_.size() + 1, 0);
    const unsigned shift = static_cast<unsigned>(bits % 32);
    std::uint64_t carry = 0;
    for (std::size_t i = 0; i < limbs_.size(); ++i) {
        const std::uint64_t value = (std::uint64_t(limbs_[i]) << shift) | carry;
        result[words + i] = static_cast<limb_type>(value);
        carry = value >> 32;
    }
    result[words + limbs_.size()] = static_cast<limb_type>(carry);
    return from_limbs(std::move(result));
}

ExactUnsignedInteger ExactUnsignedInteger::shifted_right(std::size_t bits) const {
    const std::size_t words = bits / 32;
    if (words >= limbs_.size()) return {};
    const unsigned shift = static_cast<unsigned>(bits % 32);
    std::vector<limb_type> result(limbs_.size() - words, 0);
    std::uint32_t carry = 0;
    for (std::size_t i = limbs_.size(); i-- > words;) {
        const std::uint32_t value = limbs_[i];
        result[i - words] = shift == 0 ? value :
            static_cast<std::uint32_t>((value >> shift) | (std::uint64_t(carry) << (32 - shift)));
        carry = shift == 0 ? 0 : static_cast<std::uint32_t>(value & ((std::uint64_t{1} << shift) - 1));
    }
    return from_limbs(std::move(result));
}

ExactUnsignedInteger& ExactUnsignedInteger::operator<<=(std::size_t bits) {
    *this = shifted_left(bits);
    return *this;
}

ExactUnsignedInteger& ExactUnsignedInteger::operator>>=(std::size_t bits) {
    *this = shifted_right(bits);
    return *this;
}

std::pair<ExactUnsignedInteger, ExactUnsignedInteger>
ExactUnsignedInteger::div_mod(const ExactUnsignedInteger& divisor) const {
    if (divisor.is_zero()) throw std::domain_error("ExactUnsignedInteger division by zero");
    if (*this < divisor) return {ExactUnsignedInteger{}, *this};
    if (divisor.limbs_.size() == 1) {
        std::vector<limb_type> quotient(limbs_.size(), 0);
        std::uint64_t remainder = 0;
        for (std::size_t i = limbs_.size(); i-- > 0;) {
            const std::uint64_t current = (remainder << 32) | limbs_[i];
            quotient[i] = static_cast<limb_type>(current / divisor.limbs_[0]);
            remainder = current % divisor.limbs_[0];
        }
        return {from_limbs(std::move(quotient)), ExactUnsignedInteger(remainder)};
    }

    // Normalized Knuth Algorithm D. The extra dividend limb makes quotient correction bounded.
    const unsigned shift = leading_zeroes(divisor.limbs_.back());
    auto v = divisor.shifted_left(shift).limbs_;
    auto u = shifted_left(shift).limbs_;
    u.push_back(0);
    const std::size_t n = v.size();
    const std::size_t m = u.size() - n - 1;
    std::vector<limb_type> q(m + 1, 0);
    for (std::size_t jj = m + 1; jj-- > 0;) {
        const std::size_t j = jj;
        const std::uint64_t top = (std::uint64_t(u[j + n]) << 32) | u[j + n - 1];
        std::uint64_t qhat = top / v[n - 1];
        std::uint64_t rhat = top % v[n - 1];
        if (qhat == limb_base) {
            --qhat;
            rhat += v[n - 1];
        }
        while (rhat < limb_base && qhat * v[n - 2] > (rhat << 32) + u[j + n - 2]) {
            --qhat;
            rhat += v[n - 1];
        }

        std::uint64_t carry = 0;
        std::uint64_t borrow = 0;
        for (std::size_t i = 0; i < n; ++i) {
            const std::uint64_t product = qhat * v[i] + carry;
            carry = product >> 32;
            const std::uint64_t sub = std::uint64_t(static_cast<limb_type>(product)) + borrow;
            const std::uint64_t current = u[j + i];
            u[j + i] = static_cast<limb_type>(current - sub);
            borrow = current < sub ? 1 : 0;
        }
        const std::uint64_t sub = carry + borrow;
        const bool negative = std::uint64_t(u[j + n]) < sub;
        u[j + n] = static_cast<limb_type>(std::uint64_t(u[j + n]) - sub);
        if (negative) {
            --qhat;
            std::uint64_t add_carry = 0;
            for (std::size_t i = 0; i < n; ++i) {
                const std::uint64_t sum = std::uint64_t(u[j + i]) + v[i] + add_carry;
                u[j + i] = static_cast<limb_type>(sum);
                add_carry = sum >> 32;
            }
            u[j + n] = static_cast<limb_type>(std::uint64_t(u[j + n]) + add_carry);
        }
        q[j] = static_cast<limb_type>(qhat);
    }
    u.resize(n);
    return {from_limbs(std::move(q)), from_limbs(std::move(u)).shifted_right(shift)};
}

std::vector<std::uint8_t> ExactUnsignedInteger::canonical_bytes() const {
    std::vector<std::uint8_t> result;
    result.reserve(8 + limbs_.size() * 4);
    const std::uint64_t count = limbs_.size();
    for (unsigned i = 0; i < 8; ++i) result.push_back(static_cast<std::uint8_t>(count >> (8 * i)));
    for (const auto limb : limbs_)
        for (unsigned i = 0; i < 4; ++i) result.push_back(static_cast<std::uint8_t>(limb >> (8 * i)));
    return result;
}

ExactUnsignedInteger operator+(ExactUnsignedInteger a, const ExactUnsignedInteger& b) { return a += b; }
ExactUnsignedInteger operator-(ExactUnsignedInteger a, const ExactUnsignedInteger& b) { return a -= b; }
ExactUnsignedInteger operator*(ExactUnsignedInteger a, const ExactUnsignedInteger& b) { return a *= b; }
ExactUnsignedInteger operator/(const ExactUnsignedInteger& a, const ExactUnsignedInteger& b) { return a.div_mod(b).first; }
ExactUnsignedInteger operator%(const ExactUnsignedInteger& a, const ExactUnsignedInteger& b) { return a.div_mod(b).second; }
ExactUnsignedInteger operator<<(ExactUnsignedInteger a, std::size_t bits) { return a <<= bits; }
ExactUnsignedInteger operator>>(ExactUnsignedInteger a, std::size_t bits) { return a >>= bits; }

ExactUnsignedInteger gcd(ExactUnsignedInteger a, ExactUnsignedInteger b) {
    while (!b.is_zero()) {
        auto remainder = a % b;
        a = std::move(b);
        b = std::move(remainder);
    }
    return a;
}

} // namespace ygor::mesh_boolean::qualification
