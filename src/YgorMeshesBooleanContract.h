#pragma once
#ifndef YGOR_MESHES_BOOLEAN_CONTRACT_H_
#define YGOR_MESHES_BOOLEAN_CONTRACT_H_

#include "YgorMath.h"
#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t engine_version_major = 1, engine_version_minor = 0,
                        engine_version_patch = 0;
constexpr std::uint16_t replay_schema_version = 2;
enum class operation : std::uint8_t {
  regularized_union,
  regularized_intersection,
  a_minus_b,
  b_minus_a,
  symmetric_difference
};
struct occupancy_pair {
  bool in_a = false, in_b = false;
};
constexpr bool operator==(occupancy_pair a, occupancy_pair b) noexcept {
  return a.in_a == b.in_a && a.in_b == b.in_b;
}
constexpr bool operator!=(occupancy_pair a, occupancy_pair b) noexcept {
  return !(a == b);
}
constexpr bool operation_value(operation op, bool a, bool b) noexcept {
  return op == operation::regularized_union
             ? a || b
             : op == operation::regularized_intersection
                   ? a && b
                   : op == operation::a_minus_b
                         ? a && !b
                         : op == operation::b_minus_a
                               ? !a && b
                               : op == operation::symmetric_difference ? a != b
                                                                       : false;
}
class operation_contract {
  operation op_;
  std::array<bool, 4> table_;

public:
  explicit constexpr operation_contract(operation op) noexcept
      : op_(op), table_{{operation_value(op, false, false),
                         operation_value(op, false, true),
                         operation_value(op, true, false),
                         operation_value(op, true, true)}} {}
  constexpr operation selected_operation() const noexcept { return op_; }
  constexpr bool occupied(bool a, bool b) const noexcept {
    return table_[(a ? 2U : 0U) | (b ? 1U : 0U)];
  }
  constexpr const std::array<bool, 4> &table() const noexcept { return table_; }
  // 0 rejects the patch, +1 keeps it, -1 reverses it; occupied volume is on the
  // negative side.
  constexpr int patch_orientation(occupancy_pair negative,
                                  occupancy_pair positive) const noexcept {
    const bool n = occupied(negative.in_a, negative.in_b),
               p = occupied(positive.in_a, positive.in_b);
    return n == p ? 0 : (n ? 1 : -1);
  }
};

