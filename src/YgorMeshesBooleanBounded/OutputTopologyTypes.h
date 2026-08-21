#pragma once

#include "CanonicalBytes.h"
#include "ContractVersions.h"
#include "Identity.h"
#include "Policies.h"
#include "Resources.h"
#include "SelectionTypes.h"
#include "Sha256.h"

#include <array>
#include <cstdint>
#include <limits>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

inline constexpr char output_topology_provider_identity_v1[] =
    "paired_boundary_face_cycle_construction_v1";
inline constexpr char output_incidence_audit_provider_identity_v1[] =
    "canonical_retained_incidence_audit_v2";
inline constexpr char output_vertex_provider_identity_v1[] =
    "sorted_nonempty_occurrence_materialization_v2";
inline constexpr char output_region_provider_identity_v1[] =
    "exact_positive_area_continuation_components_v2";
inline constexpr char output_boundary_provider_identity_v1[] =
    "oriented_dart_cancellation_v2";
inline constexpr char output_edge_provider_identity_v1[] =
    "explicit_endpoint_atomic_pair_v2";
inline constexpr char output_successor_provider_identity_v1[] =
    "continuation_skipping_dart_permutation_v2";
inline constexpr char output_cycle_provider_identity_v1[] =
    "role_independent_canonical_successor_cycles_v2";
inline constexpr char output_contour_provider_identity_v1[] =
    "lineage_first_single_outer_region_v2";
inline constexpr char output_witness_provider_identity_v1[] =
    "certified_contour_side_witness_v1";
inline constexpr char output_admissibility_provider_identity_v1[] =
    "bounded_planar_embedding_audit_v2";
inline constexpr char output_vertex_link_provider_identity_v1[] =
    "halfedge_rotation_link_audit_v1";
inline constexpr char output_verification_provider_identity_v1[] =
    "independent_polygonal_topology_rebuild_v2";

inline constexpr std::uint64_t output_topology_invalid_ordinal =
    std::numeric_limits<std::uint64_t>::max();

// ---------------------------------------------------------------------------
// Strong identity domains. Component 11 never aliases these to predecessor
// IDs, raw `I`, `size_t`, offsets, or pointers.
// ---------------------------------------------------------------------------
struct output_incidence_audit_tag;
struct zero_measure_support_evidence_tag;
struct output_vertex_occurrence_tag;
struct output_coordinate_reference_tag;
struct output_face_region_tag;
struct region_member_tag;
struct continuation_consumption_tag;
struct region_split_evidence_tag;
struct boundary_dart_tag;
struct boundary_transition_tag;
struct paired_output_edge_tag;
struct output_halfedge_tag;
struct halfedge_endpoint_fan_ref_tag;
struct face_cycle_tag;
struct cycle_halfedge_ref_tag;
struct contour_node_tag;
struct contour_relation_tag;
struct contour_witness_tag;
struct zero_measure_boundary_tag;
struct admissibility_evidence_tag;
struct carrier_balance_audit_tag;
struct vertex_link_evidence_tag;

using output_incidence_audit_id = strong_id<output_incidence_audit_tag>;
using zero_measure_support_evidence_id =
    strong_id<zero_measure_support_evidence_tag>;
using output_vertex_occurrence_id = strong_id<output_vertex_occurrence_tag>;
using output_coordinate_reference_id = strong_id<output_coordinate_reference_tag>;
using output_face_region_id = strong_id<output_face_region_tag>;
using region_member_id = strong_id<region_member_tag>;
using continuation_consumption_id = strong_id<continuation_consumption_tag>;
using region_split_evidence_id = strong_id<region_split_evidence_tag>;
using boundary_dart_id = strong_id<boundary_dart_tag>;
using boundary_transition_id = strong_id<boundary_transition_tag>;
using paired_output_edge_id = strong_id<paired_output_edge_tag>;
using output_halfedge_id = strong_id<output_halfedge_tag>;
using halfedge_endpoint_fan_ref_id = strong_id<halfedge_endpoint_fan_ref_tag>;
using face_cycle_id = strong_id<face_cycle_tag>;
using cycle_halfedge_ref_id = strong_id<cycle_halfedge_ref_tag>;
using contour_node_id = strong_id<contour_node_tag>;
using contour_relation_id = strong_id<contour_relation_tag>;
using contour_witness_id = strong_id<contour_witness_tag>;
using zero_measure_boundary_id = strong_id<zero_measure_boundary_tag>;
using admissibility_evidence_id = strong_id<admissibility_evidence_tag>;
using carrier_balance_audit_id = strong_id<carrier_balance_audit_tag>;
using vertex_link_evidence_id = strong_id<vertex_link_evidence_tag>;

