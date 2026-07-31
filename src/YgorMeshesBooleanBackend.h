#pragma once
#ifndef YGOR_MESHES_BOOLEAN_BACKEND_H_
#define YGOR_MESHES_BOOLEAN_BACKEND_H_

#include "YgorMeshesBooleanExactResult.h"
#include "YgorMeshesBooleanPreparation.h"

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <utility>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t backend_adapter_schema = 1;
constexpr const char *axis_aligned_box_workload_profile =
    "axis_aligned_box_pair_v1";

enum class backend_adapter_role : std::uint8_t {
  producer,
  diagnostic_only
};

enum class backend_comparison_outcome : std::uint8_t {
  agree,
  disagree,
  unsupported
};

struct backend_request_limits {
  std::uint16_t schema = backend_adapter_schema;
  std::uint64_t max_diagnostic_cells = 4096;
  std::uint64_t max_diagnostic_records = 65536;
  std::uint64_t max_diagnostic_bytes = 8U * 1024U * 1024U;
};

struct backend_execution_state {
  std::uint16_t schema = backend_adapter_schema;
  backend_selection_mode selection = backend_selection_mode::explicit_backend;
  std::vector<backend_id> attempted_backends;
  std::optional<product_error> primary_failure;
};

template <class T, class I> struct backend_request {
  std::uint16_t schema = backend_adapter_schema;
  prepared_operand<T, I> operand_a;
  prepared_operand<T, I> operand_b;
  operation selected_operation = operation::regularized_union;
  boolean_options engine_options;
  boolean_product_options product_options;
  backend_request_limits limits;
  std::shared_ptr<const exact_kernel_services<T>> kernel;
  std::shared_ptr<const verifier_service> verifiers;
  std::shared_ptr<cancellation_source> cancellation;
  diagnostic_consumer diagnostics;
  deterministic_executor_factory executor_factory;
  digest request_digest;

  backend_request(prepared_operand<T, I> a, prepared_operand<T, I> b,
                  operation op, boolean_options engine,
                  boolean_product_options product, backend_request_limits l,
                  std::shared_ptr<const exact_kernel_services<T>> k,
                  std::shared_ptr<const verifier_service> v,
                  std::shared_ptr<cancellation_source> c,
                  diagnostic_consumer d, deterministic_executor_factory e,
                  digest request)
      : operand_a(std::move(a)), operand_b(std::move(b)),
        selected_operation(op), engine_options(std::move(engine)),
        product_options(std::move(product)), limits(l), kernel(std::move(k)),
        verifiers(std::move(v)), cancellation(std::move(c)),
        diagnostics(std::move(d)), executor_factory(std::move(e)),
        request_digest(request) {}
};

template <class T, class I>
using backend_request_handle = std::shared_ptr<const backend_request<T, I>>;

struct backend_diagnostic_payload {
  std::uint16_t schema = backend_adapter_schema;
  std::string workload_profile;
  digest request_digest;
  operation selected_operation = operation::regularized_union;
  std::array<std::vector<exact_scalar>, 3> cuts;
  std::vector<std::uint8_t> occupied_cells;
  std::uint64_t occupied_cell_count = 0;
  std::uint64_t connected_components = 0;
  std::uint64_t boundary_rectangles = 0;
  exact_scalar exact_volume;
  std::vector<std::uint8_t> canonical_bytes;
  digest semantic_digest;
};

template <class T, class I> struct backend_attempt {
  std::uint16_t schema = backend_adapter_schema;
  backend_identity backend;
  backend_adapter_role role = backend_adapter_role::producer;
  digest request_digest;
  std::optional<boolean_product_result_handle<T, I>> product;
  std::optional<backend_diagnostic_payload> diagnostic;
  std::vector<std::uint8_t> canonical_bytes;
  digest attempt_digest;
};

struct backend_attempt_summary {
  std::uint16_t schema = backend_adapter_schema;
  backend_id requested_backend = backend_id::experimental_exact_v1;
  bool identity_available = true;
  backend_identity backend;
  backend_adapter_role role = backend_adapter_role::producer;
  bool succeeded = false;
  std::optional<product_error> failure;
  digest payload_digest;
  digest attempt_digest;
};

struct backend_comparison_record {
  std::uint16_t schema = backend_adapter_schema;
  backend_identity producer;
  backend_identity comparator;
  backend_comparison_outcome outcome = backend_comparison_outcome::unsupported;
  digest request_digest;
  digest expected_digest;
  digest observed_digest;
  std::uint64_t mismatched_cells = 0;
  bool exact_volume_matches = false;
  bool component_count_matches = false;
  bool output_bounds_match = false;
  std::string message_key;
  digest report_digest;
};

product_status_or<digest>
backend_comparison_digest(const backend_comparison_record &) noexcept;

template <class T, class I> struct backend_execution_result {
  std::uint16_t schema = backend_adapter_schema;
  digest request_digest;
  boolean_product_result_handle<T, I> product;
  std::vector<backend_attempt_summary> attempts;
  std::vector<backend_comparison_record> comparisons;
  digest execution_digest;

  bool has_disagreement() const noexcept {
    for (const auto &comparison : comparisons)
      if (comparison.outcome == backend_comparison_outcome::disagree)
        return true;
    return false;
  }
};

