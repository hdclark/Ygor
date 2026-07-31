#include "MeshBooleanTestHarness.h"

#include <YgorMeshesBooleanQualificationMatrix.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

namespace {

digest tagged_digest(char kind, std::uint64_t ordinal) {
  canonical_encoder encoder;
  encoder.byte(static_cast<std::uint8_t>(kind));
  encoder.u64(ordinal);
  return domain_digest({{'Y', 'G', 'B', 'Q', 'M', 'X', '0', '1'}},
                       encoder.bytes());
}

std::vector<qualification_toolchain_matrix_observation>
passing_toolchain_observations(const qualification_matrix_plan &plan) {
  std::vector<qualification_toolchain_matrix_observation> result;
  std::uint64_t ordinal = 1;
  for (const auto &descriptor : plan.toolchain_cases) {
    qualification_toolchain_matrix_observation observation;
    observation.compiler_version = "compiler-version-" + std::to_string(ordinal);
    observation.standard_library_version =
        "stdlib-version-" + std::to_string(ordinal);
    observation.operating_system = "qualification-os";
    observation.target_triple = "qualification-target-64";
    observation.compile_flags = {"-std=c++17", "-fno-fast-math",
                                 "-ffp-contract=off"};
    observation.environment_digest = tagged_digest('E', ordinal);
    observation.build_log_digest = tagged_digest('B', ordinal);
    observation.test_log_digest = tagged_digest('T', ordinal);
    observation.canonical_result_digest = tagged_digest('R', ordinal);
    observation.executed = true;
    observation.tests_passed = true;
    observation.sanitizer_clean =
        descriptor.sanitizer != qualification_sanitizer_mode::none;
    observation.strict_floating_point_verified = true;
    observation.debug_library_verified = descriptor.debug_library !=
                                         qualification_debug_library_mode::none;
    observation.concurrency_suite_passed =
        descriptor.concurrency_suite_required;
    auto made = make_qualification_toolchain_matrix_observation(
        descriptor, std::move(observation));
    require(made.has_value(), "passing toolchain observation canonicalizes");
    result.push_back(std::move(made.value()));
    ++ordinal;
  }
  return result;
}

std::vector<qualification_determinism_matrix_observation>
passing_determinism_observations(const qualification_matrix_plan &plan) {
  std::vector<qualification_determinism_matrix_observation> result;
  const auto exact = tagged_digest('A', 1);
  const auto bytes = tagged_digest('B', 1);
  const auto failure = tagged_digest('F', 1);
  const auto diagnostics = tagged_digest('D', 1);
  const auto certificate = tagged_digest('C', 1);
  std::uint64_t ordinal = 1;
  for (const auto &descriptor : plan.determinism_cases) {
    qualification_determinism_matrix_observation observation;
    observation.canonical_exact_artifact_digest = exact;
    observation.canonical_result_bytes_digest = bytes;
    observation.canonical_failure_digest = failure;
    observation.canonical_diagnostics_digest = diagnostics;
    observation.canonical_certificate_digest = certificate;
    observation.replay_log_digest = tagged_digest('L', ordinal);
    observation.completed_runs = 3;
    observation.completed = true;
    observation.rounding_mode_controlled = descriptor.axis ==
        qualification_determinism_axis::ambient_rounding_mode;
    observation.separate_process = descriptor.separate_process;
    auto made = make_qualification_determinism_matrix_observation(
        descriptor, std::move(observation));
    require(made.has_value(), "passing determinism observation canonicalizes");
    result.push_back(std::move(made.value()));
    ++ordinal;
  }
  return result;
}

std::vector<qualification_resource_matrix_observation>
passing_resource_observations(const qualification_matrix_plan &plan) {
  std::vector<qualification_resource_matrix_observation> result;
  std::uint64_t ordinal = 1;
  for (const auto &descriptor : plan.resource_cases) {
    qualification_resource_matrix_observation observation;
    observation.observed_error_key = descriptor.expected_error_key;
    observation.publication_state_before_digest = tagged_digest('P', ordinal);
    observation.publication_state_after_digest =
        observation.publication_state_before_digest;
    observation.replay_digest = tagged_digest('R', ordinal);
    observation.observed_cancellation_latency_milliseconds =
        descriptor.kind == qualification_resource_case_kind::cancellation
            ? descriptor.cancellation_latency_limit_milliseconds
            : 0;
    observation.limit_triggered = true;
    observation.typed_failure_observed = true;
    observation.transaction_rolled_back = true;
    observation.replay_passed = true;
    auto made = make_qualification_resource_matrix_observation(
        descriptor, std::move(observation));
    require(made.has_value(), "passing resource observation canonicalizes");
    result.push_back(std::move(made.value()));
    ++ordinal;
  }
  return result;
}

std::vector<qualification_fuzz_matrix_observation>
passing_fuzz_observations(const qualification_matrix_plan &plan,
                          std::uint64_t outcomes = 2) {
  std::vector<qualification_fuzz_matrix_observation> result;
  std::uint64_t ordinal = 1;
  for (const auto &descriptor : plan.fuzz_cases) {
    qualification_fuzz_matrix_observation observation;
    observation.engine = "ygor-construction-aware-fuzzer";
    observation.engine_version = "1";
    observation.aggregate_cpu_seconds =
        descriptor.minimum_aggregate_cpu_seconds;
    observation.wall_seconds = descriptor.minimum_aggregate_cpu_seconds;
    observation.worker_count = descriptor.minimum_worker_count;
    observation.unique_outcome_count = outcomes;
    observation.serialized_outcome_count = outcomes;
    observation.minimized_outcome_count = outcomes;
    observation.promoted_regression_count = outcomes;
    observation.seed_set_digest = tagged_digest('S', ordinal);
    observation.dictionary_digest = tagged_digest('D', ordinal);
    observation.mutator_digest = tagged_digest('M', ordinal);
    observation.corpus_digest = tagged_digest('C', ordinal);
    observation.failure_index_digest = tagged_digest('F', ordinal);
    observation.replay_digest = tagged_digest('R', ordinal);
    observation.complete = true;
    auto made = make_qualification_fuzz_matrix_observation(
        descriptor, std::move(observation));
    require(made.has_value(), "passing fuzz observation canonicalizes");
    result.push_back(std::move(made.value()));
    ++ordinal;
  }
  return result;
}

void catalog_and_coverage_contracts() {
  const auto plan = make_default_qualification_matrix_plan();
  require(validate_qualification_matrix_plan(plan).has_value(),
          "default P6.8 matrix plan validates");
  require_equal(plan.toolchain_cases.size(), std::size_t(15),
                "compiler, library, architecture, sanitizer, and type matrix is frozen");
  require_equal(plan.determinism_cases.size(), std::size_t(22),
                "every required determinism axis has independent variants");
  require_equal(plan.resource_cases.size(), std::size_t(4),
                "bytes, work, timeout, and cancellation are independent");
  require_equal(plan.fuzz_cases.size(), std::size_t(8),
                "sanitized valid/invalid, chain, and long fuzz campaigns are frozen");

  std::set<qualification_determinism_axis> axes;
  std::set<qualification_sanitizer_mode> sanitizers;
  std::set<qualification_architecture> architectures;
  std::set<qualification_floating_point_mode> floating_point_modes;
  std::set<index_tag> indices;
  for (const auto &entry : plan.determinism_cases)
    axes.insert(entry.axis);
  for (const auto &entry : plan.toolchain_cases) {
    sanitizers.insert(entry.sanitizer);
    architectures.insert(entry.architecture);
    floating_point_modes.insert(entry.floating_point_mode);
    indices.insert(entry.type.index);
  }
  require_equal(axes.size(), static_cast<std::size_t>(
                                 qualification_determinism_axis::count),
                "all determinism axes are present");
  require(sanitizers.count(
              qualification_sanitizer_mode::address_and_undefined) &&
              sanitizers.count(qualification_sanitizer_mode::thread),
          "ASan/UBSan and TSan are both mandatory");
  require(architectures.count(qualification_architecture::x86_64) &&
              architectures.count(qualification_architecture::aarch64),
          "x86-64 and an independent 64-bit architecture are mandatory");
  require(floating_point_modes.size() == 2,
          "default and controlled strict floating-point modes are mandatory");
  require(indices.size() == 2, "32-bit and 64-bit index paths are mandatory");
  for (const auto &entry : plan.fuzz_cases)
    require(entry.minimum_aggregate_cpu_seconds >=
                qualification_minimum_fuzz_cpu_seconds,
            "every campaign retains the Plan 16 twenty-four CPU-hour floor");

  auto encoded = encode_qualification_matrix_plan(plan);
  require(encoded.has_value() && encoded.value() == plan.canonical_bytes &&
              plan.plan_digest != digest{},
          "matrix plan has stable canonical bytes and digest");
  auto replay = make_default_qualification_matrix_plan();
  require(replay.canonical_bytes == plan.canonical_bytes &&
              replay.plan_digest == plan.plan_digest,
          "default matrix plan replays byte-identically");
}

void complete_report_contracts() {
  const auto plan = make_default_qualification_matrix_plan();
  auto toolchains = passing_toolchain_observations(plan);
  auto determinism = passing_determinism_observations(plan);
  auto resources = passing_resource_observations(plan);
  auto fuzz = passing_fuzz_observations(plan);
  auto report = make_qualification_matrix_report(
      plan, toolchains, determinism, resources, fuzz);
  require(report.has_value() && report.value().complete &&
              report.value().blocking_issue_count == 0 &&
              report.value().passed_toolchain_cases ==
                  plan.toolchain_cases.size() &&
              report.value().passed_determinism_cases ==
                  plan.determinism_cases.size() &&
              report.value().passed_resource_cases ==
                  plan.resource_cases.size() &&
              report.value().passed_fuzz_cases == plan.fuzz_cases.size() &&
              qualification_matrix_gate_passes(report.value(), plan),
          "complete matrix evidence passes the P6.8 gate");
  require(validate_qualification_matrix_report(report.value(), plan).has_value(),
          "complete matrix report independently reconstructs");
  auto encoded = encode_qualification_matrix_report(report.value());
  require(encoded.has_value() && encoded.value() == report.value().canonical_bytes,
          "matrix report exposes canonical bound bytes");

  std::reverse(toolchains.begin(), toolchains.end());
  std::reverse(determinism.begin(), determinism.end());
  std::reverse(resources.begin(), resources.end());
  std::reverse(fuzz.begin(), fuzz.end());
  auto reordered = make_qualification_matrix_report(
      plan, std::move(toolchains), std::move(determinism),
      std::move(resources), std::move(fuzz));
  require(reordered.has_value() &&
              reordered.value().canonical_bytes == report.value().canonical_bytes &&
              reordered.value().report_digest == report.value().report_digest,
          "matrix observation arrival order cannot affect evidence");
}

void toolchain_fail_closed_contracts() {
  const auto plan = make_default_qualification_matrix_plan();
  const auto ordinary = std::find_if(
      plan.toolchain_cases.begin(), plan.toolchain_cases.end(),
      [](const auto &entry) {
        return entry.sanitizer == qualification_sanitizer_mode::none &&
               entry.debug_library == qualification_debug_library_mode::none;
      });
  require(ordinary != plan.toolchain_cases.end(),
          "ordinary toolchain descriptor exists");
  auto observations = passing_toolchain_observations(plan);
  auto value = observations[static_cast<std::size_t>(
      std::distance(plan.toolchain_cases.begin(), ordinary))];
  value.strict_floating_point_verified = false;
  value.observation_digest = {};
  require(!make_qualification_toolchain_matrix_observation(*ordinary, value)
               .has_value(),
          "unverified strict floating point fails closed");

  qualification_toolchain_matrix_observation skipped;
  skipped.skipped = true;
  skipped.skip_reason = "runner unavailable";
  skipped.skip_evidence_digest = tagged_digest('K', 1);
  require(!make_qualification_toolchain_matrix_observation(*ordinary, skipped)
               .has_value(),
          "mandatory toolchain cases cannot be silently skipped");

  const auto tsan = std::find_if(
      plan.toolchain_cases.begin(), plan.toolchain_cases.end(),
      [](const auto &entry) {
        return entry.sanitizer == qualification_sanitizer_mode::thread;
      });
  require(tsan != plan.toolchain_cases.end(), "TSan descriptor exists");
  value = observations[static_cast<std::size_t>(
      std::distance(plan.toolchain_cases.begin(), tsan))];
  value.concurrency_suite_passed = false;
  value.observation_digest = {};
  require(!make_qualification_toolchain_matrix_observation(*tsan, value)
               .has_value(),
          "TSan without concurrency coverage fails closed");
}

void determinism_fail_closed_contracts() {
  const auto plan = make_default_qualification_matrix_plan();
  auto toolchains = passing_toolchain_observations(plan);
  auto determinism = passing_determinism_observations(plan);
  auto resources = passing_resource_observations(plan);
  auto fuzz = passing_fuzz_observations(plan);

  determinism.back().canonical_result_bytes_digest = tagged_digest('X', 99);
  determinism.back().observation_digest = {};
  auto remade = make_qualification_determinism_matrix_observation(
      plan.determinism_cases.back(), determinism.back());
  require(remade.has_value(),
          "an individually well-formed alternate digest remains observable");
  determinism.back() = std::move(remade.value());
  auto report = make_qualification_matrix_report(
      plan, std::move(toolchains), std::move(determinism),
      std::move(resources), std::move(fuzz));
  require(report.has_value() && !report.value().complete &&
              report.value().blocking_issue_count == 1 &&
              !qualification_matrix_gate_passes(report.value(), plan),
          "any canonical determinism disagreement blocks P6.8");

  const auto rounding = std::find_if(
      plan.determinism_cases.begin(), plan.determinism_cases.end(),
      [](const auto &entry) {
        return entry.axis ==
               qualification_determinism_axis::ambient_rounding_mode;
      });
  require(rounding != plan.determinism_cases.end(),
          "rounding-mode descriptor exists");
  auto observations = passing_determinism_observations(plan);
  auto value = observations[static_cast<std::size_t>(
      std::distance(plan.determinism_cases.begin(), rounding))];
  value.rounding_mode_controlled = false;
  value.observation_digest = {};
  require(!make_qualification_determinism_matrix_observation(*rounding, value)
               .has_value(),
          "ambient rounding variants require controlled replay evidence");
}

void resource_fail_closed_contracts() {
  const auto plan = make_default_qualification_matrix_plan();
  auto observations = passing_resource_observations(plan);
  auto value = observations.front();
  value.partial_publication_observed = true;
  value.observation_digest = {};
  require(!make_qualification_resource_matrix_observation(
               plan.resource_cases.front(), value)
               .has_value(),
          "partial publication under a resource failure is forbidden");

  const auto cancellation = std::find_if(
      plan.resource_cases.begin(), plan.resource_cases.end(),
      [](const auto &entry) {
        return entry.kind == qualification_resource_case_kind::cancellation;
      });
  require(cancellation != plan.resource_cases.end(),
          "cancellation descriptor exists");
  value = observations[static_cast<std::size_t>(
      std::distance(plan.resource_cases.begin(), cancellation))];
  value.observed_cancellation_latency_milliseconds =
      cancellation->cancellation_latency_limit_milliseconds + 1;
  value.observation_digest = {};
  require(!make_qualification_resource_matrix_observation(*cancellation, value)
               .has_value(),
          "cancellation beyond the declared latency fails closed");
}

void fuzz_duration_and_preservation_contracts() {
  const auto plan = make_default_qualification_matrix_plan();
  auto observations = passing_fuzz_observations(plan);
  auto value = observations.front();
  value.aggregate_cpu_seconds =
      plan.fuzz_cases.front().minimum_aggregate_cpu_seconds - 1;
  value.observation_digest = {};
  require(!make_qualification_fuzz_matrix_observation(plan.fuzz_cases.front(),
                                                       value)
               .has_value(),
          "under-duration fuzz evidence cannot satisfy P6.8");

  value = observations.front();
  value.minimized_outcome_count = value.unique_outcome_count - 1;
  value.observation_digest = {};
  require(!make_qualification_fuzz_matrix_observation(plan.fuzz_cases.front(),
                                                       value)
               .has_value(),
          "every unique fuzz outcome must be minimized");

  value = observations.front();
  value.unresolved_outcome_count = 1;
  value.observation_digest = {};
  require(!make_qualification_fuzz_matrix_observation(plan.fuzz_cases.front(),
                                                       value)
               .has_value(),
          "unresolved fuzz outcomes remain blocking");

  auto report = make_qualification_matrix_report(
      plan, passing_toolchain_observations(plan),
      passing_determinism_observations(plan),
      passing_resource_observations(plan),
      passing_fuzz_observations(plan));
  require(report.has_value(), "bound report exists for mutation test");
  auto stale = report.value();
  stale.report_digest.bytes[0] ^= 1U;
  require(!validate_qualification_matrix_report(stale, plan).has_value(),
          "matrix report digest mutation fails reconstruction");
}

} // namespace

int main() {
  harness tests;
  tests.add("P6.8.catalog", catalog_and_coverage_contracts);
  tests.add("P6.8.report", complete_report_contracts);
  tests.add("P6.8.toolchains", toolchain_fail_closed_contracts);
  tests.add("P6.8.determinism", determinism_fail_closed_contracts);
  tests.add("P6.8.resources", resource_fail_closed_contracts);
  tests.add("P6.8.fuzz", fuzz_duration_and_preservation_contracts);
  return tests.run(std::cout, std::cerr);
}
