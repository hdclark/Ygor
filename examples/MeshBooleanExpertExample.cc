#include <YgorMeshesBooleanExecutor.h>
#include <YgorMeshesBooleanService.h>

#include <cstdint>
#include <iostream>
#include <memory>

namespace {

using namespace ygor::mesh_boolean;
using mesh_type = fv_surface_mesh<double, std::uint32_t>;

mesh_type make_box(double lo, double hi) {
  mesh_type mesh;
  mesh.vertices = {{lo, lo, lo}, {hi, lo, lo}, {hi, hi, lo}, {lo, hi, lo},
                   {lo, lo, hi}, {hi, lo, hi}, {hi, hi, hi}, {lo, hi, hi}};
  mesh.faces = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
  return mesh;
}

class counting_store final
    : public boolean_product_store<double, std::uint32_t> {
public:
  std::uint64_t publications = 0;

  product_status_or<bool>
  publish(boolean_product_result_handle<double, std::uint32_t> product)
      override {
    if (!product || !product->exact_result.valid())
      return make_product_error(product_error_code::stale_binding,
                                "expert_example.store_binding");
    ++publications;
    return true;
  }
};

boolean_service_options expert_options() {
  boolean_service_options options;
  options.product.backend.mode = backend_selection_mode::explicit_backend;
  options.product.backend.requested_backend =
      backend_id::experimental_exact_v1;
  options.product.backend.allow_experimental_backend = true;
  options.product.qualification.mode =
      qualification_policy_mode::allow_explicit_unqualified;
  options.product.result.representation =
      result_representation::exact_stratified;
  options.product.attributes.mode =
      attribute_transfer_mode::preserve_supported_with_report;
  return options;
}

} // namespace

int main() {
  std::cout << "expert/internal dependency-injection example\n";

  // This API is intentionally separate from the ordinary one-call service.
  // Applications normally call boolean_operation(...), which owns these
  // dependencies and does not expose internal verifier/backend registration.
  auto dependencies =
      make_default_boolean_service_dependencies<double, std::uint32_t>();
  if (!dependencies.has_value()) {
    std::cerr << dependencies.error().message_key << '\n';
    return 1;
  }

  auto executor_creations = std::make_shared<std::uint64_t>(0);
  dependencies.value().executor_factory =
      [executor_creations](const execution_policy &policy) {
        ++*executor_creations;
        return std::make_unique<deterministic_executor>(policy);
      };
  auto store = std::make_shared<counting_store>();
  dependencies.value().store = store;

  const auto result = boolean_operation_expert(
      make_box(0.0, 1.0), make_box(3.0, 4.0),
      operation::regularized_union, expert_options(), dependencies.value());
  if (!result.has_value()) {
    std::cerr << result.error().message_key << ": " << result.error().detail
              << '\n';
    return 1;
  }
  if (*executor_creations != 1 || store->publications != 1 ||
      !result.value()->verification.passed ||
      !result.value()->exact_result.valid()) {
    std::cerr << "expert dependency hooks were not used transactionally\n";
    return 1;
  }

  std::cout << "executor creations: " << *executor_creations
            << ", immutable publications: " << store->publications << '\n';
  return 0;
}
