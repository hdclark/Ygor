#pragma once

namespace ygor::mesh_boolean::bounded {

template <class T, class I>
bool canonical_halfedge_builder<T, I>::build_pairs_and_edges() {
  std::vector<canonical_pairing_proposal> proposals;
  proposals.reserve(artifact_->halfedges_.size());
  for (const auto &halfedge : artifact_->halfedges_) {
    if (halfedge.predecessor_edge_use >= source_->edge_uses().size())
      return fail(canonical_halfedge_subcode::malformed_reference,
                  bounded_boolean_error_category::internal_invariant_error,
                  "halfedge predecessor edge use is invalid",
                  canonical_halfedge_checkpoint::pairing_keys);
    const auto &use = source_->edge_uses()[halfedge.predecessor_edge_use];
    canonical_pairing_proposal proposal;
    proposal.pairing_key = canonical_pairing_key(artifact_->operand_, use);
    proposal.halfedge_key = halfedge.key;
    proposal.halfedge = halfedge.canonical_id;
    proposal.origin = halfedge.origin;
    proposal.destination = halfedge.destination;
    proposal.source_origin = halfedge.source_origin;
    proposal.source_destination = halfedge.source_destination;
    proposal.triangle = halfedge.triangle;
    proposal.local_slot = halfedge.local_slot;
    proposal.predecessor_edge_use = halfedge.predecessor_edge_use;
    proposal.facet = halfedge.source_facet;
    proposal.shell = halfedge.shell;
    proposal.source_directed_use = halfedge.source_directed_use;
    proposals.push_back(std::move(proposal));
  }
  std::sort(proposals.begin(), proposals.end());
  artifact_->source_edge_to_edge_.assign(validated_->edges().size(),
                                          canonical_invalid_ordinal);
  artifact_->source_diagonal_to_edge_.assign(source_->diagonals().size(),
                                              canonical_invalid_ordinal);
  artifact_->edges_.reserve(counts_.edges);

  std::size_t begin = 0;
  while (begin < proposals.size()) {
    std::size_t end = begin + 1;
    while (end < proposals.size() &&
           proposals[end].pairing_key == proposals[begin].pairing_key)
      ++end;
    if (end - begin != 2)
      return fail(canonical_halfedge_subcode::pairing_run_size,
                  bounded_boolean_error_category::internal_invariant_error,
                  "canonical edge pairing run does not contain exactly two uses",
                  canonical_halfedge_checkpoint::pairing_sort);
    const auto &p0 = proposals[begin];
    const auto &p1 = proposals[begin + 1];
    if (p0.halfedge == p1.halfedge || p0.origin != p1.destination ||
        p0.destination != p1.origin ||
        p0.source_origin != p1.source_destination ||
        p0.source_destination != p1.source_origin)
      return fail(canonical_halfedge_subcode::pairing_endpoint_mismatch,
                  bounded_boolean_error_category::internal_invariant_error,
                  "paired halfedges do not use reversed endpoints",
                  canonical_halfedge_checkpoint::pairing_sort);

    const auto edge_id = static_cast<std::uint64_t>(artifact_->edges_.size());
    const auto low_halfedge = std::min(p0.halfedge, p1.halfedge);
    const auto high_halfedge = std::max(p0.halfedge, p1.halfedge);
    auto &h0 = artifact_->halfedges_[low_halfedge];
    auto &h1 = artifact_->halfedges_[high_halfedge];
    h0.pair = high_halfedge;
    h1.pair = low_halfedge;
    h0.edge = edge_id;
    h1.edge = edge_id;

    canonical_manifold_edge_record<T> edge;
    edge.canonical_id = edge_id;
    edge.key = p0.pairing_key;
    edge.edge_class = p0.pairing_key.edge_class;
    edge.halfedges = {low_halfedge, high_halfedge};
    edge.triangles = {h0.triangle, h1.triangle};
    edge.facets = {h0.source_facet, h1.source_facet};
    edge.endpoints = {std::min(h0.origin, h0.destination),
                      std::max(h0.origin, h0.destination)};
    if (h0.origin == edge.endpoints[0] &&
        h0.destination == edge.endpoints[1])
      edge.representative = h0.canonical_id;
    else if (h1.origin == edge.endpoints[0] &&
             h1.destination == edge.endpoints[1])
      edge.representative = h1.canonical_id;
    else
      return fail(canonical_halfedge_subcode::pairing_endpoint_mismatch,
                  bounded_boolean_error_category::internal_invariant_error,
                  "canonical edge lacks low-to-high representative",
                  canonical_halfedge_checkpoint::edge_assignment);

    if (edge.edge_class == canonical_edge_class::source_edge) {
      const auto source_edge = edge.key.primary;
      if (source_edge >= validated_->edges().size() ||
          source_edge >= artifact_->source_edge_to_edge_.size() ||
          artifact_->source_edge_to_edge_[source_edge] !=
              canonical_invalid_ordinal ||
          h0.source_undirected_edge != source_edge ||
          h1.source_undirected_edge != source_edge ||
          h0.source_directed_use >= validated_->directed_uses().size() ||
          h1.source_directed_use >= validated_->directed_uses().size())
        return fail(canonical_halfedge_subcode::source_edge_provenance_mismatch,
                    bounded_boolean_error_category::internal_invariant_error,
                    "source-edge provenance is inconsistent",
                    canonical_halfedge_checkpoint::source_edge_validation);
      const auto &u0 = validated_->directed_uses()[h0.source_directed_use];
      const auto &u1 = validated_->directed_uses()[h1.source_directed_use];
      const auto &predecessor_edge = validated_->edges()[source_edge];
      if (u0.reciprocal != h1.source_directed_use ||
          u1.reciprocal != h0.source_directed_use ||
          u0.undirected_edge != source_edge ||
          u1.undirected_edge != source_edge ||
          u0.origin != h0.source_origin ||
          u0.destination != h0.source_destination ||
          u1.origin != h1.source_origin ||
          u1.destination != h1.source_destination ||
          !((predecessor_edge.uses[0] == h0.source_directed_use &&
             predecessor_edge.uses[1] == h1.source_directed_use) ||
            (predecessor_edge.uses[1] == h0.source_directed_use &&
             predecessor_edge.uses[0] == h1.source_directed_use)))
        return fail(canonical_halfedge_subcode::source_edge_provenance_mismatch,
                    bounded_boolean_error_category::internal_invariant_error,
                    "source-edge directed-use evidence is inconsistent",
                    canonical_halfedge_checkpoint::source_edge_validation);
      edge.source_undirected_edge = source_edge;
      edge.source_directed_uses = {h0.source_directed_use,
                                  h1.source_directed_use};
      edge.source_feature_owner = true;
      edge.symbolic_contact_owner = true;
      edge.classification_barrier_inside_source_facet = true;
      edge.retained_surface_feature = true;
      artifact_->source_edge_to_edge_[source_edge] = edge_id;
    } else if (edge.edge_class ==
               canonical_edge_class::facet_internal_diagonal) {
      const auto facet = edge.key.primary;
      const auto diagonal = edge.key.secondary;
      if (diagonal >= source_->diagonals().size() ||
          diagonal >= artifact_->source_diagonal_to_edge_.size() ||
          facet >= source_->facets().size() ||
          artifact_->source_diagonal_to_edge_[diagonal] !=
              canonical_invalid_ordinal ||
          h0.source_facet != facet || h1.source_facet != facet ||
          h0.source_diagonal != diagonal || h1.source_diagonal != diagonal)
        return fail(canonical_halfedge_subcode::diagonal_provenance_mismatch,
                    bounded_boolean_error_category::internal_invariant_error,
                    "facet-internal diagonal provenance is inconsistent",
                    canonical_halfedge_checkpoint::diagonal_validation);
      const auto &predecessor_diagonal = source_->diagonals()[diagonal];
      if (!((predecessor_diagonal.edge_uses[0] ==
                 h0.predecessor_edge_use &&
             predecessor_diagonal.edge_uses[1] ==
                 h1.predecessor_edge_use) ||
            (predecessor_diagonal.edge_uses[1] ==
                 h0.predecessor_edge_use &&
             predecessor_diagonal.edge_uses[0] ==
                 h1.predecessor_edge_use)) ||
          source_->edge_uses()[h0.predecessor_edge_use].opposite_edge_use !=
              h1.predecessor_edge_use ||
          source_->edge_uses()[h1.predecessor_edge_use].opposite_edge_use !=
              h0.predecessor_edge_use)
        return fail(canonical_halfedge_subcode::diagonal_provenance_mismatch,
                    bounded_boolean_error_category::internal_invariant_error,
                    "facet-internal diagonal reciprocal evidence is inconsistent",
                    canonical_halfedge_checkpoint::diagonal_validation);
      edge.source_facet = facet;
      edge.source_diagonal = diagonal;
      edge.source_feature_owner = false;
      edge.symbolic_contact_owner = false;
      edge.classification_barrier_inside_source_facet = false;
      edge.retained_surface_feature = false;
      artifact_->source_diagonal_to_edge_[diagonal] = edge_id;
    } else {
      return fail(canonical_halfedge_subcode::candidate_visibility_conflation,
                  bounded_boolean_error_category::internal_invariant_error,
                  "unknown canonical edge class",
                  canonical_halfedge_checkpoint::edge_assignment);
    }
    edge.bound = canonical_edge_bound(
        artifact_->vertices_[edge.endpoints[0]],
        artifact_->vertices_[edge.endpoints[1]]);
    artifact_->edges_.push_back(std::move(edge));
    begin = end;
  }
  if (artifact_->edges_.size() != counts_.edges)
    return fail(canonical_halfedge_subcode::pairing_run_size,
                bounded_boolean_error_category::internal_invariant_error,
                "canonical edge count differs from preflight",
                canonical_halfedge_checkpoint::edge_assignment);
  return check_cancel(canonical_halfedge_checkpoint::edge_assignment);
}

template <class T, class I>
bool canonical_halfedge_builder<T, I>::build_vertex_fans() {
  std::vector<std::vector<std::uint64_t>> outgoing(artifact_->vertices_.size());
  for (const auto &halfedge : artifact_->halfedges_) {
    if (halfedge.origin >= outgoing.size())
      return fail(canonical_halfedge_subcode::malformed_reference,
                  bounded_boolean_error_category::internal_invariant_error,
                  "halfedge origin is outside represented vertex domain",
                  canonical_halfedge_checkpoint::incidence_grouping);
    outgoing[halfedge.origin].push_back(halfedge.canonical_id);
  }
  artifact_->fans_.resize(outgoing.size());
  for (std::uint64_t vertex_id = 0; vertex_id < outgoing.size(); ++vertex_id) {
    auto &incidence = outgoing[vertex_id];
    std::sort(incidence.begin(), incidence.end(), [&](auto a, auto b) {
      return artifact_->halfedges_[a].key < artifact_->halfedges_[b].key;
    });
    if (incidence.empty())
      return fail(canonical_halfedge_subcode::fan_coverage_mismatch,
                  bounded_boolean_error_category::internal_invariant_error,
                  "represented vertex has no outgoing halfedge",
                  canonical_halfedge_checkpoint::incidence_grouping);
    auto &fan = artifact_->fans_[vertex_id];
    fan.canonical_id = vertex_id;
    fan.vertex = vertex_id;
    fan.outgoing_halfedges.reserve(incidence.size());
    std::set<std::uint64_t> visited;
    auto current = incidence.front();
    for (std::size_t step = 0; step < incidence.size(); ++step) {
      if (current >= artifact_->halfedges_.size() ||
          artifact_->halfedges_[current].origin != vertex_id ||
          !visited.insert(current).second)
        return fail(canonical_halfedge_subcode::fan_transition_mismatch,
                    bounded_boolean_error_category::internal_invariant_error,
                    "vertex fan transition repeated or left the vertex",
                    canonical_halfedge_checkpoint::fan_traversal);
      fan.outgoing_halfedges.push_back(current);
      const auto next = canonical_fan_next(artifact_->halfedges_, current);
      if (next >= artifact_->halfedges_.size() ||
          canonical_fan_previous(artifact_->halfedges_, next) != current)
        return fail(canonical_halfedge_subcode::fan_transition_mismatch,
                    bounded_boolean_error_category::internal_invariant_error,
                    "vertex fan transition is not bijective",
                    canonical_halfedge_checkpoint::fan_traversal);
      current = next;
    }
    if (current != incidence.front() || visited.size() != incidence.size())
      return fail(canonical_halfedge_subcode::fan_coverage_mismatch,
                  bounded_boolean_error_category::internal_invariant_error,
                  "vertex fan does not cover exactly one closed cycle",
                  canonical_halfedge_checkpoint::fan_traversal);
    artifact_->vertices_[vertex_id].outgoing_halfedge = incidence.front();
    artifact_->vertices_[vertex_id].fan = vertex_id;
  }
  return check_cancel(canonical_halfedge_checkpoint::fan_traversal);
}

} // namespace ygor::mesh_boolean::bounded

#include "CanonicalHalfedgeBuildFinalize.h"
