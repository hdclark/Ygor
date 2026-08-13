#include "SelectionFixtures.h"
#include "YgorMeshesBooleanBounded/SelectionCoincidence.h"
#include "YgorMeshesBooleanBounded/SelectionEdgeOccurrences.h"
#include "YgorMeshesBooleanBounded/SelectionVerifier.h"
#include "YgorMeshesBooleanBounded/SelectionVertexOccurrences.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace bounded = ygor::mesh_boolean::bounded;
using namespace selection_tests;

namespace ygor::mesh_boolean::bounded {
struct selection_artifact_test_access {
  template <class T, class I>
  static auto &dispositions(retained_surface_complex<T, I> &artifact) {
    return artifact.dispositions_;
  }
  template <class T, class I>
  static auto &retained_uses(retained_surface_complex<T, I> &artifact) {
    return artifact.retained_uses_;
  }
  template <class T, class I>
  static auto &incidences(retained_surface_complex<T, I> &artifact) {
    return artifact.incidences_;
  }
  template <class T, class I>
  static auto &digest(retained_surface_complex<T, I> &artifact) {
    return artifact.digest_;
  }
};
} // namespace ygor::mesh_boolean::bounded

namespace {

using retained_type = bounded::retained_surface_complex<scalar, index_type>;

void test_truth_table() {
  // All 80 Component 01 operation/side cells: retention is exactly the frozen
  // table, and orientation places the result occupied side on the negative
  // side.
  for (std::uint8_t op = 1; op <= 5; ++op) {
    for (std::uint8_t bits = 0; bits < 16; ++bits) {
      const auto operation = static_cast<boolean_operation>(op);
      bounded::side_occupancy raw{bool(bits & 1), bool(bits & 2), bool(bits & 4),
                                  bool(bits & 8)};
      const auto cell = bounded::evaluate_truth(operation, raw);
      const bool result_neg = bounded::operation_value(operation, raw.a_negative,
                                                       raw.b_negative);
      const bool result_pos = bounded::operation_value(operation, raw.a_positive,
                                                       raw.b_positive);
      require(cell.result_negative == result_neg, "truth negative side matches");
      require(cell.result_positive == result_pos, "truth positive side matches");
      require(cell.retain == (result_neg != result_pos),
              "retain is exactly a result-side transition");
      if (cell.retain) {
        require(cell.orientation == (result_neg
                                         ? bounded::boundary_orientation::preserve
                                         : bounded::boundary_orientation::reverse),
                "orientation places result occupied side on negative side");
        require(cell.multiplicity == 1, "retained cell has multiplicity one");
      } else {
        require(cell.orientation == bounded::boundary_orientation::not_applicable,
                "discarded cell has no orientation");
        require(cell.multiplicity == 0, "discarded cell has zero multiplicity");
      }
    }
  }
  // Truth-table bytes and digest are deterministic.
  const auto first = bounded::materialize_truth_table();
  const auto second = bounded::materialize_truth_table();
  require(first.bytes == second.bytes, "truth table bytes are deterministic");
  require(first.digest == second.digest, "truth table digest is deterministic");
}

void test_disjoint() {
  // Two disjoint boxes: union keeps both, intersection is empty, A-B keeps A.
  auto un = build_selection_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                    boolean_operation::set_union);
  require(retained_count(*un.retained, bounded::operand_id::a) == 6,
          "disjoint union keeps all of A");
  require(retained_count(*un.retained, bounded::operand_id::b) == 6,
          "disjoint union keeps all of B");
  require(reversed_count(*un.retained, bounded::operand_id::a) == 0,
          "disjoint union preserves A orientation");
  require(un.retained->planned_edges().size() == 24,
          "disjoint union has twenty-four planned edges");

  auto inter = build_selection_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                       boolean_operation::intersection);
  require(inter.retained->retained_uses().empty(),
          "disjoint intersection is empty");
  require(inter.retained->planned_edges().empty(),
          "disjoint intersection has no edges");

  auto diff = build_selection_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                      boolean_operation::a_minus_b);
  require(retained_count(*diff.retained, bounded::operand_id::a) == 6,
          "disjoint A-B keeps all of A");
  require(retained_count(*diff.retained, bounded::operand_id::b) == 0,
          "disjoint A-B discards all of B");

  auto sym = build_selection_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                     boolean_operation::symmetric_difference);
  require(retained_count(*sym.retained, bounded::operand_id::a) == 6 &&
              retained_count(*sym.retained, bounded::operand_id::b) == 6,
          "disjoint symmetric difference keeps both");
}

