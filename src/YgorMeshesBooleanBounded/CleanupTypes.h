#pragma once

#include "CanonicalBytes.h"
#include "ContractVersions.h"
#include "Identity.h"
#include "OutputTriangulationTypes.h"
#include "Policies.h"
#include "Resources.h"
#include "Sha256.h"

#include <array>
#include <cstdint>
#include <limits>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

inline constexpr char cleanup_provider_identity_v1[] =
    "append_only_generation_checked_cleanup_complex_v1";

inline constexpr std::uint64_t cleanup_invalid_ordinal =
    std::numeric_limits<std::uint64_t>::max();

// ---------------------------------------------------------------------------
// Strong identity domains.
// ---------------------------------------------------------------------------
struct cleaned_vertex_tag;
struct cleaned_paired_edge_tag;
struct cleaned_halfedge_tag;
struct cleaned_triangle_tag;
struct cleaned_component_tag;
struct cleanup_obligation_tag;
struct cleanup_action_tag;
struct cleanup_action_certificate_tag;

using cleaned_vertex_id = strong_id<cleaned_vertex_tag>;
using cleaned_paired_edge_id = strong_id<cleaned_paired_edge_tag>;
using cleaned_halfedge_id = strong_id<cleaned_halfedge_tag>;
using cleaned_triangle_id = strong_id<cleaned_triangle_tag>;
using cleaned_component_id = strong_id<cleaned_component_tag>;
using cleanup_obligation_id = strong_id<cleanup_obligation_tag>;
using cleanup_action_id = strong_id<cleanup_action_tag>;
using cleanup_action_certificate_id = strong_id<cleanup_action_certificate_tag>;

// ---------------------------------------------------------------------------
// Closed enums.
// ---------------------------------------------------------------------------
enum class cleanup_provider_kind : std::uint8_t {
  append_only_generation_checked_cleanup_complex_v1 = 1,
};

enum class cleanup_verification_disposition : std::uint8_t {
  not_verified = 0,
  independently_verified = 1,
};

enum class cleanup_obligation_class : std::uint8_t {
  zero_area_triangle = 1,
  symbolic_tie_triangle = 2,
  prohibited_zero_length_edge = 3,
  collinear_chain_cell = 4,
  pinched_equal_coordinate_patch = 5,
  deferred_zero_measure_contour = 6,
  sub_threshold_sliver = 7,
  component_removal_review = 8,
  verifier_discovered_defect = 9,
  aggregate = 10,
  invalid = 11,
};

enum class cleanup_obligation_disposition : std::uint8_t {
  pending = 1,
  proven_final_valid = 2,
  discharged_by_action = 3,
  absorbed_with_member_evidence = 4,
  removed_with_component = 5,
  rejected_budget = 6,
  rejected_topology = 7,
  rejected_geometry = 8,
  failed_resource = 9,
  failed_cancellation = 10,
  failed_invariant = 11,
  invalid = 12,
};

enum class final_cleanup_status : std::uint8_t {
  verified_clean = 1,
  verified_empty = 2,
  failed_outstanding_obligations = 3,
  invalid = 4,
};

enum class cleanup_checkpoint : std::uint32_t {
  context_capability_validation = 1,
  predecessor_validation = 2,
  count_preflight = 3,
  resource_reservation = 4,
  mutable_complex_construction = 5,
  pair_cell_fan_audit = 6,
  obligation_import = 7,
  defect_scan = 8,
  interaction_index_construction = 9,
  candidate_discovery = 10,
  candidate_ordering = 11,
  stale_revalidation = 12,
  patch_collection = 13,
  topology_eligibility = 14,
  proposal_construction = 15,
  budget_reservation = 16,
  replacement_construction = 17,
  replacement_verification = 18,
  external_interaction = 19,
  certificate_construction = 20,
  patch_splice = 21,
  obligation_transition = 22,
  progress_audit = 23,
  final_manifold_verification = 24,
  canonicalization = 25,
  canonical_encoding = 26,
  independent_verification = 27,
  idempotence_verification = 28,
  resource_reconciliation = 29,
  commit = 30,
};

