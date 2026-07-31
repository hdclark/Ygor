#include "YgorMeshesBooleanQualificationMatrix.h"

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

constexpr std::array<char, 8> plan_tag{{'Y', 'G', 'B', 'Q', 'M', 'P', '0', '1'}};
constexpr std::array<char, 8> toolchain_case_tag{{'Y', 'G', 'B', 'Q', 'M', 'T', '0', '1'}};
constexpr std::array<char, 8> determinism_case_tag{{'Y', 'G', 'B', 'Q', 'M', 'D', '0', '1'}};
constexpr std::array<char, 8> resource_case_tag{{'Y', 'G', 'B', 'Q', 'M', 'R', '0', '1'}};
constexpr std::array<char, 8> fuzz_case_tag{{'Y', 'G', 'B', 'Q', 'M', 'F', '0', '1'}};
constexpr std::array<char, 8> toolchain_observation_tag{{'Y', 'G', 'B', 'Q', 'M', 'O', '0', '1'}};
constexpr std::array<char, 8> determinism_observation_tag{{'Y', 'G', 'B', 'Q', 'M', 'E', '0', '1'}};
constexpr std::array<char, 8> resource_observation_tag{{'Y', 'G', 'B', 'Q', 'M', 'L', '0', '1'}};
constexpr std::array<char, 8> fuzz_observation_tag{{'Y', 'G', 'B', 'Q', 'M', 'U', '0', '1'}};
constexpr std::array<char, 8> report_tag{{'Y', 'G', 'B', 'Q', 'M', 'Q', '0', '1'}};

product_error matrix_error(const char *key) {
  return make_product_error(product_error_code::qualification_policy_violation,
                            key);
}

bool zero(const digest &value) noexcept { return value == digest{}; }

bool text(const std::string &value, bool allow_empty = false) noexcept {
  return (allow_empty || !value.empty()) && value.size() <= 1024U &&
         std::find(value.begin(), value.end(), '\0') == value.end();
}

template <class E> bool enum_before_count(E value, E count) noexcept {
  return static_cast<unsigned>(value) < static_cast<unsigned>(count);
}

bool known(qualification_compiler_family value) noexcept {
  return enum_before_count(value, qualification_compiler_family::count);
}
bool known(qualification_compiler_release value) noexcept {
  return enum_before_count(value, qualification_compiler_release::count);
}
bool known(qualification_build_mode value) noexcept {
  return enum_before_count(value, qualification_build_mode::count);
}
bool known(qualification_sanitizer_mode value) noexcept {
  return enum_before_count(value, qualification_sanitizer_mode::count);
}
bool known(qualification_debug_library_mode value) noexcept {
  return enum_before_count(value, qualification_debug_library_mode::count);
}
bool known(qualification_determinism_axis value) noexcept {
  return enum_before_count(value, qualification_determinism_axis::count);
}
bool known(qualification_resource_case_kind value) noexcept {
  return enum_before_count(value, qualification_resource_case_kind::count);
}
bool known(qualification_fuzz_family value) noexcept {
  return enum_before_count(value, qualification_fuzz_family::count);
}
bool known(qualification_standard_library value) noexcept {
  return value >= qualification_standard_library::libstdcxx &&
         value <= qualification_standard_library::other;
}
bool known(qualification_architecture value) noexcept {
  return value >= qualification_architecture::x86_64 &&
         value <= qualification_architecture::other_64_bit;
}
bool known(qualification_floating_point_mode value) noexcept {
  return value >=
             qualification_floating_point_mode::strict_default_round_to_nearest &&
         value <=
             qualification_floating_point_mode::strict_controlled_rounding_matrix;
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

void encode_string_vector(canonical_encoder &encoder,
                          const std::vector<std::string> &values) {
  encoder.u64(values.size());
  for (const auto &value : values)
    encoder.string(value);
}

bool valid_string_vector(const std::vector<std::string> &values) noexcept {
  return !values.empty() && values.size() <= 256U &&
         std::all_of(values.begin(), values.end(),
                     [](const auto &value) { return text(value); });
}

void encode_toolchain_case(canonical_encoder &encoder,
                           const qualification_toolchain_matrix_case &value,
                           bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.byte(static_cast<std::uint8_t>(value.compiler));
  encoder.byte(static_cast<std::uint8_t>(value.release));
  encoder.byte(static_cast<std::uint8_t>(value.standard_library));
  encoder.byte(static_cast<std::uint8_t>(value.architecture));
  encoder.byte(static_cast<std::uint8_t>(value.build));
  encoder.byte(static_cast<std::uint8_t>(value.sanitizer));
  encoder.byte(static_cast<std::uint8_t>(value.debug_library));
  encoder.byte(static_cast<std::uint8_t>(value.floating_point_mode));
  encoder.byte(static_cast<std::uint8_t>(value.type.coordinate));
  encoder.byte(static_cast<std::uint8_t>(value.type.index));
  encoder.boolean(value.concurrency_suite_required);
  encoder.boolean(value.documented_skip_permitted);
  if (include_digest)
    encode_digest(encoder, value.case_digest);
}

void encode_determinism_case(
    canonical_encoder &encoder,
    const qualification_determinism_matrix_case &value, bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.byte(static_cast<std::uint8_t>(value.axis));
  encoder.string(value.variant);
  encoder.string(value.equivalence_group);
  encoder.u64(value.numeric_parameter);
  encoder.boolean(value.separate_process);
  if (include_digest)
    encode_digest(encoder, value.case_digest);
}

void encode_resource_case(canonical_encoder &encoder,
                          const qualification_resource_matrix_case &value,
                          bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.byte(static_cast<std::uint8_t>(value.kind));
  encoder.u64(value.declared_limit);
  encoder.u64(value.cancellation_latency_limit_milliseconds);
  encoder.string(value.expected_error_key);
  encoder.boolean(value.replay_required);
  if (include_digest)
    encode_digest(encoder, value.case_digest);
}

void encode_fuzz_case(canonical_encoder &encoder,
                      const qualification_fuzz_matrix_case &value,
                      bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.byte(static_cast<std::uint8_t>(value.family));
  encoder.string(value.configuration_identifier);
  encoder.byte(static_cast<std::uint8_t>(value.sanitizer));
  encoder.u64(value.minimum_aggregate_cpu_seconds);
  encoder.u32(value.minimum_worker_count);
  if (include_digest)
    encode_digest(encoder, value.case_digest);
}

digest toolchain_case_digest(
    const qualification_toolchain_matrix_case &value) {
  canonical_encoder encoder;
  encode_toolchain_case(encoder, value, false);
  return domain_digest(toolchain_case_tag, encoder.bytes());
}

digest determinism_case_digest(
    const qualification_determinism_matrix_case &value) {
  canonical_encoder encoder;
  encode_determinism_case(encoder, value, false);
  return domain_digest(determinism_case_tag, encoder.bytes());
}

digest resource_case_digest(const qualification_resource_matrix_case &value) {
  canonical_encoder encoder;
  encode_resource_case(encoder, value, false);
  return domain_digest(resource_case_tag, encoder.bytes());
}

digest fuzz_case_digest(const qualification_fuzz_matrix_case &value) {
  canonical_encoder encoder;
  encode_fuzz_case(encoder, value, false);
  return domain_digest(fuzz_case_tag, encoder.bytes());
}

product_status_or<qualification_toolchain_matrix_case>
canonicalize_toolchain_case(qualification_toolchain_matrix_case value) {
  if (value.schema != qualification_matrix_schema_version ||
      !text(value.identifier) || !known(value.compiler) ||
      !known(value.release) || !known(value.standard_library) ||
      !known(value.architecture) || !known(value.build) ||
      !known(value.sanitizer) || !known(value.debug_library) ||
      !known(value.floating_point_mode) || !known(value.type.coordinate) ||
      !known(value.type.index))
    return matrix_error("qualification_matrix.toolchain_case_malformed");
  if (value.sanitizer == qualification_sanitizer_mode::thread &&
      !value.concurrency_suite_required)
    return matrix_error("qualification_matrix.tsan_concurrency_required");
  if (value.sanitizer != qualification_sanitizer_mode::thread &&
      value.concurrency_suite_required)
    return matrix_error("qualification_matrix.concurrency_without_tsan");
  if (value.debug_library != qualification_debug_library_mode::none &&
      value.build != qualification_build_mode::debug)
    return matrix_error("qualification_matrix.debug_library_requires_debug");
  if (value.debug_library == qualification_debug_library_mode::libstdcxx_debug &&
      value.standard_library != qualification_standard_library::libstdcxx)
    return matrix_error("qualification_matrix.libstdcxx_debug_mismatch");
  if (value.debug_library == qualification_debug_library_mode::libcxx_debug &&
      value.standard_library != qualification_standard_library::libcxx)
    return matrix_error("qualification_matrix.libcxx_debug_mismatch");
  const auto calculated = toolchain_case_digest(value);
  if (!zero(value.case_digest) && value.case_digest != calculated)
    return matrix_error("qualification_matrix.toolchain_case_digest");
  value.case_digest = calculated;
  return value;
}

product_status_or<qualification_determinism_matrix_case>
canonicalize_determinism_case(qualification_determinism_matrix_case value) {
  if (value.schema != qualification_matrix_schema_version ||
      !text(value.identifier) || !known(value.axis) || !text(value.variant) ||
      !text(value.equivalence_group))
    return matrix_error("qualification_matrix.determinism_case_malformed");
  const auto process_axis =
      value.axis == qualification_determinism_axis::separate_process_replay;
  if (value.separate_process && !process_axis)
    return matrix_error("qualification_matrix.unexpected_separate_process");
  const auto calculated = determinism_case_digest(value);
  if (!zero(value.case_digest) && value.case_digest != calculated)
    return matrix_error("qualification_matrix.determinism_case_digest");
  value.case_digest = calculated;
  return value;
}

product_status_or<qualification_resource_matrix_case>
canonicalize_resource_case(qualification_resource_matrix_case value) {
  if (value.schema != qualification_matrix_schema_version ||
      !text(value.identifier) || !known(value.kind) ||
      value.declared_limit == 0 || !text(value.expected_error_key))
    return matrix_error("qualification_matrix.resource_case_malformed");
  if (value.kind == qualification_resource_case_kind::cancellation) {
    if (value.cancellation_latency_limit_milliseconds == 0)
      return matrix_error("qualification_matrix.cancellation_latency_missing");
  } else if (value.cancellation_latency_limit_milliseconds != 0) {
    return matrix_error("qualification_matrix.unexpected_cancellation_latency");
  }
  if (!value.replay_required)
    return matrix_error("qualification_matrix.resource_replay_required");
  const auto calculated = resource_case_digest(value);
  if (!zero(value.case_digest) && value.case_digest != calculated)
    return matrix_error("qualification_matrix.resource_case_digest");
  value.case_digest = calculated;
  return value;
}

product_status_or<qualification_fuzz_matrix_case>
canonicalize_fuzz_case(qualification_fuzz_matrix_case value) {
  if (value.schema != qualification_matrix_schema_version ||
      !text(value.identifier) || !known(value.family) ||
      !text(value.configuration_identifier) || !known(value.sanitizer) ||
      value.minimum_aggregate_cpu_seconds <
          qualification_minimum_fuzz_cpu_seconds ||
      value.minimum_worker_count == 0)
    return matrix_error("qualification_matrix.fuzz_case_malformed");
  const auto unsanitized =
      value.family == qualification_fuzz_family::operation_chain ||
      value.family == qualification_fuzz_family::long_running_unsanitized;
  if (unsanitized !=
      (value.sanitizer == qualification_sanitizer_mode::none))
    return matrix_error("qualification_matrix.fuzz_sanitizer_mismatch");
  const auto calculated = fuzz_case_digest(value);
  if (!zero(value.case_digest) && value.case_digest != calculated)
    return matrix_error("qualification_matrix.fuzz_case_digest");
  value.case_digest = calculated;
  return value;
}

void encode_toolchain_observation(
    canonical_encoder &encoder,
    const qualification_toolchain_matrix_observation &value,
    bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.case_identifier);
  encode_digest(encoder, value.case_digest);
  encoder.string(value.compiler_version);
  encoder.string(value.standard_library_version);
  encoder.string(value.operating_system);
  encoder.string(value.target_triple);
  encode_string_vector(encoder, value.compile_flags);
  encode_digest(encoder, value.environment_digest);
  encode_digest(encoder, value.build_log_digest);
  encode_digest(encoder, value.test_log_digest);
  encode_digest(encoder, value.canonical_result_digest);
  encoder.boolean(value.executed);
  encoder.boolean(value.skipped);
  encoder.string(value.skip_reason);
  encode_digest(encoder, value.skip_evidence_digest);
  encoder.boolean(value.tests_passed);
  encoder.boolean(value.sanitizer_clean);
  encoder.boolean(value.strict_floating_point_verified);
  encoder.boolean(value.debug_library_verified);
  encoder.boolean(value.concurrency_suite_passed);
  if (include_digest)
    encode_digest(encoder, value.observation_digest);
}

