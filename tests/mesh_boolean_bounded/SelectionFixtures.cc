#include "SelectionFixtures.h"

#include <utility>

namespace selection_tests {

std::string diagnostic(const bounded_boolean_error &error) {
  return std::string(error.summary) + " [subcode " +
         std::to_string(error.subcode) + ", checkpoint " +
         std::to_string(error.checkpoint) + "]";
}

selection_fixture build_selection_fixture(scalar x0, scalar y0, scalar z0,
                                          scalar x1, scalar y1, scalar z1,
                                          scalar X0, scalar Y0, scalar Z0,
                                          scalar X1, scalar Y1, scalar Z1,
                                          boolean_operation operation) {
  selection_fixture fixture;
  fixture.classification = classification_tests::build_classification_fixture(
      broad_phase_tests::box(x0, y0, z0, x1, y1, z1),
      broad_phase_tests::box(X0, Y0, Z0, X1, Y1, Z1), operation);

  const auto &predecessor = fixture.classification.broad.predecessor;
  bounded::resource_manager resources(resource_policy::conservative_defaults());
  bounded::selection_capabilities capabilities;
  capabilities.owner = predecessor.context.owner;
  capabilities.resources = &resources;
  auto retained = bounded::build_retained_surface_complex(
      predecessor.context, *predecessor.precision, predecessor.manifolds,
      fixture.classification.relations, fixture.classification.intersections,
      fixture.classification.classification, capabilities);
  if (!retained.has_value())
    throw std::runtime_error("selection build failed: " +
                             diagnostic(*retained.error()));
  fixture.retained = *retained.value();
  return fixture;
}

std::uint64_t retained_count(const retained_type &artifact,
                             bounded::operand_id operand) {
  std::uint64_t count = 0;
  for (const auto &use : artifact.retained_uses())
    if (use.source_operand == operand)
      ++count;
  return count;
}

std::uint64_t reversed_count(const retained_type &artifact,
                             bounded::operand_id operand) {
  std::uint64_t count = 0;
  for (const auto &use : artifact.retained_uses())
    if (use.source_operand == operand && !use.preserve_source_orientation)
      ++count;
  return count;
}

} // namespace selection_tests
