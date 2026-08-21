#include "YgorMeshesBooleanQualificationSuites.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <new>
#include <set>
#include <stdexcept>
#include <tuple>

namespace ygor {
namespace mesh_boolean {
namespace {

constexpr std::array<char, 8> plan_tag{{'Y', 'G', 'B', 'Q', 'S', 'P', '0', '1'}};
constexpr std::array<char, 8> preparation_case_tag{{'Y', 'G', 'B', 'Q', 'S', 'C', '0', '1'}};
constexpr std::array<char, 8> result_case_tag{{'Y', 'G', 'B', 'Q', 'S', 'R', '0', '1'}};
constexpr std::array<char, 8> attribute_case_tag{{'Y', 'G', 'B', 'Q', 'S', 'A', '0', '1'}};
constexpr std::array<char, 8> bound_tag{{'Y', 'G', 'B', 'Q', 'S', 'B', '0', '1'}};
constexpr std::array<char, 8> preparation_observation_tag{{'Y', 'G', 'B', 'Q', 'S', 'O', '0', '1'}};
constexpr std::array<char, 8> result_observation_tag{{'Y', 'G', 'B', 'Q', 'S', 'M', '0', '1'}};
constexpr std::array<char, 8> attribute_observation_tag{{'Y', 'G', 'B', 'Q', 'S', 'T', '0', '1'}};
constexpr std::array<char, 8> report_tag{{'Y', 'G', 'B', 'Q', 'S', 'Q', '0', '1'}};

product_error suite_error(const char *key) {
  return make_product_error(product_error_code::qualification_policy_violation,
                            key);
}

bool zero(const digest &value) noexcept { return value == digest{}; }

bool text(const std::string &value) noexcept {
  return !value.empty() && value.size() <= 1024U &&
         std::find(value.begin(), value.end(), '\0') == value.end();
}

std::uint64_t double_bits(double value) noexcept {
  std::uint64_t bits = 0;
  static_assert(sizeof(bits) == sizeof(value), "binary64 size mismatch");
  std::memcpy(&bits, &value, sizeof(bits));
  return bits;
}

double double_value(std::uint64_t bits) noexcept {
  double value = 0.0;
  std::memcpy(&value, &bits, sizeof(value));
  return value;
}

bool nonnegative_finite_bits(std::uint64_t bits) noexcept {
  const auto value = double_value(bits);
  return std::isfinite(value) && value >= 0.0 && !std::signbit(value);
}

bool positive_finite_bits(std::uint64_t bits) noexcept {
  return nonnegative_finite_bits(bits) && double_value(bits) > 0.0;
}

bool known(model_unit value) noexcept {
  return value >= model_unit::unspecified && value <= model_unit::foot;
}
bool known(preparation_mode value) noexcept {
  return value >= preparation_mode::strict_validation &&
         value <= preparation_mode::normalized;
}
bool known(normalization_mode value) noexcept {
  return value >= normalization_mode::disabled &&
         value <= normalization_mode::geometry_changing;
}
bool known(normalization_operation value) noexcept {
  return value >= normalization_operation::irrelevant_storage_removal &&
         value < normalization_operation::count;
}
bool known(nonplanar_facet_policy value) noexcept {
  return value >= nonplanar_facet_policy::reject &&
         value <= nonplanar_facet_policy::axis_aligned_refit;
}
bool known(result_representation value) noexcept {
  return value >= result_representation::exact_stratified &&
         value <= result_representation::certified_approximate_mesh;
}
bool known(product_realization_semantics value) noexcept {
  return value >= product_realization_semantics::not_requested &&
         value <= product_realization_semantics::certified_approximate_embedding_v1;
}
bool known(qualification_preparation_case_kind value) noexcept {
  return value >= qualification_preparation_case_kind::strict_validation &&
         value <= qualification_preparation_case_kind::repair;
}
bool known(qualification_attribute_case_kind value) noexcept {
  return value >= qualification_attribute_case_kind::transfer_mode &&
         value <= qualification_attribute_case_kind::multi_source_mapping_and_query;
}
bool known(normalization_displacement_claim value) noexcept {
  return value >= normalization_displacement_claim::exact_zero &&
         value <= normalization_displacement_claim::records_present;
}

bool known(attribute_transfer_mode value) noexcept {
  return value >= attribute_transfer_mode::omit_all_with_report &&
         value <= attribute_transfer_mode::require_lossless;
}
bool known(attribute_conflict_policy value) noexcept {
  return value >= attribute_conflict_policy::report_and_omit &&
         value <= attribute_conflict_policy::reject;
}
bool known(attribute_identifier_policy value) noexcept {
  return value >= attribute_identifier_policy::preserve &&
         value <= attribute_identifier_policy::omit_with_report;
}
bool known(attribute_merge_policy value) noexcept {
  return value >= attribute_merge_policy::require_equal &&
         value <= attribute_merge_policy::omit_with_report;
}
bool known(attribute_vertex_copy_policy value) noexcept {
  return value >= attribute_vertex_copy_policy::exact_source_or_equal_merge &&
         value <= attribute_vertex_copy_policy::omit_with_report;
}
bool known(attribute_interpolation_policy value) noexcept {
  return value == attribute_interpolation_policy::prohibit_and_report;
}
bool known(attribute_sharp_edge_policy value) noexcept {
  return value >= attribute_sharp_edge_policy::any_source &&
         value <= attribute_sharp_edge_policy::omit_with_report;
}
bool known(attribute_texture_seam_policy value) noexcept {
  return value >= attribute_texture_seam_policy::preserve_source_sets &&
         value <= attribute_texture_seam_policy::omit_with_report;
}
bool known(attribute_construction_provenance_policy value) noexcept {
  return value >= attribute_construction_provenance_policy::compact_digest &&
         value <= attribute_construction_provenance_policy::omit_with_report;
}

const char *operation_token(normalization_operation value) noexcept {
  switch (value) {
  case normalization_operation::irrelevant_storage_removal:
    return "irrelevant-storage-removal";
  case normalization_operation::exact_duplicate_consolidation:
    return "exact-duplicate-consolidation";
  case normalization_operation::orientation_repair:
    return "orientation-repair";
  case normalization_operation::seam_aware_vertex_consolidation:
    return "seam-aware-vertex-consolidation";
  case normalization_operation::crack_closure:
    return "crack-closure";
  case normalization_operation::nonplanar_facet_handling:
    return "nonplanar-facet-handling";
  case normalization_operation::overlapping_facet_resolution:
    return "overlapping-facet-resolution";
  case normalization_operation::sliver_handling:
    return "sliver-handling";
  case normalization_operation::self_intersection_repair:
    return "self-intersection-repair";
  case normalization_operation::count:
    break;
  }
  return "unknown";
}

bool operation_uses_structural_mode(normalization_operation value) noexcept {
  return value == normalization_operation::irrelevant_storage_removal ||
         value == normalization_operation::exact_duplicate_consolidation ||
         value == normalization_operation::orientation_repair;
}

bool operation_uses_explicit_tolerance(normalization_operation value) noexcept {
  return value == normalization_operation::seam_aware_vertex_consolidation ||
         value == normalization_operation::crack_closure ||
         value == normalization_operation::nonplanar_facet_handling ||
         value == normalization_operation::sliver_handling;
}

void encode_digest(canonical_encoder &encoder, const digest &value) {
  encoder.raw(value.bytes.data(), value.bytes.size());
}

void encode_attribute_policy(canonical_encoder &encoder,
                             const attribute_transfer_policy_contract &policy) {
  encoder.u16(policy.schema);
  encoder.byte(static_cast<std::uint8_t>(policy.mode));
  encoder.byte(static_cast<std::uint8_t>(policy.conflicts));
  encoder.byte(static_cast<std::uint8_t>(policy.body_ids));
  encoder.byte(static_cast<std::uint8_t>(policy.shell_ids));
  encoder.byte(static_cast<std::uint8_t>(policy.facet_ids));
  encoder.byte(static_cast<std::uint8_t>(policy.materials));
  encoder.byte(static_cast<std::uint8_t>(policy.face_metadata));
  encoder.byte(static_cast<std::uint8_t>(policy.vertex_normals));
  encoder.byte(static_cast<std::uint8_t>(policy.vertex_colours));
  encoder.byte(static_cast<std::uint8_t>(policy.interpolation));
  encoder.byte(static_cast<std::uint8_t>(policy.sharp_edges));
  encoder.byte(static_cast<std::uint8_t>(policy.texture_seams));
  encoder.byte(static_cast<std::uint8_t>(policy.opaque_channels));
  encoder.byte(static_cast<std::uint8_t>(policy.construction_provenance));
  encoder.boolean(policy.report_absent_supported_channels);
}

bool valid_attribute_policy(const attribute_transfer_policy_contract &policy) noexcept {
  return policy.schema == product_contract_schema_version && known(policy.mode) &&
         known(policy.conflicts) && known(policy.body_ids) &&
         known(policy.shell_ids) && known(policy.facet_ids) &&
         known(policy.materials) && known(policy.face_metadata) &&
         known(policy.vertex_normals) && known(policy.vertex_colours) &&
         known(policy.interpolation) && known(policy.sharp_edges) &&
         known(policy.texture_seams) && known(policy.opaque_channels) &&
         known(policy.construction_provenance);
}

void encode_bound(canonical_encoder &encoder,
                  const qualification_explicit_bound_evidence &value,
                  bool include_digest) {
  encoder.u16(value.schema);
  encoder.byte(static_cast<std::uint8_t>(value.unit));
  encoder.u64(value.declared_limit_binary64_bits);
  encoder.u64(value.observed_maximum_binary64_bits);
  encode_digest(encoder, value.policy_digest);
  encode_digest(encoder, value.independent_verification_digest);
  encoder.boolean(value.independently_verified);
  encoder.boolean(value.hidden_epsilon_used);
  if (include_digest)
    encode_digest(encoder, value.evidence_digest);
}

digest bound_digest(const qualification_explicit_bound_evidence &value) {
  canonical_encoder encoder;
  encode_bound(encoder, value, false);
  return domain_digest(bound_tag, encoder.bytes());
}

bool valid_bound(const qualification_explicit_bound_evidence &value,
                 bool required, model_unit unit, std::uint64_t declared_bits,
                 const digest &policy_digest) noexcept {
  if (value.schema != qualification_suite_schema_version || !known(value.unit) ||
      !nonnegative_finite_bits(value.declared_limit_binary64_bits) ||
      !nonnegative_finite_bits(value.observed_maximum_binary64_bits) ||
      value.unit != unit ||
      value.declared_limit_binary64_bits != declared_bits ||
      value.policy_digest != policy_digest || value.hidden_epsilon_used ||
      value.evidence_digest != bound_digest(value))
    return false;
  if (!required)
    return value.unit == model_unit::unspecified && declared_bits == 0 &&
           value.observed_maximum_binary64_bits == 0 &&
           zero(value.independent_verification_digest) &&
           !value.independently_verified;
  return unit != model_unit::unspecified && positive_finite_bits(declared_bits) &&
         !zero(value.independent_verification_digest) &&
         value.independently_verified &&
         double_value(value.observed_maximum_binary64_bits) <=
             double_value(declared_bits);
}

void encode_preparation_case(canonical_encoder &encoder,
                             const qualification_preparation_suite_case &value,
                             bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.byte(static_cast<std::uint8_t>(value.kind));
  encoder.byte(static_cast<std::uint8_t>(value.preparation));
  encoder.byte(static_cast<std::uint8_t>(value.normalization));
  encoder.boolean(value.operation.has_value());
  if (value.operation)
    encoder.byte(static_cast<std::uint8_t>(*value.operation));
  encoder.byte(static_cast<std::uint8_t>(value.tolerance_unit));
  encoder.u64(value.model_tolerance_binary64_bits);
  encoder.byte(static_cast<std::uint8_t>(value.nonplanar_facets));
  encoder.boolean(value.coordinates_may_change);
  encode_digest(encoder, value.policy_digest);
  if (include_digest)
    encode_digest(encoder, value.case_digest);
}

void encode_result_case(canonical_encoder &encoder,
                        const qualification_result_mode_suite_case &value,
                        bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.byte(static_cast<std::uint8_t>(value.representation));
  encoder.byte(static_cast<std::uint8_t>(value.semantics));
  encoder.byte(static_cast<std::uint8_t>(value.tolerance_unit));
  encoder.u64(value.displacement_limit_binary64_bits);
  encoder.u64(value.support_plane_limit_binary64_bits);
  encoder.boolean(value.mesh_required);
  encoder.boolean(value.certificate_replay_required);
  encoder.boolean(value.strict_reingestion_required);
  encoder.boolean(value.approximation_bounds_required);
  encode_digest(encoder, value.policy_digest);
  if (include_digest)
    encode_digest(encoder, value.case_digest);
}

void encode_attribute_case(canonical_encoder &encoder,
                           const qualification_attribute_suite_case &value,
                           bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.identifier);
  encoder.byte(static_cast<std::uint8_t>(value.kind));
  encode_attribute_policy(encoder, value.policy);
  encoder.boolean(value.require_seam_evidence);
  encoder.boolean(value.require_conflict_evidence);
  encoder.boolean(value.require_omission_evidence);
  encoder.boolean(value.require_multi_source_mapping);
  encoder.boolean(value.require_downstream_source_query);
  encoder.string(value.geometry_invariance_group);
  encode_digest(encoder, value.policy_digest);
  if (include_digest)
    encode_digest(encoder, value.case_digest);
}

digest preparation_case_digest(
    const qualification_preparation_suite_case &value) {
  canonical_encoder encoder;
  encode_preparation_case(encoder, value, false);
  return domain_digest(preparation_case_tag, encoder.bytes());
}

digest result_case_digest(const qualification_result_mode_suite_case &value) {
  canonical_encoder encoder;
  encode_result_case(encoder, value, false);
  return domain_digest(result_case_tag, encoder.bytes());
}

digest attribute_case_digest(const qualification_attribute_suite_case &value) {
  canonical_encoder encoder;
  encode_attribute_case(encoder, value, false);
  return domain_digest(attribute_case_tag, encoder.bytes());
}

digest strict_policy_digest() {
  canonical_encoder encoder;
  encoder.u16(product_contract_schema_version);
  encoder.byte(static_cast<std::uint8_t>(preparation_mode::strict_validation));
  encoder.byte(static_cast<std::uint8_t>(normalization_mode::disabled));
  encoder.byte(static_cast<std::uint8_t>(model_unit::unspecified));
  encoder.u64(0);
  encoder.u64(0);
  return domain_digest({{'Y', 'G', 'B', 'Q', 'S', 'V', '0', '1'}},
                       encoder.bytes());
}

digest result_policy_digest(result_representation representation,
                            product_realization_semantics semantics,
                            model_unit unit, std::uint64_t displacement,
                            std::uint64_t support_plane) {
  canonical_encoder encoder;
  encoder.u16(product_contract_schema_version);
  encoder.byte(static_cast<std::uint8_t>(representation));
  encoder.byte(static_cast<std::uint8_t>(semantics));
  encoder.byte(static_cast<std::uint8_t>(unit));
  encoder.u64(displacement);
  encoder.u64(support_plane);
  return domain_digest({{'Y', 'G', 'B', 'Q', 'S', 'Y', '0', '1'}},
                       encoder.bytes());
}

product_status_or<qualification_preparation_suite_case>
canonicalize_preparation_case(qualification_preparation_suite_case value) {
  if (value.schema != qualification_suite_schema_version ||
      !text(value.identifier) || !known(value.kind) ||
      !known(value.preparation) || !known(value.normalization) ||
      !known(value.tolerance_unit) || !known(value.nonplanar_facets) ||
      (value.operation && !known(*value.operation)) ||
      !nonnegative_finite_bits(value.model_tolerance_binary64_bits) ||
      zero(value.policy_digest))
    return suite_error("qualification_suite.preparation_case_malformed");

  if (value.kind == qualification_preparation_case_kind::strict_validation) {
    if (value.preparation != preparation_mode::strict_validation ||
        value.normalization != normalization_mode::disabled || value.operation ||
        value.tolerance_unit != model_unit::unspecified ||
        value.model_tolerance_binary64_bits != 0 ||
        value.nonplanar_facets != nonplanar_facet_policy::reject ||
        value.coordinates_may_change)
      return suite_error("qualification_suite.strict_case_semantics");
  } else if (value.kind == qualification_preparation_case_kind::diagnosis) {
    if (value.preparation != preparation_mode::diagnosis_only ||
        value.normalization != normalization_mode::diagnosis_only ||
        value.operation || value.coordinates_may_change ||
        value.tolerance_unit != model_unit::unspecified ||
        value.model_tolerance_binary64_bits != 0 ||
        value.nonplanar_facets != nonplanar_facet_policy::reject)
      return suite_error("qualification_suite.diagnosis_case_semantics");
  } else {
    if (value.preparation != preparation_mode::normalized || !value.operation)
      return suite_error("qualification_suite.repair_case_semantics");
    const auto operation = *value.operation;
    const auto structural = operation_uses_structural_mode(operation);
    if ((structural && value.normalization != normalization_mode::structural_only) ||
        (!structural && value.normalization != normalization_mode::geometry_changing))
      return suite_error("qualification_suite.repair_mode_mismatch");
    const auto tolerance_required = operation_uses_explicit_tolerance(operation);
    if (tolerance_required != value.coordinates_may_change)
      return suite_error("qualification_suite.repair_displacement_contract");
    if (tolerance_required) {
      if (value.tolerance_unit == model_unit::unspecified ||
          !positive_finite_bits(value.model_tolerance_binary64_bits))
        return suite_error("qualification_suite.repair_tolerance_missing");
    } else if (value.tolerance_unit != model_unit::unspecified ||
               value.model_tolerance_binary64_bits != 0) {
      return suite_error("qualification_suite.repair_hidden_tolerance");
    }
    if ((operation == normalization_operation::nonplanar_facet_handling) !=
        (value.nonplanar_facets != nonplanar_facet_policy::reject))
      return suite_error("qualification_suite.nonplanar_policy_mismatch");
  }
  const auto calculated = preparation_case_digest(value);
  if (!zero(value.case_digest) && value.case_digest != calculated)
    return suite_error("qualification_suite.preparation_case_digest");
  value.case_digest = calculated;
  return value;
}

product_status_or<qualification_result_mode_suite_case>
canonicalize_result_case(qualification_result_mode_suite_case value) {
  if (value.schema != qualification_suite_schema_version ||
      !text(value.identifier) || !known(value.representation) ||
      !known(value.semantics) || !known(value.tolerance_unit) ||
      !nonnegative_finite_bits(value.displacement_limit_binary64_bits) ||
      !nonnegative_finite_bits(value.support_plane_limit_binary64_bits) ||
      zero(value.policy_digest))
    return suite_error("qualification_suite.result_case_malformed");
  if (value.representation == result_representation::exact_stratified) {
    if (value.semantics != product_realization_semantics::not_requested ||
        value.mesh_required || value.certificate_replay_required ||
        value.strict_reingestion_required ||
        value.approximation_bounds_required ||
        value.tolerance_unit != model_unit::unspecified ||
        value.displacement_limit_binary64_bits ||
        value.support_plane_limit_binary64_bits)
      return suite_error("qualification_suite.exact_stratified_semantics");
  } else if (value.representation == result_representation::exact_in_T_mesh) {
    if (value.semantics != product_realization_semantics::exact_in_T ||
        !value.mesh_required || !value.certificate_replay_required ||
        !value.strict_reingestion_required ||
        value.approximation_bounds_required ||
        value.tolerance_unit != model_unit::unspecified ||
        value.displacement_limit_binary64_bits ||
        value.support_plane_limit_binary64_bits)
      return suite_error("qualification_suite.exact_in_T_semantics");
  } else {
    if (value.semantics !=
            product_realization_semantics::certified_approximate_embedding_v1 ||
        !value.mesh_required || !value.certificate_replay_required ||
        !value.strict_reingestion_required ||
        !value.approximation_bounds_required ||
        value.tolerance_unit == model_unit::unspecified ||
        !positive_finite_bits(value.displacement_limit_binary64_bits) ||
        !positive_finite_bits(value.support_plane_limit_binary64_bits))
      return suite_error("qualification_suite.approximate_semantics");
  }
  const auto calculated = result_case_digest(value);
  if (!zero(value.case_digest) && value.case_digest != calculated)
    return suite_error("qualification_suite.result_case_digest");
  value.case_digest = calculated;
  return value;
}

product_status_or<qualification_attribute_suite_case>
canonicalize_attribute_case(qualification_attribute_suite_case value) {
  if (value.schema != qualification_suite_schema_version ||
      !text(value.identifier) || !known(value.kind) ||
      !valid_attribute_policy(value.policy) ||
      !text(value.geometry_invariance_group))
    return suite_error("qualification_suite.attribute_case_malformed");
  const auto calculated_policy = attribute_transfer_policy_digest(value.policy);
  if ((!zero(value.policy_digest) && value.policy_digest != calculated_policy) ||
      zero(calculated_policy))
    return suite_error("qualification_suite.attribute_policy_digest");
  value.policy_digest = calculated_policy;
  const auto calculated = attribute_case_digest(value);
  if (!zero(value.case_digest) && value.case_digest != calculated)
    return suite_error("qualification_suite.attribute_case_digest");
  value.case_digest = calculated;
  return value;
}

void encode_preparation_observation(
    canonical_encoder &encoder,
    const qualification_preparation_suite_observation &value,
    bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.case_identifier);
  encode_digest(encoder, value.case_digest);
  encode_digest(encoder, value.source_digest);
  encode_digest(encoder, value.output_digest);
  encode_digest(encoder, value.policy_digest);
  encode_digest(encoder, value.report_digest);
  encode_digest(encoder, value.independent_verification_digest);
  encoder.boolean(value.strict_validation_ran);
  encoder.boolean(value.strict_validation_passed);
  encoder.boolean(value.independent_report_verified);
  encoder.boolean(value.prepared_operand_available);
  encoder.boolean(value.prepared_operand_strictly_revalidated);
  encoder.boolean(value.coordinates_changed);
  encoder.u64(value.edit_count);
  encoder.byte(static_cast<std::uint8_t>(value.displacement));
  encoder.u64(value.displacement_record_count);
  encode_bound(encoder, value.tolerance_evidence, true);
  if (include_digest)
    encode_digest(encoder, value.observation_digest);
}

void encode_result_observation(
    canonical_encoder &encoder,
    const qualification_result_mode_suite_observation &value,
    bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.case_identifier);
  encode_digest(encoder, value.case_digest);
  encode_digest(encoder, value.exact_result_digest);
  encode_digest(encoder, value.output_digest);
  encode_digest(encoder, value.policy_digest);
  encode_digest(encoder, value.independent_verification_digest);
  encoder.boolean(value.exact_result_verified);
  encoder.boolean(value.exact_result_retained);
  encoder.boolean(value.mesh_published);
  encoder.boolean(value.representation_semantics_verified);
  encoder.boolean(value.strict_reingestion_passed);
  encoder.boolean(value.independent_topology_passed);
  encoder.boolean(value.embedding_passed);
  encoder.boolean(value.orientation_passed);
  encoder.boolean(value.shell_nesting_passed);
  encoder.boolean(value.certificate_replay_passed);
  encoder.boolean(value.approximation_bounds_passed);
  encode_bound(encoder, value.displacement_evidence, true);
  encode_bound(encoder, value.support_plane_evidence, true);
  if (include_digest)
    encode_digest(encoder, value.observation_digest);
}

