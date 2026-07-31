          observations[i].case_identifier)
        return performance_error("qualification_performance.observation_duplicate");

    qualification_performance_report report;
    report.identifier = plan.identifier + ".report";
    report.plan_digest = plan.plan_digest;
    report.observations = std::move(observations);
    std::map<std::string, qualification_performance_case_statistics>
        statistics_by_id;

    for (const auto &descriptor : plan.cases) {
      const auto found = std::lower_bound(
          report.observations.begin(), report.observations.end(),
          descriptor.identifier, [](const auto &entry, const std::string &id) {
            return entry.case_identifier < id;
          });
