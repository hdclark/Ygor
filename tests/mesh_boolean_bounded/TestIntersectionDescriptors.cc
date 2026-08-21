#include "YgorMeshesBooleanBounded/IntersectionDescriptors.h"

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

const intersection_descriptor_record *find_descriptor(
    const intersection_descriptor_tables &tables,
    intersection_descriptor_locus locus,
    intersection_descriptor_category category) {
  for (const auto &record : tables.records)
    if (record.key.locus == locus && record.key.category == category)
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
           relation_contact_dimension::area, 0, 0, operand_id::b),
      seed(5, feature_relation_status::endpoint_crossing,
           relation_contact_dimension::point, 1, 0, operand_id::b),
      seed(6, feature_relation_status::tangency,
           relation_contact_dimension::point, 0, 0, operand_id::a)};
  seeds[3].has_symbolic_decision = true;
  seeds[3].symbolic_rule_ordinal = 17;

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
  for (std::size_t i = 0; i < interning.events.size(); ++i) {
    auto &key = interning.events[i].key;
    key.semantic_namespace.bytes[0] = static_cast<std::uint8_t>(0x70 + i);
    key.event_class = intersection_event_class::edge_facet_point;
    key.first_owner =
        feature(operand_id::a, relation_feature_kind::source_facet, 10);
    key.second_owner =
        feature(operand_id::b, relation_feature_kind::source_facet, 20);
    key.public_relation = seeds[i == 0 ? 0 : 3].key;
    key.construction_geometric_lineage = i + 1;
    key.contact_status = i == 0 ? feature_relation_status::proper_crossing
                                : feature_relation_status::point_contact;
    key.contact_dimension = relation_contact_dimension::point;
  }
  for (std::size_t i = 0; i < interning.occurrences.size(); ++i) {
    auto &key = interning.occurrences[i].key;
    key.event = interning.events[interning.occurrences[i].event.ordinal()].key;
    key.discriminator.role = occurrence_role::single_occurrence;
  }
  interning.occurrences[1].key.discriminator.role =
      occurrence_role::topology_separated_contact;
  interning.occurrences[1].key.discriminator.occurrence_lineage = 101;
  interning.occurrences[1].topology_separate = true;
  interning.occurrences[1].requires_contact_separation = true;
  interning.seed_bindings.resize(seeds.size());
  const std::vector<event_id> event_ids{
      event_id{0}, event_id{0}, event_id{0}, event_id{1}, event_id{1},
      event_id{0}, event_id{0}};
  const std::vector<event_occurrence_id> occurrence_ids{
      event_occurrence_id{0}, event_occurrence_id{0}, event_occurrence_id{1},
      event_occurrence_id{2}, event_occurrence_id{2}, event_occurrence_id{0},
      event_occurrence_id{0}};
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
  sequence.memberships = {0, 3};
  source_edges.sequences.push_back(sequence);
  source_edge_sequence_record unaffected_sequence;
  unaffected_sequence.id = source_edge_sequence_id{1};
  unaffected_sequence.source_edge =
      feature(operand_id::a, relation_feature_kind::source_edge, 6);
  source_edges.sequences.push_back(unaffected_sequence);
  for (std::uint64_t i = 0; i != 3; ++i) {
    source_edge_membership_record membership;
    membership.id = source_edge_membership_id{i};
    membership.key.source_edge = sequence.source_edge;
    membership.occurrence = event_occurrence_id{i};
    membership.event = interning.occurrences[i].event;
    membership.internal_diagonal_discovery = i == 1;
    membership.bookkeeping_only = i == 1;
    source_edges.memberships.push_back(membership);
    source_edges.membership_sequence_index.push_back(membership.id);
  }
  source_edge_cluster_record source_cluster;
  source_cluster.id = source_edge_cluster_id{0};
  source_cluster.sequence = source_edge_sequence_id{0};
  source_cluster.key.source_edge = sequence.source_edge;
  source_cluster.member_occurrences = {0, 3};
  source_edges.clusters.push_back(source_cluster);
  source_edges.cluster_occurrence_index = {event_occurrence_id{0},
                                            event_occurrence_id{1},
                                            event_occurrence_id{2}};
  source_edge_interval_record source_interval;
  source_interval.id = source_edge_interval_id{0};
  source_interval.sequence = source_edge_sequence_id{0};
  source_interval.key.source_edge = sequence.source_edge;
  source_interval.key.left.kind = boundary_reference_kind::cluster;
  source_interval.key.left.cluster = source_cluster.key;
  source_interval.key.right.kind = boundary_reference_kind::end_sentinel;
  source_interval.key.canonical_ordinal = 0;
  source_interval.key.interval_class =
      intersection_interval_length::definitely_positive;
  source_interval.length_disposition =
      intersection_interval_length::definitely_positive;
  source_interval.propagation_allowed = true;
  source_interval.retention_allowed = false;
  source_edges.intervals.push_back(source_interval);
  auto zero_interval = source_interval;
  zero_interval.id = source_edge_interval_id{1};
  zero_interval.key.canonical_ordinal = 1;
  zero_interval.key.interval_class = intersection_interval_length::exact_zero;
  zero_interval.length_disposition = intersection_interval_length::exact_zero;
  zero_interval.propagation_allowed = false;
  source_edges.intervals.push_back(zero_interval);
  auto overlap_interval = source_interval;
  overlap_interval.id = source_edge_interval_id{2};
  overlap_interval.key.canonical_ordinal = 2;
  overlap_interval.key.interval_class = intersection_interval_length::overlap;
  overlap_interval.length_disposition = intersection_interval_length::overlap;
  overlap_interval.duplicate_required = true;
  overlap_interval.retention_allowed = true;
  source_edges.intervals.push_back(overlap_interval);

  transverse_carrier_arrangement_tables transverse;
  transverse_carrier_record transverse_carrier;
  transverse_carrier.id = transverse_carrier_id{0};
  transverse.carriers.push_back(transverse_carrier);
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
  span.classification_cut = true;
  span.output_edge_allowed = true;
  transverse.spans.push_back(span);

  coplanar_carrier_arrangement_tables coplanar;
  collinear_overlap_carrier_record overlap_carrier;
  overlap_carrier.id = collinear_overlap_carrier_id{0};
  overlap_carrier.start_occurrence = event_occurrence_id{1};
  overlap_carrier.end_occurrence = event_occurrence_id{2};
  coplanar.carriers.push_back(overlap_carrier);
  coplanar_overlap_record overlap;
  overlap.id = coplanar_overlap_record_id{0};
  overlap.kind = relation_coplanar_component_kind::coincident_sheet_boundary;
  overlap.distinct_sheet_occurrences = true;
  overlap.boundary_events = {0, 2};
  coplanar.overlaps.push_back(overlap);
  auto ordinary_overlap = overlap;
  ordinary_overlap.id = coplanar_overlap_record_id{1};
  ordinary_overlap.kind = relation_coplanar_component_kind::area_boundary;
  ordinary_overlap.distinct_sheet_occurrences = false;
  ordinary_overlap.zero_measure = false;
  coplanar.overlaps.push_back(ordinary_overlap);
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

  intersection_descriptor_tables descriptors;
  require(build_intersection_descriptors(
      seeds, interning, incidence, source_edges, transverse, coplanar, tables,
      descriptors, error));
  require(verify_intersection_descriptors(
      seeds, interning, incidence, source_edges, transverse, coplanar, tables,
      descriptors, error));

  const auto *proper = find_descriptor(
      descriptors,
      intersection_descriptor_locus::source_edge_cluster_boundary,
      intersection_descriptor_category::proper_crossing);
  require(proper != nullptr);
  require(!proper->continuation_allowed);
  require(proper->classification_consumable);
  require(proper->selection_consumable);
  require(proper->signed_crossing_delta == 0);

  const auto *endpoint = find_descriptor(
      descriptors,
      intersection_descriptor_locus::source_edge_cluster_boundary,
      intersection_descriptor_category::endpoint_crossing);
  require(endpoint != nullptr);
  require(endpoint->signed_crossing_delta == 1);
  require(endpoint->symbolic_owner == operand_id::b);

  const auto *tangent = find_descriptor(
      descriptors,
      intersection_descriptor_locus::source_edge_cluster_boundary,
      intersection_descriptor_category::tangent);
  require(tangent != nullptr);
  require(tangent->continuation_allowed);
  require(tangent->classification_consumable);

  const auto *separated = find_descriptor(
      descriptors,
      intersection_descriptor_locus::separated_contact_occurrence,
      intersection_descriptor_category::topology_separated_contact);
  require(separated != nullptr);
  require(separated->occurrence_separation_required);
  require(!separated->continuation_allowed);

  const auto *transparent = find_descriptor(
      descriptors,
      intersection_descriptor_locus::transparent_internal_diagonal_adjacency,
      intersection_descriptor_category::bookkeeping_only);
  require(transparent != nullptr);
  require(transparent->continuation_allowed);
  require(transparent->topology_consumable);
  require(transparent->provenance.count != 0);

  const auto *whole_edge = find_descriptor(
      descriptors, intersection_descriptor_locus::whole_source_edge,
      intersection_descriptor_category::no_influence);
  require(whole_edge != nullptr);
  require(whole_edge->continuation_allowed);
  require(whole_edge->key.source_feature == unaffected_sequence.source_edge);

  const auto *interval = find_descriptor(
      descriptors, intersection_descriptor_locus::source_edge_open_interval,
      intersection_descriptor_category::no_influence);
  require(interval != nullptr);
  require(interval->continuation_allowed);
  require(interval->signed_crossing_delta == 0);
  require(interval->provenance.count != 0);

  const auto *zero = find_descriptor(
      descriptors, intersection_descriptor_locus::source_edge_open_interval,
      intersection_descriptor_category::bookkeeping_only);
  require(zero != nullptr);
  require(!zero->continuation_allowed);

  const auto *overlap_interval_descriptor = find_descriptor(
      descriptors, intersection_descriptor_locus::source_edge_open_interval,
      intersection_descriptor_category::coincident_sheet_interior);
  require(overlap_interval_descriptor != nullptr);
  require(overlap_interval_descriptor->occurrence_separation_required);
  require(overlap_interval_descriptor->selection_consumable);

  const auto *span_descriptor = find_descriptor(
      descriptors, intersection_descriptor_locus::transverse_active_span,
      intersection_descriptor_category::proper_crossing);
  require(span_descriptor != nullptr);
  require(span_descriptor->classification_consumable);
  require(span_descriptor->selection_consumable);

  const auto *coincident = find_descriptor(
      descriptors, intersection_descriptor_locus::coincident_sheet,
      intersection_descriptor_category::coincident_sheet_boundary);
  require(coincident != nullptr);
  require(coincident->occurrence_separation_required);
  require(coincident->provenance.count != 0);

  const auto *coplanar_boundary = find_descriptor(
      descriptors, intersection_descriptor_locus::coplanar_overlap,
      intersection_descriptor_category::coplanar_overlap_boundary);
  require(coplanar_boundary != nullptr);
  require(coplanar_boundary->classification_consumable);
  require(coplanar_boundary->selection_consumable);
  require(coplanar_boundary->provenance.count != 0);

  const auto *symbolic = find_descriptor(
      descriptors,
      intersection_descriptor_locus::source_edge_cluster_boundary,
      intersection_descriptor_category::contact_delimiter);
  require(symbolic != nullptr);
  require(symbolic->symbolic_owner == operand_id::b);
  require(symbolic->symbolic_rule_ordinal == 17);

  auto mutated = descriptors;
  mutated.records.front().signed_crossing_delta += 1;
  require(!verify_intersection_descriptors(
      seeds, interning, incidence, source_edges, transverse, coplanar, tables,
      mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::verifier_rejection));

  mutated = descriptors;
  require(!mutated.provenance.empty());
  mutated.provenance.front() =
      event_incidence_id{intersection_invalid_ordinal};
  require(!verify_intersection_descriptors(
      seeds, interning, incidence, source_edges, transverse, coplanar, tables,
      mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::verifier_rejection));

  mutated = descriptors;
  mutated.records.pop_back();
  require(!verify_intersection_descriptors(
      seeds, interning, incidence, source_edges, transverse, coplanar, tables,
      mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::verifier_rejection));

  auto crossing_diagonal = source_edges;
  crossing_diagonal.memberships.front().internal_diagonal_discovery = true;
  crossing_diagonal.memberships.front().bookkeeping_only = true;
  require(!build_intersection_descriptors(
      seeds, interning, incidence, crossing_diagonal, transverse, coplanar,
      tables, mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::descriptor_mismatch));

  auto inactive_transverse = transverse;
  inactive_transverse.spans.front().activation =
      intersection_span_activation::inactive;
  intersection_aggregate_tables inactive_aggregates;
  require(build_intersection_aggregates(
      seeds, interning, incidence, source_edges, inactive_transverse, coplanar,
      inactive_aggregates, error));
  require(build_intersection_descriptors(
      seeds, interning, incidence, source_edges, inactive_transverse, coplanar,
      inactive_aggregates, mutated, error));
  const auto *inactive = find_descriptor(
      mutated, intersection_descriptor_locus::transverse_inactive_gap,
      intersection_descriptor_category::no_influence);
  require(inactive != nullptr);
  require(!inactive->continuation_allowed);
  require(inactive->topology_consumable);

  auto unresolved_transverse = transverse;
  unresolved_transverse.spans.front().activation =
      intersection_span_activation::unresolved;
  intersection_aggregate_tables unresolved_aggregates;
  require(build_intersection_aggregates(
      seeds, interning, incidence, source_edges, unresolved_transverse, coplanar,
      unresolved_aggregates, error));
  require(!build_intersection_descriptors(
      seeds, interning, incidence, source_edges, unresolved_transverse, coplanar,
      unresolved_aggregates, mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::descriptor_mismatch));


  auto unknown_transverse = transverse;
  unknown_transverse.spans.front().activation =
      static_cast<intersection_span_activation>(255);
  intersection_aggregate_tables unknown_transverse_aggregates;
  require(build_intersection_aggregates(
      seeds, interning, incidence, source_edges, unknown_transverse, coplanar,
      unknown_transverse_aggregates, error));
  require(!build_intersection_descriptors(
      seeds, interning, incidence, source_edges, unknown_transverse, coplanar,
      unknown_transverse_aggregates, mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::descriptor_mismatch));

  auto unknown_coplanar = coplanar;
  unknown_coplanar.overlaps.front().kind =
      static_cast<relation_coplanar_component_kind>(255);
  intersection_aggregate_tables unknown_coplanar_aggregates;
  require(build_intersection_aggregates(
      seeds, interning, incidence, source_edges, transverse, unknown_coplanar,
      unknown_coplanar_aggregates, error));
  require(!build_intersection_descriptors(
      seeds, interning, incidence, source_edges, transverse, unknown_coplanar,
      unknown_coplanar_aggregates, mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::descriptor_mismatch));

  auto uncertain_edges = source_edges;
  uncertain_edges.intervals.front().length_disposition =
      intersection_interval_length::uncertain;
  intersection_aggregate_tables uncertain_aggregates;
  require(build_intersection_aggregates(
      seeds, interning, incidence, uncertain_edges, transverse, coplanar,
      uncertain_aggregates, error));
  require(!build_intersection_descriptors(
      seeds, interning, incidence, uncertain_edges, transverse, coplanar,
      uncertain_aggregates, mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::descriptor_mismatch));

  auto unknown_edges = source_edges;
  unknown_edges.intervals.front().length_disposition =
      static_cast<intersection_interval_length>(255);
  intersection_aggregate_tables unknown_edge_aggregates;
  require(build_intersection_aggregates(
      seeds, interning, incidence, unknown_edges, transverse, coplanar,
      unknown_edge_aggregates, error));
  require(!build_intersection_descriptors(
      seeds, interning, incidence, unknown_edges, transverse, coplanar,
      unknown_edge_aggregates, mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::descriptor_mismatch));
  return 0;
}
