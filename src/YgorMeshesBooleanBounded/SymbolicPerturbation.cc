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

bool valid_subject(symbolic_relation_subject_kind kind) noexcept {
  switch (kind) {
  case symbolic_relation_subject_kind::relation:
  case symbolic_relation_subject_kind::event_occurrence:
  case symbolic_relation_subject_kind::coplanar_component:
    return true;
  }
  return false;
}

symbolic_relation_side side_from_offset(
    symbolic_offset_disposition value) noexcept {
  return value == symbolic_offset_disposition::negative
             ? symbolic_relation_side::negative
         : value == symbolic_offset_disposition::positive
             ? symbolic_relation_side::positive
             : symbolic_relation_side::coincident;
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
    const symbolic_policy_table &table, const symbolic_rule_key &key,
    symbolic_relation_subject_kind subject_kind, std::uint64_t subject_ordinal,
    const symbolic_eligibility_record &eligibility) {
  if (!verify_symbolic_policy(table) || !eligible(eligibility) ||
      !valid_symbolic_rule_key(key) || !valid_subject(subject_kind))
    return boolean_outcome<symbolic_relation_decision_record>::failure(
        symbolic_error(relation_subcode::symbolic_ineligible,
                       "symbolic relation is not eligible for matrix lookup"));

  const auto ordinal = symbolic_rule_ordinal(key);
  if (ordinal >= table.rules.size())
    return boolean_outcome<symbolic_relation_decision_record>::failure(
        symbolic_error(relation_subcode::unsupported_version,
                       "symbolic matrix lookup is not total"));
  const auto &rule = table.rules[static_cast<std::size_t>(ordinal)];
  if (rule.key != key)
    return boolean_outcome<symbolic_relation_decision_record>::failure(
        symbolic_error(relation_subcode::internal_invariant,
                       "symbolic matrix lookup key is inconsistent"));

  symbolic_relation_decision_record decision;
  decision.request = eligibility.request;
  decision.rule_key = rule.key;
  decision.exchanged_rule_key = rule.exchanged_key;
  decision.subject_kind = subject_kind;
  decision.subject_ordinal = subject_ordinal;
  decision.operation = key.operation;
  decision.acting_operand = key.acting_operand;
  decision.matrix_family = key.relation;
  decision.orientation = key.orientation;
  decision.stable_rule_ordinal = ordinal;
  decision.exchange_rule_ordinal = rule.exchange_rule_ordinal;
  decision.feature_priority = rule.feature_priority;
  decision.half_open_owner = rule.half_open_owner;
  decision.symbolic_crossing_contribution = rule.crossing_contribution;
  decision.coincident_owner_rank = rule.coincident_owner;
  decision.conceptual_side = side_from_offset(rule.conceptual_offset);
  decision.conceptual_order = rule.conceptual_offset;
  decision.contact_class = rule.contact_class;
  decision.expected_disposition = rule.expected_disposition;
  decision.explanation = rule.explanation;
  decision.tie_key = rule.tie_key;
  decision.tie_key_schema = rule.tie_key_schema;
  decision.owner_rank_eligible = rule.owner_rank_eligible;
  decision.occurrence_separation_required =
      rule.occurrence_separation_required;
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
      decision.request != eligibility.request ||
      !valid_subject(decision.subject_kind) || decision.reserved8 != 0 ||
      decision.schema_version !=
          contract_versions::relation_symbolic_decision_schema ||
      decision.reserved != 0)
    return fail(relation_subcode::verifier_rejection,
                "symbolic relation eligibility, subject, or request mismatch");
  if (!valid_symbolic_rule_key(decision.rule_key) ||
      decision.stable_rule_ordinal !=
          symbolic_rule_ordinal(decision.rule_key) ||
      decision.stable_rule_ordinal >= table.rules.size())
    return fail(relation_subcode::unsupported_version,
                "symbolic relation rule ordinal is out of range");
  const auto &rule = table.rules[decision.stable_rule_ordinal];
  if (rule.key != decision.rule_key ||
      decision.exchanged_rule_key != rule.exchanged_key ||
      decision.exchange_rule_ordinal != rule.exchange_rule_ordinal ||
      decision.operation != rule.key.operation ||
      decision.acting_operand != rule.key.acting_operand ||
      decision.matrix_family != rule.key.relation ||
      decision.orientation != rule.key.orientation ||
      decision.feature_priority != rule.feature_priority ||
      decision.half_open_owner != rule.half_open_owner ||
      decision.symbolic_crossing_contribution !=
          rule.crossing_contribution ||
      decision.coincident_owner_rank != rule.coincident_owner ||
      decision.conceptual_side != side_from_offset(rule.conceptual_offset) ||
      decision.conceptual_order != rule.conceptual_offset ||
      decision.contact_class != rule.contact_class ||
      decision.expected_disposition != rule.expected_disposition ||
      decision.explanation != rule.explanation ||
      !(decision.tie_key == rule.tie_key) ||
      decision.tie_key_schema != rule.tie_key_schema ||
      decision.owner_rank_eligible != rule.owner_rank_eligible ||
      decision.occurrence_separation_required !=
          rule.occurrence_separation_required)
    return fail(relation_subcode::verifier_rejection,
                "symbolic relation does not reproduce its frozen matrix rule");
  if (decision.exchange_rule_ordinal >= table.rules.size() ||
      table.rules[decision.exchange_rule_ordinal].key !=
          decision.exchanged_rule_key ||
      table.rules[decision.exchange_rule_ordinal].exchange_rule_ordinal !=
          decision.stable_rule_ordinal)
    return fail(relation_subcode::verifier_rejection,
                "symbolic relation operand exchange is not involutive");
  if (!decision.nominal_geometry_unchanged)
    return fail(relation_subcode::symbolic_geometry_change,
                "symbolic relation attempted to alter nominal geometry");
  return true;
}

} // namespace ygor::mesh_boolean::bounded