// ---------------------------------------------------------------------------
// Closed enums. Every value is explicit; unknown values fail decoding with no
// default fallthrough.
// ---------------------------------------------------------------------------
enum class output_topology_provider_kind : std::uint8_t {
  paired_boundary_face_cycle_construction_v1 = 1,
};

enum class output_topology_verification_disposition : std::uint8_t {
  not_verified = 0,
  independently_verified = 1,
};

enum class output_incidence_disposition : std::uint8_t {
  paired_boundary = 1,
  paired_zero_measure_boundary = 2,
  transparent_internal = 3,
  consumed_owner_seam = 4,
  support_only_zero_measure = 5,
  suppressed_audit_only = 6,
  invalid = 7,
};

enum class coordinate_source : std::uint8_t {
  source_vertex = 1,
  canonical_event = 2,
  supported_predecessor_construction = 3,
  invalid = 4,
};

enum class edge_role : std::uint8_t {
  whole_source_edge = 1,
  split_source_edge_interval = 2,
  transverse_carrier_interval = 3,
  coplanar_overlap_boundary = 4,
  topology_separation_contact = 5,
  supported_other = 6,
  invalid = 7,
};

enum class endpoint_nominal_relation : std::uint8_t {
  definitely_distinct = 1,
  bit_equal = 2,
  uncertainty_overlapping = 3,
  same_occurrence_loop = 4,
  invalid = 5,
};

enum class cycle_geometric_category : std::uint8_t {
  definite_positive_area = 1,
  proven_zero_measure = 2,
  bounded_deferred = 3,
  occurrence_distinct_coincident = 4,
  invalid = 5,
};

enum class contour_role : std::uint8_t {
  outer = 1,
  hole = 2,
  deferred_zero_measure = 3,
  coincident_distinct = 4,
  invalid = 5,
};

enum class witness_source : std::uint8_t {
  predecessor_arrangement_cell = 1,
  certified_retained_sector_construction = 2,
  invalid = 3,
};

enum class bounded_admissibility : std::uint8_t {
  definite_valid = 1,
  exact_lineage_tie_valid = 2,
  deferred_degeneracy_valid = 3,
  uncertain_topology_changing = 4,
  definite_invalid = 5,
  invalid = 6,
};

enum class output_topology_checkpoint : std::uint32_t {
  context_capability_validation = 1,
  predecessor_validation = 2,
  count_preflight = 3,
  resource_reservation = 4,
  incidence_audit = 5,
  zero_measure_partition = 6,
  vertex_occurrence_allocation = 7,
  continuation_graph = 8,
  provisional_region_construction = 9,
  final_region_split = 10,
  boundary_extraction = 11,
  pair_materialization = 12,
  carrier_balance = 13,
  successor_construction = 14,
  cycle_extraction = 15,
  contour_role_assignment = 16,
  witness_validation = 17,
  admissibility = 18,
  vertex_link_reconstruction = 19,
  member_set_audit = 20,
  canonicalization = 21,
  producer_checks = 22,
  canonical_encoding = 23,
  independent_verification = 24,
  resource_reconciliation = 25,
  commit = 26,
};

enum class output_topology_subcode : std::uint32_t {
  unsupported_version = 110001,
  wrong_owner = 110002,
  wrong_operation = 110003,
  predecessor_digest_mismatch = 110004,
  predecessor_not_verified = 110005,
  count_overflow = 110006,
  byte_count_overflow = 110007,
  work_limit = 110008,
  resource_preflight = 110009,
  cancelled = 110010,
  missing_incidence = 110011,
  duplicate_incidence = 110012,
  invalid_disposition = 110013,
  support_only_with_topology = 110014,
  zero_measure_without_pair = 110015,
  empty_occurrence_requirement = 110016,
  occurrence_link_mismatch = 110017,
  pair_endpoint_read_before_initialize = 110018,
  pair_reciprocity_mismatch = 110019,
  nonreciprocal_continuation = 110020,
  region_split_failure = 110021,
  boundary_walk_failure = 110022,
  successor_permutation_failure = 110023,
  cycle_extraction_failure = 110024,
  cycle_identity_role_contamination = 110025,
  contour_role_failure = 110026,
  missing_contour_witness = 110027,
  witness_not_strictly_separated = 110028,
  containment_uncertainty = 110029,
  carrier_balance_failure = 110030,
  vertex_link_failure = 110031,
  codec_error = 110032,
  digest_mismatch = 110033,
  verifier_rejection = 110034,
  resource_reconciliation_failed = 110035,
  transaction_failure = 110036,
  internal_invariant = 110037,
  strict_profile_mismatch = 110038,
};

