#pragma once

#include "ClassificationFixtures.h"
#include "YgorMeshesBooleanBounded/Selection.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace selection_tests {
namespace bounded = ygor::mesh_boolean::bounded;
using scalar = double;
using index_type = std::uint32_t;
using mesh_type = fv_surface_mesh<scalar, index_type>;

using classification_type = bounded::classification_complex<scalar, index_type>;
using retained_type = bounded::retained_surface_complex<scalar, index_type>;

inline void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

struct selection_fixture final {
  classification_tests::classification_fixture classification;
  std::shared_ptr<const retained_type> retained;
};

// Build a retained-surface complex from two axis-aligned boxes.
selection_fixture build_selection_fixture(scalar x0, scalar y0, scalar z0,
                                          scalar x1, scalar y1, scalar z1,
                                          scalar X0, scalar Y0, scalar Z0,
                                          scalar X1, scalar Y1, scalar Z1,
                                          boolean_operation operation);

// Count retained uses from a given source operand.
std::uint64_t retained_count(const retained_type &artifact,
                             bounded::operand_id operand);

// Count reversed retained uses from a given source operand.
std::uint64_t reversed_count(const retained_type &artifact,
                             bounded::operand_id operand);

std::string diagnostic(const bounded_boolean_error &error);

} // namespace selection_tests
