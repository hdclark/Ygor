#include "MeshBooleanOutputFixtures.h"

#include <YgorMeshesBooleanService.h>
#include <YgorMeshesBooleanExecutor.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

using namespace ygor;
using namespace ygor::mesh_boolean;

namespace {

using coordinate_type = double;
using index_type = std::uint32_t;

void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}


class recording_product_store final
    : public boolean_product_store<coordinate_type, index_type> {
public:
  std::uint64_t publications = 0;
  product_status_or<bool>
  publish(boolean_product_result_handle<coordinate_type, index_type> product)
      override {
    require(product && product->exact_result.valid(),
            "recording store product binding");
    ++publications;
    return true;
  }
};

class throwing_backend final
    : public boolean_backend<coordinate_type, index_type> {
  std::shared_ptr<const boolean_backend<coordinate_type, index_type>> delegate_;

public:
  explicit throwing_backend(
      std::shared_ptr<const boolean_backend<coordinate_type, index_type>> delegate)
      : delegate_(std::move(delegate)) {}

  const backend_identity &identity() const noexcept override {
    return delegate_->identity();
  }
  backend_adapter_role role() const noexcept override {
    return backend_adapter_role::producer;
  }
  product_status_or<backend_attempt<coordinate_type, index_type>>
  evaluate(const backend_request<coordinate_type, index_type> &,
           const backend_execution_state &) const override {
    throw std::runtime_error("injected backend exception");
  }
  product_status_or<bool>
  verify(const backend_request<coordinate_type, index_type> &,
         const backend_attempt<coordinate_type, index_type> &) const noexcept
      override {
    return true;
  }
};

boolean_service_options explicit_exact_options(
    result_representation representation =
        result_representation::exact_stratified) {
  boolean_service_options options;
  options.product.backend.mode = backend_selection_mode::explicit_backend;
  options.product.backend.requested_backend =
      backend_id::experimental_exact_v1;
  options.product.backend.allow_experimental_backend = true;
  options.product.qualification.mode =
      qualification_policy_mode::allow_explicit_unqualified;
  options.product.result.representation = representation;
  options.product.attributes.mode =
      attribute_transfer_mode::preserve_supported_with_report;
  if (representation == result_representation::exact_in_T_mesh) {
    options.product.realization.semantics =
        product_realization_semantics::exact_in_T;
    options.product.realization.search.strategy =
        realization_search_strategy::nearest_only;
  }
  return options;
}

void conservative_default_rejects_unqualified_backend() {
  const boolean_service_options defaults;
  require(defaults.product.backend.mode ==
                  backend_selection_mode::qualified_default &&
              !defaults.product.backend.requested_backend &&
              defaults.product.backend.fallback_chain.empty() &&
              defaults.product.backend.fallback_on.empty() &&
              !defaults.product.backend.allow_experimental_backend &&
              defaults.product.preparation.mode ==
                  preparation_mode::strict_validation &&
              !defaults.normalization &&
              defaults.product.mandatory_verification &&
              defaults.product.result.representation ==
                  result_representation::exact_stratified &&
              defaults.product.result
                  .retain_exact_result_on_realization_failure,
          "conservative service defaults");
  const auto a = input_test::box<coordinate_type, index_type>(0.0, 1.0);
  const auto b = input_test::box<coordinate_type, index_type>(0.5, 1.5);
  const auto result = boolean_operation(a, b, operation::regularized_union);
  require(!result.has_value() &&
              result.error().code == product_error_code::backend_unqualified,
          "default service refuses unavailable qualified backend");
}

void explicit_opt_in_returns_complete_envelope() {
  const auto a = input_test::box<coordinate_type, index_type>(0.0, 2.0);
  const auto b = input_test::box<coordinate_type, index_type>(1.0, 3.0);
  auto options = explicit_exact_options();
  const auto result =
      boolean_operation(a, b, operation::regularized_union, options);
  require(result.has_value(), "explicit service exact result");
  const auto &product = *result.value();
  require(product.representation == result_representation::exact_stratified &&
              product.exact_result.valid() && !product.mesh &&
              product.backend.producer.id ==
                  backend_id::experimental_exact_v1 &&
              product.backend.selection ==
                  backend_selection_mode::explicit_backend &&
              product.preparation.mode == preparation_mode::strict_validation &&
              product.verification.passed && !product.qualification.qualified &&
              product.attributes.exact_result_digest ==
                  product.exact_result->canonical_digest &&
              validate_product_result(product).has_value(),
          "complete conservative result envelope");
}

