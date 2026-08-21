      return performance_error("qualification_performance.case_not_in_plan");
    if (value.schema != qualification_performance_schema_version ||
        value.case_identifier != descriptor.identifier ||
        value.case_digest != descriptor.case_digest ||
        value.protocol_digest != plan.protocol.protocol_digest ||
        zero(value.hardware_digest) || zero(value.environment_digest) ||
        zero(value.command_digest) ||
        value.completed_warmup_runs != plan.protocol.warmup_runs)
      return performance_error("qualification_performance.observation_malformed");

    if (descriptor.baseline_comparison_required) {
      auto baseline = canonicalize_samples(
          plan.protocol, descriptor, std::move(value.baseline_samples),
          "qualification_performance.baseline_samples");
      if (!baseline.has_value())
        return baseline.error();
      value.baseline_samples = std::move(baseline.value());
    } else if (!value.baseline_samples.empty()) {
      return performance_error("qualification_performance.unexpected_baseline");
    }
    auto candidate = canonicalize_samples(
        plan.protocol, descriptor, std::move(value.candidate_samples),
        "qualification_performance.candidate_samples");
    if (!candidate.has_value())
      return candidate.error();
    value.candidate_samples = std::move(candidate.value());

    if (descriptor.baseline_comparison_required) {
      if (value.baseline_samples.front().typed_outcome_key !=
              value.candidate_samples.front().typed_outcome_key ||
          value.baseline_samples.front().canonical_semantic_digest !=
              value.candidate_samples.front().canonical_semantic_digest ||
          value.baseline_samples.front().canonical_failure_digest !=
              value.candidate_samples.front().canonical_failure_digest)
        return performance_error(
            "qualification_performance.baseline_semantic_mismatch");
    }

    if (descriptor.kind ==
        qualification_performance_workload_kind::cancellation_transaction) {
      for (const auto &entry : value.candidate_samples)
        if (entry.metric(qualification_performance_metric::
                             cancellation_latency_milliseconds) >
            descriptor.cancellation_latency_limit_milliseconds)
          return performance_error(