void encode_attribute_observation(
    canonical_encoder &encoder,
    const qualification_attribute_suite_observation &value,
    bool include_digest) {
  encoder.u16(value.schema);
  encoder.string(value.case_identifier);
  encode_digest(encoder, value.case_digest);
  encode_digest(encoder, value.policy_digest);
  encode_digest(encoder, value.exact_result_digest);
  encode_digest(encoder, value.geometry_digest);
  encode_digest(encoder, value.report_digest);
  encoder.boolean(value.independent_report_verified);
  encoder.boolean(value.seam_evidence_verified);
  encoder.boolean(value.conflict_evidence_verified);
  encoder.boolean(value.omission_evidence_verified);
  encoder.boolean(value.multi_source_mapping_verified);
  encoder.boolean(value.downstream_source_query_verified);
  encode_digest(encoder, value.downstream_source_query_digest);
  encoder.boolean(value.attribute_values_invariant_to_geometry);
  encoder.boolean(value.hidden_tolerance_used);
  if (include_digest)
    encode_digest(encoder, value.observation_digest);
}

digest preparation_observation_digest(
    const qualification_preparation_suite_observation &value) {
  canonical_encoder encoder;
  encode_preparation_observation(encoder, value, false);
  return domain_digest(preparation_observation_tag, encoder.bytes());
}

