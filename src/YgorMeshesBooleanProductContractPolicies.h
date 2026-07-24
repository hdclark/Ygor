#pragma once
#ifndef YGOR_MESHES_BOOLEAN_PRODUCT_CONTRACT_POLICIES_H_
#define YGOR_MESHES_BOOLEAN_PRODUCT_CONTRACT_POLICIES_H_

#include "YgorMeshesBooleanContract.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t product_contract_schema_version = 3;

struct product_schema_versions {
  std::uint16_t options = product_contract_schema_version;
  std::uint16_t artifact = product_contract_schema_version;
  std::uint16_t error = product_contract_schema_version;
  std::uint16_t certificate = product_contract_schema_version;
  std::uint16_t replay = product_contract_schema_version;
};

enum class backend_id : std::uint16_t { experimental_exact_v1 = 1 };
enum class backend_maturity : std::uint8_t {
  experimental,
  candidate,
  qualified,
  deprecated
};
enum class backend_selection_mode : std::uint8_t {
  explicit_backend,
  qualified_default,
  diagnostic_compare,
  explicit_fallback_chain
};
enum class backend_capability : std::uint8_t {
  exact_set_semantics,
  exact_coordinates,
  stratified_output,
  manifold_mesh_output,
  deterministic_canonical_output,
  certified_failure_categories,
  provenance_mapping,
  strict_prepared_operands,
  exact_in_T_output,
  certified_approximate_output,
  count
};

constexpr std::uint64_t backend_capability_bit(backend_capability c) noexcept {
  return std::uint64_t(1) << static_cast<unsigned>(c);
}

struct backend_capabilities {
  std::uint16_t schema = product_contract_schema_version;
  std::uint64_t bits = 0;
  bool has(backend_capability c) const noexcept {
    return (bits & backend_capability_bit(c)) != 0;
  }
  void set(backend_capability c, bool enabled = true) noexcept {
    if (enabled)
      bits |= backend_capability_bit(c);
    else
      bits &= ~backend_capability_bit(c);
  }
};

struct backend_version {
  std::uint16_t major = 0, minor = 0, patch = 0;
};

struct backend_identity {
  std::uint16_t schema = product_contract_schema_version;
  backend_id id = backend_id::experimental_exact_v1;
  backend_version adapter_version;
  std::string build_identifier;
  backend_capabilities capabilities;
  digest capability_digest;
  backend_maturity maturity = backend_maturity::experimental;
};

enum class preparation_mode : std::uint8_t {
  strict_validation,
  diagnosis_only,
  normalized
};
enum class normalization_mode : std::uint8_t {
  disabled,
  diagnosis_only,
  structural_only,
  geometry_changing
};
enum class model_unit : std::uint8_t {
  unspecified,
  unitless,
  millimetre,
  centimetre,
  metre,
  inch,
  foot
};
enum class normalization_operation : std::uint8_t {
  irrelevant_storage_removal,
  exact_duplicate_consolidation,
  orientation_repair,
  seam_aware_vertex_consolidation,
  crack_closure,
  nonplanar_facet_handling,
  overlapping_facet_resolution,
  sliver_handling,
  self_intersection_repair,
  count
};

enum class nonplanar_facet_policy : std::uint8_t {
  reject = 0,
  triangulate = 1,
  axis_aligned_refit = 2
};

constexpr std::uint64_t
normalization_operation_bit(normalization_operation o) noexcept {
  return std::uint64_t(1) << static_cast<unsigned>(o);
}

struct normalization_policy_contract {
  std::uint16_t schema = product_contract_schema_version;
  normalization_mode mode = normalization_mode::disabled;
  model_unit unit = model_unit::unspecified;
  double model_tolerance = 0.0;
  std::uint64_t enabled_operations = 0;
  nonplanar_facet_policy nonplanar_facets = nonplanar_facet_policy::reject;
};

struct preparation_policy_contract {
  std::uint16_t schema = product_contract_schema_version;
  preparation_mode mode = preparation_mode::strict_validation;
  normalization_policy_contract normalization;
};

enum class result_representation : std::uint8_t {
  exact_stratified,
  exact_in_T_mesh,
  certified_approximate_mesh
};
enum class product_realization_semantics : std::uint8_t {
  not_requested,
  exact_in_T,
  certified_approximate_embedding_v1
};
enum class realization_search_strategy : std::uint8_t {
  none,
  nearest_only,
  neighboring_values,
  deterministic_bounded_search
};

struct realization_search_policy {
  std::uint16_t schema = product_contract_schema_version;
  realization_search_strategy strategy = realization_search_strategy::none;
  std::uint64_t max_candidates = 0;
  std::uint64_t max_backtracks = 0;
};

struct approximation_policy_contract {
  std::uint16_t schema = product_contract_schema_version;
  bool enabled = false;
  model_unit unit = model_unit::unspecified;
  double max_vertex_displacement = 0.0;
  double max_axis_displacement_x = 0.0;
  double max_axis_displacement_y = 0.0;
  double max_axis_displacement_z = 0.0;
  double max_support_plane_deviation = 0.0;
  bool allow_original_vertex_movement = false;
  double declared_model_tolerance = 0.0;
  std::string application_acceptance_metadata;
};

struct product_realization_policy {
  std::uint16_t schema = product_contract_schema_version;
  product_realization_semantics semantics =
      product_realization_semantics::not_requested;
  realization_search_policy search;
  approximation_policy_contract approximation;
};

struct product_result_policy {
  std::uint16_t schema = product_contract_schema_version;
  result_representation representation = result_representation::exact_stratified;
  bool retain_exact_result_on_realization_failure = true;
};

