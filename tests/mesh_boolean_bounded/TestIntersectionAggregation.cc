#include "YgorMeshesBooleanBounded/IntersectionAggregation.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

using namespace ygor::mesh_boolean::bounded;

namespace {

void require(bool value) {
  if (!value)
    std::abort();
}

relation_feature_key feature(operand_id operand, relation_feature_kind kind,
                             std::uint64_t primary) {
  relation_feature_key key;
  key.operand = operand;
  key.kind = kind;
  key.primary = primary;
  return key;
}

relation_event_seed_record seed(std::uint64_t ordinal,
                                feature_relation_status status,
                                relation_contact_dimension dimension,
                                std::int32_t numeric,
                                std::int8_t symbolic,
                                operand_id owner) {
  relation_event_seed_record value;
  value.id = relation_event_seed_id{ordinal};
  value.key.semantic_namespace.bytes[0] = 0x66;
  value.key.family = feature_relation_family::symbolic_contact;
  value.key.first =
      feature(operand_id::a, relation_feature_kind::source_facet, 10);
  value.key.second =
      feature(operand_id::b, relation_feature_kind::source_facet, 20);
  value.key.occurrence = static_cast<std::uint32_t>(ordinal + 1);
  value.source_relation = feature_relation_id{ordinal};
  value.contact_status = status;
  value.contact_dimension = dimension;
  value.numeric_crossing = numeric;
  value.symbolic_crossing = symbolic;
  value.half_open_owner = owner;
  value.precision_evidence_complete = true;
  return value;
}

void add_record(event_incidence_tables &tables,
                event_incidence_kind kind, event_id event,
                event_occurrence_id occurrence, event_seed_binding_id binding,
                relation_feature_key source_feature,
                std::uint64_t primary, std::uint64_t secondary,
                std::int32_t numeric, std::int8_t symbolic) {
  event_incidence_record record;
  record.id = event_incidence_id{tables.records.size()};
  record.kind = kind;
  record.event = event;
  record.occurrence = occurrence;
  record.seed_binding = binding;
  record.feature = source_feature;
  record.payload_primary = primary;
  record.payload_secondary = secondary;
  record.numeric_crossing = numeric;
  record.symbolic_crossing = symbolic;
  record.source_feature_owner =
      kind == event_incidence_kind::source_facet ||
      kind == event_incidence_kind::source_shell;
  tables.records.push_back(record);
}

void add_seed_records(event_incidence_tables &tables,
                      const relation_event_seed_record &seed_record,
                      event_id event, event_occurrence_id occurrence,
                      relation_feature_key facet,
                      relation_feature_key shell) {
  const event_seed_binding_id binding{seed_record.id.ordinal()};
  if (seed_record.numeric_crossing != 0 ||
      seed_record.symbolic_crossing != 0)
    add_record(tables, event_incidence_kind::crossing_contribution, event,
               occurrence, binding, relation_feature_key{}, 0, 0,
               seed_record.numeric_crossing, seed_record.symbolic_crossing);
  add_record(tables, event_incidence_kind::descriptor_precursor, event,
             occurrence, binding, relation_feature_key{},
             static_cast<std::uint64_t>(seed_record.contact_status),
             static_cast<std::uint64_t>(seed_record.contact_dimension),
             seed_record.numeric_crossing, seed_record.symbolic_crossing);
  add_record(tables, event_incidence_kind::source_facet, event, occurrence,
             binding, facet, 0, 0, 0, 0);
  add_record(tables, event_incidence_kind::source_shell, event, occurrence,
             binding, shell, 0, 0, 0, 0);
}

const crossing_aggregate_record *find_crossing(
    const intersection_aggregate_tables &tables,
    intersection_aggregate_locus locus, std::uint64_t ordinal) {
  for (const auto &record : tables.crossing)
    if (record.locus == locus && record.locus_ordinal == ordinal)
      return &record;
  return nullptr;
}

const contact_aggregate_record *find_contact(
    const intersection_aggregate_tables &tables,
    intersection_aggregate_locus locus, std::uint64_t ordinal,
    feature_relation_status status) {
  for (const auto &record : tables.contact)
    if (record.locus == locus && record.locus_ordinal == ordinal &&
        record.contact_status == status)
      return &record;
  return nullptr;
}

} // namespace

