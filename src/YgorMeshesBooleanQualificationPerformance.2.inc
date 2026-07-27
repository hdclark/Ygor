  encoder.string(value.reviewer);
  encoder.string(value.rationale);
  encode_digest(encoder, value.evidence_digest);
  encoder.boolean(value.approved);
  if (include_digest)
    encode_digest(encoder, value.review_digest);
}

void encode_sample(canonical_encoder &encoder,
                   const qualification_performance_sample &value,
                   bool include_digest) {
  encoder.u16(value.schema);
  encoder.u32(value.ordinal);
  encoder.u64(value.metric_presence_mask);
  encode_u64_array(encoder, value.metrics);
  encoder.byte(static_cast<std::uint8_t>(value.outcome));
  encoder.string(value.typed_outcome_key);
  encode_digest(encoder, value.canonical_semantic_digest);
  encode_digest(encoder, value.canonical_failure_digest);
  encode_digest(encoder, value.run_log_digest);
  if (include_digest)
    encode_digest(encoder, value.sample_digest);
}

void encode_samples(canonical_encoder &encoder,
                    const std::vector<qualification_performance_sample> &values) {
  encoder.u64(values.size());
  for (const auto &entry : values)
    encode_sample(encoder, entry, true);
}

void encode_observation(canonical_encoder &encoder,
                        const qualification_performance_observation &value,
                        bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.case_identifier);
  encode_digest(encoder, value.case_digest);
  encode_digest(encoder, value.protocol_digest);
  encode_digest(encoder, value.hardware_digest);
  encode_digest(encoder, value.environment_digest);
  encode_digest(encoder, value.command_digest);
  encoder.u32(value.completed_warmup_runs);
  encode_samples(encoder, value.baseline_samples);
  encode_samples(encoder, value.candidate_samples);
  encode_digest(encoder, value.publication_state_before_digest);
  encode_digest(encoder, value.publication_state_after_digest);
  encode_digest(encoder, value.replay_digest);
  encoder.boolean(value.limit_or_cancellation_triggered);
  encoder.boolean(value.typed_failure_observed);
  encoder.boolean(value.transaction_rolled_back);
  encoder.boolean(value.partial_publication_observed);
  if (include_digest)
    encode_digest(encoder, value.observation_digest);
}

void encode_statistics(canonical_encoder &encoder,
                       const qualification_performance_case_statistics &value,
                       bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.case_identifier);
  encode_digest(encoder, value.case_digest);
  encoder.boolean(value.baseline_present);
  encode_u64_array(encoder, value.baseline_medians);
  encode_u64_array(encoder, value.baseline_mads);
  encode_u64_array(encoder, value.candidate_medians);
  encode_u64_array(encoder, value.candidate_mads);
  encode_digest(encoder, value.canonical_semantic_digest);
  encode_digest(encoder, value.canonical_failure_digest);
  if (include_digest)
    encode_digest(encoder, value.statistics_digest);
}

void encode_plan_semantic(canonical_encoder &encoder,
                          const qualification_performance_plan &value) {
  encoder.u16(value.schema);
  encoder.u32(value.checker_version);
  encoder.string(value.identifier);
  encode_protocol(encoder, value.protocol, true);
  encoder.u64(value.cases.size());
  for (const auto &entry : value.cases)
    encode_case(encoder, entry, true);
  encoder.u64(value.thresholds.size());
  for (const auto &entry : value.thresholds)
    encode_threshold(encoder, entry, true);
  encoder.u64(value.threshold_reviews.size());
  for (const auto &entry : value.threshold_reviews)
    encode_review(encoder, entry, true);
}

void encode_report_semantic(canonical_encoder &encoder,
                            const qualification_performance_report &value) {
  encoder.u16(value.schema);
  encoder.u32(value.checker_version);
  encoder.string(value.identifier);
  encode_digest(encoder, value.plan_digest);
  encoder.u64(value.observations.size());
  for (const auto &entry : value.observations)
    encode_observation(encoder, entry, true);
  encoder.u64(value.statistics.size());
  for (const auto &entry : value.statistics)
    encode_statistics(encoder, entry, true);
  encoder.u64(value.passed_case_count);
  encoder.u64(value.passed_threshold_count);
  encoder.u64(value.blocking_issue_count);
  encoder.boolean(value.complete);
}

digest protocol_digest(const qualification_performance_protocol_v1 &value) {
  canonical_encoder encoder;
  encode_protocol(encoder, value, false);
  return domain_digest(protocol_tag, encoder.bytes());
}
digest case_digest(const qualification_performance_case &value) {
  canonical_encoder encoder;
  encode_case(encoder, value, false);
  return domain_digest(case_tag, encoder.bytes());
}
digest threshold_digest(const qualification_performance_threshold &value) {
  canonical_encoder encoder;
  encode_threshold(encoder, value, false);
  return domain_digest(threshold_tag, encoder.bytes());
}
digest review_digest(const qualification_performance_threshold_review &value) {
  canonical_encoder encoder;
  encode_review(encoder, value, false);
  return domain_digest(review_tag, encoder.bytes());
}
digest sample_digest(const qualification_performance_sample &value) {
  canonical_encoder encoder;
  encode_sample(encoder, value, false);
  return domain_digest(sample_tag, encoder.bytes());
}
digest observation_digest(const qualification_performance_observation &value) {
  canonical_encoder encoder;
  encode_observation(encoder, value, false);
  return domain_digest(observation_tag, encoder.bytes());
}
digest statistics_digest(
    const qualification_performance_case_statistics &value) {
  canonical_encoder encoder;
  encode_statistics(encoder, value, false);
  return domain_digest(statistics_tag, encoder.bytes());
}

product_status_or<qualification_performance_protocol_v1>
canonicalize_protocol(qualification_performance_protocol_v1 value) {
  if (value.schema != qualification_performance_schema_version ||
      !text(value.identifier) || !text(value.version) || !known(value.statistic) ||
      value.warmup_runs == 0 || value.measured_runs < 5U ||
      (value.measured_runs % 2U) == 0U || !value.retain_raw_samples ||
      !value.monotonic_wall_clock_required ||
      !value.process_cpu_clock_required ||
      !value.controlled_exclusive_host_required || !value.cpu_affinity_required ||
      !value.frequency_policy_recorded || !value.allocator_policy_recorded ||
      !value.peak_rss_external_observer_required ||
      !value.authoritative_accounting_separate)
    return performance_error("qualification_performance.protocol_malformed");
  const auto calculated = protocol_digest(value);
  if (!zero(value.protocol_digest) && value.protocol_digest != calculated)
    return performance_error("qualification_performance.protocol_digest");
  value.protocol_digest = calculated;
  return value;
}

product_status_or<qualification_performance_case>
canonicalize_case(qualification_performance_case value) {
  if (value.schema != qualification_performance_schema_version ||
      !text(value.identifier) || !known(value.kind) || !known(value.scale) ||
      !text(value.fixture_identifier) || !text(value.geometry_category) ||
      !text(value.normalization_policy_identifier, true) ||
      !known(value.selected_operation) || !known(value.type.coordinate) ||
      !known(value.type.index) || !known(value.expected_outcome) ||
      !text(value.expected_error_key, true) || !text(value.baseline_family, true) ||
      !text(value.comparison_group, true) ||
      value.required_metrics_mask != qualification_performance_all_metrics_mask ||
