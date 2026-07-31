#include "IntersectionCanonicalization.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error canonicalization_error(const char *summary) {
  return intersection_error(
      intersection_subcode::canonicalization_error,
      bounded_boolean_error_category::internal_invariant_error, summary,
      intersection_checkpoint::canonical_remap);
}

bounded_boolean_error canonicalization_verifier_error(const char *summary) {
  return intersection_error(
      intersection_subcode::verifier_rejection,
      bounded_boolean_error_category::internal_invariant_error, summary,
      intersection_checkpoint::canonical_remap);
}

bool valid_operation(boolean_operation operation) noexcept {
  switch (operation) {
  case boolean_operation::set_union:
  case boolean_operation::intersection:
  case boolean_operation::a_minus_b:
  case boolean_operation::b_minus_a:
  case boolean_operation::symmetric_difference:
    return true;
  }
  return false;
}

bool valid_header(const intersection_canonicalization_header &header) noexcept {
  return header.owner.anchor != nullptr && valid_operation(header.operation) &&
         header.schema_version ==
             contract_versions::intersection_artifact_schema &&
         header.provider_version == contract_versions::intersection_provider &&
         header.semantic_policy_version ==
             contract_versions::intersection_semantic_policy &&
         header.codec_version == contract_versions::intersection_codec &&
         header.verifier_version == contract_versions::intersection_verifier &&
         header.reserved16 == 0 && header.reserved32 == 0;
}

bool checked_range(intersection_range range, std::size_t size) noexcept {
  return range.begin <= size &&
         range.count <= size - static_cast<std::size_t>(range.begin);
}

template <class Id>
bool valid_id(Id id, std::size_t size) noexcept {
  return id.ordinal() < size;
}

template <class Record>
bool dense_ids(const std::vector<Record> &records) noexcept {
  for (std::size_t i = 0; i < records.size(); ++i)
    if (records[i].id.ordinal() != i)
      return false;
  return true;
}

template <class Id>
bool valid_id_vector(const std::vector<Id> &ids, std::size_t size) noexcept {
  return std::all_of(ids.begin(), ids.end(),
                     [=](Id id) { return valid_id(id, size); });
}

bool valid_ranges(const std::vector<intersection_range> &ranges,
                  std::size_t index_size) noexcept {
  return std::all_of(ranges.begin(), ranges.end(),
                     [=](intersection_range range) {
                       return checked_range(range, index_size);
                     });
}

bool valid_certificate(const ordering_certificate_record &certificate) noexcept {
  if (certificate.policy_version !=
          contract_versions::intersection_bounded_ordering_policy ||
      certificate.reserved8 != 0 || !certificate.topology_safe ||
      certificate.first_parameter.ordinal() == intersection_invalid_ordinal ||
      certificate.second_parameter.ordinal() == intersection_invalid_ordinal)
    return false;
  switch (certificate.disposition) {
  case intersection_order_disposition::definitely_before:
  case intersection_order_disposition::definitely_after:
    return true;
  case intersection_order_disposition::exact_equal:
    return certificate.exact_evidence_lineage != 0;
  case intersection_order_disposition::unresolved_overlap:
    return certificate.comparison_evidence_lineage != 0;
  case intersection_order_disposition::invalid:
    return false;
  }
  return false;
}

