  for (const auto &descriptor : plan.cases) {
    if (!scope_matches(threshold, descriptor))
      continue;
    const auto found = statistics.find(descriptor.identifier);
    if (found == statistics.end() || !found->second.baseline_present)
      return false;
    matches.push_back({&descriptor, &found->second});
  }
  if (matches.empty())
    return false;

  if (threshold.kind ==
      qualification_performance_threshold_kind::geometric_mean_speedup) {
    natural_product left;
    natural_product right;
    for (const auto &entry : matches) {
      const auto &value = *entry.second;
      left.multiply(value.baseline_medians[metric]);
      left.multiply(threshold.denominator);
      right.multiply(value.candidate_medians[metric]);
      right.multiply(threshold.numerator);
    }
    return compare(left, right) >= 0;
  }

  for (const auto &entry : matches) {
    const auto &value = *entry.second;
    const auto baseline = value.baseline_medians[metric];
    const auto candidate = value.candidate_medians[metric];
    switch (threshold.kind) {
    case qualification_performance_threshold_kind::case_speedup:
      if (!ratio_greater_equal(baseline, threshold.denominator, candidate,
                               threshold.numerator))
        return false;
      break;
    case qualification_performance_threshold_kind::maximum_case_slowdown:
    case qualification_performance_threshold_kind::authoritative_bytes_nonincrease:
      if (!ratio_greater_equal(baseline, threshold.numerator, candidate,
                               threshold.denominator))
        return false;
      break;
    case qualification_performance_threshold_kind::absolute_upper_bound:
      if (!ratio_greater_equal(threshold.numerator, 1U, candidate,
                               threshold.denominator))
        return false;
      break;
    case qualification_performance_threshold_kind::geometric_mean_speedup:
    case qualification_performance_threshold_kind::count:
      return false;
    }
  }
  return true;
}

} // namespace

const char *qualification_performance_workload_kind_token(
    qualification_performance_workload_kind value) noexcept {
  switch (value) {
  case qualification_performance_workload_kind::strict_validation:
    return "strict_validation";
  case qualification_performance_workload_kind::normalization_diagnosis:
    return "normalization_diagnosis";
  case qualification_performance_workload_kind::normalization_repair:
    return "normalization_repair";
  case qualification_performance_workload_kind::backend_evaluation:
    return "backend_evaluation";
  case qualification_performance_workload_kind::topology_preflight:
    return "topology_preflight";
  case qualification_performance_workload_kind::exact_result_serialization:
    return "exact_result_serialization";
  case qualification_performance_workload_kind::exact_in_T_realization:
    return "exact_in_T_realization";
  case qualification_performance_workload_kind::certified_approximate_realization:
    return "certified_approximate_realization";
  case qualification_performance_workload_kind::mandatory_verification:
    return "mandatory_verification";
  case qualification_performance_workload_kind::exhaustive_verification:
    return "exhaustive_verification";
  case qualification_performance_workload_kind::operation_chain:
    return "operation_chain";
  case qualification_performance_workload_kind::resource_limit_transaction:
    return "resource_limit_transaction";
  case qualification_performance_workload_kind::cancellation_transaction:
    return "cancellation_transaction";
  case qualification_performance_workload_kind::count:
    break;
  }
  return "unknown";
}

const char *qualification_performance_metric_token(
    qualification_performance_metric value) noexcept {
  switch (value) {
  case qualification_performance_metric::wall_time_nanoseconds:
    return "wall_time_nanoseconds";
  case qualification_performance_metric::cpu_time_nanoseconds:
    return "cpu_time_nanoseconds";
  case qualification_performance_metric::peak_rss_bytes:
    return "peak_rss_bytes";
  case qualification_performance_metric::authoritative_bytes:
    return "authoritative_bytes";
  case qualification_performance_metric::peak_exact_number_bits:
    return "peak_exact_number_bits";
  case qualification_performance_metric::peak_exact_number_limbs:
    return "peak_exact_number_limbs";
  case qualification_performance_metric::candidate_count:
    return "candidate_count";
  case qualification_performance_metric::event_count:
    return "event_count";
  case qualification_performance_metric::patch_count:
    return "patch_count";
  case qualification_performance_metric::verifier_time_nanoseconds:
    return "verifier_time_nanoseconds";
  case qualification_performance_metric::realization_search_nodes:
    return "realization_search_nodes";
  case qualification_performance_metric::realization_components:
    return "realization_components";
  case qualification_performance_metric::serialization_bytes:
    return "serialization_bytes";
  case qualification_performance_metric::cancellation_latency_milliseconds:
    return "cancellation_latency_milliseconds";
  case qualification_performance_metric::count:
    break;
  }
  return "unknown";
}

const char *qualification_performance_threshold_kind_token(
    qualification_performance_threshold_kind value) noexcept {
  switch (value) {
  case qualification_performance_threshold_kind::geometric_mean_speedup:
    return "geometric_mean_speedup";
  case qualification_performance_threshold_kind::case_speedup:
    return "case_speedup";
  case qualification_performance_threshold_kind::maximum_case_slowdown:
    return "maximum_case_slowdown";
  case qualification_performance_threshold_kind::authoritative_bytes_nonincrease:
    return "authoritative_bytes_nonincrease";
  case qualification_performance_threshold_kind::absolute_upper_bound:
    return "absolute_upper_bound";
  case qualification_performance_threshold_kind::count:
    break;
  }
  return "unknown";
}

qualification_performance_plan make_default_qualification_performance_plan() {
  qualification_performance_plan plan;
  plan.identifier = "plan16.p6.9.performance-v1";
  plan.protocol.identifier = "controlled-strict-fp-release-v1";
  plan.protocol.version = "1";
  plan.cases = default_cases();
  plan.thresholds = frozen_thresholds();
  auto made = make_qualification_performance_plan(std::move(plan));
  if (!made.has_value())
    throw std::logic_error(
        "default mesh Boolean qualification performance plan is invalid");
  return std::move(made.value());
}

product_status_or<qualification_performance_plan>
make_qualification_performance_plan(qualification_performance_plan value) {
  try {
    if (value.schema != qualification_performance_schema_version ||
        value.checker_version != qualification_performance_checker_version ||
        !text(value.identifier) || value.cases.empty() ||
        value.cases.size() > 4096U || value.thresholds.empty() ||
        value.thresholds.size() > 256U || value.threshold_reviews.size() > 256U)
      return performance_error("qualification_performance.plan_malformed");

    auto protocol = canonicalize_protocol(std::move(value.protocol));
    if (!protocol.has_value())
      return protocol.error();
    value.protocol = std::move(protocol.value());
