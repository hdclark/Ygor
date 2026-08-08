#pragma once

#include "CanonicalHalfedgeTypes.h"
#include "SourceTriangleComplex.h"
#include "ValidatedOperand.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct canonical_halfedge_test_access;

template <class T> struct canonical_manifold_vertex_record final {
  std::uint64_t canonical_id = 0;
  canonical_vertex_key key{};
  std::uint64_t source_vertex = 0;
  std::uint32_t occurrence = 0;
  std::uint64_t shell = 0;
  std::uint64_t shell_group = canonical_invalid_ordinal;
  std::array<std::uint64_t, 3> nominal_bits{};
  std::array<T, 3> committed_point{};
  std::array<T, 3> lower{};
  std::array<T, 3> upper{};
  T radial_error = T(0);
  source_geometry_basis_kind geometry_basis =
      source_geometry_basis_kind::nominal_embedded;
  std::uint64_t presentation_vertex = 0;
  std::uint64_t outgoing_halfedge = canonical_invalid_ordinal;
  std::uint64_t fan = canonical_invalid_ordinal;
  canonical_bound3<T> bound{};
};

template <class T> struct canonical_manifold_triangle_record final {
  std::uint64_t canonical_id = 0;
  canonical_triangle_key key{};
  std::uint64_t source_triangle = 0;
  std::uint64_t source_facet = 0;
  std::uint64_t ring = 0;
  std::uint64_t shell = 0;
  std::array<std::uint64_t, 3> vertices{};
  std::array<std::uint64_t, 3> source_vertices{};
  std::array<std::uint64_t, 3> halfedges{};
  std::uint64_t facet_group = canonical_invalid_ordinal;
  std::uint64_t shell_group = canonical_invalid_ordinal;
  facet_geometry_basis_ref basis{};
  source_orientation_evidence<T> orientation{};
  canonical_bound3<T> bound{};
};

struct canonical_manifold_halfedge_record final {
  std::uint64_t canonical_id = 0;
  canonical_halfedge_key key{};
  std::uint64_t triangle = 0;
  std::uint8_t local_slot = 0;
  std::uint64_t origin = 0;
  std::uint64_t destination = 0;
  std::uint64_t source_origin = 0;
  std::uint64_t source_destination = 0;
  std::uint64_t next = 0;
  std::uint64_t previous = 0;
  std::uint64_t pair = canonical_invalid_ordinal;
  std::uint64_t edge = canonical_invalid_ordinal;
  canonical_edge_class edge_class = canonical_edge_class::source_edge;
  std::uint64_t source_facet = 0;
  std::uint64_t ring = 0;
  std::uint64_t shell = 0;
  std::uint64_t source_directed_use = canonical_invalid_ordinal;
  std::uint64_t source_undirected_edge = canonical_invalid_ordinal;
  std::uint64_t source_corner = canonical_invalid_ordinal;
  std::uint64_t source_diagonal = canonical_invalid_ordinal;
  std::uint64_t predecessor_edge_use = canonical_invalid_ordinal;
};

template <class T> struct canonical_manifold_edge_record final {
  std::uint64_t canonical_id = 0;
  canonical_edge_key key{};
  canonical_edge_class edge_class = canonical_edge_class::source_edge;
  std::array<std::uint64_t, 2> endpoints{};
  std::array<std::uint64_t, 2> halfedges{};
  std::array<std::uint64_t, 2> triangles{};
  std::array<std::uint64_t, 2> facets{};
  std::uint64_t representative = 0;
  std::uint64_t source_undirected_edge = canonical_invalid_ordinal;
  std::array<std::uint64_t, 2> source_directed_uses{
      canonical_invalid_ordinal, canonical_invalid_ordinal};
  std::uint64_t source_facet = canonical_invalid_ordinal;
  std::uint64_t source_diagonal = canonical_invalid_ordinal;
  bool source_feature_owner = false;
  bool symbolic_contact_owner = false;
  bool classification_barrier_inside_source_facet = false;
  bool retained_surface_feature = false;
  canonical_bound3<T> bound{};

  bool remains_within_source_facet() const noexcept {
    return edge_class == canonical_edge_class::facet_internal_diagonal;
  }
};

struct canonical_vertex_fan_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t vertex = 0;
  std::vector<std::uint64_t> outgoing_halfedges;
};

template <class T> struct canonical_source_facet_group_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t source_facet = 0;
  std::uint64_t ring = 0;
  std::uint64_t shell = 0;
  std::vector<std::uint64_t> source_vertices;
  std::vector<std::uint64_t> vertices;
  std::vector<std::uint64_t> triangles;
  std::vector<std::uint64_t> internal_edges;
  std::vector<std::uint64_t> boundary_halfedges;
  facet_geometry_basis_ref basis{};
  canonical_bound3<T> bound{};
  bounded_boolean_digest source_semantic_digest{};
  bounded_boolean_digest exact_triangulation_digest{};
  bounded_boolean_digest digest{};
};

