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

} // namespace
