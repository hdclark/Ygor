#pragma once

#include "BroadPhaseTypes.h"
#include "CandidateDomainPolicy.h"
#include "CanonicalHalfedgeOperand.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T> struct broad_phase_edge_primitive final {
  broad_phase_edge_primitive_id id{0};
  std::uint64_t ordinal = 0;
  operand_id operand = operand_id::a;
  manifold_edge_id edge{0};
  canonical_edge_key semantic_key{};
  canonical_edge_class edge_class = canonical_edge_class::source_edge;
  manifold_halfedge_id representative{0};
  std::array<manifold_halfedge_id, 2> halfedges{
      manifold_halfedge_id{0}, manifold_halfedge_id{0}};
  std::array<manifold_vertex_id, 2> endpoints{
      manifold_vertex_id{0}, manifold_vertex_id{0}};
  std::array<manifold_triangle_id, 2> incident_triangles{
      manifold_triangle_id{0}, manifold_triangle_id{0}};
  std::array<std::uint64_t, 2> source_facets{};
  std::uint64_t source_undirected_edge = broad_phase_invalid_ordinal;
  std::array<std::uint64_t, 2> source_directed_uses{
      broad_phase_invalid_ordinal, broad_phase_invalid_ordinal};
  std::uint64_t source_facet = broad_phase_invalid_ordinal;
  std::uint64_t source_diagonal = broad_phase_invalid_ordinal;
  bool source_feature_owner = false;
  bool symbolic_contact_owner = false;
  bool classification_barrier_inside_source_facet = false;
  bool retained_surface_feature = false;
  canonical_bound3<T> bound{};
  std::array<std::array<floating_uint_t<T>, 2>, 3> endpoint_bits{};
  bounded_boolean_digest geometry_attachment_digest{};
  bounded_boolean_digest precision_attachment_digest{};
  candidate_domain_disposition inclusion = candidate_domain_disposition::included;
  std::uint32_t reserved = 0;
};

template <class T> struct broad_phase_triangle_primitive final {
  broad_phase_triangle_primitive_id id{0};
  std::uint64_t ordinal = 0;
  operand_id operand = operand_id::a;
  manifold_triangle_id triangle{0};
  canonical_triangle_key semantic_key{};
  std::array<manifold_vertex_id, 3> vertices{
      manifold_vertex_id{0}, manifold_vertex_id{0}, manifold_vertex_id{0}};
  std::array<manifold_halfedge_id, 3> halfedges{
      manifold_halfedge_id{0}, manifold_halfedge_id{0},
      manifold_halfedge_id{0}};
  std::uint64_t source_triangle = 0;
  std::uint64_t source_facet = 0;
  std::uint64_t ring = 0;
  std::uint64_t shell = 0;
  std::uint64_t facet_group = broad_phase_invalid_ordinal;
  std::uint64_t shell_group = broad_phase_invalid_ordinal;
  facet_geometry_basis_ref basis{};
  source_orientation_evidence<T> orientation{};
  canonical_bound3<T> bound{};
  std::array<axis_endpoint_key<T>, 3> axis_keys{};
  std::array<std::uint64_t, 3> dense_ranks{};
  rank_morton_key morton{};
  std::uint64_t spatial_ordinal = broad_phase_invalid_ordinal;
  bounded_boolean_digest geometry_attachment_digest{};
  bounded_boolean_digest precision_attachment_digest{};
  std::uint32_t reserved = 0;
};

template <class T> struct broad_phase_primitive_table final {
  operand_id operand = operand_id::a;
  std::vector<broad_phase_edge_primitive<T>> edges;
  std::vector<broad_phase_triangle_primitive<T>> triangles;
  bounded_boolean_digest predecessor_digest{};
  bounded_boolean_digest source_semantic_digest{};
  bounded_boolean_digest exact_topology_digest{};
  bounded_boolean_digest geometry_attachment_digest{};
  bounded_boolean_digest precision_attachment_digest{};
};

