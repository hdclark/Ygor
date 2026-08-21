#pragma once

#include "CanonicalBytes.h"
#include "ContractVersions.h"
#include "Identity.h"
#include "Policies.h"
#include "Resources.h"
#include "Sha256.h"

#include <array>
#include <cstdint>
#include <limits>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

inline constexpr char output_triangulation_provider_identity_v1[] =
    "bounded_indexed_ear_decomposition_v1";
inline constexpr char triangulation_projection_provider_identity_v1[] =
    "authoritative_support_frame_projection_v1";
inline constexpr char triangulation_region_model_provider_identity_v1[] =
    "occurrence_preserving_constraint_model_v1";
inline constexpr char triangulation_predicate_provider_identity_v1[] =
    "component03_truth_layers_planar_relations_v2";
inline constexpr char triangulation_contour_provider_identity_v1[] =
    "canonical_complete_visibility_bridge_v1";
inline constexpr char triangulation_escalation_provider_identity_v1[] =
    "neighbor_chain_truth_layer_escalation_v2";
inline constexpr char triangulation_diagonal_provider_identity_v1[] =
    "atomic_pending_twin_diagonal_v1";
inline constexpr char triangulation_degeneracy_provider_identity_v1[] =
    "length_certificate_degenerate_cell_handoff_v2";
inline constexpr char triangulation_overlap_provider_identity_v1[] =
    "region_rank_interval_aabb_index_v1";
inline constexpr char triangulation_coverage_provider_identity_v1[] =
    "boundary_euler_area_nonoverlap_v1";
inline constexpr char triangulation_verification_provider_identity_v1[] =
    "independent_truth_layer_cell_rebuild_v2";

inline constexpr std::uint64_t triangulation_invalid_ordinal =
    std::numeric_limits<std::uint64_t>::max();

// ---------------------------------------------------------------------------
// Strong identity domains.
// ---------------------------------------------------------------------------
struct triangulation_region_tag;
struct support_frame_record_tag;
struct projected_occurrence_tag;
struct planar_predicate_evidence_tag;
struct orientation_escalation_tag;
struct internal_diagonal_tag;
struct triangulation_halfedge_tag;
struct output_triangle_tag;
struct triangle_corner_ref_tag;
struct triangle_edge_use_ref_tag;
struct residual_cell_tag;
struct cleanup_obligation_tag;
struct boundary_assignment_tag;
struct coverage_certificate_tag;
struct cleanup_handoff_length_certificate_tag;

using triangulation_region_id = strong_id<triangulation_region_tag>;
using support_frame_record_id = strong_id<support_frame_record_tag>;
using projected_occurrence_id = strong_id<projected_occurrence_tag>;
using planar_predicate_evidence_id = strong_id<planar_predicate_evidence_tag>;
using orientation_escalation_id = strong_id<orientation_escalation_tag>;
using internal_diagonal_id = strong_id<internal_diagonal_tag>;
using triangulation_halfedge_id = strong_id<triangulation_halfedge_tag>;
using output_triangle_id = strong_id<output_triangle_tag>;
using triangle_corner_ref_id = strong_id<triangle_corner_ref_tag>;
using triangle_edge_use_ref_id = strong_id<triangle_edge_use_ref_tag>;
using residual_cell_id = strong_id<residual_cell_tag>;
using cleanup_obligation_id = strong_id<cleanup_obligation_tag>;
using boundary_assignment_id = strong_id<boundary_assignment_tag>;
using coverage_certificate_id = strong_id<coverage_certificate_tag>;
using cleanup_handoff_length_certificate_id =
    strong_id<cleanup_handoff_length_certificate_tag>;

// ---------------------------------------------------------------------------
// Closed enums.
// ---------------------------------------------------------------------------
enum class output_triangulation_provider_kind : std::uint8_t {
  bounded_indexed_ear_decomposition_v1 = 1,
};

enum class triangulation_verification_disposition : std::uint8_t {
  not_verified = 0,
  independently_verified = 1,
};

