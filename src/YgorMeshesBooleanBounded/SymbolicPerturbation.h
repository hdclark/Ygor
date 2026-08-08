#pragma once

#include "Outcome.h"
#include "RelationKeys.h"
#include "SymbolicPolicy.h"

#include <cstdint>

namespace ygor::mesh_boolean::bounded {

enum class symbolic_eligibility_reason : std::uint8_t {
  none = 0,
  exact_formula_zero = 1,
  shared_source_endpoint = 2,
  collinear_source_edge_lineage = 3,
  coplanar_source_facet_lineage = 4,
  equal_source_feature_lineage = 5,
  representational_tie = 6,
  coincident_source_contract = 7,
};

enum class symbolic_relation_subject_kind : std::uint8_t {
  relation = 1,
  event_occurrence = 2,
  coplanar_component = 3,
};

struct symbolic_eligibility_record final {
  relation_request_key request{};
  exact_relation_status exact_relation = exact_relation_status::unavailable;
  symbolic_eligibility_reason reason = symbolic_eligibility_reason::none;
  std::uint16_t evidence_formula_version = 0;
  bool exact_lineage_tie = false;
  bool representational_tie_evidence = false;
  bool structural_category_eligible = false;
  bool tolerance_compatible = false;
  bool rounded_nominal_zero = false; // diagnostic only
  bool inherited_uncertainty = false;
  bool separated_realizations_possible = true;
  bool owner_is_original_source_feature = false;
  std::uint8_t reserved8 = 0;
  std::uint32_t reserved = 0;
};

struct symbolic_relation_decision_record final {
  symbolic_relation_decision_id id{0};
  relation_request_key request{};
  symbolic_rule_key rule_key{};
  symbolic_rule_key exchanged_rule_key{};
  symbolic_relation_subject_kind subject_kind =
      symbolic_relation_subject_kind::relation;
  std::uint64_t subject_ordinal = 0;
  boolean_operation operation = boolean_operation::set_union;
  operand_id acting_operand = operand_id::a;
  relation_family matrix_family = relation_family::vertex_vertex;
  orientation_relation orientation = orientation_relation::indeterminate;
  std::uint64_t stable_rule_ordinal = 0;
  std::uint64_t exchange_rule_ordinal = 0;
  std::uint8_t feature_priority = 0;
  operand_id half_open_owner = operand_id::a;
  std::int8_t symbolic_crossing_contribution = 0;
  operand_id coincident_owner_rank = operand_id::a;
  symbolic_relation_side conceptual_side = symbolic_relation_side::coincident;
  symbolic_offset_disposition conceptual_order =
      symbolic_offset_disposition::coincident;
  symbolic_contact_class contact_class = symbolic_contact_class::point_contact;
  symbolic_expected_disposition expected_disposition =
      symbolic_expected_disposition::classification_only;
  symbolic_explanation_code explanation =
      symbolic_explanation_code::exact_vertex_tie;
  symbolic_tie_key_description tie_key{};
  std::uint16_t tie_key_schema = contract_versions::symbolic_policy;
  bool owner_rank_eligible = false;
  bool occurrence_separation_required = false;
  bool nominal_geometry_unchanged = true;
  std::uint8_t reserved8 = 0;
  std::uint16_t schema_version =
      contract_versions::relation_symbolic_decision_schema;
  std::uint32_t reserved = 0;
};

boolean_outcome<symbolic_relation_decision_record>
resolve_symbolic_relation_decision(
    const symbolic_policy_table &table, const symbolic_rule_key &key,
    symbolic_relation_subject_kind subject_kind, std::uint64_t subject_ordinal,
    const symbolic_eligibility_record &eligibility);

bool verify_symbolic_relation_decision(
    const symbolic_policy_table &table,
    const symbolic_eligibility_record &eligibility,
    const symbolic_relation_decision_record &decision,
    bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