void encode_determinism_observation(
    canonical_encoder &encoder,
    const qualification_determinism_matrix_observation &value,
    bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.case_identifier);
  encode_digest(encoder, value.case_digest);
  encode_digest(encoder, value.canonical_exact_artifact_digest);
  encode_digest(encoder, value.canonical_result_bytes_digest);
  encode_digest(encoder, value.canonical_failure_digest);
  encode_digest(encoder, value.canonical_diagnostics_digest);
  encode_digest(encoder, value.canonical_certificate_digest);
  encode_digest(encoder, value.replay_log_digest);
  encoder.u64(value.completed_runs);
  encoder.boolean(value.completed);
  encoder.boolean(value.rounding_mode_controlled);
  encoder.boolean(value.separate_process);
  if (include_digest)
    encode_digest(encoder, value.observation_digest);
}

void encode_resource_observation(
    canonical_encoder &encoder,
    const qualification_resource_matrix_observation &value,
    bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.case_identifier);
  encode_digest(encoder, value.case_digest);
  encoder.string(value.observed_error_key);
  encode_digest(encoder, value.publication_state_before_digest);
  encode_digest(encoder, value.publication_state_after_digest);
  encode_digest(encoder, value.replay_digest);
  encoder.u64(value.observed_cancellation_latency_milliseconds);
  encoder.boolean(value.limit_triggered);
  encoder.boolean(value.typed_failure_observed);
  encoder.boolean(value.transaction_rolled_back);
  encoder.boolean(value.partial_publication_observed);
  encoder.boolean(value.replay_passed);
  if (include_digest)
    encode_digest(encoder, value.observation_digest);
}

void encode_fuzz_observation(canonical_encoder &encoder,
                             const qualification_fuzz_matrix_observation &value,
                             bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.case_identifier);
  encode_digest(encoder, value.case_digest);
  encoder.string(value.engine);
  encoder.string(value.engine_version);
  encoder.u64(value.aggregate_cpu_seconds);
  encoder.u64(value.wall_seconds);
  encoder.u32(value.worker_count);
  encoder.u64(value.unique_outcome_count);
  encoder.u64(value.serialized_outcome_count);
  encoder.u64(value.minimized_outcome_count);
  encoder.u64(value.promoted_regression_count);
  encoder.u64(value.unresolved_outcome_count);
  encoder.u64(value.false_success_count);
  encoder.u64(value.nondeterministic_outcome_count);
  encoder.u64(value.infrastructure_failure_count);
  encode_digest(encoder, value.seed_set_digest);
  encode_digest(encoder, value.dictionary_digest);
  encode_digest(encoder, value.mutator_digest);
  encode_digest(encoder, value.corpus_digest);
  encode_digest(encoder, value.failure_index_digest);
  encode_digest(encoder, value.replay_digest);
  encoder.boolean(value.complete);
  if (include_digest)
    encode_digest(encoder, value.observation_digest);
}

