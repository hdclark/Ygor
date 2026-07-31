        value.observation_digest != calculated)
      return performance_error("qualification_performance.observation_digest");
    value.observation_digest = calculated;
    return value;
  } catch (const std::bad_alloc &) {
    return performance_error("qualification_performance.allocation_failure");
  }
}

product_status_or<qualification_performance_report>
make_qualification_performance_report(
    const qualification_performance_plan &plan,
    std::vector<qualification_performance_observation> observations,
    bool requested_complete) {
  try {
    auto valid_plan = validate_qualification_performance_plan(plan);
    if (!valid_plan.has_value())
      return valid_plan.error();
    if (!valid_plan.value())
