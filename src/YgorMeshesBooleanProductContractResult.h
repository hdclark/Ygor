#pragma once
#ifndef YGOR_MESHES_BOOLEAN_PRODUCT_CONTRACT_RESULT_H_
#define YGOR_MESHES_BOOLEAN_PRODUCT_CONTRACT_RESULT_H_

#include "YgorMeshesBooleanProductContractPolicies.h"

namespace ygor {
namespace mesh_boolean {

enum class exact_result_topology : std::uint8_t {
  empty,
  closed_embedded_two_manifold,
  stratified_non_manifold
};

struct backend_provenance {
  std::uint16_t schema = product_contract_schema_version;
  backend_identity producer;
  backend_selection_mode selection = backend_selection_mode::explicit_backend;
  bool fallback_used = false;
  std::vector<backend_id> attempted_backends;
  std::optional<product_error> primary_failure;
};

struct exact_result_storage {
  std::uint16_t schema = product_contract_schema_version;
  operation selected_operation = operation::regularized_union;
  exact_result_topology topology = exact_result_topology::empty;
  backend_provenance backend;
  std::vector<std::uint8_t> canonical_bytes;
  digest canonical_digest;
};

class exact_result_handle {
  std::shared_ptr<const exact_result_storage> storage_;
  explicit exact_result_handle(std::shared_ptr<const exact_result_storage> s)
      : storage_(std::move(s)) {}
  friend product_status_or<exact_result_handle>
  make_exact_result_handle(operation, exact_result_topology, backend_provenance,
                           std::vector<std::uint8_t>);

public:
  exact_result_handle() = default;
  bool valid() const noexcept { return static_cast<bool>(storage_); }
  const exact_result_storage &get() const {
    if (!storage_)
      throw std::logic_error("empty exact_result_handle");
    return *storage_;
  }
  const exact_result_storage *operator->() const noexcept { return storage_.get(); }
  std::size_t use_count() const noexcept { return storage_.use_count(); }
};

product_status_or<exact_result_handle>
make_exact_result_handle(operation, exact_result_topology, backend_provenance,
                         std::vector<std::uint8_t>);

struct preparation_provenance {
  std::uint16_t schema = product_contract_schema_version;
  preparation_mode mode = preparation_mode::strict_validation;
  digest input_digest;
  digest prepared_digest;
  digest policy_digest;
  digest report_digest;
  bool geometry_changed = false;
};

struct realization_certificate_reference {
  std::uint16_t schema = product_contract_schema_version;
  product_realization_semantics semantics =
      product_realization_semantics::not_requested;
  digest exact_result_digest;
  digest certificate_digest;
};

struct realization_attempt_record {
  std::uint16_t schema = product_contract_schema_version;
  result_representation requested = result_representation::exact_stratified;
  product_realization_semantics semantics =
      product_realization_semantics::not_requested;
  bool succeeded = false;
  std::optional<product_error> failure;
};

struct attribute_transfer_report_contract {
  std::uint16_t schema = product_contract_schema_version;
  std::uint64_t omissions = 0;
  std::uint64_t conflicts = 0;
  digest report_digest;
};

struct product_verification_summary {
  std::uint16_t schema = product_contract_schema_version;
  bool passed = false;
  std::uint16_t verifier_set_version = 0;
  digest verifier_set_digest;
  digest report_digest;
};

struct qualification_provenance {
  std::uint16_t schema = product_contract_schema_version;
  bool qualified = false;
  std::optional<qualification_manifest_reference> manifest;
  std::string workload_profile;
};

template <class T, class I> struct certified_mesh_payload {
  std::shared_ptr<const boolean_success<T, I>> success;
  product_realization_semantics semantics =
      product_realization_semantics::not_requested;
  digest exact_result_digest;
  realization_certificate_reference certificate;
};

template <class T, class I> struct boolean_product_result {
  product_schema_versions schemas;
  result_representation representation = result_representation::exact_stratified;
  operation selected_operation = operation::regularized_union;
  backend_provenance backend;
  preparation_provenance preparation;
  exact_result_handle exact_result;
  std::optional<certified_mesh_payload<T, I>> mesh;
  std::optional<realization_attempt_record> realization;
  attribute_transfer_report_contract attributes;
  product_verification_summary verification;
  qualification_provenance qualification;
};

bool product_digest_is_zero(const digest &) noexcept;
product_status_or<bool>
validate_backend_provenance(const backend_provenance &) noexcept;
bool same_backend_identity(const backend_identity &,
                           const backend_identity &) noexcept;

template <class T, class I>
product_status_or<bool>
validate_product_result(const boolean_product_result<T, I> &r) noexcept {
  auto fail = [](product_error_code code, const char *key) {
    return product_status_or<bool>(make_product_error(code, key));
  };
  if (r.schemas.options != product_contract_schema_version ||
      r.schemas.artifact != product_contract_schema_version ||
      r.schemas.error != product_contract_schema_version ||
      r.schemas.certificate != product_contract_schema_version ||
      r.schemas.replay != product_contract_schema_version)
    return fail(product_error_code::stale_binding, "product_result.schema");
  auto bp = validate_backend_provenance(r.backend);
  if (!bp.has_value())
    return bp.error();
  if (!r.exact_result.valid())
    return fail(product_error_code::stale_binding,
                "product_result.exact_result_missing");
  const auto &exact = r.exact_result.get();
  if (exact.selected_operation != r.selected_operation ||
      !same_backend_identity(exact.backend.producer, r.backend.producer) ||
      exact.canonical_bytes.empty() ||
      product_digest_is_zero(exact.canonical_digest))
    return fail(product_error_code::stale_binding,
                "product_result.exact_result_binding");
  if (r.preparation.schema != product_contract_schema_version ||
      product_digest_is_zero(r.preparation.input_digest) ||
      product_digest_is_zero(r.preparation.prepared_digest) ||
      product_digest_is_zero(r.preparation.policy_digest) ||
      product_digest_is_zero(r.preparation.report_digest) ||
      (r.preparation.mode == preparation_mode::strict_validation &&
       r.preparation.geometry_changed))
    return fail(product_error_code::stale_binding,
                "product_result.preparation_binding");
  if (r.attributes.schema != product_contract_schema_version ||
      product_digest_is_zero(r.attributes.report_digest))
    return fail(product_error_code::stale_binding,
                "product_result.attribute_report_binding");
  if (!r.verification.passed ||
      r.verification.schema != product_contract_schema_version ||
      product_digest_is_zero(r.verification.verifier_set_digest) ||
      product_digest_is_zero(r.verification.report_digest))
    return fail(product_error_code::verifier_disagreement,
                "product_result.verification");
  if (r.representation == result_representation::exact_stratified) {
    if (r.mesh)
      return fail(product_error_code::stale_binding,
                  "product_result.mesh_with_exact_representation");
  } else {
    if (!r.mesh || !r.mesh->success || !r.realization)
      return fail(product_error_code::stale_binding,
                  "product_result.mesh_missing");
    const auto expected_semantics =
        r.representation == result_representation::exact_in_T_mesh
            ? product_realization_semantics::exact_in_T
            : product_realization_semantics::certified_approximate_embedding_v1;
    if (r.mesh->semantics != expected_semantics ||
        r.mesh->certificate.schema != product_contract_schema_version ||
        r.mesh->certificate.semantics != expected_semantics ||
        r.mesh->exact_result_digest != exact.canonical_digest ||
        r.mesh->certificate.exact_result_digest != exact.canonical_digest ||
        product_digest_is_zero(r.mesh->certificate.certificate_digest) ||
        r.mesh->success->selected_operation != r.selected_operation)
      return fail(product_error_code::stale_binding,
                  "product_result.realization_binding");
  }
  if (r.realization) {
    if (r.realization->schema != product_contract_schema_version)
      return fail(product_error_code::stale_binding,
                  "product_result.realization_schema");
    if (r.realization->succeeded != static_cast<bool>(r.mesh))
      return fail(product_error_code::stale_binding,
                  "product_result.realization_outcome");
    if (r.realization->succeeded) {
      if (r.realization->requested != r.representation ||
          r.realization->semantics != r.mesh->semantics ||
          r.realization->failure)
        return fail(product_error_code::stale_binding,
                    "product_result.realization_success_binding");
    } else {
      if (!r.realization->failure || r.mesh ||
          r.representation != result_representation::exact_stratified ||
          r.realization->requested == result_representation::exact_stratified ||
          r.realization->semantics ==
              product_realization_semantics::not_requested)
        return fail(product_error_code::stale_binding,
                    "product_result.realization_failure_missing");
    }
  }
  if (r.qualification.qualified) {
    if (!r.qualification.manifest ||
        product_digest_is_zero(r.qualification.manifest->manifest_digest))
      return fail(product_error_code::qualification_policy_violation,
                  "product_result.qualification_binding");
  } else if (r.backend.producer.maturity == backend_maturity::qualified) {
    return fail(product_error_code::qualification_policy_violation,
                "product_result.qualified_backend_without_manifest");
  }
  return true;
}

struct product_replay_binding {
  std::uint16_t schema = product_contract_schema_version;
  digest options_digest;
  digest preparation_policy_digest;
  digest preparation_report_digest;
  backend_id producer = backend_id::experimental_exact_v1;
  backend_version adapter_version;
  std::string build_identifier;
  digest capability_digest;
  std::vector<backend_id> fallback_chain;
  digest exact_result_digest;
  digest realization_policy_digest;
  digest attribute_policy_digest;
  digest verifier_set_digest;
  digest qualification_manifest_digest;
};

product_status_or<bool>
validate_product_replay_binding(const product_replay_binding &,
                                const boolean_product_options &,
                                const backend_identity &,
                                const exact_result_handle *) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_product_replay_binding(const product_replay_binding &);
product_status_or<product_replay_binding>
decode_product_replay_binding(const std::vector<std::uint8_t> &,
                              const product_decode_limits & = {});

} // namespace mesh_boolean
} // namespace ygor

#endif
