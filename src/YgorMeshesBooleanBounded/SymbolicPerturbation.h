#pragma once

#include "RelationKeys.h"
#include "SymbolicPolicy.h"
#include "Outcome.h"

#include <cstdint>

namespace ygor::mesh_boolean::bounded {

struct symbolic_eligibility_record final {
  relation_request_key request{};
  exact_relation_status exact_relation = exact_relation_status::unavailable;
  bool exact_lineage_tie = false;
  bool representational_tie_evidence = false;
  bool structural_category_eligible = false;
  bool tolerance_compatible = false;
  bool rounded_nominal_zero = false; // diagnostic only
  std::uint32_t reserved = 0;
};

struct symbolic_relation_decision_record final {
  symbolic_relation_decision_id id{0};
  relation_request_key request{};
  boolean_operation operation = boolean_operation::set_union;
  operand_id acting_operand = operand_id::a;
  relation_family matrix_family = relation_family::vertex_vertex;
  orientation_relation orientation = orientation_relation::indeterminate;
  std::uint64_t stable_rule_ordinal = 0;
  std::uint8_t feature_priority = 0;
  operand_id half_open_owner = operand_id::a;
  std::int8_t symbolic_crossing_contribution = 0;
  operand_id coincident_owner_rank = operand_id::a;
  symbolic_relation_side conceptual_side = symbolic_relation_side::coincident;
  bool occurrence_separation_required = false;
  bool nominal_geometry_unchanged = true;
  std::uint32_t reserved = 0;
};

boolean_outcome<symbolic_relation_decision_record>
resolve_symbolic_relation_decision(
    const symbolic_policy_table &table, boolean_operation operation,
    operand_id acting_operand, relation_family family,
    orientation_relation orientation,
    const symbolic_eligibility_record &eligibility);

bool verify_symbolic_relation_decision(
    const symbolic_policy_table &table,
    const symbolic_eligibility_record &eligibility,
    const symbolic_relation_decision_record &decision,
    bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
