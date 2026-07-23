#pragma once
#ifndef YGOR_MESHES_BOOLEAN_EXACT_RESULT_H_
#define YGOR_MESHES_BOOLEAN_EXACT_RESULT_H_

#include "YgorMeshesBooleanProductContractPolicies.h"
#include "YgorMeshesBooleanSelection.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t exact_stratified_boundary_schema = 1;
constexpr std::uint16_t exact_stratified_boundary_checker_version = 1;

struct exact_result_decode_limits {
  std::uint64_t max_record_bytes = 64U * 1024U * 1024U;
  std::uint64_t max_entities = 4U * 1024U * 1024U;
  std::uint64_t max_references = 32U * 1024U * 1024U;
  std::uint64_t max_exact_hex_bytes = 8U * 1024U * 1024U;
  std::uint64_t max_string_bytes = 16U * 1024U;
};

struct exact_result_backend_binding {
  std::uint16_t schema = product_contract_schema_version;
  backend_identity producer;
  backend_selection_mode selection = backend_selection_mode::explicit_backend;
  bool fallback_used = false;
  std::vector<backend_id> attempted_backends;
  std::optional<product_error_code> primary_failure;
};

struct exact_result_preparation_binding {
  std::uint16_t schema = product_contract_schema_version;
  preparation_mode mode = preparation_mode::strict_validation;
  digest input_digest;
  digest prepared_digest;
  digest policy_digest;
  digest report_digest;
  bool geometry_changed = false;
};

struct exact_feature_reference {
  std::uint16_t kind = 0;
  std::uint64_t primary = 0;
  std::uint64_t secondary = 0;
};

struct exact_construction_record {
  construction_node_id id;
  construction_kind kind = construction_kind::exact_relation;
  std::vector<construction_node_id> children;
  std::vector<exact_feature_reference> defining_sources;
  std::vector<defining_relation_id> defining_relations;
  std::vector<std::uint8_t> exact_result;
};

struct exact_defining_relation_record {
  defining_relation_id id;
  defining_relation_kind kind = defining_relation_kind::coordinate_equality;
  std::uint16_t formula_version = 1;
  construction_node_id construction;
  std::vector<construction_node_id> operand_nodes;
  std::vector<exact_feature_reference> defining_sources;
  std::array<exact_scalar, 4> coefficients{{exact_scalar(0), exact_scalar(0),
                                            exact_scalar(0), exact_scalar(0)}};
  exact_sign expected = exact_sign::zero;
};

struct exact_result_vertex {
  selected_vertex_id id;
  global_vertex_id source;
  symbolic_vertex_id symbolic;
  exact_point3 coordinate;
  std::vector<original_vertex_id> original_vertices;
  std::vector<construction_node_id> constructions;
};

struct exact_result_edge_geometry {
  selected_edge_id edge;
  global_edge_kind kind = global_edge_kind::source_edge;
  std::vector<symbolic_curve_id> curves;
};

struct exact_result_patch_geometry {
  selected_patch_id patch;
  exact_plane3 plane;
  exact_scalar projected_double_area;
};

struct exact_result_curve {
  symbolic_curve_id id;
  symbolic_curve_kind kind = symbolic_curve_kind::carrier;
  exact_line3 carrier;
  std::optional<symbolic_curve_id> parent_carrier;
  std::optional<exact_interval> parameters;
  std::optional<symbolic_vertex_id> lower;
  std::optional<symbolic_vertex_id> upper;
  std::vector<facet_id> facets;
  std::vector<construction_node_id> constructions;
};

struct exact_result_source_contributor {
  sheet_use_id use;
  source_sheet_member_id member;
  operand_id operand;
  shell_id shell;
  facet_id facet;
  bool source_plane_agrees = true;
  patch_plane_side occupied_side = patch_plane_side::negative;
  bool representative = false;
};

struct exact_result_patch_provenance {
  selected_patch_id patch;
  std::vector<exact_result_source_contributor> contributors;
};

