#include "YgorMeshesBooleanQualificationCandidate.h"

#include <algorithm>
#include <array>
#include <map>
#include <new>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace ygor {
namespace mesh_boolean {
namespace {

constexpr std::array<char, 8> gate_tag{{'Y','G','B','Q','C','G','0','1'}};
constexpr std::array<char, 8> case_tag{{'Y','G','B','Q','C','C','0','1'}};
constexpr std::array<char, 8> plan_tag{{'Y','G','B','Q','C','P','0','1'}};
constexpr std::array<char, 8> observation_tag{{'Y','G','B','Q','C','O','0','1'}};
constexpr std::array<char, 8> regression_tag{{'Y','G','B','Q','C','R','0','1'}};
constexpr std::array<char, 8> issue_tag{{'Y','G','B','Q','C','I','0','1'}};
constexpr std::array<char, 8> campaign_tag{{'Y','G','B','Q','C','M','0','1'}};

product_error candidate_error(const char *key) {
  return make_product_error(product_error_code::qualification_policy_violation,
                            key);
}

bool zero(const digest &value) noexcept { return value == digest{}; }

bool text(const std::string &value, bool allow_empty = false) noexcept {
  return (allow_empty || !value.empty()) && value.size() <= 4096U &&
         std::find(value.begin(), value.end(), '\0') == value.end();
}

template <class E> bool before_count(E value, E count) noexcept {
  return static_cast<unsigned>(value) < static_cast<unsigned>(count);
}

bool known(qualification_candidate_gate_kind value) noexcept {
  return before_count(value, qualification_candidate_gate_kind::count);
}
bool known(qualification_candidate_issue_kind value) noexcept {
  return before_count(value, qualification_candidate_issue_kind::count);
}
bool known(qualification_candidate_issue_disposition value) noexcept {
  return before_count(value, qualification_candidate_issue_disposition::count);
}
bool known(qualification_outcome value) noexcept {
  return value >= qualification_outcome::verified_exact_success &&
         value <= qualification_outcome::infrastructure_failure;
}
bool known(product_error_code value) noexcept {
  return value >= product_error_code::input_contract_error &&
         value <= product_error_code::verifier_disagreement;
}
bool known(backend_id value) noexcept {
  return value == backend_id::experimental_exact_v1 ||
         value == backend_id::independent_axis_aligned_box_v1;
}
bool known(result_representation value) noexcept {
  return value >= result_representation::exact_stratified &&
         value <= result_representation::certified_approximate_mesh;
}
bool known(preparation_mode value) noexcept {
  return value >= preparation_mode::strict_validation &&
         value <= preparation_mode::normalized;
}
bool known(operation value) noexcept {
  return value >= operation::regularized_union &&
         value <= operation::symmetric_difference;
}
bool known(coordinate_tag value) noexcept {
  return value >= coordinate_tag::binary32 && value <= coordinate_tag::binary64;
}
bool known(index_tag value) noexcept {
  return value >= index_tag::uint32 && value <= index_tag::uint64;
}

void encode_digest(canonical_encoder &encoder, const digest &value) {
  encoder.raw(value.bytes.data(), value.bytes.size());
}

void encode_dimension(canonical_encoder &encoder,
                      const qualification_dimension_key &value) {
  encoder.u16(static_cast<std::uint16_t>(value.backend));
  encoder.byte(static_cast<std::uint8_t>(value.representation));
  encoder.byte(static_cast<std::uint8_t>(value.preparation));
  encoder.byte(static_cast<std::uint8_t>(value.selected_operation));
  encoder.byte(static_cast<std::uint8_t>(value.coordinate));
  encoder.byte(static_cast<std::uint8_t>(value.index));
  encoder.string(value.toolchain_identifier);
  encoder.string(value.geometry_category);
}

bool valid_dimension(const qualification_dimension_key &value) noexcept {
  return known(value.backend) && known(value.representation) &&
         known(value.preparation) && known(value.selected_operation) &&
         known(value.coordinate) && known(value.index) &&
         text(value.toolchain_identifier) && text(value.geometry_category);
}

void encode_gate(canonical_encoder &encoder,
                 const qualification_candidate_gate_binding &value,
                 bool include_digest) {
  encoder.u16(value.schema);
  encoder.byte(static_cast<std::uint8_t>(value.kind));
  encoder.string(value.identifier);
  encode_digest(encoder, value.plan_digest);
  encode_digest(encoder, value.report_digest);
  encode_digest(encoder, value.independent_validation_digest);
  encoder.boolean(value.complete);
  encoder.boolean(value.passed);
  if (include_digest)
    encode_digest(encoder, value.binding_digest);
}

digest gate_digest(const qualification_candidate_gate_binding &value) {
  canonical_encoder encoder;
  encode_gate(encoder, value, false);
  return domain_digest(gate_tag, encoder.bytes());
}

void encode_execution_case(canonical_encoder &encoder,
                           const qualification_candidate_execution_case &value,
                           bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.string(value.source_identifier);
  encode_digest(encoder, value.source_plan_digest);
  encode_digest(encoder, value.source_case_digest);
  encode_dimension(encoder, value.dimensions);
  encoder.u64(value.expected_outcomes.size());
  for (const auto outcome : value.expected_outcomes)
    encoder.byte(static_cast<std::uint8_t>(outcome));
  encoder.u64(value.expected_failure_codes.size());
  for (const auto code : value.expected_failure_codes)
    encoder.u16(static_cast<std::uint16_t>(code));
  encoder.boolean(value.require_regression_promotion_on_engine_defect);
  if (include_digest)
    encode_digest(encoder, value.case_digest);
}

digest execution_case_digest(
    const qualification_candidate_execution_case &value) {
  canonical_encoder encoder;
  encode_execution_case(encoder, value, false);
  return domain_digest(case_tag, encoder.bytes());
}

void encode_plan_semantic(canonical_encoder &encoder,
                          const qualification_candidate_campaign_plan &value) {
  encoder.u16(value.schema);
  encoder.u32(value.checker_version);
  encoder.string(value.identifier);
  encoder.u64(value.manifest_canonical_bytes.size());
  if (!value.manifest_canonical_bytes.empty())
    encoder.raw(value.manifest_canonical_bytes.data(),
                value.manifest_canonical_bytes.size());
  encode_digest(encoder, value.manifest_digest);
  encoder.u64(value.required_gates.size());
  for (const auto &entry : value.required_gates)
    encode_gate(encoder, entry, true);
  encoder.u64(value.execution_cases.size());
  for (const auto &entry : value.execution_cases)
    encode_execution_case(encoder, entry, true);
}

void encode_observation(
    canonical_encoder &encoder,
    const qualification_candidate_execution_observation &value,
    bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.case_identifier);
  encode_digest(encoder, value.case_digest);
  encoder.byte(static_cast<std::uint8_t>(value.outcome));
  encoder.boolean(value.completed);
  encoder.string(value.typed_outcome_key);
  encode_digest(encoder, value.accounting_digest);
  encode_digest(encoder, value.canonical_result_digest);
  encode_digest(encoder, value.canonical_failure_digest);
  encode_digest(encoder, value.replay_digest);
  encode_digest(encoder, value.run_log_digest);
  encoder.boolean(value.performance_regression_observed);
  encoder.boolean(value.resource_regression_observed);
  encode_digest(encoder, value.regression_evidence_digest);
  if (include_digest)
    encode_digest(encoder, value.observation_digest);
}

digest observation_digest(
    const qualification_candidate_execution_observation &value) {
  canonical_encoder encoder;
  encode_observation(encoder, value, false);
  return domain_digest(observation_tag, encoder.bytes());
}

void encode_regression(
    canonical_encoder &encoder,
    const qualification_candidate_regression_binding &value,
    bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.promotion_identifier);
  encoder.string(value.permanent_test_id);
  encode_digest(encoder, value.minimized_case_digest);
  encode_digest(encoder, value.canonical_case_bytes_digest);
  encode_digest(encoder, value.minimization_transcript_digest);
  encode_digest(encoder, value.promotion_artifact_digest);
  encode_digest(encoder, value.corpus_digest_before);
  encode_digest(encoder, value.corpus_digest_after);
  if (include_digest)
    encode_digest(encoder, value.binding_digest);
}

