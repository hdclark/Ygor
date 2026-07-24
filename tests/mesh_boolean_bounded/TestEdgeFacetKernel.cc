#include "YgorMeshesBooleanBounded/EdgeFacetRelations.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

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

template <class T>
projected_source_point<T> point2(std::uint64_t identity, T x, T y) {
  projected_source_point<T> result;
  result.source_vertex = identity;
  result.nominal = {x, y};
  result.enclosure = {*finite_interval<T>::checked_singleton(x),
                      *finite_interval<T>::checked_singleton(y)};
  return result;
}

relation_feature_key edge_feature(operand_id operand, std::uint64_t id) {
  relation_feature_key result;
  result.operand = operand;
  result.kind = relation_feature_kind::source_edge;
  result.primary = id;
  return result;
}

template <class T>
source_edge_relation_input<T>
edge_input(const context_owner_token &owner, operand_id operand,
           std::uint64_t id, std::uint64_t first_identity, T x0, T y0, T z0,
           std::uint64_t second_identity, T x1, T y1, T z1) {
  source_edge_relation_input<T> result;
  result.feature = edge_feature(operand, id);
  result.start = point3(owner, first_identity, x0, y0, z0);
  result.end = point3(owner, second_identity, x1, y1, z1);
  return result;
}

template <class T>
source_edge_facet_input<T>
fixture(const context_owner_token &owner, T x0, T y0, T z0, T x1, T y1,
        T z1) {
  source_edge_facet_input<T> result;
  result.edge = edge_input<T>(owner, operand_id::a, 3, 100, x0, y0, z0,
                              101, x1, y1, z1);
  result.edge_source_vertices = {100, 101};
  result.facet_feature.operand = operand_id::b;
  result.facet_feature.kind = relation_feature_kind::source_facet;
  result.facet_feature.primary = 4;
  result.facet_feature.secondary = 2;
  result.source_facet = 4;
  result.ring = 2;
  result.shell = 1;
  result.material_side = occupied_side::negative;
  result.dropped_axis = 2;
  result.support = {point3<T>(owner, 10, T(0), T(0), T(0)),
                    point3<T>(owner, 11, T(1), T(0), T(0)),
                    point3<T>(owner, 12, T(1), T(1), T(0))};
  result.polygon = {point2<T>(10, T(0), T(0)),
                    point2<T>(11, T(1), T(0)),
                    point2<T>(12, T(1), T(1)),
                    point2<T>(13, T(0), T(1))};
  result.polygon_orientation = bounded_planar_sign::positive;

  const std::array<std::array<T, 3>, 4> coordinates{{
      {{0.0, 0.0, 0.0}}, {{1.0, 0.0, 0.0}},
      {{1.0, 1.0, 0.0}}, {{0.0, 1.0, 0.0}}}};
  for (std::size_t edge = 0; edge < result.polygon.size(); ++edge) {
    const auto next = (edge + 1) % result.polygon.size();
    const auto feature = edge_feature(operand_id::b, 20 + edge);
    auto boundary_input = edge_input<T>(
        owner, operand_id::b, 20 + edge, result.polygon[edge].source_vertex,
        coordinates[edge][0], coordinates[edge][1], coordinates[edge][2],
        result.polygon[next].source_vertex, coordinates[next][0],
        coordinates[next][1], coordinates[next][2]);
    auto relation = classify_source_edge_relation(
        result.edge, boundary_input, owner, static_cast<T>(1.0e-6));
    check(relation.has_value(), "boundary edge relation construction");
    if (!relation.has_value())
      continue;
    source_edge_facet_boundary_relation<T> boundary;
    boundary.binding.request = relation_request_id(edge + 1);
    boundary.binding.feature = feature;
    boundary.binding.owner = {
        edge, result.polygon[edge].source_vertex,
        result.polygon[next].source_vertex};
    boundary.binding.parameter_source_vertices = {
        result.polygon[edge].source_vertex,
        result.polygon[next].source_vertex};
    boundary.relation = std::move(*relation.value());
    result.boundary_relations.push_back(std::move(boundary));
  }
  return result;
}

