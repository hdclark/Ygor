#include "OutputTopologyFixtures.h"
#include "YgorMeshesBooleanBounded/OutputTopologyVerifier.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace bounded = ygor::mesh_boolean::bounded;
using namespace output_topology_tests;

namespace ygor::mesh_boolean::bounded {
struct output_topology_artifact_test_access {
  template <class T, class I>
  static auto &halfedges(polygonal_output_complex<T, I> &artifact) {
    return artifact.halfedges_;
  }
  template <class T, class I>
  static auto &paired_edges(polygonal_output_complex<T, I> &artifact) {
    return artifact.paired_edges_;
  }
  template <class T, class I>
  static auto &face_cycles(polygonal_output_complex<T, I> &artifact) {
    return artifact.face_cycles_;
  }
  template <class T, class I>
  static auto &contour_nodes(polygonal_output_complex<T, I> &artifact) {
    return artifact.contour_nodes_;
  }
  template <class T, class I>
  static auto &digest(polygonal_output_complex<T, I> &artifact) {
    return artifact.digest_;
  }
};
} // namespace ygor::mesh_boolean::bounded

namespace {

using output_type = bounded::polygonal_output_complex<scalar, index_type>;

void test_contracts() {
  // Disjoint union of two unit boxes: 12 retained faces, 24 planned edges.
  auto un = build_output_topology_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                          boolean_operation::set_union);
  const auto &artifact = *un.output;
  require(artifact.paired_edges().size() == 24,
          "disjoint union has twenty-four paired edges");
  require(artifact.halfedges().size() == 48,
          "disjoint union has forty-eight halfedges");
  require(artifact.face_regions().size() == 12,
          "disjoint union has twelve face regions");
  require(artifact.face_cycles().size() == 12,
          "disjoint union has twelve face cycles");
  require(artifact.cycle_halfedge_refs().size() == 48,
          "cycle halfedge references equal the halfedge count");
  require(artifact.vertex_occurrences().size() == 16,
          "disjoint union has sixteen output vertices");
  require(artifact.boundary_darts().size() == 48,
          "disjoint union has one boundary dart per incidence");
  require(artifact.contour_nodes().size() == 12,
          "disjoint union has one contour per region");
  require(artifact.contour_witnesses().size() == 12,
          "disjoint union has one witness per contour");
  require(artifact.incidence_audits().size() == 48,
          "disjoint union has one audit row per incidence");
}

void test_empty() {
  // Disjoint intersection is empty: a fully audited empty artifact.
  auto inter = build_output_topology_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                             boolean_operation::intersection);
  const auto &artifact = *inter.output;
  require(artifact.paired_edges().empty(), "empty intersection has no edges");
  require(artifact.halfedges().empty(), "empty intersection has no halfedges");
  require(artifact.face_regions().empty(), "empty intersection has no regions");
  require(artifact.face_cycles().empty(), "empty intersection has no cycles");
  require(artifact.vertex_occurrences().empty(),
          "empty intersection has no vertices");
  require(artifact.verification() ==
              bounded::output_topology_verification_disposition::
                  independently_verified,
          "empty artifact is independently verified");
}

void test_containment() {
  // A inside B: B-A keeps B's outer shell and A's reversed cavity boundary.
  auto bma = build_output_topology_fixture(1, 1, 1, 2, 2, 2, 0, 0, 0, 3, 3, 3,
                                           boolean_operation::b_minus_a);
  const auto &artifact = *bma.output;
  require(artifact.face_regions().size() == 12,
          "B-A has twelve regions (outer B plus cavity A)");
  require(artifact.face_cycles().size() == 12,
          "B-A has twelve cycles");
}

void test_reciprocity() {
  auto un = build_output_topology_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                          boolean_operation::set_union);
  const auto &artifact = *un.output;
  for (const auto &halfedge : artifact.halfedges()) {
    const auto &pair = artifact.halfedges()[halfedge.pair];
    require(pair.pair == halfedge.canonical_id, "pair field is reciprocal");
    require(halfedge.origin == pair.destination &&
                halfedge.destination == pair.origin,
            "paired halfedge endpoints are reversed");
    require(halfedge.successor != bounded::output_topology_invalid_ordinal,
            "every halfedge has a successor");
    require(artifact.halfedges()[halfedge.successor].predecessor ==
                halfedge.canonical_id,
            "successor and predecessor are inverse");
  }
}

void test_vertex_links() {
  auto un = build_output_topology_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                          boolean_operation::set_union);
  const auto &artifact = *un.output;
  for (const auto &link : artifact.vertex_link_evidence()) {
    require(link.outgoing_count == 3,
            "box corner has three outgoing halfedges");
    // The rotation walk around_origin(h) = pair(predecessor(h)) must close.
    const auto start = artifact.outgoing_halfedges()[link.outgoing_begin];
    std::uint64_t current = start;
    std::uint64_t walked = 0;
    do {
      current = artifact.halfedges()[artifact.halfedges()[current].predecessor]
                    .pair;
      ++walked;
      require(walked <= link.outgoing_count, "vertex link walk closes");
    } while (current != start);
    require(walked == link.outgoing_count, "vertex link walk is a full cycle");
  }
}

void test_canonical() {
  auto first = build_output_topology_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                             boolean_operation::set_union);
  auto second = build_output_topology_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                              boolean_operation::set_union);
  require(first.output->canonical_bytes() == second.output->canonical_bytes(),
          "output topology canonical bytes are deterministic");
  require(first.output->digest() == second.output->digest(),
          "output topology digest is deterministic");
  require(first.output->verification() ==
              bounded::output_topology_verification_disposition::
                  independently_verified,
          "output topology is independently verified");
}

void test_verifier_rejects_corruption() {
  auto fixture = build_output_topology_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                               boolean_operation::set_union);
  const auto &retained = *fixture.selection.retained;

  {
    // Break a halfedge's pair reciprocity.
    output_type copy = *fixture.output;
    auto &halfedges = bounded::output_topology_artifact_test_access::halfedges(copy);
    halfedges[0].pair = halfedges[2].canonical_id;
    bounded_boolean_error error;
    require(!bounded::verify_polygonal_output_complex_structural(copy, retained,
                                                                 error),
            "verifier rejects broken pair reciprocity");
  }
  {
    // Forge the digest.
    output_type copy = *fixture.output;
    bounded::output_topology_artifact_test_access::digest(copy) = {};
    bounded_boolean_error error;
    require(!bounded::verify_polygonal_output_complex_structural(copy, retained,
                                                                 error),
            "verifier rejects a forged digest");
  }
  {
    // Assign two outer contours to one region.
    output_type copy = *fixture.output;
    auto &contours = bounded::output_topology_artifact_test_access::contour_nodes(copy);
    for (auto &node : contours) {
      if (node.role == bounded::contour_role::outer) {
        node.region = 0;
        break;
      }
    }
    bounded_boolean_error error;
    require(!bounded::verify_polygonal_output_complex_structural(copy, retained,
                                                                 error),
            "verifier rejects a duplicated outer contour");
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
    if (suite == "all" || suite == "reciprocity")
      test_reciprocity();
    if (suite == "all" || suite == "links")
      test_vertex_links();
    if (suite == "all" || suite == "canonical")
      test_canonical();
    if (suite == "all" || suite == "mutation")
      test_verifier_rejects_corruption();
    std::cout << "Component 11 output topology suite passed: " << suite << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
