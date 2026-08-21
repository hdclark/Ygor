#include "TriangulationFixtures.h"

namespace triangulation_tests {

std::string diagnostic(const bounded_boolean_error &error) {
  return std::string(error.summary) + " [subcode " +
         std::to_string(error.subcode) + ", checkpoint " +
         std::to_string(error.checkpoint) + "]";
}

triangulation_fixture build_triangulation_fixture(
    scalar x0, scalar y0, scalar z0, scalar x1, scalar y1, scalar z1,
    scalar X0, scalar Y0, scalar Z0, scalar X1, scalar Y1, scalar Z1,
    boolean_operation operation) {
  triangulation_fixture fixture;
  fixture.topology = output_topology_tests::build_output_topology_fixture(
      x0, y0, z0, x1, y1, z1, X0, Y0, Z0, X1, Y1, Z1, operation);

  const auto &predecessor =
      fixture.topology.selection.classification.broad.predecessor;
  bounded::resource_manager resources(resource_policy::conservative_defaults());
  bounded::output_triangulation_capabilities capabilities;
  capabilities.owner = predecessor.context.owner;
  capabilities.resources = &resources;

  auto output = bounded::build_triangulated_output_complex(
      predecessor.context, *predecessor.precision, predecessor.manifolds,
      fixture.topology.selection.classification.intersections,
      fixture.topology.selection.retained, fixture.topology.output,
      capabilities);
  if (!output.has_value())
    throw std::runtime_error("triangulation build failed: " +
                             diagnostic(*output.error()));
  fixture.output = *output.value();
  return fixture;
}

} // namespace triangulation_tests
