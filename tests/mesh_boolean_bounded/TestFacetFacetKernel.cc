#include "YgorMeshesBooleanBounded/FacetFacetRelations.h"
#include "YgorMeshesBooleanBounded/PrecisionVerifier.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>

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
bounded_point3<T> point3(const context_owner_token &owner,
                         std::uint64_t identity, T x, T y, T z) {
  bounded_point3<T> result;
  result.owner = owner;
  result.coordinates.owner = owner;
  result.provenance = provenance_id(identity);
  result.lineage = geometric_lineage_id(identity);
  const std::array<T, 3> values{{x, y, z}};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    auto component = checked_bounded_singleton(owner, values[axis]);
    check(component.has_value(), "bounded singleton construction");
    if (component.has_value())
      result.coordinates.components[axis] = std::move(*component.value());
  }
  result.coordinates.radial_error_upper = T(0);
  return result;
}

relation_feature_key facet_feature(operand_id operand, std::uint64_t id) {
  relation_feature_key result;
  result.operand = operand;
  result.kind = relation_feature_kind::source_facet;
  result.primary = id;
  result.secondary = id + 10;
  return result;
}

template <class T>
source_facet_support_input<T> facet(
    const context_owner_token &owner, operand_id operand, std::uint64_t id,
    std::array<std::array<T, 3>, 3> points) {
  source_facet_support_input<T> result;
  result.feature = facet_feature(operand, id);
  result.source_facet = id;
  result.ring = id + 10;
  result.shell = id + 20;
  result.material_side = occupied_side::negative;
  result.dropped_axis = 2;
  for (std::size_t index = 0; index < 3; ++index)
    result.support[index] = point3(owner, 100 * id + index,
                                   points[index][0], points[index][1],
                                   points[index][2]);
  return result;
}

template <class T>
source_facet_source_facet_relation_record<T> classify_or_fail(
    const char *name, source_facet_support_input<T> first,
    source_facet_support_input<T> second,
    const context_owner_token &owner) {
  auto relation = classify_source_facet_support_relation(
      std::move(first), std::move(second), owner, static_cast<T>(1.0e-10));
  check(relation.has_value(), std::string(name) + " should classify");
  if (!relation.has_value()) {
    if (relation.error())
      std::cerr << "  subcode=" << relation.error()->subcode
                << " summary=" << relation.error()->summary << '\n';
    return {};
  }
  check(valid_source_facet_relation_record(*relation.value()),
        std::string(name) + " record validates");
  check(relation.value()->semantic_digest ==
            sha256::digest(encode_source_facet_relation_semantics(
                *relation.value())),
        std::string(name) + " digest reproduces");
  return std::move(*relation.value());
}

void test_exact_plane_relations() {
  const std::array<double, 3> a0{{0.0, 0.0, 0.0}};
  const std::array<double, 3> a1{{1.0, 0.0, 0.0}};
  const std::array<double, 3> a2{{0.0, 1.0, 0.0}};
  const std::array<double, 3> b0{{0.0, 0.0, 1.0}};
  const std::array<double, 3> b1{{1.0, 0.0, 1.0}};
  const std::array<double, 3> b2{{0.0, 1.0, 1.0}};
  const auto parallel = exact_plane_normals_parallel_3d(a0, a1, a2,
                                                         b0, b1, b2);
  check(parallel.status == exact_relation_status::exact_zero,
        "parallel support normals are exactly recognized");
  const auto orientation = exact_plane_normal_dot_3d(a0, a1, a2,
                                                      b0, b1, b2);
  check(orientation.status == exact_relation_status::exact_positive,
        "same support orientation is exactly recognized");

  const std::array<double, 3> c0{{0.0, 0.0, 0.0}};
  const std::array<double, 3> c1{{0.0, 1.0, 0.0}};
  const std::array<double, 3> c2{{0.0, 0.0, 1.0}};
  const auto transverse = exact_plane_normals_parallel_3d(a0, a1, a2,
                                                           c0, c1, c2);
  check(transverse.status == exact_relation_status::exact_positive,
        "transverse support normals are exactly recognized");

  const auto residual = exact_plane_point_residual_3d(a0, a1, a2, b0);
  check(residual.formula ==
            exact_relation_formula_code::plane_point_residual_3d &&
        residual.status == exact_relation_status::exact_positive,
        "plane-point residual has a distinct formula and bounded-sign convention");
  exact_relation_evidence evidence;
  evidence.owner = context_owner_token::create();
  evidence.formula_code = static_cast<std::uint16_t>(residual.formula);
  evidence.status = residual.status;
  evidence.normalization_exponent = residual.normalization_exponent;
  evidence.capacity_used = residual.capacity_used;
  const std::vector<double> inputs{
      a0[0], a0[1], a0[2], a1[0], a1[1], a1[2],
      a2[0], a2[1], a2[2], b0[0], b0[1], b0[2]};
  check(verify_exact_relation(evidence, inputs, &evidence.owner),
        "plane-point residual replay reconstructs the same exact sign");
}