template <class T>
inline bool exact_bound_equal(const canonical_bound3<T> &a,
                              const canonical_bound3<T> &b) noexcept {
  if (a.schema_version != b.schema_version ||
      a.formula_version != b.formula_version)
    return false;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    if (to_bits(a.axes[axis].lower()) != to_bits(b.axes[axis].lower()) ||
        to_bits(a.axes[axis].upper()) != to_bits(b.axes[axis].upper()))
      return false;
  }
  return true;
}

template <class T>
inline std::array<std::array<floating_uint_t<T>, 2>, 3>
canonical_bound_endpoint_bits(const canonical_bound3<T> &bound) noexcept {
  std::array<std::array<floating_uint_t<T>, 2>, 3> out{};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    out[axis][0] = to_bits(bound.axes[axis].lower());
    out[axis][1] = to_bits(bound.axes[axis].upper());
  }
  return out;
}

template <class T, class I>
bool build_broad_phase_primitive_table(
    const canonical_halfedge_operand<T, I> &source,
    const context_owner_token &owner,
    broad_phase_primitive_table<T> &out,
    bounded_boolean_error &error) {
  if (!owner.anchor || !source.owner().same_owner(owner)) {
    error = broad_phase_error(broad_phase_subcode::wrong_owner,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase predecessor owner mismatch",
                              broad_phase_checkpoint::predecessor_validation);
    return false;
  }
  if (source.verification() !=
      canonical_halfedge_verification_disposition::independently_verified) {
    error = broad_phase_error(broad_phase_subcode::predecessor_not_verified,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase predecessor is not independently verified",
                              broad_phase_checkpoint::predecessor_validation);
    return false;
  }

  out = broad_phase_primitive_table<T>{};
  out.operand = source.operand();
  out.predecessor_digest = source.digest();
  out.source_semantic_digest = source.source_semantic_digest();
  out.exact_topology_digest = source.exact_topology_digest();
  out.geometry_attachment_digest = source.geometry_attachment_digest();
  out.precision_attachment_digest = source.precision_attachment_digest();

  try {
    out.edges.reserve(source.edges().size());
    out.triangles.reserve(source.triangles().size());
  } catch (...) {
    error = broad_phase_error(broad_phase_subcode::resource_preflight,
                              bounded_boolean_error_category::resource_limit,
                              "broad-phase primitive allocation failed",
                              broad_phase_checkpoint::fixed_resource_reservation);
    return false;
  }

  for (std::uint64_t ordinal = 0; ordinal < source.edges().size(); ++ordinal) {
    const auto *edge = source.edge(manifold_edge_id{ordinal}, owner);
    if (!edge || edge->canonical_id != ordinal || edge->key.operand != source.operand() ||
        edge->representative >= source.halfedges().size() ||
        edge->endpoints[0] >= source.vertices().size() ||
        edge->endpoints[1] >= source.vertices().size() ||
        edge->halfedges[0] >= source.halfedges().size() ||
        edge->halfedges[1] >= source.halfedges().size() ||
        edge->triangles[0] >= source.triangles().size() ||
        edge->triangles[1] >= source.triangles().size() ||
        edge->halfedges[0] == edge->halfedges[1] ||
        edge->endpoints[0] == edge->endpoints[1] || !edge->bound.valid()) {
      error = broad_phase_error(broad_phase_subcode::malformed_edge_primitive,
                                bounded_boolean_error_category::internal_invariant_error,
                                "malformed canonical edge primitive",
                                broad_phase_checkpoint::independent_bound_reconstruction);
      error.witnesses[0] = ordinal;
      error.witness_count = 1;
      return false;
    }
    const auto *representative =
        source.halfedge(manifold_halfedge_id{edge->representative}, owner);
    const auto *left_halfedge =
        source.halfedge(manifold_halfedge_id{edge->halfedges[0]}, owner);
    const auto *right_halfedge =
        source.halfedge(manifold_halfedge_id{edge->halfedges[1]}, owner);
    const auto *v0 = source.vertex(manifold_vertex_id{edge->endpoints[0]}, owner);
    const auto *v1 = source.vertex(manifold_vertex_id{edge->endpoints[1]}, owner);
    if (!representative || !left_halfedge || !right_halfedge || !v0 || !v1 ||
        representative->edge != ordinal || left_halfedge->pair != edge->halfedges[1] ||
        right_halfedge->pair != edge->halfedges[0]) {
      error = broad_phase_error(broad_phase_subcode::duplicate_edge_representative,
                                bounded_boolean_error_category::internal_invariant_error,
                                "canonical edge representative or reciprocal pair is malformed",
                                broad_phase_checkpoint::independent_bound_reconstruction);
      error.witnesses[0] = ordinal;
      error.witness_count = 1;
      return false;
    }
    const auto reconstructed = canonical_bound_hull(v0->bound, v1->bound);
    if (!reconstructed.valid() || !exact_bound_equal(reconstructed, edge->bound)) {
      error = broad_phase_error(broad_phase_subcode::bound_reconstruction_mismatch,
                                bounded_boolean_error_category::internal_invariant_error,
                                "canonical edge bound does not match endpoint hull",
                                broad_phase_checkpoint::independent_bound_reconstruction);
      error.witnesses[0] = ordinal;
      error.witness_count = 1;
      return false;
    }
    const auto decision = candidate_domain_v1(
        source.operand() == operand_id::a
            ? directed_candidate_role::a_edge_b_triangle
            : directed_candidate_role::b_edge_a_triangle,
        edge->edge_class);
    if (decision.disposition != candidate_domain_disposition::included ||
        decision.reason != topological_filter_reason::not_filtered) {
      error = broad_phase_error(broad_phase_subcode::domain_inclusion_mismatch,
                                bounded_boolean_error_category::internal_invariant_error,
                                "V1 broad-phase domain excluded a canonical edge",
                                broad_phase_checkpoint::provider_policy_validation);
      error.witnesses[0] = ordinal;
      error.witness_count = 1;
      return false;
    }
    if (edge->edge_class == canonical_edge_class::facet_internal_diagonal &&
        (edge->source_feature_owner || edge->symbolic_contact_owner ||
         edge->classification_barrier_inside_source_facet ||
         edge->retained_surface_feature ||
         edge->source_diagonal == canonical_invalid_ordinal ||
         edge->source_facet == canonical_invalid_ordinal)) {
      error = broad_phase_error(
          broad_phase_subcode::internal_diagonal_contamination,
          bounded_boolean_error_category::internal_invariant_error,
          "facet-internal diagonal has forbidden source-feature semantics",
          broad_phase_checkpoint::independent_bound_reconstruction);
      error.witnesses[0] = ordinal;
      error.witness_count = 1;
      return false;
    }

    broad_phase_edge_primitive<T> primitive;
    primitive.id = broad_phase_edge_primitive_id{ordinal};
    primitive.ordinal = ordinal;
    primitive.operand = source.operand();
    primitive.edge = manifold_edge_id{ordinal};
    primitive.semantic_key = edge->key;
    primitive.edge_class = edge->edge_class;
    primitive.representative = manifold_halfedge_id{edge->representative};
    primitive.halfedges = {manifold_halfedge_id{edge->halfedges[0]},
                           manifold_halfedge_id{edge->halfedges[1]}};
    primitive.endpoints = {manifold_vertex_id{edge->endpoints[0]},
                           manifold_vertex_id{edge->endpoints[1]}};
    primitive.incident_triangles = {
        manifold_triangle_id{edge->triangles[0]},
        manifold_triangle_id{edge->triangles[1]}};
    primitive.source_facets = edge->facets;
    primitive.source_undirected_edge = edge->source_undirected_edge;
    primitive.source_directed_uses = edge->source_directed_uses;
    primitive.source_facet = edge->source_facet;
    primitive.source_diagonal = edge->source_diagonal;
    primitive.source_feature_owner = edge->source_feature_owner;
    primitive.symbolic_contact_owner = edge->symbolic_contact_owner;
    primitive.classification_barrier_inside_source_facet =
        edge->classification_barrier_inside_source_facet;
    primitive.retained_surface_feature = edge->retained_surface_feature;
    primitive.bound = edge->bound;
    primitive.endpoint_bits = canonical_bound_endpoint_bits(edge->bound);
    primitive.geometry_attachment_digest = source.geometry_attachment_digest();
    primitive.precision_attachment_digest = source.precision_attachment_digest();
    primitive.inclusion = decision.disposition;
    out.edges.push_back(std::move(primitive));
  }

  for (std::uint64_t ordinal = 0; ordinal < source.triangles().size(); ++ordinal) {
    const auto *triangle = source.triangle(manifold_triangle_id{ordinal}, owner);
    if (!triangle || triangle->canonical_id != ordinal ||
        triangle->key.operand != source.operand() || !triangle->bound.valid()) {
      error = broad_phase_error(broad_phase_subcode::malformed_triangle_primitive,
                                bounded_boolean_error_category::internal_invariant_error,
                                "malformed canonical triangle primitive",
                                broad_phase_checkpoint::independent_bound_reconstruction);
      error.witnesses[0] = ordinal;
      error.witness_count = 1;
      return false;
    }
    canonical_bound3<T> reconstructed;
    bool initialized = false;
    for (std::size_t slot = 0; slot < 3; ++slot) {
      if (triangle->vertices[slot] >= source.vertices().size() ||
          triangle->halfedges[slot] >= source.halfedges().size()) {
        error = broad_phase_error(broad_phase_subcode::malformed_triangle_primitive,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "triangle primitive references an invalid feature",
                                  broad_phase_checkpoint::independent_bound_reconstruction);
        error.witnesses[0] = ordinal;
        error.witness_count = 1;
        return false;
      }
      const auto *vertex =
          source.vertex(manifold_vertex_id{triangle->vertices[slot]}, owner);
      if (!vertex || !vertex->bound.valid()) {
        error = broad_phase_error(broad_phase_subcode::malformed_bound,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "triangle vertex bound is invalid",
                                  broad_phase_checkpoint::independent_bound_reconstruction);
        error.witnesses[0] = ordinal;
        error.witness_count = 1;
        return false;
      }
      reconstructed = initialized ? canonical_bound_hull(reconstructed, vertex->bound)
                                  : vertex->bound;
      initialized = true;
    }
    if (!initialized || !reconstructed.valid() ||
        !exact_bound_equal(reconstructed, triangle->bound)) {
      error = broad_phase_error(broad_phase_subcode::bound_reconstruction_mismatch,
                                bounded_boolean_error_category::internal_invariant_error,
                                "canonical triangle bound does not match vertex hull",
                                broad_phase_checkpoint::independent_bound_reconstruction);
      error.witnesses[0] = ordinal;
      error.witness_count = 1;
      return false;
    }

    broad_phase_triangle_primitive<T> primitive;
    primitive.id = broad_phase_triangle_primitive_id{ordinal};
    primitive.ordinal = ordinal;
    primitive.operand = source.operand();
    primitive.triangle = manifold_triangle_id{ordinal};
    primitive.semantic_key = triangle->key;
    for (std::size_t slot = 0; slot < 3; ++slot) {
      primitive.vertices[slot] = manifold_vertex_id{triangle->vertices[slot]};
      primitive.halfedges[slot] = manifold_halfedge_id{triangle->halfedges[slot]};
    }
    primitive.source_triangle = triangle->source_triangle;
    primitive.source_facet = triangle->source_facet;
    primitive.ring = triangle->ring;
    primitive.shell = triangle->shell;
    primitive.facet_group = triangle->facet_group;
    primitive.shell_group = triangle->shell_group;
    primitive.basis = triangle->basis;
    primitive.orientation = triangle->orientation;
    primitive.bound = triangle->bound;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      primitive.axis_keys[axis].lower =
          finite_total_order_key(triangle->bound.axes[axis].lower());
      primitive.axis_keys[axis].upper =
          finite_total_order_key(triangle->bound.axes[axis].upper());
    }
    primitive.geometry_attachment_digest = source.geometry_attachment_digest();
    primitive.precision_attachment_digest = source.precision_attachment_digest();
    out.triangles.push_back(std::move(primitive));
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
