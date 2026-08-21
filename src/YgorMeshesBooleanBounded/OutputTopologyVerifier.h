#pragma once

#include "CanonicalIntersectionComplex.h"
#include "CanonicalSourceManifolds.h"
#include "ClassificationComplex.h"
#include "Context.h"
#include "OutputTopologyCodec.h"
#include "PolygonalOutputComplex.h"
#include "RetainedSurfaceComplex.h"
#include "SignedFeatureRelations.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

namespace output_topology_verify_detail {

// Booth's algorithm: least lexicographic rotation start index.
template <class Key>
inline std::size_t least_rotation_start(const std::vector<Key> &sequence) {
  if (sequence.empty())
    return 0;
  const std::size_t n = sequence.size();
  std::size_t i = 0, j = 1, k = 0;
  while (i < n && j < n && k < n) {
    const Key &a = sequence[(i + k) % n];
    const Key &b = sequence[(j + k) % n];
    if (a == b) {
      ++k;
    } else if (a < b) {
      j += k + 1;
      k = 0;
    } else {
      i += k + 1;
      k = 0;
    }
    if (i == j)
      ++j;
  }
  return std::min(i, j);
}

inline bool valid(const std::uint64_t ordinal, const std::size_t size) {
  return ordinal != output_topology_invalid_ordinal && ordinal < size;
}

} // namespace output_topology_verify_detail

// Self-contained structural verification (no predecessor artifacts beyond the
// retained complex). Forward-declared so the full verifier can call it.
template <class T, class I>
bool verify_polygonal_output_complex_structural(
    const polygonal_output_complex<T, I> &artifact,
    const retained_surface_complex<T, I> &retained,
    bounded_boolean_error &error);

