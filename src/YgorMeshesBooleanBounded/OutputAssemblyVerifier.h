#pragma once

#include "AssembledOutputCandidate.h"
#include "CleanedTriangleManifold.h"
#include "Context.h"
#include "FloatingBits.h"
#include "OutputAssemblyCodec.h"
#include "PrecisionContext.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
bool verify_assembled_output_candidate_structural(
    const assembled_output_candidate<T, I> &candidate,
    const cleaned_triangle_manifold<T> &cleaned, bounded_boolean_error &error);

template <class T, class I>
bool verify_assembled_output_candidate(
    const assembled_output_candidate<T, I> &candidate,
    const boolean_context<T, I> &context, const precision_context<T> &precision,
    const cleaned_triangle_manifold<T> &cleaned, bounded_boolean_error &error) {
  const auto fail = [&](output_assembly_subcode subcode, const char *summary,
                        output_assembly_checkpoint checkpoint) {
    error = output_assembly_error(
        subcode, bounded_boolean_error_category::internal_invariant_error,
        summary, checkpoint);
    return false;
  };

  if (!candidate.owner().anchor)
    return fail(output_assembly_subcode::wrong_owner, "assembly owner is absent",
                output_assembly_checkpoint::context_capability_validation);
  if (!candidate.owner().same_owner(context.owner))
    return fail(output_assembly_subcode::wrong_owner,
                "assembly owner disagrees with the context",
                output_assembly_checkpoint::context_capability_validation);
  if (candidate.operation() != context.operation)
    return fail(output_assembly_subcode::wrong_operation,
                "assembly operation disagrees with the context",
                output_assembly_checkpoint::context_capability_validation);
  if (candidate.context_digest() != context.context_digest ||
      candidate.precision_digest() != precision.digest() ||
      candidate.cleaned_digest() != cleaned.digest())
    return fail(output_assembly_subcode::predecessor_digest_mismatch,
                "assembly predecessor digest mismatch",
                output_assembly_checkpoint::predecessor_validation);
  if (candidate.topology_status() !=
          candidate_topology_status::assembled_pending_independent_verification ||
      candidate.geometry_status() !=
          candidate_geometry_status::finite_and_bounded_pending_independent_verification)
    return fail(output_assembly_subcode::pending_status_violation,
                "assembly candidate has a non-pending status",
                output_assembly_checkpoint::candidate_construction);

  return verify_assembled_output_candidate_structural(candidate, cleaned, error);
}

