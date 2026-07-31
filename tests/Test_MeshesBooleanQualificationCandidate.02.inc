  value.minimized_case_digest = tagged_digest('M', 100);
  value.canonical_case_bytes_digest = tagged_digest('M', 101);
  value.minimization_transcript_digest = tagged_digest('M', 102);
  value.promotion_artifact_digest = tagged_digest('M', 103);
  value.corpus_digest_before = tagged_digest('C', 100);
  value.corpus_digest_after = tagged_digest('C', 101);
  auto made = make_qualification_candidate_regression_binding(std::move(value));
  require(made.has_value(), "regression promotion binding canonicalizes");
  return made.value();
}

qualification_candidate_issue resolved_defect(
    const qualification_candidate_execution_case &descriptor,
    const qualification_candidate_execution_observation &rerun) {
  qualification_candidate_issue issue;
  issue.identifier = "issue.resolved.engine-defect";
  issue.case_identifier = descriptor.identifier;
  issue.case_digest = descriptor.case_digest;
  issue.kind = qualification_candidate_issue_kind::false_success;
  issue.initial_observation_digest = tagged_digest('I', 1);
  issue.detected_evidence_digest = tagged_digest('I', 2);
  issue.disposition =
      qualification_candidate_issue_disposition::resolved_engine_defect;
  issue.reviewer = "qualification-reviewer";
  issue.rationale = "The minimized case reproduced an engine defect.";
  issue.resolution_evidence_digest = tagged_digest('I', 3);
  issue.regression = regression_fixture();
  issue.affected_configuration_identifiers = {"gcc-strict-release-x86_64"};
  issue.rerun_observation_digests = {rerun.observation_digest};
  issue.resolution_verified = true;
  auto made = make_qualification_candidate_issue(std::move(issue));
  require(made.has_value(), "resolved engine defect canonicalizes");
  return made.value();
}

std::vector<qualification_candidate_execution_observation>
passing_observations(const qualification_candidate_campaign_plan &plan) {
  return {observation(find_execution(plan, "candidate.case.exact_success"),
                      qualification_outcome::verified_exact_success, 1),
          observation(find_execution(plan, "candidate.case.resolved_defect"),
                      qualification_outcome::verified_exact_success, 2),
          observation(find_execution(plan, "candidate.case.expected_failure"),
                      qualification_outcome::expected_typed_failure, 3)};
}

void plan_contracts() {
  const auto plan = plan_fixture();
  require_equal(plan.required_gates.size(),
                static_cast<std::size_t>(
                    qualification_candidate_gate_kind::count),
                "all P6.2-P6.9 gates are bound");
  require_equal(plan.execution_cases.size(), std::size_t(3),
                "frozen execution inventory is retained");
  require(validate_qualification_candidate_campaign_plan(plan).has_value(),
          "candidate plan independently validates");
  auto encoded = encode_qualification_candidate_campaign_plan(plan);
  require(encoded.has_value() && encoded.value() == plan.canonical_bytes,
          "candidate plan canonical bytes are reusable");

  auto bad = plan;
  bad.required_gates.pop_back();
  bad.canonical_bytes.clear();
  bad.plan_digest = {};
  require(!make_qualification_candidate_campaign_plan(std::move(bad)).has_value(),
          "missing a prior qualification gate fails closed");
}

void complete_campaign_contracts() {
  const auto plan = plan_fixture();
  auto observations = passing_observations(plan);
  std::vector<qualification_candidate_issue> issues{
      resolved_defect(find_execution(plan, "candidate.case.resolved_defect"), observations[1])};
  auto campaign = make_qualification_candidate_campaign(
      plan, observations, issues);
  require(campaign.has_value() && campaign.value().complete &&
              campaign.value().blocking_issue_count == 0 &&
              campaign.value().passed_case_count == plan.execution_cases.size() &&
              campaign.value().resolved_issue_count == 1 &&
              campaign.value().promoted_regression_count == 1 &&
              qualification_candidate_campaign_gate_passes(campaign.value(), plan),
          "complete rerun and regression evidence closes P6.10");
  require(validate_qualification_candidate_campaign(campaign.value(), plan)
              .has_value(),
          "candidate campaign independently reconstructs");

  std::reverse(observations.begin(), observations.end());
  std::reverse(issues.begin(), issues.end());
  auto reordered = make_qualification_candidate_campaign(
      plan, std::move(observations), std::move(issues));
  require(reordered.has_value() &&
              reordered.value().canonical_bytes == campaign.value().canonical_bytes &&
              reordered.value().campaign_digest == campaign.value().campaign_digest,
          "arrival order cannot affect candidate evidence");
}

void unresolved_and_missing_contracts() {
  const auto plan = plan_fixture();
  auto observations = passing_observations(plan);
  observations[0] = observation(find_execution(plan, "candidate.case.exact_success"),
                                qualification_outcome::false_success, 11);
  qualification_candidate_issue unresolved;
  unresolved.identifier = "issue.unresolved.false-success";
  unresolved.case_identifier = find_execution(plan, "candidate.case.exact_success").identifier;
  unresolved.case_digest = find_execution(plan, "candidate.case.exact_success").case_digest;
  unresolved.kind = qualification_candidate_issue_kind::false_success;
  unresolved.initial_observation_digest = observations[0].observation_digest;
  unresolved.detected_evidence_digest = tagged_digest('U', 1);
  auto made_issue = make_qualification_candidate_issue(std::move(unresolved));
  require(made_issue.has_value(), "unresolved issue remains observable");
  auto campaign = make_qualification_candidate_campaign(
      plan, observations, {made_issue.value()});
  require(campaign.has_value() && !campaign.value().complete &&
              campaign.value().blocking_issue_count != 0 &&
              !qualification_candidate_campaign_gate_passes(campaign.value(), plan),
          "unresolved false success blocks the candidate gate");

  observations = passing_observations(plan);
  observations.pop_back();
  campaign = make_qualification_candidate_campaign(plan, observations, {});
  require(campaign.has_value() && !campaign.value().complete &&
              campaign.value().blocking_issue_count == 1,
          "missing frozen execution remains a blocking issue");
}

void resolution_fail_closed_contracts() {
  const auto plan = plan_fixture();
  auto observations = passing_observations(plan);
  auto issue = resolved_defect(find_execution(plan, "candidate.case.resolved_defect"), observations[1]);
  issue.regression.reset();
  issue.issue_digest = {};
  require(!make_qualification_candidate_issue(std::move(issue)).has_value(),
          "resolved engine defect without regression promotion is rejected");

  issue = resolved_defect(find_execution(plan, "candidate.case.resolved_defect"), observations[1]);
  issue.rerun_observation_digests[0] = tagged_digest('Z', 999);
  issue.issue_digest = {};
  auto remade = make_qualification_candidate_issue(std::move(issue));
  require(remade.has_value(), "well-formed stale rerun remains inspectable");
  require(!make_qualification_candidate_campaign(
               plan, observations, {remade.value()})
