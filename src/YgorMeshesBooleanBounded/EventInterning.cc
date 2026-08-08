#include "EventInterning.h"

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <tuple>

namespace ygor::mesh_boolean::bounded {
namespace {

bool fail(bounded_boolean_error &error, intersection_subcode subcode,
          const char *summary, intersection_checkpoint checkpoint) {
  error = intersection_error(subcode,
                             bounded_boolean_error_category::input_contract_error,
                             summary, checkpoint);
  return false;
}

bool proposal_less(const normalized_event_seed_proposal &a,
                   const normalized_event_seed_proposal &b) noexcept {
  return std::tie(a.event_key, a.occurrence_key, a.seed_key, a.seed) <
         std::tie(b.event_key, b.occurrence_key, b.seed_key, b.seed);
}

bool occurrence_flags(const normalized_event_seed_proposal &proposal,
                      intersection_occurrence_record &record) noexcept {
  const auto role = proposal.occurrence_key.discriminator.role;
  record.may_share_output_coordinate = true;
  record.topology_separate = role != occurrence_role::single_occurrence;
  record.local_cluster_compatible =
      role == occurrence_role::single_occurrence ||
      role == occurrence_role::coincident_sheet_member ||
      role == occurrence_role::symbolic_side_occurrence;
  record.requires_contact_separation =
      role == occurrence_role::topology_separated_contact ||
      role == occurrence_role::coincident_sheet_member ||
      role == occurrence_role::overlap_boundary_occurrence;
  return true;
}

} // namespace

bool intern_normalized_event_seeds(
    const std::vector<normalized_event_seed_proposal> &proposals,
    event_interning_tables &tables, bounded_boolean_error &error) {
  tables = {};
  if (proposals.empty())
    return true;

  std::vector<std::size_t> order(proposals.size());
  std::iota(order.begin(), order.end(), std::size_t{0});
  std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
    return proposal_less(proposals[a], proposals[b]);
  });

  for (std::size_t i = 0; i < order.size(); ++i) {
    const auto &proposal = proposals[order[i]];
    if (proposal.seed.ordinal() >= proposals.size() ||
        proposal.schema_version !=
            contract_versions::intersection_seed_binding_schema ||
        proposal.reserved != 0 || !proposal.precision_evidence_complete)
      return fail(error, intersection_subcode::malformed_seed,
                  "Component 08 interning received a malformed normalized proposal",
                  intersection_checkpoint::event_grouping);
    if (i != 0) {
      const auto &previous = proposals[order[i - 1]];
      if (proposal.seed_key == previous.seed_key)
        return fail(error, intersection_subcode::duplicate_event_incompatible,
                    "Component 08 encountered a duplicate event-seed key",
                    intersection_checkpoint::event_grouping);
    }
  }

  try {
    tables.seed_bindings.resize(proposals.size());
    tables.seed_to_event.assign(proposals.size(),
                                event_id{intersection_invalid_ordinal});
    tables.seed_to_occurrence.assign(
        proposals.size(),
        event_occurrence_id{intersection_invalid_ordinal});
    tables.event_binding_index.reserve(proposals.size());
    tables.occurrence_binding_index.reserve(proposals.size());
    tables.events.reserve(proposals.size());
    tables.occurrences.reserve(proposals.size());
  } catch (...) {
    return fail(error, intersection_subcode::resource_preflight,
                "Component 08 could not reserve interning tables",
                intersection_checkpoint::event_occurrence_id_assignment);
  }

  std::vector<event_id> proposal_events(
      proposals.size(), event_id{intersection_invalid_ordinal});
  std::vector<event_occurrence_id> proposal_occurrences(
      proposals.size(), event_occurrence_id{intersection_invalid_ordinal});
  std::size_t cursor = 0;
  while (cursor < order.size()) {
    const auto event_begin = cursor;
    const auto &event_key = proposals[order[cursor]].event_key;
    while (cursor < order.size() &&
           proposals[order[cursor]].event_key == event_key)
      ++cursor;
    const auto event_end = cursor;

    intersection_event_record event_record;
    event_record.id = event_id{tables.events.size()};
    event_record.key = event_key;
    event_record.point.construction =
        proposals[order[event_begin]].construction;
    event_record.point.source_vertex =
        proposals[order[event_begin]].accepted_source_vertex;
    event_record.point.kind =
        event_record.point.source_vertex.kind ==
                relation_feature_kind::source_vertex
            ? bounded_point_reference_kind::source_point
            : bounded_point_reference_kind::constructed_point;
    event_record.occurrences.begin = tables.occurrences.size();
    event_record.seed_bindings.begin = tables.event_binding_index.size();

    for (std::size_t i = event_begin; i < event_end; ++i) {
      const auto &proposal = proposals[order[i]];
      if (proposal.construction != event_record.point.construction ||
          proposal.accepted_source_vertex != event_record.point.source_vertex)
        return fail(error,
                    intersection_subcode::authoritative_construction_conflict,
                    "Component 08 event group has incompatible point authority",
                    intersection_checkpoint::event_grouping);
    }

    std::size_t occurrence_cursor = event_begin;
    while (occurrence_cursor < event_end) {
      const auto occurrence_begin = occurrence_cursor;
      const auto &occurrence_key =
          proposals[order[occurrence_cursor]].occurrence_key;
      while (occurrence_cursor < event_end &&
             proposals[order[occurrence_cursor]].occurrence_key ==
                 occurrence_key)
        ++occurrence_cursor;

      intersection_occurrence_record occurrence_record;
      occurrence_record.id = event_occurrence_id{tables.occurrences.size()};
      occurrence_record.event = event_record.id;
      occurrence_record.key = occurrence_key;
      occurrence_record.seed_bindings.begin =
          tables.occurrence_binding_index.size();
      occurrence_flags(proposals[order[occurrence_begin]], occurrence_record);

      for (std::size_t i = occurrence_begin; i < occurrence_cursor; ++i) {
        const auto proposal_index = order[i];
        proposal_events[proposal_index] = event_record.id;
        proposal_occurrences[proposal_index] = occurrence_record.id;
        tables.occurrence_binding_index.push_back(
            event_seed_binding_id{proposals[proposal_index].seed.ordinal()});
      }
      occurrence_record.seed_bindings.count =
          tables.occurrence_binding_index.size() -
          occurrence_record.seed_bindings.begin;
      tables.occurrences.push_back(std::move(occurrence_record));
    }

    for (std::size_t i = event_begin; i < event_end; ++i)
      tables.event_binding_index.push_back(
          event_seed_binding_id{proposals[order[i]].seed.ordinal()});
    event_record.occurrences.count =
        tables.occurrences.size() - event_record.occurrences.begin;
    event_record.seed_bindings.count =
        tables.event_binding_index.size() - event_record.seed_bindings.begin;
    tables.events.push_back(std::move(event_record));
  }

  for (std::size_t proposal_index = 0; proposal_index < proposals.size();
       ++proposal_index) {
    const auto &proposal = proposals[proposal_index];
    const auto seed_ordinal = proposal.seed.ordinal();
    if (seed_ordinal >= proposals.size())
      return fail(error, intersection_subcode::seed_mapping_incomplete,
                  "Component 08 seed ordinal is not dense",
                  intersection_checkpoint::event_occurrence_id_assignment);
    auto &binding = tables.seed_bindings[seed_ordinal];
    binding.id = event_seed_binding_id{seed_ordinal};
    binding.seed = proposal.seed;
    binding.seed_key = proposal.seed_key;
    binding.canonical_seed_ordinal = seed_ordinal;
    binding.event = proposal_events[proposal_index];
    binding.occurrence = proposal_occurrences[proposal_index];
    binding.relation = proposal.relation;
    binding.construction = proposal.construction;
    binding.accepted_source_vertex = proposal.accepted_source_vertex;
    binding.incidence = proposal.declared_incidence;
    binding.candidate_incidence = proposal.declared_candidate_incidence;
    binding.expected_membership_role = proposal.expected_membership_role;
    binding.expected_carrier_role = proposal.expected_carrier_role;
    binding.designated_authority = proposal.designated_authority;
    binding.duplicate_consumer = proposal.duplicate_consumer;
    binding.compatibility_verified = true;
    tables.seed_to_event[seed_ordinal] = binding.event;
    tables.seed_to_occurrence[seed_ordinal] = binding.occurrence;
  }

  return verify_event_interning_tables(proposals, tables, error);
}

