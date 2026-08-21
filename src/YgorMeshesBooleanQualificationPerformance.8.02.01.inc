    if (!made.has_value())
      return made.error();
    const auto &rebuilt = made.value();
    return rebuilt.statistics.size() == value.statistics.size() &&
           rebuilt.passed_case_count == value.passed_case_count &&
           rebuilt.passed_threshold_count == value.passed_threshold_count &&
           rebuilt.blocking_issue_count == value.blocking_issue_count &&
           rebuilt.complete == value.complete &&
           rebuilt.canonical_bytes == value.canonical_bytes &&
           rebuilt.report_digest == value.report_digest;
  } catch (const std::bad_alloc &) {
    return performance_error("qualification_performance.allocation_failure");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_performance_report(