int main() {
  std::vector<relation_event_seed_record> seeds{
      seed(0, feature_relation_status::proper_crossing,
           relation_contact_dimension::point, 1, 0, operand_id::a),
      seed(1, feature_relation_status::proper_crossing,
           relation_contact_dimension::point, -1, 0, operand_id::b),
      seed(2, feature_relation_status::tangency,
           relation_contact_dimension::point, 0, 0, operand_id::a),
      seed(3, feature_relation_status::point_contact,
           relation_contact_dimension::point, 0, 1, operand_id::b),
      seed(4, feature_relation_status::coincidence_opposite_orientation,
           relation_contact_dimension::area, 0, 0, operand_id::b)};

  event_interning_tables interning;
  interning.events.resize(2);
  interning.events[0].id = event_id{0};
  interning.events[1].id = event_id{1};
  interning.occurrences.resize(3);
  interning.occurrences[0].id = event_occurrence_id{0};
  interning.occurrences[0].event = event_id{0};
  interning.occurrences[1].id = event_occurrence_id{1};
  interning.occurrences[1].event = event_id{0};
  interning.occurrences[2].id = event_occurrence_id{2};
  interning.occurrences[2].event = event_id{1};
  interning.seed_bindings.resize(seeds.size());
  const event_id event_ids[] = {event_id{0}, event_id{0}, event_id{0},
                                event_id{1}, event_id{1}};
  const event_occurrence_id occurrence_ids[] = {
      event_occurrence_id{0}, event_occurrence_id{0}, event_occurrence_id{1},
      event_occurrence_id{2}, event_occurrence_id{2}};
  for (std::size_t i = 0; i < seeds.size(); ++i) {
    auto &binding = interning.seed_bindings[i];
    binding.id = event_seed_binding_id{i};
    binding.seed = relation_event_seed_id{i};
    binding.event = event_ids[i];
    binding.occurrence = occurrence_ids[i];
  }

  event_incidence_tables incidence;
  const auto facet_a =
      feature(operand_id::a, relation_feature_kind::source_facet, 10);
  const auto facet_b =
      feature(operand_id::b, relation_feature_kind::source_facet, 20);
  const auto shell_a =
      feature(operand_id::a, relation_feature_kind::sheet_occurrence, 100);
  const auto shell_b =
      feature(operand_id::b, relation_feature_kind::sheet_occurrence, 200);
  for (std::size_t i = 0; i < seeds.size(); ++i)
    add_seed_records(incidence, seeds[i], event_ids[i], occurrence_ids[i],
                     i < 3 ? facet_a : facet_b,
                     i < 3 ? shell_a : shell_b);
  incidence.event_ranges.resize(interning.events.size());
  incidence.occurrence_ranges.resize(interning.occurrences.size());
  incidence.seed_ranges.resize(interning.seed_bindings.size());

  source_edge_arrangement_tables source_edges;
  source_edge_sequence_record sequence;
  sequence.id = source_edge_sequence_id{0};
  sequence.source_edge =
      feature(operand_id::a, relation_feature_kind::source_edge, 5);
  sequence.memberships = {0, 2};
  source_edges.sequences.push_back(sequence);
  for (std::uint64_t i = 0; i != 2; ++i) {
    source_edge_membership_record membership;
    membership.id = source_edge_membership_id{i};
    membership.occurrence = event_occurrence_id{i};
    source_edges.memberships.push_back(membership);
    source_edges.membership_sequence_index.push_back(membership.id);
  }
  source_edge_cluster_record source_cluster;
  source_cluster.id = source_edge_cluster_id{0};
  source_cluster.sequence = source_edge_sequence_id{0};
  source_cluster.key.source_edge = sequence.source_edge;
  source_cluster.member_occurrences = {0, 2};
  source_edges.clusters.push_back(source_cluster);
  source_edges.cluster_occurrence_index = {event_occurrence_id{0},
                                            event_occurrence_id{1}};
  source_edge_interval_record source_interval;
  source_interval.id = source_edge_interval_id{0};
  source_interval.sequence = source_edge_sequence_id{0};
  source_interval.key.source_edge = sequence.source_edge;
  source_interval.key.left.kind = boundary_reference_kind::cluster;
  source_interval.key.left.cluster = source_cluster.key;
  source_interval.key.right.kind = boundary_reference_kind::end_sentinel;
  source_edges.intervals.push_back(source_interval);

  transverse_carrier_arrangement_tables transverse;
  for (std::uint64_t i = 0; i != 2; ++i) {
    carrier_cluster_record cluster;
    cluster.id = carrier_cluster_id{i};
    cluster.carrier = transverse_carrier_id{0};
    cluster.occurrence_members = {i, 1};
    transverse.clusters.push_back(cluster);
  }
  transverse.cluster_occurrence_index = {event_occurrence_id{0},
                                           event_occurrence_id{2}};
  carrier_active_span_record span;
  span.id = carrier_active_span_id{0};
  span.carrier = transverse_carrier_id{0};
  span.left = carrier_cluster_id{0};
  span.right = carrier_cluster_id{1};
  span.activation =
      intersection_span_activation::active_transverse_intersection;
  transverse.spans.push_back(span);

  coplanar_carrier_arrangement_tables coplanar;
  collinear_overlap_carrier_record overlap_carrier;
  overlap_carrier.id = collinear_overlap_carrier_id{0};
  overlap_carrier.start_occurrence = event_occurrence_id{1};
  overlap_carrier.end_occurrence = event_occurrence_id{2};
  coplanar.carriers.push_back(overlap_carrier);
  coplanar_overlap_record overlap;
  overlap.id = coplanar_overlap_record_id{0};
  overlap.boundary_events = {0, 2};
  coplanar.overlaps.push_back(overlap);
  coplanar.overlap_boundary_event_index = {event_occurrence_id{1},
                                            event_occurrence_id{2}};

  intersection_aggregate_tables tables;
  bounded_boolean_error error;
  require(build_intersection_aggregates(seeds, interning, incidence,
                                        source_edges, transverse, coplanar,
                                        tables, error));
  require(verify_intersection_aggregates(seeds, interning, incidence,
                                         source_edges, transverse, coplanar,
                                         tables, error));

  const auto *event_zero = find_crossing(
      tables, intersection_aggregate_locus::conceptual_event, 0);
  require(event_zero != nullptr);
  require(event_zero->numeric_signed_sum == 0);
  require(event_zero->entering_count == 1);
  require(event_zero->leaving_count == 1);
  require(event_zero->members.count == 2);
  require(event_zero->zero_net_contact_retained);
  require(event_zero->conserved && event_zero->member_order_verified);
  require(event_zero->facet_subtotals.count == 1);
  require(event_zero->shell_subtotals.count == 1);

  const auto *symbolic = find_crossing(
      tables, intersection_aggregate_locus::event_occurrence, 2);
  require(symbolic != nullptr);
  require(symbolic->numeric_signed_sum == 0);
  require(symbolic->symbolic_signed_sum == 1);
  require(symbolic->symbolic_owner == operand_id::b);
  require(symbolic->symbolic_owner_mask == 2);

  const auto *tangent = find_contact(
      tables, intersection_aggregate_locus::event_occurrence, 1,
      feature_relation_status::tangency);
  require(tangent != nullptr);
  require(tangent->zero_net_retained);
  require(tangent->tangent_retained);
  require(tangent->members.count == 1);

  const auto *coincident = find_contact(
      tables, intersection_aggregate_locus::event_occurrence, 2,
      feature_relation_status::coincidence_opposite_orientation);
  require(coincident != nullptr);
  require(coincident->coincidence_retained);
  require(coincident->members.count == 1);

  require(find_crossing(tables, intersection_aggregate_locus::source_edge, 0) !=
          nullptr);
  require(find_crossing(tables,
                        intersection_aggregate_locus::source_edge_interval,
                        0) != nullptr);
  require(find_crossing(tables, intersection_aggregate_locus::carrier_span,
                        0) != nullptr);
  require(find_crossing(tables,
                        intersection_aggregate_locus::overlap_boundary,
                        1) != nullptr);
  require(find_crossing(tables,
                        intersection_aggregate_locus::coplanar_component,
                        0) != nullptr);

  auto mutated = tables;
  mutated.crossing.front().numeric_signed_sum += 1;
  require(!verify_intersection_aggregates(
      seeds, interning, incidence, source_edges, transverse, coplanar, mutated,
      error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::verifier_rejection));

  mutated = tables;
  require(mutated.crossing_members.size() >= 2);
  std::swap(mutated.crossing_members[0], mutated.crossing_members[1]);
  require(!verify_intersection_aggregates(
      seeds, interning, incidence, source_edges, transverse, coplanar, mutated,
      error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::verifier_rejection));

  auto malformed_incidence = incidence;
  malformed_incidence.records.front().kind =
      static_cast<event_incidence_kind>(255);
  require(!build_intersection_aggregates(
      seeds, interning, malformed_incidence, source_edges, transverse, coplanar,
      mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::aggregate_mismatch));

  malformed_incidence = incidence;
  malformed_incidence.records.front().reserved16 = 1;
  require(!build_intersection_aggregates(
      seeds, interning, malformed_incidence, source_edges, transverse, coplanar,
      mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::aggregate_mismatch));

  auto malformed_source_edges = source_edges;
  malformed_source_edges.cluster_occurrence_index.front() =
      event_occurrence_id{intersection_invalid_ordinal};
  require(!build_intersection_aggregates(
      seeds, interning, incidence, malformed_source_edges, transverse, coplanar,
      mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::aggregate_mismatch));
  return 0;
}
