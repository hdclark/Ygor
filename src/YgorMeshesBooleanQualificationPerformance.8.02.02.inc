    const qualification_performance_report &value) {
  // A report cannot be validated without its plan. Canonical bytes are still
  // self-bound by the report digest and were produced only by the report maker.
  if (value.canonical_bytes.empty() || zero(value.report_digest) ||
      domain_digest(report_tag, value.canonical_bytes) != value.report_digest)
    return performance_error("qualification_performance.report_not_canonical");
  return value.canonical_bytes;
}

bool qualification_performance_gate_passes(
    const qualification_performance_report &report,
    const qualification_performance_plan &plan) noexcept {
  auto valid = validate_qualification_performance_report(report, plan);
