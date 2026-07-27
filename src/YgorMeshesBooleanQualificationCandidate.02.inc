  if (value.regression)
    encode_regression(encoder, *value.regression, true);
  encoder.u64(value.affected_configuration_identifiers.size());
  for (const auto &entry : value.affected_configuration_identifiers)
    encoder.string(entry);
  encoder.u64(value.rerun_observation_digests.size());
  for (const auto &entry : value.rerun_observation_digests)
    encode_digest(encoder, entry);
  encoder.boolean(value.resolution_verified);
  if (include_digest)
    encode_digest(encoder, value.issue_digest);
}

digest issue_digest(const qualification_candidate_issue &value) {
  canonical_encoder encoder;
  encode_issue(encoder, value, false);
  return domain_digest(issue_tag, encoder.bytes());
}

void encode_campaign_semantic(canonical_encoder &encoder,
                              const qualification_candidate_campaign &value) {
  encoder.u16(value.schema);
  encoder.u32(value.checker_version);
  encoder.string(value.identifier);
  encode_digest(encoder, value.plan_digest);
  encoder.u64(value.observations.size());
  for (const auto &entry : value.observations)
    encode_observation(encoder, entry, true);
  encoder.u64(value.issues.size());
  for (const auto &entry : value.issues)
    encode_issue(encoder, entry, true);
  encoder.u64(value.passed_case_count);
  encoder.u64(value.resolved_issue_count);
  encoder.u64(value.promoted_regression_count);
  encoder.u64(value.blocking_issue_count);
  encoder.boolean(value.complete);
}

template <class V, class Key>
bool unique_sorted(const V &values, Key key) {
  for (std::size_t i = 1; i < values.size(); ++i)
    if (key(values[i - 1]) == key(values[i]))
      return false;
  return true;
}

bool successful_outcome(qualification_outcome value) noexcept {
  return value == qualification_outcome::verified_exact_success ||
         value ==
             qualification_outcome::verified_certified_approximate_success;
}

bool outcome_requires_error_code(qualification_outcome value) noexcept {
  return value == qualification_outcome::expected_typed_failure ||
         value == qualification_outcome::unexpected_typed_failure ||
         value == qualification_outcome::timeout_or_resource_limit;
}

bool expected_failure_contract(qualification_outcome value) noexcept {
  return value == qualification_outcome::expected_typed_failure ||
         value == qualification_outcome::timeout_or_resource_limit;
}

std::optional<qualification_candidate_issue_kind>
issue_kind_for_outcome(qualification_outcome value) noexcept {
  switch (value) {
  case qualification_outcome::unexpected_typed_failure:
    return qualification_candidate_issue_kind::unexpected_typed_failure;
  case qualification_outcome::backend_disagreement:
    return qualification_candidate_issue_kind::backend_disagreement;
  case qualification_outcome::verifier_disagreement:
    return qualification_candidate_issue_kind::verifier_disagreement;
  case qualification_outcome::false_success:
    return qualification_candidate_issue_kind::false_success;
  case qualification_outcome::nondeterministic_outcome:
    return qualification_candidate_issue_kind::nondeterministic_outcome;
  case qualification_outcome::timeout_or_resource_limit:
    return qualification_candidate_issue_kind::timeout_or_resource_limit;
  case qualification_outcome::infrastructure_failure:
    return qualification_candidate_issue_kind::infrastructure_failure;
  default:
    return std::nullopt;
  }
}

qualification_candidate_issue_kind issue_kind_for_mismatch(
    const qualification_candidate_execution_observation &observation) noexcept {
  if (observation.outcome == qualification_outcome::expected_typed_failure)
    return qualification_candidate_issue_kind::unexpected_typed_failure;
  if (observation.outcome == qualification_outcome::timeout_or_resource_limit)
    return qualification_candidate_issue_kind::timeout_or_resource_limit;
  if (successful_outcome(observation.outcome))
    return qualification_candidate_issue_kind::false_success;
  const auto direct = issue_kind_for_outcome(observation.outcome);
  return direct.value_or(
      qualification_candidate_issue_kind::unexpected_typed_failure);
}

bool expected(
    const qualification_candidate_execution_case &descriptor,
    const qualification_candidate_execution_observation &observation) noexcept {
  if (std::find(descriptor.expected_outcomes.begin(),
                descriptor.expected_outcomes.end(), observation.outcome) ==
      descriptor.expected_outcomes.end())
    return false;
  if (!outcome_requires_error_code(observation.outcome))
    return !observation.error_code.has_value();
  return observation.error_code &&
         std::find(descriptor.expected_failure_codes.begin(),
                   descriptor.expected_failure_codes.end(),
                   *observation.error_code) !=
             descriptor.expected_failure_codes.end();
}

bool contains_digest(const std::vector<digest> &values,
                     const digest &needle) noexcept {
  return std::find(values.begin(), values.end(), needle) != values.end();
}

product_status_or<qualification_candidate_gate_binding>
canonicalize_gate(qualification_candidate_gate_binding value) {
  if (value.schema != qualification_candidate_schema_version ||
      !known(value.kind) || !text(value.identifier) || zero(value.plan_digest) ||
      zero(value.report_digest) || zero(value.independent_validation_digest) ||
      !value.complete || !value.passed)
    return candidate_error("qualification_candidate.gate_malformed");
  const auto calculated = gate_digest(value);
  if (!zero(value.binding_digest) && value.binding_digest != calculated)
    return candidate_error("qualification_candidate.gate_digest");
  value.binding_digest = calculated;
  return value;
}

product_status_or<qualification_candidate_execution_case>
canonicalize_execution_case(qualification_candidate_execution_case value) {
  if (value.schema != qualification_candidate_schema_version ||
      !text(value.identifier) || !text(value.source_identifier) ||
      zero(value.source_plan_digest) || zero(value.source_case_digest) ||
      !valid_dimension(value.dimensions) || value.expected_outcomes.empty() ||