digest regression_digest(
    const qualification_candidate_regression_binding &value) {
  canonical_encoder encoder;
  encode_regression(encoder, value, false);
  return domain_digest(regression_tag, encoder.bytes());
}

void encode_issue(canonical_encoder &encoder,
                  const qualification_candidate_issue &value,
                  bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.string(value.case_identifier);
  encode_digest(encoder, value.case_digest);
  encoder.byte(static_cast<std::uint8_t>(value.kind));
  encode_digest(encoder, value.initial_observation_digest);
  encode_digest(encoder, value.detected_evidence_digest);
  encoder.byte(static_cast<std::uint8_t>(value.disposition));
  encoder.string(value.reviewer);
  encoder.string(value.rationale);
  encode_digest(encoder, value.resolution_evidence_digest);
  encode_digest(encoder, value.reviewed_gate_or_plan_digest);
  encoder.boolean(value.regression.has_value());
  if (value.regression)
    encode_regression(encoder, *value.regression, true);
  encoder.u64(value.affected_configuration_identifiers.size());
  for (const auto &entry : value.affected_configuration_identifiers)
    encoder.string(entry);
  encoder.u64(value.rerun_observation_digests.size());
  for (const auto &entry : value.rerun_observation_digests)
    encode_digest(encoder, entry);
  encoder.boolean(value.resolution_verified);
  if (include_digest)
    encode_digest(encoder, value.issue_digest);
}