template <class T, class I>
using backend_execution_result_handle =
    std::shared_ptr<const backend_execution_result<T, I>>;

template <class T, class I> class boolean_backend {
public:
  virtual ~boolean_backend() = default;
  virtual const backend_identity &identity() const noexcept = 0;
  virtual backend_adapter_role role() const noexcept = 0;

  virtual product_status_or<backend_attempt<T, I>>
  evaluate(const backend_request<T, I> &,
           const backend_execution_state &) const = 0;

  virtual product_status_or<bool>
  verify(const backend_request<T, I> &,
         const backend_attempt<T, I> &) const noexcept = 0;

  virtual product_status_or<backend_comparison_record>
  compare(const backend_request<T, I> &,
          const backend_attempt<T, I> &,
          const backend_attempt<T, I> &) const noexcept {
    return make_product_error(product_error_code::backend_capability_mismatch,
                              "backend.compare_not_supported");
  }
};

template <class T, class I> struct backend_registry_entry {
  backend_identity identity;
  backend_adapter_role role = backend_adapter_role::producer;
  std::shared_ptr<const boolean_backend<T, I>> adapter;
};

template <class T, class I> class backend_registry {
  mutable std::mutex mutex_;
  bool frozen_ = false;
  std::map<backend_id, backend_registry_entry<T, I>> entries_;
  digest registry_digest_;

public:
  product_status_or<bool>
  register_backend(std::shared_ptr<const boolean_backend<T, I>>);
  product_status_or<bool> freeze();
  bool frozen() const;
  digest registry_digest() const;
  product_status_or<backend_registry_entry<T, I>> lookup(backend_id) const;
  std::vector<backend_registry_entry<T, I>> entries() const;
};

template <class T, class I>
product_status_or<backend_request_handle<T, I>> make_backend_request(
    prepared_operand<T, I>, prepared_operand<T, I>, operation,
    boolean_options, boolean_product_options,
    std::shared_ptr<const exact_kernel_services<T>>,
    std::shared_ptr<const verifier_service>,
    std::shared_ptr<cancellation_source> = {}, diagnostic_consumer = {},
    deterministic_executor_factory = {}, const backend_request_limits & = {});

template <class T, class I>
product_status_or<bool>
validate_backend_request(const backend_request<T, I> &) noexcept;

template <class T, class I>
product_status_or<std::shared_ptr<const boolean_backend<T, I>>>
make_experimental_exact_backend();

template <class T, class I>
product_status_or<std::shared_ptr<const boolean_backend<T, I>>>
make_axis_aligned_box_reference_backend();

template <class T, class I>
product_status_or<std::shared_ptr<backend_registry<T, I>>>
make_default_backend_registry(bool include_diagnostic_reference = true);

template <class T, class I>
product_status_or<backend_execution_result_handle<T, I>>
evaluate_backend_request(const backend_registry<T, I> &,
                         const backend_request<T, I> &,
                         const qualification_manifest * = nullptr);

template <class T, class I>
product_status_or<bool> validate_backend_execution_result(
    const backend_execution_result<T, I> &,
    const backend_request<T, I> &) noexcept;

#define YGOR_BACKEND_EXTERN(T, I)                                              \
  extern template class backend_registry<T, I>;                               \
  extern template product_status_or<backend_request_handle<T, I>>             \
  make_backend_request(                                                       \
      prepared_operand<T, I>, prepared_operand<T, I>, operation,              \
      boolean_options, boolean_product_options,                               \
      std::shared_ptr<const exact_kernel_services<T>>,                         \
      std::shared_ptr<const verifier_service>,                                 \
      std::shared_ptr<cancellation_source>, diagnostic_consumer,               \
      deterministic_executor_factory, const backend_request_limits &);         \
  extern template product_status_or<bool> validate_backend_request(            \
      const backend_request<T, I> &) noexcept;                                 \
  extern template product_status_or<                                           \
      std::shared_ptr<const boolean_backend<T, I>>>                            \
  make_experimental_exact_backend<T, I>();                                     \
  extern template product_status_or<                                           \
      std::shared_ptr<const boolean_backend<T, I>>>                            \
  make_axis_aligned_box_reference_backend<T, I>();                             \
  extern template product_status_or<std::shared_ptr<backend_registry<T, I>>>   \
  make_default_backend_registry<T, I>(bool);                                   \
  extern template product_status_or<backend_execution_result_handle<T, I>>    \
  evaluate_backend_request(const backend_registry<T, I> &,                     \
                           const backend_request<T, I> &,                       \
                           const qualification_manifest *);                    \
  extern template product_status_or<bool> validate_backend_execution_result(  \
      const backend_execution_result<T, I> &,                                  \
      const backend_request<T, I> &) noexcept

YGOR_BACKEND_EXTERN(float, std::uint32_t);
YGOR_BACKEND_EXTERN(float, std::uint64_t);
YGOR_BACKEND_EXTERN(double, std::uint32_t);
YGOR_BACKEND_EXTERN(double, std::uint64_t);
#undef YGOR_BACKEND_EXTERN

} // namespace mesh_boolean
} // namespace ygor

#endif
