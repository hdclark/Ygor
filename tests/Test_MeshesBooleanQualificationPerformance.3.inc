    std::vector<qualification_performance_observation> &observations,
    const std::string &case_identifier, qualification_performance_metric metric,
    std::uint64_t value) {
  const auto &descriptor = find_case(plan, case_identifier);
  const auto found = std::find_if(
      observations.begin(), observations.end(), [&](const auto &entry) {
        return entry.case_identifier == case_identifier;
      });
  require(found != observations.end(), "observation to mutate exists");
  for (auto &sample : found->candidate_samples) {
    sample.metrics[static_cast<std::size_t>(metric)] = value;
    sample.sample_digest = {};
  }
  found->observation_digest = {};
  auto remade = make_qualification_performance_observation(plan, descriptor,
                                                            std::move(*found));
  require(remade.has_value(), "mutated observation canonicalizes");
  *found = std::move(remade.value());
}

void catalog_contracts() {
  const auto plan = make_default_qualification_performance_plan();
  const auto valid_plan = validate_qualification_performance_plan(plan);
  require(valid_plan.has_value() && valid_plan.value(),
          "default P6.9 performance plan validates");
  require_equal(plan.cases.size(), std::size_t(29),
                "P6.9 workload catalog is frozen");
  require_equal(plan.thresholds.size(), std::size_t(4),
                "all plan_speed P13 thresholds are frozen");
  require_equal(plan.protocol.warmup_runs, std::uint32_t(3),
                "controlled protocol warmups are frozen");
  require_equal(plan.protocol.measured_runs, std::uint32_t(7),
                "controlled protocol measured repetitions are frozen");
  require(plan.protocol.retain_raw_samples &&
              plan.protocol.controlled_exclusive_host_required &&
              plan.protocol.cpu_affinity_required &&
              plan.protocol.peak_rss_external_observer_required &&
              plan.protocol.authoritative_accounting_separate,
          "protocol separates raw timing, external RSS, and authoritative bytes");

  std::set<qualification_performance_workload_kind> kinds;
  std::set<qualification_performance_scale> scales;
  std::set<std::string> baseline_families;
  std::set<std::string> repair_policies;
  for (const auto &descriptor : plan.cases) {
    kinds.insert(descriptor.kind);
    scales.insert(descriptor.scale);
    if (descriptor.baseline_comparison_required)
      baseline_families.insert(descriptor.baseline_family);
    if (!descriptor.normalization_policy_identifier.empty())
      repair_policies.insert(descriptor.normalization_policy_identifier);
    require_equal(descriptor.required_metrics_mask,
                  qualification_performance_all_metrics_mask,
                  "every workload records the complete P6.9 metric set");
  }
  require_equal(kinds.size(), static_cast<std::size_t>(
                                  qualification_performance_workload_kind::count),
                "every required workload stage is represented");
  require_equal(scales.size(), static_cast<std::size_t>(
                                   qualification_performance_scale::count),
                "small, medium, large, and adversarial scales are represented");
  require_equal(baseline_families.size(), std::size_t(7),
                "B1-B7 baseline families are retained");
  require_equal(repair_policies.size(), std::size_t(9),
                "each normalization repair class is benchmarked separately");

  auto encoded = encode_qualification_performance_plan(plan);
  require(encoded.has_value() && encoded.value() == plan.canonical_bytes &&
              plan.plan_digest != digest{},
          "performance plan exposes stable canonical bytes and digest");
  const auto replay = make_default_qualification_performance_plan();
  require(replay.canonical_bytes == plan.canonical_bytes &&
              replay.plan_digest == plan.plan_digest,
          "default performance plan replays byte-identically");
}

void complete_report_contracts() {
  const auto plan = make_default_qualification_performance_plan();
  auto observations = passing_observations(plan);
  auto report = make_qualification_performance_report(plan, observations);
  require(report.has_value() && report.value().complete &&
              report.value().blocking_issue_count == 0 &&
              report.value().passed_case_count == plan.cases.size() &&
              report.value().passed_threshold_count == plan.thresholds.size() &&
              qualification_performance_gate_passes(report.value(), plan),
          "complete controlled evidence passes P6.9");
  const auto valid_report =
      validate_qualification_performance_report(report.value(), plan);