digest issue_digest(const qualification_candidate_issue &value) {
  canonical_encoder encoder;
  encode_issue(encoder, value, false);
  return domain_digest(issue_tag, encoder.bytes());
}

void encode_campaign_semantic(canonical_encoder &encoder,
                              const qualification_candidate_campaign &value) {
  encoder.u16(value.schema);
  encoder.u32(value.checker_version);
  encoder.string(value.identifier);
  encode_digest(encoder, value.plan_digest);
  encoder.u64(value.observations.size());
  for (const auto &entry : value.observations)
    encode_observation(encoder, entry, true);
  encoder.u64(value.issues.size());
  for (const auto &entry : value.issues)
    encode_issue(encoder, entry, true);
  encoder.u64(value.passed_case_count);
  encoder.u64(value.resolved_issue_count);
  encoder.u64(value.promoted_regression_count);
  encoder.u64(value.blocking_issue_count);
  encoder.boolean(value.complete);
}

template <class V, class Key>
bool unique_sorted(const V &values, Key key) {
  for (std::size_t i = 1; i < values.size(); ++i)
    if (key(values[i - 1]) == key(values[i]))
      return false;
  return true;
}

bool is_failure_outcome(qualification_outcome value) noexcept {
  return value == qualification_outcome::expected_typed_failure ||
         value == qualification_outcome::unexpected_typed_failure ||
         value == qualification_outcome::timeout_or_resource_limit ||
         value == qualification_outcome::infrastructure_failure;
}

std::optional<qualification_candidate_issue_kind>
issue_kind_for_outcome(qualification_outcome value) noexcept {
  switch (value) {
  case qualification_outcome::unexpected_typed_failure:
    return qualification_candidate_issue_kind::unexpected_typed_failure;
  case qualification_outcome::backend_disagreement:
    return qualification_candidate_issue_kind::backend_disagreement;
  case qualification_outcome::verifier_disagreement:
    return qualification_candidate_issue_kind::verifier_disagreement;
  case qualification_outcome::false_success:
    return qualification_candidate_issue_kind::false_success;
  case qualification_outcome::nondeterministic_outcome:
    return qualification_candidate_issue_kind::nondeterministic_outcome;
  case qualification_outcome::timeout_or_resource_limit:
    return qualification_candidate_issue_kind::timeout_or_resource_limit;
  case qualification_outcome::infrastructure_failure:
    return qualification_candidate_issue_kind::infrastructure_failure;
  default:
    return std::nullopt;
  }
}

bool expected(const qualification_candidate_execution_case &descriptor,
              qualification_outcome outcome) noexcept {
  return std::find(descriptor.expected_outcomes.begin(),
                   descriptor.expected_outcomes.end(), outcome) !=
         descriptor.expected_outcomes.end();
}

bool contains_digest(const std::vector<digest> &values,
                     const digest &needle) noexcept {
  return std::find(values.begin(), values.end(), needle) != values.end();
}

product_status_or<qualification_candidate_gate_binding>
canonicalize_gate(qualification_candidate_gate_binding value) {
  if (value.schema != qualification_candidate_schema_version ||
      !known(value.kind) || !text(value.identifier) || zero(value.plan_digest) ||
      zero(value.report_digest) || zero(value.independent_validation_digest) ||
      !value.complete || !value.passed)
    return candidate_error("qualification_candidate.gate_malformed");
  const auto calculated = gate_digest(value);
  if (!zero(value.binding_digest) && value.binding_digest != calculated)
    return candidate_error("qualification_candidate.gate_digest");
  value.binding_digest = calculated;
  return value;
}

