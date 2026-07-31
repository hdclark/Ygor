#include <YgorMeshesBooleanQualificationMatrix.h>

#include <set>

namespace {

void default_matrix_deferred_inventory_contracts() {
  const auto matrix = make_default_qualification_matrix_plan();
  require_equal(matrix.fuzz_cases.size(), std::size_t(8),
                "the frozen P6.8 plan retains all eight fuzz-duration entries");

  auto manifest_value = manifest_fixture();
  std::set<std::string> manifest_toolchains;
  for (const auto &toolchain : manifest_value.toolchains)
    manifest_toolchains.insert(toolchain.identifier);

  std::uint64_t ordinal = 0;
  for (const auto &fuzz_case : matrix.fuzz_cases) {
    if (!manifest_toolchains.insert(fuzz_case.configuration_identifier).second)
      continue;
    auto toolchain = manifest_value.toolchains.front();
    toolchain.identifier = fuzz_case.configuration_identifier;
    toolchain.compiler_name =
        fuzz_case.configuration_identifier.find("clang") != std::string::npos
            ? "Clang"
            : "GCC";
    toolchain.compiler_version = "frozen-offline-configuration";
    toolchain.build_type = "Qualification";
    toolchain.compile_flags = {"-std=c++17", "-fno-fast-math",
                               "-ffp-contract=off"};
    if (fuzz_case.sanitizer ==
        qualification_sanitizer_mode::address_and_undefined)
      toolchain.compile_flags.push_back("-fsanitize=address,undefined");
    else if (fuzz_case.sanitizer == qualification_sanitizer_mode::thread)
      toolchain.compile_flags.push_back("-fsanitize=thread");
    toolchain.environment_digest = tagged_digest('E', 600U + ordinal);
    manifest_value.toolchains.push_back(std::move(toolchain));
    ++ordinal;
  }
  manifest_value.material_binding_digest = {};
  manifest_value.manifest_digest = {};
  auto made_manifest =
      make_qualification_campaign_manifest(std::move(manifest_value));
  require(made_manifest.has_value(),
          "candidate manifest binds every frozen fuzz configuration");
  auto encoded_manifest =
      encode_qualification_campaign_manifest(made_manifest.value());
  require(encoded_manifest.has_value(),
          "expanded candidate manifest encodes canonically");

  auto plan_value = plan_fixture();
  plan_value.manifest_canonical_bytes = std::move(encoded_manifest.value());
  plan_value.manifest_digest = made_manifest.value().manifest_digest;
  std::set<std::string> expected_deferred;
  ordinal = 0;
  for (const auto &fuzz_case : matrix.fuzz_cases) {
    require(fuzz_case.minimum_aggregate_cpu_seconds ==
                qualification_minimum_fuzz_cpu_seconds,
            "every frozen fuzz entry retains the 24 CPU-hour floor");

    qualification_candidate_execution_case execution;
    execution.identifier = "candidate.matrix." + fuzz_case.identifier;
    execution.source_identifier = fuzz_case.identifier;
    execution.source_plan_digest = matrix.plan_digest;
    execution.source_case_digest = fuzz_case.case_digest;
    execution.dimensions = plan_value.execution_cases.front().dimensions;
    execution.dimensions.toolchain_identifier =
        fuzz_case.configuration_identifier;
    execution.dimensions.geometry_category =
        std::string("p6.8.fuzz.") +
        qualification_fuzz_family_token(fuzz_case.family);
    execution.expected_outcomes = {
        qualification_outcome::verified_exact_success};
    auto made_execution =
        make_qualification_candidate_execution_case(std::move(execution));
    require(made_execution.has_value(),
            "a frozen P6.8 fuzz entry binds into the P6.10 execution inventory");
    expected_deferred.insert(made_execution.value().identifier);
    plan_value.execution_cases.push_back(std::move(made_execution.value()));
    ++ordinal;
  }
  require_equal(ordinal, std::uint64_t(8),
                "all frozen fuzz entries enter the candidate inventory");

  plan_value.canonical_bytes.clear();
  plan_value.plan_digest = {};
  auto made_plan =
      make_qualification_candidate_campaign_plan(std::move(plan_value));
  require(made_plan.has_value(),
          "candidate plan canonicalizes with the complete P6.8 fuzz inventory");
  const auto plan = std::move(made_plan.value());

  qualification_candidate_run_options options;
  ordinal = 0;
  for (const auto &identifier : expected_deferred) {
    qualification_candidate_deferred_execution deferred;
    deferred.case_identifier = identifier;
    deferred.reason =
        "The frozen 24 CPU-hour fuzz allocation requires reviewed offline execution.";
    deferred.evidence_digest = tagged_digest('F', 700U + ordinal);
    options.deferred_executions.push_back(std::move(deferred));
    ++ordinal;
  }

  std::vector<std::string> visited;
  auto pending = run_qualification_candidate_campaign(
      plan,
      [&](const qualification_candidate_execution_case &descriptor)
          -> product_status_or<qualification_candidate_execution_observation> {
        visited.push_back(descriptor.identifier);
        if (descriptor.identifier == "candidate.case.expected_failure")
          return observation(descriptor,
                             qualification_outcome::expected_typed_failure, 803);
        return observation(
            descriptor, qualification_outcome::verified_exact_success,
            descriptor.identifier == "candidate.case.exact_success" ? 801 : 802);
      },
      std::move(options));

  require(pending.has_value() && !pending.value().campaign.complete &&
              pending.value().campaign.blocking_issue_count == 8 &&
              pending.value().campaign.issues.size() == 8 &&
              pending.value().campaign.passed_case_count ==
                  plan.execution_cases.size() - expected_deferred.size() &&
              visited.size() ==
                  plan.execution_cases.size() - expected_deferred.size(),
          "all eight offline allocations remain explicit blocking campaign evidence");

  for (const auto &identifier : expected_deferred) {
    require(std::find(visited.begin(), visited.end(), identifier) == visited.end(),
            "no frozen offline fuzz allocation reaches the online executor");
    const auto observed = std::find_if(
        pending.value().campaign.observations.begin(),
        pending.value().campaign.observations.end(), [&](const auto &entry) {
          return entry.case_identifier == identifier;
        });
    require(observed != pending.value().campaign.observations.end() &&
                observed->outcome ==
                    qualification_outcome::infrastructure_failure &&
                observed->typed_outcome_key ==
                    "qualification_candidate.deferred_offline",
            "each frozen fuzz allocation has canonical deferral evidence");

    const auto issue = std::find_if(
        pending.value().campaign.issues.begin(),
        pending.value().campaign.issues.end(), [&](const auto &entry) {
          return entry.case_identifier == identifier;
        });
    require(issue != pending.value().campaign.issues.end() &&
                issue->kind ==
                    qualification_candidate_issue_kind::infrastructure_failure &&
                issue->disposition ==
                    qualification_candidate_issue_disposition::unresolved_blocking,
            "each frozen fuzz allocation retains its own unresolved blocking issue");
  }
}

} // namespace
