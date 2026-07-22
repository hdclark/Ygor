#pragma once

#include "ExactRational.h"

#include <cstdint>

namespace ygor::mesh_boolean::qualification {

template<class Bits>
struct ExactFloatImportResult {
    ExactRational value;
    Bits source_bits = 0;
    bool negative_zero = false;
    bool source_negative = false;

    bool is_zero() const noexcept { return value.is_zero(); }
};

using ExactFloatResult = ExactFloatImportResult<std::uint32_t>;
using ExactDoubleResult = ExactFloatImportResult<std::uint64_t>;

class ExactFloatImport {
public:
    static ExactFloatResult from_float(float value);
    static ExactDoubleResult from_double(double value);
};

ExactFloatResult import_exact(float value);
ExactDoubleResult import_exact(double value);

} // namespace ygor::mesh_boolean::qualification
