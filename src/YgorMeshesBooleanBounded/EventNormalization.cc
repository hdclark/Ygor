#include "EventNormalization.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace ygor::mesh_boolean::bounded {
namespace {

bool valid_contact_status(feature_relation_status status) noexcept {
  return status >= feature_relation_status::proper_crossing &&
         status <= feature_relation_status::coincidence_opposite_orientation;
}

intersection_event_class classify_event(
    const relation_event_seed_record &seed,
    const relation_construction_record &construction) noexcept {
  if (seed.accepted_source_vertex_reused ||
      construction.precedence ==
          relation_construction_precedence::accepted_source_vertex)
    return intersection_event_class::source_vertex_contact;
  if (seed.contact_status == feature_relation_status::tangency)
    return intersection_event_class::tangent_point;
  if (seed.contact_status == feature_relation_status::segment_contact ||
      seed.contact_status == feature_relation_status::overlap ||
      seed.contact_status == feature_relation_status::coincidence_same_orientation ||
      seed.contact_status ==
          feature_relation_status::coincidence_opposite_orientation ||
      construction.precedence ==
          relation_construction_precedence::coplanar_overlap_endpoint)
    return intersection_event_class::overlap_endpoint;
  if (seed.key.family == feature_relation_family::source_edge_source_edge)
    return intersection_event_class::edge_edge_point;
  if (seed.key.family == feature_relation_family::source_edge_source_facet)
    return intersection_event_class::edge_facet_point;
  if (seed.key.family == feature_relation_family::symbolic_contact)
    return intersection_event_class::symbolic_tie;
  if (seed.key.family == feature_relation_family::source_vertex_source_facet)
    return intersection_event_class::source_vertex_contact;
  return intersection_event_class::multi_feature_meeting;
}

occurrence_role classify_occurrence(
    const relation_event_seed_record &seed,
    intersection_event_class event_class) noexcept {
  if (!seed.distinct_occurrence_required)
    return occurrence_role::single_occurrence;
  if (event_class == intersection_event_class::overlap_endpoint)
    return occurrence_role::overlap_boundary_occurrence;
  if (seed.has_symbolic_decision)
    return occurrence_role::symbolic_side_occurrence;
  if (seed.contact_status ==
          feature_relation_status::coincidence_same_orientation ||
      seed.contact_status ==
          feature_relation_status::coincidence_opposite_orientation)
    return occurrence_role::coincident_sheet_member;
  if (seed.symbolic_occurrence_rank != 0)
    return occurrence_role::multiplicity_occurrence;
  return occurrence_role::topology_separated_contact;
}

intersection_carrier_role classify_carrier(
    const relation_event_seed_record &seed,
    const relation_construction_record &construction,
    intersection_event_class event_class) noexcept {
  if (construction.precedence ==
          relation_construction_precedence::coplanar_overlap_endpoint ||
      event_class == intersection_event_class::overlap_endpoint) {
    if (seed.conceptual_side == symbolic_relation_side::negative)
      return intersection_carrier_role::overlap_start;
    if (seed.conceptual_side == symbolic_relation_side::positive)
      return intersection_carrier_role::overlap_end;
    return intersection_carrier_role::coplanar_boundary;
  }
  if (seed.key.family == feature_relation_family::source_facet_source_facet)
    return intersection_carrier_role::transverse_endpoint;
  return intersection_carrier_role::none;
}

intersection_membership_role classify_membership(
    const relation_event_seed_record &seed,
    intersection_event_class event_class) noexcept {
  if (seed.accepted_source_vertex_reused)
    return intersection_membership_role::endpoint;
  if (event_class == intersection_event_class::overlap_endpoint) {
    if (seed.conceptual_side == symbolic_relation_side::negative)
      return intersection_membership_role::overlap_start;
    if (seed.conceptual_side == symbolic_relation_side::positive)
      return intersection_membership_role::overlap_end;
  }
  return intersection_membership_role::interior;
}

bool range_valid(std::uint64_t begin, std::uint64_t count,
                 std::uint64_t size) noexcept {
  return begin <= size && count <= size - begin;
}

bool fail(bounded_boolean_error &error, intersection_subcode subcode,
          const char *summary) {
  error = intersection_error(subcode,
                             bounded_boolean_error_category::input_contract_error,
                             summary,
                             intersection_checkpoint::seed_normalization);
  return false;
}

} // namespace

