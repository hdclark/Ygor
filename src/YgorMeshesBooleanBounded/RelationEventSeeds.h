#pragma once

#include "SignedFeatureRelations.h"
#include "Outcome.h"

#include <vector>

namespace ygor::mesh_boolean::bounded {

struct relation_event_seed_proposal final {
  relation_event_seed_key key{};
  feature_relation_id source_relation{0};
  relation_construction_id construction{0};
  feature_relation_status contact_status =
      feature_relation_status::not_evaluated;
  relation_contact_dimension contact_dimension = relation_contact_dimension::none;
  relation_construction_kind construction_kind =
      relation_construction_kind::bounded_point;
  relation_feature_key accepted_source_vertex{};
  bool accepted_source_vertex_reused = false;
  bool has_symbolic_decision = false;
  symbolic_relation_decision_id symbolic_decision{0};
  std::uint64_t symbolic_rule_ordinal = 0;
  std::uint64_t symbolic_exchange_rule_ordinal = 0;
  symbolic_relation_subject_kind symbolic_subject_kind =
      symbolic_relation_subject_kind::relation;
  std::uint64_t symbolic_subject_ordinal = 0;
  std::uint32_t symbolic_occurrence_rank = 0;
  symbolic_relation_side conceptual_side = symbolic_relation_side::coincident;
  symbolic_offset_disposition conceptual_order =
      symbolic_offset_disposition::coincident;
  symbolic_contact_class symbolic_contact =
      symbolic_contact_class::point_contact;
  symbolic_expected_disposition symbolic_expected =
      symbolic_expected_disposition::classification_only;
  symbolic_explanation_code symbolic_explanation =
      symbolic_explanation_code::exact_vertex_tie;
  std::uint16_t symbolic_tie_key_schema = 0;
  operand_id coincident_owner_rank = operand_id::a;
  bool symbolic_owner_rank_eligible = false;
  std::int32_t numeric_crossing = 0;
  std::int8_t symbolic_crossing = 0;
  operand_id half_open_owner = operand_id::a;
  std::uint64_t truth_begin = 0;
  std::uint64_t truth_count = 0;
  std::uint64_t construction_ledger_begin = 0;
  std::uint64_t construction_ledger_count = 0;
  std::vector<relation_feature_key> incidence;
  std::vector<relation_event_seed_candidate_incidence_record>
      candidate_incidence;
  bool precision_evidence_complete = false;
  bool distinct_occurrence_required = false;
};

struct relation_event_seed_table final {
  std::vector<relation_event_seed_record> records;
  std::vector<relation_feature_key> incidence;
  std::vector<relation_event_seed_candidate_incidence_record>
      candidate_incidence;
  bounded_boolean_digest semantic_digest{};
};

boolean_outcome<relation_event_seed_table> canonicalize_relation_event_seeds(
    std::vector<relation_event_seed_proposal> proposals,
    const relation_capabilities &capabilities);

std::vector<std::uint8_t>
encode_relation_event_seed_table(const relation_event_seed_table &table);

} // namespace ygor::mesh_boolean::bounded