enum class cleanup_subcode : std::uint32_t {
  unsupported_version = 130001,
  wrong_owner = 130002,
  wrong_operation = 130003,
  predecessor_digest_mismatch = 130004,
  predecessor_not_verified = 130005,
  invalid_coverage = 130006,
  zero_budget_contradiction = 130007,
  import_failure = 130008,
  pair_cell_fan_contradiction = 130009,
  count_overflow = 130010,
  resource_preflight = 130011,
  stale_candidate = 130012,
  patch_closure_failure = 130013,
  edge_link_condition_failure = 130014,
  fan_partition_failure = 130015,
  occurrence_separation_conflict = 130016,
  unsupported_topology_effect = 130017,
  coordinate_proposal_exhausted = 130018,
  displacement_budget_exceeded = 130019,
  feature_removal_budget_exceeded = 130020,
  component_removal_failure = 130021,
  replacement_orientation_invalid = 130022,
  coverage_failure = 130023,
  self_intersection = 130024,
  external_intersection = 130025,
  side_plausibility_contradiction = 130026,
  obligation_transition_failure = 130027,
  no_monotonic_progress = 130028,
  candidate_set_exhausted = 130029,
  active_reservation_at_commit = 130030,
  precision_update_incomplete = 130031,
  final_obligation_remains = 130032,
  final_manifold_failure = 130033,
  canonicalization_failure = 130034,
  codec_error = 130035,
  digest_mismatch = 130036,
  verifier_rejection = 130037,
  idempotence_failure = 130038,
  cancelled = 130039,
  internal_invariant = 130040,
  strict_profile_mismatch = 130041,
};

inline bounded_boolean_error cleanup_error(cleanup_subcode subcode,
                                           bounded_boolean_error_category category,
                                           const char *summary,
                                           cleanup_checkpoint checkpoint) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 13;
  error.stage = static_cast<std::uint16_t>(stage_id::output_topology);
  error.checkpoint = static_cast<std::uint32_t>(checkpoint);
  error.summary = summary;
  return error;
}

struct cleanup_cancellation_observer final {
  std::uint16_t version = contract_versions::cleanup_verifier;
  std::uint16_t reserved16 = 0;
  void *state = nullptr;
  void (*poll)(void *, cleanup_checkpoint) noexcept = nullptr;
  std::uint32_t reserved32 = 0;
};

struct cleanup_capabilities final {
  std::uint16_t provider_version = contract_versions::cleanup_provider;
  std::uint16_t codec_version = contract_versions::cleanup_codec;
  std::uint16_t verifier_version = contract_versions::cleanup_verifier;
  context_owner_token owner{};
  const bounded_boolean_cancellation_token *cancellation = nullptr;
  const cleanup_cancellation_observer *cancellation_observer = nullptr;
  resource_manager *resources = nullptr;
  std::uint64_t maximum_vertices = (std::uint64_t{1} << 34);
  std::uint64_t maximum_edges = (std::uint64_t{1} << 36);
  std::uint64_t maximum_triangles = (std::uint64_t{1} << 37);
  std::uint64_t maximum_actions = (std::uint64_t{1} << 34);
  std::uint64_t maximum_canonical_bytes = (std::uint64_t{1} << 35);
  std::uint64_t maximum_work_units = (std::uint64_t{1} << 40);
  std::uint32_t reserved = 0;
};

struct cleanup_codec_limits final {
  std::uint64_t maximum_section_bytes = (std::uint64_t{1} << 34);
  std::uint64_t reserved = 0;
};

inline bool cleanup_cancelled(const cleanup_capabilities &capabilities,
                              cleanup_checkpoint checkpoint) noexcept {
  if (capabilities.cancellation_observer &&
      capabilities.cancellation_observer->poll)
    capabilities.cancellation_observer->poll(
        capabilities.cancellation_observer->state, checkpoint);
  return capabilities.cancellation &&
         capabilities.cancellation->cancellation_requested();
}

struct cleanup_statistics final {
  std::uint64_t vertex_count = 0;
  std::uint64_t paired_edge_count = 0;
  std::uint64_t halfedge_count = 0;
  std::uint64_t triangle_count = 0;
  std::uint64_t component_count = 0;
  std::uint64_t obligation_count = 0;
  std::uint64_t action_count = 0;
  std::uint64_t verifier_work_units = 0;
  std::uint64_t persistent_bytes = 0;
  std::uint64_t canonical_bytes = 0;
};

// ---------------------------------------------------------------------------
// Canonical identity keys.
// ---------------------------------------------------------------------------
struct cleaned_vertex_key final {
  std::uint64_t component11_occurrence = 0;
  std::uint16_t schema_version = contract_versions::cleanup_artifact_schema;

