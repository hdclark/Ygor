#pragma once

#include "RelationTypes.h"
#include "Resources.h"

#include <cstdint>
#include <limits>

namespace ygor::mesh_boolean::bounded {

inline constexpr char intersection_provider_identity_v1[] =
    "canonical_lineage_event_arrangement_v1";
inline constexpr char intersection_occurrence_policy_identity_v1[] =
    "shared_event_separate_occurrence_v1";
inline constexpr char intersection_ordering_policy_identity_v1[] =
    "bounded_interval_sweep_cluster_order_v1";
inline constexpr char intersection_partition_policy_identity_v1[] =
    "endpoint_cluster_open_interval_partition_v1";
inline constexpr char intersection_activation_policy_identity_v1[] =
    "relation_interval_supported_connectivity_v1";

inline constexpr std::uint64_t intersection_invalid_ordinal =
    std::numeric_limits<std::uint64_t>::max();

struct event_tag;
struct event_occurrence_tag;
struct event_seed_binding_tag;
struct event_incidence_tag;
struct source_edge_membership_tag;
struct source_edge_sequence_tag;
struct source_edge_cluster_tag;
struct source_edge_interval_tag;
struct transverse_carrier_tag;
struct coplanar_support_tag;
struct collinear_overlap_carrier_tag;
struct carrier_membership_tag;
struct carrier_cluster_tag;
struct carrier_active_span_tag;
struct coplanar_overlap_record_tag;
struct coplanar_region_incidence_tag;
struct crossing_aggregate_tag;
struct contact_aggregate_tag;
struct intersection_descriptor_tag;
struct ordering_certificate_tag;
struct intersection_verifier_evidence_tag;
struct intersection_diagnostic_tag;
struct intersection_replay_checkpoint_tag;

using event_id = strong_id<event_tag>;
using event_occurrence_id = strong_id<event_occurrence_tag>;
using event_seed_binding_id = strong_id<event_seed_binding_tag>;
using event_incidence_id = strong_id<event_incidence_tag>;
using source_edge_membership_id = strong_id<source_edge_membership_tag>;
using source_edge_sequence_id = strong_id<source_edge_sequence_tag>;
using source_edge_cluster_id = strong_id<source_edge_cluster_tag>;
using source_edge_interval_id = strong_id<source_edge_interval_tag>;
using transverse_carrier_id = strong_id<transverse_carrier_tag>;
using coplanar_support_id = strong_id<coplanar_support_tag>;
using collinear_overlap_carrier_id = strong_id<collinear_overlap_carrier_tag>;
using carrier_membership_id = strong_id<carrier_membership_tag>;
using carrier_cluster_id = strong_id<carrier_cluster_tag>;
using carrier_active_span_id = strong_id<carrier_active_span_tag>;
using coplanar_overlap_record_id = strong_id<coplanar_overlap_record_tag>;
using coplanar_region_incidence_id = strong_id<coplanar_region_incidence_tag>;
using crossing_aggregate_id = strong_id<crossing_aggregate_tag>;
using contact_aggregate_id = strong_id<contact_aggregate_tag>;
using intersection_descriptor_id = strong_id<intersection_descriptor_tag>;
using ordering_certificate_id = strong_id<ordering_certificate_tag>;
using intersection_verifier_evidence_id =
    strong_id<intersection_verifier_evidence_tag>;
using intersection_diagnostic_id = strong_id<intersection_diagnostic_tag>;
using intersection_replay_checkpoint_id =
    strong_id<intersection_replay_checkpoint_tag>;

enum class intersection_provider_kind : std::uint8_t {
  canonical_lineage_event_arrangement_v1 = 1,
};

enum class bounded_point_reference_kind : std::uint8_t {
  source_point = 1,
  constructed_point = 2,
};

enum class intersection_verification_disposition : std::uint8_t {
  not_verified = 0,
  independently_verified = 1,
};

enum class intersection_event_class : std::uint8_t {
  source_vertex_contact = 1,
  edge_facet_point = 2,
  edge_edge_point = 3,
  overlap_endpoint = 4,
  tangent_point = 5,
  multi_feature_meeting = 6,
  symbolic_tie = 7,
  coincident_cluster_member = 8,
};

enum class occurrence_role : std::uint8_t {
  single_occurrence = 1,
  topology_separated_contact = 2,
  coincident_sheet_member = 3,
  symbolic_side_occurrence = 4,
  overlap_boundary_occurrence = 5,
  multiplicity_occurrence = 6,
};

enum class intersection_membership_role : std::uint8_t {
  endpoint = 1,
  interior = 2,
  overlap_start = 3,
  overlap_end = 4,
  interval_member = 5,
};

enum class source_edge_sentinel_side : std::uint8_t {
  start = 1,
  end = 2,
};

enum class intersection_order_disposition : std::uint8_t {
  definitely_before = 1,
  exact_equal = 2,
  definitely_after = 3,
  unresolved_overlap = 4,
  invalid = 5,
};

enum class intersection_interval_length : std::uint8_t {
  definitely_positive = 1,
  exact_zero = 2,
  uncertain = 3,
  overlap = 4,
};

enum class intersection_carrier_class : std::uint8_t {
  transverse = 1,
  coplanar = 2,
  original_source_edge = 3,
  collinear_overlap = 4,
};

enum class intersection_descriptor_category : std::uint8_t {
  proper_crossing = 1,
  endpoint_crossing = 2,
  tangent = 3,
  contact_delimiter = 4,
  coplanar_overlap_boundary = 5,
  coincident_sheet_boundary = 6,
  coincident_sheet_interior = 7,
  bookkeeping_only = 8,
  topology_separated_contact = 9,
  no_influence = 10,
  unresolved = 11,
  invalid = 12,
};

enum class intersection_descriptor_locus : std::uint8_t {
  whole_source_edge = 1,
  source_edge_open_interval = 2,
  source_edge_cluster_boundary = 3,
  source_vertex_sector = 4,
  source_facet_original_edge_adjacency = 5,
  transparent_internal_diagonal_adjacency = 6,
  transverse_active_span = 7,
  transverse_inactive_gap = 8,
  coplanar_overlap = 9,
  coincident_sheet = 10,
  separated_contact_occurrence = 11,
};

enum class intersection_cluster_equivalence : std::uint8_t {
  exact_parameter_coincidence = 1,
  exact_coordinate_coincidence = 2,
  lineage_authorized_unresolved = 3,
  mixed_symbolic_tie = 4,
};

enum class intersection_span_activation : std::uint8_t {
  inactive = 1,
  active_transverse_intersection = 2,
  active_overlap_boundary = 3,
  active_coincident_boundary = 4,
  contact_only = 5,
  unresolved = 6,
  invalid = 7,
};

enum class event_incidence_kind : std::uint8_t {
  source_vertex = 1,
  source_edge = 2,
  source_edge_interval = 3,
  source_facet = 4,
  source_triangle = 5,
  oriented_halfedge = 6,
  source_shell = 7,
  relation = 8,
  candidate = 9,
  transverse_carrier = 10,
  collinear_or_coplanar_carrier = 11,
  overlap_component = 12,
  crossing_contribution = 13,
  symbolic_decision = 14,
  descriptor_precursor = 15,
};

enum class intersection_aggregate_locus : std::uint8_t {
  event_occurrence = 1,
  cluster = 2,
  source_edge = 3,
  source_edge_boundary = 4,
  carrier_cluster = 5,
  carrier_span = 6,
  overlap_boundary = 7,
  source_facet = 8,
  shell = 9,
};

enum class intersection_checkpoint : std::uint32_t {
  context_capability_validation = 1,
  predecessor_validation = 2,
  count_preflight = 3,
  resource_reservation = 4,
  seed_normalization = 5,
  event_grouping = 6,
  event_occurrence_id_assignment = 7,
  authoritative_point_attachment = 8,
  incidence_proposals = 9,
  incidence_publication = 10,
  source_edge_membership_proposals = 11,
  source_edge_ordering = 12,
  source_edge_partition = 13,
  transverse_carriers = 14,
  coplanar_carriers = 15,
  aggregate_reconstruction = 16,
  descriptor_derivation = 17,
  source_facet_reconciliation = 18,
  canonical_remap = 19,
  producer_invariants = 20,
  canonical_encoding = 21,
  independent_verification = 22,
  resource_reconciliation = 23,
  transaction_commit = 24,
};

enum class intersection_subcode : std::uint32_t {
  unsupported_version = 80001,
  wrong_owner = 80002,
  wrong_operation = 80003,
  predecessor_mismatch = 80004,
  predecessor_not_verified = 80005,
  malformed_seed = 80006,
  malformed_event_key = 80007,
  malformed_occurrence_key = 80008,
  malformed_reference = 80009,
  forbidden_identity_material = 80010,
  count_overflow = 80011,
  capacity_exceeded = 80012,
  resource_preflight = 80013,
  duplicate_event_incompatible = 80014,
  authoritative_construction_conflict = 80015,
  occurrence_semantic_conflict = 80016,
  occurrence_merge_or_split = 80017,
  missing_authoritative_point = 80018,
  source_vertex_point_mismatch = 80019,
  secondary_witness_incompatible = 80020,
  seed_mapping_incomplete = 80021,
  incidence_incomplete = 80022,
  unrelated_feature_incidence = 80023,
  internal_diagonal_public_ownership = 80024,
  membership_incomplete = 80025,
  parameter_invalid = 80026,
  bounded_order_contradiction = 80027,
  exact_equal_without_evidence = 80028,
  nominal_fallback_forbidden = 80029,
  unresolved_topology_order = 80030,
  source_edge_sequence_invalid = 80031,
  source_edge_partition_invalid = 80032,
  transverse_carrier_invalid = 80033,
  unsupported_gap_connected = 80034,
  transverse_span_invalid = 80035,
  coplanar_routed_transverse = 80036,
  overlap_carrier_invalid = 80037,
  geometric_weld_forbidden = 80038,
  cluster_invalid = 80039,
  aggregate_mismatch = 80040,
  member_erased = 80041,
  descriptor_mismatch = 80042,
  facet_reconciliation_failed = 80043,
  canonicalization_error = 80044,
  codec_error = 80045,
  digest_mismatch = 80046,
  verifier_rejection = 80047,
  resource_reconciliation_failed = 80048,
  cancelled = 80049,
  transaction_failure = 80050,
  internal_invariant = 80051,
};

inline bounded_boolean_error intersection_error(
    intersection_subcode subcode, bounded_boolean_error_category category,
    const char *summary, intersection_checkpoint checkpoint) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 8;
  error.stage = static_cast<std::uint16_t>(stage_id::intersection_registry);
  error.checkpoint = static_cast<std::uint32_t>(checkpoint);
  error.summary = summary;
  return error;
}

