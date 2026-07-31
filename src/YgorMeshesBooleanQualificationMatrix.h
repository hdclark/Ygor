#pragma once
#ifndef YGOR_MESHES_BOOLEAN_QUALIFICATION_MATRIX_H_
#define YGOR_MESHES_BOOLEAN_QUALIFICATION_MATRIX_H_

#include "YgorMeshesBooleanQualification.h"

#include <cstdint>
#include <string>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t qualification_matrix_schema_version = 1;
constexpr std::uint32_t qualification_matrix_checker_version = 1;
constexpr std::uint64_t qualification_minimum_fuzz_cpu_seconds = 24ULL * 60ULL * 60ULL;

enum class qualification_compiler_family : std::uint8_t {
  gcc,
  clang,
  count
};

enum class qualification_compiler_release : std::uint8_t {
  current,
  oldest_supported,
  count
};

enum class qualification_build_mode : std::uint8_t {
  debug,
  optimized,
  count
};

enum class qualification_sanitizer_mode : std::uint8_t {
  none,
  address_and_undefined,
  thread,
  count
};

enum class qualification_debug_library_mode : std::uint8_t {
  none,
  libstdcxx_debug,
  libcxx_debug,
  count
};

enum class qualification_determinism_axis : std::uint8_t {
  worker_count,
  task_partition,
  queue_bound,
  broad_phase,
  predicate_filter,
  allocation_perturbation,
  hash_seed,
  ambient_rounding_mode,
  separate_process_replay,
  count
};

enum class qualification_resource_case_kind : std::uint8_t {
  authoritative_bytes,
  work_units,
  wall_timeout,
  cancellation,
  count
};

enum class qualification_fuzz_family : std::uint8_t {
  valid_geometry,
  invalid_preparation,
  operation_chain,
  long_running_unsanitized,
  count
};

struct qualification_toolchain_matrix_case {
  std::uint16_t schema = qualification_matrix_schema_version;
  std::string identifier;
  qualification_compiler_family compiler = qualification_compiler_family::gcc;
  qualification_compiler_release release =
      qualification_compiler_release::current;
  qualification_standard_library standard_library =
      qualification_standard_library::libstdcxx;
  qualification_architecture architecture = qualification_architecture::x86_64;
  qualification_build_mode build = qualification_build_mode::debug;
  qualification_sanitizer_mode sanitizer =
      qualification_sanitizer_mode::none;
  qualification_debug_library_mode debug_library =
      qualification_debug_library_mode::none;
  qualification_floating_point_mode floating_point_mode =
      qualification_floating_point_mode::strict_default_round_to_nearest;
  qualification_type_binding type;
  bool concurrency_suite_required = false;
  bool documented_skip_permitted = false;
  digest case_digest;
};

struct qualification_determinism_matrix_case {
  std::uint16_t schema = qualification_matrix_schema_version;
  std::string identifier;
  qualification_determinism_axis axis =
      qualification_determinism_axis::worker_count;
  std::string variant;
  std::string equivalence_group;
  std::uint64_t numeric_parameter = 0;
  bool separate_process = false;
  digest case_digest;
};

struct qualification_resource_matrix_case {
  std::uint16_t schema = qualification_matrix_schema_version;
  std::string identifier;
  qualification_resource_case_kind kind =
      qualification_resource_case_kind::authoritative_bytes;
  std::uint64_t declared_limit = 0;
  std::uint64_t cancellation_latency_limit_milliseconds = 0;
  std::string expected_error_key;
  bool replay_required = true;
  digest case_digest;
};

struct qualification_fuzz_matrix_case {
  std::uint16_t schema = qualification_matrix_schema_version;
  std::string identifier;
  qualification_fuzz_family family =
      qualification_fuzz_family::valid_geometry;
  std::string configuration_identifier;
  qualification_sanitizer_mode sanitizer =
      qualification_sanitizer_mode::none;
  std::uint64_t minimum_aggregate_cpu_seconds =
      qualification_minimum_fuzz_cpu_seconds;
  std::uint32_t minimum_worker_count = 1;
  digest case_digest;
};

struct qualification_matrix_plan {
  std::uint16_t schema = qualification_matrix_schema_version;
  std::uint32_t checker_version = qualification_matrix_checker_version;
  std::string identifier;
  std::vector<qualification_toolchain_matrix_case> toolchain_cases;
  std::vector<qualification_determinism_matrix_case> determinism_cases;
  std::vector<qualification_resource_matrix_case> resource_cases;
  std::vector<qualification_fuzz_matrix_case> fuzz_cases;
  std::vector<std::uint8_t> canonical_bytes;
  digest plan_digest;
};

struct qualification_toolchain_matrix_observation {
  std::uint16_t schema = qualification_matrix_schema_version;
  std::string case_identifier;
  digest case_digest;
  std::string compiler_version;
  std::string standard_library_version;
  std::string operating_system;
  std::string target_triple;
  std::vector<std::string> compile_flags;
  digest environment_digest;
  digest build_log_digest;
  digest test_log_digest;
  digest canonical_result_digest;
  bool executed = false;
  bool skipped = false;
  std::string skip_reason;
  digest skip_evidence_digest;
  bool tests_passed = false;
  bool sanitizer_clean = false;
  bool strict_floating_point_verified = false;
  bool debug_library_verified = false;
  bool concurrency_suite_passed = false;
  digest observation_digest;
};

