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

inline constexpr char selection_provider_identity_v1[] =
    "side_truth_and_lineage_selection_v1";
inline constexpr char selection_coincidence_provider_identity_v1[] =
    "symbolic_sheet_cell_ownership_v1";
inline constexpr char selection_retained_incidence_provider_identity_v1[] =
    "semantic_boundary_incidence_v1";
inline constexpr char selection_edge_occurrence_provider_identity_v1[] =
    "exact_mate_signature_pairs_v1";
inline constexpr char selection_vertex_occurrence_provider_identity_v1[] =
    "degree_two_local_link_cycles_v1";
inline constexpr char selection_feasibility_provider_identity_v1[] =
    "complete_retained_complex_audit_v1";

inline constexpr std::uint64_t selection_invalid_ordinal =
    std::numeric_limits<std::uint64_t>::max();

// ---------------------------------------------------------------------------
// Strong identity domains. Component 10 never aliases these to predecessor
// IDs, raw `I`, `size_t`, or offsets.
// ---------------------------------------------------------------------------
struct selection_disposition_tag;
struct selection_side_tuple_tag;
struct selection_truth_evidence_tag;
struct coincidence_sheet_cell_tag;
struct coincidence_member_tag;
struct owner_decision_tag;
struct suppression_record_tag;
struct multiplicity_tag;
struct retained_surface_use_tag;
struct retained_use_provenance_tag;
struct retained_incidence_tag;
struct continuation_tag;
struct edge_mate_group_tag;
struct planned_edge_occurrence_tag;
struct carrier_balance_tag;
struct endpoint_domain_tag;
struct local_port_tag;
struct face_corner_arc_tag;
struct edge_mate_arc_tag;
struct local_link_component_tag;
struct vertex_occurrence_requirement_tag;
struct selection_evidence_tag;

using selection_disposition_id = strong_id<selection_disposition_tag>;
using selection_side_tuple_id = strong_id<selection_side_tuple_tag>;
using selection_truth_evidence_id = strong_id<selection_truth_evidence_tag>;
using coincidence_sheet_cell_id = strong_id<coincidence_sheet_cell_tag>;
using coincidence_member_id = strong_id<coincidence_member_tag>;
using owner_decision_id = strong_id<owner_decision_tag>;
using suppression_record_id = strong_id<suppression_record_tag>;
using multiplicity_id = strong_id<multiplicity_tag>;
using retained_surface_use_id = strong_id<retained_surface_use_tag>;
using retained_use_provenance_id = strong_id<retained_use_provenance_tag>;
using retained_incidence_id = strong_id<retained_incidence_tag>;
using continuation_id = strong_id<continuation_tag>;
using edge_mate_group_id = strong_id<edge_mate_group_tag>;
using planned_edge_occurrence_id = strong_id<planned_edge_occurrence_tag>;
using carrier_balance_id = strong_id<carrier_balance_tag>;
using endpoint_domain_id = strong_id<endpoint_domain_tag>;
using local_port_id = strong_id<local_port_tag>;
using face_corner_arc_id = strong_id<face_corner_arc_tag>;
using edge_mate_arc_id = strong_id<edge_mate_arc_tag>;
using local_link_component_id = strong_id<local_link_component_tag>;
using vertex_occurrence_requirement_id =
    strong_id<vertex_occurrence_requirement_tag>;
using selection_evidence_id = strong_id<selection_evidence_tag>;

// ---------------------------------------------------------------------------
// Closed enums. Every value is explicit and nonzero; unknown values fail
// decoding with no default fallthrough.
// ---------------------------------------------------------------------------
enum class selection_provider_kind : std::uint8_t {
  side_truth_and_lineage_selection_v1 = 1,
};

enum class selection_verification_disposition : std::uint8_t {
  not_verified = 0,
  independently_verified = 1,
};

enum class final_disposition : std::uint8_t {
  retain_preserve = 1,
  retain_reverse = 2,
  discard_equal_sides = 3,
  suppress_internal = 4,
  suppress_non_owner = 5,
  cancel_coincident = 6,
  represented_by_multiplicity = 7,
  invalid = 8,
};

enum class side_state_origin : std::uint8_t {
  source_shell = 1,
  numeric_classification = 2,
  boundary_derived = 3,
  symbolic = 4,
  coincident_owned = 5,
};

