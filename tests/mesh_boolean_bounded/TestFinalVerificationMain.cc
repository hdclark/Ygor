#include "FinalVerificationFixtures.h"

#include <cstdint>
#include <iostream>
#include <string>

namespace bounded = ygor::mesh_boolean::bounded;
using namespace final_verification_tests;

namespace ygor::mesh_boolean::bounded {
struct output_assembly_artifact_test_access {
  template <class T, class I>
  static auto &mesh(assembled_output_candidate<T, I> &artifact) {
    return artifact.mesh_;
  }
};
} // namespace ygor::mesh_boolean::bounded

namespace {

using candidate_type = bounded::assembled_output_candidate<scalar, index_type>;

void test_contracts() {
  auto un = build_final_verification_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                             boolean_operation::set_union);
  const auto &artifact = *un.output;
  require(artifact.mesh().vertices.size() == 16,
          "verified mesh has sixteen vertices");
  require(artifact.mesh().faces.size() == 24,
          "verified mesh has twenty-four faces");
  require(artifact.topology_report().component_count == 2,
          "verified topology reports two components");
}

void test_empty() {
  auto inter = build_final_verification_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                                boolean_operation::intersection);
  const auto &artifact = *inter.output;
  require(artifact.mesh().vertices.empty(), "verified empty result");
  require(artifact.mesh().faces.empty(), "verified empty result has no faces");
}

void test_containment() {
  auto bma = build_final_verification_fixture(1, 1, 1, 2, 2, 2, 0, 0, 0, 3, 3, 3,
                                              boolean_operation::b_minus_a);
  const auto &artifact = *bma.output;
  require(artifact.mesh().faces.size() == 24,
          "verified B-A has twenty-four faces");
}

void test_canonical() {
  auto first = build_final_verification_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                                boolean_operation::set_union);
  auto second = build_final_verification_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                                 boolean_operation::set_union);
  require(first.output->digest() == second.output->digest(),
          "verified result digest is deterministic");
}

void test_rejects_corruption() {
  auto fixture = build_final_verification_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                                  boolean_operation::set_union);
  const auto &predecessor =
      fixture.assembly.cleanup.triangulation.topology.selection.classification.broad
          .predecessor;

  {
    // Collapse every vertex to the origin: every triangle becomes degenerate.
    candidate_type copy = *fixture.assembly.output;
    auto &mesh = bounded::output_assembly_artifact_test_access::mesh(copy);
    for (auto &vertex : mesh.vertices) {
      vertex.x = 0;
      vertex.y = 0;
      vertex.z = 0;
    }
    bounded::resource_manager resources(resource_policy::conservative_defaults());
    bounded::final_verification_capabilities capabilities;
    capabilities.owner = predecessor.context.owner;
    capabilities.resources = &resources;
    auto result = bounded::verify_and_publish(
        predecessor.context, *predecessor.precision,
        fixture.assembly.cleanup.output, std::make_shared<const candidate_type>(copy),
        capabilities);
    require(!result.has_value(), "final verification rejects degenerate triangles");
  }
}

} // namespace

int main(int argc, char **argv) {
  try {
    const std::string suite = argc > 1 ? argv[1] : "all";
    if (suite == "all" || suite == "contracts")
      test_contracts();
    if (suite == "all" || suite == "empty")
      test_empty();
    if (suite == "all" || suite == "containment")
      test_containment();
    if (suite == "all" || suite == "canonical")
      test_canonical();
    if (suite == "all" || suite == "mutation")
      test_rejects_corruption();
    std::cout << "Component 15 final verification suite passed: " << suite
              << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
