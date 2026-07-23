#include "BroadPhaseFixtures.h"

#include <algorithm>
#include <cfenv>
#include <utility>

namespace broad_phase_tests {

mesh_type box(scalar x0, scalar y0, scalar z0, scalar x1, scalar y1, scalar z1) {
  mesh_type mesh;
  mesh.vertices = {{x0, y0, z0}, {x1, y0, z0}, {x1, y1, z0}, {x0, y1, z0},
                   {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}};
  mesh.faces = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4},
                {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
  return mesh;
}

mesh_type permuted_box(scalar x0, scalar y0, scalar z0,
                       scalar x1, scalar y1, scalar z1) {
  auto mesh = box(x0, y0, z0, x1, y1, z1);
  for (auto &face : mesh.faces)
    if (!face.empty())
      std::rotate(face.begin(), face.begin() + 1, face.end());
  std::rotate(mesh.faces.begin(), mesh.faces.begin() + 2, mesh.faces.end());
  return mesh;
}

std::string diagnostic(const bounded_boolean_error &error) {
  return std::string(error.summary) + " [subcode " +
         std::to_string(error.subcode) + ", checkpoint " +
         std::to_string(error.checkpoint) + "]";
}

bounded::broad_phase_capabilities capabilities(predecessor_fixture &fixture) {
  bounded::broad_phase_capabilities out;
  out.owner = fixture.context.owner;
  out.resources = fixture.resources.get();
  return out;
}

predecessor_fixture build_predecessors(
    const mesh_type &a, const mesh_type &b,
    bounded::source_triangulation_provider_kind provider,
    bool compare_reference) {
  bounded_boolean_options<scalar> options;
  options.tolerance = scalar(0.25);
  options.verification.level = verification_level::exhaustive_diagnostics_v1;
  auto pending = bounded::build_pending_invocation(
      a, b, boolean_operation::intersection, options);
  require(pending.has_value(), "broad-phase pending invocation");

  bounded::precision_bootstrap_capabilities bootstrap;
  require(bootstrap.strict_build && bounded::strict_floating_build_enabled() &&
              bounded::runtime_floating_profile_qualified<scalar>() &&
              std::fegetround() == FE_TONEAREST,
          "broad-phase strict floating profile");
  auto preflight = bounded::preflight_precision(*pending.value(), bootstrap);
  require(preflight.has_value(), "broad-phase precision preflight");
  auto context = bounded::finalize_context(
      std::move(*pending.value()),
      bounded::make_precision_bootstrap_record(*preflight.value()));
  require(context.has_value(), "broad-phase context finalization");
  bounded::precision_runtime_capabilities precision_caps;
  precision_caps.expected_owner = &context.value()->owner;
  auto precision = bounded::build_precision_context(
      *preflight.value(), *context.value(), precision_caps);
  require(precision.has_value(), "broad-phase precision context");

  auto resources =
      std::make_unique<bounded::resource_manager>(context.value()->options.resources);
  bounded::input_validation_capabilities validation_caps;
  validation_caps.owner = context.value()->owner;
  validation_caps.resources = resources.get();
  auto validated_a = bounded::validate_operand(
      bounded::operand_id::a, context.value()->sources->a, *context.value(),
      **precision.value(), validation_caps);
  require(validated_a.has_value(), "broad-phase validated operand A");
  auto validated_b = bounded::validate_operand(
      bounded::operand_id::b, context.value()->sources->b, *context.value(),
      **precision.value(), validation_caps);
  require(validated_b.has_value(), "broad-phase validated operand B");

  bounded::source_triangulation_capabilities triangulation_caps;
  triangulation_caps.owner = context.value()->owner;
  triangulation_caps.resources = resources.get();
  triangulation_caps.provider = provider;
  triangulation_caps.compare_with_reference = compare_reference;
  auto source_a = bounded::triangulate_source_operand(
      *validated_a.value(), *context.value(), **precision.value(),
      triangulation_caps);
  require(source_a.has_value(), "broad-phase triangulated operand A");
  auto source_b = bounded::triangulate_source_operand(
      *validated_b.value(), *context.value(), **precision.value(),
      triangulation_caps);
  require(source_b.has_value(), "broad-phase triangulated operand B");

  bounded::canonical_halfedge_capabilities halfedge_caps;
  halfedge_caps.owner = context.value()->owner;
  halfedge_caps.resources = resources.get();
  auto manifolds = bounded::build_canonical_source_manifolds(
      *validated_a.value(), *validated_b.value(), *source_a.value(),
      *source_b.value(), *context.value(), **precision.value(), halfedge_caps);
  require(manifolds.has_value(), "broad-phase canonical source manifolds");

  return {std::move(*context.value()), std::move(*precision.value()),
          std::move(*manifolds.value()), std::move(resources)};
}

built_fixture build(const mesh_type &a, const mesh_type &b,
                    bounded::source_triangulation_provider_kind provider,
                    bool compare_reference) {
  auto predecessor =
      build_predecessors(a, b, provider, compare_reference);
  auto caps = capabilities(predecessor);
  auto artifact = bounded::build_canonical_candidate_stream(
      predecessor.context, *predecessor.precision, predecessor.manifolds, caps);
  if (!artifact.has_value())
    throw std::runtime_error(diagnostic(*artifact.error()));
  return {std::move(predecessor), std::move(*artifact.value())};
}

} // namespace broad_phase_tests
