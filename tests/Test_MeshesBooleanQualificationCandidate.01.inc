  resources.policy_digest = tagged_digest('R', 1);
  resources.wall_timeout_milliseconds = 600000;
  resources.authoritative_byte_limit = 1024ULL * 1024ULL * 1024ULL;
  resources.work_unit_limit = 100000000;
  resources.cancellation_latency_limit_milliseconds = 1000;
  manifest.resource_policies.push_back(resources);

  manifest.performance.identifier = "controlled-exclusive-host-v1";
  manifest.performance.version = "1";
  manifest.performance.hardware_identifier = "controlled-hardware";
  manifest.performance.hardware_digest = tagged_digest('Q', 1);
  manifest.performance.measurement_protocol_digest = tagged_digest('Q', 2);
  manifest.performance.warmup_runs = 3;
  manifest.performance.measured_runs = 7;
  manifest.performance.controlled_exclusive_host = true;

  qualification_threshold threshold;
  threshold.metric = "false_success_count";
  threshold.relation = qualification_threshold_relation::equal;
  threshold.numerator = 0;
  threshold.denominator = 1;
  threshold.unit = "count";
  manifest.thresholds.push_back(threshold);
  manifest.exclusions = {"unknown-provenance-inputs"};
  manifest.known_limitations = {"candidate evidence does not promote a default"};

  auto made = make_qualification_campaign_manifest(std::move(manifest));
  require(made.has_value(), "candidate manifest canonicalizes");
  return made.value();
}

qualification_candidate_gate_binding gate(
    qualification_candidate_gate_kind kind, std::uint64_t ordinal) {
  qualification_candidate_gate_binding value;
  value.kind = kind;
  value.identifier = qualification_candidate_gate_kind_token(kind);
  value.plan_digest = tagged_digest('A', ordinal);
  value.report_digest = tagged_digest('B', ordinal);
  value.independent_validation_digest = tagged_digest('C', ordinal);
  value.complete = true;
  value.passed = true;
  auto made = make_qualification_candidate_gate_binding(std::move(value));
  require(made.has_value(), "prior qualification gate canonicalizes");
  return made.value();
}

qualification_candidate_execution_case execution_case(
    std::string identifier, qualification_outcome expected_outcome,
    std::uint64_t ordinal) {
  qualification_candidate_execution_case value;
  value.identifier = std::move(identifier);
  value.source_identifier = "frozen-source-case-" + std::to_string(ordinal);
  value.source_plan_digest = tagged_digest('S', ordinal);
  value.source_case_digest = tagged_digest('K', ordinal);
  value.dimensions.backend = backend_id::experimental_exact_v1;
  value.dimensions.representation = result_representation::exact_stratified;
  value.dimensions.preparation = preparation_mode::strict_validation;
  value.dimensions.selected_operation = operation::regularized_union;
  value.dimensions.coordinate = coordinate_tag::binary64;
  value.dimensions.index = index_tag::uint64;
  value.dimensions.toolchain_identifier = "gcc-strict-release-x86_64";
  value.dimensions.geometry_category = "candidate-category-" +
                                       std::to_string(ordinal);
  value.expected_outcomes = {expected_outcome};
  if (expected_outcome == qualification_outcome::expected_typed_failure)
    value.expected_failure_codes = {product_error_code::input_contract_error};
  else if (expected_outcome ==
           qualification_outcome::timeout_or_resource_limit)
    value.expected_failure_codes = {product_error_code::resource_limit};
  auto made = make_qualification_candidate_execution_case(std::move(value));
  require(made.has_value(), "candidate execution case canonicalizes");
  return made.value();
}

qualification_candidate_campaign_plan plan_fixture() {
  const auto manifest = manifest_fixture();
  auto encoded = encode_qualification_campaign_manifest(manifest);
  require(encoded.has_value(), "candidate manifest encodes");

  qualification_candidate_campaign_plan plan;
  plan.identifier = "p6.10-candidate-campaign-v1";
  plan.manifest_canonical_bytes = std::move(encoded.value());
  plan.manifest_digest = manifest.manifest_digest;
  for (std::uint64_t i = 0;
       i != static_cast<std::uint64_t>(qualification_candidate_gate_kind::count);
       ++i)
    plan.required_gates.push_back(
        gate(static_cast<qualification_candidate_gate_kind>(i), i + 1U));
  plan.execution_cases = {
      execution_case("candidate.case.exact_success",
                     qualification_outcome::verified_exact_success, 1),
      execution_case("candidate.case.resolved_defect",
                     qualification_outcome::verified_exact_success, 2),
      execution_case("candidate.case.expected_failure",
                     qualification_outcome::expected_typed_failure, 3)};
  auto made = make_qualification_candidate_campaign_plan(std::move(plan));
  require(made.has_value(), "candidate campaign plan canonicalizes");
  return made.value();
}


const qualification_candidate_execution_case &find_execution(
    const qualification_candidate_campaign_plan &plan, const std::string &id) {
  const auto found = std::find_if(
      plan.execution_cases.begin(), plan.execution_cases.end(),
      [&](const auto &entry) { return entry.identifier == id; });
  require(found != plan.execution_cases.end(), "candidate execution case exists");
  return *found;
}

qualification_candidate_execution_observation observation(
    const qualification_candidate_execution_case &descriptor,
    qualification_outcome outcome, std::uint64_t ordinal) {
  qualification_candidate_execution_observation value;
  value.case_identifier = descriptor.identifier;
  value.case_digest = descriptor.case_digest;
  value.outcome = outcome;
  if (outcome == qualification_outcome::expected_typed_failure)
    value.error_code = product_error_code::input_contract_error;
  else if (outcome == qualification_outcome::unexpected_typed_failure)
    value.error_code = product_error_code::internal_invariant_error;
  else if (outcome == qualification_outcome::timeout_or_resource_limit)
    value.error_code = product_error_code::resource_limit;
  value.completed = true;
  value.typed_outcome_key = "candidate.outcome." + std::to_string(ordinal);
  value.accounting_digest = tagged_digest('A', ordinal + 100);
  if (outcome == qualification_outcome::verified_exact_success ||
      outcome == qualification_outcome::verified_certified_approximate_success)
    value.canonical_result_digest = tagged_digest('O', ordinal);
  else
    value.canonical_failure_digest = tagged_digest('X', ordinal);
  value.replay_digest = tagged_digest('R', ordinal + 100);
  value.run_log_digest = tagged_digest('L', ordinal + 100);
  auto made = make_qualification_candidate_execution_observation(
      std::move(value));
  require(made.has_value(), "candidate observation canonicalizes");
  return made.value();
}

qualification_candidate_regression_binding regression_fixture() {
  qualification_candidate_regression_binding value;
  value.promotion_identifier = "p6.10-regression-promotion-1";
  value.permanent_test_id = "C14.P610.regression.1";
