#include "CleanupFixtures.h"

namespace cleanup_tests {

std::string diagnostic(const bounded_boolean_error &error) {
  return std::string(error.summary) + " [subcode " +
         std::to_string(error.subcode) + ", checkpoint " +
         std::to_string(error.checkpoint) + "]";
}

cleanup_fixture build_cleanup_fixture(
    scalar x0, scalar y0, scalar z0, scalar x1, scalar y1, scalar z1,
    scalar X0, scalar Y0, scalar Z0, scalar X1, scalar Y1, scalar Z1,
    boolean_operation operation) {
  cleanup_fixture fixture;
  fixture.triangulation = triangulation_tests::build_triangulation_fixture(
      x0, y0, z0, x1, y1, z1, X0, Y0, Z0, X1, Y1, Z1, operation);

  const auto &predecessor =
      fixture.triangulation.topology.selection.classification.broad.predecessor;
  bounded::resource_manager resources(resource_policy::conservative_defaults());
  bounded::cleanup_capabilities capabilities;
  capabilities.owner = predecessor.context.owner;
  capabilities.resources = &resources;

  auto output = bounded::build_cleaned_triangle_manifold(
      predecessor.context, *predecessor.precision, predecessor.manifolds,
      fixture.triangulation.topology.selection.classification.intersections,
      fixture.triangulation.topology.selection.retained,
      fixture.triangulation.topology.output, fixture.triangulation.output,
      capabilities);
  if (!output.has_value())
    throw std::runtime_error("cleanup build failed: " +
                             diagnostic(*output.error()));
  fixture.output = *output.value();
  return fixture;
}

} // namespace cleanup_tests
