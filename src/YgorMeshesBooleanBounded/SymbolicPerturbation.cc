#include "StrictFloatingBuild.h"
#include "SymbolicPerturbation.h"

namespace ygor::mesh_boolean::bounded {
namespace {

bool eligible(const symbolic_eligibility_record &record) noexcept {
  return valid_relation_request_key(record.request) &&
         record.exact_relation == exact_relation_status::exact_zero &&
         record.reason != symbolic_eligibility_reason::none &&
         record.evidence_formula_version != 0 &&
         (record.exact_lineage_tie || record.representational_tie_evidence) &&
         record.structural_category_eligible && record.tolerance_compatible &&
         !record.separated_realizations_possible &&
         record.owner_is_original_source_feature && record.reserved8 == 0 &&
         record.reserved == 0;
}

symbolic_relation_side side_from_contribution(std::int8_t value) noexcept {
  return value < 0 ? symbolic_relation_side::negative
                   : (value > 0 ? symbolic_relation_side::positive
                                : symbolic_relation_side::coincident);
}

bounded_boolean_error symbolic_error(relation_subcode subcode,
                                     const char *summary) {
  return relation_error(subcode,
                        bounded_boolean_error_category::internal_invariant_error,
                        summary,
                        relation_checkpoint::symbolic_matrix_lookup);
}

} // namespace

boolean_outcome<symbolic_relation_decision_record>
resolve_symbolic_relation_decision(
    const symbolic_policy_table &table, boolean_operation operation,
    operand_id acting_operand, relation_family family,
    orientation_relation orientation,
    const symbolic_eligibility_record &eligibility) {
  if (!verify_symbolic_policy(table) || !eligible(eligibility))
    return boolean_outcome<symbolic_relation_decision_record>::failure(
        symbolic_error(relation_subcode::symbolic_ineligible,
                       "symbolic relation is not eligible for matrix lookup"));

  const symbolic_rule *match = nullptr;
  std::uint64_t match_ordinal = 0;
  for (std::uint64_t i = 0; i < table.rules.size(); ++i) {
    const auto &rule = table.rules[static_cast<std::size_t>(i)];
    if (rule.operation == operation && rule.acting_operand == acting_operand &&
        rule.relation == family && rule.orientation == orientation) {
      if (match)
        return boolean_outcome<symbolic_relation_decision_record>::failure(
            symbolic_error(relation_subcode::internal_invariant,
                           "symbolic matrix lookup is not unique"));
      match = &rule;
      match_ordinal = i;
    }
  }
  if (!match)
    return boolean_outcome<symbolic_relation_decision_record>::failure(
        symbolic_error(relation_subcode::unsupported_version,
                       "symbolic matrix lookup is not total"));

  symbolic_relation_decision_record decision;
  decision.request = eligibility.request;
  decision.operation = operation;
  decision.acting_operand = acting_operand;
  decision.matrix_family = family;
  decision.orientation = orientation;
  decision.stable_rule_ordinal = match_ordinal;
  decision.feature_priority = match->feature_priority;
  decision.half_open_owner = match->half_open_owner;
  decision.symbolic_crossing_contribution = match->crossing_contribution;
  decision.coincident_owner_rank = match->coincident_owner;
  decision.conceptual_side = side_from_contribution(match->crossing_contribution);
  decision.occurrence_separation_required =
      family == relation_family::vertex_vertex ||
      family == relation_family::equal_edge ||
      family == relation_family::coincident_face;
  decision.nominal_geometry_unchanged = true;
  bounded_boolean_error error;
  if (!verify_symbolic_relation_decision(table, eligibility, decision, error))
    return boolean_outcome<symbolic_relation_decision_record>::failure(error);
  return boolean_outcome<symbolic_relation_decision_record>::success(
      std::move(decision));
}

bool verify_symbolic_relation_decision(
    const symbolic_policy_table &table,
    const symbolic_eligibility_record &eligibility,
    const symbolic_relation_decision_record &decision,
    bounded_boolean_error &error) {
  const auto fail = [&](relation_subcode subcode, const char *summary) {
    error = symbolic_error(subcode, summary);
    return false;
  };
  if (!verify_symbolic_policy(table) || !eligible(eligibility) ||
      decision.request != eligibility.request || decision.reserved != 0)
    return fail(relation_subcode::verifier_rejection,
                "symbolic relation eligibility or request mismatch");
  if (decision.stable_rule_ordinal >= table.rules.size())
    return fail(relation_subcode::unsupported_version,
                "symbolic relation rule ordinal is out of range");
  const auto &rule = table.rules[decision.stable_rule_ordinal];
  if (rule.operation != decision.operation ||
      rule.acting_operand != decision.acting_operand ||
      rule.relation != decision.matrix_family ||
      rule.orientation != decision.orientation ||
      rule.feature_priority != decision.feature_priority ||
      rule.half_open_owner != decision.half_open_owner ||
      rule.crossing_contribution !=
          decision.symbolic_crossing_contribution ||
      rule.coincident_owner != decision.coincident_owner_rank ||
      decision.conceptual_side !=
          side_from_contribution(rule.crossing_contribution))
    return fail(relation_subcode::verifier_rejection,
                "symbolic relation does not reproduce its frozen matrix rule");
  if (!decision.nominal_geometry_unchanged)
    return fail(relation_subcode::symbolic_geometry_change,
                "symbolic relation attempted to alter nominal geometry");
  return true;
}

} // namespace ygor::mesh_boolean::bounded