bool valid_intersection_inputs(
    const event_interning_tables &interning,
    const event_coordinate_tables &coordinates,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    const intersection_descriptor_tables &descriptors) noexcept {
  if (!dense_ids(interning.events) || !dense_ids(interning.occurrences) ||
      !dense_ids(interning.seed_bindings) ||
      interning.seed_to_event.size() != interning.seed_bindings.size() ||
      interning.seed_to_occurrence.size() != interning.seed_bindings.size() ||
      !valid_id_vector(interning.event_binding_index,
                       interning.seed_bindings.size()) ||
      !valid_id_vector(interning.occurrence_binding_index,
                       interning.seed_bindings.size()) ||
      !valid_id_vector(interning.seed_to_event, interning.events.size()) ||
      !valid_id_vector(interning.seed_to_occurrence,
                       interning.occurrences.size()))
    return false;
  for (const auto &event : interning.events)
    if (!valid_intersection_event_key(event.key) ||
        !checked_range(event.construction_witnesses,
                       coordinates.construction_witness_index.size()) ||
        !checked_range(event.occurrences, interning.occurrences.size()) ||
        !checked_range(event.seed_bindings,
                       interning.event_binding_index.size()) ||
        event.schema_version != contract_versions::intersection_event_schema ||
        event.reserved16 != 0 || event.reserved32 != 0)
      return false;
  for (const auto &occurrence : interning.occurrences)
    if (!valid_id(occurrence.event, interning.events.size()) ||
        !valid_intersection_occurrence_key(occurrence.key) ||
        !checked_range(occurrence.seed_bindings,
                       interning.occurrence_binding_index.size()) ||
        occurrence.schema_version !=
            contract_versions::intersection_occurrence_schema ||
        occurrence.reserved16 != 0)
      return false;

  if (!dense_ids(incidence.records) ||
      !valid_id_vector(incidence.by_event, incidence.records.size()) ||
      !valid_id_vector(incidence.by_occurrence, incidence.records.size()) ||
      !valid_id_vector(incidence.by_seed, incidence.records.size()) ||
      !valid_id_vector(incidence.seed_candidate_index,
                       incidence.records.size()) ||
      !valid_id_vector(incidence.by_relation, incidence.records.size()) ||
      !valid_id_vector(incidence.by_candidate, incidence.records.size()) ||
      !valid_id_vector(incidence.by_source_feature,
                       incidence.records.size()) ||
      !valid_id_vector(incidence.by_halfedge, incidence.records.size()) ||
      !valid_ranges(incidence.event_ranges, incidence.by_event.size()) ||
      !valid_ranges(incidence.occurrence_ranges,
                    incidence.by_occurrence.size()) ||
      !valid_ranges(incidence.seed_ranges, incidence.by_seed.size()) ||
      !valid_ranges(incidence.seed_candidate_ranges,
                    incidence.seed_candidate_index.size()) ||
      !valid_ranges(incidence.relation_ranges, incidence.by_relation.size()) ||
      !valid_ranges(incidence.candidate_ranges,
                    incidence.by_candidate.size()))
    return false;
  for (const auto &range : incidence.source_feature_ranges)
    if (!checked_range(range.incidence, incidence.by_source_feature.size()))
      return false;
  for (const auto &range : incidence.halfedge_ranges)
    if (!checked_range(range.incidence, incidence.by_halfedge.size()))
      return false;

  if (!dense_ids(source_edges.memberships) ||
      !dense_ids(source_edges.sequences) || !dense_ids(source_edges.clusters) ||
      !dense_ids(source_edges.intervals) ||
      !dense_ids(source_edges.ordering_certificates) ||
      !std::all_of(source_edges.ordering_certificates.begin(),
                   source_edges.ordering_certificates.end(),
                   valid_certificate) ||
      !valid_id_vector(source_edges.membership_sequence_index,
                       source_edges.memberships.size()) ||
      !valid_id_vector(source_edges.cluster_occurrence_index,
                       interning.occurrences.size()) ||
      !valid_id_vector(source_edges.cluster_membership_index,
                       source_edges.memberships.size()) ||
      !valid_id_vector(source_edges.sequence_cluster_index,
                       source_edges.clusters.size()) ||
      !valid_id_vector(source_edges.sequence_interval_index,
                       source_edges.intervals.size()))
    return false;
  for (const auto &sequence : source_edges.sequences)
    if (!checked_range(sequence.memberships,
                       source_edges.membership_sequence_index.size()) ||
        !checked_range(sequence.clusters,
                       source_edges.sequence_cluster_index.size()) ||
        !checked_range(sequence.intervals,
                       source_edges.sequence_interval_index.size()))
      return false;
  for (const auto &cluster : source_edges.clusters)
    if (!checked_range(cluster.member_occurrences,
                       source_edges.cluster_occurrence_index.size()) ||
        !checked_range(cluster.membership_ids,
                       source_edges.cluster_membership_index.size()) ||
        (cluster.ordering_certificate.ordinal() !=
             intersection_invalid_ordinal &&
         !valid_id(cluster.ordering_certificate,
                   source_edges.ordering_certificates.size())))
      return false;
  for (const auto &membership : source_edges.memberships)
    if (membership.ordering_certificate.ordinal() !=
            intersection_invalid_ordinal &&
        !valid_id(membership.ordering_certificate,
                  source_edges.ordering_certificates.size()))
      return false;

  if (!dense_ids(transverse.carriers) || !dense_ids(transverse.memberships) ||
      !dense_ids(transverse.clusters) || !dense_ids(transverse.spans) ||
      !dense_ids(transverse.ordering_certificates) ||
      !std::all_of(transverse.ordering_certificates.begin(),
                   transverse.ordering_certificates.end(),
                   valid_certificate) ||
      !valid_id_vector(transverse.carrier_membership_index,
                       transverse.memberships.size()) ||
      !valid_id_vector(transverse.cluster_occurrence_index,
                       interning.occurrences.size()) ||
      !valid_id_vector(transverse.cluster_membership_index,
                       transverse.memberships.size()) ||
      !valid_id_vector(transverse.carrier_cluster_index,
                       transverse.clusters.size()) ||
      !valid_id_vector(transverse.carrier_span_index,
                       transverse.spans.size()))
    return false;
  for (const auto &carrier : transverse.carriers)
    if (!checked_range(carrier.relation_provenance,
                       transverse.carrier_relation_provenance.size()) ||
        !checked_range(carrier.candidate_provenance,
                       transverse.carrier_candidate_provenance.size()) ||
        !checked_range(carrier.memberships,
                       transverse.carrier_membership_index.size()) ||
        !checked_range(carrier.clusters,
                       transverse.carrier_cluster_index.size()) ||
        !checked_range(carrier.active_spans,
                       transverse.carrier_span_index.size()))
      return false;
  for (const auto &membership : transverse.memberships)
    if (membership.ordering_certificate.ordinal() !=
            intersection_invalid_ordinal &&
        !valid_id(membership.ordering_certificate,
                  transverse.ordering_certificates.size()))
      return false;
  for (const auto &cluster : transverse.clusters)
    if (!checked_range(cluster.occurrence_members,
                       transverse.cluster_occurrence_index.size()) ||
        !checked_range(cluster.membership_members,
                       transverse.cluster_membership_index.size()) ||
        (cluster.ordering_certificate.ordinal() !=
             intersection_invalid_ordinal &&
         !valid_id(cluster.ordering_certificate,
                   transverse.ordering_certificates.size())))
      return false;
  for (const auto &span : transverse.spans)
    if (!checked_range(span.provenance,
                       transverse.span_relation_provenance.size()) ||
        !checked_range(span.region_incidence,
                       transverse.span_region_incidence.size()))
      return false;

  if (!dense_ids(coplanar.supports) || !dense_ids(coplanar.carriers) ||
      !dense_ids(coplanar.overlaps) || !dense_ids(coplanar.regions))
    return false;
  for (const auto &support : coplanar.supports)
    if (!checked_range(support.provenance,
                       coplanar.support_relation_provenance.size()) ||
        !checked_range(support.original_boundary_edges,
                       coplanar.support_original_boundary_edge_index.size()) ||
        !checked_range(support.boundary_events,
                       coplanar.support_boundary_event_index.size()) ||
        !checked_range(support.boundary_carriers,
                       coplanar.support_boundary_carrier_index.size()) ||
        !checked_range(support.overlap_components,
                       coplanar.support_overlap_index.size()) ||
        !checked_range(support.region_incidence,
                       coplanar.support_region_index.size()))
      return false;
  for (const auto &carrier : coplanar.carriers)
    if (!checked_range(carrier.provenance,
                       coplanar.carrier_relation_provenance.size()) ||
        !checked_range(carrier.source_provenance,
                       coplanar.carrier_source_provenance.size()))
      return false;
  for (const auto &overlap : coplanar.overlaps)
    if (!checked_range(overlap.boundary_events,
                       coplanar.overlap_boundary_event_index.size()) ||
        !checked_range(overlap.boundary_carriers,
                       coplanar.overlap_boundary_carrier_index.size()) ||
        !checked_range(overlap.provenance,
                       coplanar.overlap_relation_provenance.size()))
      return false;
  for (const auto &region : coplanar.regions)
    if (!checked_range(region.boundary_events,
                       coplanar.region_boundary_event_index.size()) ||
        !checked_range(region.boundary_carriers,
                       coplanar.region_boundary_carrier_index.size()) ||
        !checked_range(region.coverage_witnesses,
                       coplanar.region_coverage_witness_index.size()))
      return false;

  if (!dense_ids(aggregates.crossing) || !dense_ids(aggregates.contact))
    return false;
  for (const auto &aggregate : aggregates.crossing)
    if (!checked_range(aggregate.members, aggregates.crossing_members.size()) ||
        !checked_range(aggregate.facet_subtotals,
                       aggregates.facet_subtotals.size()) ||
        !checked_range(aggregate.shell_subtotals,
                       aggregates.shell_subtotals.size()))
      return false;
  for (const auto &subtotal : aggregates.facet_subtotals)
    if (!checked_range(subtotal.members,
                       aggregates.facet_subtotal_members.size()))
      return false;
  for (const auto &subtotal : aggregates.shell_subtotals)
    if (!checked_range(subtotal.members,
                       aggregates.shell_subtotal_members.size()))
      return false;
  for (const auto &aggregate : aggregates.contact)
    if (!checked_range(aggregate.members, aggregates.contact_members.size()))
      return false;

  if (!dense_ids(descriptors.records))
    return false;
  for (const auto &descriptor : descriptors.records)
    if (!valid_intersection_descriptor_key(descriptor.key) ||
        !checked_range(descriptor.provenance, descriptors.provenance.size()) ||
        descriptor.schema_version !=
            contract_versions::intersection_descriptor_schema ||
        descriptor.reserved8 != 0)
      return false;
  return true;
}

template <class T>
bool add_vector_bytes(std::uint64_t &total, const std::vector<T> &values) {
  if (values.size() >
      std::numeric_limits<std::uint64_t>::max() / sizeof(T))
    return false;
  const auto bytes = static_cast<std::uint64_t>(values.size()) * sizeof(T);
  if (total > std::numeric_limits<std::uint64_t>::max() - bytes)
    return false;
  total += bytes;
  return true;
}

ordering_certificate_id remap_certificate(ordering_certificate_id id,
                                          std::uint64_t offset) noexcept {
  return id.ordinal() == intersection_invalid_ordinal
             ? id
             : ordering_certificate_id{id.ordinal() + offset};
}

bool same_range(intersection_range a, intersection_range b) noexcept {
  return a.begin == b.begin && a.count == b.count;
}

bool same_ranges(const std::vector<intersection_range> &a,
                 const std::vector<intersection_range> &b) noexcept {
  if (a.size() != b.size())
    return false;
  for (std::size_t i = 0; i < a.size(); ++i)
    if (!same_range(a[i], b[i]))
      return false;
  return true;
}

template <class Record, class Equal>
bool same_records(const std::vector<Record> &a, const std::vector<Record> &b,
                  Equal equal) noexcept {
  if (a.size() != b.size())
    return false;
  for (std::size_t i = 0; i < a.size(); ++i)
    if (!equal(a[i], b[i]))
      return false;
  return true;
}

bool same_point_reference(const bounded_point_reference &a,
                          const bounded_point_reference &b) noexcept {
  return a.kind == b.kind && a.source_vertex == b.source_vertex &&
         a.construction == b.construction &&
         a.precision_ledger == b.precision_ledger &&
         a.schema_version == b.schema_version &&
         a.reserved16 == b.reserved16 && a.reserved32 == b.reserved32;
}

bool same_event(const intersection_event_record &a,
                const intersection_event_record &b) noexcept {
  return a.id == b.id && a.key == b.key &&
         same_point_reference(a.point, b.point) &&
         same_range(a.construction_witnesses, b.construction_witnesses) &&
         same_range(a.occurrences, b.occurrences) &&
         same_range(a.seed_bindings, b.seed_bindings) &&
         same_range(a.incidence, b.incidence) &&
         same_range(a.crossing_aggregates, b.crossing_aggregates) &&
         same_range(a.contact_aggregates, b.contact_aggregates) &&
         a.schema_version == b.schema_version &&
         a.reserved16 == b.reserved16 && a.reserved32 == b.reserved32;
}

bool same_occurrence(const intersection_occurrence_record &a,
                     const intersection_occurrence_record &b) noexcept {
  return a.id == b.id && a.event == b.event && a.key == b.key &&
         same_range(a.seed_bindings, b.seed_bindings) &&
         same_range(a.incidence, b.incidence) &&
         same_range(a.source_edge_memberships, b.source_edge_memberships) &&
         same_range(a.carrier_memberships, b.carrier_memberships) &&
         same_range(a.cluster_memberships, b.cluster_memberships) &&
         same_range(a.aggregate_contributions, b.aggregate_contributions) &&
         same_range(a.descriptors, b.descriptors) &&
         a.may_share_output_coordinate == b.may_share_output_coordinate &&
         a.topology_separate == b.topology_separate &&
         a.local_cluster_compatible == b.local_cluster_compatible &&
         a.requires_contact_separation == b.requires_contact_separation &&
         a.schema_version == b.schema_version &&
         a.reserved16 == b.reserved16;
}

