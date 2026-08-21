  sample.metrics[static_cast<std::size_t>(
      qualification_performance_metric::cancellation_latency_milliseconds)] =
      cancellation_latency;
  sample.outcome = descriptor.expected_outcome;
  sample.typed_outcome_key = descriptor.expected_error_key.empty()
                                 ? "qualification.verified-success"
                                 : descriptor.expected_error_key;
  if (successful(sample.outcome))
    sample.canonical_semantic_digest =
        tagged_digest("semantic:" + descriptor.identifier);
  else
    sample.canonical_failure_digest =
        tagged_digest("failure:" + descriptor.identifier);
  sample.run_log_digest = tagged_digest(
      "run:" + descriptor.identifier + ":" + run_kind + ":" +
      std::to_string(ordinal));
  auto made = make_qualification_performance_sample(std::move(sample));
  require(made.has_value(), "performance sample canonicalizes");
  return std::move(made.value());
}

qualification_performance_observation passing_observation(
    const qualification_performance_plan &plan,
    const qualification_performance_case &descriptor) {
  qualification_performance_observation observation;
  observation.case_identifier = descriptor.identifier;
  observation.case_digest = descriptor.case_digest;
  observation.protocol_digest = plan.protocol.protocol_digest;
  observation.hardware_digest = tagged_digest("hardware:controlled-host-v1");
  observation.environment_digest =
      tagged_digest("environment:" + descriptor.identifier);
  observation.command_digest = tagged_digest("command:" + descriptor.identifier);
  observation.completed_warmup_runs = plan.protocol.warmup_runs;

  std::uint64_t baseline_wall = 0;
  std::uint64_t candidate_wall = 100;
  if (descriptor.baseline_comparison_required) {
    baseline_wall = descriptor.baseline_family == "B6" ? 600 : 500;
  }
  const auto cancellation_latency =
      descriptor.kind ==
              qualification_performance_workload_kind::cancellation_transaction
          ? 200U
          : 0U;
  for (std::uint32_t ordinal = 1; ordinal <= plan.protocol.measured_runs;
       ++ordinal) {
    if (descriptor.baseline_comparison_required)
      observation.baseline_samples.push_back(sample_for(
          descriptor, ordinal, baseline_wall, 1000, cancellation_latency,
          "baseline"));
    observation.candidate_samples.push_back(sample_for(
        descriptor, ordinal, candidate_wall, 1000, cancellation_latency,
        "candidate"));
  }

  const auto transactional =
      descriptor.kind ==
          qualification_performance_workload_kind::resource_limit_transaction ||
      descriptor.kind ==
          qualification_performance_workload_kind::cancellation_transaction;
  if (transactional) {
    observation.publication_state_before_digest =
        tagged_digest("publication:" + descriptor.identifier);
    observation.publication_state_after_digest =
        observation.publication_state_before_digest;
    observation.replay_digest = tagged_digest("replay:" + descriptor.identifier);
    observation.limit_or_cancellation_triggered = true;
    observation.typed_failure_observed = true;
    observation.transaction_rolled_back = true;
  }
  auto made = make_qualification_performance_observation(
      plan, descriptor, std::move(observation));
  require(made.has_value(), "performance observation canonicalizes");
  return std::move(made.value());
}

std::vector<qualification_performance_observation> passing_observations(
    const qualification_performance_plan &plan) {
  std::vector<qualification_performance_observation> result;
  result.reserve(plan.cases.size());
  for (const auto &descriptor : plan.cases)
    result.push_back(passing_observation(plan, descriptor));
  return result;
}

void set_candidate_metric(
    const qualification_performance_plan &plan,
