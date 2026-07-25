#include "MeshBooleanOutputFixtures.h"

#include <YgorMeshesBooleanBackend.h>
#include <YgorMeshesBooleanPreparation.h>
#include <YgorMeshesExactKernel.h>

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

backend_capabilities producer_capabilities() {
  backend_capabilities capabilities;
  capabilities.set(backend_capability::exact_set_semantics);
  capabilities.set(backend_capability::exact_coordinates);
  capabilities.set(backend_capability::stratified_output);
  capabilities.set(backend_capability::manifold_mesh_output);
  capabilities.set(backend_capability::deterministic_canonical_output);
  capabilities.set(backend_capability::certified_failure_categories);
  capabilities.set(backend_capability::provenance_mapping);
  capabilities.set(backend_capability::strict_prepared_operands);
  capabilities.set(backend_capability::exact_in_T_output);
  return capabilities;
}

class drifting_backend final : public boolean_backend<coordinate_type, index_type> {
  backend_identity identity_;

public:
  drifting_backend() {
    auto made = make_backend_identity(
        backend_id::independent_axis_aligned_box_v1, {1, 0, 0},
        "drifting-backend", producer_capabilities(),
        backend_maturity::experimental);
    require(made.has_value(), "make drift identity");
    identity_ = made.value();
  }

  const backend_identity &identity() const noexcept override { return identity_; }
  backend_adapter_role role() const noexcept override {
    return backend_adapter_role::producer;
  }

  void drift_version() { ++identity_.adapter_version.patch; }
  void drift_capabilities() {
    identity_.capabilities.set(backend_capability::provenance_mapping, false);
    identity_.capability_digest =
        backend_capability_digest(identity_.capabilities);
  }

  product_status_or<backend_attempt<coordinate_type, index_type>>
  evaluate(const backend_request<coordinate_type, index_type> &,
           const backend_execution_state &) const override {
    return make_product_error(product_error_code::backend_unavailable,
                              "drifting_backend.should_not_run");
  }

  product_status_or<bool>
  verify(const backend_request<coordinate_type, index_type> &,
         const backend_attempt<coordinate_type, index_type> &) const noexcept
      override {
    return true;
  }
};

class corrupting_reference_backend final
    : public boolean_backend<coordinate_type, index_type> {
  std::shared_ptr<const boolean_backend<coordinate_type, index_type>> delegate_;

public:
  explicit corrupting_reference_backend(
      std::shared_ptr<const boolean_backend<coordinate_type, index_type>> delegate)
      : delegate_(std::move(delegate)) {}

  const backend_identity &identity() const noexcept override {
    return delegate_->identity();
  }
  backend_adapter_role role() const noexcept override {
    return backend_adapter_role::diagnostic_only;
  }

  product_status_or<backend_attempt<coordinate_type, index_type>>
  evaluate(const backend_request<coordinate_type, index_type> &request,
           const backend_execution_state &state) const override {
    auto attempt = delegate_->evaluate(request, state);
    if (!attempt.has_value())
      return attempt.error();
    require(attempt.value().diagnostic.has_value(),
            "reference diagnostic payload");
    require(!attempt.value().diagnostic->occupied_cells.empty(),
            "reference diagnostic cells");
    attempt.value().diagnostic->occupied_cells[0] ^= 1U;
    return attempt.value();
  }

  product_status_or<bool>
  verify(const backend_request<coordinate_type, index_type> &request,
         const backend_attempt<coordinate_type, index_type> &attempt) const
      noexcept override {
    return delegate_->verify(request, attempt);
  }

  product_status_or<backend_comparison_record>
  compare(const backend_request<coordinate_type, index_type> &request,
          const backend_attempt<coordinate_type, index_type> &producer,
          const backend_attempt<coordinate_type, index_type> &diagnostic) const
      noexcept override {
    return delegate_->compare(request, producer, diagnostic);
  }
};

