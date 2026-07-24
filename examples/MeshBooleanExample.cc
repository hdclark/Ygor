#include <YgorMeshesBooleanOutput.h>
#include <YgorMeshesBooleanPreparation.h>
#include <YgorMeshesExactKernel.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>

namespace {

using mesh_type = fv_surface_mesh<double, std::uint64_t>;
using namespace ygor::mesh_boolean;

// The engine verifies every intermediate artifact. Register the verifiers once
// and reuse the resulting immutable service for as many operations as needed.
status_or<std::shared_ptr<const verifier_service>> make_verifier_service() {
  auto registry = std::make_shared<verifier_registry>();
  constexpr auto coordinate = coordinate_tag::binary64;
  constexpr auto index = index_tag::uint64;

  const std::array<status_or<bool>, 10> registrations{{
      register_input_topology_verifier(*registry, coordinate, index),
      register_broad_phase_verifier(*registry, coordinate, index),
      register_intersection_events_verifier(*registry, coordinate, index),
      register_symbolic_registry_verifier(*registry, coordinate, index),
      register_local_refinement_verifier(*registry, coordinate, index),
      register_global_arrangement_verifier(*registry, coordinate, index),
      register_cell_classification_verifier(*registry, coordinate, index),
      register_boolean_selection_verifier(*registry, coordinate, index),
      register_geometry_realization_verifier(*registry, coordinate, index),
      register_boolean_output_verifier(*registry, coordinate, index),
  }};
  for (const auto &registration : registrations) {
    if (!registration.has_value())
      return registration.error();
  }

  auto frozen = registry->freeze();
  if (!frozen.has_value())
    return frozen.error();
  return std::shared_ptr<const verifier_service>(std::move(registry));
}

// This is the current expert workflow after operands have passed an explicit
// preparation decision. The future one-call product service is tracked by P5.
boolean_result<double, std::uint64_t>
run_boolean(const prepared_operand<double, std::uint64_t> &a,
            const prepared_operand<double, std::uint64_t> &b,
            operation requested,
            const std::shared_ptr<const exact_kernel_services<double>> &kernel,
            const std::shared_ptr<const verifier_service> &verifiers) {
  // This expert API defaults to strict exact-in-double realization and manifold
  // output. That mode is useful for dyadic/exact workflows, but it is not the
  // ordinary CAD-output target; P5 will replace this product-facing example.
  boolean_options options;

  auto context =
      make_boolean_context(a, b, requested, options, kernel, verifiers);
  if (!context.has_value())
    return context.error();

  // This runs the remaining exact Boolean stages. Success guarantees that the
  // public mesh passed manifold, embedding, orientation, and realization checks.
  return assemble_boolean_output(*context.value());
}

void report_error(const boolean_error &error) {
  std::cerr << render_error(error) << '\n';
  if (error.code == boolean_error_code::result_topology_not_supported) {
    std::cerr << "The exact selected boundary exists, but is not a closed "
                 "embedded two-manifold.\n";
  } else if (error.code == boolean_error_code::output_not_representable) {
    std::cerr << "The exact result cannot be represented by double coordinates "
                 "without changing it.\n";
  }
}

// Only the executable test needs sample inputs. Applications already have the
// two meshes described in run_boolean(); outward face winding is significant.
mesh_type make_box(double lo, double hi) {
  mesh_type mesh;
  mesh.vertices = {{lo, lo, lo}, {hi, lo, lo}, {hi, hi, lo}, {lo, hi, lo},
                   {lo, lo, hi}, {hi, lo, hi}, {hi, hi, hi}, {lo, hi, hi}};
  mesh.faces = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
  return mesh;
}

} // namespace

int main() {
  auto verifiers = make_verifier_service();
  if (!verifiers.has_value()) {
    report_error(verifiers.error());
    return 1;
  }
  std::shared_ptr<const exact_kernel_services<double>> kernel =
      std::make_shared<exact_kernel<double>>();

  // These generated fixtures have known provenance and are intended to satisfy
  // the strict B-rep contract. Imported STL/OBJ/scan/CAD tessellations require
  // an explicit diagnosis or repair policy and application review; they must
  // not be substituted here as though strict validation were automatic healing.
  const mesh_type a = make_box(0.0, 1.0);
  const mesh_type b = make_box(3.0, 4.0);
  const strict_validation_policy strict_policy;
  const boolean_options options;
  auto prepared_a = validate_operand_strict(a, strict_policy, options, kernel,
                                            verifiers.value());
  auto prepared_b = validate_operand_strict(b, strict_policy, options, kernel,
                                            verifiers.value());
  if (!prepared_a.has_value() || !prepared_b.has_value()) {
    report_error(!prepared_a.has_value() ? prepared_a.error()
                                         : prepared_b.error());
    return 1;
  }
  struct example_case {
    const char *name;
    operation requested;
    std::size_t expected_vertices;
    std::size_t expected_faces;
  };
  const std::array<example_case, 4> cases{{
      {"union", operation::regularized_union, 16, 24},
      {"intersection", operation::regularized_intersection, 0, 0},
      {"subtraction A-B", operation::a_minus_b, 8, 12},
      {"exclusion (symmetric difference)", operation::symmetric_difference, 16,
       24},
  }};

  for (const auto &entry : cases) {
    auto result = run_boolean(prepared_a.value(), prepared_b.value(),
                              entry.requested, kernel, verifiers.value());
    if (!result.has_value()) {
      std::cerr << entry.name << " failed: ";
      report_error(result.error());
      return 1;
    }

    // Copying success.mesh extracts an ordinary manifold mesh independent of
    // the result wrapper. An empty intersection is a successful empty mesh.
    mesh_type manifold_mesh = result.value()->mesh;
    if (manifold_mesh.vertices.size() != entry.expected_vertices ||
        manifold_mesh.faces.size() != entry.expected_faces) {
      std::cerr << entry.name << " produced unexpected output dimensions\n";
      return 1;
    }
    std::cout << entry.name << ": " << manifold_mesh.vertices.size()
              << " vertices, " << manifold_mesh.faces.size() << " faces\n";
  }

  // The explicit validation call above must reject malformed input instead of
  // repairing it. This also keeps the example's error path covered by CTest.
  mesh_type open_mesh = a;
  open_mesh.faces.pop_back();
  auto rejected = validate_operand_strict(open_mesh, strict_policy, options,
                                          kernel, verifiers.value());
  if (rejected.has_value() ||
      rejected.error().code != boolean_error_code::input_contract_error ||
      rejected.error().stage != boolean_stage::input_validation) {
    std::cerr << "invalid input did not produce the expected typed error\n";
    return 1;
  }

  return 0;
}