bool same_seed_binding(const event_seed_binding_record &a,
                       const event_seed_binding_record &b) noexcept {
  return a.id == b.id && a.seed == b.seed && a.seed_key == b.seed_key &&
         a.canonical_seed_ordinal == b.canonical_seed_ordinal &&
         a.event == b.event && a.occurrence == b.occurrence &&
         a.relation == b.relation && a.construction == b.construction &&
         a.accepted_source_vertex == b.accepted_source_vertex &&
         same_range(a.incidence, b.incidence) &&
         same_range(a.candidate_incidence, b.candidate_incidence) &&
         a.expected_membership_role == b.expected_membership_role &&
         a.expected_carrier_role == b.expected_carrier_role &&
         a.designated_authority == b.designated_authority &&
         a.duplicate_consumer == b.duplicate_consumer &&
         a.compatibility_verified == b.compatibility_verified &&
         a.reserved8 == b.reserved8 && a.schema_version == b.schema_version &&
         a.reserved32 == b.reserved32;
}

bool same_incidence(const event_incidence_record &a,
                    const event_incidence_record &b) noexcept {
  return a.id == b.id && a.key == b.key && a.event == b.event &&
         a.occurrence == b.occurrence && a.seed_binding == b.seed_binding &&
         a.kind == b.kind && a.feature == b.feature &&
         a.relation == b.relation && a.candidate == b.candidate &&
         a.payload_primary == b.payload_primary &&
         a.payload_secondary == b.payload_secondary &&
         a.payload_occurrence == b.payload_occurrence &&
         a.numeric_crossing == b.numeric_crossing &&
         a.symbolic_crossing == b.symbolic_crossing &&
         a.orientation == b.orientation &&
         a.source_feature_owner == b.source_feature_owner &&
         a.bookkeeping_only == b.bookkeeping_only &&
         a.schema_version == b.schema_version && a.reserved16 == b.reserved16;
}

bool same_source_feature_range(
    const source_feature_incidence_range_record &a,
    const source_feature_incidence_range_record &b) noexcept {
  return a.kind == b.kind && a.feature == b.feature &&
         same_range(a.incidence, b.incidence) &&
         a.schema_version == b.schema_version && a.reserved16 == b.reserved16;
}

bool same_halfedge_range(const oriented_halfedge_incidence_range_record &a,
                         const oriented_halfedge_incidence_range_record &b) noexcept {
  return a.operand == b.operand && a.halfedge == b.halfedge &&
         same_range(a.incidence, b.incidence) &&
         a.schema_version == b.schema_version &&
         a.reserved16 == b.reserved16 && a.reserved32 == b.reserved32;
}

bool same_source_membership(const source_edge_membership_record &a,
                            const source_edge_membership_record &b) noexcept {
  return a.id == b.id && a.key == b.key && a.occurrence == b.occurrence &&
         a.event == b.event && a.parameter == b.parameter &&
         same_range(a.contributions, b.contributions) &&
         same_range(a.incident_facet_uses, b.incident_facet_uses) &&
         a.ordering_certificate == b.ordering_certificate &&
         a.exact_equal_eligible == b.exact_equal_eligible &&
         a.cluster_eligible == b.cluster_eligible &&
         a.internal_diagonal_discovery == b.internal_diagonal_discovery &&
         a.bookkeeping_only == b.bookkeeping_only &&
         a.schema_version == b.schema_version && a.reserved16 == b.reserved16;
}

bool same_sentinel(const source_edge_endpoint_sentinel_record &a,
                   const source_edge_endpoint_sentinel_record &b) noexcept {
  return a.side == b.side && a.source_vertex == b.source_vertex &&
         a.source_edge == b.source_edge;
}

bool same_source_sequence(const source_edge_sequence_record &a,
                          const source_edge_sequence_record &b) noexcept {
  return a.id == b.id && a.source_edge == b.source_edge &&
         same_sentinel(a.start, b.start) && same_sentinel(a.end, b.end) &&
         same_range(a.clusters, b.clusters) &&
         same_range(a.memberships, b.memberships) &&
         same_range(a.intervals, b.intervals) &&
         same_range(a.aggregates, b.aggregates) &&
         same_range(a.descriptors, b.descriptors) &&
         a.sequence_digest == b.sequence_digest &&
         a.comparison_count == b.comparison_count &&
         a.canonical_forward == b.canonical_forward &&
         a.reserved8 == b.reserved8 && a.schema_version == b.schema_version &&
         a.reserved32 == b.reserved32;
}

bool same_source_cluster(const source_edge_cluster_record &a,
                         const source_edge_cluster_record &b) noexcept {
  return a.id == b.id && a.sequence == b.sequence && a.key == b.key &&
         same_range(a.member_occurrences, b.member_occurrences) &&
         same_range(a.membership_ids, b.membership_ids) &&
         same_range(a.contributions, b.contributions) &&
         a.predecessor == b.predecessor && a.successor == b.successor &&
         a.ordering_certificate == b.ordering_certificate &&
         a.shared_output_coordinate == b.shared_output_coordinate &&
         a.separate_output_occurrences == b.separate_output_occurrences &&
         a.schema_version == b.schema_version;
}

bool same_source_interval(const source_edge_interval_record &a,
                          const source_edge_interval_record &b) noexcept {
  return a.id == b.id && a.sequence == b.sequence && a.key == b.key &&
         a.left_parameter == b.left_parameter &&
         a.right_parameter == b.right_parameter &&
         a.length_disposition == b.length_disposition &&
         a.left_crossing_delta == b.left_crossing_delta &&
         a.right_crossing_delta == b.right_crossing_delta &&
         a.accumulated_crossing == b.accumulated_crossing &&
         a.propagation_allowed == b.propagation_allowed &&
         a.retention_allowed == b.retention_allowed &&
         a.split_required == b.split_required &&
         a.duplicate_required == b.duplicate_required &&
         same_range(a.provenance, b.provenance) &&
         same_range(a.descriptors, b.descriptors) &&
         a.schema_version == b.schema_version && a.reserved16 == b.reserved16;
}

bool same_transverse_carrier(const transverse_carrier_record &a,
                             const transverse_carrier_record &b) noexcept {
  return a.id == b.id && a.key == b.key &&
         a.construction == b.construction &&
         same_range(a.relation_provenance, b.relation_provenance) &&
         same_range(a.candidate_provenance, b.candidate_provenance) &&
         same_range(a.memberships, b.memberships) &&
         same_range(a.clusters, b.clusters) &&
         same_range(a.active_spans, b.active_spans) &&
         same_range(a.region_incidence, b.region_incidence) &&
         same_range(a.aggregates, b.aggregates) &&
         same_range(a.descriptors, b.descriptors) &&
         a.carrier_digest == b.carrier_digest &&
         a.schema_version == b.schema_version &&
         a.reserved16 == b.reserved16 && a.reserved32 == b.reserved32;
}

bool same_carrier_membership(const carrier_membership_record &a,
                             const carrier_membership_record &b,
                             std::uint64_t certificate_offset) noexcept {
  return a.id == b.id && a.carrier == b.carrier &&
         a.occurrence == b.occurrence && a.event == b.event &&
         a.parameter == b.parameter &&
         a.relation_lineage == b.relation_lineage &&
         a.ordering_certificate ==
             remap_certificate(b.ordering_certificate, certificate_offset) &&
         a.schema_version == b.schema_version &&
         a.reserved16 == b.reserved16 && a.reserved32 == b.reserved32;
}

bool same_carrier_cluster(const carrier_cluster_record &a,
                          const carrier_cluster_record &b,
                          std::uint64_t certificate_offset) noexcept {
  return a.id == b.id && a.carrier == b.carrier &&
         a.equivalence == b.equivalence &&
         same_range(a.occurrence_members, b.occurrence_members) &&
         same_range(a.membership_members, b.membership_members) &&
         a.predecessor == b.predecessor && a.successor == b.successor &&
         a.ordering_certificate ==
             remap_certificate(b.ordering_certificate, certificate_offset) &&
         a.shared_output_coordinate == b.shared_output_coordinate &&
         a.separate_output_occurrences == b.separate_output_occurrences &&
         a.schema_version == b.schema_version;
}