digest result_observation_digest(
    const qualification_result_mode_suite_observation &value) {
  canonical_encoder encoder;
  encode_result_observation(encoder, value, false);
  return domain_digest(result_observation_tag, encoder.bytes());
}

digest attribute_observation_digest(
    const qualification_attribute_suite_observation &value) {
  canonical_encoder encoder;
  encode_attribute_observation(encoder, value, false);
  return domain_digest(attribute_observation_tag, encoder.bytes());
}

bool preparation_observation_passes(
    const qualification_preparation_suite_case &descriptor,
    const qualification_preparation_suite_observation &value) noexcept {
  if (value.schema != qualification_suite_schema_version ||
      value.case_identifier != descriptor.identifier ||
      value.case_digest != descriptor.case_digest ||
      value.policy_digest != descriptor.policy_digest ||
      zero(value.source_digest) || zero(value.independent_verification_digest) ||
      !value.strict_validation_ran ||
      !known(value.displacement) ||
      value.observation_digest != preparation_observation_digest(value))
    return false;

  const auto bound_required = descriptor.coordinates_may_change;
  if (!valid_bound(value.tolerance_evidence, bound_required,
                   descriptor.tolerance_unit,
                   descriptor.model_tolerance_binary64_bits,
                   descriptor.policy_digest))
    return false;

  if (descriptor.kind == qualification_preparation_case_kind::strict_validation) {
    return value.strict_validation_passed && zero(value.output_digest) &&
           zero(value.report_digest) && !value.independent_report_verified &&
           !value.prepared_operand_available &&
           !value.prepared_operand_strictly_revalidated &&
           !value.coordinates_changed && value.edit_count == 0 &&
           value.displacement == normalization_displacement_claim::exact_zero &&
           value.displacement_record_count == 0;
  }
  if (descriptor.kind == qualification_preparation_case_kind::diagnosis) {
    return !zero(value.report_digest) && value.independent_report_verified &&
           !value.prepared_operand_available && zero(value.output_digest) &&
           !value.prepared_operand_strictly_revalidated &&
           !value.coordinates_changed && value.edit_count == 0 &&
           value.displacement == normalization_displacement_claim::exact_zero &&
           value.displacement_record_count == 0;
  }
  if (zero(value.report_digest) || zero(value.output_digest) ||
      !value.independent_report_verified || !value.prepared_operand_available ||
      !value.prepared_operand_strictly_revalidated || value.edit_count == 0)
    return false;
  if (!descriptor.coordinates_may_change)
    return !value.coordinates_changed &&
           value.displacement == normalization_displacement_claim::exact_zero &&
           value.displacement_record_count == 0;
  return value.coordinates_changed &&
         value.displacement == normalization_displacement_claim::records_present &&
         value.displacement_record_count != 0;
}

