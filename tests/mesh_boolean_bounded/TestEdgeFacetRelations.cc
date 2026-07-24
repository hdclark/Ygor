#include "BroadPhaseFixtures.h"
#include "YgorMeshesBooleanBounded/EdgeFacetRelations.h"
#include "YgorMeshesBooleanBounded/RelationBuild.h"

#include <algorithm>
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

struct stages final {
  bounded::candidate_source_edge_relation_stage<double> edges;
  bounded::candidate_source_edge_facet_relation_stage<double> facets;
};

stages build_stages(built_fixture &fixture) {
  auto caps = capabilities(fixture);
  auto edge_stage = bounded::build_candidate_source_edge_relations(
      *fixture.artifact, fixture.predecessor.context.context_digest,
      fixture.predecessor.precision->tolerance(), caps);
  if (!edge_stage.has_value())
    throw std::runtime_error(diagnostic(*edge_stage.error()));
  auto facet_stage = bounded::build_candidate_source_edge_facet_relations(
      *fixture.artifact, *edge_stage.value(),
      fixture.predecessor.context.context_digest,
      fixture.predecessor.precision->tolerance(), caps);
  if (!facet_stage.has_value())
    throw std::runtime_error(diagnostic(*facet_stage.error()));
  return {std::move(*edge_stage.value()), std::move(*facet_stage.value())};
}

void test_candidate_derived_preflight_bound() {
  auto fixture = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(0.5, 0.25, 0.125, 1.5, 1.25, 1.125));
  std::uint64_t maximum_boundary = 0;
  for (const auto &facet : fixture.predecessor.manifolds->a()->facet_groups())
    maximum_boundary = std::max(
        maximum_boundary,
        static_cast<std::uint64_t>(facet.boundary_halfedges.size()));
  for (const auto &facet : fixture.predecessor.manifolds->b()->facet_groups())
    maximum_boundary = std::max(
        maximum_boundary,
        static_cast<std::uint64_t>(facet.boundary_halfedges.size()));
  bounded::relation_preflight_plan plan;
  bounded_boolean_error error;
  auto caps = capabilities(fixture);
  require(bounded::preflight_relation_foundation(*fixture.artifact, caps, plan,
                                                 error),
          "candidate-derived relation preflight succeeds");
  require(plan.request_upper_bound ==
              fixture.artifact->candidates().size() * (maximum_boundary + 1),
          "preflight covers complete facet-boundary plus edge/facet proposals");
  require(plan.dependency_upper_bound ==
              fixture.artifact->candidates().size() * maximum_boundary,
          "preflight covers every edge/facet boundary dependency");
}

void test_candidate_request_integration() {
  auto fixture = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(0.5, 0.25, 0.125, 1.5, 1.25, 1.125));
  require(!fixture.artifact->candidates().empty(),
          "edge/facet integration fixture requires candidates");
  auto value = build_stages(fixture);

  require(!value.facets.request_graph.requests.empty(),
          "candidate discovery should induce edge/facet requests");
  require(value.facets.evaluation_count ==
              value.facets.request_graph.requests.size(),
          "each unique edge/facet request is evaluated exactly once");
  require(value.facets.relations.size() ==
              value.facets.request_graph.requests.size(),
          "every edge/facet request has one numerical producer");
  require(value.facets.candidate_ranges.size() ==
              fixture.artifact->candidates().size(),
          "every candidate has a deterministic edge/facet coverage range");

  bool observed_contact = false;
  for (const auto &request : value.facets.request_graph.requests) {
    require(request.key.family ==
                bounded::relation_request_family::source_edge_source_facet,
            "edge/facet graph contains only edge/facet requests");
    require(request.key.scope ==
                bounded::relation_record_scope::public_source_feature,
            "edge/facet graph publishes only original source features");
  }
  for (const auto &relation : value.facets.relations) {
    require(bounded::valid_source_edge_facet_relation_record(relation),
            "integrated edge/facet relation validates");
    require(relation.boundary_relation_requests.size() >= 3,
            "edge/facet relation binds the complete facet boundary");
    observed_contact = observed_contact ||
                       relation.contact !=
                           bounded::source_edge_facet_contact_class::none;
  }
  require(observed_contact,
          "overlapping boxes should produce at least one edge/facet contact");

  auto caps = capabilities(fixture);
  bounded_boolean_error verification_error;
  require(bounded::verify_candidate_source_edge_facet_relation_stage(
              *fixture.artifact, value.edges,
              fixture.predecessor.context.context_digest,
              fixture.predecessor.precision->tolerance(), caps, value.facets,
              verification_error),
          "edge/facet stage independently reconstructs");

  auto owner_changed = value.facets;
  owner_changed.owner = bounded::context_owner_token::create();
  owner_changed.request_graph.owner = owner_changed.owner;
  require(
      bounded::encode_candidate_source_edge_facet_relation_semantics(
          owner_changed) ==
          bounded::encode_candidate_source_edge_facet_relation_semantics(
              value.facets),
      "runtime owner is excluded from edge/facet stage semantics");
}

void test_mutation_rejection() {
  auto fixture = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(0.5, 0.25, 0.125, 1.5, 1.25, 1.125));
  auto value = build_stages(fixture);
  require(!value.facets.relations.empty(),
          "mutation fixture requires an edge/facet relation");

  auto mutated = value.facets;
  mutated.relations.front().semantic_digest.bytes[0] ^= 0x80U;
  auto caps = capabilities(fixture);
  bounded_boolean_error error;
  require(!bounded::verify_candidate_source_edge_facet_relation_stage(
              *fixture.artifact, value.edges,
              fixture.predecessor.context.context_digest,
              fixture.predecessor.precision->tolerance(), caps, mutated,
              error),
          "mutated edge/facet relation must be rejected");
}

void test_empty_stage_and_publication_gate() {
  auto separated = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(4.0, 4.0, 4.0, 5.0, 5.0, 5.0));
  require(separated.artifact->candidates().empty(),
          "separated fixture should have no broad-phase candidates");
  auto empty = build_stages(separated);
  require(empty.facets.request_graph.requests.empty() &&
              empty.facets.relations.empty() &&
              empty.facets.candidate_ranges.empty(),
          "empty candidate stream produces an empty verified edge/facet stage");

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
                  bounded::relation_checkpoint::facet_facet_evaluation),
          "publication gate advances only after verified edge/facet relations");
}

} // namespace

int main() {
  try {
    test_candidate_derived_preflight_bound();
    test_candidate_request_integration();
    test_mutation_rejection();
    test_empty_stage_and_publication_gate();
    std::cout << "Component 07 edge/facet integration checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
