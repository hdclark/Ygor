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
#include <string>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

inline constexpr char qualification_harness_identity_v1[] =
    "explicit_registry_private_result_merge_v1";

inline constexpr std::uint64_t qualification_invalid_ordinal =
    std::numeric_limits<std::uint64_t>::max();

struct qualification_test_tag;
struct qualification_case_tag;
struct qualification_assertion_tag;
struct qualification_evidence_tag;

using qualification_test_id = strong_id<qualification_test_tag>;
using qualification_case_id = strong_id<qualification_case_tag>;
using qualification_assertion_id = strong_id<qualification_assertion_tag>;
using qualification_evidence_id = strong_id<qualification_evidence_tag>;

enum class qualification_scope : std::uint8_t {
  component = 1,
  integration = 2,
  continuous = 3,
  release = 4,
  invalid = 5,
};

enum class qualification_test_level : std::uint8_t {
  unit = 1,
  known_answer = 2,
  property = 3,
  metamorphic = 4,
  adversarial = 5,
  mutation = 6,
  replay = 7,
  performance = 8,
  invalid = 9,
};

enum class qualification_test_status : std::uint8_t {
  pass = 1,
  fail = 2,
  skip = 3,
  infrastructure_error = 4,
  invalid = 5,
};

enum class expected_outcome : std::uint8_t {
  success = 1,
  typed_failure = 2,
  policy_dependent = 3,
  not_applicable = 4,
  invalid = 5,
};

enum class release_rejection_reason : std::uint8_t {
  none = 0,
  required_test_failed = 1,
  required_skip = 2,
  missing_test = 3,
  stale_clause_mapping = 4,
  mutation_survivor = 5,
  replay_mismatch = 6,
  oracle_disagreement = 7,
  performance_regression = 8,
  unsupported_required_platform = 9,
  evidence_self_audit_failure = 10,
  qualification_digest_mismatch = 11,
  incomplete_matrix = 12,
  invalid = 13,
};

enum class qualification_checkpoint : std::uint32_t {
  registry_validation = 1,
  manifest_validation = 2,
  case_expansion = 3,
  case_execution = 4,
  result_merge = 5,
  evidence_construction = 6,
  self_audit = 7,
  release_decision = 8,
  commit = 9,
};

enum class qualification_subcode : std::uint32_t {
  duplicate_test_id = 160001,
  missing_test = 160002,
  malformed_manifest = 160003,
  manifest_digest_mismatch = 160004,
  required_skip = 160005,
  incomplete_matrix = 160006,
  seed_mismatch = 160007,
  campaign_mismatch = 160008,
  shrink_non_reduction = 160009,
  corpus_content_mismatch = 160010,
  replay_mismatch = 160011,
  missing_counter = 160012,
  performance_gate_failure = 160013,
  evidence_self_audit_failure = 160014,
  qualification_digest_mismatch = 160015,
  mutation_survivor = 160016,
  unsupported_platform = 160017,
  cancelled = 160018,
  internal_invariant = 160019,
};

inline bounded_boolean_error qualification_error(
    qualification_subcode subcode, bounded_boolean_error_category category,
    const char *summary, qualification_checkpoint checkpoint) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 16;
  error.stage = static_cast<std::uint16_t>(stage_id::qualification);
  error.checkpoint = static_cast<std::uint32_t>(checkpoint);
  error.summary = summary;
  return error;
}

struct qualification_test_descriptor final {
  std::uint16_t version = 1;
  qualification_scope scope = qualification_scope::component;
  qualification_test_level level = qualification_test_level::unit;
  std::uint16_t component = 0;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

struct qualification_test_result final {
  std::uint64_t ordinal = 0;
  qualification_test_status status = qualification_test_status::invalid;
};

struct qualification_release_decision final {
  bool passed = false;
  release_rejection_reason reason = release_rejection_reason::invalid;
  std::uint64_t total_tests = 0;
  std::uint64_t passed_tests = 0;
  std::uint64_t failed_tests = 0;
  std::uint64_t skipped_tests = 0;
};

struct qualification_evidence final {
  std::uint16_t version = 1;
  qualification_scope scope = qualification_scope::invalid;
  std::vector<qualification_test_result> results;
  qualification_release_decision decision;
  bounded_boolean_digest digest{};
  std::vector<std::uint8_t> canonical_bytes;
};

} // namespace ygor::mesh_boolean::bounded
