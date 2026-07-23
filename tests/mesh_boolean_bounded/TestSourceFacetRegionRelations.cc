#include "YgorMeshesBooleanBounded/SourceFacetRegionSegmentBuild.h"

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

finite_interval<double> parameter(double value) {
  return *finite_interval<double>::checked_singleton(value);
}

source_facet_boundary_edge_owner edge_owner(
    const std::vector<projected_source_point<double>> &polygon,
    std::uint64_t ordinal) {
  return {ordinal, polygon[ordinal].source_vertex,
          polygon[(ordinal + 1) % polygon.size()].source_vertex};
}

std::vector<projected_source_point<double>> square() {
  return {point(10, 0.0, 0.0), point(11, 1.0, 0.0),
          point(12, 1.0, 1.0), point(13, 0.0, 1.0)};
}

source_facet_segment_contact_proposal<double> point_contact(
    std::uint64_t lineage, double t,
    const projected_source_point<double> &contact_point,
    std::vector<std::uint64_t> vertex_owners,
    std::vector<source_facet_boundary_edge_owner> edge_owners) {
  source_facet_segment_contact_proposal<double> result;
  result.kind = source_facet_segment_contact_kind::point_contact;
  result.lineage = lineage;
  result.first_rounded_parameter = t;
  result.first_parameter = parameter(t);
  result.first_point = contact_point;
  result.first_source_vertex_owners = std::move(vertex_owners);
  result.first_source_edge_owners = std::move(edge_owners);
  result.second_rounded_parameter = t;
  result.second_parameter = parameter(t);
  result.second_point = contact_point;
  return result;
}

source_facet_segment_contact_proposal<double> overlap_contact(
    std::uint64_t lineage, double first_t,
    const projected_source_point<double> &first_point,
    std::vector<std::uint64_t> first_vertex_owners,
    double second_t,
    const projected_source_point<double> &second_point,
    std::vector<std::uint64_t> second_vertex_owners,
    source_facet_boundary_edge_owner owner) {
  source_facet_segment_contact_proposal<double> result;
  result.kind = source_facet_segment_contact_kind::boundary_overlap;
  result.lineage = lineage;
  result.first_rounded_parameter = first_t;
  result.first_parameter = parameter(first_t);
  result.first_point = first_point;
  result.first_point_source_identity_valid =
      !first_vertex_owners.empty();
  result.first_source_vertex_owners =
      std::move(first_vertex_owners);
  result.first_source_edge_owners = {owner};
  result.second_rounded_parameter = second_t;
  result.second_parameter = parameter(second_t);
  result.second_point = second_point;
  result.second_point_source_identity_valid =
      !second_vertex_owners.empty();
  result.second_source_vertex_owners =
      std::move(second_vertex_owners);
  result.second_source_edge_owners = {owner};
  result.overlap_source_edge_owners = {owner};
  return result;
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
      point(20, 0.0, 0.0), point(21, 2.0, 0.0),
      point(22, 2.0, 2.0), point(23, 1.0, 1.0),
      point(24, 0.0, 2.0)};
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

void test_segment_crossing_partition() {
  const auto polygon = square();
  const auto start = point(1000, -1.0, 0.5);
  const auto end = point(1001, 2.0, 0.5);
  std::vector<source_facet_segment_contact_proposal<double>> contacts;
  contacts.push_back(point_contact(
      22, 2.0 / 3.0, point(0, 1.0, 0.5), {},
      {edge_owner(polygon, 1)}));
  contacts.push_back(point_contact(
      11, 1.0 / 3.0, point(0, 0.0, 0.5), {},
      {edge_owner(polygon, 3)}));

  auto result = partition_source_facet_segment(
      4, 2, start, false, end, false, polygon,
      bounded_planar_sign::positive, polygon.size(), true,
      std::move(contacts));
  check(result.has_value(),
        "transverse segment should partition against the complete polygon");
  if (!result.has_value())
    return;

  const auto &record = *result.value();
  check(valid_source_facet_segment_partition_record(record),
        "segment partition independently validates");
  check(record.contacts.size() == 2 &&
            record.contacts.front().lineage == 11 &&
            record.contacts.back().lineage == 22,
        "contact proposals are canonically ordered by proven parameter order");
  check(record.breakpoints.size() == 4 && record.intervals.size() == 3,
        "two point contacts create four breakpoints and three open cells");
  check(record.intervals[0].classification ==
            source_facet_segment_interval_class::outside &&
            record.intervals[1].classification ==
                source_facet_segment_interval_class::interior &&
            record.intervals[2].classification ==
                source_facet_segment_interval_class::outside,
        "open interval witnesses preserve outside/interior/outside order");
  check(record.breakpoints[1].region.source_edge_owners.front().edge_ordinal ==
            3 &&
            record.breakpoints[2].region.source_edge_owners.front().edge_ordinal ==
                1,
        "breakpoints preserve original source-boundary ownership");

  auto mutated = record;
  mutated.intervals[1].classification =
      source_facet_segment_interval_class::outside;
  check(!valid_source_facet_segment_partition_record(mutated),
        "mutated public interval semantics must invalidate the digest and invariants");
}

