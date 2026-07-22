#pragma once

#include "SourceTriangulationFixtures.h"
#include "YgorMeshesBooleanBounded/CanonicalHalfedgeBuild.h"
#include "YgorMeshesBooleanBounded/CanonicalHalfedgeQueries.h"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace ygor::mesh_boolean::bounded {

struct canonical_halfedge_test_access final {
  template <class T, class I>
  static canonical_halfedge_operand<T, I>
  copy(const canonical_halfedge_operand<T, I> &artifact) {
    return artifact;
  }
  template <class T, class I>
  static auto &vertices(canonical_halfedge_operand<T, I> &artifact) {
    return artifact.vertices_;
  }
  template <class T, class I>
  static auto &triangles(canonical_halfedge_operand<T, I> &artifact) {
    return artifact.triangles_;
  }
  template <class T, class I>
  static auto &halfedges(canonical_halfedge_operand<T, I> &artifact) {
    return artifact.halfedges_;
  }
  template <class T, class I>
  static auto &edges(canonical_halfedge_operand<T, I> &artifact) {
    return artifact.edges_;
  }
  template <class T, class I>
  static auto &fans(canonical_halfedge_operand<T, I> &artifact) {
    return artifact.fans_;
  }
  template <class T, class I>
  static auto &facet_groups(canonical_halfedge_operand<T, I> &artifact) {
    return artifact.facet_groups_;
  }
  template <class T, class I>
  static auto &canonical_bytes(canonical_halfedge_operand<T, I> &artifact) {
    return artifact.canonical_bytes_;
  }
};

} // namespace ygor::mesh_boolean::bounded

namespace canonical_halfedge_tests {
namespace bounded = ygor::mesh_boolean::bounded;

inline void require(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}

template <class T, class I> struct built_fixture final {
  source_triangulation_tests::fixture<T, I> predecessor;
  std::shared_ptr<const bounded::source_triangle_complex<T, I>> source;
  std::shared_ptr<const bounded::canonical_halfedge_operand<T, I>> artifact;
};

template <class T, class I>
built_fixture<T, I> build(const fv_surface_mesh<T, I> &mesh) {
  auto predecessor = source_triangulation_tests::make_fixture<T, I>(mesh);
  auto triangulated = source_triangulation_tests::triangulate(predecessor);
  require(triangulated.has_value(), "source triangulation for canonical halfedge");
  auto source = *triangulated.value();
  bounded::canonical_halfedge_capabilities capabilities;
  capabilities.owner = predecessor.context.owner;
  capabilities.resources = predecessor.resources.get();
  auto built = bounded::build_canonical_halfedge_operand(
      predecessor.operand, source, predecessor.context,
      *predecessor.precision, capabilities);
  if (!built.has_value()) {
    std::string diagnostic = built.error()->summary;
    diagnostic += " [subcode " + std::to_string(built.error()->subcode);
    if (built.error()->witness_count)
      diagnostic += ", finding " +
                    std::to_string(built.error()->witnesses[0]);
    diagnostic += "]";
    throw std::runtime_error(diagnostic);
  }
  return {std::move(predecessor), std::move(source),
          std::move(*built.value())};
}

template <class T, class I>
bounded::canonical_halfedge_capabilities capabilities(
    built_fixture<T, I> &fixture) {
  bounded::canonical_halfedge_capabilities out;
  out.owner = fixture.predecessor.context.owner;
  out.resources = fixture.predecessor.resources.get();
  return out;
}

} // namespace canonical_halfedge_tests
