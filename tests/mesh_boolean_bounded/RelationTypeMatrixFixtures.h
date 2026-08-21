#pragma once

// Deterministic templated predecessors for the Component 07 type/index
// qualification matrix.  The public `fv_surface_mesh<T,I>` and the complete
// Component 01-06 predecessor chain are all header-only templates restricted
// to T in {float, double} and I in {uint32_t, uint64_t}.  This fixture builder
// mirrors the scalar-index `broad_phase_tests` fixture but keeps the full type
// pair explicit so each of the four supported profiles can be exercised
// independently and cross-compared for byte-identical semantics.

#include "YgorMeshesBooleanBounded/BroadPhaseBuild.h"
#include "YgorMeshesBooleanBounded/CanonicalHalfedgeBuild.h"
#include "YgorMeshesBooleanBounded/InputValidation.h"
#include "YgorMeshesBooleanBounded/PrecisionBootstrap.h"
#include "YgorMeshesBooleanBounded/RelationBuild.h"
#include "YgorMeshesBooleanBounded/SourceTriangulation.h"

#include <cfenv>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace relation_type_matrix {
namespace bounded = ygor::mesh_boolean::bounded;

inline void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

template <class T, class I>
fv_surface_mesh<T, I> typed_box(T x0, T y0, T z0, T x1, T y1, T z1) {
  fv_surface_mesh<T, I> mesh;
  mesh.vertices = {{x0, y0, z0}, {x1, y0, z0}, {x1, y1, z0}, {x0, y1, z0},
                   {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}};
  mesh.faces = {{I{0}, I{3}, I{2}, I{1}}, {I{4}, I{5}, I{6}, I{7}},
                {I{0}, I{1}, I{5}, I{4}}, {I{1}, I{2}, I{6}, I{5}},
                {I{2}, I{3}, I{7}, I{6}}, {I{3}, I{0}, I{4}, I{7}}};
  return mesh;
}

template <class T, class I>
struct typed_predecessor final {
  bounded::boolean_context<T, I> context;
  std::shared_ptr<const bounded::precision_context<T>> precision;
  std::shared_ptr<const bounded::canonical_source_manifolds<T, I>> manifolds;
  std::unique_ptr<bounded::resource_manager> resources;
};

template <class T, class I>
struct typed_fixture final {
  typed_predecessor<T, I> predecessor;
  std::shared_ptr<const bounded::canonical_candidate_stream<T, I>> artifact;
};