product_status_or<qualification_candidate_execution_case>
canonicalize_execution_case(qualification_candidate_execution_case value) {
  if (value.schema != qualification_candidate_schema_version ||
      !text(value.identifier) || !text(value.source_identifier) ||
      zero(value.source_plan_digest) || zero(value.source_case_digest) ||
      !valid_dimension(value.dimensions) || value.expected_outcomes.empty() ||
      value.expected_outcomes.size() > 16U ||
      value.expected_failure_codes.size() > 32U)
    return candidate_error("qualification_candidate.case_malformed");
  std::sort(value.expected_outcomes.begin(), value.expected_outcomes.end());
  value.expected_outcomes.erase(
      std::unique(value.expected_outcomes.begin(), value.expected_outcomes.end()),
      value.expected_outcomes.end());
  if (!std::all_of(value.expected_outcomes.begin(),
                   value.expected_outcomes.end(),
                   [](auto entry) { return known(entry); }))
    return candidate_error("qualification_candidate.case_outcome_unknown");
  std::sort(value.expected_failure_codes.begin(),
            value.expected_failure_codes.end());
  value.expected_failure_codes.erase(
      std::unique(value.expected_failure_codes.begin(),
                  value.expected_failure_codes.end()),
      value.expected_failure_codes.end());
  if (!std::all_of(value.expected_failure_codes.begin(),
                   value.expected_failure_codes.end(),
                   [](auto entry) { return known(entry); }))
    return candidate_error("qualification_candidate.case_error_unknown");
  const bool failure_expected = std::any_of(
      value.expected_outcomes.begin(), value.expected_outcomes.end(),
      [](auto entry) { return is_failure_outcome(entry); });
  if (failure_expected != !value.expected_failure_codes.empty())
    return candidate_error("qualification_candidate.case_failure_contract");
  const auto calculated = execution_case_digest(value);
  if (!zero(value.case_digest) && value.case_digest != calculated)
    return candidate_error("qualification_candidate.case_digest");
  value.case_digest = calculated;
  return value;
}

product_status_or<qualification_candidate_execution_observation>
canonicalize_observation(qualification_candidate_execution_observation value) {
  if (value.schema != qualification_candidate_schema_version ||
      !text(value.case_identifier) || zero(value.case_digest) ||
      !known(value.outcome) || !value.completed ||
      !text(value.typed_outcome_key) || zero(value.accounting_digest) ||
      zero(value.replay_digest) || zero(value.run_log_digest) ||
      (zero(value.canonical_result_digest) &&
       zero(value.canonical_failure_digest)))
    return candidate_error("qualification_candidate.observation_malformed");
  const bool regression = value.performance_regression_observed ||
                          value.resource_regression_observed;
  if (regression != !zero(value.regression_evidence_digest))
    return candidate_error("qualification_candidate.regression_evidence");
  const auto calculated = observation_digest(value);
  if (!zero(value.observation_digest) && value.observation_digest != calculated)
    return candidate_error("qualification_candidate.observation_digest");
  value.observation_digest = calculated;
  return value;
}

product_status_or<qualification_candidate_regression_binding>
canonicalize_regression(qualification_candidate_regression_binding value) {
  if (value.schema != qualification_candidate_schema_version ||
      !text(value.promotion_identifier) || !text(value.permanent_test_id) ||
      zero(value.minimized_case_digest) || zero(value.canonical_case_bytes_digest) ||
      zero(value.minimization_transcript_digest) ||
      zero(value.promotion_artifact_digest) || zero(value.corpus_digest_before) ||
      zero(value.corpus_digest_after) ||
      value.corpus_digest_before == value.corpus_digest_after)
    return candidate_error("qualification_candidate.regression_malformed");
  const auto calculated = regression_digest(value);
  if (!zero(value.binding_digest) && value.binding_digest != calculated)
    return candidate_error("qualification_candidate.regression_digest");
  value.binding_digest = calculated;
  return value;
}

bool engine_defect_kind(qualification_candidate_issue_kind kind) noexcept {
  return kind != qualification_candidate_issue_kind::infrastructure_failure;
}

