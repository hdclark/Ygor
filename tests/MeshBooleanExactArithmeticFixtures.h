#pragma once

#include "MeshBooleanExactKernelFixtures.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace exact_arithmetic_test {

using namespace ygor::mesh_boolean;

constexpr std::size_t reference_bit_limit = 4096;

inline big_uint from_hex(const std::string &text) {
  auto value = big_uint::from_hex(text);
  if (!value.has_value())
    throw std::runtime_error("invalid exact-arithmetic fixture");
  return std::move(value.value());
}

inline big_uint patterned_operand(std::size_t limbs, std::uint32_t salt = 0) {
  if (limbs == 0)
    return big_uint();
  static constexpr char digits[] = "0123456789abcdef";
  std::string text;
  text.reserve(limbs * 8);
  for (std::size_t i = 0; i < limbs * 8; ++i) {
    const auto x = static_cast<std::uint32_t>(i * 13 + salt * 7 + i / 3);
    text.push_back(digits[(x ^ (x >> 3)) & 15U]);
  }
  text.front() = digits[8U + (salt & 7U)];
  return from_hex(text);
}

// This is intentionally the old, simple shift/subtract algorithm. Keep it
// bounded: it is an independent test oracle, never a production fallback.
inline std::pair<big_uint, big_uint> reference_divide(const big_uint &dividend,
                                                     const big_uint &divisor) {
  if (divisor.is_zero())
    throw std::invalid_argument("reference division by zero");
  if (dividend.bit_length() > reference_bit_limit ||
      divisor.bit_length() > reference_bit_limit)
    throw std::invalid_argument("reference division operand exceeds bound");
  if (dividend < divisor)
    return {big_uint(), dividend};
  big_uint quotient, remainder;
  for (std::size_t bit = dividend.bit_length(); bit--;) {
    remainder = remainder.shifted_left(1);
    const auto shifted = dividend.shifted_right(bit);
    const auto bytes = shifted.canonical_bytes();
    if (!bytes.empty() && (bytes.back() & 1U) != 0)
      remainder = remainder + big_uint(1);
    if (remainder.compare(divisor) >= 0) {
      remainder = remainder - divisor;
      quotient = quotient + big_uint(1).shifted_left(bit);
    }
  }
  return {quotient, remainder};
}

inline big_uint reference_gcd(big_uint a, big_uint b) {
  while (!b.is_zero()) {
    auto remainder = reference_divide(a, b).second;
    a = std::move(b);
    b = std::move(remainder);
  }
  return a;
}

struct small_rational {
  std::int64_t numerator = 0;
  std::uint64_t denominator = 1;
};

inline std::uint64_t magnitude(std::int64_t value) {
  return value < 0 ? std::uint64_t(-(value + 1)) + 1U
                   : static_cast<std::uint64_t>(value);
}

inline small_rational normalized(small_rational value) {
  if (value.denominator == 0)
    throw std::invalid_argument("small rational zero denominator");
  if (value.numerator == 0)
    return {};
  const auto common = std::gcd(magnitude(value.numerator), value.denominator);
  value.numerator /= static_cast<std::int64_t>(common);
  value.denominator /= common;
  return value;
}

inline std::uint64_t checked_u64_multiply(std::uint64_t a, std::uint64_t b) {
  if (b != 0 && a > std::numeric_limits<std::uint64_t>::max() / b)
    throw std::overflow_error("small rational oracle overflow");
  return a * b;
}

inline std::int64_t signed_magnitude(std::uint64_t value, bool negative) {
  const auto negative_limit = std::uint64_t(std::numeric_limits<std::int64_t>::max()) + 1U;
  if (value > (negative ? negative_limit
                        : std::uint64_t(std::numeric_limits<std::int64_t>::max())))
    throw std::overflow_error("small rational oracle overflow");
  if (!negative)
    return static_cast<std::int64_t>(value);
  return value == negative_limit ? std::numeric_limits<std::int64_t>::min()
                                 : -static_cast<std::int64_t>(value);
}

