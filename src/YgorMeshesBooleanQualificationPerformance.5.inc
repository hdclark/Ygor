  return changed == reviews.size();
}

product_status_or<std::vector<qualification_performance_sample>>
canonicalize_samples(const qualification_performance_protocol_v1 &protocol,
                     const qualification_performance_case &descriptor,
                     std::vector<qualification_performance_sample> values,
                     const char *error_key) {
  if (values.size() != protocol.measured_runs)
    return performance_error(error_key);
  for (auto &entry : values) {
    auto made = canonicalize_sample(std::move(entry));
    if (!made.has_value())
      return made.error();
    entry = std::move(made.value());
  }
  std::sort(values.begin(), values.end(), [](const auto &a, const auto &b) {
    return a.ordinal < b.ordinal;
  });
  for (std::size_t i = 0; i < values.size(); ++i) {
    const auto &entry = values[i];
    if (entry.ordinal != i + 1U ||
        (entry.metric_presence_mask & descriptor.required_metrics_mask) !=
            descriptor.required_metrics_mask ||
        entry.outcome != descriptor.expected_outcome)
      return performance_error(error_key);
    if (descriptor.expected_error_key.empty()) {
      if (!text(entry.typed_outcome_key))
        return performance_error(error_key);
    } else if (entry.typed_outcome_key != descriptor.expected_error_key) {
      return performance_error(error_key);
    }
    if (entry.metric(qualification_performance_metric::wall_time_nanoseconds) ==
            0U ||
        entry.metric(qualification_performance_metric::cpu_time_nanoseconds) ==
            0U ||
        entry.metric(qualification_performance_metric::peak_rss_bytes) == 0U)
      return performance_error(error_key);
    if (i != 0U &&
        (entry.typed_outcome_key != values.front().typed_outcome_key ||
         entry.canonical_semantic_digest !=
             values.front().canonical_semantic_digest ||
         entry.canonical_failure_digest !=
             values.front().canonical_failure_digest))
      return performance_error(error_key);
  }
  return values;
}

std::uint64_t median(std::vector<std::uint64_t> values) {
  std::sort(values.begin(), values.end());
  return values[values.size() / 2U];
}

std::uint64_t median_absolute_deviation(
    const std::vector<std::uint64_t> &values, std::uint64_t centre) {
  std::vector<std::uint64_t> deviations;
  deviations.reserve(values.size());
  for (const auto value : values)
    deviations.push_back(value >= centre ? value - centre : centre - value);
  return median(std::move(deviations));
}

qualification_performance_case_statistics statistics_for(
    const qualification_performance_case &descriptor,
    const qualification_performance_observation &observation) {
  qualification_performance_case_statistics result;
  result.case_identifier = descriptor.identifier;
  result.case_digest = descriptor.case_digest;
  result.baseline_present = !observation.baseline_samples.empty();
  for (std::size_t metric = 0; metric < qualification_performance_metric_count;
       ++metric) {
    std::vector<std::uint64_t> candidate;
    candidate.reserve(observation.candidate_samples.size());
    for (const auto &sample : observation.candidate_samples)
      candidate.push_back(sample.metrics[metric]);
    result.candidate_medians[metric] = median(candidate);
    result.candidate_mads[metric] = median_absolute_deviation(
        candidate, result.candidate_medians[metric]);
    if (result.baseline_present) {
      std::vector<std::uint64_t> baseline;
      baseline.reserve(observation.baseline_samples.size());
      for (const auto &sample : observation.baseline_samples)
        baseline.push_back(sample.metrics[metric]);
      result.baseline_medians[metric] = median(baseline);
      result.baseline_mads[metric] = median_absolute_deviation(
          baseline, result.baseline_medians[metric]);
    }
  }
  result.canonical_semantic_digest =
      observation.candidate_samples.front().canonical_semantic_digest;
  result.canonical_failure_digest =
      observation.candidate_samples.front().canonical_failure_digest;
  result.statistics_digest = statistics_digest(result);
  return result;
}

class natural_product {
  static constexpr std::uint32_t base = 10000U;
  std::vector<std::uint32_t> digits_{1U};

  void normalize() {
    while (digits_.size() > 1U && digits_.back() == 0U)
      digits_.pop_back();
  }

public:
  void multiply(std::uint64_t factor) {
    if (factor == 0U) {
      digits_.assign(1U, 0U);
      return;
    }
    std::vector<std::uint32_t> rhs;
    while (factor != 0U) {
      rhs.push_back(static_cast<std::uint32_t>(factor % base));
      factor /= base;
    }
    std::vector<std::uint64_t> product(digits_.size() + rhs.size(), 0U);
    for (std::size_t i = 0; i < digits_.size(); ++i)
      for (std::size_t j = 0; j < rhs.size(); ++j)
        product[i + j] +=
            static_cast<std::uint64_t>(digits_[i]) * rhs[j];
    digits_.assign(product.size(), 0U);
    std::uint64_t carry = 0U;
    for (std::size_t i = 0; i < product.size(); ++i) {
      const auto value = product[i] + carry;
      digits_[i] = static_cast<std::uint32_t>(value % base);
      carry = value / base;
    }
    while (carry != 0U) {
      digits_.push_back(static_cast<std::uint32_t>(carry % base));
      carry /= base;
    }
    normalize();
  }

  friend int compare(const natural_product &a,
                     const natural_product &b) noexcept {
    if (a.digits_.size() != b.digits_.size())
      return a.digits_.size() < b.digits_.size() ? -1 : 1;
    for (std::size_t i = a.digits_.size(); i != 0U; --i)
      if (a.digits_[i - 1U] != b.digits_[i - 1U])
        return a.digits_[i - 1U] < b.digits_[i - 1U] ? -1 : 1;
    return 0;
  }
};

bool scope_matches(const qualification_performance_threshold &threshold,
                   const qualification_performance_case &descriptor) {
  return threshold.scope == "*" || threshold.scope == descriptor.identifier ||
         threshold.scope == descriptor.baseline_family ||
         threshold.scope == descriptor.comparison_group;
}

bool ratio_greater_equal(std::uint64_t left_value, std::uint64_t left_factor,
                         std::uint64_t right_value,
                         std::uint64_t right_factor) {
  natural_product left;
  left.multiply(left_value);
  left.multiply(left_factor);
  natural_product right;
  right.multiply(right_value);
  right.multiply(right_factor);
  return compare(left, right) >= 0;
}

bool threshold_passes(
    const qualification_performance_threshold &threshold,
    const qualification_performance_plan &plan,
    const std::map<std::string, qualification_performance_case_statistics>
        &statistics) {
  const auto metric = static_cast<std::size_t>(threshold.metric);
  std::vector<std::pair<const qualification_performance_case *,
                        const qualification_performance_case_statistics *>>
      matches;