bool result_observation_passes(
    const qualification_result_mode_suite_case &descriptor,
    const qualification_result_mode_suite_observation &value) noexcept {
  if (value.schema != qualification_suite_schema_version ||
      value.case_identifier != descriptor.identifier ||
      value.case_digest != descriptor.case_digest ||
      value.policy_digest != descriptor.policy_digest ||
      zero(value.exact_result_digest) ||
      zero(value.independent_verification_digest) ||
      !value.exact_result_verified || !value.exact_result_retained ||
      !value.representation_semantics_verified ||
      value.observation_digest != result_observation_digest(value))
    return false;

  const auto approximate = descriptor.approximation_bounds_required;
  if (!valid_bound(value.displacement_evidence, approximate,
                   descriptor.tolerance_unit,
                   descriptor.displacement_limit_binary64_bits,
                   descriptor.policy_digest) ||
      !valid_bound(value.support_plane_evidence, approximate,
                   descriptor.tolerance_unit,
                   descriptor.support_plane_limit_binary64_bits,
                   descriptor.policy_digest))
    return false;

  if (descriptor.mesh_required != value.mesh_published)
    return false;
  if (!descriptor.mesh_required)
    return zero(value.output_digest) && !value.strict_reingestion_passed &&
           !value.independent_topology_passed && !value.embedding_passed &&
           !value.orientation_passed && !value.shell_nesting_passed &&
           !value.certificate_replay_passed &&
           !value.approximation_bounds_passed;
  return !zero(value.output_digest) && value.strict_reingestion_passed &&
         value.independent_topology_passed && value.embedding_passed &&
         value.orientation_passed && value.shell_nesting_passed &&
         value.certificate_replay_passed &&
         value.approximation_bounds_passed == approximate;
}

bool attribute_observation_passes(
    const qualification_attribute_suite_case &descriptor,
    const qualification_attribute_suite_observation &value) noexcept {
  if (value.schema != qualification_suite_schema_version ||
      value.case_identifier != descriptor.identifier ||
      value.case_digest != descriptor.case_digest ||
      value.policy_digest != descriptor.policy_digest ||
      zero(value.exact_result_digest) || zero(value.geometry_digest) ||
      zero(value.report_digest) || !value.independent_report_verified ||
      !value.attribute_values_invariant_to_geometry || value.hidden_tolerance_used ||
      value.observation_digest != attribute_observation_digest(value))
    return false;
  if (descriptor.require_seam_evidence && !value.seam_evidence_verified)
    return false;
  if (descriptor.require_conflict_evidence && !value.conflict_evidence_verified)
    return false;
  if (descriptor.require_omission_evidence && !value.omission_evidence_verified)
    return false;
  if (descriptor.require_multi_source_mapping &&
      !value.multi_source_mapping_verified)
    return false;
  if (descriptor.require_downstream_source_query &&
      (!value.downstream_source_query_verified ||
       zero(value.downstream_source_query_digest)))
    return false;
  return true;
}

attribute_transfer_policy_contract base_attribute_policy() {
  attribute_transfer_policy_contract policy;
  policy.mode = attribute_transfer_mode::preserve_supported_with_report;
  policy.conflicts = attribute_conflict_policy::report_and_omit;
  return policy;
}

qualification_attribute_suite_case attribute_case(
    std::string identifier, qualification_attribute_case_kind kind,
    attribute_transfer_policy_contract policy, bool seams, bool conflicts,
    bool omissions, bool multi_source, bool query) {
  qualification_attribute_suite_case result;
  result.identifier = std::move(identifier);
  result.kind = kind;
  result.policy = std::move(policy);
  result.require_seam_evidence = seams;
  result.require_conflict_evidence = conflicts;
  result.require_omission_evidence = omissions;
  result.require_multi_source_mapping = multi_source;
  result.require_downstream_source_query = query;
  result.geometry_invariance_group = "attribute-values-do-not-affect-geometry-v1";
  result.policy_digest = attribute_transfer_policy_digest(result.policy);
  return result;
}

bool check_attribute_catalog_coverage(
    const std::vector<qualification_attribute_suite_case> &cases) {
  constexpr std::size_t expected_case_count = 23;
  constexpr std::size_t kind_count =
      static_cast<std::size_t>(
          qualification_attribute_case_kind::multi_source_mapping_and_query) +
      1U;
  if (cases.size() != expected_case_count)
    return false;

  std::array<std::uint64_t, kind_count> counts{};
  std::set<attribute_transfer_mode> transfer_modes;
  std::set<attribute_conflict_policy> conflicts;
  std::set<attribute_identifier_policy> identifiers;
  std::set<attribute_merge_policy> merges;
  std::set<attribute_vertex_copy_policy> vertex_copy;
  std::set<attribute_sharp_edge_policy> sharp_edges;
  std::set<attribute_texture_seam_policy> seams;
  std::set<attribute_construction_provenance_policy> provenance;
  bool multi_source = false, query = false, seam_evidence = false,
       conflict_evidence = false, omission_evidence = false;
  for (const auto &entry : cases) {
    const auto kind_index = static_cast<std::size_t>(entry.kind);
    if (kind_index >= counts.size())
      return false;
    ++counts[kind_index];
    switch (entry.kind) {
    case qualification_attribute_case_kind::transfer_mode:
      transfer_modes.insert(entry.policy.mode);
      break;
    case qualification_attribute_case_kind::conflict_policy:
      conflicts.insert(entry.policy.conflicts);
      break;
    case qualification_attribute_case_kind::identifier_policy:
      if (entry.policy.body_ids != entry.policy.shell_ids ||
          entry.policy.body_ids != entry.policy.facet_ids)
        return false;
      identifiers.insert(entry.policy.body_ids);
      break;
    case qualification_attribute_case_kind::merge_policy:
      if (entry.policy.materials != entry.policy.face_metadata ||
          entry.policy.materials != entry.policy.opaque_channels)
        return false;
      merges.insert(entry.policy.materials);
      break;
    case qualification_attribute_case_kind::vertex_copy_policy:
      if (entry.policy.vertex_normals != entry.policy.vertex_colours)
        return false;
      vertex_copy.insert(entry.policy.vertex_normals);
      break;
    case qualification_attribute_case_kind::sharp_edge_policy:
      sharp_edges.insert(entry.policy.sharp_edges);
      break;
    case qualification_attribute_case_kind::texture_seam_policy:
      seams.insert(entry.policy.texture_seams);
      break;
    case qualification_attribute_case_kind::construction_provenance_policy:
      provenance.insert(entry.policy.construction_provenance);
      break;
    case qualification_attribute_case_kind::multi_source_mapping_and_query:
      if (!entry.require_multi_source_mapping ||
          !entry.require_downstream_source_query)
        return false;
      multi_source = true;
      query = true;
      break;
    }
    multi_source = multi_source || entry.require_multi_source_mapping;
    query = query || entry.require_downstream_source_query;
    seam_evidence = seam_evidence || entry.require_seam_evidence;
    conflict_evidence = conflict_evidence || entry.require_conflict_evidence;
    omission_evidence = omission_evidence || entry.require_omission_evidence;
  }

  const std::array<std::uint64_t, kind_count> expected_counts{{
      3, // transfer mode
      2, // conflict policy
      2, // identifier policy
      4, // merge policy
      3, // vertex copy policy
      3, // sharp-edge policy
      3, // texture-seam policy
      2, // construction provenance policy
      1  // multi-source mapping and query
  }};
  return counts == expected_counts && transfer_modes.size() == 3 &&
         conflicts.size() == 2 && identifiers.size() == 2 &&
         merges.size() == 4 && vertex_copy.size() == 3 &&
         sharp_edges.size() == 3 && seams.size() == 3 &&
         provenance.size() == 2 && multi_source && query && seam_evidence &&
         conflict_evidence && omission_evidence;
}

} // namespace