bool normalize_event_seed_records(
    const std::vector<relation_event_seed_record> &seeds,
    const std::vector<relation_construction_record> &constructions,
    std::vector<normalized_event_seed_proposal> &proposals,
    bounded_boolean_error &error) {
  proposals.clear();
  try {
    proposals.reserve(seeds.size());
  } catch (...) {
    return fail(error, intersection_subcode::resource_preflight,
                "Component 08 could not reserve seed normalization proposals");
  }

  for (std::size_t ordinal = 0; ordinal < seeds.size(); ++ordinal) {
    const auto &seed = seeds[ordinal];
    if (seed.id.ordinal() != ordinal ||
        seed.schema_version != contract_versions::relation_event_seed_schema ||
        seed.reserved != 0 || !valid_contact_status(seed.contact_status) ||
        seed.contact_dimension == relation_contact_dimension::none ||
        !seed.precision_evidence_complete)
      return fail(error, intersection_subcode::malformed_seed,
                  "Component 08 rejected a malformed Component 07 event seed");
    if (seed.key.family == feature_relation_family::triangle_local_witness ||
        seed.key.first.kind == relation_feature_kind::source_triangle ||
        seed.key.first.kind == relation_feature_kind::facet_internal_diagonal ||
        seed.key.second.kind == relation_feature_kind::source_triangle ||
        seed.key.second.kind == relation_feature_kind::facet_internal_diagonal)
      return fail(error, intersection_subcode::internal_diagonal_public_ownership,
                  "Component 08 event identity cannot be owned by triangle-local topology");
    if (!valid_relation_event_seed_key(seed.key))
      return fail(error, intersection_subcode::malformed_seed,
                  "Component 08 rejected an invalid Component 07 event-seed key");
    if (seed.construction.ordinal() >= constructions.size())
      return fail(error, intersection_subcode::malformed_reference,
                  "Component 08 seed construction reference is out of range");

    const auto &construction = constructions[seed.construction.ordinal()];
    if (construction.id != seed.construction ||
        construction.source_relation != seed.source_relation ||
        construction.kind != seed.construction_kind ||
        construction.component_count == 0 ||
        construction.component_count > construction.nominal_bits.size() ||
        !construction.finite || !construction.tolerance_compatible ||
        !construction.precision_evidence_complete ||
        construction.reserved != 0)
      return fail(error, intersection_subcode::authoritative_construction_conflict,
                  "Component 08 seed and authoritative construction disagree");
    if (construction.kind != relation_construction_kind::bounded_point)
      return fail(error, intersection_subcode::missing_authoritative_point,
                  "Component 08 point event seed lacks a bounded-point construction");
    if (!range_valid(seed.construction_ledger_begin,
                     seed.construction_ledger_count,
                     std::numeric_limits<std::uint64_t>::max()) ||
        !range_valid(seed.incidence_begin, seed.incidence_count,
                     std::numeric_limits<std::uint64_t>::max()) ||
        !range_valid(seed.candidate_incidence_begin,
                     seed.candidate_incidence_count,
                     std::numeric_limits<std::uint64_t>::max()))
      return fail(error, intersection_subcode::count_overflow,
                  "Component 08 seed range arithmetic overflowed");
    if (seed.accepted_source_vertex_reused &&
        (!valid_relation_feature_key(seed.accepted_source_vertex) ||
         seed.accepted_source_vertex.kind != relation_feature_kind::source_vertex ||
         construction.precedence !=
             relation_construction_precedence::accepted_source_vertex ||
         construction.authoritative_source_feature !=
             seed.accepted_source_vertex ||
         !construction.accepted_source_vertex))
      return fail(error, intersection_subcode::source_vertex_point_mismatch,
                  "Component 08 accepted source-vertex authority is inconsistent");
    if (!seed.accepted_source_vertex_reused &&
        construction.precedence ==
            relation_construction_precedence::accepted_source_vertex)
      return fail(error, intersection_subcode::source_vertex_point_mismatch,
                  "Component 08 source-vertex construction lacks accepted-vertex seed evidence");

    normalized_event_seed_proposal proposal;
    proposal.seed = seed.id;
    proposal.seed_key = seed.key;
    proposal.relation = seed.source_relation;
    proposal.construction = seed.construction;
    proposal.accepted_source_vertex = seed.accepted_source_vertex;
    proposal.declared_construction_witnesses = {
        seed.construction_ledger_begin, seed.construction_ledger_count};
    proposal.declared_incidence = {seed.incidence_begin, seed.incidence_count};
    proposal.declared_candidate_incidence = {
        seed.candidate_incidence_begin, seed.candidate_incidence_count};
    proposal.numeric_crossing = seed.numeric_crossing;
    proposal.symbolic_crossing = seed.symbolic_crossing;
    proposal.half_open_owner = seed.half_open_owner;
    proposal.designated_authority =
        construction.precedence !=
        relation_construction_precedence::verification_witness;
    proposal.distinct_occurrence_required = seed.distinct_occurrence_required;
    proposal.precision_evidence_complete = seed.precision_evidence_complete;

    auto &event = proposal.event_key;
    event.semantic_namespace = seed.key.semantic_namespace;
    event.event_class = classify_event(seed, construction);
    event.first_operand = seed.key.first.operand;
    event.second_operand = seed.key.second.operand;
    event.first_owner = seed.key.first;
    event.second_owner = seed.key.second;
    event.public_relation = seed.key;
    event.public_relation.occurrence = 0;
    event.construction_kind = construction.kind;
    event.construction_precedence = construction.precedence;
    event.authoritative_source_feature =
        construction.authoritative_source_feature;
    event.reused_source_vertex = seed.accepted_source_vertex_reused
                                     ? seed.accepted_source_vertex
                                     : relation_feature_key{};
    event.construction_source_provenance = construction.source_provenance;
    event.construction_geometric_lineage = construction.geometric_lineage;
    event.carrier_role = classify_carrier(seed, construction, event.event_class);
    event.contact_status = seed.contact_status;
    event.contact_dimension = seed.contact_dimension;
    event.symbolic_rule_ordinal =
        seed.has_symbolic_decision ? seed.symbolic_rule_ordinal : 0;
    event.symbolic_tie_key_schema =
        seed.has_symbolic_decision ? seed.symbolic_tie_key_schema : 0;

    auto &occurrence = proposal.occurrence_key;
    occurrence.event = event;
    occurrence.discriminator.role = classify_occurrence(seed, event.event_class);
    occurrence.discriminator.component07_occurrence = seed.key.occurrence;
    occurrence.discriminator.symbolic_side = seed.conceptual_side;
    occurrence.discriminator.symbolic_priority = seed.symbolic_occurrence_rank;
    occurrence.discriminator.multiplicity_slot = seed.symbolic_occurrence_rank;
    occurrence.discriminator.occurrence_lineage =
        seed.distinct_occurrence_required
            ? (seed.symbolic_subject_ordinal != 0
                   ? seed.symbolic_subject_ordinal
                   : std::uint64_t{seed.key.occurrence} + 1)
            : 0;

    proposal.expected_membership_role =
        classify_membership(seed, event.event_class);
    proposal.expected_carrier_role = event.carrier_role;

    if (!valid_intersection_event_key(event) ||
        !valid_intersection_occurrence_key(occurrence))
      return fail(error, intersection_subcode::malformed_event_key,
                  "Component 08 produced an invalid normalized event or occurrence key");

    try {
      proposals.push_back(std::move(proposal));
    } catch (...) {
      return fail(error, intersection_subcode::resource_preflight,
                  "Component 08 could not retain a normalized seed proposal");
    }
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