enum class solid_policy : std::uint8_t { outward_oriented_nested_shells };
enum class verification_level : std::uint8_t { mandatory, exhaustive };
enum class trace_level : std::uint8_t { off, failures, stages, full };
enum class determinism_policy : std::uint8_t { strict };
enum class classification_strategy : std::uint8_t {
  independent_patch_side_v1
};
enum class realization_strategy : std::uint8_t {
  nearest_only,
  neighboring_values
};
enum class realization_semantics : std::uint8_t { exact_in_T };
enum class original_coordinate_policy : std::uint8_t { preserve_bits };
enum class realization_topology_policy : std::uint8_t { triangulated_v1 };
enum class pair_certification_policy : std::uint8_t {
  conservative_domain_aabb_v1
};
enum class realization_certificate_level : std::uint8_t { full };
enum class output_topology_policy : std::uint8_t {
  triangulated_v1_no_simplification
};
enum class result_topology_policy : std::uint8_t {
  closed_embedded_two_manifold
};
enum class boolean_error_code : std::uint8_t {
  input_contract_error,
  unsupported_platform,
  resource_limit,
  index_overflow,
  result_topology_not_supported,
  output_not_representable,
  internal_invariant_error
};
enum class boolean_stage : std::uint8_t {
  context_setup,
  input_validation,
  broad_phase,
  intersection_events,
  symbolic_registry,
  local_refinement,
  global_arrangement,
  cell_classification,
  boolean_selection,
  result_topology_preflight,
  geometry_realization,
  output_assembly,
  final_verification
};
enum class resource_kind : std::uint8_t {
  authoritative_bytes,
  stage_private_bytes,
  work_units,
  entities,
  candidates,
  raw_events,
  symbolic_vertices,
  symbolic_curves,
  local_vertices,
  local_atomic_edges,
  local_halfedges,
  local_boundary_walks,
  local_faces,
  local_patches,
  local_certificate_entries,
  reconciliation_requests,
  successor_generations,
  global_vertices,
  global_atomic_edges,
  global_halfedges,
  global_patches,
  source_sheet_members,
  sheet_uses,
  seams,
  seam_sectors,
  source_edge_sectors,
  coincident_memberships,
  side_nodes,
  vertex_occurrences,
  vertex_sectors,
  link_rays,
  link_arcs,
  link_regions,
  side_transitions,
  probe_descriptors,
  mapping_entries,
  arrangement_certificate_entries,
  planar_scratch,
  radial_scratch,
  link_scratch,
  cells,
  classification_transitions,
  seed_certificates,
  patch_side_labels,
  propagation_records,
  selection_decisions,
  selected_patches,
  selected_cycles,
  selected_halfedges,
  selected_edges,
  selected_vertices,
  selected_vertex_occurrences,
  topology_obstructions,
  selection_provenance,
  exact_number_bits,
  diagnostic_records,
  diagnostic_bytes,
  trace_records,
  trace_bytes,
  realization_attempts,
  realization_graph_nodes,
  realization_graph_edges,
  realization_components,
  realization_pair_boxes,
  realization_pair_candidates,
  realization_pair_checks,
  realization_solver_trail,
  realization_component_transcripts,
  realization_verifier_witnesses,
  output_vertices,
  output_faces,
  output_face_indices,
  output_involved_entries,
  output_components,
  output_mappings,
  output_certificate_entries,
  output_canonical_bytes,
  cancellation,
  verifier_work,
  verifier_scratch_bytes,
  evidence_records,
  evidence_bytes,
  report_bytes,
  dependency_nodes,
  dependency_edges,
  replay_bytes,
  minimization_work,
  count
};
enum class performance_role : std::uint8_t { producer, verifier };
enum class performance_counter : std::uint8_t {
  small_integer_operations,
  large_integer_operations,
  limb_additions,
  limb_multiplications,
  division_calls,
  divided_limbs,
  gcd_calls,
  rational_normalizations,
  cross_cancellations,
  max_numerator_limbs,
  max_denominator_limbs,
  orient2d_calls,
  orient3d_calls,
  filter_accepts,
  filter_fallbacks,
  exact_fallbacks,
  geometric_exact_divisions,
  support_plane_constructions,
  point_in_polygon_edge_tests,
  ring_edge_candidate_pairs,
  exact_ring_edge_tests,
  ear_candidates,
  exact_ear_tests,
  self_embedding_candidate_pairs,
  exact_facet_pair_tests,
  shell_location_queries,
  canonicalization_refinements,
  canonicalization_branches,
  broad_phase_node_pairs,
  broad_phase_leaf_facet_pairs,
  broad_phase_final_candidates,
  broad_phase_false_positives,
  broad_phase_build_comparisons,
  broad_phase_verifier_candidate_checks,
  event_candidate_facet_pairs,
  plane_relation_classes,
  exact_carrier_polygon_tests,
  raw_events,
  duplicate_derivations,
  exact_equality_checks,
  hash_bucket_probes,
  canonical_key_encodings,
  symbolic_vertices,
  symbolic_curves,
  constraints,
  candidate_constraint_pairs,
  exact_constraint_intersections,
  dcel_entities,
  reconciliation_passes,
  global_patches,
  global_edges,
  global_uses,
  link_entities,
  patch_witness_slabs,
  patch_witness_crossings,
  probe_constraints,
  classification_source_facets,
  classification_source_triangles,
  classification_probes,
  ray_box_candidates,
  exact_ray_facet_tests,
  accepted_ray_hits,
  alternate_rays,
  reconstructed_rays,
  exterior_attachment_candidates,
  realized_variables,
  realization_axis_candidates,
  realization_obligations,
  realization_pair_boxes,
  realization_pair_candidates,
  realization_exact_pair_checks,
  realization_constraint_components,
  realization_solver_nodes,
  realization_rejected_prefixes,
  realization_complete_assignments,
  allocation_count,
  copied_artifact_bytes,
  plane_side_calls,
  dot_sign_calls,
  orient2d_filter_accepts,
  orient2d_filter_fallbacks,
  orient3d_filter_accepts,
  orient3d_filter_fallbacks,
  plane_side_filter_accepts,
  plane_side_filter_fallbacks,
  dot_sign_filter_accepts,
  dot_sign_filter_fallbacks,
  global_incidence_records,
  global_index_lookups,
  link_direction_candidates,
  exact_link_direction_tests,
  symbolic_point_candidates,
  symbolic_endpoint_incidences,
  coplanar_edge_candidates,
  exact_coplanar_edge_tests,
  link_direction_sort_comparisons,
  count
};
struct performance_counter_snapshot {
  std::array<std::uint64_t,
             static_cast<std::size_t>(performance_counter::count)>
      values{{}};
  std::array<std::uint64_t, static_cast<std::size_t>(resource_kind::count)>
      resources{{}};
  std::uint64_t value(performance_counter c) const noexcept {
    return values[static_cast<std::size_t>(c)];
  }
  std::uint64_t resource(resource_kind k) const noexcept {
    return resources[static_cast<std::size_t>(k)];
  }
};
struct stage_performance_snapshot {
  std::uint64_t producer_nanoseconds = 0, verifier_nanoseconds = 0;
  performance_counter_snapshot producer, verifier;
};
struct performance_snapshot {
  std::uint16_t schema = 1;
  bool collected = false;
  std::array<stage_performance_snapshot,
             static_cast<std::size_t>(boolean_stage::final_verification) + 1>
      stages{{}};
  const stage_performance_snapshot &stage(boolean_stage s) const noexcept {
    return stages[static_cast<std::size_t>(s)];
  }
};
enum class artifact_slot : std::uint8_t {
  validated_operands,
  candidate_stream,
  raw_event_set,
  symbolic_complex,
  refined_facet_patches,
  arrangement_complex,
  labeled_arrangement,
  selected_exact_boundary,
  realized_boundary,
  assembled_output,
  final_verification
};

class context_owner_token {
  std::uint64_t value_ = 0;
  explicit constexpr context_owner_token(std::uint64_t v) : value_(v) {}
  friend context_owner_token make_context_owner_token();

public:
  constexpr context_owner_token() = default;
  constexpr bool operator==(context_owner_token o) const {
    return value_ == o.value_;
  }
  constexpr bool operator!=(context_owner_token o) const {
    return !(*this == o);
  }
  constexpr std::uint64_t value_for_debug() const { return value_; }
};
context_owner_token make_context_owner_token();

