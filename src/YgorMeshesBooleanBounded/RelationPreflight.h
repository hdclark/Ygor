#pragma once

#include "CanonicalCandidateStream.h"
#include "RelationRequestGraph.h"

namespace ygor::mesh_boolean::bounded {

struct relation_preflight_plan final {
  std::uint64_t candidate_count = 0;
  std::uint64_t initial_request_upper_bound = 0;
  std::uint64_t relation_upper_bound = 0;
  std::uint64_t interval_evidence_upper_bound = 0;
  std::uint64_t region_record_upper_bound = 0;
  std::uint64_t construction_upper_bound = 0;
  std::uint64_t symbolic_upper_bound = 0;
  std::uint64_t event_seed_upper_bound = 0;
  std::uint64_t disposition_upper_bound = 0;
  std::uint64_t request_upper_bound = 0;
  std::uint64_t dependency_upper_bound = 0;
  std::uint64_t witness_upper_bound = 0;
  std::uint64_t fixed_work_units = 0;
  std::uint64_t fixed_persistent_bytes = 0;
  std::uint64_t fixed_temporary_bytes = 0;
};

template <class T, class I>
bool preflight_relation_foundation(
    const canonical_candidate_stream<T, I> &candidates,
    const relation_capabilities &capabilities, relation_preflight_plan &plan,
    bounded_boolean_error &error);

} // namespace ygor::mesh_boolean::bounded