product_status_or<qualification_candidate_issue>
canonicalize_issue(qualification_candidate_issue value) {
  if (value.schema != qualification_candidate_schema_version ||
      !text(value.identifier) || !text(value.case_identifier) ||
      zero(value.case_digest) || !known(value.kind) ||
      zero(value.initial_observation_digest) ||
      zero(value.detected_evidence_digest) || !known(value.disposition))
    return candidate_error("qualification_candidate.issue_malformed");

  if (value.disposition ==
      qualification_candidate_issue_disposition::unresolved_blocking) {
    if (!value.reviewer.empty() || !value.rationale.empty() ||
        !zero(value.resolution_evidence_digest) ||
        !zero(value.reviewed_gate_or_plan_digest) || value.regression ||
        !value.affected_configuration_identifiers.empty() ||
        !value.rerun_observation_digests.empty() || value.resolution_verified)
      return candidate_error("qualification_candidate.unresolved_has_resolution");
  } else {
    if (!text(value.reviewer) || !text(value.rationale) ||
        zero(value.resolution_evidence_digest) || !value.resolution_verified ||
        value.affected_configuration_identifiers.empty() ||
        value.affected_configuration_identifiers.size() !=
            value.rerun_observation_digests.size())
      return candidate_error("qualification_candidate.resolution_malformed");
    std::vector<std::pair<std::string, digest>> reruns;
    reruns.reserve(value.affected_configuration_identifiers.size());
    for (std::size_t i = 0;
         i != value.affected_configuration_identifiers.size(); ++i)
      reruns.emplace_back(value.affected_configuration_identifiers[i],
                          value.rerun_observation_digests[i]);
    std::sort(reruns.begin(), reruns.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    if (!unique_sorted(reruns, [](const auto &entry) { return entry.first; }) ||
        !std::all_of(reruns.begin(), reruns.end(), [](const auto &entry) {
          return text(entry.first) && !zero(entry.second);
        }))
      return candidate_error("qualification_candidate.rerun_malformed");
    value.affected_configuration_identifiers.clear();
    value.rerun_observation_digests.clear();
    for (auto &entry : reruns) {
      value.affected_configuration_identifiers.push_back(std::move(entry.first));
      value.rerun_observation_digests.push_back(entry.second);
    }

    if (value.disposition ==
        qualification_candidate_issue_disposition::resolved_engine_defect) {
      if (!engine_defect_kind(value.kind) || !value.regression)
        return candidate_error("qualification_candidate.defect_without_regression");
      auto regression = canonicalize_regression(std::move(*value.regression));
      if (!regression.has_value())
        return regression.error();
      value.regression = std::move(regression.value());
      if (!zero(value.reviewed_gate_or_plan_digest))
        return candidate_error("qualification_candidate.defect_unexpected_review");
    } else {
      if (value.regression)
        return candidate_error("qualification_candidate.nondefect_regression");
      if (value.disposition ==
          qualification_candidate_issue_disposition::resolved_infrastructure) {
        if (value.kind !=
                qualification_candidate_issue_kind::infrastructure_failure &&
            value.kind != qualification_candidate_issue_kind::timeout_or_resource_limit)
          return candidate_error("qualification_candidate.infrastructure_kind");
        if (!zero(value.reviewed_gate_or_plan_digest))
          return candidate_error("qualification_candidate.infrastructure_review");
      } else if (value.disposition ==
                 qualification_candidate_issue_disposition::resolved_policy_difference) {
        if (value.kind != qualification_candidate_issue_kind::backend_disagreement ||
            zero(value.reviewed_gate_or_plan_digest))
          return candidate_error("qualification_candidate.policy_resolution");
      } else if (value.disposition ==
                 qualification_candidate_issue_disposition::resolved_performance_review) {
        if ((value.kind != qualification_candidate_issue_kind::performance_regression &&
             value.kind != qualification_candidate_issue_kind::resource_regression) ||
            zero(value.reviewed_gate_or_plan_digest))
          return candidate_error("qualification_candidate.performance_resolution");
      }
    }
  }
  const auto calculated = issue_digest(value);
  if (!zero(value.issue_digest) && value.issue_digest != calculated)
    return candidate_error("qualification_candidate.issue_digest");
  value.issue_digest = calculated;
  return value;
}

} // namespace

const char *qualification_candidate_gate_kind_token(
    qualification_candidate_gate_kind value) noexcept {
  switch (value) {
  case qualification_candidate_gate_kind::permanent_corpus: return "permanent_corpus";
  case qualification_candidate_gate_kind::generation_and_chains: return "generation_and_chains";
  case qualification_candidate_gate_kind::backend_comparison: return "backend_comparison";
  case qualification_candidate_gate_kind::false_success_accounting: return "false_success_accounting";
  case qualification_candidate_gate_kind::cad_ingestion: return "cad_ingestion";
  case qualification_candidate_gate_kind::profile_suites: return "profile_suites";
  case qualification_candidate_gate_kind::platform_matrix: return "platform_matrix";
  case qualification_candidate_gate_kind::performance: return "performance";
  default: return "unknown";
  }
}