inline bounded_boolean_error output_topology_error(
    output_topology_subcode subcode, bounded_boolean_error_category category,
    const char *summary, output_topology_checkpoint checkpoint) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 11;
  error.stage = static_cast<std::uint16_t>(stage_id::output_topology);
  error.checkpoint = static_cast<std::uint32_t>(checkpoint);
  error.summary = summary;
  return error;
}

struct output_topology_cancellation_observer final {
  std::uint16_t version = contract_versions::output_topology_verifier;
  std::uint16_t reserved16 = 0;
  void *state = nullptr;
  void (*poll)(void *, output_topology_checkpoint) noexcept = nullptr;
  std::uint32_t reserved32 = 0;
};

struct output_topology_capabilities final {
  std::uint16_t provider_version = contract_versions::output_topology_provider;
  std::uint16_t incidence_audit_version =
      contract_versions::output_topology_incidence_audit_schema;
  std::uint16_t codec_version = contract_versions::output_topology_codec;
  std::uint16_t verifier_version = contract_versions::output_topology_verifier;
  context_owner_token owner{};
  const bounded_boolean_cancellation_token *cancellation = nullptr;
  const output_topology_cancellation_observer *cancellation_observer = nullptr;
  resource_manager *resources = nullptr;
  std::uint64_t maximum_incidences = (std::uint64_t{1} << 37);
  std::uint64_t maximum_vertex_occurrences = (std::uint64_t{1} << 34);
  std::uint64_t maximum_regions = (std::uint64_t{1} << 34);
  std::uint64_t maximum_paired_edges = (std::uint64_t{1} << 36);
  std::uint64_t maximum_halfedges = (std::uint64_t{1} << 37);
  std::uint64_t maximum_cycles = (std::uint64_t{1} << 34);
  std::uint64_t maximum_contours = (std::uint64_t{1} << 34);
  std::uint64_t maximum_witnesses = (std::uint64_t{1} << 34);
  std::uint64_t maximum_admissibility = (std::uint64_t{1} << 38);
  std::uint64_t maximum_canonical_bytes = (std::uint64_t{1} << 35);
  std::uint64_t maximum_work_units = (std::uint64_t{1} << 40);
  std::uint32_t reserved = 0;
};

struct output_topology_codec_limits final {
  std::uint64_t maximum_section_bytes = (std::uint64_t{1} << 34);
  std::uint64_t reserved = 0;
};

inline bool output_topology_cancelled(
    const output_topology_capabilities &capabilities,
    output_topology_checkpoint checkpoint) noexcept {
  if (capabilities.cancellation_observer &&
      capabilities.cancellation_observer->poll)
    capabilities.cancellation_observer->poll(
        capabilities.cancellation_observer->state, checkpoint);
  return capabilities.cancellation &&
         capabilities.cancellation->cancellation_requested();
}

struct output_topology_statistics final {
  std::uint64_t incidence_audit_count = 0;
  std::uint64_t zero_measure_support_count = 0;
  std::uint64_t coordinate_reference_count = 0;
  std::uint64_t vertex_occurrence_count = 0;
  std::uint64_t region_count = 0;
  std::uint64_t region_member_count = 0;
  std::uint64_t continuation_consumption_count = 0;
  std::uint64_t boundary_dart_count = 0;
  std::uint64_t paired_edge_count = 0;
  std::uint64_t halfedge_count = 0;
  std::uint64_t endpoint_fan_ref_count = 0;
  std::uint64_t face_cycle_count = 0;
  std::uint64_t cycle_halfedge_ref_count = 0;
  std::uint64_t contour_node_count = 0;
  std::uint64_t contour_witness_count = 0;
  std::uint64_t zero_measure_boundary_count = 0;
  std::uint64_t admissibility_evidence_count = 0;
  std::uint64_t carrier_balance_audit_count = 0;
  std::uint64_t vertex_link_evidence_count = 0;
  std::uint64_t verifier_work_units = 0;
  std::uint64_t persistent_bytes = 0;
  std::uint64_t canonical_bytes = 0;
};