const char *qualification_suite_family_token(
    qualification_suite_family value) noexcept {
  switch (value) {
  case qualification_suite_family::preparation:
    return "preparation";
  case qualification_suite_family::result_mode:
    return "result_mode";
  case qualification_suite_family::attribute_and_provenance:
    return "attribute_and_provenance";
  }
  return "unknown";
}

const char *qualification_preparation_case_kind_token(
    qualification_preparation_case_kind value) noexcept {
  switch (value) {
  case qualification_preparation_case_kind::strict_validation:
    return "strict_validation";
  case qualification_preparation_case_kind::diagnosis:
    return "diagnosis";
  case qualification_preparation_case_kind::repair:
    return "repair";
  }
  return "unknown";
}

const char *qualification_attribute_case_kind_token(
    qualification_attribute_case_kind value) noexcept {
  switch (value) {
  case qualification_attribute_case_kind::transfer_mode:
    return "transfer_mode";
  case qualification_attribute_case_kind::conflict_policy:
    return "conflict_policy";
  case qualification_attribute_case_kind::identifier_policy:
    return "identifier_policy";
  case qualification_attribute_case_kind::merge_policy:
    return "merge_policy";
  case qualification_attribute_case_kind::vertex_copy_policy:
    return "vertex_copy_policy";
  case qualification_attribute_case_kind::sharp_edge_policy:
    return "sharp_edge_policy";
  case qualification_attribute_case_kind::texture_seam_policy:
    return "texture_seam_policy";
  case qualification_attribute_case_kind::construction_provenance_policy:
    return "construction_provenance_policy";
  case qualification_attribute_case_kind::multi_source_mapping_and_query:
    return "multi_source_mapping_and_query";
  }
  return "unknown";
}

qualification_profile_suite_plan make_default_qualification_profile_suite_plan() {
  qualification_profile_suite_plan plan;
  plan.identifier = "mesh-boolean-p6.7-profile-suites-v1";

  qualification_preparation_suite_case strict;
  strict.identifier = "preparation.strict-validation-v1";
  strict.kind = qualification_preparation_case_kind::strict_validation;
  strict.policy_digest = strict_policy_digest();
  plan.preparation_cases.push_back(strict);

  qualification_preparation_suite_case diagnosis;
  diagnosis.identifier = "preparation.diagnosis-only-v1";
  diagnosis.kind = qualification_preparation_case_kind::diagnosis;
  diagnosis.preparation = preparation_mode::diagnosis_only;
  diagnosis.normalization = normalization_mode::diagnosis_only;
  normalization_policy diagnosis_policy;
  diagnosis_policy.mode = normalization_mode::diagnosis_only;
  auto diagnosis_digest = normalization_policy_digest(diagnosis_policy);
  if (!diagnosis_digest.has_value())
    throw std::logic_error("default diagnosis policy rejected");
  diagnosis.policy_digest = diagnosis_digest.value();
  plan.preparation_cases.push_back(diagnosis);

  for (unsigned raw = 0;
       raw < static_cast<unsigned>(normalization_operation::count); ++raw) {
    const auto operation = static_cast<normalization_operation>(raw);
    qualification_preparation_suite_case entry;
    entry.identifier = std::string("preparation.repair.") +
                       operation_token(operation) + "-v1";
    entry.kind = qualification_preparation_case_kind::repair;
    entry.preparation = preparation_mode::normalized;
    entry.normalization = operation_uses_structural_mode(operation)
                              ? normalization_mode::structural_only
                              : normalization_mode::geometry_changing;
    entry.operation = operation;
    entry.coordinates_may_change = operation_uses_explicit_tolerance(operation);
    if (entry.coordinates_may_change) {
      entry.tolerance_unit = model_unit::millimetre;
      entry.model_tolerance_binary64_bits = double_bits(0.01);
    }
    if (operation == normalization_operation::nonplanar_facet_handling)
      entry.nonplanar_facets = nonplanar_facet_policy::triangulate;
    normalization_policy policy;
    policy.mode = entry.normalization;
    policy.unit = entry.tolerance_unit;
    policy.model_tolerance = double_value(entry.model_tolerance_binary64_bits);
    policy.enabled_operations = normalization_operation_bit(operation);
    policy.nonplanar_facets = entry.nonplanar_facets;
    auto digest_value = normalization_policy_digest(policy);
    if (!digest_value.has_value())
      throw std::logic_error("default repair policy rejected");
    entry.policy_digest = digest_value.value();
    plan.preparation_cases.push_back(std::move(entry));
  }

  qualification_result_mode_suite_case exact;
  exact.identifier = "result.exact-stratified-v1";
  exact.policy_digest = result_policy_digest(
      exact.representation, exact.semantics, exact.tolerance_unit, 0, 0);
  plan.result_mode_cases.push_back(exact);

  qualification_result_mode_suite_case strict_mesh;
  strict_mesh.identifier = "result.exact-in-T-v1";
  strict_mesh.representation = result_representation::exact_in_T_mesh;
  strict_mesh.semantics = product_realization_semantics::exact_in_T;
  strict_mesh.mesh_required = true;
  strict_mesh.certificate_replay_required = true;
  strict_mesh.strict_reingestion_required = true;
  strict_mesh.policy_digest = result_policy_digest(
      strict_mesh.representation, strict_mesh.semantics,
      strict_mesh.tolerance_unit, 0, 0);
  plan.result_mode_cases.push_back(strict_mesh);

  qualification_result_mode_suite_case approximate;
  approximate.identifier = "result.certified-approximate-embedding-v1";
  approximate.representation =
      result_representation::certified_approximate_mesh;
  approximate.semantics =
      product_realization_semantics::certified_approximate_embedding_v1;
  approximate.tolerance_unit = model_unit::millimetre;
  approximate.displacement_limit_binary64_bits = double_bits(0.01);
  approximate.support_plane_limit_binary64_bits = double_bits(0.005);
  approximate.mesh_required = true;
  approximate.certificate_replay_required = true;
  approximate.strict_reingestion_required = true;
  approximate.approximation_bounds_required = true;
  approximate.policy_digest = result_policy_digest(
      approximate.representation, approximate.semantics,
      approximate.tolerance_unit, approximate.displacement_limit_binary64_bits,
      approximate.support_plane_limit_binary64_bits);
  plan.result_mode_cases.push_back(approximate);

  {
    auto policy = base_attribute_policy();
    policy.mode = attribute_transfer_mode::omit_all_with_report;
    plan.attribute_cases.push_back(attribute_case(
        "attribute.transfer.omit-all-v1",
        qualification_attribute_case_kind::transfer_mode, policy, false, false,
        true, false, false));
    policy = base_attribute_policy();
    policy.mode = attribute_transfer_mode::preserve_supported_with_report;
    plan.attribute_cases.push_back(attribute_case(
        "attribute.transfer.preserve-supported-v1",
        qualification_attribute_case_kind::transfer_mode, policy, true, true,
        true, true, true));
    policy = base_attribute_policy();
    policy.mode = attribute_transfer_mode::require_lossless;
    plan.attribute_cases.push_back(attribute_case(
        "attribute.transfer.require-lossless-v1",
        qualification_attribute_case_kind::transfer_mode, policy, true, true,
        false, true, true));
  }
  for (const auto value : {attribute_conflict_policy::report_and_omit,
                           attribute_conflict_policy::reject}) {
    auto policy = base_attribute_policy();
    policy.conflicts = value;
    plan.attribute_cases.push_back(attribute_case(
        value == attribute_conflict_policy::report_and_omit
            ? "attribute.conflict.report-and-omit-v1"
            : "attribute.conflict.reject-v1",
        qualification_attribute_case_kind::conflict_policy, policy, false, true,
        value == attribute_conflict_policy::report_and_omit, true, true));
  }
  for (const auto value : {attribute_identifier_policy::preserve,
                           attribute_identifier_policy::omit_with_report}) {
    auto policy = base_attribute_policy();
    policy.body_ids = policy.shell_ids = policy.facet_ids = value;
    plan.attribute_cases.push_back(attribute_case(
        value == attribute_identifier_policy::preserve
            ? "attribute.identifiers.preserve-v1"
            : "attribute.identifiers.omit-v1",
        qualification_attribute_case_kind::identifier_policy, policy, false,
        false, value == attribute_identifier_policy::omit_with_report, true,
        true));
  }
  for (const auto value : {attribute_merge_policy::require_equal,
                           attribute_merge_policy::choose_representative,
                           attribute_merge_policy::deterministic_set_union,
                           attribute_merge_policy::omit_with_report}) {
    auto policy = base_attribute_policy();
    policy.materials = policy.face_metadata = policy.opaque_channels = value;
    const char *token = "require-equal";
    if (value == attribute_merge_policy::choose_representative)
      token = "choose-representative";
    else if (value == attribute_merge_policy::deterministic_set_union)
      token = "deterministic-set-union";
    else if (value == attribute_merge_policy::omit_with_report)
      token = "omit";
    plan.attribute_cases.push_back(attribute_case(
        std::string("attribute.merge.") + token + "-v1",
        qualification_attribute_case_kind::merge_policy, policy, false,
        value == attribute_merge_policy::require_equal,
        value == attribute_merge_policy::omit_with_report, true, true));
  }
  for (const auto value : {
           attribute_vertex_copy_policy::exact_source_or_equal_merge,
           attribute_vertex_copy_policy::choose_representative,
           attribute_vertex_copy_policy::omit_with_report}) {
    auto policy = base_attribute_policy();
    policy.vertex_normals = policy.vertex_colours = value;
    const char *token = "exact-source-or-equal-merge";
    if (value == attribute_vertex_copy_policy::choose_representative)
      token = "choose-representative";
    else if (value == attribute_vertex_copy_policy::omit_with_report)
      token = "omit";
    plan.attribute_cases.push_back(attribute_case(
        std::string("attribute.vertex-copy.") + token + "-v1",
        qualification_attribute_case_kind::vertex_copy_policy, policy, false,
        value == attribute_vertex_copy_policy::exact_source_or_equal_merge,
        value == attribute_vertex_copy_policy::omit_with_report, true, true));
  }
  for (const auto value : {attribute_sharp_edge_policy::any_source,
                           attribute_sharp_edge_policy::require_equal,
                           attribute_sharp_edge_policy::omit_with_report}) {
    auto policy = base_attribute_policy();
    policy.sharp_edges = value;
    const char *token = value == attribute_sharp_edge_policy::any_source
                            ? "any-source"
                            : value == attribute_sharp_edge_policy::require_equal
                                  ? "require-equal"
                                  : "omit";
    plan.attribute_cases.push_back(attribute_case(
        std::string("attribute.sharp-edge.") + token + "-v1",
        qualification_attribute_case_kind::sharp_edge_policy, policy, false,
        value == attribute_sharp_edge_policy::require_equal,
        value == attribute_sharp_edge_policy::omit_with_report, true, true));
  }
  for (const auto value : {
           attribute_texture_seam_policy::preserve_source_sets,
           attribute_texture_seam_policy::require_equal,
           attribute_texture_seam_policy::omit_with_report}) {
    auto policy = base_attribute_policy();
    policy.texture_seams = value;
    const char *token =
        value == attribute_texture_seam_policy::preserve_source_sets
            ? "preserve-source-sets"
            : value == attribute_texture_seam_policy::require_equal
                  ? "require-equal"
                  : "omit";
    plan.attribute_cases.push_back(attribute_case(
        std::string("attribute.texture-seam.") + token + "-v1",
        qualification_attribute_case_kind::texture_seam_policy, policy, true,
        value == attribute_texture_seam_policy::require_equal,
        value == attribute_texture_seam_policy::omit_with_report, true, true));
  }
  for (const auto value : {
           attribute_construction_provenance_policy::compact_digest,
           attribute_construction_provenance_policy::omit_with_report}) {
    auto policy = base_attribute_policy();
    policy.construction_provenance = value;
    plan.attribute_cases.push_back(attribute_case(
        value == attribute_construction_provenance_policy::compact_digest
            ? "attribute.construction-provenance.compact-digest-v1"
            : "attribute.construction-provenance.omit-v1",
        qualification_attribute_case_kind::construction_provenance_policy,
        policy, false, false,
        value ==
            attribute_construction_provenance_policy::omit_with_report,
        true, true));
  }
  plan.attribute_cases.push_back(attribute_case(
      "attribute.multi-source-mapping-and-query-v1",
      qualification_attribute_case_kind::multi_source_mapping_and_query,
      base_attribute_policy(), true, true, true, true, true));

  auto made = make_qualification_profile_suite_plan(std::move(plan));
  if (!made.has_value())
    throw std::logic_error("default qualification suite plan rejected");
  return made.value();
}

