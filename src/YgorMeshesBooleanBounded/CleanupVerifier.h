#pragma once

#include "CanonicalIntersectionComplex.h"
#include "CanonicalSourceManifolds.h"
#include "CleanupCodec.h"
#include "Context.h"
#include "FloatingBits.h"
#include "PolygonalOutputComplex.h"
#include "RetainedSurfaceComplex.h"
#include "TriangulatedOutputComplex.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T>
bool verify_cleaned_triangle_manifold_structural(
    const cleaned_triangle_manifold<T> &artifact,
    bounded_boolean_error &error);

template <class T, class I>
bool verify_cleaned_triangle_manifold(
    const cleaned_triangle_manifold<T> &artifact,
    const boolean_context<T, I> &context,
    const precision_context<T> &precision,
    const polygonal_output_complex<T, I> &polygonal,
    const triangulated_output_complex<T, I> &triangulated,
    bounded_boolean_error &error) {
  const auto fail = [&](cleanup_subcode subcode, const char *summary,
                        cleanup_checkpoint checkpoint) {
    error = cleanup_error(subcode,
                          bounded_boolean_error_category::internal_invariant_error,
                          summary, checkpoint);
    return false;
  };

  if (!artifact.owner().anchor)
    return fail(cleanup_subcode::wrong_owner, "cleanup owner is absent",
                cleanup_checkpoint::context_capability_validation);
  if (!artifact.owner().same_owner(context.owner))
    return fail(cleanup_subcode::wrong_owner,
                "cleanup owner disagrees with the context",
                cleanup_checkpoint::context_capability_validation);
  if (artifact.operation() != context.operation)
    return fail(cleanup_subcode::wrong_operation,
                "cleanup operation disagrees with the context",
                cleanup_checkpoint::context_capability_validation);
  if (artifact.context_digest() != context.context_digest ||
      artifact.precision_digest() != precision.digest() ||
      artifact.polygonal_digest() != polygonal.digest() ||
      artifact.triangulated_digest() != triangulated.digest())
    return fail(cleanup_subcode::predecessor_digest_mismatch,
                "cleanup predecessor digest mismatch",
                cleanup_checkpoint::predecessor_validation);
  if (triangulated.verification() !=
      triangulation_verification_disposition::independently_verified)
    return fail(cleanup_subcode::predecessor_not_verified,
                "triangulated complex is not independently verified",
                cleanup_checkpoint::predecessor_validation);

  return verify_cleaned_triangle_manifold_structural(artifact, error);
}

