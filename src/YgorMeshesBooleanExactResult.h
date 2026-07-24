#pragma once
#ifndef YGOR_MESHES_BOOLEAN_EXACT_RESULT_H_
#define YGOR_MESHES_BOOLEAN_EXACT_RESULT_H_

#include "YgorMeshesBooleanProductContractPolicies.h"
#include "YgorMeshesBooleanProductContractResult.h"
#include "YgorMeshesBooleanSelection.h"

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t exact_stratified_boundary_schema = 2;
constexpr std::uint16_t exact_stratified_boundary_checker_version = 2;
constexpr std::uint16_t exact_coordinate_export_schema = 2;
constexpr std::uint16_t exact_coordinate_export_checker_version = 2;

struct exact_result_decode_limits {
  std::uint64_t max_record_bytes = 64U * 1024U * 1024U;
  std::uint64_t max_entities = 4U * 1024U * 1024U;
  std::uint64_t max_references = 32U * 1024U * 1024U;
  std::uint64_t max_exact_hex_bytes = 8U * 1024U * 1024U;
  std::uint64_t max_string_bytes = 16U * 1024U;
};

struct exact_coordinate_export_limits {
  std::uint64_t max_record_bytes = 72U * 1024U * 1024U;
  std::uint64_t max_indexed_entities = 4U * 1024U * 1024U;
  exact_result_decode_limits exact_result;
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
  std::array<exact_scalar, 4> coefficients{
      {exact_scalar(0), exact_scalar(0), exact_scalar(0), exact_scalar(0)}};
  exact_sign expected = exact_sign::zero;
};

