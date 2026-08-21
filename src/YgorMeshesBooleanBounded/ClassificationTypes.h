#pragma once

#include "CanonicalBytes.h"
#include "ContractVersions.h"
#include "Identity.h"
#include "Resources.h"
#include "Sha256.h"

#include <array>
#include <cstdint>
#include <limits>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

inline constexpr char classification_provider_identity_v1[] =
    "lineage_surface_arrangement_v1";
inline constexpr char triangle_arrangement_provider_identity_v1[] =
    "authoritative_segment_rotation_system_v1";
inline constexpr char facet_atom_provider_identity_v1[] =
    "transparent_internal_diagonal_cells_v1";
inline constexpr char adjacency_provider_identity_v1[] =
    "descriptor_gated_surface_adjacency_v1";
inline constexpr char grouping_provider_identity_v1[] =
    "canonical_zero_delta_components_v1";
inline constexpr char quotient_provider_identity_v1[] =
    "canonical_signed_constraint_graph_v1";
inline constexpr char atom_witness_provider_identity_v1[] =
    "certified_local_cell_witness_v1";
inline constexpr char shell_query_provider_identity_v1[] =
    "bounded_shell_query_provider_v1";
inline constexpr char shell_query_direction_policy_identity_v1[] =
    "canonical_primitive_integer_directions_v1";
inline constexpr char propagation_provider_identity_v1[] =
    "canonical_spanning_forest_potential_v1";
inline constexpr char symbolic_side_provider_identity_v1[] =
    "lineage_symbolic_side_projection_v1";

inline constexpr std::uint64_t classification_invalid_ordinal =
    std::numeric_limits<std::uint64_t>::max();

struct classification_atom_tag;
struct local_arrangement_vertex_tag;
struct local_arrangement_locus_tag;
struct local_arrangement_dart_tag;
struct local_arrangement_cell_tag;
struct atom_boundary_use_tag;
struct classification_sector_tag;
struct classification_occurrence_tag;
struct atom_adjacency_tag;
struct classification_group_tag;
struct group_membership_tag;
struct quotient_edge_tag;
struct quotient_edge_member_tag;
struct propagation_component_tag;
struct seed_candidate_tag;
struct seed_query_tag;
struct seed_attempt_tag;
struct seed_hit_tag;
struct shell_contribution_tag;
struct propagation_assignment_tag;
struct constraint_residual_tag;
struct atom_side_label_tag;
struct classification_evidence_tag;

using classification_atom_id = strong_id<classification_atom_tag>;
using local_arrangement_vertex_id = strong_id<local_arrangement_vertex_tag>;
using local_arrangement_locus_id = strong_id<local_arrangement_locus_tag>;
using local_arrangement_dart_id = strong_id<local_arrangement_dart_tag>;
using local_arrangement_cell_id = strong_id<local_arrangement_cell_tag>;
using atom_boundary_use_id = strong_id<atom_boundary_use_tag>;
using classification_sector_id = strong_id<classification_sector_tag>;
using classification_occurrence_id = strong_id<classification_occurrence_tag>;
using atom_adjacency_id = strong_id<atom_adjacency_tag>;
using classification_group_id = strong_id<classification_group_tag>;
using group_membership_id = strong_id<group_membership_tag>;
using quotient_edge_id = strong_id<quotient_edge_tag>;
using quotient_edge_member_id = strong_id<quotient_edge_member_tag>;
using propagation_component_id = strong_id<propagation_component_tag>;
using seed_candidate_id = strong_id<seed_candidate_tag>;
using seed_query_id = strong_id<seed_query_tag>;
using seed_attempt_id = strong_id<seed_attempt_tag>;
using seed_hit_id = strong_id<seed_hit_tag>;
using shell_contribution_id = strong_id<shell_contribution_tag>;
using propagation_assignment_id = strong_id<propagation_assignment_tag>;
using constraint_residual_id = strong_id<constraint_residual_tag>;
using atom_side_label_id = strong_id<atom_side_label_tag>;
using classification_evidence_id = strong_id<classification_evidence_tag>;

enum class classification_provider_kind : std::uint8_t {
  lineage_surface_arrangement_v1 = 1,
};

enum class classification_verification_disposition : std::uint8_t {
  not_verified = 0,
  independently_verified = 1,
};

