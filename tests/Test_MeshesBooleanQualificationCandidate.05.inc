namespace {

void expected_outcome_contracts() {
  const auto plan = plan_fixture();
  for (const auto forbidden_outcome : {
           qualification_outcome::unexpected_typed_failure,
           qualification_outcome::backend_disagreement,
           qualification_outcome::verifier_disagreement,
           qualification_outcome::false_success,
           qualification_outcome::nondeterministic_outcome,
           qualification_outcome::infrastructure_failure}) {
    auto forbidden = plan.execution_cases.front();
    forbidden.expected_outcomes = {forbidden_outcome};
    forbidden.expected_failure_codes.clear();
    forbidden.case_digest = {};
    require(!make_qualification_candidate_execution_case(std::move(forbidden))
                 .has_value(),
            "blocking outcome cannot be frozen as an expected result");
  }

  auto resource_limit = plan.execution_cases.front();
  resource_limit.expected_outcomes = {
      qualification_outcome::timeout_or_resource_limit};
  resource_limit.expected_failure_codes = {product_error_code::resource_limit};
  resource_limit.case_digest = {};
  require(make_qualification_candidate_execution_case(std::move(resource_limit))
              .has_value(),
          "an explicitly contracted resource-limit case remains permitted");
}

void canonical_state_contracts() {
  const auto plan = plan_fixture();

  auto stale_checker = plan;
  stale_checker.checker_version = 1;
  stale_checker.canonical_bytes.clear();
  stale_checker.plan_digest = {};
  require(!make_qualification_candidate_campaign_plan(std::move(stale_checker))
               .has_value(),
          "checker version one cannot claim checker version two semantics");

  auto reordered_gates = plan;
  std::reverse(reordered_gates.required_gates.begin(),
               reordered_gates.required_gates.end());
  auto valid_gates =
      validate_qualification_candidate_campaign_plan(reordered_gates);
  require(valid_gates.has_value() && !valid_gates.value(),
          "reordered in-memory gates cannot retain stale canonical bytes");

  auto reordered_plan = plan;
  std::reverse(reordered_plan.execution_cases.begin(),
               reordered_plan.execution_cases.end());
  auto valid_plan =
      validate_qualification_candidate_campaign_plan(reordered_plan);
  require(valid_plan.has_value() && !valid_plan.value(),
          "reordered in-memory plan cannot retain stale canonical bytes");
  require(!encode_qualification_candidate_campaign_plan(reordered_plan)
               .has_value(),
          "reordered in-memory plan cannot be encoded as canonical");

  auto made_campaign = make_qualification_candidate_campaign(
      plan, passing_observations(plan), {});
  require(made_campaign.has_value(),
          "canonical-state fixture campaign assembles");
  auto reordered_campaign = made_campaign.value();
  std::reverse(reordered_campaign.observations.begin(),
               reordered_campaign.observations.end());
  auto valid_campaign = validate_qualification_candidate_campaign(
      reordered_campaign, plan);
  require(valid_campaign.has_value() && !valid_campaign.value(),
          "reordered in-memory campaign cannot retain stale canonical bytes");
  require(!encode_qualification_candidate_campaign(reordered_campaign)
               .has_value(),
          "reordered in-memory campaign cannot be encoded as canonical");

  auto stale_counts = made_campaign.value();
  ++stale_counts.passed_case_count;
  valid_campaign = validate_qualification_candidate_campaign(stale_counts, plan);
  require(valid_campaign.has_value() && !valid_campaign.value(),
          "stale canonical bytes cannot mask mutated campaign counters");
  require(!encode_qualification_candidate_campaign(stale_counts).has_value(),
          "mutated campaign counters cannot be encoded as canonical");
}

} // namespace
