      if (remade.value().observation_digest != found->observation_digest)
        return performance_error(
            "qualification_performance.observation_not_canonical");
      auto stats = statistics_for(descriptor, *found);
      statistics_by_id.emplace(descriptor.identifier, stats);
      report.statistics.push_back(std::move(stats));
