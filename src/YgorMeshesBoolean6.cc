#include <YgorMeshesBooleanOutput.h>
#include <YgorMeshesExactKernel.h>
#include <YgorMeshesBoolean6.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>

namespace {

using namespace ygor::mesh_boolean;

//using mesh_type = fv_surface_mesh<double, std::uint64_t>;
//using namespace ygor::mesh_boolean;
//

// The engine verifies every intermediate artifact. Register the verifiers once
// and reuse the resulting immutable service for as many operations as needed.
template <class T, class I>
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

// This is the complete application-facing workflow. The operands remain alive
// and unchanged while the context uses them; the context deliberately stores
// references rather than copying potentially large meshes.
template <class T, class I>
boolean_result<T, I>
run_boolean(const fv_surface_mesh<T,I> &a, const fv_surface_mesh<T,I> &b, operation requested,
            const std::shared_ptr<const exact_kernel_services<T>> &kernel,
            const std::shared_ptr<const verifier_service> &verifiers) {
  // The defaults select regularized solid semantics, deterministic execution,
  // mandatory verification, exact-in-double realization, and manifold output.
  boolean_options options;

  auto context =
      make_boolean_context(a, b, requested, options, kernel, verifiers);
  if (!context.has_value())
    return context.error();

  // Validate unknown-provenance inputs explicitly. Non-finite coordinates,
  // open/non-manifold meshes, bad orientation, self-intersections, and invalid
  // shell nesting are reported here as input_contract_error values.
  auto validated = validate_operands(*context.value());
  if (!validated.has_value())
    return validated.error();

  // This runs the remaining exact Boolean stages. Success guarantees that the
  // public mesh passed manifold, embedding, orientation, and realization checks.
  return assemble_boolean_output(*context.value());
}

std::string report_error(const boolean_error &error) {
  std::stringstream ss;
  ss << render_error(error) << '\n';
  if (error.code == boolean_error_code::result_topology_not_supported) {
    ss << "The exact selected boundary exists, but is not a closed "
                 "embedded two-manifold.\n";
  } else if (error.code == boolean_error_code::output_not_representable) {
    ss << "The exact result cannot be represented by double coordinates "
                 "without changing it.\n";
  }
  return ss.str();
}

} // namespace


template <class T, class I>
fv_surface_mesh<T, I>
BooleanMeshOp6(const fv_surface_mesh<T, I> &lhs,
               const fv_surface_mesh<T, I> &rhs,
               MeshBooleanOperation6 op){

    using namespace ygor::mesh_boolean;
    //using mesh_type = fv_surface_mesh<T, I>;

    auto verifiers = make_verifier_service<T,I>();
    if(!verifiers.has_value()) {
        throw std::runtime_error(report_error(verifiers.error()));
    }
    std::shared_ptr<const exact_kernel_services<T>> kernel =
        std::make_shared<exact_kernel<T>>();

    ygor::mesh_boolean::operation l_op;
    if(false){
    }else if(op == MeshBooleanOperation6::Union){
        l_op = ygor::mesh_boolean::operation::regularized_union;
    }else if(op == MeshBooleanOperation6::Intersection){
        l_op = ygor::mesh_boolean::operation::regularized_intersection;
    }else if(op == MeshBooleanOperation6::Subtraction){
        l_op = ygor::mesh_boolean::operation::a_minus_b;
    }else if(op == MeshBooleanOperation6::Exclusion){
        l_op = ygor::mesh_boolean::operation::symmetric_difference;
    }else{
        throw std::runtime_error("Unrecognized boolean operation");
    }

    auto result = run_boolean(lhs, rhs, l_op, kernel, verifiers.value());
    if(!result.has_value()) {
        throw std::runtime_error(report_error(result.error()));
    }

    return result.value()->mesh;
}


template <class T, class I>
fv_surface_mesh<T, I>
BooleanUnion6(const fv_surface_mesh<T, I> &lhs,
              const fv_surface_mesh<T, I> &rhs){
    return BooleanMeshOp6(lhs, rhs, MeshBooleanOperation6::Union);
}


