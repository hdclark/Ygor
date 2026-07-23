#include "YgorMeshesBooleanBounded/PrimitiveRelationKernel.h"

#include <cstdint>
#include <iostream>

using namespace ygor::mesh_boolean::bounded;

namespace {
int failures = 0;
void check(bool condition, const char *message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

void test_exact_tie_layers() {
  const auto owner = context_owner_token::create();
  auto scalar = checked_bounded_singleton(owner, 0.0);
  check(scalar.has_value(), "zero bounded singleton should build");
  if (!scalar.has_value())
    return;

  exact_relation_evidence exact;
  exact.owner = owner;
  exact.formula_code =
      static_cast<std::uint16_t>(exact_relation_formula_code::orient_2d);
  exact.status = exact_relation_status::exact_zero;
  auto predicate = assemble_predicate_result(*scalar.value(), exact);
  check(predicate.has_value(), "exact tie predicate should assemble");
  if (!predicate.has_value())
    return;

  auto truth = make_relation_truth_record(
      *predicate.value(), rounded_operation_code::determinant_2x2,
      exact_relation_formula_code::orient_2d);
  check(truth.has_value(), "exact tie relation truth should assemble");
  if (!truth.has_value())
    return;
  check(truth.value()->rounded_nominal_bits == to_bits(0.0),
        "rounded nominal bits are preserved");
  check(truth.value()->bounded_sign ==
            bounded_sign_status::overlaps_boundary,
        "bounded sign is preserved independently");
  check(truth.value()->exact_relation == exact_relation_status::exact_zero,
        "exact relation is preserved independently");
  check(truth.value()->disposition ==
            predicate_disposition::retain_tie_for_consumer_eligibility,
        "consumer disposition is preserved independently");
  check(valid_relation_truth_record(*truth.value()),
        "assembled relation truth independently validates");
}

void test_formula_mismatch_rejected() {
  const auto owner = context_owner_token::create();
  auto scalar = checked_bounded_singleton(owner, 1.0);
  check(scalar.has_value(), "positive bounded singleton should build");
  if (!scalar.has_value())
    return;

  exact_relation_evidence exact;
  exact.owner = owner;
  exact.formula_code = static_cast<std::uint16_t>(
      exact_relation_formula_code::finite_scalar_comparison);
  exact.status = exact_relation_status::exact_positive;
  auto predicate = assemble_predicate_result(*scalar.value(), exact);
  check(predicate.has_value(), "positive predicate should assemble");
  if (!predicate.has_value())
    return;

  auto truth = make_relation_truth_record(
      *predicate.value(), rounded_operation_code::source_import,
      exact_relation_formula_code::orient_2d);
  check(!truth.has_value(), "mismatched exact formula must fail closed");
  check(truth.error() &&
            truth.error()->subcode == static_cast<std::uint32_t>(
                                         relation_subcode::exact_relation_invalid),
        "formula mismatch has stable Component 07 failure");
}

void test_predicate_layer_mutation_rejected() {
  const auto owner = context_owner_token::create();
  auto scalar = checked_bounded_singleton(owner, 1.0);
  check(scalar.has_value(), "positive bounded singleton should build");
  if (!scalar.has_value())
    return;

  exact_relation_evidence exact;
  exact.owner = owner;
  exact.formula_code = static_cast<std::uint16_t>(
      exact_relation_formula_code::finite_scalar_comparison);
  exact.status = exact_relation_status::exact_positive;
  auto predicate = assemble_predicate_result(*scalar.value(), exact);
  check(predicate.has_value(), "positive predicate should assemble");
  if (!predicate.has_value())
    return;

  predicate.value()->uncertainty_width = 1.0;
  check(!valid_predicate_result(*predicate.value()),
        "mutated Component 03 truth evidence must fail validation");
  auto truth = make_relation_truth_record(
      *predicate.value(), rounded_operation_code::source_import,
      exact_relation_formula_code::finite_scalar_comparison);
  check(!truth.has_value(),
        "Component 07 must reject mutated Component 03 truth evidence");
}

void test_unknown_exact_status_rejected() {
  const auto owner = context_owner_token::create();
  auto scalar = checked_bounded_singleton(owner, 1.0);
  check(scalar.has_value(), "positive bounded singleton should build");
  if (!scalar.has_value())
    return;

  exact_relation_evidence exact;
  exact.owner = owner;
  exact.formula_code = static_cast<std::uint16_t>(
      exact_relation_formula_code::finite_scalar_comparison);
  exact.status = static_cast<exact_relation_status>(255);
  auto predicate = assemble_predicate_result(*scalar.value(), exact);
  check(predicate.has_value(),
        "legacy assembler leaves unknown exact status for validation");
  if (!predicate.has_value())
    return;
  check(!valid_predicate_result(*predicate.value()),
        "unknown exact-relation status must fail validation");

  relation_truth_record record;
  record.rounded_nominal_bits = to_bits(1.0);
  record.bounded_sign = bounded_sign_status::definitely_positive;
  record.exact_relation = static_cast<exact_relation_status>(255);
  record.disposition = predicate_disposition::accept_numeric_sign;
  record.rounded_formula =
      static_cast<std::uint16_t>(rounded_operation_code::source_import);
  record.exact_formula = static_cast<std::uint16_t>(
      exact_relation_formula_code::finite_scalar_comparison);
  check(!valid_relation_truth_record(record),
        "unknown exact status in stored relation truth must fail validation");

  record.exact_relation = exact_relation_status::unavailable;
  check(!valid_relation_truth_record(record),
        "requested exact formula cannot store unavailable exact evidence");
  record.exact_formula =
      static_cast<std::uint16_t>(exact_relation_formula_code::invalid);
  record.exact_relation = exact_relation_status::exact_positive;
  check(!valid_relation_truth_record(record),
        "exact evidence cannot omit its registered formula");
}

void test_failed_exact_evaluation_rejected() {
  const auto owner = context_owner_token::create();
  auto scalar = checked_bounded_singleton(owner, 1.0);
  check(scalar.has_value(), "positive bounded singleton should build");
  if (!scalar.has_value())
    return;

  exact_relation_record exact;
  exact.formula = exact_relation_formula_code::orient_2d;
  exact.status = exact_relation_status::unavailable;
  exact.evaluation_status = numeric_status::expansion_capacity_exceeded;
  exact.capacity_used = 8;
  exact.capacity_limit = 8;
  auto truth = assemble_relation_truth_record(
      *scalar.value(), exact, rounded_operation_code::determinant_2x2);
  check(!truth.has_value(),
        "failed exact-relation evaluation must not become ordinary uncertainty");
  check(truth.error() &&
            truth.error()->subcode == static_cast<std::uint32_t>(
                                         relation_subcode::exact_relation_invalid),
        "failed exact evaluation has stable Component 07 failure");
}

void test_unavailable_exact_is_not_tie() {
  const auto owner = context_owner_token::create();
  auto interval = finite_interval<double>::create(-1.0, 1.0);
  check(interval.has_value(), "uncertain interval should build");
  if (!interval)
    return;
  bounded_scalar<double> scalar;
  scalar.rounded_nominal = 0.0;
  scalar.uncertainty_enclosure = *interval;
  scalar.identity.owner = owner;

  exact_relation_record exact;
  exact.formula = exact_relation_formula_code::invalid;
  exact.status = exact_relation_status::unavailable;
  exact.evaluation_status = numeric_status::success;
  auto truth = assemble_relation_truth_record(
      scalar, exact, rounded_operation_code::source_import);
  check(truth.has_value(), "unavailable exact relation remains representable");
  if (!truth.has_value())
    return;
  check(truth.value()->exact_relation == exact_relation_status::unavailable,
        "unavailable exact relation is not converted to zero");
  check(truth.value()->disposition ==
            predicate_disposition::fail_condition_or_tolerance,
        "rounded zero without exact tie remains fail-closed uncertainty");
}
} // namespace

int main() {
  test_exact_tie_layers();
  test_formula_mismatch_rejected();
  test_predicate_layer_mutation_rejected();
  test_unknown_exact_status_rejected();
  test_failed_exact_evaluation_rejected();
  test_unavailable_exact_is_not_tie();
  if (failures != 0)
    std::cerr << failures << " Component 07 truth-layer checks failed\n";
  return failures == 0 ? 0 : 1;
}