class disagreeing_reference_backend final
    : public boolean_backend<coordinate_type, index_type> {
  std::shared_ptr<const boolean_backend<coordinate_type, index_type>> delegate_;

public:
  explicit disagreeing_reference_backend(
      std::shared_ptr<const boolean_backend<coordinate_type, index_type>> delegate)
      : delegate_(std::move(delegate)) {}

  const backend_identity &identity() const noexcept override {
    return delegate_->identity();
  }
  backend_adapter_role role() const noexcept override {
    return backend_adapter_role::diagnostic_only;
  }

  product_status_or<backend_attempt<coordinate_type, index_type>>
  evaluate(const backend_request<coordinate_type, index_type> &request,
           const backend_execution_state &state) const override {
    return delegate_->evaluate(request, state);
  }

  product_status_or<bool>
  verify(const backend_request<coordinate_type, index_type> &request,
         const backend_attempt<coordinate_type, index_type> &attempt) const
      noexcept override {
    return delegate_->verify(request, attempt);
  }

  product_status_or<backend_comparison_record>
  compare(const backend_request<coordinate_type, index_type> &request,
          const backend_attempt<coordinate_type, index_type> &producer,
          const backend_attempt<coordinate_type, index_type> &diagnostic) const
      noexcept override {
    auto comparison = delegate_->compare(request, producer, diagnostic);
    if (!comparison.has_value())
      return comparison.error();
    comparison.value().outcome = backend_comparison_outcome::disagree;
    comparison.value().mismatched_cells = 1;
    comparison.value().exact_volume_matches = false;
    comparison.value().message_key = "test.forced_disagreement";
    auto digest = backend_comparison_digest(comparison.value());
    if (!digest.has_value())
      return digest.error();
    comparison.value().report_digest = digest.value();
    return comparison.value();
  }
};

struct prepared_fixture {
  prepared_operand<coordinate_type, index_type> a;
  prepared_operand<coordinate_type, index_type> b;
  std::shared_ptr<const exact_kernel_services<coordinate_type>> kernel;
  std::shared_ptr<const verifier_service> verifiers;

  prepared_fixture(prepared_operand<coordinate_type, index_type> aa,
                   prepared_operand<coordinate_type, index_type> bb,
                   std::shared_ptr<const exact_kernel_services<coordinate_type>> k,
                   std::shared_ptr<const verifier_service> v)
      : a(std::move(aa)), b(std::move(bb)), kernel(std::move(k)),
        verifiers(std::move(v)) {}
};

prepared_fixture make_box_fixture() {
  auto a = input_test::box<coordinate_type, index_type>(0.0, 2.0);
  auto b = input_test::box<coordinate_type, index_type>(1.0, 3.0);
  std::shared_ptr<const exact_kernel_services<coordinate_type>> kernel =
      std::make_shared<exact_kernel<coordinate_type>>();
  std::shared_ptr<const verifier_service> verifiers = output_test::registry();
  strict_validation_policy policy;
  boolean_options engine;
  auto prepared_a =
      validate_operand_strict(a, policy, engine, kernel, verifiers);
  auto prepared_b =
      validate_operand_strict(b, policy, engine, kernel, verifiers);
  require(prepared_a.has_value() && prepared_b.has_value(),
          "strict box preparation");
  return prepared_fixture(std::move(prepared_a.value()),
                          std::move(prepared_b.value()), std::move(kernel),
                          std::move(verifiers));
}

prepared_fixture make_nonbox_fixture() {
  auto a = input_test::tetra<coordinate_type, index_type>();
  auto b = input_test::cube<coordinate_type, index_type>();
  std::shared_ptr<const exact_kernel_services<coordinate_type>> kernel =
      std::make_shared<exact_kernel<coordinate_type>>();
  std::shared_ptr<const verifier_service> verifiers = output_test::registry();
  strict_validation_policy policy;
  boolean_options engine;
  auto prepared_a =
      validate_operand_strict(a, policy, engine, kernel, verifiers);
  auto prepared_b =
      validate_operand_strict(b, policy, engine, kernel, verifiers);
  require(prepared_a.has_value() && prepared_b.has_value(),
          "strict nonbox preparation");
  return prepared_fixture(std::move(prepared_a.value()),
                          std::move(prepared_b.value()), std::move(kernel),
                          std::move(verifiers));
}