template <class T>
bool verify_cleaned_triangle_manifold_structural(
    const cleaned_triangle_manifold<T> &artifact,
    bounded_boolean_error &error) {
  const auto fail = [&](cleanup_subcode subcode, const char *summary,
                        cleanup_checkpoint checkpoint) {
    error = cleanup_error(subcode,
                          bounded_boolean_error_category::internal_invariant_error,
                          summary, checkpoint);
    return false;
  };

  const auto &vertices = artifact.vertices();
  const auto &edges = artifact.paired_edges();
  const auto &halfedges = artifact.halfedges();
  const auto &triangles = artifact.triangles();

  // No residual or pending obligation may remain on ordinary success.
  if (artifact.status() == final_cleanup_status::verified_clean) {
    for (const auto &disposition : artifact.obligation_dispositions())
      if (disposition.disposition !=
              cleanup_obligation_disposition::proven_final_valid &&
          disposition.disposition !=
              cleanup_obligation_disposition::discharged_by_action &&
          disposition.disposition !=
              cleanup_obligation_disposition::absorbed_with_member_evidence &&
          disposition.disposition !=
              cleanup_obligation_disposition::removed_with_component)
        return fail(cleanup_subcode::final_obligation_remains,
                    "an obligation is not discharged",
                    cleanup_checkpoint::final_manifold_verification);
  }

  // Finite vertex coordinates.
  for (const auto &vertex : vertices)
    for (std::size_t axis = 0; axis < 3; ++axis)
      if (!finite_bits(from_bits<T>(static_cast<floating_uint_t<T>>(
              vertex.nominal_bits[axis]))))
        return fail(cleanup_subcode::final_manifold_failure,
                    "cleaned vertex coordinate is not finite",
                    cleanup_checkpoint::final_manifold_verification);

  // Pair reciprocity and reversed endpoints.
  std::vector<std::uint64_t> edge_use_count(edges.size(), 0);
  for (std::size_t h = 0; h < halfedges.size(); ++h) {
    const auto &halfedge = halfedges[h];
    if (halfedge.pair >= halfedges.size() ||
        halfedges[halfedge.pair].pair != h)
      return fail(cleanup_subcode::pair_cell_fan_contradiction,
                  "cleaned halfedge pair is not reciprocal",
                  cleanup_checkpoint::pair_cell_fan_audit);
    if (halfedge.origin != halfedges[halfedge.pair].destination ||
        halfedge.destination != halfedges[halfedge.pair].origin)
      return fail(cleanup_subcode::pair_cell_fan_contradiction,
                  "cleaned halfedge endpoints are not reversed",
                  cleanup_checkpoint::pair_cell_fan_audit);
    if (halfedge.origin >= vertices.size() ||
        halfedge.destination >= vertices.size())
      return fail(cleanup_subcode::pair_cell_fan_contradiction,
                  "cleaned halfedge endpoint is out of range",
                  cleanup_checkpoint::pair_cell_fan_audit);
    if (halfedge.edge >= edges.size())
      return fail(cleanup_subcode::pair_cell_fan_contradiction,
                  "cleaned halfedge edge reference is out of range",
                  cleanup_checkpoint::pair_cell_fan_audit);
    if (halfedge.triangle >= triangles.size())
      return fail(cleanup_subcode::pair_cell_fan_contradiction,
                  "cleaned halfedge triangle reference is out of range",
                  cleanup_checkpoint::pair_cell_fan_audit);
  }

  // Each paired edge has exactly two incident triangles.
  for (std::size_t e = 0; e < edges.size(); ++e) {
    const auto &edge = edges[e];
    if (edge.halfedges[0] >= halfedges.size() ||
        edge.halfedges[1] >= halfedges.size())
      return fail(cleanup_subcode::pair_cell_fan_contradiction,
                  "cleaned edge halfedge slot is out of range",
                  cleanup_checkpoint::pair_cell_fan_audit);
    const auto &h0 = halfedges[edge.halfedges[0]];
    const auto &h1 = halfedges[edge.halfedges[1]];
    if (h0.edge != e || h1.edge != e)
      return fail(cleanup_subcode::pair_cell_fan_contradiction,
                  "cleaned halfedge does not map back to its edge",
                  cleanup_checkpoint::pair_cell_fan_audit);
    if (h0.triangle == h1.triangle || h0.triangle >= triangles.size() ||
        h1.triangle >= triangles.size())
      return fail(cleanup_subcode::pair_cell_fan_contradiction,
                  "cleaned edge does not have two distinct incident triangles",
                  cleanup_checkpoint::pair_cell_fan_audit);
    if (edge.triangles[0] != h0.triangle || edge.triangles[1] != h1.triangle)
      return fail(cleanup_subcode::pair_cell_fan_contradiction,
                  "cleaned edge triangle slots disagree with halfedges",
                  cleanup_checkpoint::pair_cell_fan_audit);
  }

  // Triangle closure and distinct corners.
  for (const auto &triangle : triangles) {
    if (triangle.corners[0] == triangle.corners[1] ||
        triangle.corners[1] == triangle.corners[2] ||
        triangle.corners[2] == triangle.corners[0])
      return fail(cleanup_subcode::pair_cell_fan_contradiction,
                  "cleaned triangle has repeated corners",
                  cleanup_checkpoint::pair_cell_fan_audit);
    if (triangle.category != triangle_geometric_category::definite_positive_area)
      return fail(cleanup_subcode::final_manifold_failure,
                  "cleaned triangle is not definite positive area",
                  cleanup_checkpoint::final_manifold_verification);
    for (std::size_t k = 0; k < 3; ++k) {
      if (triangle.corners[k] >= vertices.size() ||
          triangle.halfedges[k] >= halfedges.size())
        return fail(cleanup_subcode::pair_cell_fan_contradiction,
                    "cleaned triangle corner is out of range",
                    cleanup_checkpoint::pair_cell_fan_audit);
      const auto &halfedge = halfedges[triangle.halfedges[k]];
      if (halfedge.triangle != triangle.canonical_id)
        return fail(cleanup_subcode::pair_cell_fan_contradiction,
                    "cleaned triangle halfedge does not map back",
                    cleanup_checkpoint::pair_cell_fan_audit);
      if (halfedge.origin != triangle.corners[k] ||
          halfedge.destination != triangle.corners[(k + 1) % 3])
        return fail(cleanup_subcode::pair_cell_fan_contradiction,
                    "cleaned triangle halfedge does not match corners",
                    cleanup_checkpoint::pair_cell_fan_audit);
    }
  }

  // One closed fan per vertex occurrence via around_origin(h) = pair(next(next(h))).
  for (std::size_t v = 0; v < vertices.size(); ++v) {
    const auto &vertex = vertices[v];
    if (vertex.link_begin + vertex.link_count > artifact.link_index().size())
      return fail(cleanup_subcode::pair_cell_fan_contradiction,
                  "cleaned vertex link range is out of bounds",
                  cleanup_checkpoint::pair_cell_fan_audit);
    if (vertex.link_count == 0)
      return fail(cleanup_subcode::pair_cell_fan_contradiction,
                  "cleaned vertex has an empty link",
                  cleanup_checkpoint::pair_cell_fan_audit);
    const std::uint64_t start = artifact.link_index()[vertex.link_begin];
    std::uint64_t current = start;
    std::uint64_t walked = 0;
    do {
      if (current >= halfedges.size() || halfedges[current].origin != v)
        return fail(cleanup_subcode::pair_cell_fan_contradiction,
                    "cleaned vertex link walk leaves the occurrence",
                    cleanup_checkpoint::pair_cell_fan_audit);
      ++walked;
      if (walked > vertex.link_count)
        return fail(cleanup_subcode::pair_cell_fan_contradiction,
                    "cleaned vertex link walk does not close",
                    cleanup_checkpoint::pair_cell_fan_audit);
      const std::uint64_t prev = halfedges[halfedges[current].next].next;
      current = halfedges[prev].pair;
    } while (current != start);
    if (walked != vertex.link_count)
      return fail(cleanup_subcode::pair_cell_fan_contradiction,
                  "cleaned vertex link walk count disagrees with link",
                  cleanup_checkpoint::pair_cell_fan_audit);
  }

  // Per-component Euler characteristic (closed orientable genus-g surface).
  for (const auto &component : artifact.components()) {
    if (component.triangles_begin + component.triangles_count >
        artifact.component_triangle_index().size())
      return fail(cleanup_subcode::final_manifold_failure,
                  "cleaned component triangle range is out of bounds",
                  cleanup_checkpoint::final_manifold_verification);
    const std::int64_t chi = static_cast<std::int64_t>(component.vertex_count) -
                             static_cast<std::int64_t>(component.edge_count) +
                             static_cast<std::int64_t>(component.face_count);
    if (chi != component.euler_chi)
      return fail(cleanup_subcode::final_manifold_failure,
                  "cleaned component Euler characteristic disagrees",
                  cleanup_checkpoint::final_manifold_verification);
    if (chi % 2 != 0)
      return fail(cleanup_subcode::final_manifold_failure,
                  "cleaned component Euler characteristic is odd",
                  cleanup_checkpoint::final_manifold_verification);
  }

  // Re-encode and verify the digest.
  {
    cleanup_codec_limits limits;
    std::vector<std::uint8_t> bytes;
    bounded_boolean_error codec_error;
    if (!encode_cleaned_triangle_manifold(artifact, bytes, limits, codec_error)) {
      error = codec_error;
      return false;
    }
    if (sha256::digest(bytes) != artifact.digest())
      return fail(cleanup_subcode::digest_mismatch,
                  "cleanup digest does not match canonical bytes",
                  cleanup_checkpoint::canonical_encoding);
  }

  return true;
}

} // namespace ygor::mesh_boolean::bounded
