#pragma once

#include "CanonicalCandidateStream.h"
#include "SourceEdgeRelationKernel.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <limits>
#include <new>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct candidate_source_edge_relation_range final {
  candidate_id candidate{0};
  std::uint64_t begin = 0;
  std::uint64_t count = 0;
  std::uint32_t reserved = 0;
};

template <class T> struct candidate_source_edge_relation_stage final {
  std::uint16_t schema_version =
      contract_versions::relation_source_edge_edge_schema;
  std::uint16_t policy_version =
      contract_versions::relation_source_edge_edge_policy;
  context_owner_token owner{};
  relation_request_graph request_graph{};
  std::vector<source_edge_relation_record<T>> relations;
  std::vector<relation_request_id> candidate_relations;
  std::vector<candidate_source_edge_relation_range> candidate_ranges;
  std::uint64_t evaluation_count = 0;
  std::uint32_t reserved = 0;
  bounded_boolean_digest semantic_digest{};
};

inline bounded_boolean_error candidate_source_edge_relation_error(
    relation_subcode subcode, const char *summary,
    relation_checkpoint checkpoint = relation_checkpoint::edge_edge_evaluation,
    bounded_boolean_error_category category =
        bounded_boolean_error_category::internal_invariant_error) {
  return relation_error(subcode, category, summary, checkpoint);
}