enum class sheet_relation : std::uint8_t {
  ordinary = 1,
  same_orientation_coincidence = 2,
  opposite_orientation_coincidence = 3,
  partial_overlap = 4,
  occurrence_distinct_coincidence = 5,
};

enum class incidence_disposition : std::uint8_t {
  planned_edge = 1,
  transparent_continuation = 2,
  topology_separation_delimiter = 3,
  zero_measure_support = 4,
  consumed_owner_seam = 5,
  suppressed_audit_only = 6,
};

enum class carrier_kind : std::uint8_t {
  whole_source_edge = 1,
  source_edge_interval = 2,
  transverse_carrier = 3,
  coplanar_overlap_boundary = 4,
  contact_delimiter = 5,
};

enum class endpoint_role : std::uint8_t {
  start = 1,
  end = 2,
};

enum class direction_role : std::uint8_t {
  forward = 1,
  reverse = 2,
};

enum class arc_kind : std::uint8_t {
  face_corner = 1,
  edge_mate = 2,
};

enum class endpoint_domain_kind : std::uint8_t {
  source_vertex = 1,
  event_occurrence = 2,
  source_edge_interval_endpoint = 3,
  carrier_endpoint = 4,
  contact_delimiter = 5,
};

enum class occurrence_separation_class : std::uint8_t {
  ordinary = 1,
  topology_separated_contact = 2,
  coincident_occurrence = 3,
  multiplicity_slot = 4,
};

enum class verifier_disposition : std::uint8_t {
  accepted = 1,
  rejected = 2,
};

enum class selection_checkpoint : std::uint32_t {
  context_capability_validation = 1,
  predecessor_validation = 2,
  count_preflight = 3,
  resource_reservation = 4,
  atom_coverage_validation = 5,
  side_tuple_construction = 6,
  truth_evaluation = 7,
  sheet_cell_reconstruction = 8,
  owner_resolution = 9,
  disposition_audit = 10,
  retained_use_construction = 11,
  incidence_normalization = 12,
  continuation_construction = 13,
  edge_pairing = 14,
  local_port_construction = 15,
  edge_mate_arc_construction = 16,
  local_link_cycle_extraction = 17,
  endpoint_remap = 18,
  feasibility_audit = 19,
  canonicalization = 20,
  producer_checks = 21,
  canonical_encoding = 22,
  independent_verification = 23,
  resource_reconciliation = 24,
  commit = 25,
};

enum class selection_subcode : std::uint32_t {
  unsupported_version = 100001,
  wrong_owner = 100002,
  wrong_operation = 100003,
  wrong_operand_role = 100004,
  predecessor_digest_mismatch = 100005,
  predecessor_not_verified = 100006,
  count_overflow = 100007,
  byte_count_overflow = 100008,
  work_limit = 100009,
  resource_preflight = 100010,
  cancelled = 100011,
  missing_atom = 100012,
  duplicate_atom = 100013,
  invalid_side_tuple = 100014,
  invalid_side_state = 100015,
  invalid_truth_cell = 100016,
  wrong_retain_decision = 100017,
  wrong_orientation = 100018,
  malformed_sheet_cell = 100019,
  sheet_membership_incomplete = 100020,
  wrong_owner_decision = 100021,
  multiple_owners = 100022,
  no_owner = 100023,
  internal_sheet_retained = 100024,
  required_sheet_suppressed = 100025,
  multiplicity_corruption = 100026,
  retained_use_provenance_mismatch = 100027,
  incidence_endpoint_error = 100028,
  incidence_direction_error = 100029,
  incidence_carrier_error = 100030,
  internal_diagonal_leakage = 100031,
  invalid_continuation = 100032,
  nonreciprocal_continuation = 100033,
  malformed_surface_occurrence_descriptor = 100034,
  mate_cardinality_error = 100035,
  mate_direction_error = 100036,
  mate_endpoint_incompatibility = 100037,
  nonreciprocal_expected_mate = 100038,
  slot_discriminator_mismatch = 100039,
  same_owner_requirement_error = 100040,
  incidence_underconsumption = 100041,
  incidence_overconsumption = 100042,
  malformed_endpoint_domain = 100043,
  malformed_local_port = 100044,
  malformed_arc = 100045,
  open_local_link = 100046,
  branched_local_link = 100047,
  multicycle_local_link = 100048,
  occurrence_separation_crossing = 100049,
  endpoint_remap_mismatch = 100050,
  carrier_balance_failure = 100051,
  canonicalization_error = 100052,
  reverse_map_error = 100053,
  codec_error = 100054,
  digest_mismatch = 100055,
  verifier_rejection = 100056,
  resource_reconciliation_failed = 100057,
  transaction_failure = 100058,
  internal_invariant = 100059,
};