enum class support_frame_disposition : std::uint8_t {
  inherited_authoritative = 1,
  canonical_source_facet_fallback = 2,
  invalid = 3,
};

enum class constraint_role : std::uint8_t {
  component11_boundary = 1,
  selected_bridge = 2,
  generated_diagonal = 3,
  residual_only_edge = 4,
  invalid = 5,
};

enum class predicate_query_kind : std::uint8_t {
  orientation = 1,
  point_in_triangle = 2,
  segment_relation = 3,
  polygon_area = 4,
  invalid = 5,
};

enum class planar_predicate_disposition : std::uint8_t {
  definite_positive = 1,
  definite_negative = 2,
  exact_zero = 3,
  uncertain = 4,
  invalid = 5,
};

enum class truth_layer_disposition : std::uint8_t {
  definite = 1,
  exact_tie = 2,
  uncertain = 3,
  invalid = 4,
};

enum class triangle_geometric_category : std::uint8_t {
  definite_positive_area = 1,
  positive_area_within_cleanup_margin = 2,
  zero_area_cleanup_required = 3,
  symbolic_exact_tie_cleanup_required = 4,
  invalid = 5,
};

enum class diagonal_state : std::uint8_t {
  both_triangle_sides_assigned = 1,
  invalid = 2,
};

enum class boundary_assignment_kind : std::uint8_t {
  triangle = 1,
  residual_patch = 2,
  invalid = 3,
};

enum class coverage_disposition : std::uint8_t {
  verified = 1,
  empty_verified = 2,
  residual_only_verified = 3,
  mismatch = 4,
  invalid = 5,
};

enum class cleanup_length_metric : std::uint8_t {
  minimum_altitude = 1,
  endpoint_separation = 2,
  support_residual = 3,
  invalid = 4,
};

enum class physical_dimension : std::uint8_t {
  length = 1,
  invalid = 2,
};

enum class output_triangulation_checkpoint : std::uint32_t {
  context_capability_validation = 1,
  predecessor_validation = 2,
  count_preflight = 3,
  resource_reservation = 4,
  region_task_creation = 5,
  support_frame_selection = 6,
  projected_occurrence_construction = 7,
  contour_construction = 8,
  contour_role_audit = 9,
  bridge_enumeration = 10,
  bridge_allocation = 11,
  ear_decomposition = 12,
  orientation_escalation = 13,
  diagonal_allocation = 14,
  region_completion = 15,
  region_coverage = 16,
  region_finalization = 17,
  canonical_merge = 18,
  dense_id_assignment = 19,
  boundary_preservation_audit = 20,
  producer_checks = 21,
  canonical_encoding = 22,
  independent_verification = 23,
  resource_reconciliation = 24,
  commit = 25,
};

enum class output_triangulation_subcode : std::uint32_t {
  unsupported_version = 120001,
  wrong_owner = 120002,
  wrong_operation = 120003,
  predecessor_digest_mismatch = 120004,
  predecessor_not_verified = 120005,
  invalid_region_range = 120006,
  boundary_pair_contradiction = 120007,
  unsupported_contour_hierarchy = 120008,
  support_frame_unavailable = 120009,
  projection_non_finite = 120010,
  projection_ill_conditioned = 120011,
  unresolved_contour_topology = 120012,
  bridge_candidate_exhausted = 120013,
  orientation_escalation_exhausted = 120014,
  inverted_required_triangle = 120015,
  ear_candidate_stalled = 120016,
  diagonal_pair_failure = 120017,
  boundary_unassigned = 120018,
  boundary_multiply_assigned = 120019,
  unsupported_residual_form = 120020,
  triangle_closure_failure = 120021,
  connectivity_mismatch = 120022,
  euler_mismatch = 120023,
  area_mismatch = 120024,
  forbidden_overlap = 120025,
  coverage_missing_pocket = 120026,
  hidden_coordinate_change = 120027,
  canonicalization_failure = 120028,
  codec_error = 120029,
  digest_mismatch = 120030,
  verifier_rejection = 120031,
  count_overflow = 120032,
  resource_preflight = 120033,
  cancelled = 120034,
  internal_invariant = 120035,
  strict_profile_mismatch = 120036,
};