boolean_product_options explicit_exact_options() {
  boolean_product_options options;
  options.backend.mode = backend_selection_mode::explicit_backend;
  options.backend.requested_backend = backend_id::experimental_exact_v1;
  options.backend.allow_experimental_backend = true;
  options.qualification.mode =
      qualification_policy_mode::allow_explicit_unqualified;
  options.result.representation = result_representation::exact_in_T_mesh;
  options.realization.semantics = product_realization_semantics::exact_in_T;
  options.realization.search.strategy =
      realization_search_strategy::nearest_only;
  return options;
}

boolean_product_options diagnostic_options() {
  auto options = explicit_exact_options();
  options.backend.mode = backend_selection_mode::diagnostic_compare;
  options.backend.diagnostic_backends = {
      backend_id::independent_axis_aligned_box_v1};
  options.qualification.workload_profile = axis_aligned_box_workload_profile;
  return options;
}

backend_request_handle<coordinate_type, index_type>
make_request(const prepared_fixture &fixture,
             boolean_product_options options,
             operation op = operation::regularized_union,
             std::shared_ptr<cancellation_source> cancellation = {},
             const backend_request_limits &limits = {}) {
  auto made = make_backend_request(
      fixture.a, fixture.b, op, boolean_options{}, std::move(options),
      fixture.kernel, fixture.verifiers, std::move(cancellation), {}, limits);
  require(made.has_value(), "make backend request");
  return made.value();
}

void explicit_exact_backend() {
  const auto fixture = make_box_fixture();
  auto request = make_request(fixture, explicit_exact_options());
  auto registry = make_default_backend_registry<coordinate_type, index_type>(false);
  require(registry.has_value(), "exact registry");
  auto result = evaluate_backend_request(*registry.value(), *request);
  require(result.has_value(), "explicit exact backend execution");
  require(result.value()->product->backend.producer.id ==
              backend_id::experimental_exact_v1 &&
              result.value()->attempts.size() == 1 &&
              result.value()->comparisons.empty(),
          "explicit producer provenance");
  require(result.value()->product->representation ==
              result_representation::exact_in_T_mesh &&
              result.value()->product->mesh &&
              result.value()->product->mesh->success,
          "explicit exact mesh result");
}

void diagnostic_agreement_and_disagreement_preservation() {
  const auto fixture = make_box_fixture();
  auto request = make_request(fixture, diagnostic_options(),
                              operation::symmetric_difference);
  auto registry = make_default_backend_registry<coordinate_type, index_type>();
  require(registry.has_value(), "diagnostic registry");
  auto result = evaluate_backend_request(*registry.value(), *request);
  require(result.has_value(), "diagnostic comparison execution");
  require(result.value()->comparisons.size() == 1 &&
              result.value()->comparisons.front().outcome ==
                  backend_comparison_outcome::agree &&
              !result.value()->has_disagreement(),
          "independent box reference agreement");

  auto exact = make_experimental_exact_backend<coordinate_type, index_type>();
  auto reference =
      make_axis_aligned_box_reference_backend<coordinate_type, index_type>();
  require(exact.has_value() && reference.has_value(),
          "make disagreement adapters");
  auto disagreement_registry =
      std::make_shared<backend_registry<coordinate_type, index_type>>();
  require(disagreement_registry->register_backend(exact.value()).has_value(),
          "register disagreement producer");
  auto disagreeing =
      std::make_shared<disagreeing_reference_backend>(reference.value());
  require(disagreement_registry->register_backend(disagreeing).has_value(),
          "register disagreement comparator");
  require(disagreement_registry->freeze().has_value(),
          "freeze disagreement registry");
  auto disagreement =
      evaluate_backend_request(*disagreement_registry, *request);
  require(disagreement.has_value() && disagreement.value()->has_disagreement() &&
              disagreement.value()->product->backend.producer.id ==
                  backend_id::experimental_exact_v1,
          "diagnostic disagreement is evidence, not majority publication");
}