inline bounded_boolean_error selection_error(
    selection_subcode subcode, bounded_boolean_error_category category,
    const char *summary, selection_checkpoint checkpoint) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 10;
  error.stage = static_cast<std::uint16_t>(stage_id::selection);
  error.checkpoint = static_cast<std::uint32_t>(checkpoint);
  error.summary = summary;
  return error;
}

struct selection_cancellation_observer final {
  std::uint16_t version = contract_versions::selection_verifier;
  std::uint16_t reserved16 = 0;
  void *state = nullptr;
  void (*poll)(void *, selection_checkpoint) noexcept = nullptr;
  std::uint32_t reserved32 = 0;
};

struct selection_capabilities final {
  std::uint16_t provider_version = contract_versions::selection_provider;
  std::uint16_t truth_provider_version = contract_versions::selection_truth_provider;
  std::uint16_t coincidence_provider_version =
      contract_versions::selection_coincidence_provider;
  std::uint16_t incidence_provider_version =
      contract_versions::selection_retained_incidence_provider;
  std::uint16_t edge_provider_version =
      contract_versions::selection_edge_occurrence_provider;
  std::uint16_t vertex_provider_version =
      contract_versions::selection_vertex_occurrence_provider;
  std::uint16_t feasibility_provider_version =
      contract_versions::selection_feasibility_provider;
  std::uint16_t codec_version = contract_versions::selection_codec;
  std::uint16_t verifier_version = contract_versions::selection_verifier;
  context_owner_token owner{};
  const bounded_boolean_cancellation_token *cancellation = nullptr;
  const selection_cancellation_observer *cancellation_observer = nullptr;
  resource_manager *resources = nullptr;
  std::uint64_t maximum_dispositions = (std::uint64_t{1} << 34);
  std::uint64_t maximum_sheet_cells = (std::uint64_t{1} << 34);
  std::uint64_t maximum_retained_uses = (std::uint64_t{1} << 34);
  std::uint64_t maximum_incidences = (std::uint64_t{1} << 37);
  std::uint64_t maximum_continuations = (std::uint64_t{1} << 36);
  std::uint64_t maximum_edge_occurrences = (std::uint64_t{1} << 36);
  std::uint64_t maximum_vertex_occurrences = (std::uint64_t{1} << 34);
  std::uint64_t maximum_local_ports = (std::uint64_t{1} << 37);
  std::uint64_t maximum_canonical_bytes = (std::uint64_t{1} << 35);
  std::uint64_t maximum_work_units = (std::uint64_t{1} << 40);
  std::uint32_t reserved = 0;
};

inline bool selection_cancelled(
    const selection_capabilities &capabilities,
    selection_checkpoint checkpoint) noexcept {
  if (capabilities.cancellation_observer &&
      capabilities.cancellation_observer->poll)
    capabilities.cancellation_observer->poll(
        capabilities.cancellation_observer->state, checkpoint);
  return capabilities.cancellation &&
         capabilities.cancellation->cancellation_requested();
}

struct selection_statistics final {
  std::uint64_t atom_count = 0;
  std::uint64_t disposition_count = 0;
  std::uint64_t side_tuple_count = 0;
  std::uint64_t sheet_cell_count = 0;
  std::uint64_t member_count = 0;
  std::uint64_t owner_decision_count = 0;
  std::uint64_t suppression_count = 0;
  std::uint64_t multiplicity_count = 0;
  std::uint64_t retained_use_count = 0;
  std::uint64_t incidence_count = 0;
  std::uint64_t continuation_count = 0;
  std::uint64_t edge_occurrence_count = 0;
  std::uint64_t carrier_balance_count = 0;
  std::uint64_t endpoint_domain_count = 0;
  std::uint64_t local_port_count = 0;
  std::uint64_t face_corner_arc_count = 0;
  std::uint64_t edge_mate_arc_count = 0;
  std::uint64_t link_component_count = 0;
  std::uint64_t vertex_occurrence_count = 0;
  std::uint64_t verifier_work_units = 0;
  std::uint64_t persistent_bytes = 0;
  std::uint64_t canonical_bytes = 0;
};

