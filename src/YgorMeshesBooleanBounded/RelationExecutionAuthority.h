#pragma once

#include "RelationExecutionAuthorityTypes.h"
#include "CandidateSourceEdgeRelations.h"
#include "EdgeFacetRelations.h"
#include "FacetFacetRelations.h"

#include <algorithm>
#include <vector>

namespace ygor::mesh_boolean::bounded {

namespace relation_execution_authority_detail {

template <class T, class I>
relation_feature_key candidate_edge_feature(
    const canonical_candidate_stream<T, I> &candidates,
    const canonical_candidate_record<T> &candidate) {
  const auto operand =
      candidate.role == directed_candidate_role::a_edge_b_triangle
          ? operand_id::a
          : operand_id::b;
  const auto &edge =
      candidates.primitive_table(operand).edges[candidate.edge.ordinal()];
  if (edge.edge_class == canonical_edge_class::source_edge)
    return candidate_source_edge_relation_detail::source_edge_feature(edge);
  relation_feature_key out;
  out.operand = operand;
  out.kind = relation_feature_kind::facet_internal_diagonal;
  out.primary = edge.source_facet;
  out.secondary = edge.source_diagonal;
  return out;
}

template <class T, class I>
relation_feature_key candidate_triangle_feature(
    const canonical_candidate_stream<T, I> &candidates,
    const canonical_candidate_record<T> &candidate) {
  const auto operand =
      candidate.role == directed_candidate_role::a_edge_b_triangle
          ? operand_id::b
          : operand_id::a;
  const auto &triangle =
      candidates.primitive_table(operand).triangles[candidate.triangle.ordinal()];
  relation_feature_key out;
  out.operand = operand;
  out.kind = relation_feature_kind::source_triangle;
  out.primary = triangle.source_triangle;
  out.secondary = triangle.source_facet;
  return out;
}

inline const canonical_relation_request *find_request(
    const relation_request_graph &graph,
    const relation_request_key &key) noexcept {
  const auto found = std::lower_bound(
      graph.requests.begin(), graph.requests.end(), key,
      [](const canonical_relation_request &request,
         const relation_request_key &candidate) { return request.key < candidate; });
  return found == graph.requests.end() || found->key != key ? nullptr : &*found;
}

} // namespace relation_execution_authority_detail

template <class T, class I>
boolean_outcome<relation_execution_authority> build_relation_execution_authority(
    const canonical_candidate_stream<T, I> &candidates,
    const bounded_boolean_digest &semantic_namespace,
    const relation_capabilities &capabilities) {
  std::vector<relation_request_proposal> edge_proposals;
  std::vector<relation_request_proposal> edge_facet_proposals;
  std::vector<relation_request_proposal> facet_proposals;
  bounded_boolean_error error;
  if (!candidate_source_edge_relation_detail::append_candidate_proposals(
          candidates, semantic_namespace, edge_proposals, capabilities, error) ||
      !candidate_source_edge_facet_detail::append_candidate_proposals(
          candidates, semantic_namespace, edge_facet_proposals, capabilities,
          error) ||
      !candidate_source_facet_relation_detail::append_candidate_proposals(
          candidates, semantic_namespace, facet_proposals, capabilities, error))
    return boolean_outcome<relation_execution_authority>::failure(error);

  std::vector<relation_request_proposal> proposals;
  proposals.reserve(edge_proposals.size() + edge_facet_proposals.size() +
                    facet_proposals.size() * 2 +
                    candidates.candidates().size());
  proposals.insert(proposals.end(), edge_proposals.begin(), edge_proposals.end());
  proposals.insert(proposals.end(), edge_facet_proposals.begin(),
                   edge_facet_proposals.end());

  for (const auto &candidate : candidates.candidates()) {
    relation_request_proposal bookkeeping;
    bookkeeping.key.semantic_namespace = semantic_namespace;
    bookkeeping.key.family = relation_request_family::source_edge_source_facet;
    bookkeeping.key.scope = relation_record_scope::bookkeeping_only;
    bookkeeping.key.first =
        relation_execution_authority_detail::candidate_edge_feature(candidates,
                                                                    candidate);
    bookkeeping.key.second =
        relation_execution_authority_detail::candidate_triangle_feature(
            candidates, candidate);
    bookkeeping.key.directed_use = candidate.id.ordinal();
    bookkeeping.key.formula_version = contract_versions::exact_relation_formulas;
    bookkeeping.key.policy_version = contract_versions::relation_request_key_schema;
    bookkeeping.candidate_witnesses.push_back(candidate.id);
    if (!valid_relation_request_key(bookkeeping.key))
      return boolean_outcome<relation_execution_authority>::failure(
          relation_error(relation_subcode::malformed_request_key,
                         bounded_boolean_error_category::internal_invariant_error,
                         "Component 07 triangle-local authority key is malformed",
                         relation_checkpoint::candidate_scan));
    proposals.push_back(std::move(bookkeeping));
  }

  for (auto &proposal : facet_proposals) {
    for (const auto witness : proposal.candidate_witnesses) {
      const auto &candidate = candidates.candidates()[witness.ordinal()];
      relation_request_key bookkeeping;
      bookkeeping.semantic_namespace = semantic_namespace;
      bookkeeping.family = relation_request_family::source_edge_source_facet;
      bookkeeping.scope = relation_record_scope::bookkeeping_only;
      bookkeeping.first =
          relation_execution_authority_detail::candidate_edge_feature(candidates,
                                                                      candidate);
      bookkeeping.second =
          relation_execution_authority_detail::candidate_triangle_feature(
              candidates, candidate);
      bookkeeping.directed_use = candidate.id.ordinal();
      proposal.dependencies.push_back(bookkeeping);
    }
    proposals.push_back(proposal);
    relation_request_proposal overlay;
    overlay.key = proposal.key;
    overlay.key.family = relation_request_family::coplanar_source_facet_overlay;
    overlay.dependencies.push_back(proposal.key);
    overlay.candidate_witnesses = proposal.candidate_witnesses;
    proposals.push_back(std::move(overlay));
  }

  auto graph = build_relation_request_graph(std::move(proposals), capabilities);
  if (!graph.has_value())
    return boolean_outcome<relation_execution_authority>::failure(*graph.error());
  relation_execution_authority authority;
  authority.owner = capabilities.owner;
  authority.graph = std::move(*graph.value());
  authority.closed_before_evaluation = true;
  bounded_boolean_error verification_error;
  authority.independently_verified =
      verify_relation_request_graph(authority.graph, verification_error);
  if (!authority.independently_verified)
    return boolean_outcome<relation_execution_authority>::failure(
        verification_error);
  authority.semantic_digest = sha256::digest(
      encode_relation_execution_authority_semantics(authority));
  return boolean_outcome<relation_execution_authority>::success(
      std::move(authority));
}

} // namespace ygor::mesh_boolean::bounded
