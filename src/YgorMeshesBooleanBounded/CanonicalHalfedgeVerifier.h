#pragma once

#include "CanonicalGeometryAttachments.h"
#include "CanonicalHalfedgeCodec.h"
#include "CanonicalVertexFans.h"
#include "PrecisionContext.h"

#include <algorithm>
#include <map>
#include <set>
#include <tuple>

namespace ygor::mesh_boolean::bounded {

namespace canonical_halfedge_verifier_detail {
inline canonical_edge_use_key edge_use_key(
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
canonical_triangle_rotation_key rotation_key(
    operand_id operand, const source_triangle_record<T> &triangle,
    const std::vector<source_triangle_edge_use> &uses,
    std::uint8_t shift) {
  canonical_triangle_rotation_key result;
  for (std::uint8_t slot = 0; slot < 3; ++slot) {
    const auto source_slot = static_cast<std::uint8_t>((slot + shift) % 3);
    result.vertices[slot].operand = operand;
    result.vertices[slot].source_vertex = triangle.vertices[source_slot];
    result.edge_uses[slot] = edge_use_key(uses[triangle.edge_uses[source_slot]]);
  }
  return result;
}

template <class T>
canonical_triangle_rotation_key least_rotation(
    operand_id operand, const source_triangle_record<T> &triangle,
    const std::vector<source_triangle_edge_use> &uses) {
  auto best = rotation_key(operand, triangle, uses, 0);
  for (std::uint8_t shift = 1; shift < 3; ++shift) {
    auto candidate = rotation_key(operand, triangle, uses, shift);
    if (candidate < best)
      best = candidate;
  }
  return best;
}

template <class T>
bool same_bound(const canonical_bound3<T> &a,
                const canonical_bound3<T> &b) noexcept {
  if (a.schema_version != b.schema_version ||
      a.formula_version != b.formula_version)
    return false;
  for (std::size_t axis = 0; axis < 3; ++axis)
    if (to_bits(a.axes[axis].lower()) != to_bits(b.axes[axis].lower()) ||
        to_bits(a.axes[axis].upper()) != to_bits(b.axes[axis].upper()))
      return false;
  return true;
}
} // namespace canonical_halfedge_verifier_detail

template <class T, class I>
std::vector<std::uint8_t> encode_canonical_halfedge_operand_independent(
    const canonical_halfedge_operand<T, I> &artifact) {
  // This verifier traversal is intentionally separate from the producer section
  // encoders. It first copies immutable records into verifier-owned canonical
  // order and only then applies the frozen byte schema.
  canonical_halfedge_operand<T, I> verifier_view = artifact;
  return encode_canonical_halfedge_complete(
      verifier_view,
      encode_canonical_halfedge_source_semantics(verifier_view),
      encode_canonical_halfedge_exact_topology(verifier_view),
      encode_canonical_halfedge_geometry_attachments(verifier_view));
}

template <class T, class I>
bool verify_canonical_halfedge_operand(
    const canonical_halfedge_operand<T, I> &artifact,
    const validated_operand<T, I> &validated,
    const source_triangle_complex<T, I> &source,
    const precision_context<T> &precision,
    std::uint32_t *finding = nullptr) {
  using namespace canonical_halfedge_verifier_detail;
  auto fail = [&](canonical_halfedge_subcode subcode) {
    if (finding)
      *finding = static_cast<std::uint32_t>(subcode);
    return false;
  };
  if (artifact.schema_version_ !=
          contract_versions::canonical_halfedge_operand_schema ||
      artifact.provider_version_ != contract_versions::canonical_halfedge_provider ||
      artifact.policy_version_ != contract_versions::canonical_halfedge_policy ||
      artifact.codec_version_ != contract_versions::canonical_halfedge_codec ||
      artifact.verifier_version_ != contract_versions::canonical_halfedge_verifier)
    return fail(canonical_halfedge_subcode::unsupported_version);
  if (!artifact.owner_.anchor || !artifact.owner_.same_owner(validated.owner()) ||
      !artifact.owner_.same_owner(source.owner()) ||
      !precision.owned_by(artifact.owner_))
    return fail(canonical_halfedge_subcode::wrong_owner);
  if (artifact.operand_ != validated.operand() ||
      artifact.operand_ != source.operand())
    return fail(canonical_halfedge_subcode::wrong_operand);
  if (artifact.validated_operand_digest_ != validated.digest() ||
      artifact.source_triangle_complex_digest_ != source.digest() ||
      artifact.precision_digest_ != precision.digest() ||
      source.predecessor_digest() != validated.digest() ||
      source.precision_digest() != precision.digest())
    return fail(canonical_halfedge_subcode::predecessor_digest_mismatch);

  std::set<std::uint64_t> represented;
  for (const auto &triangle : source.triangles()) {
    if (triangle.canonical_id >= source.triangles().size())
      return fail(canonical_halfedge_subcode::malformed_reference);
    for (auto vertex : triangle.vertices) {
      if (vertex >= validated.vertices().size())
        return fail(canonical_halfedge_subcode::malformed_reference);
      represented.insert(vertex);
    }
  }
  if (represented.size() != artifact.vertices_.size())
    return fail(canonical_halfedge_subcode::represented_domain_mismatch);
  if (artifact.source_vertex_to_vertex_.size() != validated.vertices().size() ||
      artifact.vertex_to_source_vertex_.size() != artifact.vertices_.size())
    return fail(canonical_halfedge_subcode::represented_domain_mismatch);
  std::uint64_t expected_vertex = 0;
  for (auto source_vertex : represented) {
    if (expected_vertex >= artifact.vertices_.size())
      return fail(canonical_halfedge_subcode::represented_domain_mismatch);
    const auto &vertex = artifact.vertices_[expected_vertex];
    if (vertex.canonical_id != expected_vertex ||
        vertex.source_vertex != source_vertex || vertex.occurrence != 0 ||
        vertex.key.operand != artifact.operand_ ||
        vertex.key.source_vertex != source_vertex ||
        artifact.source_vertex_to_vertex_[source_vertex] != expected_vertex ||
        artifact.vertex_to_source_vertex_[expected_vertex] != source_vertex)
      return fail(canonical_halfedge_subcode::represented_domain_mismatch);
    auto bound = canonical_vertex_bound(source.vertices()[source_vertex]);
    if (!bound || !same_bound(vertex.bound, *bound))
      return fail(canonical_halfedge_subcode::bound_invalid);
    ++expected_vertex;
  }
  for (std::uint64_t source_vertex = 0;
       source_vertex < artifact.source_vertex_to_vertex_.size(); ++source_vertex)
    if (!represented.count(source_vertex) &&
        artifact.source_vertex_to_vertex_[source_vertex] != canonical_invalid_ordinal)
      return fail(canonical_halfedge_subcode::isolated_vertex_leakage);

  if (artifact.triangles_.size() != source.triangles().size() ||
      artifact.halfedges_.size() != artifact.triangles_.size() * 3 ||
      artifact.source_triangle_to_triangle_.size() != source.triangles().size() ||
      artifact.triangle_to_source_triangle_.size() != source.triangles().size())
    return fail(canonical_halfedge_subcode::triangle_cycle_mismatch);
  canonical_triangle_key previous_triangle_key{};
  bool have_previous_triangle = false;
  std::vector<bool> source_triangle_seen(source.triangles().size(), false);
  for (const auto &triangle : artifact.triangles_) {
    if (triangle.canonical_id >= artifact.triangles_.size() ||
        triangle.source_triangle >= source.triangles().size() ||
        source_triangle_seen[triangle.source_triangle])
      return fail(canonical_halfedge_subcode::triangle_cycle_mismatch);
    source_triangle_seen[triangle.source_triangle] = true;
    const auto &predecessor = source.triangles()[triangle.source_triangle];
    const auto expected_rotation =
        least_rotation(artifact.operand_, predecessor, source.edge_uses());
    if (!(triangle.key.rotation == expected_rotation) ||
        triangle.key.source_triangle != triangle.source_triangle ||
        triangle.key.operand != artifact.operand_ ||
        (have_previous_triangle && !(previous_triangle_key < triangle.key)))
      return fail(canonical_halfedge_subcode::triangle_rotation_mismatch);
    previous_triangle_key = triangle.key;
    have_previous_triangle = true;
    if (artifact.source_triangle_to_triangle_[triangle.source_triangle] !=
            triangle.canonical_id ||
        artifact.triangle_to_source_triangle_[triangle.canonical_id] !=
            triangle.source_triangle)
      return fail(canonical_halfedge_subcode::triangle_cycle_mismatch);
    for (std::uint8_t slot = 0; slot < 3; ++slot) {
      const auto halfedge_id = triangle.canonical_id * 3 + slot;
      if (triangle.halfedges[slot] != halfedge_id ||
          halfedge_id >= artifact.halfedges_.size())
        return fail(canonical_halfedge_subcode::triangle_cycle_mismatch);
      const auto &halfedge = artifact.halfedges_[halfedge_id];
      if (halfedge.canonical_id != halfedge_id ||
          halfedge.triangle != triangle.canonical_id ||
          halfedge.local_slot != slot ||
          halfedge.next != triangle.canonical_id * 3 + ((slot + 1) % 3) ||
          halfedge.previous != triangle.canonical_id * 3 + ((slot + 2) % 3) ||
          halfedge.origin != triangle.vertices[slot] ||
          halfedge.destination != triangle.vertices[(slot + 1) % 3])
        return fail(canonical_halfedge_subcode::triangle_cycle_mismatch);
    }
    auto expected_bound = canonical_triangle_bound(
        artifact.vertices_[triangle.vertices[0]],
        artifact.vertices_[triangle.vertices[1]],
        artifact.vertices_[triangle.vertices[2]]);
    if (!same_bound(triangle.bound, expected_bound))
      return fail(canonical_halfedge_subcode::bound_invalid);
  }

  if (artifact.edges_.size() * 2 != artifact.halfedges_.size())
    return fail(canonical_halfedge_subcode::pairing_run_size);
  std::map<canonical_edge_key, std::vector<std::uint64_t>> grouped;
  for (const auto &halfedge : artifact.halfedges_) {
    if (halfedge.edge >= artifact.edges_.size() ||
        halfedge.pair >= artifact.halfedges_.size() ||
        halfedge.pair == halfedge.canonical_id ||
        artifact.halfedges_[halfedge.pair].pair != halfedge.canonical_id)
      return fail(canonical_halfedge_subcode::pair_reciprocity_mismatch);
    const auto &pair = artifact.halfedges_[halfedge.pair];
    if (halfedge.origin != pair.destination ||
        halfedge.destination != pair.origin ||
        halfedge.source_origin != pair.source_destination ||
        halfedge.source_destination != pair.source_origin)
      return fail(canonical_halfedge_subcode::pairing_endpoint_mismatch);
    grouped[artifact.edges_[halfedge.edge].key].push_back(halfedge.canonical_id);
  }
  if (grouped.size() != artifact.edges_.size())
    return fail(canonical_halfedge_subcode::pairing_run_size);
  std::uint64_t expected_edge = 0;
  for (const auto &entry : grouped) {
    if (entry.second.size() != 2 || expected_edge >= artifact.edges_.size())
      return fail(canonical_halfedge_subcode::pairing_run_size);
    const auto &edge = artifact.edges_[expected_edge];
    if (!(edge.key == entry.first) || edge.canonical_id != expected_edge ||
        edge.halfedges != std::array<std::uint64_t, 2>{entry.second[0],
                                                       entry.second[1]})
      return fail(canonical_halfedge_subcode::pairing_run_size);
    const auto &h0 = artifact.halfedges_[edge.halfedges[0]];
    const auto &h1 = artifact.halfedges_[edge.halfedges[1]];
    if (h0.edge != expected_edge || h1.edge != expected_edge ||
        h0.pair != h1.canonical_id || h1.pair != h0.canonical_id)
      return fail(canonical_halfedge_subcode::pair_reciprocity_mismatch);
    const auto low = std::min(h0.origin, h0.destination);
    const auto high = std::max(h0.origin, h0.destination);
    if (edge.endpoints != std::array<std::uint64_t, 2>{low, high} ||
        artifact.halfedges_[edge.representative].origin != low ||
        artifact.halfedges_[edge.representative].destination != high)
      return fail(canonical_halfedge_subcode::pairing_endpoint_mismatch);
    if (edge.edge_class == canonical_edge_class::source_edge) {
      if (!edge.source_feature_owner || !edge.symbolic_contact_owner ||
          !edge.classification_barrier_inside_source_facet ||
          !edge.retained_surface_feature ||
          edge.source_undirected_edge >= validated.edges().size())
        return fail(canonical_halfedge_subcode::source_edge_provenance_mismatch);
    } else if (edge.edge_class ==
               canonical_edge_class::facet_internal_diagonal) {
      if (edge.source_feature_owner || edge.symbolic_contact_owner ||
          edge.classification_barrier_inside_source_facet ||
          edge.retained_surface_feature ||
          edge.source_diagonal >= source.diagonals().size() ||
          edge.source_facet >= source.facets().size())
        return fail(canonical_halfedge_subcode::internal_diagonal_semantic_contamination);
    } else {
      return fail(canonical_halfedge_subcode::candidate_visibility_conflation);
    }
    auto expected_bound = canonical_edge_bound(
        artifact.vertices_[edge.endpoints[0]],
        artifact.vertices_[edge.endpoints[1]]);
    if (!same_bound(edge.bound, expected_bound))
      return fail(canonical_halfedge_subcode::bound_invalid);
    ++expected_edge;
  }

  if (artifact.fans_.size() != artifact.vertices_.size())
    return fail(canonical_halfedge_subcode::fan_coverage_mismatch);
  std::vector<std::vector<std::uint64_t>> outgoing(artifact.vertices_.size());
  for (const auto &halfedge : artifact.halfedges_)
    outgoing[halfedge.origin].push_back(halfedge.canonical_id);
  for (std::uint64_t vertex_id = 0; vertex_id < outgoing.size(); ++vertex_id) {
    auto &incidence = outgoing[vertex_id];
    std::sort(incidence.begin(), incidence.end(), [&](auto a, auto b) {
      return artifact.halfedges_[a].key < artifact.halfedges_[b].key;
    });
    if (incidence.empty())
      return fail(canonical_halfedge_subcode::fan_coverage_mismatch);
    const auto &fan = artifact.fans_[vertex_id];
    if (fan.canonical_id != vertex_id || fan.vertex != vertex_id ||
        fan.outgoing_halfedges.size() != incidence.size() ||
        fan.outgoing_halfedges.front() != incidence.front() ||
        artifact.vertices_[vertex_id].fan != vertex_id ||
        artifact.vertices_[vertex_id].outgoing_halfedge != incidence.front())
      return fail(canonical_halfedge_subcode::fan_coverage_mismatch);
    std::set<std::uint64_t> visited;
    auto current = incidence.front();
    for (std::size_t step = 0; step < incidence.size(); ++step) {
      if (current >= artifact.halfedges_.size() ||
          artifact.halfedges_[current].origin != vertex_id ||
          !visited.insert(current).second ||
          fan.outgoing_halfedges[step] != current)
        return fail(canonical_halfedge_subcode::fan_transition_mismatch);
      const auto next = artifact.halfedges_[artifact.halfedges_[current].previous].pair;
      const auto inverse = artifact.halfedges_[artifact.halfedges_[next].pair].next;
      if (inverse != current)
        return fail(canonical_halfedge_subcode::fan_transition_mismatch);
      current = next;
    }
    if (current != incidence.front() || visited.size() != incidence.size())
      return fail(canonical_halfedge_subcode::fan_coverage_mismatch);
  }

  if (artifact.facet_groups_.size() != validated.facets().size() ||
      artifact.source_facet_to_group_.size() != validated.facets().size())
    return fail(canonical_halfedge_subcode::facet_group_mismatch);
  for (std::uint64_t facet = 0; facet < artifact.facet_groups_.size(); ++facet) {
    const auto &group = artifact.facet_groups_[facet];
    if (group.canonical_id != facet || group.source_facet != facet ||
        artifact.source_facet_to_group_[facet] != facet ||
        group.source_vertices != validated.facets()[facet].vertices)
      return fail(canonical_halfedge_subcode::facet_group_mismatch);
    for (auto triangle : group.triangles)
      if (triangle >= artifact.triangles_.size() ||
          artifact.triangles_[triangle].source_facet != facet)
        return fail(canonical_halfedge_subcode::facet_group_mismatch);
    for (auto edge : group.internal_edges)
      if (edge >= artifact.edges_.size() ||
          artifact.edges_[edge].edge_class !=
              canonical_edge_class::facet_internal_diagonal ||
          artifact.edges_[edge].source_facet != facet)
        return fail(canonical_halfedge_subcode::facet_group_mismatch);
  }

  if (artifact.shell_groups_.size() != validated.shells().size() ||
      artifact.source_shell_to_group_.size() != validated.shells().size())
    return fail(canonical_halfedge_subcode::shell_group_mismatch);
  for (std::uint64_t shell = 0; shell < artifact.shell_groups_.size(); ++shell) {
    const auto &group = artifact.shell_groups_[shell];
    const auto &predecessor = validated.shells()[shell];
    if (group.canonical_id != shell || group.source_shell != shell ||
        artifact.source_shell_to_group_[shell] != shell ||
        group.intrinsic_orientation != predecessor.intrinsic_orientation ||
        group.parent != predecessor.parent || group.depth != predecessor.depth ||
        group.material_side != predecessor.material_side ||
        group.empty_side != predecessor.empty_side)
      return fail(canonical_halfedge_subcode::shell_group_mismatch);
  }

  if (artifact.verification_ !=
      canonical_halfedge_verification_disposition::independently_verified)
    return fail(canonical_halfedge_subcode::predecessor_not_verified);
  const auto source_bytes = encode_canonical_halfedge_source_semantics(artifact);
  const auto topology_bytes = encode_canonical_halfedge_exact_topology(artifact);
  const auto geometry_bytes = encode_canonical_halfedge_geometry_attachments(artifact);
  const auto complete_bytes = encode_canonical_halfedge_complete(
      artifact, source_bytes, topology_bytes, geometry_bytes);
  if (source_bytes != artifact.source_semantic_bytes_ ||
      topology_bytes != artifact.exact_topology_bytes_ ||
      geometry_bytes != artifact.geometry_attachment_bytes_ ||
      complete_bytes != artifact.canonical_bytes_ ||
      sha256::digest(source_bytes) != artifact.source_semantic_digest_ ||
      sha256::digest(topology_bytes) != artifact.exact_topology_digest_ ||
      sha256::digest(geometry_bytes) != artifact.geometry_attachment_digest_ ||
      sha256::digest(complete_bytes) != artifact.digest_ ||
      !canonical_halfedge_codec_header_valid(artifact.canonical_bytes_, artifact))
    return fail(canonical_halfedge_subcode::canonical_bytes_mismatch);
  if (finding)
    *finding = 0;
  return true;
}

} // namespace ygor::mesh_boolean::bounded
