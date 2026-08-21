#pragma once

#include "QualificationTypes.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ygor::mesh_boolean::bounded {

// Explicit qualification registry: tests are registered by stable ASCII ID in
// deterministic order (never via static constructor order). The harness merges
// results by case key and produces a fail-closed release decision.
class qualification_registry final {
public:
  struct entry {
    qualification_test_descriptor descriptor;
    bool (*run)() = nullptr;
  };

  bool register_test(const char *id, qualification_test_descriptor descriptor,
                     bool (*run)()) {
    if (id == nullptr || run == nullptr)
      return false;
    if (tests_.count(std::string(id)) != 0)
      return false;
    tests_.emplace(std::string(id), entry{descriptor, run});
    return true;
  }

  const std::map<std::string, entry> &tests() const noexcept { return tests_; }

private:
  std::map<std::string, entry> tests_;
};

struct qualification_harness_result {
  bool complete = false;
  qualification_evidence evidence;
  bounded_boolean_error error;
};

// Run every registered test in canonical ID order, encode the ordered results,
// compute the evidence digest, and derive a fail-closed release decision.
inline qualification_harness_result run_qualification(
    const qualification_registry &registry, qualification_scope scope) {
  qualification_harness_result harness;
  auto &evidence = harness.evidence;
  evidence.scope = scope;

  for (const auto &test : registry.tests()) {
    qualification_test_result result;
    result.ordinal = evidence.results.size();
    try {
      if (test.second.run())
        result.status = qualification_test_status::pass;
      else
        result.status = qualification_test_status::fail;
    } catch (...) {
      result.status = qualification_test_status::infrastructure_error;
    }
    evidence.results.push_back(result);
  }

  // Encode ordered results canonically.
  canonical_writer writer;
  writer.u32(0x51554631); // "QUF1"
  writer.u16(evidence.version);
  writer.u8(static_cast<std::uint8_t>(scope));
  writer.u64(evidence.results.size());
  for (const auto &result : evidence.results) {
    writer.u64(result.ordinal);
    writer.u8(static_cast<std::uint8_t>(result.status));
  }
  evidence.canonical_bytes = writer.take();
  evidence.digest = sha256::digest(evidence.canonical_bytes);

  // Release decision (fail closed).
  auto &decision = evidence.decision;
  decision.reason = release_rejection_reason::none;
  decision.total_tests = evidence.results.size();
  for (const auto &result : evidence.results) {
    switch (result.status) {
    case qualification_test_status::pass:
      ++decision.passed_tests;
      break;
    case qualification_test_status::fail:
      ++decision.failed_tests;
      decision.reason = release_rejection_reason::required_test_failed;
      break;
    case qualification_test_status::skip:
      ++decision.skipped_tests;
      decision.reason = release_rejection_reason::required_skip;
      break;
    default:
      ++decision.failed_tests;
      decision.reason = release_rejection_reason::required_test_failed;
      break;
    }
  }
  decision.passed = decision.reason == release_rejection_reason::none &&
                    decision.total_tests > 0;

  harness.complete = true;
  return harness;
}

} // namespace ygor::mesh_boolean::bounded