digest toolchain_observation_digest(
    const qualification_toolchain_matrix_observation &value) {
  canonical_encoder encoder;
  encode_toolchain_observation(encoder, value, false);
  return domain_digest(toolchain_observation_tag, encoder.bytes());
}

digest determinism_observation_digest(
    const qualification_determinism_matrix_observation &value) {
  canonical_encoder encoder;
  encode_determinism_observation(encoder, value, false);
  return domain_digest(determinism_observation_tag, encoder.bytes());
}

digest resource_observation_digest(
    const qualification_resource_matrix_observation &value) {
  canonical_encoder encoder;
  encode_resource_observation(encoder, value, false);
  return domain_digest(resource_observation_tag, encoder.bytes());
}

digest fuzz_observation_digest(
    const qualification_fuzz_matrix_observation &value) {
  canonical_encoder encoder;
  encode_fuzz_observation(encoder, value, false);
  return domain_digest(fuzz_observation_tag, encoder.bytes());
}

bool toolchain_observation_passes(
    const qualification_toolchain_matrix_case &descriptor,
    const qualification_toolchain_matrix_observation &value) noexcept {
  if (value.schema != qualification_matrix_schema_version ||
      value.case_identifier != descriptor.identifier ||
      value.case_digest != descriptor.case_digest ||
      value.observation_digest != toolchain_observation_digest(value))
    return false;
  if (value.skipped) {
    return descriptor.documented_skip_permitted && !value.executed &&
           text(value.skip_reason) && !zero(value.skip_evidence_digest) &&
           !value.tests_passed && !value.sanitizer_clean &&
           !value.strict_floating_point_verified &&
           !value.debug_library_verified && !value.concurrency_suite_passed &&
           zero(value.environment_digest) && zero(value.build_log_digest) &&
           zero(value.test_log_digest) && zero(value.canonical_result_digest) &&
           value.compiler_version.empty() &&
           value.standard_library_version.empty() &&
           value.operating_system.empty() && value.target_triple.empty() &&
           value.compile_flags.empty();
  }
  if (!value.executed || !value.skip_reason.empty() ||
      !zero(value.skip_evidence_digest) || !text(value.compiler_version) ||
      !text(value.standard_library_version) ||
      !text(value.operating_system) || !text(value.target_triple) ||
      !valid_string_vector(value.compile_flags) ||
      zero(value.environment_digest) || zero(value.build_log_digest) ||
      zero(value.test_log_digest) || zero(value.canonical_result_digest) ||
      !value.tests_passed || !value.strict_floating_point_verified)
    return false;
  const auto sanitizer_required =
      descriptor.sanitizer != qualification_sanitizer_mode::none;
  if (value.sanitizer_clean != sanitizer_required)
    return false;
  const auto debug_required =
      descriptor.debug_library != qualification_debug_library_mode::none;
  if (value.debug_library_verified != debug_required)
    return false;
  return value.concurrency_suite_passed ==
         descriptor.concurrency_suite_required;
}

bool determinism_observation_passes(
    const qualification_determinism_matrix_case &descriptor,
    const qualification_determinism_matrix_observation &value) noexcept {
  if (value.schema != qualification_matrix_schema_version ||
      value.case_identifier != descriptor.identifier ||
      value.case_digest != descriptor.case_digest || !value.completed ||
      value.completed_runs == 0 || zero(value.replay_log_digest) ||
      value.observation_digest != determinism_observation_digest(value))
    return false;
  if (zero(value.canonical_exact_artifact_digest) &&
      zero(value.canonical_result_bytes_digest) &&
      zero(value.canonical_failure_digest))
    return false;
  if (zero(value.canonical_diagnostics_digest) ||
      zero(value.canonical_certificate_digest))
    return false;
  const auto rounding =
      descriptor.axis == qualification_determinism_axis::ambient_rounding_mode;
  if (value.rounding_mode_controlled != rounding)
    return false;
  return value.separate_process == descriptor.separate_process;
}

bool resource_observation_passes(
    const qualification_resource_matrix_case &descriptor,
    const qualification_resource_matrix_observation &value) noexcept {
  if (value.schema != qualification_matrix_schema_version ||
      value.case_identifier != descriptor.identifier ||
      value.case_digest != descriptor.case_digest ||
      value.observed_error_key != descriptor.expected_error_key ||
      zero(value.publication_state_before_digest) ||
      value.publication_state_before_digest !=
          value.publication_state_after_digest ||
      !value.limit_triggered || !value.typed_failure_observed ||
      !value.transaction_rolled_back || value.partial_publication_observed ||
      value.observation_digest != resource_observation_digest(value))
    return false;
  if (descriptor.replay_required != value.replay_passed ||
      (descriptor.replay_required && zero(value.replay_digest)))
    return false;
  if (descriptor.kind == qualification_resource_case_kind::cancellation)
    return value.observed_cancellation_latency_milliseconds <=
           descriptor.cancellation_latency_limit_milliseconds;
  return value.observed_cancellation_latency_milliseconds == 0;
}

bool fuzz_observation_passes(
    const qualification_fuzz_matrix_case &descriptor,
    const qualification_fuzz_matrix_observation &value) noexcept {
  if (value.schema != qualification_matrix_schema_version ||
      value.case_identifier != descriptor.identifier ||
      value.case_digest != descriptor.case_digest || !text(value.engine) ||
      !text(value.engine_version) ||
      value.aggregate_cpu_seconds < descriptor.minimum_aggregate_cpu_seconds ||
      value.wall_seconds == 0 ||
      value.worker_count < descriptor.minimum_worker_count ||
      value.serialized_outcome_count != value.unique_outcome_count ||
      value.minimized_outcome_count != value.unique_outcome_count ||
      value.promoted_regression_count != value.unique_outcome_count ||
      value.unresolved_outcome_count != 0 || value.false_success_count != 0 ||
      value.nondeterministic_outcome_count != 0 ||
      value.infrastructure_failure_count != 0 || zero(value.seed_set_digest) ||
      zero(value.dictionary_digest) || zero(value.mutator_digest) ||
      zero(value.corpus_digest) || zero(value.failure_index_digest) ||
      zero(value.replay_digest) || !value.complete ||
      value.observation_digest != fuzz_observation_digest(value))
    return false;
  return true;
}

qualification_toolchain_matrix_case toolchain_case(
    std::string identifier, qualification_compiler_family compiler,
    qualification_compiler_release release,
    qualification_standard_library standard_library,
    qualification_architecture architecture, qualification_build_mode build,
    qualification_sanitizer_mode sanitizer,
    qualification_debug_library_mode debug_library, coordinate_tag coordinate,
    index_tag index, bool concurrency = false) {
  qualification_toolchain_matrix_case value;
  value.identifier = std::move(identifier);
  value.compiler = compiler;
  value.release = release;
  value.standard_library = standard_library;
  value.architecture = architecture;
  value.build = build;
  value.sanitizer = sanitizer;
  value.debug_library = debug_library;
  value.type.coordinate = coordinate;
  value.type.index = index;
  value.concurrency_suite_required = concurrency;
  return value;
}

qualification_determinism_matrix_case determinism_case(
    std::string identifier, qualification_determinism_axis axis,
    std::string variant, std::uint64_t numeric_parameter = 0,
    bool separate_process = false) {
  qualification_determinism_matrix_case value;
  value.identifier = std::move(identifier);
  value.axis = axis;
  value.variant = std::move(variant);
  value.equivalence_group = "p6.8-canonical-regression-subset-v1";
  value.numeric_parameter = numeric_parameter;
  value.separate_process = separate_process;
  return value;
}

qualification_resource_matrix_case resource_case(
    std::string identifier, qualification_resource_case_kind kind,
    std::uint64_t limit, std::string error_key,
    std::uint64_t cancellation_latency = 0) {
  qualification_resource_matrix_case value;
  value.identifier = std::move(identifier);
  value.kind = kind;
  value.declared_limit = limit;
  value.expected_error_key = std::move(error_key);
  value.cancellation_latency_limit_milliseconds = cancellation_latency;
  return value;
}

