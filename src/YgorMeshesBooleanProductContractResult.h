#pragma once
#ifndef YGOR_MESHES_BOOLEAN_PRODUCT_CONTRACT_RESULT_H_
#define YGOR_MESHES_BOOLEAN_PRODUCT_CONTRACT_RESULT_H_

#include "YgorMeshesBooleanProductContractPolicies.h"

namespace ygor {
namespace mesh_boolean {

class exact_result_handle;
template <class T, class I> struct certified_approximate_certificate;
template <class T, class I>
product_status_or<bool> verify_certified_approximate_embedding(
    const exact_result_handle &, const product_realization_policy &,
    const boolean_success<T, I> &,
    const certified_approximate_certificate<T, I> &) noexcept;

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
      make_exact_result_handle(operation, exact_result_topology,
                               backend_provenance, std::vector<std::uint8_t>);

public:
  exact_result_handle() = default;
  bool valid() const noexcept { return static_cast<bool>(storage_); }
  const exact_result_storage &get() const {
    if (!storage_)
      throw std::logic_error("empty exact_result_handle");
    return *storage_;
  }
  const exact_result_storage *operator->() const noexcept {
    return storage_.get();
  }
  std::size_t use_count() const noexcept { return storage_.use_count(); }
};

product_status_or<bool> validate_canonical_exact_result_bytes(
    operation, exact_result_topology, const backend_provenance &,
    const std::vector<std::uint8_t> &) noexcept;

product_status_or<exact_result_handle>
    make_exact_result_handle(operation, exact_result_topology,
                             backend_provenance, std::vector<std::uint8_t>);

struct preparation_provenance {
  std::uint16_t schema = product_contract_schema_version;
  preparation_mode mode = preparation_mode::strict_validation;
  digest input_digest;
  digest prepared_digest;
  digest policy_digest;
  digest report_digest;
  bool geometry_changed = false;
};

product_status_or<bool>
validate_durable_exact_result_bindings(const exact_result_handle &,
                                       const backend_provenance &,
                                       const preparation_provenance &) noexcept;
product_status_or<bool> validate_durable_mesh_selection_binding(
    const exact_result_handle &, const digest &) noexcept;

struct realization_certificate_reference {
  std::uint16_t schema = product_contract_schema_version;
  product_realization_semantics semantics =
      product_realization_semantics::not_requested;
  backend_identity backend;
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
  struct strict_vertex_binding {
    std::uint64_t output_vertex = 0;
    std::uint64_t selected_vertex = 0;
    std::array<std::uint64_t, 3> accepted_bits{{0, 0, 0}};
    digest exact_coordinate_digest;
  };

