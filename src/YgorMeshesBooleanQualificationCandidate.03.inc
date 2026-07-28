      value.expected_outcomes.size() > 16U ||
      value.expected_failure_codes.size() > 32U)
    return candidate_error("qualification_candidate.case_malformed");
  std::sort(value.expected_outcomes.begin(), value.expected_outcomes.end());
  value.expected_outcomes.erase(
      std::unique(value.expected_outcomes.begin(), value.expected_outcomes.end()),
      value.expected_outcomes.end());
  if (!std::all_of(value.expected_outcomes.begin(),
                   value.expected_outcomes.end(),
                   [](auto entry) { return known(entry); }))
    return candidate_error("qualification_candidate.case_outcome_unknown");
  if (!std::all_of(value.expected_outcomes.begin(),
                   value.expected_outcomes.end(), [](auto entry) {
                     return entry ==
                                qualification_outcome::verified_exact_success ||
                            entry == qualification_outcome::
                                         verified_certified_approximate_success ||
                            entry ==
                                qualification_outcome::expected_typed_failure ||
                            entry == qualification_outcome::
                                         timeout_or_resource_limit;
                   }))
    return candidate_error(
        "qualification_candidate.case_expected_outcome_forbidden");
  std::sort(value.expected_failure_codes.begin(),
            value.expected_failure_codes.end());
  value.expected_failure_codes.erase(
      std::unique(value.expected_failure_codes.begin(),
                  value.expected_failure_codes.end()),
      value.expected_failure_codes.end());
  if (!std::all_of(value.expected_failure_codes.begin(),
                   value.expected_failure_codes.end(),
                   [](auto entry) { return known(entry); }))
    return candidate_error("qualification_candidate.case_error_unknown");
  const bool failure_expected = std::any_of(
      value.expected_outcomes.begin(), value.expected_outcomes.end(),
      [](auto entry) { return expected_failure_contract(entry); });
  if (failure_expected != !value.expected_failure_codes.empty())
    return candidate_error("qualification_candidate.case_failure_contract");
  const auto calculated = execution_case_digest(value);
  if (!zero(value.case_digest) && value.case_digest != calculated)
    return candidate_error("qualification_candidate.case_digest");
  value.case_digest = calculated;
  return value;
}

product_status_or<qualification_candidate_execution_observation>
canonicalize_observation(qualification_candidate_execution_observation value) {
  if (value.schema != qualification_candidate_schema_version ||
      !text(value.case_identifier) || zero(value.case_digest) ||
      !known(value.outcome) || !value.completed ||
      outcome_requires_error_code(value.outcome) != value.error_code.has_value() ||
      (value.error_code && !known(*value.error_code)) ||
      !text(value.typed_outcome_key) || zero(value.accounting_digest) ||
      zero(value.replay_digest) || zero(value.run_log_digest))
    return candidate_error("qualification_candidate.observation_malformed");
  const bool result_present = !zero(value.canonical_result_digest);
  const bool failure_present = !zero(value.canonical_failure_digest);
  if (successful_outcome(value.outcome) != result_present ||
      successful_outcome(value.outcome) == failure_present)
    return candidate_error("qualification_candidate.observation_payload_kind");
  const bool regression = value.performance_regression_observed ||
                          value.resource_regression_observed;
  if (regression != !zero(value.regression_evidence_digest))
    return candidate_error("qualification_candidate.regression_evidence");
  const auto calculated = observation_digest(value);
  if (!zero(value.observation_digest) && value.observation_digest != calculated)
    return candidate_error("qualification_candidate.observation_digest");
  value.observation_digest = calculated;
  return value;
}

product_status_or<qualification_candidate_regression_binding>
canonicalize_regression(qualification_candidate_regression_binding value) {
  if (value.schema != qualification_candidate_schema_version ||
      !text(value.promotion_identifier) || !text(value.permanent_test_id) ||
      zero(value.minimized_case_digest) || zero(value.canonical_case_bytes_digest) ||
      zero(value.minimization_transcript_digest) ||
      zero(value.promotion_artifact_digest) || zero(value.corpus_digest_before) ||
      zero(value.corpus_digest_after) ||
      value.corpus_digest_before == value.corpus_digest_after)
    return candidate_error("qualification_candidate.regression_malformed");
  const auto calculated = regression_digest(value);
  if (!zero(value.binding_digest) && value.binding_digest != calculated)
    return candidate_error("qualification_candidate.regression_digest");
  value.binding_digest = calculated;
  return value;
}

bool engine_defect_kind(qualification_candidate_issue_kind kind) noexcept {
  return kind != qualification_candidate_issue_kind::infrastructure_failure;
}

product_status_or<qualification_candidate_issue>
canonicalize_issue(qualification_candidate_issue value) {
  if (value.schema != qualification_candidate_schema_version ||
      !text(value.identifier) || !text(value.case_identifier) ||
      zero(value.case_digest) || !known(value.kind) ||
      zero(value.initial_observation_digest) ||
      zero(value.detected_evidence_digest) || !known(value.disposition))
    return candidate_error("qualification_candidate.issue_malformed");

  if (value.disposition ==
      qualification_candidate_issue_disposition::unresolved_blocking) {
    if (!value.reviewer.empty() || !value.rationale.empty() ||
        !zero(value.resolution_evidence_digest) ||
        !zero(value.reviewed_gate_or_plan_digest) || value.regression ||
        !value.affected_configuration_identifiers.empty() ||
        !value.rerun_observation_digests.empty() || value.resolution_verified)
      return candidate_error("qualification_candidate.unresolved_has_resolution");
  } else {
    if (!text(value.reviewer) || !text(value.rationale) ||
        zero(value.resolution_evidence_digest) || !value.resolution_verified ||
        value.affected_configuration_identifiers.empty() ||
        value.affected_configuration_identifiers.size() !=
            value.rerun_observation_digests.size())
      return candidate_error("qualification_candidate.resolution_malformed");
    std::vector<std::pair<std::string, digest>> reruns;
    reruns.reserve(value.affected_configuration_identifiers.size());
    for (std::size_t i = 0;
         i != value.affected_configuration_identifiers.size(); ++i)
      reruns.emplace_back(value.affected_configuration_identifiers[i],
                          value.rerun_observation_digests[i]);
    std::sort(reruns.begin(), reruns.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });
    if (!unique_sorted(reruns, [](const auto &entry) { return entry.first; }) ||
        !std::all_of(reruns.begin(), reruns.end(), [](const auto &entry) {
          return text(entry.first) && !zero(entry.second);
        }))
      return candidate_error("qualification_candidate.rerun_malformed");
    std::set<digest> unique_rerun_digests;
    for (const auto &entry : reruns)
      unique_rerun_digests.insert(entry.second);
    if (unique_rerun_digests.size() != reruns.size())
      return candidate_error("qualification_candidate.rerun_digest_duplicate");
    value.affected_configuration_identifiers.clear();
    value.rerun_observation_digests.clear();
    for (auto &entry : reruns) {
      value.affected_configuration_identifiers.push_back(std::move(entry.first));
      value.rerun_observation_digests.push_back(entry.second);
    }
