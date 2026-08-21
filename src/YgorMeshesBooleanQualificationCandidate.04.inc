
    if (value.disposition ==
        qualification_candidate_issue_disposition::resolved_engine_defect) {
      if (!engine_defect_kind(value.kind) || !value.regression)
        return candidate_error("qualification_candidate.defect_without_regression");
      auto regression = canonicalize_regression(std::move(*value.regression));
      if (!regression.has_value())
        return regression.error();
      value.regression = std::move(regression.value());
      if (!zero(value.reviewed_gate_or_plan_digest))
        return candidate_error("qualification_candidate.defect_unexpected_review");
    } else {
      if (value.regression)
        return candidate_error("qualification_candidate.nondefect_regression");
      if (value.disposition ==
          qualification_candidate_issue_disposition::resolved_infrastructure) {
        if (value.kind !=
                qualification_candidate_issue_kind::infrastructure_failure &&
            value.kind != qualification_candidate_issue_kind::timeout_or_resource_limit)
          return candidate_error("qualification_candidate.infrastructure_kind");
        if (!zero(value.reviewed_gate_or_plan_digest))
          return candidate_error("qualification_candidate.infrastructure_review");
      } else if (value.disposition ==
                 qualification_candidate_issue_disposition::resolved_policy_difference) {
        if (value.kind != qualification_candidate_issue_kind::backend_disagreement ||
            zero(value.reviewed_gate_or_plan_digest))
          return candidate_error("qualification_candidate.policy_resolution");
      } else if (value.disposition ==
                 qualification_candidate_issue_disposition::resolved_performance_review) {
        if ((value.kind != qualification_candidate_issue_kind::performance_regression &&
             value.kind != qualification_candidate_issue_kind::resource_regression) ||
            zero(value.reviewed_gate_or_plan_digest))
          return candidate_error("qualification_candidate.performance_resolution");
      }
    }
  }
  const auto calculated = issue_digest(value);
  if (!zero(value.issue_digest) && value.issue_digest != calculated)
    return candidate_error("qualification_candidate.issue_digest");
  value.issue_digest = calculated;
  return value;
}

} // namespace

const char *qualification_candidate_gate_kind_token(
    qualification_candidate_gate_kind value) noexcept {
  switch (value) {
  case qualification_candidate_gate_kind::permanent_corpus: return "permanent_corpus";
  case qualification_candidate_gate_kind::generation_and_chains: return "generation_and_chains";
  case qualification_candidate_gate_kind::backend_comparison: return "backend_comparison";
  case qualification_candidate_gate_kind::false_success_accounting: return "false_success_accounting";
  case qualification_candidate_gate_kind::cad_ingestion: return "cad_ingestion";
  case qualification_candidate_gate_kind::profile_suites: return "profile_suites";
  case qualification_candidate_gate_kind::platform_matrix: return "platform_matrix";
  case qualification_candidate_gate_kind::performance: return "performance";
  default: return "unknown";
  }
}

const char *qualification_candidate_issue_kind_token(
    qualification_candidate_issue_kind value) noexcept {
  switch (value) {
  case qualification_candidate_issue_kind::unexpected_typed_failure: return "unexpected_typed_failure";
  case qualification_candidate_issue_kind::backend_disagreement: return "backend_disagreement";
  case qualification_candidate_issue_kind::verifier_disagreement: return "verifier_disagreement";
  case qualification_candidate_issue_kind::false_success: return "false_success";
  case qualification_candidate_issue_kind::nondeterministic_outcome: return "nondeterministic_outcome";
  case qualification_candidate_issue_kind::timeout_or_resource_limit: return "timeout_or_resource_limit";
  case qualification_candidate_issue_kind::infrastructure_failure: return "infrastructure_failure";
  case qualification_candidate_issue_kind::performance_regression: return "performance_regression";
  case qualification_candidate_issue_kind::resource_regression: return "resource_regression";
  default: return "unknown";
  }
}

const char *qualification_candidate_issue_disposition_token(
    qualification_candidate_issue_disposition value) noexcept {
  switch (value) {
  case qualification_candidate_issue_disposition::unresolved_blocking: return "unresolved_blocking";
  case qualification_candidate_issue_disposition::resolved_engine_defect: return "resolved_engine_defect";
  case qualification_candidate_issue_disposition::resolved_infrastructure: return "resolved_infrastructure";
  case qualification_candidate_issue_disposition::resolved_policy_difference: return "resolved_policy_difference";
  case qualification_candidate_issue_disposition::resolved_performance_review: return "resolved_performance_review";
  default: return "unknown";
  }
}

product_status_or<qualification_candidate_gate_binding>
make_qualification_candidate_gate_binding(qualification_candidate_gate_binding value) {
  try { return canonicalize_gate(std::move(value)); }
  catch (const std::bad_alloc &) { return candidate_error("qualification_candidate.allocation_failure"); }
}

product_status_or<qualification_candidate_execution_case>
make_qualification_candidate_execution_case(qualification_candidate_execution_case value) {
  try { return canonicalize_execution_case(std::move(value)); }
  catch (const std::bad_alloc &) { return candidate_error("qualification_candidate.allocation_failure"); }
}

product_status_or<qualification_candidate_campaign_plan>
make_qualification_candidate_campaign_plan(qualification_candidate_campaign_plan value) {
  try {
    if (value.schema != qualification_candidate_schema_version ||
        value.checker_version != qualification_candidate_checker_version ||
        !text(value.identifier) || value.manifest_canonical_bytes.empty() ||
        zero(value.manifest_digest) || value.required_gates.size() !=
            static_cast<std::size_t>(qualification_candidate_gate_kind::count) ||
        value.execution_cases.empty())
      return candidate_error("qualification_candidate.plan_malformed");

    auto manifest = decode_qualification_campaign_manifest(value.manifest_canonical_bytes);
    if (!manifest.has_value() || manifest.value().manifest_digest != value.manifest_digest ||
        manifest.value().repository_dirty)
      return candidate_error("qualification_candidate.manifest_binding");

    for (auto &entry : value.required_gates) {
      auto made = canonicalize_gate(std::move(entry));
      if (!made.has_value()) return made.error();
      entry = std::move(made.value());
