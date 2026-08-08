#include "StrictFloatingBuild.h"
#include "RelationRequestGraph.h"

#include <algorithm>
#include <limits>
#include <new>
#include <utility>

namespace ygor::mesh_boolean::bounded {
namespace {

bounded_boolean_error graph_error(relation_subcode subcode, const char *summary,
                                  relation_checkpoint checkpoint,
                                  bounded_boolean_error_category category =
                                      bounded_boolean_error_category::internal_invariant_error) {
  return relation_error(subcode, category, summary, checkpoint);
}

bool checked_append(std::uint64_t count, std::uint64_t &total) noexcept {
  return checked_add<std::uint64_t>(total, count, total);
}

struct counted_less final {
  std::uint64_t *counter = nullptr;
  bool operator()(const relation_request_proposal &a,
                  const relation_request_proposal &b) const noexcept {
    if (counter)
      ++*counter;
    return a.key < b.key;
  }
};

std::vector<relation_request_key> canonical_keys(
    std::vector<relation_request_key> values) {
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  return values;
}

std::vector<candidate_id> canonical_witnesses(std::vector<candidate_id> values) {
  std::sort(values.begin(), values.end());
  values.erase(std::unique(values.begin(), values.end()), values.end());
  return values;
}

std::size_t find_request(const std::vector<canonical_relation_request> &requests,
                         const relation_request_key &key) {
  const auto it = std::lower_bound(
      requests.begin(), requests.end(), key,
      [](const canonical_relation_request &request,
         const relation_request_key &candidate) { return request.key < candidate; });
  if (it == requests.end() || it->key != key)
    return requests.size();
  return static_cast<std::size_t>(it - requests.begin());
}

} // namespace

std::vector<std::uint8_t>
encode_relation_request_graph_semantics(const relation_request_graph &graph) {
  canonical_writer writer;
  writer.u32(0x37475259U); // YRG7
  writer.u16(graph.schema_version);
  writer.u16(graph.graph_policy_version);
  writer.u64(graph.requests.size());
  writer.u64(graph.dependencies.size());
  writer.u64(graph.reverse_consumers.size());
  writer.u64(graph.candidate_witnesses.size());
  writer.u64(graph.proposal_count);
  for (const auto &request : graph.requests) {
    writer.u64(request.id.ordinal());
    encode_relation_request_key(writer, request.key);
    writer.u64(request.dependency_begin);
    writer.u64(request.dependency_count);
    writer.u64(request.reverse_consumer_begin);
    writer.u64(request.reverse_consumer_count);
    writer.u64(request.witness_begin);
    writer.u64(request.witness_count);
    writer.u32(request.reserved);
  }
  for (const auto &dependency : graph.dependencies) {
    writer.u64(dependency.id.ordinal());
    writer.u64(dependency.producer.ordinal());
    writer.u64(dependency.consumer.ordinal());
    writer.u32(dependency.reserved);
  }
  for (const auto consumer : graph.reverse_consumers)
    writer.u64(consumer.ordinal());
  for (const auto witness : graph.candidate_witnesses)
    writer.u64(witness.ordinal());
  return writer.take();
}

boolean_outcome<relation_request_graph> build_relation_request_graph(
    std::vector<relation_request_proposal> proposals,
    const relation_capabilities &capabilities) {
  try {
    if (!capabilities.owner.anchor)
      return boolean_outcome<relation_request_graph>::failure(graph_error(
          relation_subcode::wrong_owner, "relation graph owner is not bound",
          relation_checkpoint::context_policy_capability_validation));
    if (relation_cancelled(capabilities, relation_checkpoint::dependency_closure))
      return boolean_outcome<relation_request_graph>::failure(graph_error(
          relation_subcode::cancelled, "relation graph construction cancelled",
          relation_checkpoint::dependency_closure,
          bounded_boolean_error_category::cancelled));
    if (proposals.size() > capabilities.maximum_requests)
      return boolean_outcome<relation_request_graph>::failure(graph_error(
          relation_subcode::work_limit, "relation request limit exceeded",
          relation_checkpoint::count_representability_preflight,
          bounded_boolean_error_category::resource_limit));

    relation_request_graph graph;
    graph.owner = capabilities.owner;
    graph.proposal_count = proposals.size();
    for (auto &proposal : proposals) {
      if (!valid_relation_request_key(proposal.key))
        return boolean_outcome<relation_request_graph>::failure(graph_error(
            relation_subcode::malformed_request_key,
            "malformed owner-free relation request key",
            relation_checkpoint::initial_request_grouping));
      proposal.dependencies = canonical_keys(std::move(proposal.dependencies));
      proposal.candidate_witnesses =
          canonical_witnesses(std::move(proposal.candidate_witnesses));
    }
    std::sort(proposals.begin(), proposals.end(), counted_less{&graph.sort_comparisons});

    std::vector<std::vector<relation_request_key>> grouped_dependencies;
    std::vector<std::vector<candidate_id>> grouped_witnesses;
    for (std::size_t begin = 0; begin < proposals.size();) {
      std::size_t end = begin + 1;
      while (end < proposals.size() && proposals[end].key == proposals[begin].key)
        ++end;
      std::vector<relation_request_key> dependencies;
      std::vector<candidate_id> witnesses;
      for (std::size_t i = begin; i < end; ++i) {
        dependencies.insert(dependencies.end(), proposals[i].dependencies.begin(),
                            proposals[i].dependencies.end());
        witnesses.insert(witnesses.end(), proposals[i].candidate_witnesses.begin(),
                         proposals[i].candidate_witnesses.end());
      }
      dependencies = canonical_keys(std::move(dependencies));
      witnesses = canonical_witnesses(std::move(witnesses));
      canonical_relation_request request;
      request.id = relation_request_id(graph.requests.size());
      request.key = proposals[begin].key;
      graph.requests.push_back(std::move(request));
      grouped_dependencies.push_back(std::move(dependencies));
      grouped_witnesses.push_back(std::move(witnesses));
      begin = end;
    }

    std::vector<std::vector<relation_request_id>> reverse(graph.requests.size());
    std::uint64_t dependency_total = 0;
    std::uint64_t witness_total = 0;
    for (std::size_t consumer = 0; consumer < graph.requests.size(); ++consumer) {
      auto &request = graph.requests[consumer];
      request.dependency_begin = graph.dependencies.size();
      request.witness_begin = graph.candidate_witnesses.size();
      for (const auto &dependency_key : grouped_dependencies[consumer]) {
        const auto producer = find_request(graph.requests, dependency_key);
        if (producer == graph.requests.size())
          return boolean_outcome<relation_request_graph>::failure(graph_error(
              relation_subcode::missing_dependency,
              "relation dependency is absent from closed request graph",
              relation_checkpoint::dependency_closure));
        const auto producer_family = static_cast<std::uint8_t>(
            graph.requests[producer].key.family);
        const auto consumer_family =
            static_cast<std::uint8_t>(request.key.family);
        if (producer_family >= consumer_family)
          return boolean_outcome<relation_request_graph>::failure(graph_error(
              producer_family == consumer_family
                  ? relation_subcode::same_family_dependency
                  : relation_subcode::forward_dependency,
              producer_family == consumer_family
                  ? "same-family relation dependency is prohibited in V1"
                  : "forward relation dependency is prohibited",
              relation_checkpoint::graph_finalization));
        canonical_relation_dependency dependency;
        dependency.id = relation_dependency_id(graph.dependencies.size());
        dependency.producer = relation_request_id(producer);
        dependency.consumer = relation_request_id(consumer);
        graph.dependencies.push_back(dependency);
        reverse[producer].push_back(relation_request_id(consumer));
      }
      request.dependency_count =
          graph.dependencies.size() - request.dependency_begin;
      graph.candidate_witnesses.insert(graph.candidate_witnesses.end(),
                                       grouped_witnesses[consumer].begin(),
                                       grouped_witnesses[consumer].end());
      request.witness_count =
          graph.candidate_witnesses.size() - request.witness_begin;
      if (!checked_append(request.dependency_count, dependency_total) ||
          !checked_append(request.witness_count, witness_total))
        return boolean_outcome<relation_request_graph>::failure(graph_error(
            relation_subcode::count_overflow,
            "relation graph count overflow",
            relation_checkpoint::count_representability_preflight,
            bounded_boolean_error_category::index_overflow));
    }
    if (dependency_total > capabilities.maximum_dependencies ||
        witness_total > capabilities.maximum_consumers)
      return boolean_outcome<relation_request_graph>::failure(graph_error(
          relation_subcode::work_limit,
          "relation graph dependency or witness limit exceeded",
          relation_checkpoint::count_representability_preflight,
          bounded_boolean_error_category::resource_limit));

    for (std::size_t producer = 0; producer < graph.requests.size(); ++producer) {
      auto &request = graph.requests[producer];
      request.reverse_consumer_begin = graph.reverse_consumers.size();
      graph.reverse_consumers.insert(graph.reverse_consumers.end(),
                                     reverse[producer].begin(), reverse[producer].end());
      request.reverse_consumer_count =
          graph.reverse_consumers.size() - request.reverse_consumer_begin;
    }
    if (graph.reverse_consumers.size() > capabilities.maximum_consumers)
      return boolean_outcome<relation_request_graph>::failure(graph_error(
          relation_subcode::work_limit,
          "relation reverse-consumer limit exceeded",
          relation_checkpoint::count_representability_preflight,
          bounded_boolean_error_category::resource_limit));

    graph.semantic_digest =
        sha256::digest(encode_relation_request_graph_semantics(graph));
    bounded_boolean_error verification_error;
    if (!verify_relation_request_graph(graph, verification_error))
      return boolean_outcome<relation_request_graph>::failure(verification_error);
    return boolean_outcome<relation_request_graph>::success(std::move(graph));
  } catch (const std::bad_alloc &) {
    return boolean_outcome<relation_request_graph>::failure(graph_error(
        relation_subcode::resource_preflight,
        "relation graph allocation failed",
        relation_checkpoint::discovery_resource_reservation,
        bounded_boolean_error_category::resource_limit));
  } catch (...) {
    return boolean_outcome<relation_request_graph>::failure(graph_error(
        relation_subcode::internal_invariant,
        "relation graph construction raised an unexpected exception",
        relation_checkpoint::graph_finalization));
  }
}


#include "RelationRequestGraphVerifier.inc"

} // namespace ygor::mesh_boolean::bounded