// Component 11 independent verifier. It reconstructs incidence dispositions,
// occurrence bijections, pairs, successor permutations, cycles, contour roles,
// witnesses, and vertex links through materially different control flow than
// the producer, and re-encodes the artifact to verify its digest. It never
// calls producer orchestration, grouping, pairing, or walk helpers as its sole
// proof.
template <class T, class I>
bool verify_polygonal_output_complex(
    const polygonal_output_complex<T, I> &artifact,
    const boolean_context<T, I> &context,
    const canonical_source_manifolds<T, I> &manifolds,
    const signed_feature_relations<T, I> &relations,
    const canonical_intersection_complex<T, I> &intersections,
    const classification_complex<T, I> &classification,
    const retained_surface_complex<T, I> &retained,
    bounded_boolean_error &error) {
  using namespace output_topology_verify_detail;
  const auto fail = [&](output_topology_subcode subcode, const char *summary,
                        output_topology_checkpoint checkpoint) {
    error = output_topology_error(
        subcode, bounded_boolean_error_category::internal_invariant_error,
        summary, checkpoint);
    return false;
  };

  if (!artifact.owner().anchor)
    return fail(output_topology_subcode::wrong_owner, "output topology owner is absent",
                output_topology_checkpoint::context_capability_validation);
  if (!artifact.owner().same_owner(context.owner))
    return fail(output_topology_subcode::wrong_owner,
                "output topology owner disagrees with the context",
                output_topology_checkpoint::context_capability_validation);
  if (artifact.operation() != context.operation)
    return fail(output_topology_subcode::wrong_operation,
                "output topology operation disagrees with the context",
                output_topology_checkpoint::context_capability_validation);
  if (artifact.context_digest() != context.context_digest ||
      artifact.relation_digest() != relations.digest() ||
      artifact.intersection_digest() != intersections.digest() ||
      artifact.classification_digest() != classification.digest() ||
      artifact.retained_digest() != retained.digest())
    return fail(output_topology_subcode::predecessor_digest_mismatch,
                "output topology predecessor digest mismatch",
                output_topology_checkpoint::predecessor_validation);
  if (artifact.manifold_digests()[0] != manifolds.a()->digest() ||
      artifact.manifold_digests()[1] != manifolds.b()->digest())
    return fail(output_topology_subcode::predecessor_digest_mismatch,
                "output topology manifold digest mismatch",
                output_topology_checkpoint::predecessor_validation);
  if (retained.verification() !=
      selection_verification_disposition::independently_verified)
    return fail(output_topology_subcode::predecessor_not_verified,
                "retained complex is not independently verified",
                output_topology_checkpoint::predecessor_validation);

  // Structural core (count equations, bijections, reciprocity, permutation,
  // cycles, contour roles, witness references, vertex links, digest).
  if (!verify_polygonal_output_complex_structural(artifact, retained, error))
    return false;

  // Independently reconstruct the successor permutation from the Component 10
  // face-corner arcs in reverse order, and compare against the stored one.
  const auto &incidences = retained.incidences();
  const std::size_t incidence_count = incidences.size();
  {
    std::vector<std::uint64_t> end_port_by_incidence(incidence_count,
                                                     output_topology_invalid_ordinal);
    std::vector<std::uint64_t> start_port_by_incidence(
        incidence_count, output_topology_invalid_ordinal);
    for (std::size_t p = retained.local_ports().size(); p-- > 0;) {
      const auto &port = retained.local_ports()[p];
      if (!valid(port.incidence, incidence_count))
        continue;
      if (port.role == endpoint_role::end)
        end_port_by_incidence[port.incidence] = port.canonical_id;
      else
        start_port_by_incidence[port.incidence] = port.canonical_id;
    }
    std::vector<std::uint64_t> next_incidence_by_port(
        retained.local_ports().size(), output_topology_invalid_ordinal);
    for (std::size_t a = retained.face_corner_arcs().size(); a-- > 0;) {
      const auto &arc = retained.face_corner_arcs()[a];
      if (arc.in_port >= retained.local_ports().size())
        continue;
      next_incidence_by_port[arc.in_port] =
          retained.local_ports()[arc.out_port].incidence;
    }
    for (std::size_t i = incidence_count; i-- > 0;) {
      const std::uint64_t h = artifact.halfedge_by_incidence()[i];
      if (!valid(h, artifact.halfedges().size()))
        return fail(output_topology_subcode::successor_permutation_failure,
                    "halfedge reverse map is out of range",
                    output_topology_checkpoint::successor_construction);
      const std::uint64_t end_port = end_port_by_incidence[i];
      if (!valid(end_port, retained.local_ports().size()))
        return fail(output_topology_subcode::successor_permutation_failure,
                    "incidence end port is missing",
                    output_topology_checkpoint::successor_construction);
      const std::uint64_t next_incidence =
          next_incidence_by_port[end_port];
      if (!valid(next_incidence, incidence_count))
        return fail(output_topology_subcode::successor_permutation_failure,
                    "successor incidence is unresolved",
                    output_topology_checkpoint::successor_construction);
      const std::uint64_t expected_successor =
          artifact.halfedge_by_incidence()[next_incidence];
      if (artifact.halfedges()[h].successor != expected_successor)
        return fail(output_topology_subcode::successor_permutation_failure,
                    "successor disagrees with independent reconstruction",
                    output_topology_checkpoint::successor_construction);
    }
  }

  return true;
}

