#pragma once

#include "FloatingBits.h"
#include "PredicateResults.h"
#include "SignedFeatureRelations.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace ygor::mesh_boolean::bounded {

bounded_boolean_error primitive_relation_error(
    relation_subcode subcode, const char *summary,
    relation_checkpoint checkpoint = relation_checkpoint::truth_record_assembly);

bool valid_relation_truth_record(const relation_truth_record &record) noexcept;

inline void encode_relation_truth_record(canonical_writer &writer,
                                         const relation_truth_record &record) {
  writer.u16(record.schema_version);
  writer.u64(record.rounded_nominal_bits);
  writer.u64(record.lower_bits);
  writer.u64(record.upper_bits);
  writer.u64(record.separation_margin_bits);
  writer.u64(record.uncertainty_width_bits);
  for (const auto bits : record.contributor_bits)
    writer.u64(bits);
  writer.u64(record.bounded_value);
  writer.u64(record.source_provenance);
  writer.u64(record.geometric_lineage);
  writer.u64(record.precision_ledger_entry);
  writer.u64(record.trace_root);
  writer.u64(record.exact_evidence);
  writer.u64(record.exact_trace_root);
  for (const auto input : record.exact_ordered_inputs)
    writer.u64(input);
  writer.u8(static_cast<std::uint8_t>(record.bounded_sign));
  writer.u8(static_cast<std::uint8_t>(record.exact_relation));
  writer.u8(static_cast<std::uint8_t>(record.disposition));
  writer.u16(record.rounded_formula);
  writer.u16(record.exact_formula);
  writer.u16(record.bounded_schema_version);
  writer.u16(record.bounded_provider_version);
  writer.u16(record.exact_schema_version);
  writer.u16(record.exact_ordered_input_count);
  writer.u32(static_cast<std::uint32_t>(record.exact_normalization_exponent));
  writer.u32(record.exact_capacity_used);
  writer.u32(record.exact_capacity_limit);
  writer.u8(static_cast<std::uint8_t>(record.bounded_publication));
  writer.boolean(record.alternate_formulation_available);
  writer.u32(record.reserved);
}

inline std::vector<std::uint8_t> encode_relation_truth_record_semantics(
    const relation_truth_record &record) {
  canonical_writer writer;
  encode_relation_truth_record(writer, record);
  return writer.take();
}

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
  result.lower_bits = static_cast<std::uint64_t>(to_bits(
      predicate.rounded_and_bounded.uncertainty_enclosure.lower()));
  result.upper_bits = static_cast<std::uint64_t>(to_bits(
      predicate.rounded_and_bounded.uncertainty_enclosure.upper()));
  result.separation_margin_bits =
      static_cast<std::uint64_t>(to_bits(predicate.separation_margin));
  result.uncertainty_width_bits =
      static_cast<std::uint64_t>(to_bits(predicate.uncertainty_width));
  const double contributors[]{predicate.contributors.inherited_a,
                              predicate.contributors.inherited_b,
                              predicate.contributors.machine_floor,
                              predicate.contributors.construction,
                              predicate.contributors.conditioning,
                              predicate.contributors.conversion,
                              predicate.contributors.prior_cleanup,
                              predicate.contributors.current_cleanup};
  for (std::size_t i = 0; i < result.contributor_bits.size(); ++i)
    result.contributor_bits[i] =
        static_cast<std::uint64_t>(to_bits(contributors[i]));
  const auto &identity = predicate.rounded_and_bounded.identity;
  result.bounded_value = identity.value.ordinal();
  result.source_provenance = identity.provenance.ordinal();
  result.geometric_lineage = identity.lineage.ordinal();
  result.precision_ledger_entry = identity.ledger_entry.ordinal();
  result.trace_root = predicate.trace_root;
  result.exact_evidence = predicate.exact_relation.id.ordinal();
  result.exact_trace_root = predicate.exact_relation.operation_trace_root;
  if (predicate.exact_relation.ordered_inputs.size() >
      result.exact_ordered_inputs.size())
    return boolean_outcome<relation_truth_record>::failure(
        primitive_relation_error(
            relation_subcode::exact_relation_invalid,
            "Component 07 exact ordered input evidence exceeds the public schema",
            relation_checkpoint::exact_relation_evaluation));
  result.exact_ordered_input_count = static_cast<std::uint16_t>(
      predicate.exact_relation.ordered_inputs.size());
  for (std::size_t i = 0; i < predicate.exact_relation.ordered_inputs.size(); ++i)
    result.exact_ordered_inputs[i] =
        predicate.exact_relation.ordered_inputs[i].ordinal();
  result.bounded_sign = predicate.bounded_sign;
  result.exact_relation = predicate.exact_relation.status;
  result.disposition = predicate.disposition;
  result.rounded_formula = static_cast<std::uint16_t>(rounded_formula);
  result.exact_formula = static_cast<std::uint16_t>(exact_formula);
  result.bounded_schema_version = identity.schema_version;
  result.bounded_provider_version = identity.provider_version;
  result.exact_schema_version = predicate.exact_relation.schema_version;
  result.exact_normalization_exponent =
      predicate.exact_relation.normalization_exponent;
  result.exact_capacity_used = predicate.exact_relation.capacity_used;
  result.exact_capacity_limit = predicate.exact_relation.capacity_used;
  result.bounded_publication = identity.publication;
  result.alternate_formulation_available =
      predicate.disposition == predicate_disposition::try_permitted_alternate;
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
      exact.schema_version != exact_formula_registry_version ||
      exact.reserved != 0 ||
      exact.ordered_input_count > exact.ordered_inputs.size() ||
      (exact_formula_requested &&
       (exact.evidence_id == 0 || exact.ordered_input_count == 0 ||
        exact.operation_trace_root == 0)) ||
      exact.capacity_used > std::numeric_limits<std::uint32_t>::max())
    return boolean_outcome<relation_truth_record>::failure(
        primitive_relation_error(
            relation_subcode::exact_relation_invalid,
            "Component 07 exact relation evidence is invalid or not representable",
            relation_checkpoint::exact_relation_evaluation));

  exact_relation_evidence evidence;
  evidence.formula_code = static_cast<std::uint16_t>(exact.formula);
  evidence.owner = rounded_and_bounded.identity.owner;
  evidence.id = exact_relation_id(exact.evidence_id);
  evidence.status = exact.status;
  evidence.normalization_exponent = exact.normalization_exponent;
  evidence.capacity_used = static_cast<std::uint32_t>(exact.capacity_used);
  evidence.operation_trace_root = exact.operation_trace_root;
  evidence.ordered_inputs.reserve(exact.ordered_input_count);
  for (std::size_t i = 0; i < exact.ordered_input_count; ++i)
    evidence.ordered_inputs.emplace_back(exact.ordered_inputs[i]);

  auto predicate = assemble_predicate_result(
      std::move(rounded_and_bounded), std::move(evidence),
      alternate_available);
  if (!predicate.has_value())
    return boolean_outcome<relation_truth_record>::failure(*predicate.error());
  auto result = make_relation_truth_record(
      *predicate.value(), rounded_formula, exact.formula);
  if (!result.has_value())
    return result;
  result.value()->exact_capacity_limit =
      static_cast<std::uint32_t>(exact.capacity_limit);
  result.value()->exact_trace_root = exact.operation_trace_root;
  if (!valid_relation_truth_record(*result.value()))
    return boolean_outcome<relation_truth_record>::failure(
        primitive_relation_error(
            relation_subcode::truth_layer_mismatch,
            "Component 07 complete primitive truth evidence is invalid"));
  return result;
}

} // namespace ygor::mesh_boolean::bounded
