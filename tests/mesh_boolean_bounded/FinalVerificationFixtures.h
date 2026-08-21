#pragma once

#include "OutputAssemblyFixtures.h"
#include "YgorMeshesBooleanBounded/FinalVerification.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace final_verification_tests {
namespace bounded = ygor::mesh_boolean::bounded;
using scalar = double;
using index_type = std::uint32_t;

using verified_type = bounded::verified_boolean_result<scalar, index_type>;

inline void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

struct final_verification_fixture final {
  assembly_tests::assembly_fixture assembly;
  std::shared_ptr<const verified_type> output;
};

final_verification_fixture build_final_verification_fixture(
    scalar x0, scalar y0, scalar z0, scalar x1, scalar y1, scalar z1,
    scalar X0, scalar Y0, scalar Z0, scalar X1, scalar Y1, scalar Z1,
    boolean_operation operation);

std::string diagnostic(const bounded_boolean_error &error);

} // namespace final_verification_tests
