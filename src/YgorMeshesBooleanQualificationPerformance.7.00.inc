
    for (auto &entry : value.cases) {
      auto made = canonicalize_case(std::move(entry));
      if (!made.has_value())
        return made.error();
      entry = std::move(made.value());
    }
    std::sort(value.cases.begin(), value.cases.end(), [](const auto &a,
                                                         const auto &b) {
      return a.identifier < b.identifier;
    });
    if (!unique_identifiers(value.cases) || !complete_case_coverage(value.cases))
      return performance_error(
          "qualification_performance.case_coverage_incomplete");

    for (auto &entry : value.thresholds) {
      auto made = canonicalize_threshold(std::move(entry));
      if (!made.has_value())
        return made.error();
      entry = std::move(made.value());
    }
    std::sort(value.thresholds.begin(), value.thresholds.end(),
              [](const auto &a, const auto &b) {
                return a.identifier < b.identifier;
              });
    if (!unique_identifiers(value.thresholds))
      return performance_error("qualification_performance.threshold_duplicate");

    for (auto &entry : value.threshold_reviews) {
      auto made = canonicalize_review(std::move(entry));
      if (!made.has_value())
        return made.error();
      entry = std::move(made.value());
    }
    std::sort(value.threshold_reviews.begin(), value.threshold_reviews.end(),
              [](const auto &a, const auto &b) {
                return a.threshold_identifier < b.threshold_identifier;
              });
    if (!review_bindings_valid(value.thresholds, value.threshold_reviews))
      return performance_error(
          "qualification_performance.threshold_review_binding");

    canonical_encoder encoder;
    encode_plan_semantic(encoder, value);
    const auto bytes = encoder.bytes();
    const auto calculated = domain_digest(plan_tag, bytes);
    if ((!value.canonical_bytes.empty() && value.canonical_bytes != bytes) ||
        (!zero(value.plan_digest) && value.plan_digest != calculated))
      return performance_error("qualification_performance.plan_digest");
    value.canonical_bytes = bytes;
    value.plan_digest = calculated;
    return value;
  } catch (const std::bad_alloc &) {
    return performance_error("qualification_performance.allocation_failure");
  }
}

product_status_or<bool> validate_qualification_performance_plan(
    const qualification_performance_plan &value) noexcept {
  try {
    auto copy = value;
    copy.canonical_bytes.clear();
    copy.plan_digest = {};
    auto made = make_qualification_performance_plan(std::move(copy));