template <class T, class I>
typed_predecessor<T, I> build_typed_predecessors(
    const fv_surface_mesh<T, I> &a, const fv_surface_mesh<T, I> &b,
    bounded_execution_mode execution_mode, std::uint32_t requested_workers) {
  static_assert(std::is_same<T, float>::value || std::is_same<T, double>::value,
                "type matrix supports binary32/binary64 only");
  static_assert(std::is_same<I, std::uint32_t>::value ||
                    std::is_same<I, std::uint64_t>::value,
                "type matrix supports uint32/uint64 only");
  bounded::floating_environment_guard floating_environment;
  require(floating_environment.qualified(),
          "type-matrix floating environment qualification");

  bounded_boolean_options<T> options;
  options.tolerance = T(0.25);
  options.verification.level = verification_level::exhaustive_diagnostics_v1;
  options.execution.mode = execution_mode;
  options.execution.requested_workers = requested_workers;
  auto pending = bounded::build_pending_invocation(
      a, b, boolean_operation::intersection, options);
  require(pending.has_value(), "type-matrix pending invocation");

  bounded::precision_bootstrap_capabilities bootstrap;
  require(bootstrap.strict_build && bounded::strict_floating_build_enabled() &&
              bounded::runtime_floating_profile_qualified<T>() &&
              std::fegetround() == FE_TONEAREST,
          "type-matrix strict floating profile");
  auto preflight = bounded::preflight_precision(*pending.value(), bootstrap);
  require(preflight.has_value(), "type-matrix precision preflight");
  auto context = bounded::finalize_context(
      std::move(*pending.value()),
      bounded::make_precision_bootstrap_record(*preflight.value()));
  require(context.has_value(), "type-matrix context finalization");
  bounded::precision_runtime_capabilities precision_caps;
  precision_caps.expected_owner = &context.value()->owner;
  auto precision = bounded::build_precision_context(
      *preflight.value(), *context.value(), precision_caps);
  require(precision.has_value(), "type-matrix precision context");

  auto resources = std::make_unique<bounded::resource_manager>(
      context.value()->options.resources);
  bounded::input_validation_capabilities validation_caps;
  validation_caps.owner = context.value()->owner;
  validation_caps.resources = resources.get();
  auto validated_a = bounded::validate_operand(
      bounded::operand_id::a, context.value()->sources->a, *context.value(),
      **precision.value(), validation_caps);
  require(validated_a.has_value(), "type-matrix validated operand A");
  auto validated_b = bounded::validate_operand(
      bounded::operand_id::b, context.value()->sources->b, *context.value(),
      **precision.value(), validation_caps);
  require(validated_b.has_value(), "type-matrix validated operand B");

  bounded::source_triangulation_capabilities triangulation_caps;
  triangulation_caps.owner = context.value()->owner;
  triangulation_caps.resources = resources.get();
  triangulation_caps.provider =
      bounded::source_triangulation_provider_kind::indexed_dependency_v1;
  triangulation_caps.compare_with_reference = true;
  auto source_a = bounded::triangulate_source_operand(
      *validated_a.value(), *context.value(), **precision.value(),
      triangulation_caps);
  require(source_a.has_value(), "type-matrix triangulated operand A");
  auto source_b = bounded::triangulate_source_operand(
      *validated_b.value(), *context.value(), **precision.value(),
      triangulation_caps);
  require(source_b.has_value(), "type-matrix triangulated operand B");

  bounded::canonical_halfedge_capabilities halfedge_caps;
  halfedge_caps.owner = context.value()->owner;
  halfedge_caps.resources = resources.get();
  auto manifolds = bounded::build_canonical_source_manifolds(
      *validated_a.value(), *validated_b.value(), *source_a.value(),
      *source_b.value(), *context.value(), **precision.value(), halfedge_caps);
  require(manifolds.has_value(), "type-matrix canonical source manifolds");

  return {std::move(*context.value()), std::move(*precision.value()),
          std::move(*manifolds.value()), std::move(resources)};
}

template <class T, class I>
typed_fixture<T, I> build_typed(
    const fv_surface_mesh<T, I> &a, const fv_surface_mesh<T, I> &b,
    bounded_execution_mode execution_mode, std::uint32_t requested_workers) {
  auto predecessor = build_typed_predecessors<T, I>(
      a, b, execution_mode, requested_workers);
  bounded::broad_phase_capabilities caps;
  caps.owner = predecessor.context.owner;
  caps.resources = predecessor.resources.get();
  auto artifact = bounded::build_canonical_candidate_stream(
      predecessor.context, *predecessor.precision, predecessor.manifolds, caps);
  if (!artifact.has_value())
    throw std::runtime_error("type-matrix candidate stream failed");
  return {std::move(predecessor), std::move(*artifact.value())};
}

template <class T, class I>
struct typed_relation_attempt final {
  std::shared_ptr<const bounded::signed_feature_relations<T, I>> artifact;
  bounded_boolean_error error{};
  bool success = false;
};

template <class T, class I>
typed_relation_attempt<T, I> build_typed_relation(typed_fixture<T, I> &fixture) {
  bounded::resource_manager resources(resource_policy::conservative_defaults());
  bounded::relation_capabilities capabilities;
  capabilities.owner = fixture.predecessor.context.owner;
  capabilities.resources = &resources;
  auto outcome = bounded::build_signed_feature_relations(
      fixture.predecessor.context, *fixture.predecessor.precision,
      fixture.artifact, capabilities);
  typed_relation_attempt<T, I> result;
  result.success = outcome.has_value();
  if (result.success)
    result.artifact = *outcome.value();
  else
    result.error = *outcome.error();
  return result;
}

template <class T, class I>
std::string describe_attempt(const typed_relation_attempt<T, I> &attempt) {
  if (attempt.success)
    return "success";
  return std::string(attempt.error.summary) +
         " [subcode " + std::to_string(attempt.error.subcode) +
         ", checkpoint " + std::to_string(attempt.error.checkpoint) + "]";
}

} // namespace relation_type_matrix