struct intersection_cancellation_observer final {
  std::uint16_t version = contract_versions::intersection_cancellation_observer;
  std::uint16_t reserved16 = 0;
  void *state = nullptr;
  void (*poll)(void *, intersection_checkpoint) noexcept = nullptr;
  std::uint32_t reserved32 = 0;
};

struct intersection_capabilities final {
  std::uint16_t provider_version = contract_versions::intersection_provider;
  std::uint16_t semantic_policy_version =
      contract_versions::intersection_semantic_policy;
  std::uint16_t codec_version = contract_versions::intersection_codec;
  std::uint16_t verifier_version = contract_versions::intersection_verifier;
  context_owner_token owner{};
  const bounded_boolean_cancellation_token *cancellation = nullptr;
  const intersection_cancellation_observer *cancellation_observer = nullptr;
  resource_manager *resources = nullptr;
  std::uint64_t maximum_events = (std::uint64_t{1} << 34);
  std::uint64_t maximum_occurrences = (std::uint64_t{1} << 35);
  std::uint64_t maximum_seed_bindings = (std::uint64_t{1} << 34);
  std::uint64_t maximum_incidence = (std::uint64_t{1} << 37);
  std::uint64_t maximum_memberships = (std::uint64_t{1} << 37);
  std::uint64_t maximum_clusters = (std::uint64_t{1} << 36);
  std::uint64_t maximum_intervals = (std::uint64_t{1} << 36);
  std::uint64_t maximum_carriers = (std::uint64_t{1} << 34);
  std::uint64_t maximum_overlaps = (std::uint64_t{1} << 35);
  std::uint64_t maximum_aggregates = (std::uint64_t{1} << 36);
  std::uint64_t maximum_descriptors = (std::uint64_t{1} << 37);
  std::uint64_t maximum_canonical_bytes = (std::uint64_t{1} << 35);
  std::uint64_t maximum_work_units = (std::uint64_t{1} << 40);
  std::uint32_t reserved = 0;
};