bool same_carrier_span(const carrier_active_span_record &a,
                       const carrier_active_span_record &b) noexcept {
  return a.id == b.id && a.carrier == b.carrier && a.left == b.left &&
         a.right == b.right && a.activation == b.activation &&
         a.relation_interval_lineage == b.relation_interval_lineage &&
         same_range(a.region_incidence, b.region_incidence) &&
         same_range(a.provenance, b.provenance) &&
         a.classification_cut == b.classification_cut &&
         a.contact_delimiter == b.contact_delimiter &&
         a.output_edge_allowed == b.output_edge_allowed &&
         a.reserved8 == b.reserved8 && a.schema_version == b.schema_version;
}

bool same_coplanar_support(const coplanar_support_record &a,
                           const coplanar_support_record &b) noexcept {
  return a.id == b.id && a.key == b.key && a.first_facet == b.first_facet &&
         a.second_facet == b.second_facet &&
         a.support_lineage == b.support_lineage &&
         a.opposite_orientation == b.opposite_orientation &&
         a.symbolic_owner == b.symbolic_owner &&
         same_range(a.original_boundary_edges, b.original_boundary_edges) &&
         same_range(a.boundary_events, b.boundary_events) &&
         same_range(a.boundary_carriers, b.boundary_carriers) &&
         same_range(a.overlap_components, b.overlap_components) &&
         same_range(a.region_incidence, b.region_incidence) &&
         same_range(a.provenance, b.provenance) &&
         a.schema_version == b.schema_version && a.reserved16 == b.reserved16;
}

bool same_overlap_carrier(const collinear_overlap_carrier_record &a,
                          const collinear_overlap_carrier_record &b) noexcept {
  return a.id == b.id && a.key == b.key &&
         a.first_parameter_interval == b.first_parameter_interval &&
         a.second_parameter_interval == b.second_parameter_interval &&
         a.first_parameter_evidence == b.first_parameter_evidence &&
         a.second_parameter_evidence == b.second_parameter_evidence &&
         a.start_occurrence == b.start_occurrence &&
         a.end_occurrence == b.end_occurrence &&
         a.start_source_vertex == b.start_source_vertex &&
         a.end_source_vertex == b.end_source_vertex &&
         a.symbolic_owner == b.symbolic_owner &&
         a.half_open_first == b.half_open_first &&
         a.half_open_second == b.half_open_second &&
         a.separate_sheet_required == b.separate_sheet_required &&
         a.parameter_correspondence_verified ==
             b.parameter_correspondence_verified &&
         a.zero_length == b.zero_length && a.reserved8 == b.reserved8 &&
         same_range(a.provenance, b.provenance) &&
         same_range(a.source_provenance, b.source_provenance) &&
         same_range(a.contributions, b.contributions) &&
         same_range(a.descriptors, b.descriptors) &&
         a.schema_version == b.schema_version && a.reserved16 == b.reserved16;
}

bool same_overlap(const coplanar_overlap_record &a,
                  const coplanar_overlap_record &b) noexcept {
  return a.id == b.id && a.key == b.key && a.support == b.support &&
         a.component07_component == b.component07_component &&
         a.kind == b.kind && a.symbolic_owner == b.symbolic_owner &&
         a.sheet_mask == b.sheet_mask && a.closed == b.closed &&
         a.distinct_sheet_occurrences == b.distinct_sheet_occurrences &&
         a.zero_measure == b.zero_measure &&
         same_range(a.boundary_events, b.boundary_events) &&
         same_range(a.boundary_carriers, b.boundary_carriers) &&
         same_range(a.provenance, b.provenance) &&
         a.component_digest == b.component_digest &&
         a.schema_version == b.schema_version && a.reserved16 == b.reserved16;
}

bool same_region(const coplanar_region_incidence_record &a,
                 const coplanar_region_incidence_record &b) noexcept {
  return a.id == b.id && a.support == b.support &&
         a.first_facet == b.first_facet && a.second_facet == b.second_facet &&
         a.first_triangle == b.first_triangle &&
         a.second_triangle == b.second_triangle &&
         a.component == b.component &&
         a.component_lineage == b.component_lineage &&
         a.classification == b.classification &&
         a.relation_status == b.relation_status &&
         a.symbolic_owner == b.symbolic_owner && a.sheet_mask == b.sheet_mask &&
         a.internal_diagonal_coverage_only ==
             b.internal_diagonal_coverage_only &&
         a.coverage_complete == b.coverage_complete &&
         a.reserved8 == b.reserved8 &&
         same_range(a.boundary_events, b.boundary_events) &&
         same_range(a.boundary_carriers, b.boundary_carriers) &&
         same_range(a.coverage_witnesses, b.coverage_witnesses) &&
         a.source_facet_semantic_digest == b.source_facet_semantic_digest &&
         a.schema_version == b.schema_version && a.reserved16 == b.reserved16;
}

bool same_subtotal(const crossing_subtotal_record &a,
                   const crossing_subtotal_record &b) noexcept {
  return a.source_feature == b.source_feature &&
         a.numeric_signed_sum == b.numeric_signed_sum &&
         a.symbolic_signed_sum == b.symbolic_signed_sum &&
         a.symbolic_owner == b.symbolic_owner &&
         a.symbolic_owner_mask == b.symbolic_owner_mask &&
         a.mixed_symbolic_ownership == b.mixed_symbolic_ownership &&
         same_range(a.members, b.members) &&
         a.schema_version == b.schema_version && a.reserved16 == b.reserved16;
}

bool same_crossing_aggregate(const crossing_aggregate_record &a,
                             const crossing_aggregate_record &b) noexcept {
  return a.id == b.id && a.locus == b.locus &&
         a.source_feature == b.source_feature &&
         a.locus_ordinal == b.locus_ordinal &&
         a.numeric_signed_sum == b.numeric_signed_sum &&
         a.symbolic_signed_sum == b.symbolic_signed_sum &&
         a.entering_count == b.entering_count &&
         a.leaving_count == b.leaving_count &&
         a.symbolic_owner == b.symbolic_owner &&
         a.symbolic_owner_mask == b.symbolic_owner_mask &&
         a.mixed_symbolic_ownership == b.mixed_symbolic_ownership &&
         a.zero_net_contact_retained == b.zero_net_contact_retained &&
         same_range(a.members, b.members) &&
         same_range(a.facet_subtotals, b.facet_subtotals) &&
         same_range(a.shell_subtotals, b.shell_subtotals) &&
         a.member_order_verified == b.member_order_verified &&
         a.conserved == b.conserved && a.schema_version == b.schema_version &&
         a.reserved8 == b.reserved8;
}

bool same_contact_aggregate(const contact_aggregate_record &a,
                            const contact_aggregate_record &b) noexcept {
  return a.id == b.id && a.locus == b.locus &&
         a.source_feature == b.source_feature &&
         a.locus_ordinal == b.locus_ordinal &&
         a.contact_status == b.contact_status &&
         a.contact_dimension == b.contact_dimension &&
         a.symbolic_owner == b.symbolic_owner &&
         a.symbolic_owner_mask == b.symbolic_owner_mask &&
         a.mixed_symbolic_ownership == b.mixed_symbolic_ownership &&
         same_range(a.members, b.members) &&
         a.zero_net_retained == b.zero_net_retained &&
         a.tangent_retained == b.tangent_retained &&
         a.coincidence_retained == b.coincidence_retained &&
         a.reconstructed == b.reconstructed &&
         a.schema_version == b.schema_version && a.reserved16 == b.reserved16;
}

bool same_descriptor(const intersection_descriptor_record &a,
                     const intersection_descriptor_record &b) noexcept {
  return a.id == b.id && a.key == b.key &&
         a.signed_crossing_delta == b.signed_crossing_delta &&
         a.symbolic_owner == b.symbolic_owner &&
         a.symbolic_rule_ordinal == b.symbolic_rule_ordinal &&
         same_range(a.provenance, b.provenance) &&
         a.continuation_allowed == b.continuation_allowed &&
         a.occurrence_separation_required ==
             b.occurrence_separation_required &&
         a.classification_consumable == b.classification_consumable &&
         a.selection_consumable == b.selection_consumable &&
         a.topology_consumable == b.topology_consumable &&
         a.reserved8 == b.reserved8 && a.schema_version == b.schema_version;
}

