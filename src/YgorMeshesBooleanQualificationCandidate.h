#pragma once
#ifndef YGOR_MESHES_BOOLEAN_QUALIFICATION_CANDIDATE_H_
#define YGOR_MESHES_BOOLEAN_QUALIFICATION_CANDIDATE_H_

#include "YgorMeshesBooleanQualification.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t qualification_candidate_schema_version = 1;
constexpr std::uint32_t qualification_candidate_checker_version = 1;

enum class qualification_candidate_gate_kind : std::uint8_t {
  permanent_corpus,
  generation_and_chains,
  backend_comparison,
  false_success_accounting,
  cad_ingestion,
  profile_suites,
  platform_matrix,
  performance,
  count
};

enum class qualification_candidate_issue_kind : std::uint8_t {
  unexpected_typed_failure,
  backend_disagreement,
  verifier_disagreement,
  false_success,
  nondeterministic_outcome,
  timeout_or_resource_limit,
  infrastructure_failure,
  performance_regression,
  resource_regression,
  count
};

enum class qualification_candidate_issue_disposition : std::uint8_t {
  unresolved_blocking,
  resolved_engine_defect,
  resolved_infrastructure,
  resolved_policy_difference,
  resolved_performance_review,
  count
};

struct qualification_candidate_gate_binding {
  std::uint16_t schema = qualification_candidate_schema_version;
  qualification_candidate_gate_kind kind =
      qualification_candidate_gate_kind::permanent_corpus;
  std::string identifier;
  digest plan_digest;
  digest report_digest;
  digest independent_validation_digest;
  bool complete = false;
  bool passed = false;
  digest binding_digest;
};

struct qualification_candidate_execution_case {
  std::uint16_t schema = qualification_candidate_schema_version;
  std::string identifier;
  std::string source_identifier;
  digest source_plan_digest;
  digest source_case_digest;
  qualification_dimension_key dimensions;
  std::vector<qualification_outcome> expected_outcomes;
  std::vector<product_error_code> expected_failure_codes;
  bool require_regression_promotion_on_engine_defect = true;
  digest case_digest;
};

struct qualification_candidate_campaign_plan {
  std::uint16_t schema = qualification_candidate_schema_version;
  std::uint32_t checker_version = qualification_candidate_checker_version;
  std::string identifier;
  std::vector<std::uint8_t> manifest_canonical_bytes;
  digest manifest_digest;
  std::vector<qualification_candidate_gate_binding> required_gates;
  std::vector<qualification_candidate_execution_case> execution_cases;
  std::vector<std::uint8_t> canonical_bytes;
  digest plan_digest;
};

struct qualification_candidate_execution_observation {
  std::uint16_t schema = qualification_candidate_schema_version;
  std::string case_identifier;
  digest case_digest;
  qualification_outcome outcome = qualification_outcome::infrastructure_failure;
  std::optional<product_error_code> error_code;
  bool completed = false;
  std::string typed_outcome_key;
  digest accounting_digest;
  digest canonical_result_digest;
  digest canonical_failure_digest;
  digest replay_digest;
  digest run_log_digest;
  bool performance_regression_observed = false;
  bool resource_regression_observed = false;
  digest regression_evidence_digest;
  digest observation_digest;
};

struct qualification_candidate_regression_binding {
  std::uint16_t schema = qualification_candidate_schema_version;
  std::string promotion_identifier;
  std::string permanent_test_id;
  digest minimized_case_digest;
  digest canonical_case_bytes_digest;
  digest minimization_transcript_digest;
  digest promotion_artifact_digest;
  digest corpus_digest_before;
  digest corpus_digest_after;
  digest binding_digest;
};

struct qualification_candidate_issue {
  std::uint16_t schema = qualification_candidate_schema_version;
  std::string identifier;
  std::string case_identifier;
  digest case_digest;
  qualification_candidate_issue_kind kind =
      qualification_candidate_issue_kind::unexpected_typed_failure;
  digest initial_observation_digest;
  digest detected_evidence_digest;
  qualification_candidate_issue_disposition disposition =
      qualification_candidate_issue_disposition::unresolved_blocking;
  std::string reviewer;
  std::string rationale;
  digest resolution_evidence_digest;
  digest reviewed_gate_or_plan_digest;
  std::optional<qualification_candidate_regression_binding> regression;
  std::vector<std::string> affected_configuration_identifiers;
  std::vector<digest> rerun_observation_digests;
  bool resolution_verified = false;
  digest issue_digest;
};

struct qualification_candidate_campaign {
  std::uint16_t schema = qualification_candidate_schema_version;
  std::uint32_t checker_version = qualification_candidate_checker_version;
  std::string identifier;
  digest plan_digest;
  std::vector<qualification_candidate_execution_observation> observations;
  std::vector<qualification_candidate_issue> issues;
  std::uint64_t passed_case_count = 0;
  std::uint64_t resolved_issue_count = 0;
  std::uint64_t promoted_regression_count = 0;
  std::uint64_t blocking_issue_count = 0;
  bool complete = false;
  std::vector<std::uint8_t> canonical_bytes;
  digest campaign_digest;
};

const char *qualification_candidate_gate_kind_token(
    qualification_candidate_gate_kind) noexcept;
const char *qualification_candidate_issue_kind_token(
    qualification_candidate_issue_kind) noexcept;
const char *qualification_candidate_issue_disposition_token(
    qualification_candidate_issue_disposition) noexcept;

product_status_or<qualification_candidate_gate_binding>
make_qualification_candidate_gate_binding(qualification_candidate_gate_binding);
product_status_or<qualification_candidate_execution_case>
make_qualification_candidate_execution_case(qualification_candidate_execution_case);
product_status_or<qualification_candidate_campaign_plan>
make_qualification_candidate_campaign_plan(qualification_candidate_campaign_plan);
product_status_or<bool> validate_qualification_candidate_campaign_plan(
    const qualification_candidate_campaign_plan &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_candidate_campaign_plan(
    const qualification_candidate_campaign_plan &);

product_status_or<qualification_candidate_execution_observation>
make_qualification_candidate_execution_observation(
    qualification_candidate_execution_observation);
product_status_or<qualification_candidate_regression_binding>
make_qualification_candidate_regression_binding(
    qualification_candidate_regression_binding);
product_status_or<qualification_candidate_issue>
make_qualification_candidate_issue(qualification_candidate_issue);

product_status_or<qualification_candidate_campaign>
make_qualification_candidate_campaign(
    const qualification_candidate_campaign_plan &,
    std::vector<qualification_candidate_execution_observation>,
    std::vector<qualification_candidate_issue>, bool complete = true);
product_status_or<bool> validate_qualification_candidate_campaign(
    const qualification_candidate_campaign &,
    const qualification_candidate_campaign_plan &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_candidate_campaign(const qualification_candidate_campaign &);
bool qualification_candidate_campaign_gate_passes(
    const qualification_candidate_campaign &,
    const qualification_candidate_campaign_plan &) noexcept;

} // namespace mesh_boolean
} // namespace ygor

#endif