template <class Tag> class strong_id {
  std::uint64_t value_ = invalid_value;
  explicit constexpr strong_id(std::uint64_t v) : value_(v) {}

public:
  static constexpr std::uint64_t invalid_value =
      std::numeric_limits<std::uint64_t>::max();
  constexpr strong_id() = default;
  static constexpr strong_id from_canonical_value(std::uint64_t v) {
    return v == invalid_value ? throw std::invalid_argument("reserved ID")
                              : strong_id(v);
  }
  constexpr bool valid() const { return value_ != invalid_value; }
  constexpr std::uint64_t value_for_debug() const { return value_; }
  constexpr bool operator==(strong_id o) const { return value_ == o.value_; }
  constexpr bool operator!=(strong_id o) const { return !(*this == o); }
  constexpr bool operator<(strong_id o) const { return value_ < o.value_; }
};
#define YGOR_MB_ID(n)                                                          \
  struct n##_tag {};                                                           \
  using n = strong_id<n##_tag>
YGOR_MB_ID(operand_id);
YGOR_MB_ID(shell_id);
YGOR_MB_ID(original_vertex_id);
YGOR_MB_ID(edge_use_id);
YGOR_MB_ID(undirected_edge_id);
YGOR_MB_ID(facet_id);
YGOR_MB_ID(candidate_id);
YGOR_MB_ID(raw_event_id);
YGOR_MB_ID(construction_node_id);
YGOR_MB_ID(defining_relation_id);
YGOR_MB_ID(symbolic_vertex_id);
YGOR_MB_ID(symbolic_curve_id);
YGOR_MB_ID(local_vertex_id);
YGOR_MB_ID(shared_atomic_edge_id);
YGOR_MB_ID(local_atomic_edge_id);
YGOR_MB_ID(local_halfedge_id);
YGOR_MB_ID(local_boundary_walk_id);
YGOR_MB_ID(local_face_id);
YGOR_MB_ID(local_patch_id);
YGOR_MB_ID(global_halfedge_id);
YGOR_MB_ID(global_vertex_id);
YGOR_MB_ID(vertex_occurrence_id);
YGOR_MB_ID(global_atomic_edge_id);
YGOR_MB_ID(global_patch_id);
YGOR_MB_ID(source_sheet_member_id);
YGOR_MB_ID(sheet_use_id);
YGOR_MB_ID(seam_id);
YGOR_MB_ID(seam_sector_id);
YGOR_MB_ID(source_edge_sector_id);
YGOR_MB_ID(coincident_group_id);
YGOR_MB_ID(patch_side_id);
YGOR_MB_ID(vertex_sector_id);
YGOR_MB_ID(link_ray_id);
YGOR_MB_ID(link_arc_id);
YGOR_MB_ID(link_region_id);
YGOR_MB_ID(side_transition_id);
YGOR_MB_ID(open_region_component_id);
YGOR_MB_ID(cell_id);
YGOR_MB_ID(classification_region_id);
YGOR_MB_ID(classification_transition_id);
YGOR_MB_ID(seed_certificate_id);
YGOR_MB_ID(propagation_path_id);
YGOR_MB_ID(patch_side_label_id);
YGOR_MB_ID(patch_selection_decision_id);
YGOR_MB_ID(selected_patch_id);
YGOR_MB_ID(selected_cycle_id);
YGOR_MB_ID(selected_halfedge_id);
YGOR_MB_ID(selected_edge_id);
YGOR_MB_ID(selected_vertex_id);
YGOR_MB_ID(selected_vertex_occurrence_id);
YGOR_MB_ID(topology_obstruction_id);
YGOR_MB_ID(selection_certificate_id);
YGOR_MB_ID(realization_vertex_id);
YGOR_MB_ID(realization_triangle_id);
YGOR_MB_ID(realization_halfedge_id);
YGOR_MB_ID(realization_obligation_id);
YGOR_MB_ID(realization_constraint_component_id);
YGOR_MB_ID(candidate_value_id);
YGOR_MB_ID(candidate_assignment_id);
YGOR_MB_ID(realization_certificate_id);
YGOR_MB_ID(output_vertex_id);
YGOR_MB_ID(output_face_id);
YGOR_MB_ID(output_component_id);
YGOR_MB_ID(output_assembly_certificate_id);
#undef YGOR_MB_ID
inline operand_id operand_a() { return operand_id::from_canonical_value(0); }
inline operand_id operand_b() { return operand_id::from_canonical_value(1); }
struct original_vertex_ref {
  operand_id operand;
  original_vertex_id vertex;
};
struct facet_ref {
  operand_id operand;
  facet_id facet;
};
inline bool operator==(const original_vertex_ref &a,
                       const original_vertex_ref &b) {
  return a.operand == b.operand && a.vertex == b.vertex;
}
inline bool operator==(const facet_ref &a, const facet_ref &b) {
  return a.operand == b.operand && a.facet == b.facet;
}
using feature_ref = std::variant<
    operand_id, shell_id, original_vertex_id, edge_use_id, undirected_edge_id,
    facet_id, candidate_id, raw_event_id, symbolic_vertex_id, symbolic_curve_id,
    local_vertex_id, shared_atomic_edge_id, local_atomic_edge_id,
    local_halfedge_id, local_boundary_walk_id, local_face_id, local_patch_id,
    global_vertex_id, vertex_occurrence_id, global_atomic_edge_id,
    global_halfedge_id,
    global_patch_id, source_sheet_member_id, sheet_use_id, seam_id,
    seam_sector_id, source_edge_sector_id, coincident_group_id, patch_side_id,
    vertex_sector_id, link_ray_id, link_arc_id, link_region_id,
    side_transition_id, open_region_component_id, cell_id,
    classification_region_id, classification_transition_id, seed_certificate_id,
    propagation_path_id, patch_side_label_id, patch_selection_decision_id,
    selected_patch_id, selected_cycle_id, selected_halfedge_id,
    selected_edge_id, selected_vertex_id, selected_vertex_occurrence_id,
    topology_obstruction_id, selection_certificate_id,
    realization_vertex_id, realization_triangle_id, realization_halfedge_id,
    realization_obligation_id, realization_constraint_component_id,
    candidate_value_id, candidate_assignment_id,
    realization_certificate_id, output_vertex_id, output_face_id,
    output_component_id, output_assembly_certificate_id, original_vertex_ref,
    facet_ref, defining_relation_id>;

