      if (found == report.observations.end() ||
          found->case_identifier != descriptor.identifier) {
        ++report.blocking_issue_count;
        continue;
      }
      auto remade = make_qualification_performance_observation(
          plan, descriptor, *found);
      if (!remade.has_value())
        return remade.error();
