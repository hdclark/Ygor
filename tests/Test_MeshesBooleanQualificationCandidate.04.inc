#include <YgorMeshesBooleanQualificationCandidateRunner.h>

namespace {

void runner_contracts() {
  const auto plan = plan_fixture();
  const auto passing_executor = [&]
      (const qualification_candidate_execution_case &descriptor)
      -> product_status_or<qualification_candidate_execution_observation> {
    if (descriptor.identifier == "candidate.case.expected_failure")
      return observation(descriptor,
                         qualification_outcome::expected_typed_failure, 403);
    return observation(descriptor,
                       qualification_outcome::verified_exact_success,
                       descriptor.identifier == "candidate.case.exact_success"
                           ? 401
                           : 402);
  };

  const auto anomalous_executor = [&]
      (const qualification_candidate_execution_case &descriptor)
      -> product_status_or<qualification_candidate_execution_observation> {
    if (descriptor.identifier != "candidate.case.expected_failure")
      return passing_executor(descriptor);
    auto wrong = observation(
        descriptor, qualification_outcome::expected_typed_failure, 410);
    wrong.error_code = product_error_code::resource_limit;
    wrong.observation_digest = {};
    return make_qualification_candidate_execution_observation(std::move(wrong));
  };

  std::vector<std::string> visited;
  std::uint64_t sink_calls = 0;
  auto complete = run_qualification_candidate_campaign(
      plan,
      [&](const auto &descriptor)
          -> product_status_or<qualification_candidate_execution_observation> {
        visited.push_back(descriptor.identifier);
        return passing_executor(descriptor);
      },
      {}, {},
      [&](const auto &bound_plan, const auto &campaign)
          -> product_status_or<bool> {
        ++sink_calls;
        return qualification_candidate_campaign_gate_passes(campaign,
                                                             bound_plan);
      });
  require(complete.has_value() && complete.value().campaign.complete &&
              complete.value().published && sink_calls == 1 &&
              visited.size() == plan.execution_cases.size(),
          "runner publishes one complete canonical campaign");
  for (std::size_t i = 0; i != visited.size(); ++i)
    require(visited[i] == plan.execution_cases[i].identifier,
            "runner executes the frozen canonical case order");

  auto anomalous =
      run_qualification_candidate_campaign(plan, anomalous_executor);
  require(anomalous.has_value() && !anomalous.value().campaign.complete &&
              anomalous.value().campaign.blocking_issue_count != 0 &&
              anomalous.value().campaign.issues.size() == 1 &&
              anomalous.value().campaign.issues.front().kind ==
                  qualification_candidate_issue_kind::unexpected_typed_failure,
          "runner retains an exact unresolved issue for a current anomaly");

  auto resource_plan_value = plan_fixture();
  resource_plan_value.execution_cases.push_back(execution_case(
      "candidate.case.expected_resource_limit",
      qualification_outcome::timeout_or_resource_limit, 4));
  resource_plan_value.canonical_bytes.clear();
  resource_plan_value.plan_digest = {};
  auto resource_plan_made =
      make_qualification_candidate_campaign_plan(std::move(resource_plan_value));
  require(resource_plan_made.has_value(),
          "resource-limit candidate plan canonicalizes");
  const auto resource_plan = std::move(resource_plan_made.value());
  const auto resource_executor = [&]
      (const qualification_candidate_execution_case &descriptor)
      -> product_status_or<qualification_candidate_execution_observation> {
    if (descriptor.identifier == "candidate.case.expected_resource_limit")
      return observation(
          descriptor, qualification_outcome::timeout_or_resource_limit, 420);
    return passing_executor(descriptor);
  };

  auto resource_pending =
      run_qualification_candidate_campaign(resource_plan, resource_executor);
  require(resource_pending.has_value() &&
              !resource_pending.value().campaign.complete &&
              resource_pending.value().campaign.blocking_issue_count == 1 &&
              resource_pending.value().campaign.passed_case_count ==
                  resource_plan.execution_cases.size() &&
              resource_pending.value().campaign.issues.size() == 1 &&
              resource_pending.value().campaign.issues.front().kind ==
                  qualification_candidate_issue_kind::timeout_or_resource_limit,
          "planned resource-limit outcome remains retained and blocking");
  require(!make_qualification_candidate_campaign(
               resource_plan, resource_pending.value().campaign.observations, {})
               .has_value(),
          "closure checker rejects a resource limit without a retained issue");

  qualification_candidate_run_options resource_options;
  resource_options.retained_issues =
      resource_pending.value().campaign.issues;
  auto resource_closed = run_qualification_candidate_campaign(
      resource_plan, resource_executor, std::move(resource_options),
      [&](const auto &, const auto &observations,
          std::vector<qualification_candidate_issue> issues)
          -> product_status_or<std::vector<qualification_candidate_issue>> {
        require_equal(issues.size(), std::size_t(1),
                      "one resource issue enters reconciliation");
        const auto final_observation = std::find_if(
            observations.begin(), observations.end(), [&](const auto &entry) {
              return entry.case_identifier == issues.front().case_identifier;
            });
        require(final_observation != observations.end(),
                "resource issue has a final rerun observation");
        auto &issue = issues.front();
        issue.disposition =
            qualification_candidate_issue_disposition::resolved_infrastructure;
        issue.reviewer = "candidate-resource-reviewer";
        issue.rationale =
            "The planned resource boundary was independently reviewed and rerun.";
        issue.resolution_evidence_digest = tagged_digest('I', 421);
        issue.affected_configuration_identifiers = {
            "gcc-strict-release-x86_64"};
        issue.rerun_observation_digests = {
            final_observation->observation_digest};
        issue.resolution_verified = true;
        issue.issue_digest = {};
        return issues;
      });
  require(resource_closed.has_value() &&
              resource_closed.value().campaign.complete &&
              resource_closed.value().campaign.blocking_issue_count == 0 &&
              resource_closed.value().campaign.resolved_issue_count == 1 &&
              resource_closed.value().campaign.passed_case_count ==
                  resource_plan.execution_cases.size(),
          "reviewed resource limit closes only after a bound rerun");

  qualification_candidate_run_options rerun_options;
  rerun_options.retained_issues = anomalous.value().campaign.issues;
  auto resolved = run_qualification_candidate_campaign(
      plan, passing_executor, std::move(rerun_options),
      [&](const auto &, const auto &observations,
          std::vector<qualification_candidate_issue> issues)
          -> product_status_or<std::vector<qualification_candidate_issue>> {
        require_equal(issues.size(), std::size_t(1),
                      "one retained issue enters reconciliation");
        const auto final_observation = std::find_if(
            observations.begin(), observations.end(), [&](const auto &entry) {
              return entry.case_identifier == issues.front().case_identifier;
            });
        require(final_observation != observations.end(),
                "resolved issue has a final rerun observation");
        auto &issue = issues.front();
        issue.disposition =
            qualification_candidate_issue_disposition::resolved_engine_defect;
        issue.reviewer = "candidate-runner-reviewer";
        issue.rationale = "The minimized defect was fixed and rerun.";
        issue.resolution_evidence_digest = tagged_digest('R', 450);
        issue.regression = regression_fixture();
        issue.affected_configuration_identifiers = {
            "gcc-strict-release-x86_64"};
        issue.rerun_observation_digests = {
            final_observation->observation_digest};
        issue.resolution_verified = true;
        issue.issue_digest = {};
        return issues;
      });
  require(resolved.has_value() && resolved.value().campaign.complete &&
              resolved.value().campaign.resolved_issue_count == 1 &&
              resolved.value().campaign.promoted_regression_count == 1,
          "runner closes a reviewed defect only after promotion and rerun");

  qualification_candidate_run_options recurrence_options;
  recurrence_options.retained_issues = resolved.value().campaign.issues;
  auto recurring = run_qualification_candidate_campaign(
      plan, anomalous_executor, std::move(recurrence_options));
  require(recurring.has_value(),
          "a recurring deterministic anomaly remains inspectable");
  const auto unresolved_recurrences = static_cast<std::size_t>(std::count_if(
      recurring.value().campaign.issues.begin(),
      recurring.value().campaign.issues.end(), [](const auto &issue) {
        return issue.disposition ==
               qualification_candidate_issue_disposition::unresolved_blocking;
      }));
  require(!recurring.value().campaign.complete &&
              recurring.value().campaign.issues.size() == 2 &&
              unresolved_recurrences == 1,
          "a resolved deterministic anomaly reopens as a new issue generation");

  auto multiple_anomalies = run_qualification_candidate_campaign(
      plan, [&](const auto &descriptor)
                -> product_status_or<
                    qualification_candidate_execution_observation> {
        if (descriptor.identifier != "candidate.case.exact_success")
          return passing_executor(descriptor);
        auto wrong = observation(descriptor, qualification_outcome::false_success,
                                 460);
        wrong.performance_regression_observed = true;
        wrong.regression_evidence_digest = tagged_digest('P', 460);
        wrong.observation_digest = {};
        return make_qualification_candidate_execution_observation(
            std::move(wrong));
      });
  require(multiple_anomalies.has_value() &&
              multiple_anomalies.value().campaign.issues.size() == 2,
          "runner retains one issue for every simultaneous anomaly kind");
  auto incomplete_issues = multiple_anomalies.value().campaign.issues;
  incomplete_issues.pop_back();
  require(!make_qualification_candidate_campaign(
               plan, multiple_anomalies.value().campaign.observations,
               std::move(incomplete_issues))
               .has_value(),
          "one unresolved issue cannot cover multiple current anomalies");

  std::uint64_t executions = 0;
  bool cancellation_sink_called = false;
  std::uint64_t cancellation_checks = 0;
  qualification_candidate_run_options cancelled_options;
  cancelled_options.cancellation_requested = [&] {
    return cancellation_checks++ == 1;
  };
  auto cancelled = run_qualification_candidate_campaign(
      plan,
      [&](const auto &descriptor)
          -> product_status_or<qualification_candidate_execution_observation> {
        ++executions;
        return passing_executor(descriptor);
      },
      std::move(cancelled_options), {},
      [&](const auto &, const auto &) -> product_status_or<bool> {
        cancellation_sink_called = true;
        return true;
      });
  require(!cancelled.has_value() && executions == 1 &&
              !cancellation_sink_called,
          "cancellation aborts before any campaign publication");

  bool failure_sink_called = false;
  auto infrastructure_failure = run_qualification_candidate_campaign(
      plan,
      [&](const auto &)
          -> product_status_or<qualification_candidate_execution_observation> {
        return make_product_error(product_error_code::resource_limit,
                                  "candidate.runner.fixture_failure");
      },
      {}, {},
      [&](const auto &, const auto &) -> product_status_or<bool> {
        failure_sink_called = true;
        return true;
      });
  require(!infrastructure_failure.has_value() && !failure_sink_called,
          "executor failure cannot leak a partial campaign to the sink");
}


void deferred_offline_contracts() {
  const auto plan = plan_fixture();
  const auto passing_executor = [&]
      (const qualification_candidate_execution_case &descriptor)
      -> product_status_or<qualification_candidate_execution_observation> {
    if (descriptor.identifier == "candidate.case.expected_failure")
      return observation(descriptor,
                         qualification_outcome::expected_typed_failure, 503);
    return observation(descriptor,
                       qualification_outcome::verified_exact_success,
                       descriptor.identifier == "candidate.case.exact_success"
                           ? 501
                           : 502);
  };

  qualification_candidate_run_options deferred_options;
  qualification_candidate_deferred_execution deferred;
  deferred.case_identifier = "candidate.case.expected_failure";
  deferred.reason =
      "The 24-hour fuzz-duration campaign is scheduled for offline execution.";
  deferred.evidence_digest = tagged_digest('D', 500);
  deferred_options.deferred_executions.push_back(deferred);

  std::vector<std::string> visited;
  std::uint64_t sink_calls = 0;
  auto pending = run_qualification_candidate_campaign(
      plan,
      [&](const auto &descriptor)
          -> product_status_or<qualification_candidate_execution_observation> {
        visited.push_back(descriptor.identifier);
        return passing_executor(descriptor);
      },
      deferred_options, {},
      [&](const auto &bound_plan, const auto &campaign)
          -> product_status_or<bool> {
        ++sink_calls;
        return !qualification_candidate_campaign_gate_passes(campaign,
                                                              bound_plan);
      });
  require(pending.has_value() && pending.value().published && sink_calls == 1 &&
              !pending.value().campaign.complete &&
              pending.value().campaign.blocking_issue_count == 1 &&
              pending.value().campaign.passed_case_count ==
                  plan.execution_cases.size() - 1 &&
              visited.size() == plan.execution_cases.size() - 1,
          "offline deferral publishes complete partial evidence but remains blocking");
  require(std::find(visited.begin(), visited.end(), deferred.case_identifier) ==
              visited.end(),
          "deferred campaign entry is not passed to the online executor");

  const auto deferred_observation = std::find_if(
      pending.value().campaign.observations.begin(),
      pending.value().campaign.observations.end(), [&](const auto &entry) {
        return entry.case_identifier == deferred.case_identifier;
      });
  require(deferred_observation != pending.value().campaign.observations.end() &&
              deferred_observation->outcome ==
                  qualification_outcome::infrastructure_failure &&
              deferred_observation->typed_outcome_key ==
                  "qualification_candidate.deferred_offline" &&
              pending.value().campaign.issues.size() == 1 &&
              pending.value().campaign.issues.front().kind ==
                  qualification_candidate_issue_kind::infrastructure_failure &&
              pending.value().campaign.issues.front().disposition ==
                  qualification_candidate_issue_disposition::unresolved_blocking,
          "deferred entry has canonical infrastructure evidence and an unresolved issue");

  qualification_candidate_run_options reordered_options;
  qualification_candidate_deferred_execution second;
  second.case_identifier = "candidate.case.resolved_defect";
  second.reason = "The controlled host allocation is deferred offline.";
  second.evidence_digest = tagged_digest('D', 501);
  reordered_options.deferred_executions = {second, deferred};
  auto first_order = run_qualification_candidate_campaign(
      plan, passing_executor, reordered_options);
  std::reverse(reordered_options.deferred_executions.begin(),
               reordered_options.deferred_executions.end());
  auto reverse_order = run_qualification_candidate_campaign(
      plan, passing_executor, std::move(reordered_options));
  require(first_order.has_value() && reverse_order.has_value() &&
              first_order.value().campaign.canonical_bytes ==
                  reverse_order.value().campaign.canonical_bytes,
          "deferred-option arrival order cannot change canonical campaign evidence");

  const auto rejects_before_execution = [&](auto mutate) {
    qualification_candidate_run_options invalid;
    invalid.deferred_executions = {deferred};
    mutate(invalid.deferred_executions);
    std::uint64_t executions = 0;
    auto rejected = run_qualification_candidate_campaign(
        plan,
        [&](const auto &descriptor)
            -> product_status_or<qualification_candidate_execution_observation> {
          ++executions;
          return passing_executor(descriptor);
        },
        std::move(invalid));
    require(!rejected.has_value() && executions == 0,
            "malformed deferred inventory fails before campaign execution");
  };
  rejects_before_execution([](auto &entries) {
    entries.front().case_identifier = "candidate.case.foreign";
  });
  rejects_before_execution([](auto &entries) {
    entries.push_back(entries.front());
  });
  rejects_before_execution([](auto &entries) {
    entries.front().reason.clear();
  });
  rejects_before_execution([](auto &entries) {
    entries.front().evidence_digest = {};
  });

  qualification_candidate_run_options offline_rerun;
  offline_rerun.retained_issues = pending.value().campaign.issues;
  auto closed = run_qualification_candidate_campaign(
      plan, passing_executor, std::move(offline_rerun),
      [&](const auto &, const auto &observations,
          std::vector<qualification_candidate_issue> issues)
          -> product_status_or<std::vector<qualification_candidate_issue>> {
        require_equal(issues.size(), std::size_t(1),
                      "one deferred issue enters offline reconciliation");
        const auto final_observation = std::find_if(
            observations.begin(), observations.end(), [&](const auto &entry) {
              return entry.case_identifier == issues.front().case_identifier;
            });
        require(final_observation != observations.end() &&
                    final_observation->outcome ==
                        qualification_outcome::expected_typed_failure,
                "offline rerun supplies the frozen case's allowed outcome");
        auto &issue = issues.front();
        issue.disposition =
            qualification_candidate_issue_disposition::resolved_infrastructure;
        issue.reviewer = "candidate-offline-campaign-reviewer";
        issue.rationale =
            "The deferred campaign was executed on its frozen configuration.";
        issue.resolution_evidence_digest = tagged_digest('D', 502);
        issue.affected_configuration_identifiers = {
            "gcc-strict-release-x86_64"};
        issue.rerun_observation_digests = {
            final_observation->observation_digest};
        issue.resolution_verified = true;
        issue.issue_digest = {};
        return issues;
      });
  require(closed.has_value() && closed.value().campaign.complete &&
              closed.value().campaign.blocking_issue_count == 0 &&
              closed.value().campaign.resolved_issue_count == 1 &&
              qualification_candidate_campaign_gate_passes(
                  closed.value().campaign, plan),
          "later offline execution and reviewed rerun close the deferred issue");
}

} // namespace