enum class classification_atom_role : std::uint8_t {
  positive_area = 1,
  boundary_sector = 2,
  zero_measure_occurrence = 3,
};

enum class local_locus_role : std::uint8_t {
  triangle_boundary = 1,
  original_source_edge_interval = 2,
  transparent_internal_diagonal = 3,
  transverse_carrier_span = 4,
  coplanar_overlap_boundary = 5,
  coincident_boundary = 6,
  tangent_contact_delimiter = 7,
  outside_sentinel = 8,
};

enum class classification_adjacency_class : std::uint8_t {
  uncut_continuation = 1,
  transparent_diagonal = 2,
  numeric_crossing = 3,
  tangent_continuation = 4,
  contact_delimiter = 5,
  symbolic_delimiter = 6,
  coplanar_boundary = 7,
  coincident_sheet = 8,
  topology_separated_contact = 9,
  intentionally_absent = 10,
  invalid = 11,
};

enum class union_eligibility : std::uint8_t {
  required = 1,
  permitted = 2,
  prohibited = 3,
};

enum class boundary_contact_state : std::uint8_t {
  none = 1,
  point_contact = 2,
  edge_contact = 3,
  face_contact = 4,
  tangent = 5,
  transverse = 6,
  coplanar = 7,
  coincident = 8,
  occurrence_separated = 9,
};

enum class seed_source_kind : std::uint8_t {
  empty_opposite = 1,
  certified_exterior_bound = 2,
  boundary_derived_anchor = 3,
  bounded_shell_query = 4,
};

enum class seed_attempt_disposition : std::uint8_t {
  successful = 1,
  rejected_boundary = 2,
  rejected_uncertain = 3,
  rejected_tangent = 4,
  rejected_coplanar = 5,
  directions_exhausted = 6,
};

enum class seed_hit_disposition : std::uint8_t {
  proper_hit = 1,
  exact_edge_hit = 2,
  exact_vertex_hit = 3,
  tangent_hit = 4,
  coplanar_hit = 5,
  behind_origin = 6,
  boundary_overlap = 7,
  unresolved = 8,
};

enum class occupancy_state : std::uint8_t {
  strict_outside = 1,
  strict_inside = 2,
  boundary = 3,
  symbolic_negative = 4,
  symbolic_positive = 5,
  coincident_owned = 6,
  unresolved = 7,
  invalid = 8,
};

enum class side_occupancy_origin : std::uint8_t {
  numeric = 1,
  boundary_derived = 2,
  symbolic = 3,
  coincident_owned = 4,
};

enum class propagation_edge_role : std::uint8_t {
  numeric_constraint = 1,
  symbolic_relation = 2,
  contact_relation = 3,
};

enum class classification_checkpoint : std::uint32_t {
  capability_validation = 1,
  predecessor_validation = 2,
  count_preflight = 3,
  triangle_incidence_proposals = 4,
  local_normalization = 5,
  rotation_system_construction = 6,
  local_cell_enumeration = 7,
  local_coverage_certification = 8,
  internal_diagonal_matching = 9,
  facet_atom_publication = 10,
  sector_construction = 11,
  raw_adjacency_construction = 12,
  reverse_delta_verification = 13,
  zero_delta_grouping = 14,
  group_canonicalization = 15,
  quotient_construction = 16,
  anchor_discovery = 17,
  witness_selection = 18,
  shell_query = 19,
  winding_propagation = 20,
  residual_verification = 21,
  side_label_derivation = 22,
  artifact_assembly = 23,
  independent_verification = 24,
  canonical_encoding = 25,
  commit = 26,
};

