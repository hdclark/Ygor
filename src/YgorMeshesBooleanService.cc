#include "YgorMeshesBooleanService.h"

#include "YgorMeshesBooleanOutput.h"
#include "YgorMeshesBooleanExecutor.h"
#include "YgorMeshesExactKernel.h"

#include <array>
#include <exception>
#include <memory>
#include <type_traits>

namespace ygor {
namespace mesh_boolean {
namespace {

product_error service_error(product_error_code code, const char *key) {
  return make_product_error(code, key);
}

product_error promote_service_error(const boolean_error &failure,
                                    const char *key) {
  auto result = make_product_error(promote_error_code(failure.code), key,
                                   failure.subcode);
  result.detail = failure.detail.empty() ? failure.message_key : failure.detail;
  result.replay_binding_digest = failure.replay.setup_digest;
  return result;
}

template <class T, class I>
class pass_through_product_store final : public boolean_product_store<T, I> {
public:
  product_status_or<bool>
  publish(boolean_product_result_handle<T, I> product) override {
    if (!product)
      return service_error(product_error_code::internal_invariant_error,
                           "service.store_null_product");
    return true;
  }
};

template <class T> constexpr coordinate_tag service_coordinate_tag() {
  return std::is_same<T, float>::value ? coordinate_tag::binary32
                                      : coordinate_tag::binary64;
}

template <class I> constexpr index_tag service_index_tag() {
  return std::is_same<I, std::uint32_t>::value ? index_tag::uint32
                                               : index_tag::uint64;
}

bool same_normalization_contract(const normalization_policy_contract &contract,
                                 const normalization_policy &policy) {
  return contract.schema == product_contract_schema_version &&
         contract.mode == policy.mode && contract.unit == policy.unit &&
         contract.model_tolerance == policy.model_tolerance &&
         contract.enabled_operations == policy.enabled_operations &&
         contract.nonplanar_facets == policy.nonplanar_facets;
}

product_status_or<bool>
validate_service_options(const boolean_service_options &options) noexcept {
  try {
    if (options.schema != boolean_service_schema)
      return service_error(product_error_code::stale_binding,
                           "service.options_schema");
    auto engine = validate_options(options.engine);
    if (!engine.has_value())
      return promote_service_error(engine.error(), "service.engine_options");
    auto product = validate_product_options(options.product);
    if (!product.has_value())
      return product.error();
    if (!options.product.mandatory_verification)
      return service_error(product_error_code::qualification_policy_violation,
                           "service.mandatory_verification");
    if (options.product.preparation.mode == preparation_mode::strict_validation) {
      if (options.normalization)
        return service_error(product_error_code::input_contract_error,
                             "service.unused_normalization_policy");
    } else {
      if (!options.normalization)
        return service_error(product_error_code::normalization_required,
                             "service.normalization_policy_required");
      if (!same_normalization_contract(options.product.preparation.normalization,
                                       *options.normalization))
        return service_error(product_error_code::stale_binding,
                             "service.normalization_contract_binding");
    }
    if (options.product.preparation.mode == preparation_mode::diagnosis_only)
      return service_error(product_error_code::normalization_required,
                           "service.diagnosis_only_not_executable");
    if (options.product.qualification.manifest) {
      if (!options.qualification_manifest_ptr ||
          options.product.qualification.manifest->identifier !=
              options.qualification_manifest_ptr->identifier ||
          options.product.qualification.manifest->manifest_digest !=
              options.qualification_manifest_ptr->manifest_digest)
        return service_error(product_error_code::stale_binding,
                             "service.qualification_manifest_binding");
    }
    if (options.qualification_manifest_ptr) {
      auto manifest = validate_qualification_manifest(
          *options.qualification_manifest_ptr);
      if (!manifest.has_value())
        return manifest.error();
    }
    return true;
  } catch (...) {
    return service_error(product_error_code::internal_invariant_error,
                         "service.options_exception");
  }
}

template <class T, class I>
product_status_or<prepared_operand<T, I>> prepare_operand_for_service(
    const fv_surface_mesh<T, I> &mesh, const boolean_service_options &options,
    const boolean_service_dependencies<T, I> &dependencies,
    cancellation_source *cancellation) {
  if (options.product.preparation.mode == preparation_mode::strict_validation) {
    auto prepared = validate_operand_strict(
        mesh, options.strict_validation, options.engine, dependencies.kernel,
        dependencies.verifiers, cancellation);
    if (!prepared.has_value())
      return promote_service_error(prepared.error(),
                                   "service.strict_preparation");
    return std::move(prepared.value());
  }
  normalization_report report;
  auto prepared = normalize_operand(mesh, *options.normalization, report,
                                    cancellation);
  if (!prepared.has_value())
    return promote_service_error(prepared.error(),
                                 "service.normalization_failed");
  return std::move(prepared.value());
}

} // namespace

template <class T, class I>
product_status_or<boolean_service_dependencies<T, I>>
make_default_boolean_service_dependencies(
    bool include_diagnostic_reference_backend) {
  try {
    auto verifier = std::make_shared<verifier_registry>();
    const auto coordinate = service_coordinate_tag<T>();
    const auto index = service_index_tag<I>();
    const std::array<status_or<bool>, 10> registrations{{
        register_input_topology_verifier(*verifier, coordinate, index),
        register_broad_phase_verifier(*verifier, coordinate, index),
        register_intersection_events_verifier(*verifier, coordinate, index),
        register_symbolic_registry_verifier(*verifier, coordinate, index),
        register_local_refinement_verifier(*verifier, coordinate, index),
        register_global_arrangement_verifier(*verifier, coordinate, index),
        register_cell_classification_verifier(*verifier, coordinate, index),
        register_boolean_selection_verifier(*verifier, coordinate, index),
        register_geometry_realization_verifier(*verifier, coordinate, index),
        register_boolean_output_verifier(*verifier, coordinate, index),
    }};
    for (const auto &registration : registrations)
      if (!registration.has_value())
        return promote_service_error(registration.error(),
                                     "service.verifier_registration");
    auto frozen = verifier->freeze();
    if (!frozen.has_value())
      return promote_service_error(frozen.error(),
                                   "service.verifier_freeze");
    auto backends = make_default_backend_registry<T, I>(
        include_diagnostic_reference_backend);
    if (!backends.has_value())
      return backends.error();
    boolean_service_dependencies<T, I> result;
    result.kernel = std::make_shared<exact_kernel<T>>();
    result.verifiers = std::shared_ptr<const verifier_service>(verifier);
    result.backends = std::shared_ptr<const backend_registry<T, I>>(
        std::move(backends.value()));
    result.executor_factory = [](const execution_policy &policy) {
      return std::make_unique<deterministic_executor>(policy);
    };
    result.store = std::make_shared<pass_through_product_store<T, I>>();
    return result;
  } catch (const std::bad_alloc &) {
    return service_error(product_error_code::resource_limit,
                         "service.dependencies_allocation");
  } catch (...) {
    return service_error(product_error_code::internal_invariant_error,
                         "service.dependencies_exception");
  }
}

template <class T, class I>
product_status_or<boolean_product_result_handle<T, I>>
boolean_operation_expert(
    const fv_surface_mesh<T, I> &a, const fv_surface_mesh<T, I> &b,
    operation selected_operation, const boolean_service_options &options,
    const boolean_service_dependencies<T, I> &dependencies) {
  try {
    auto valid = validate_service_options(options);
    if (!valid.has_value())
      return valid.error();
    if (!dependencies.kernel || !dependencies.verifiers ||
        !dependencies.backends || !dependencies.backends->frozen() ||
        !dependencies.executor_factory || !dependencies.store)
      return service_error(product_error_code::backend_unavailable,
                           "service.dependencies");
    auto cancellation = options.cancellation;
    if (!cancellation)
      cancellation = std::make_shared<cancellation_source>();
    if (cancellation->token().cancelled())
      return service_error(product_error_code::resource_limit,
                           "service.cancelled_before_preparation");
    auto prepared_a = prepare_operand_for_service(
        a, options, dependencies, cancellation.get());
    if (!prepared_a.has_value())
      return prepared_a.error();
    auto prepared_b = prepare_operand_for_service(
        b, options, dependencies, cancellation.get());
    if (!prepared_b.has_value())
      return prepared_b.error();
    auto request = make_backend_request(
        std::move(prepared_a.value()), std::move(prepared_b.value()),
        selected_operation, options.engine, options.product,
        dependencies.kernel, dependencies.verifiers, cancellation,
        options.diagnostics, dependencies.executor_factory,
        options.backend_limits);
    if (!request.has_value())
      return request.error();
    auto executed = evaluate_backend_request(
        *dependencies.backends, *request.value(),
        options.qualification_manifest_ptr.get());
    if (!executed.has_value())
      return executed.error();
    auto verified = validate_backend_execution_result(*executed.value(),
                                                      *request.value());
    if (!verified.has_value())
      return verified.error();
    auto published = dependencies.store->publish(executed.value()->product);
    if (!published.has_value())
      return published.error();
    if (!published.value())
      return service_error(product_error_code::internal_invariant_error,
                           "service.store_declined_product");
    return executed.value()->product;
  } catch (const std::bad_alloc &) {
    return service_error(product_error_code::resource_limit,
                         "service.operation_allocation");
  } catch (const std::exception &exception) {
    auto failure = service_error(product_error_code::internal_invariant_error,
                                 "service.operation_exception");
    failure.detail = exception.what();
    return failure;
  } catch (...) {
    return service_error(product_error_code::internal_invariant_error,
                         "service.operation_exception");
  }
}

template <class T, class I>
product_status_or<boolean_product_result_handle<T, I>> boolean_operation(
    const fv_surface_mesh<T, I> &a, const fv_surface_mesh<T, I> &b,
    operation selected_operation, const boolean_service_options &options) {
  auto valid = validate_service_options(options);
  if (!valid.has_value())
    return valid.error();
  if (options.product.backend.mode == backend_selection_mode::qualified_default &&
      (!options.qualification_manifest_ptr ||
       !options.product.qualification.manifest))
    return service_error(product_error_code::backend_unqualified,
                         "service.qualified_default_manifest_required");
  auto dependencies = make_default_boolean_service_dependencies<T, I>(
      options.include_diagnostic_reference_backend);
  if (!dependencies.has_value())
    return dependencies.error();
  return boolean_operation_expert(a, b, selected_operation, options,
                                  dependencies.value());
}

#define YGOR_BOOLEAN_SERVICE_INSTANTIATE(T, I)                                 \
  template product_status_or<boolean_service_dependencies<T, I>>              \
  make_default_boolean_service_dependencies<T, I>(bool);                      \
  template product_status_or<boolean_product_result_handle<T, I>>             \
  boolean_operation(const fv_surface_mesh<T, I> &,                            \
                    const fv_surface_mesh<T, I> &, operation,                  \
                    const boolean_service_options &);                          \
  template product_status_or<boolean_product_result_handle<T, I>>             \
  boolean_operation_expert(                                                    \
      const fv_surface_mesh<T, I> &, const fv_surface_mesh<T, I> &, operation, \
      const boolean_service_options &,                                         \
      const boolean_service_dependencies<T, I> &)

YGOR_BOOLEAN_SERVICE_INSTANTIATE(float, std::uint32_t);
YGOR_BOOLEAN_SERVICE_INSTANTIATE(float, std::uint64_t);
YGOR_BOOLEAN_SERVICE_INSTANTIATE(double, std::uint32_t);
YGOR_BOOLEAN_SERVICE_INSTANTIATE(double, std::uint64_t);
#undef YGOR_BOOLEAN_SERVICE_INSTANTIATE

} // namespace mesh_boolean
} // namespace ygor
