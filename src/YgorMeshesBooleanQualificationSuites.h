#pragma once
#ifndef YGOR_MESHES_BOOLEAN_QUALIFICATION_SUITES_H_
#define YGOR_MESHES_BOOLEAN_QUALIFICATION_SUITES_H_

#include "YgorMeshesBooleanAttributes.h"
#include "YgorMeshesBooleanNormalization.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t qualification_suite_schema_version = 1;
constexpr std::uint32_t qualification_suite_checker_version = 1;

enum class qualification_suite_family : std::uint8_t {
  preparation,
  result_mode,
  attribute_and_provenance
};

enum class qualification_preparation_case_kind : std::uint8_t {
  strict_validation,
  diagnosis,
  repair
};

enum class qualification_attribute_case_kind : std::uint8_t {
  transfer_mode,
  conflict_policy,
  identifier_policy,
  merge_policy,
  vertex_copy_policy,
  sharp_edge_policy,
  texture_seam_policy,
  construction_provenance_policy,
  multi_source_mapping_and_query
};

// Binary64 bits are retained instead of a host-formatted decimal.  The checker
// decodes them only to reject negative, non-finite, or out-of-bound values.
struct qualification_explicit_bound_evidence {
  std::uint16_t schema = qualification_suite_schema_version;
  model_unit unit = model_unit::unspecified;
  std::uint64_t declared_limit_binary64_bits = 0;
  std::uint64_t observed_maximum_binary64_bits = 0;
  digest policy_digest;
  digest independent_verification_digest;
  digest evidence_digest;
  bool independently_verified = false;
  bool hidden_epsilon_used = false;
};

struct qualification_preparation_suite_case {
  std::uint16_t schema = qualification_suite_schema_version;
  std::string identifier;
  qualification_preparation_case_kind kind =
      qualification_preparation_case_kind::strict_validation;
  preparation_mode preparation = preparation_mode::strict_validation;
  normalization_mode normalization = normalization_mode::disabled;
  std::optional<normalization_operation> operation;
  model_unit tolerance_unit = model_unit::unspecified;
  std::uint64_t model_tolerance_binary64_bits = 0;
  nonplanar_facet_policy nonplanar_facets = nonplanar_facet_policy::reject;
  bool coordinates_may_change = false;
  digest policy_digest;
  digest case_digest;
};

struct qualification_result_mode_suite_case {
  std::uint16_t schema = qualification_suite_schema_version;
  std::string identifier;
  result_representation representation =
      result_representation::exact_stratified;
  product_realization_semantics semantics =
      product_realization_semantics::not_requested;
  model_unit tolerance_unit = model_unit::unspecified;
  std::uint64_t displacement_limit_binary64_bits = 0;
  std::uint64_t support_plane_limit_binary64_bits = 0;
  bool mesh_required = false;
  bool certificate_replay_required = false;
  bool strict_reingestion_required = false;
  bool approximation_bounds_required = false;
  digest policy_digest;
  digest case_digest;
};

struct qualification_attribute_suite_case {
  std::uint16_t schema = qualification_suite_schema_version;
  std::string identifier;
  qualification_attribute_case_kind kind =
      qualification_attribute_case_kind::transfer_mode;
  attribute_transfer_policy_contract policy;
  bool require_seam_evidence = false;
  bool require_conflict_evidence = false;
  bool require_omission_evidence = false;
  bool require_multi_source_mapping = false;
  bool require_downstream_source_query = false;
  std::string geometry_invariance_group;
  digest policy_digest;
  digest case_digest;
};

struct qualification_profile_suite_plan {
  std::uint16_t schema = qualification_suite_schema_version;
  std::uint32_t checker_version = qualification_suite_checker_version;
  std::string identifier;
  std::vector<qualification_preparation_suite_case> preparation_cases;
  std::vector<qualification_result_mode_suite_case> result_mode_cases;
  std::vector<qualification_attribute_suite_case> attribute_cases;
  std::vector<std::uint8_t> canonical_bytes;
  digest plan_digest;
};

struct qualification_preparation_suite_observation {
  std::uint16_t schema = qualification_suite_schema_version;
  std::string case_identifier;
  digest case_digest;
  digest source_digest;
  digest output_digest;
  digest policy_digest;
  digest report_digest;
  digest independent_verification_digest;
  bool strict_validation_ran = false;
  bool strict_validation_passed = false;
  bool independent_report_verified = false;
  bool prepared_operand_available = false;
  bool prepared_operand_strictly_revalidated = false;
  bool coordinates_changed = false;
  std::uint64_t edit_count = 0;
  normalization_displacement_claim displacement =
      normalization_displacement_claim::exact_zero;
  std::uint64_t displacement_record_count = 0;
  qualification_explicit_bound_evidence tolerance_evidence;
  digest observation_digest;
};

