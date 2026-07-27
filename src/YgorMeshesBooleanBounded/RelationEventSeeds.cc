#include "StrictFloatingBuild.h"
#include "RelationEventSeeds.h"

#include <algorithm>
#include <new>
#include <tuple>
#include <utility>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error seed_error(
    relation_subcode subcode, const char *summary,
    bounded_boolean_error_category category =
        bounded_boolean_error_category::internal_invariant_error) {
  return relation_error(
      subcode, category, summary,
      relation_checkpoint::event_seed_and_disposition_reconciliation);
}

bool proposal_less(const relation_event_seed_proposal &a,
                   const relation_event_seed_proposal &b) noexcept {
  return a.key < b.key;
}

void canonicalize_incidence(std::vector<relation_feature_key> &incidence) {
  std::sort(incidence.begin(), incidence.end());
  incidence.erase(std::unique(incidence.begin(), incidence.end()),
                  incidence.end());
}

bool valid_operand(operand_id operand) noexcept {
  return operand == operand_id::a || operand == operand_id::b;
}

bool valid_contact_status(feature_relation_status status) noexcept {
  const auto value = static_cast<std::uint8_t>(status);
  return value >= static_cast<std::uint8_t>(
                      feature_relation_status::proper_crossing) &&
         value <= static_cast<std::uint8_t>(
                      feature_relation_status::coincidence_opposite_orientation);
}

bool valid_contact_dimension(relation_contact_dimension dimension) noexcept {
  const auto value = static_cast<std::uint8_t>(dimension);
  return value >= 1 && value <= 3;
}

bool valid_construction_kind(relation_construction_kind kind) noexcept {
  const auto value = static_cast<std::uint8_t>(kind);
  return value >= 1 && value <= 4;
}

bool valid_conceptual_side(symbolic_relation_side side) noexcept {
  return side == symbolic_relation_side::negative ||
         side == symbolic_relation_side::coincident ||
         side == symbolic_relation_side::positive;
}

bool candidate_incidence_less(
    const relation_event_seed_candidate_incidence_record &a,
    const relation_event_seed_candidate_incidence_record &b) noexcept {
  return std::tie(a.candidate, a.candidate_edge, a.source_triangle,
                  a.edge_halfedges, a.triangle_halfedges,
                  a.internal_diagonal_witness, a.source_feature_owner) <
         std::tie(b.candidate, b.candidate_edge, b.source_triangle,
                  b.edge_halfedges, b.triangle_halfedges,
                  b.internal_diagonal_witness, b.source_feature_owner);
}

bool candidate_incidence_equal(
    const relation_event_seed_candidate_incidence_record &a,
    const relation_event_seed_candidate_incidence_record &b) noexcept {
  return std::tie(a.candidate, a.disposition, a.candidate_edge,
                  a.source_triangle, a.edge_halfedges, a.triangle_halfedges,
                  a.internal_diagonal_witness, a.source_feature_owner,
                  a.schema_version, a.reserved) ==
         std::tie(b.candidate, b.disposition, b.candidate_edge,
                  b.source_triangle, b.edge_halfedges, b.triangle_halfedges,
                  b.internal_diagonal_witness, b.source_feature_owner,
                  b.schema_version, b.reserved);
}

bool valid_candidate_incidence(
    const relation_event_seed_candidate_incidence_record &record) noexcept {
  return valid_relation_feature_key(record.candidate_edge) &&
         valid_relation_feature_key(record.source_triangle) &&
         record.candidate_edge.operand != record.source_triangle.operand &&
         record.source_triangle.kind == relation_feature_kind::source_triangle &&
         record.disposition.ordinal() == record.candidate.ordinal() &&
         record.schema_version ==
             contract_versions::relation_event_seed_incidence_schema &&
         record.reserved == 0;
}

