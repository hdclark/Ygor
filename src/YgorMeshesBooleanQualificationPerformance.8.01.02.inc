    for (const auto &entry : report.observations) {
      const auto known_case = std::lower_bound(
          plan.cases.begin(), plan.cases.end(), entry.case_identifier,
          [](const auto &descriptor, const std::string &identifier) {
            return descriptor.identifier < identifier;
          });
      if (known_case == plan.cases.end() ||
          known_case->identifier != entry.case_identifier)
        ++report.blocking_issue_count;
    }

    for (const auto &entry : plan.thresholds) {
      if (threshold_passes(entry, plan, statistics_by_id))
        ++report.passed_threshold_count;
      else if (entry.blocking)
        ++report.blocking_issue_count;
    }
