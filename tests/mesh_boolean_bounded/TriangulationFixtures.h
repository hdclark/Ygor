#pragma once

#include "OutputTopologyFixtures.h"
#include "YgorMeshesBooleanBounded/OutputTriangulation.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace triangulation_tests {
namespace bounded = ygor::mesh_boolean::bounded;
using scalar = double;
using index_type = std::uint32_t;

using triangulated_type = bounded::triangulated_output_complex<scalar, index_type>;

inline void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

struct triangulation_fixture final {
  output_topology_tests::output_topology_fixture topology;
  std::shared_ptr<const triangulated_type> output;
};

triangulation_fixture build_triangulation_fixture(
    scalar x0, scalar y0, scalar z0, scalar x1, scalar y1, scalar z1,
    scalar X0, scalar Y0, scalar Z0, scalar X1, scalar Y1, scalar Z1,
    boolean_operation operation);

std::string diagnostic(const bounded_boolean_error &error);

} // namespace triangulation_tests
