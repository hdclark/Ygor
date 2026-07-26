#pragma once
#ifndef YGOR_MESHES_BOOLEAN_QUALIFICATION_H_
#define YGOR_MESHES_BOOLEAN_QUALIFICATION_H_

#include "YgorMeshesBooleanProductContractPolicies.h"

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t qualification_campaign_schema_version = 1;
constexpr std::uint16_t qualification_result_summary_schema_version = 1;
constexpr std::uint16_t qualification_human_report_schema_version = 1;

// This taxonomy is the only machine-readable outcome vocabulary accepted by
// qualification summaries.  Producer-specific statuses must be normalized to
// one of these values before they can affect a qualification decision.
enum class qualification_outcome : std::uint8_t {
  verified_exact_success,
  verified_certified_approximate_success,
  expected_typed_failure,
  unexpected_typed_failure,
  backend_disagreement,
  verifier_disagreement,
  false_success,
  nondeterministic_outcome,
  timeout_or_resource_limit,
  infrastructure_failure
};

enum class qualification_architecture : std::uint8_t {
  x86_64,
  aarch64,
  ppc64le,
  riscv64,
  other_64_bit
};

enum class qualification_standard_library : std::uint8_t {
  libstdcxx,
  libcxx,
  msvc_stl,
  other
};

enum class qualification_floating_point_mode : std::uint8_t {
  strict_default_round_to_nearest,
  strict_controlled_rounding_matrix
};

enum class qualification_corpus_source : std::uint8_t {
  generated_construction_known,
  internally_generated_cad_like,
  licensed_external,
  private_external,
  minimized_regression
};

enum class qualification_redistribution : std::uint8_t {
  repository_embedded,
  content_addressed_external,
  private_digest_only
};

enum class qualification_threshold_relation : std::uint8_t {
  less_equal,
  greater_equal,
  equal
};

enum class qualification_report_decision : std::uint8_t {
  candidate,
  qualified,
  rejected,
  revoked
};

enum class qualification_report_section_kind : std::uint8_t {
  executive_result,
  repository_and_commands,
  platform_matrix,
  corpus_coverage,
  generators_and_fuzzing,
  outcomes,
  disagreements,
  sanitizer_and_determinism,
  performance_memory_cancellation,
  promotion_decisions,
  known_limitations,
  replay_artifacts,
  count
};

enum class qualification_artifact_kind : std::uint8_t {
  canonical_inputs,
  canonical_outputs,
  replay_bundle,
  verifier_log,
  backend_comparison,
  sanitizer_log,
  fuzz_corpus,
  performance_data,
  external_corpus_index,
  other
};

struct qualification_backend_binding {
  backend_identity identity;
  std::string implementation_name;
  std::string implementation_version;
  digest adapter_source_digest;
  digest adapter_binary_digest;
  digest dependency_digest;
  bool diagnostic_only = false;
};

struct qualification_result_mode_binding {
  result_representation representation =
      result_representation::exact_stratified;
  product_realization_semantics semantics =
      product_realization_semantics::not_requested;
  digest policy_digest;
};

struct qualification_preparation_binding {
  preparation_mode mode = preparation_mode::strict_validation;
  model_unit tolerance_unit = model_unit::unspecified;
  std::uint64_t model_tolerance_binary64_bits = 0;
  digest policy_digest;
};

struct qualification_type_binding {
  coordinate_tag coordinate = coordinate_tag::binary32;
  index_tag index = index_tag::uint32;
};

struct qualification_toolchain_binding {
  std::string identifier;
  std::string compiler_name;
  std::string compiler_version;
  qualification_standard_library standard_library =
      qualification_standard_library::libstdcxx;
  std::string standard_library_version;
  qualification_architecture architecture = qualification_architecture::x86_64;
  std::string operating_system;
  std::string operating_system_version;
  std::string target_triple;
  std::string build_type;
  qualification_floating_point_mode floating_point_mode =
      qualification_floating_point_mode::strict_default_round_to_nearest;
  std::vector<std::string> compile_flags;
  digest environment_digest;
};

struct qualification_verifier_binding {
  std::string identifier;
  std::string version;
  digest implementation_digest;
  bool mandatory = true;
};