inline std::int64_t checked_multiply(std::int64_t a, std::uint64_t b) {
  return signed_magnitude(checked_u64_multiply(magnitude(a), b), a < 0);
}

inline std::int64_t checked_add(std::int64_t a, std::int64_t b) {
  if ((b > 0 && a > std::numeric_limits<std::int64_t>::max() - b) ||
      (b < 0 && a < std::numeric_limits<std::int64_t>::min() - b))
    throw std::overflow_error("small rational oracle overflow");
  return a + b;
}

inline small_rational add(small_rational a, small_rational b) {
  a = normalized(a);
  b = normalized(b);
  return normalized({checked_add(checked_multiply(a.numerator, b.denominator),
                                 checked_multiply(b.numerator, a.denominator)),
                     checked_u64_multiply(a.denominator, b.denominator)});
}

inline small_rational multiply(small_rational a, small_rational b) {
  a = normalized(a);
  b = normalized(b);
  const bool negative = (a.numerator < 0) != (b.numerator < 0);
  const auto product = checked_u64_multiply(magnitude(a.numerator),
                                            magnitude(b.numerator));
  if (product > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
    throw std::overflow_error("small rational oracle overflow");
  return normalized({negative ? -static_cast<std::int64_t>(product)
                              : static_cast<std::int64_t>(product),
                     checked_u64_multiply(a.denominator, b.denominator)});
}

inline small_rational divide(small_rational a, small_rational b) {
  a = normalized(a);
  b = normalized(b);
  if (b.numerator == 0)
    throw std::invalid_argument("small rational division by zero");
  return multiply(a, {b.numerator < 0 ? -static_cast<std::int64_t>(b.denominator)
                                      : static_cast<std::int64_t>(b.denominator),
                      magnitude(b.numerator)});
}

inline exact_rational exact(small_rational value) {
  value = normalized(value);
  return exact_rational(big_int(value.numerator), big_uint(value.denominator));
}

inline std::uint64_t decode_u64(const std::vector<std::uint8_t> &bytes,
                                std::size_t &offset) {
  if (bytes.size() - offset < 8)
    throw std::invalid_argument("truncated canonical integer length");
  std::uint64_t value = 0;
  for (unsigned i = 0; i < 8; ++i)
    value = (value << 8) | bytes[offset++];
  return value;
}

inline big_uint decode_unsigned(const std::vector<std::uint8_t> &bytes,
                                std::size_t &offset) {
  const auto size = decode_u64(bytes, offset);
  if (size > bytes.size() - offset)
    throw std::invalid_argument("truncated canonical integer");
  if (size == 0)
    return big_uint();
  static constexpr char digits[] = "0123456789abcdef";
  std::string hex;
  hex.reserve(static_cast<std::size_t>(size) * 2);
  for (std::uint64_t i = 0; i < size; ++i) {
    const auto byte = bytes[offset++];
    hex.push_back(digits[byte >> 4]);
    hex.push_back(digits[byte & 15U]);
  }
  return from_hex(hex);
}

inline exact_rational decode_rational(const std::vector<std::uint8_t> &bytes) {
  if (bytes.empty())
    throw std::invalid_argument("empty canonical rational");
  std::size_t offset = 0;
  const auto sign_byte = bytes[offset++];
  const auto magnitude_value = decode_unsigned(bytes, offset);
  integer_sign sign;
  if (sign_byte == 0)
    sign = integer_sign::zero;
  else if (sign_byte == 1)
    sign = integer_sign::positive;
  else if (sign_byte == 255)
    sign = integer_sign::negative;
  else
    throw std::invalid_argument("invalid canonical integer sign");
  const auto denominator = decode_unsigned(bytes, offset);
  if (offset != bytes.size())
    throw std::invalid_argument("trailing canonical rational bytes");
  return exact_rational(big_int(sign, magnitude_value), denominator);
}

inline std::uint64_t checksum_bytes(std::uint64_t checksum,
                                    const std::vector<std::uint8_t> &bytes) {
  for (const auto byte : bytes)
    checksum = (checksum ^ byte) * 1099511628211ULL;
  return checksum;
}

} // namespace exact_arithmetic_test
