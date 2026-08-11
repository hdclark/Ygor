#pragma once

#include "RelationRequestGraph.h"

#include <algorithm>

namespace ygor::mesh_boolean::bounded {

struct relation_execution_authority final {
  std::uint16_t schema_version =
      contract_versions::relation_execution_authority_schema;
  std::uint16_t graph_policy_version = contract_versions::relation_graph_policy;
  context_owner_token owner{};
  relation_request_graph graph{};
  bool closed_before_evaluation = false;
  bool independently_verified = false;
  std::uint32_t reserved = 0;
  bounded_boolean_digest semantic_digest{};
};

inline std::vector<std::uint8_t> encode_relation_execution_authority_semantics(
    const relation_execution_authority &authority) {
  canonical_writer writer;
  writer.u16(authority.schema_version);
  writer.u16(authority.graph_policy_version);
  writer.boolean(authority.closed_before_evaluation);
  writer.boolean(authority.independently_verified);
  writer.u32(authority.reserved);
  writer.sized_bytes(encode_relation_request_graph_semantics(authority.graph));
  return writer.take();
}

inline bool execution_authorizes(const relation_execution_authority &authority,
                                 const relation_request_graph &stage) noexcept {
  if (!authority.closed_before_evaluation ||
      !authority.independently_verified)
    return false;
  for (const auto &request : stage.requests) {
    const auto found = std::lower_bound(
        authority.graph.requests.begin(), authority.graph.requests.end(),
        request.key,
        [](const canonical_relation_request &authority_request,
           const relation_request_key &candidate) {
          return authority_request.key < candidate;
        });
    if (found == authority.graph.requests.end() || found->key != request.key)
      return false;
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