bool canonicalize_candidate_incidence(
    std::vector<relation_event_seed_candidate_incidence_record> &records) {
  for (const auto &record : records)
    if (!valid_candidate_incidence(record))
      return false;
  std::sort(records.begin(), records.end(), candidate_incidence_less);
  std::vector<relation_event_seed_candidate_incidence_record> unique;
  unique.reserve(records.size());
  for (const auto &record : records) {
    if (!unique.empty() && unique.back().candidate == record.candidate) {
      if (!candidate_incidence_equal(unique.back(), record))
        return false;
      continue;
    }
    unique.push_back(record);
  }
  records = std::move(unique);
  return true;
}

bool proposal_semantics_equal(const relation_event_seed_proposal &a,
                              const relation_event_seed_proposal &b) noexcept {
  return std::tie(a.source_relation, a.construction, a.contact_status,
                  a.contact_dimension, a.construction_kind,
                  a.accepted_source_vertex,
                  a.accepted_source_vertex_reused, a.has_symbolic_decision,
                  a.symbolic_decision, a.symbolic_rule_ordinal,
                  a.symbolic_occurrence_rank, a.conceptual_side,
                  a.numeric_crossing, a.symbolic_crossing, a.half_open_owner,
                  a.truth_begin, a.truth_count,
                  a.construction_ledger_begin,
                  a.construction_ledger_count,
                  a.precision_evidence_complete,
                  a.distinct_occurrence_required) ==
         std::tie(b.source_relation, b.construction, b.contact_status,
                  b.contact_dimension, b.construction_kind,
                  b.accepted_source_vertex,
                  b.accepted_source_vertex_reused, b.has_symbolic_decision,
                  b.symbolic_decision, b.symbolic_rule_ordinal,
                  b.symbolic_occurrence_rank, b.conceptual_side,
                  b.numeric_crossing, b.symbolic_crossing, b.half_open_owner,
                  b.truth_begin, b.truth_count,
                  b.construction_ledger_begin,
                  b.construction_ledger_count,
                  b.precision_evidence_complete,
                  b.distinct_occurrence_required);
}

bool valid_proposal_semantics(const relation_event_seed_proposal &proposal) {
  if (!valid_contact_status(proposal.contact_status) ||
      !valid_contact_dimension(proposal.contact_dimension) ||
      !valid_construction_kind(proposal.construction_kind) ||
      !valid_operand(proposal.half_open_owner) ||
      !valid_conceptual_side(proposal.conceptual_side) ||
      proposal.numeric_crossing < -1 || proposal.numeric_crossing > 1 ||
      proposal.symbolic_crossing < -1 || proposal.symbolic_crossing > 1 ||
      proposal.truth_count == 0 || proposal.construction_ledger_count == 0 ||
      !proposal.precision_evidence_complete)
    return false;
  if (!valid_relation_feature_key(proposal.accepted_source_vertex, true))
    return false;
  if (proposal.accepted_source_vertex_reused)
    return proposal.accepted_source_vertex.kind ==
           relation_feature_kind::source_vertex;
  return proposal.accepted_source_vertex.kind == relation_feature_kind::none;
}

void encode_candidate_incidence(
    canonical_writer &writer,
    const relation_event_seed_candidate_incidence_record &record) {
  writer.u64(record.id.ordinal());
  writer.u64(record.seed.ordinal());
  writer.u64(record.candidate.ordinal());
  writer.u64(record.disposition.ordinal());
  encode_relation_feature_key(writer, record.candidate_edge);
  encode_relation_feature_key(writer, record.source_triangle);
  for (const auto halfedge : record.edge_halfedges)
    writer.u64(halfedge);
  for (const auto halfedge : record.triangle_halfedges)
    writer.u64(halfedge);
  writer.boolean(record.internal_diagonal_witness);
  writer.boolean(record.source_feature_owner);
  writer.u16(record.schema_version);
  writer.u32(record.reserved);
}

} // namespace

