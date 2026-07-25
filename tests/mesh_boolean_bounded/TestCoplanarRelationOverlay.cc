#include "YgorMeshesBooleanBounded/CoplanarRelationOverlay.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

using namespace ygor::mesh_boolean::bounded;

namespace {
int failures = 0;

void check(bool value, const std::string &message) {
  if (!value) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <class T>
bounded_point3<T> point3(const context_owner_token &owner,
                         std::uint64_t identity,
                         const std::array<T, 3> &value) {
  bounded_point3<T> point;
  point.owner = owner;
  point.coordinates.owner = owner;
  point.provenance = provenance_id(identity);
  point.lineage = geometric_lineage_id(identity);
  for (std::size_t axis = 0; axis < 3; ++axis) {
    auto scalar = checked_bounded_singleton(owner, value[axis]);
    check(scalar.has_value(), "bounded point singleton");
    if (scalar.has_value())
      point.coordinates.components[axis] = std::move(*scalar.value());
  }
  point.coordinates.radial_error_upper = T(0);
  return point;
}

relation_feature_key feature(operand_id operand, relation_feature_kind kind,
                             std::uint64_t primary,
                             std::uint64_t secondary = 0) {
  relation_feature_key result;
  result.operand = operand;
  result.kind = kind;
  result.primary = primary;
  result.secondary = secondary;
  return result;
}

template <class T>
struct polygon_fixture final {
  coplanar_facet_polygon_input<T> facet;
  source_facet_support_input<T> support;
  std::vector<std::array<T, 3>> points;
};

template <class T>
polygon_fixture<T> polygon(const context_owner_token &owner,
                           operand_id operand, std::uint64_t id,
                           std::vector<std::array<T, 2>> xy,
                           bool reverse = false) {
  if (reverse)
    std::reverse(xy.begin(), xy.end());
  polygon_fixture<T> result;
  result.facet.feature = feature(operand, relation_feature_kind::source_facet,
                                 id, id + 1000);
  result.facet.source_facet = id;
  result.facet.ring = id + 1000;
  result.facet.shell = id + 2000;
  result.facet.polygon.reserve(xy.size());
  result.facet.boundary_edges.reserve(xy.size());
  result.points.reserve(xy.size());
  for (std::size_t i = 0; i < xy.size(); ++i) {
    const std::array<T, 3> p{{xy[i][0], xy[i][1], T(0)}};
    result.points.push_back(p);
    auto bounded = point3(owner, id * 100 + i, p);
    result.facet.polygon.push_back(source_edge_facet_detail::project_point(
        bounded, 2, id * 100 + i, i));
    result.facet.boundary_edges.push_back(feature(
        operand, relation_feature_kind::source_edge, id * 1000 + i,
        id * 1000 + ((i + 1) % xy.size())));
  }
  const auto orientation = bounded_source_polygon_kernel<T>::polygon_orientation(
      result.facet.polygon);
  check(orientation.has_value(), "polygon orientation available");
  if (orientation)
    result.facet.orientation = orientation->bounded_sign;

  result.support.feature = result.facet.feature;
  result.support.source_facet = result.facet.source_facet;
  result.support.ring = result.facet.ring;
  result.support.shell = result.facet.shell;
  result.support.material_side = occupied_side::negative;
  result.support.dropped_axis = 2;
  result.support.support[0] = point3(owner, id * 100 + 30, result.points[0]);
  result.support.support[1] = point3(owner, id * 100 + 31, result.points[1]);
  result.support.support[2] = point3(owner, id * 100 + 32, result.points[2]);
  return result;
}

template <class T>
source_edge_relation_input<T> edge_input(
    const context_owner_token &owner, const relation_feature_key &edge,
    const std::array<T, 3> &start, const std::array<T, 3> &end,
    std::uint64_t identity) {
  source_edge_relation_input<T> result;
  result.feature = edge;
  result.start = point3(owner, identity * 2, start);
  result.end = point3(owner, identity * 2 + 1, end);
  return result;
}

template <class T>
std::vector<coplanar_boundary_relation_input<T>> boundary_relations(
    const context_owner_token &owner, const polygon_fixture<T> &first,
    const polygon_fixture<T> &second) {
  std::vector<coplanar_boundary_relation_input<T>> result;
  const T residual =
      std::max(T(1e-10), T(32) * std::numeric_limits<T>::epsilon());
  std::uint64_t request = 0;
  for (std::size_t i = 0; i < first.points.size(); ++i) {
    for (std::size_t j = 0; j < second.points.size(); ++j) {
      auto a = edge_input(owner, first.facet.boundary_edges[i], first.points[i],
                          first.points[(i + 1) % first.points.size()],
                          10000 + request);
      auto b = edge_input(owner, second.facet.boundary_edges[j], second.points[j],
                          second.points[(j + 1) % second.points.size()],
                          20000 + request);
      auto relation = classify_source_edge_relation(a, b, owner, residual);
      check(relation.has_value(), "boundary relation classification");
      if (relation.has_value())
        result.push_back({relation_request_id(request), i, j,
                          std::move(*relation.value()), 0});
      ++request;
    }
  }
  return result;
}

template <class T>
source_facet_coplanar_overlay_record<T> classify(
    const char *name, polygon_fixture<T> first, polygon_fixture<T> second,
    const context_owner_token &owner) {
  const T residual =
      std::max(T(1e-10), T(32) * std::numeric_limits<T>::epsilon());
  auto support = classify_source_facet_support_relation(
      first.support, second.support, owner, residual);
  check(support.has_value(), std::string(name) + " support relation");
  if (!support.has_value())
    return {};
  auto dependencies = boundary_relations(owner, first, second);
  auto overlay = classify_coplanar_facet_overlay(
      std::move(first.facet), std::move(second.facet),
      std::move(*support.value()), std::move(dependencies), owner);
  check(overlay.has_value(), std::string(name) + " overlay classification");
  if (!overlay.has_value()) {
    if (overlay.error())
      std::cerr << "  subcode=" << overlay.error()->subcode
                << " summary=" << overlay.error()->summary << '\n';
    return {};
  }
  check(valid_coplanar_overlay_record(*overlay.value()),
        std::string(name) + " overlay validates");
  check(verify_coplanar_overlay_record(*overlay.value()),
        std::string(name) + " overlay independently reconstructs");
  check(overlay.value()->semantic_digest ==
            sha256::digest(
                encode_coplanar_overlay_semantics(*overlay.value())),
        std::string(name) + " digest reproduces");
  return std::move(*overlay.value());
}

void known_answers() {
  const auto owner = context_owner_token::create();
  auto square = [](double x0, double y0, double x1, double y1) {
    return std::vector<std::array<double, 2>>{{{x0, y0}}, {{x1, y0}},
                                               {{x1, y1}}, {{x0, y1}}};
  };

  auto disjoint = classify(
      "disjoint", polygon<double>(owner, operand_id::a, 1, square(0, 0, 1, 1)),
      polygon<double>(owner, operand_id::b, 2, square(2, 0, 3, 1)), owner);
  check(disjoint.classification == coplanar_facet_overlay_class::disjoint,
        "disjoint coplanar facets");

  auto point = classify(
      "point", polygon<double>(owner, operand_id::a, 3, square(0, 0, 1, 1)),
      polygon<double>(owner, operand_id::b, 4, square(1, 1, 2, 2)), owner);
  check(point.classification == coplanar_facet_overlay_class::point_contact,
        "point-only coplanar contact");

  auto segment = classify(
      "segment", polygon<double>(owner, operand_id::a, 5, square(0, 0, 1, 1)),
      polygon<double>(owner, operand_id::b, 6, square(1, 0, 2, 1)), owner);
  check(segment.classification ==
            coplanar_facet_overlay_class::segment_contact,
        "segment-only coplanar contact");

  auto overlap = classify(
      "area overlap",
      polygon<double>(owner, operand_id::a, 7, square(0, 0, 2, 2)),
      polygon<double>(owner, operand_id::b, 8, square(1, -1, 3, 1)), owner);
  check(overlap.classification == coplanar_facet_overlay_class::area_overlap &&
            overlap.proper_crossing_count == 2,
        "crossing boundaries produce area overlap");

  auto containment = classify(
      "containment",
      polygon<double>(owner, operand_id::a, 9, square(0, 0, 4, 4)),
      polygon<double>(owner, operand_id::b, 10, square(1, 1, 2, 2)), owner);
  check(containment.classification ==
            coplanar_facet_overlay_class::first_contains_second,
        "strict containment is directed");

  auto equal = classify(
      "equal",
      polygon<double>(owner, operand_id::a, 11, square(0, 0, 2, 2)),
      polygon<double>(owner, operand_id::b, 12, square(0, 0, 2, 2)), owner);
  check(equal.classification ==
            coplanar_facet_overlay_class::equal_same_orientation &&
            equal.distinct_sheet_occurrences,
        "equal geometry preserves distinct sheet occurrences");

  auto opposite = classify(
      "equal opposite",
      polygon<double>(owner, operand_id::a, 13, square(0, 0, 2, 2)),
      polygon<double>(owner, operand_id::b, 14, square(0, 0, 2, 2), true),
      owner);
  check(opposite.classification ==
            coplanar_facet_overlay_class::equal_opposite_orientation,
        "equal opposite sheets retain orientation relation");
}

void failures_and_mutations() {
  const auto owner = context_owner_token::create();
  const std::vector<std::array<double, 2>> square{
      {{0, 0}}, {{1, 0}}, {{1, 1}}, {{0, 1}}};
  auto first = polygon<double>(owner, operand_id::a, 20, square);
  auto second = polygon<double>(owner, operand_id::b, 21, square);
  const double residual = 1e-10;
  auto support = classify_source_facet_support_relation(
      first.support, second.support, owner, residual);
  check(support.has_value(), "mutation support available");
  auto dependencies = boundary_relations(owner, first, second);
  auto incomplete = dependencies;
  incomplete.pop_back();
  auto missing = classify_coplanar_facet_overlay(
      first.facet, second.facet, *support.value(), std::move(incomplete), owner);
  check(!missing.has_value() && missing.error() &&
            missing.error()->subcode == static_cast<std::uint32_t>(
                relation_subcode::coplanar_overlay_dependency_missing),
        "missing boundary relation fails closed");

  auto overlay = classify_coplanar_facet_overlay(
      first.facet, second.facet, *support.value(), std::move(dependencies), owner);
  check(overlay.has_value(), "mutation source overlay");
  if (overlay.has_value()) {
    auto forged = *overlay.value();
    forged.classification = coplanar_facet_overlay_class::disjoint;
    forged.semantic_digest =
        sha256::digest(encode_coplanar_overlay_semantics(forged));
    check(valid_coplanar_overlay_record(forged) &&
              !verify_coplanar_overlay_record(forged),
          "self-consistent forged classification is independently rejected");
    overlay.value()->semantic_digest.bytes[0] ^= 0x80U;
    check(!valid_coplanar_overlay_record(*overlay.value()),
          "overlay digest mutation rejected");
  }

  auto wrong_owner = classify_coplanar_facet_overlay(
      first.facet, second.facet, *support.value(),
      boundary_relations(owner, first, second), context_owner_token::create());
  check(!wrong_owner.has_value(), "cross-owner overlay input rejected");
}

void owner_free_semantics() {
  const std::vector<std::array<double, 2>> outer{
      {{0, 0}}, {{4, 0}}, {{4, 4}}, {{0, 4}}};
  const std::vector<std::array<double, 2>> inner{
      {{1, 1}}, {{2, 1}}, {{2, 2}}, {{1, 2}}};
  const auto first_owner = context_owner_token::create();
  const auto second_owner = context_owner_token::create();
  auto first = classify(
      "owner-free first",
      polygon<double>(first_owner, operand_id::a, 40, outer),
      polygon<double>(first_owner, operand_id::b, 41, inner), first_owner);
  auto second = classify(
      "owner-free second",
      polygon<double>(second_owner, operand_id::a, 40, outer),
      polygon<double>(second_owner, operand_id::b, 41, inner), second_owner);
  check(first.semantic_digest == second.semantic_digest,
        "runtime owner tokens are excluded from overlay semantics");
}

void float_profile() {
  const auto owner = context_owner_token::create();
  const std::vector<std::array<float, 2>> outer{
      {{0, 0}}, {{4, 0}}, {{4, 4}}, {{0, 4}}};
  const std::vector<std::array<float, 2>> inner{
      {{1, 1}}, {{2, 1}}, {{2, 2}}, {{1, 2}}};
  auto result = classify("float containment",
                         polygon<float>(owner, operand_id::a, 30, outer),
                         polygon<float>(owner, operand_id::b, 31, inner), owner);
  check(result.classification ==
            coplanar_facet_overlay_class::first_contains_second,
        "float profile reproduces containment");
}
} // namespace

int main() {
  known_answers();
  failures_and_mutations();
  owner_free_semantics();
  float_profile();
  if (failures == 0) {
    std::cout << "Component 07 coplanar overlay checks passed\n";
    return 0;
  }
  std::cerr << failures << " coplanar overlay checks failed\n";
  return 1;
}
