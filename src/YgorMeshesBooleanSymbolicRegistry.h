#pragma once
#ifndef YGOR_MESHES_BOOLEAN_SYMBOLIC_REGISTRY_H_
#define YGOR_MESHES_BOOLEAN_SYMBOLIC_REGISTRY_H_
#include "YgorMeshesBooleanIntersectionEvents.h"
namespace ygor {
namespace mesh_boolean {
constexpr std::uint64_t symbolic_complex_type_tag = 0x59474253594d3036ULL;
constexpr std::uint16_t symbolic_complex_schema = 6;
constexpr std::uint16_t symbolic_reconciliation_schema_v1 = 1;
enum class symbolic_curve_kind : std::uint8_t { carrier, atomic_interval };
enum class symbolic_order_kind : std::uint8_t { planar_angular, carrier_radial };
struct planar_ray_group {
  exact_vector2 direction;
  std::vector<symbolic_curve_id> curves;
};
struct planar_angular_order {
  symbolic_order_kind kind = symbolic_order_kind::planar_angular;
  facet_id facet;
  symbolic_vertex_id vertex;
  std::vector<planar_ray_group> groups;
};
struct radial_sheet_group {
  exact_vector2 direction;
  std::vector<facet_id> facets;
};
struct carrier_radial_order {
  symbolic_order_kind kind = symbolic_order_kind::carrier_radial;
  symbolic_curve_id carrier;
  std::vector<radial_sheet_group> groups;
};
struct symbolic_vertex {
  symbolic_vertex_id id;
  exact_point3 point;
  std::vector<original_vertex_id> original_vertices;
  std::vector<edge_use_id> edge_uses;
  std::vector<undirected_edge_id> undirected_edges;
  std::vector<facet_id> facets;
  std::vector<raw_event_id> overlap_regions;
  std::vector<raw_event_id> raw_points;
  std::vector<symbolic_curve_id> incident_curves;
  std::vector<construction_node_id> constructions;
};
struct symbolic_curve {
  symbolic_curve_id id;
  symbolic_curve_kind kind = symbolic_curve_kind::carrier;
  exact_line3 carrier;
  std::optional<symbolic_curve_id> parent_carrier;
  std::optional<exact_interval> parameters;
  std::optional<symbolic_vertex_id> lower, upper;
  std::vector<raw_event_id> raw_intervals;
  std::vector<candidate_id> raw_carriers;
  std::vector<facet_id> facets;
  std::vector<raw_event_id> overlap_regions;
  std::vector<raw_curve_ownership> ownership;
  std::vector<symbolic_vertex_id> ordered_vertices;
  std::vector<symbolic_curve_id> ordered_intervals;
  std::vector<construction_node_id> constructions;
};
struct source_edge_split_sequence {
  undirected_edge_id edge;
  original_vertex_id canonical_origin, canonical_destination;
  std::vector<symbolic_vertex_id> vertices;
  std::vector<exact_scalar> parameters;
};
struct directed_edge_split_view {
  edge_use_id edge_use;
  undirected_edge_id sequence;
  bool forward = true;
};
struct original_vertex_mapping {
  original_vertex_id source;
  symbolic_vertex_id symbolic;
};
struct raw_point_mapping {
  raw_event_id source;
  symbolic_vertex_id symbolic;
};
struct raw_interval_mapping {
  raw_event_id source;
  std::vector<symbolic_curve_id> atomic_intervals;
  orientation_parity parity = orientation_parity::agree;
};
struct raw_carrier_mapping {
  std::uint64_t source_ordinal = 0;
  candidate_id candidate;
  symbolic_curve_id carrier;
};
struct symbolic_region_boundary_cycle {
  region_cycle_role role = region_cycle_role::outer;
  std::vector<symbolic_vertex_id> vertices;
  std::vector<std::vector<symbolic_curve_id>> boundary_intervals;
};
struct raw_region_mapping {
  raw_event_id source;
  std::vector<symbolic_region_boundary_cycle> boundary_cycles;
};
struct symbolic_reconciliation_request {
  std::uint16_t schema = symbolic_reconciliation_schema_v1;
  digest prior_digest;
  std::uint64_t prior_generation = 1;
  facet_id facet;
  symbolic_curve_id first_curve, second_curve;
  exact_point3 point;
  std::vector<construction_node_id> constructions;
  digest canonical_key;
};
inline int reconciliation_request_structural_compare(
    const symbolic_reconciliation_request &a,
    const symbolic_reconciliation_request &b) noexcept {
  if (a.facet != b.facet) return a.facet < b.facet ? -1 : 1;
  if (a.first_curve != b.first_curve)
    return a.first_curve < b.first_curve ? -1 : 1;
  if (a.second_curve != b.second_curve)
    return a.second_curve < b.second_curve ? -1 : 1;
  int c = canonical_encoding_compare(a.point.x, b.point.x);
  if (!c) c = canonical_encoding_compare(a.point.y, b.point.y);
  if (!c) c = canonical_encoding_compare(a.point.z, b.point.z);
  return c;
}
inline bool reconciliation_request_less(
    const symbolic_reconciliation_request &a,
    const symbolic_reconciliation_request &b) noexcept {
  if (a.canonical_key != b.canonical_key)
    return a.canonical_key < b.canonical_key;
  return reconciliation_request_structural_compare(a, b) < 0;
}
inline bool reconciliation_request_equal(
    const symbolic_reconciliation_request &a,
    const symbolic_reconciliation_request &b) noexcept {
  return reconciliation_request_structural_compare(a, b) == 0;
}
template <class T, class I> struct symbolic_complex {
  context_owner_token owner;
  digest setup_digest, upstream_digest, validated_digest, kernel_policy_digest,
      artifact_digest;
  std::shared_ptr<const published_artifact<raw_event_set<T, I>>> raw_events;
  std::shared_ptr<const published_artifact<validated_operands<T, I>>> validated;
  std::shared_ptr<const construction_storage> constructions;
  std::vector<symbolic_vertex> vertices;
  std::vector<symbolic_curve> curves;
  std::vector<source_edge_split_sequence> edge_sequences;
  std::vector<directed_edge_split_view> directed_edge_views;
  std::vector<planar_angular_order> angular_orders;
  std::vector<carrier_radial_order> radial_orders;
  std::vector<original_vertex_mapping> original_vertices;
  std::vector<raw_point_mapping> raw_points;
  std::vector<raw_interval_mapping> raw_intervals;
  std::vector<raw_carrier_mapping> raw_carriers;
  std::vector<raw_region_mapping> raw_regions;
  std::uint64_t generation = 1;
  std::shared_ptr<const published_artifact<symbolic_complex<T, I>>>
      prior_generation;
  std::vector<symbolic_reconciliation_request> reconciliation_history;
  std::vector<std::uint8_t> canonical_symbolic_bytes, artifact_bytes;
};
status_or<bool> register_symbolic_registry_verifier(verifier_registry &,
                                                    coordinate_tag, index_tag);
template <class T, class I>
status_or<std::shared_ptr<const published_artifact<symbolic_complex<T, I>>>>
build_symbolic_complex(boolean_context<T, I> &);
template <class T, class I>
status_or<std::shared_ptr<const published_artifact<symbolic_complex<T, I>>>>
reconcile_symbolic_complex(
    boolean_context<T, I> &,
    std::shared_ptr<const published_artifact<symbolic_complex<T, I>>>,
    std::vector<symbolic_reconciliation_request>);
#define YGOR_SYMBOL_EXTERN(T, I)                                               \
  extern template status_or<                                                   \
      std::shared_ptr<const published_artifact<symbolic_complex<T, I>>>>       \
  build_symbolic_complex(boolean_context<T, I> &)
YGOR_SYMBOL_EXTERN(float, std::uint32_t);
YGOR_SYMBOL_EXTERN(float, std::uint64_t);
YGOR_SYMBOL_EXTERN(double, std::uint32_t);
YGOR_SYMBOL_EXTERN(double, std::uint64_t);
#undef YGOR_SYMBOL_EXTERN
#define YGOR_RECONCILE_EXTERN(T, I)                                            \
  extern template status_or<                                                   \
      std::shared_ptr<const published_artifact<symbolic_complex<T, I>>>>       \
  reconcile_symbolic_complex(                                                  \
      boolean_context<T, I> &,                                                 \
      std::shared_ptr<const published_artifact<symbolic_complex<T, I>>>,       \
      std::vector<symbolic_reconciliation_request>)
YGOR_RECONCILE_EXTERN(float, std::uint32_t);
YGOR_RECONCILE_EXTERN(float, std::uint64_t);
YGOR_RECONCILE_EXTERN(double, std::uint32_t);
YGOR_RECONCILE_EXTERN(double, std::uint64_t);
#undef YGOR_RECONCILE_EXTERN
} // namespace mesh_boolean
} // namespace ygor
#endif