void exact_mesh_is_deterministic_across_threads_and_replay() {
  const auto a = input_test::box<coordinate_type, index_type>(0.0, 2.0);
  const auto b = input_test::box<coordinate_type, index_type>(1.0, 3.0);
  auto one = explicit_exact_options(result_representation::exact_in_T_mesh);
  one.engine.execution.max_threads = 1;
  auto two = one;
  two.engine.execution.max_threads = 2;
  const auto r1 = boolean_operation(a, b, operation::regularized_union, one);
  const auto replay = boolean_operation(a, b, operation::regularized_union, one);
  const auto r2 = boolean_operation(a, b, operation::regularized_union, two);
  require(r1.has_value() && replay.has_value() && r2.has_value(),
          "service mesh executions");
  require(r1.value()->mesh && replay.value()->mesh && r2.value()->mesh &&
              r1.value()->realization && r1.value()->verification.passed &&
              r1.value()->attributes.output_digest ==
                  r1.value()->mesh->attribute_binding.output_digest &&
              !r1.value()->qualification.qualified &&
              r1.value()->backend.producer.id ==
                  backend_id::experimental_exact_v1 &&
              r1.value()->exact_result->canonical_bytes ==
                  replay.value()->exact_result->canonical_bytes &&
              r1.value()->mesh->output_canonical_bytes ==
                  replay.value()->mesh->output_canonical_bytes &&
              r1.value()->mesh->output_canonical_bytes ==
                  r2.value()->mesh->output_canonical_bytes &&
              r1.value()->mesh->attribute_binding.output_digest ==
                  r2.value()->mesh->attribute_binding.output_digest,
          "thread-count and replay output determinism");
}

void normalized_preparation_is_explicit_and_auditable() {
  auto a = input_test::box<coordinate_type, index_type>(0.0, 2.0);
  auto b = input_test::box<coordinate_type, index_type>(1.0, 3.0);
  a.vertices.push_back({99.0, 99.0, 99.0});
  b.vertices.push_back({98.0, 98.0, 98.0});

  auto options = explicit_exact_options();
  normalization_policy policy;
  policy.mode = normalization_mode::structural_only;
  policy.enabled_operations = normalization_operation_bit(
      normalization_operation::irrelevant_storage_removal);
  options.normalization = policy;
  options.product.preparation.mode = preparation_mode::normalized;
  options.product.preparation.normalization.mode = policy.mode;
  options.product.preparation.normalization.unit = policy.unit;
  options.product.preparation.normalization.model_tolerance =
      policy.model_tolerance;
  options.product.preparation.normalization.enabled_operations =
      policy.enabled_operations;
  options.product.preparation.normalization.nonplanar_facets =
      policy.nonplanar_facets;

  const auto result =
      boolean_operation(a, b, operation::regularized_union, options);
  require(result.has_value() &&
              result.value()->preparation.mode == preparation_mode::normalized &&
              result.value()->preparation.input_digest !=
                  result.value()->preparation.prepared_digest &&
              !product_digest_is_zero(
                  result.value()->preparation.report_digest) &&
              validate_product_result(*result.value()).has_value(),
          "normalized service provenance");

  auto mismatch = options;
  mismatch.product.preparation.normalization.enabled_operations =
      normalization_operation_bit(
          normalization_operation::exact_duplicate_consolidation);
  const auto rejected =
      boolean_operation(a, b, operation::regularized_union, mismatch);
  require(!rejected.has_value() &&
              rejected.error().code == product_error_code::stale_binding,
          "normalization policy binding enforced");
}

void realization_failure_retains_exact_authority() {
  const auto a = input_test::cube<coordinate_type, index_type>();
  const auto b =
      input_test::third_intersection_prism<coordinate_type, index_type>();
  auto options = explicit_exact_options(result_representation::exact_in_T_mesh);
  options.product.result.retain_exact_result_on_realization_failure = true;
  const auto result =
      boolean_operation(a, b, operation::regularized_intersection, options);
  require(result.has_value() &&
              result.value()->representation ==
                  result_representation::exact_stratified &&
              result.value()->exact_result.valid() && !result.value()->mesh &&
              result.value()->realization &&
              result.value()->realization->failure &&
              result.value()->realization->failure->code ==
                  product_error_code::output_not_representable,
          "service retains exact result after finite realization failure");
}