template <class T>
source_edge_facet_relation_record<T>
classify_or_fail(const char *name, source_edge_facet_input<T> input,
                 const context_owner_token &owner) {
  auto relation = classify_source_edge_facet_relation(
      input, owner, static_cast<T>(1.0e-6));
  check(relation.has_value(), std::string(name) + " should classify");
  if (!relation.has_value()) {
    if (relation.error())
      std::cerr << "  subcode=" << relation.error()->subcode
                << " summary=" << relation.error()->summary << '\n';
    return {};
  }
  check(valid_source_edge_facet_relation_record(*relation.value()),
        std::string(name) + " record validates");
  check(relation.value()->semantic_digest ==
            sha256::digest(encode_source_edge_facet_relation_semantics(
                *relation.value())),
        std::string(name) + " digest reproduces");
  return std::move(*relation.value());
}

void test_support_and_transverse_categories() {
  const auto owner = context_owner_token::create();
  auto separated = classify_or_fail(
      "same side", fixture(owner, 0.5, 0.5, 1.0, 0.5, 0.5, 2.0), owner);
  check(separated.support ==
            source_edge_facet_support_class::definitely_separated_same_side &&
            separated.contact == source_edge_facet_contact_class::none &&
            separated.events.empty(),
        "same-side support rejects transverse contact");

  auto crossing = classify_or_fail(
      "interior crossing", fixture(owner, 0.5, 0.5, -1.0, 0.5, 0.5, 1.0),
      owner);
  check(crossing.support ==
            source_edge_facet_support_class::transverse_support_crossing &&
            crossing.contact ==
                source_edge_facet_contact_class::proper_face_crossing &&
            crossing.events.size() == 1,
        "opposite endpoint signs produce one proper crossing");
  if (!crossing.events.empty()) {
    check(crossing.events[0].parameter.rounded_nominal == 0.5,
          "proper crossing parameter is canonical");
    check(crossing.events[0].region.classification ==
              source_facet_point_region_class::interior,
          "proper crossing classifies against the complete polygon");
    check(crossing.events[0].numeric_crossing != 0,
          "proper crossing retains signed occupancy transition");
  }

  auto boundary = classify_or_fail(
      "boundary crossing", fixture(owner, 0.0, 0.5, -1.0, 0.0, 0.5, 1.0),
      owner);
  check(boundary.contact ==
                source_edge_facet_contact_class::boundary_crossing &&
            boundary.events.size() == 1 &&
            boundary.events[0].numeric_crossing == 0,
        "source-boundary crossing defers numeric ownership to the source fan");

  auto outside = classify_or_fail(
      "outside crossing", fixture(owner, 2.0, 0.5, -1.0, 2.0, 0.5, 1.0),
      owner);
  check(outside.support ==
            source_edge_facet_support_class::transverse_support_crossing &&
            outside.contact == source_edge_facet_contact_class::none &&
            outside.events.empty(),
        "support-plane crossing outside the full polygon is not a contact");
}

void test_endpoint_categories() {
  const auto owner = context_owner_token::create();
  auto endpoint = classify_or_fail(
      "interior endpoint", fixture(owner, 0.5, 0.5, 0.0, 0.5, 0.5, 1.0),
      owner);
  check(endpoint.support ==
            source_edge_facet_support_class::endpoint_support_tie &&
            endpoint.contact ==
                source_edge_facet_contact_class::endpoint_contact &&
            endpoint.events.size() == 1,
        "exact endpoint support tie publishes endpoint contact");
  if (!endpoint.events.empty())
    check(endpoint.events[0].parameter.domain ==
              parameter_domain_status::stable_endpoint,
          "endpoint contact reuses exact source endpoint parameter");

  auto tangent = classify_or_fail(
      "boundary endpoint", fixture(owner, 0.0, 0.5, 0.0, 0.0, 0.5, 1.0),
      owner);
  check(tangent.contact == source_edge_facet_contact_class::tangent_contact &&
            tangent.events.size() == 1 &&
            tangent.events[0].region.classification ==
                source_facet_point_region_class::original_edge,
        "endpoint on original boundary is distinguished as tangent contact");
}

