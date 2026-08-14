#pragma once

#include "CanonicalIntersectionComplex.h"
#include "CanonicalSourceManifolds.h"
#include "Context.h"
#include "FloatingBits.h"
#include "PolygonalOutputComplex.h"
#include "RetainedSurfaceComplex.h"
#include "TriangulationCodec.h"
#include "TriangulatedOutputComplex.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

// Self-contained structural verification (no predecessor artifacts beyond the
// polygonal complex). Forward-declared so the full verifier can call it.
template <class T, class I>
bool verify_triangulated_output_complex_structural(
    const triangulated_output_complex<T, I> &artifact,
    const polygonal_output_complex<T, I> &polygonal,
    bounded_boolean_error &error);

template <class T, class I>
bool verify_triangulated_output_complex(
    const triangulated_output_complex<T, I> &artifact,
    const boolean_context<T, I> &context,
    const canonical_source_manifolds<T, I> &manifolds,
    const canonical_intersection_complex<T, I> &intersections,
    const retained_surface_complex<T, I> &retained,
    const polygonal_output_complex<T, I> &polygonal,
    bounded_boolean_error &error) {
  const auto fail = [&](output_triangulation_subcode subcode,
                        const char *summary,
                        output_triangulation_checkpoint checkpoint) {
    error = output_triangulation_error(
        subcode, bounded_boolean_error_category::internal_invariant_error,
        summary, checkpoint);
    return false;
  };

  if (!artifact.owner().anchor)
    return fail(output_triangulation_subcode::wrong_owner,
                "triangulation owner is absent",
                output_triangulation_checkpoint::context_capability_validation);
  if (!artifact.owner().same_owner(context.owner))
    return fail(output_triangulation_subcode::wrong_owner,
                "triangulation owner disagrees with the context",
                output_triangulation_checkpoint::context_capability_validation);
  if (artifact.operation() != context.operation)
    return fail(output_triangulation_subcode::wrong_operation,
                "triangulation operation disagrees with the context",
                output_triangulation_checkpoint::context_capability_validation);
  if (artifact.context_digest() != context.context_digest ||
      artifact.intersection_digest() != intersections.digest() ||
      artifact.retained_digest() != retained.digest() ||
      artifact.polygonal_digest() != polygonal.digest())
    return fail(output_triangulation_subcode::predecessor_digest_mismatch,
                "triangulation predecessor digest mismatch",
                output_triangulation_checkpoint::predecessor_validation);
  if (artifact.manifold_digests()[0] != manifolds.a()->digest() ||
      artifact.manifold_digests()[1] != manifolds.b()->digest())
    return fail(output_triangulation_subcode::predecessor_digest_mismatch,
                "triangulation manifold digest mismatch",
                output_triangulation_checkpoint::predecessor_validation);
  if (polygonal.verification() !=
      output_topology_verification_disposition::independently_verified)
    return fail(output_triangulation_subcode::predecessor_not_verified,
                "polygonal complex is not independently verified",
                output_triangulation_checkpoint::predecessor_validation);

  return verify_triangulated_output_complex_structural(artifact, polygonal,
                                                       error);
}