template <class T> struct canonical_source_shell_group_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t source_shell = 0;
  std::vector<std::uint64_t> vertices;
  std::vector<std::uint64_t> source_edges;
  std::vector<std::uint64_t> facets;
  std::vector<std::uint64_t> triangles;
  std::vector<std::uint64_t> edges;
  shell_orientation intrinsic_orientation = shell_orientation::outward;
  std::int64_t parent = -1;
  std::uint32_t depth = 0;
  occupied_side material_side = occupied_side::negative;
  occupied_side empty_side = occupied_side::positive;
  canonical_bound3<T> bound{};
  bounded_boolean_digest digest{};
};

template <class T, class I> class canonical_halfedge_operand final {
public:
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  std::uint16_t provider_version() const noexcept { return provider_version_; }
  std::uint16_t policy_version() const noexcept { return policy_version_; }
  std::uint16_t codec_version() const noexcept { return codec_version_; }
  std::uint16_t verifier_version() const noexcept { return verifier_version_; }
  operand_id operand() const noexcept { return operand_; }
  const context_owner_token &owner() const noexcept { return owner_; }
  canonical_halfedge_verification_disposition verification() const noexcept {
    return verification_;
  }
  const auto &vertices() const noexcept { return vertices_; }
  const auto &triangles() const noexcept { return triangles_; }
  const auto &halfedges() const noexcept { return halfedges_; }
  const auto &edges() const noexcept { return edges_; }
  const auto &fans() const noexcept { return fans_; }
  const auto &facet_groups() const noexcept { return facet_groups_; }
  const auto &shell_groups() const noexcept { return shell_groups_; }
  const auto &source_vertex_to_vertex() const noexcept {
    return source_vertex_to_vertex_;
  }
  const auto &vertex_to_source_vertex() const noexcept {
    return vertex_to_source_vertex_;
  }
  const auto &source_triangle_to_triangle() const noexcept {
    return source_triangle_to_triangle_;
  }
  const auto &triangle_to_source_triangle() const noexcept {
    return triangle_to_source_triangle_;
  }
  const auto &source_directed_use_to_halfedge() const noexcept {
    return source_directed_use_to_halfedge_;
  }
  const auto &source_edge_to_edge() const noexcept { return source_edge_to_edge_; }
  const auto &source_diagonal_to_edge() const noexcept {
    return source_diagonal_to_edge_;
  }
  const auto &source_facet_to_group() const noexcept {
    return source_facet_to_group_;
  }
  const auto &source_shell_to_group() const noexcept {
    return source_shell_to_group_;
  }
  const canonical_halfedge_statistics &statistics() const noexcept {
    return statistics_;
  }
  const std::vector<std::uint8_t> &source_semantic_bytes() const noexcept {
    return source_semantic_bytes_;
  }
  const std::vector<std::uint8_t> &exact_topology_bytes() const noexcept {
    return exact_topology_bytes_;
  }
  const std::vector<std::uint8_t> &geometry_attachment_bytes() const noexcept {
    return geometry_attachment_bytes_;
  }
  const std::vector<std::uint8_t> &canonical_bytes() const noexcept {
    return canonical_bytes_;
  }
  const bounded_boolean_digest &source_semantic_digest() const noexcept {
    return source_semantic_digest_;
  }
  const bounded_boolean_digest &exact_topology_digest() const noexcept {
    return exact_topology_digest_;
  }
  const bounded_boolean_digest &geometry_attachment_digest() const noexcept {
    return geometry_attachment_digest_;
  }
  const bounded_boolean_digest &precision_attachment_digest() const noexcept {
    return precision_attachment_digest_;
  }
  const bounded_boolean_digest &replay_presentation_digest() const noexcept {
    return replay_presentation_digest_;
  }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }

  const canonical_manifold_vertex_record<T> *vertex(
      manifold_vertex_id id, const context_owner_token &owner) const noexcept {
    if (!owner.same_owner(owner_) || id.ordinal() >= vertices_.size())
      return nullptr;
    return &vertices_[id.ordinal()];
  }
  const canonical_manifold_triangle_record<T> *triangle(
      manifold_triangle_id id, const context_owner_token &owner) const noexcept {
    if (!owner.same_owner(owner_) || id.ordinal() >= triangles_.size())
      return nullptr;
    return &triangles_[id.ordinal()];
  }
  const canonical_manifold_halfedge_record *halfedge(
      manifold_halfedge_id id, const context_owner_token &owner) const noexcept {
    if (!owner.same_owner(owner_) || id.ordinal() >= halfedges_.size())
      return nullptr;
    return &halfedges_[id.ordinal()];
  }
  const canonical_manifold_edge_record<T> *edge(
      manifold_edge_id id, const context_owner_token &owner) const noexcept {
    if (!owner.same_owner(owner_) || id.ordinal() >= edges_.size())
      return nullptr;
    return &edges_[id.ordinal()];
  }