struct qualification_corpus_binding {
  std::string identifier;
  std::string version;
  qualification_corpus_source source =
      qualification_corpus_source::generated_construction_known;
  qualification_redistribution redistribution =
      qualification_redistribution::repository_embedded;
  std::string license_or_provenance;
  std::uint64_t case_count = 0;
  digest corpus_digest;
  digest category_coverage_digest;
  digest expected_outcome_digest;
};

struct qualification_generator_binding {
  std::string identifier;
  std::string version;
  digest implementation_digest;
  std::uint64_t first_seed = 0;
  std::uint64_t last_seed = 0;
  digest parameter_range_digest;
};

struct qualification_fuzz_campaign_binding {
  std::string identifier;
  std::string engine;
  std::string engine_version;
  digest dictionary_digest;
  digest mutator_digest;
  digest seed_set_digest;
  std::uint64_t duration_seconds = 0;
  std::uint32_t worker_count = 0;
};

struct qualification_chain_binding {
  std::string identifier;
  std::string version;
  digest definition_digest;
  std::uint64_t chain_count = 0;
  std::uint32_t minimum_steps = 0;
  std::uint32_t maximum_steps = 0;
};

struct qualification_resource_binding {
  std::string identifier;
  digest policy_digest;
  std::uint64_t wall_timeout_milliseconds = 0;
  std::uint64_t authoritative_byte_limit = 0;
  std::uint64_t work_unit_limit = 0;
  std::uint64_t cancellation_latency_limit_milliseconds = 0;
};

struct qualification_performance_protocol {
  std::string identifier;
  std::string version;
  std::string hardware_identifier;
  digest hardware_digest;
  digest measurement_protocol_digest;
  std::uint32_t warmup_runs = 0;
  std::uint32_t measured_runs = 0;
  bool controlled_exclusive_host = false;
};

struct qualification_threshold {
  std::string metric;
  qualification_threshold_relation relation =
      qualification_threshold_relation::less_equal;
  std::uint64_t numerator = 0;
  std::uint64_t denominator = 1;
  std::string unit;
  bool blocking = true;
};

struct qualification_compatibility_review {
  digest prior_manifest_digest;
  digest reviewed_change_digest;
  std::string reviewer;
  std::string rationale;
  digest evidence_digest;
  bool approved = false;
};

struct qualification_campaign_manifest {
  std::uint16_t schema = qualification_campaign_schema_version;
  std::string identifier;
  std::string workload_profile;
  std::string repository_commit;
  digest repository_tree_digest;
  bool repository_dirty = false;
  std::string created_utc;
  std::vector<qualification_backend_binding> backends;
  std::vector<qualification_result_mode_binding> result_modes;
  std::vector<qualification_preparation_binding> preparation_policies;
  std::vector<qualification_type_binding> type_specializations;
  std::vector<qualification_toolchain_binding> toolchains;
  std::vector<qualification_verifier_binding> verifiers;
  std::vector<qualification_corpus_binding> corpora;
  std::vector<qualification_generator_binding> generators;
  std::vector<qualification_fuzz_campaign_binding> fuzz_campaigns;
  std::vector<qualification_chain_binding> operation_chains;
  std::vector<qualification_resource_binding> resource_policies;
  qualification_performance_protocol performance;
  std::vector<qualification_threshold> thresholds;
  std::vector<std::string> exclusions;
  std::vector<std::string> known_limitations;
  std::vector<qualification_compatibility_review> compatibility_reviews;
  digest material_binding_digest;
  digest manifest_digest;
};

struct qualification_dimension_key {
  backend_id backend = backend_id::experimental_exact_v1;
  result_representation representation =
      result_representation::exact_stratified;
  preparation_mode preparation = preparation_mode::strict_validation;
  operation selected_operation = operation::regularized_union;
  coordinate_tag coordinate = coordinate_tag::binary32;
  index_tag index = index_tag::uint32;
  std::string toolchain_identifier;
  std::string geometry_category;
};