inline bounded_boolean_error output_triangulation_error(
    output_triangulation_subcode subcode,
    bounded_boolean_error_category category, const char *summary,
    output_triangulation_checkpoint checkpoint) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 12;
  error.stage = static_cast<std::uint16_t>(stage_id::output_topology);
  error.checkpoint = static_cast<std::uint32_t>(checkpoint);
  error.summary = summary;
  return error;
}

struct output_triangulation_cancellation_observer final {
  std::uint16_t version = contract_versions::output_triangulation_verifier;
  std::uint16_t reserved16 = 0;
  void *state = nullptr;
  void (*poll)(void *, output_triangulation_checkpoint) noexcept = nullptr;
  std::uint32_t reserved32 = 0;
};

struct output_triangulation_capabilities final {
  std::uint16_t provider_version = contract_versions::output_triangulation_provider;
  std::uint16_t codec_version = contract_versions::output_triangulation_codec;
  std::uint16_t verifier_version = contract_versions::output_triangulation_verifier;
  context_owner_token owner{};
  const bounded_boolean_cancellation_token *cancellation = nullptr;
  const output_triangulation_cancellation_observer *cancellation_observer =
      nullptr;
  resource_manager *resources = nullptr;
  std::uint64_t maximum_regions = (std::uint64_t{1} << 34);
  std::uint64_t maximum_projected_occurrences = (std::uint64_t{1} << 34);
  std::uint64_t maximum_triangles = (std::uint64_t{1} << 37);
  std::uint64_t maximum_diagonals = (std::uint64_t{1} << 36);
  std::uint64_t maximum_internal_halfedges = (std::uint64_t{1} << 37);
  std::uint64_t maximum_predicate_evidence = (std::uint64_t{1} << 38);
  std::uint64_t maximum_canonical_bytes = (std::uint64_t{1} << 35);
  std::uint64_t maximum_work_units = (std::uint64_t{1} << 40);
  std::uint32_t reserved = 0;
};

struct output_triangulation_codec_limits final {
  std::uint64_t maximum_section_bytes = (std::uint64_t{1} << 34);
  std::uint64_t reserved = 0;
};

inline bool output_triangulation_cancelled(
    const output_triangulation_capabilities &capabilities,
    output_triangulation_checkpoint checkpoint) noexcept {
  if (capabilities.cancellation_observer &&
      capabilities.cancellation_observer->poll)
    capabilities.cancellation_observer->poll(
        capabilities.cancellation_observer->state, checkpoint);
  return capabilities.cancellation &&
         capabilities.cancellation->cancellation_requested();
}

struct output_triangulation_statistics final {
  std::uint64_t region_count = 0;
  std::uint64_t support_frame_count = 0;
  std::uint64_t projected_occurrence_count = 0;
  std::uint64_t predicate_evidence_count = 0;
  std::uint64_t internal_diagonal_count = 0;
  std::uint64_t internal_halfedge_count = 0;
  std::uint64_t triangle_count = 0;
  std::uint64_t corner_ref_count = 0;
  std::uint64_t edge_use_ref_count = 0;
  std::uint64_t residual_cell_count = 0;
  std::uint64_t cleanup_obligation_count = 0;
  std::uint64_t boundary_assignment_count = 0;
  std::uint64_t coverage_certificate_count = 0;
  std::uint64_t verifier_work_units = 0;
  std::uint64_t persistent_bytes = 0;
  std::uint64_t canonical_bytes = 0;
};

// ---------------------------------------------------------------------------
// Canonical identity keys.
// ---------------------------------------------------------------------------
struct triangulation_region_key final {
  std::uint64_t component11_region = 0;
  std::uint16_t schema_version =
      contract_versions::output_triangulation_artifact_schema;

