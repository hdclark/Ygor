#pragma once
#ifndef YGOR_MESHES_BOOLEAN_QUALIFICATION_PERFORMANCE_H_
#define YGOR_MESHES_BOOLEAN_QUALIFICATION_PERFORMANCE_H_

#include "YgorMeshesBooleanQualification.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t qualification_performance_schema_version = 1;
constexpr std::uint32_t qualification_performance_checker_version = 1;
constexpr std::uint32_t qualification_performance_default_warmup_runs = 3;
constexpr std::uint32_t qualification_performance_default_measured_runs = 7;

enum class qualification_performance_workload_kind : std::uint8_t {
  strict_validation,
  normalization_diagnosis,
  normalization_repair,
  backend_evaluation,
  topology_preflight,
  exact_result_serialization,
  exact_in_T_realization,
  certified_approximate_realization,
  mandatory_verification,
  exhaustive_verification,
  operation_chain,
  resource_limit_transaction,
  cancellation_transaction,
  count
};

enum class qualification_performance_scale : std::uint8_t {
  small,
  medium,
  large,
  adversarial,
  count
};

enum class qualification_performance_metric : std::uint8_t {
  wall_time_nanoseconds,
  cpu_time_nanoseconds,
  peak_rss_bytes,
  authoritative_bytes,
  peak_exact_number_bits,
  peak_exact_number_limbs,
  candidate_count,
  event_count,
  patch_count,
  verifier_time_nanoseconds,
  realization_search_nodes,
  realization_components,
  serialization_bytes,
  cancellation_latency_milliseconds,
  count
};

enum class qualification_performance_threshold_kind : std::uint8_t {
  geometric_mean_speedup,
  case_speedup,
  maximum_case_slowdown,
  authoritative_bytes_nonincrease,
  absolute_upper_bound,
  count
};

enum class qualification_performance_statistic : std::uint8_t {
  median_and_mad,
  count
};

constexpr std::size_t qualification_performance_metric_count =
    static_cast<std::size_t>(qualification_performance_metric::count);
constexpr std::uint64_t qualification_performance_all_metrics_mask =
    (std::uint64_t{1} << qualification_performance_metric_count) - 1U;

constexpr std::uint64_t qualification_performance_metric_bit(
    qualification_performance_metric metric) noexcept {
  return std::uint64_t{1} << static_cast<std::size_t>(metric);
}

struct qualification_performance_protocol_v1 {
  std::uint16_t schema = qualification_performance_schema_version;
  std::string identifier;
  std::string version;
  qualification_performance_statistic statistic =
      qualification_performance_statistic::median_and_mad;
  std::uint32_t warmup_runs = qualification_performance_default_warmup_runs;
  std::uint32_t measured_runs = qualification_performance_default_measured_runs;
  bool retain_raw_samples = true;
  bool monotonic_wall_clock_required = true;
  bool process_cpu_clock_required = true;
  bool controlled_exclusive_host_required = true;
  bool cpu_affinity_required = true;
  bool frequency_policy_recorded = true;
  bool allocator_policy_recorded = true;
  bool peak_rss_external_observer_required = true;
  bool authoritative_accounting_separate = true;
  digest protocol_digest;
};

struct qualification_performance_case {
  std::uint16_t schema = qualification_performance_schema_version;
  std::string identifier;
  qualification_performance_workload_kind kind =
      qualification_performance_workload_kind::strict_validation;
  qualification_performance_scale scale =
      qualification_performance_scale::small;
  std::string fixture_identifier;
  std::string geometry_category;
  std::string normalization_policy_identifier;
  operation selected_operation = operation::regularized_union;
  qualification_type_binding type;
  qualification_outcome expected_outcome =
      qualification_outcome::verified_exact_success;
  std::string expected_error_key;
  bool baseline_comparison_required = false;
  std::string baseline_family;
  std::string comparison_group;
  std::uint64_t required_metrics_mask =
      qualification_performance_all_metrics_mask;
  std::uint64_t cancellation_latency_limit_milliseconds = 0;
  digest workload_recipe_digest;
  digest case_digest;
};

struct qualification_performance_threshold {
  std::uint16_t schema = qualification_performance_schema_version;
  std::string identifier;
  qualification_performance_threshold_kind kind =
      qualification_performance_threshold_kind::absolute_upper_bound;
  qualification_performance_metric metric =
      qualification_performance_metric::wall_time_nanoseconds;
  std::string scope;
  std::uint64_t numerator = 0;
  std::uint64_t denominator = 1;
  bool blocking = true;
  digest threshold_digest;
};

struct qualification_performance_threshold_review {
  std::uint16_t schema = qualification_performance_schema_version;
  std::string threshold_identifier;
  digest prior_threshold_digest;
  digest revised_threshold_digest;
  std::string reviewer;
  std::string rationale;
  digest evidence_digest;
  bool approved = false;
  digest review_digest;
};