bool same_certificate(const ordering_certificate_record &a,
                      const ordering_certificate_record &b) noexcept {
  return a.id == b.id && a.disposition == b.disposition &&
         a.first_parameter == b.first_parameter &&
         a.second_parameter == b.second_parameter &&
         a.exact_evidence_lineage == b.exact_evidence_lineage &&
         a.comparison_evidence_lineage == b.comparison_evidence_lineage &&
         a.topology_safe == b.topology_safe &&
         a.policy_version == b.policy_version && a.reserved8 == b.reserved8;
}

bool same_statistics(const intersection_statistics &a,
                     const intersection_statistics &b) noexcept {
  return a.seed_count == b.seed_count && a.event_count == b.event_count &&
         a.occurrence_count == b.occurrence_count &&
         a.seed_binding_count == b.seed_binding_count &&
         a.incidence_count == b.incidence_count &&
         a.source_edge_membership_count == b.source_edge_membership_count &&
         a.source_edge_sequence_count == b.source_edge_sequence_count &&
         a.source_edge_cluster_count == b.source_edge_cluster_count &&
         a.source_edge_interval_count == b.source_edge_interval_count &&
         a.transverse_carrier_count == b.transverse_carrier_count &&
         a.carrier_membership_count == b.carrier_membership_count &&
         a.carrier_cluster_count == b.carrier_cluster_count &&
         a.carrier_span_count == b.carrier_span_count &&
         a.coplanar_support_count == b.coplanar_support_count &&
         a.overlap_count == b.overlap_count &&
         a.aggregate_count == b.aggregate_count &&
         a.descriptor_count == b.descriptor_count &&
         a.ordering_certificate_count == b.ordering_certificate_count &&
         a.diagnostic_count == b.diagnostic_count &&
         a.replay_checkpoint_count == b.replay_checkpoint_count &&
         a.sort_comparisons == b.sort_comparisons &&
         a.verifier_work_units == b.verifier_work_units &&
         a.persistent_bytes == b.persistent_bytes &&
         a.canonical_bytes == b.canonical_bytes;
}

} // namespace

template <class T, class I> class intersection_builder final {
public:
  static void assign(
      canonical_intersection_complex<T, I> &out,
      const intersection_canonicalization_header &header,
      const event_interning_tables &interning,
      const event_coordinate_tables &coordinates,
      const event_incidence_tables &incidence,
      const source_edge_arrangement_tables &source_edges,
      const transverse_carrier_arrangement_tables &transverse,
      const coplanar_carrier_arrangement_tables &coplanar,
      const intersection_aggregate_tables &aggregates,
      const intersection_descriptor_tables &descriptors,
      const intersection_statistics &statistics) {
    out.owner_ = header.owner;
    out.operation_ = header.operation;
    out.provider_ =
        intersection_provider_kind::canonical_lineage_event_arrangement_v1;
    out.verification_ = intersection_verification_disposition::not_verified;
    out.schema_version_ = header.schema_version;
    out.provider_version_ = header.provider_version;
    out.semantic_policy_version_ = header.semantic_policy_version;
    out.codec_version_ = header.codec_version;
    out.verifier_version_ = header.verifier_version;
    out.reserved_ = 0;
    out.context_digest_ = header.context_digest;
    out.precision_digest_ = header.precision_digest;
    out.relation_digest_ = header.relation_digest;
    out.source_semantic_digests_ = header.source_semantic_digests;
    out.exact_triangulation_digests_ = header.exact_triangulation_digests;
    out.section_digests_ = {};

    out.events_ = interning.events;
    out.occurrences_ = interning.occurrences;
    out.seed_bindings_ = interning.seed_bindings;
    out.construction_witness_index_ = coordinates.construction_witness_index;
    out.event_binding_index_ = interning.event_binding_index;
    out.occurrence_binding_index_ = interning.occurrence_binding_index;
    out.seed_to_event_ = interning.seed_to_event;
    out.seed_to_occurrence_ = interning.seed_to_occurrence;

    out.incidence_ = incidence.records;
    out.incidence_by_event_ = incidence.by_event;
    out.incidence_by_occurrence_ = incidence.by_occurrence;
    out.incidence_by_seed_ = incidence.by_seed;
    out.incidence_by_seed_candidate_ = incidence.seed_candidate_index;
    out.event_incidence_ranges_ = incidence.event_ranges;
    out.occurrence_incidence_ranges_ = incidence.occurrence_ranges;
    out.seed_incidence_ranges_ = incidence.seed_ranges;
    out.seed_candidate_incidence_ranges_ = incidence.seed_candidate_ranges;
    out.incidence_by_relation_ = incidence.by_relation;
    out.relation_incidence_ranges_ = incidence.relation_ranges;
    out.incidence_by_candidate_ = incidence.by_candidate;
    out.candidate_incidence_ranges_ = incidence.candidate_ranges;
    out.incidence_by_source_feature_ = incidence.by_source_feature;
    out.source_feature_incidence_ranges_ = incidence.source_feature_ranges;
    out.incidence_by_halfedge_ = incidence.by_halfedge;
    out.halfedge_incidence_ranges_ = incidence.halfedge_ranges;

    out.source_edge_memberships_ = source_edges.memberships;
    out.source_edge_sequences_ = source_edges.sequences;
    out.source_edge_clusters_ = source_edges.clusters;
    out.source_edge_intervals_ = source_edges.intervals;
    out.source_edge_membership_sequence_index_ =
        source_edges.membership_sequence_index;
    out.source_edge_cluster_occurrence_index_ =
        source_edges.cluster_occurrence_index;
    out.source_edge_cluster_membership_index_ =
        source_edges.cluster_membership_index;
    out.source_edge_sequence_cluster_index_ =
        source_edges.sequence_cluster_index;
    out.source_edge_sequence_interval_index_ =
        source_edges.sequence_interval_index;

    out.transverse_carriers_ = transverse.carriers;
    out.carrier_memberships_ = transverse.memberships;
    out.carrier_clusters_ = transverse.clusters;
    out.carrier_active_spans_ = transverse.spans;
    out.carrier_relation_provenance_ =
        transverse.carrier_relation_provenance;
    out.carrier_candidate_provenance_ =
        transverse.carrier_candidate_provenance;
    out.carrier_membership_index_ = transverse.carrier_membership_index;
    out.carrier_cluster_occurrence_index_ =
        transverse.cluster_occurrence_index;
    out.carrier_cluster_membership_index_ =
        transverse.cluster_membership_index;
    out.transverse_carrier_cluster_index_ = transverse.carrier_cluster_index;
    out.transverse_carrier_span_index_ = transverse.carrier_span_index;
    out.carrier_span_relation_provenance_ =
        transverse.span_relation_provenance;
    out.carrier_span_region_incidence_ = transverse.span_region_incidence;

    const auto certificate_offset =
        static_cast<std::uint64_t>(source_edges.ordering_certificates.size());
    for (auto &membership : out.carrier_memberships_)
      membership.ordering_certificate =
          remap_certificate(membership.ordering_certificate, certificate_offset);
    for (auto &cluster : out.carrier_clusters_)
      cluster.ordering_certificate =
          remap_certificate(cluster.ordering_certificate, certificate_offset);

    out.coplanar_supports_ = coplanar.supports;
    out.overlap_carriers_ = coplanar.carriers;
    out.coplanar_overlaps_ = coplanar.overlaps;
    out.coplanar_region_incidence_ = coplanar.regions;
    out.coplanar_support_relation_provenance_ =
        coplanar.support_relation_provenance;
    out.coplanar_support_candidate_provenance_ =
        coplanar.support_candidate_provenance;
    out.coplanar_support_original_boundary_edge_index_ =
        coplanar.support_original_boundary_edge_index;
    out.coplanar_support_boundary_event_index_ =
        coplanar.support_boundary_event_index;
    out.coplanar_support_boundary_carrier_index_ =
        coplanar.support_boundary_carrier_index;
    out.coplanar_support_overlap_index_ = coplanar.support_overlap_index;
    out.coplanar_support_region_index_ = coplanar.support_region_index;
    out.overlap_carrier_relation_provenance_ =
        coplanar.carrier_relation_provenance;
    out.overlap_carrier_candidate_provenance_ =
        coplanar.carrier_candidate_provenance;
    out.overlap_carrier_source_provenance_ =
        coplanar.carrier_source_provenance;
    out.coplanar_overlap_boundary_event_index_ =
        coplanar.overlap_boundary_event_index;
    out.coplanar_overlap_boundary_carrier_index_ =
        coplanar.overlap_boundary_carrier_index;
    out.coplanar_overlap_relation_provenance_ =
        coplanar.overlap_relation_provenance;
    out.coplanar_region_boundary_event_index_ =
        coplanar.region_boundary_event_index;
    out.coplanar_region_boundary_carrier_index_ =
        coplanar.region_boundary_carrier_index;
    out.coplanar_region_coverage_witness_index_ =
        coplanar.region_coverage_witness_index;

    out.crossing_aggregates_ = aggregates.crossing;
    out.crossing_aggregate_members_ = aggregates.crossing_members;
    out.crossing_facet_subtotals_ = aggregates.facet_subtotals;
    out.crossing_facet_subtotal_members_ = aggregates.facet_subtotal_members;
    out.crossing_shell_subtotals_ = aggregates.shell_subtotals;
    out.crossing_shell_subtotal_members_ = aggregates.shell_subtotal_members;
    out.contact_aggregates_ = aggregates.contact;
    out.contact_aggregate_members_ = aggregates.contact_members;
    out.descriptors_ = descriptors.records;
    out.descriptor_provenance_ = descriptors.provenance;

    out.ordering_certificates_ = source_edges.ordering_certificates;
    out.ordering_certificates_.reserve(
        source_edges.ordering_certificates.size() +
        transverse.ordering_certificates.size());
    for (const auto &certificate : transverse.ordering_certificates) {
      auto copy = certificate;
      copy.id = ordering_certificate_id{copy.id.ordinal() + certificate_offset};
      out.ordering_certificates_.push_back(std::move(copy));
    }

    out.diagnostics_.clear();
    out.replay_checkpoints_.clear();
    out.statistics_ = statistics;
    out.verification_evidence_ = {};
    out.canonical_bytes_.clear();
    out.digest_ = {};
  }
};

