#pragma once

#include "YgorMeshesBooleanBounded/InputValidation.h"
#include "YgorMeshesBooleanBounded/PrecisionBootstrap.h"
#include "YgorMeshesBooleanBounded/SourceTriangulation.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace source_triangulation_tests {
namespace bounded = ygor::mesh_boolean::bounded;

inline void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

template <class T, class I> fv_surface_mesh<T, I> box() {
  fv_surface_mesh<T, I> mesh;
  mesh.vertices = {{T(0), T(0), T(0)}, {T(1), T(0), T(0)},
                   {T(1), T(1), T(0)}, {T(0), T(1), T(0)},
                   {T(0), T(0), T(1)}, {T(1), T(0), T(1)},
                   {T(1), T(1), T(1)}, {T(0), T(1), T(1)}};
  mesh.faces = {{I(0), I(3), I(2), I(1)},
                {I(4), I(5), I(6), I(7)},
                {I(0), I(1), I(5), I(4)},
                {I(1), I(2), I(6), I(5)},
                {I(2), I(3), I(7), I(6)},
                {I(3), I(0), I(4), I(7)}};
  return mesh;
}

template <class T, class I> fv_surface_mesh<T, I> concave_prism() {
  const std::vector<std::array<T, 2>> polygon{{T(0), T(0)}, {T(2), T(0)},
                                              {T(2), T(1)}, {T(1), T(1)},
                                              {T(1), T(2)}, {T(0), T(2)}};
  fv_surface_mesh<T, I> mesh;
  for (const auto &point : polygon)
    mesh.vertices.push_back({point[0], point[1], T(0)});
  for (const auto &point : polygon)
    mesh.vertices.push_back({point[0], point[1], T(1)});
  const I n = static_cast<I>(polygon.size());
  std::vector<I> bottom, top;
  for (I i = 0; i < n; ++i) {
    bottom.push_back(static_cast<I>(n - 1 - i));
    top.push_back(static_cast<I>(n + i));
  }
  mesh.faces.push_back(bottom);
  mesh.faces.push_back(top);
  for (I i = 0; i < n; ++i) {
    const I j = static_cast<I>((i + 1) % n);
    mesh.faces.push_back({i, j, static_cast<I>(n + j), static_cast<I>(n + i)});
  }
  return mesh;
}

template <class T, class I> struct fixture final {
  bounded::boolean_context<T, I> context;
  std::shared_ptr<const bounded::precision_context<T>> precision;
  std::shared_ptr<const bounded::validated_operand<T, I>> operand;
  std::unique_ptr<bounded::resource_manager> resources;
};

template <class T, class I>
fixture<T, I> make_fixture(const fv_surface_mesh<T, I> &mesh,
                           T input_precision = T(0)) {
  fv_surface_mesh<T, I> empty;
  bounded_boolean_options<T> options;
  options.tolerance = T(0.25);
  options.input_precision_a = input_precision;
  auto pending = bounded::build_pending_invocation(
      mesh, empty, boolean_operation::set_union, options);
  require(pending.has_value(), "pending source triangulation fixture");
  bounded::precision_bootstrap_capabilities bootstrap_capabilities;
  if (!bootstrap_capabilities.strict_build ||
      !bounded::strict_floating_build_enabled() ||
      !bounded::runtime_floating_profile_qualified<T>() ||
      std::fegetround() != FE_TONEAREST)
    throw std::runtime_error(
        std::string("source triangulation strict profile diagnostic: capability=") +
        (bootstrap_capabilities.strict_build ? "1" : "0") +
        " build=" + (bounded::strict_floating_build_enabled() ? "1" : "0") +
        " runtime=" +
        (bounded::runtime_floating_profile_qualified<T>() ? "1" : "0") +
        " rounding=" + std::to_string(std::fegetround()));
  auto preflight = bounded::preflight_precision(*pending.value(),
                                                 bootstrap_capabilities);
  if (!preflight.has_value())
    throw std::runtime_error(std::string("precision preflight for source triangulation: ") +
                             preflight.error()->summary + " [subcode " +
                             std::to_string(preflight.error()->subcode) + "]");
  auto context = bounded::finalize_context(
      std::move(*pending.value()),
      bounded::make_precision_bootstrap_record(*preflight.value()));
  require(context.has_value(), "source triangulation context");
  bounded::precision_runtime_capabilities precision_capabilities;
  precision_capabilities.expected_owner = &context.value()->owner;
  auto precision = bounded::build_precision_context(
      *preflight.value(), *context.value(), precision_capabilities);
  require(precision.has_value(), "source triangulation precision context");
  auto resources =
      std::make_unique<bounded::resource_manager>(context.value()->options.resources);
  bounded::input_validation_capabilities validation_capabilities;
  validation_capabilities.owner = context.value()->owner;
  validation_capabilities.resources = resources.get();
  auto operand = bounded::validate_operand(
      bounded::operand_id::a, context.value()->sources->a, *context.value(),
      **precision.value(), validation_capabilities);
  require(operand.has_value(), "validated source triangulation operand");
  return {std::move(*context.value()), std::move(*precision.value()),
          std::move(*operand.value()), std::move(resources)};
}

template <class T, class I>
auto triangulate(fixture<T, I> &fixture,
                 bounded::source_triangulation_provider_kind provider =
                     bounded::source_triangulation_provider_kind::indexed_dependency_v1,
                 bool compare_reference = true) {
  bounded::source_triangulation_capabilities capabilities;
  capabilities.owner = fixture.context.owner;
  capabilities.resources = fixture.resources.get();
  capabilities.provider = provider;
  capabilities.compare_with_reference = compare_reference;
  auto result = bounded::triangulate_source_operand(
      fixture.operand, fixture.context, *fixture.precision, capabilities);
  if (!result.has_value()) {
    std::string diagnostic = result.error()->summary;
    diagnostic += " [subcode " + std::to_string(result.error()->subcode);
    if (result.error()->witness_count != 0)
      diagnostic += ", finding " +
                    std::to_string(result.error()->witnesses[0]);
    diagnostic += "]";
    throw std::runtime_error(diagnostic);
  }
  return result;
}

} // namespace source_triangulation_tests
