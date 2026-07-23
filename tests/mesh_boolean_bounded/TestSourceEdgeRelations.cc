#include "YgorMeshesBooleanBounded/SourceEdgeRelationKernel.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>

using namespace ygor::mesh_boolean::bounded;

namespace {

int failures = 0;

void check(bool condition, const std::string &message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <class T>
bounded_point3<T> point(const context_owner_token &owner,
                        const std::array<T, 3> &coordinates,
                        std::uint64_t identity) {
  bounded_point3<T> result;
  result.owner = owner;
  result.coordinates.owner = owner;
  result.provenance = provenance_id(identity);
  result.lineage = geometric_lineage_id(identity);
  for (std::size_t axis = 0; axis < 3; ++axis) {
    auto component = checked_bounded_singleton(owner, coordinates[axis]);
    if (!component.has_value()) {
      check(false, "test point singleton construction failed");
      continue;
    }
    result.coordinates.components[axis] = std::move(*component.value());
  }
  result.coordinates.radial_error_upper = T(0);
  return result;
}

template <class T>
source_edge_relation_input<T>
edge(const context_owner_token &owner, operand_id operand,
     std::uint64_t source_edge, const std::array<T, 3> &start,
     const std::array<T, 3> &end) {
  source_edge_relation_input<T> result;
  result.feature.operand = operand;
  result.feature.kind = relation_feature_kind::source_edge;
  result.feature.primary = source_edge;
  result.start = point(owner, start, source_edge * 2 + 1);
  result.end = point(owner, end, source_edge * 2 + 2);
  return result;
}

template <class T>
void expect_relation(
    const char *name, const std::array<T, 3> &a0,
    const std::array<T, 3> &a1, const std::array<T, 3> &b0,
    const std::array<T, 3> &b1, source_edge_support_class support,
    source_edge_contact_class contact,
    source_edge_orientation_relation orientation =
        source_edge_orientation_relation::not_applicable) {
  const auto owner = context_owner_token::create();
  const auto first = edge(owner, operand_id::a, 3, a0, a1);
  const auto second = edge(owner, operand_id::b, 7, b0, b1);
  auto relation =
      classify_source_edge_relation(first, second, owner, T(1e-10));
  check(relation.has_value(), std::string(name) + " should classify");
  if (!relation.has_value())
    return;
  check(relation.value()->support == support,
        std::string(name) + " support class");
  check(relation.value()->contact == contact,
        std::string(name) + " contact class");
  check(relation.value()->orientation == orientation,
        std::string(name) + " orientation class");
  check(valid_source_edge_relation_record(*relation.value()),
        std::string(name) + " record validates");
  check(relation.value()->semantic_digest ==
            sha256::digest(
                encode_source_edge_relation_semantics(*relation.value())),
        std::string(name) + " semantic digest is reproducible");
}

template <class T> void exercise_relation_categories() {
  expect_relation<T>(
      "skew", {{T(0), T(0), T(0)}}, {{T(1), T(0), T(0)}},
      {{T(0), T(1), T(1)}}, {{T(0), T(2), T(1)}},
      source_edge_support_class::skew_separated,
      source_edge_contact_class::none);
  expect_relation<T>(
      "proper crossing", {{T(-1), T(0), T(0)}},
      {{T(1), T(0), T(0)}}, {{T(0), T(-1), T(0)}},
      {{T(0), T(1), T(0)}},
      source_edge_support_class::nonparallel_coplanar,
      source_edge_contact_class::proper_crossing);
  expect_relation<T>(
      "endpoint contact", {{T(0), T(0), T(0)}},
      {{T(1), T(0), T(0)}}, {{T(1), T(0), T(0)}},
      {{T(1), T(1), T(0)}},
      source_edge_support_class::nonparallel_coplanar,
      source_edge_contact_class::endpoint_contact);
  expect_relation<T>(
      "parallel separated", {{T(0), T(0), T(0)}},
      {{T(1), T(0), T(0)}}, {{T(0), T(1), T(0)}},
      {{T(1), T(1), T(0)}},
      source_edge_support_class::parallel_separated,
      source_edge_contact_class::none);
  expect_relation<T>(
      "collinear disjoint", {{T(0), T(0), T(0)}},
      {{T(1), T(0), T(0)}}, {{T(2), T(0), T(0)}},
      {{T(3), T(0), T(0)}}, source_edge_support_class::collinear,
      source_edge_contact_class::none,
      source_edge_orientation_relation::same);
  expect_relation<T>(
      "collinear point", {{T(0), T(0), T(0)}},
      {{T(1), T(0), T(0)}}, {{T(1), T(0), T(0)}},
      {{T(2), T(0), T(0)}}, source_edge_support_class::collinear,
      source_edge_contact_class::point_contact,
      source_edge_orientation_relation::same);
  expect_relation<T>(
      "partial overlap", {{T(0), T(0), T(0)}},
      {{T(2), T(0), T(0)}}, {{T(1), T(0), T(0)}},
      {{T(3), T(0), T(0)}}, source_edge_support_class::collinear,
      source_edge_contact_class::partial_overlap,
      source_edge_orientation_relation::same);
  expect_relation<T>(
      "first contains second", {{T(0), T(0), T(0)}},
      {{T(3), T(0), T(0)}}, {{T(1), T(0), T(0)}},
      {{T(2), T(0), T(0)}}, source_edge_support_class::collinear,
      source_edge_contact_class::first_contains_second,
      source_edge_orientation_relation::same);
  expect_relation<T>(
      "second contains first", {{T(0), T(0), T(0)}},
      {{T(1), T(0), T(0)}}, {{T(-1), T(0), T(0)}},
      {{T(2), T(0), T(0)}}, source_edge_support_class::collinear,
      source_edge_contact_class::second_contains_first,
      source_edge_orientation_relation::same);
  expect_relation<T>(
      "equal same orientation", {{T(0), T(0), T(0)}},
      {{T(1), T(0), T(0)}}, {{T(0), T(0), T(0)}},
      {{T(1), T(0), T(0)}}, source_edge_support_class::collinear,
      source_edge_contact_class::equal,
      source_edge_orientation_relation::same);
  expect_relation<T>(
      "equal opposite orientation", {{T(0), T(0), T(0)}},
      {{T(1), T(0), T(0)}}, {{T(1), T(0), T(0)}},
      {{T(0), T(0), T(0)}}, source_edge_support_class::collinear,
      source_edge_contact_class::equal,
      source_edge_orientation_relation::opposite);
}

void test_exact_stored_coordinate_bindings() {
  const std::array<double, 3> a0{{0.0, 0.0, 0.0}};
  const std::array<double, 3> a1{{1.0, 0.0, 0.0}};
  const std::array<double, 3> b0{{0.0, 1.0, 0.0}};
  const std::array<double, 3> b1{{1.0, 1.0, 0.0}};

  const auto parallel =
      exact_segment_directions_parallel_3d(a0, a1, b0, b1);
  check(parallel.evaluation_status == numeric_status::success &&
            parallel.status == exact_relation_status::exact_zero &&
            parallel.formula ==
                exact_relation_formula_code::collinearity_3d,
        "stored-coordinate parallel relation is exact");

  const auto separated_collinearity =
      exact_collinearity_3d(a0, a1, b0);
  check(separated_collinearity.evaluation_status ==
                numeric_status::success &&
            separated_collinearity.status ==
                exact_relation_status::exact_positive,
        "stored-coordinate noncollinearity is exact");

  const auto coplanarity = exact_coplanarity_3d(a0, a1, b0, b1);
  check(coplanarity.evaluation_status == numeric_status::success &&
            coplanarity.status == exact_relation_status::exact_zero &&
            coplanarity.formula ==
                exact_relation_formula_code::coplanarity_3d,
        "stored-coordinate coplanarity relation is exact");
}

void test_fail_closed_contracts() {
  const auto owner = context_owner_token::create();
  auto first = edge(owner, operand_id::a, 1,
                    std::array<double, 3>{{0.0, 0.0, 0.0}},
                    std::array<double, 3>{{1.0, 0.0, 0.0}});
  auto second = edge(owner, operand_id::b, 2,
                     std::array<double, 3>{{0.0, -1.0, 0.0}},
                     std::array<double, 3>{{0.0, 1.0, 0.0}});

  auto relation =
      classify_source_edge_relation(first, second, owner, 1e-10);
  check(relation.has_value(), "mutation fixture should classify");
  if (relation.has_value()) {
    relation.value()->first_parameters[0].rounded_nominal += 0.25;
    check(!valid_source_edge_relation_record(*relation.value()),
          "parameter mutation invalidates the record");
  }

  const auto other_owner = context_owner_token::create();
  auto wrong_owner =
      classify_source_edge_relation(first, second, other_owner, 1e-10);
  check(!wrong_owner.has_value(), "wrong owner fails closed");
  check(wrong_owner.error() &&
            wrong_owner.error()->subcode ==
                static_cast<std::uint32_t>(
                    relation_subcode::source_edge_relation_malformed),
        "wrong owner has stable source-edge failure");

  first.feature.kind = relation_feature_kind::facet_internal_diagonal;
  auto diagonal =
      classify_source_edge_relation(first, second, owner, 1e-10);
  check(!diagonal.has_value(),
        "internal diagonal cannot publish a public source-edge relation");
}

void test_unresolved_direction_rejected() {
  const auto owner = context_owner_token::create();
  auto first = edge(owner, operand_id::a, 1,
                    std::array<double, 3>{{0.0, 0.0, 0.0}},
                    std::array<double, 3>{{1.0, 0.0, 0.0}});
  auto second = edge(owner, operand_id::b, 2,
                     std::array<double, 3>{{0.0, -1.0, 0.0}},
                     std::array<double, 3>{{0.0, 1.0, 0.0}});

  auto uncertain = finite_interval<double>::create(-1.0, 1.0);
  check(uncertain.has_value(), "uncertain fixture interval should build");
  if (!uncertain)
    return;
  first.end.coordinates.components[0].uncertainty_enclosure = *uncertain;
  auto relation =
      classify_source_edge_relation(first, second, owner, 1e-10);
  check(!relation.has_value(),
        "direction whose squared norm is not definitely positive fails");
  check(relation.error() &&
            relation.error()->subcode ==
                static_cast<std::uint32_t>(
                    relation_subcode::source_edge_direction_degenerate),
        "unresolved direction has stable failure");
}

} // namespace

int main() {
  exercise_relation_categories<float>();
  exercise_relation_categories<double>();
  test_exact_stored_coordinate_bindings();
  test_fail_closed_contracts();
  test_unresolved_direction_rejected();
  if (failures != 0)
    std::cerr << failures
              << " Component 07 source-edge relation checks failed\n";
  return failures == 0 ? 0 : 1;
}