struct qualification_performance_plan {
  std::uint16_t schema = qualification_performance_schema_version;
  std::uint32_t checker_version = qualification_performance_checker_version;
  std::string identifier;
  qualification_performance_protocol_v1 protocol;
  std::vector<qualification_performance_case> cases;
  std::vector<qualification_performance_threshold> thresholds;
  std::vector<qualification_performance_threshold_review> threshold_reviews;
  std::vector<std::uint8_t> canonical_bytes;
  digest plan_digest;
};

struct qualification_performance_sample {
  std::uint16_t schema = qualification_performance_schema_version;
  std::uint32_t ordinal = 0;
  std::uint64_t metric_presence_mask = 0;
  std::array<std::uint64_t, qualification_performance_metric_count> metrics{{}};
  qualification_outcome outcome = qualification_outcome::infrastructure_failure;
  std::string typed_outcome_key;
  digest canonical_semantic_digest;
  digest canonical_failure_digest;
  digest run_log_digest;
  digest sample_digest;

  std::uint64_t metric(qualification_performance_metric value) const noexcept {
    return metrics[static_cast<std::size_t>(value)];
  }
};

struct qualification_performance_observation {
  std::uint16_t schema = qualification_performance_schema_version;
  std::string case_identifier;
  digest case_digest;
  digest protocol_digest;
  digest hardware_digest;
  digest environment_digest;
  digest command_digest;
  std::uint32_t completed_warmup_runs = 0;
  std::vector<qualification_performance_sample> baseline_samples;
  std::vector<qualification_performance_sample> candidate_samples;
  digest publication_state_before_digest;
  digest publication_state_after_digest;
  digest replay_digest;
  bool limit_or_cancellation_triggered = false;
  bool typed_failure_observed = false;
  bool transaction_rolled_back = false;
  bool partial_publication_observed = false;
  digest observation_digest;
};

struct qualification_performance_case_statistics {
  std::uint16_t schema = qualification_performance_schema_version;
  std::string case_identifier;
  digest case_digest;
  bool baseline_present = false;
  std::array<std::uint64_t, qualification_performance_metric_count>
      baseline_medians{{}};
  std::array<std::uint64_t, qualification_performance_metric_count>
      baseline_mads{{}};
  std::array<std::uint64_t, qualification_performance_metric_count>
      candidate_medians{{}};
  std::array<std::uint64_t, qualification_performance_metric_count>
      candidate_mads{{}};
  digest canonical_semantic_digest;
  digest canonical_failure_digest;
  digest statistics_digest;
};

struct qualification_performance_report {
  std::uint16_t schema = qualification_performance_schema_version;
  std::uint32_t checker_version = qualification_performance_checker_version;
  std::string identifier;
  digest plan_digest;
  std::vector<qualification_performance_observation> observations;
  std::vector<qualification_performance_case_statistics> statistics;
  std::uint64_t passed_case_count = 0;
  std::uint64_t passed_threshold_count = 0;
  std::uint64_t blocking_issue_count = 0;
  bool complete = false;
  std::vector<std::uint8_t> canonical_bytes;
  digest report_digest;
};

const char *qualification_performance_workload_kind_token(
    qualification_performance_workload_kind) noexcept;
const char *qualification_performance_metric_token(
    qualification_performance_metric) noexcept;
const char *qualification_performance_threshold_kind_token(
    qualification_performance_threshold_kind) noexcept;

qualification_performance_plan make_default_qualification_performance_plan();
product_status_or<qualification_performance_plan>
make_qualification_performance_plan(qualification_performance_plan);
product_status_or<bool> validate_qualification_performance_plan(
    const qualification_performance_plan &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_performance_plan(const qualification_performance_plan &);
product_status_or<qualification_performance_threshold>
make_qualification_performance_threshold(qualification_performance_threshold);
product_status_or<qualification_performance_threshold_review>
make_qualification_performance_threshold_review(
    qualification_performance_threshold_review);

product_status_or<qualification_performance_sample>
make_qualification_performance_sample(qualification_performance_sample);
product_status_or<qualification_performance_observation>
make_qualification_performance_observation(
    const qualification_performance_plan &,
    const qualification_performance_case &,
    qualification_performance_observation);

product_status_or<qualification_performance_report>
make_qualification_performance_report(
    const qualification_performance_plan &,
    std::vector<qualification_performance_observation>, bool complete = true);
product_status_or<bool> validate_qualification_performance_report(
    const qualification_performance_report &,
    const qualification_performance_plan &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_performance_report(const qualification_performance_report &);
bool qualification_performance_gate_passes(
    const qualification_performance_report &,
    const qualification_performance_plan &) noexcept;

} // namespace mesh_boolean
} // namespace ygor

#endif
