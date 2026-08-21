#pragma once

#include "BroadPhaseFixtures.h"
#include "YgorMeshesBooleanBounded/Classification.h"
#include "YgorMeshesBooleanBounded/IntersectionBuild.h"
#include "YgorMeshesBooleanBounded/RelationBuild.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace classification_tests {
namespace bounded = ygor::mesh_boolean::bounded;
using scalar = double;
using index_type = std::uint32_t;
using mesh_type = fv_surface_mesh<scalar, index_type>;

using relation_type = bounded::signed_feature_relations<scalar, index_type>;
using intersection_type = bounded::canonical_intersection_complex<scalar, index_type>;
using classification_type = bounded::classification_complex<scalar, index_type>;

inline void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

struct classification_fixture final {
  broad_phase_tests::built_fixture broad;
  std::shared_ptr<const relation_type> relations;
  std::shared_ptr<const intersection_type> intersections;
  std::shared_ptr<const classification_type> classification;
};

classification_fixture build_classification_fixture(
    const mesh_type &a, const mesh_type &b,
    boolean_operation operation = boolean_operation::intersection);

// A classification fixture built from two axis-aligned boxes.
classification_fixture box_fixture(scalar x0, scalar y0, scalar z0, scalar x1,
                                   scalar y1, scalar z1, scalar X0, scalar Y0,
                                   scalar Z0, scalar X1, scalar Y1, scalar Z1,
                                   boolean_operation operation = boolean_operation::intersection);

std::string diagnostic(const bounded_boolean_error &error);

} // namespace classification_tests