void fallback_and_prohibited_fallback() {
  const auto fixture = make_box_fixture();
  auto options = explicit_exact_options();
  options.backend.mode = backend_selection_mode::explicit_fallback_chain;
  options.backend.requested_backend.reset();
  options.backend.fallback_chain = {
      backend_id::independent_axis_aligned_box_v1,
      backend_id::experimental_exact_v1};
  options.backend.fallback_on = {product_error_code::backend_unavailable};
  auto request = make_request(fixture, options);
  auto exact_only =
      make_default_backend_registry<coordinate_type, index_type>(false);
  require(exact_only.has_value(), "fallback exact registry");
  auto result = evaluate_backend_request(*exact_only.value(), *request);
  require(result.has_value() && result.value()->attempts.size() == 2 &&
              result.value()->product->backend.fallback_used &&
              result.value()->product->backend.primary_failure &&
              result.value()->product->backend.primary_failure->code ==
                  product_error_code::backend_unavailable &&
              result.value()->product->backend.attempted_backends ==
                  options.backend.fallback_chain,
          "auditable missing-primary fallback");

  auto default_registry =
      make_default_backend_registry<coordinate_type, index_type>();
  require(default_registry.has_value(), "prohibited fallback registry");
  auto prohibited =
      evaluate_backend_request(*default_registry.value(), *request);
  require(!prohibited.has_value() &&
              prohibited.error().code ==
                  product_error_code::backend_capability_mismatch,
          "diagnostic adapter cannot become fallback producer");
}

void qualification_and_drift_fail_closed() {
  const auto fixture = make_box_fixture();
  boolean_product_options qualified;
  auto request = make_request(fixture, qualified);
  auto registry = make_default_backend_registry<coordinate_type, index_type>();
  require(registry.has_value(), "qualified-default registry");
  auto exact = make_experimental_exact_backend<coordinate_type, index_type>();
  auto reference =
      make_axis_aligned_box_reference_backend<coordinate_type, index_type>();
  require(exact.has_value() && reference.has_value(),
          "make deterministic registry adapters");
  auto reverse =
      std::make_shared<backend_registry<coordinate_type, index_type>>();
  require(reverse->register_backend(reference.value()).has_value() &&
              reverse->register_backend(exact.value()).has_value() &&
              reverse->freeze().has_value() &&
              reverse->registry_digest() == registry.value()->registry_digest() &&
              reverse->entries().front().identity.id ==
                  backend_id::experimental_exact_v1,
          "registry selection order is canonical");
  auto unavailable = evaluate_backend_request(*registry.value(), *request);
  require(!unavailable.has_value() &&
              unavailable.error().code == product_error_code::backend_unqualified,
          "qualified default rejects absent manifest profile");

  auto drift_options = explicit_exact_options();
  drift_options.backend.requested_backend =
      backend_id::independent_axis_aligned_box_v1;
  auto drift_request = make_request(fixture, drift_options);

  auto version_registry =
      std::make_shared<backend_registry<coordinate_type, index_type>>();
  auto version_backend = std::make_shared<drifting_backend>();
  require(version_registry->register_backend(version_backend).has_value() &&
              version_registry->freeze().has_value(),
          "freeze version drift registry");
  version_backend->drift_version();
  auto version = evaluate_backend_request(*version_registry, *drift_request);
  require(!version.has_value() &&
              version.error().code == product_error_code::backend_unavailable,
          "adapter version drift rejected");

  auto capability_registry =
      std::make_shared<backend_registry<coordinate_type, index_type>>();
  auto capability_backend = std::make_shared<drifting_backend>();
  require(capability_registry->register_backend(capability_backend).has_value() &&
              capability_registry->freeze().has_value(),
          "freeze capability drift registry");
  capability_backend->drift_capabilities();
  auto capability =
      evaluate_backend_request(*capability_registry, *drift_request);
  require(!capability.has_value() &&
              capability.error().code ==
                  product_error_code::backend_capability_mismatch,
          "adapter capability drift rejected");
}

