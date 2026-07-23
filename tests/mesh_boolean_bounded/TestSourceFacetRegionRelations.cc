#include "YgorMeshesBooleanBounded/SourceFacetRegionKernel.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace ygor::mesh_boolean::bounded;

namespace {
int failures = 0;
void check(bool condition, const char *message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

projected_source_point<double> point(std::uint64_t id, double x, double y) {
  projected_source_point<double> result;
  result.source_vertex = id;
  result.nominal = {x, y};
  result.enclosure = {*finite_interval<double>::checked_singleton(x),
                      *finite_interval<double>::checked_singleton(y)};
  return result;
}

std::vector<projected_source_point<double>> square() {
  return {point(10, 0.0, 0.0), point(11, 1.0, 0.0),
          point(12, 1.0, 1.0), point(13, 0.0, 1.0)};
}

void test_interior_and_outside() {
  const auto polygon = square();
  auto interior = classify_source_facet_point(
      4, 2, point(99, 0.5, 0.5), false, polygon,
      bounded_planar_sign::positive);
  check(interior.has_value(), "square interior should classify");
  if (interior.has_value()) {
    check(interior.value()->classification ==
              source_facet_point_region_class::interior,
          "square interior class is stable");
    check(interior.value()->complete_boundary_traversal,
          "interior classification traverses the complete boundary");
    check(valid_source_facet_point_region_record(*interior.value()),
          "interior region record independently validates");
    check(interior.value()->boundary_test_count == polygon.size(),
          "one orientation record is retained per original boundary edge");
    check(interior.value()->parity_crossing_count == 1,
          "half-open parity counts one positive-ray crossing");
    auto mutated = *interior.value();
    mutated.orientation_evidence.front().formula_version = 0;
    check(!valid_source_facet_point_region_record(mutated),
          "mutated edge-orientation evidence must fail validation");
    mutated = *interior.value();
    mutated.polygon_orientation_evidence.bounded_sign =
        bounded_planar_sign::uncertain;
    check(!valid_source_facet_point_region_record(mutated),
          "mutated polygon-orientation evidence must fail validation");
  }

  auto outside = classify_source_facet_point(
      4, 2, point(100, 1.5, 0.5), false, polygon,
      bounded_planar_sign::positive);
  check(outside.has_value(), "square outside should classify");
  if (outside.has_value())
    check(outside.value()->classification ==
              source_facet_point_region_class::outside,
          "square outside class is stable");
}

void test_concave_and_reversed_rings() {
  const std::vector<projected_source_point<double>> concave{
      point(20, 0.0, 0.0), point(21, 2.0, 0.0), point(22, 2.0, 2.0),
      point(23, 1.0, 1.0), point(24, 0.0, 2.0)};
  auto lobe = classify_source_facet_point(
      8, 3, point(200, 1.5, 1.25), false, concave,
      bounded_planar_sign::positive);
  check(lobe.has_value() &&
            lobe.value()->classification ==
                source_facet_point_region_class::interior,
        "concave lobe should classify as interior");
  auto notch = classify_source_facet_point(
      8, 3, point(201, 1.0, 1.75), false, concave,
      bounded_planar_sign::positive);
  check(notch.has_value() &&
            notch.value()->classification ==
                source_facet_point_region_class::outside,
        "concave notch should classify as outside");

  auto reversed = square();
  std::reverse(reversed.begin(), reversed.end());
  auto reversed_interior = classify_source_facet_point(
      4, 2, point(202, 0.5, 0.5), false, reversed,
      bounded_planar_sign::negative);
  check(reversed_interior.has_value() &&
            reversed_interior.value()->classification ==
                source_facet_point_region_class::interior,
        "parity classification is stable under ring reversal");
}

void test_boundary_ownership() {
  const auto polygon = square();
  auto edge = classify_source_facet_point(
      4, 2, point(99, 1.0, 0.5), false, polygon,
      bounded_planar_sign::positive);
  check(edge.has_value(), "exact point on original edge should classify");
  if (edge.has_value()) {
    check(edge.value()->classification ==
              source_facet_point_region_class::original_edge,
          "point on source edge retains edge class");
    check(valid_source_facet_point_region_record(*edge.value()),
          "source-edge region record independently validates");
    check(edge.value()->source_edge_owners.size() == 1 &&
              edge.value()->source_edge_owners.front().edge_ordinal == 1,
          "point on source edge retains canonical boundary owner");
  }

  auto vertex = classify_source_facet_point(
      4, 2, point(10, 0.0, 0.0), true, polygon,
      bounded_planar_sign::positive);
  check(vertex.has_value(), "source vertex identity should classify");
  if (vertex.has_value()) {
    check(vertex.value()->classification ==
              source_facet_point_region_class::original_vertex,
          "source vertex identity retains vertex class");
    check(valid_source_facet_point_region_record(*vertex.value()),
          "source-vertex region record independently validates");
    check(vertex.value()->source_vertex_owners.size() == 1 &&
              vertex.value()->source_vertex_owners.front() == 10,
          "source vertex owner is retained");
    check(vertex.value()->source_edge_owners.size() == 2,
          "both incident original boundary edges are retained");
  }
}

void test_uncertainty_fails_closed() {
  auto polygon = square();
  auto query = point(99, 1.0, 0.5);
  query.enclosure[0] = *finite_interval<double>::create(0.9, 1.1);
  auto result = classify_source_facet_point(
      4, 2, query, false, polygon, bounded_planar_sign::positive);
  check(!result.has_value(),
        "boundary uncertainty that changes region semantics must fail closed");
  check(result.error() &&
            result.error()->subcode == static_cast<std::uint32_t>(
                                         relation_subcode::source_facet_region_unresolved),
        "unresolved boundary has stable Component 07 failure");
}

void test_orientation_mismatch_rejected() {
  const auto polygon = square();
  auto result = classify_source_facet_point(
      4, 2, point(300, 0.5, 0.5), false, polygon,
      bounded_planar_sign::negative);
  check(!result.has_value(),
        "frozen orientation that disagrees with the polygon must fail");
  check(result.error() &&
            result.error()->subcode == static_cast<std::uint32_t>(
                                         relation_subcode::malformed_source_polygon),
        "orientation mismatch has stable Component 07 failure");
}

void test_orientation_uncertainty_fails_closed() {
  auto polygon = square();
  polygon[2].enclosure[0] = *finite_interval<double>::create(-2.0, 2.0);
  auto result = classify_source_facet_point(
      4, 2, point(301, 0.5, 0.5), false, polygon,
      bounded_planar_sign::positive);
  check(!result.has_value(),
        "uncertain complete-polygon orientation must fail closed");
  check(result.error() &&
            result.error()->subcode == static_cast<std::uint32_t>(
                                         relation_subcode::source_facet_region_unresolved),
        "orientation uncertainty has stable Component 07 failure");
}

void test_identity_geometry_mismatch_rejected() {
  const auto polygon = square();
  auto query = point(10, 0.25, 0.25);
  auto result = classify_source_facet_point(
      4, 2, query, true, polygon, bounded_planar_sign::positive);
  check(!result.has_value(),
        "source identity with different accepted geometry must fail");
  check(result.error() &&
            result.error()->subcode == static_cast<std::uint32_t>(
                                         relation_subcode::source_facet_boundary_ownership),
        "identity mismatch has stable Component 07 failure");
}
} // namespace

int main() {
  test_interior_and_outside();
  test_concave_and_reversed_rings();
  test_boundary_ownership();
  test_uncertainty_fails_closed();
  test_orientation_mismatch_rejected();
  test_orientation_uncertainty_fails_closed();
  test_identity_geometry_mismatch_rejected();
  if (failures != 0)
    std::cerr << failures << " Component 07 source-facet region checks failed\n";
  return failures == 0 ? 0 : 1;
}