// ---------------------------------------------------------------------------
// Canonical identity keys. They never contain nominal coordinates, hashes,
// discovery order, worker/task identities, or future output IDs.
// ---------------------------------------------------------------------------

// The side occupancy tuple is the complete 4-bit input to the frozen Component
// 01 truth table.
struct selection_side_occupancy final {
  bool a_negative = false;
  bool b_negative = false;
  bool a_positive = false;
  bool b_positive = false;

  friend bool operator==(const selection_side_occupancy &a,
                         const selection_side_occupancy &b) noexcept {
    return std::tie(a.a_negative, a.b_negative, a.a_positive, a.b_positive) ==
           std::tie(b.a_negative, b.b_negative, b.a_positive, b.b_positive);
  }
  friend bool operator!=(const selection_side_occupancy &a,
                         const selection_side_occupancy &b) noexcept {
    return !(a == b);
  }
};

struct selection_disposition_key final {
  operand_id source_operand = operand_id::a;
  std::uint64_t atom_ordinal = 0;
  std::uint64_t shell = 0;
  std::uint64_t source_facet = 0;
  selection_side_occupancy occupancy{};
  boolean_operation operation = boolean_operation::set_union;
  std::uint16_t truth_version = contract_versions::selection_truth_provider;
  std::uint16_t symbolic_version = contract_versions::symbolic_policy;
  std::uint16_t schema_version = contract_versions::selection_disposition_schema;

