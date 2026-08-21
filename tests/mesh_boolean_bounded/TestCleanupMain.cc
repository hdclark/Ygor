#include "CleanupFixtures.h"
#include "YgorMeshesBooleanBounded/CleanupVerifier.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace bounded = ygor::mesh_boolean::bounded;
using namespace cleanup_tests;

namespace ygor::mesh_boolean::bounded {
struct cleanup_artifact_test_access {
  template <class T>
  static auto &triangles(cleaned_triangle_manifold<T> &artifact) {
    return artifact.triangles_;
  }
  template <class T>
  static auto &halfedges(cleaned_triangle_manifold<T> &artifact) {
    return artifact.halfedges_;
  }
  template <class T>
  static auto &digest(cleaned_triangle_manifold<T> &artifact) {
    return artifact.digest_;
  }
};
} // namespace ygor::mesh_boolean::bounded

namespace {

using cleaned_type = bounded::cleaned_triangle_manifold<scalar>;

void test_contracts() {
  // Disjoint union of two unit boxes: two genus-0 components.
  auto un = build_cleanup_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                  boolean_operation::set_union);
  const auto &artifact = *un.output;
  require(artifact.vertices().size() == 16,
          "disjoint union has sixteen cleaned vertices");
  require(artifact.paired_edges().size() == 36,
          "disjoint union has thirty-six cleaned edges");
  require(artifact.halfedges().size() == 72,
          "disjoint union has seventy-two cleaned halfedges");
  require(artifact.triangles().size() == 24,
          "disjoint union has twenty-four cleaned triangles");
  require(artifact.components().size() == 2,
          "disjoint union has two cleaned components");
  require(artifact.status() == bounded::final_cleanup_status::verified_clean,
          "disjoint union is verified clean");
  require(artifact.actions().empty(),
          "disjoint union required no cleanup actions");
}

void test_empty() {
  auto inter = build_cleanup_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                     boolean_operation::intersection);
  const auto &artifact = *inter.output;
  require(artifact.vertices().empty(), "empty intersection has no vertices");
  require(artifact.triangles().empty(), "empty intersection has no triangles");
  require(artifact.status() == bounded::final_cleanup_status::verified_empty,
          "empty artifact is verified empty");
}

void test_containment() {
  // B-A: outer shell plus reversed cavity shell, two components.
  auto bma = build_cleanup_fixture(1, 1, 1, 2, 2, 2, 0, 0, 0, 3, 3, 3,
                                   boolean_operation::b_minus_a);
  const auto &artifact = *bma.output;
  require(artifact.triangles().size() == 24,
          "B-A has twenty-four cleaned triangles");
  require(artifact.components().size() == 2,
          "B-A has two cleaned components");
}

void test_links() {
  auto un = build_cleanup_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                  boolean_operation::set_union);
  const auto &artifact = *un.output;
  // Each box corner is incident to at least its three cube edges plus any
  // face diagonals; the fan must form one closed cycle.
  for (const auto &vertex : artifact.vertices()) {
    require(vertex.link_count >= 3, "box vertex has a nonempty fan");
    const auto start = artifact.link_index()[vertex.link_begin];
    std::uint64_t current = start;
    std::uint64_t walked = 0;
    do {
      const auto &halfedge = artifact.halfedges()[current];
      const std::uint64_t prev =
          artifact.halfedges()[artifact.halfedges()[current].next].next;
      current = artifact.halfedges()[prev].pair;
      ++walked;
      require(walked <= vertex.link_count, "box vertex fan closes");
    } while (current != start);
    require(walked == vertex.link_count, "box vertex fan is a single cycle");
  }
}

void test_euler() {
  auto un = build_cleanup_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                  boolean_operation::set_union);
  const auto &artifact = *un.output;
  for (const auto &component : artifact.components())
    require(component.euler_chi == 2,
            "each genus-0 component has Euler characteristic two");
}

void test_canonical() {
  auto first = build_cleanup_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                     boolean_operation::set_union);
  auto second = build_cleanup_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                      boolean_operation::set_union);
  require(first.output->canonical_bytes() == second.output->canonical_bytes(),
          "cleanup canonical bytes are deterministic");
  require(first.output->digest() == second.output->digest(),
          "cleanup digest is deterministic");
}

void test_verifier_rejects_corruption() {
  auto fixture = build_cleanup_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                       boolean_operation::set_union);

  {
    // Break a halfedge pair reciprocity.
    cleaned_type copy = *fixture.output;
    auto &halfedges = bounded::cleanup_artifact_test_access::halfedges(copy);
    halfedges[0].pair = halfedges[2].canonical_id;
    bounded_boolean_error error;
    require(!bounded::verify_cleaned_triangle_manifold_structural(copy, error),
            "verifier rejects broken pair reciprocity");
  }
  {
    // Forge the digest.
    cleaned_type copy = *fixture.output;
    bounded::cleanup_artifact_test_access::digest(copy) = {};
    bounded_boolean_error error;
    require(!bounded::verify_cleaned_triangle_manifold_structural(copy, error),
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
    if (suite == "all" || suite == "links")
      test_links();
    if (suite == "all" || suite == "euler")
      test_euler();
    if (suite == "all" || suite == "canonical")
      test_canonical();
    if (suite == "all" || suite == "mutation")
      test_verifier_rejects_corruption();
    std::cout << "Component 13 cleanup suite passed: " << suite << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
