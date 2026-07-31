    if (!made.has_value())
      return made.error();
    return made.value().canonical_bytes == value.canonical_bytes &&
           made.value().plan_digest == value.plan_digest;
  } catch (const std::bad_alloc &) {
    return performance_error("qualification_performance.allocation_failure");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_performance_plan(
    const qualification_performance_plan &value) {
  auto valid = validate_qualification_performance_plan(value);
  if (!valid.has_value())
    return valid.error();
  if (!valid.value())
    return performance_error("qualification_performance.plan_not_canonical");
  return value.canonical_bytes;
}

product_status_or<qualification_performance_threshold>
make_qualification_performance_threshold(
    qualification_performance_threshold value) {
  try {
    return canonicalize_threshold(std::move(value));
  } catch (const std::bad_alloc &) {
    return performance_error("qualification_performance.allocation_failure");
  }
}

product_status_or<qualification_performance_threshold_review>
make_qualification_performance_threshold_review(
    qualification_performance_threshold_review value) {
  try {
    return canonicalize_review(std::move(value));
  } catch (const std::bad_alloc &) {
    return performance_error("qualification_performance.allocation_failure");
  }
}

product_status_or<qualification_performance_sample>
make_qualification_performance_sample(qualification_performance_sample value) {
  try {
    return canonicalize_sample(std::move(value));
  } catch (const std::bad_alloc &) {
    return performance_error("qualification_performance.allocation_failure");
  }
}

product_status_or<qualification_performance_observation>
make_qualification_performance_observation(
    const qualification_performance_plan &plan,
    const qualification_performance_case &descriptor,
    qualification_performance_observation value) {
  try {
    auto valid_plan = validate_qualification_performance_plan(plan);
    if (!valid_plan.has_value())
      return valid_plan.error();
    if (!valid_plan.value())
      return performance_error("qualification_performance.plan_not_canonical");
    const auto found = std::find_if(plan.cases.begin(), plan.cases.end(),
                                    [&](const auto &entry) {
                                      return entry.identifier ==
                                             descriptor.identifier;
                                    });
    if (found == plan.cases.end() || found->case_digest != descriptor.case_digest)