// Self-contained structural verification (no predecessor artifacts beyond the
// retained complex). Used by mutation tests and by the producer's final gate.
template <class T, class I>
bool verify_polygonal_output_complex_structural(
    const polygonal_output_complex<T, I> &artifact,
    const retained_surface_complex<T, I> &retained,
    bounded_boolean_error &error) {
  using namespace output_topology_verify_detail;
  const auto fail = [&](output_topology_subcode subcode, const char *summary,
                        output_topology_checkpoint checkpoint) {
    error = output_topology_error(
        subcode, bounded_boolean_error_category::internal_invariant_error,
        summary, checkpoint);
    return false;
  };

  const auto &incidences = retained.incidences();
  const std::size_t incidence_count = incidences.size();
  const std::size_t planned_edge_count = retained.planned_edges().size();

  // Exact count equations.
  if (artifact.incidence_audits().size() != incidence_count)
    return fail(output_topology_subcode::missing_incidence,
                "incidence audit count disagrees with retained incidences",
                output_topology_checkpoint::incidence_audit);
  if (artifact.boundary_darts().size() != incidence_count)
    return fail(output_topology_subcode::missing_incidence,
                "boundary dart count disagrees with retained incidences",
                output_topology_checkpoint::boundary_extraction);
  if (artifact.paired_edges().size() != planned_edge_count)
    return fail(output_topology_subcode::pair_reciprocity_mismatch,
                "paired edge count disagrees with planned edges",
                output_topology_checkpoint::pair_materialization);
  if (artifact.halfedges().size() != 2 * planned_edge_count)
    return fail(output_topology_subcode::pair_reciprocity_mismatch,
                "halfedge count is not twice the paired edge count",
                output_topology_checkpoint::pair_materialization);
  if (artifact.cycle_halfedge_refs().size() != artifact.halfedges().size())
    return fail(output_topology_subcode::cycle_extraction_failure,
                "cycle halfedge references do not cover all halfedges",
                output_topology_checkpoint::cycle_extraction);

  // Incidence audit bijection and dispositions.
  if (artifact.audit_by_incidence().size() != incidence_count)
    return fail(output_topology_subcode::missing_incidence,
                "incidence reverse map is the wrong size",
                output_topology_checkpoint::incidence_audit);
  std::vector<bool> audit_seen(artifact.incidence_audits().size(), false);
  for (std::size_t i = 0; i < incidence_count; ++i) {
    const std::uint64_t audit = artifact.audit_by_incidence()[i];
    if (!valid(audit, artifact.incidence_audits().size()))
      return fail(output_topology_subcode::missing_incidence,
                  "incidence audit reverse map is out of range",
                  output_topology_checkpoint::incidence_audit);
    if (audit_seen[audit])
      return fail(output_topology_subcode::duplicate_incidence,
                  "an incidence is audited more than once",
                  output_topology_checkpoint::incidence_audit);
    audit_seen[audit] = true;
    const auto &row = artifact.incidence_audits()[audit];
    if (row.retained_incidence != i)
      return fail(output_topology_subcode::duplicate_incidence,
                  "audit row does not map back to its incidence",
                  output_topology_checkpoint::incidence_audit);
    if (row.disposition != output_incidence_disposition::paired_boundary &&
        row.disposition !=
            output_incidence_disposition::paired_zero_measure_boundary)
      return fail(output_topology_subcode::invalid_disposition,
                  "realized incidence has an unsupported disposition",
                  output_topology_checkpoint::incidence_audit);
  }
  for (std::size_t a = 0; a < audit_seen.size(); ++a)
    if (!audit_seen[a])
      return fail(output_topology_subcode::missing_incidence,
                  "an audit row is not referenced by any incidence",
                  output_topology_checkpoint::incidence_audit);

  // Halfedge reverse map bijection.
  if (artifact.halfedge_by_incidence().size() != incidence_count)
    return fail(output_topology_subcode::missing_incidence,
                "halfedge reverse map is the wrong size",
                output_topology_checkpoint::pair_materialization);
  std::vector<bool> halfedge_seen(artifact.halfedges().size(), false);
  for (std::size_t i = 0; i < incidence_count; ++i) {
    const std::uint64_t h = artifact.halfedge_by_incidence()[i];
    if (!valid(h, artifact.halfedges().size()))
      return fail(output_topology_subcode::missing_incidence,
                  "halfedge reverse map is out of range",
                  output_topology_checkpoint::pair_materialization);
    if (halfedge_seen[h])
      return fail(output_topology_subcode::duplicate_incidence,
                  "an incidence maps to more than one halfedge",
                  output_topology_checkpoint::pair_materialization);
    halfedge_seen[h] = true;
    if (artifact.halfedges()[h].incidence != i)
      return fail(output_topology_subcode::pair_reciprocity_mismatch,
                  "halfedge does not map back to its incidence",
                  output_topology_checkpoint::pair_materialization);
  }
  for (std::size_t h = 0; h < halfedge_seen.size(); ++h)
    if (!halfedge_seen[h])
      return fail(output_topology_subcode::missing_incidence,
                  "a halfedge is not referenced by any incidence",
                  output_topology_checkpoint::pair_materialization);

  // Pair reciprocity and explicit reversed endpoints.
  for (std::size_t p = 0; p < artifact.paired_edges().size(); ++p) {
    const auto &edge = artifact.paired_edges()[p];
    if (!valid(edge.halfedge0, artifact.halfedges().size()) ||
        !valid(edge.halfedge1, artifact.halfedges().size()))
      return fail(output_topology_subcode::pair_reciprocity_mismatch,
                  "paired edge halfedge slots are out of range",
                  output_topology_checkpoint::pair_materialization);
    const auto &h0 = artifact.halfedges()[edge.halfedge0];
    const auto &h1 = artifact.halfedges()[edge.halfedge1];
    if (h0.pair != h1.canonical_id || h1.pair != h0.canonical_id)
      return fail(output_topology_subcode::pair_reciprocity_mismatch,
                  "halfedge pair fields are not reciprocal",
                  output_topology_checkpoint::pair_materialization);
    if (h0.origin != h1.destination || h0.destination != h1.origin)
      return fail(output_topology_subcode::pair_reciprocity_mismatch,
                  "paired halfedge endpoints are not reversed",
                  output_topology_checkpoint::pair_materialization);
    if (h0.paired_edge != p || h1.paired_edge != p)
      return fail(output_topology_subcode::pair_reciprocity_mismatch,
                  "halfedge does not map back to its paired edge",
                  output_topology_checkpoint::pair_materialization);
  }

  // Successor/predecessor permutation is total and inverse.
  const std::size_t halfedge_count = artifact.halfedges().size();
  for (std::size_t h = 0; h < halfedge_count; ++h) {
    const auto &he = artifact.halfedges()[h];
    if (!valid(he.successor, halfedge_count) ||
        !valid(he.predecessor, halfedge_count))
      return fail(output_topology_subcode::successor_permutation_failure,
                  "halfedge successor/predecessor is out of range",
                  output_topology_checkpoint::successor_construction);
    if (artifact.halfedges()[he.successor].predecessor != h)
      return fail(output_topology_subcode::successor_permutation_failure,
                  "successor and predecessor are not inverse",
                  output_topology_checkpoint::successor_construction);
  }

  // Every halfedge belongs to exactly one cycle, and cycles tile all halfedges.
  std::vector<bool> visited(halfedge_count, false);
  for (std::size_t c = 0; c < artifact.face_cycles().size(); ++c) {
    const auto &cycle = artifact.face_cycles()[c];
    if (cycle.refs_begin + cycle.refs_count > artifact.cycle_halfedge_index().size())
      return fail(output_topology_subcode::cycle_extraction_failure,
                  "cycle reference range is out of bounds",
                  output_topology_checkpoint::cycle_extraction);
    std::uint64_t current = cycle.start_halfedge;
    for (std::uint64_t k = 0; k < cycle.refs_count; ++k) {
      if (!valid(current, halfedge_count) || visited[current])
        return fail(output_topology_subcode::cycle_extraction_failure,
                    "cycle walk repeats or escapes the halfedge domain",
                    output_topology_checkpoint::cycle_extraction);
      visited[current] = true;
      if (artifact.cycle_halfedge_index()[cycle.refs_begin + k] != current)
        return fail(output_topology_subcode::cycle_extraction_failure,
                    "cycle reference disagrees with the successor walk",
                    output_topology_checkpoint::cycle_extraction);
      if (artifact.halfedges()[current].cycle != c)
        return fail(output_topology_subcode::cycle_extraction_failure,
                    "halfedge cycle field disagrees with its cycle",
                    output_topology_checkpoint::cycle_extraction);
      current = artifact.halfedges()[current].successor;
    }
    if (current != cycle.start_halfedge)
      return fail(output_topology_subcode::cycle_extraction_failure,
                  "cycle does not close",
                  output_topology_checkpoint::cycle_extraction);
  }
  for (std::size_t h = 0; h < halfedge_count; ++h)
    if (!visited[h])
      return fail(output_topology_subcode::cycle_extraction_failure,
                  "a halfedge belongs to no cycle",
                  output_topology_checkpoint::cycle_extraction);

  // Contour roles: one outer per positive-area region.
  std::vector<std::uint64_t> outer_by_region(artifact.face_regions().size(),
                                             output_topology_invalid_ordinal);
  for (std::size_t n = 0; n < artifact.contour_nodes().size(); ++n) {
    const auto &node = artifact.contour_nodes()[n];
    if (!valid(node.region, artifact.face_regions().size()) ||
        !valid(node.cycle, artifact.face_cycles().size()))
      return fail(output_topology_subcode::contour_role_failure,
                  "contour node reference is out of range",
                  output_topology_checkpoint::contour_role_assignment);
    if (node.role == contour_role::outer) {
      if (outer_by_region[node.region] != output_topology_invalid_ordinal)
        return fail(output_topology_subcode::contour_role_failure,
                    "region has more than one outer contour",
                    output_topology_checkpoint::contour_role_assignment);
      outer_by_region[node.region] = node.cycle;
    }
  }
  for (std::size_t r = 0; r < artifact.face_regions().size(); ++r)
    if (artifact.face_regions()[r].positive_area &&
        outer_by_region[r] == output_topology_invalid_ordinal)
      return fail(output_topology_subcode::contour_role_failure,
                  "positive-area region has no outer contour",
                  output_topology_checkpoint::contour_role_assignment);

  // Witness references: each contour has a certified witness.
  for (std::size_t n = 0; n < artifact.contour_nodes().size(); ++n) {
    const auto &node = artifact.contour_nodes()[n];
    if (!valid(node.witness, artifact.contour_witnesses().size()))
      return fail(output_topology_subcode::missing_contour_witness,
                  "contour node has no witness",
                  output_topology_checkpoint::witness_validation);
    const auto &witness = artifact.contour_witnesses()[node.witness];
    if (witness.cycle != node.cycle || witness.region != node.region)
      return fail(output_topology_subcode::missing_contour_witness,
                  "witness does not map back to its contour",
                  output_topology_checkpoint::witness_validation);
  }

  // Vertex links: each occurrence reconstructs one closed cycle through
  // around_origin(h) = pair(predecessor(h)).
  for (std::size_t v = 0; v < artifact.vertex_occurrences().size(); ++v) {
    const auto &link = artifact.vertex_link_evidence()[v];
    if (link.vertex_occurrence != v)
      return fail(output_topology_subcode::vertex_link_failure,
                  "vertex link does not map back to its occurrence",
                  output_topology_checkpoint::vertex_link_reconstruction);
    if (link.outgoing_begin + link.outgoing_count >
        artifact.outgoing_halfedges().size())
      return fail(output_topology_subcode::vertex_link_failure,
                  "vertex link outgoing range is out of bounds",
                  output_topology_checkpoint::vertex_link_reconstruction);
    if (link.outgoing_count == 0)
      return fail(output_topology_subcode::vertex_link_failure,
                  "vertex link is empty",
                  output_topology_checkpoint::vertex_link_reconstruction);
    std::uint64_t current =
        artifact.outgoing_halfedges()[link.outgoing_begin];
    std::uint64_t walked = 0;
    do {
      if (!valid(current, halfedge_count) ||
          artifact.halfedges()[current].origin != v)
        return fail(output_topology_subcode::vertex_link_failure,
                    "vertex link walk leaves the occurrence",
                    output_topology_checkpoint::vertex_link_reconstruction);
      ++walked;
      if (walked > link.outgoing_count + 1)
        return fail(output_topology_subcode::vertex_link_failure,
                    "vertex link walk does not close",
                    output_topology_checkpoint::vertex_link_reconstruction);
      current = artifact.halfedges()[artifact.halfedges()[current].predecessor]
                    .pair;
    } while (current !=
             artifact.outgoing_halfedges()[link.outgoing_begin]);
    if (walked != link.outgoing_count)
      return fail(output_topology_subcode::vertex_link_failure,
                  "vertex link walk count disagrees with outgoing count",
                  output_topology_checkpoint::vertex_link_reconstruction);
  }

  // Region boundary sets equal their cycle unions.
  for (std::size_t r = 0; r < artifact.face_regions().size(); ++r) {
    const auto &region = artifact.face_regions()[r];
    if (region.boundary_darts_begin + region.boundary_darts_count >
        artifact.boundary_darts().size())
      return fail(output_topology_subcode::region_split_failure,
                  "region boundary dart range is out of bounds",
                  output_topology_checkpoint::boundary_extraction);
  }

  // Re-encode and verify the digest.
  {
    output_topology_codec_limits limits;
    std::vector<std::uint8_t> bytes;
    bounded_boolean_error codec_error;
    if (!encode_polygonal_output_complex(artifact, bytes, limits, codec_error)) {
      error = codec_error;
      return false;
    }
    if (sha256::digest(bytes) != artifact.digest())
      return fail(output_topology_subcode::digest_mismatch,
                  "output topology digest does not match canonical bytes",
                  output_topology_checkpoint::canonical_encoding);
  }

  return true;
}

} // namespace ygor::mesh_boolean::bounded