inline bool intersection_cancelled(
    const intersection_capabilities &capabilities,
    intersection_checkpoint checkpoint) noexcept {
  if (capabilities.cancellation_observer &&
      capabilities.cancellation_observer->poll) {
    capabilities.cancellation_observer->poll(
        capabilities.cancellation_observer->state, checkpoint);
  }
  return capabilities.cancellation &&
         capabilities.cancellation->cancellation_requested();
}

struct intersection_resource_estimate final {
  std::uint64_t persistent_bytes = 0;
  std::uint64_t temporary_bytes = 0;
  std::uint64_t work_units = 0;
  std::uint64_t event_count = 0;
  std::uint64_t occurrence_count = 0;
  std::uint64_t seed_binding_count = 0;
  std::uint64_t incidence_count = 0;
  std::uint64_t membership_count = 0;
  std::uint64_t cluster_count = 0;
  std::uint64_t interval_count = 0;
  std::uint64_t carrier_count = 0;
  std::uint64_t overlap_count = 0;
  std::uint64_t aggregate_count = 0;
  std::uint64_t descriptor_count = 0;
};

struct intersection_statistics final {
  std::uint64_t seed_count = 0;
  std::uint64_t event_count = 0;
  std::uint64_t occurrence_count = 0;
  std::uint64_t seed_binding_count = 0;
  std::uint64_t incidence_count = 0;
  std::uint64_t source_edge_membership_count = 0;
  std::uint64_t source_edge_sequence_count = 0;
  std::uint64_t source_edge_cluster_count = 0;
  std::uint64_t source_edge_interval_count = 0;
  std::uint64_t transverse_carrier_count = 0;
  std::uint64_t carrier_membership_count = 0;
  std::uint64_t carrier_cluster_count = 0;
  std::uint64_t carrier_span_count = 0;
  std::uint64_t coplanar_support_count = 0;
  std::uint64_t overlap_count = 0;
  std::uint64_t aggregate_count = 0;
  std::uint64_t descriptor_count = 0;
  std::uint64_t ordering_certificate_count = 0;
  std::uint64_t diagnostic_count = 0;
  std::uint64_t replay_checkpoint_count = 0;
  std::uint64_t sort_comparisons = 0;
  std::uint64_t verifier_work_units = 0;
  std::uint64_t persistent_bytes = 0;
  std::uint64_t canonical_bytes = 0;
};

} // namespace ygor::mesh_boolean::bounded