// Compare feature references in the same field order used by their canonical
// encoding, without allocating that encoding.
inline int canonical_feature_compare(const feature_ref &a,
                                     const feature_ref &b) {
  if (a.index() != b.index())
    return a.index() < b.index() ? -1 : 1;
  return std::visit(
      [&](const auto &x) {
        using X = typename std::decay<decltype(x)>::type;
        const auto &y = std::get<X>(b);
        if constexpr (std::is_same<X, original_vertex_ref>::value) {
          if (x.operand != y.operand)
            return x.operand < y.operand ? -1 : 1;
          if (x.vertex != y.vertex)
            return x.vertex < y.vertex ? -1 : 1;
        } else if constexpr (std::is_same<X, facet_ref>::value) {
          if (x.operand != y.operand)
            return x.operand < y.operand ? -1 : 1;
          if (x.facet != y.facet)
            return x.facet < y.facet ? -1 : 1;
        } else if (x != y) {
          return x < y ? -1 : 1;
        }
        return 0;
      },
      a);
}
inline bool canonical_feature_less(const feature_ref &a,
                                   const feature_ref &b) {
  return canonical_feature_compare(a, b) < 0;
}

struct digest {
  std::array<std::uint8_t, 16> bytes{{}};
  bool operator==(const digest &o) const { return bytes == o.bytes; }
  bool operator!=(const digest &o) const { return !(*this == o); }
  bool operator<(const digest &o) const { return bytes < o.bytes; }
  std::string hex() const;
};
struct replay_metadata {
  std::uint16_t schema = replay_schema_version,
                engine_major = engine_version_major;
  digest setup_digest;
  bool provisional = true;
};
struct boolean_error {
  boolean_error_code code = boolean_error_code::internal_invariant_error;
  std::uint32_t subcode = 0;
  boolean_stage stage = boolean_stage::context_setup;
  std::vector<feature_ref> features;
  std::uint64_t requested = 0, limit = 0, current = 0;
  std::string message_key, detail;
  replay_metadata replay;
  std::vector<std::uint8_t> replay_payload;
};
boolean_error make_error(boolean_error_code, boolean_stage, std::string,
                         std::uint32_t = 0);
std::string render_error(const boolean_error &);

template <class V> class status_or {
  std::variant<V, boolean_error> data_;

public:
  status_or(V v) : data_(std::move(v)) {}
  status_or(boolean_error e) : data_(std::move(e)) {}
  bool has_value() const { return std::holds_alternative<V>(data_); }
  V *value_if() { return std::get_if<V>(&data_); }
  const V *value_if() const { return std::get_if<V>(&data_); }
  boolean_error *error_if() { return std::get_if<boolean_error>(&data_); }
  const boolean_error *error_if() const {
    return std::get_if<boolean_error>(&data_);
  }
  V &value() {
    if (auto p = value_if())
      return *p;
    assert(false);
    throw std::logic_error("status_or has error");
  }
  const V &value() const {
    if (auto p = value_if())
      return *p;
    assert(false);
    throw std::logic_error("status_or has error");
  }
  boolean_error &error() {
    if (auto p = error_if())
      return *p;
    assert(false);
    throw std::logic_error("status_or has value");
  }
  const boolean_error &error() const {
    if (auto p = error_if())
      return *p;
    assert(false);
    throw std::logic_error("status_or has value");
  }
};

class canonical_encoder {
  std::vector<std::uint8_t> bytes_;

public:
  void byte(std::uint8_t v) { bytes_.push_back(v); }
  void boolean(bool v) { byte(v ? 1 : 0); }
  void u16(std::uint16_t);
  void u32(std::uint32_t);
  void u64(std::uint64_t);
  void signed_magnitude(std::int64_t);
  void raw(const std::uint8_t *, std::size_t);
  void byte_string(const std::vector<std::uint8_t> &v) {
    u64(v.size());
    raw(v.data(), v.size());
  }
  void string(const std::string &s) {
    u64(s.size());
    raw(reinterpret_cast<const std::uint8_t *>(s.data()), s.size());
  }
  template <class Tag> void id(strong_id<Tag> v) {
    if (!v.valid())
      throw std::invalid_argument("invalid ID");
    u64(v.value_for_debug());
  }
  template <class T> void floating(T v) {
    static_assert(std::is_same<T, float>::value ||
                      std::is_same<T, double>::value,
                  "binary float only");
    typename std::conditional<sizeof(T) == 4, std::uint32_t,
                              std::uint64_t>::type bits = 0;
    std::memcpy(&bits, &v, sizeof(v));
    if constexpr (sizeof(T) == 4)
      u32(bits);
    else
      u64(bits);
  }
  const std::vector<std::uint8_t> &bytes() const { return bytes_; }
};
digest md5_digest(const std::uint8_t *, std::size_t);
digest domain_digest(const std::array<char, 8> &,
                     const std::vector<std::uint8_t> &);