private:
  std::uint16_t schema_version_ =
      contract_versions::canonical_halfedge_operand_schema;
  std::uint16_t provider_version_ = contract_versions::canonical_halfedge_provider;
  std::uint16_t policy_version_ = contract_versions::canonical_halfedge_policy;
  std::uint16_t codec_version_ = contract_versions::canonical_halfedge_codec;
  std::uint16_t verifier_version_ = contract_versions::canonical_halfedge_verifier;
  operand_id operand_ = operand_id::a;
  context_owner_token owner_{};
  canonical_halfedge_verification_disposition verification_ =
      canonical_halfedge_verification_disposition::unverified;
  bounded_boolean_digest validated_operand_digest_{};
  bounded_boolean_digest source_triangle_complex_digest_{};
  bounded_boolean_digest precision_digest_{};
  bounded_boolean_digest precision_attachment_digest_{};
  std::shared_ptr<const validated_operand<T, I>> validated_;
  std::shared_ptr<const source_triangle_complex<T, I>> source_triangles_;
  std::vector<canonical_manifold_vertex_record<T>> vertices_;
  std::vector<canonical_manifold_triangle_record<T>> triangles_;
  std::vector<canonical_manifold_halfedge_record> halfedges_;
  std::vector<canonical_manifold_edge_record<T>> edges_;
  std::vector<canonical_vertex_fan_record> fans_;
  std::vector<canonical_source_facet_group_record<T>> facet_groups_;
  std::vector<canonical_source_shell_group_record<T>> shell_groups_;
  std::vector<std::uint64_t> source_vertex_to_vertex_;
  std::vector<std::uint64_t> vertex_to_source_vertex_;
  std::vector<std::uint64_t> source_triangle_to_triangle_;
  std::vector<std::uint64_t> triangle_to_source_triangle_;
  std::vector<std::uint64_t> source_directed_use_to_halfedge_;
  std::vector<std::uint64_t> source_edge_to_edge_;
  std::vector<std::uint64_t> source_diagonal_to_edge_;
  std::vector<std::uint64_t> source_facet_to_group_;
  std::vector<std::uint64_t> source_shell_to_group_;
  canonical_halfedge_statistics statistics_{};
  std::vector<std::uint8_t> source_semantic_bytes_;
  std::vector<std::uint8_t> exact_topology_bytes_;
  std::vector<std::uint8_t> geometry_attachment_bytes_;
  std::vector<std::uint8_t> canonical_bytes_;
  bounded_boolean_digest source_semantic_digest_{};
  bounded_boolean_digest exact_topology_digest_{};
  bounded_boolean_digest geometry_attachment_digest_{};
  bounded_boolean_digest replay_presentation_digest_{};
  bounded_boolean_digest digest_{};

  template <class U, class J> friend class canonical_halfedge_builder;
  template <class U, class J>
  friend bool verify_canonical_halfedge_operand(
      const canonical_halfedge_operand<U, J> &, const validated_operand<U, J> &,
      const source_triangle_complex<U, J> &, const precision_context<U> &,
      std::uint32_t *);
  template <class U, class J>
  friend std::vector<std::uint8_t>
  encode_canonical_halfedge_source_semantics(
      const canonical_halfedge_operand<U, J> &);
  template <class U, class J>
  friend std::vector<std::uint8_t>
  encode_canonical_halfedge_exact_topology(
      const canonical_halfedge_operand<U, J> &);
  template <class U, class J>
  friend std::vector<std::uint8_t>
  encode_canonical_halfedge_geometry_attachments(
      const canonical_halfedge_operand<U, J> &);
  template <class U, class J>
  friend std::vector<std::uint8_t>
  encode_canonical_halfedge_complete(
      const canonical_halfedge_operand<U, J> &,
      const std::vector<std::uint8_t> &,
      const std::vector<std::uint8_t> &,
      const std::vector<std::uint8_t> &);
  template <class U, class J>
  friend std::vector<std::uint8_t>
  encode_canonical_halfedge_operand_independent(
      const canonical_halfedge_operand<U, J> &);
  friend struct canonical_halfedge_test_access;
};

} // namespace ygor::mesh_boolean::bounded