struct exact_result_source_vertex_occurrence {
  vertex_occurrence_id occurrence;
  global_vertex_id vertex;
  operand_id operand;
  shell_id shell;
  std::vector<global_halfedge_id> incident_halfedges;
  std::vector<vertex_sector_id> link_regions;
};

struct exact_result_link_ray {
  link_ray_id id;
  exact_vector3 direction;
  link_ray_id antipode;
};

struct exact_result_link_arc {
  link_arc_id id;
  vertex_occurrence_id occurrence;
  link_ray_id origin;
  link_ray_id destination;
  std::vector<sheet_use_id> layers;
};

struct exact_result_vertex_sector {
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

struct exact_stratified_boundary {
  std::uint16_t schema = exact_stratified_boundary_schema;
  operation selected_operation = operation::regularized_union;
  selected_boundary_topology topology = selected_boundary_topology::empty;
  exact_result_backend_binding backend;
  exact_result_preparation_binding preparation;
  digest setup_digest;
  digest labeled_digest;
  digest arrangement_digest;
  digest selected_artifact_digest;
  digest selected_semantic_digest;

  std::vector<patch_selection_decision> decisions;
  std::vector<exact_result_vertex> vertices;
  std::vector<selected_vertex_occurrence> vertex_occurrences;
  std::vector<selected_edge> edges;
  std::vector<exact_result_edge_geometry> edge_geometry;
  std::vector<selected_halfedge> halfedges;
  std::vector<selected_cycle> cycles;
  std::vector<selected_patch> patches;
  std::vector<exact_result_patch_geometry> patch_geometry;
  std::vector<topology_obstruction> topology_obstructions;
  std::vector<exact_result_curve> curves;
  std::vector<exact_construction_record> constructions;
  std::vector<exact_defining_relation_record> defining_relations;
  std::vector<exact_result_patch_provenance> provenance;
  std::vector<exact_result_source_vertex_occurrence> source_vertex_occurrences;
  std::vector<exact_result_link_ray> link_rays;
  std::vector<exact_result_link_arc> link_arcs;
  std::vector<exact_result_vertex_sector> vertex_sectors;
  boolean_selection_certificate certificate;
  digest canonical_digest;
};

product_status_or<bool>
validate_exact_stratified_boundary(const exact_stratified_boundary &) noexcept;

product_status_or<std::shared_ptr<const exact_stratified_boundary>>
freeze_exact_stratified_boundary(exact_stratified_boundary);

product_status_or<std::vector<std::uint8_t>>
encode_exact_stratified_boundary(const exact_stratified_boundary &);

product_status_or<std::shared_ptr<const exact_stratified_boundary>>
decode_exact_stratified_boundary(
    const std::vector<std::uint8_t> &,
    const exact_result_decode_limits & = exact_result_decode_limits{});

product_status_or<bool> verify_serialized_exact_stratified_boundary(
    const std::vector<std::uint8_t> &,
    const exact_result_decode_limits & = exact_result_decode_limits{}) noexcept;

template <class T, class I>
product_status_or<exact_stratified_boundary> detach_exact_stratified_boundary(
    const selected_exact_boundary<T, I> &, exact_result_backend_binding,
    exact_result_preparation_binding);

#define YGOR_EXACT_RESULT_EXTERN(T, I)                                         \
  extern template product_status_or<exact_stratified_boundary>                 \
  detach_exact_stratified_boundary(                                            \
      const selected_exact_boundary<T, I> &, exact_result_backend_binding,      \
      exact_result_preparation_binding)
YGOR_EXACT_RESULT_EXTERN(float, std::uint32_t);
YGOR_EXACT_RESULT_EXTERN(float, std::uint64_t);
YGOR_EXACT_RESULT_EXTERN(double, std::uint32_t);
YGOR_EXACT_RESULT_EXTERN(double, std::uint64_t);
#undef YGOR_EXACT_RESULT_EXTERN

} // namespace mesh_boolean
} // namespace ygor

#endif