struct execution_policy {
  std::uint32_t max_threads = 1, max_queued_tasks = 1024;
};
struct tracing_policy {
  trace_level level = trace_level::failures;
  bool collect_noncanonical_timings = false;
};
struct classification_policy {
  std::uint16_t schema = 1;
  std::uint16_t probe_formula_version = 1;
  classification_strategy strategy =
      classification_strategy::independent_patch_side_v1;
};
struct realization_policy {
  std::uint16_t schema = 2, solver_version = 2;
  realization_semantics semantics = realization_semantics::exact_in_T;
  realization_strategy strategy = realization_strategy::nearest_only;
  original_coordinate_policy original_coordinates =
      original_coordinate_policy::preserve_bits;
  realization_topology_policy topology =
      realization_topology_policy::triangulated_v1;
  pair_certification_policy pair_certification =
      pair_certification_policy::conservative_domain_aabb_v1;
  realization_certificate_level certificate_level =
      realization_certificate_level::full;
  std::uint32_t neighboring_value_radius = 0;
};
struct diagnostic_policy {
  bool forward_to_ygor_logger = false;
};
struct output_policy {
  std::uint16_t schema = 1, ordering_version = 1, encoding_version = 1;
  output_topology_policy topology =
      output_topology_policy::triangulated_v1_no_simplification;
  bool include_compact_provenance = false;
};
struct resource_limit {
  bool unlimited = true;
  std::uint64_t value = 0;
};
struct resource_policy {
  resource_limit authoritative_bytes, stage_private_bytes, work_units,
      entities_per_store, candidates, raw_events, symbolic_vertices,
      symbolic_curves, local_vertices, local_atomic_edges, local_halfedges,
      local_boundary_walks, local_faces, local_patches,
      local_certificate_entries, reconciliation_requests, successor_generations,
      global_vertices, global_atomic_edges, global_halfedges, global_patches,
      source_sheet_members, sheet_uses, seams, seam_sectors,
      source_edge_sectors, coincident_memberships, side_nodes,
      vertex_occurrences, vertex_sectors, link_rays, link_arcs, link_regions,
      side_transitions, probe_descriptors, mapping_entries,
      arrangement_certificate_entries, planar_scratch, radial_scratch,
      link_scratch, cells, classification_transitions, seed_certificates,
      patch_side_labels, propagation_records, selection_decisions,
      selected_patches, selected_cycles, selected_halfedges, selected_edges,
      selected_vertices, selected_vertex_occurrences, topology_obstructions,
      selection_provenance, exact_number_bits,
      diagnostic_records, diagnostic_bytes, trace_records, trace_bytes,
       realization_attempts, realization_graph_nodes, realization_graph_edges,
       realization_components, realization_pair_boxes,
       realization_pair_candidates, realization_pair_checks,
       realization_solver_trail, realization_component_transcripts,
       realization_verifier_witnesses, output_vertices, output_faces,
       output_face_indices,
      output_involved_entries, output_components, output_mappings,
      output_certificate_entries, output_canonical_bytes, verifier_work,
      verifier_scratch_bytes, evidence_records, evidence_bytes, report_bytes,
      dependency_nodes, dependency_edges, replay_bytes, minimization_work;
};
struct boolean_options {
  solid_policy solids = solid_policy::outward_oriented_nested_shells;
  determinism_policy determinism = determinism_policy::strict;
  verification_level verification = verification_level::mandatory;
  execution_policy execution;
  tracing_policy tracing;
  classification_policy classification;
  realization_policy realization;
  result_topology_policy result_topology =
      result_topology_policy::closed_embedded_two_manifold;
  output_policy output;
  diagnostic_policy diagnostics;
  resource_policy resources;
};
status_or<std::vector<std::uint8_t>> encode_options(const boolean_options &);
status_or<bool> validate_options(const boolean_options &);

enum class coordinate_tag : std::uint8_t { binary32, binary64 };
enum class index_tag : std::uint8_t { uint32, uint64 };
enum class endian_tag : std::uint8_t { little, big };
enum class rounding_tag : std::uint8_t { nearest };
struct platform_facts {
  coordinate_tag coordinate;
  index_tag index;
  std::uint8_t coordinate_bytes, index_bytes;
  std::uint16_t char_bits;
  endian_tag endian;
  std::uint8_t uint32_bytes, uint64_bytes;
  std::uint32_t radix, digits;
  std::int32_t min_exponent, max_exponent;
  bool iec559, has_subnormals, fast_math, finite_math_only;
  rounding_tag rounding;
};
template <class T, class I>
struct is_supported_boolean_types
    : std::integral_constant<bool,
                             (std::is_same<T, float>::value ||
                              std::is_same<T, double>::value) &&
                                 (std::is_same<I, std::uint32_t>::value ||
                                  std::is_same<I, std::uint64_t>::value)> {};

class cancellation_token;
class cancellation_source {
  std::shared_ptr<std::atomic<bool>> s_ =
      std::make_shared<std::atomic<bool>>(false);

public:
  cancellation_token token() const;
  void cancel() { s_->store(true); }
};
class cancellation_token {
  std::shared_ptr<std::atomic<bool>> s_;
  explicit cancellation_token(std::shared_ptr<std::atomic<bool>> s)
      : s_(std::move(s)) {}
  friend class cancellation_source;

public:
  bool cancelled() const { return s_ && s_->load(); }
};
inline cancellation_token cancellation_source::token() const {
  return cancellation_token(s_);
}
status_or<std::uint64_t>
    checked_add(std::uint64_t, std::uint64_t,
                boolean_stage = boolean_stage::context_setup);