enum class classification_subcode : std::uint32_t {
  unsupported_version = 90001,
  wrong_owner = 90002,
  wrong_operation = 90003,
  predecessor_mismatch = 90004,
  predecessor_not_verified = 90005,
  count_overflow = 90006,
  byte_count_overflow = 90007,
  work_limit = 90008,
  resource_preflight = 90009,
  cancelled = 90010,
  malformed_reference = 90011,
  triangle_incidence_outside_facet = 90012,
  missing_authoritative_endpoint = 90013,
  duplicate_lineage_contradiction = 90014,
  proper_cross_without_event = 90015,
  unresolved_angular_order = 90016,
  invalid_rotation_system = 90017,
  open_face_walk = 90018,
  cell_coverage_gap = 90019,
  cell_witness_unavailable = 90020,
  internal_diagonal_interval_mismatch = 90021,
  semantic_atom_depends_on_diagonal = 90022,
  atom_coverage_gap = 90023,
  invalid_transition = 90024,
  adjacency_missing_reverse = 90025,
  crossing_delta_mismatch = 90026,
  prohibited_union = 90027,
  required_continuation_missing = 90028,
  group_membership_incomplete = 90029,
  quotient_multiplicity_inconsistency = 90030,
  nonzero_self_loop = 90031,
  conflicting_parallel_constraints = 90032,
  no_positive_area_seed_candidate = 90033,
  seed_witness_not_certified = 90034,
  shell_query_origin_overlaps_boundary = 90035,
  shell_query_attempt_uncertain = 90036,
  shell_query_directions_exhausted = 90037,
  half_open_ownership_contradiction = 90038,
  shell_contribution_inconsistent = 90039,
  unanchored_propagation_component = 90040,
  conflicting_anchors = 90041,
  propagated_path_disagreement = 90042,
  nonzero_cycle_residual = 90043,
  local_conservation_failure = 90044,
  nonboundary_winding_outside_domain = 90045,
  missing_side_label = 90046,
  occurrence_separation_lost = 90047,
  codec_error = 90048,
  verifier_rejection = 90049,
  resource_reconciliation_failed = 90050,
  transaction_failure = 90051,
  internal_invariant = 90052,
};

inline bounded_boolean_error classification_error(
    classification_subcode subcode, bounded_boolean_error_category category,
    const char *summary, classification_checkpoint checkpoint) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 9;
  error.stage = static_cast<std::uint16_t>(stage_id::classification);
  error.checkpoint = static_cast<std::uint32_t>(checkpoint);
  error.summary = summary;
  return error;
}

struct classification_cancellation_observer final {
  std::uint16_t version = contract_versions::classification_verifier;
  std::uint16_t reserved16 = 0;
  void *state = nullptr;
  void (*poll)(void *, classification_checkpoint) noexcept = nullptr;
  std::uint32_t reserved32 = 0;
};

// Resource subkinds. Component 09 documents its per-table accounting against the
// dedicated classification_* resource kinds (see Resources.h); temporary bytes,
// canonical bytes, diagnostics, and verifier work are folded into the existing
// temporary_bytes/persistent_bytes/diagnostic_findings/verification_findings
// kinds.  Every table is additionally bounded by an explicit preflight maximum.
struct classification_capabilities final {
  std::uint16_t provider_version = contract_versions::classification_provider;
  std::uint16_t atom_domain_version = contract_versions::atom_domain_schema;
  std::uint16_t codec_version = contract_versions::classification_codec;
  std::uint16_t verifier_version = contract_versions::classification_verifier;
  context_owner_token owner{};
  const bounded_boolean_cancellation_token *cancellation = nullptr;
  const classification_cancellation_observer *cancellation_observer = nullptr;
  resource_manager *resources = nullptr;
  std::uint64_t maximum_atoms = (std::uint64_t{1} << 34);
  std::uint64_t maximum_sectors = (std::uint64_t{1} << 36);
  std::uint64_t maximum_occurrences = (std::uint64_t{1} << 34);
  std::uint64_t maximum_adjacency = (std::uint64_t{1} << 37);
  std::uint64_t maximum_union_proposals = (std::uint64_t{1} << 37);
  std::uint64_t maximum_groups = (std::uint64_t{1} << 34);
  std::uint64_t maximum_quotient_edges = (std::uint64_t{1} << 36);
  std::uint64_t maximum_seed_queries = (std::uint64_t{1} << 34);
  std::uint64_t maximum_side_labels = (std::uint64_t{1} << 34);
  std::uint64_t maximum_propagation_assignments = (std::uint64_t{1} << 34);
  std::uint64_t maximum_canonical_bytes = (std::uint64_t{1} << 35);
  std::uint64_t maximum_work_units = (std::uint64_t{1} << 40);
  std::uint32_t reserved = 0;
};

struct classification_codec_limits final {
  std::uint64_t maximum_section_bytes = (std::uint64_t{1} << 34);
  std::uint64_t reserved = 0;
};

struct classification_verifier_limits final {
  std::uint64_t maximum_work_units = (std::uint64_t{1} << 38);
  bool exhaustive = false;
  std::uint32_t reserved = 0;
};