namespace {

intersection_statistics make_statistics(
    const event_interning_tables &interning,
    const event_coordinate_tables &coordinates,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    const intersection_descriptor_tables &descriptors,
    bool &ok) {
  intersection_statistics statistics;
  statistics.seed_count = interning.seed_bindings.size();
  statistics.event_count = interning.events.size();
  statistics.occurrence_count = interning.occurrences.size();
  statistics.seed_binding_count = interning.seed_bindings.size();
  statistics.incidence_count = incidence.records.size();
  statistics.source_edge_membership_count = source_edges.memberships.size();
  statistics.source_edge_sequence_count = source_edges.sequences.size();
  statistics.source_edge_cluster_count = source_edges.clusters.size();
  statistics.source_edge_interval_count = source_edges.intervals.size();
  statistics.transverse_carrier_count = transverse.carriers.size();
  statistics.carrier_membership_count = transverse.memberships.size();
  statistics.carrier_cluster_count = transverse.clusters.size();
  statistics.carrier_span_count = transverse.spans.size();
  statistics.coplanar_support_count = coplanar.supports.size();
  statistics.overlap_count = coplanar.overlaps.size();
  statistics.aggregate_count =
      aggregates.crossing.size() + aggregates.contact.size();
  statistics.descriptor_count = descriptors.records.size();
  statistics.ordering_certificate_count =
      source_edges.ordering_certificates.size() +
      transverse.ordering_certificates.size();
  statistics.diagnostic_count = 0;
  statistics.replay_checkpoint_count = 0;
  statistics.sort_comparisons = 0;
  statistics.verifier_work_units = 0;
  statistics.canonical_bytes = 0;

  std::uint64_t bytes = 0;
#define YGOR_ADD_BYTES(values) ok = ok && add_vector_bytes(bytes, (values))
  YGOR_ADD_BYTES(interning.events);
  YGOR_ADD_BYTES(interning.occurrences);
  YGOR_ADD_BYTES(interning.seed_bindings);
  YGOR_ADD_BYTES(interning.event_binding_index);
  YGOR_ADD_BYTES(interning.occurrence_binding_index);
  YGOR_ADD_BYTES(interning.seed_to_event);
  YGOR_ADD_BYTES(interning.seed_to_occurrence);
  YGOR_ADD_BYTES(coordinates.construction_witness_index);
  YGOR_ADD_BYTES(incidence.records);
  YGOR_ADD_BYTES(incidence.by_event);
  YGOR_ADD_BYTES(incidence.event_ranges);
  YGOR_ADD_BYTES(incidence.by_occurrence);
  YGOR_ADD_BYTES(incidence.occurrence_ranges);
  YGOR_ADD_BYTES(incidence.by_seed);
  YGOR_ADD_BYTES(incidence.seed_ranges);
  YGOR_ADD_BYTES(incidence.seed_candidate_index);
  YGOR_ADD_BYTES(incidence.seed_candidate_ranges);
  YGOR_ADD_BYTES(incidence.by_relation);
  YGOR_ADD_BYTES(incidence.relation_ranges);
  YGOR_ADD_BYTES(incidence.by_candidate);
  YGOR_ADD_BYTES(incidence.candidate_ranges);
  YGOR_ADD_BYTES(incidence.by_source_feature);
  YGOR_ADD_BYTES(incidence.source_feature_ranges);
  YGOR_ADD_BYTES(incidence.by_halfedge);
  YGOR_ADD_BYTES(incidence.halfedge_ranges);
  YGOR_ADD_BYTES(source_edges.memberships);
  YGOR_ADD_BYTES(source_edges.membership_sequence_index);
  YGOR_ADD_BYTES(source_edges.sequences);
  YGOR_ADD_BYTES(source_edges.clusters);
  YGOR_ADD_BYTES(source_edges.cluster_occurrence_index);
  YGOR_ADD_BYTES(source_edges.cluster_membership_index);
  YGOR_ADD_BYTES(source_edges.sequence_cluster_index);
  YGOR_ADD_BYTES(source_edges.intervals);
  YGOR_ADD_BYTES(source_edges.sequence_interval_index);
  YGOR_ADD_BYTES(source_edges.ordering_certificates);
  YGOR_ADD_BYTES(transverse.carriers);
  YGOR_ADD_BYTES(transverse.carrier_relation_provenance);
  YGOR_ADD_BYTES(transverse.carrier_candidate_provenance);
  YGOR_ADD_BYTES(transverse.memberships);
  YGOR_ADD_BYTES(transverse.carrier_membership_index);
  YGOR_ADD_BYTES(transverse.clusters);
  YGOR_ADD_BYTES(transverse.cluster_occurrence_index);
  YGOR_ADD_BYTES(transverse.cluster_membership_index);
  YGOR_ADD_BYTES(transverse.carrier_cluster_index);
  YGOR_ADD_BYTES(transverse.spans);
  YGOR_ADD_BYTES(transverse.carrier_span_index);
  YGOR_ADD_BYTES(transverse.span_relation_provenance);
  YGOR_ADD_BYTES(transverse.span_region_incidence);
  YGOR_ADD_BYTES(transverse.ordering_certificates);
  YGOR_ADD_BYTES(coplanar.supports);
  YGOR_ADD_BYTES(coplanar.support_relation_provenance);
  YGOR_ADD_BYTES(coplanar.support_candidate_provenance);
  YGOR_ADD_BYTES(coplanar.support_original_boundary_edge_index);
  YGOR_ADD_BYTES(coplanar.support_boundary_event_index);
  YGOR_ADD_BYTES(coplanar.support_boundary_carrier_index);
  YGOR_ADD_BYTES(coplanar.support_overlap_index);
  YGOR_ADD_BYTES(coplanar.support_region_index);
  YGOR_ADD_BYTES(coplanar.carriers);
  YGOR_ADD_BYTES(coplanar.carrier_relation_provenance);
  YGOR_ADD_BYTES(coplanar.carrier_candidate_provenance);
  YGOR_ADD_BYTES(coplanar.carrier_source_provenance);
  YGOR_ADD_BYTES(coplanar.overlaps);
  YGOR_ADD_BYTES(coplanar.overlap_boundary_event_index);
  YGOR_ADD_BYTES(coplanar.overlap_boundary_carrier_index);
  YGOR_ADD_BYTES(coplanar.overlap_relation_provenance);
  YGOR_ADD_BYTES(coplanar.regions);
  YGOR_ADD_BYTES(coplanar.region_boundary_event_index);
  YGOR_ADD_BYTES(coplanar.region_boundary_carrier_index);
  YGOR_ADD_BYTES(coplanar.region_coverage_witness_index);
  YGOR_ADD_BYTES(aggregates.crossing);
  YGOR_ADD_BYTES(aggregates.crossing_members);
  YGOR_ADD_BYTES(aggregates.facet_subtotals);
  YGOR_ADD_BYTES(aggregates.facet_subtotal_members);
  YGOR_ADD_BYTES(aggregates.shell_subtotals);
  YGOR_ADD_BYTES(aggregates.shell_subtotal_members);
  YGOR_ADD_BYTES(aggregates.contact);
  YGOR_ADD_BYTES(aggregates.contact_members);
  YGOR_ADD_BYTES(descriptors.records);
  YGOR_ADD_BYTES(descriptors.provenance);
#undef YGOR_ADD_BYTES
  statistics.persistent_bytes = bytes;
  return statistics;
}

template <class T, class I>
bool verify_projection(
    const intersection_canonicalization_header &header,
    const event_interning_tables &interning,
    const event_coordinate_tables &coordinates,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    const intersection_descriptor_tables &descriptors,
    const canonical_intersection_complex<T, I> &artifact) {
  if (!artifact.owner().same_owner(header.owner) ||
      artifact.operation() != header.operation ||
      artifact.provider() !=
          intersection_provider_kind::canonical_lineage_event_arrangement_v1 ||
      artifact.verification() !=
          intersection_verification_disposition::not_verified ||
      artifact.schema_version() != header.schema_version ||
      artifact.provider_version() != header.provider_version ||
      artifact.semantic_policy_version() != header.semantic_policy_version ||
      artifact.codec_version() != header.codec_version ||
      artifact.verifier_version() != header.verifier_version ||
      !(artifact.context_digest() == header.context_digest) ||
      !(artifact.precision_digest() == header.precision_digest) ||
      !(artifact.relation_digest() == header.relation_digest) ||
      artifact.source_semantic_digests() != header.source_semantic_digests ||
      artifact.exact_triangulation_digests() !=
          header.exact_triangulation_digests)
    return false;

#define YGOR_SAME_ACCESSOR(accessor, expected)                               \
  if (artifact.accessor() != (expected))                                    \
    return false
  YGOR_SAME_ACCESSOR(construction_witness_index,
                     coordinates.construction_witness_index);
  YGOR_SAME_ACCESSOR(event_binding_index, interning.event_binding_index);
  YGOR_SAME_ACCESSOR(occurrence_binding_index,
                     interning.occurrence_binding_index);
  YGOR_SAME_ACCESSOR(seed_to_event, interning.seed_to_event);
  YGOR_SAME_ACCESSOR(seed_to_occurrence, interning.seed_to_occurrence);
  YGOR_SAME_ACCESSOR(incidence_by_event, incidence.by_event);
  YGOR_SAME_ACCESSOR(incidence_by_occurrence, incidence.by_occurrence);
  YGOR_SAME_ACCESSOR(incidence_by_seed, incidence.by_seed);
  YGOR_SAME_ACCESSOR(incidence_by_seed_candidate,
                     incidence.seed_candidate_index);
  if (!same_ranges(artifact.event_incidence_ranges(), incidence.event_ranges))
    return false;
  if (!same_ranges(artifact.occurrence_incidence_ranges(),
                   incidence.occurrence_ranges))
    return false;
  if (!same_ranges(artifact.seed_incidence_ranges(), incidence.seed_ranges))
    return false;
  if (!same_ranges(artifact.seed_candidate_incidence_ranges(),
                   incidence.seed_candidate_ranges))
    return false;
  YGOR_SAME_ACCESSOR(incidence_by_relation, incidence.by_relation);
  if (!same_ranges(artifact.relation_incidence_ranges(),
                   incidence.relation_ranges))
    return false;
  YGOR_SAME_ACCESSOR(incidence_by_candidate, incidence.by_candidate);
  if (!same_ranges(artifact.candidate_incidence_ranges(),
                   incidence.candidate_ranges))
    return false;
  YGOR_SAME_ACCESSOR(incidence_by_source_feature,
                     incidence.by_source_feature);
  YGOR_SAME_ACCESSOR(incidence_by_halfedge, incidence.by_halfedge);
  YGOR_SAME_ACCESSOR(source_edge_membership_sequence_index,
                     source_edges.membership_sequence_index);
  YGOR_SAME_ACCESSOR(source_edge_cluster_occurrence_index,
                     source_edges.cluster_occurrence_index);
  YGOR_SAME_ACCESSOR(source_edge_cluster_membership_index,
                     source_edges.cluster_membership_index);
  YGOR_SAME_ACCESSOR(source_edge_sequence_cluster_index,
                     source_edges.sequence_cluster_index);
  YGOR_SAME_ACCESSOR(source_edge_sequence_interval_index,
                     source_edges.sequence_interval_index);
  YGOR_SAME_ACCESSOR(carrier_relation_provenance,
                     transverse.carrier_relation_provenance);
  YGOR_SAME_ACCESSOR(carrier_candidate_provenance,
                     transverse.carrier_candidate_provenance);
  YGOR_SAME_ACCESSOR(carrier_membership_index,
                     transverse.carrier_membership_index);
  YGOR_SAME_ACCESSOR(carrier_cluster_occurrence_index,
                     transverse.cluster_occurrence_index);
  YGOR_SAME_ACCESSOR(carrier_cluster_membership_index,
                     transverse.cluster_membership_index);
  YGOR_SAME_ACCESSOR(transverse_carrier_cluster_index,
                     transverse.carrier_cluster_index);
  YGOR_SAME_ACCESSOR(transverse_carrier_span_index,
                     transverse.carrier_span_index);
  YGOR_SAME_ACCESSOR(carrier_span_relation_provenance,
                     transverse.span_relation_provenance);
  YGOR_SAME_ACCESSOR(carrier_span_region_incidence,
                     transverse.span_region_incidence);
  YGOR_SAME_ACCESSOR(coplanar_support_relation_provenance,
                     coplanar.support_relation_provenance);
  YGOR_SAME_ACCESSOR(coplanar_support_candidate_provenance,
                     coplanar.support_candidate_provenance);
  YGOR_SAME_ACCESSOR(coplanar_support_original_boundary_edge_index,
                     coplanar.support_original_boundary_edge_index);
  YGOR_SAME_ACCESSOR(coplanar_support_boundary_event_index,
                     coplanar.support_boundary_event_index);
  YGOR_SAME_ACCESSOR(coplanar_support_boundary_carrier_index,
                     coplanar.support_boundary_carrier_index);
  YGOR_SAME_ACCESSOR(coplanar_support_overlap_index,
                     coplanar.support_overlap_index);
  YGOR_SAME_ACCESSOR(coplanar_support_region_index,
                     coplanar.support_region_index);
  YGOR_SAME_ACCESSOR(overlap_carrier_relation_provenance,
                     coplanar.carrier_relation_provenance);
  YGOR_SAME_ACCESSOR(overlap_carrier_candidate_provenance,
                     coplanar.carrier_candidate_provenance);
  YGOR_SAME_ACCESSOR(overlap_carrier_source_provenance,
                     coplanar.carrier_source_provenance);
  YGOR_SAME_ACCESSOR(coplanar_overlap_boundary_event_index,
                     coplanar.overlap_boundary_event_index);
  YGOR_SAME_ACCESSOR(coplanar_overlap_boundary_carrier_index,
                     coplanar.overlap_boundary_carrier_index);
  YGOR_SAME_ACCESSOR(coplanar_overlap_relation_provenance,
                     coplanar.overlap_relation_provenance);
  YGOR_SAME_ACCESSOR(coplanar_region_boundary_event_index,
                     coplanar.region_boundary_event_index);
  YGOR_SAME_ACCESSOR(coplanar_region_boundary_carrier_index,
                     coplanar.region_boundary_carrier_index);
  YGOR_SAME_ACCESSOR(coplanar_region_coverage_witness_index,
                     coplanar.region_coverage_witness_index);
  YGOR_SAME_ACCESSOR(crossing_aggregate_members,
                     aggregates.crossing_members);
  YGOR_SAME_ACCESSOR(crossing_facet_subtotal_members,
                     aggregates.facet_subtotal_members);
  YGOR_SAME_ACCESSOR(crossing_shell_subtotal_members,
                     aggregates.shell_subtotal_members);
  YGOR_SAME_ACCESSOR(contact_aggregate_members, aggregates.contact_members);
  YGOR_SAME_ACCESSOR(descriptor_provenance, descriptors.provenance);
#undef YGOR_SAME_ACCESSOR

  if (!same_records(artifact.events(), interning.events, same_event) ||
      !same_records(artifact.occurrences(), interning.occurrences,
                    same_occurrence) ||
      !same_records(artifact.seed_bindings(), interning.seed_bindings,
                    same_seed_binding) ||
      !same_records(artifact.incidence(), incidence.records, same_incidence) ||
      !same_records(artifact.source_feature_incidence_ranges(),
                    incidence.source_feature_ranges,
                    same_source_feature_range) ||
      !same_records(artifact.halfedge_incidence_ranges(),
                    incidence.halfedge_ranges, same_halfedge_range) ||
      !same_records(artifact.source_edge_memberships(),
                    source_edges.memberships, same_source_membership) ||
      !same_records(artifact.source_edge_sequences(), source_edges.sequences,
                    same_source_sequence) ||
      !same_records(artifact.source_edge_clusters(), source_edges.clusters,
                    same_source_cluster) ||
      !same_records(artifact.source_edge_intervals(), source_edges.intervals,
                    same_source_interval) ||
      !same_records(artifact.transverse_carriers(), transverse.carriers,
                    same_transverse_carrier) ||
      !same_records(artifact.carrier_active_spans(), transverse.spans,
                    same_carrier_span) ||
      !same_records(artifact.coplanar_supports(), coplanar.supports,
                    same_coplanar_support) ||
      !same_records(artifact.overlap_carriers(), coplanar.carriers,
                    same_overlap_carrier) ||
      !same_records(artifact.coplanar_overlaps(), coplanar.overlaps,
                    same_overlap) ||
      !same_records(artifact.coplanar_region_incidence(), coplanar.regions,
                    same_region) ||
      !same_records(artifact.crossing_aggregates(), aggregates.crossing,
                    same_crossing_aggregate) ||
      !same_records(artifact.crossing_facet_subtotals(),
                    aggregates.facet_subtotals, same_subtotal) ||
      !same_records(artifact.crossing_shell_subtotals(),
                    aggregates.shell_subtotals, same_subtotal) ||
      !same_records(artifact.contact_aggregates(), aggregates.contact,
                    same_contact_aggregate) ||
      !same_records(artifact.descriptors(), descriptors.records,
                    same_descriptor))
    return false;

  const auto offset = source_edges.ordering_certificates.size();
  if (!same_records(
          artifact.carrier_memberships(), transverse.memberships,
          [offset](const carrier_membership_record &a,
                   const carrier_membership_record &b) {
            return same_carrier_membership(a, b, offset);
          }) ||
      !same_records(
          artifact.carrier_clusters(), transverse.clusters,
          [offset](const carrier_cluster_record &a,
                   const carrier_cluster_record &b) {
            return same_carrier_cluster(a, b, offset);
          }))
    return false;
  if (artifact.ordering_certificates().size() !=
      offset + transverse.ordering_certificates.size())
    return false;
  for (std::size_t i = 0; i < offset; ++i)
    if (!same_certificate(artifact.ordering_certificates()[i],
                          source_edges.ordering_certificates[i]))
      return false;
  for (std::size_t i = 0; i < transverse.ordering_certificates.size(); ++i) {
    auto expected = transverse.ordering_certificates[i];
    expected.id = ordering_certificate_id{expected.id.ordinal() + offset};
    if (!same_certificate(artifact.ordering_certificates()[offset + i],
                          expected))
      return false;
  }
  for (std::size_t i = 0; i < transverse.memberships.size(); ++i)
    if (artifact.carrier_memberships()[i].ordering_certificate !=
        remap_certificate(transverse.memberships[i].ordering_certificate,
                          offset))
      return false;
  for (std::size_t i = 0; i < transverse.clusters.size(); ++i)
    if (artifact.carrier_clusters()[i].ordering_certificate !=
        remap_certificate(transverse.clusters[i].ordering_certificate, offset))
      return false;

  bool ok = true;
  const auto expected_statistics =
      make_statistics(interning, coordinates, incidence, source_edges, transverse,
                      coplanar, aggregates, descriptors, ok);
  const auto &evidence = artifact.verification_evidence();
  return ok && same_statistics(artifact.statistics(), expected_statistics) &&
         artifact.section_digests() ==
             std::array<bounded_boolean_digest, 8>{} &&
         evidence.schema_version ==
             contract_versions::intersection_exhaustive_evidence_schema &&
         evidence.verifier_version == contract_versions::intersection_verifier &&
         !evidence.seed_regrouped && !evidence.incidence_reconstructed &&
         !evidence.arrangements_reconstructed &&
         !evidence.descriptors_reconstructed && !evidence.exhaustive_mode &&
         evidence.reserved8 == 0 &&
         evidence.reconstructed_digest == bounded_boolean_digest{} &&
         evidence.work_units == 0 && artifact.diagnostics().empty() &&
         artifact.replay_checkpoints().empty() &&
         artifact.canonical_bytes().empty() &&
         artifact.digest() == bounded_boolean_digest{};
}

} // namespace

