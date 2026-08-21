    report.complete = requested_complete && report.blocking_issue_count == 0U &&
                      report.passed_case_count == plan.cases.size() &&
                      report.passed_threshold_count == plan.thresholds.size();

    canonical_encoder encoder;
    encode_report_semantic(encoder, report);
    report.canonical_bytes = encoder.bytes();
    report.report_digest = domain_digest(report_tag, report.canonical_bytes);
    return report;