template <class T, class I>
bool verify_triangulated_output_complex_structural(
    const triangulated_output_complex<T, I> &artifact,
    const polygonal_output_complex<T, I> &polygonal,
    bounded_boolean_error &error) {
  const auto fail = [&](output_triangulation_subcode subcode,
                        const char *summary,
                        output_triangulation_checkpoint checkpoint) {
    error = output_triangulation_error(
        subcode, bounded_boolean_error_category::internal_invariant_error,
        summary, checkpoint);
    return false;
  };

  const std::size_t boundary_halfedge_count = polygonal.halfedges().size();
  const auto &assignments = artifact.boundary_assignments();

  // Boundary conservation: each Component 11 boundary halfedge assigned exactly
  // once, and the reverse map is a bijection.
  if (artifact.boundary_assignment_by_halfedge().size() !=
      boundary_halfedge_count)
    return fail(output_triangulation_subcode::boundary_unassigned,
                "boundary assignment reverse map has the wrong size",
                output_triangulation_checkpoint::boundary_preservation_audit);
  std::vector<std::uint64_t> assignment_count(boundary_halfedge_count, 0);
  for (std::size_t a = 0; a < assignments.size(); ++a) {
    const auto &assignment = assignments[a];
    if (assignment.component11_halfedge >= boundary_halfedge_count)
      return fail(output_triangulation_subcode::boundary_unassigned,
                  "boundary assignment references an out-of-range halfedge",
                  output_triangulation_checkpoint::boundary_preservation_audit);
    ++assignment_count[assignment.component11_halfedge];
    if (artifact.boundary_assignment_by_halfedge()[assignment.component11_halfedge] !=
        a)
      return fail(output_triangulation_subcode::boundary_unassigned,
                  "boundary assignment reverse map is inconsistent",
                  output_triangulation_checkpoint::boundary_preservation_audit);
  }
  for (std::size_t h = 0; h < boundary_halfedge_count; ++h) {
    const bool is_boundary =
        artifact.boundary_assignment_by_halfedge()[h] !=
        triangulation_invalid_ordinal;
    if (is_boundary && assignment_count[h] != 1)
      return fail(output_triangulation_subcode::boundary_multiply_assigned,
                  "boundary halfedge is assigned more than once",
                  output_triangulation_checkpoint::boundary_preservation_audit);
  }

  // Triangle closure: three distinct corners and a closed edge-use cycle.
  const auto &triangles = artifact.triangles();
  const auto &corner_refs = artifact.corner_refs();
  const auto &edge_uses = artifact.edge_use_refs();
  const auto &internal_halfedges = artifact.internal_halfedges();
  for (const auto &triangle : triangles) {
    if (triangle.corners[0] == triangle.corners[1] ||
        triangle.corners[1] == triangle.corners[2] ||
        triangle.corners[2] == triangle.corners[0])
      return fail(output_triangulation_subcode::triangle_closure_failure,
                  "triangle has repeated corners",
                  output_triangulation_checkpoint::region_coverage);
    for (std::size_t k = 0; k < 3; ++k) {
      if (triangle.corner_refs[k] >= corner_refs.size() ||
          triangle.edge_use_refs[k] >= edge_uses.size())
        return fail(output_triangulation_subcode::triangle_closure_failure,
                    "triangle corner or edge-use reference is out of range",
                    output_triangulation_checkpoint::region_coverage);
      if (corner_refs[triangle.corner_refs[k]].triangle !=
              triangle.canonical_id ||
          corner_refs[triangle.corner_refs[k]].occurrence != triangle.corners[k])
        return fail(output_triangulation_subcode::triangle_closure_failure,
                    "triangle corner reference is inconsistent",
                    output_triangulation_checkpoint::region_coverage);
      const auto &edge_use = edge_uses[triangle.edge_use_refs[k]];
      if (edge_use.triangle != triangle.canonical_id)
        return fail(output_triangulation_subcode::triangle_closure_failure,
                    "triangle edge-use reference is inconsistent",
                    output_triangulation_checkpoint::region_coverage);
      const std::uint64_t from = triangle.corners[k];
      const std::uint64_t to = triangle.corners[(k + 1) % 3];
      if (edge_use.is_boundary) {
        if (edge_use.halfedge >= boundary_halfedge_count)
          return fail(output_triangulation_subcode::triangle_closure_failure,
                      "boundary edge use references an out-of-range halfedge",
                      output_triangulation_checkpoint::region_coverage);
        const auto &he = polygonal.halfedges()[edge_use.halfedge];
        if (he.origin != from || he.destination != to)
          return fail(output_triangulation_subcode::triangle_closure_failure,
                      "boundary edge use does not match triangle corners",
                      output_triangulation_checkpoint::region_coverage);
      } else {
        if (edge_use.halfedge >= internal_halfedges.size())
          return fail(output_triangulation_subcode::triangle_closure_failure,
                      "internal edge use references an out-of-range halfedge",
                      output_triangulation_checkpoint::region_coverage);
        const auto &he = internal_halfedges[edge_use.halfedge];
        if (he.origin != from || he.destination != to)
          return fail(output_triangulation_subcode::triangle_closure_failure,
                      "internal edge use does not match triangle corners",
                      output_triangulation_checkpoint::region_coverage);
      }
    }
  }

  // Internal halfedge balance: each internal halfedge used by exactly one edge
  // use, and its pair used by exactly one edge use.
  std::vector<std::uint64_t> internal_use_count(internal_halfedges.size(), 0);
  for (const auto &edge_use : edge_uses)
    if (!edge_use.is_boundary)
      ++internal_use_count[edge_use.halfedge];
  for (std::size_t h = 0; h < internal_halfedges.size(); ++h) {
    if (internal_use_count[h] != 1)
      return fail(output_triangulation_subcode::diagonal_pair_failure,
                  "internal halfedge is not used exactly once",
                  output_triangulation_checkpoint::region_coverage);
    const auto &he = internal_halfedges[h];
    if (he.pair >= internal_halfedges.size() ||
        internal_halfedges[he.pair].pair != h)
      return fail(output_triangulation_subcode::diagonal_pair_failure,
                  "internal halfedge pair is not reciprocal",
                  output_triangulation_checkpoint::region_coverage);
    if (he.origin != internal_halfedges[he.pair].destination ||
        he.destination != internal_halfedges[he.pair].origin)
      return fail(output_triangulation_subcode::diagonal_pair_failure,
                  "internal halfedge endpoints are not reversed",
                  output_triangulation_checkpoint::region_coverage);
  }
  // Each diagonal has exactly two final triangle uses.
  for (const auto &diagonal : artifact.diagonals()) {
    if (diagonal.halfedges[0] >= internal_halfedges.size() ||
        diagonal.halfedges[1] >= internal_halfedges.size())
      return fail(output_triangulation_subcode::diagonal_pair_failure,
                  "diagonal halfedge slot is out of range",
                  output_triangulation_checkpoint::region_coverage);
    if (diagonal.state != diagonal_state::both_triangle_sides_assigned)
      return fail(output_triangulation_subcode::diagonal_pair_failure,
                  "diagonal is not fully assigned to two triangles",
                  output_triangulation_checkpoint::region_coverage);
  }

  // Per-region Euler certificate and orientation consistency.
  for (const auto &certificate : artifact.coverage_certificates()) {
    const std::uint64_t r = certificate.region;
    if (r >= polygonal.face_regions().size())
      return fail(output_triangulation_subcode::invalid_region_range,
                  "coverage certificate region is out of range",
                  output_triangulation_checkpoint::region_coverage);
    const std::uint64_t v = certificate.vertex_count;
    const std::uint64_t e = certificate.edge_count;
    const std::uint64_t f = certificate.face_count;
    const std::uint64_t h = certificate.hole_count;
    // V - E + F = 1 - H for a connected planar disk triangulation.
    if (v < 3 || e < 3 || f < 1)
      return fail(output_triangulation_subcode::euler_mismatch,
                  "coverage certificate counts are degenerate",
                  output_triangulation_checkpoint::region_coverage);
    if (v + f != e + 1 - h)
      return fail(output_triangulation_subcode::euler_mismatch,
                  "coverage certificate violates Euler characteristic",
                  output_triangulation_checkpoint::region_coverage);
    if (certificate.triangle_count != f ||
        certificate.diagonal_count != e - (v))
      return fail(output_triangulation_subcode::euler_mismatch,
                  "coverage certificate ranges disagree with counts",
                  output_triangulation_checkpoint::region_coverage);
    if (certificate.disposition != coverage_disposition::verified)
      return fail(output_triangulation_subcode::coverage_missing_pocket,
                  "coverage certificate is not verified",
                  output_triangulation_checkpoint::region_coverage);
  }

  // Every triangle references a certificate-covered region.
  for (const auto &triangle : triangles)
    if (triangle.region >= artifact.coverage_certificates().size() ||
        artifact.coverage_certificates()[triangle.region].region !=
            triangle.region)
      return fail(output_triangulation_subcode::invalid_region_range,
                  "triangle region is not covered by a certificate",
                  output_triangulation_checkpoint::region_coverage);

  // Re-encode and verify the digest.
  {
    output_triangulation_codec_limits limits;
    std::vector<std::uint8_t> bytes;
    bounded_boolean_error codec_error;
    if (!encode_triangulated_output_complex(artifact, bytes, limits,
                                            codec_error)) {
      error = codec_error;
      return false;
    }
    if (sha256::digest(bytes) != artifact.digest())
      return fail(output_triangulation_subcode::digest_mismatch,
                  "triangulation digest does not match canonical bytes",
                  output_triangulation_checkpoint::canonical_encoding);
  }

  return true;
}

} // namespace ygor::mesh_boolean::bounded
