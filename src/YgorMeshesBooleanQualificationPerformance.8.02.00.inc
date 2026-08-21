  } catch (const std::bad_alloc &) {
    return performance_error("qualification_performance.allocation_failure");
  }
}

product_status_or<bool> validate_qualification_performance_report(
    const qualification_performance_report &value,
    const qualification_performance_plan &plan) noexcept {
  try {
    if (value.schema != qualification_performance_schema_version ||
        value.checker_version != qualification_performance_checker_version ||
        value.identifier != plan.identifier + ".report" ||
        value.plan_digest != plan.plan_digest)
      return performance_error("qualification_performance.report_malformed");
    auto made = make_qualification_performance_report(
        plan, value.observations, value.complete);