  friend bool operator<(const selection_disposition_key &a,
                        const selection_disposition_key &b) noexcept {
    return std::tie(a.source_operand, a.atom_ordinal, a.shell, a.source_facet,
                    a.occupancy.a_negative, a.occupancy.b_negative,
                    a.occupancy.a_positive, a.occupancy.b_positive,
                    a.operation, a.truth_version, a.symbolic_version,
                    a.schema_version) <
           std::tie(b.source_operand, b.atom_ordinal, b.shell, b.source_facet,
                    b.occupancy.a_negative, b.occupancy.b_negative,
                    b.occupancy.a_positive, b.occupancy.b_positive,
                    b.operation, b.truth_version, b.symbolic_version,
                    b.schema_version);
  }
  friend bool operator==(const selection_disposition_key &a,
                         const selection_disposition_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

// One directed retained incidence. This identifies a single face use, not an
// undirected edge identity. The two member descriptors of a valid planned edge
// may differ in source operand, source facet, sheet owner, and multiplicity.
struct surface_occurrence_descriptor final {
  operand_id source_operand = operand_id::a;
  std::uint64_t source_shell = 0;
  std::uint64_t source_facet = 0;
  std::uint64_t sheet_owner_lineage = 0;
  std::uint64_t retained_use_lineage = 0;
  std::uint32_t multiplicity_occurrence = 0;
  std::int8_t result_side_transition = 0; // -1, 0, or 1
  std::uint64_t start_sector_lineage = 0;
  std::uint64_t end_sector_lineage = 0;
  occurrence_separation_class separation =
      occurrence_separation_class::ordinary;
  std::uint64_t separation_lineage = 0;
  std::uint16_t schema_version =
      contract_versions::selection_surface_occurrence_descriptor_schema;

  friend bool operator<(const surface_occurrence_descriptor &a,
                        const surface_occurrence_descriptor &b) noexcept {
    return std::tie(a.source_operand, a.source_shell, a.source_facet,
                    a.sheet_owner_lineage, a.retained_use_lineage,
                    a.multiplicity_occurrence, a.result_side_transition,
                    a.start_sector_lineage, a.end_sector_lineage, a.separation,
                    a.separation_lineage, a.schema_version) <
           std::tie(b.source_operand, b.source_shell, b.source_facet,
                    b.sheet_owner_lineage, b.retained_use_lineage,
                    b.multiplicity_occurrence, b.result_side_transition,
                    b.start_sector_lineage, b.end_sector_lineage, b.separation,
                    b.separation_lineage, b.schema_version);
  }
  friend bool operator==(const surface_occurrence_descriptor &a,
                         const surface_occurrence_descriptor &b) noexcept {
    return !(a < b) && !(b < a);
  }
  friend bool operator!=(const surface_occurrence_descriptor &a,
                         const surface_occurrence_descriptor &b) noexcept {
    return !(a == b);
  }
};

// Canonical unordered descriptor pair: sorted by descriptor order, never by
// operand preference or hash order.
struct surface_occurrence_descriptor_pair final {
  surface_occurrence_descriptor first{};
  surface_occurrence_descriptor second{};

  static surface_occurrence_descriptor_pair ordered(
      surface_occurrence_descriptor a, surface_occurrence_descriptor b) {
    surface_occurrence_descriptor_pair out;
    if (b < a)
      std::swap(a, b);
    out.first = a;
    out.second = b;
    return out;
  }

  friend bool operator<(const surface_occurrence_descriptor_pair &a,
                        const surface_occurrence_descriptor_pair &b) noexcept {
    return std::tie(a.first, a.second) < std::tie(b.first, b.second);
  }
  friend bool operator==(const surface_occurrence_descriptor_pair &a,
                         const surface_occurrence_descriptor_pair &b) noexcept {
    return a.first == b.first && a.second == b.second;
  }
};

// The undirected carrier identity shared by the two members of a planned edge.
struct selection_carrier_identity final {
  carrier_kind kind = carrier_kind::whole_source_edge;
  operand_id source_operand = operand_id::a;
  std::uint64_t lineage = 0;
  std::uint64_t span_lineage = 0;
  occurrence_separation_class separation =
      occurrence_separation_class::ordinary;
  std::uint64_t separation_lineage = 0;
  std::uint16_t schema_version = contract_versions::selection_incidence_schema;

  friend bool operator<(const selection_carrier_identity &a,
                        const selection_carrier_identity &b) noexcept {
    return std::tie(a.kind, a.source_operand, a.lineage, a.span_lineage,
                    a.separation, a.separation_lineage, a.schema_version) <
           std::tie(b.kind, b.source_operand, b.lineage, b.span_lineage,
                    b.separation, b.separation_lineage, b.schema_version);
  }
  friend bool operator==(const selection_carrier_identity &a,
                         const selection_carrier_identity &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

// Undirected edge-occurrence slot key. Groups the two directed members that
// share one future output edge.
struct edge_occurrence_slot_key final {
  selection_carrier_identity carrier{};
  std::array<std::uint64_t, 2> endpoint_domains{
      selection_invalid_ordinal, selection_invalid_ordinal};
  std::uint64_t output_slot_discriminator = 0;
  surface_occurrence_descriptor_pair expected_pair{};
  std::uint16_t schema_version = contract_versions::selection_edge_occurrence_schema;

  static edge_occurrence_slot_key canonical(
      selection_carrier_identity carrier, std::uint64_t start_domain,
      std::uint64_t end_domain, std::uint64_t discriminator,
      surface_occurrence_descriptor_pair expected) {
    edge_occurrence_slot_key key;
    key.carrier = carrier;
    key.endpoint_domains[0] = std::min(start_domain, end_domain);
    key.endpoint_domains[1] = std::max(start_domain, end_domain);
    key.output_slot_discriminator = discriminator;
    key.expected_pair = expected;
    return key;
  }

  friend bool operator<(const edge_occurrence_slot_key &a,
                        const edge_occurrence_slot_key &b) noexcept {
    return std::tie(a.carrier, a.endpoint_domains, a.output_slot_discriminator,
                    a.expected_pair, a.schema_version) <
           std::tie(b.carrier, b.endpoint_domains, b.output_slot_discriminator,
                    b.expected_pair, b.schema_version);
  }
  friend bool operator==(const edge_occurrence_slot_key &a,
                         const edge_occurrence_slot_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

// Local port identity at one endpoint of one directed incidence.
struct local_port_key final {
  std::uint64_t endpoint_domain = 0;
  std::uint64_t incidence = 0;
  std::uint64_t retained_use = 0;
  endpoint_role role = endpoint_role::start;
  std::uint64_t sector_lineage = 0;
  occurrence_separation_class separation =
      occurrence_separation_class::ordinary;
  std::uint64_t separation_lineage = 0;
  std::uint16_t schema_version = contract_versions::selection_vertex_occurrence_schema;

  friend bool operator<(const local_port_key &a, const local_port_key &b) noexcept {
    return std::tie(a.endpoint_domain, a.incidence, a.retained_use, a.role,
                    a.sector_lineage, a.separation, a.separation_lineage,
                    a.schema_version) <
           std::tie(b.endpoint_domain, b.incidence, b.retained_use, b.role,
                    b.sector_lineage, b.separation, b.separation_lineage,
                    b.schema_version);
  }
  friend bool operator==(const local_port_key &a,
                         const local_port_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

// ---------------------------------------------------------------------------
// Immutable artifact records (one per table entry).
// ---------------------------------------------------------------------------
struct selection_side_tuple_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t atom = selection_invalid_ordinal;
  operand_id source_operand = operand_id::a;
  std::uint64_t source_shell = 0;
  selection_side_occupancy occupancy{};
  std::array<side_state_origin, 4> origins{};
  bool coincident = false;
  std::uint64_t sheet_cell = selection_invalid_ordinal;
};

struct selection_truth_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t side_tuple = selection_invalid_ordinal;
  std::uint64_t atom = selection_invalid_ordinal;
  boolean_operation operation = boolean_operation::set_union;
  bool result_negative = false;
  bool result_positive = false;
  bool retain = false;
  boundary_orientation orientation = boundary_orientation::not_applicable;
  std::uint8_t multiplicity = 0;
  std::uint8_t owner_priority = 0;
  std::uint16_t truth_version = contract_versions::truth_table;
};

struct selection_disposition_record final {
  std::uint64_t canonical_id = 0;
  selection_disposition_key key{};
  std::uint64_t atom = selection_invalid_ordinal;
  std::uint64_t side_tuple = selection_invalid_ordinal;
  std::uint64_t truth = selection_invalid_ordinal;
  final_disposition disposition = final_disposition::invalid;
  std::uint64_t retained_use = selection_invalid_ordinal;
  std::uint64_t sheet_cell = selection_invalid_ordinal;
  std::uint64_t multiplicity = selection_invalid_ordinal;
  bool source_orientation_reversed = false;
};

struct coincidence_member_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t sheet_cell = selection_invalid_ordinal;
  std::uint64_t atom = selection_invalid_ordinal;
  operand_id source_operand = operand_id::a;
  std::uint64_t source_facet = 0;
};

struct coincidence_sheet_cell_record final {
  std::uint64_t canonical_id = 0;
  sheet_relation relation = sheet_relation::ordinary;
  std::uint64_t support_lineage = 0;
  bool opposite_orientation = false;
  operand_id symbolic_owner = operand_id::a;
  std::uint64_t members_begin = 0;
  std::uint64_t members_count = 0;
  std::uint64_t owner_atom = selection_invalid_ordinal;
  std::uint64_t owner_decision = selection_invalid_ordinal;
  std::uint64_t multiplicity = selection_invalid_ordinal;
  bool cancelled = false;
  bool internal = false;
  bool distinct_occurrences = false;
};

struct owner_decision_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t sheet_cell = selection_invalid_ordinal;
  std::uint64_t owner_atom = selection_invalid_ordinal;
  operand_id owner_operand = operand_id::a;
  std::array<std::uint64_t, 6> rank_components{};
  std::uint64_t symbolic_rule_ordinal = 0;
};

struct suppression_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t atom = selection_invalid_ordinal;
  final_disposition reason = final_disposition::suppress_internal;
  std::uint64_t owner_atom = selection_invalid_ordinal;
};

struct multiplicity_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t sheet_cell = selection_invalid_ordinal;
  std::uint32_t occurrence_count = 0;
  std::uint32_t occurrence_slot = 0;
  std::uint64_t separation_lineage = 0;
};

struct retained_use_provenance_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t retained_use = selection_invalid_ordinal;
  std::uint64_t atom = selection_invalid_ordinal;
  operand_id source_operand = operand_id::a;
  std::uint64_t source_shell = 0;
  std::uint64_t source_facet = 0;
  std::uint64_t source_triangle = selection_invalid_ordinal;
};

struct retained_surface_use_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t atom = selection_invalid_ordinal;
  operand_id source_operand = operand_id::a;
  std::uint64_t source_shell = 0;
  std::uint64_t source_facet = 0;
  bool preserve_source_orientation = true;
  selection_side_occupancy result_occupancy{};
  std::uint64_t sheet_owner_lineage = 0;
  std::uint64_t multiplicity_occurrence = 0;
  occurrence_separation_class separation =
      occurrence_separation_class::ordinary;
  std::uint64_t separation_lineage = 0;
  std::uint64_t provenance = selection_invalid_ordinal;
};

