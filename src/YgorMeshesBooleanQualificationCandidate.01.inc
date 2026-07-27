  return domain_digest(gate_tag, encoder.bytes());
}

void encode_execution_case(canonical_encoder &encoder,
                           const qualification_candidate_execution_case &value,
                           bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.string(value.source_identifier);
  encode_digest(encoder, value.source_plan_digest);
  encode_digest(encoder, value.source_case_digest);
  encode_dimension(encoder, value.dimensions);
  encoder.u64(value.expected_outcomes.size());
  for (const auto outcome : value.expected_outcomes)
    encoder.byte(static_cast<std::uint8_t>(outcome));
  encoder.u64(value.expected_failure_codes.size());
  for (const auto code : value.expected_failure_codes)
    encoder.u16(static_cast<std::uint16_t>(code));
  encoder.boolean(value.require_regression_promotion_on_engine_defect);
  if (include_digest)
    encode_digest(encoder, value.case_digest);
}

digest execution_case_digest(
    const qualification_candidate_execution_case &value) {
  canonical_encoder encoder;
  encode_execution_case(encoder, value, false);
  return domain_digest(case_tag, encoder.bytes());
}

void encode_plan_semantic(canonical_encoder &encoder,
                          const qualification_candidate_campaign_plan &value) {
  encoder.u16(value.schema);
  encoder.u32(value.checker_version);
  encoder.string(value.identifier);
  encoder.u64(value.manifest_canonical_bytes.size());
  if (!value.manifest_canonical_bytes.empty())
    encoder.raw(value.manifest_canonical_bytes.data(),
                value.manifest_canonical_bytes.size());
  encode_digest(encoder, value.manifest_digest);
  encoder.u64(value.required_gates.size());
  for (const auto &entry : value.required_gates)
    encode_gate(encoder, entry, true);
  encoder.u64(value.execution_cases.size());
  for (const auto &entry : value.execution_cases)
    encode_execution_case(encoder, entry, true);
}

void encode_observation(
    canonical_encoder &encoder,
    const qualification_candidate_execution_observation &value,
    bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.case_identifier);
  encode_digest(encoder, value.case_digest);
  encoder.byte(static_cast<std::uint8_t>(value.outcome));
  encoder.boolean(value.error_code.has_value());
  if (value.error_code)
    encoder.u16(static_cast<std::uint16_t>(*value.error_code));
  encoder.boolean(value.completed);
  encoder.string(value.typed_outcome_key);
  encode_digest(encoder, value.accounting_digest);
  encode_digest(encoder, value.canonical_result_digest);
  encode_digest(encoder, value.canonical_failure_digest);
  encode_digest(encoder, value.replay_digest);
  encode_digest(encoder, value.run_log_digest);
  encoder.boolean(value.performance_regression_observed);
  encoder.boolean(value.resource_regression_observed);
  encode_digest(encoder, value.regression_evidence_digest);
  if (include_digest)
    encode_digest(encoder, value.observation_digest);
}

digest observation_digest(
    const qualification_candidate_execution_observation &value) {
  canonical_encoder encoder;
  encode_observation(encoder, value, false);
  return domain_digest(observation_tag, encoder.bytes());
}

void encode_regression(
    canonical_encoder &encoder,
    const qualification_candidate_regression_binding &value,
    bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.promotion_identifier);
  encoder.string(value.permanent_test_id);
  encode_digest(encoder, value.minimized_case_digest);
  encode_digest(encoder, value.canonical_case_bytes_digest);
  encode_digest(encoder, value.minimization_transcript_digest);
  encode_digest(encoder, value.promotion_artifact_digest);
  encode_digest(encoder, value.corpus_digest_before);
  encode_digest(encoder, value.corpus_digest_after);
  if (include_digest)
    encode_digest(encoder, value.binding_digest);
}

digest regression_digest(
    const qualification_candidate_regression_binding &value) {
  canonical_encoder encoder;
  encode_regression(encoder, value, false);
  return domain_digest(regression_tag, encoder.bytes());
}

void encode_issue(canonical_encoder &encoder,
                  const qualification_candidate_issue &value,
                  bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.string(value.case_identifier);
  encode_digest(encoder, value.case_digest);
  encoder.byte(static_cast<std::uint8_t>(value.kind));
  encode_digest(encoder, value.initial_observation_digest);
  encode_digest(encoder, value.detected_evidence_digest);
  encoder.byte(static_cast<std::uint8_t>(value.disposition));
  encoder.string(value.reviewer);
  encoder.string(value.rationale);
  encode_digest(encoder, value.resolution_evidence_digest);
  encode_digest(encoder, value.reviewed_gate_or_plan_digest);
  encoder.boolean(value.regression.has_value());