product_status_or<qualification_profile_suite_plan>
make_qualification_profile_suite_plan(qualification_profile_suite_plan plan) {
  try {
    if (plan.schema != qualification_suite_schema_version ||
        plan.checker_version != qualification_suite_checker_version ||
        !text(plan.identifier) || plan.preparation_cases.empty() ||
        plan.result_mode_cases.empty() || plan.attribute_cases.empty())
      return suite_error("qualification_suite.plan_malformed");

    for (auto &entry : plan.preparation_cases) {
      auto canonical = canonicalize_preparation_case(std::move(entry));
      if (!canonical.has_value())
        return canonical.error();
      entry = std::move(canonical.value());
    }
    for (auto &entry : plan.result_mode_cases) {
      auto canonical = canonicalize_result_case(std::move(entry));
      if (!canonical.has_value())
        return canonical.error();
      entry = std::move(canonical.value());
    }
    for (auto &entry : plan.attribute_cases) {
      auto canonical = canonicalize_attribute_case(std::move(entry));
      if (!canonical.has_value())
        return canonical.error();
      entry = std::move(canonical.value());
    }
    const auto by_identifier = [](const auto &a, const auto &b) {
      return a.identifier < b.identifier;
    };
    std::sort(plan.preparation_cases.begin(), plan.preparation_cases.end(),
              by_identifier);
    std::sort(plan.result_mode_cases.begin(), plan.result_mode_cases.end(),
              by_identifier);
    std::sort(plan.attribute_cases.begin(), plan.attribute_cases.end(),
              by_identifier);
    const auto unique_identifiers = [](const auto &values) {
      for (std::size_t i = 1; i != values.size(); ++i)
        if (values[i - 1].identifier == values[i].identifier)
          return false;
      return true;
    };
    if (!unique_identifiers(plan.preparation_cases) ||
        !unique_identifiers(plan.result_mode_cases) ||
        !unique_identifiers(plan.attribute_cases))
      return suite_error("qualification_suite.duplicate_case_identifier");

    std::map<normalization_operation, std::uint64_t> repair_operations;
    std::uint64_t strict_count = 0, diagnosis_count = 0;
    for (const auto &entry : plan.preparation_cases) {
      if (entry.kind == qualification_preparation_case_kind::strict_validation)
        ++strict_count;
      else if (entry.kind == qualification_preparation_case_kind::diagnosis)
        ++diagnosis_count;
      else
        ++repair_operations[*entry.operation];
    }
    if (strict_count != 1 || diagnosis_count != 1 ||
        plan.preparation_cases.size() !=
            2U + static_cast<std::size_t>(normalization_operation::count) ||
        repair_operations.size() !=
            static_cast<std::size_t>(normalization_operation::count) ||
        std::any_of(repair_operations.begin(), repair_operations.end(),
                    [](const auto &entry) { return entry.second != 1; }))
      return suite_error("qualification_suite.preparation_coverage_incomplete");

    std::set<result_representation> representations;
    for (const auto &entry : plan.result_mode_cases)
      representations.insert(entry.representation);
    if (representations.size() != 3 || plan.result_mode_cases.size() != 3)
      return suite_error("qualification_suite.result_mode_coverage_incomplete");
    if (!check_attribute_catalog_coverage(plan.attribute_cases))
      return suite_error("qualification_suite.attribute_coverage_incomplete");

    canonical_encoder encoder;
    encoder.u16(plan.schema);
    encoder.u32(plan.checker_version);
    encoder.string(plan.identifier);
    encoder.u64(plan.preparation_cases.size());
    for (const auto &entry : plan.preparation_cases)
      encode_preparation_case(encoder, entry, true);
    encoder.u64(plan.result_mode_cases.size());
    for (const auto &entry : plan.result_mode_cases)
      encode_result_case(encoder, entry, true);
    encoder.u64(plan.attribute_cases.size());
    for (const auto &entry : plan.attribute_cases)
      encode_attribute_case(encoder, entry, true);
    const auto bytes = encoder.bytes();
    const auto calculated = domain_digest(plan_tag, bytes);
    if ((!plan.canonical_bytes.empty() && plan.canonical_bytes != bytes) ||
        (!zero(plan.plan_digest) && plan.plan_digest != calculated))
      return suite_error("qualification_suite.plan_binding_mismatch");
    plan.canonical_bytes = bytes;
    plan.plan_digest = calculated;
    return plan;
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "qualification_suite.plan_allocation");
  }
}

