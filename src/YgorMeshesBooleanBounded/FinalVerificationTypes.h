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

inline constexpr char final_verification_provider_identity_v1[] =
    "serial_complete_final_verification_v1";

inline constexpr std::uint64_t final_verification_invalid_ordinal =
    std::numeric_limits<std::uint64_t>::max();

struct final_verification_check_tag;
struct final_verification_finding_tag;
struct final_verification_evidence_tag;

using final_verification_check_id = strong_id<final_verification_check_tag>;
using final_verification_finding_id = strong_id<final_verification_finding_tag>;
using final_verification_evidence_id = strong_id<final_verification_evidence_tag>;

enum class final_verification_provider_kind : std::uint8_t {
  serial_complete_final_verification_v1 = 1,
};

enum class check_disposition : std::uint8_t {
  passed = 1,
  not_applicable = 2,
  failed = 3,
  invalid = 4,
};

enum class final_verification_checkpoint : std::uint32_t {
  capability_validation = 1,
  candidate_audit = 2,
  dependency_audit = 3,
  count_preflight = 4,
  resource_reservation = 5,
  lexical_readback = 6,
  topology_reconstruction = 7,
  bijection = 8,
  triangle_geometry = 9,
  shell_orientation = 10,
  forbidden_intersection = 11,
  precision_aggregation = 12,
  report_regeneration = 13,
  canonical_encoding = 14,
  reingestion = 15,
  verified_result = 16,
  self_audit = 17,
  resource_reconciliation = 18,
  commit = 19,
};

enum class final_verification_subcode : std::uint32_t {
  unsupported_version = 150001,
  wrong_owner = 150002,
  wrong_operation = 150003,
  predecessor_digest_mismatch = 150004,
  predecessor_not_verified = 150005,
  illegal_candidate_status = 150006,
  count_overflow = 150007,
  resource_preflight = 150008,
  lexical_failure = 150009,
  edge_pair_failure = 150010,
  vertex_link_failure = 150011,
  component_failure = 150012,
  shell_orientation_failure = 150013,
  bijection_failure = 150014,
  triangle_geometry_failure = 150015,
  forbidden_intersection = 150016,
  precision_exceeds_tolerance = 150017,
  report_contradiction = 150018,
  codec_error = 150019,
  digest_mismatch = 150020,
  reingestion_mismatch = 150021,
  verifier_rejection = 150022,
  resource_reconciliation_failed = 150023,
  cancelled = 150024,
  internal_invariant = 150025,
  strict_profile_mismatch = 150026,
};

inline bounded_boolean_error final_verification_error(
    final_verification_subcode subcode,
    bounded_boolean_error_category category, const char *summary,
    final_verification_checkpoint checkpoint) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 15;
  error.stage = static_cast<std::uint16_t>(stage_id::publication);
  error.checkpoint = static_cast<std::uint32_t>(checkpoint);
  error.summary = summary;
  return error;
}

struct final_verification_cancellation_observer final {
  std::uint16_t version = contract_versions::final_verification_verifier;
  std::uint16_t reserved16 = 0;
  void *state = nullptr;
  void (*poll)(void *, final_verification_checkpoint) noexcept = nullptr;
  std::uint32_t reserved32 = 0;
};

struct final_verification_capabilities final {
  std::uint16_t provider_version = contract_versions::final_verification_provider;
  std::uint16_t codec_version = contract_versions::final_verification_codec;
  std::uint16_t verifier_version = contract_versions::final_verification_verifier;
  context_owner_token owner{};
  const bounded_boolean_cancellation_token *cancellation = nullptr;
  const final_verification_cancellation_observer *cancellation_observer =
      nullptr;
  resource_manager *resources = nullptr;
  std::uint64_t maximum_pair_checks = (std::uint64_t{1} << 40);
  std::uint64_t maximum_canonical_bytes = (std::uint64_t{1} << 35);
  std::uint64_t maximum_work_units = (std::uint64_t{1} << 40);
  std::uint32_t reserved = 0;
};

inline bool final_verification_cancelled(
    const final_verification_capabilities &capabilities,
    final_verification_checkpoint checkpoint) noexcept {
  if (capabilities.cancellation_observer &&
      capabilities.cancellation_observer->poll)
    capabilities.cancellation_observer->poll(
        capabilities.cancellation_observer->state, checkpoint);
  return capabilities.cancellation &&
         capabilities.cancellation->cancellation_requested();
}

struct final_verification_statistics final {
  std::uint64_t vertex_count = 0;
  std::uint64_t edge_count = 0;
  std::uint64_t facet_count = 0;
  std::uint64_t component_count = 0;
  std::uint64_t pair_checks = 0;
  std::uint64_t verifier_work_units = 0;
  std::uint64_t persistent_bytes = 0;
  std::uint64_t canonical_bytes = 0;
};

struct topology_report_record final {
  std::uint64_t vertex_count = 0;
  std::uint64_t edge_count = 0;
  std::uint64_t facet_count = 0;
  std::uint64_t component_count = 0;
};

struct geometry_report_record final {
  std::uint64_t output_precision_bits = 0;
  std::uint64_t maximum_authorized_tolerance_bits = 0;
  std::uint64_t maximum_realized_displacement_bits = 0;
};

struct check_evidence_record final {
  std::uint64_t canonical_id = 0;
  std::uint32_t check_class = 0;
  check_disposition disposition = check_disposition::invalid;
};

} // namespace ygor::mesh_boolean::bounded
