      zero(value.workload_recipe_digest))
    return performance_error("qualification_performance.case_malformed");

  const bool repair = value.kind ==
                      qualification_performance_workload_kind::normalization_repair;
  if (repair != !value.normalization_policy_identifier.empty())
    return performance_error(
        "qualification_performance.normalization_policy_mismatch");
  if (success_outcome(value.expected_outcome) != value.expected_error_key.empty())
    return performance_error("qualification_performance.expected_outcome_mismatch");
  if (value.baseline_comparison_required) {
    if (!text(value.baseline_family) || !text(value.comparison_group))
      return performance_error("qualification_performance.baseline_binding_missing");
  } else if (!value.baseline_family.empty() || !value.comparison_group.empty()) {
    return performance_error("qualification_performance.unexpected_baseline_binding");
  }
  const bool cancellation = value.kind ==
      qualification_performance_workload_kind::cancellation_transaction;
  if (cancellation != (value.cancellation_latency_limit_milliseconds != 0U))
    return performance_error("qualification_performance.cancellation_limit_mismatch");
  if (transaction_kind(value.kind) &&
      value.expected_outcome != qualification_outcome::timeout_or_resource_limit)
    return performance_error("qualification_performance.transaction_outcome");

  const auto calculated = case_digest(value);
  if (!zero(value.case_digest) && value.case_digest != calculated)
    return performance_error("qualification_performance.case_digest");
  value.case_digest = calculated;
  return value;
}

product_status_or<qualification_performance_threshold>
canonicalize_threshold(qualification_performance_threshold value) {
  if (value.schema != qualification_performance_schema_version ||
      !text(value.identifier) || !known(value.kind) || !known(value.metric) ||
      !text(value.scope) || value.numerator == 0U || value.denominator == 0U ||
      !value.blocking)
    return performance_error("qualification_performance.threshold_malformed");
  if (value.kind ==
          qualification_performance_threshold_kind::authoritative_bytes_nonincrease &&
      value.metric != qualification_performance_metric::authoritative_bytes)
    return performance_error("qualification_performance.threshold_metric_mismatch");
  if (value.kind !=
          qualification_performance_threshold_kind::authoritative_bytes_nonincrease &&
      value.kind != qualification_performance_threshold_kind::absolute_upper_bound &&
      value.metric != qualification_performance_metric::wall_time_nanoseconds)
    return performance_error("qualification_performance.threshold_metric_mismatch");
  const auto calculated = threshold_digest(value);
  if (!zero(value.threshold_digest) && value.threshold_digest != calculated)
    return performance_error("qualification_performance.threshold_digest");
  value.threshold_digest = calculated;
  return value;
}

product_status_or<qualification_performance_threshold_review>
canonicalize_review(qualification_performance_threshold_review value) {
  if (value.schema != qualification_performance_schema_version ||
      !text(value.threshold_identifier) || zero(value.prior_threshold_digest) ||
      zero(value.revised_threshold_digest) ||
      value.prior_threshold_digest == value.revised_threshold_digest ||
      !text(value.reviewer) || !text(value.rationale) ||
      zero(value.evidence_digest) || !value.approved)
    return performance_error("qualification_performance.review_malformed");
  const auto calculated = review_digest(value);
  if (!zero(value.review_digest) && value.review_digest != calculated)
    return performance_error("qualification_performance.review_digest");
  value.review_digest = calculated;
  return value;
}

product_status_or<qualification_performance_sample>
canonicalize_sample(qualification_performance_sample value) {
  if (value.schema != qualification_performance_schema_version ||
      value.ordinal == 0U || value.metric_presence_mask == 0U ||
      (value.metric_presence_mask & ~qualification_performance_all_metrics_mask) != 0U ||
      !known(value.outcome) || !text(value.typed_outcome_key) ||
      zero(value.run_log_digest))
    return performance_error("qualification_performance.sample_malformed");
  for (std::size_t i = 0; i < value.metrics.size(); ++i)
    if ((value.metric_presence_mask & (std::uint64_t{1} << i)) == 0U &&
        value.metrics[i] != 0U)
      return performance_error("qualification_performance.unreported_metric_nonzero");
  if (success_outcome(value.outcome)) {
    if (zero(value.canonical_semantic_digest) ||
        !zero(value.canonical_failure_digest))
      return performance_error("qualification_performance.sample_success_binding");
  } else if (!zero(value.canonical_semantic_digest) ||
             zero(value.canonical_failure_digest)) {
    return performance_error("qualification_performance.sample_failure_binding");
  }
  const auto calculated = sample_digest(value);
  if (!zero(value.sample_digest) && value.sample_digest != calculated)
    return performance_error("qualification_performance.sample_digest");
  value.sample_digest = calculated;
  return value;
}

template <class T>
bool unique_identifiers(const std::vector<T> &values) {
  for (std::size_t i = 1; i < values.size(); ++i)
    if (values[i - 1].identifier == values[i].identifier)
      return false;
  return true;
}

bool frozen_shape_equal(const qualification_performance_threshold &a,
                        const qualification_performance_threshold &b) noexcept {
  return a.identifier == b.identifier && a.kind == b.kind &&
         a.metric == b.metric && a.scope == b.scope && a.blocking == b.blocking;
}

qualification_performance_threshold threshold(
    const char *identifier, qualification_performance_threshold_kind kind,
    qualification_performance_metric metric, const char *scope,
    std::uint64_t numerator, std::uint64_t denominator) {
  qualification_performance_threshold value;
  value.identifier = identifier;
  value.kind = kind;
  value.metric = metric;
  value.scope = scope;
  value.numerator = numerator;
  value.denominator = denominator;
  return value;
}

std::vector<qualification_performance_threshold> frozen_thresholds() {
  std::vector<qualification_performance_threshold> result;
  result.push_back(threshold(
      geomean_threshold_id,
      qualification_performance_threshold_kind::geometric_mean_speedup,
      qualification_performance_metric::wall_time_nanoseconds,
      comparison_group_id, 4U, 1U));
  result.push_back(threshold(
      b6_threshold_id,
      qualification_performance_threshold_kind::case_speedup,
      qualification_performance_metric::wall_time_nanoseconds, "B6", 5U, 1U));
  result.push_back(threshold(
      per_case_threshold_id,
      qualification_performance_threshold_kind::maximum_case_slowdown,
      qualification_performance_metric::wall_time_nanoseconds,
      comparison_group_id, 115U, 100U));
  result.push_back(threshold(
      bytes_threshold_id,
      qualification_performance_threshold_kind::authoritative_bytes_nonincrease,
      qualification_performance_metric::authoritative_bytes,
      comparison_group_id, 1U, 1U));
  for (auto &entry : result) {
    auto made = canonicalize_threshold(std::move(entry));
    if (!made.has_value())
      throw std::logic_error("frozen P6.9 threshold is invalid");
    entry = std::move(made.value());
  }
  std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
    return a.identifier < b.identifier;
  });
  return result;
}

digest recipe_digest_for(const std::string &identifier) {
  canonical_encoder encoder;
  encoder.string(identifier);
  return domain_digest(recipe_tag, encoder.bytes());
}

qualification_performance_case performance_case(
    const char *identifier, qualification_performance_workload_kind kind,
    qualification_performance_scale scale, const char *fixture,
    const char *category, operation op = operation::regularized_union) {
  qualification_performance_case value;
  value.identifier = identifier;
  value.kind = kind;
  value.scale = scale;
  value.fixture_identifier = fixture;
  value.geometry_category = category;
  value.selected_operation = op;
