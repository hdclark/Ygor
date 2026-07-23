#pragma once

#include "BroadPhaseTypes.h"
#include "PredicateResults.h"
#include "Resources.h"

#include <array>
#include <cstdint>
#include <limits>

namespace ygor::mesh_boolean::bounded {

struct relation_request_tag;
struct feature_relation_tag;
struct relation_dependency_tag;
struct relation_construction_tag;
struct symbolic_relation_decision_tag;
struct relation_event_seed_tag;
struct relation_candidate_disposition_tag;
struct relation_verifier_evidence_tag;

using relation_request_id = strong_id<relation_request_tag>;
using feature_relation_id = strong_id<feature_relation_tag>;
using relation_dependency_id = strong_id<relation_dependency_tag>;
using relation_construction_id = strong_id<relation_construction_tag>;
using symbolic_relation_decision_id = strong_id<symbolic_relation_decision_tag>;
using relation_event_seed_id = strong_id<relation_event_seed_tag>;
using relation_candidate_disposition_id =
    strong_id<relation_candidate_disposition_tag>;
using relation_verifier_evidence_id = strong_id<relation_verifier_evidence_tag>;

inline constexpr std::uint64_t relation_invalid_ordinal =
    std::numeric_limits<std::uint64_t>::max();

inline constexpr char relation_provider_identity_v1[] =
    "canonical_source_feature_relation_graph_v1";
inline constexpr char relation_dependency_policy_identity_v1[] =
    "support_region_edge_edge_edge_facet_facet_symbolic_dag_v1";
inline constexpr char relation_reduction_policy_identity_v1[] =
    "triangle_discovery_source_feature_ownership_v1";
inline constexpr char relation_truth_policy_identity_v1[] =
    "rounded_exact_relation_uncertainty_orthogonal_v1";
inline constexpr char relation_downstream_boundary_identity_v1[] =
    "relation_side_rank_evidence_no_final_selection_v1";

// Values in these enums are part of the Component 07 replay and canonical-byte
// contract. Do not renumber released values.
enum class relation_provider_kind : std::uint8_t {
  canonical_source_feature_relation_graph_v1 = 1,
};

enum class relation_request_family : std::uint8_t {
  imported_source_geometry = 1,
  rounded_bounded_primitive = 2,
  exact_stored_coordinate_relation = 3,
  source_point_source_facet_region = 4,
  source_edge_source_edge = 5,
  source_edge_source_facet = 6,
  source_facet_source_facet = 7,
  coplanar_source_facet_overlay = 8,
  composite_contact = 9,
  authoritative_construction = 10,
  numeric_crossing_multiplicity = 11,
  symbolic_eligibility = 12,
  symbolic_relation_decision = 13,
  event_seed = 14,
  candidate_disposition = 15,
};

enum class relation_record_scope : std::uint8_t {
  public_source_feature = 1,
  bookkeeping_only = 2,
};

enum class feature_relation_family : std::uint8_t {
  source_vertex_source_facet = 1,
  source_edge_source_edge = 2,
  source_edge_source_facet = 3,
  source_facet_source_facet = 4,
  triangle_local_witness = 5,
  symbolic_contact = 6,
};

enum class feature_relation_status : std::uint8_t {
  not_evaluated = 0,
  definitely_separated = 1,
  proper_crossing = 2,
  endpoint_crossing = 3,
  point_contact = 4,
  segment_contact = 5,
  tangency = 6,
  overlap = 7,
  containment = 8,
  coincidence_same_orientation = 9,
  coincidence_opposite_orientation = 10,
};

enum class candidate_relation_disposition_kind : std::uint8_t {
  no_public_relation = 1,
  mapped_to_public_relation = 2,
  bookkeeping_witness = 3,
};

enum class relation_construction_kind : std::uint8_t {
  bounded_point = 1,
  bounded_parameter = 2,
  bounded_interval = 3,
  bounded_carrier = 4,
};

enum class symbolic_relation_side : std::int8_t {
  negative = -1,
  coincident = 0,
  positive = 1,
};

enum class relation_verification_disposition : std::uint8_t {
  unverified = 0,
  independently_verified = 1,
};

enum class relation_checkpoint : std::uint32_t {
  context_policy_capability_validation = 1,
  predecessor_validation = 2,
  count_representability_preflight = 3,
  discovery_resource_reservation = 4,
  candidate_scan = 5,
  initial_request_grouping = 6,
  dependency_closure = 7,
  graph_finalization = 8,
  rounded_primitive_evaluation = 9,
  exact_relation_evaluation = 10,
  truth_record_assembly = 11,
  source_facet_region_evaluation = 12,
  edge_edge_evaluation = 13,
  edge_facet_evaluation = 14,
  facet_facet_evaluation = 15,
  coplanar_overlay_evaluation = 16,
  construction_validation = 17,
  crossing_multiplicity = 18,
  symbolic_eligibility = 19,
  symbolic_matrix_lookup = 20,
  downstream_selection_boundary_audit = 21,
  event_seed_and_disposition_reconciliation = 22,
  canonical_id_and_reference_remap = 23,
  producer_verification = 24,
  canonical_encoding = 25,
  independent_verification = 26,
  resource_reconciliation = 27,
  transaction_commit = 28,
};

enum class relation_subcode : std::uint32_t {
  unsupported_version = 70001,
  wrong_owner = 70002,
  predecessor_mismatch = 70003,
  predecessor_not_verified = 70004,
  malformed_candidate = 70005,
  count_overflow = 70006,
  byte_count_overflow = 70007,
  work_limit = 70008,
  resource_preflight = 70009,
  cancelled = 70010,
  malformed_request_key = 70011,
  incompatible_duplicate_request = 70012,
  duplicate_authoritative_producer = 70013,
  missing_dependency = 70014,
  forward_dependency = 70015,
  same_family_dependency = 70016,
  cyclic_dependency = 70017,
  unclosed_dependency = 70018,
  candidate_disposition_missing = 70019,
  candidate_disposition_duplicate = 70020,
  candidate_disposition_contradiction = 70021,
  owner_in_semantics = 70022,
  canonical_order_mismatch = 70023,
  canonical_id_mismatch = 70024,
  codec_error = 70025,
  digest_mismatch = 70026,
  verifier_rejection = 70027,
  unsupported_relation_kernel = 70028,
  transaction_failure = 70029,
  internal_invariant = 70030,
  symbolic_ineligible = 70031,
  symbolic_geometry_change = 70032,
  symbolic_selection_boundary = 70033,
};

inline bounded_boolean_error relation_error(
    relation_subcode subcode, bounded_boolean_error_category category,
    const char *summary, relation_checkpoint checkpoint) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 7;
  error.stage = static_cast<std::uint16_t>(stage_id::relation_kernel);
  error.checkpoint = static_cast<std::uint32_t>(checkpoint);
  error.summary = summary;
  return error;
}