product_status_or<bool> validate_qualification_profile_suite_plan(
    const qualification_profile_suite_plan &plan) noexcept {
  try {
    auto copy = plan;
    copy.canonical_bytes.clear();
    copy.plan_digest = {};
    auto made = make_qualification_profile_suite_plan(std::move(copy));
    if (!made.has_value() || made.value().canonical_bytes != plan.canonical_bytes ||
        made.value().plan_digest != plan.plan_digest)
      return suite_error("qualification_suite.plan_validation_failed");
    return true;
  } catch (...) {
    return suite_error("qualification_suite.plan_validation_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_profile_suite_plan(
    const qualification_profile_suite_plan &plan) {
  auto valid = validate_qualification_profile_suite_plan(plan);
  if (!valid.has_value())
    return valid.error();
  return plan.canonical_bytes;
}

product_status_or<qualification_preparation_suite_observation>
make_qualification_preparation_suite_observation(
    const qualification_preparation_suite_case &descriptor,
    qualification_preparation_suite_observation value) {
  try {
    auto canonical_descriptor = canonicalize_preparation_case(descriptor);
    if (!canonical_descriptor.has_value())
      return canonical_descriptor.error();
    const auto &entry = canonical_descriptor.value();
    if (value.schema != qualification_suite_schema_version ||
        (!value.case_identifier.empty() &&
         value.case_identifier != entry.identifier) ||
        (!zero(value.case_digest) && value.case_digest != entry.case_digest) ||
        (!zero(value.policy_digest) &&
         value.policy_digest != entry.policy_digest))
      return suite_error("qualification_suite.preparation_observation_binding");
    if ((value.tolerance_evidence.schema != qualification_suite_schema_version) ||
        (value.tolerance_evidence.unit != model_unit::unspecified &&
         value.tolerance_evidence.unit != entry.tolerance_unit) ||
        (value.tolerance_evidence.declared_limit_binary64_bits != 0 &&
         value.tolerance_evidence.declared_limit_binary64_bits !=
             entry.model_tolerance_binary64_bits) ||
        (!zero(value.tolerance_evidence.policy_digest) &&
         value.tolerance_evidence.policy_digest != entry.policy_digest))
      return suite_error("qualification_suite.preparation_bound_binding");
    value.case_identifier = entry.identifier;
    value.case_digest = entry.case_digest;
    value.policy_digest = entry.policy_digest;
    value.tolerance_evidence.schema = qualification_suite_schema_version;
    value.tolerance_evidence.unit = entry.tolerance_unit;
    value.tolerance_evidence.declared_limit_binary64_bits =
        entry.model_tolerance_binary64_bits;
    value.tolerance_evidence.policy_digest = entry.policy_digest;
    const auto expected_bound_digest = bound_digest(value.tolerance_evidence);
    if (!zero(value.tolerance_evidence.evidence_digest) &&
        value.tolerance_evidence.evidence_digest != expected_bound_digest)
      return suite_error("qualification_suite.preparation_bound_digest");
    value.tolerance_evidence.evidence_digest = expected_bound_digest;
    const auto supplied_observation_digest = value.observation_digest;
    value.observation_digest = {};
    const auto expected_observation_digest =
        preparation_observation_digest(value);
    if (!zero(supplied_observation_digest) &&
        supplied_observation_digest != expected_observation_digest)
      return suite_error("qualification_suite.preparation_observation_digest");
    value.observation_digest = expected_observation_digest;
    if (!preparation_observation_passes(entry, value))
      return suite_error("qualification_suite.preparation_observation_failed");
    return value;
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "qualification_suite.preparation_observation_allocation");
  }
}

product_status_or<qualification_result_mode_suite_observation>
make_qualification_result_mode_suite_observation(
    const qualification_result_mode_suite_case &descriptor,
    qualification_result_mode_suite_observation value) {
  try {
    auto canonical_descriptor = canonicalize_result_case(descriptor);
    if (!canonical_descriptor.has_value())
      return canonical_descriptor.error();
    const auto &entry = canonical_descriptor.value();
    if (value.schema != qualification_suite_schema_version ||
        (!value.case_identifier.empty() &&
         value.case_identifier != entry.identifier) ||
        (!zero(value.case_digest) && value.case_digest != entry.case_digest) ||
        (!zero(value.policy_digest) &&
         value.policy_digest != entry.policy_digest))
      return suite_error("qualification_suite.result_observation_binding");
    auto check_bound_binding = [&](const qualification_explicit_bound_evidence &bound,
                                   std::uint64_t declared) {
      return bound.schema == qualification_suite_schema_version &&
             (bound.unit == model_unit::unspecified ||
              bound.unit == entry.tolerance_unit) &&
             (bound.declared_limit_binary64_bits == 0 ||
              bound.declared_limit_binary64_bits == declared) &&
             (zero(bound.policy_digest) ||
              bound.policy_digest == entry.policy_digest);
    };
    if (!check_bound_binding(value.displacement_evidence,
                             entry.displacement_limit_binary64_bits) ||
        !check_bound_binding(value.support_plane_evidence,
                             entry.support_plane_limit_binary64_bits))
      return suite_error("qualification_suite.result_bound_binding");
    value.case_identifier = entry.identifier;
    value.case_digest = entry.case_digest;
    value.policy_digest = entry.policy_digest;
    for (auto *bound : {&value.displacement_evidence,
                        &value.support_plane_evidence}) {
      bound->schema = qualification_suite_schema_version;
      bound->unit = entry.tolerance_unit;
      bound->policy_digest = entry.policy_digest;
    }
    value.displacement_evidence.declared_limit_binary64_bits =
        entry.displacement_limit_binary64_bits;
    value.support_plane_evidence.declared_limit_binary64_bits =
        entry.support_plane_limit_binary64_bits;
    const auto displacement_digest = bound_digest(value.displacement_evidence);
    const auto support_plane_digest = bound_digest(value.support_plane_evidence);
    if ((!zero(value.displacement_evidence.evidence_digest) &&
         value.displacement_evidence.evidence_digest != displacement_digest) ||
        (!zero(value.support_plane_evidence.evidence_digest) &&
         value.support_plane_evidence.evidence_digest != support_plane_digest))
      return suite_error("qualification_suite.result_bound_digest");
    value.displacement_evidence.evidence_digest = displacement_digest;
    value.support_plane_evidence.evidence_digest = support_plane_digest;
    const auto supplied_observation_digest = value.observation_digest;
    value.observation_digest = {};
    const auto expected_observation_digest = result_observation_digest(value);
    if (!zero(supplied_observation_digest) &&
        supplied_observation_digest != expected_observation_digest)
      return suite_error("qualification_suite.result_observation_digest");
    value.observation_digest = expected_observation_digest;
    if (!result_observation_passes(entry, value))
      return suite_error("qualification_suite.result_observation_failed");
    return value;
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "qualification_suite.result_observation_allocation");
  }
}

product_status_or<qualification_attribute_suite_observation>
make_qualification_attribute_suite_observation(
    const qualification_attribute_suite_case &descriptor,
    qualification_attribute_suite_observation value) {
  try {
    auto canonical_descriptor = canonicalize_attribute_case(descriptor);
    if (!canonical_descriptor.has_value())
      return canonical_descriptor.error();
    const auto &entry = canonical_descriptor.value();
    if (value.schema != qualification_suite_schema_version ||
        (!value.case_identifier.empty() &&
         value.case_identifier != entry.identifier) ||
        (!zero(value.case_digest) && value.case_digest != entry.case_digest) ||
        (!zero(value.policy_digest) &&
         value.policy_digest != entry.policy_digest))
      return suite_error("qualification_suite.attribute_observation_binding");
    value.case_identifier = entry.identifier;
    value.case_digest = entry.case_digest;
    value.policy_digest = entry.policy_digest;
    const auto supplied_observation_digest = value.observation_digest;
    value.observation_digest = {};
    const auto expected_observation_digest = attribute_observation_digest(value);
    if (!zero(supplied_observation_digest) &&
        supplied_observation_digest != expected_observation_digest)
      return suite_error("qualification_suite.attribute_observation_digest");
    value.observation_digest = expected_observation_digest;
    if (!attribute_observation_passes(entry, value))
      return suite_error("qualification_suite.attribute_observation_failed");
    return value;
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "qualification_suite.attribute_observation_allocation");
  }
}

