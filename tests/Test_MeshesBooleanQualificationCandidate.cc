#include "MeshBooleanTestHarness.h"

#include <YgorMeshesBooleanQualificationCandidate.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace ygor::mesh_boolean;
using namespace ygor::mesh_boolean::testing;

namespace {

digest tagged_digest(char tag, std::uint64_t ordinal) {
  canonical_encoder encoder;
  encoder.byte(static_cast<std::uint8_t>(tag));
  encoder.u64(ordinal);
  return domain_digest({{'Y','G','B','Q','C','T','0','1'}}, encoder.bytes());
}

backend_identity candidate_backend() {
  backend_capabilities capabilities;
  for (auto capability :
       {backend_capability::exact_set_semantics,
        backend_capability::exact_coordinates,
        backend_capability::stratified_output,
        backend_capability::manifold_mesh_output,
        backend_capability::deterministic_canonical_output,
        backend_capability::certified_failure_categories,
        backend_capability::provenance_mapping,
        backend_capability::strict_prepared_operands,
        backend_capability::exact_in_T_output,
        backend_capability::certified_approximate_output})
    capabilities.set(capability);
  auto made = make_backend_identity(backend_id::experimental_exact_v1,
                                    {1, 0, 0}, "candidate-campaign-build",
                                    capabilities, backend_maturity::candidate);
  require(made.has_value(), "candidate backend identity canonicalizes");
  return made.value();
}

qualification_campaign_manifest manifest_fixture() {
  qualification_campaign_manifest manifest;
  manifest.identifier = "p6.10-candidate-manifest-v1";
  manifest.workload_profile = "construction-known-polyhedra-v1";
  manifest.repository_commit = "0123456789abcdef0123456789abcdef01234567";
  manifest.repository_tree_digest = tagged_digest('T', 1);
  manifest.created_utc = "2026-07-27T00:00:00Z";

  qualification_backend_binding backend;
  backend.identity = candidate_backend();
  backend.implementation_name = "experimental_exact_v1";
  backend.implementation_version = "1.0.0";
  backend.adapter_source_digest = tagged_digest('B', 1);
  backend.adapter_binary_digest = tagged_digest('B', 2);
  backend.dependency_digest = tagged_digest('B', 3);
  manifest.backends.push_back(backend);

  qualification_result_mode_binding mode;
  mode.representation = result_representation::exact_stratified;
  mode.semantics = product_realization_semantics::not_requested;
  mode.policy_digest = tagged_digest('M', 1);
  manifest.result_modes.push_back(mode);

  qualification_preparation_binding preparation;
  preparation.mode = preparation_mode::strict_validation;
  preparation.policy_digest = tagged_digest('P', 1);
  manifest.preparation_policies.push_back(preparation);
  manifest.type_specializations.push_back(
      {coordinate_tag::binary64, index_tag::uint64});

  qualification_toolchain_binding toolchain;
  toolchain.identifier = "gcc-strict-release-x86_64";
  toolchain.compiler_name = "GCC";
  toolchain.compiler_version = "candidate-version";
  toolchain.standard_library = qualification_standard_library::libstdcxx;
  toolchain.standard_library_version = "candidate-version";
  toolchain.architecture = qualification_architecture::x86_64;
  toolchain.operating_system = "Linux";
  toolchain.operating_system_version = "controlled";
  toolchain.target_triple = "x86_64-linux-gnu";
  toolchain.build_type = "Release";
  toolchain.floating_point_mode =
      qualification_floating_point_mode::strict_default_round_to_nearest;
  toolchain.compile_flags = {"-std=c++17", "-fno-fast-math",
                             "-ffp-contract=off"};
  toolchain.environment_digest = tagged_digest('E', 1);
  manifest.toolchains.push_back(toolchain);

  qualification_verifier_binding verifier;
  verifier.identifier = "mandatory-independent-verifiers";
  verifier.version = "1";
  verifier.implementation_digest = tagged_digest('V', 1);
  manifest.verifiers.push_back(verifier);

  qualification_corpus_binding corpus;
  corpus.identifier = "permanent-corpus-v1";
  corpus.version = "1";
  corpus.source = qualification_corpus_source::generated_construction_known;
  corpus.redistribution = qualification_redistribution::repository_embedded;
  corpus.license_or_provenance = "project-generated";
  corpus.case_count = 10000;
  corpus.corpus_digest = tagged_digest('C', 1);
  corpus.category_coverage_digest = tagged_digest('C', 2);
  corpus.expected_outcome_digest = tagged_digest('C', 3);
  manifest.corpora.push_back(corpus);

  qualification_generator_binding generator;
  generator.identifier = "construction-aware-v1";
  generator.version = "1";
  generator.implementation_digest = tagged_digest('G', 1);
  generator.first_seed = 1;
  generator.last_seed = 10000;
  generator.parameter_range_digest = tagged_digest('G', 2);
  manifest.generators.push_back(generator);

  qualification_fuzz_campaign_binding fuzz;
  fuzz.identifier = "candidate-fuzz-matrix";
  fuzz.engine = "in-tree-construction-aware";
  fuzz.engine_version = "1";
  fuzz.dictionary_digest = tagged_digest('F', 1);
  fuzz.mutator_digest = tagged_digest('F', 2);
  fuzz.seed_set_digest = tagged_digest('F', 3);
  fuzz.duration_seconds = 86400;
  fuzz.worker_count = 2;
  manifest.fuzz_campaigns.push_back(fuzz);

  qualification_chain_binding chain;
  chain.identifier = "candidate-operation-chains";
  chain.version = "1";
  chain.definition_digest = tagged_digest('H', 1);
  chain.chain_count = 1000;
  chain.minimum_steps = 5;
  chain.maximum_steps = 9;
  manifest.operation_chains.push_back(chain);

  qualification_resource_binding resources;
  resources.identifier = "candidate-resource-policy";
  resources.policy_digest = tagged_digest('R', 1);
  resources.wall_timeout_milliseconds = 600000;
  resources.authoritative_byte_limit = 1024ULL * 1024ULL * 1024ULL;
  resources.work_unit_limit = 100000000;
  resources.cancellation_latency_limit_milliseconds = 1000;
  manifest.resource_policies.push_back(resources);

  manifest.performance.identifier = "controlled-exclusive-host-v1";
  manifest.performance.version = "1";
  manifest.performance.hardware_identifier = "controlled-hardware";
  manifest.performance.hardware_digest = tagged_digest('Q', 1);
  manifest.performance.measurement_protocol_digest = tagged_digest('Q', 2);
  manifest.performance.warmup_runs = 3;
  manifest.performance.measured_runs = 7;
  manifest.performance.controlled_exclusive_host = true;

  qualification_threshold threshold;
  threshold.metric = "false_success_count";
  threshold.relation = qualification_threshold_relation::equal;
  threshold.numerator = 0;
  threshold.denominator = 1;
  threshold.unit = "count";
  manifest.thresholds.push_back(threshold);
  manifest.exclusions = {"unknown-provenance-inputs"};
  manifest.known_limitations = {"candidate evidence does not promote a default"};

  auto made = make_qualification_campaign_manifest(std::move(manifest));
  require(made.has_value(), "candidate manifest canonicalizes");
  return made.value();
}

qualification_candidate_gate_binding gate(
    qualification_candidate_gate_kind kind, std::uint64_t ordinal) {
  qualification_candidate_gate_binding value;
  value.kind = kind;
  value.identifier = qualification_candidate_gate_kind_token(kind);
  value.plan_digest = tagged_digest('A', ordinal);
  value.report_digest = tagged_digest('B', ordinal);
  value.independent_validation_digest = tagged_digest('C', ordinal);
  value.complete = true;
  value.passed = true;
  auto made = make_qualification_candidate_gate_binding(std::move(value));
  require(made.has_value(), "prior qualification gate canonicalizes");
  return made.value();
}

qualification_candidate_execution_case execution_case(
    std::string identifier, qualification_outcome expected_outcome,
    std::uint64_t ordinal) {
  qualification_candidate_execution_case value;
  value.identifier = std::move(identifier);
  value.source_identifier = "frozen-source-case-" + std::to_string(ordinal);
  value.source_plan_digest = tagged_digest('S', ordinal);
  value.source_case_digest = tagged_digest('K', ordinal);
  value.dimensions.backend = backend_id::experimental_exact_v1;
  value.dimensions.representation = result_representation::exact_stratified;
  value.dimensions.preparation = preparation_mode::strict_validation;
  value.dimensions.selected_operation = operation::regularized_union;
  value.dimensions.coordinate = coordinate_tag::binary64;
  value.dimensions.index = index_tag::uint64;
  value.dimensions.toolchain_identifier = "gcc-strict-release-x86_64";
  value.dimensions.geometry_category = "candidate-category-" +
                                       std::to_string(ordinal);
  value.expected_outcomes = {expected_outcome};
  if (expected_outcome == qualification_outcome::expected_typed_failure)
    value.expected_failure_codes = {product_error_code::input_contract_error};
  auto made = make_qualification_candidate_execution_case(std::move(value));
  require(made.has_value(), "candidate execution case canonicalizes");
  return made.value();
}

qualification_candidate_campaign_plan plan_fixture() {
  const auto manifest = manifest_fixture();
  auto encoded = encode_qualification_campaign_manifest(manifest);
  require(encoded.has_value(), "candidate manifest encodes");

  qualification_candidate_campaign_plan plan;
  plan.identifier = "p6.10-candidate-campaign-v1";
  plan.manifest_canonical_bytes = std::move(encoded.value());
  plan.manifest_digest = manifest.manifest_digest;
  for (std::uint64_t i = 0;
       i != static_cast<std::uint64_t>(qualification_candidate_gate_kind::count);
       ++i)
    plan.required_gates.push_back(
        gate(static_cast<qualification_candidate_gate_kind>(i), i + 1U));
  plan.execution_cases = {
      execution_case("candidate.case.exact_success",
                     qualification_outcome::verified_exact_success, 1),
      execution_case("candidate.case.resolved_defect",
                     qualification_outcome::verified_exact_success, 2),
      execution_case("candidate.case.expected_failure",
                     qualification_outcome::expected_typed_failure, 3)};
  auto made = make_qualification_candidate_campaign_plan(std::move(plan));
  require(made.has_value(), "candidate campaign plan canonicalizes");
  return made.value();
}

const qualification_candidate_execution_case &find_execution(
    const qualification_candidate_campaign_plan &plan, const std::string &id) {
  const auto found = std::find_if(
      plan.execution_cases.begin(), plan.execution_cases.end(),
      [&](const auto &entry) { return entry.identifier == id; });
  require(found != plan.execution_cases.end(), "candidate execution case exists");
  return *found;
}

qualification_candidate_execution_observation observation(
    const qualification_candidate_execution_case &descriptor,
    qualification_outcome outcome, std::uint64_t ordinal) {
  qualification_candidate_execution_observation value;
  value.case_identifier = descriptor.identifier;
  value.case_digest = descriptor.case_digest;
  value.outcome = outcome;
  value.completed = true;
  value.typed_outcome_key = "candidate.outcome." + std::to_string(ordinal);
  value.accounting_digest = tagged_digest('A', ordinal + 100);
  if (outcome == qualification_outcome::verified_exact_success ||
      outcome == qualification_outcome::verified_certified_approximate_success)
    value.canonical_result_digest = tagged_digest('O', ordinal);
  else
    value.canonical_failure_digest = tagged_digest('X', ordinal);
  value.replay_digest = tagged_digest('R', ordinal + 100);
  value.run_log_digest = tagged_digest('L', ordinal + 100);
  auto made = make_qualification_candidate_execution_observation(
      std::move(value));
  require(made.has_value(), "candidate observation canonicalizes");
  return made.value();
}

qualification_candidate_regression_binding regression_fixture() {
  qualification_candidate_regression_binding value;
  value.promotion_identifier = "p6.10-regression-promotion-1";
  value.permanent_test_id = "C14.P610.regression.1";
  value.minimized_case_digest = tagged_digest('M', 100);
  value.canonical_case_bytes_digest = tagged_digest('M', 101);
  value.minimization_transcript_digest = tagged_digest('M', 102);
  value.promotion_artifact_digest = tagged_digest('M', 103);
  value.corpus_digest_before = tagged_digest('C', 100);
  value.corpus_digest_after = tagged_digest('C', 101);
  auto made = make_qualification_candidate_regression_binding(std::move(value));
  require(made.has_value(), "regression promotion binding canonicalizes");
  return made.value();
}

qualification_candidate_issue resolved_defect(
    const qualification_candidate_execution_case &descriptor,
    const qualification_candidate_execution_observation &rerun) {
  qualification_candidate_issue issue;
  issue.identifier = "issue.resolved.engine-defect";
  issue.case_identifier = descriptor.identifier;
  issue.case_digest = descriptor.case_digest;
  issue.kind = qualification_candidate_issue_kind::false_success;
  issue.initial_observation_digest = tagged_digest('I', 1);
  issue.detected_evidence_digest = tagged_digest('I', 2);
  issue.disposition =
      qualification_candidate_issue_disposition::resolved_engine_defect;
  issue.reviewer = "qualification-reviewer";
  issue.rationale = "The minimized case reproduced an engine defect.";
  issue.resolution_evidence_digest = tagged_digest('I', 3);
  issue.regression = regression_fixture();
  issue.affected_configuration_identifiers = {"gcc-strict-release-x86_64"};
  issue.rerun_observation_digests = {rerun.observation_digest};
  issue.resolution_verified = true;
  auto made = make_qualification_candidate_issue(std::move(issue));
  require(made.has_value(), "resolved engine defect canonicalizes");
  return made.value();
}

std::vector<qualification_candidate_execution_observation>
passing_observations(const qualification_candidate_campaign_plan &plan) {
  return {observation(find_execution(plan, "candidate.case.exact_success"),
                      qualification_outcome::verified_exact_success, 1),
          observation(find_execution(plan, "candidate.case.resolved_defect"),
                      qualification_outcome::verified_exact_success, 2),
          observation(find_execution(plan, "candidate.case.expected_failure"),
                      qualification_outcome::expected_typed_failure, 3)};
}

void plan_contracts() {
  const auto plan = plan_fixture();
  require_equal(plan.required_gates.size(),
                static_cast<std::size_t>(
                    qualification_candidate_gate_kind::count),
                "all P6.2-P6.9 gates are bound");
  require_equal(plan.execution_cases.size(), std::size_t(3),
                "frozen execution inventory is retained");
  require(validate_qualification_candidate_campaign_plan(plan).has_value(),
          "candidate plan independently validates");
  auto encoded = encode_qualification_candidate_campaign_plan(plan);
  require(encoded.has_value() && encoded.value() == plan.canonical_bytes,
          "candidate plan canonical bytes are reusable");

  auto bad = plan;
  bad.required_gates.pop_back();
  bad.canonical_bytes.clear();
  bad.plan_digest = {};
  require(!make_qualification_candidate_campaign_plan(std::move(bad)).has_value(),
          "missing a prior qualification gate fails closed");
}

void complete_campaign_contracts() {
  const auto plan = plan_fixture();
  auto observations = passing_observations(plan);
  std::vector<qualification_candidate_issue> issues{
      resolved_defect(find_execution(plan, "candidate.case.resolved_defect"),
                      observations[1])};
  auto campaign = make_qualification_candidate_campaign(
      plan, observations, issues);
  require(campaign.has_value() && campaign.value().complete &&
              campaign.value().blocking_issue_count == 0 &&
              campaign.value().passed_case_count == plan.execution_cases.size() &&
              campaign.value().resolved_issue_count == 1 &&
              campaign.value().promoted_regression_count == 1 &&
              qualification_candidate_campaign_gate_passes(campaign.value(), plan),
          "complete rerun and regression evidence closes P6.10");
  require(validate_qualification_candidate_campaign(campaign.value(), plan)
              .has_value(),
          "candidate campaign independently reconstructs");

  std::reverse(observations.begin(), observations.end());
  std::reverse(issues.begin(), issues.end());
  auto reordered = make_qualification_candidate_campaign(
      plan, std::move(observations), std::move(issues));
  require(reordered.has_value() &&
              reordered.value().canonical_bytes == campaign.value().canonical_bytes &&
              reordered.value().campaign_digest == campaign.value().campaign_digest,
          "arrival order cannot affect candidate evidence");
}

void unresolved_and_missing_contracts() {
  const auto plan = plan_fixture();
  auto observations = passing_observations(plan);
  observations[0] = observation(find_execution(plan, "candidate.case.exact_success"),
                                qualification_outcome::false_success, 11);
  qualification_candidate_issue unresolved;
  unresolved.identifier = "issue.unresolved.false-success";
  unresolved.case_identifier =
      find_execution(plan, "candidate.case.exact_success").identifier;
  unresolved.case_digest =
      find_execution(plan, "candidate.case.exact_success").case_digest;
  unresolved.kind = qualification_candidate_issue_kind::false_success;
  unresolved.initial_observation_digest = observations[0].observation_digest;
  unresolved.detected_evidence_digest = tagged_digest('U', 1);
  auto made_issue = make_qualification_candidate_issue(std::move(unresolved));
  require(made_issue.has_value(), "unresolved issue remains observable");
  auto campaign = make_qualification_candidate_campaign(
      plan, observations, {made_issue.value()});
  require(campaign.has_value() && !campaign.value().complete &&
              campaign.value().blocking_issue_count != 0 &&
              !qualification_candidate_campaign_gate_passes(campaign.value(), plan),
          "unresolved false success blocks the candidate gate");

  observations = passing_observations(plan);
  observations.pop_back();
  campaign = make_qualification_candidate_campaign(plan, observations, {});
  require(campaign.has_value() && !campaign.value().complete &&
              campaign.value().blocking_issue_count == 1,
          "missing frozen execution remains a blocking issue");
}

void resolution_fail_closed_contracts() {
  const auto plan = plan_fixture();
  auto observations = passing_observations(plan);
  auto issue = resolved_defect(
      find_execution(plan, "candidate.case.resolved_defect"), observations[1]);
  issue.regression.reset();
  issue.issue_digest = {};
  require(!make_qualification_candidate_issue(std::move(issue)).has_value(),
          "resolved engine defect without regression promotion is rejected");

  issue = resolved_defect(
      find_execution(plan, "candidate.case.resolved_defect"), observations[1]);
  issue.rerun_observation_digests[0] = tagged_digest('Z', 999);
  issue.issue_digest = {};
  auto remade = make_qualification_candidate_issue(std::move(issue));
  require(remade.has_value(), "well-formed stale rerun remains inspectable");
  require(!make_qualification_candidate_campaign(
               plan, observations, {remade.value()})
               .has_value(),
          "resolution not bound to the final rerun is rejected");

  auto stale = observations.front();
  stale.case_digest = tagged_digest('Z', 1);
  stale.observation_digest = {};
  auto made_stale = make_qualification_candidate_execution_observation(stale);
  require(made_stale.has_value(), "stale observation is individually canonical");
  observations.front() = made_stale.value();
  require(!make_qualification_candidate_campaign(plan, observations, {}).has_value(),
          "foreign case binding cannot enter the campaign");
}

void performance_review_contracts() {
  const auto plan = plan_fixture();
  auto observations = passing_observations(plan);
  auto regressed = observations[0];
  regressed.performance_regression_observed = true;
  regressed.regression_evidence_digest = tagged_digest('P', 900);
  regressed.observation_digest = {};
  auto made_regressed =
      make_qualification_candidate_execution_observation(regressed);
  require(made_regressed.has_value(), "performance regression is observable");
  observations[0] = made_regressed.value();

  qualification_candidate_issue unresolved;
  unresolved.identifier = "issue.performance.blocking";
  unresolved.case_identifier =
      find_execution(plan, "candidate.case.exact_success").identifier;
  unresolved.case_digest =
      find_execution(plan, "candidate.case.exact_success").case_digest;
  unresolved.kind = qualification_candidate_issue_kind::performance_regression;
  unresolved.initial_observation_digest = observations[0].observation_digest;
  unresolved.detected_evidence_digest = tagged_digest('P', 901);
  auto made = make_qualification_candidate_issue(std::move(unresolved));
  require(made.has_value(), "performance issue canonicalizes");
  auto campaign = make_qualification_candidate_campaign(
      plan, observations, {made.value()});
  require(campaign.has_value() && !campaign.value().complete,
          "unreviewed performance regression blocks P6.10");
}

} // namespace

int main() {
  harness tests;
  tests.add("P6.10.plan", [] { plan_contracts(); });
  tests.add("P6.10.complete", [] { complete_campaign_contracts(); });
  tests.add("P6.10.unresolved", [] { unresolved_and_missing_contracts(); });
  tests.add("P6.10.resolution", [] { resolution_fail_closed_contracts(); });
  tests.add("P6.10.performance", [] { performance_review_contracts(); });
  return tests.run(std::cout, std::cerr);
}