struct retained_incidence_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t retained_use = selection_invalid_ordinal;
  std::uint64_t atom = selection_invalid_ordinal;
  std::uint64_t start_domain = selection_invalid_ordinal;
  std::uint64_t end_domain = selection_invalid_ordinal;
  selection_carrier_identity carrier{};
  direction_role direction = direction_role::forward;
  std::uint64_t start_sector_lineage = 0;
  std::uint64_t end_sector_lineage = 0;
  surface_occurrence_descriptor descriptor{};
  surface_occurrence_descriptor expected_opposite{};
  incidence_disposition disposition = incidence_disposition::planned_edge;
  std::uint64_t continuation = selection_invalid_ordinal;
  std::uint64_t planned_edge = selection_invalid_ordinal;
  std::uint64_t carrier_balance = selection_invalid_ordinal;
};

struct continuation_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t forward_incidence = selection_invalid_ordinal;
  std::uint64_t reverse_incidence = selection_invalid_ordinal;
  std::uint64_t seam_lineage = 0;
};

struct edge_mate_group_record final {
  std::uint64_t canonical_id = 0;
  edge_occurrence_slot_key slot{};
  std::uint64_t forward_incidence = selection_invalid_ordinal;
  std::uint64_t reverse_incidence = selection_invalid_ordinal;
};

