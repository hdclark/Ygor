    }
    std::sort(value.required_gates.begin(), value.required_gates.end(),
              [](const auto &a, const auto &b) { return a.kind < b.kind; });
    if (!unique_sorted(value.required_gates,
                       [](const auto &entry) { return entry.kind; }))
      return candidate_error("qualification_candidate.gate_duplicate");
    for (std::size_t i = 0; i != value.required_gates.size(); ++i)
      if (static_cast<std::size_t>(value.required_gates[i].kind) != i)
        return candidate_error("qualification_candidate.gate_coverage");

    for (auto &entry : value.execution_cases) {
      auto made = canonicalize_execution_case(std::move(entry));
      if (!made.has_value()) return made.error();
      entry = std::move(made.value());
    }
    std::sort(value.execution_cases.begin(), value.execution_cases.end(),
              [](const auto &a, const auto &b) { return a.identifier < b.identifier; });
    if (!unique_sorted(value.execution_cases,
                       [](const auto &entry) { return entry.identifier; }))
      return candidate_error("qualification_candidate.case_duplicate");

    canonical_encoder encoder;
    encode_plan_semantic(encoder, value);
    const auto bytes = encoder.bytes();
    const auto calculated = domain_digest(plan_tag, bytes);
    if ((!value.canonical_bytes.empty() && value.canonical_bytes != bytes) ||
        (!zero(value.plan_digest) && value.plan_digest != calculated))
      return candidate_error("qualification_candidate.plan_digest");
    value.canonical_bytes = bytes;
    value.plan_digest = calculated;
    return value;
  } catch (const std::bad_alloc &) {
    return candidate_error("qualification_candidate.allocation_failure");
  }
}

product_status_or<bool> validate_qualification_candidate_campaign_plan(
    const qualification_candidate_campaign_plan &value) noexcept {
  try {
    auto made = make_qualification_candidate_campaign_plan(value);
    return made.has_value() && made.value().canonical_bytes == value.canonical_bytes &&
           made.value().plan_digest == value.plan_digest;
  } catch (const std::bad_alloc &) {
    return candidate_error("qualification_candidate.allocation_failure");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_candidate_campaign_plan(
    const qualification_candidate_campaign_plan &value) {
  auto valid = validate_qualification_candidate_campaign_plan(value);
  if (!valid.has_value() || !valid.value())
    return candidate_error("qualification_candidate.plan_not_canonical");
  return value.canonical_bytes;
}

product_status_or<qualification_candidate_execution_observation>
make_qualification_candidate_execution_observation(
    qualification_candidate_execution_observation value) {
  try { return canonicalize_observation(std::move(value)); }
  catch (const std::bad_alloc &) { return candidate_error("qualification_candidate.allocation_failure"); }
}

product_status_or<qualification_candidate_regression_binding>
make_qualification_candidate_regression_binding(
    qualification_candidate_regression_binding value) {
  try { return canonicalize_regression(std::move(value)); }
  catch (const std::bad_alloc &) { return candidate_error("qualification_candidate.allocation_failure"); }
}

product_status_or<qualification_candidate_issue>
make_qualification_candidate_issue(qualification_candidate_issue value) {
  try { return canonicalize_issue(std::move(value)); }
  catch (const std::bad_alloc &) { return candidate_error("qualification_candidate.allocation_failure"); }
}

product_status_or<qualification_candidate_campaign>
make_qualification_candidate_campaign(
    const qualification_candidate_campaign_plan &plan,
    std::vector<qualification_candidate_execution_observation> observations,
    std::vector<qualification_candidate_issue> issues, bool requested_complete) {
  try {
    auto valid_plan = validate_qualification_candidate_campaign_plan(plan);
    if (!valid_plan.has_value() || !valid_plan.value())
      return candidate_error("qualification_candidate.plan_invalid");
    for (auto &entry : observations) {
      auto made = canonicalize_observation(std::move(entry));
      if (!made.has_value()) return made.error();
      entry = std::move(made.value());
    }
    std::sort(observations.begin(), observations.end(),
              [](const auto &a, const auto &b) { return a.case_identifier < b.case_identifier; });
    if (!unique_sorted(observations,
                       [](const auto &entry) { return entry.case_identifier; }))
      return candidate_error("qualification_candidate.observation_duplicate");
    for (auto &entry : issues) {
      auto made = canonicalize_issue(std::move(entry));
      if (!made.has_value()) return made.error();
      entry = std::move(made.value());
    }
    std::sort(issues.begin(), issues.end(),
              [](const auto &a, const auto &b) { return a.identifier < b.identifier; });
    if (!unique_sorted(issues, [](const auto &entry) { return entry.identifier; }))
      return candidate_error("qualification_candidate.issue_duplicate");

    qualification_candidate_campaign result;
    result.identifier = plan.identifier + ".campaign";
    result.plan_digest = plan.plan_digest;
    result.observations = std::move(observations);
    result.issues = std::move(issues);

    std::map<std::string, const qualification_candidate_execution_observation *> observation_by_case;
    for (const auto &entry : result.observations) {
      const auto descriptor = std::lower_bound(
          plan.execution_cases.begin(), plan.execution_cases.end(),
          entry.case_identifier, [](const auto &candidate, const std::string &id) {
            return candidate.identifier < id;
          });
      if (descriptor == plan.execution_cases.end() ||
          descriptor->identifier != entry.case_identifier ||
