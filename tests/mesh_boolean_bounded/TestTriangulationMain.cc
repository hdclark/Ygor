#include "TriangulationFixtures.h"
#include "YgorMeshesBooleanBounded/TriangulationVerifier.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace bounded = ygor::mesh_boolean::bounded;
using namespace triangulation_tests;

namespace ygor::mesh_boolean::bounded {
struct output_triangulation_artifact_test_access {
  template <class T, class I>
  static auto &triangles(triangulated_output_complex<T, I> &artifact) {
    return artifact.triangles_;
  }
  template <class T, class I>
  static auto &diagonals(triangulated_output_complex<T, I> &artifact) {
    return artifact.diagonals_;
  }
  template <class T, class I>
  static auto &boundary_assignments(
      triangulated_output_complex<T, I> &artifact) {
    return artifact.boundary_assignments_;
  }
  template <class T, class I>
  static auto &digest(triangulated_output_complex<T, I> &artifact) {
    return artifact.digest_;
  }
};
} // namespace ygor::mesh_boolean::bounded

namespace {

using triangulated_type = bounded::triangulated_output_complex<scalar, index_type>;

void test_contracts() {
  // Disjoint union of two unit boxes: 12 quad faces -> 24 triangles.
  auto un = build_triangulation_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                        boolean_operation::set_union);
  const auto &artifact = *un.output;
  require(artifact.triangles().size() == 24,
          "disjoint union triangulates to twenty-four triangles");
  require(artifact.diagonals().size() == 12,
          "disjoint union has twelve internal diagonals");
  require(artifact.internal_halfedges().size() == 24,
          "disjoint union has twenty-four internal halfedges");
  require(artifact.boundary_assignments().size() == 48,
          "disjoint union has forty-eight boundary assignments");
  require(artifact.coverage_certificates().size() == 12,
          "disjoint union has twelve coverage certificates");
  require(artifact.support_frames().size() == 12,
          "disjoint union has twelve support frames");
}

void test_empty() {
  auto inter = build_triangulation_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                           boolean_operation::intersection);
  const auto &artifact = *inter.output;
  require(artifact.triangles().empty(), "empty intersection has no triangles");
  require(artifact.diagonals().empty(), "empty intersection has no diagonals");
  require(artifact.verification() ==
              bounded::triangulation_verification_disposition::
                  independently_verified,
          "empty artifact is independently verified");
}

void test_containment() {
  auto bma = build_triangulation_fixture(1, 1, 1, 2, 2, 2, 0, 0, 0, 3, 3, 3,
                                         boolean_operation::b_minus_a);
  const auto &artifact = *bma.output;
  require(artifact.triangles().size() == 24,
          "B-A triangulates to twenty-four triangles");
}

void test_boundary_conservation() {
  auto un = build_triangulation_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                        boolean_operation::set_union);
  const auto &artifact = *un.output;
  std::vector<std::uint64_t> counts(
      un.topology.output->halfedges().size(), 0);
  for (const auto &assignment : artifact.boundary_assignments())
    ++counts[assignment.component11_halfedge];
  for (std::size_t h = 0; h < counts.size(); ++h)
    require(counts[h] == 1,
            "every Component 11 boundary halfedge is assigned exactly once");
}

void test_euler() {
  auto un = build_triangulation_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                        boolean_operation::set_union);
  const auto &artifact = *un.output;
  for (const auto &certificate : artifact.coverage_certificates()) {
    require(certificate.vertex_count == 4 &&
                certificate.face_count == 2 &&
                certificate.edge_count == 5 &&
                certificate.hole_count == 0,
            "quad region has V=4, E=5, F=2, H=0");
    require(certificate.vertex_count - certificate.edge_count +
                    certificate.face_count ==
                1,
            "quad region satisfies Euler V-E+F=1");
  }
}

void test_canonical() {
  auto first = build_triangulation_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                           boolean_operation::set_union);
  auto second = build_triangulation_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                            boolean_operation::set_union);
  require(first.output->canonical_bytes() == second.output->canonical_bytes(),
          "triangulation canonical bytes are deterministic");
  require(first.output->digest() == second.output->digest(),
          "triangulation digest is deterministic");
}

void test_verifier_rejects_corruption() {
  auto fixture = build_triangulation_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                             boolean_operation::set_union);
  const auto &polygonal = *fixture.topology.output;

  {
    // Remove one boundary assignment.
    triangulated_type copy = *fixture.output;
    auto &assignments =
        bounded::output_triangulation_artifact_test_access::boundary_assignments(
            copy);
    assignments.pop_back();
    bounded_boolean_error error;
    require(!bounded::verify_triangulated_output_complex_structural(
                copy, polygonal, error),
            "verifier rejects a missing boundary assignment");
  }
  {
    // Break an internal halfedge pair reciprocity.
    triangulated_type copy = *fixture.output;
    auto &diagonals =
        bounded::output_triangulation_artifact_test_access::diagonals(copy);
    diagonals[0].halfedges[0] = diagonals[0].halfedges[1];
    bounded_boolean_error error;
    require(!bounded::verify_triangulated_output_complex_structural(
                copy, polygonal, error),
            "verifier rejects a broken diagonal pair");
  }
  {
    // Forge the digest.
    triangulated_type copy = *fixture.output;
    bounded::output_triangulation_artifact_test_access::digest(copy) = {};
    bounded_boolean_error error;
    require(!bounded::verify_triangulated_output_complex_structural(
                copy, polygonal, error),
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
    if (suite == "all" || suite == "containment")
      test_containment();
    if (suite == "all" || suite == "boundary")
      test_boundary_conservation();
    if (suite == "all" || suite == "euler")
      test_euler();
    if (suite == "all" || suite == "canonical")
      test_canonical();
    if (suite == "all" || suite == "mutation")
      test_verifier_rejects_corruption();
    std::cout << "Component 12 triangulation suite passed: " << suite << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