std::vector<std::uint8_t>
encode_relation_event_seed_table(const relation_event_seed_table &table) {
  canonical_writer writer;
  writer.u32(0x37534559U); // YES7
  writer.u16(contract_versions::relation_event_seed_schema);
  writer.u64(table.records.size());
  writer.u64(table.incidence.size());
  writer.u64(table.candidate_incidence.size());
  for (const auto &record : table.records) {
    writer.u64(record.id.ordinal());
    encode_relation_event_seed_key(writer, record.key);
    writer.u64(record.source_relation.ordinal());
    writer.u64(record.construction.ordinal());
    writer.u8(static_cast<std::uint8_t>(record.contact_status));
    writer.u8(static_cast<std::uint8_t>(record.contact_dimension));
    writer.u8(static_cast<std::uint8_t>(record.construction_kind));
    encode_relation_feature_key(writer, record.accepted_source_vertex);
    writer.boolean(record.accepted_source_vertex_reused);
    writer.boolean(record.has_symbolic_decision);
    writer.u64(record.symbolic_decision.ordinal());
    writer.u64(record.symbolic_rule_ordinal);
    writer.u32(record.symbolic_occurrence_rank);
    writer.u8(static_cast<std::uint8_t>(record.conceptual_side));
    writer.u32(static_cast<std::uint32_t>(record.numeric_crossing));
    writer.u8(static_cast<std::uint8_t>(record.symbolic_crossing));
    writer.u8(static_cast<std::uint8_t>(record.half_open_owner));
    writer.u64(record.truth_begin);
    writer.u64(record.truth_count);
    writer.u64(record.construction_ledger_begin);
    writer.u64(record.construction_ledger_count);
    writer.u64(record.incidence_begin);
    writer.u64(record.incidence_count);
    writer.u64(record.candidate_incidence_begin);
    writer.u64(record.candidate_incidence_count);
    writer.boolean(record.precision_evidence_complete);
    writer.boolean(record.distinct_occurrence_required);
    writer.u16(record.schema_version);
    writer.u32(record.reserved);
  }
  for (const auto &feature : table.incidence)
    encode_relation_feature_key(writer, feature);
  for (const auto &record : table.candidate_incidence)
    encode_candidate_incidence(writer, record);
  return writer.take();
}