status_or<std::uint64_t>
    checked_multiply(std::uint64_t, std::uint64_t,
                     boolean_stage = boolean_stage::context_setup);
struct checked_cardinality {
  std::uint64_t high = 0, low = 0;
};
template <class I>
constexpr bool index_capacity_accepts(checked_cardinality n) noexcept {
  static_assert(std::is_unsigned<I>::value, "unsigned output index required");
  if constexpr (sizeof(I) == sizeof(std::uint64_t))
    return n.high == 0 || (n.high == 1 && n.low == 0);
  return n.high == 0 &&
         (n.low == 0 || n.low - 1 <= std::numeric_limits<I>::max());
}
template <class I> status_or<I> checked_output_index(std::uint64_t n) {
  static_assert(std::is_unsigned<I>::value, "unsigned output index required");
  if (n > std::numeric_limits<I>::max())
    return make_error(boolean_error_code::index_overflow,
                      boolean_stage::output_assembly, "output_index_overflow");
  return static_cast<I>(n);
}
class resource_accountant;
class resource_reservation {
  resource_accountant *accountant_ = nullptr;
  resource_kind kind_ = resource_kind::work_units;
  std::uint64_t amount_ = 0;
  bool committed_ = false;
  friend class resource_accountant;
  resource_reservation(resource_accountant &, resource_kind, std::uint64_t);

public:
  resource_reservation() = default;
  resource_reservation(const resource_reservation &) = delete;
  resource_reservation &operator=(const resource_reservation &) = delete;
  resource_reservation(resource_reservation &&) noexcept;
  resource_reservation &operator=(resource_reservation &&) noexcept;
  ~resource_reservation();
  void commit() noexcept { committed_ = true; }
  void rollback() noexcept;
  std::uint64_t amount() const noexcept { return amount_; }
};
class resource_accountant {
  resource_policy limits_;
  mutable std::mutex mutex_;
  std::array<std::uint64_t, static_cast<std::size_t>(resource_kind::count)>
      used_{{}};
  resource_limit limit_for(resource_kind) const;

public:
  explicit resource_accountant(resource_policy p) : limits_(std::move(p)) {}
  status_or<bool> reserve(resource_kind, std::uint64_t, boolean_stage);
  status_or<resource_reservation> reserve_scoped(resource_kind, std::uint64_t,
                                                 boolean_stage);
  void release(resource_kind, std::uint64_t);
  std::uint64_t used(resource_kind) const;
};

class artifact_generation_catalog {
  struct entry {
    std::uint64_t generation = 0;
    std::shared_ptr<const void> artifact;
  };
  context_owner_token owner_;
  mutable std::mutex mutex_;
  std::array<entry, 11> latest_{};

public:
  explicit artifact_generation_catalog(context_owner_token owner)
      : owner_(owner) {}
  std::uint64_t latest_generation(artifact_slot) const;
  std::shared_ptr<const void> latest(artifact_slot) const;
  status_or<std::uint64_t>
  compare_and_publish(artifact_slot, std::uint64_t expected_generation,
                      context_owner_token, std::shared_ptr<const void>);
};