inline bool classification_cancelled(
    const classification_capabilities &capabilities,
    classification_checkpoint checkpoint) noexcept {
  if (capabilities.cancellation_observer &&
      capabilities.cancellation_observer->poll)
    capabilities.cancellation_observer->poll(
        capabilities.cancellation_observer->state, checkpoint);
  return capabilities.cancellation &&
         capabilities.cancellation->cancellation_requested();
}

struct classification_statistics final {
  std::uint64_t triangle_count = 0;
  std::uint64_t local_vertex_count = 0;
  std::uint64_t local_locus_count = 0;
  std::uint64_t local_dart_count = 0;
  std::uint64_t local_cell_count = 0;
  std::uint64_t atom_count = 0;
  std::uint64_t sector_count = 0;
  std::uint64_t occurrence_count = 0;
  std::uint64_t adjacency_count = 0;
  std::uint64_t union_proposal_count = 0;
  std::uint64_t group_count = 0;
  std::uint64_t quotient_edge_count = 0;
  std::uint64_t propagation_component_count = 0;
  std::uint64_t seed_query_count = 0;
  std::uint64_t seed_attempt_count = 0;
  std::uint64_t seed_hit_count = 0;
  std::uint64_t propagation_assignment_count = 0;
  std::uint64_t side_label_count = 0;
  std::uint64_t verifier_work_units = 0;
  std::uint64_t persistent_bytes = 0;
  std::uint64_t canonical_bytes = 0;
};

// Canonical identity keys. These never contain nominal coordinates, local
// bookkeeping ordinals, DSU roots, or task identities; they carry only source
// topology lineage, authoritative Component 08 descriptor lineage, and frozen
// schema versions.
struct classification_atom_key final {
  operand_id operand = operand_id::a;
  std::uint64_t shell = 0;
  std::uint64_t source_facet = 0;
  // Sorted semantic boundary-cycle lineage: (edge or carrier descriptor lineage)
  // sequences in canonical order.
  std::vector<std::uint64_t> boundary_lineage;
  std::vector<std::uint64_t> incident_descriptor_lineage;
  std::uint16_t schema_version = contract_versions::atom_domain_schema;

  friend bool operator<(const classification_atom_key &a,
                        const classification_atom_key &b) noexcept {
    return std::tie(a.operand, a.shell, a.source_facet, a.boundary_lineage,
                    a.incident_descriptor_lineage, a.schema_version) <
           std::tie(b.operand, b.shell, b.source_facet, b.boundary_lineage,
                    b.incident_descriptor_lineage, b.schema_version);
  }
  friend bool operator==(const classification_atom_key &a,
                         const classification_atom_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
  friend bool operator!=(const classification_atom_key &a,
                         const classification_atom_key &b) noexcept {
    return !(a == b);
  }
};

// A classification adjacency key names the exact directed source topology locus
// or Component 08 descriptor lineage that the transition crosses.
struct classification_adjacency_key final {
  classification_atom_key source{};
  classification_atom_key destination{};
  classification_adjacency_class adjacency_class =
      classification_adjacency_class::uncut_continuation;
  union_eligibility eligibility = union_eligibility::required;
  std::int32_t total_delta = 0;
  // Sparse per-shell deltas: (shell ordinal, signed delta) sorted by shell.
  std::vector<std::pair<std::uint64_t, std::int32_t>> shell_deltas;
  std::uint64_t semantic_locus_lineage = 0;
  std::uint64_t descriptor_lineage = 0;
  std::uint16_t schema_version =
      contract_versions::classification_adjacency_schema;

  friend bool operator<(const classification_adjacency_key &a,
                        const classification_adjacency_key &b) noexcept {
    return std::tie(a.source, a.destination, a.adjacency_class, a.eligibility,
                    a.total_delta, a.shell_deltas, a.semantic_locus_lineage,
                    a.descriptor_lineage, a.schema_version) <
           std::tie(b.source, b.destination, b.adjacency_class, b.eligibility,
                    b.total_delta, b.shell_deltas, a.semantic_locus_lineage,
                    b.descriptor_lineage, b.schema_version);
  }
  friend bool operator==(const classification_adjacency_key &a,
                         const classification_adjacency_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

} // namespace ygor::mesh_boolean::bounded
