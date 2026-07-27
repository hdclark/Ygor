          descriptor->case_digest != entry.case_digest)
        return candidate_error("qualification_candidate.observation_foreign");
      observation_by_case.emplace(entry.case_identifier, &entry);
    }

    std::multimap<std::string, const qualification_candidate_issue *> issues_by_case;
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
      if (entry.disposition ==
          qualification_candidate_issue_disposition::unresolved_blocking) {
        ++result.blocking_issue_count;
      } else {
        const auto observed = observation_by_case.find(entry.case_identifier);
        if (observed == observation_by_case.end() ||
            !expected(*descriptor, *observed->second) ||
            observed->second->performance_regression_observed ||
            observed->second->resource_regression_observed ||
            !contains_digest(entry.rerun_observation_digests,
                             observed->second->observation_digest))
          return candidate_error("qualification_candidate.resolution_not_rerun");
        ++result.resolved_issue_count;
        if (entry.regression)
          ++result.promoted_regression_count;
      }
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
      if (outcome_ok && !regression) {
        ++result.passed_case_count;
        continue;
      }
      bool matching_unresolved = false;
      const auto range = issues_by_case.equal_range(descriptor.identifier);
      for (auto it = range.first; it != range.second; ++it) {
        const auto &issue = *it->second;
        if (issue.disposition !=
            qualification_candidate_issue_disposition::unresolved_blocking)
          continue;
        const auto outcome_kind = issue_kind_for_outcome(observed->second->outcome);
        if ((!outcome_ok && outcome_kind && issue.kind == *outcome_kind) ||
            (observed->second->performance_regression_observed &&
             issue.kind == qualification_candidate_issue_kind::performance_regression) ||
            (observed->second->resource_regression_observed &&
             issue.kind == qualification_candidate_issue_kind::resource_regression))
          matching_unresolved = true;
      }
      if (!matching_unresolved)
        ++result.blocking_issue_count;
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
    auto made = make_qualification_candidate_campaign(
        plan, value.observations, value.issues, value.complete);
    if (!made.has_value()) return made.error();
    const auto &rebuilt = made.value();
    return rebuilt.passed_case_count == value.passed_case_count &&
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
encode_qualification_candidate_campaign(const qualification_candidate_campaign &value) {
  if (value.canonical_bytes.empty() || zero(value.campaign_digest) ||
      domain_digest(campaign_tag, value.canonical_bytes) != value.campaign_digest)
    return candidate_error("qualification_candidate.campaign_not_canonical");
  return value.canonical_bytes;
}

bool qualification_candidate_campaign_gate_passes(
    const qualification_candidate_campaign &campaign,
    const qualification_candidate_campaign_plan &plan) noexcept {
  auto valid = validate_qualification_candidate_campaign(campaign, plan);
  return valid.has_value() && valid.value() && campaign.complete &&
         campaign.blocking_issue_count == 0U &&
         campaign.passed_case_count == plan.execution_cases.size();
}
