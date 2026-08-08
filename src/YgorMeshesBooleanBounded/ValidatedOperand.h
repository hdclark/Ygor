#pragma once

#include "InputValidationTypes.h"
#include "PrecisionContext.h"

#include <array>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct normalized_position_record final {
  std::uint64_t source_position = 0;
  std::uint64_t source_facet = 0;
  std::uint64_t canonical_facet = 0;
  ring_position_action action = ring_position_action::retained;
  std::uint64_t retained_corner = 0;
};
struct validated_vertex_record final {
  std::uint64_t canonical_id = 0;
  std::array<std::uint64_t, 3> coordinate_bits{};
  std::uint64_t presentation_vertex = 0;
};
template <class T> struct bounded_source_vertex_evidence final {
  std::uint64_t vertex = 0;
  std::array<T, 3> lower{};
  std::array<T, 3> upper{};
  T radial_error = T(0);
};
struct validated_facet_record final {
  std::uint64_t canonical_id = 0;
  std::vector<std::uint64_t> vertices;
  std::vector<std::array<std::uint64_t, 3>> decomposition;
  std::array<std::uint64_t, 3> support_vertices{};
  std::array<long double, 4> support_plane{};
  std::uint8_t dropped_axis = 0;
  long double projected_area = 0;
  std::uint64_t shell = 0;
  std::uint64_t presentation_facet = 0;
};
struct directed_source_use_record final {
  std::uint64_t origin = 0, destination = 0, facet = 0, corner = 0,
                reciprocal = 0, undirected_edge = 0;
};
struct undirected_source_edge_record final {
  std::uint64_t canonical_id = 0, low = 0, high = 0;
  std::array<std::uint64_t, 2> uses{};
};
struct validated_vertex_link_record final {
  std::uint64_t vertex = 0;
  std::vector<std::uint64_t> cyclic_facets;
};
struct geometry_relation_record final {
  std::uint64_t facet_a = 0, triangle_a = 0, facet_b = 0, triangle_b = 0;
  validation_relation relation = validation_relation::definitely_disjoint;
  std::uint8_t shared_dimension = 0;
  bool uncertainty_separated = false;
};
struct shell_pair_record final {
  std::uint64_t shell_a = 0, shell_b = 0;
  shell_pair_relation relation = shell_pair_relation::definitely_disjoint;
};
struct edge_wedge_record final {
  std::uint64_t edge = 0;
  std::array<std::uint64_t, 2> facets{};
  bool locally_embedded = false;
};
struct vertex_star_record final {
  std::uint64_t vertex = 0, incident_facets = 0;
  bool closed_disk = false;
};
struct validated_shell_record final {
  std::uint64_t canonical_id = 0;
  std::vector<std::uint64_t> facets;
  shell_orientation intrinsic_orientation = shell_orientation::outward;
  std::int64_t parent = -1;
  std::uint32_t depth = 0;
  occupied_side material_side = occupied_side::negative;
  occupied_side empty_side = occupied_side::positive;
  long double signed_volume = 0;
  long double volume_uncertainty = 0;
};
struct input_validation_statistics final {
  std::uint64_t triangle_pairs = 0, relation_calls = 0, canonical_branches = 0,
                verifier_work = 0, persistent_bytes = 0;
};

template <class T, class I> class validated_operand final {
public:
  std::uint16_t schema_version() const noexcept { return schema_version_; }
  operand_id operand() const noexcept { return operand_; }
  const context_owner_token &owner() const noexcept { return owner_; }
  const bounded_boolean_digest &source_digest() const noexcept {
    return source_digest_;
  }
  const bounded_boolean_digest &precision_digest() const noexcept {
    return precision_digest_;
  }
  const bounded_boolean_digest &context_digest() const noexcept {
    return context_digest_;
  }
  input_certificate_disposition certificate() const noexcept {
    return certificate_;
  }
  const auto &vertices() const noexcept { return vertices_; }
  const auto &facets() const noexcept { return facets_; }
  const auto &bounded_vertices() const noexcept { return bounded_vertices_; }
  const auto &directed_uses() const noexcept { return directed_uses_; }
  const auto &edges() const noexcept { return edges_; }
  const auto &vertex_links() const noexcept { return vertex_links_; }
  const auto &shells() const noexcept { return shells_; }
  const auto &relations() const noexcept { return relations_; }
  const auto &shell_pairs() const noexcept { return shell_pairs_; }
  const auto &edge_wedges() const noexcept { return edge_wedges_; }
  const auto &vertex_stars() const noexcept { return vertex_stars_; }
  const auto &statistics() const noexcept { return statistics_; }
  const auto &presentation_normalization() const noexcept {
    return normalization_;
  }
  const auto &canonical_bytes() const noexcept { return canonical_bytes_; }
  const bounded_boolean_digest &digest() const noexcept { return digest_; }
  const auto &source_presentation_bytes() const noexcept {
    return source_presentation_bytes_;
  }
  const bounded_boolean_digest &source_presentation_digest() const noexcept {
    return source_presentation_digest_;
  }

private:
  std::uint16_t schema_version_ = contract_versions::validated_operand;
  operand_id operand_ = operand_id::a;
  context_owner_token owner_{};
  bounded_boolean_digest source_digest_{}, precision_digest_{}, context_digest_{};
  input_certificate_disposition certificate_ =
      input_certificate_disposition::topology_only_nonpublishable;
  std::vector<validated_vertex_record> vertices_;
  std::vector<validated_facet_record> facets_;
  std::vector<bounded_source_vertex_evidence<T>> bounded_vertices_;
  std::vector<directed_source_use_record> directed_uses_;
  std::vector<undirected_source_edge_record> edges_;
  std::vector<validated_vertex_link_record> vertex_links_;
  std::vector<validated_shell_record> shells_;
  std::vector<geometry_relation_record> relations_;
  std::vector<shell_pair_record> shell_pairs_;
  std::vector<edge_wedge_record> edge_wedges_;
  std::vector<vertex_star_record> vertex_stars_;
  input_validation_statistics statistics_{};
  std::vector<normalized_position_record> normalization_;
  std::vector<std::uint8_t> canonical_bytes_;
  bounded_boolean_digest digest_{};
  std::vector<std::uint8_t> source_presentation_bytes_;
  bounded_boolean_digest source_presentation_digest_{};
  template <class U, class J> friend class input_validation_builder;
  template <class U, class J>
  friend bool verify_validated_operand(const validated_operand<U, J> &,
                                       const immutable_source_mesh<U, J> &,
                                       const precision_context<U> &);
  friend struct validated_operand_test_access;
};

} // namespace ygor::mesh_boolean::bounded