namespace candidate_source_edge_relation_detail {

template <class T, class I>
const canonical_halfedge_operand<T, I> *operand_topology(
    const canonical_candidate_stream<T, I> &candidates,
    operand_id operand) noexcept {
  const auto &manifolds = candidates.manifolds();
  if (!manifolds)
    return nullptr;
  const auto &selected = operand == operand_id::a ? manifolds->a() : manifolds->b();
  return selected.get();
}

template <class T>
relation_feature_key source_edge_feature(
    const broad_phase_edge_primitive<T> &edge) noexcept {
  relation_feature_key feature;
  feature.operand = edge.operand;
  feature.kind = relation_feature_kind::source_edge;
  feature.primary = edge.semantic_key.primary;
  feature.secondary = edge.semantic_key.secondary;
  return feature;
}

template <class T>
bool valid_original_edge_primitive(
    const broad_phase_edge_primitive<T> &edge) noexcept {
  return edge.edge_class == canonical_edge_class::source_edge &&
         edge.semantic_key.operand == edge.operand &&
         edge.semantic_key.edge_class == canonical_edge_class::source_edge &&
         edge.source_feature_owner &&
         edge.source_undirected_edge != broad_phase_invalid_ordinal &&
         valid_relation_feature_key(source_edge_feature(edge));
}

template <class T>
bool import_vertex_point(const canonical_manifold_vertex_record<T> &vertex,
                         operand_id operand,
                         const context_owner_token &owner,
                         bounded_point3<T> &out) noexcept {
  if (!owner.anchor || vertex.key.operand != operand ||
      vertex.key.source_vertex != vertex.source_vertex ||
      vertex.canonical_id == canonical_invalid_ordinal ||
      !finite_bits(vertex.radial_error) || vertex.radial_error < T(0) ||
      !vertex.bound.valid())
    return false;

  std::uint64_t semantic_vertex = 0;
  std::uint64_t scalar_base = 0;
  if (!checked_multiply<std::uint64_t>(vertex.canonical_id, std::uint64_t{2},
                                       semantic_vertex) ||
      !checked_add<std::uint64_t>(semantic_vertex,
                                  static_cast<std::uint64_t>(operand),
                                  semantic_vertex) ||
      !checked_multiply<std::uint64_t>(semantic_vertex, std::uint64_t{4},
                                       scalar_base))
    return false;

  out = bounded_point3<T>{};
  out.owner = owner;
  out.coordinates.owner = owner;
  out.provenance = provenance_id(semantic_vertex);
  out.lineage = geometric_lineage_id(semantic_vertex);
  out.coordinates.radial_error_upper = vertex.radial_error;

  for (std::size_t axis = 0; axis < 3; ++axis) {
    const T nominal = from_bits<T>(vertex.nominal_bits[axis]);
    auto enclosure = finite_interval<T>::create(vertex.lower[axis],
                                                 vertex.upper[axis]);
    std::uint64_t value_id = 0;
    if (!enclosure || !enclosure->contains(nominal) ||
        to_bits(vertex.committed_point[axis]) != vertex.nominal_bits[axis] ||
        !checked_add<std::uint64_t>(scalar_base,
                                    static_cast<std::uint64_t>(axis),
                                    value_id))
      return false;

    auto &component = out.coordinates.components[axis];
    component.rounded_nominal = nominal;
    component.uncertainty_enclosure = *enclosure;
    component.identity.owner = owner;
    component.identity.value = bounded_value_id(value_id);
    component.identity.provenance = out.provenance;
    component.identity.lineage = out.lineage;
    component.identity.publication = bounded_publication_state::committed;
    const double inherited = static_cast<double>(vertex.radial_error);
    if (!std::isfinite(inherited) || inherited < 0.0)
      return false;
    if (operand == operand_id::a)
      component.contributors.inherited_a = inherited;
    else
      component.contributors.inherited_b = inherited;
  }
  return source_edge_relation_detail::valid_point(out, owner);
}

template <class T, class I>
bool make_edge_input(const canonical_candidate_stream<T, I> &candidates,
                     const relation_feature_key &feature,
                     source_edge_relation_input<T> &out) noexcept {
  if (!valid_relation_feature_key(feature) ||
      feature.kind != relation_feature_kind::source_edge)
    return false;
  const auto *topology = operand_topology(candidates, feature.operand);
  if (!topology || !topology->owner().same_owner(candidates.owner()))
    return false;

  const auto &table = candidates.primitive_table(feature.operand);
  const broad_phase_edge_primitive<T> *primitive = nullptr;
  for (const auto &candidate : table.edges) {
    if (source_edge_feature(candidate) == feature) {
      if (primitive)
        return false;
      primitive = &candidate;
    }
  }
  if (!primitive || !valid_original_edge_primitive(*primitive) ||
      primitive->endpoints[0].ordinal() >= topology->vertices().size() ||
      primitive->endpoints[1].ordinal() >= topology->vertices().size())
    return false;

  const auto *start = topology->vertex(primitive->endpoints[0], candidates.owner());
  const auto *end = topology->vertex(primitive->endpoints[1], candidates.owner());
  if (!start || !end)
    return false;

  out = source_edge_relation_input<T>{};
  out.feature = feature;
  out.original_source_edge = true;
  return import_vertex_point(*start, feature.operand, candidates.owner(),
                             out.start) &&
         import_vertex_point(*end, feature.operand, candidates.owner(), out.end);
}

template <class T, class I>
bool candidate_operands(const canonical_candidate_record<T> &candidate,
                        operand_id &edge_operand,
                        operand_id &triangle_operand) noexcept {
  switch (candidate.role) {
  case directed_candidate_role::a_edge_b_triangle:
    edge_operand = operand_id::a;
    triangle_operand = operand_id::b;
    return true;
  case directed_candidate_role::b_edge_a_triangle:
    edge_operand = operand_id::b;
    triangle_operand = operand_id::a;
    return true;
  }
  return false;
}

template <class T, class I>
bool append_candidate_proposals(
    const canonical_candidate_stream<T, I> &candidates,
    const bounded_boolean_digest &semantic_namespace,
    std::vector<relation_request_proposal> &proposals,
    const relation_capabilities &capabilities,
    bounded_boolean_error &error) {
  const auto &records = candidates.candidates();
  for (std::size_t ordinal = 0; ordinal < records.size(); ++ordinal) {
    if (relation_cancelled(capabilities)) {
      error = candidate_source_edge_relation_error(
          relation_subcode::cancelled,
          "Component 07 source-edge request discovery cancelled",
          relation_checkpoint::candidate_scan,
          bounded_boolean_error_category::cancelled);
      return false;
    }
    const auto &candidate = records[ordinal];
    operand_id edge_operand = operand_id::a;
    operand_id triangle_operand = operand_id::b;
    if (candidate.id.ordinal() != ordinal || candidate.ordinal != ordinal ||
        candidate.family !=
            broad_phase_relation_family::canonical_edge_source_triangle ||
        candidate.filter_reason != topological_filter_reason::not_filtered ||
        candidate.reserved != 0 ||
        !candidate_operands<T, I>(candidate, edge_operand, triangle_operand)) {
      error = candidate_source_edge_relation_error(
          relation_subcode::source_edge_relation_malformed,
          "Component 07 candidate source-edge request is malformed",
          relation_checkpoint::candidate_scan);
      return false;
    }

    const auto &edge_table = candidates.primitive_table(edge_operand);
    const auto &triangle_table = candidates.primitive_table(triangle_operand);
    if (candidate.edge.ordinal() >= edge_table.edges.size() ||
        candidate.triangle.ordinal() >= triangle_table.triangles.size()) {
      error = candidate_source_edge_relation_error(
          relation_subcode::source_edge_relation_malformed,
          "Component 07 candidate primitive reference is out of range",
          relation_checkpoint::candidate_scan);
      return false;
    }
    const auto &candidate_edge = edge_table.edges[candidate.edge.ordinal()];
    const auto &candidate_triangle =
        triangle_table.triangles[candidate.triangle.ordinal()];
    if (candidate_edge.id != candidate.edge ||
        candidate_triangle.id != candidate.triangle ||
        candidate_edge.operand != edge_operand ||
        candidate_triangle.operand != triangle_operand ||
        candidate.edge_class != candidate_edge.edge_class) {
      error = candidate_source_edge_relation_error(
          relation_subcode::source_edge_relation_malformed,
          "Component 07 candidate primitive handshake is inconsistent",
          relation_checkpoint::candidate_scan);
      return false;
    }

    if (candidate_edge.edge_class ==
        canonical_edge_class::facet_internal_diagonal)
      continue;
    if (!valid_original_edge_primitive(candidate_edge)) {
      error = candidate_source_edge_relation_error(
          relation_subcode::source_edge_relation_malformed,
          "Component 07 candidate source edge is not a public source feature",
          relation_checkpoint::candidate_scan);
      return false;
    }

    const auto *opposite = operand_topology(candidates, triangle_operand);
    if (!opposite ||
        candidate_triangle.triangle.ordinal() >= opposite->triangles().size()) {
      error = candidate_source_edge_relation_error(
          relation_subcode::source_edge_relation_malformed,
          "Component 07 opposite source triangle is unavailable",
          relation_checkpoint::candidate_scan);
      return false;
    }
    const auto *triangle = opposite->triangle(candidate_triangle.triangle,
                                               candidates.owner());
    if (!triangle || triangle->canonical_id !=
                         candidate_triangle.triangle.ordinal()) {
      error = candidate_source_edge_relation_error(
          relation_subcode::source_edge_relation_malformed,
          "Component 07 opposite source triangle handshake failed",
          relation_checkpoint::candidate_scan);
      return false;
    }

    for (const auto halfedge_id : triangle->halfedges) {
      if (halfedge_id >= opposite->halfedges().size()) {
        error = candidate_source_edge_relation_error(
            relation_subcode::source_edge_relation_malformed,
            "Component 07 opposite triangle halfedge is out of range",
            relation_checkpoint::candidate_scan);
        return false;
      }
      const auto *halfedge = opposite->halfedge(manifold_halfedge_id{halfedge_id},
                                                candidates.owner());
      if (!halfedge || halfedge->edge >= opposite->edges().size()) {
        error = candidate_source_edge_relation_error(
            relation_subcode::source_edge_relation_malformed,
            "Component 07 opposite triangle halfedge is malformed",
            relation_checkpoint::candidate_scan);
        return false;
      }
      const auto *edge = opposite->edge(manifold_edge_id{halfedge->edge},
                                        candidates.owner());
      if (!edge) {
        error = candidate_source_edge_relation_error(
            relation_subcode::source_edge_relation_malformed,
            "Component 07 opposite triangle edge is unavailable",
            relation_checkpoint::candidate_scan);
        return false;
      }
      if (edge->edge_class == canonical_edge_class::facet_internal_diagonal)
        continue;
      if (edge->canonical_id >= triangle_table.edges.size()) {
        error = candidate_source_edge_relation_error(
            relation_subcode::source_edge_relation_malformed,
            "Component 07 opposite source-edge primitive is out of range",
            relation_checkpoint::candidate_scan);
        return false;
      }
      const auto &opposite_edge = triangle_table.edges[edge->canonical_id];
      if (opposite_edge.edge.ordinal() != edge->canonical_id ||
          !valid_original_edge_primitive(opposite_edge)) {
        error = candidate_source_edge_relation_error(
            relation_subcode::source_edge_relation_malformed,
            "Component 07 opposite source-edge primitive handshake failed",
            relation_checkpoint::candidate_scan);
        return false;
      }

      relation_request_proposal proposal;
      proposal.key.semantic_namespace = semantic_namespace;
      proposal.key.family = relation_request_family::source_edge_source_edge;
      proposal.key.scope = relation_record_scope::public_source_feature;
      proposal.key.first = source_edge_feature(candidate_edge);
      proposal.key.second = source_edge_feature(opposite_edge);
      if (proposal.key.second < proposal.key.first)
        std::swap(proposal.key.first, proposal.key.second);
      proposal.key.formula_version = contract_versions::exact_relation_formulas;
      proposal.key.policy_version = contract_versions::relation_request_key_schema;
      proposal.candidate_witnesses.push_back(candidate.id);
      if (!valid_relation_request_key(proposal.key)) {
        error = candidate_source_edge_relation_error(
            relation_subcode::source_edge_relation_malformed,
            "Component 07 generated an invalid source-edge request key",
            relation_checkpoint::candidate_scan);
        return false;
      }
      proposals.push_back(std::move(proposal));
    }
  }
  return true;
}

template <class T>
bool relation_semantics_equal(const source_edge_relation_record<T> &a,
                              const source_edge_relation_record<T> &b) {
  return encode_source_edge_relation_semantics(a) ==
         encode_source_edge_relation_semantics(b);
}

} // namespace candidate_source_edge_relation_detail