qualification_fuzz_matrix_case fuzz_case(
    std::string identifier, qualification_fuzz_family family,
    std::string configuration, qualification_sanitizer_mode sanitizer,
    std::uint32_t workers) {
  qualification_fuzz_matrix_case value;
  value.identifier = std::move(identifier);
  value.family = family;
  value.configuration_identifier = std::move(configuration);
  value.sanitizer = sanitizer;
  value.minimum_worker_count = workers;
  return value;
}

template <class V> bool unique_identifiers(const std::vector<V> &values) {
  for (std::size_t i = 1; i != values.size(); ++i)
    if (values[i - 1].identifier == values[i].identifier)
      return false;
  return true;
}

bool complete_toolchain_coverage(
    const std::vector<qualification_toolchain_matrix_case> &cases) {
  std::set<std::pair<qualification_compiler_family,
                     qualification_compiler_release>> compiler_releases;
  std::set<qualification_standard_library> standard_libraries;
  std::set<qualification_architecture> architectures;
  std::set<qualification_build_mode> builds;
  std::set<qualification_sanitizer_mode> sanitizers;
  std::set<qualification_debug_library_mode> debug_libraries;
  std::set<qualification_floating_point_mode> floating_point_modes;
  std::set<coordinate_tag> coordinates;
  std::set<index_tag> indices;
  bool tsan_concurrency = false;
  for (const auto &entry : cases) {
    compiler_releases.emplace(entry.compiler, entry.release);
    standard_libraries.insert(entry.standard_library);
    architectures.insert(entry.architecture);
    builds.insert(entry.build);
    sanitizers.insert(entry.sanitizer);
    debug_libraries.insert(entry.debug_library);
    floating_point_modes.insert(entry.floating_point_mode);
    coordinates.insert(entry.type.coordinate);
    indices.insert(entry.type.index);
    tsan_concurrency =
        tsan_concurrency ||
        (entry.sanitizer == qualification_sanitizer_mode::thread &&
         entry.concurrency_suite_required);
  }
  for (const auto compiler : {qualification_compiler_family::gcc,
                              qualification_compiler_family::clang})
    for (const auto release : {qualification_compiler_release::current,
                               qualification_compiler_release::oldest_supported})
      if (compiler_releases.count({compiler, release}) == 0)
        return false;
  const auto independent_architecture = std::any_of(
      architectures.begin(), architectures.end(), [](const auto architecture) {
        return architecture != qualification_architecture::x86_64;
      });
  return standard_libraries.count(qualification_standard_library::libstdcxx) &&
         standard_libraries.count(qualification_standard_library::libcxx) &&
         architectures.count(qualification_architecture::x86_64) &&
         independent_architecture &&
         builds.count(qualification_build_mode::debug) &&
         builds.count(qualification_build_mode::optimized) &&
         sanitizers.count(qualification_sanitizer_mode::address_and_undefined) &&
         sanitizers.count(qualification_sanitizer_mode::thread) &&
         debug_libraries.count(
             qualification_debug_library_mode::libstdcxx_debug) &&
         debug_libraries.count(
             qualification_debug_library_mode::libcxx_debug) &&
         floating_point_modes.size() == 2 && coordinates.size() == 2 &&
         indices.size() == 2 && tsan_concurrency;
}

bool complete_determinism_coverage(
    const std::vector<qualification_determinism_matrix_case> &cases) {
  constexpr auto axis_count =
      static_cast<std::size_t>(qualification_determinism_axis::count);
  std::array<std::uint64_t, axis_count> counts{};
  bool controlled_process = false;
  for (const auto &entry : cases) {
    ++counts[static_cast<std::size_t>(entry.axis)];
    controlled_process =
        controlled_process ||
        (entry.axis == qualification_determinism_axis::separate_process_replay &&
         entry.separate_process);
  }
  return controlled_process &&
         std::all_of(counts.begin(), counts.end(),
                     [](const auto count) { return count >= 2; });
}

bool complete_resource_coverage(
    const std::vector<qualification_resource_matrix_case> &cases) {
  std::set<qualification_resource_case_kind> kinds;
  for (const auto &entry : cases)
    kinds.insert(entry.kind);
  return kinds.size() ==
         static_cast<std::size_t>(qualification_resource_case_kind::count);
}

bool complete_fuzz_coverage(
    const std::vector<qualification_fuzz_matrix_case> &cases) {
  std::map<std::pair<std::string, qualification_sanitizer_mode>,
           std::set<qualification_fuzz_family>> sanitized;
  bool chain = false, long_running = false;
  for (const auto &entry : cases) {
    if (entry.family == qualification_fuzz_family::valid_geometry ||
        entry.family == qualification_fuzz_family::invalid_preparation) {
      sanitized[{entry.configuration_identifier, entry.sanitizer}].insert(
          entry.family);
    } else if (entry.family == qualification_fuzz_family::operation_chain) {
      chain = true;
    } else if (entry.family ==
               qualification_fuzz_family::long_running_unsanitized) {
      long_running = true;
    }
  }
  if (!chain || !long_running || sanitized.empty())
    return false;
  bool asan = false, tsan = false;
  for (const auto &entry : sanitized) {
    if (entry.first.second == qualification_sanitizer_mode::none ||
        entry.second.size() != 2)
      return false;
    asan = asan || entry.first.second ==
                       qualification_sanitizer_mode::address_and_undefined;
    tsan = tsan ||
           entry.first.second == qualification_sanitizer_mode::thread;
  }
  return asan && tsan;
}

using determinism_signature =
    std::tuple<digest, digest, digest, digest, digest>;

determinism_signature signature(
    const qualification_determinism_matrix_observation &value) {
  return std::make_tuple(value.canonical_exact_artifact_digest,
                         value.canonical_result_bytes_digest,
                         value.canonical_failure_digest,
                         value.canonical_diagnostics_digest,
                         value.canonical_certificate_digest);
}

} // namespace

const char *qualification_compiler_family_token(
    qualification_compiler_family value) noexcept {
  switch (value) {
  case qualification_compiler_family::gcc:
    return "gcc";
  case qualification_compiler_family::clang:
    return "clang";
  case qualification_compiler_family::count:
    break;
  }
  return "unknown";
}

const char *qualification_sanitizer_mode_token(
    qualification_sanitizer_mode value) noexcept {
  switch (value) {
  case qualification_sanitizer_mode::none:
    return "none";
  case qualification_sanitizer_mode::address_and_undefined:
    return "asan-ubsan";
  case qualification_sanitizer_mode::thread:
    return "tsan";
  case qualification_sanitizer_mode::count:
    break;
  }
  return "unknown";
}

const char *qualification_determinism_axis_token(
    qualification_determinism_axis value) noexcept {
  switch (value) {
  case qualification_determinism_axis::worker_count:
    return "worker-count";
  case qualification_determinism_axis::task_partition:
    return "task-partition";
  case qualification_determinism_axis::queue_bound:
    return "queue-bound";
  case qualification_determinism_axis::broad_phase:
    return "broad-phase";
  case qualification_determinism_axis::predicate_filter:
    return "predicate-filter";
  case qualification_determinism_axis::allocation_perturbation:
    return "allocation-perturbation";
  case qualification_determinism_axis::hash_seed:
    return "hash-seed";
  case qualification_determinism_axis::ambient_rounding_mode:
    return "ambient-rounding-mode";
  case qualification_determinism_axis::separate_process_replay:
    return "separate-process-replay";
  case qualification_determinism_axis::count:
    break;
  }
  return "unknown";
}

const char *qualification_fuzz_family_token(
    qualification_fuzz_family value) noexcept {
  switch (value) {
  case qualification_fuzz_family::valid_geometry:
    return "valid-geometry";
  case qualification_fuzz_family::invalid_preparation:
    return "invalid-preparation";
  case qualification_fuzz_family::operation_chain:
    return "operation-chain";
  case qualification_fuzz_family::long_running_unsanitized:
    return "long-running-unsanitized";
  case qualification_fuzz_family::count:
    break;
  }
  return "unknown";
}

