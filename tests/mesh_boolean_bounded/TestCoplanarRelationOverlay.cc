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
  result.facet.dropped_axis = 2;
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
std::uint64_t interval_count(
    const source_facet_coplanar_overlay_record<T> &record,
    source_facet_segment_interval_class classification) {
  std::uint64_t result = 0;
  for (const auto &partition : record.boundary_partitions)
    for (const auto &interval : partition.partition.intervals)
      if (interval.classification == classification)
        ++result;
  return result;
}


template <class T>
void check_topology_tables(
    const source_facet_coplanar_overlay_record<T> &record,
    const std::string &name) {
  check(record.complete_event_lineage &&
            record.complete_authorized_arc_coverage &&
            record.complete_overlap_component_assembly,
        name + " complete coplanar topology flags");
  for (std::size_t i = 0; i < record.event_nodes.size(); ++i) {
    const auto &node = record.event_nodes[i];
    check(node.id == i && !node.occurrences.empty() &&
              std::is_sorted(node.occurrences.begin(), node.occurrences.end()),
          name + " canonical event node");
  }
  for (std::size_t i = 0; i < record.oriented_arcs.size(); ++i) {
    const auto &arc = record.oriented_arcs[i];
    check(arc.id == i && arc.start_node < record.event_nodes.size() &&
              arc.end_node < record.event_nodes.size() &&
              arc.start_node != arc.end_node && !arc.occurrences.empty() &&
              std::is_sorted(arc.occurrences.begin(), arc.occurrences.end()),
          name + " canonical oriented arc");
  }
  for (std::size_t i = 0; i < record.overlap_components.size(); ++i) {
    const auto &component = record.overlap_components[i];
    check(component.id == i && !component.node_ids.empty() &&
              std::is_sorted(component.node_ids.begin(),
                             component.node_ids.end()) &&
              std::is_sorted(component.arc_ids.begin(),
                             component.arc_ids.end()),
          name + " canonical overlap component");
  }
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
  check_topology_tables(*overlay.value(), name);
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
  check(disjoint.complete_boundary_partition_coverage &&
            disjoint.boundary_partitions.size() == 8 &&
            interval_count(disjoint,
                           source_facet_segment_interval_class::interior) == 0,
        "disjoint overlay publishes complete outside boundary partitions");
  check(disjoint.event_nodes.empty() && disjoint.oriented_arcs.empty() &&
            disjoint.overlap_components.empty(),
        "disjoint overlay has no event topology");

  auto point = classify(
      "point", polygon<double>(owner, operand_id::a, 3, square(0, 0, 1, 1)),
      polygon<double>(owner, operand_id::b, 4, square(1, 1, 2, 2)), owner);
  check(point.classification == coplanar_facet_overlay_class::point_contact,
        "point-only coplanar contact");
  check(point.event_nodes.size() == 1 && point.oriented_arcs.empty() &&
            point.overlap_components.size() == 1 &&
            point.overlap_components[0].kind ==
                coplanar_overlap_component_kind::isolated_point &&
            point.overlap_components[0].sheet_mask == 3 &&
            !point.overlap_components[0].closed,
        "point contact is one distinct-sheet event component");

  auto segment = classify(
      "segment", polygon<double>(owner, operand_id::a, 5, square(0, 0, 1, 1)),
      polygon<double>(owner, operand_id::b, 6, square(1, 0, 2, 1)), owner);
  check(segment.classification ==
            coplanar_facet_overlay_class::segment_contact,
        "segment-only coplanar contact");
  check(interval_count(
            segment,
            source_facet_segment_interval_class::original_edge_overlap) != 0,
        "segment contact retains overlap intervals and source ownership");
  check(segment.event_nodes.size() == 2 &&
            segment.oriented_arcs.size() == 1 &&
            segment.oriented_arcs[0].kind ==
                coplanar_overlap_arc_kind::shared_boundary &&
            segment.oriented_arcs[0].occurrences.size() == 2 &&
            segment.oriented_arcs[0].overlap_lineages.size() == 1 &&
            segment.overlap_components.size() == 1 &&
            segment.overlap_components[0].kind ==
                coplanar_overlap_component_kind::boundary_segment &&
            segment.overlap_components[0].sheet_mask == 3 &&
            !segment.overlap_components[0].closed,
        "segment contact merges mirrored occurrences only by relation lineage");

  auto overlap = classify(
      "area overlap",
      polygon<double>(owner, operand_id::a, 7, square(0, 0, 2, 2)),
      polygon<double>(owner, operand_id::b, 8, square(1, -1, 3, 1)), owner);
  check(overlap.classification == coplanar_facet_overlay_class::area_overlap &&
            overlap.proper_crossing_count == 2,
        "crossing boundaries produce area overlap");
  check(interval_count(overlap,
                       source_facet_segment_interval_class::interior) == 4,
        "crossing overlay partitions retain four authorized interior arcs");
  check(overlap.event_nodes.size() == 4 &&
            overlap.oriented_arcs.size() == 4 &&
            overlap.overlap_components.size() == 1 &&
            overlap.overlap_components[0].kind ==
                coplanar_overlap_component_kind::area_boundary &&
            overlap.overlap_components[0].sheet_mask == 3 &&
            overlap.overlap_components[0].closed,
        "crossing overlap assembles one closed mixed-sheet boundary");

  auto containment = classify(
      "containment",
      polygon<double>(owner, operand_id::a, 9, square(0, 0, 4, 4)),
      polygon<double>(owner, operand_id::b, 10, square(1, 1, 2, 2)), owner);
  check(containment.classification ==
            coplanar_facet_overlay_class::first_contains_second,
        "strict containment is directed");
  check(interval_count(containment,
                       source_facet_segment_interval_class::interior) == 4,
        "contained facet boundary is completely classified as interior");
  check(containment.event_nodes.size() == 4 &&
            containment.oriented_arcs.size() == 4 &&
            containment.overlap_components.size() == 1 &&
            containment.overlap_components[0].kind ==
                coplanar_overlap_component_kind::area_boundary &&
            containment.overlap_components[0].sheet_mask == 2 &&
            containment.overlap_components[0].closed,
        "containment preserves the contained source-sheet boundary cycle");

  auto equal = classify(
      "equal",
      polygon<double>(owner, operand_id::a, 11, square(0, 0, 2, 2)),
      polygon<double>(owner, operand_id::b, 12, square(0, 0, 2, 2)), owner);
  check(equal.classification ==
            coplanar_facet_overlay_class::equal_same_orientation &&
            equal.distinct_sheet_occurrences,
        "equal geometry preserves distinct sheet occurrences");
  check(interval_count(
            equal,
            source_facet_segment_interval_class::original_edge_overlap) == 8,
        "equal facets retain complete double-sheet boundary partitions");
  check(equal.event_nodes.size() == 4 && equal.oriented_arcs.size() == 4 &&
            equal.overlap_components.size() == 1 &&
            equal.overlap_components[0].kind ==
                coplanar_overlap_component_kind::coincident_sheet_boundary &&
            equal.overlap_components[0].sheet_mask == 3 &&
            equal.overlap_components[0].closed,
        "equal facets retain one closed distinct-occurrence sheet cycle");
  for (const auto &arc : equal.oriented_arcs)
    check(arc.kind == coplanar_overlap_arc_kind::shared_boundary &&
              arc.occurrences.size() == 2 &&
              arc.occurrences[0].polygon != arc.occurrences[1].polygon &&
              arc.overlap_lineages.size() == 1,
          "equal shared arc retains two source-sheet occurrences");

  const std::vector<std::array<double, 2>> split_boundary{
      {{0, 0}}, {{2, 0}}, {{2, 2}}, {{1, 2}}, {{0, 2}}};
  auto equal_split = classify(
      "equal split boundary",
      polygon<double>(owner, operand_id::a, 15, square(0, 0, 2, 2)),
      polygon<double>(owner, operand_id::b, 16, split_boundary), owner);
  check(equal_split.classification ==
            coplanar_facet_overlay_class::equal_same_orientation &&
            equal_split.event_nodes.size() == 5 &&
            equal_split.oriented_arcs.size() == 5 &&
            equal_split.overlap_components.size() == 1 &&
            equal_split.overlap_components[0].closed,
        "equal facets with distinct source-boundary tessellations canonicalize");
  for (const auto &arc : equal_split.oriented_arcs)
    check(arc.kind == coplanar_overlap_arc_kind::shared_boundary &&
              arc.occurrences.size() == 2 &&
              arc.overlap_lineages.size() == 1,
          "split equality retains exact mirrored source-edge lineage");

  auto opposite = classify(
      "equal opposite",
      polygon<double>(owner, operand_id::a, 13, square(0, 0, 2, 2)),
      polygon<double>(owner, operand_id::b, 14, square(0, 0, 2, 2), true),
      owner);
  check(opposite.classification ==
            coplanar_facet_overlay_class::equal_opposite_orientation,
        "equal opposite sheets retain orientation relation");
  check(opposite.overlap_components.size() == 1 &&
            opposite.overlap_components[0].kind ==
                coplanar_overlap_component_kind::coincident_sheet_boundary &&
            opposite.overlap_components[0].closed,
        "opposite coincident sheets retain a closed occurrence cycle");
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
    check(!valid_coplanar_overlay_record(forged) &&
              !verify_coplanar_overlay_record(forged),
          "self-consistent forged classification is independently rejected");

    auto split_event = *overlay.value();
    check(!split_event.event_nodes.empty() &&
              split_event.event_nodes.front().occurrences.size() >= 2,
          "split-event mutation source is available");
    if (!split_event.event_nodes.empty() &&
        split_event.event_nodes.front().occurrences.size() >= 2) {
      auto occurrence = split_event.event_nodes.front().occurrences.back();
      split_event.event_nodes.front().occurrences.pop_back();
      coplanar_overlap_event_node<double> extra;
      extra.id = split_event.event_nodes.size();
      extra.occurrences = {std::move(occurrence)};
      extra.representative = split_event.event_nodes.front().representative;
      split_event.event_nodes.push_back(std::move(extra));
      split_event.semantic_digest = sha256::digest(
          encode_coplanar_overlay_semantics(split_event));
      check(!valid_coplanar_overlay_record(split_event),
            "split exact event-equivalence class is rejected after digest repair");
    }

    auto forged_lineage = *overlay.value();
    check(!forged_lineage.oriented_arcs.empty(),
          "shared-lineage mutation source is available");
    if (!forged_lineage.oriented_arcs.empty()) {
      forged_lineage.oriented_arcs.front().overlap_lineages = {
          relation_request_id(999999)};
      forged_lineage.semantic_digest = sha256::digest(
          encode_coplanar_overlay_semantics(forged_lineage));
      check(!valid_coplanar_overlay_record(forged_lineage),
            "forged shared-boundary lineage is rejected after digest repair");
    }

    auto self_loop = *overlay.value();
    if (!self_loop.oriented_arcs.empty()) {
      self_loop.oriented_arcs.front().end_node =
          self_loop.oriented_arcs.front().start_node;
      self_loop.semantic_digest =
          sha256::digest(encode_coplanar_overlay_semantics(self_loop));
      check(!valid_coplanar_overlay_record(self_loop),
            "self-loop boundary arc is rejected after digest repair");
    }

    auto open_component = *overlay.value();
    if (!open_component.overlap_components.empty()) {
      open_component.overlap_components.front().closed = false;
      open_component.semantic_digest = sha256::digest(
          encode_coplanar_overlay_semantics(open_component));
      check(!valid_coplanar_overlay_record(open_component),
            "open coincident-sheet component is rejected after digest repair");
    }

    auto missing_interval = *overlay.value();
    if (!missing_interval.oriented_arcs.empty()) {
      missing_interval.oriented_arcs.pop_back();
      missing_interval.semantic_digest = sha256::digest(
          encode_coplanar_overlay_semantics(missing_interval));
      check(!valid_coplanar_overlay_record(missing_interval),
            "missing authorized interval arc is rejected after digest repair");
    }
    auto partition_mutation = *overlay.value();
    partition_mutation.boundary_partitions.front()
        .partition.semantic_digest.bytes[0] ^= 0x80U;
    partition_mutation.semantic_digest = sha256::digest(
        encode_coplanar_overlay_semantics(partition_mutation));
    check(!valid_coplanar_overlay_record(partition_mutation),
          "boundary partition mutation is rejected after outer digest repair");
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
