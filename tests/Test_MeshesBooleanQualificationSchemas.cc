#include "YgorMeshesBooleanProductContract.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ygor::mesh_boolean;

namespace {

void require(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

digest fixture_digest(std::uint64_t seed) {
  canonical_encoder e;
  e.u64(seed);
  e.string("qualification-fixture");
  return domain_digest({{'Y', 'G', 'B', 'Q', 'T', 'S', 'T', '1'}},
                       e.bytes());
}

backend_identity qualified_backend() {
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
                                    {1, 2, 3}, "qualification-build",
                                    capabilities,
                                    backend_maturity::qualified);
  require(made.has_value(), "qualified backend identity");
  return made.value();
}

qualification_campaign_manifest campaign_fixture() {
  qualification_campaign_manifest manifest;
  manifest.identifier = "qualification-campaign-v1";
  manifest.workload_profile = "construction-known-polyhedra-v1";
  manifest.repository_commit = "0123456789abcdef0123456789abcdef01234567";
  manifest.repository_tree_digest = fixture_digest(1);
  manifest.created_utc = "2026-07-26T00:00:00Z";

  qualification_backend_binding backend;
  backend.identity = qualified_backend();
  backend.implementation_name = "experimental_exact_v1";
  backend.implementation_version = "1.2.3";
  backend.adapter_source_digest = fixture_digest(2);
  backend.adapter_binary_digest = fixture_digest(3);
  backend.dependency_digest = fixture_digest(4);
  manifest.backends.push_back(backend);

  qualification_result_mode_binding result;
  result.representation = result_representation::exact_stratified;
  result.semantics = product_realization_semantics::not_requested;
  result.policy_digest = fixture_digest(5);
  manifest.result_modes.push_back(result);

  qualification_preparation_binding preparation;
  preparation.mode = preparation_mode::strict_validation;
  preparation.policy_digest = fixture_digest(6);
  manifest.preparation_policies.push_back(preparation);

  manifest.type_specializations.push_back(
      {coordinate_tag::binary64, index_tag::uint64});

  qualification_toolchain_binding toolchain;
  toolchain.identifier = "gcc-14-libstdcxx-x86_64-debug";
  toolchain.compiler_name = "GCC";
  toolchain.compiler_version = "14.2.0";
  toolchain.standard_library = qualification_standard_library::libstdcxx;
  toolchain.standard_library_version = "14.2.0";
  toolchain.architecture = qualification_architecture::x86_64;
  toolchain.operating_system = "Linux";
  toolchain.operating_system_version = "Debian 13";
  toolchain.target_triple = "x86_64-linux-gnu";
  toolchain.build_type = "Debug";
  toolchain.floating_point_mode =
      qualification_floating_point_mode::strict_default_round_to_nearest;
  toolchain.compile_flags = {"-std=c++17", "-frounding-math",
                             "-fno-fast-math"};
  toolchain.environment_digest = fixture_digest(7);
  manifest.toolchains.push_back(toolchain);

  qualification_verifier_binding verifier;
  verifier.identifier = "independent-exact-result-reader";
  verifier.version = "1";
  verifier.implementation_digest = fixture_digest(8);
  manifest.verifiers.push_back(verifier);

  qualification_corpus_binding corpus;
  corpus.identifier = "construction-known-smoke";
  corpus.version = "1";
  corpus.source = qualification_corpus_source::generated_construction_known;
  corpus.redistribution =
      qualification_redistribution::repository_embedded;
  corpus.license_or_provenance = "project-generated";
  corpus.case_count = 32;
  corpus.corpus_digest = fixture_digest(9);
  corpus.category_coverage_digest = fixture_digest(10);
  corpus.expected_outcome_digest = fixture_digest(11);
  manifest.corpora.push_back(corpus);

  qualification_generator_binding generator;
  generator.identifier = "exact-halfspace-polytope";
  generator.version = "1";
  generator.implementation_digest = fixture_digest(12);
  generator.first_seed = 10;
  generator.last_seed = 42;
  generator.parameter_range_digest = fixture_digest(13);
  manifest.generators.push_back(generator);

  qualification_fuzz_campaign_binding fuzz;
  fuzz.identifier = "valid-geometry-smoke";
  fuzz.engine = "in-tree-deterministic-fuzzer";
  fuzz.engine_version = "1";
  fuzz.dictionary_digest = fixture_digest(14);
  fuzz.mutator_digest = fixture_digest(15);
  fuzz.seed_set_digest = fixture_digest(16);
  fuzz.duration_seconds = 60;
  fuzz.worker_count = 2;
  manifest.fuzz_campaigns.push_back(fuzz);

  qualification_chain_binding chain;
  chain.identifier = "five-step-reingestion";
  chain.version = "1";
  chain.definition_digest = fixture_digest(17);
  chain.chain_count = 4;
  chain.minimum_steps = 5;
  chain.maximum_steps = 7;
  manifest.operation_chains.push_back(chain);

  qualification_resource_binding resources;
  resources.identifier = "qualification-default-limits";
  resources.policy_digest = fixture_digest(18);
  resources.wall_timeout_milliseconds = 600000;
  resources.authoritative_byte_limit = 1024ULL * 1024ULL * 1024ULL;
  resources.work_unit_limit = 100000000;
  resources.cancellation_latency_limit_milliseconds = 1000;
  manifest.resource_policies.push_back(resources);

  manifest.performance.identifier = "controlled-host-v1";
  manifest.performance.version = "1";
  manifest.performance.hardware_identifier = "fixture-hardware";
  manifest.performance.hardware_digest = fixture_digest(19);
  manifest.performance.measurement_protocol_digest = fixture_digest(20);
  manifest.performance.warmup_runs = 2;
  manifest.performance.measured_runs = 5;
  manifest.performance.controlled_exclusive_host = true;

  qualification_threshold threshold;
  threshold.metric = "false_success_count";
  threshold.relation = qualification_threshold_relation::equal;
  threshold.numerator = 0;
  threshold.denominator = 1;
  threshold.unit = "count";
  manifest.thresholds.push_back(threshold);

  manifest.exclusions = {"unknown-provenance tessellations"};
  manifest.known_limitations = {"exact_in_T is not a practical-output target"};

  auto made = make_qualification_campaign_manifest(std::move(manifest));
  require(made.has_value(), "campaign fixture");
  return made.value();
}

qualification_result_summary
summary_fixture(const qualification_campaign_manifest &manifest) {
  qualification_result_summary summary;
  summary.manifest_digest = manifest.manifest_digest;
  summary.run_identifier = "qualification-run-0001";
  summary.repository_commit = manifest.repository_commit;
  summary.started_utc = "2026-07-26T00:00:00Z";
  summary.finished_utc = "2026-07-26T00:10:00Z";
  summary.complete = true;

  qualification_outcome_count count;
  count.dimensions.backend = backend_id::experimental_exact_v1;
  count.dimensions.representation = result_representation::exact_stratified;
  count.dimensions.preparation = preparation_mode::strict_validation;
  count.dimensions.selected_operation = operation::regularized_union;
  count.dimensions.coordinate = coordinate_tag::binary64;
  count.dimensions.index = index_tag::uint64;
  count.dimensions.toolchain_identifier =
      "gcc-14-libstdcxx-x86_64-debug";
  count.dimensions.geometry_category = "rotated-convex";
  count.outcome = qualification_outcome::verified_exact_success;
  count.count = 32;
  summary.counts.push_back(count);

  qualification_artifact_reference artifact;
  artifact.kind = qualification_artifact_kind::replay_bundle;
  artifact.identifier = "run-0001-replay";
  artifact.location = "artifacts/run-0001-replay.tar";
  artifact.content_digest = fixture_digest(21);
  artifact.byte_count = 4096;
  summary.artifacts.push_back(artifact);

  auto made = make_qualification_result_summary(std::move(summary));
  require(made.has_value(), "summary fixture");
  return made.value();
}

qualification_human_report
report_fixture(const qualification_campaign_manifest &manifest,
               const qualification_result_summary &summary) {
  qualification_human_report report;
  report.manifest_digest = manifest.manifest_digest;
  report.summary_digest = summary.summary_digest;
  report.title = "Mesh Boolean Qualification Report";
  report.decision = qualification_report_decision::qualified;
  report.claim_scope =
      "experimental_exact_v1 / exact_stratified / strict validation / "
      "construction-known-polyhedra-v1";
  report.generated_utc = "2026-07-26T00:15:00Z";
  report.blocking_issue_count = summary.blocking_issue_count;
  report.false_success_count = summary.false_success_count;

  const std::vector<std::string> headings{
      "Executive result",
      "Repository and commands",
      "Platform matrix",
      "Corpus coverage",
      "Generators and fuzzing",
      "Outcomes",
      "Disagreements",
      "Sanitizer and determinism",
      "Performance, memory, and cancellation",
      "Promotion decisions",
      "Known limitations",
      "Replay artifacts"};
  for (std::size_t i = 0; i != headings.size(); ++i) {
    qualification_report_section section;
    section.kind = static_cast<qualification_report_section_kind>(i);
    section.heading = headings[i];
    section.lines = {"Fixture evidence is present and digest-bound."};
    report.sections.push_back(std::move(section));
  }

  auto made = make_qualification_human_report(std::move(report));
  require(made.has_value(), "report fixture");
  return made.value();
}

void canonical_round_trip_and_limits() {
  const auto manifest = campaign_fixture();
  auto bytes = encode_qualification_campaign_manifest(manifest);
  require(bytes.has_value(), "manifest encode");
  auto decoded = decode_qualification_campaign_manifest(bytes.value());
  require(decoded.has_value() &&
              decoded.value().manifest_digest == manifest.manifest_digest &&
              decoded.value().material_binding_digest ==
                  manifest.material_binding_digest,
          "manifest canonical round trip");

  auto trailing = bytes.value();
  trailing.push_back(0);
  require(!decode_qualification_campaign_manifest(trailing).has_value(),
          "manifest trailing bytes rejected");

  qualification_decode_limits limits;
  limits.max_record_bytes = bytes.value().size() - 1;
  require(!decode_qualification_campaign_manifest(bytes.value(), limits)
               .has_value(),
          "manifest record limit enforced");

  auto mutated = manifest;
  mutated.toolchains[0].compiler_version = "14.2.1";
  require(!validate_qualification_campaign_manifest(mutated).has_value(),
          "material mutation invalidates manifest digest");

  auto missing_verifier = manifest;
  missing_verifier.verifiers.clear();
  missing_verifier.material_binding_digest = {};
  missing_verifier.manifest_digest = {};
  require(!make_qualification_campaign_manifest(missing_verifier).has_value(),
          "required binding collection enforced");

  auto dirty = manifest;
  dirty.repository_dirty = true;
  dirty.material_binding_digest = {};
  dirty.manifest_digest = {};
  require(!make_qualification_campaign_manifest(dirty).has_value(),
          "dirty repository cannot define qualification campaign");

  auto hidden_tolerance = manifest;
  hidden_tolerance.preparation_policies[0].model_tolerance_binary64_bits = 1;
  hidden_tolerance.material_binding_digest = {};
  hidden_tolerance.manifest_digest = {};
  require(!make_qualification_campaign_manifest(hidden_tolerance).has_value(),
          "strict preparation cannot hide model tolerance");

  auto short_chain = manifest;
  short_chain.operation_chains[0].minimum_steps = 4;
  short_chain.material_binding_digest = {};
  short_chain.manifest_digest = {};
  require(!make_qualification_campaign_manifest(short_chain).has_value(),
          "qualification chain floor enforced");

  auto unknown_mode = manifest;
  unknown_mode.result_modes[0].representation =
      static_cast<result_representation>(255);
  unknown_mode.material_binding_digest = {};
  unknown_mode.manifest_digest = {};
  require(!make_qualification_campaign_manifest(unknown_mode).has_value(),
          "unknown qualification enum rejected");
}

void material_change_and_review() {
  const auto prior = campaign_fixture();

  const auto require_material_invalidation =
      [&](const char *message, const auto &mutate) {
        auto next_input = prior;
        next_input.compatibility_reviews.clear();
        next_input.material_binding_digest = {};
        next_input.manifest_digest = {};
        mutate(next_input);
        auto next = make_qualification_campaign_manifest(std::move(next_input));
        require(next.has_value(), std::string("valid changed campaign: ") + message);
        require(next.value().material_binding_digest !=
                    prior.material_binding_digest &&
                    !qualification_claim_remains_valid(prior, next.value()),
                message);
      };

  require_material_invalidation("backend capability change invalidates",
      [](qualification_campaign_manifest &next) {
        auto capabilities = next.backends[0].identity.capabilities;
        capabilities.set(backend_capability::certified_approximate_output, false);
        auto identity = make_backend_identity(
            next.backends[0].identity.id,
            next.backends[0].identity.adapter_version,
            next.backends[0].identity.build_identifier, capabilities,
            next.backends[0].identity.maturity);
        require(identity.has_value(), "changed backend identity");
        next.backends[0].identity = identity.value();
      });
  require_material_invalidation("result policy change invalidates",
      [](qualification_campaign_manifest &next) {
        next.result_modes[0].policy_digest = fixture_digest(30);
      });
  require_material_invalidation("preparation policy change invalidates",
      [](qualification_campaign_manifest &next) {
        next.preparation_policies[0].policy_digest = fixture_digest(31);
      });
  require_material_invalidation("type specialization change invalidates",
      [](qualification_campaign_manifest &next) {
        next.type_specializations[0].index = index_tag::uint32;
      });
  require_material_invalidation("compiler mode change invalidates",
      [](qualification_campaign_manifest &next) {
        next.toolchains[0].compiler_version = "14.3.0";
      });
  require_material_invalidation("verifier change invalidates",
      [](qualification_campaign_manifest &next) {
        next.verifiers[0].implementation_digest = fixture_digest(32);
      });
  require_material_invalidation("corpus change invalidates",
      [](qualification_campaign_manifest &next) {
        ++next.corpora[0].case_count;
      });
  require_material_invalidation("generator change invalidates",
      [](qualification_campaign_manifest &next) {
        ++next.generators[0].last_seed;
      });
  require_material_invalidation("fuzz campaign change invalidates",
      [](qualification_campaign_manifest &next) {
        ++next.fuzz_campaigns[0].duration_seconds;
      });
  require_material_invalidation("operation chain change invalidates",
      [](qualification_campaign_manifest &next) {
        ++next.operation_chains[0].chain_count;
      });
  require_material_invalidation("resource policy change invalidates",
      [](qualification_campaign_manifest &next) {
        ++next.resource_policies[0].wall_timeout_milliseconds;
      });
  require_material_invalidation("performance protocol change invalidates",
      [](qualification_campaign_manifest &next) {
        ++next.performance.measured_runs;
      });
  require_material_invalidation("threshold change invalidates",
      [](qualification_campaign_manifest &next) {
        next.thresholds[0].numerator = 1;
      });
  require_material_invalidation("exclusion change invalidates",
      [](qualification_campaign_manifest &next) {
        next.exclusions.push_back("scan-derived inputs");
      });
  require_material_invalidation("known limitation change invalidates",
      [](qualification_campaign_manifest &next) {
        next.known_limitations.push_back("aarch64 campaign pending");
      });

  auto incompatible_schema = prior;
  ++incompatible_schema.schema;
  incompatible_schema.material_binding_digest = {};
  incompatible_schema.manifest_digest = {};
  require(!make_qualification_campaign_manifest(incompatible_schema).has_value() &&
              !qualification_claim_remains_valid(prior, incompatible_schema),
          "schema change invalidates rather than reinterprets claim");
  auto changed_input = prior;
  changed_input.manifest_digest = {};
  changed_input.material_binding_digest = {};
  changed_input.toolchains[0].compiler_version = "14.3.0";
  auto changed = make_qualification_campaign_manifest(changed_input);
  require(changed.has_value(), "changed campaign");
  require(!qualification_claim_remains_valid(prior, changed.value()),
          "material compiler change invalidates prior claim");

  auto reviewed_input = changed.value();
  reviewed_input.manifest_digest = {};
  qualification_compatibility_review review;
  review.prior_manifest_digest = prior.manifest_digest;
  review.reviewed_change_digest = qualification_material_change_digest(
      prior.material_binding_digest, changed.value().material_binding_digest);
  review.reviewer = "release-review-board";
  review.rationale = "Compiler patch update reviewed against compatibility suite";
  review.evidence_digest = fixture_digest(22);
  review.approved = true;
  reviewed_input.compatibility_reviews.push_back(review);
  auto reviewed = make_qualification_campaign_manifest(reviewed_input);
  require(reviewed.has_value() &&
              qualification_claim_remains_valid(prior, reviewed.value()),
          "exact reviewed material change preserves claim");

  auto wrong = reviewed.value();
  wrong.compatibility_reviews[0].reviewed_change_digest = fixture_digest(23);
  wrong.manifest_digest = {};
  auto wrong_made = make_qualification_campaign_manifest(wrong);
  require(wrong_made.has_value() &&
              !qualification_claim_remains_valid(prior, wrong_made.value()),
          "unrelated compatibility review does not preserve claim");
}

void outcome_accounting_and_report() {
  const auto manifest = campaign_fixture();
  const auto summary = summary_fixture(manifest);
  require(summary.blocking_issue_count == 0 &&
              summary.false_success_count == 0,
          "success summary accounting");

  auto summary_bytes = encode_qualification_result_summary(summary);
  require(summary_bytes.has_value(), "summary encode");
  auto summary_round_trip =
      decode_qualification_result_summary(summary_bytes.value());
  require(summary_round_trip.has_value() &&
              summary_round_trip.value().summary_digest ==
                  summary.summary_digest,
          "summary canonical round trip");

  const auto report = report_fixture(manifest, summary);
  auto markdown = render_qualification_report_markdown(report);
  require(markdown.has_value() &&
              markdown.value().find("# Mesh Boolean Qualification Report") ==
                  0 &&
              markdown.value().find("False successes: 0") !=
                  std::string::npos,
          "deterministic human report");

  auto report_bytes = encode_qualification_human_report(report);
  require(report_bytes.has_value(), "report encode");
  auto report_round_trip =
      decode_qualification_human_report(report_bytes.value());
  require(report_round_trip.has_value() &&
              report_round_trip.value().report_digest == report.report_digest,
          "report canonical round trip");

  auto false_summary_input = summary;
  false_summary_input.summary_digest = {};
  false_summary_input.counts[0].outcome = qualification_outcome::false_success;
  auto false_summary =
      make_qualification_result_summary(std::move(false_summary_input));
  require(false_summary.has_value() &&
              false_summary.value().blocking_issue_count == 32 &&
              false_summary.value().false_success_count == 32,
          "false success counted separately and blocking");

  auto invalid_report = report;
  invalid_report.report_digest = {};
  invalid_report.markdown_digest = {};
  invalid_report.blocking_issue_count = 32;
  invalid_report.false_success_count = 32;
  require(!make_qualification_human_report(invalid_report).has_value(),
          "qualified report cannot contain false success");
}

void evidence_and_runtime_authorization() {
  const auto manifest = campaign_fixture();
  const auto summary = summary_fixture(manifest);
  const auto report = report_fixture(manifest, summary);
  auto evidence = make_qualification_evidence_binding(manifest, summary, report);
  require(evidence.has_value(), "cross-bound qualification evidence");

  qualification_profile profile;
  profile.backend = qualified_backend().id;
  profile.capability_digest = qualified_backend().capability_digest;
  profile.representation = result_representation::exact_stratified;
  profile.preparation = preparation_mode::strict_validation;
  profile.workload_profile = manifest.workload_profile;
  auto selector = make_qualification_manifest(
      "qualified-default-v1", {profile}, evidence.value());
  require(selector.has_value(), "evidence-bound runtime manifest");

  boolean_product_options options;
  options.qualification.workload_profile = manifest.workload_profile;
  options.qualification.manifest = qualification_manifest_reference{
      product_contract_schema_version, selector.value().identifier,
      selector.value().manifest_digest};
  const auto backend = qualified_backend();
  require(authorize_backend(options, backend, &selector.value()).has_value(),
          "qualified default authorizes exact evidence-bound profile");

  auto stale_selector = selector.value();
  stale_selector.evidence.result_summary_digest.bytes[0] ^= 1;
  require(!validate_qualification_manifest(stale_selector).has_value(),
          "runtime selector rejects stale evidence binding");

  auto incomplete_summary = summary;
  incomplete_summary.summary_digest = {};
  incomplete_summary.complete = false;
  auto incomplete_made =
      make_qualification_result_summary(std::move(incomplete_summary));
  require(incomplete_made.has_value() &&
              !make_qualification_evidence_binding(
                   manifest, incomplete_made.value(), report)
                   .has_value(),
          "incomplete campaign cannot create qualification evidence");
}

} // namespace

int main() {
  try {
    canonical_round_trip_and_limits();
    material_change_and_review();
    outcome_accounting_and_report();
    evidence_and_runtime_authorization();
    std::cout << "ok\n";
    return 0;
  } catch (const std::exception &exception) {
    std::cerr << exception.what() << '\n';
    return 1;
  }
}
