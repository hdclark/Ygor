#pragma once
#ifndef YGOR_MESHES_BOOLEAN_REALIZATION_H_
#define YGOR_MESHES_BOOLEAN_REALIZATION_H_

#include "YgorMeshesBooleanSelection.h"
#include <functional>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint64_t realized_boundary_type_tag = 0x5947425245413131ULL;
constexpr std::uint16_t realized_boundary_schema = 2;

enum class realization_edge_role : std::uint8_t {
  selected_edge,
  triangulation_diagonal,
  hole_bridge
};
enum class realization_obligation_kind : std::uint8_t {
  exact_target_equality,
  defining_relation,
  finite_coordinate,
  fixed_original_bits,
  distinct_vertices,
  selected_edge_order,
  triangle_orientation,
  shared_vertex_relation,
  shared_edge_relation,
  nonadjacent_disjointness,
  patch_embedding,
  vertex_link_order,
  edge_radial_order,
  patch_side_orientation,
  global_embedding
};
enum class realization_relation : std::uint8_t {
  finite,
  equal_bits,
  exact_equal,
  distinct,
  negative,
  positive,
  point,
  segment,
  disjoint,
  embedded
};

template <class T> struct realization_axis_candidate {
  candidate_value_id id;
  coordinate_bits<T> bits;
  std::uint32_t step_distance = 0, rank = 0;
  exact_scalar absolute_error;
};
template <class T> struct realization_axis_domain {
  realization_vertex_id vertex;
  std::uint8_t axis = 0;
  exact_scalar target;
  std::vector<realization_axis_candidate<T>> values;
};
template <class T> struct realization_vertex {
  realization_vertex_id id;
  selected_vertex_id selected;
  symbolic_vertex_id symbolic;
  exact_point3 exact_coordinate;
  digest exact_digest;
  std::vector<std::uint8_t> owner_free_semantic_key;
  std::vector<construction_node_id> derivations;
  std::vector<original_vertex_id> original_sources;
  std::optional<original_vertex_id> preserved_source;
  std::optional<std::array<coordinate_bits<T>, 3>> preserved_source_bits;
  std::array<coordinate_bits<T>, 3> accepted_bits;
  vec3<T> coordinate;
  std::array<std::uint32_t, 3> accepted_axis_rank{{0, 0, 0}};
  std::uint64_t accepted_point_rank = 0;
  std::vector<selected_edge_id> selected_edges;
  std::vector<selected_patch_id> patches;
  std::vector<realization_triangle_id> triangles;
  std::vector<realization_obligation_id> obligations;
};
struct realization_halfedge {
  realization_halfedge_id id;
  realization_triangle_id triangle;
  realization_vertex_id origin, destination;
  realization_halfedge_id next, previous;
  std::optional<realization_halfedge_id> twin;
  realization_edge_role role = realization_edge_role::triangulation_diagonal;
  std::optional<selected_edge_id> selected_edge;
};
struct realization_triangle {
  realization_triangle_id id;
  selected_patch_id patch;
  std::vector<std::uint8_t> owner_free_semantic_key;
  std::array<realization_vertex_id, 3> vertices;
  std::array<realization_halfedge_id, 3> halfedges;
  projection_axis projection = projection_axis::drop_z;
  exact_sign exact_orientation = exact_sign::positive;
};
struct realization_obligation {
  realization_obligation_id id;
  realization_obligation_kind kind =
      realization_obligation_kind::finite_coordinate;
  std::uint16_t version = 1;
  std::vector<realization_vertex_id> vertices;
  std::vector<realization_triangle_id> triangles;
  std::vector<selected_edge_id> selected_edges;
  std::vector<selected_patch_id> selected_patches;
  realization_relation expected = realization_relation::finite;
  realization_relation actual = realization_relation::finite;
  std::vector<std::uint8_t> witness;
  std::optional<defining_relation_id> defining_relation;
  realization_obligation() = default;
  realization_obligation(realization_obligation_id id_,
                         realization_obligation_kind kind_,
                         std::uint16_t version_,
                         std::vector<realization_vertex_id> vertices_,
                         std::vector<realization_triangle_id> triangles_,
                         std::vector<selected_edge_id> selected_edges_,
                         std::vector<selected_patch_id> selected_patches_,
                         realization_relation expected_,
                         realization_relation actual_,
                         std::vector<std::uint8_t> witness_,
                         std::optional<defining_relation_id> relation_ = {})
      : id(id_), kind(kind_), version(version_), vertices(std::move(vertices_)),
        triangles(std::move(triangles_)),
        selected_edges(std::move(selected_edges_)),
        selected_patches(std::move(selected_patches_)), expected(expected_),
        actual(actual_), witness(std::move(witness_)),
        defining_relation(relation_) {}
};
struct realization_domain_box {
  realization_triangle_id triangle;
  exact_point3 lower, upper;
};
struct realization_triangle_pair {
  realization_triangle_id lower, upper;
  bool operator==(const realization_triangle_pair &other) const {
    return lower == other.lower && upper == other.upper;
  }
  bool operator!=(const realization_triangle_pair &other) const {
    return !(*this == other);
  }
};
struct realization_constraint_component {
  realization_constraint_component_id id;
  std::vector<realization_vertex_id> variables;
  std::vector<realization_obligation_id> obligations;
  digest graph_digest;
};
struct realization_component_transcript {
  realization_constraint_component_id component;
  std::vector<std::uint64_t> accepted_ranks;
  std::vector<realization_obligation_id> rejected_prefix_witnesses;
  std::uint64_t visited_nodes = 0, complete_assignments = 0;
  digest transcript_digest;
};
struct realization_search_summary {
  digest domain_digest;
  std::uint64_t visited_nodes = 0, complete_assignments = 0;
  std::uint64_t pair_checks = 0;
  std::optional<candidate_assignment_id> accepted_assignment;
  bool nearest_passed = false, exhausted = false;
};
struct realization_certificate {
  realization_certificate_id id;
  std::uint16_t triangulation_version = 1, obligation_version = 1,
                solver_version = 1;
  std::uint64_t vertices = 0, triangles = 0, halfedges = 0, obligations = 0,
                 witnesses = 0, components = 0, pair_boxes = 0,
                 pair_candidates = 0;
  digest selected_digest, kernel_policy_digest, policy_digest,
       triangulation_digest, obligation_digest, assignment_digest,
       component_digest, pair_digest,
      semantic_digest;
};

