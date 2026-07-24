#pragma once
#ifndef YGOR_MESHES_BOOLEAN_SELECTION_H_
#define YGOR_MESHES_BOOLEAN_SELECTION_H_
#include "YgorMeshesBooleanCellClassification.h"

namespace ygor { namespace mesh_boolean {
constexpr std::uint64_t selected_exact_boundary_type_tag = 0x59474253454c3130ULL;
constexpr std::uint16_t selected_exact_boundary_schema = 3;
enum class patch_decision_kind : std::uint8_t { discard_exterior, discard_internal, select_preserved, select_reversed };
enum class selected_orientation : std::uint8_t { none, preserved, reversed };
enum class selected_boundary_topology : std::uint8_t {
  empty,
  closed_embedded_two_manifold,
  closed_stratified_nonmanifold
};
enum class topology_obstruction_kind : std::uint8_t {
  disconnected_geometric_vertex_link,
  multiple_edge_occurrences,
  noncircular_surface_link,
  nonembedded_stratum_contact
};

struct patch_selection_decision {
  patch_selection_decision_id id;
  global_patch_id patch;
  patch_side_label_id negative_label, positive_label;
  occupancy_pair negative_occupancy, positive_occupancy;
  bool result_negative = false, result_positive = false;
  patch_decision_kind kind = patch_decision_kind::discard_exterior;
  selected_orientation orientation = selected_orientation::none;
  std::optional<selected_patch_id> selected;
  std::optional<sheet_use_id> representative;
  std::vector<sheet_use_id> provenance;
};
struct selected_vertex { selected_vertex_id id; global_vertex_id source; symbolic_vertex_id symbolic; };
struct selected_vertex_occurrence {
  selected_vertex_occurrence_id id;
  selected_vertex_id vertex;
  std::vector<selected_halfedge_id> incident_halfedges;
};
struct selected_edge { selected_edge_id id; global_atomic_edge_id source; selected_vertex_id lower, upper; std::vector<selected_halfedge_id> uses; };
struct selected_halfedge {
  selected_halfedge_id id;
  selected_patch_id patch;
  selected_cycle_id cycle;
  selected_edge_id edge;
  selected_vertex_id origin, destination;
  selected_vertex_occurrence_id origin_occurrence, destination_occurrence;
  selected_halfedge_id next, previous;
};
struct selected_cycle { selected_cycle_id id; selected_patch_id patch; bool hole = false; std::vector<selected_halfedge_id> halfedges; };
struct selected_patch { selected_patch_id id; global_patch_id source; selected_orientation orientation = selected_orientation::preserved; std::vector<selected_cycle_id> cycles; sheet_use_id representative; std::vector<sheet_use_id> provenance; };
struct topology_obstruction {
  topology_obstruction_id id;
  topology_obstruction_kind kind =
      topology_obstruction_kind::disconnected_geometric_vertex_link;
  std::optional<selected_vertex_id> vertex;
  std::optional<global_atomic_edge_id> edge;
  std::vector<std::uint64_t> occurrences;
};
struct boolean_selection_certificate {
  selection_certificate_id id;
  std::uint64_t decisions = 0, discard_exterior = 0, discard_internal = 0,
                select_preserved = 0, select_reversed = 0,
                selected_patches = 0, selected_cycles = 0,
                selected_halfedges = 0, selected_edges = 0,
                 selected_vertices = 0, selected_vertex_occurrences = 0,
                 topology_obstructions = 0, provenance_uses = 0,
                 connected_components = 0;
  selected_boundary_topology topology = selected_boundary_topology::empty;
  digest semantic_digest;
};

template<class T,class I> struct selected_exact_boundary {
  context_owner_token owner;
  operation selected_operation = operation::regularized_union;
  std::optional<context_preparation_provenance> preparation_provenance;
  digest setup_digest, labeled_digest, arrangement_digest, artifact_digest;
  std::shared_ptr<const published_artifact<labeled_arrangement<T,I>>> labeled;
  std::shared_ptr<const published_artifact<arrangement_complex<T,I>>> arrangement;
  std::shared_ptr<const construction_storage> constructions;
  std::vector<patch_selection_decision> decisions;
  std::vector<selected_vertex> vertices;
  std::vector<selected_vertex_occurrence> vertex_occurrences;
  std::vector<selected_edge> edges;
  std::vector<selected_halfedge> halfedges;
  std::vector<selected_cycle> cycles;
  std::vector<selected_patch> patches;
  selected_boundary_topology topology = selected_boundary_topology::empty;
  std::vector<topology_obstruction> topology_obstructions;
  boolean_selection_certificate certificate;
  std::vector<std::uint8_t> canonical_bytes, artifact_bytes;
};
status_or<bool> register_boolean_selection_verifier(verifier_registry&, coordinate_tag, index_tag);
template<class T,class I> status_or<std::shared_ptr<const published_artifact<selected_exact_boundary<T,I>>>> select_boolean_boundary(boolean_context<T,I>&);
template<class T,class I> bool selected_exact_boundary_has_canonical_encoding(const selected_exact_boundary<T,I>&);
#define YGOR_SELECTION_EXTERN(T,I) extern template status_or<std::shared_ptr<const published_artifact<selected_exact_boundary<T,I>>>> select_boolean_boundary(boolean_context<T,I>&)
YGOR_SELECTION_EXTERN(float,std::uint32_t);
YGOR_SELECTION_EXTERN(float,std::uint64_t);
YGOR_SELECTION_EXTERN(double,std::uint32_t);
YGOR_SELECTION_EXTERN(double,std::uint64_t);
#undef YGOR_SELECTION_EXTERN
#define YGOR_SELECTION_ENCODING_EXTERN(T,I) extern template bool selected_exact_boundary_has_canonical_encoding(const selected_exact_boundary<T,I>&)
YGOR_SELECTION_ENCODING_EXTERN(float,std::uint32_t);
YGOR_SELECTION_ENCODING_EXTERN(float,std::uint64_t);
YGOR_SELECTION_ENCODING_EXTERN(double,std::uint32_t);
YGOR_SELECTION_ENCODING_EXTERN(double,std::uint64_t);
#undef YGOR_SELECTION_ENCODING_EXTERN
} }
#endif