template <class T, class I>
bool canonicalize_intersection_tables(
    const intersection_canonicalization_header &header,
    const event_interning_tables &interning,
    const event_coordinate_tables &coordinates,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    const intersection_descriptor_tables &descriptors,
    canonical_intersection_complex<T, I> &artifact,
    bounded_boolean_error &error) {
  if (!valid_header(header) ||
      !valid_intersection_inputs(interning, coordinates, incidence,
                                 source_edges, transverse, coplanar, aggregates,
                                 descriptors)) {
    error = canonicalization_error(
        "Component 08 canonical table input is malformed");
    return false;
  }
  bool ok = true;
  const auto statistics =
      make_statistics(interning, coordinates, incidence, source_edges, transverse,
                      coplanar, aggregates, descriptors, ok);
  if (!ok) {
    error = canonicalization_error(
        "Component 08 canonical table byte accounting overflowed");
    return false;
  }
  canonical_intersection_complex<T, I> candidate;
  intersection_builder<T, I>::assign(
      candidate, header, interning, coordinates, incidence, source_edges,
      transverse, coplanar, aggregates, descriptors, statistics);
  if (!verify_projection(header, interning, coordinates, incidence,
                         source_edges, transverse, coplanar, aggregates,
                         descriptors, candidate)) {
    error = canonicalization_error(
        "Component 08 canonical table producer invariant failed");
    return false;
  }
  artifact = std::move(candidate);
  return true;
}

