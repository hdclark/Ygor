#include "MeshBooleanTestHarness.h"

#include <YgorMeshesBooleanQualificationPerformance.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

namespace {

digest tagged_digest(const std::string &label) {
  canonical_encoder encoder;
  encoder.string(label);
  return domain_digest({{'Y', 'G', 'B', 'Q', 'P', 'X', '0', '1'}},
                       encoder.bytes());
}

bool successful(qualification_outcome value) {
  return value == qualification_outcome::verified_exact_success ||
         value ==
             qualification_outcome::verified_certified_approximate_success;
}

const qualification_performance_case &find_case(
    const qualification_performance_plan &plan, const std::string &identifier) {
  const auto found = std::find_if(
      plan.cases.begin(), plan.cases.end(), [&](const auto &entry) {
        return entry.identifier == identifier;
      });
  require(found != plan.cases.end(), "performance case exists");
  return *found;
}

qualification_performance_threshold &find_threshold(
    qualification_performance_plan &plan, const std::string &identifier) {
  const auto found = std::find_if(
      plan.thresholds.begin(), plan.thresholds.end(), [&](const auto &entry) {
        return entry.identifier == identifier;
      });
  require(found != plan.thresholds.end(), "performance threshold exists");
  return *found;
}

qualification_performance_sample sample_for(
    const qualification_performance_case &descriptor, std::uint32_t ordinal,
    std::uint64_t wall_nanoseconds, std::uint64_t authoritative_bytes,
    std::uint64_t cancellation_latency, const std::string &run_kind) {
  qualification_performance_sample sample;
  sample.ordinal = ordinal;
  sample.metric_presence_mask = qualification_performance_all_metrics_mask;
  sample.metrics[static_cast<std::size_t>(
      qualification_performance_metric::wall_time_nanoseconds)] =
      wall_nanoseconds;
  sample.metrics[static_cast<std::size_t>(
      qualification_performance_metric::cpu_time_nanoseconds)] =
      wall_nanoseconds / 2U + 1U;
  sample.metrics[static_cast<std::size_t>(
      qualification_performance_metric::peak_rss_bytes)] = 8192;
  sample.metrics[static_cast<std::size_t>(
      qualification_performance_metric::authoritative_bytes)] =
      authoritative_bytes;
  sample.metrics[static_cast<std::size_t>(
      qualification_performance_metric::peak_exact_number_bits)] = 384;
  sample.metrics[static_cast<std::size_t>(
      qualification_performance_metric::peak_exact_number_limbs)] = 6;
  sample.metrics[static_cast<std::size_t>(
      qualification_performance_metric::candidate_count)] = 23;
  sample.metrics[static_cast<std::size_t>(
      qualification_performance_metric::event_count)] = 31;
  sample.metrics[static_cast<std::size_t>(
      qualification_performance_metric::patch_count)] = 41;
  sample.metrics[static_cast<std::size_t>(
      qualification_performance_metric::verifier_time_nanoseconds)] =
      wall_nanoseconds / 3U + 1U;
  sample.metrics[static_cast<std::size_t>(
      qualification_performance_metric::realization_search_nodes)] = 53;
  sample.metrics[static_cast<std::size_t>(
      qualification_performance_metric::realization_components)] = 3;
  sample.metrics[static_cast<std::size_t>(
      qualification_performance_metric::serialization_bytes)] = 2048;