void test_transverse_carrier() {
  const auto owner = context_owner_token::create();
  auto horizontal = facet<double>(owner, operand_id::a, 1,
      {{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}}});
  auto vertical = facet<double>(owner, operand_id::b, 2,
      {{{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}});
  auto relation = classify_or_fail("transverse", horizontal, vertical, owner);
  check(relation.classification ==
            source_facet_support_relation_class::transverse &&
        relation.has_transverse_carrier &&
        relation.transverse_carrier.residuals_accepted,
        "transverse planes publish a residual-checked carrier");
  if (relation.has_transverse_carrier) {
    check(relation.transverse_carrier.direction_squared.lower() > 0.0,
          "carrier direction has a positive conditioning lower bound");
    check(relation.transverse_carrier.point.rounded[0] == 0.0 &&
          relation.transverse_carrier.point.rounded[2] == 0.0,
          "carrier point lies on both known supports");
  }
}

void test_parallel_and_coplanar_classes() {
  const auto owner = context_owner_token::create();
  auto base = facet<double>(owner, operand_id::a, 1,
      {{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}}});
  auto separated = facet<double>(owner, operand_id::b, 2,
      {{{0.0, 0.0, 1.0}, {1.0, 0.0, 1.0}, {0.0, 1.0, 1.0}}});
  auto separated_relation = classify_or_fail(
      "parallel separated", base, separated, owner);
  check(separated_relation.classification ==
            source_facet_support_relation_class::parallel_separated &&
        separated_relation.has_coplanarity_truth &&
        !separated_relation.has_transverse_carrier,
        "parallel offset supports are distinguished from coplanarity");

  auto same = facet<double>(owner, operand_id::b, 3,
      {{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}}});
  auto same_relation = classify_or_fail("coplanar same", base, same, owner);
  check(same_relation.classification ==
            source_facet_support_relation_class::coplanar_same_orientation &&
        same_relation.has_orientation_truth,
        "exact coplanar supports retain same orientation");

  auto opposite = facet<double>(owner, operand_id::b, 4,
      {{{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0}}});
  auto opposite_relation = classify_or_fail(
      "coplanar opposite", base, opposite, owner);
  check(opposite_relation.classification ==
            source_facet_support_relation_class::coplanar_opposite_orientation,
        "exact coplanar supports retain opposite orientation");
}

void test_fail_closed_mutations() {
  const auto owner = context_owner_token::create();
  auto base = facet<double>(owner, operand_id::a, 1,
      {{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}}});
  auto vertical = facet<double>(owner, operand_id::b, 2,
      {{{0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}});
  auto relation = classify_or_fail("mutation source", base, vertical, owner);
  relation.semantic_digest.bytes[0] ^= 0x80U;
  check(!valid_source_facet_relation_record(relation),
        "facet/facet semantic digest mutation is rejected");

  auto wrong_owner = classify_source_facet_support_relation(
      base, vertical, context_owner_token::create(), 1.0e-10);
  check(!wrong_owner.has_value(), "wrong runtime owner fails closed");

  auto degenerate = base;
  degenerate.support[2] = degenerate.support[1];
  auto invalid = classify_source_facet_support_relation(
      degenerate, vertical, owner, 1.0e-10);
  check(!invalid.has_value(), "degenerate support triple fails closed");
}

void test_float_profile() {
  const auto owner = context_owner_token::create();
  auto horizontal = facet<float>(owner, operand_id::a, 1,
      {{{0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}}});
  auto vertical = facet<float>(owner, operand_id::b, 2,
      {{{0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}}});
  auto relation = classify_or_fail("float transverse", horizontal, vertical,
                                   owner);
  check(relation.classification ==
            source_facet_support_relation_class::transverse,
        "float profile reproduces the transverse known answer");
}

} // namespace

int main() {
  test_exact_plane_relations();
  test_transverse_carrier();
  test_parallel_and_coplanar_classes();
  test_fail_closed_mutations();
  test_float_profile();
  if (failures == 0) {
    std::cout << "Component 07 facet/facet support checks passed\n";
    return 0;
  }
  std::cerr << failures << " facet/facet checks failed\n";
  return 1;
}