struct relation_capabilities final {
  std::uint16_t provider_version = contract_versions::relation_provider;
  std::uint16_t graph_policy_version = contract_versions::relation_graph_policy;
  std::uint16_t truth_policy_version = contract_versions::relation_truth_policy;
  std::uint16_t codec_version = contract_versions::relation_codec;
  std::uint16_t verifier_version = contract_versions::relation_verifier;
  context_owner_token owner{};
  const bounded_boolean_cancellation_token *cancellation = nullptr;
  resource_manager *resources = nullptr;
  std::uint64_t maximum_requests = (std::uint64_t{1} << 34);
  std::uint64_t maximum_dependencies = (std::uint64_t{1} << 35);
  std::uint64_t maximum_consumers = (std::uint64_t{1} << 35);
  std::uint64_t maximum_relations = (std::uint64_t{1} << 34);
  std::uint64_t maximum_constructions = (std::uint64_t{1} << 34);
  std::uint64_t maximum_symbolic_decisions = (std::uint64_t{1} << 34);
  std::uint64_t maximum_event_seeds = (std::uint64_t{1} << 34);
  std::uint64_t maximum_canonical_bytes = (std::uint64_t{1} << 34);
  std::uint64_t maximum_work_units = (std::uint64_t{1} << 38);
  std::uint32_t reserved = 0;
};

inline bool relation_cancelled(const relation_capabilities &capabilities) noexcept {
  return capabilities.cancellation &&
         capabilities.cancellation->cancellation_requested();
}

struct relation_statistics final {
  std::uint64_t candidate_count = 0;
  std::uint64_t request_proposal_count = 0;
  std::uint64_t unique_request_count = 0;
  std::uint64_t dependency_count = 0;
  std::uint64_t reverse_consumer_count = 0;
  std::uint64_t candidate_witness_count = 0;
  std::uint64_t public_relation_count = 0;
  std::uint64_t bookkeeping_relation_count = 0;
  std::uint64_t construction_count = 0;
  std::uint64_t symbolic_decision_count = 0;
  std::uint64_t event_seed_count = 0;
  std::uint64_t sort_comparisons = 0;
  std::uint64_t verifier_work_units = 0;
  std::uint64_t persistent_bytes = 0;
};

} // namespace ygor::mesh_boolean::bounded
