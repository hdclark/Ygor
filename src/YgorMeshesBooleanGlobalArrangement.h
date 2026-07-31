#pragma once
#ifndef YGOR_MESHES_BOOLEAN_GLOBAL_ARRANGEMENT_H_
#define YGOR_MESHES_BOOLEAN_GLOBAL_ARRANGEMENT_H_
#include "YgorMeshesBooleanLocalRefinement.h"

namespace ygor { namespace mesh_boolean {
constexpr std::uint64_t arrangement_complex_type_tag = 0x5947424152523038ULL;
constexpr std::uint16_t arrangement_complex_schema = 2;

enum class global_edge_kind : std::uint8_t { source_edge, intersection_seam, coincidence_boundary, transparent_artificial };
enum class patch_plane_side : std::uint8_t { negative, positive };
enum class side_transition_kind : std::uint8_t { transparent, seam_sector, source_edge_sector, vertex_sector, sheet_crossing, coincidence_crossing, tangent_contact };
enum class local_map_kind : std::uint8_t { vertex, point_incidence, edge, halfedge, boundary_walk, face, patch, source_chain, constraint_chain, artificial_cut };

struct arrangement_vertex {
  global_vertex_id id;
  symbolic_vertex_id symbolic;
  std::vector<local_vertex_ref> local_occurrences;
  std::vector<global_atomic_edge_id> incident_edges;
  std::vector<vertex_occurrence_id> occurrences;
};
struct vertex_occurrence {
  vertex_occurrence_id id;
  global_vertex_id vertex;
  operand_id operand;
  shell_id shell;
  std::vector<local_vertex_ref> local_germs;
  std::vector<global_halfedge_id> incident_halfedges;
  std::vector<vertex_sector_id> link_regions;
};
struct global_atomic_edge {
  global_atomic_edge_id id;
  global_vertex_id lower, upper;
  global_edge_kind kind = global_edge_kind::source_edge;
  std::vector<local_atomic_edge_ref> local_occurrences;
  std::vector<symbolic_curve_id> curves;
};
struct source_sheet_member {
  source_sheet_member_id id;
  operand_id operand;
  shell_id shell;
  std::vector<facet_id> facets;
  std::vector<local_patch_ref> local_patches;
};
struct global_patch {
  global_patch_id id;
  exact_plane3 plane;
  exact_scalar projected_double_area;
  std::vector<global_vertex_id> outer;
  std::vector<std::vector<global_vertex_id>> holes;
  std::vector<sheet_use_id> uses;
};
struct sheet_patch_use {
  sheet_use_id id;
  source_sheet_member_id member;
  global_patch_id patch;
  operand_id operand;
  shell_id shell;
  facet_id facet;
  local_patch_ref local_patch;
  bool source_plane_agrees = true;
  patch_plane_side occupied_side = patch_plane_side::negative;
  std::vector<global_halfedge_id> boundary;
};
struct global_halfedge {
  global_halfedge_id id, next, previous, sheet_mate;
  sheet_use_id use;
  global_atomic_edge_id edge;
  global_vertex_id origin, destination;
  vertex_occurrence_id origin_occurrence, destination_occurrence;
  local_halfedge_ref local;
};
struct radial_layer { std::vector<sheet_use_id> uses; };
struct seam_sector {
  seam_sector_id id;
  seam_id seam;
  std::uint64_t lower_layer = 0, upper_layer = 0;
  std::vector<patch_side_id> incident_sides;
};
struct seam_record {
  seam_id id;
  global_atomic_edge_id edge;
  std::vector<sheet_use_id> incident_uses;
  std::vector<radial_layer> radial_layers;
  std::vector<seam_sector_id> sectors;
};
struct source_edge_sector {
  source_edge_sector_id id;
  global_atomic_edge_id edge;
  std::uint64_t lower_layer = 0, upper_layer = 0;
  std::vector<sheet_use_id> incident_uses;
};
enum class vertex_germ_kind : std::uint8_t { full_circle, semicircle, convex_arc, reflex_arc, wedge, terminal_contact };
struct directed_link_ray {
  link_ray_id id;
  exact_vector3 direction;
  link_ray_id antipode;
};
struct spherical_link_arc {
  link_arc_id id;
  vertex_occurrence_id occurrence;
  link_ray_id origin, destination;
  std::vector<sheet_use_id> layers;
};
struct vertex_sector {
  vertex_sector_id id;
  global_vertex_id vertex;
  vertex_occurrence_id occurrence;
  link_region_id region;
  vertex_germ_kind germ = vertex_germ_kind::wedge;
  std::vector<link_ray_id> boundary_rays;
  std::vector<link_arc_id> boundary_arcs;
  exact_vector3 witness_direction;
  std::vector<exact_sign> witness_evidence;
  std::vector<seam_sector_id> seam_continuations;
  std::vector<source_edge_sector_id> source_edge_continuations;
};
enum class probe_base_stratum_kind : std::uint8_t { universe, patch_side, seam_sector, source_edge_sector, vertex_sector };
struct probe_constraint { exact_plane3 plane; exact_sign required = exact_sign::positive; };
struct open_probe_descriptor {
  patch_side_id side;
  open_region_component_id component;
  probe_base_stratum_kind base_kind = probe_base_stratum_kind::universe;
  std::uint64_t base_id = 0;
  std::optional<symbolic_vertex_id> base_vertex;
  std::optional<exact_point3> exact_base;
  exact_vector3 direction;
  std::vector<probe_constraint> constraints;
  std::vector<exact_sign> evidence;
  std::uint16_t formula_version = 1;
};
struct coincident_group {
  coincident_group_id id;
  global_patch_id patch;
  std::vector<sheet_use_id> members;
};
struct patch_side {
  patch_side_id id;
  global_patch_id patch;
  patch_plane_side side = patch_plane_side::negative;
  open_region_component_id component;
};
struct side_transition {
  side_transition_id id;
  side_transition_kind kind = side_transition_kind::transparent;
  patch_side_id from, to;
  bool region_crossing = false;
  std::vector<sheet_use_id> uses;
  std::optional<coincident_group_id> coincidence;
};
struct local_entity_image {
  local_map_kind kind = local_map_kind::vertex;
  facet_id facet;
  std::uint64_t local_id = 0;
  std::vector<std::uint64_t> global_fragments;
  bool retained_incidence_only = false;
};
struct global_arrangement_certificate {
  std::uint64_t local_vertices = 0, local_edges = 0, local_halfedges = 0,
                local_walks = 0, local_faces = 0, local_patches = 0;
  std::uint64_t mate_pairs = 0, patch_cycles = 0, seam_sectors = 0,
                coincident_memberships = 0, side_transitions = 0;
  digest semantic_digest;
};

template<class T, class I> struct arrangement_complex {
  context_owner_token owner;
  digest setup_digest, refined_digest, symbolic_digest, validated_digest,
         kernel_policy_digest, artifact_digest;
  std::shared_ptr<const published_artifact<refined_facet_patches<T,I>>> refined;
  std::shared_ptr<const published_artifact<symbolic_complex<T,I>>> symbolic;
  std::shared_ptr<const published_artifact<validated_operands<T,I>>> validated;
  std::shared_ptr<const construction_storage> constructions;
  classification_strategy classification =
      classification_strategy::independent_patch_side_v1;
  std::vector<arrangement_vertex> vertices;
  std::vector<vertex_occurrence> vertex_occurrences;
  std::vector<global_atomic_edge> edges;
  std::vector<source_sheet_member> sheet_members;
  std::vector<global_patch> patches;
  std::vector<sheet_patch_use> sheet_uses;
  std::vector<global_halfedge> halfedges;
  std::vector<seam_record> seams;
  std::vector<seam_sector> seam_sectors;
  std::vector<source_edge_sector> source_edge_sectors;
  std::vector<directed_link_ray> link_rays;
  std::vector<spherical_link_arc> link_arcs;
  std::vector<vertex_sector> vertex_sectors;
  std::vector<coincident_group> coincident_groups;
  std::vector<patch_side> patch_sides;
  std::vector<side_transition> transitions;
  std::vector<open_probe_descriptor> probes;
  std::vector<local_entity_image> local_maps;
  global_arrangement_certificate certificate;
  std::vector<std::uint8_t> quotient_bytes, canonical_bytes, artifact_bytes;
};

status_or<bool> register_global_arrangement_verifier(verifier_registry&, coordinate_tag, index_tag);
template<class T,class I> status_or<std::shared_ptr<const published_artifact<arrangement_complex<T,I>>>> build_global_arrangement(boolean_context<T,I>&);
#define YGOR_ARRANGEMENT_EXTERN(T,I) extern template status_or<std::shared_ptr<const published_artifact<arrangement_complex<T,I>>>> build_global_arrangement(boolean_context<T,I>&)
YGOR_ARRANGEMENT_EXTERN(float,std::uint32_t);
YGOR_ARRANGEMENT_EXTERN(float,std::uint64_t);
YGOR_ARRANGEMENT_EXTERN(double,std::uint32_t);
YGOR_ARRANGEMENT_EXTERN(double,std::uint64_t);
#undef YGOR_ARRANGEMENT_EXTERN
} }
#endif