struct exact_result_vertex {
  selected_vertex_id id;
  global_vertex_id source;
  symbolic_vertex_id symbolic;
  exact_point3 coordinate;
  std::vector<original_vertex_ref> original_vertices;
  struct original_raw_bits_record {
    original_vertex_ref source;
    coordinate_tag coordinate = coordinate_tag::binary64;
    std::array<std::uint64_t, 3> bits{{0, 0, 0}};
  };
  std::vector<original_raw_bits_record> original_raw_bits;
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
  std::vector<patch_side_label> side_labels;
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

product_status_or<exact_result_handle> make_exact_result_handle(
    const std::shared_ptr<const exact_stratified_boundary> &);

product_status_or<std::shared_ptr<const exact_stratified_boundary>>
read_exact_result(const exact_result_handle &,
                  const exact_result_decode_limits & = {});

// Exact-coordinate export keeps canonical selected IDs and surface
// occurrences distinct. The index tag covers the dense selected vertex,
// occurrence, edge, halfedge, cycle, and patch strata. Source and construction
// provenance retain their stable 64-bit IDs. Coordinates stay canonical
// rationals and are never converted to a floating-point type.
struct exact_coordinate_export {
  std::uint16_t schema = exact_coordinate_export_schema;
  index_tag index = index_tag::uint64;
  std::uint64_t entity_capacity = std::numeric_limits<std::uint64_t>::max();
  operation selected_operation = operation::regularized_union;
  selected_boundary_topology topology = selected_boundary_topology::empty;
  digest exact_result_digest;
  digest canonical_digest;
  exact_result_handle exact_result;
  std::shared_ptr<const exact_stratified_boundary> boundary;
  std::vector<std::uint8_t> canonical_bytes;
};

using exact_coordinate_export_handle =
    std::shared_ptr<const exact_coordinate_export>;

product_status_or<exact_coordinate_export_handle> export_exact_coordinates(
    const exact_result_handle &, index_tag,
    const exact_coordinate_export_limits & = {});

product_status_or<std::vector<std::uint8_t>>
encode_exact_coordinate_export(const exact_coordinate_export_handle &);

product_status_or<exact_coordinate_export_handle> decode_exact_coordinate_export(
    const std::vector<std::uint8_t> &,
    const exact_coordinate_export_limits & = {});

product_status_or<bool> verify_serialized_exact_coordinate_export(
    const std::vector<std::uint8_t> &,
    const exact_coordinate_export_limits & = {}) noexcept;

product_status_or<bool> validate_exact_coordinate_export_binding(
    const exact_coordinate_export_handle &, const exact_result_handle &) noexcept;

template <class T> constexpr coordinate_tag exact_result_coordinate_type() {
  static_assert(std::is_same<T, float>::value || std::is_same<T, double>::value,
                "durable exact results support binary32 and binary64 targets");
  return std::is_same<T, float>::value ? coordinate_tag::binary32
                                       : coordinate_tag::binary64;
}

template <class I> constexpr index_tag exact_result_index_type() {
  static_assert(std::is_same<I, std::uint32_t>::value ||
                    std::is_same<I, std::uint64_t>::value,
                "durable exact results support 32-bit and 64-bit indices");
  return std::is_same<I, std::uint32_t>::value ? index_tag::uint32
                                               : index_tag::uint64;
}

template <class I>
product_status_or<exact_coordinate_export_handle> export_exact_coordinates(
    const exact_result_handle &exact,
    const exact_coordinate_export_limits &limits = {}) {
  return export_exact_coordinates(exact, exact_result_index_type<I>(), limits);
}

template <class T, class I> struct exact_result_realization_request {
  exact_result_handle exact_result;
  result_representation representation =
      result_representation::exact_stratified;
  product_realization_policy policy;
  coordinate_tag coordinate = exact_result_coordinate_type<T>();
  index_tag index = exact_result_index_type<I>();
  digest exact_result_digest;
};

template <class T, class I>
product_status_or<exact_result_realization_request<T, I>>
request_later_realization(const exact_result_handle &exact,
                          result_representation representation,
                          product_realization_policy policy) {
  auto fail = [](const char *key) {
    return product_status_or<exact_result_realization_request<T, I>>(
        make_product_error(product_error_code::approximation_policy_rejected,
                           key));
  };
  auto decoded = read_exact_result(exact);
  if (!decoded.has_value())
    return decoded.error();
  boolean_product_options options;
  options.result.representation = representation;
  options.realization = policy;
  auto valid_policy = validate_product_options(options);
  if (!valid_policy.has_value())
    return valid_policy.error();
  if (policy.schema != product_contract_schema_version)
    return fail("later_realization.schema");
  if (representation == result_representation::exact_stratified) {
    if (policy.semantics != product_realization_semantics::not_requested ||
        policy.search.strategy != realization_search_strategy::none ||
        policy.approximation.enabled)
      return fail("later_realization.exact_coordinate_policy");
  } else if (representation == result_representation::exact_in_T_mesh) {
    if (policy.semantics != product_realization_semantics::exact_in_T ||
        policy.approximation.enabled)
      return fail("later_realization.exact_in_T_policy");
  } else if (representation ==
             result_representation::certified_approximate_mesh) {
    if (policy.semantics !=
            product_realization_semantics::certified_approximate_embedding_v1 ||
        !policy.approximation.enabled)
      return fail("later_realization.approximate_policy");
  } else {
    return fail("later_realization.representation");
  }
  exact_result_realization_request<T, I> request;
  request.exact_result = exact;
  request.representation = representation;
  request.policy = std::move(policy);
  request.exact_result_digest = exact->canonical_digest;
  return request;
}

template <class T, class I>
product_status_or<exact_stratified_boundary>
detach_exact_stratified_boundary(const selected_exact_boundary<T, I> &,
                                 exact_result_backend_binding,
                                 exact_result_preparation_binding);

template <class T, class I>
product_status_or<boolean_product_result_handle<T, I>>
publish_exact_boolean_result(boolean_context<T, I> &,
                             exact_result_backend_binding,
                             exact_result_preparation_binding);

template <class T, class I>
product_status_or<boolean_product_result_handle<T, I>>
evaluate_boolean_product_result(boolean_context<T, I> &,
                                exact_result_backend_binding,
                                exact_result_preparation_binding,
                                result_representation);

template <class T, class I>
product_status_or<boolean_product_result_handle<T, I>>
evaluate_boolean_product_result(boolean_context<T, I> &,
                                exact_result_backend_binding,
                                exact_result_preparation_binding,
                                result_representation,
                                product_realization_policy);

template <class T, class I>
product_status_or<boolean_product_result_handle<T, I>>
record_failed_realization(const boolean_product_result_handle<T, I> &exact,
                          result_representation requested,
                          product_realization_semantics semantics,
                          product_error failure) {
  if (!exact ||
      exact->representation != result_representation::exact_stratified ||
      exact->mesh || requested == result_representation::exact_stratified ||
      semantics == product_realization_semantics::not_requested)
    return make_product_error(product_error_code::stale_binding,
                              "failed_realization.exact_result");
  auto result = *exact;
  result.realization = realization_attempt_record{};
  result.realization->requested = requested;
  result.realization->semantics = semantics;
  result.realization->failure = std::move(failure);
  return freeze_boolean_product_result(std::move(result));
}

#define YGOR_EXACT_RESULT_EXTERN(T, I)                                         \
  extern template product_status_or<exact_stratified_boundary>                 \
  detach_exact_stratified_boundary(const selected_exact_boundary<T, I> &,      \
                                   exact_result_backend_binding,               \
                                   exact_result_preparation_binding);          \
  extern template product_status_or<boolean_product_result_handle<T, I>>       \
  publish_exact_boolean_result(boolean_context<T, I> &,                        \
                               exact_result_backend_binding,                   \
                               exact_result_preparation_binding);              \
  extern template product_status_or<boolean_product_result_handle<T, I>>       \
  evaluate_boolean_product_result(                                             \
      boolean_context<T, I> &, exact_result_backend_binding,                   \
      exact_result_preparation_binding, result_representation);                \
  extern template product_status_or<boolean_product_result_handle<T, I>>       \
  evaluate_boolean_product_result(                                             \
      boolean_context<T, I> &, exact_result_backend_binding,                   \
      exact_result_preparation_binding, result_representation,                 \
      product_realization_policy)
YGOR_EXACT_RESULT_EXTERN(float, std::uint32_t);
YGOR_EXACT_RESULT_EXTERN(float, std::uint64_t);
YGOR_EXACT_RESULT_EXTERN(double, std::uint32_t);
YGOR_EXACT_RESULT_EXTERN(double, std::uint64_t);
#undef YGOR_EXACT_RESULT_EXTERN

} // namespace mesh_boolean
} // namespace ygor

#endif
