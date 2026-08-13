#include "ClassificationFixtures.h"
#include "YgorMeshesBooleanBounded/BoundedShellQuery.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace bounded = ygor::mesh_boolean::bounded;
using namespace classification_tests;

namespace ygor::mesh_boolean::bounded {
struct classification_artifact_test_access {
  static auto &groups(classification_complex<double, std::uint32_t> &artifact) {
    return artifact.groups_;
  }
  static auto &atoms(classification_complex<double, std::uint32_t> &artifact) {
    return artifact.atoms_;
  }
  static auto &side_labels(classification_complex<double, std::uint32_t> &artifact) {
    return artifact.side_labels_;
  }
  static auto &quotient_edges(classification_complex<double, std::uint32_t> &artifact) {
    return artifact.quotient_edges_;
  }
};
} // namespace ygor::mesh_boolean::bounded

namespace {

void test_shell_query_directions() {
  const auto &directions = bounded::canonical_primitive_directions();
  require(directions.size() == 13, "canonical direction count");
  for (std::size_t i = 0; i < directions.size(); ++i) {
    const auto &d = directions[i];
    require(d.x != 0 || d.y != 0 || d.z != 0, "direction is nonzero");
    // First nonzero component is positive (no opposite duplicates).
    if (d.x != 0)
      require(d.x > 0, "first nonzero direction component is positive");
    else if (d.y != 0)
      require(d.y > 0, "first nonzero direction component is positive");
    else
      require(d.z > 0, "first nonzero direction component is positive");
  }
  for (std::size_t i = 0; i < directions.size(); ++i)
    for (std::size_t j = i + 1; j < directions.size(); ++j)
      require(directions[i] != directions[j], "directions are distinct");
}

void test_shell_query_winding() {
  auto fixture = box_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5);
  const auto &box = *fixture.broad.predecessor.manifolds->a();

  bounded::shell_query_point<double> inside;
  inside.nominal = {{0.3, 0.4, 0.6}};
  bounded::shell_query_point<double> outside;
  outside.nominal = {{2.0, 2.0, 2.0}};

  bool inside_definite = false;
  bool outside_definite = false;
  for (const auto &direction : bounded::canonical_primitive_directions()) {
    auto result_in = bounded::bounded_shell_query(box, inside, direction);
    if (result_in.disposition == bounded::shell_query_disposition::definite) {
      require(result_in.total_winding == 1,
              "interior shell query winding is one");
      inside_definite = true;
      break;
    }
  }
  require(inside_definite, "interior shell query reaches a definite direction");
  for (const auto &direction : bounded::canonical_primitive_directions()) {
    auto result_out = bounded::bounded_shell_query(box, outside, direction);
    if (result_out.disposition == bounded::shell_query_disposition::definite) {
      require(result_out.total_winding == 0,
              "exterior shell query winding is zero");
      outside_definite = true;
      break;
    }
  }
  require(outside_definite, "exterior shell query reaches a definite direction");
}

std::vector<std::int64_t> atom_windings(bounded::operand_id operand,
                                        const classification_type &artifact) {
  std::vector<std::int64_t> result;
  for (const auto &atom : artifact.atoms()) {
    if (atom.operand != operand)
      continue;
    if (atom.group >= artifact.groups().size())
      throw std::runtime_error("atom group out of range");
    result.push_back(artifact.groups()[atom.group].total_winding);
  }
  std::sort(result.begin(), result.end());
  return result;
}

bool all_equal(const std::vector<std::int64_t> &values, std::int64_t expected) {
  return !values.empty() && std::all_of(values.begin(), values.end(),
                                        [expected](std::int64_t value) {
                                          return value == expected;
                                        });
}

void test_disjoint() {
  auto fixture = box_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5);
  const auto a = atom_windings(bounded::operand_id::a, *fixture.classification);
  const auto b = atom_windings(bounded::operand_id::b, *fixture.classification);
  require(all_equal(a, 0), "disjoint operand A is outside B");
  require(all_equal(b, 0), "disjoint operand B is outside A");
  require(fixture.classification->groups().size() >= 2,
          "disjoint operands produce at least two groups");
}

void test_containment() {
  auto fixture = box_fixture(1, 1, 1, 2, 2, 2, 0, 0, 0, 3, 3, 3);
  const auto a = atom_windings(bounded::operand_id::a, *fixture.classification);
  const auto b = atom_windings(bounded::operand_id::b, *fixture.classification);
  require(all_equal(a, 1), "contained operand A is inside B");
  require(all_equal(b, 0), "containing operand B is outside A");
}

void test_overlap() {
  auto fixture = box_fixture(0, 0, 0, 1, 1, 1, 0.5, 0.5, 0.5, 1.5, 1.5, 1.5);
  const auto a = atom_windings(bounded::operand_id::a, *fixture.classification);
  const auto b = atom_windings(bounded::operand_id::b, *fixture.classification);
  // Overlap: each operand must contain atoms both inside and outside the other.
  require(std::find(a.begin(), a.end(), 0) != a.end() &&
              std::find(a.begin(), a.end(), 1) != a.end(),
          "overlap operand A has inside and outside atoms");
  require(std::find(b.begin(), b.end(), 0) != b.end() &&
              std::find(b.begin(), b.end(), 1) != b.end(),
          "overlap operand B has inside and outside atoms");
}