  friend bool operator<(const triangulation_region_key &a,
                        const triangulation_region_key &b) noexcept {
    return std::tie(a.component11_region, a.schema_version) <
           std::tie(b.component11_region, b.schema_version);
  }
  friend bool operator==(const triangulation_region_key &a,
                         const triangulation_region_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

struct projected_occurrence_key final {
  std::uint64_t region = 0;
  std::uint64_t component11_occurrence = 0;
  std::uint16_t schema_version =
      contract_versions::output_triangulation_artifact_schema;

  friend bool operator<(const projected_occurrence_key &a,
                        const projected_occurrence_key &b) noexcept {
    return std::tie(a.region, a.component11_occurrence, a.schema_version) <
           std::tie(b.region, b.component11_occurrence, b.schema_version);
  }
  friend bool operator==(const projected_occurrence_key &a,
                         const projected_occurrence_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

struct internal_diagonal_key final {
  std::uint64_t region = 0;
  std::array<std::uint64_t, 2> endpoints{triangulation_invalid_ordinal,
                                         triangulation_invalid_ordinal};
  std::uint16_t schema_version =
      contract_versions::output_triangulation_artifact_schema;

  static internal_diagonal_key canonical(std::uint64_t region, std::uint64_t a,
                                         std::uint64_t b) {
    internal_diagonal_key key;
    key.region = region;
    key.endpoints[0] = std::min(a, b);
    key.endpoints[1] = std::max(a, b);
    return key;
  }

  friend bool operator<(const internal_diagonal_key &a,
                        const internal_diagonal_key &b) noexcept {
    return std::tie(a.region, a.endpoints, a.schema_version) <
           std::tie(b.region, b.endpoints, b.schema_version);
  }
  friend bool operator==(const internal_diagonal_key &a,
                         const internal_diagonal_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

struct output_triangle_key final {
  std::uint64_t region = 0;
  std::array<std::uint64_t, 3> corners{};
  std::uint16_t schema_version =
      contract_versions::output_triangulation_artifact_schema;

  // Orientation-preserving least rotation of the three corners.
  static output_triangle_key canonical(std::uint64_t region,
                                       std::array<std::uint64_t, 3> corners) {
    output_triangle_key key;
    key.region = region;
    std::size_t best = 0;
    for (std::size_t i = 1; i < 3; ++i) {
      const std::array<std::uint64_t, 3> rotated{
          corners[i], corners[(i + 1) % 3], corners[(i + 2) % 3]};
      const std::array<std::uint64_t, 3> best_rot{
          corners[best], corners[(best + 1) % 3], corners[(best + 2) % 3]};
      if (rotated < best_rot)
        best = i;
    }
    key.corners = {corners[best], corners[(best + 1) % 3],
                   corners[(best + 2) % 3]};
    return key;
  }

  friend bool operator<(const output_triangle_key &a,
                        const output_triangle_key &b) noexcept {
    return std::tie(a.region, a.corners, a.schema_version) <
           std::tie(b.region, b.corners, b.schema_version);
  }
  friend bool operator==(const output_triangle_key &a,
                         const output_triangle_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

// ---------------------------------------------------------------------------
// Immutable artifact records.
// ---------------------------------------------------------------------------
struct support_frame_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t region = triangulation_invalid_ordinal;
  support_frame_disposition disposition =
      support_frame_disposition::invalid;
  operand_id source_operand = operand_id::a;
  std::uint64_t source_facet = 0;
  std::uint8_t dropped_axis = 0;
};

struct projected_occurrence_record final {
  std::uint64_t canonical_id = 0;
  projected_occurrence_key key{};
  std::uint64_t region = triangulation_invalid_ordinal;
  std::uint64_t component11_occurrence = triangulation_invalid_ordinal;
  std::array<std::uint64_t, 2> nominal_bits{};
  std::array<std::uint64_t, 2> lower_bits{};
  std::array<std::uint64_t, 2> upper_bits{};
};

// Three-layer planar predicate evidence: rounded nominal, exact relation, and
// uncertainty enclosure are kept separate and never collapsed.
struct planar_predicate_evidence_record final {
  std::uint64_t canonical_id = 0;
  predicate_query_kind kind = predicate_query_kind::invalid;
  std::uint64_t region = triangulation_invalid_ordinal;
  std::array<std::uint64_t, 4> operands{triangulation_invalid_ordinal,
                                        triangulation_invalid_ordinal,
                                        triangulation_invalid_ordinal,
                                        triangulation_invalid_ordinal};
  std::uint8_t operand_count = 0;
  std::int8_t exact_sign = 0;      // exact relation sign: -1, 0, +1, 2=unavailable
  std::array<std::uint64_t, 2> enclosure_lower_bits{};
  std::array<std::uint64_t, 2> enclosure_upper_bits{};
  planar_predicate_disposition disposition = planar_predicate_disposition::invalid;
};

struct orientation_escalation_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t region = triangulation_invalid_ordinal;
  std::uint64_t corner_occurrence = triangulation_invalid_ordinal;
  std::uint8_t steps = 0;
  planar_predicate_disposition terminal = planar_predicate_disposition::invalid;
};

struct internal_diagonal_record final {
  std::uint64_t canonical_id = 0;
  internal_diagonal_key key{};
  std::uint64_t region = triangulation_invalid_ordinal;
  std::array<std::uint64_t, 2> halfedges{triangulation_invalid_ordinal,
                                         triangulation_invalid_ordinal};
  std::array<std::uint64_t, 2> endpoints{triangulation_invalid_ordinal,
                                         triangulation_invalid_ordinal};
  diagonal_state state = diagonal_state::invalid;
};

struct triangulation_halfedge_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t pair = triangulation_invalid_ordinal;
  std::uint64_t origin = triangulation_invalid_ordinal;
  std::uint64_t destination = triangulation_invalid_ordinal;
  std::uint64_t region = triangulation_invalid_ordinal;
  std::uint64_t diagonal = triangulation_invalid_ordinal;
};

struct output_triangle_record final {
  std::uint64_t canonical_id = 0;
  output_triangle_key key{};
  std::uint64_t region = triangulation_invalid_ordinal;
  std::array<std::uint64_t, 3> corners{};
  std::array<std::uint64_t, 3> corner_refs{};
  std::array<std::uint64_t, 3> edge_use_refs{};
  triangle_geometric_category category =
      triangle_geometric_category::invalid;
};

struct triangle_corner_ref_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t triangle = triangulation_invalid_ordinal;
  std::uint64_t occurrence = triangulation_invalid_ordinal;
};

struct triangle_edge_use_ref_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t triangle = triangulation_invalid_ordinal;
  bool is_boundary = false;
  std::uint64_t halfedge = triangulation_invalid_ordinal;
};

struct boundary_assignment_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t component11_halfedge = triangulation_invalid_ordinal;
  std::uint64_t triangle = triangulation_invalid_ordinal;
  boundary_assignment_kind kind = boundary_assignment_kind::invalid;
};

struct cleanup_handoff_length_certificate_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t triangle = triangulation_invalid_ordinal;
  cleanup_length_metric metric = cleanup_length_metric::invalid;
  physical_dimension dimension = physical_dimension::invalid;
  std::array<std::uint64_t, 2> length_lower_bits{};
  std::array<std::uint64_t, 2> length_upper_bits{};
  bool budget_reserved = false;
  bool budget_committed = false;
};

struct coverage_certificate_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t region = triangulation_invalid_ordinal;
  coverage_disposition disposition = coverage_disposition::invalid;
  std::uint64_t vertex_count = 0;
  std::uint64_t edge_count = 0;
  std::uint64_t face_count = 0;
  std::uint64_t hole_count = 0;
  std::uint64_t triangle_begin = 0;
  std::uint64_t triangle_count = 0;
  std::uint64_t diagonal_begin = 0;
  std::uint64_t diagonal_count = 0;
};

} // namespace ygor::mesh_boolean::bounded