template <class T>
std::vector<std::uint8_t> encode_candidate_source_edge_relation_semantics(
    const candidate_source_edge_relation_stage<T> &stage) {
  canonical_writer writer;
  writer.u32(0x37454943U); // CIE7
  writer.u16(stage.schema_version);
  writer.u16(stage.policy_version);
  writer.sized_bytes(encode_relation_request_graph_semantics(stage.request_graph));
  writer.u64(stage.relations.size());
  for (const auto &relation : stage.relations)
    writer.sized_bytes(encode_source_edge_relation_semantics(relation));
  writer.u64(stage.candidate_relations.size());
  for (const auto request : stage.candidate_relations)
    writer.u64(request.ordinal());
  writer.u64(stage.candidate_ranges.size());
  for (const auto &range : stage.candidate_ranges) {
    writer.u64(range.candidate.ordinal());
    writer.u64(range.begin);
    writer.u64(range.count);
    writer.u32(range.reserved);
  }
  writer.u64(stage.evaluation_count);
  writer.u32(stage.reserved);
  return writer.take();
}

template <class T, class I>
bool verify_candidate_source_edge_relation_stage(
    const canonical_candidate_stream<T, I> &candidates,
    const bounded_boolean_digest &semantic_namespace, T residual_boundary,
    const relation_capabilities &capabilities,
    const candidate_source_edge_relation_stage<T> &stage,
    bounded_boolean_error &error) {
  using namespace candidate_source_edge_relation_detail;
  const auto fail = [&](relation_subcode subcode, const char *summary) {
    error = candidate_source_edge_relation_error(
        subcode, summary, relation_checkpoint::producer_verification);
    return false;
  };
  if (stage.schema_version !=
          contract_versions::relation_source_edge_edge_schema ||
      stage.policy_version !=
          contract_versions::relation_source_edge_edge_policy ||
      stage.reserved != 0 || !stage.owner.same_owner(capabilities.owner) ||
      !stage.owner.same_owner(candidates.owner()) ||
      !stage.request_graph.owner.same_owner(stage.owner) ||
      stage.evaluation_count != stage.relations.size() ||
      stage.relations.size() != stage.request_graph.requests.size() ||
      stage.candidate_ranges.size() != candidates.candidates().size())
    return fail(relation_subcode::source_edge_relation_invariant,
                "Component 07 source-edge integration header is malformed");

  std::vector<relation_request_proposal> proposals;
  if (!append_candidate_proposals(candidates, semantic_namespace, proposals,
                                  capabilities, error))
    return false;
  auto reconstructed = build_relation_request_graph(std::move(proposals),
                                                     capabilities);
  if (!reconstructed.has_value()) {
    error = *reconstructed.error();
    return false;
  }
  if (encode_relation_request_graph_semantics(*reconstructed.value()) !=
      encode_relation_request_graph_semantics(stage.request_graph))
    return fail(relation_subcode::source_edge_relation_invariant,
                "Component 07 source-edge request graph did not reconstruct");

  for (std::size_t i = 0; i < stage.relations.size(); ++i) {
    const auto &request = stage.request_graph.requests[i];
    if (request.id.ordinal() != i ||
        request.key.family != relation_request_family::source_edge_source_edge ||
        request.key.scope != relation_record_scope::public_source_feature)
      return fail(relation_subcode::source_edge_relation_invariant,
                  "Component 07 source-edge producer request is malformed");
    source_edge_relation_input<T> first;
    source_edge_relation_input<T> second;
    if (!make_edge_input(candidates, request.key.first, first) ||
        !make_edge_input(candidates, request.key.second, second))
      return fail(relation_subcode::source_edge_relation_malformed,
                  "Component 07 source-edge verifier could not reconstruct inputs");
    auto reevaluated = classify_source_edge_relation(
        first, second, capabilities.owner, residual_boundary);
    if (!reevaluated.has_value()) {
      error = *reevaluated.error();
      return false;
    }
    if (!valid_source_edge_relation_record(stage.relations[i]) ||
        !relation_semantics_equal(stage.relations[i], *reevaluated.value()))
      return fail(relation_subcode::source_edge_relation_invariant,
                  "Component 07 source-edge numerical record did not reconstruct");
  }

  std::vector<std::vector<relation_request_id>> expected(
      candidates.candidates().size());
  for (const auto &request : stage.request_graph.requests) {
    if (request.witness_begin > stage.request_graph.candidate_witnesses.size() ||
        request.witness_count >
            stage.request_graph.candidate_witnesses.size() -
                request.witness_begin)
      return fail(relation_subcode::source_edge_relation_invariant,
                  "Component 07 source-edge witness range is malformed");
    for (std::uint64_t offset = 0; offset < request.witness_count; ++offset) {
      const auto witness = stage.request_graph.candidate_witnesses[
          request.witness_begin + offset];
      if (witness.ordinal() >= expected.size())
        return fail(relation_subcode::source_edge_relation_invariant,
                    "Component 07 source-edge witness is out of range");
      expected[witness.ordinal()].push_back(request.id);
    }
  }

  std::uint64_t expected_begin = 0;
  for (std::size_t candidate = 0; candidate < expected.size(); ++candidate) {
    auto &links = expected[candidate];
    std::sort(links.begin(), links.end());
    links.erase(std::unique(links.begin(), links.end()), links.end());
    const auto &range = stage.candidate_ranges[candidate];
    if (range.candidate.ordinal() != candidate || range.begin != expected_begin ||
        range.count != links.size() || range.reserved != 0 ||
        range.begin > stage.candidate_relations.size() ||
        range.count > stage.candidate_relations.size() - range.begin)
      return fail(relation_subcode::source_edge_relation_invariant,
                  "Component 07 candidate source-edge range is malformed");
    for (std::size_t offset = 0; offset < links.size(); ++offset)
      if (stage.candidate_relations[range.begin + offset] != links[offset])
        return fail(relation_subcode::source_edge_relation_invariant,
                    "Component 07 candidate source-edge coverage is incomplete");
    expected_begin += links.size();
  }
  if (expected_begin != stage.candidate_relations.size())
    return fail(relation_subcode::source_edge_relation_invariant,
                "Component 07 candidate source-edge links contain trailing data");

  if (stage.semantic_digest != sha256::digest(
          encode_candidate_source_edge_relation_semantics(stage)))
    return fail(relation_subcode::digest_mismatch,
                "Component 07 source-edge integration digest mismatch");

  auto owner_changed = stage;
  owner_changed.owner = context_owner_token::create();
  owner_changed.request_graph.owner = owner_changed.owner;
  if (encode_candidate_source_edge_relation_semantics(owner_changed) !=
      encode_candidate_source_edge_relation_semantics(stage))
    return fail(relation_subcode::owner_in_semantics,
                "Component 07 source-edge integration encoded a runtime owner");
  return true;
}

