          descriptor->case_digest != entry.case_digest)
        return candidate_error("qualification_candidate.observation_foreign");
      observation_by_case.emplace(entry.case_identifier, &entry);
    }

    std::multimap<std::string, const qualification_candidate_issue *>
        issues_by_case;
    for (const auto &entry : result.issues) {
      const auto descriptor = std::lower_bound(
          plan.execution_cases.begin(), plan.execution_cases.end(),
          entry.case_identifier, [](const auto &candidate, const std::string &id) {
            return candidate.identifier < id;
          });
      if (descriptor == plan.execution_cases.end() ||
          descriptor->identifier != entry.case_identifier ||
          descriptor->case_digest != entry.case_digest)
        return candidate_error("qualification_candidate.issue_foreign");
      issues_by_case.emplace(entry.case_identifier, &entry);
    }

    const auto has_unresolved_kind =
        [&](const qualification_candidate_execution_case &descriptor,
            const qualification_candidate_execution_observation &observation,
            qualification_candidate_issue_kind kind) {
          const auto range = issues_by_case.equal_range(descriptor.identifier);
          return std::any_of(range.first, range.second, [&](const auto &entry) {
            const auto &issue = *entry.second;
            return issue.disposition ==
                       qualification_candidate_issue_disposition::
                           unresolved_blocking &&
                   issue.initial_observation_digest ==
                       observation.observation_digest &&
                   issue.kind == kind;
          });
        };

    const auto has_current_timeout_issue =
        [&](const qualification_candidate_execution_case &descriptor,
            const qualification_candidate_execution_observation &observation) {
          if (observation.outcome !=
              qualification_outcome::timeout_or_resource_limit)
            return true;
          const auto range = issues_by_case.equal_range(descriptor.identifier);
          return std::any_of(range.first, range.second, [&](const auto &entry) {
            const auto &issue = *entry.second;
            if (issue.kind !=
                qualification_candidate_issue_kind::timeout_or_resource_limit)
              return false;
            if (issue.disposition ==
                qualification_candidate_issue_disposition::unresolved_blocking)
              return issue.initial_observation_digest ==
                     observation.observation_digest;
            return contains_digest(issue.rerun_observation_digests,
                                   observation.observation_digest);
          });
        };

    const auto has_all_current_issues =
        [&](const qualification_candidate_execution_case &descriptor,
            const qualification_candidate_execution_observation &observation) {
          if (!has_current_timeout_issue(descriptor, observation))
            return false;
          const bool outcome_ok = expected(descriptor, observation);
          if (!outcome_ok &&
              !has_unresolved_kind(descriptor, observation,
                                   issue_kind_for_mismatch(observation)))
            return false;
          if (observation.performance_regression_observed &&
              !has_unresolved_kind(
                  descriptor, observation,
                  qualification_candidate_issue_kind::performance_regression))
            return false;
          if (observation.resource_regression_observed &&
              !has_unresolved_kind(
                  descriptor, observation,
                  qualification_candidate_issue_kind::resource_regression))
            return false;
          return true;
        };

    for (const auto &entry : result.issues) {
      if (entry.disposition ==
          qualification_candidate_issue_disposition::unresolved_blocking) {
        ++result.blocking_issue_count;
        continue;
      }

      const auto descriptor = std::lower_bound(
          plan.execution_cases.begin(), plan.execution_cases.end(),
          entry.case_identifier, [](const auto &candidate, const std::string &id) {
            return candidate.identifier < id;
          });
      const auto observed = observation_by_case.find(entry.case_identifier);
      if (descriptor == plan.execution_cases.end() ||
          observed == observation_by_case.end())
        return candidate_error("qualification_candidate.resolution_not_rerun");

      const bool outcome_ok = expected(*descriptor, *observed->second);
      const bool current_anomaly =
          !outcome_ok || observed->second->performance_regression_observed ||
          observed->second->resource_regression_observed;
      const bool resolution_current =
          outcome_ok && !observed->second->performance_regression_observed &&
          !observed->second->resource_regression_observed &&
          contains_digest(entry.rerun_observation_digests,
                          observed->second->observation_digest);
      if (!resolution_current &&
          (!current_anomaly ||
           !has_all_current_issues(*descriptor, *observed->second)))
        return candidate_error("qualification_candidate.resolution_not_rerun");

      ++result.resolved_issue_count;
      if (entry.regression)
        ++result.promoted_regression_count;
    }

    for (const auto &descriptor : plan.execution_cases) {
      const auto observed = observation_by_case.find(descriptor.identifier);
      if (observed == observation_by_case.end()) {
        ++result.blocking_issue_count;
        continue;
      }
      const bool outcome_ok = expected(descriptor, *observed->second);
      const bool regression = observed->second->performance_regression_observed ||
                              observed->second->resource_regression_observed;
      const bool current_issues_complete =
          has_all_current_issues(descriptor, *observed->second);
      if (outcome_ok && !regression) {
        if (!current_issues_complete)
          return candidate_error("qualification_candidate.current_issue_missing");
        ++result.passed_case_count;
        continue;
      }
      if (!current_issues_complete)
        return candidate_error("qualification_candidate.current_issue_missing");
    }

    result.complete = requested_complete && result.blocking_issue_count == 0U &&
                      result.passed_case_count == plan.execution_cases.size();
    canonical_encoder encoder;
    encode_campaign_semantic(encoder, result);
    result.canonical_bytes = encoder.bytes();
    result.campaign_digest = domain_digest(campaign_tag, result.canonical_bytes);
    return result;
  } catch (const std::bad_alloc &) {
    return candidate_error("qualification_candidate.allocation_failure");
  }
}

