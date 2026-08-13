#pragma once

#include "SelectionFixtures.h"
#include "YgorMeshesBooleanBounded/OutputTopology.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace output_topology_tests {
namespace bounded = ygor::mesh_boolean::bounded;
using scalar = double;
using index_type = std::uint32_t;

using output_type = bounded::polygonal_output_complex<scalar, index_type>;

inline void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

struct output_topology_fixture final {
  selection_tests::selection_fixture selection;
  std::shared_ptr<const output_type> output;
};

output_topology_fixture build_output_topology_fixture(
    scalar x0, scalar y0, scalar z0, scalar x1, scalar y1, scalar z1,
    scalar X0, scalar Y0, scalar Z0, scalar X1, scalar Y1, scalar Z1,
    boolean_operation operation);

std::string diagnostic(const bounded_boolean_error &error);

} // namespace output_topology_tests