void test_canonical_bytes() {
  auto first = box_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5);
  auto second = box_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5);
  require(first.classification->canonical_bytes() ==
              second.classification->canonical_bytes(),
          "classification canonical bytes are deterministic");
  require(first.classification->digest() == second.classification->digest(),
          "classification digest is deterministic");
  require(first.classification->verification() ==
              bounded::classification_verification_disposition::independently_verified,
          "classification is independently verified");
}

void test_side_labels() {
  auto fixture = box_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5);
  require(!fixture.classification->side_labels().empty(),
          "classification publishes side labels");
  for (const auto &atom : fixture.classification->atoms()) {
    require(atom.side_label < fixture.classification->side_labels().size(),
            "every atom has a side label");
  }
}

void test_verifier_rejects_corruption() {
  auto fixture = box_fixture(0, 0, 0, 1, 1, 1, 4, 4, 4, 5, 5, 5);
  const auto &manifolds = *fixture.broad.predecessor.manifolds;
  const auto &relations = *fixture.relations;
  const auto &intersections = *fixture.intersections;

  {
    // Corrupt a group winding to an out-of-domain value.
    classification_type copy = *fixture.classification;
    auto &groups = bounded::classification_artifact_test_access::groups(copy);
    if (!groups.empty())
      groups.front().total_winding = 2;
    bounded_boolean_error error;
    require(!bounded::verify_classification_complex(
                copy, relations, intersections, manifolds, error),
            "verifier rejects out-of-domain winding");
  }
  {
    // Corrupt a side label so it disagrees with the group winding.
    classification_type copy = *fixture.classification;
    auto &labels = bounded::classification_artifact_test_access::side_labels(copy);
    if (!labels.empty())
      labels.front().negative_side = bounded::occupancy_state::strict_inside;
    bounded_boolean_error error;
    require(!bounded::verify_classification_complex(
                copy, relations, intersections, manifolds, error),
            "verifier rejects inconsistent side label");
  }
  {
    // Corrupt an atom's group assignment so it disagrees with its group's
    // membership list.
    classification_type copy = *fixture.classification;
    auto &atoms = bounded::classification_artifact_test_access::atoms(copy);
    bool mutated = false;
    for (auto &atom : atoms) {
      if (atom.group != bounded::classification_invalid_ordinal &&
          atom.group < fixture.classification->groups().size() &&
          atom.group > 0) {
        atom.group = atom.group - 1;
        mutated = true;
        break;
      }
    }
    require(mutated, "classification atom group mutation applies");
    bounded_boolean_error error;
    require(!bounded::verify_classification_complex(
                copy, relations, intersections, manifolds, error),
            "verifier rejects inconsistent atom-to-group map");
  }
}

void test_shell_contribution_consistency() {
  auto fixture = box_fixture(1, 1, 1, 2, 2, 2, 0, 0, 0, 3, 3, 3);
  for (const auto &group : fixture.classification->groups()) {
    std::int64_t sum = 0;
    for (const auto &entry : group.shell_winding)
      sum += entry.second;
    require(sum == group.total_winding,
            "shell contributions sum to the group winding");
  }
}

void test_quotient_reverse() {
  auto fixture = box_fixture(0, 0, 0, 1, 1, 1, 0.5, 0.5, 0.5, 1.5, 1.5, 1.5);
  for (const auto &edge : fixture.classification->quotient_edges()) {
    if (edge.role != bounded::propagation_edge_role::numeric_constraint)
      continue;
    require(edge.reverse < fixture.classification->quotient_edges().size(),
            "quotient edge has a reverse");
    const auto &reverse = fixture.classification->quotient_edges()[edge.reverse];
    require(reverse.source_group == edge.destination_group &&
                reverse.destination_group == edge.source_group &&
                reverse.total_delta == -edge.total_delta,
            "quotient reverse edge negates the delta");
  }
}

} // namespace

int main(int argc, char **argv) {
  try {
    const std::string suite = argc > 1 ? argv[1] : "all";
    if (suite == "all" || suite == "shell_query") {
      test_shell_query_directions();
      test_shell_query_winding();
    }
    if (suite == "all" || suite == "atoms") {
      test_disjoint();
      test_containment();
      test_overlap();
    }
    if (suite == "all" || suite == "grouping")
      test_shell_contribution_consistency();
    if (suite == "all" || suite == "propagation")
      test_quotient_reverse();
    if (suite == "all" || suite == "contacts")
      test_side_labels();
    if (suite == "all" || suite == "canonical")
      test_canonical_bytes();
    if (suite == "all" || suite == "mutation")
      test_verifier_rejects_corruption();
    std::cout << "Component 09 classification suite passed: " << suite << '\n';
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