product_status_or<bool> validate_qualification_candidate_campaign(
    const qualification_candidate_campaign &value,
    const qualification_candidate_campaign_plan &plan) noexcept {
  try {
    if (value.schema != qualification_candidate_schema_version ||
        value.checker_version != qualification_candidate_checker_version ||
        value.identifier != plan.identifier + ".campaign" ||
        value.plan_digest != plan.plan_digest)
      return candidate_error("qualification_candidate.campaign_malformed");
    canonical_encoder supplied_encoder;
    encode_campaign_semantic(supplied_encoder, value);
    const auto supplied_bytes = supplied_encoder.bytes();
    auto made = make_qualification_candidate_campaign(
        plan, value.observations, value.issues, value.complete);
    if (!made.has_value())
      return made.error();
    const auto &rebuilt = made.value();
    return supplied_bytes == value.canonical_bytes &&
           rebuilt.passed_case_count == value.passed_case_count &&
           rebuilt.resolved_issue_count == value.resolved_issue_count &&
           rebuilt.promoted_regression_count == value.promoted_regression_count &&
           rebuilt.blocking_issue_count == value.blocking_issue_count &&
           rebuilt.complete == value.complete &&
           rebuilt.canonical_bytes == value.canonical_bytes &&
           rebuilt.campaign_digest == value.campaign_digest;
  } catch (const std::bad_alloc &) {
    return candidate_error("qualification_candidate.allocation_failure");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_candidate_campaign(
    const qualification_candidate_campaign &value) {
  try {
    canonical_encoder supplied_encoder;
    encode_campaign_semantic(supplied_encoder, value);
    const auto supplied_bytes = supplied_encoder.bytes();
    if (value.canonical_bytes.empty() || zero(value.campaign_digest) ||
        supplied_bytes != value.canonical_bytes ||
        domain_digest(campaign_tag, value.canonical_bytes) != value.campaign_digest)
      return candidate_error("qualification_candidate.campaign_not_canonical");
    return value.canonical_bytes;
  } catch (const std::bad_alloc &) {
    return candidate_error("qualification_candidate.allocation_failure");
  }
}

bool qualification_candidate_campaign_gate_passes(
    const qualification_candidate_campaign &campaign,
    const qualification_candidate_campaign_plan &plan) noexcept {
  auto valid = validate_qualification_candidate_campaign(campaign, plan);
  return valid.has_value() && valid.value() && campaign.complete &&
         campaign.blocking_issue_count == 0U &&
         campaign.passed_case_count == plan.execution_cases.size();
}

} // namespace mesh_boolean
} // namespace ygor
