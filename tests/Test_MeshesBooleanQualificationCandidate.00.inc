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