qualification_matrix_plan make_default_qualification_matrix_plan() {
  qualification_matrix_plan plan;
  plan.identifier = "mesh-boolean-qualification-matrix-p6.8-v1";

  plan.toolchain_cases = {
      toolchain_case("toolchain.gcc-current-debug-libstdcxx-x86-u32-v1",
                     qualification_compiler_family::gcc,
                     qualification_compiler_release::current,
                     qualification_standard_library::libstdcxx,
                     qualification_architecture::x86_64,
                     qualification_build_mode::debug,
                     qualification_sanitizer_mode::none,
                     qualification_debug_library_mode::none,
                     coordinate_tag::binary32, index_tag::uint32),
      toolchain_case("toolchain.gcc-current-optimized-libstdcxx-x86-u64-v1",
                     qualification_compiler_family::gcc,
                     qualification_compiler_release::current,
                     qualification_standard_library::libstdcxx,
                     qualification_architecture::x86_64,
                     qualification_build_mode::optimized,
                     qualification_sanitizer_mode::none,
                     qualification_debug_library_mode::none,
                     coordinate_tag::binary64, index_tag::uint64),
      toolchain_case("toolchain.gcc-oldest-debug-libstdcxx-x86-u64-v1",
                     qualification_compiler_family::gcc,
                     qualification_compiler_release::oldest_supported,
                     qualification_standard_library::libstdcxx,
                     qualification_architecture::x86_64,
                     qualification_build_mode::debug,
                     qualification_sanitizer_mode::none,
                     qualification_debug_library_mode::none,
                     coordinate_tag::binary64, index_tag::uint64),
      toolchain_case("toolchain.gcc-oldest-optimized-libstdcxx-x86-u32-v1",
                     qualification_compiler_family::gcc,
                     qualification_compiler_release::oldest_supported,
                     qualification_standard_library::libstdcxx,
                     qualification_architecture::x86_64,
                     qualification_build_mode::optimized,
                     qualification_sanitizer_mode::none,
                     qualification_debug_library_mode::none,
                     coordinate_tag::binary32, index_tag::uint32),
      toolchain_case("toolchain.clang-current-debug-libcxx-x86-u32-v1",
                     qualification_compiler_family::clang,
                     qualification_compiler_release::current,
                     qualification_standard_library::libcxx,
                     qualification_architecture::x86_64,
                     qualification_build_mode::debug,
                     qualification_sanitizer_mode::none,
                     qualification_debug_library_mode::none,
                     coordinate_tag::binary32, index_tag::uint32),
      toolchain_case("toolchain.clang-current-optimized-libstdcxx-x86-u64-v1",
                     qualification_compiler_family::clang,
                     qualification_compiler_release::current,
                     qualification_standard_library::libstdcxx,
                     qualification_architecture::x86_64,
                     qualification_build_mode::optimized,
                     qualification_sanitizer_mode::none,
                     qualification_debug_library_mode::none,
                     coordinate_tag::binary64, index_tag::uint64),
      toolchain_case("toolchain.clang-oldest-debug-libstdcxx-x86-u64-v1",
                     qualification_compiler_family::clang,
                     qualification_compiler_release::oldest_supported,
                     qualification_standard_library::libstdcxx,
                     qualification_architecture::x86_64,
                     qualification_build_mode::debug,
                     qualification_sanitizer_mode::none,
                     qualification_debug_library_mode::none,
                     coordinate_tag::binary64, index_tag::uint64),
      toolchain_case("toolchain.clang-oldest-optimized-libcxx-x86-u32-v1",
                     qualification_compiler_family::clang,
                     qualification_compiler_release::oldest_supported,
                     qualification_standard_library::libcxx,
                     qualification_architecture::x86_64,
                     qualification_build_mode::optimized,
                     qualification_sanitizer_mode::none,
                     qualification_debug_library_mode::none,
                     coordinate_tag::binary32, index_tag::uint32),
      toolchain_case("toolchain.gcc-current-debug-libstdcxx-aarch64-u64-v1",
                     qualification_compiler_family::gcc,
                     qualification_compiler_release::current,
                     qualification_standard_library::libstdcxx,
                     qualification_architecture::aarch64,
                     qualification_build_mode::debug,
                     qualification_sanitizer_mode::none,
                     qualification_debug_library_mode::none,
                     coordinate_tag::binary64, index_tag::uint64),
      toolchain_case("toolchain.clang-current-optimized-libcxx-aarch64-u32-v1",
                     qualification_compiler_family::clang,
                     qualification_compiler_release::current,
                     qualification_standard_library::libcxx,
                     qualification_architecture::aarch64,
                     qualification_build_mode::optimized,
                     qualification_sanitizer_mode::none,
                     qualification_debug_library_mode::none,
                     coordinate_tag::binary32, index_tag::uint32),
      toolchain_case("toolchain.gcc-current-asan-ubsan-debug-u64-v1",
                     qualification_compiler_family::gcc,
                     qualification_compiler_release::current,
                     qualification_standard_library::libstdcxx,
                     qualification_architecture::x86_64,
                     qualification_build_mode::debug,
                     qualification_sanitizer_mode::address_and_undefined,
                     qualification_debug_library_mode::none,
                     coordinate_tag::binary64, index_tag::uint64),
      toolchain_case("toolchain.clang-current-asan-ubsan-debug-u32-v1",
                     qualification_compiler_family::clang,
                     qualification_compiler_release::current,
                     qualification_standard_library::libcxx,
                     qualification_architecture::x86_64,
                     qualification_build_mode::debug,
                     qualification_sanitizer_mode::address_and_undefined,
                     qualification_debug_library_mode::none,
                     coordinate_tag::binary32, index_tag::uint32),
      toolchain_case("toolchain.clang-current-tsan-debug-u64-v1",
                     qualification_compiler_family::clang,
                     qualification_compiler_release::current,
                     qualification_standard_library::libstdcxx,
                     qualification_architecture::x86_64,
                     qualification_build_mode::debug,
                     qualification_sanitizer_mode::thread,
                     qualification_debug_library_mode::none,
                     coordinate_tag::binary64, index_tag::uint64, true),
      toolchain_case("toolchain.gcc-current-libstdcxx-debug-u32-v1",
                     qualification_compiler_family::gcc,
                     qualification_compiler_release::current,
                     qualification_standard_library::libstdcxx,
                     qualification_architecture::x86_64,
                     qualification_build_mode::debug,
                     qualification_sanitizer_mode::none,
                     qualification_debug_library_mode::libstdcxx_debug,
                     coordinate_tag::binary32, index_tag::uint32),
      toolchain_case("toolchain.clang-current-libcxx-debug-u64-v1",
                     qualification_compiler_family::clang,
                     qualification_compiler_release::current,
                     qualification_standard_library::libcxx,
                     qualification_architecture::x86_64,
                     qualification_build_mode::debug,
                     qualification_sanitizer_mode::none,
                     qualification_debug_library_mode::libcxx_debug,
                     coordinate_tag::binary64, index_tag::uint64)};
  for (auto &entry : plan.toolchain_cases) {
    if (entry.build == qualification_build_mode::optimized ||
        entry.sanitizer != qualification_sanitizer_mode::none)
      entry.floating_point_mode =
          qualification_floating_point_mode::strict_controlled_rounding_matrix;
  }

  plan.determinism_cases = {
      determinism_case("determinism.workers.1-v1",
                       qualification_determinism_axis::worker_count, "one", 1),
      determinism_case("determinism.workers.4-v1",
                       qualification_determinism_axis::worker_count, "four", 4),
      determinism_case("determinism.partition.contiguous-v1",
                       qualification_determinism_axis::task_partition,
                       "contiguous"),
      determinism_case("determinism.partition.interleaved-v1",
                       qualification_determinism_axis::task_partition,
                       "interleaved"),
      determinism_case("determinism.queue.1-v1",
                       qualification_determinism_axis::queue_bound, "one", 1),
      determinism_case("determinism.queue.64-v1",
                       qualification_determinism_axis::queue_bound, "sixty-four",
                       64),
      determinism_case("determinism.broad-phase.bvh-v1",
                       qualification_determinism_axis::broad_phase, "bvh"),
      determinism_case("determinism.broad-phase.sorted-sweep-v1",
                       qualification_determinism_axis::broad_phase,
                       "sorted-sweep"),
      determinism_case("determinism.filter.accept-v1",
                       qualification_determinism_axis::predicate_filter,
                       "accept"),
      determinism_case("determinism.filter.forced-fallback-v1",
                       qualification_determinism_axis::predicate_filter,
                       "forced-fallback"),
      determinism_case("determinism.filter.mixed-v1",
                       qualification_determinism_axis::predicate_filter,
                       "mixed"),
      determinism_case("determinism.allocation.seed-0-v1",
                       qualification_determinism_axis::allocation_perturbation,
                       "seed-0", 0),
      determinism_case("determinism.allocation.seed-1-v1",
                       qualification_determinism_axis::allocation_perturbation,
                       "seed-1", 1),
      determinism_case("determinism.hash.seed-0-v1",
                       qualification_determinism_axis::hash_seed, "seed-0", 0),
      determinism_case("determinism.hash.seed-max-v1",
                       qualification_determinism_axis::hash_seed, "seed-max",
                       ~std::uint64_t(0)),
      determinism_case("determinism.rounding.nearest-v1",
                       qualification_determinism_axis::ambient_rounding_mode,
                       "nearest"),
      determinism_case("determinism.rounding.downward-v1",
                       qualification_determinism_axis::ambient_rounding_mode,
                       "downward"),
      determinism_case("determinism.rounding.upward-v1",
                       qualification_determinism_axis::ambient_rounding_mode,
                       "upward"),
      determinism_case("determinism.rounding.toward-zero-v1",
                       qualification_determinism_axis::ambient_rounding_mode,
                       "toward-zero"),
      determinism_case("determinism.process.in-process-v1",
                       qualification_determinism_axis::separate_process_replay,
                       "in-process"),
      determinism_case("determinism.process.child-a-v1",
                       qualification_determinism_axis::separate_process_replay,
                       "child-a", 1, true),
      determinism_case("determinism.process.child-b-v1",
                       qualification_determinism_axis::separate_process_replay,
                       "child-b", 2, true)};

  plan.resource_cases = {
      resource_case("resource.authoritative-bytes-v1",
                    qualification_resource_case_kind::authoritative_bytes,
                    1024, "mesh_boolean.resource_limit.authoritative_bytes"),
      resource_case("resource.work-units-v1",
                    qualification_resource_case_kind::work_units, 1024,
                    "mesh_boolean.resource_limit.work_units"),
      resource_case("resource.wall-timeout-v1",
                    qualification_resource_case_kind::wall_timeout, 1000,
                    "mesh_boolean.resource_limit.wall_timeout"),
      resource_case("resource.cancellation-v1",
                    qualification_resource_case_kind::cancellation, 1,
                    "mesh_boolean.cancelled", 250)};

  plan.fuzz_cases = {
      fuzz_case("fuzz.gcc-asan-ubsan.valid-v1",
                qualification_fuzz_family::valid_geometry,
                "gcc-current-asan-ubsan-v1",
                qualification_sanitizer_mode::address_and_undefined, 1),
      fuzz_case("fuzz.gcc-asan-ubsan.invalid-v1",
                qualification_fuzz_family::invalid_preparation,
                "gcc-current-asan-ubsan-v1",
                qualification_sanitizer_mode::address_and_undefined, 1),
      fuzz_case("fuzz.clang-asan-ubsan.valid-v1",
                qualification_fuzz_family::valid_geometry,
                "clang-current-asan-ubsan-v1",
                qualification_sanitizer_mode::address_and_undefined, 1),
      fuzz_case("fuzz.clang-asan-ubsan.invalid-v1",
                qualification_fuzz_family::invalid_preparation,
                "clang-current-asan-ubsan-v1",
                qualification_sanitizer_mode::address_and_undefined, 1),
      fuzz_case("fuzz.clang-tsan.valid-v1",
                qualification_fuzz_family::valid_geometry,
                "clang-current-tsan-v1", qualification_sanitizer_mode::thread,
                1),
      fuzz_case("fuzz.clang-tsan.invalid-v1",
                qualification_fuzz_family::invalid_preparation,
                "clang-current-tsan-v1", qualification_sanitizer_mode::thread,
                1),
      fuzz_case("fuzz.operation-chain.unsanitized-v1",
                qualification_fuzz_family::operation_chain,
                "operation-chain-unsanitized-v1",
                qualification_sanitizer_mode::none, 1),
      fuzz_case("fuzz.long-running.unsanitized-v1",
                qualification_fuzz_family::long_running_unsanitized,
                "large-exact-growth-unsanitized-v1",
                qualification_sanitizer_mode::none, 1)};

  auto made = make_qualification_matrix_plan(std::move(plan));
  if (!made.has_value())
    throw std::logic_error("default qualification matrix plan rejected");
  return made.value();
}

