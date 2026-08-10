#pragma once

#include "FloatingBits.h"
#include "PredicateResults.h"
#include "SignedFeatureRelations.h"

#include <cstdint>
#include <limits>
#include <utility>

namespace ygor::mesh_boolean::bounded {

bounded_boolean_error primitive_relation_error(
    relation_subcode subcode, const char *summary,
    relation_checkpoint checkpoint = relation_checkpoint::truth_record_assembly);

bool valid_relation_truth_record(const relation_truth_record &record) noexcept;

template <class T>
boolean_outcome<relation_truth_record> make_relation_truth_record(
    const predicate_result<T> &predicate,
    rounded_operation_code rounded_formula,
    exact_relation_formula_code exact_formula) {
  static_assert(supported_precision_scalar_v<T>);

  if (!valid_predicate_result(predicate) ||
      !registered_rounded_operation(rounded_formula) ||
      !valid_exact_formula_code(static_cast<std::uint16_t>(exact_formula)))
    return boolean_outcome<relation_truth_record>::failure(
        primitive_relation_error(
            relation_subcode::bounded_operation_invalid,
            "Component 07 primitive formula binding is unsupported",
            relation_checkpoint::rounded_primitive_evaluation));

  const bool exact_formula_requested =
      exact_formula != exact_relation_formula_code::invalid;
  if (predicate.exact_relation.formula_code !=
          static_cast<std::uint16_t>(exact_formula) ||
      predicate.exact_relation.status == exact_relation_status::invalid ||
      (exact_formula_requested &&
       predicate.exact_relation.status == exact_relation_status::unavailable) ||
      (!exact_formula_requested &&
       predicate.exact_relation.status != exact_relation_status::unavailable))
    return boolean_outcome<relation_truth_record>::failure(
        primitive_relation_error(
            relation_subcode::exact_relation_invalid,
            "Component 07 exact relation formula or evidence is invalid",
            relation_checkpoint::exact_relation_evaluation));

  if (predicate.bounded_sign != classify_bounded_sign(
                                    predicate.rounded_and_bounded
                                        .uncertainty_enclosure) ||
      predicate.bounded_sign == bounded_sign_status::invalid ||
      predicate.disposition == predicate_disposition::fail_invalid)
    return boolean_outcome<relation_truth_record>::failure(
        primitive_relation_error(
            relation_subcode::truth_layer_mismatch,
            "Component 07 predicate truth layers are inconsistent"));

  relation_truth_record result;
  result.rounded_nominal_bits = static_cast<std::uint64_t>(
      to_bits(predicate.rounded_and_bounded.rounded_nominal));
  result.bounded_sign = predicate.bounded_sign;
  result.exact_relation = predicate.exact_relation.status;
  result.disposition = predicate.disposition;
  result.rounded_formula = static_cast<std::uint16_t>(rounded_formula);
  result.exact_formula = static_cast<std::uint16_t>(exact_formula);
  if (!valid_relation_truth_record(result))
    return boolean_outcome<relation_truth_record>::failure(
        primitive_relation_error(
            relation_subcode::truth_layer_mismatch,
            "Component 07 assembled primitive truth record is invalid"));
  return boolean_outcome<relation_truth_record>::success(std::move(result));
}

template <class T>
boolean_outcome<relation_truth_record> assemble_relation_truth_record(
    bounded_scalar<T> rounded_and_bounded,
    const exact_relation_record &exact,
    rounded_operation_code rounded_formula,
    bool alternate_available = false) {
  const bool exact_formula_requested =
      exact.formula != exact_relation_formula_code::invalid;
  if (exact.evaluation_status != numeric_status::success ||
      exact.status == exact_relation_status::invalid ||
      (exact_formula_requested &&
       (exact.status == exact_relation_status::unavailable ||
        !registered_exact_formula(exact.formula))) ||
      (!exact_formula_requested &&
       exact.status != exact_relation_status::unavailable) ||
      exact.capacity_used > exact.capacity_limit ||
      exact.capacity_used > std::numeric_limits<std::uint32_t>::max())
    return boolean_outcome<relation_truth_record>::failure(
        primitive_relation_error(
            relation_subcode::exact_relation_invalid,
            "Component 07 exact relation evidence is invalid or not representable",
            relation_checkpoint::exact_relation_evaluation));

  exact_relation_evidence evidence;
  evidence.formula_code = static_cast<std::uint16_t>(exact.formula);
  evidence.owner = rounded_and_bounded.identity.owner;
  evidence.status = exact.status;
  evidence.normalization_exponent = exact.normalization_exponent;
  evidence.capacity_used = static_cast<std::uint32_t>(exact.capacity_used);

  auto predicate = assemble_predicate_result(
      std::move(rounded_and_bounded), std::move(evidence),
      alternate_available);
  if (!predicate.has_value())
    return boolean_outcome<relation_truth_record>::failure(*predicate.error());
  return make_relation_truth_record(
      *predicate.value(), rounded_formula, exact.formula);
}

} // namespace ygor::mesh_boolean::bounded