template <class T, class I>
bool verify_intersection_canonicalization(
    const intersection_canonicalization_header &header,
    const event_interning_tables &interning,
    const event_coordinate_tables &coordinates,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    const intersection_descriptor_tables &descriptors,
    const canonical_intersection_complex<T, I> &artifact,
    bounded_boolean_error &error) {
  if (!valid_header(header) ||
      !valid_intersection_inputs(interning, coordinates, incidence,
                                 source_edges, transverse, coplanar, aggregates,
                                 descriptors) ||
      !verify_projection(header, interning, coordinates, incidence,
                         source_edges, transverse, coplanar, aggregates,
                         descriptors, artifact)) {
    error = canonicalization_verifier_error(
        "Component 08 canonical table verifier rejected the artifact");
    return false;
  }
  return true;
}

#define YGOR_INSTANTIATE_INTERSECTION_CANONICALIZATION(T, I)                \
  template bool canonicalize_intersection_tables<T, I>(                    \
      const intersection_canonicalization_header &,                         \
      const event_interning_tables &, const event_coordinate_tables &,      \
      const event_incidence_tables &,                                       \
      const source_edge_arrangement_tables &,                               \
      const transverse_carrier_arrangement_tables &,                        \
      const coplanar_carrier_arrangement_tables &,                          \
      const intersection_aggregate_tables &,                                \
      const intersection_descriptor_tables &,                              \
      canonical_intersection_complex<T, I> &, bounded_boolean_error &);     \
  template bool verify_intersection_canonicalization<T, I>(                \
      const intersection_canonicalization_header &,                         \
      const event_interning_tables &, const event_coordinate_tables &,      \
      const event_incidence_tables &,                                       \
      const source_edge_arrangement_tables &,                               \
      const transverse_carrier_arrangement_tables &,                        \
      const coplanar_carrier_arrangement_tables &,                          \
      const intersection_aggregate_tables &,                                \
      const intersection_descriptor_tables &,                              \
      const canonical_intersection_complex<T, I> &, bounded_boolean_error &)

YGOR_INSTANTIATE_INTERSECTION_CANONICALIZATION(float, std::uint32_t);
YGOR_INSTANTIATE_INTERSECTION_CANONICALIZATION(float, std::uint64_t);
YGOR_INSTANTIATE_INTERSECTION_CANONICALIZATION(double, std::uint32_t);
YGOR_INSTANTIATE_INTERSECTION_CANONICALIZATION(double, std::uint64_t);

#undef YGOR_INSTANTIATE_INTERSECTION_CANONICALIZATION

} // namespace ygor::mesh_boolean::bounded
