#include "YgorMeshesBooleanBounded/RelationRequestGraph.h"
#include "YgorMeshesBooleanBounded/RelationCanonicalization.h"
#include "YgorMeshesBooleanBounded/RelationEventSeeds.h"
#include "YgorMeshesBooleanBounded/SymbolicPerturbation.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace ygor::mesh_boolean::bounded;

namespace {

int failures = 0;

void check(bool condition, const char *message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

relation_feature_key feature(operand_id operand, relation_feature_kind kind,
                             std::uint64_t primary,
                             std::uint64_t secondary = 0) {
  relation_feature_key key;
  key.operand = operand;
  key.kind = kind;
  key.primary = primary;
  key.secondary = secondary;
  return key;
}

relation_request_key request(const bounded_boolean_digest &space,
                             relation_request_family family,
                             relation_feature_key first,
                             relation_feature_key second = {}) {
  relation_request_key key;
  key.semantic_namespace = space;
  key.family = family;
  key.first = first;
  key.second = second;
  return key;
}

relation_capabilities capabilities(context_owner_token owner) {
  relation_capabilities result;
  result.owner = std::move(owner);
  return result;
}

void test_empty_graph() {
  const auto owner = context_owner_token::create();
  auto result = build_relation_request_graph({}, capabilities(owner));
  check(result.has_value(), "empty request graph should build");
  if (!result.has_value())
    return;
  check(result.value()->requests.empty(), "empty graph has no requests");
  check(result.value()->dependencies.empty(), "empty graph has no dependencies");
  bounded_boolean_error error;
  check(verify_relation_request_graph(*result.value(), error),
        "empty graph should independently verify");
}

void test_deduplicate_and_close() {
  bounded_boolean_digest space;
  space.bytes[0] = 0x71;
  const auto imported = request(
      space, relation_request_family::imported_source_geometry,
      feature(operand_id::a, relation_feature_kind::source_vertex, 4));
  const auto edge_edge = request(
      space, relation_request_family::source_edge_source_edge,
      feature(operand_id::a, relation_feature_kind::source_edge, 2, 7),
      feature(operand_id::b, relation_feature_kind::source_edge, 3, 8));

  relation_request_proposal first;
  first.key = edge_edge;
  first.dependencies = {imported};
  first.candidate_witnesses = {candidate_id(3), candidate_id(1)};
  relation_request_proposal duplicate = first;
  duplicate.candidate_witnesses = {candidate_id(1), candidate_id(2)};
  relation_request_proposal source;
  source.key = imported;

  auto result = build_relation_request_graph(
      {first, source, duplicate}, capabilities(context_owner_token::create()));
  check(result.has_value(), "closed graph should build");
  if (!result.has_value())
    return;
  const auto &graph = *result.value();
  check(graph.proposal_count == 3, "proposal count is retained");
  check(graph.requests.size() == 2, "duplicate request has one producer");
  check(graph.dependencies.size() == 1, "one canonical dependency is emitted");
  check(graph.candidate_witnesses.size() == 3,
        "candidate witnesses are unioned and deduplicated");
  check(graph.requests[0].key == imported,
        "family precedence participates in canonical order");
  check(graph.requests[1].key == edge_edge,
        "composite request follows imported dependency");
  check(graph.dependencies[0].producer.ordinal() == 0 &&
            graph.dependencies[0].consumer.ordinal() == 1,
        "dependency direction is canonical");
}

void test_dependency_rejection() {
  bounded_boolean_digest space;
  space.bytes[0] = 0x72;
  const auto first = request(
      space, relation_request_family::source_edge_source_edge,
      feature(operand_id::a, relation_feature_kind::source_edge, 1, 2),
      feature(operand_id::b, relation_feature_kind::source_edge, 3, 4));
  const auto second = request(
      space, relation_request_family::source_edge_source_edge,
      feature(operand_id::a, relation_feature_kind::source_edge, 5, 6),
      feature(operand_id::b, relation_feature_kind::source_edge, 7, 8));
  relation_request_proposal a;
  a.key = first;
  a.dependencies = {second};
  relation_request_proposal b;
  b.key = second;
  auto result = build_relation_request_graph(
      {a, b}, capabilities(context_owner_token::create()));
  check(!result.has_value(), "same-family dependency must fail closed");
  check(result.error() &&
            result.error()->subcode ==
                static_cast<std::uint32_t>(
                    relation_subcode::same_family_dependency),
        "same-family dependency has stable typed failure");
}

void test_owner_exclusion_and_remap() {
  bounded_boolean_digest space;
  space.bytes[31] = 0x7f;
  const auto key = request(
      space, relation_request_family::source_edge_source_facet,
      feature(operand_id::a, relation_feature_kind::source_edge, 9, 10),
      feature(operand_id::b, relation_feature_kind::source_facet, 11));
  relation_request_proposal proposal;
  proposal.key = key;
  auto left = build_relation_request_graph(
      {proposal}, capabilities(context_owner_token::create()));
  auto right = build_relation_request_graph(
      {proposal}, capabilities(context_owner_token::create()));
  check(left.has_value() && right.has_value(),
        "owner-exclusion graphs should build");
  if (left.has_value() && right.has_value()) {
    check(encode_relation_request_graph_semantics(*left.value()) ==
              encode_relation_request_graph_semantics(*right.value()),
          "runtime owner anchor must not affect semantic bytes");
    check(left.value()->semantic_digest == right.value()->semantic_digest,
          "runtime owner anchor must not affect semantic digest");
  }
  check(remap_relation_request_key(remap_relation_request_key(key)) == key,
        "operand remapping is involutive");

  auto invalid = key;
  invalid.scope = relation_record_scope::public_source_feature;
  invalid.first.kind = relation_feature_kind::facet_internal_diagonal;
  check(!valid_relation_request_key(invalid),
        "internal diagonal cannot own a public relation");
}

void test_seed_and_disposition_canonicalization() {
  bounded_boolean_digest space;
  space.bytes[5] = 0x57;
  relation_event_seed_proposal a;
  a.key.semantic_namespace = space;
  a.key.family = feature_relation_family::source_edge_source_edge;
  a.key.first = feature(operand_id::a, relation_feature_kind::source_edge, 1, 2);
  a.key.second = feature(operand_id::b, relation_feature_kind::source_edge, 3, 4);
  a.source_relation = feature_relation_id(7);
  a.construction = relation_construction_id(8);
  a.incidence = {feature(operand_id::b, relation_feature_kind::source_edge, 3, 4),
                 feature(operand_id::a, relation_feature_kind::source_edge, 1, 2)};
  auto duplicate = a;
  duplicate.incidence = {
      feature(operand_id::a, relation_feature_kind::source_vertex, 9)};
  auto seeds = canonicalize_relation_event_seeds(
      {a, duplicate}, capabilities(context_owner_token::create()));
  check(seeds.has_value(), "compatible duplicate event seeds should merge");
  if (seeds.has_value()) {
    check(seeds.value()->records.size() == 1,
          "event-seed lineage key has one authoritative producer");
    check(seeds.value()->incidence.size() == 3,
          "event-seed incidence is canonically unioned");
  }

  std::vector<relation_candidate_disposition_proposal> proposals(2);
  proposals[0].candidate = candidate_id(1);
  proposals[1].candidate = candidate_id(0);
  auto dispositions = canonicalize_candidate_dispositions(
      proposals, 2, capabilities(context_owner_token::create()));
  check(dispositions.has_value(),
        "complete candidate dispositions should canonicalize");
  if (dispositions.has_value())
    check(dispositions.value()->front().candidate.ordinal() == 0 &&
              dispositions.value()->back().candidate.ordinal() == 1,
          "candidate dispositions are published in candidate order");

  proposals.push_back(proposals.back());
  auto duplicate_disposition = canonicalize_candidate_dispositions(
      proposals, 2, capabilities(context_owner_token::create()));
  check(!duplicate_disposition.has_value(),
        "duplicate candidate disposition must fail closed");
}

void test_symbolic_boundary() {
  bounded_boolean_digest space;
  space.bytes[3] = 0x33;
  symbolic_eligibility_record eligibility;
  eligibility.request = request(
      space, relation_request_family::symbolic_eligibility,
      feature(operand_id::a, relation_feature_kind::source_vertex, 1),
      feature(operand_id::b, relation_feature_kind::source_facet, 2));
  eligibility.exact_relation = exact_relation_status::exact_zero;
  eligibility.reason = symbolic_eligibility_reason::exact_formula_zero;
  eligibility.evidence_formula_version = 1;
  eligibility.exact_lineage_tie = true;
  eligibility.structural_category_eligible = true;
  eligibility.tolerance_compatible = true;
  eligibility.rounded_nominal_zero = true;
  eligibility.separated_realizations_possible = false;
  eligibility.owner_is_original_source_feature = true;

  const auto table = materialize_symbolic_policy();
  auto decision = resolve_symbolic_relation_decision(
      table, boolean_operation::set_union, operand_id::a,
      relation_family::vertex_face, orientation_relation::same, eligibility);
  check(decision.has_value(), "eligible exact tie should resolve symbolically");
  if (decision.has_value()) {
    check(decision.value()->nominal_geometry_unchanged,
          "symbolic decision must preserve nominal geometry");
    bounded_boolean_error error;
    check(verify_symbolic_relation_decision(table, eligibility,
                                            *decision.value(), error),
          "symbolic decision should independently reproduce matrix rule");
  }

  auto rounded_only = eligibility;
  rounded_only.exact_relation = exact_relation_status::unavailable;
  auto rejected = resolve_symbolic_relation_decision(
      table, boolean_operation::set_union, operand_id::a,
      relation_family::vertex_face, orientation_relation::same, rounded_only);
  check(!rejected.has_value(),
        "rounded zero without exact relation must not enter symbolic policy");

  auto reasonless = eligibility;
  reasonless.reason = symbolic_eligibility_reason::none;
  rejected = resolve_symbolic_relation_decision(
      table, boolean_operation::set_union, operand_id::a,
      relation_family::vertex_face, orientation_relation::same, reasonless);
  check(!rejected.has_value(),
        "exact zero without a structural eligibility reason must fail closed");

  auto possibly_separated = eligibility;
  possibly_separated.separated_realizations_possible = true;
  rejected = resolve_symbolic_relation_decision(
      table, boolean_operation::set_union, operand_id::a,
      relation_family::vertex_face, orientation_relation::same,
      possibly_separated);
  check(!rejected.has_value(),
        "a tie admitting separated realizations must not enter symbolic policy");
}

} // namespace

int main(int argc, char **argv) {
  const std::string suite = argc > 1 ? argv[1] : "all";
  if (suite == "all" || suite == "graph") {
    test_empty_graph();
    test_deduplicate_and_close();
    test_dependency_rejection();
  }
  if (suite == "all" || suite == "owner")
    test_owner_exclusion_and_remap();
  if (suite == "all" || suite == "canonical")
    test_seed_and_disposition_canonicalization();
  if (suite == "all" || suite == "symbolic")
    test_symbolic_boundary();
  if (failures != 0)
    std::cerr << failures << " Component 07 foundation checks failed\n";
  return failures == 0 ? 0 : 1;
}