template <class T, class I>
fv_surface_mesh<T, I>
BooleanIntersection6(const fv_surface_mesh<T, I> &lhs,
                     const fv_surface_mesh<T, I> &rhs){
    return BooleanMeshOp6(lhs, rhs, MeshBooleanOperation6::Intersection);
}


template <class T, class I>
fv_surface_mesh<T, I>
BooleanExclusion6(const fv_surface_mesh<T, I> &lhs,
                  const fv_surface_mesh<T, I> &rhs){
    return BooleanMeshOp6(lhs, rhs, MeshBooleanOperation6::Exclusion);
}


template <class T, class I>
fv_surface_mesh<T, I>
BooleanSubtraction6(const fv_surface_mesh<T, I> &lhs,
                    const fv_surface_mesh<T, I> &rhs){
    return BooleanMeshOp6(lhs, rhs, MeshBooleanOperation6::Subtraction);
}


// Explicit template instantiations.
#ifndef YGOR_MESHES_BOOLEAN6_DISABLE_ALL_SPECIALIZATIONS

template fv_surface_mesh<float,  uint32_t> BooleanMeshOp6       (const fv_surface_mesh<float,  uint32_t> &, const fv_surface_mesh<float,  uint32_t> &, MeshBooleanOperation6);
template fv_surface_mesh<float,  uint32_t> BooleanUnion6        (const fv_surface_mesh<float,  uint32_t> &, const fv_surface_mesh<float,  uint32_t> &);
template fv_surface_mesh<float,  uint32_t> BooleanIntersection6 (const fv_surface_mesh<float,  uint32_t> &, const fv_surface_mesh<float,  uint32_t> &);
template fv_surface_mesh<float,  uint32_t> BooleanExclusion6    (const fv_surface_mesh<float,  uint32_t> &, const fv_surface_mesh<float,  uint32_t> &);
template fv_surface_mesh<float,  uint32_t> BooleanSubtraction6  (const fv_surface_mesh<float,  uint32_t> &, const fv_surface_mesh<float,  uint32_t> &);

template fv_surface_mesh<float,  uint64_t> BooleanMeshOp6       (const fv_surface_mesh<float,  uint64_t> &, const fv_surface_mesh<float,  uint64_t> &, MeshBooleanOperation6);
template fv_surface_mesh<float,  uint64_t> BooleanUnion6        (const fv_surface_mesh<float,  uint64_t> &, const fv_surface_mesh<float,  uint64_t> &);
template fv_surface_mesh<float,  uint64_t> BooleanIntersection6 (const fv_surface_mesh<float,  uint64_t> &, const fv_surface_mesh<float,  uint64_t> &);
template fv_surface_mesh<float,  uint64_t> BooleanExclusion6    (const fv_surface_mesh<float,  uint64_t> &, const fv_surface_mesh<float,  uint64_t> &);
template fv_surface_mesh<float,  uint64_t> BooleanSubtraction6  (const fv_surface_mesh<float,  uint64_t> &, const fv_surface_mesh<float,  uint64_t> &);

template fv_surface_mesh<double, uint32_t> BooleanMeshOp6       (const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &, MeshBooleanOperation6);
template fv_surface_mesh<double, uint32_t> BooleanUnion6        (const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &);
template fv_surface_mesh<double, uint32_t> BooleanIntersection6 (const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &);
template fv_surface_mesh<double, uint32_t> BooleanExclusion6    (const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &);
template fv_surface_mesh<double, uint32_t> BooleanSubtraction6  (const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &);

template fv_surface_mesh<double, uint64_t> BooleanMeshOp6       (const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &, MeshBooleanOperation6);
template fv_surface_mesh<double, uint64_t> BooleanUnion6        (const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &);
template fv_surface_mesh<double, uint64_t> BooleanIntersection6 (const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &);
template fv_surface_mesh<double, uint64_t> BooleanExclusion6    (const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &);
template fv_surface_mesh<double, uint64_t> BooleanSubtraction6  (const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &);

#endif // YGOR_MESHES_BOOLEAN6_DISABLE_ALL_SPECIALIZATIONS