void test_containment() {
  // A inside B.
  auto un = build_selection_fixture(1, 1, 1, 2, 2, 2, 0, 0, 0, 3, 3, 3,
                                    boolean_operation::set_union);
  require(retained_count(*un.retained, bounded::operand_id::b) == 6,
          "containment union keeps B");
  require(retained_count(*un.retained, bounded::operand_id::a) == 0,
          "containment union suppresses internal A");

  auto inter = build_selection_fixture(1, 1, 1, 2, 2, 2, 0, 0, 0, 3, 3, 3,
                                       boolean_operation::intersection);
  require(retained_count(*inter.retained, bounded::operand_id::a) == 6,
          "containment intersection keeps A");
  require(retained_count(*inter.retained, bounded::operand_id::b) == 0,
          "containment intersection discards B");

  auto adb = build_selection_fixture(1, 1, 1, 2, 2, 2, 0, 0, 0, 3, 3, 3,
                                     boolean_operation::a_minus_b);
  require(adb.retained->retained_uses().empty(), "A inside B means A-B is empty");

  auto bma = build_selection_fixture(1, 1, 1, 2, 2, 2, 0, 0, 0, 3, 3, 3,
                                     boolean_operation::b_minus_a);
  require(retained_count(*bma.retained, bounded::operand_id::b) == 6,
          "B-A keeps B's outer shell");
  require(retained_count(*bma.retained, bounded::operand_id::a) == 6,
          "B-A keeps A's cavity boundary");
  require(reversed_count(*bma.retained, bounded::operand_id::a) == 6,
          "B-A reverses A's cavity boundary");
}

void test_result_occupancy_transition() {
  auto un = build_selection_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                    boolean_operation::set_union);
  for (const auto &use : un.retained->retained_uses()) {
    const bool neg = bounded::operation_value(
        un.retained->operation(), use.result_occupancy.a_negative,
        use.result_occupancy.b_negative);
    const bool pos = bounded::operation_value(
        un.retained->operation(), use.result_occupancy.a_positive,
        use.result_occupancy.b_positive);
    require(neg != pos, "every retained use is a result boundary");
  }
}

void test_coincidence_owner_rank() {
  // The frozen owner rank prefers the operation's symbolic preferred operand,
  // then the canonical source feature, then the atom ordinal.
  const auto rank_a = bounded::selection_coincidence::compute_owner_rank(
      boolean_operation::set_union, bounded::operand_id::a, 3, 7, true, 0);
  const auto rank_b = bounded::selection_coincidence::compute_owner_rank(
      boolean_operation::set_union, bounded::operand_id::b, 3, 7, true, 0);
  require(rank_a < rank_b, "union owner rank prefers operand A");
  const auto rank_b_minus = bounded::selection_coincidence::compute_owner_rank(
      boolean_operation::b_minus_a, bounded::operand_id::a, 3, 7, true, 0);
  const auto rank_b_minus_b = bounded::selection_coincidence::compute_owner_rank(
      boolean_operation::b_minus_a, bounded::operand_id::b, 3, 7, true, 0);
  require(rank_b_minus_b < rank_b_minus,
          "B-A owner rank prefers operand B");
  const auto rank_near = bounded::selection_coincidence::compute_owner_rank(
      boolean_operation::set_union, bounded::operand_id::a, 3, 9, true, 0);
  require(rank_a < rank_near, "owner rank prefers smaller source feature");
}

void test_edge_grouping() {
  using namespace bounded::selection_edge_occurrences;
  using descriptor = bounded::surface_occurrence_descriptor;
  using role = bounded::direction_role;
  // A proper transverse seam pairs an A-owned use with a B-owned use.
  descriptor da;
  da.source_operand = bounded::operand_id::a;
  da.source_facet = 1;
  da.sheet_owner_lineage = 11;
  da.retained_use_lineage = 1;
  da.result_side_transition = -1;
  descriptor db;
  db.source_operand = bounded::operand_id::b;
  db.source_facet = 2;
  db.sheet_owner_lineage = 22;
  db.retained_use_lineage = 2;
  db.result_side_transition = 1;

  directed_proposal pa;
  pa.incidence = 0;
  pa.start_domain = 5;
  pa.end_domain = 9;
  pa.direction = role::forward;
  pa.descriptor = da;
  pa.expected_opposite = db;
  directed_proposal pb;
  pb.incidence = 1;
  pb.start_domain = 9;
  pb.end_domain = 5;
  pb.direction = role::reverse;
  pb.descriptor = db;
  pb.expected_opposite = da;

  std::vector<edge_pair> pairs;
  const auto status = group_directed_proposals({pa, pb}, pairs);
  require(status == grouping_status::success,
          "reciprocal cross-operand pair groups successfully");
  require(pairs.size() == 1 && pairs.front().cross_operand &&
              pairs.front().cross_owner,
          "transverse seam is cross-operand and cross-owner");

  // Cardinality != 2 must fail.
  directed_proposal pc = pa;
  const auto bad = group_directed_proposals({pa, pb, pc}, pairs);
  require(bad == grouping_status::mate_cardinality,
          "three uses on one edge are rejected");

  // Non-reciprocal descriptors must fail: both members claim the same
  // (descriptor, expected) pair, so reciprocity cannot hold.
  directed_proposal pa2 = pa;
  pa2.descriptor = da;
  pa2.expected_opposite = db;
  directed_proposal pb2 = pb;
  pb2.descriptor = da;
  pb2.expected_opposite = db;
  const auto nonreciprocal = group_directed_proposals({pa2, pb2}, pairs);
  require(nonreciprocal == grouping_status::nonreciprocal_descriptor,
          "non-reciprocal descriptors are rejected");
}

