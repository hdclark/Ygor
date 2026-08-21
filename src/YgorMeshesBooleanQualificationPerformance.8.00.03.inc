      return performance_error("qualification_performance.plan_not_canonical");

    std::sort(observations.begin(), observations.end(), [](const auto &a,
                                                           const auto &b) {
      return a.case_identifier < b.case_identifier;
    });
    for (std::size_t i = 1; i < observations.size(); ++i)
      if (observations[i - 1].case_identifier ==