enum class invariant_code : std::uint32_t {
  input_binding = 0x020001,
  input_coordinates = 0x020002,
  input_rings = 0x020003,
  input_facets = 0x020004,
  input_edges = 0x020005,
  input_vertex_links = 0x020006,
  input_shells = 0x020007,
  input_canonical_encoding = 0x020008,
  broad_phase_binding = 0x040001,
  broad_phase_bounds = 0x040002,
  broad_phase_candidates = 0x040003,
  broad_phase_canonical_encoding = 0x040004,
  event_binding = 0x050001,
  event_ledger = 0x050002,
  event_geometry = 0x050003,
  event_canonical_encoding = 0x050004,
  symbolic_binding = 0x060001,
  symbolic_identity = 0x060002,
  symbolic_order = 0x060003,
  symbolic_canonical_encoding = 0x060004,
  local_binding = 0x070001,
  local_constraints = 0x070002,
  local_dcel = 0x070003,
  local_coverage = 0x070004,
  local_canonical_encoding = 0x070005,
  arrangement_binding = 0x080001,
  arrangement_maps = 0x080002,
  arrangement_cycles = 0x080003,
  arrangement_seams = 0x080004,
  arrangement_coincidence = 0x080005,
  arrangement_side_graph = 0x080006,
  arrangement_canonical_encoding = 0x080007,
  arrangement_occurrences = 0x080008,
  arrangement_vertex_links = 0x080009,
  arrangement_open_probes = 0x08000a,
  classification_binding = 0x090001,
  classification_regions = 0x090002,
  classification_transfers = 0x090003,
  classification_side_labels = 0x090004,
  classification_canonical_encoding = 0x090005,
  selection_binding = 0x0a0001,
  selection_decisions = 0x0a0002,
  selection_topology = 0x0a0003,
  selection_orientation = 0x0a0004,
  selection_canonical_encoding = 0x0a0005,
  realization_binding = 0x0b0001,
  realization_coordinates = 0x0b0002,
  realization_triangulation = 0x0b0003,
  realization_domains = 0x0b0004,
  realization_obligations = 0x0b0005,
  realization_embedding = 0x0b0006,
  realization_search = 0x0b0007,
  realization_canonical_encoding = 0x0b0008,
  output_binding = 0x0c0001,
  output_structure = 0x0c0002,
  output_coordinates = 0x0c0003,
  output_topology = 0x0c0004,
  output_mappings = 0x0c0005,
  output_certificate = 0x0c0006,
  output_canonical_encoding = 0x0c0007,
  verification_binding = 0x0d0001,
  verification_digest = 0x0d0002
};
enum class verification_outcome : std::uint8_t {
  pass,
  invariant_failure,
  malformed_artifact,
  stale_evidence,
  resource_limit,
  verifier_defect
};
enum class check_status : std::uint8_t {
  passed,
  failed,
  not_run_due_to_prior_failure
};
struct invariant_result {
  invariant_code code = invariant_code::verification_binding;
  check_status status = check_status::not_run_due_to_prior_failure;
  std::vector<feature_ref> entities;
  std::uint32_t subcode = 0;
};
enum class evidence_kind : std::uint8_t {
  binding,
  raw_scan,
  exact_relation,
  topology,
  coverage,
  canonical_encoding
};
struct evidence_record {
  std::uint16_t schema = 1;
  evidence_kind kind = evidence_kind::binding;
  invariant_code invariant = invariant_code::verification_binding;
  std::vector<feature_ref> entities;
  std::vector<std::uint8_t> exact_payload;
  std::vector<digest> dependencies;
  digest evidence_digest;
};
struct replay_seed {
  std::uint16_t schema = 1;
  artifact_slot slot = artifact_slot::validated_operands;
  digest artifact_digest;
  std::vector<std::uint8_t> payload;
  digest seed_digest;
};
struct verification_spec {
  std::uint16_t schema = 1, checker_version = 1;
  verification_level level = verification_level::mandatory;
  artifact_slot slot = artifact_slot::validated_operands;
  std::uint64_t artifact_type_tag = 0;
  std::uint16_t artifact_schema = 1;
  std::vector<invariant_code> required_invariants;
  digest invariant_set_digest;
};
struct verification_report {
  std::uint16_t schema = 1, checker_version = 1;
  context_owner_token owner;
  boolean_stage stage = boolean_stage::context_setup;
  artifact_slot slot = artifact_slot::validated_operands;
  std::uint64_t artifact_type_tag = 0;
  std::uint16_t artifact_schema = 1;
  digest setup_digest, artifact_digest, invariant_set_digest, report_digest;
  verification_outcome outcome = verification_outcome::verifier_defect;
  std::vector<invariant_result> results;
  std::vector<evidence_record> evidence;
  std::vector<digest> dependency_digests;
  std::optional<replay_seed> replay;
  bool passed() const noexcept { return outcome == verification_outcome::pass; }
};
struct artifact_view {
  context_owner_token owner;
  artifact_slot slot;
  std::uint64_t artifact_type_tag;
  std::uint16_t artifact_schema = 1;
  std::uint64_t generation = 1;
  digest artifact_digest;
  std::shared_ptr<const void> lifetime;
  const void *payload = nullptr;
};
struct raw_operand_view {
  coordinate_tag coordinate;
  index_tag index;
  const void *operand_a = nullptr;
  const void *operand_b = nullptr;
};
struct context_preparation_provenance {
  digest input_digest;
  digest prepared_digest;
  digest policy_digest;
  digest report_digest;
  bool normalized = false;
  bool geometry_changed = false;
};
inline bool operator==(const context_preparation_provenance &a,
                       const context_preparation_provenance &b) noexcept {
  return a.input_digest == b.input_digest &&
         a.prepared_digest == b.prepared_digest &&
         a.policy_digest == b.policy_digest &&
         a.report_digest == b.report_digest &&
         a.normalized == b.normalized &&
         a.geometry_changed == b.geometry_changed;
}
inline bool operator!=(const context_preparation_provenance &a,
                       const context_preparation_provenance &b) noexcept {
  return !(a == b);
}
struct verification_environment_view {
  context_owner_token owner;
  digest setup_digest;
  operation op = operation::regularized_union;
  const boolean_options *options = nullptr;
  coordinate_tag coordinate = coordinate_tag::binary32;
  index_tag index = index_tag::uint32;
  const void *exact_kernel = nullptr;
  raw_operand_view raw_operands;
  resource_accountant *accountant = nullptr;
  std::function<bool()> cancelled;
};
class verifier_service {
public:
  virtual ~verifier_service() = default;
  virtual status_or<verification_report>
  verify(const artifact_view &, const verification_spec &,
         const verification_environment_view &) const noexcept = 0;
};
template <class T> class exact_kernel_services {
public:
  virtual ~exact_kernel_services() = default;
  virtual coordinate_tag coordinate_type() const noexcept = 0;
  virtual std::vector<std::uint8_t> arithmetic_policy_bytes() const = 0;
  virtual std::uint64_t implementation_type_tag() const noexcept = 0;
};
using diagnostic_consumer = std::function<void(const std::string &)>;
struct replay_descriptor {
  digest input_a, input_b, setup;
  std::uint64_t input_a_bytes = 0, input_b_bytes = 0;
  operation op;
  boolean_options options;
  platform_facts platform;
};
class deterministic_executor;
using deterministic_executor_factory =
    std::function<std::unique_ptr<deterministic_executor>(
        const execution_policy &)>;
