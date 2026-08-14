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

inline constexpr char execution_service_identity_v1[] =
    "bounded_fifo_worker_service_v1";

inline constexpr std::uint64_t execution_invalid_ordinal =
    std::numeric_limits<std::uint64_t>::max();

struct execution_plan_tag;
struct execution_task_tag;
struct execution_report_tag;
struct execution_trace_event_tag;

using execution_plan_id = strong_id<execution_plan_tag>;
using execution_task_id = strong_id<execution_task_tag>;
using execution_report_id = strong_id<execution_report_tag>;
using execution_trace_event_id = strong_id<execution_trace_event_tag>;

enum class execution_service_kind : std::uint8_t {
  bounded_fifo_worker_service_v1 = 1,
};

enum class execution_stage_status : std::uint8_t {
  planned = 1,
  running = 2,
  merged = 3,
  committed = 4,
  rolled_back = 5,
  cancelled = 6,
  invalid = 7,
};

enum class execution_task_status : std::uint8_t {
  pending = 1,
  running = 2,
  completed = 3,
  failed = 4,
  cancelled = 5,
  invalid = 6,
};

enum class execution_worker_environment_status : std::uint8_t {
  qualified = 1,
  failed_setup = 2,
  failed_verification = 3,
  invalid = 4,
};

enum class execution_checkpoint : std::uint32_t {
  capability_validation = 1,
  worker_selection = 2,
  worker_creation = 3,
  resource_admission = 4,
  plan_construction = 5,
  queue_admission = 6,
  task_dispatch = 7,
  completion_tracking = 8,
  canonical_merge = 9,
  failure_arbitration = 10,
  cancellation = 11,
  worker_join = 12,
  resource_reconciliation = 13,
  commit = 14,
};

enum class execution_subcode : std::uint32_t {
  unsupported_version = 170001,
  unsupported_policy = 170002,
  invalid_worker_count = 170003,
  worker_creation_failure = 170004,
  queue_capacity_invalid = 170005,
  queue_closed = 170006,
  task_descriptor_malformed = 170007,
  duplicate_task_key = 170008,
  task_range_invalid = 170009,
  task_count_limit = 170010,
  reservation_failure = 170011,
  task_owner_mismatch = 170012,
  task_exception = 170013,
  unexpected_worker_exception = 170014,
  floating_environment_failure = 170015,
  cancellation_checkpoint_mismatch = 170016,
  nested_parallelism_forbidden = 170017,
  prefix_overflow = 170018,
  count_fill_mismatch = 170019,
  missing_output_record = 170020,
  reduction_failure = 170021,
  primary_failure_mismatch = 170022,
  report_mismatch = 170023,
  replay_mismatch = 170024,
  active_work_at_commit = 170025,
  reservation_leak = 170026,
  shutdown_invariant = 170027,
  serial_equivalence_failure = 170028,
  resource_preflight = 170029,
  cancelled = 170030,
  internal_invariant = 170031,
  strict_profile_mismatch = 170032,
};

inline bounded_boolean_error execution_error(
    execution_subcode subcode, bounded_boolean_error_category category,
    const char *summary, execution_checkpoint checkpoint) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 17;
  error.stage = static_cast<std::uint16_t>(stage_id::execution_service);
  error.checkpoint = static_cast<std::uint32_t>(checkpoint);
  error.summary = summary;
  return error;
}

struct execution_cancellation_observer final {
  std::uint16_t version = contract_versions::execution_verifier;
  std::uint16_t reserved16 = 0;
  void *state = nullptr;
  void (*poll)(void *, execution_checkpoint) noexcept = nullptr;
  std::uint32_t reserved32 = 0;
};

struct execution_capabilities final {
  std::uint16_t provider_version = contract_versions::execution_provider;
  std::uint16_t codec_version = contract_versions::execution_codec;
  std::uint16_t verifier_version = contract_versions::execution_verifier;
  context_owner_token owner{};
  const bounded_boolean_cancellation_token *cancellation = nullptr;
  const execution_cancellation_observer *cancellation_observer = nullptr;
  resource_manager *resources = nullptr;
  std::uint32_t requested_workers = 0;
  std::uint32_t maximum_workers = 1;
  std::uint64_t maximum_queue_capacity = (std::uint64_t{1} << 20);
  std::uint64_t maximum_items = (std::uint64_t{1} << 40);
  std::uint32_t reserved = 0;
};

inline bool execution_cancelled(
    const execution_capabilities &capabilities,
    execution_checkpoint checkpoint) noexcept {
  if (capabilities.cancellation_observer &&
      capabilities.cancellation_observer->poll)
    capabilities.cancellation_observer->poll(
        capabilities.cancellation_observer->state, checkpoint);
  return capabilities.cancellation &&
         capabilities.cancellation->cancellation_requested();
}

struct execution_statistics final {
  std::uint64_t item_count = 0;
  std::uint64_t task_count = 0;
  std::uint64_t requested_workers = 0;
  std::uint64_t actual_workers = 0;
  std::uint64_t serial_tasks = 0;
  std::uint64_t parallel_tasks = 0;
  std::uint64_t verifier_work_units = 0;
};

struct execution_report final {
  execution_stage_status status = execution_stage_status::invalid;
  execution_statistics statistics{};
  std::vector<execution_task_status> task_statuses;
  std::uint64_t failed_task = execution_invalid_ordinal;
};

} // namespace ygor::mesh_boolean::bounded
