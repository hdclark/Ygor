#pragma once
#ifndef YGOR_MESHES_BOOLEAN_LOCAL_REFINEMENT_H_
#define YGOR_MESHES_BOOLEAN_LOCAL_REFINEMENT_H_
#include "YgorMeshesBooleanSymbolicRegistry.h"

namespace ygor {
namespace mesh_boolean {
constexpr std::uint64_t refined_facet_patches_type_tag = 0x5947425245463037ULL;
constexpr std::uint16_t refined_facet_patches_schema = 1;

enum class local_constraint_source : std::uint8_t {
  source_boundary,
  intersection,
  overlap_boundary
};
enum class local_point_location : std::uint8_t {
  source_vertex,
  source_edge_interior,
  facet_interior
};
enum class halfedge_direction : std::uint8_t { canonical, reverse };
enum class boundary_area_class : std::uint8_t { positive, negative, zero_area };
enum class face_extent : std::uint8_t { bounded, unbounded };
enum class cell_role : std::uint8_t { outside_source, source_domain };
enum class decomposition_kind : std::uint8_t {
  identity,
  polygon_with_holes,
  canonical_cut
};

template <class Id> struct facet_local_ref {
  context_owner_token owner;
  facet_id facet;
  Id id;
  bool operator==(const facet_local_ref &o) const {
    return owner == o.owner && facet == o.facet && id == o.id;
  }
  bool operator!=(const facet_local_ref &o) const { return !(*this == o); }
};
using local_vertex_ref = facet_local_ref<local_vertex_id>;
using local_atomic_edge_ref = facet_local_ref<local_atomic_edge_id>;
using local_halfedge_ref = facet_local_ref<local_halfedge_id>;
using local_boundary_walk_ref = facet_local_ref<local_boundary_walk_id>;
using local_face_ref = facet_local_ref<local_face_id>;
using local_patch_ref = facet_local_ref<local_patch_id>;

struct local_constraint_label {
  local_constraint_source source_kind = local_constraint_source::intersection;
  feature_ref source;
  std::optional<symbolic_curve_id> curve;
  std::optional<raw_event_id> overlap_region;
  orientation_parity direction = orientation_parity::agree;
  std::uint32_t multiplicity = 1;
  std::vector<construction_node_id> derivations;
};
int canonical_label_compare(const local_constraint_label &,
                            const local_constraint_label &) noexcept;
struct local_vertex {
  local_vertex_ref id;
  symbolic_vertex_id symbolic;
  exact_point2 projected;
  std::vector<local_halfedge_ref> outgoing;
};
struct local_point_incidence {
  local_vertex_ref vertex;
  local_point_location location = local_point_location::facet_interior;
  std::vector<feature_ref> sources;
  std::vector<raw_event_id> events;
  bool used_by_edge = false;
};
struct local_atomic_edge {
  local_atomic_edge_ref id;
  std::optional<shared_atomic_edge_id> shared_semantic_edge;
  local_vertex_ref lower, upper;
  std::optional<symbolic_curve_id> canonical_interval;
  std::vector<local_constraint_label> labels;
  bool source_boundary = false;
  bool artificial = false;
};
struct local_halfedge {
  local_halfedge_ref id, twin, next, previous;
  local_atomic_edge_ref edge;
  local_vertex_ref origin, destination;
  local_boundary_walk_ref walk;
  local_face_ref left_face;
  halfedge_direction direction = halfedge_direction::canonical;
};
struct local_boundary_walk {
  local_boundary_walk_ref id;
  std::vector<local_halfedge_ref> halfedges;
  exact_scalar signed_double_area;
  boundary_area_class area_class = boundary_area_class::zero_area;
  local_face_ref face;
};
struct local_face {
  local_face_ref id;
  face_extent extent = face_extent::bounded;
  cell_role role = cell_role::outside_source;
  std::vector<local_boundary_walk_ref> walks;
};
struct local_patch {
  local_patch_ref id;
  facet_id facet;
  shell_id shell;
  operand_id operand;
  local_face_ref parent_face;
  decomposition_kind decomposition = decomposition_kind::identity;
  local_boundary_walk_ref outer;
  std::vector<local_boundary_walk_ref> holes;
  exact_scalar signed_double_area;
};
struct source_edge_chain {
  edge_use_id source;
  std::vector<local_atomic_edge_ref> edges;
  bool forward = true;
};
struct constraint_chain {
  feature_ref source;
  std::vector<local_atomic_edge_ref> edges;
  orientation_parity direction = orientation_parity::agree;
  std::uint32_t multiplicity = 1;
};
struct local_coverage_certificate {
  exact_scalar source_double_area, patch_double_area;
  std::uint64_t vertices = 0, edges = 0, halfedges = 0, faces = 0,
                 components = 0, bounded_faces = 0, walks_positive = 0,
                 walks_negative = 0, walks_zero = 0, source_domain_faces = 0,
                 holes = 0, artificial_edges = 0;
};
struct local_refinement {
  facet_id facet;
  operand_id operand;
  shell_id shell;
  projection_axis projection = projection_axis::drop_z;
  std::vector<local_vertex> vertices;
  std::vector<local_point_incidence> point_incidences;
  std::vector<local_atomic_edge> edges;
  std::vector<local_halfedge> halfedges;
  std::vector<local_boundary_walk> walks;
  std::vector<local_face> faces;
  std::vector<local_patch> patches;
  std::vector<source_edge_chain> source_boundary;
  std::vector<constraint_chain> constraints;
  local_coverage_certificate certificate;
};
struct shared_atomic_edge {
  shared_atomic_edge_id id;
  symbolic_vertex_id lower, upper;
  std::vector<local_atomic_edge_ref> occurrences;
};
template <class T, class I> struct refined_facet_patches {
  context_owner_token owner;
  digest setup_digest, symbolic_digest, validated_digest, kernel_policy_digest,
      artifact_digest;
  std::shared_ptr<const published_artifact<symbolic_complex<T, I>>> symbolic;
  std::shared_ptr<const published_artifact<validated_operands<T, I>>> validated;
  std::shared_ptr<const construction_storage> constructions;
  std::vector<shared_atomic_edge> shared_edges;
  std::vector<local_refinement> facets;
  std::vector<std::uint8_t> canonical_bytes, artifact_bytes;
};

status_or<bool> register_local_refinement_verifier(verifier_registry &,
                                                   coordinate_tag, index_tag);
template <class T, class I>
status_or<
    std::shared_ptr<const published_artifact<refined_facet_patches<T, I>>>>
refine_source_facets(boolean_context<T, I> &);
#define YGOR_LOCAL_EXTERN(T, I)                                                \
  extern template status_or<                                                   \
      std::shared_ptr<const published_artifact<refined_facet_patches<T, I>>>>  \
  refine_source_facets(boolean_context<T, I> &)
YGOR_LOCAL_EXTERN(float, std::uint32_t);
YGOR_LOCAL_EXTERN(float, std::uint64_t);
YGOR_LOCAL_EXTERN(double, std::uint32_t);
YGOR_LOCAL_EXTERN(double, std::uint64_t);
#undef YGOR_LOCAL_EXTERN
} // namespace mesh_boolean
} // namespace ygor
#endif
