  value.type.coordinate = coordinate_tag::binary64;
  value.type.index = index_tag::uint64;
  value.workload_recipe_digest = recipe_digest_for(identifier);
  return value;
}

std::vector<qualification_performance_case> default_cases() {
  std::vector<qualification_performance_case> result;
  result.push_back(performance_case("validation.small",
      qualification_performance_workload_kind::strict_validation,
      qualification_performance_scale::small, "strict-valid-small-v1",
      "representative-valid"));
  result.push_back(performance_case("validation.adversarial",
      qualification_performance_workload_kind::strict_validation,
      qualification_performance_scale::adversarial,
      "strict-valid-adversarial-v1", "dense-thin-high-valence"));
  result.push_back(performance_case("normalization.diagnosis",
      qualification_performance_workload_kind::normalization_diagnosis,
      qualification_performance_scale::medium, "normalization-defects-v1",
      "controlled-defect-mixture"));

  const std::array<const char *, 9> repairs{{
      "structural_canonicalization", "exact_duplicate_consolidation",
      "orientation_and_shell_polarity", "seam_aware_vertex_consolidation",
      "crack_and_gap_closure", "non_planar_polygon_handling",
      "duplicate_overlap_resolution", "sliver_handling",
      "self_intersection_repair"}};
  for (std::size_t i = 0; i < repairs.size(); ++i) {
    auto value = performance_case(
        (std::string("normalization.repair.") + repairs[i]).c_str(),
        qualification_performance_workload_kind::normalization_repair,
        i + 1U == repairs.size() ? qualification_performance_scale::adversarial
                                 : qualification_performance_scale::medium,
        repairs[i], "controlled-normalization-defect");
    value.normalization_policy_identifier = repairs[i];
    result.push_back(std::move(value));
  }

  result.push_back(performance_case("backend.evaluation",
      qualification_performance_workload_kind::backend_evaluation,
      qualification_performance_scale::large, "backend-profile-large-v1",
      "representative-and-adversarial"));
  result.push_back(performance_case("topology.preflight",
      qualification_performance_workload_kind::topology_preflight,
      qualification_performance_scale::adversarial, "stratified-links-v1",
      "stratified-high-valence"));
  result.push_back(performance_case("serialization.exact_result",
      qualification_performance_workload_kind::exact_result_serialization,
      qualification_performance_scale::large, "durable-exact-result-large-v1",
      "dense-stratified-result"));
  result.push_back(performance_case("realization.exact_in_T",
      qualification_performance_workload_kind::exact_in_T_realization,
      qualification_performance_scale::medium, "finite-T-representable-v1",
      "non-dyadic-and-representable"));
  auto approximate = performance_case("realization.certified_approximate",
      qualification_performance_workload_kind::certified_approximate_realization,
      qualification_performance_scale::adversarial,
      "certified-approximate-adversarial-v1", "thin-sliver-high-valence");
  approximate.expected_outcome =
      qualification_outcome::verified_certified_approximate_success;
  result.push_back(std::move(approximate));
  result.push_back(performance_case("verification.mandatory",
      qualification_performance_workload_kind::mandatory_verification,
      qualification_performance_scale::large, "verified-output-large-v1",
      "representative-output"));
  result.push_back(performance_case("verification.exhaustive",
      qualification_performance_workload_kind::exhaustive_verification,
      qualification_performance_scale::small, "bounded-exhaustive-v1",
      "bounded-oracle"));
  result.push_back(performance_case("chain.multi_step",
      qualification_performance_workload_kind::operation_chain,
      qualification_performance_scale::large, "operation-chain-large-v1",
      "reingested-mixed-operations"));

  auto resource = performance_case("transaction.resource_limit",
      qualification_performance_workload_kind::resource_limit_transaction,
      qualification_performance_scale::adversarial, "resource-limit-v1",
      "authoritative-byte-limit");
  resource.expected_outcome = qualification_outcome::timeout_or_resource_limit;
  resource.expected_error_key = "mesh_boolean.resource_limit";
  result.push_back(std::move(resource));
  auto cancellation = performance_case("transaction.cancellation",
      qualification_performance_workload_kind::cancellation_transaction,
      qualification_performance_scale::adversarial, "mid-stage-cancellation-v1",
      "cancellation-publication-boundary");
  cancellation.expected_outcome =
      qualification_outcome::timeout_or_resource_limit;
  cancellation.expected_error_key = "mesh_boolean.cancelled";
  cancellation.cancellation_latency_limit_milliseconds = 250U;
  result.push_back(std::move(cancellation));

  const std::array<const char *, 7> families{{"B1", "B2", "B3", "B4",
                                              "B5", "B6", "B7"}};
  for (std::size_t i = 0; i < families.size(); ++i) {
    const auto identifier = std::string("baseline.") + families[i];
    auto value = performance_case(
        identifier.c_str(),
        i == 5U ? qualification_performance_workload_kind::mandatory_verification
                : qualification_performance_workload_kind::backend_evaluation,
        i == 5U ? qualification_performance_scale::large
                : qualification_performance_scale::medium,
        families[i], "plan-speed-nontrivial", operation::regularized_union);
    value.baseline_comparison_required = true;
    value.baseline_family = families[i];
    value.comparison_group = comparison_group_id;
    result.push_back(std::move(value));
  }

  for (auto &entry : result) {
    auto made = canonicalize_case(std::move(entry));
    if (!made.has_value())
      throw std::logic_error("default P6.9 case is invalid");
    entry = std::move(made.value());
  }
  std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) {
    return a.identifier < b.identifier;
  });
  return result;
}

bool complete_case_coverage(
    const std::vector<qualification_performance_case> &cases) {
  if (cases.size() != 29U)
    return false;
  std::set<qualification_performance_workload_kind> kinds;
  std::set<qualification_performance_scale> scales;
  std::set<std::string> repairs;
  std::set<std::string> baselines;
  for (const auto &entry : cases) {
    kinds.insert(entry.kind);
    scales.insert(entry.scale);
    if (entry.kind ==
        qualification_performance_workload_kind::normalization_repair)
      repairs.insert(entry.normalization_policy_identifier);
    if (entry.baseline_comparison_required) {
      if (entry.comparison_group != comparison_group_id)
        return false;
      baselines.insert(entry.baseline_family);
    }
  }
  return kinds.size() == static_cast<std::size_t>(
                             qualification_performance_workload_kind::count) &&
         scales.size() == static_cast<std::size_t>(
                              qualification_performance_scale::count) &&
         repairs.size() == 9U && baselines.size() == 7U;
}

bool review_bindings_valid(
    const std::vector<qualification_performance_threshold> &thresholds,
    const std::vector<qualification_performance_threshold_review> &reviews) {
  const auto frozen = frozen_thresholds();
  if (thresholds.size() != frozen.size())
    return false;
  std::map<std::string, qualification_performance_threshold_review> by_id;
  for (const auto &entry : reviews)
    if (!by_id.emplace(entry.threshold_identifier, entry).second)
      return false;
  std::size_t changed = 0;
  for (std::size_t i = 0; i < frozen.size(); ++i) {
    const auto &prior = frozen[i];
    const auto &next = thresholds[i];
    if (!frozen_shape_equal(prior, next))
      return false;
    const auto found = by_id.find(next.identifier);
    if (prior.threshold_digest == next.threshold_digest) {
      if (found != by_id.end())
        return false;
      continue;
    }
    ++changed;
    if (found == by_id.end() || !found->second.approved ||
        found->second.prior_threshold_digest != prior.threshold_digest ||
        found->second.revised_threshold_digest != next.threshold_digest)
      return false;
  }
