               .has_value(),
          "resolution not bound to the final rerun is rejected");

  auto wrong_failure = observation(
      find_execution(plan, "candidate.case.expected_failure"),
      qualification_outcome::expected_typed_failure, 33);
  wrong_failure.error_code = product_error_code::resource_limit;
  wrong_failure.observation_digest = {};
  auto made_wrong_failure =
      make_qualification_candidate_execution_observation(wrong_failure);
  require(made_wrong_failure.has_value(),
          "alternate typed failure remains individually observable");
  observations = passing_observations(plan);
  observations[2] = made_wrong_failure.value();
  auto wrong_failure_campaign =
      make_qualification_candidate_campaign(plan, observations, {});
  require(wrong_failure_campaign.has_value() &&
              !wrong_failure_campaign.value().complete &&
              wrong_failure_campaign.value().blocking_issue_count != 0,
          "wrong typed failure code cannot satisfy the frozen case contract");

  observations = passing_observations(plan);
  auto stale = observations.front();
  stale.case_digest = tagged_digest('Z', 1);
  stale.observation_digest = {};
  auto made_stale = make_qualification_candidate_execution_observation(stale);
  require(made_stale.has_value(), "stale observation is individually canonical");
  observations.front() = made_stale.value();
  require(!make_qualification_candidate_campaign(plan, observations, {}).has_value(),
          "foreign case binding cannot enter the campaign");
}

void performance_review_contracts() {
  const auto plan = plan_fixture();
  auto observations = passing_observations(plan);
  auto regressed = observations[0];
  regressed.performance_regression_observed = true;
  regressed.regression_evidence_digest = tagged_digest('P', 900);
  regressed.observation_digest = {};
  auto made_regressed =
      make_qualification_candidate_execution_observation(regressed);
  require(made_regressed.has_value(), "performance regression is observable");
  observations[0] = made_regressed.value();

  qualification_candidate_issue unresolved;
  unresolved.identifier = "issue.performance.blocking";
  unresolved.case_identifier = find_execution(plan, "candidate.case.exact_success").identifier;
  unresolved.case_digest = find_execution(plan, "candidate.case.exact_success").case_digest;
  unresolved.kind = qualification_candidate_issue_kind::performance_regression;
  unresolved.initial_observation_digest = observations[0].observation_digest;
  unresolved.detected_evidence_digest = tagged_digest('P', 901);
  auto made = make_qualification_candidate_issue(std::move(unresolved));
  require(made.has_value(), "performance issue canonicalizes");
  auto campaign = make_qualification_candidate_campaign(
      plan, observations, {made.value()});
  require(campaign.has_value() && !campaign.value().complete,
          "unreviewed performance regression blocks P6.10");
}

} // namespace

int main() {
  harness tests;
  tests.add("P6.10.plan", [] { plan_contracts(); });
  tests.add("P6.10.complete", [] { complete_campaign_contracts(); });
  tests.add("P6.10.unresolved", [] { unresolved_and_missing_contracts(); });
  tests.add("P6.10.resolution", [] { resolution_fail_closed_contracts(); });
  tests.add("P6.10.performance", [] { performance_review_contracts(); });
  return tests.run(std::cout, std::cerr);
}