  friend bool operator<(const cleaned_vertex_key &a,
                        const cleaned_vertex_key &b) noexcept {
    return std::tie(a.component11_occurrence, a.schema_version) <
           std::tie(b.component11_occurrence, b.schema_version);
  }
  friend bool operator==(const cleaned_vertex_key &a,
                         const cleaned_vertex_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

struct cleaned_edge_key final {
  std::array<std::uint64_t, 2> endpoints{cleanup_invalid_ordinal,
                                         cleanup_invalid_ordinal};
  std::uint16_t schema_version = contract_versions::cleanup_artifact_schema;

  static cleaned_edge_key canonical(std::uint64_t a, std::uint64_t b) {
    cleaned_edge_key key;
    key.endpoints[0] = std::min(a, b);
    key.endpoints[1] = std::max(a, b);
    return key;
  }

  friend bool operator<(const cleaned_edge_key &a,
                        const cleaned_edge_key &b) noexcept {
    return std::tie(a.endpoints, a.schema_version) <
           std::tie(b.endpoints, b.schema_version);
  }
  friend bool operator==(const cleaned_edge_key &a,
                         const cleaned_edge_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

struct cleaned_triangle_key final {
  std::array<std::uint64_t, 3> corners{};
  std::uint16_t schema_version = contract_versions::cleanup_artifact_schema;

  static cleaned_triangle_key canonical(std::array<std::uint64_t, 3> corners) {
    cleaned_triangle_key key;
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

  friend bool operator<(const cleaned_triangle_key &a,
                        const cleaned_triangle_key &b) noexcept {
    return std::tie(a.corners, a.schema_version) <
           std::tie(b.corners, b.schema_version);
  }
  friend bool operator==(const cleaned_triangle_key &a,
                         const cleaned_triangle_key &b) noexcept {
    return !(a < b) && !(b < a);
  }
};

// ---------------------------------------------------------------------------
// Immutable artifact records.
// ---------------------------------------------------------------------------
struct cleaned_vertex_record final {
  std::uint64_t canonical_id = 0;
  cleaned_vertex_key key{};
  std::uint64_t component11_occurrence = cleanup_invalid_ordinal;
  std::array<std::uint64_t, 3> nominal_bits{};
  std::array<std::uint64_t, 3> lower_bits{};
  std::array<std::uint64_t, 3> upper_bits{};
  std::uint64_t radial_error_bits = 0;
  std::uint64_t link_begin = 0;
  std::uint64_t link_count = 0;
  std::uint64_t component = cleanup_invalid_ordinal;
};

struct cleaned_paired_edge_record final {
  std::uint64_t canonical_id = 0;
  cleaned_edge_key key{};
  std::array<std::uint64_t, 2> halfedges{cleanup_invalid_ordinal,
                                         cleanup_invalid_ordinal};
  std::array<std::uint64_t, 2> endpoints{cleanup_invalid_ordinal,
                                         cleanup_invalid_ordinal};
  std::array<std::uint64_t, 2> triangles{cleanup_invalid_ordinal,
                                         cleanup_invalid_ordinal};
};

struct cleaned_halfedge_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t pair = cleanup_invalid_ordinal;
  std::uint64_t origin = cleanup_invalid_ordinal;
  std::uint64_t destination = cleanup_invalid_ordinal;
  std::uint64_t edge = cleanup_invalid_ordinal;
  std::uint64_t triangle = cleanup_invalid_ordinal;
  std::uint64_t next = cleanup_invalid_ordinal;
};

struct cleaned_triangle_record final {
  std::uint64_t canonical_id = 0;
  cleaned_triangle_key key{};
  std::array<std::uint64_t, 3> corners{};
  std::array<std::uint64_t, 3> halfedges{};
  std::uint64_t component = cleanup_invalid_ordinal;
  triangle_geometric_category category =
      triangle_geometric_category::invalid;
};

struct cleaned_component_record final {
  std::uint64_t canonical_id = 0;
  std::uint64_t triangles_begin = 0;
  std::uint64_t triangles_count = 0;
  std::uint64_t vertex_count = 0;
  std::uint64_t edge_count = 0;
  std::uint64_t face_count = 0;
  std::int64_t euler_chi = 0;
};

struct cleanup_obligation_disposition_record final {
  std::uint64_t canonical_id = 0;
  cleanup_obligation_class obligation_class =
      cleanup_obligation_class::invalid;
  std::uint64_t source = cleanup_invalid_ordinal;
  cleanup_obligation_disposition disposition =
      cleanup_obligation_disposition::invalid;
};

struct cleanup_action_record final {
  std::uint64_t canonical_id = 0;
  std::uint8_t action_class = 0;
  std::uint8_t motion_class = 0;
  std::uint64_t certificate = cleanup_invalid_ordinal;
};

} // namespace ygor::mesh_boolean::bounded