enum class attribute_transfer_mode : std::uint8_t {
  omit_all_with_report,
  preserve_supported_with_report,
  require_lossless
};
enum class attribute_conflict_policy : std::uint8_t {
  report_and_omit,
  reject
};

struct attribute_transfer_policy_contract {
  std::uint16_t schema = product_contract_schema_version;
  attribute_transfer_mode mode =
      attribute_transfer_mode::omit_all_with_report;
  attribute_conflict_policy conflicts =
      attribute_conflict_policy::report_and_omit;
};

enum class qualification_policy_mode : std::uint8_t {
  require_qualified_profile,
  allow_explicit_unqualified
};

struct qualification_manifest_reference {
  std::uint16_t schema = product_contract_schema_version;
  std::string identifier;
  digest manifest_digest;
};

struct qualification_policy_contract {
  std::uint16_t schema = product_contract_schema_version;
  qualification_policy_mode mode =
      qualification_policy_mode::require_qualified_profile;
  std::string workload_profile;
  std::optional<qualification_manifest_reference> manifest;
};

enum class product_error_code : std::uint16_t {
  input_contract_error,
  unsupported_platform,
  resource_limit,
  index_overflow,
  result_topology_not_supported,
  output_not_representable,
  internal_invariant_error,
  normalization_required,
  normalization_failed,
  backend_unavailable,
  backend_capability_mismatch,
  backend_disagreement,
  backend_unqualified,
  exact_result_serialization_error,
  attribute_transfer_conflict,
  approximation_policy_rejected,
  qualification_policy_violation,
  stale_binding,
  replay_mismatch,
  verifier_disagreement
};

struct product_error {
  std::uint16_t schema = product_contract_schema_version;
  product_error_code code = product_error_code::internal_invariant_error;
  std::uint32_t subcode = 0;
  std::string message_key;
  std::string detail;
  std::optional<backend_identity> backend;
  digest replay_binding_digest;
};

product_error make_product_error(product_error_code, std::string,
                                 std::uint32_t = 0);
product_error_code promote_error_code(boolean_error_code) noexcept;
bool fallback_permitted_for(product_error_code) noexcept;

template <class V> class product_status_or {
  std::variant<V, product_error> data_;

public:
  product_status_or(V v) : data_(std::move(v)) {}
  product_status_or(product_error e) : data_(std::move(e)) {}
  bool has_value() const noexcept { return std::holds_alternative<V>(data_); }
  V *value_if() noexcept { return std::get_if<V>(&data_); }
  const V *value_if() const noexcept { return std::get_if<V>(&data_); }
  product_error *error_if() noexcept { return std::get_if<product_error>(&data_); }
  const product_error *error_if() const noexcept {
    return std::get_if<product_error>(&data_);
  }
  V &value() {
    if (auto *p = value_if())
      return *p;
    throw std::logic_error("product_status_or has error");
  }
  const V &value() const {
    if (const auto *p = value_if())
      return *p;
    throw std::logic_error("product_status_or has error");
  }
  product_error &error() {
    if (auto *p = error_if())
      return *p;
    throw std::logic_error("product_status_or has value");
  }
  const product_error &error() const {
    if (const auto *p = error_if())
      return *p;
    throw std::logic_error("product_status_or has value");
  }
};

struct backend_selection_policy {
  std::uint16_t schema = product_contract_schema_version;
  backend_selection_mode mode = backend_selection_mode::qualified_default;
  std::optional<backend_id> requested_backend;
  std::vector<backend_id> diagnostic_backends;
  std::vector<backend_id> fallback_chain;
  std::vector<product_error_code> fallback_on;
  bool allow_experimental_backend = false;
};

struct boolean_product_options {
  product_schema_versions schemas;
  backend_selection_policy backend;
  preparation_policy_contract preparation;
  product_result_policy result;
  product_realization_policy realization;
  attribute_transfer_policy_contract attributes;
  qualification_policy_contract qualification;
  bool mandatory_verification = true;
};

struct product_decode_limits {
  std::uint64_t max_record_bytes = 1024 * 1024;
  std::uint64_t max_string_bytes = 16 * 1024;
  std::uint64_t max_vector_elements = 1024;
};

product_status_or<bool>
validate_product_options(const boolean_product_options &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_product_options(const boolean_product_options &);
product_status_or<boolean_product_options>
decode_product_options(const std::vector<std::uint8_t> &,
                       const product_decode_limits & = {});
digest product_options_digest(const boolean_product_options &);

digest backend_capability_digest(const backend_capabilities &);
product_status_or<backend_identity>
make_backend_identity(backend_id, backend_version, std::string,
                      backend_capabilities, backend_maturity);
product_status_or<bool>
validate_backend_identity(const backend_identity &) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_backend_identity(const backend_identity &);
product_status_or<backend_identity>
decode_backend_identity(const std::vector<std::uint8_t> &,
                        const product_decode_limits & = {});

struct qualification_profile {
  backend_id backend = backend_id::experimental_exact_v1;
  digest capability_digest;
  result_representation representation = result_representation::exact_stratified;
  preparation_mode preparation = preparation_mode::strict_validation;
  std::string workload_profile;
};

struct qualification_manifest {
  std::uint16_t schema = product_contract_schema_version;
  std::string identifier;
  std::vector<qualification_profile> profiles;
  digest manifest_digest;
};

product_status_or<qualification_manifest>
make_qualification_manifest(std::string, std::vector<qualification_profile>);
product_status_or<bool>
validate_qualification_manifest(const qualification_manifest &) noexcept;
product_status_or<bool>
authorize_backend(const boolean_product_options &, const backend_identity &,
                  const qualification_manifest * = nullptr) noexcept;

} // namespace mesh_boolean
} // namespace ygor

#endif