template <class T, class I>
bool verify_assembled_output_candidate_structural(
    const assembled_output_candidate<T, I> &candidate,
    const cleaned_triangle_manifold<T> &cleaned, bounded_boolean_error &error) {
  const auto fail = [&](output_assembly_subcode subcode, const char *summary,
                        output_assembly_checkpoint checkpoint) {
    error = output_assembly_error(
        subcode, bounded_boolean_error_category::internal_invariant_error,
        summary, checkpoint);
    return false;
  };

  const auto &mesh = candidate.mesh();
  const std::size_t vertex_count = mesh.vertices.size();
  const std::size_t facet_count = mesh.faces.size();

  // Count bijection with the cleaned manifold.
  if (vertex_count != cleaned.vertices().size() ||
      facet_count != cleaned.triangles().size())
    return fail(output_assembly_subcode::map_mismatch,
                "public mesh counts disagree with the cleaned manifold",
                output_assembly_checkpoint::map_construction);

  // Each face is a triangle with three distinct in-range indices.
  std::map<std::pair<std::uint64_t, std::uint64_t>,
           std::vector<std::pair<std::uint64_t, std::uint64_t>>>
      uses_by_edge;
  std::vector<std::vector<std::uint64_t>> incident_corners(vertex_count);
  for (std::size_t f = 0; f < facet_count; ++f) {
    const auto &face = mesh.faces[f];
    if (face.size() != 3)
      return fail(output_assembly_subcode::face_write_readback_mismatch,
                  "public facet is not a triangle",
                  output_assembly_checkpoint::topology_reconstruction);
    const std::uint64_t i0 = static_cast<std::uint64_t>(face[0]);
    const std::uint64_t i1 = static_cast<std::uint64_t>(face[1]);
    const std::uint64_t i2 = static_cast<std::uint64_t>(face[2]);
    if (i0 >= vertex_count || i1 >= vertex_count || i2 >= vertex_count)
      return fail(output_assembly_subcode::face_write_readback_mismatch,
                  "public facet index is out of range",
                  output_assembly_checkpoint::topology_reconstruction);
    if (i0 == i1 || i1 == i2 || i2 == i0)
      return fail(output_assembly_subcode::face_write_readback_mismatch,
                  "public facet has repeated indices",
                  output_assembly_checkpoint::topology_reconstruction);
    const auto add_use = [&](std::uint64_t a, std::uint64_t b) {
      uses_by_edge[std::make_pair(std::min(a, b), std::max(a, b))]
          .push_back(std::make_pair(f, a));
    };
    add_use(i0, i1);
    add_use(i1, i2);
    add_use(i2, i0);
    incident_corners[i0].push_back(f);
    incident_corners[i1].push_back(f);
    incident_corners[i2].push_back(f);
  }

  // Each undirected edge has exactly two opposite uses.
  for (const auto &entry : uses_by_edge) {
    const auto &uses = entry.second;
    if (uses.size() != 2)
      return fail(output_assembly_subcode::edge_use_mismatch,
                  "public edge does not have exactly two uses",
                  output_assembly_checkpoint::topology_reconstruction);
    if (uses[0].second == uses[1].second)
      return fail(output_assembly_subcode::edge_use_mismatch,
                  "public edge uses are not reversed",
                  output_assembly_checkpoint::topology_reconstruction);
  }

  // Each vertex has a nonempty set of incident corners (no isolated vertex).
  for (std::size_t v = 0; v < vertex_count; ++v)
    if (incident_corners[v].empty())
      return fail(output_assembly_subcode::vertex_link_mismatch,
                  "public vertex is isolated",
                  output_assembly_checkpoint::topology_reconstruction);

  // Maps are bijections.
  if (candidate.vertex_by_occurrence().size() != cleaned.vertices().size() ||
      candidate.occurrence_by_vertex().size() != vertex_count ||
      candidate.facet_by_triangle().size() != cleaned.triangles().size() ||
      candidate.triangle_by_facet().size() != facet_count)
    return fail(output_assembly_subcode::map_mismatch,
                "assembly map sizes are inconsistent",
                output_assembly_checkpoint::map_construction);
  for (std::size_t o = 0; o < cleaned.vertices().size(); ++o) {
    const std::uint64_t p = candidate.vertex_by_occurrence()[o];
    if (p >= vertex_count || candidate.occurrence_by_vertex()[p] != o)
      return fail(output_assembly_subcode::map_mismatch,
                  "assembly vertex maps are not inverse",
                  output_assembly_checkpoint::map_construction);
  }
  for (std::size_t t = 0; t < cleaned.triangles().size(); ++t) {
    const std::uint64_t f = candidate.facet_by_triangle()[t];
    if (f >= facet_count || candidate.triangle_by_facet()[f] != t)
      return fail(output_assembly_subcode::map_mismatch,
                  "assembly facet maps are not inverse",
                  output_assembly_checkpoint::map_construction);
  }

  // Re-encode public content and verify it matches the stored bytes.
  {
    std::vector<std::uint8_t> content;
    if (!encode_public_mesh_content(candidate, content, error))
      return false;
    if (content != candidate.public_content_bytes())
      return fail(output_assembly_subcode::digest_mismatch,
                  "public content bytes disagree with re-encoding",
                  output_assembly_checkpoint::canonical_encoding);
  }

  // Re-encode the full candidate and verify the digest.
  {
    output_assembly_codec_limits limits;
    std::vector<std::uint8_t> bytes;
    bounded_boolean_error codec_error;
    if (!encode_assembled_output_candidate(candidate, bytes, limits, codec_error)) {
      error = codec_error;
      return false;
    }
    if (sha256::digest(bytes) != candidate.digest())
      return fail(output_assembly_subcode::digest_mismatch,
                  "assembly digest does not match canonical bytes",
                  output_assembly_checkpoint::digest_construction);
  }

  return true;
}

} // namespace ygor::mesh_boolean::bounded