template <class T, class I> struct realized_boundary {
  context_owner_token owner;
  digest setup_digest, selected_digest, kernel_policy_digest, policy_digest,
      artifact_digest;
  std::shared_ptr<const published_artifact<selected_exact_boundary<T, I>>>
      selected;
  std::shared_ptr<const published_artifact<symbolic_complex<T, I>>> symbolic;
  std::shared_ptr<const construction_storage> constructions;
  std::vector<realization_vertex<T>> vertices;
  std::vector<realization_axis_domain<T>> axis_domains;
  std::vector<realization_triangle> triangles;
  std::vector<realization_halfedge> halfedges;
  std::vector<realization_obligation> obligations;
  std::vector<realization_domain_box> pair_boxes;
  std::vector<realization_triangle_pair> pair_candidates;
  std::vector<realization_constraint_component> components;
  std::vector<realization_component_transcript> component_transcripts;
  realization_search_summary search;
  realization_certificate certificate;
  std::vector<std::uint8_t> canonical_bytes, artifact_bytes;
};

namespace detail {
struct realization_solver_variable {
  std::uint64_t id = 0, domain_size = 0;
};
struct realization_solver_constraint {
  std::uint64_t id = 0;
  std::vector<std::uint64_t> variables;
};
struct realization_solver_component_result {
  std::vector<std::uint64_t> variables, constraints, accepted_ranks,
      rejected_prefix_witnesses;
  std::uint64_t visited_nodes = 0, complete_assignments = 0;
};
struct realization_solver_result {
  bool accepted = false, limited = false;
  std::uint64_t visited_nodes = 0, complete_assignments = 0;
  std::vector<realization_solver_component_result> components;
};
using realization_constraint_evaluator = std::function<bool(
    std::uint64_t, const std::vector<std::pair<std::uint64_t, std::uint64_t>> &)>;
realization_solver_result solve_realization_constraint_components(
    std::vector<realization_solver_variable>,
    std::vector<realization_solver_constraint>,
    const realization_constraint_evaluator &, std::uint64_t node_limit,
    std::uint64_t component_limit = std::numeric_limits<std::uint64_t>::max(),
    std::uint64_t trail_limit = std::numeric_limits<std::uint64_t>::max(),
    const std::function<bool()> &cancelled = {});
std::vector<realization_triangle_pair> conservative_realization_triangle_pairs(
    const std::vector<realization_domain_box> &,
    std::uint64_t *overlap_checks = nullptr,
    std::uint64_t max_overlap_checks = std::numeric_limits<std::uint64_t>::max(),
    std::uint64_t max_candidates = std::numeric_limits<std::uint64_t>::max(),
    bool *limited = nullptr);
} // namespace detail

status_or<bool> register_geometry_realization_verifier(verifier_registry &,
                                                       coordinate_tag,
                                                       index_tag);
template <class T, class I>
status_or<std::shared_ptr<const published_artifact<realized_boundary<T, I>>>>
realize_selected_boundary(boolean_context<T, I> &);
template <class T, class I>
bool verify_realization_exact_substitution(const realized_boundary<T, I>&);
template <class T, class I>
bool verify_realization_constraint_evidence(const realized_boundary<T, I>&);
template <class T, class I>
status_or<bool> verify_realization_constraint_evidence_checked(
    const realized_boundary<T, I>&, resource_accountant *);
#define YGOR_REALIZATION_EXTERN(T, I)                                          \
  extern template status_or<                                                   \
      std::shared_ptr<const published_artifact<realized_boundary<T, I>>>>      \
  realize_selected_boundary(boolean_context<T, I> &)
YGOR_REALIZATION_EXTERN(float, std::uint32_t);
YGOR_REALIZATION_EXTERN(float, std::uint64_t);
YGOR_REALIZATION_EXTERN(double, std::uint32_t);
YGOR_REALIZATION_EXTERN(double, std::uint64_t);
#undef YGOR_REALIZATION_EXTERN

} // namespace mesh_boolean
} // namespace ygor
#endif