product_status_or<qualification_matrix_plan>
make_qualification_matrix_plan(qualification_matrix_plan plan) {
  try {
    if (plan.schema != qualification_matrix_schema_version ||
        plan.checker_version != qualification_matrix_checker_version ||
        !text(plan.identifier) || plan.toolchain_cases.empty() ||
        plan.determinism_cases.empty() || plan.resource_cases.empty() ||
        plan.fuzz_cases.empty())
      return matrix_error("qualification_matrix.plan_malformed");

    for (auto &entry : plan.toolchain_cases) {
      auto made = canonicalize_toolchain_case(std::move(entry));
      if (!made.has_value())
        return made.error();
      entry = std::move(made.value());
    }
    for (auto &entry : plan.determinism_cases) {
      auto made = canonicalize_determinism_case(std::move(entry));
      if (!made.has_value())
        return made.error();
      entry = std::move(made.value());
    }
    for (auto &entry : plan.resource_cases) {
      auto made = canonicalize_resource_case(std::move(entry));
      if (!made.has_value())
        return made.error();
      entry = std::move(made.value());
    }
    for (auto &entry : plan.fuzz_cases) {
      auto made = canonicalize_fuzz_case(std::move(entry));
      if (!made.has_value())
        return made.error();
      entry = std::move(made.value());
    }

    const auto by_identifier = [](const auto &a, const auto &b) {
      return a.identifier < b.identifier;
    };
    std::sort(plan.toolchain_cases.begin(), plan.toolchain_cases.end(),
              by_identifier);
    std::sort(plan.determinism_cases.begin(), plan.determinism_cases.end(),
              by_identifier);
    std::sort(plan.resource_cases.begin(), plan.resource_cases.end(),
              by_identifier);
    std::sort(plan.fuzz_cases.begin(), plan.fuzz_cases.end(), by_identifier);
    if (!unique_identifiers(plan.toolchain_cases) ||
        !unique_identifiers(plan.determinism_cases) ||
        !unique_identifiers(plan.resource_cases) ||
        !unique_identifiers(plan.fuzz_cases))
      return matrix_error("qualification_matrix.duplicate_case_identifier");
    if (!complete_toolchain_coverage(plan.toolchain_cases))
      return matrix_error("qualification_matrix.toolchain_coverage_incomplete");
    if (!complete_determinism_coverage(plan.determinism_cases))
      return matrix_error("qualification_matrix.determinism_coverage_incomplete");
    if (!complete_resource_coverage(plan.resource_cases))
      return matrix_error("qualification_matrix.resource_coverage_incomplete");
    if (!complete_fuzz_coverage(plan.fuzz_cases))
      return matrix_error("qualification_matrix.fuzz_coverage_incomplete");

    canonical_encoder encoder;
    encoder.u16(plan.schema);
    encoder.u32(plan.checker_version);
    encoder.string(plan.identifier);
    encoder.u64(plan.toolchain_cases.size());
    for (const auto &entry : plan.toolchain_cases)
      encode_toolchain_case(encoder, entry, true);
    encoder.u64(plan.determinism_cases.size());
    for (const auto &entry : plan.determinism_cases)
      encode_determinism_case(encoder, entry, true);
    encoder.u64(plan.resource_cases.size());
    for (const auto &entry : plan.resource_cases)
      encode_resource_case(encoder, entry, true);
    encoder.u64(plan.fuzz_cases.size());
    for (const auto &entry : plan.fuzz_cases)
      encode_fuzz_case(encoder, entry, true);
    const auto bytes = encoder.bytes();
    const auto calculated = domain_digest(plan_tag, bytes);
    if ((!plan.canonical_bytes.empty() && plan.canonical_bytes != bytes) ||
        (!zero(plan.plan_digest) && plan.plan_digest != calculated))
      return matrix_error("qualification_matrix.plan_binding_mismatch");
    plan.canonical_bytes = bytes;
    plan.plan_digest = calculated;
    return plan;
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "qualification_matrix.plan_allocation");
  }
}

