#pragma once
#ifndef YGOR_MESHES_BOOLEAN_SERVICE_H_
#define YGOR_MESHES_BOOLEAN_SERVICE_H_

#include "YgorMeshesBooleanBackend.h"
#include "YgorMeshesBooleanNormalization.h"

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t boolean_service_schema = 1;

struct boolean_service_options {
  std::uint16_t schema = boolean_service_schema;
  boolean_options engine;
  boolean_product_options product;
  strict_validation_policy strict_validation;
  std::optional<normalization_policy> normalization;
  backend_request_limits backend_limits;
  std::shared_ptr<const qualification_manifest> qualification_manifest_ptr;
  std::shared_ptr<cancellation_source> cancellation;
  diagnostic_consumer diagnostics;
  bool include_diagnostic_reference_backend = true;
};

template <class T, class I> class boolean_product_store {
public:
  virtual ~boolean_product_store() = default;
  virtual product_status_or<bool>
  publish(boolean_product_result_handle<T, I>) = 0;
};

template <class T, class I> struct boolean_service_dependencies {
  std::shared_ptr<const exact_kernel_services<T>> kernel;
  std::shared_ptr<const verifier_service> verifiers;
  std::shared_ptr<const backend_registry<T, I>> backends;
  deterministic_executor_factory executor_factory;
  std::shared_ptr<boolean_product_store<T, I>> store;
};

template <class T, class I>
product_status_or<boolean_service_dependencies<T, I>>
make_default_boolean_service_dependencies(
    bool include_diagnostic_reference_backend = true);

template <class T, class I>
product_status_or<boolean_product_result_handle<T, I>> boolean_operation(
    const fv_surface_mesh<T, I> &, const fv_surface_mesh<T, I> &, operation,
    const boolean_service_options & = {});

template <class T, class I>
product_status_or<boolean_product_result_handle<T, I>>
boolean_operation_expert(
    const fv_surface_mesh<T, I> &, const fv_surface_mesh<T, I> &, operation,
    const boolean_service_options &,
    const boolean_service_dependencies<T, I> &);

#define YGOR_BOOLEAN_SERVICE_EXTERN(T, I)                                      \
  extern template product_status_or<boolean_service_dependencies<T, I>>       \
  make_default_boolean_service_dependencies<T, I>(bool);                      \
  extern template product_status_or<boolean_product_result_handle<T, I>>      \
  boolean_operation(const fv_surface_mesh<T, I> &,                            \
                    const fv_surface_mesh<T, I> &, operation,                  \
                    const boolean_service_options &);                          \
  extern template product_status_or<boolean_product_result_handle<T, I>>      \
  boolean_operation_expert(                                                    \
      const fv_surface_mesh<T, I> &, const fv_surface_mesh<T, I> &, operation, \
      const boolean_service_options &,                                         \
      const boolean_service_dependencies<T, I> &)

YGOR_BOOLEAN_SERVICE_EXTERN(float, std::uint32_t);
YGOR_BOOLEAN_SERVICE_EXTERN(float, std::uint64_t);
YGOR_BOOLEAN_SERVICE_EXTERN(double, std::uint32_t);
YGOR_BOOLEAN_SERVICE_EXTERN(double, std::uint64_t);
#undef YGOR_BOOLEAN_SERVICE_EXTERN

} // namespace mesh_boolean
} // namespace ygor

#endif