  std::shared_ptr<const boolean_success<T, I>> success;
  product_realization_semantics semantics =
      product_realization_semantics::not_requested;
  digest exact_result_digest;
  digest realization_semantic_digest;
  digest output_semantic_digest;
  std::vector<std::uint8_t> realization_canonical_bytes;
  std::vector<std::uint8_t> output_canonical_bytes;
  std::vector<strict_vertex_binding> strict_vertices;
  product_realization_policy policy;
  std::shared_ptr<const certified_approximate_certificate<T, I>>
      approximate_certificate;
  std::uint64_t obligation_count = 0;
  std::uint64_t defining_relation_obligation_count = 0;
  std::uint64_t constraint_component_count = 0;
  realization_certificate_reference certificate;
};

template <class T, class I>
digest strict_mesh_certificate_digest(const certified_mesh_payload<T, I> &m) {
  canonical_encoder e;
  e.raw(m.exact_result_digest.bytes.data(), m.exact_result_digest.bytes.size());
  e.raw(m.realization_semantic_digest.bytes.data(),
        m.realization_semantic_digest.bytes.size());
  e.raw(m.output_semantic_digest.bytes.data(),
        m.output_semantic_digest.bytes.size());
  e.byte_string(m.realization_canonical_bytes);
  e.byte_string(m.output_canonical_bytes);
  e.u64(m.strict_vertices.size());
  for (const auto &v : m.strict_vertices) {
    e.u64(v.output_vertex);
    e.u64(v.selected_vertex);
    for (auto bits : v.accepted_bits)
      e.u64(bits);
    e.raw(v.exact_coordinate_digest.bytes.data(),
          v.exact_coordinate_digest.bytes.size());
  }
  e.u64(m.obligation_count);
  e.u64(m.defining_relation_obligation_count);
  e.u64(m.constraint_component_count);
  return domain_digest({{'Y', 'G', 'B', 'E', 'X', 'M', '0', '2'}}, e.bytes());
}

template <class T, class I> struct boolean_product_result {
  product_schema_versions schemas;
  result_representation representation =
      result_representation::exact_stratified;
  operation_contract operation{operation::regularized_union};
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
  const auto selected_operation = r.operation.selected_operation();
  if (exact.selected_operation != selected_operation ||
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
  auto exact_bindings = validate_durable_exact_result_bindings(
      r.exact_result, r.backend, r.preparation);
  if (!exact_bindings.has_value())
    return exact_bindings.error();
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
        !same_backend_identity(r.mesh->certificate.backend,
                               r.backend.producer) ||
        r.mesh->exact_result_digest != exact.canonical_digest ||
        r.mesh->certificate.exact_result_digest != exact.canonical_digest ||
        product_digest_is_zero(r.mesh->certificate.certificate_digest) ||
        r.mesh->success->selected_operation != selected_operation)
      return fail(product_error_code::stale_binding,
                  "product_result.realization_binding");
    auto mesh_binding = validate_durable_mesh_selection_binding(
        r.exact_result, r.mesh->success->selected_boundary_digest);
    if (!mesh_binding.has_value())
      return mesh_binding.error();
    if (expected_semantics == product_realization_semantics::exact_in_T) {
      const auto &m = *r.mesh;
      if (m.realization_canonical_bytes.empty() ||
          m.output_canonical_bytes.empty() ||
          m.strict_vertices.size() != m.success->mesh.vertices.size() ||
          m.output_semantic_digest != m.success->canonical_output_digest ||
          m.output_semantic_digest != m.success->summary.semantic_digest ||
          m.realization_semantic_digest !=
              domain_digest({{'Y', 'G', 'B', 'C', 'A', 'N', '1', '1'}},
                            m.realization_canonical_bytes) ||
          m.output_semantic_digest !=
              domain_digest({{'Y', 'G', 'B', 'C', 'A', 'N', '1', '2'}},
                            m.output_canonical_bytes) ||
          m.certificate.certificate_digest !=
              strict_mesh_certificate_digest(m) ||
          m.obligation_count < m.strict_vertices.size() * 3 ||
          m.defining_relation_obligation_count > m.obligation_count ||
          (m.strict_vertices.empty() && m.obligation_count != 0))
        return fail(product_error_code::verifier_disagreement,
                    "product_result.strict_realization_evidence");
      for (std::size_t i = 0; i < m.strict_vertices.size(); ++i) {
        const auto &binding = m.strict_vertices[i];
        const auto &point = m.success->mesh.vertices[i];
        std::array<std::uint64_t, 3> bits{{0, 0, 0}};
        const std::array<T, 3> coordinates{{point.x, point.y, point.z}};
        for (std::size_t axis = 0; axis < 3; ++axis) {
          static_assert(sizeof(T) <= sizeof(std::uint64_t),
                        "unsupported coordinate width");
          std::memcpy(&bits[axis], &coordinates[axis], sizeof(T));
        }
        if (binding.output_vertex != i || binding.accepted_bits != bits ||
            product_digest_is_zero(binding.exact_coordinate_digest))
          return fail(product_error_code::verifier_disagreement,
                      "product_result.strict_vertex_binding");
      }
      if (m.approximate_certificate)
        return fail(product_error_code::verifier_disagreement,
                    "product_result.strict_has_approximate_certificate");
    } else {
      const auto &m = *r.mesh;
      if (!m.approximate_certificate || !m.strict_vertices.empty() ||
          m.realization_canonical_bytes.empty() ||
          m.output_canonical_bytes.empty() ||
          m.output_semantic_digest != m.success->canonical_output_digest ||
          m.output_semantic_digest != m.success->summary.semantic_digest ||
          m.policy.semantics != expected_semantics)
        return fail(product_error_code::verifier_disagreement,
                    "product_result.approximate_evidence");
      auto replay = verify_certified_approximate_embedding(
          r.exact_result, m.policy, *m.success, *m.approximate_certificate);
      if (!replay.has_value())
        return replay.error();
    }
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

template <class T, class I>
using boolean_product_result_handle =
    std::shared_ptr<const boolean_product_result<T, I>>;

template <class T, class I>
product_status_or<boolean_product_result_handle<T, I>>
freeze_boolean_product_result(boolean_product_result<T, I> result) {
  auto valid = validate_product_result(result);
  if (!valid.has_value())
    return valid.error();
  try {
    return std::make_shared<const boolean_product_result<T, I>>(
        std::move(result));
  } catch (const std::bad_alloc &) {
    return make_product_error(product_error_code::resource_limit,
                              "product_result.allocation");
  }
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

product_status_or<bool> validate_product_replay_binding(
    const product_replay_binding &, const boolean_product_options &,
    const backend_identity &, const exact_result_handle *) noexcept;
product_status_or<std::vector<std::uint8_t>>
encode_product_replay_binding(const product_replay_binding &);
product_status_or<product_replay_binding>
decode_product_replay_binding(const std::vector<std::uint8_t> &,
                              const product_decode_limits & = {});

} // namespace mesh_boolean
} // namespace ygor

#endif
