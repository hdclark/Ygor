#pragma once

#include "TriangulationFixtures.h"
#include "YgorMeshesBooleanBounded/MeshCleanup.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace cleanup_tests {
namespace bounded = ygor::mesh_boolean::bounded;
using scalar = double;
using index_type = std::uint32_t;

using cleaned_type = bounded::cleaned_triangle_manifold<scalar>;

inline void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

struct cleanup_fixture final {
  triangulation_tests::triangulation_fixture triangulation;
  std::shared_ptr<const cleaned_type> output;
};

cleanup_fixture build_cleanup_fixture(
    scalar x0, scalar y0, scalar z0, scalar x1, scalar y1, scalar z1,
    scalar X0, scalar Y0, scalar Z0, scalar X1, scalar Y1, scalar Z1,
    boolean_operation operation);

std::string diagnostic(const bounded_boolean_error &error);

} // namespace cleanup_tests
