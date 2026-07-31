  require(valid_report.has_value() && valid_report.value(),
          "performance report independently reconstructs");
  auto encoded = encode_qualification_performance_report(report.value());
  require(encoded.has_value() && encoded.value() == report.value().canonical_bytes,
          "performance report exposes canonical bytes");

  std::reverse(observations.begin(), observations.end());
  auto reordered =
      make_qualification_performance_report(plan, std::move(observations));
  require(reordered.has_value() &&
              reordered.value().canonical_bytes == report.value().canonical_bytes &&
              reordered.value().report_digest == report.value().report_digest,
          "observation arrival order cannot affect performance evidence");
}

void threshold_gate_contracts() {
  const auto plan = make_default_qualification_performance_plan();

  auto geomean = passing_observations(plan);
  set_candidate_metric(plan, geomean, "baseline.B2",
                       qualification_performance_metric::wall_time_nanoseconds,
                       500);
  set_candidate_metric(plan, geomean, "baseline.B3",
                       qualification_performance_metric::wall_time_nanoseconds,
                       500);
  auto geomean_report =
      make_qualification_performance_report(plan, std::move(geomean));
  require(geomean_report.has_value() && !geomean_report.value().complete &&
              geomean_report.value().blocking_issue_count == 1 &&
              !qualification_performance_gate_passes(geomean_report.value(), plan),
          "B1-B7 geometric-mean regression blocks qualification");

  auto b6 = passing_observations(plan);
  set_candidate_metric(plan, b6, "baseline.B6",
                       qualification_performance_metric::wall_time_nanoseconds,
                       130);
  auto b6_report = make_qualification_performance_report(plan, std::move(b6));
  require(b6_report.has_value() && !b6_report.value().complete &&
              b6_report.value().blocking_issue_count == 1,
          "B6 below the frozen five-times target blocks qualification");

  auto slowdown = passing_observations(plan);
  set_candidate_metric(plan, slowdown, "baseline.B1",
                       qualification_performance_metric::wall_time_nanoseconds,
                       600);
  set_candidate_metric(plan, slowdown, "baseline.B2",
                       qualification_performance_metric::wall_time_nanoseconds,
                       50);
  auto slowdown_report =
      make_qualification_performance_report(plan, std::move(slowdown));
  require(slowdown_report.has_value() && !slowdown_report.value().complete &&
              slowdown_report.value().blocking_issue_count == 1,
          "an unreviewed per-case slowdown above fifteen percent blocks");

  auto bytes = passing_observations(plan);
  set_candidate_metric(plan, bytes, "baseline.B4",
                       qualification_performance_metric::authoritative_bytes,
                       1001);
  auto bytes_report =
      make_qualification_performance_report(plan, std::move(bytes));
  require(bytes_report.has_value() && !bytes_report.value().complete &&
              bytes_report.value().blocking_issue_count == 1,
          "authoritative-byte growth blocks qualification");
}

void threshold_review_contracts() {
  auto plan = make_default_qualification_performance_plan();
  auto &b6 = find_threshold(plan, "plan_speed.p13.b6_speedup");
  const auto prior_digest = b6.threshold_digest;
  b6.numerator = 4;
  b6.threshold_digest = {};
  auto revised = make_qualification_performance_threshold(b6);
  require(revised.has_value(), "revised threshold canonicalizes");
  b6 = std::move(revised.value());

  qualification_performance_threshold_review review;
  review.threshold_identifier = b6.identifier;
  review.prior_threshold_digest = prior_digest;
  review.revised_threshold_digest = b6.threshold_digest;
  review.reviewer = "performance-review-board";
  review.rationale =
      "controlled evidence approved a revised B6 target without weakening exactness";
  review.evidence_digest = tagged_digest("reviewed-b6-evidence");
  review.approved = true;
  auto made_review =
      make_qualification_performance_threshold_review(std::move(review));
  require(made_review.has_value(), "approved threshold review canonicalizes");
