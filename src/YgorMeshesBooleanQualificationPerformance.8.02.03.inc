  return valid.has_value() && valid.value() && report.complete &&
         report.blocking_issue_count == 0U &&
         report.passed_case_count == plan.cases.size() &&
         report.passed_threshold_count == plan.thresholds.size();
}

} // namespace mesh_boolean
} // namespace ygor