boolean_outcome<relation_event_seed_table> canonicalize_relation_event_seeds(
    std::vector<relation_event_seed_proposal> proposals,
    const relation_capabilities &capabilities) {
  try {
    if (!capabilities.owner.anchor)
      return boolean_outcome<relation_event_seed_table>::failure(seed_error(
          relation_subcode::wrong_owner,
          "event-seed canonicalization owner is not bound"));
    if (relation_cancelled(capabilities))
      return boolean_outcome<relation_event_seed_table>::failure(seed_error(
          relation_subcode::cancelled,
          "event-seed canonicalization cancelled",
          bounded_boolean_error_category::cancelled));
    if (proposals.size() > capabilities.maximum_event_seeds)
      return boolean_outcome<relation_event_seed_table>::failure(seed_error(
          relation_subcode::work_limit, "event-seed limit exceeded",
          bounded_boolean_error_category::resource_limit));
    for (auto &proposal : proposals) {
      if (!valid_relation_event_seed_key(proposal.key) ||
          !valid_proposal_semantics(proposal))
        return boolean_outcome<relation_event_seed_table>::failure(seed_error(
            relation_subcode::malformed_request_key,
            "event seed has malformed semantic evidence"));
      for (const auto &feature : proposal.incidence)
        if (!valid_relation_feature_key(feature))
          return boolean_outcome<relation_event_seed_table>::failure(seed_error(
              relation_subcode::malformed_request_key,
              "event seed has malformed source-feature incidence"));
      canonicalize_incidence(proposal.incidence);
      if (!canonicalize_candidate_incidence(proposal.candidate_incidence))
        return boolean_outcome<relation_event_seed_table>::failure(seed_error(
            relation_subcode::candidate_disposition_contradiction,
            "event seed has contradictory candidate incidence"));
    }
    std::sort(proposals.begin(), proposals.end(), proposal_less);

    relation_event_seed_table table;
    for (std::size_t begin = 0; begin < proposals.size();) {
      std::size_t end = begin + 1;
      while (end < proposals.size() &&
             proposals[end].key == proposals[begin].key)
        ++end;
      std::vector<relation_feature_key> incidence;
      std::vector<relation_event_seed_candidate_incidence_record>
          candidate_incidence;
      for (std::size_t i = begin; i < end; ++i) {
        if (!proposal_semantics_equal(proposals[begin], proposals[i]))
          return boolean_outcome<relation_event_seed_table>::failure(seed_error(
              relation_subcode::candidate_disposition_contradiction,
              "duplicate event-seed producers disagree"));
        incidence.insert(incidence.end(), proposals[i].incidence.begin(),
                         proposals[i].incidence.end());
        candidate_incidence.insert(candidate_incidence.end(),
                                   proposals[i].candidate_incidence.begin(),
                                   proposals[i].candidate_incidence.end());
      }
      canonicalize_incidence(incidence);
      if (!canonicalize_candidate_incidence(candidate_incidence))
        return boolean_outcome<relation_event_seed_table>::failure(seed_error(
            relation_subcode::candidate_disposition_contradiction,
            "duplicate event-seed candidate incidence disagrees"));
      if (incidence.size() < 2 || candidate_incidence.empty())
        return boolean_outcome<relation_event_seed_table>::failure(seed_error(
            relation_subcode::candidate_disposition_missing,
            "event seed lacks complete source and candidate incidence"));

      const auto &source = proposals[begin];
      relation_event_seed_record record;
      record.id = relation_event_seed_id(table.records.size());
      record.key = source.key;
      record.source_relation = source.source_relation;
      record.construction = source.construction;
      record.contact_status = source.contact_status;
      record.contact_dimension = source.contact_dimension;
      record.construction_kind = source.construction_kind;
      record.accepted_source_vertex = source.accepted_source_vertex;
      record.accepted_source_vertex_reused =
          source.accepted_source_vertex_reused;
      record.has_symbolic_decision = source.has_symbolic_decision;
      record.symbolic_decision = source.symbolic_decision;
      record.symbolic_rule_ordinal = source.symbolic_rule_ordinal;
      record.symbolic_occurrence_rank = source.symbolic_occurrence_rank;
      record.conceptual_side = source.conceptual_side;
      record.numeric_crossing = source.numeric_crossing;
      record.symbolic_crossing = source.symbolic_crossing;
      record.half_open_owner = source.half_open_owner;
      record.truth_begin = source.truth_begin;
      record.truth_count = source.truth_count;
      record.construction_ledger_begin = source.construction_ledger_begin;
      record.construction_ledger_count = source.construction_ledger_count;
      record.incidence_begin = table.incidence.size();
      record.incidence_count = incidence.size();
      record.candidate_incidence_begin = table.candidate_incidence.size();
      record.candidate_incidence_count = candidate_incidence.size();
      record.precision_evidence_complete = source.precision_evidence_complete;
      record.distinct_occurrence_required =
          source.distinct_occurrence_required;
      table.records.push_back(record);
      table.incidence.insert(table.incidence.end(), incidence.begin(),
                             incidence.end());
      for (auto &candidate : candidate_incidence) {
        candidate.id = relation_event_seed_incidence_id(
            table.candidate_incidence.size());
        candidate.seed = record.id;
        table.candidate_incidence.push_back(candidate);
      }
      begin = end;
    }
    if (table.incidence.size() > capabilities.maximum_consumers ||
        table.candidate_incidence.size() >
            capabilities.maximum_event_seed_incidence)
      return boolean_outcome<relation_event_seed_table>::failure(seed_error(
          relation_subcode::work_limit, "event-seed incidence limit exceeded",
          bounded_boolean_error_category::resource_limit));
    table.semantic_digest =
        sha256::digest(encode_relation_event_seed_table(table));
    return boolean_outcome<relation_event_seed_table>::success(std::move(table));
  } catch (const std::bad_alloc &) {
    return boolean_outcome<relation_event_seed_table>::failure(seed_error(
        relation_subcode::resource_preflight,
        "event-seed canonicalization allocation failed",
        bounded_boolean_error_category::resource_limit));
  } catch (...) {
    return boolean_outcome<relation_event_seed_table>::failure(seed_error(
        relation_subcode::internal_invariant,
        "event-seed canonicalization raised an unexpected exception"));
  }
}

} // namespace ygor::mesh_boolean::bounded