product_status_or<bool>
validate_qualification_matrix_plan(const qualification_matrix_plan &plan) noexcept {
  try {
    auto copy = plan;
    copy.canonical_bytes.clear();
    copy.plan_digest = {};
    auto made = make_qualification_matrix_plan(std::move(copy));
    if (!made.has_value() || made.value().canonical_bytes != plan.canonical_bytes ||
        made.value().plan_digest != plan.plan_digest)
      return matrix_error("qualification_matrix.plan_validation_failed");
    return true;
  } catch (...) {
    return matrix_error("qualification_matrix.plan_validation_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_matrix_plan(const qualification_matrix_plan &plan) {
  auto valid = validate_qualification_matrix_plan(plan);
  if (!valid.has_value())
    return valid.error();
  return plan.canonical_bytes;
}

product_status_or<qualification_toolchain_matrix_observation>
make_qualification_toolchain_matrix_observation(
    const qualification_toolchain_matrix_case &descriptor,
    qualification_toolchain_matrix_observation value) {
  try {
    auto made_descriptor = canonicalize_toolchain_case(descriptor);
    if (!made_descriptor.has_value())
      return made_descriptor.error();
    const auto &entry = made_descriptor.value();
    if (value.schema != qualification_matrix_schema_version ||
        (!value.case_identifier.empty() &&
         value.case_identifier != entry.identifier) ||
        (!zero(value.case_digest) && value.case_digest != entry.case_digest))
      return matrix_error("qualification_matrix.toolchain_observation_binding");
    value.case_identifier = entry.identifier;
    value.case_digest = entry.case_digest;
    const auto supplied = value.observation_digest;
    value.observation_digest = {};
    const auto calculated = toolchain_observation_digest(value);
    if (!zero(supplied) && supplied != calculated)
      return matrix_error("qualification_matrix.toolchain_observation_digest");
    value.observation_digest = calculated;
    if (!toolchain_observation_passes(entry, value))
      return matrix_error("qualification_matrix.toolchain_observation_failed");
    return value;
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "qualification_matrix.toolchain_observation_allocation");
  }
}

product_status_or<qualification_determinism_matrix_observation>
make_qualification_determinism_matrix_observation(
    const qualification_determinism_matrix_case &descriptor,
    qualification_determinism_matrix_observation value) {
  try {
    auto made_descriptor = canonicalize_determinism_case(descriptor);
    if (!made_descriptor.has_value())
      return made_descriptor.error();
    const auto &entry = made_descriptor.value();
    if (value.schema != qualification_matrix_schema_version ||
        (!value.case_identifier.empty() &&
         value.case_identifier != entry.identifier) ||
        (!zero(value.case_digest) && value.case_digest != entry.case_digest))
      return matrix_error("qualification_matrix.determinism_observation_binding");
    value.case_identifier = entry.identifier;
    value.case_digest = entry.case_digest;
    const auto supplied = value.observation_digest;
    value.observation_digest = {};
    const auto calculated = determinism_observation_digest(value);
    if (!zero(supplied) && supplied != calculated)
      return matrix_error("qualification_matrix.determinism_observation_digest");
    value.observation_digest = calculated;
    if (!determinism_observation_passes(entry, value))
      return matrix_error("qualification_matrix.determinism_observation_failed");
    return value;
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "qualification_matrix.determinism_observation_allocation");
  }
}

product_status_or<qualification_resource_matrix_observation>
make_qualification_resource_matrix_observation(
    const qualification_resource_matrix_case &descriptor,
    qualification_resource_matrix_observation value) {
  try {
    auto made_descriptor = canonicalize_resource_case(descriptor);
    if (!made_descriptor.has_value())
      return made_descriptor.error();
    const auto &entry = made_descriptor.value();
    if (value.schema != qualification_matrix_schema_version ||
        (!value.case_identifier.empty() &&
         value.case_identifier != entry.identifier) ||
        (!zero(value.case_digest) && value.case_digest != entry.case_digest))
      return matrix_error("qualification_matrix.resource_observation_binding");
    value.case_identifier = entry.identifier;
    value.case_digest = entry.case_digest;
    const auto supplied = value.observation_digest;
    value.observation_digest = {};
    const auto calculated = resource_observation_digest(value);
    if (!zero(supplied) && supplied != calculated)
      return matrix_error("qualification_matrix.resource_observation_digest");
    value.observation_digest = calculated;
    if (!resource_observation_passes(entry, value))
      return matrix_error("qualification_matrix.resource_observation_failed");
    return value;
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "qualification_matrix.resource_observation_allocation");
  }
}

product_status_or<qualification_fuzz_matrix_observation>
make_qualification_fuzz_matrix_observation(
    const qualification_fuzz_matrix_case &descriptor,
    qualification_fuzz_matrix_observation value) {
  try {
    auto made_descriptor = canonicalize_fuzz_case(descriptor);
    if (!made_descriptor.has_value())
      return made_descriptor.error();
    const auto &entry = made_descriptor.value();
    if (value.schema != qualification_matrix_schema_version ||
        (!value.case_identifier.empty() &&
         value.case_identifier != entry.identifier) ||
        (!zero(value.case_digest) && value.case_digest != entry.case_digest))
      return matrix_error("qualification_matrix.fuzz_observation_binding");
    value.case_identifier = entry.identifier;
    value.case_digest = entry.case_digest;
    const auto supplied = value.observation_digest;
    value.observation_digest = {};
    const auto calculated = fuzz_observation_digest(value);
    if (!zero(supplied) && supplied != calculated)
      return matrix_error("qualification_matrix.fuzz_observation_digest");
    value.observation_digest = calculated;
    if (!fuzz_observation_passes(entry, value))
      return matrix_error("qualification_matrix.fuzz_observation_failed");
    return value;
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "qualification_matrix.fuzz_observation_allocation");
  }
}

