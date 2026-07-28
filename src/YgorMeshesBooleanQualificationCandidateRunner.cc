#include "YgorMeshesBooleanQualificationCandidateRunner.h"

#include <algorithm>
#include <array>
#include <new>
#include <optional>
#include <string>
#include <utility>

namespace ygor {
namespace mesh_boolean {
namespace {

constexpr std::array<char, 8> runner_issue_evidence_tag{
    {'Y', 'G', 'B', 'Q', 'R', 'I', '0', '1'}};

product_error runner_error(product_error_code code, const char *key) {
  return make_product_error(code, key);
}

bool expected_observation(
    const qualification_candidate_execution_case &descriptor,
    const qualification_candidate_execution_observation &observation) noexcept {
  if (std::find(descriptor.expected_outcomes.begin(),
                descriptor.expected_outcomes.end(), observation.outcome) ==
      descriptor.expected_outcomes.end())
    return false;

  const bool typed =
      observation.outcome == qualification_outcome::expected_typed_failure ||
      observation.outcome == qualification_outcome::timeout_or_resource_limit;
  if (!typed)
    return !observation.error_code.has_value();
  return observation.error_code &&
         std::find(descriptor.expected_failure_codes.begin(),
                   descriptor.expected_failure_codes.end(),
                   *observation.error_code) !=
             descriptor.expected_failure_codes.end();
}

qualification_candidate_issue_kind unexpected_issue_kind(
    qualification_outcome outcome) noexcept {
  switch (outcome) {
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
  case qualification_outcome::verified_exact_success:
  case qualification_outcome::verified_certified_approximate_success:
    return qualification_candidate_issue_kind::false_success;
  case qualification_outcome::expected_typed_failure:
  case qualification_outcome::unexpected_typed_failure:
    return qualification_candidate_issue_kind::unexpected_typed_failure;
  }
  return qualification_candidate_issue_kind::infrastructure_failure;
}

digest automatic_issue_evidence(
    const qualification_candidate_execution_observation &observation,
    qualification_candidate_issue_kind kind, std::uint64_t history_ordinal) {
  canonical_encoder encoder;
  encoder.raw(observation.case_digest.bytes.data(),
              observation.case_digest.bytes.size());
  encoder.raw(observation.observation_digest.bytes.data(),
              observation.observation_digest.bytes.size());
  encoder.byte(static_cast<std::uint8_t>(kind));
  encoder.u64(history_ordinal);
  return domain_digest(runner_issue_evidence_tag, encoder.bytes());
}

bool current_unresolved_issue_retained(
    const std::vector<qualification_candidate_issue> &issues,
    const qualification_candidate_execution_observation &observation,
    qualification_candidate_issue_kind kind) noexcept {
  return std::any_of(issues.begin(), issues.end(), [&](const auto &issue) {
    return issue.case_identifier == observation.case_identifier &&
           issue.case_digest == observation.case_digest && issue.kind == kind &&
           issue.initial_observation_digest == observation.observation_digest &&
           issue.disposition ==
               qualification_candidate_issue_disposition::unresolved_blocking;
  });
}

product_status_or<qualification_candidate_issue> make_automatic_issue(
    const qualification_candidate_execution_observation &observation,
    qualification_candidate_issue_kind kind, std::uint64_t history_ordinal) {
  qualification_candidate_issue issue;
  issue.identifier = "candidate.issue." + observation.case_digest.hex() + "." +
                     qualification_candidate_issue_kind_token(kind) + "." +
                     observation.observation_digest.hex() + "." +
                     std::to_string(history_ordinal);
  issue.case_identifier = observation.case_identifier;
  issue.case_digest = observation.case_digest;
  issue.kind = kind;
  issue.initial_observation_digest = observation.observation_digest;
  issue.detected_evidence_digest =
      automatic_issue_evidence(observation, kind, history_ordinal);
  return make_qualification_candidate_issue(std::move(issue));
}

product_status_or<std::vector<qualification_candidate_issue>>
canonicalize_issues(std::vector<qualification_candidate_issue> issues) {
  std::vector<qualification_candidate_issue> result;
  result.reserve(issues.size());
  for (auto &issue : issues) {
    auto made = make_qualification_candidate_issue(std::move(issue));
    if (!made.has_value())
      return made.error();
    result.push_back(std::move(made.value()));
  }
  return result;
}

} // namespace

product_status_or<qualification_candidate_run_result>
run_qualification_candidate_campaign(
    const qualification_candidate_campaign_plan &plan,
    qualification_candidate_case_executor executor,
    qualification_candidate_run_options options,
    qualification_candidate_issue_reconciler reconcile,
    qualification_candidate_campaign_sink sink) {
  try {
    auto valid_plan = validate_qualification_candidate_campaign_plan(plan);
    if (!valid_plan.has_value())
      return valid_plan.error();
    if (!valid_plan.value() || !executor)
      return runner_error(product_error_code::qualification_policy_violation,
                          "qualification_candidate.runner_executor_required");

    const auto cancelled = [&]() {
      return options.cancellation_requested &&
             options.cancellation_requested();
    };

    std::vector<qualification_candidate_execution_observation> observations;
    observations.reserve(plan.execution_cases.size());
    for (const auto &descriptor : plan.execution_cases) {
      if (cancelled())
        return runner_error(product_error_code::resource_limit,
                            "qualification_candidate.runner_cancelled");
      auto observed = executor(descriptor);
      if (!observed.has_value())
        return observed.error();
      auto canonical = make_qualification_candidate_execution_observation(
          std::move(observed.value()));
      if (!canonical.has_value())
        return canonical.error();
      if (canonical.value().case_identifier != descriptor.identifier ||
          canonical.value().case_digest != descriptor.case_digest)
        return runner_error(product_error_code::stale_binding,
                            "qualification_candidate.runner_case_binding");
      observations.push_back(std::move(canonical.value()));
      if (cancelled())
        return runner_error(product_error_code::resource_limit,
                            "qualification_candidate.runner_cancelled");
    }

    auto canonical_issues =
        canonicalize_issues(std::move(options.retained_issues));
    if (!canonical_issues.has_value())
      return canonical_issues.error();
    auto issues = std::move(canonical_issues.value());

    for (std::size_t i = 0; i != observations.size(); ++i) {
      const auto &descriptor = plan.execution_cases[i];
      const auto &observation = observations[i];
      std::vector<qualification_candidate_issue_kind> required;
      const auto require_issue = [&](qualification_candidate_issue_kind kind) {
        if (std::find(required.begin(), required.end(), kind) == required.end())
          required.push_back(kind);
      };
      if (!expected_observation(descriptor, observation))
        require_issue(unexpected_issue_kind(observation.outcome));
      // A frozen resource-limit contract makes the normalized outcome expected,
      // but it does not make the event issue-free. Plan 16 requires every
      // timeout/resource observation to remain retained and reviewed.
      if (observation.outcome ==
          qualification_outcome::timeout_or_resource_limit)
        require_issue(
            qualification_candidate_issue_kind::timeout_or_resource_limit);
      if (observation.performance_regression_observed)
        require_issue(
            qualification_candidate_issue_kind::performance_regression);
      if (observation.resource_regression_observed)
        require_issue(
            qualification_candidate_issue_kind::resource_regression);
      for (const auto kind : required) {
        if (current_unresolved_issue_retained(issues, observation, kind))
          continue;
        const auto history_ordinal = static_cast<std::uint64_t>(std::count_if(
            issues.begin(), issues.end(), [&](const auto &issue) {
              return issue.case_identifier == observation.case_identifier &&
                     issue.case_digest == observation.case_digest &&
                     issue.kind == kind;
            }));
        auto made =
            make_automatic_issue(observation, kind, history_ordinal);
        if (!made.has_value())
          return made.error();
        issues.push_back(std::move(made.value()));
      }
    }

    if (reconcile) {
      auto reconciled = reconcile(plan, observations, std::move(issues));
      if (!reconciled.has_value())
        return reconciled.error();
      canonical_issues = canonicalize_issues(std::move(reconciled.value()));
      if (!canonical_issues.has_value())
        return canonical_issues.error();
      issues = std::move(canonical_issues.value());
    }

    auto campaign = make_qualification_candidate_campaign(
        plan, std::move(observations), std::move(issues),
        options.requested_complete);
    if (!campaign.has_value())
      return campaign.error();

    qualification_candidate_run_result result;
    result.campaign = std::move(campaign.value());
    if (sink) {
      auto accepted = sink(plan, result.campaign);
      if (!accepted.has_value())
        return accepted.error();
      if (!accepted.value())
        return runner_error(product_error_code::qualification_policy_violation,
                            "qualification_candidate.runner_publication_rejected");
      result.published = true;
    }
    return result;
  } catch (const std::bad_alloc &) {
    return runner_error(product_error_code::resource_limit,
                        "qualification_candidate.runner_allocation_failure");
  } catch (...) {
    return runner_error(product_error_code::internal_invariant_error,
                        "qualification_candidate.runner_callback_exception");
  }
}

} // namespace mesh_boolean
} // namespace ygor
