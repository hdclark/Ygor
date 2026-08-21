#include "OutputAssemblyFixtures.h"
#include "YgorMeshesBooleanBounded/OutputAssemblyVerifier.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace bounded = ygor::mesh_boolean::bounded;
using namespace assembly_tests;

namespace ygor::mesh_boolean::bounded {
struct output_assembly_artifact_test_access {
  template <class T, class I>
  static auto &mesh(assembled_output_candidate<T, I> &artifact) {
    return artifact.mesh_;
  }
  template <class T, class I>
  static auto &digest(assembled_output_candidate<T, I> &artifact) {
    return artifact.digest_;
  }
};
} // namespace ygor::mesh_boolean::bounded

namespace {

using candidate_type = bounded::assembled_output_candidate<scalar, index_type>;

void test_contracts() {
  // Disjoint union of two unit boxes: 16 vertices, 24 triangular faces.
  auto un = build_assembly_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                   boolean_operation::set_union);
  const auto &artifact = *un.output;
  require(artifact.mesh().vertices.size() == 16,
          "assembled mesh has sixteen vertices");
  require(artifact.mesh().faces.size() == 24,
          "assembled mesh has twenty-four faces");
  require(artifact.components().size() == 2,
          "assembled mesh has two components");
  for (const auto &face : artifact.mesh().faces)
    require(face.size() == 3, "every assembled face is a triangle");
  require(artifact.topology_status() ==
              bounded::candidate_topology_status::
                  assembled_pending_independent_verification,
          "candidate has pending topology status");
  require(artifact.verification() ==
              bounded::output_assembly_verification_disposition::
                  independently_verified,
          "candidate is independently verified");
}

void test_empty() {
  auto inter = build_assembly_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                      boolean_operation::intersection);
  const auto &artifact = *inter.output;
  require(artifact.mesh().vertices.empty(), "empty result has no vertices");
  require(artifact.mesh().faces.empty(), "empty result has no faces");
  require(artifact.components().empty(), "empty result has no components");
}

void test_maps() {
  auto un = build_assembly_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                   boolean_operation::set_union);
  const auto &artifact = *un.output;
  require(artifact.vertex_by_occurrence().size() == 16 &&
              artifact.occurrence_by_vertex().size() == 16,
          "vertex maps cover every cleaned occurrence");
  require(artifact.facet_by_triangle().size() == 24 &&
              artifact.triangle_by_facet().size() == 24,
          "facet maps cover every cleaned triangle");
  for (std::size_t o = 0; o < 16; ++o) {
    const auto p = artifact.vertex_by_occurrence()[o];
    require(artifact.occurrence_by_vertex()[p] == o,
            "vertex maps are inverse");
  }
}

void test_canonical() {
  auto first = build_assembly_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                      boolean_operation::set_union);
  auto second = build_assembly_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                       boolean_operation::set_union);
  require(first.output->canonical_bytes() == second.output->canonical_bytes(),
          "assembly canonical bytes are deterministic");
  require(first.output->digest() == second.output->digest(),
          "assembly digest is deterministic");
}

void test_verifier_rejects_corruption() {
  auto fixture = build_assembly_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                        boolean_operation::set_union);
  const auto &cleaned = *fixture.cleanup.output;

  {
    // Corrupt a face index.
    candidate_type copy = *fixture.output;
    auto &mesh = bounded::output_assembly_artifact_test_access::mesh(copy);
    mesh.faces[0][0] = mesh.faces[0][1];
    bounded_boolean_error error;
    require(!bounded::verify_assembled_output_candidate_structural(
                copy, cleaned, error),
            "verifier rejects a corrupted face index");
  }
  {
    // Forge the digest.
    candidate_type copy = *fixture.output;
    bounded::output_assembly_artifact_test_access::digest(copy) = {};
    bounded_boolean_error error;
    require(!bounded::verify_assembled_output_candidate_structural(
                copy, cleaned, error),
            "verifier rejects a forged digest");
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
    if (suite == "all" || suite == "maps")
      test_maps();
    if (suite == "all" || suite == "canonical")
      test_canonical();
    if (suite == "all" || suite == "mutation")
      test_verifier_rejects_corruption();
    std::cout << "Component 14 output assembly suite passed: " << suite << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
