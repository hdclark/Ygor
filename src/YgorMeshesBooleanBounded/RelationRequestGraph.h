#pragma once

#include "RelationKeys.h"
#include "Outcome.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct relation_request_proposal final {
  relation_request_key key{};
  std::vector<relation_request_key> dependencies;
  std::vector<candidate_id> candidate_witnesses;
};

struct canonical_relation_request final {
  relation_request_id id{0};
  relation_request_key key{};
  std::uint64_t dependency_begin = 0;
  std::uint64_t dependency_count = 0;
  std::uint64_t reverse_consumer_begin = 0;
  std::uint64_t reverse_consumer_count = 0;
  std::uint64_t witness_begin = 0;
  std::uint64_t witness_count = 0;
  std::uint32_t reserved = 0;
};

struct canonical_relation_dependency final {
  relation_dependency_id id{0};
  relation_request_id producer{0};
  relation_request_id consumer{0};
  std::uint32_t reserved = 0;
};

struct relation_request_graph final {
  std::uint16_t schema_version = contract_versions::relation_request_graph_schema;
  std::uint16_t graph_policy_version = contract_versions::relation_graph_policy;
  context_owner_token owner{};
  std::vector<canonical_relation_request> requests;
  std::vector<canonical_relation_dependency> dependencies;
  std::vector<relation_request_id> reverse_consumers;
  std::vector<candidate_id> candidate_witnesses;
  std::uint64_t proposal_count = 0;
  std::uint64_t sort_comparisons = 0;
  bounded_boolean_digest semantic_digest{};
};

boolean_outcome<relation_request_graph> build_relation_request_graph(
    std::vector<relation_request_proposal> proposals,
    const relation_capabilities &capabilities);

bool verify_relation_request_graph(const relation_request_graph &graph,
                                   bounded_boolean_error &error);
std::vector<std::uint8_t>
encode_relation_request_graph_semantics(const relation_request_graph &graph);

} // namespace ygor::mesh_boolean::bounded