void test_segment_boundary_overlap_partition() {
  const auto polygon = square();
  const auto start = point(1100, -0.5, 0.0);
  const auto end = point(1101, 1.5, 0.0);
  std::vector<source_facet_segment_contact_proposal<double>> contacts;
  contacts.push_back(overlap_contact(
      31, 0.25, point(10, 0.0, 0.0), {10}, 0.75,
      point(11, 1.0, 0.0), {11}, edge_owner(polygon, 0)));

  auto result = partition_source_facet_segment(
      4, 2, start, false, end, false, polygon,
      bounded_planar_sign::positive, polygon.size(), true,
      std::move(contacts));
  check(result.has_value(),
        "coplanar boundary overlap should produce a partition");
  if (!result.has_value())
    return;

  const auto &record = *result.value();
  check(record.intervals.size() == 3,
        "one boundary overlap creates three open cells");
  check(record.intervals[1].classification ==
            source_facet_segment_interval_class::original_edge_overlap,
        "middle cell retains explicit boundary-overlap classification");
  check(record.intervals[1].source_edge_owners.size() == 1 &&
            record.intervals[1].source_edge_owners.front().edge_ordinal == 0,
        "boundary overlap retains the original source edge owner");
  check(record.breakpoints[1].region.classification ==
            source_facet_point_region_class::original_vertex &&
            record.breakpoints[2].region.classification ==
                source_facet_point_region_class::original_vertex,
        "overlap endpoints retain source-vertex ownership");
}

void test_segment_parameter_order_fails_closed() {
  const auto polygon = square();
  const auto start = point(1200, -1.0, 0.5);
  const auto end = point(1201, 2.0, 0.5);

  auto first = point_contact(
      41, 0.4, point(0, 0.0, 0.5), {},
      {edge_owner(polygon, 3)});
  first.first_parameter =
      *finite_interval<double>::create(0.3, 0.5);
  first.second_parameter = first.first_parameter;
  auto second = point_contact(
      42, 0.5, point(0, 1.0, 0.5), {},
      {edge_owner(polygon, 1)});
  second.first_parameter =
      *finite_interval<double>::create(0.4, 0.6);
  second.second_parameter = second.first_parameter;

  auto result = partition_source_facet_segment(
      4, 2, start, false, end, false, polygon,
      bounded_planar_sign::positive, polygon.size(), true,
      {first, second});
  check(!result.has_value(),
        "overlapping non-exact parameter enclosures must fail closed");
  check(result.error() &&
            result.error()->subcode == static_cast<std::uint32_t>(
                relation_subcode::source_facet_segment_order_unresolved),
        "unresolved parameter order has a stable Component 07 subcode");
}

source_facet_segment_partition_record<double> crossing_partition() {
  const auto polygon = square();
  const auto start = point(1300, -1.0, 0.5);
  const auto end = point(1301, 2.0, 0.5);
  auto result = partition_source_facet_segment(
      4, 2, start, false, end, false, polygon,
      bounded_planar_sign::positive, polygon.size(), true,
      {point_contact(51, 1.0 / 3.0, point(0, 0.0, 0.5), {},
                     {edge_owner(polygon, 3)}),
       point_contact(52, 2.0 / 3.0, point(0, 1.0, 0.5), {},
                     {edge_owner(polygon, 1)})});
  return result.has_value()
             ? *result.value()
             : source_facet_segment_partition_record<double>{};
}

void test_triangle_local_reconciliation_and_retriangulation() {
  auto first = crossing_partition();
  auto second = crossing_partition();
  check(valid_source_facet_segment_partition_record(first) &&
            valid_source_facet_segment_partition_record(second),
        "qualification partitions should be valid before reconciliation");
  if (!valid_source_facet_segment_partition_record(first) ||
      !valid_source_facet_segment_partition_record(second))
    return;

  source_facet_triangle_local_witness<double> a;
  a.triangle = 101;
  a.local_witness = 1;
  a.local_edge_role =
      source_triangle_edge_role::facet_internal_diagonal;
  a.absorption =
      source_facet_triangle_absorption_kind::bookkeeping_only;
  a.public_index = 1;
  a.parameter = parameter(0.5);
  a.semantic_classification =
      source_facet_point_region_class::interior;
  a.internal_diagonal = 7001;
  a.exact_triangulation_digest.bytes[0] = 1;

  auto b = a;
  b.triangle = 202;
  b.internal_diagonal = 8002;
  b.exact_triangulation_digest.bytes[0] = 2;

  auto reconciled_a =
      reconcile_source_facet_triangle_local_witnesses(
          std::move(first), {a});
  auto reconciled_b =
      reconcile_source_facet_triangle_local_witnesses(
          std::move(second), {b});
  check(reconciled_a.has_value() && reconciled_b.has_value(),
        "distinct legal triangulations should reconcile");
  if (!reconciled_a.has_value() || !reconciled_b.has_value())
    return;

  check(reconciled_a.value()->semantic_digest ==
            reconciled_b.value()->semantic_digest,
        "triangle-local bookkeeping is excluded from the semantic digest");
  check(equivalent_source_facet_segment_semantics(
            *reconciled_a.value(), *reconciled_b.value()),
        "alternative triangulations preserve complete public partition semantics");

  auto invalid = crossing_partition();
  a.absorption =
      source_facet_triangle_absorption_kind::public_breakpoint;
  a.public_index = 1;
  auto rejected =
      reconcile_source_facet_triangle_local_witnesses(
          std::move(invalid), {a});
  check(!rejected.has_value(),
        "an internal diagonal cannot own a public breakpoint");
  check(rejected.error() &&
            rejected.error()->subcode == static_cast<std::uint32_t>(
                relation_subcode::source_facet_triangle_reconciliation),
        "internal-diagonal ownership rejection has a stable subcode");
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
  test_segment_crossing_partition();
  test_segment_boundary_overlap_partition();
  test_segment_parameter_order_fails_closed();
  test_triangle_local_reconciliation_and_retriangulation();
  if (failures != 0)
    std::cerr << failures
              << " Component 07 source-facet region checks failed\n";
  return failures == 0 ? 0 : 1;
}