void cancellation_and_resource_limits_fail_without_partial_publication() {
  const auto a = input_test::box<coordinate_type, index_type>(0.0, 2.0);
  const auto b = input_test::box<coordinate_type, index_type>(1.0, 3.0);
  auto cancelled_options = explicit_exact_options();
  cancelled_options.cancellation = std::make_shared<cancellation_source>();
  cancelled_options.cancellation->cancel();
  const auto cancelled = boolean_operation(
      a, b, operation::regularized_union, cancelled_options);
  require(!cancelled.has_value() &&
              cancelled.error().code == product_error_code::resource_limit,
          "preparation cancellation is typed");

  auto limited = explicit_exact_options();
  limited.engine.resources.work_units.unlimited = false;
  limited.engine.resources.work_units.value = 1;
  const auto exhausted =
      boolean_operation(a, b, operation::regularized_union, limited);
  require(!exhausted.has_value() &&
              exhausted.error().code == product_error_code::resource_limit,
          "service resource limit is typed");

  const auto recovered = boolean_operation(
      a, b, operation::regularized_union, explicit_exact_options());
  require(recovered.has_value() && recovered.value()->exact_result.valid(),
          "failed invocation publishes no state into the next invocation");
}

void expert_dependencies_are_isolated_and_validated() {
  const auto a = input_test::box<coordinate_type, index_type>(0.0, 2.0);
  const auto b = input_test::box<coordinate_type, index_type>(1.0, 3.0);
  auto options = explicit_exact_options();
  auto dependencies =
      make_default_boolean_service_dependencies<coordinate_type, index_type>();
  require(dependencies.has_value(), "default expert dependencies");
  auto executor_creations = std::make_shared<std::uint64_t>(0);
  dependencies.value().executor_factory =
      [executor_creations](const execution_policy &policy) {
        ++*executor_creations;
        return std::make_unique<deterministic_executor>(policy);
      };
  auto store = std::make_shared<recording_product_store>();
  dependencies.value().store = store;
  const auto expert = boolean_operation_expert(
      a, b, operation::regularized_union, options, dependencies.value());
  require(expert.has_value() && *executor_creations == 1 &&
              store->publications == 1,
          "expert service execution and isolated injection");

  auto missing = dependencies.value();
  missing.verifiers.reset();
  const auto rejected = boolean_operation_expert(
      a, b, operation::regularized_union, options, missing);
  require(!rejected.has_value() &&
              rejected.error().code == product_error_code::backend_unavailable,
          "expert dependencies fail closed");

  auto delegate =
      make_experimental_exact_backend<coordinate_type, index_type>();
  require(delegate.has_value(), "throwing backend delegate");
  auto throwing_registry =
      std::make_shared<backend_registry<coordinate_type, index_type>>();
  require(throwing_registry
              ->register_backend(std::make_shared<throwing_backend>(
                  std::move(delegate.value())))
              .has_value() &&
              throwing_registry->freeze().has_value(),
          "throwing expert registry");
  auto throwing = dependencies.value();
  throwing.backends = throwing_registry;
  const auto exception = boolean_operation_expert(
      a, b, operation::regularized_union, options, throwing);
  require(!exception.has_value() &&
              exception.error().code ==
                  product_error_code::internal_invariant_error,
          "expert backend exception is contained and typed");
}

} // namespace

int main() {
  const std::pair<const char *, void (*)()> tests[] = {
      {"conservative_default", conservative_default_rejects_unqualified_backend},
      {"complete_envelope", explicit_opt_in_returns_complete_envelope},
      {"thread_replay", exact_mesh_is_deterministic_across_threads_and_replay},
      {"normalized", normalized_preparation_is_explicit_and_auditable},
      {"retain_exact", realization_failure_retains_exact_authority},
      {"cancel_resource",
       cancellation_and_resource_limits_fail_without_partial_publication},
      {"expert", expert_dependencies_are_isolated_and_validated},
  };
  try {
    for (const auto &test : tests) {
      test.second();
      std::cout << "PASS " << test.first << '\n';
    }
  } catch (const std::exception &exception) {
    std::cerr << "FAIL " << exception.what() << '\n';
    return 1;
  }
  return 0;
}