product_status_or<qualification_matrix_report>
make_qualification_matrix_report(
    const qualification_matrix_plan &plan,
    std::vector<qualification_toolchain_matrix_observation> toolchains,
    std::vector<qualification_determinism_matrix_observation> determinism,
    std::vector<qualification_resource_matrix_observation> resources,
    std::vector<qualification_fuzz_matrix_observation> fuzz, bool complete) {
  try {
    auto plan_valid = validate_qualification_matrix_plan(plan);
    if (!plan_valid.has_value())
      return plan_valid.error();
    qualification_matrix_report report;
    report.identifier = plan.identifier + ".report";
    report.plan_digest = plan.plan_digest;

    auto toolchain_map =
        std::map<std::string, qualification_toolchain_matrix_observation>{};
    for (auto &value : toolchains)
      if (!toolchain_map.emplace(value.case_identifier, std::move(value)).second)
        return matrix_error("qualification_matrix.duplicate_toolchain_observation");
    for (const auto &descriptor : plan.toolchain_cases) {
      const auto found = toolchain_map.find(descriptor.identifier);
      if (found == toolchain_map.end()) {
        ++report.blocking_issue_count;
        continue;
      }
      auto made = make_qualification_toolchain_matrix_observation(
          descriptor, std::move(found->second));
      if (!made.has_value()) {
        ++report.blocking_issue_count;
      } else {
        ++report.passed_toolchain_cases;
        report.toolchain_observations.push_back(std::move(made.value()));
      }
      toolchain_map.erase(found);
    }
    report.blocking_issue_count += toolchain_map.size();

    auto determinism_map =
        std::map<std::string, qualification_determinism_matrix_observation>{};
    for (auto &value : determinism)
      if (!determinism_map.emplace(value.case_identifier, std::move(value)).second)
        return matrix_error("qualification_matrix.duplicate_determinism_observation");
    std::map<std::string, determinism_signature> signatures;
    for (const auto &descriptor : plan.determinism_cases) {
      const auto found = determinism_map.find(descriptor.identifier);
      if (found == determinism_map.end()) {
        ++report.blocking_issue_count;
        continue;
      }
      auto made = make_qualification_determinism_matrix_observation(
          descriptor, std::move(found->second));
      if (!made.has_value()) {
        ++report.blocking_issue_count;
      } else {
        const auto inserted = signatures.emplace(descriptor.equivalence_group,
                                                 signature(made.value()));
        if (!inserted.second && inserted.first->second != signature(made.value()))
          ++report.blocking_issue_count;
        ++report.passed_determinism_cases;
        report.determinism_observations.push_back(std::move(made.value()));
      }
      determinism_map.erase(found);
    }
    report.blocking_issue_count += determinism_map.size();

    auto resource_map =
        std::map<std::string, qualification_resource_matrix_observation>{};
    for (auto &value : resources)
      if (!resource_map.emplace(value.case_identifier, std::move(value)).second)
        return matrix_error("qualification_matrix.duplicate_resource_observation");
    for (const auto &descriptor : plan.resource_cases) {
      const auto found = resource_map.find(descriptor.identifier);
      if (found == resource_map.end()) {
        ++report.blocking_issue_count;
        continue;
      }
      auto made = make_qualification_resource_matrix_observation(
          descriptor, std::move(found->second));
      if (!made.has_value()) {
        ++report.blocking_issue_count;
      } else {
        ++report.passed_resource_cases;
        report.resource_observations.push_back(std::move(made.value()));
      }
      resource_map.erase(found);
    }
    report.blocking_issue_count += resource_map.size();

    auto fuzz_map =
        std::map<std::string, qualification_fuzz_matrix_observation>{};
    for (auto &value : fuzz)
      if (!fuzz_map.emplace(value.case_identifier, std::move(value)).second)
        return matrix_error("qualification_matrix.duplicate_fuzz_observation");
    for (const auto &descriptor : plan.fuzz_cases) {
      const auto found = fuzz_map.find(descriptor.identifier);
      if (found == fuzz_map.end()) {
        ++report.blocking_issue_count;
        continue;
      }
      auto made = make_qualification_fuzz_matrix_observation(
          descriptor, std::move(found->second));
      if (!made.has_value()) {
        ++report.blocking_issue_count;
      } else {
        ++report.passed_fuzz_cases;
        report.fuzz_observations.push_back(std::move(made.value()));
      }
      fuzz_map.erase(found);
    }
    report.blocking_issue_count += fuzz_map.size();

    const auto by_case = [](const auto &a, const auto &b) {
      return a.case_identifier < b.case_identifier;
    };
    std::sort(report.toolchain_observations.begin(),
              report.toolchain_observations.end(), by_case);
    std::sort(report.determinism_observations.begin(),
              report.determinism_observations.end(), by_case);
    std::sort(report.resource_observations.begin(),
              report.resource_observations.end(), by_case);
    std::sort(report.fuzz_observations.begin(), report.fuzz_observations.end(),
              by_case);
    report.complete =
        complete && report.blocking_issue_count == 0 &&
        report.passed_toolchain_cases == plan.toolchain_cases.size() &&
        report.passed_determinism_cases == plan.determinism_cases.size() &&
        report.passed_resource_cases == plan.resource_cases.size() &&
        report.passed_fuzz_cases == plan.fuzz_cases.size();

    canonical_encoder encoder;
    encoder.u16(report.schema);
    encoder.u32(report.checker_version);
    encoder.string(report.identifier);
    encode_digest(encoder, report.plan_digest);
    encoder.u64(report.toolchain_observations.size());
    for (const auto &value : report.toolchain_observations)
      encode_toolchain_observation(encoder, value, true);
    encoder.u64(report.determinism_observations.size());
    for (const auto &value : report.determinism_observations)
      encode_determinism_observation(encoder, value, true);
    encoder.u64(report.resource_observations.size());
    for (const auto &value : report.resource_observations)
      encode_resource_observation(encoder, value, true);
    encoder.u64(report.fuzz_observations.size());
    for (const auto &value : report.fuzz_observations)
      encode_fuzz_observation(encoder, value, true);
    encoder.u64(report.passed_toolchain_cases);
    encoder.u64(report.passed_determinism_cases);
    encoder.u64(report.passed_resource_cases);
    encoder.u64(report.passed_fuzz_cases);
    encoder.u64(report.blocking_issue_count);
    encoder.boolean(report.complete);
    report.canonical_bytes = encoder.bytes();
    report.report_digest = domain_digest(report_tag, report.canonical_bytes);
    return report;
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "qualification_matrix.report_allocation");
  }
}

product_status_or<bool> validate_qualification_matrix_report(
    const qualification_matrix_report &report,
    const qualification_matrix_plan &plan) noexcept {
  try {
    if (report.schema != qualification_matrix_schema_version ||
        report.checker_version != qualification_matrix_checker_version ||
        !text(report.identifier) || report.plan_digest != plan.plan_digest)
      return matrix_error("qualification_matrix.report_malformed");
    auto rebuilt = make_qualification_matrix_report(
        plan, report.toolchain_observations, report.determinism_observations,
        report.resource_observations, report.fuzz_observations, report.complete);
    if (!rebuilt.has_value() ||
        rebuilt.value().passed_toolchain_cases !=
            report.passed_toolchain_cases ||
        rebuilt.value().passed_determinism_cases !=
            report.passed_determinism_cases ||
        rebuilt.value().passed_resource_cases != report.passed_resource_cases ||
        rebuilt.value().passed_fuzz_cases != report.passed_fuzz_cases ||
        rebuilt.value().blocking_issue_count != report.blocking_issue_count ||
        rebuilt.value().complete != report.complete ||
        rebuilt.value().canonical_bytes != report.canonical_bytes ||
        rebuilt.value().report_digest != report.report_digest)
      return matrix_error("qualification_matrix.report_validation_failed");
    return true;
  } catch (...) {
    return matrix_error("qualification_matrix.report_validation_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_matrix_report(const qualification_matrix_report &report) {
  if (report.schema != qualification_matrix_schema_version ||
      report.checker_version != qualification_matrix_checker_version ||
      report.canonical_bytes.empty() || zero(report.report_digest) ||
      report.report_digest != domain_digest(report_tag, report.canonical_bytes))
    return matrix_error("qualification_matrix.report_unbound");
  return report.canonical_bytes;
}

bool qualification_matrix_gate_passes(
    const qualification_matrix_report &report,
    const qualification_matrix_plan &plan) noexcept {
  const auto valid = validate_qualification_matrix_report(report, plan);
  return valid.has_value() && report.complete &&
         report.blocking_issue_count == 0 &&
         report.passed_toolchain_cases == plan.toolchain_cases.size() &&
         report.passed_determinism_cases == plan.determinism_cases.size() &&
         report.passed_resource_cases == plan.resource_cases.size() &&
         report.passed_fuzz_cases == plan.fuzz_cases.size();
}

} // namespace mesh_boolean
} // namespace ygor