struct qualification_determinism_matrix_observation {
  std::uint16_t schema = qualification_matrix_schema_version;
  std::string case_identifier;
  digest case_digest;
  digest canonical_exact_artifact_digest;
  digest canonical_result_bytes_digest;
  digest canonical_failure_digest;
  digest canonical_diagnostics_digest;
  digest canonical_certificate_digest;
  digest replay_log_digest;
  std::uint64_t completed_runs = 0;
  bool completed = false;
  bool rounding_mode_controlled = false;
  bool separate_process = false;
  digest observation_digest;
};

struct qualification_resource_matrix_observation {
  std::uint16_t schema = qualification_matrix_schema_version;
  std::string case_identifier;
  digest case_digest;
  std::string observed_error_key;
  digest publication_state_before_digest;
  digest publication_state_after_digest;
  digest replay_digest;
  std::uint64_t observed_cancellation_latency_milliseconds = 0;
  bool limit_triggered = false;
  bool typed_failure_observed = false;
  bool transaction_rolled_back = false;
  bool partial_publication_observed = false;
  bool replay_passed = false;
  digest observation_digest;
};

struct qualification_fuzz_matrix_observation {
  std::uint16_t schema = qualification_matrix_schema_version;
  std::string case_identifier;
  digest case_digest;
  std::string engine;
  std::string engine_version;
  std::uint64_t aggregate_cpu_seconds = 0;
  std::uint64_t wall_seconds = 0;
  std::uint32_t worker_count = 0;
  std::uint64_t unique_outcome_count = 0;
  std::uint64_t serialized_outcome_count = 0;
  std::uint64_t minimized_outcome_count = 0;
  std::uint64_t promoted_regression_count = 0;
  std::uint64_t unresolved_outcome_count = 0;
  std::uint64_t false_success_count = 0;
  std::uint64_t nondeterministic_outcome_count = 0;
  std::uint64_t infrastructure_failure_count = 0;
  digest seed_set_digest;
  digest dictionary_digest;
  digest mutator_digest;
  digest corpus_digest;
  digest failure_index_digest;
  digest replay_digest;
  bool complete = false;
  digest observation_digest;
};

struct qualification_matrix_report {
  std::uint16_t schema = qualification_matrix_schema_version;
  std::uint32_t checker_version = qualification_matrix_checker_version;
  std::string identifier;
  digest plan_digest;
  std::vector<qualification_toolchain_matrix_observation>
      toolchain_observations;
  std::vector<qualification_determinism_matrix_observation>
      determinism_observations;
  std::vector<qualification_resource_matrix_observation> resource_observations;
  std::vector<qualification_fuzz_matrix_observation> fuzz_observations;
  std::uint64_t passed_toolchain_cases = 0;
  std::uint64_t passed_determinism_cases = 0;
  std::uint64_t passed_resource_cases = 0;
  std::uint64_t passed_fuzz_cases = 0;
  std::uint64_t blocking_issue_count = 0;
  bool complete = false;
  std::vector<std::uint8_t> canonical_bytes;
  digest report_digest;
};

const char *qualification_compiler_family_token(
    qualification_compiler_family) noexcept;
const char *qualification_sanitizer_mode_token(
    qualification_sanitizer_mode) noexcept;
const char *qualification_determinism_axis_token(
    qualification_determinism_axis) noexcept;
const char *qualification_fuzz_family_token(
    qualification_fuzz_family) noexcept;

qualification_matrix_plan make_default_qualification_matrix_plan();
product_status_or<qualification_matrix_plan>
make_qualification_matrix_plan(qualification_matrix_plan);
product_status_or<bool>
validate_qualification_matrix_plan(const qualification_matrix_plan &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_matrix_plan(const qualification_matrix_plan &);

product_status_or<qualification_toolchain_matrix_observation>
make_qualification_toolchain_matrix_observation(
    const qualification_toolchain_matrix_case &,
    qualification_toolchain_matrix_observation);
product_status_or<qualification_determinism_matrix_observation>
make_qualification_determinism_matrix_observation(
    const qualification_determinism_matrix_case &,
    qualification_determinism_matrix_observation);
product_status_or<qualification_resource_matrix_observation>
make_qualification_resource_matrix_observation(
    const qualification_resource_matrix_case &,
    qualification_resource_matrix_observation);
product_status_or<qualification_fuzz_matrix_observation>
make_qualification_fuzz_matrix_observation(
    const qualification_fuzz_matrix_case &,
    qualification_fuzz_matrix_observation);

product_status_or<qualification_matrix_report>
make_qualification_matrix_report(
    const qualification_matrix_plan &,
    std::vector<qualification_toolchain_matrix_observation>,
    std::vector<qualification_determinism_matrix_observation>,
    std::vector<qualification_resource_matrix_observation>,
    std::vector<qualification_fuzz_matrix_observation>, bool complete = true);
product_status_or<bool> validate_qualification_matrix_report(
    const qualification_matrix_report &,
    const qualification_matrix_plan &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_matrix_report(const qualification_matrix_report &);
bool qualification_matrix_gate_passes(const qualification_matrix_report &,
                                      const qualification_matrix_plan &) noexcept;

} // namespace mesh_boolean
} // namespace ygor

#endif
