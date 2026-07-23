#include "BroadPhaseFixtures.h"
#include "YgorMeshesBooleanBounded/CandidateSourceEdgeRelations.h"
#include "YgorMeshesBooleanBounded/RelationBuild.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace bounded = ygor::mesh_boolean::bounded;
using broad_phase_tests::built_fixture;
using broad_phase_tests::diagnostic;
using broad_phase_tests::require;

namespace {

bounded::relation_capabilities capabilities(built_fixture &fixture) {
  bounded::relation_capabilities out;
  out.owner = fixture.predecessor.context.owner;
  out.resources = fixture.predecessor.resources.get();
  return out;
}

void test_candidate_request_integration() {
  auto fixture = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(0.5, 0.25, 0.125, 1.5, 1.25, 1.125));
  require(!fixture.artifact->candidates().empty(),
          "source-edge integration fixture requires candidates");
  auto caps = capabilities(fixture);
  auto stage = bounded::build_candidate_source_edge_relations(
      *fixture.artifact, fixture.predecessor.context.context_digest,
      fixture.predecessor.precision->tolerance(), caps);
  if (!stage.has_value())
    throw std::runtime_error(diagnostic(*stage.error()));

  const auto &value = *stage.value();
  require(!value.request_graph.requests.empty(),
          "candidate discovery should induce source-edge requests");
  require(value.evaluation_count == value.request_graph.requests.size(),
          "each unique source-edge request is evaluated exactly once");
  require(value.relations.size() == value.request_graph.requests.size(),
          "every source-edge request has one numerical producer");
  require(value.request_graph.proposal_count >= value.request_graph.requests.size(),
          "duplicate candidate discoveries may share a producer");
  require(value.candidate_ranges.size() == fixture.artifact->candidates().size(),
          "every candidate has a deterministic source-edge coverage range");

  bool shared_producer = false;
  for (const auto &request : value.request_graph.requests) {
    require(request.key.family ==
                bounded::relation_request_family::source_edge_source_edge,
            "integration graph contains only source-edge/source-edge requests");
    require(request.key.scope ==
                bounded::relation_record_scope::public_source_feature,
            "source-edge integration publishes only original source features");
    shared_producer = shared_producer || request.witness_count > 1;
  }
  require(shared_producer ||
              value.request_graph.proposal_count ==
                  value.request_graph.requests.size(),
          "producer sharing evidence is internally consistent");
  for (const auto &relation : value.relations)
    require(bounded::valid_source_edge_relation_record(relation),
            "integrated source-edge relation validates");

  bounded_boolean_error verification_error;
  require(bounded::verify_candidate_source_edge_relation_stage(
              *fixture.artifact, fixture.predecessor.context.context_digest,
              fixture.predecessor.precision->tolerance(), caps, value,
              verification_error),
          "candidate source-edge stage independently reconstructs");

  auto owner_changed = value;
  owner_changed.owner = bounded::context_owner_token::create();
  owner_changed.request_graph.owner = owner_changed.owner;
  require(bounded::encode_candidate_source_edge_relation_semantics(owner_changed) ==
              bounded::encode_candidate_source_edge_relation_semantics(value),
          "runtime owner is excluded from integration semantics");
}

void test_mutation_rejection() {
  auto fixture = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(0.5, 0.25, 0.125, 1.5, 1.25, 1.125));
  auto caps = capabilities(fixture);
  auto stage = bounded::build_candidate_source_edge_relations(
      *fixture.artifact, fixture.predecessor.context.context_digest,
      fixture.predecessor.precision->tolerance(), caps);
  if (!stage.has_value())
    throw std::runtime_error(diagnostic(*stage.error()));
  require(!stage.value()->relations.empty(),
          "mutation fixture requires a source-edge relation");

  auto mutated = *stage.value();
  mutated.relations.front().semantic_digest.bytes[0] ^= 0x80U;
  bounded_boolean_error error;
  require(!bounded::verify_candidate_source_edge_relation_stage(
              *fixture.artifact, fixture.predecessor.context.context_digest,
              fixture.predecessor.precision->tolerance(), caps, mutated, error),
          "mutated integrated relation must be rejected");
}

void test_empty_stage_and_nonempty_publication_gate() {
  auto separated = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(4.0, 4.0, 4.0, 5.0, 5.0, 5.0));
  require(separated.artifact->candidates().empty(),
          "separated fixture should have no broad-phase candidates");
  auto empty_caps = capabilities(separated);
  auto empty = bounded::build_candidate_source_edge_relations(
      *separated.artifact, separated.predecessor.context.context_digest,
      separated.predecessor.precision->tolerance(), empty_caps);
  if (!empty.has_value())
    throw std::runtime_error(diagnostic(*empty.error()));
  require(empty.value()->request_graph.requests.empty() &&
              empty.value()->relations.empty() &&
              empty.value()->candidate_ranges.empty(),
          "empty candidate stream produces an empty verified edge stage");

  auto fixture = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(0.5, 0.25, 0.125, 1.5, 1.25, 1.125));
  auto caps = capabilities(fixture);
  auto result = bounded::build_signed_feature_relations(
      fixture.predecessor.context, *fixture.predecessor.precision,
      fixture.artifact, caps);
  require(!result.has_value(),
          "non-empty Component 07 still refuses partial publication");
  require(result.error() &&
              result.error()->subcode == static_cast<std::uint32_t>(
                  bounded::relation_subcode::unsupported_relation_kernel) &&
              result.error()->checkpoint == static_cast<std::uint32_t>(
                  bounded::relation_checkpoint::edge_facet_evaluation),
          "non-empty publication now fails only after edge-edge integration");
}

} // namespace

int main() {
  try {
    test_candidate_request_integration();
    test_mutation_rejection();
    test_empty_stage_and_nonempty_publication_gate();
    std::cout << "Component 07 candidate source-edge integration checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
