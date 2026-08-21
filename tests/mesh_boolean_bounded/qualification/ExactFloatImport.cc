#include "ExactFloatImport.h"

#include <cstring>
#include <limits>
#include <stdexcept>

namespace ygor::mesh_boolean::qualification {
namespace {
template<class Bits>
ExactFloatImportResult<Bits> decode(Bits bits, unsigned exponent_bits,
                                    unsigned fraction_bits, int exponent_bias) {
    const Bits fraction_mask = (Bits{1} << fraction_bits) - 1;
    const Bits exponent_mask = (Bits{1} << exponent_bits) - 1;
    const Bits fraction = bits & fraction_mask;
    const Bits encoded_exponent = (bits >> fraction_bits) & exponent_mask;
    const bool negative = (bits >> (fraction_bits + exponent_bits)) != 0;
    if (encoded_exponent == exponent_mask)
        throw std::domain_error("ExactFloatImport rejects NaN and infinity");

    ExactFloatImportResult<Bits> result;
    result.source_bits = bits;
    result.source_negative = negative;
    if (encoded_exponent == 0 && fraction == 0) {
        result.negative_zero = negative;
        return result;
    }

    std::uint64_t significand = static_cast<std::uint64_t>(fraction);
    int exponent = 1 - exponent_bias - static_cast<int>(fraction_bits);
    if (encoded_exponent != 0) {
        significand |= std::uint64_t{1} << fraction_bits;
        exponent = static_cast<int>(encoded_exponent) - exponent_bias -
                   static_cast<int>(fraction_bits);
    }
    result.value = ExactRational::dyadic(
        ExactInteger(negative ? -1 : 1, ExactUnsignedInteger(significand)), exponent);
    return result;
}
} // namespace

ExactFloatResult ExactFloatImport::from_float(float value) {
    static_assert(sizeof(float) == sizeof(std::uint32_t), "32-bit float required");
    static_assert(std::numeric_limits<float>::is_iec559 &&
                  std::numeric_limits<float>::radix == 2 &&
                  std::numeric_limits<float>::digits == 24 &&
                  std::numeric_limits<float>::max_exponent == 128,
                  "IEC 60559 binary32 float required");
    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return decode(bits, 8, 23, 127);
}

ExactDoubleResult ExactFloatImport::from_double(double value) {
    static_assert(sizeof(double) == sizeof(std::uint64_t), "64-bit double required");
    static_assert(std::numeric_limits<double>::is_iec559 &&
                  std::numeric_limits<double>::radix == 2 &&
                  std::numeric_limits<double>::digits == 53 &&
                  std::numeric_limits<double>::max_exponent == 1024,
                  "IEC 60559 binary64 double required");
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return decode(bits, 11, 52, 1023);
}

ExactFloatResult import_exact(float value) { return ExactFloatImport::from_float(value); }
ExactDoubleResult import_exact(double value) { return ExactFloatImport::from_double(value); }

} // namespace ygor::mesh_boolean::qualification
