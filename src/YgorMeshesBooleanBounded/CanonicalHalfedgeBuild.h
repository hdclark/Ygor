#pragma once

#include "CanonicalFeatureGroups.h"
#include "CanonicalGeometryAttachments.h"
#include "CanonicalHalfedgeCodec.h"
#include "CanonicalHalfedgePairing.h"
#include "CanonicalHalfedgePreflight.h"
#include "CanonicalHalfedgeQueries.h"
#include "CanonicalHalfedgeVerifier.h"
#include "CanonicalSourceManifolds.h"
#include "CanonicalVertexFans.h"
#include "CheckedArithmetic.h"
#include "FloatingBits.h"
#include "Outcome.h"
#include "SourceTriangulation.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

namespace canonical_halfedge_build_detail {

inline canonical_edge_use_key canonical_edge_use_identity(
    const source_triangle_edge_use &use) noexcept {
  canonical_edge_use_key key;
  key.role = use.role;
  if (use.role == source_triangle_edge_role::source_boundary) {
    key.primary = use.source_undirected_edge;
    key.secondary = use.facet;
    key.directed_use = use.source_directed_use;
  } else {
    key.primary = use.facet;
    key.secondary = use.diagonal;
    key.directed_use = use.opposite_edge_use;
  }
  key.source_corner = use.source_corner;
  return key;
}

template <class T>
inline canonical_triangle_rotation_key canonical_rotation(
    operand_id operand, const source_triangle_record<T> &triangle,
    const std::vector<source_triangle_edge_use> &uses,
    std::uint8_t shift) {
  canonical_triangle_rotation_key key;
  for (std::uint8_t slot = 0; slot < 3; ++slot) {
    const auto source_slot = static_cast<std::uint8_t>((slot + shift) % 3);
    key.vertices[slot].operand = operand;
    key.vertices[slot].source_vertex = triangle.vertices[source_slot];
    key.edge_uses[slot] =
        canonical_edge_use_identity(uses[triangle.edge_uses[source_slot]]);
  }
  return key;
}

template <class T>
inline std::pair<canonical_triangle_rotation_key, std::uint8_t>
least_orientation_preserving_rotation(
    operand_id operand, const source_triangle_record<T> &triangle,
    const std::vector<source_triangle_edge_use> &uses) {
  auto best = canonical_rotation(operand, triangle, uses, 0);
  std::uint8_t best_shift = 0;
  for (std::uint8_t shift = 1; shift < 3; ++shift) {
    auto candidate = canonical_rotation(operand, triangle, uses, shift);
    if (candidate < best) {
      best = std::move(candidate);
      best_shift = shift;
    }
  }
  return {std::move(best), best_shift};
}

template <class T>
inline std::uint64_t bound_storage_bytes(const canonical_bound3<T> &) noexcept {
  return sizeof(canonical_bound3<T>);
}

inline void write_digest(canonical_writer &writer,
                         const bounded_boolean_digest &digest) {
  for (auto byte : digest.bytes)
    writer.u8(byte);
}

} // namespace canonical_halfedge_build_detail

template <class T, class I> class canonical_halfedge_builder final {
public:
  canonical_halfedge_builder(
      std::shared_ptr<const validated_operand<T, I>> validated,
      std::shared_ptr<const source_triangle_complex<T, I>> source,
      const boolean_context<T, I> &context, const precision_context<T> &precision,
      canonical_halfedge_capabilities capabilities)
      : validated_(std::move(validated)), source_(std::move(source)),
        context_(context), precision_(precision), capabilities_(capabilities) {}

  boolean_outcome<std::shared_ptr<const canonical_halfedge_operand<T, I>>> run();

private:
  struct triangle_proposal final {
    canonical_triangle_key key{};
    std::uint64_t source_triangle = 0;
    std::uint8_t shift = 0;
    friend bool operator<(const triangle_proposal &a,
                          const triangle_proposal &b) noexcept {
      return std::tie(a.key, a.source_triangle, a.shift) <
             std::tie(b.key, b.source_triangle, b.shift);
    }
  };

  bool validate_contracts();
  bool reserve_resources();
  bool build_vertices();
  bool build_triangles_and_halfedges();
  bool build_pairs_and_edges();
  bool build_vertex_fans();
  bool build_feature_groups();
  bool finalize_and_verify();
  bool check_cancel(canonical_halfedge_checkpoint checkpoint);
  bool fail(canonical_halfedge_subcode subcode,
            bounded_boolean_error_category category, const char *summary,
            canonical_halfedge_checkpoint checkpoint);
  std::uint64_t persistent_bytes() const noexcept;

  std::shared_ptr<const validated_operand<T, I>> validated_;
  std::shared_ptr<const source_triangle_complex<T, I>> source_;
  const boolean_context<T, I> &context_;
  const precision_context<T> &precision_;
  canonical_halfedge_capabilities capabilities_{};
  canonical_halfedge_preflight_counts counts_{};
  std::unique_ptr<canonical_halfedge_operand<T, I>> artifact_;
  std::vector<std::uint8_t> triangle_shifts_;
  std::optional<resource_reservation> persistent_reservation_;
  std::optional<resource_reservation> temporary_reservation_;
  std::optional<resource_reservation> work_reservation_;
  bounded_boolean_error error_{};
};

template <class T, class I>
boolean_outcome<std::shared_ptr<const canonical_halfedge_operand<T, I>>>
build_canonical_halfedge_operand(
    std::shared_ptr<const validated_operand<T, I>> validated,
    std::shared_ptr<const source_triangle_complex<T, I>> source,
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    canonical_halfedge_capabilities capabilities = {}) {
  canonical_halfedge_builder<T, I> builder(std::move(validated),
                                            std::move(source), context,
                                            precision, capabilities);
  return builder.run();
}

template <class T, class I>
boolean_outcome<std::shared_ptr<const canonical_source_manifolds<T, I>>>
build_canonical_source_manifolds(
    std::shared_ptr<const validated_operand<T, I>> validated_a,
    std::shared_ptr<const validated_operand<T, I>> validated_b,
    std::shared_ptr<const source_triangle_complex<T, I>> source_a,
    std::shared_ptr<const source_triangle_complex<T, I>> source_b,
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    canonical_halfedge_capabilities capabilities = {});

} // namespace ygor::mesh_boolean::bounded

#include "CanonicalHalfedgeBuildCore.h"
#include "CanonicalHalfedgeBuildTopology.h"
