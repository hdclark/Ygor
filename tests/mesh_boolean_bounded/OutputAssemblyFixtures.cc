#include "OutputAssemblyFixtures.h"

namespace assembly_tests {

std::string diagnostic(const bounded_boolean_error &error) {
  return std::string(error.summary) + " [subcode " +
         std::to_string(error.subcode) + ", checkpoint " +
         std::to_string(error.checkpoint) + "]";
}

assembly_fixture build_assembly_fixture(
    scalar x0, scalar y0, scalar z0, scalar x1, scalar y1, scalar z1,
    scalar X0, scalar Y0, scalar Z0, scalar X1, scalar Y1, scalar Z1,
    boolean_operation operation) {
  assembly_fixture fixture;
  fixture.cleanup = cleanup_tests::build_cleanup_fixture(
      x0, y0, z0, x1, y1, z1, X0, Y0, Z0, X1, Y1, Z1, operation);

  const auto &predecessor =
      fixture.cleanup.triangulation.topology.selection.classification.broad
          .predecessor;
  bounded::resource_manager resources(resource_policy::conservative_defaults());
  bounded::output_assembly_capabilities capabilities;
  capabilities.owner = predecessor.context.owner;
  capabilities.resources = &resources;

  auto output = bounded::assemble_output_candidate(
      predecessor.context, *predecessor.precision, fixture.cleanup.output,
      capabilities);
  if (!output.has_value())
    throw std::runtime_error("output assembly failed: " +
                             diagnostic(*output.error()));
  fixture.output = *output.value();
  return fixture;
}

} // namespace assembly_tests
