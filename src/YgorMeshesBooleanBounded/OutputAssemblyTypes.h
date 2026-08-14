#pragma once

#include "CanonicalBytes.h"
#include "CleanupTypes.h"
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

inline constexpr char output_assembly_provider_identity_v1[] =
    "direct_private_fv_surface_mesh_triangles_v1";

inline constexpr std::uint64_t assembly_invalid_ordinal =
    std::numeric_limits<std::uint64_t>::max();

struct assembly_component_tag;
struct public_vertex_position_tag;
struct public_facet_position_tag;
struct output_report_record_tag;
struct coordinate_copy_record_tag;
struct output_provenance_record_tag;

using assembly_component_id = strong_id<assembly_component_tag>;
using public_vertex_position_id = strong_id<public_vertex_position_tag>;
using public_facet_position_id = strong_id<public_facet_position_tag>;
using output_report_record_id = strong_id<output_report_record_tag>;
using coordinate_copy_record_id = strong_id<coordinate_copy_record_tag>;
using output_provenance_record_id = strong_id<output_provenance_record_tag>;

enum class output_assembly_provider_kind : std::uint8_t {
  direct_private_fv_surface_mesh_triangles_v1 = 1,
};

enum class candidate_topology_status : std::uint8_t {
  assembled_pending_independent_verification = 1,
  invalid = 2,
};

enum class candidate_geometry_status : std::uint8_t {
  finite_and_bounded_pending_independent_verification = 1,
  invalid = 2,
};

enum class coordinate_copy_disposition : std::uint8_t {
  exact_bits_preserved = 1,
  invalid = 2,
};

enum class output_assembly_verification_disposition : std::uint8_t {
  not_verified = 0,
  independently_verified = 1,
};

enum class output_assembly_checkpoint : std::uint32_t {
  context_capability_validation = 1,
  predecessor_validation = 2,
  cleaned_audit = 3,
  count_preflight = 4,
  resource_reservation = 5,
  component_reconstruction = 6,
  graph_construction = 7,
  partition_refinement = 8,
  canonical_labeling = 9,
  component_ordering = 10,
  vertex_permutation = 11,
  facet_ordering = 12,
  map_construction = 13,
  public_mesh_construction = 14,
  topology_reconstruction = 15,
  report_assembly = 16,
  provenance_assembly = 17,
  canonical_encoding = 18,
  digest_construction = 19,
  round_trip = 20,
  candidate_construction = 21,
  independent_verification = 22,
  resource_reconciliation = 23,
  commit = 24,
};

enum class output_assembly_subcode : std::uint32_t {
  unsupported_version = 140001,
  wrong_owner = 140002,
  wrong_operation = 140003,
  predecessor_digest_mismatch = 140004,
  predecessor_not_verified = 140005,
  malformed_cleaned_record = 140006,
  count_overflow = 140007,
  public_index_capacity = 140008,
  resource_preflight = 140009,
  graph_relation_mismatch = 140010,
  refinement_inconsistency = 140011,
  labeling_work_exhausted = 140012,
  automorphism_inconsistency = 140013,
  component_ordering_contradiction = 140014,
  permutation_non_bijective = 140015,
  triangle_rotation_mismatch = 140016,
  coordinate_copy_mismatch = 140017,
  face_write_readback_mismatch = 140018,
  edge_use_mismatch = 140019,
  vertex_link_mismatch = 140020,
  component_mismatch = 140021,
  duplicate_coordinate_weld = 140022,
  map_mismatch = 140023,
  precision_aggregation_mismatch = 140024,
  provenance_missing = 140025,
  codec_error = 140026,
  digest_mismatch = 140027,
  round_trip_mismatch = 140028,
  pending_status_violation = 140029,
  verifier_rejection = 140030,
  resource_reconciliation_failed = 140031,
  cancelled = 140032,
  internal_invariant = 140033,
  strict_profile_mismatch = 140034,
};

inline bounded_boolean_error output_assembly_error(
    output_assembly_subcode subcode, bounded_boolean_error_category category,
    const char *summary, output_assembly_checkpoint checkpoint) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 14;
  error.stage = static_cast<std::uint16_t>(stage_id::publication);
  error.checkpoint = static_cast<std::uint32_t>(checkpoint);
  error.summary = summary;
  return error;
}

struct output_assembly_cancellation_observer final {
  std::uint16_t version = contract_versions::output_assembly_verifier;
  std::uint16_t reserved16 = 0;
  void *state = nullptr;
  void (*poll)(void *, output_assembly_checkpoint) noexcept = nullptr;
  std::uint32_t reserved32 = 0;
};

struct output_assembly_capabilities final {
  std::uint16_t provider_version = contract_versions::output_assembly_provider;
  std::uint16_t codec_version = contract_versions::output_assembly_codec;
  std::uint16_t verifier_version = contract_versions::output_assembly_verifier;
  context_owner_token owner{};
  const bounded_boolean_cancellation_token *cancellation = nullptr;
  const output_assembly_cancellation_observer *cancellation_observer = nullptr;
  resource_manager *resources = nullptr;
  std::uint64_t maximum_vertices = (std::uint64_t{1} << 34);
  std::uint64_t maximum_facets = (std::uint64_t{1} << 37);
  std::uint64_t maximum_canonical_bytes = (std::uint64_t{1} << 35);
  std::uint64_t maximum_labeling_states = (std::uint64_t{1} << 40);
  std::uint64_t maximum_work_units = (std::uint64_t{1} << 40);
  std::uint32_t reserved = 0;
};

struct output_assembly_codec_limits final {
  std::uint64_t maximum_section_bytes = (std::uint64_t{1} << 34);
  std::uint64_t reserved = 0;
};

inline bool output_assembly_cancelled(
    const output_assembly_capabilities &capabilities,
    output_assembly_checkpoint checkpoint) noexcept {
  if (capabilities.cancellation_observer &&
      capabilities.cancellation_observer->poll)
    capabilities.cancellation_observer->poll(
        capabilities.cancellation_observer->state, checkpoint);
  return capabilities.cancellation &&
         capabilities.cancellation->cancellation_requested();
}

struct output_assembly_statistics final {
  std::uint64_t component_count = 0;
  std::uint64_t vertex_count = 0;
  std::uint64_t facet_count = 0;
  std::uint64_t graph_node_count = 0;
  std::uint64_t refinement_rounds = 0;
  std::uint64_t individualization_branches = 0;
  std::uint64_t verifier_work_units = 0;
  std::uint64_t persistent_bytes = 0;
  std::uint64_t canonical_bytes = 0;
};

// ---------------------------------------------------------------------------
// Immutable candidate records.
// ---------------------------------------------------------------------------
struct assembly_component_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t vertex_begin = 0;
  std::uint64_t vertex_count = 0;
  std::uint64_t facet_begin = 0;
  std::uint64_t facet_count = 0;
  std::uint64_t edge_count = 0;
  std::int64_t euler_chi = 0;
};

struct coordinate_copy_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t public_vertex = 0;
  std::uint64_t cleaned_occurrence = 0;
  std::array<std::uint64_t, 3> output_bits{};
  coordinate_copy_disposition disposition =
      coordinate_copy_disposition::invalid;
};

struct output_report_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t vertex_count = 0;
  std::uint64_t edge_count = 0;
  std::uint64_t facet_count = 0;
  std::uint64_t component_count = 0;
  std::uint64_t output_precision_bits = 0;
  std::uint64_t maximum_authorized_tolerance_bits = 0;
};

} // namespace ygor::mesh_boolean::bounded