product_status_or<qualification_profile_suite_report>
make_qualification_profile_suite_report(
    const qualification_profile_suite_plan &plan,
    std::vector<qualification_preparation_suite_observation> preparation,
    std::vector<qualification_result_mode_suite_observation> result_modes,
    std::vector<qualification_attribute_suite_observation> attributes,
    bool complete) {
  try {
    auto plan_valid = validate_qualification_profile_suite_plan(plan);
    if (!plan_valid.has_value())
      return plan_valid.error();
    qualification_profile_suite_report report;
    report.identifier = plan.identifier + ".report";
    report.plan_digest = plan.plan_digest;

    auto preparation_map = std::map<std::string,
                                    qualification_preparation_suite_observation>{};
    for (auto &value : preparation) {
      if (!preparation_map.emplace(value.case_identifier, std::move(value)).second)
        return suite_error("qualification_suite.duplicate_preparation_observation");
    }
    for (const auto &descriptor : plan.preparation_cases) {
      const auto found = preparation_map.find(descriptor.identifier);
      if (found == preparation_map.end()) {
        ++report.blocking_issue_count;
        continue;
      }
      auto made = make_qualification_preparation_suite_observation(
          descriptor, std::move(found->second));
      if (!made.has_value()) {
        ++report.blocking_issue_count;
        continue;
      }
      ++report.passed_preparation_cases;
      report.preparation_observations.push_back(std::move(made.value()));
      preparation_map.erase(found);
    }
    report.blocking_issue_count += preparation_map.size();

    auto result_map = std::map<std::string,
                               qualification_result_mode_suite_observation>{};
    for (auto &value : result_modes) {
      if (!result_map.emplace(value.case_identifier, std::move(value)).second)
        return suite_error("qualification_suite.duplicate_result_observation");
    }
    for (const auto &descriptor : plan.result_mode_cases) {
      const auto found = result_map.find(descriptor.identifier);
      if (found == result_map.end()) {
        ++report.blocking_issue_count;
        continue;
      }
      auto made = make_qualification_result_mode_suite_observation(
          descriptor, std::move(found->second));
      if (!made.has_value()) {
        ++report.blocking_issue_count;
        continue;
      }
      ++report.passed_result_mode_cases;
      report.result_mode_observations.push_back(std::move(made.value()));
      result_map.erase(found);
    }
    report.blocking_issue_count += result_map.size();

    auto attribute_map = std::map<std::string,
                                  qualification_attribute_suite_observation>{};
    for (auto &value : attributes) {
      if (!attribute_map.emplace(value.case_identifier, std::move(value)).second)
        return suite_error("qualification_suite.duplicate_attribute_observation");
    }
    std::map<std::string, std::pair<digest, digest>> geometry_groups;
    for (const auto &descriptor : plan.attribute_cases) {
      const auto found = attribute_map.find(descriptor.identifier);
      if (found == attribute_map.end()) {
        ++report.blocking_issue_count;
        continue;
      }
      auto made = make_qualification_attribute_suite_observation(
          descriptor, std::move(found->second));
      if (!made.has_value()) {
        ++report.blocking_issue_count;
        continue;
      }
      const auto inserted = geometry_groups.emplace(
          descriptor.geometry_invariance_group,
          std::make_pair(made.value().exact_result_digest,
                         made.value().geometry_digest));
      if (!inserted.second &&
          inserted.first->second !=
              std::make_pair(made.value().exact_result_digest,
                             made.value().geometry_digest)) {
        ++report.blocking_issue_count;
        attribute_map.erase(found);
        continue;
      }
      ++report.passed_attribute_cases;
      report.attribute_observations.push_back(std::move(made.value()));
      attribute_map.erase(found);
    }
    report.blocking_issue_count += attribute_map.size();

    const auto by_case = [](const auto &a, const auto &b) {
      return a.case_identifier < b.case_identifier;
    };
    std::sort(report.preparation_observations.begin(),
              report.preparation_observations.end(), by_case);
    std::sort(report.result_mode_observations.begin(),
              report.result_mode_observations.end(), by_case);
    std::sort(report.attribute_observations.begin(),
              report.attribute_observations.end(), by_case);
    report.complete = complete && report.blocking_issue_count == 0 &&
                      report.passed_preparation_cases ==
                          plan.preparation_cases.size() &&
                      report.passed_result_mode_cases ==
                          plan.result_mode_cases.size() &&
                      report.passed_attribute_cases == plan.attribute_cases.size();

    canonical_encoder encoder;
    encoder.u16(report.schema);
    encoder.u32(report.checker_version);
    encoder.string(report.identifier);
    encode_digest(encoder, report.plan_digest);
    encoder.u64(report.preparation_observations.size());
    for (const auto &value : report.preparation_observations)
      encode_preparation_observation(encoder, value, true);
    encoder.u64(report.result_mode_observations.size());
    for (const auto &value : report.result_mode_observations)
      encode_result_observation(encoder, value, true);
    encoder.u64(report.attribute_observations.size());
    for (const auto &value : report.attribute_observations)
      encode_attribute_observation(encoder, value, true);
    encoder.u64(report.passed_preparation_cases);
    encoder.u64(report.passed_result_mode_cases);
    encoder.u64(report.passed_attribute_cases);
    encoder.u64(report.blocking_issue_count);
    encoder.boolean(report.complete);
    report.canonical_bytes = encoder.bytes();
    report.report_digest = domain_digest(report_tag, report.canonical_bytes);
    return report;
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "qualification_suite.report_allocation");
  }
}

product_status_or<bool> validate_qualification_profile_suite_report(
    const qualification_profile_suite_report &report,
    const qualification_profile_suite_plan &plan) noexcept {
  try {
    if (report.schema != qualification_suite_schema_version ||
        report.checker_version != qualification_suite_checker_version ||
        report.plan_digest != plan.plan_digest || !text(report.identifier))
      return suite_error("qualification_suite.report_malformed");
    auto rebuilt = make_qualification_profile_suite_report(
        plan, report.preparation_observations, report.result_mode_observations,
        report.attribute_observations, report.complete);
    if (!rebuilt.has_value() ||
        rebuilt.value().passed_preparation_cases !=
            report.passed_preparation_cases ||
        rebuilt.value().passed_result_mode_cases !=
            report.passed_result_mode_cases ||
        rebuilt.value().passed_attribute_cases != report.passed_attribute_cases ||
        rebuilt.value().blocking_issue_count != report.blocking_issue_count ||
        rebuilt.value().complete != report.complete ||
        rebuilt.value().canonical_bytes != report.canonical_bytes ||
        rebuilt.value().report_digest != report.report_digest)
      return suite_error("qualification_suite.report_validation_failed");
    return true;
  } catch (...) {
    return suite_error("qualification_suite.report_validation_exception");
  }
}

product_status_or<std::vector<std::uint8_t>>
encode_qualification_profile_suite_report(
    const qualification_profile_suite_report &report) {
  if (report.schema != qualification_suite_schema_version ||
      report.checker_version != qualification_suite_checker_version ||
      report.canonical_bytes.empty() || zero(report.report_digest) ||
      report.report_digest != domain_digest(report_tag, report.canonical_bytes))
    return suite_error("qualification_suite.report_unbound");
  return report.canonical_bytes;
}

bool qualification_profile_suite_gate_passes(
    const qualification_profile_suite_report &report,
    const qualification_profile_suite_plan &plan) noexcept {
  const auto valid = validate_qualification_profile_suite_report(report, plan);
  return valid.has_value() &&
         report.schema == qualification_suite_schema_version &&
         report.checker_version == qualification_suite_checker_version &&
         report.plan_digest == plan.plan_digest &&
         report.complete && report.blocking_issue_count == 0 &&
         report.passed_preparation_cases == plan.preparation_cases.size() &&
         report.passed_result_mode_cases == plan.result_mode_cases.size() &&
         report.passed_attribute_cases == plan.attribute_cases.size() &&
         report.preparation_observations.size() == plan.preparation_cases.size() &&
         report.result_mode_observations.size() == plan.result_mode_cases.size() &&
         report.attribute_observations.size() == plan.attribute_cases.size() &&
         !report.canonical_bytes.empty() && !zero(report.report_digest) &&
         report.report_digest == domain_digest(report_tag, report.canonical_bytes);
}

} // namespace mesh_boolean
} // namespace ygor
