#include "ExactInteger.h"

#include <stdexcept>

namespace ygor::mesh_boolean::qualification {

ExactInteger::ExactInteger(std::int64_t value) {
    if (value < 0) {
        sign_ = -1;
        magnitude_ = ExactUnsignedInteger(std::uint64_t(-(value + 1)) + 1);
    } else if (value > 0) {
        sign_ = 1;
        magnitude_ = ExactUnsignedInteger(static_cast<std::uint64_t>(value));
    }
}

ExactInteger::ExactInteger(int sign, ExactUnsignedInteger magnitude)
    : sign_(sign < 0 ? -1 : sign > 0 ? 1 : 0), magnitude_(std::move(magnitude)) {
    normalize();
}

void ExactInteger::normalize() noexcept {
    if (magnitude_.is_zero()) sign_ = 0;
    else if (sign_ == 0) sign_ = 1;
}

ExactInteger ExactInteger::abs() const { return ExactInteger(is_zero() ? 0 : 1, magnitude_); }
ExactInteger ExactInteger::operator-() const { return ExactInteger(-sign_, magnitude_); }

int ExactInteger::compare(const ExactInteger& other) const noexcept {
    if (sign_ != other.sign_) return sign_ < other.sign_ ? -1 : 1;
    if (sign_ == 0) return 0;
    const int magnitude_order = magnitude_.compare(other.magnitude_);
    return sign_ > 0 ? magnitude_order : -magnitude_order;
}

ExactInteger& ExactInteger::operator+=(const ExactInteger& other) {
    if (other.sign_ == 0) return *this;
    if (sign_ == 0) {
        *this = other;
        return *this;
    }
    if (sign_ == other.sign_) {
        magnitude_ += other.magnitude_;
    } else {
        const int order = magnitude_.compare(other.magnitude_);
        if (order == 0) {
            sign_ = 0;
            magnitude_ = {};
        } else if (order > 0) {
            magnitude_ -= other.magnitude_;
        } else {
            magnitude_ = other.magnitude_ - magnitude_;
            sign_ = other.sign_;
        }
    }
    normalize();
    return *this;
}

ExactInteger& ExactInteger::operator-=(const ExactInteger& other) { return *this += -other; }

ExactInteger& ExactInteger::operator*=(const ExactInteger& other) {
    const int result_sign = sign_ * other.sign_;
    magnitude_ *= other.magnitude_;
    sign_ = result_sign;
    normalize();
    return *this;
}

std::pair<ExactInteger, ExactInteger> ExactInteger::div_mod(const ExactInteger& divisor) const {
    if (divisor.is_zero()) throw std::domain_error("ExactInteger division by zero");
    auto qr = magnitude_.div_mod(divisor.magnitude_);
    return {ExactInteger(sign_ * divisor.sign_, std::move(qr.first)),
            ExactInteger(sign_, std::move(qr.second))};
}

bool ExactInteger::is_divisible_by(const ExactInteger& divisor) const {
    return div_mod(divisor).second.is_zero();
}

std::vector<std::uint8_t> ExactInteger::canonical_bytes() const {
    auto result = magnitude_.canonical_bytes();
    result.insert(result.begin(), static_cast<std::uint8_t>(sign_ + 1));
    return result;
}

ExactInteger operator+(ExactInteger a, const ExactInteger& b) { return a += b; }
ExactInteger operator-(ExactInteger a, const ExactInteger& b) { return a -= b; }
ExactInteger operator*(ExactInteger a, const ExactInteger& b) { return a *= b; }
ExactInteger operator/(const ExactInteger& a, const ExactInteger& b) { return a.div_mod(b).first; }
ExactInteger operator%(const ExactInteger& a, const ExactInteger& b) { return a.div_mod(b).second; }

} // namespace ygor::mesh_boolean::qualification