void adapter_corruption_cancellation_and_semantic_mismatch() {
  const auto fixture = make_box_fixture();
  auto request = make_request(fixture, diagnostic_options());
  auto exact = make_experimental_exact_backend<coordinate_type, index_type>();
  auto reference =
      make_axis_aligned_box_reference_backend<coordinate_type, index_type>();
  require(exact.has_value() && reference.has_value(), "make corruption adapters");
  auto registry =
      std::make_shared<backend_registry<coordinate_type, index_type>>();
  require(registry->register_backend(exact.value()).has_value(),
          "register corruption producer");
  auto corrupting =
      std::make_shared<corrupting_reference_backend>(reference.value());
  require(registry->register_backend(corrupting).has_value() &&
              registry->freeze().has_value(),
          "register corrupting comparator");
  auto corrupted = evaluate_backend_request(*registry, *request);
  require(!corrupted.has_value() &&
              corrupted.error().code ==
                  product_error_code::verifier_disagreement,
          "adapter evidence corruption blocks publication");

  auto cancellation = std::make_shared<cancellation_source>();
  auto cancelled_request =
      make_request(fixture, diagnostic_options(), operation::regularized_union,
                   cancellation);
  cancellation->cancel();
  backend_execution_state state;
  state.selection = backend_selection_mode::diagnostic_compare;
  state.attempted_backends = {
      backend_id::experimental_exact_v1,
      backend_id::independent_axis_aligned_box_v1};
  auto cancelled = reference.value()->evaluate(*cancelled_request, state);
  require(!cancelled.has_value() &&
              cancelled.error().code == product_error_code::resource_limit,
          "diagnostic cancellation is typed and transactional");

  backend_request_limits tiny_limits;
  tiny_limits.max_diagnostic_cells = 1;
  auto limited_request = make_request(
      fixture, diagnostic_options(), operation::regularized_union, {},
      tiny_limits);
  auto limited = reference.value()->evaluate(*limited_request, state);
  require(!limited.has_value() &&
              limited.error().code == product_error_code::resource_limit,
          "diagnostic resource limit is typed and transactional");

  const auto nonbox = make_nonbox_fixture();
  auto nonbox_request = make_request(nonbox, diagnostic_options());
  auto mismatch = reference.value()->evaluate(*nonbox_request, state);
  require(!mismatch.has_value() &&
              mismatch.error().code ==
                  product_error_code::backend_capability_mismatch,
          "declared diagnostic workload rejects semantic conversion mismatch");
}

} // namespace

int main() {
  struct test_case {
    const char *name;
    void (*run)();
  };
  const test_case tests[] = {
      {"explicit", explicit_exact_backend},
      {"diagnostic", diagnostic_agreement_and_disagreement_preservation},
      {"fallback", fallback_and_prohibited_fallback},
      {"qualification_drift", qualification_and_drift_fail_closed},
      {"corruption_cancellation",
       adapter_corruption_cancellation_and_semantic_mismatch},
  };
  int failures = 0;
  for (const auto &test : tests) {
    try {
      test.run();
      std::cout << "PASS " << test.name << '\n';
    } catch (const std::exception &exception) {
      ++failures;
      std::cerr << "FAIL " << test.name << ": " << exception.what() << '\n';
    }
  }
  return failures == 0 ? 0 : 1;
}
