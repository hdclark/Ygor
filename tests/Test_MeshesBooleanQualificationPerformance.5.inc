  plan.threshold_reviews.push_back(std::move(made_review.value()));
  plan.canonical_bytes.clear();
  plan.plan_digest = {};
  auto made_plan = make_qualification_performance_plan(std::move(plan));
  require(made_plan.has_value(), "digest-bound reviewed threshold is accepted");
  plan = std::move(made_plan.value());

  auto observations = passing_observations(plan);
  set_candidate_metric(plan, observations, "baseline.B6",
                       qualification_performance_metric::wall_time_nanoseconds,
                       130);
  auto report =
      make_qualification_performance_report(plan, std::move(observations));
  require(report.has_value() &&
              qualification_performance_gate_passes(report.value(), plan),
          "an explicitly reviewed revised target is enforced exactly");

  auto rejected = make_default_qualification_performance_plan();
  auto &rejected_b6 =
      find_threshold(rejected, "plan_speed.p13.b6_speedup");
  rejected_b6.numerator = 4;
  rejected_b6.threshold_digest = {};
  auto rejected_threshold =
      make_qualification_performance_threshold(rejected_b6);
  require(rejected_threshold.has_value(), "unreviewed threshold is well formed");
  rejected_b6 = std::move(rejected_threshold.value());
  rejected.canonical_bytes.clear();
  rejected.plan_digest = {};
  require(!make_qualification_performance_plan(std::move(rejected)).has_value(),
          "silent revision of a frozen plan_speed target fails closed");
}

void transaction_and_sample_contracts() {
  const auto plan = make_default_qualification_performance_plan();
  const auto cancellation = find_case(plan, "transaction.cancellation");
  auto observation = passing_observation(plan, cancellation);
  observation.partial_publication_observed = true;
  observation.observation_digest = {};
  require(!make_qualification_performance_observation(
               plan, cancellation, std::move(observation))
               .has_value(),
          "partial publication under cancellation is rejected");

  observation = passing_observation(plan, cancellation);
  for (auto &sample : observation.candidate_samples) {
    sample.metrics[static_cast<std::size_t>(
        qualification_performance_metric::cancellation_latency_milliseconds)] =
        cancellation.cancellation_latency_limit_milliseconds + 1U;
    sample.sample_digest = {};
  }
  observation.observation_digest = {};
  require(!make_qualification_performance_observation(
               plan, cancellation, std::move(observation))
               .has_value(),
          "cancellation latency above the frozen limit is rejected");

  const auto validation = find_case(plan, "validation.small");
  observation = passing_observation(plan, validation);
  observation.candidate_samples.front().metric_presence_mask &=
      ~qualification_performance_metric_bit(
          qualification_performance_metric::peak_rss_bytes);
  observation.candidate_samples.front().sample_digest = {};
  observation.observation_digest = {};
  require(!make_qualification_performance_observation(
               plan, validation, std::move(observation))
               .has_value(),
          "missing required RSS evidence fails closed");

  observation = passing_observation(plan, validation);
  observation.candidate_samples.pop_back();
  observation.observation_digest = {};
  require(!make_qualification_performance_observation(
               plan, validation, std::move(observation))
               .has_value(),
          "a shortened raw-sample set cannot masquerade as the frozen protocol");
}

} // namespace

int main() {
  harness tests;
  tests.add("P6.9.catalog", [] { catalog_contracts(); });
  tests.add("P6.9.report", [] { complete_report_contracts(); });
  tests.add("P6.9.thresholds", [] { threshold_gate_contracts(); });
  tests.add("P6.9.reviews", [] { threshold_review_contracts(); });
  tests.add("P6.9.transaction", [] { transaction_and_sample_contracts(); });
  return tests.run(std::cout, std::cerr);
}