const char *qualification_candidate_issue_kind_token(
    qualification_candidate_issue_kind value) noexcept {
  switch (value) {
  case qualification_candidate_issue_kind::unexpected_typed_failure: return "unexpected_typed_failure";
  case qualification_candidate_issue_kind::backend_disagreement: return "backend_disagreement";
  case qualification_candidate_issue_kind::verifier_disagreement: return "verifier_disagreement";
  case qualification_candidate_issue_kind::false_success: return "false_success";
  case qualification_candidate_issue_kind::nondeterministic_outcome: return "nondeterministic_outcome";
  case qualification_candidate_issue_kind::timeout_or_resource_limit: return "timeout_or_resource_limit";
  case qualification_candidate_issue_kind::infrastructure_failure: return "infrastructure_failure";
  case qualification_candidate_issue_kind::performance_regression: return "performance_regression";
  case qualification_candidate_issue_kind::resource_regression: return "resource_regression";
  default: return "unknown";
  }
}

const char *qualification_candidate_issue_disposition_token(
    qualification_candidate_issue_disposition value) noexcept {
  switch (value) {
  case qualification_candidate_issue_disposition::unresolved_blocking: return "unresolved_blocking";
  case qualification_candidate_issue_disposition::resolved_engine_defect: return "resolved_engine_defect";
  case qualification_candidate_issue_disposition::resolved_infrastructure: return "resolved_infrastructure";
  case qualification_candidate_issue_disposition::resolved_policy_difference: return "resolved_policy_difference";
  case qualification_candidate_issue_disposition::resolved_performance_review: return "resolved_performance_review";
  default: return "unknown";
  }
}

product_status_or<qualification_candidate_gate_binding>
make_qualification_candidate_gate_binding(qualification_candidate_gate_binding value) {
  try { return canonicalize_gate(std::move(value)); }
  catch (const std::bad_alloc &) { return candidate_error("qualification_candidate.allocation_failure"); }
}

product_status_or<qualification_candidate_execution_case>
make_qualification_candidate_execution_case(qualification_candidate_execution_case value) {
  try { return canonicalize_execution_case(std::move(value)); }
  catch (const std::bad_alloc &) { return candidate_error("qualification_candidate.allocation_failure"); }
}

product_status_or<qualification_candidate_campaign_plan>
make_qualification_candidate_campaign_plan(qualification_candidate_campaign_plan value) {
  try {
    if (value.schema != qualification_candidate_schema_version ||
        value.checker_version != qualification_candidate_checker_version ||
        !text(value.identifier) || value.manifest_canonical_bytes.empty() ||
        zero(value.manifest_digest) || value.required_gates.size() !=
            static_cast<std::size_t>(qualification_candidate_gate_kind::count) ||
        value.execution_cases.empty())
      return candidate_error("qualification_candidate.plan_malformed");

    auto manifest = decode_qualification_campaign_manifest(value.manifest_canonical_bytes);
    if (!manifest.has_value() || manifest.value().manifest_digest != value.manifest_digest ||
        manifest.value().repository_dirty)
      return candidate_error("qualification_candidate.manifest_binding");

    for (auto &entry : value.required_gates) {
      auto made = canonicalize_gate(std::move(entry));
      if (!made.has_value()) return made.error();
      entry = std::move(made.value());
    }
    std::sort(value.required_gates.begin(), value.required_gates.end(),
              [](const auto &a, const auto &b) { return a.kind < b.kind; });
    if (!unique_sorted(value.required_gates,
                       [](const auto &entry) { return entry.kind; }))
      return candidate_error("qualification_candidate.gate_duplicate");
    for (std::size_t i = 0; i != value.required_gates.size(); ++i)
      if (static_cast<std::size_t>(value.required_gates[i].kind) != i)
        return candidate_error("qualification_candidate.gate_coverage");

    for (auto &entry : value.execution_cases) {
      auto made = canonicalize_execution_case(std::move(entry));
      if (!made.has_value()) return made.error();
      entry = std::move(made.value());
    }
    std::sort(value.execution_cases.begin(), value.execution_cases.end(),
              [](const auto &a, const auto &b) { return a.identifier < b.identifier; });
    if (!unique_sorted(value.execution_cases,
                       [](const auto &entry) { return entry.identifier; }))
      return candidate_error("qualification_candidate.case_duplicate");

    canonical_encoder encoder;
    encode_plan_semantic(encoder, value);
    const auto bytes = encoder.bytes();
    const auto calculated = domain_digest(plan_tag, bytes);
    if ((!value.canonical_bytes.empty() && value.canonical_bytes != bytes) ||
        (!zero(value.plan_digest) && value.plan_digest != calculated))
      return candidate_error("qualification_candidate.plan_digest");
    value.canonical_bytes = bytes;
    value.plan_digest = calculated;
    return value;
  } catch (const std::bad_alloc &) {
    return candidate_error("qualification_candidate.allocation_failure");
  }
}