bool verify_event_interning_tables(
    const std::vector<normalized_event_seed_proposal> &proposals,
    const event_interning_tables &tables,
    bounded_boolean_error &error) {
  if (tables.seed_bindings.size() != proposals.size() ||
      tables.seed_to_event.size() != proposals.size() ||
      tables.seed_to_occurrence.size() != proposals.size() ||
      tables.event_binding_index.size() != proposals.size() ||
      tables.occurrence_binding_index.size() != proposals.size())
    return fail(error, intersection_subcode::seed_mapping_incomplete,
                "Component 08 interning tables do not cover every seed",
                intersection_checkpoint::producer_invariants);

  for (std::size_t i = 0; i < tables.events.size(); ++i) {
    const auto &event = tables.events[i];
    if (event.id.ordinal() != i || !valid_intersection_event_key(event.key) ||
        event.occurrences.count == 0 || event.seed_bindings.count == 0 ||
        event.occurrences.begin > tables.occurrences.size() ||
        event.occurrences.count >
            tables.occurrences.size() - event.occurrences.begin ||
        event.seed_bindings.begin > tables.event_binding_index.size() ||
        event.seed_bindings.count >
            tables.event_binding_index.size() - event.seed_bindings.begin ||
        (i != 0 && !(tables.events[i - 1].key < event.key)))
      return fail(error, intersection_subcode::internal_invariant,
                  "Component 08 event table invariant failed",
                  intersection_checkpoint::producer_invariants);
  }

  for (std::size_t i = 0; i < tables.occurrences.size(); ++i) {
    const auto &occurrence = tables.occurrences[i];
    if (occurrence.id.ordinal() != i ||
        occurrence.event.ordinal() >= tables.events.size() ||
        !valid_intersection_occurrence_key(occurrence.key) ||
        occurrence.key.event != tables.events[occurrence.event.ordinal()].key ||
        occurrence.seed_bindings.count == 0 ||
        occurrence.seed_bindings.begin >
            tables.occurrence_binding_index.size() ||
        occurrence.seed_bindings.count >
            tables.occurrence_binding_index.size() -
                occurrence.seed_bindings.begin ||
        (i != 0 && !(tables.occurrences[i - 1].key < occurrence.key)))
      return fail(error, intersection_subcode::internal_invariant,
                  "Component 08 occurrence table invariant failed",
                  intersection_checkpoint::producer_invariants);
  }

  std::vector<std::uint8_t> seen(proposals.size(), 0);
  for (std::size_t i = 0; i < tables.seed_bindings.size(); ++i) {
    const auto &binding = tables.seed_bindings[i];
    if (binding.id.ordinal() != i || binding.seed.ordinal() != i ||
        binding.canonical_seed_ordinal != i ||
        binding.event.ordinal() >= tables.events.size() ||
        binding.occurrence.ordinal() >= tables.occurrences.size() ||
        tables.occurrences[binding.occurrence.ordinal()].event != binding.event ||
        !binding.compatibility_verified)
      return fail(error, intersection_subcode::seed_mapping_incomplete,
                  "Component 08 seed binding invariant failed",
                  intersection_checkpoint::producer_invariants);
    seen[i] = 1;
  }
  if (std::find(seen.begin(), seen.end(), std::uint8_t{0}) != seen.end())
    return fail(error, intersection_subcode::seed_mapping_incomplete,
                "Component 08 left an event seed unbound",
                intersection_checkpoint::producer_invariants);
  return true;
}

} // namespace ygor::mesh_boolean::bounded