// ---------------------------------------------------------------------------
// Canonical identity keys. They never contain nominal coordinates, hashes,
// discovery order, worker/task identities, or future output IDs.
// ---------------------------------------------------------------------------

// Complete Component 10 occurrence requirement key plus owner namespace and
// schema version.
struct output_vertex_key final {
  std::uint64_t endpoint_domain = 0;
  std::uint64_t predecessor_occurrence = 0;
  std::uint16_t schema_version =
      contract_versions::output_topology_vertex_occurrence_schema;

  friend bool operator<(const output_vertex_key &a,
                        const output_vertex_key &b) noexcept {
    return std::tie(a.endpoint_domain, a.predecessor_occurrence,
                    a.schema_version) <
           std::tie(b.endpoint_domain, b.predecessor_occurrence, b.schema_version);
  }
  friend bool operator==(const output_vertex_key &a,
                         const output_vertex_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

// Final positive-area region key. In V1 a region is a single retained use;
// the key carries the exact positive-area member component and split evidence.
struct output_face_region_key final {
  operand_id source_operand = operand_id::a;
  std::uint64_t retained_use = 0;
  std::uint64_t source_facet = 0;
  std::uint64_t support_lineage = 0;
  std::uint64_t positive_area_component = 0;
  std::uint16_t schema_version =
      contract_versions::output_topology_region_schema;

  friend bool operator<(const output_face_region_key &a,
                        const output_face_region_key &b) noexcept {
    return std::tie(a.source_operand, a.retained_use, a.source_facet,
                    a.support_lineage, a.positive_area_component,
                    a.schema_version) <
           std::tie(b.source_operand, b.retained_use, b.source_facet,
                    b.support_lineage, b.positive_area_component,
                    b.schema_version);
  }
  friend bool operator==(const output_face_region_key &a,
                         const output_face_region_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

// Paired-edge key: the complete Component 10 planned edge key, exact endpoint
// domains, and role/lineage.
struct paired_edge_key final {
  std::uint64_t planned_edge = 0;
  std::array<std::uint64_t, 2> endpoint_domains{
      output_topology_invalid_ordinal, output_topology_invalid_ordinal};
  std::uint64_t zero_measure_lineage = 0;
  std::uint16_t schema_version =
      contract_versions::output_topology_paired_edge_schema;

  static paired_edge_key canonical(std::uint64_t planned_edge,
                                   std::uint64_t start_domain,
                                   std::uint64_t end_domain) {
    paired_edge_key key;
    key.planned_edge = planned_edge;
    key.endpoint_domains[0] = std::min(start_domain, end_domain);
    key.endpoint_domains[1] = std::max(start_domain, end_domain);
    return key;
  }

  friend bool operator<(const paired_edge_key &a,
                        const paired_edge_key &b) noexcept {
    return std::tie(a.planned_edge, a.endpoint_domains, a.zero_measure_lineage,
                    a.schema_version) <
           std::tie(b.planned_edge, b.endpoint_domains, b.zero_measure_lineage,
                    b.schema_version);
  }
  friend bool operator==(const paired_edge_key &a,
                         const paired_edge_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

// Halfedge key: paired-edge key, directed incidence key, and canonical
// direction role.
struct output_halfedge_key final {
  std::uint64_t paired_edge = 0;
  std::uint64_t incidence = 0;
  direction_role direction = direction_role::forward;
  std::uint16_t schema_version =
      contract_versions::output_topology_halfedge_schema;

  friend bool operator<(const output_halfedge_key &a,
                        const output_halfedge_key &b) noexcept {
    return std::tie(a.paired_edge, a.incidence, a.direction, a.schema_version) <
           std::tie(b.paired_edge, b.incidence, b.direction, b.schema_version);
  }
  friend bool operator==(const output_halfedge_key &a,
                         const output_halfedge_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

// Role-independent face-cycle key: the orientation-preserving lexicographically
// least rotation of complete halfedge keys. Contour role is excluded.
struct face_cycle_key final {
  std::uint64_t region = 0;
  std::vector<output_halfedge_key> rotated_halfedges;
  std::uint16_t schema_version = contract_versions::output_topology_cycle_schema;

  friend bool operator<(const face_cycle_key &a,
                        const face_cycle_key &b) noexcept {
    return std::tie(a.region, a.rotated_halfedges, a.schema_version) <
           std::tie(b.region, b.rotated_halfedges, b.schema_version);
  }
  friend bool operator==(const face_cycle_key &a,
                         const face_cycle_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

// Contour key: final region key, cycle key, derived role, lineage.
struct contour_key final {
  std::uint64_t region = 0;
  std::uint64_t cycle = 0;
  contour_role role = contour_role::invalid;
  std::uint16_t schema_version =
      contract_versions::output_topology_contour_schema;

  friend bool operator<(const contour_key &a,
                        const contour_key &b) noexcept {
    return std::tie(a.region, a.cycle, a.role, a.schema_version) <
           std::tie(b.region, b.cycle, b.role, b.schema_version);
  }
  friend bool operator==(const contour_key &a,
                         const contour_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

// Witness key: region/support identity, subject cycle, intended side, source
// lineage, and version.
struct contour_witness_key final {
  std::uint64_t region = 0;
  std::uint64_t cycle = 0;
  witness_source source = witness_source::invalid;
  std::uint64_t atom = 0;
  std::uint16_t schema_version =
      contract_versions::output_topology_witness_schema;

  friend bool operator<(const contour_witness_key &a,
                        const contour_witness_key &b) noexcept {
    return std::tie(a.region, a.cycle, a.source, a.atom, a.schema_version) <
           std::tie(b.region, b.cycle, b.source, b.atom, b.schema_version);
  }
  friend bool operator==(const contour_witness_key &a,
                         const contour_witness_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

// ---------------------------------------------------------------------------
// Immutable artifact records (one per table entry).
// ---------------------------------------------------------------------------
struct output_incidence_audit_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t retained_incidence = output_topology_invalid_ordinal;
  std::uint64_t retained_use = output_topology_invalid_ordinal;
  std::uint64_t start_domain = output_topology_invalid_ordinal;
  std::uint64_t end_domain = output_topology_invalid_ordinal;
  output_incidence_disposition disposition = output_incidence_disposition::invalid;
  std::uint64_t planned_edge = output_topology_invalid_ordinal;
  std::uint64_t continuation = output_topology_invalid_ordinal;
  std::uint64_t consuming_output_entity = output_topology_invalid_ordinal;
};

struct zero_measure_support_evidence_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t retained_use = output_topology_invalid_ordinal;
  std::uint64_t attachment_lineage = 0;
  std::array<std::uint64_t, 6> nominal_bits{};
  std::array<std::uint64_t, 6> lower_bits{};
  std::array<std::uint64_t, 6> upper_bits{};
  bool no_topology_proof = false;
};

struct output_coordinate_reference_record final {
  std::uint64_t canonical_id = 0;
  coordinate_source source = coordinate_source::invalid;
  operand_id source_operand = operand_id::a;
  std::uint64_t source_lineage = 0;
  std::uint64_t construction_ledger = output_topology_invalid_ordinal;
  std::array<std::uint64_t, 3> nominal_bits{};
  std::array<std::uint64_t, 3> lower_bits{};
  std::array<std::uint64_t, 3> upper_bits{};
  std::uint64_t radial_error_bits = 0;
};

struct output_vertex_occurrence_record final {
  std::uint64_t canonical_id = 0;
  output_vertex_key key{};
  std::uint64_t endpoint_domain = output_topology_invalid_ordinal;
  std::uint64_t coordinate_reference = output_topology_invalid_ordinal;
  std::uint64_t ports_begin = 0;
  std::uint64_t ports_count = 0;
  std::uint64_t representative_port = output_topology_invalid_ordinal;
  std::uint64_t predecessor_occurrence = output_topology_invalid_ordinal;
};

struct output_face_region_record final {
  std::uint64_t canonical_id = 0;
  output_face_region_key key{};
  std::uint64_t members_begin = 0;
  std::uint64_t members_count = 0;
  std::uint64_t boundary_darts_begin = 0;
  std::uint64_t boundary_darts_count = 0;
  std::uint64_t continuation_members_begin = 0;
  std::uint64_t continuation_members_count = 0;
  bool positive_area = true;
  std::uint64_t outer_contour = output_topology_invalid_ordinal;
};

struct region_member_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t retained_use = output_topology_invalid_ordinal;
};

struct continuation_consumption_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t continuation = output_topology_invalid_ordinal;
  std::uint64_t forward_region = output_topology_invalid_ordinal;
  std::uint64_t reverse_region = output_topology_invalid_ordinal;
};

struct boundary_dart_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t incidence = output_topology_invalid_ordinal;
  std::uint64_t halfedge = output_topology_invalid_ordinal;
  std::uint64_t face_next = output_topology_invalid_ordinal;
  std::uint64_t face_prev = output_topology_invalid_ordinal;
  std::uint64_t region = output_topology_invalid_ordinal;
};

struct paired_output_edge_record final {
  std::uint64_t canonical_id = 0;
  paired_edge_key key{};
  std::uint64_t planned_edge = output_topology_invalid_ordinal;
  std::uint64_t halfedge0 = output_topology_invalid_ordinal;
  std::uint64_t halfedge1 = output_topology_invalid_ordinal;
  edge_role role = edge_role::invalid;
  endpoint_nominal_relation endpoint_relation =
      endpoint_nominal_relation::invalid;
  std::uint64_t zero_measure_boundary = output_topology_invalid_ordinal;
};

struct output_halfedge_record final {
  std::uint64_t canonical_id = 0;
  output_halfedge_key key{};
  std::uint64_t pair = output_topology_invalid_ordinal;
  std::uint64_t origin = output_topology_invalid_ordinal;
  std::uint64_t destination = output_topology_invalid_ordinal;
  std::uint64_t successor = output_topology_invalid_ordinal;
  std::uint64_t predecessor = output_topology_invalid_ordinal;
  std::uint64_t region = output_topology_invalid_ordinal;
  std::uint64_t cycle = output_topology_invalid_ordinal;
  std::uint64_t incidence = output_topology_invalid_ordinal;
  std::uint64_t paired_edge = output_topology_invalid_ordinal;
  direction_role direction = direction_role::forward;
  std::uint64_t endpoint_fan_begin = 0;
  std::uint64_t endpoint_fan_count = 0;
};

struct halfedge_endpoint_fan_ref_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t port = output_topology_invalid_ordinal;
  endpoint_role role = endpoint_role::start;
};

struct face_cycle_record final {
  std::uint64_t canonical_id = 0;
  face_cycle_key key{};
  std::uint64_t region = output_topology_invalid_ordinal;
  std::uint64_t refs_begin = 0;
  std::uint64_t refs_count = 0;
  std::uint64_t start_halfedge = output_topology_invalid_ordinal;
  cycle_geometric_category category = cycle_geometric_category::invalid;
};

struct cycle_halfedge_ref_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t halfedge = output_topology_invalid_ordinal;
};

struct contour_node_record final {
  std::uint64_t canonical_id = 0;
  contour_key key{};
  std::uint64_t cycle = output_topology_invalid_ordinal;
  std::uint64_t region = output_topology_invalid_ordinal;
  contour_role role = contour_role::invalid;
  std::uint64_t witness = output_topology_invalid_ordinal;
};

struct contour_witness_record final {
  std::uint64_t canonical_id = 0;
  contour_witness_key key{};
  std::uint64_t region = output_topology_invalid_ordinal;
  std::uint64_t cycle = output_topology_invalid_ordinal;
  std::uint64_t atom = output_topology_invalid_ordinal;
  std::array<std::uint64_t, 3> nominal_bits{};
  std::array<std::uint64_t, 6> enclosure_bits{};
  bool strict_side = false;
};

struct zero_measure_boundary_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t paired_edge = output_topology_invalid_ordinal;
  std::array<std::uint64_t, 3> nominal_bits{};
  std::array<std::uint64_t, 3> lower_bits{};
  std::array<std::uint64_t, 3> upper_bits{};
  std::uint64_t lower_length_bits = 0;
  std::uint64_t upper_length_bits = 0;
};

struct admissibility_evidence_record final {
  std::uint64_t canonical_id = 0;
  bounded_admissibility disposition = bounded_admissibility::invalid;
  std::uint64_t subject_a = 0;
  std::uint64_t subject_b = 0;
  std::uint64_t lineage = 0;
};

struct carrier_balance_audit_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t carrier_lineage = 0;
  std::uint64_t members_begin = 0;
  std::uint64_t members_count = 0;
  bool balanced = true;
};

struct vertex_link_evidence_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t vertex_occurrence = output_topology_invalid_ordinal;
  std::uint64_t outgoing_begin = 0;
  std::uint64_t outgoing_count = 0;
};

} // namespace ygor::mesh_boolean::bounded
