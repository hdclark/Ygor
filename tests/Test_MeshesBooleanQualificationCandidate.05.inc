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

  auto wrong_resource_code = plan.execution_cases.front();
  wrong_resource_code.expected_outcomes = {
      qualification_outcome::timeout_or_resource_limit};
  wrong_resource_code.expected_failure_codes = {
      product_error_code::input_contract_error};
  wrong_resource_code.case_digest = {};
  require(!make_qualification_candidate_execution_case(
               std::move(wrong_resource_code))
               .has_value(),
          "resource outcomes require the resource-limit error code");

  for (const auto forbidden_code : {
           product_error_code::resource_limit,
           product_error_code::internal_invariant_error,
           product_error_code::backend_disagreement,
           product_error_code::verifier_disagreement}) {
    auto forbidden_failure = plan.execution_cases.front();
    forbidden_failure.expected_outcomes = {
        qualification_outcome::expected_typed_failure};
    forbidden_failure.expected_failure_codes = {forbidden_code};
    forbidden_failure.case_digest = {};
    require(!make_qualification_candidate_execution_case(
                 std::move(forbidden_failure))
                 .has_value(),
            "blocking or resource failures cannot be ordinary expected typed failures");
  }

  auto ambiguous_failure = plan.execution_cases.front();
  ambiguous_failure.expected_outcomes = {
      qualification_outcome::expected_typed_failure,
      qualification_outcome::timeout_or_resource_limit};
  ambiguous_failure.expected_failure_codes = {
      product_error_code::input_contract_error,
      product_error_code::resource_limit};
  ambiguous_failure.case_digest = {};
  require(!make_qualification_candidate_execution_case(
               std::move(ambiguous_failure))
               .has_value(),
          "typed and resource outcomes require distinct frozen cases");
}

void manifest_dimension_contracts() {
  const auto plan = plan_fixture();
  const auto rejected_dimension = [&](auto mutate) {
    auto unbound = plan;
    mutate(unbound.execution_cases.front().dimensions);
    unbound.execution_cases.front().case_digest = {};
    auto remade_case = make_qualification_candidate_execution_case(
        std::move(unbound.execution_cases.front()));
    require(remade_case.has_value(),
            "known unlisted execution dimension remains individually valid");
    unbound.execution_cases.front() = std::move(remade_case.value());
    unbound.canonical_bytes.clear();
    unbound.plan_digest = {};
    require(!make_qualification_candidate_campaign_plan(std::move(unbound))
                 .has_value(),
            "execution dimensions must be present in the embedded manifest");
  };

  rejected_dimension([](auto &dimensions) {
    dimensions.backend = backend_id::independent_axis_aligned_box_v1;
  });
  rejected_dimension([](auto &dimensions) {
    dimensions.representation =
        result_representation::certified_approximate_mesh;
  });
  rejected_dimension([](auto &dimensions) {
    dimensions.preparation = preparation_mode::normalized;
  });
  rejected_dimension([](auto &dimensions) {
    dimensions.coordinate = coordinate_tag::binary32;
  });
  rejected_dimension([](auto &dimensions) {
    dimensions.toolchain_identifier = "clang-unlisted";
  });
}

void canonical_state_contracts() {
  const auto plan = plan_fixture();

  auto stale_checker = plan;
  stale_checker.checker_version = 2;
  stale_checker.canonical_bytes.clear();
  stale_checker.plan_digest = {};
  require(!make_qualification_candidate_campaign_plan(std::move(stale_checker))
               .has_value(),
          "checker version two cannot claim checker version three semantics");

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