class performance_collector;
template <class T, class I> class prepared_operand;
struct normalization_report;
template <class T, class I> class boolean_context {
  static_assert(is_supported_boolean_types<T, I>::value,
                "unsupported Boolean types");
  const fv_surface_mesh<T, I> *a_;
  const fv_surface_mesh<T, I> *b_;
  operation_contract contract_;
  boolean_options options_;
  platform_facts platform_;
  replay_descriptor replay_;
  context_owner_token owner_;
  std::shared_ptr<const exact_kernel_services<T>> kernel_;
  std::shared_ptr<const verifier_service> verifiers_;
  std::unique_ptr<deterministic_executor> executor_;
  resource_accountant accountant_;
  artifact_generation_catalog artifacts_;
  cancellation_source internal_cancel_;
  cancellation_source *caller_cancel_;
  diagnostic_consumer consumer_;
  std::shared_ptr<performance_collector> performance_;
  std::array<std::shared_ptr<const void>, 2> input_lifetimes_;
  std::array<std::shared_ptr<const fv_surface_mesh<T, I>>, 2>
      attribute_source_meshes_;
  std::array<std::shared_ptr<const normalization_report>, 2>
      normalization_reports_;
  std::optional<context_preparation_provenance> preparation_provenance_;
  boolean_context(const fv_surface_mesh<T, I> &, const fv_surface_mesh<T, I> &,
                  operation, boolean_options, platform_facts, replay_descriptor,
                  context_owner_token,
                  std::shared_ptr<const exact_kernel_services<T>>,
                  std::shared_ptr<const verifier_service>,
                  cancellation_source *, diagnostic_consumer,
                  deterministic_executor_factory);
  template <class U, class J,
            typename std::enable_if<is_supported_boolean_types<U, J>::value,
                                    int>::type>
  friend status_or<std::unique_ptr<boolean_context<U, J>>> make_boolean_context(
      const fv_surface_mesh<U, J> &, const fv_surface_mesh<U, J> &, operation,
      const boolean_options &, std::shared_ptr<const exact_kernel_services<U>>,
      std::shared_ptr<const verifier_service>, cancellation_source *,
      diagnostic_consumer, deterministic_executor_factory);
  template <class U, class J>
  friend status_or<std::unique_ptr<boolean_context<U, J>>> make_boolean_context(
      const prepared_operand<U, J> &, const prepared_operand<U, J> &,
      operation, const boolean_options &,
      std::shared_ptr<const exact_kernel_services<U>>,
      std::shared_ptr<const verifier_service>, cancellation_source *,
      diagnostic_consumer, deterministic_executor_factory);

public:
  ~boolean_context();
  boolean_context(const boolean_context &) = delete;
  const operation_contract &contract() const { return contract_; }
  const boolean_options &options() const { return options_; }
  const platform_facts &platform() const { return platform_; }
  const replay_descriptor &replay() const { return replay_; }
  const std::optional<context_preparation_provenance> &
  preparation_provenance() const noexcept {
    return preparation_provenance_;
  }
  context_owner_token owner() const { return owner_; }
  const fv_surface_mesh<T, I> &operand_a_mesh() const { return *a_; }
  const fv_surface_mesh<T, I> &operand_b_mesh() const { return *b_; }
  const fv_surface_mesh<T, I> &attribute_source_mesh(operand_id operand) const {
    const auto role = static_cast<std::size_t>(operand.value_for_debug());
    if (role > 1)
      throw std::out_of_range("attribute source operand");
    if (attribute_source_meshes_[role])
      return *attribute_source_meshes_[role];
    return role == 0 ? *a_ : *b_;
  }
  const normalization_report *normalization_report_for(
      operand_id operand) const noexcept {
    const auto role = static_cast<std::size_t>(operand.value_for_debug());
    return role < 2 ? normalization_reports_[role].get() : nullptr;
  }
  const exact_kernel_services<T> &kernel() const { return *kernel_; }
  const verifier_service &verifiers() const { return *verifiers_; }
  deterministic_executor &executor() { return *executor_; }
  resource_accountant &accountant() { return accountant_; }
  artifact_generation_catalog &artifacts() { return artifacts_; }
  const artifact_generation_catalog &artifacts() const { return artifacts_; }
  std::shared_ptr<const performance_snapshot> performance() const;
  performance_collector *performance_collector_for_internal_use() const {
    return performance_.get();
  }
  bool cancelled() const {
    return internal_cancel_.token().cancelled() ||
           (caller_cancel_ && caller_cancel_->token().cancelled());
  }
};
template <class T, class I,
          typename std::enable_if<is_supported_boolean_types<T, I>::value,
                                  int>::type = 0>
status_or<std::unique_ptr<boolean_context<T, I>>> make_boolean_context(
    const fv_surface_mesh<T, I> &, const fv_surface_mesh<T, I> &, operation,
    const boolean_options &, std::shared_ptr<const exact_kernel_services<T>>,
    std::shared_ptr<const verifier_service>, cancellation_source * = nullptr,
    diagnostic_consumer = {}, deterministic_executor_factory = {});
struct output_assembly_certificate;
struct output_summary {
  std::uint64_t vertices = 0, faces = 0, components = 0, face_indices = 0,
                involved_face_entries = 0;
  digest semantic_digest;
};
template <class T, class I> struct boolean_success {
  fv_surface_mesh<T, I> mesh;
  operation selected_operation = operation::regularized_union;
  output_policy policy;
  digest input_a_digest, input_b_digest, selected_boundary_digest,
      realized_boundary_digest, canonical_output_digest;
  output_summary summary;
  std::shared_ptr<const output_assembly_certificate> certificate;
  std::vector<std::uint8_t> compact_provenance;
};
template <class T, class I>
using boolean_result = status_or<std::shared_ptr<const boolean_success<T, I>>>;

} // namespace mesh_boolean
} // namespace ygor
#endif