inline bool operator<(const qualification_dimension_key &a,
                      const qualification_dimension_key &b) noexcept {
  return std::tie(a.backend, a.representation, a.preparation,
                  a.selected_operation, a.coordinate, a.index,
                  a.toolchain_identifier, a.geometry_category) <
         std::tie(b.backend, b.representation, b.preparation,
                  b.selected_operation, b.coordinate, b.index,
                  b.toolchain_identifier, b.geometry_category);
}
inline bool operator==(const qualification_dimension_key &a,
                       const qualification_dimension_key &b) noexcept {
  return !(a < b) && !(b < a);
}

struct qualification_outcome_count {
  qualification_dimension_key dimensions;
  qualification_outcome outcome = qualification_outcome::infrastructure_failure;
  std::uint64_t count = 0;
};

struct qualification_artifact_reference {
  qualification_artifact_kind kind = qualification_artifact_kind::other;
  std::string identifier;
  std::string location;
  digest content_digest;
  std::uint64_t byte_count = 0;
};

struct qualification_result_summary {
  std::uint16_t schema = qualification_result_summary_schema_version;
  digest manifest_digest;
  std::string run_identifier;
  std::string repository_commit;
  std::string started_utc;
  std::string finished_utc;
  bool complete = false;
  std::vector<qualification_outcome_count> counts;
  std::vector<qualification_artifact_reference> artifacts;
  std::uint64_t blocking_issue_count = 0;
  std::uint64_t false_success_count = 0;
  digest summary_digest;
};

struct qualification_report_section {
  qualification_report_section_kind kind =
      qualification_report_section_kind::executive_result;
  std::string heading;
  std::vector<std::string> lines;
};

struct qualification_human_report {
  std::uint16_t schema = qualification_human_report_schema_version;
  digest manifest_digest;
  digest summary_digest;
  std::string title;
  qualification_report_decision decision =
      qualification_report_decision::candidate;
  std::string claim_scope;
  std::string generated_utc;
  std::uint64_t blocking_issue_count = 0;
  std::uint64_t false_success_count = 0;
  std::vector<qualification_report_section> sections;
  digest markdown_digest;
  digest report_digest;
};

struct qualification_decode_limits {
  std::uint64_t max_record_bytes = 64ULL * 1024ULL * 1024ULL;
  std::uint64_t max_string_bytes = 4ULL * 1024ULL * 1024ULL;
  std::uint64_t max_vector_elements = 1000000;
  std::uint64_t max_total_strings = 4000000;
};

product_status_or<qualification_campaign_manifest>
make_qualification_campaign_manifest(qualification_campaign_manifest);
product_status_or<bool>
validate_qualification_campaign_manifest(
    const qualification_campaign_manifest &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_campaign_manifest(const qualification_campaign_manifest &);
product_status_or<qualification_campaign_manifest>
decode_qualification_campaign_manifest(
    const std::vector<std::uint8_t> &,
    const qualification_decode_limits & = {});

product_status_or<qualification_result_summary>
make_qualification_result_summary(qualification_result_summary);
product_status_or<bool>
validate_qualification_result_summary(
    const qualification_result_summary &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_result_summary(const qualification_result_summary &);
product_status_or<qualification_result_summary>
decode_qualification_result_summary(
    const std::vector<std::uint8_t> &,
    const qualification_decode_limits & = {});

product_status_or<qualification_human_report>
make_qualification_human_report(qualification_human_report);
product_status_or<bool>
validate_qualification_human_report(const qualification_human_report &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_human_report(const qualification_human_report &);
product_status_or<qualification_human_report>
decode_qualification_human_report(
    const std::vector<std::uint8_t> &,
    const qualification_decode_limits & = {});
product_status_or<std::string>
render_qualification_report_markdown(const qualification_human_report &);

// A material change invalidates a prior claim unless the new manifest contains
// an approved review over this exact change digest.
digest qualification_material_change_digest(const digest &prior_material,
                                             const digest &next_material);
bool qualification_claim_remains_valid(
    const qualification_campaign_manifest &prior,
    const qualification_campaign_manifest &next) noexcept;

// This is the only supported bridge from campaign artifacts to the compact
// product-facing qualification selector.
product_status_or<qualification_evidence_binding>
make_qualification_evidence_binding(
    const qualification_campaign_manifest &,
    const qualification_result_summary &,
    const qualification_human_report &) noexcept;

} // namespace mesh_boolean
} // namespace ygor

#endif
