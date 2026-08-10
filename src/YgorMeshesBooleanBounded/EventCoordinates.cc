#include "EventCoordinates.h"

#include <algorithm>
#include <cstdint>

namespace ygor::mesh_boolean::bounded {
namespace {

bool fail(bounded_boolean_error &error, intersection_subcode subcode,
          const char *summary) {
  error = intersection_error(subcode,
                             bounded_boolean_error_category::input_contract_error,
                             summary,
                             intersection_checkpoint::authoritative_point_attachment);
  return false;
}

bool range_valid(intersection_range range, std::uint64_t size) noexcept {
  return range.begin <= size && range.count <= size - range.begin;
}

bool valid_ledger_record(const relation_construction_ledger_record &ledger,
                         std::size_t ordinal) noexcept {
  return ledger.id.ordinal() == ordinal && ledger.component_count != 0 &&
         ledger.component_count <= ledger.nominal_bits.size() && ledger.finite &&
         ledger.tolerance_compatible && ledger.lineage_compatible &&
         ledger.enclosure_compatible && ledger.parameter_compatible &&
         ledger.residual_compatible && ledger.precision_evidence_complete &&
         ledger.reserved == 0;
}

} // namespace

bool attach_event_coordinates(
    const std::vector<normalized_event_seed_proposal> &proposals,
    const std::vector<relation_construction_record> &constructions,
    const std::vector<relation_construction_ledger_record> &construction_ledger,
    event_interning_tables &interning,
    event_coordinate_tables &coordinates,
    bounded_boolean_error &error) {
  coordinates = event_coordinate_tables{};
  std::vector<std::size_t> proposal_by_seed(
      proposals.size(), proposals.size());
  for (std::size_t i = 0; i < proposals.size(); ++i) {
    const auto seed = proposals[i].seed.ordinal();
    if (seed >= proposal_by_seed.size() || proposal_by_seed[seed] != proposals.size())
      return fail(error, intersection_subcode::seed_mapping_incomplete,
                  "Component 08 coordinate attachment seed mapping is not dense");
    proposal_by_seed[seed] = i;
  }

  try {
    coordinates.construction_witness_index.reserve(construction_ledger.size());
  } catch (...) {
    return fail(error, intersection_subcode::resource_preflight,
                "Component 08 could not reserve construction-witness index");
  }

  for (auto &event : interning.events) {
    if (event.point.construction.ordinal() >= constructions.size() ||
        !range_valid(event.seed_bindings, interning.event_binding_index.size()))
      return fail(error, intersection_subcode::missing_authoritative_point,
                  "Component 08 event point authority is out of range");
    const auto &construction = constructions[event.point.construction.ordinal()];
    if (construction.id != event.point.construction ||
        construction.kind != relation_construction_kind::bounded_point ||
        !construction.finite || !construction.tolerance_compatible ||
        !construction.precision_evidence_complete ||
        construction.ledger_begin > construction_ledger.size() ||
        construction.ledger_count == 0 ||
        construction.ledger_count >
            construction_ledger.size() - construction.ledger_begin)
      return fail(error, intersection_subcode::missing_authoritative_point,
                  "Component 08 event bounded-point authority is malformed");

    const auto &authority = construction_ledger[construction.ledger_begin];
    if (!valid_ledger_record(authority, construction.ledger_begin) ||
        authority.construction != construction.id ||
        authority.precedence != construction.precedence ||
        !authority.synthetic_authority ||
        authority.nominal_bits != construction.nominal_bits ||
        authority.lower_bits != construction.lower_bits ||
        authority.upper_bits != construction.upper_bits ||
        authority.source_provenance != construction.source_provenance ||
        authority.geometric_lineage != construction.geometric_lineage)
      return fail(error, intersection_subcode::authoritative_construction_conflict,
                  "Component 08 authoritative construction ledger disagrees with its point");

    event.point.precision_ledger = authority.id;
    if (construction.authoritative_source_feature.kind ==
        relation_feature_kind::source_vertex) {
      if (!construction.accepted_source_vertex ||
          event.point.kind != bounded_point_reference_kind::source_point ||
          event.point.source_vertex != construction.authoritative_source_feature)
        return fail(error, intersection_subcode::source_vertex_point_mismatch,
                    "Component 08 source-point reference is inconsistent");
    } else if (event.point.kind !=
                   bounded_point_reference_kind::constructed_point ||
               event.point.source_vertex.kind != relation_feature_kind::none) {
      return fail(error, intersection_subcode::missing_authoritative_point,
                  "Component 08 constructed-point reference is inconsistent");
    }

    event.construction_witnesses.begin =
        coordinates.construction_witness_index.size();
    for (std::uint64_t offset = 0; offset < construction.ledger_count; ++offset) {
      const auto ordinal = construction.ledger_begin + offset;
      const auto &ledger = construction_ledger[ordinal];
      if (!valid_ledger_record(ledger, ordinal) ||
          ledger.construction != construction.id ||
          ledger.coordinate_space != construction.coordinate_space ||
          ledger.component_count != construction.component_count ||
          ledger.projection_axis != construction.projection_axis ||
          ledger.tolerance_boundary_bits != construction.tolerance_boundary_bits)
        return fail(error, intersection_subcode::secondary_witness_incompatible,
                    "Component 08 secondary construction witness is incompatible");
      coordinates.construction_witness_index.push_back(ledger.id);
    }
    event.construction_witnesses.count =
        coordinates.construction_witness_index.size() -
        event.construction_witnesses.begin;

    for (std::uint64_t offset = 0; offset < event.seed_bindings.count; ++offset) {
      const auto binding_id = interning.event_binding_index[
          event.seed_bindings.begin + offset];
      if (binding_id.ordinal() >= interning.seed_bindings.size() ||
          binding_id.ordinal() >= proposal_by_seed.size())
        return fail(error, intersection_subcode::seed_mapping_incomplete,
                    "Component 08 event witness binding is out of range");
      const auto proposal_index = proposal_by_seed[binding_id.ordinal()];
      if (proposal_index >= proposals.size())
        return fail(error, intersection_subcode::seed_mapping_incomplete,
                    "Component 08 event witness proposal is missing");
      const auto &proposal = proposals[proposal_index];
      if (proposal.construction != construction.id ||
          !range_valid(proposal.declared_construction_witnesses,
                       construction_ledger.size()))
        return fail(error, intersection_subcode::authoritative_construction_conflict,
                    "Component 08 grouped seed construction authority changed");
      bool matching_use = false;
      for (std::uint64_t ledger_offset = 0;
           ledger_offset < proposal.declared_construction_witnesses.count;
           ++ledger_offset) {
        const auto &ledger = construction_ledger[
            proposal.declared_construction_witnesses.begin + ledger_offset];
        matching_use = matching_use ||
                       (ledger.construction == proposal.construction &&
                        ledger.source_relation == proposal.relation &&
                        ledger.occurrence == proposal.seed_key.occurrence);
      }
      if (!matching_use)
        return fail(error, intersection_subcode::secondary_witness_incompatible,
                    "Component 08 seed lacks a matching construction-ledger use");
    }
  }

  return verify_event_coordinates(proposals, constructions, construction_ledger,
                                  interning, coordinates, error);
}

bool verify_event_coordinates(
    const std::vector<normalized_event_seed_proposal> &proposals,
    const std::vector<relation_construction_record> &constructions,
    const std::vector<relation_construction_ledger_record> &construction_ledger,
    const event_interning_tables &interning,
    const event_coordinate_tables &coordinates,
    bounded_boolean_error &error) {
  (void)proposals;
  std::uint64_t expected_begin = 0;
  for (const auto &event : interning.events) {
    if (event.point.construction.ordinal() >= constructions.size() ||
        event.point.precision_ledger.ordinal() >= construction_ledger.size() ||
        event.construction_witnesses.begin != expected_begin ||
        event.construction_witnesses.count == 0 ||
        !range_valid(event.construction_witnesses,
                     coordinates.construction_witness_index.size()))
      return fail(error, intersection_subcode::internal_invariant,
                  "Component 08 event coordinate index is not canonical");
    const auto &construction = constructions[event.point.construction.ordinal()];
    if (event.construction_witnesses.count != construction.ledger_count)
      return fail(error, intersection_subcode::secondary_witness_incompatible,
                  "Component 08 event witness range is incomplete");
    for (std::uint64_t offset = 0; offset < event.construction_witnesses.count;
         ++offset) {
      const auto witness = coordinates.construction_witness_index[
          event.construction_witnesses.begin + offset];
      if (witness.ordinal() != construction.ledger_begin + offset ||
          construction_ledger[witness.ordinal()].construction !=
              event.point.construction)
        return fail(error, intersection_subcode::secondary_witness_incompatible,
                    "Component 08 event witness index does not reconstruct");
    }
    expected_begin += event.construction_witnesses.count;
  }
  if (expected_begin != coordinates.construction_witness_index.size())
    return fail(error, intersection_subcode::internal_invariant,
                "Component 08 coordinate witness index has unreachable records");
  for (const auto &occurrence : interning.occurrences) {
    if (occurrence.event.ordinal() >= interning.events.size() ||
        occurrence.key.event !=
            interning.events[occurrence.event.ordinal()].key)
      return fail(error, intersection_subcode::occurrence_semantic_conflict,
                  "Component 08 occurrence does not resolve through its parent event point");
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
