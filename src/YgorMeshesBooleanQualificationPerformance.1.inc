#include "YgorMeshesBooleanQualificationPerformance.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <new>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ygor {
namespace mesh_boolean {
namespace {

constexpr std::array<char, 8> protocol_tag{{'Y', 'G', 'B', 'Q', 'P', 'Y', '0', '1'}};
constexpr std::array<char, 8> case_tag{{'Y', 'G', 'B', 'Q', 'P', 'C', '0', '1'}};
constexpr std::array<char, 8> threshold_tag{{'Y', 'G', 'B', 'Q', 'P', 'T', '0', '1'}};
constexpr std::array<char, 8> review_tag{{'Y', 'G', 'B', 'Q', 'P', 'V', '0', '1'}};
constexpr std::array<char, 8> plan_tag{{'Y', 'G', 'B', 'Q', 'P', 'P', '0', '1'}};
constexpr std::array<char, 8> recipe_tag{{'Y', 'G', 'B', 'Q', 'P', 'K', '0', '1'}};
constexpr std::array<char, 8> sample_tag{{'Y', 'G', 'B', 'Q', 'P', 'S', '0', '1'}};
constexpr std::array<char, 8> observation_tag{{'Y', 'G', 'B', 'Q', 'P', 'O', '0', '1'}};
constexpr std::array<char, 8> statistics_tag{{'Y', 'G', 'B', 'Q', 'P', 'X', '0', '1'}};
constexpr std::array<char, 8> report_tag{{'Y', 'G', 'B', 'Q', 'P', 'R', '0', '1'}};

constexpr const char *geomean_threshold_id =
    "plan_speed.p13.b1_b7_geometric_mean_speedup";
constexpr const char *b6_threshold_id = "plan_speed.p13.b6_speedup";
constexpr const char *per_case_threshold_id =
    "plan_speed.p13.per_case_median_slowdown";
constexpr const char *bytes_threshold_id =
    "plan_speed.p13.authoritative_bytes_nonincrease";
constexpr const char *comparison_group_id = "plan-speed-b1-b7-v1";

product_error performance_error(const char *key) {
  return make_product_error(product_error_code::qualification_policy_violation,
                            key);
}

bool zero(const digest &value) noexcept { return value == digest{}; }

bool text(const std::string &value, bool allow_empty = false) noexcept {
  return (allow_empty || !value.empty()) && value.size() <= 4096U &&
         std::find(value.begin(), value.end(), '\0') == value.end();
}

template <class E> bool enum_before_count(E value, E count) noexcept {
  return static_cast<unsigned>(value) < static_cast<unsigned>(count);
}

bool known(qualification_performance_workload_kind value) noexcept {
  return enum_before_count(value, qualification_performance_workload_kind::count);
}
bool known(qualification_performance_scale value) noexcept {
  return enum_before_count(value, qualification_performance_scale::count);
}
bool known(qualification_performance_metric value) noexcept {
  return enum_before_count(value, qualification_performance_metric::count);
}
bool known(qualification_performance_threshold_kind value) noexcept {
  return enum_before_count(value, qualification_performance_threshold_kind::count);
}
bool known(qualification_performance_statistic value) noexcept {
  return enum_before_count(value, qualification_performance_statistic::count);
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
bool known(qualification_outcome value) noexcept {
  return value >= qualification_outcome::verified_exact_success &&
         value <= qualification_outcome::infrastructure_failure;
}

bool success_outcome(qualification_outcome value) noexcept {
  return value == qualification_outcome::verified_exact_success ||
         value ==
             qualification_outcome::verified_certified_approximate_success;
}

bool transaction_kind(qualification_performance_workload_kind value) noexcept {
  return value ==
             qualification_performance_workload_kind::resource_limit_transaction ||
         value ==
             qualification_performance_workload_kind::cancellation_transaction;
}

void encode_digest(canonical_encoder &encoder, const digest &value) {
  encoder.raw(value.bytes.data(), value.bytes.size());
}

template <std::size_t N>
void encode_u64_array(canonical_encoder &encoder,
                      const std::array<std::uint64_t, N> &values) {
  for (const auto value : values)
    encoder.u64(value);
}

void encode_protocol(canonical_encoder &encoder,
                     const qualification_performance_protocol_v1 &value,
                     bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.string(value.version);
  encoder.byte(static_cast<std::uint8_t>(value.statistic));
  encoder.u32(value.warmup_runs);
  encoder.u32(value.measured_runs);
  encoder.boolean(value.retain_raw_samples);
  encoder.boolean(value.monotonic_wall_clock_required);
  encoder.boolean(value.process_cpu_clock_required);
  encoder.boolean(value.controlled_exclusive_host_required);
  encoder.boolean(value.cpu_affinity_required);
  encoder.boolean(value.frequency_policy_recorded);
  encoder.boolean(value.allocator_policy_recorded);
  encoder.boolean(value.peak_rss_external_observer_required);
  encoder.boolean(value.authoritative_accounting_separate);
  if (include_digest)
    encode_digest(encoder, value.protocol_digest);
}

void encode_case(canonical_encoder &encoder,
                 const qualification_performance_case &value,
                 bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.byte(static_cast<std::uint8_t>(value.kind));
  encoder.byte(static_cast<std::uint8_t>(value.scale));
  encoder.string(value.fixture_identifier);
  encoder.string(value.geometry_category);
  encoder.string(value.normalization_policy_identifier);
  encoder.byte(static_cast<std::uint8_t>(value.selected_operation));
  encoder.byte(static_cast<std::uint8_t>(value.type.coordinate));
  encoder.byte(static_cast<std::uint8_t>(value.type.index));
  encoder.byte(static_cast<std::uint8_t>(value.expected_outcome));
  encoder.string(value.expected_error_key);
  encoder.boolean(value.baseline_comparison_required);
  encoder.string(value.baseline_family);
  encoder.string(value.comparison_group);
  encoder.u64(value.required_metrics_mask);
  encoder.u64(value.cancellation_latency_limit_milliseconds);
  encode_digest(encoder, value.workload_recipe_digest);
  if (include_digest)
    encode_digest(encoder, value.case_digest);
}

void encode_threshold(canonical_encoder &encoder,
                      const qualification_performance_threshold &value,
                      bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.byte(static_cast<std::uint8_t>(value.kind));
  encoder.byte(static_cast<std::uint8_t>(value.metric));
  encoder.string(value.scope);
  encoder.u64(value.numerator);
  encoder.u64(value.denominator);
  encoder.boolean(value.blocking);
  if (include_digest)
    encode_digest(encoder, value.threshold_digest);
}

void encode_review(canonical_encoder &encoder,
                   const qualification_performance_threshold_review &value,
                   bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.threshold_identifier);
  encode_digest(encoder, value.prior_threshold_digest);
  encode_digest(encoder, value.revised_threshold_digest);