void test_link_cycles() {
  using namespace bounded::selection_vertex_occurrences;
  // A triangular fan: three faces sharing one vertex produce a six-port cycle.
  const std::uint64_t n = 6;
  std::vector<link_port> ports(n);
  std::vector<link_arc> corners(3);
  std::vector<link_arc> mates(3);
  // Face i has ports 2i and 2i+1 connected by a corner arc.
  for (std::uint64_t i = 0; i < 3; ++i)
    corners[i] = {2 * i, 2 * i + 1};
  // Mates connect face i's second port to face (i+1)'s first port.
  for (std::uint64_t i = 0; i < 3; ++i)
    mates[i] = {2 * i + 1, 2 * ((i + 1) % 3)};
  for (std::uint64_t i = 0; i < 3; ++i) {
    ports[2 * i].corner_arc = i;
    ports[2 * i].mate_arc = (i + 2) % 3;
    ports[2 * i + 1].corner_arc = i;
    ports[2 * i + 1].mate_arc = i;
  }
  std::vector<link_cycle> cycles;
  const auto status = extract_link_cycles(ports, corners, mates, cycles);
  require(status == link_status::success, "triangular fan extracts one cycle");
  require(cycles.size() == 1 && cycles.front().ports.size() == n,
          "triangular fan has a single six-port cycle");

  // A degenerate arc (self-loop) must fail.
  std::vector<link_port> good_ports = ports;
  std::vector<link_arc> bad_corners = corners;
  bad_corners[0] = {0, 0};
  const auto branched = extract_link_cycles(good_ports, bad_corners, mates, cycles);
  require(branched == link_status::branched,
          "degenerate link arc is rejected");
}

void test_canonical() {
  auto first = build_selection_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                       boolean_operation::set_union);
  auto second = build_selection_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                        boolean_operation::set_union);
  require(first.retained->canonical_bytes() == second.retained->canonical_bytes(),
          "selection canonical bytes are deterministic");
  require(first.retained->digest() == second.retained->digest(),
          "selection digest is deterministic");
  require(first.retained->verification() ==
              bounded::selection_verification_disposition::independently_verified,
          "selection is independently verified");
}

void test_verifier_rejects_corruption() {
  auto fixture = build_selection_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5,
                                         boolean_operation::set_union);
  const auto &predecessor = fixture.classification.broad.predecessor;
  const auto &manifolds = *predecessor.manifolds;
  const auto &relations = *fixture.classification.relations;
  const auto &intersections = *fixture.classification.intersections;
  const auto &classification = *fixture.classification.classification;

  {
    // Flip a retained disposition to discard.
    retained_type copy = *fixture.retained;
    auto &dispositions =
        bounded::selection_artifact_test_access::dispositions(copy);
    bool mutated = false;
    for (auto &d : dispositions) {
      if (d.disposition == bounded::final_disposition::retain_preserve) {
        d.disposition = bounded::final_disposition::discard_equal_sides;
        mutated = true;
        break;
      }
    }
    require(mutated, "disposition mutation applies");
    bounded_boolean_error error;
    require(!bounded::verify_retained_surface_complex(
                copy, predecessor.context, manifolds, relations, intersections,
                classification, error),
            "verifier rejects a flipped disposition");
  }
  {
    // Corrupt the digest.
    retained_type copy = *fixture.retained;
    bounded::selection_artifact_test_access::digest(copy) = {};
    bounded_boolean_error error;
    require(!bounded::verify_retained_surface_complex(
                copy, predecessor.context, manifolds, relations, intersections,
                classification, error),
            "verifier rejects a forged digest");
  }
}

} // namespace

int main(int argc, char **argv) {
  try {
    const std::string suite = argc > 1 ? argv[1] : "all";
    if (suite == "all" || suite == "truth")
      test_truth_table();
    if (suite == "all" || suite == "disjoint")
      test_disjoint();
    if (suite == "all" || suite == "containment")
      test_containment();
    if (suite == "all" || suite == "occupancy")
      test_result_occupancy_transition();
    if (suite == "all" || suite == "coincidence")
      test_coincidence_owner_rank();
    if (suite == "all" || suite == "edge_vertex") {
      test_edge_grouping();
      test_link_cycles();
    }
    if (suite == "all" || suite == "canonical")
      test_canonical();
    if (suite == "all" || suite == "mutation")
      test_verifier_rejects_corruption();
    std::cout << "Component 10 selection suite passed: " << suite << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