void test_coplanar_partition_categories() {
  const auto owner = context_owner_token::create();
  auto crossing = classify_or_fail(
      "coplanar crossing", fixture(owner, -0.5, 0.5, 0.0, 1.5, 0.5, 0.0),
      owner);
  check(crossing.support == source_edge_facet_support_class::coplanar_support &&
            crossing.has_coplanar_partition &&
            crossing.contact ==
                source_edge_facet_contact_class::coplanar_containment,
        "coplanar crossing is partitioned using every boundary relation");
  if (crossing.has_coplanar_partition) {
    check(crossing.coplanar_partition.boundary_edge_relation_count == 4,
          "coplanar partition records complete boundary coverage");
    check(crossing.coplanar_partition.triangle_reconciliation_complete,
          "coplanar partition completes triangle-local reconciliation");
  }

  auto overlap = classify_or_fail(
      "coplanar overlap", fixture(owner, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0),
      owner);
  check(overlap.contact ==
            source_edge_facet_contact_class::coplanar_boundary_overlap &&
            overlap.has_coplanar_partition,
        "collinear original-boundary overlap retains overlap ownership");
}

void test_fail_closed_mutations() {
  const auto owner = context_owner_token::create();
  auto input = fixture(owner, 0.5, 0.5, -1.0, 0.5, 0.5, 1.0);
  auto relation = classify_or_fail("mutation source", input, owner);
  relation.semantic_digest.bytes[0] ^= 0x80U;
  check(!valid_source_edge_facet_relation_record(relation),
        "semantic digest mutation is rejected");

  auto mismatched_binding = input;
  mismatched_binding.boundary_relations[0]
      .binding.parameter_source_vertices[0] = 999;
  auto mismatched = classify_source_edge_facet_relation(
      mismatched_binding, owner, 1.0e-10);
  check(!mismatched.has_value(),
        "boundary parameter/source ownership mismatch fails closed");

  input.boundary_relations.pop_back();
  auto missing = classify_source_edge_facet_relation(input, owner, 1.0e-10);
  check(!missing.has_value() && missing.error() &&
            missing.error()->subcode == static_cast<std::uint32_t>(
                relation_subcode::source_edge_facet_malformed),
        "incomplete original boundary coverage fails closed");

  auto wrong_owner = classify_source_edge_facet_relation(
      fixture(owner, 0.5, 0.5, -1.0, 0.5, 0.5, 1.0),
      context_owner_token::create(), 1.0e-10);
  check(!wrong_owner.has_value(), "wrong runtime owner fails closed");
}

void test_float_profile() {
  const auto owner = context_owner_token::create();
  auto crossing = classify_or_fail(
      "float interior crossing",
      fixture(owner, 0.5F, 0.5F, -1.0F, 0.5F, 0.5F, 1.0F), owner);
  check(crossing.contact ==
                source_edge_facet_contact_class::proper_face_crossing &&
            crossing.events.size() == 1 &&
            crossing.events[0].parameter.rounded_nominal == 0.5F,
        "float profile reproduces the transverse known answer");
}

} // namespace

int main() {
  test_support_and_transverse_categories();
  test_endpoint_categories();
  test_coplanar_partition_categories();
  test_fail_closed_mutations();
  test_float_profile();
  if (failures == 0) {
    std::cout << "Component 07 edge/facet kernel checks passed\n";
    return 0;
  }
  std::cerr << failures << " edge/facet checks failed\n";
  return 1;
}
