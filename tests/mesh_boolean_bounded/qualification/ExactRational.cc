#include "ExactRational.h"

#include <limits>
#include <stdexcept>

namespace ygor::mesh_boolean::qualification {

ExactRational::ExactRational() = default;
ExactRational::ExactRational(std::int64_t value) : numerator_(value) {}

ExactRational::ExactRational(ExactInteger numerator, ExactUnsignedInteger denominator)
    : numerator_(std::move(numerator)), denominator_(std::move(denominator)) {
    normalize();
}

void ExactRational::normalize() {
    if (denominator_.is_zero()) throw std::domain_error("ExactRational zero denominator");
    if (numerator_.is_zero()) {
        denominator_ = ExactUnsignedInteger(1);
        return;
    }
    const auto divisor = gcd(numerator_.magnitude(), denominator_);
    numerator_ = ExactInteger(numerator_.sign(), numerator_.magnitude() / divisor);
    denominator_ = denominator_ / divisor;
}

ExactRational ExactRational::dyadic(ExactInteger significand, int exponent) {
    if (significand.is_zero()) return {};
    if (exponent >= 0) {
        return ExactRational(ExactInteger(significand.sign(),
            significand.magnitude().shifted_left(static_cast<std::size_t>(exponent))), ExactUnsignedInteger(1));
    }
    const std::int64_t magnitude = -static_cast<std::int64_t>(exponent);
    if (static_cast<std::uint64_t>(magnitude) > std::numeric_limits<std::size_t>::max())
        throw std::length_error("ExactRational dyadic exponent limit");
    return ExactRational(std::move(significand),
                         ExactUnsignedInteger(1).shifted_left(static_cast<std::size_t>(magnitude)));
}

int ExactRational::compare(const ExactRational& other) const {
    return (numerator_ * ExactInteger(1, other.denominator_)).compare(
        other.numerator_ * ExactInteger(1, denominator_));
}

ExactRational ExactRational::abs() const { return ExactRational(numerator_.abs(), denominator_); }
ExactRational ExactRational::operator-() const { return ExactRational(-numerator_, denominator_); }

ExactRational& ExactRational::operator+=(const ExactRational& other) {
    const auto common = gcd(denominator_, other.denominator_);
    const auto left_factor = other.denominator_ / common;
    const auto right_factor = denominator_ / common;
    numerator_ = numerator_ * ExactInteger(1, left_factor) +
                 other.numerator_ * ExactInteger(1, right_factor);
    denominator_ = denominator_ * left_factor;
    normalize();
    return *this;
}

ExactRational& ExactRational::operator-=(const ExactRational& other) { return *this += -other; }

ExactRational& ExactRational::operator*=(const ExactRational& other) {
    if (is_zero() || other.is_zero()) {
        numerator_ = ExactInteger{};
        denominator_ = ExactUnsignedInteger(1);
        return *this;
    }
    const auto g1 = gcd(numerator_.magnitude(), other.denominator_);
    const auto g2 = gcd(other.numerator_.magnitude(), denominator_);
    const ExactInteger left(numerator_.sign(), numerator_.magnitude() / g1);
    const ExactInteger right(other.numerator_.sign(), other.numerator_.magnitude() / g2);
    numerator_ = left * right;
    denominator_ = (denominator_ / g2) * (other.denominator_ / g1);
    return *this;
}

ExactRational& ExactRational::operator/=(const ExactRational& other) {
    if (other.is_zero()) throw std::domain_error("ExactRational division by zero");
    const auto g1 = gcd(numerator_.magnitude(), other.numerator_.magnitude());
    const auto g2 = gcd(other.denominator_, denominator_);
    ExactInteger result(numerator_.sign() * other.numerator_.sign(),
                        (numerator_.magnitude() / g1) * (other.denominator_ / g2));
    denominator_ = (denominator_ / g2) * (other.numerator_.magnitude() / g1);
    numerator_ = std::move(result);
    normalize();
    return *this;
}

std::vector<std::uint8_t> ExactRational::canonical_bytes() const {
    auto result = numerator_.canonical_bytes();
    const auto denominator_bytes = denominator_.canonical_bytes();
    result.insert(result.end(), denominator_bytes.begin(), denominator_bytes.end());
    return result;
}

ExactRational operator+(ExactRational a, const ExactRational& b) { return a += b; }
ExactRational operator-(ExactRational a, const ExactRational& b) { return a -= b; }
ExactRational operator*(ExactRational a, const ExactRational& b) { return a *= b; }
ExactRational operator/(ExactRational a, const ExactRational& b) { return a /= b; }

} // namespace ygor::mesh_boolean::qualification