product_status_or<bool> validate_qualification_candidate_campaign_plan(
    const qualification_candidate_campaign_plan &value) noexcept {
  try {
    auto made = make_qualification_candidate_campaign_plan(value);
    return made.has_value() && made.value().canonical_bytes == value.canonical_bytes &&
           made.value().plan_digest == value.plan_digest;
  } catch (const std::bad_alloc &) {
    return candidate_error("qualification_candidate.allocation_failure");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_candidate_campaign_plan(
    const qualification_candidate_campaign_plan &value) {
  auto valid = validate_qualification_candidate_campaign_plan(value);
  if (!valid.has_value() || !valid.value())
    return candidate_error("qualification_candidate.plan_not_canonical");
  return value.canonical_bytes;
}

product_status_or<qualification_candidate_execution_observation>
make_qualification_candidate_execution_observation(
    qualification_candidate_execution_observation value) {
  try { return canonicalize_observation(std::move(value)); }
  catch (const std::bad_alloc &) { return candidate_error("qualification_candidate.allocation_failure"); }
}

product_status_or<qualification_candidate_regression_binding>
make_qualification_candidate_regression_binding(
    qualification_candidate_regression_binding value) {
  try { return canonicalize_regression(std::move(value)); }
  catch (const std::bad_alloc &) { return candidate_error("qualification_candidate.allocation_failure"); }
}

product_status_or<qualification_candidate_issue>
make_qualification_candidate_issue(qualification_candidate_issue value) {
  try { return canonicalize_issue(std::move(value)); }
  catch (const std::bad_alloc &) { return candidate_error("qualification_candidate.allocation_failure"); }
}

product_status_or<qualification_candidate_campaign>
make_qualification_candidate_campaign(
    const qualification_candidate_campaign_plan &plan,
    std::vector<qualification_candidate_execution_observation> observations,
    std::vector<qualification_candidate_issue> issues, bool requested_complete) {
  try {
    auto valid_plan = validate_qualification_candidate_campaign_plan(plan);
    if (!valid_plan.has_value() || !valid_plan.value())
      return candidate_error("qualification_candidate.plan_invalid");
    for (auto &entry : observations) {
      auto made = canonicalize_observation(std::move(entry));
      if (!made.has_value()) return made.error();
      entry = std::move(made.value());
    }
    std::sort(observations.begin(), observations.end(),
              [](const auto &a, const auto &b) { return a.case_identifier < b.case_identifier; });
    if (!unique_sorted(observations,
                       [](const auto &entry) { return entry.case_identifier; }))
      return candidate_error("qualification_candidate.observation_duplicate");
    for (auto &entry : issues) {
      auto made = canonicalize_issue(std::move(entry));
      if (!made.has_value()) return made.error();
      entry = std::move(made.value());
    }
    std::sort(issues.begin(), issues.end(),
              [](const auto &a, const auto &b) { return a.identifier < b.identifier; });
    if (!unique_sorted(issues, [](const auto &entry) { return entry.identifier; }))
      return candidate_error("qualification_candidate.issue_duplicate");

    qualification_candidate_campaign result;
    result.identifier = plan.identifier + ".campaign";
    result.plan_digest = plan.plan_digest;
    result.observations = std::move(observations);
    result.issues = std::move(issues);

    std::map<std::string, const qualification_candidate_execution_observation *> observation_by_case;
    for (const auto &entry : result.observations) {
      const auto descriptor = std::lower_bound(
          plan.execution_cases.begin(), plan.execution_cases.end(),
          entry.case_identifier, [](const auto &candidate, const std::string &id) {
            return candidate.identifier < id;
          });
      if (descriptor == plan.execution_cases.end() ||
          descriptor->identifier != entry.case_identifier ||
          descriptor->case_digest != entry.case_digest)
        return candidate_error("qualification_candidate.observation_foreign");
      observation_by_case.emplace(entry.case_identifier, &entry);
    }

    std::multimap<std::string, const qualification_candidate_issue *> issues_by_case;
    for (const auto &entry : result.issues) {
      const auto descriptor = std::lower_bound(
          plan.execution_cases.begin(), plan.execution_cases.end(),
          entry.case_identifier, [](const auto &candidate, const std::string &id) {
            return candidate.identifier < id;
          });
      if (descriptor == plan.execution_cases.end() ||
          descriptor->identifier != entry.case_identifier ||
          descriptor->case_digest != entry.case_digest)
        return candidate_error("qualification_candidate.issue_foreign");
      issues_by_case.emplace(entry.case_identifier, &entry);
      if (entry.disposition ==
          qualification_candidate_issue_disposition::unresolved_blocking) {
        ++result.blocking_issue_count;
      } else {
        const auto observed = observation_by_case.find(entry.case_identifier);
        if (observed == observation_by_case.end() ||
            !expected(*descriptor, observed->second->outcome) ||
            observed->second->performance_regression_observed ||
            observed->second->resource_regression_observed ||
            !contains_digest(entry.rerun_observation_digests,
                             observed->second->observation_digest))
          return candidate_error("qualification_candidate.resolution_not_rerun");
        ++result.resolved_issue_count;
        if (entry.regression)
          ++result.promoted_regression_count;
      }
    }

    for (const auto &descriptor : plan.execution_cases) {
      const auto observed = observation_by_case.find(descriptor.identifier);
      if (observed == observation_by_case.end()) {
        ++result.blocking_issue_count;
        continue;
      }
      const bool outcome_ok = expected(descriptor, observed->second->outcome);
      const bool regression = observed->second->performance_regression_observed ||
                              observed->second->resource_regression_observed;
      if (outcome_ok && !regression) {
        ++result.passed_case_count;
        continue;
      }
      bool matching_unresolved = false;
      const auto range = issues_by_case.equal_range(descriptor.identifier);
      for (auto it = range.first; it != range.second; ++it) {
        const auto &issue = *it->second;
        if (issue.disposition !=
            qualification_candidate_issue_disposition::unresolved_blocking)
          continue;
        const auto outcome_kind = issue_kind_for_outcome(observed->second->outcome);
        if ((!outcome_ok && outcome_kind && issue.kind == *outcome_kind) ||
            (observed->second->performance_regression_observed &&
             issue.kind == qualification_candidate_issue_kind::performance_regression) ||
            (observed->second->resource_regression_observed &&
             issue.kind == qualification_candidate_issue_kind::resource_regression))
          matching_unresolved = true;
      }
      if (!matching_unresolved)
        ++result.blocking_issue_count;
    }

    result.complete = requested_complete && result.blocking_issue_count == 0U &&
                      result.passed_case_count == plan.execution_cases.size();
    canonical_encoder encoder;
    encode_campaign_semantic(encoder, result);
    result.canonical_bytes = encoder.bytes();
    result.campaign_digest = domain_digest(campaign_tag, result.canonical_bytes);
    return result;
  } catch (const std::bad_alloc &) {
    return candidate_error("qualification_candidate.allocation_failure");
  }
}

product_status_or<bool> validate_qualification_candidate_campaign(
    const qualification_candidate_campaign &value,
    const qualification_candidate_campaign_plan &plan) noexcept {
  try {
    if (value.schema != qualification_candidate_schema_version ||
        value.checker_version != qualification_candidate_checker_version ||
        value.identifier != plan.identifier + ".campaign" ||
        value.plan_digest != plan.plan_digest)
      return candidate_error("qualification_candidate.campaign_malformed");
    auto made = make_qualification_candidate_campaign(
        plan, value.observations, value.issues, value.complete);
    if (!made.has_value()) return made.error();
    const auto &rebuilt = made.value();
    return rebuilt.passed_case_count == value.passed_case_count &&
           rebuilt.resolved_issue_count == value.resolved_issue_count &&
           rebuilt.promoted_regression_count == value.promoted_regression_count &&
           rebuilt.blocking_issue_count == value.blocking_issue_count &&
           rebuilt.complete == value.complete &&
           rebuilt.canonical_bytes == value.canonical_bytes &&
           rebuilt.campaign_digest == value.campaign_digest;
  } catch (const std::bad_alloc &) {
    return candidate_error("qualification_candidate.allocation_failure");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_candidate_campaign(const qualification_candidate_campaign &value) {
  if (value.canonical_bytes.empty() || zero(value.campaign_digest) ||
      domain_digest(campaign_tag, value.canonical_bytes) != value.campaign_digest)
    return candidate_error("qualification_candidate.campaign_not_canonical");
  return value.canonical_bytes;
}

bool qualification_candidate_campaign_gate_passes(
    const qualification_candidate_campaign &campaign,
    const qualification_candidate_campaign_plan &plan) noexcept {
  auto valid = validate_qualification_candidate_campaign(campaign, plan);
  return valid.has_value() && valid.value() && campaign.complete &&
         campaign.blocking_issue_count == 0U &&
         campaign.passed_case_count == plan.execution_cases.size();
}

} // namespace mesh_boolean
} // namespace ygor