struct planned_edge_occurrence_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t mate_group = selection_invalid_ordinal;
  std::uint64_t start_domain = selection_invalid_ordinal;
  std::uint64_t end_domain = selection_invalid_ordinal;
  surface_occurrence_descriptor_pair expected_pair{};
  bool cross_operand = false;
  bool cross_owner = false;
};

struct carrier_balance_record final {
  std::uint64_t canonical_id = 0;
  selection_carrier_identity carrier{};
  std::uint64_t members_begin = 0;
  std::uint64_t members_count = 0;
  bool balanced = true;
};

struct endpoint_domain_record final {
  std::uint64_t canonical_id = 0;
  endpoint_domain_kind kind = endpoint_domain_kind::source_vertex;
  operand_id source_operand = operand_id::a;
  std::uint64_t lineage = 0;
  std::uint64_t event_occurrence = selection_invalid_ordinal;
  occurrence_separation_class separation =
      occurrence_separation_class::ordinary;
  std::uint64_t separation_lineage = 0;
};

struct local_port_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t endpoint_domain = selection_invalid_ordinal;
  std::uint64_t incidence = selection_invalid_ordinal;
  std::uint64_t retained_use = selection_invalid_ordinal;
  endpoint_role role = endpoint_role::start;
  std::uint64_t face_corner_arc = selection_invalid_ordinal;
  std::uint64_t edge_mate_arc = selection_invalid_ordinal;
};

struct face_corner_arc_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t in_port = selection_invalid_ordinal;
  std::uint64_t out_port = selection_invalid_ordinal;
  std::uint64_t retained_use = selection_invalid_ordinal;
};

struct edge_mate_arc_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t first_port = selection_invalid_ordinal;
  std::uint64_t second_port = selection_invalid_ordinal;
  std::uint64_t planned_edge = selection_invalid_ordinal;
};

struct local_link_component_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t endpoint_domain = selection_invalid_ordinal;
  std::uint64_t members_begin = 0;
  std::uint64_t members_count = 0;
  std::uint64_t vertex_occurrence = selection_invalid_ordinal;
};

struct vertex_occurrence_requirement_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t endpoint_domain = selection_invalid_ordinal;
  std::uint64_t cycle_begin = 0;
  std::uint64_t cycle_count = 0;
  std::uint64_t representative_port = selection_invalid_ordinal;
};

} // namespace ygor::mesh_boolean::bounded