struct qualification_result_mode_suite_observation {
  std::uint16_t schema = qualification_suite_schema_version;
  std::string case_identifier;
  digest case_digest;
  digest exact_result_digest;
  digest output_digest;
  digest policy_digest;
  digest independent_verification_digest;
  bool exact_result_verified = false;
  bool exact_result_retained = false;
  bool mesh_published = false;
  bool representation_semantics_verified = false;
  bool strict_reingestion_passed = false;
  bool independent_topology_passed = false;
  bool embedding_passed = false;
  bool orientation_passed = false;
  bool shell_nesting_passed = false;
  bool certificate_replay_passed = false;
  bool approximation_bounds_passed = false;
  qualification_explicit_bound_evidence displacement_evidence;
  qualification_explicit_bound_evidence support_plane_evidence;
  digest observation_digest;
};

struct qualification_attribute_suite_observation {
  std::uint16_t schema = qualification_suite_schema_version;
  std::string case_identifier;
  digest case_digest;
  digest policy_digest;
  digest exact_result_digest;
  digest geometry_digest;
  digest report_digest;
  bool independent_report_verified = false;
  bool seam_evidence_verified = false;
  bool conflict_evidence_verified = false;
  bool omission_evidence_verified = false;
  bool multi_source_mapping_verified = false;
  bool downstream_source_query_verified = false;
  digest downstream_source_query_digest;
  bool attribute_values_invariant_to_geometry = false;
  bool hidden_tolerance_used = false;
  digest observation_digest;
};

struct qualification_profile_suite_report {
  std::uint16_t schema = qualification_suite_schema_version;
  std::uint32_t checker_version = qualification_suite_checker_version;
  std::string identifier;
  digest plan_digest;
  std::vector<qualification_preparation_suite_observation>
      preparation_observations;
  std::vector<qualification_result_mode_suite_observation>
      result_mode_observations;
  std::vector<qualification_attribute_suite_observation>
      attribute_observations;
  std::uint64_t passed_preparation_cases = 0;
  std::uint64_t passed_result_mode_cases = 0;
  std::uint64_t passed_attribute_cases = 0;
  std::uint64_t blocking_issue_count = 0;
  bool complete = false;
  std::vector<std::uint8_t> canonical_bytes;
  digest report_digest;
};

const char *qualification_suite_family_token(qualification_suite_family) noexcept;
const char *qualification_preparation_case_kind_token(
    qualification_preparation_case_kind) noexcept;
const char *qualification_attribute_case_kind_token(
    qualification_attribute_case_kind) noexcept;

qualification_profile_suite_plan make_default_qualification_profile_suite_plan();
product_status_or<qualification_profile_suite_plan>
make_qualification_profile_suite_plan(qualification_profile_suite_plan);
product_status_or<bool> validate_qualification_profile_suite_plan(
    const qualification_profile_suite_plan &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_profile_suite_plan(
    const qualification_profile_suite_plan &);

product_status_or<qualification_preparation_suite_observation>
make_qualification_preparation_suite_observation(
    const qualification_preparation_suite_case &,
    qualification_preparation_suite_observation);
product_status_or<qualification_result_mode_suite_observation>
make_qualification_result_mode_suite_observation(
    const qualification_result_mode_suite_case &,
    qualification_result_mode_suite_observation);
product_status_or<qualification_attribute_suite_observation>
make_qualification_attribute_suite_observation(
    const qualification_attribute_suite_case &,
    qualification_attribute_suite_observation);

product_status_or<qualification_profile_suite_report>
make_qualification_profile_suite_report(
    const qualification_profile_suite_plan &,
    std::vector<qualification_preparation_suite_observation>,
    std::vector<qualification_result_mode_suite_observation>,
    std::vector<qualification_attribute_suite_observation>,
    bool complete = true);
product_status_or<bool> validate_qualification_profile_suite_report(
    const qualification_profile_suite_report &,
    const qualification_profile_suite_plan &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_qualification_profile_suite_report(
    const qualification_profile_suite_report &);
bool qualification_profile_suite_gate_passes(
    const qualification_profile_suite_report &,
    const qualification_profile_suite_plan &) noexcept;

} // namespace mesh_boolean
} // namespace ygor

#endif