template <class T, class I>
boolean_outcome<candidate_source_edge_relation_stage<T>>
build_candidate_source_edge_relations(
    const canonical_candidate_stream<T, I> &candidates,
    const bounded_boolean_digest &semantic_namespace, T residual_boundary,
    const relation_capabilities &capabilities) {
  using stage_type = candidate_source_edge_relation_stage<T>;
  using namespace candidate_source_edge_relation_detail;
  static_assert(supported_precision_scalar_v<T>);
  try {
    if (!capabilities.owner.anchor ||
        !capabilities.owner.same_owner(candidates.owner()) ||
        candidates.verification() !=
            broad_phase_verification_disposition::independently_verified ||
        !finite_bits(residual_boundary) || residual_boundary < T(0))
      return boolean_outcome<stage_type>::failure(
          candidate_source_edge_relation_error(
              relation_subcode::source_edge_relation_malformed,
              "Component 07 source-edge integration handshake failed",
              relation_checkpoint::predecessor_validation));
    if (relation_cancelled(capabilities))
      return boolean_outcome<stage_type>::failure(
          candidate_source_edge_relation_error(
              relation_subcode::cancelled,
              "Component 07 source-edge integration cancelled",
              relation_checkpoint::candidate_scan,
              bounded_boolean_error_category::cancelled));

    std::vector<relation_request_proposal> proposals;
    std::uint64_t proposal_reserve = 0;
    if (!checked_multiply<std::uint64_t>(candidates.candidates().size(),
                                         std::uint64_t{3}, proposal_reserve) ||
        proposal_reserve > capabilities.maximum_requests ||
        proposal_reserve > capabilities.maximum_relations ||
        proposal_reserve > static_cast<std::uint64_t>(
                               std::numeric_limits<std::size_t>::max()))
      return boolean_outcome<stage_type>::failure(
          candidate_source_edge_relation_error(
              relation_subcode::work_limit,
              "Component 07 source-edge request bound exceeds capabilities",
              relation_checkpoint::count_representability_preflight,
              bounded_boolean_error_category::resource_limit));
    proposals.reserve(static_cast<std::size_t>(proposal_reserve));
    bounded_boolean_error discovery_error;
    if (!append_candidate_proposals(candidates, semantic_namespace, proposals,
                                    capabilities, discovery_error))
      return boolean_outcome<stage_type>::failure(discovery_error);

    auto graph = build_relation_request_graph(std::move(proposals), capabilities);
    if (!graph.has_value())
      return boolean_outcome<stage_type>::failure(*graph.error());
    if (graph.value()->requests.size() > capabilities.maximum_relations)
      return boolean_outcome<stage_type>::failure(
          candidate_source_edge_relation_error(
              relation_subcode::work_limit,
              "Component 07 source-edge relation limit exceeded",
              relation_checkpoint::count_representability_preflight,
              bounded_boolean_error_category::resource_limit));

    stage_type stage;
    stage.owner = capabilities.owner;
    stage.request_graph = std::move(*graph.value());
    stage.relations.reserve(stage.request_graph.requests.size());
    for (const auto &request : stage.request_graph.requests) {
      if (relation_cancelled(capabilities))
        return boolean_outcome<stage_type>::failure(
            candidate_source_edge_relation_error(
                relation_subcode::cancelled,
                "Component 07 source-edge evaluation cancelled",
                relation_checkpoint::edge_edge_evaluation,
                bounded_boolean_error_category::cancelled));
      source_edge_relation_input<T> first;
      source_edge_relation_input<T> second;
      if (!make_edge_input(candidates, request.key.first, first) ||
          !make_edge_input(candidates, request.key.second, second))
        return boolean_outcome<stage_type>::failure(
            candidate_source_edge_relation_error(
                relation_subcode::source_edge_relation_malformed,
                "Component 07 source-edge producer could not reconstruct inputs",
                relation_checkpoint::edge_edge_evaluation));
      auto relation = classify_source_edge_relation(
          first, second, capabilities.owner, residual_boundary);
      if (!relation.has_value())
        return boolean_outcome<stage_type>::failure(*relation.error());
      stage.relations.push_back(std::move(*relation.value()));
      ++stage.evaluation_count;
    }

    std::vector<std::vector<relation_request_id>> candidate_links(
        candidates.candidates().size());
    for (const auto &request : stage.request_graph.requests) {
      for (std::uint64_t offset = 0; offset < request.witness_count; ++offset) {
        const auto witness = stage.request_graph.candidate_witnesses[
            request.witness_begin + offset];
        if (witness.ordinal() >= candidate_links.size())
          return boolean_outcome<stage_type>::failure(
              candidate_source_edge_relation_error(
                  relation_subcode::source_edge_relation_invariant,
                  "Component 07 source-edge witness is out of range",
                  relation_checkpoint::event_seed_and_disposition_reconciliation));
        candidate_links[witness.ordinal()].push_back(request.id);
      }
    }
    stage.candidate_ranges.reserve(candidate_links.size());
    for (std::size_t candidate = 0; candidate < candidate_links.size();
         ++candidate) {
      auto &links = candidate_links[candidate];
      std::sort(links.begin(), links.end());
      links.erase(std::unique(links.begin(), links.end()), links.end());
      candidate_source_edge_relation_range range;
      range.candidate = candidate_id(candidate);
      range.begin = stage.candidate_relations.size();
      range.count = links.size();
      stage.candidate_relations.insert(stage.candidate_relations.end(),
                                       links.begin(), links.end());
      stage.candidate_ranges.push_back(range);
    }

    const auto semantic_bytes =
        encode_candidate_source_edge_relation_semantics(stage);
    if (semantic_bytes.size() > capabilities.maximum_canonical_bytes)
      return boolean_outcome<stage_type>::failure(
          candidate_source_edge_relation_error(
              relation_subcode::byte_count_overflow,
              "Component 07 source-edge integration bytes exceed capabilities",
              relation_checkpoint::canonical_encoding,
              bounded_boolean_error_category::resource_limit));
    stage.semantic_digest = sha256::digest(semantic_bytes);
    bounded_boolean_error verification_error;
    if (!verify_candidate_source_edge_relation_stage(
            candidates, semantic_namespace, residual_boundary, capabilities,
            stage, verification_error))
      return boolean_outcome<stage_type>::failure(verification_error);
    return boolean_outcome<stage_type>::success(std::move(stage));
  } catch (const std::bad_alloc &) {
    return boolean_outcome<stage_type>::failure(
        candidate_source_edge_relation_error(
            relation_subcode::resource_preflight,
            "Component 07 source-edge integration allocation failed",
            relation_checkpoint::discovery_resource_reservation,
            bounded_boolean_error_category::resource_limit));
  } catch (...) {
    return boolean_outcome<stage_type>::failure(
        candidate_source_edge_relation_error(
            relation_subcode::internal_invariant,
            "Component 07 source-edge integration raised an unexpected exception",
            relation_checkpoint::producer_verification));
  }
}

} // namespace ygor::mesh_boolean::bounded
