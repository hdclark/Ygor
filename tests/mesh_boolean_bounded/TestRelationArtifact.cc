#include "BroadPhaseFixtures.h"
#include "YgorMeshesBooleanBounded/RelationBuild.h"
#include "YgorMeshesBooleanBounded/RelationVerifier.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace bounded = ygor::mesh_boolean::bounded;
using broad_phase_tests::built_fixture;
using broad_phase_tests::diagnostic;
using broad_phase_tests::require;

namespace ygor::mesh_boolean::bounded {

struct relation_artifact_test_access final {
  template <class T, class I>
  static signed_feature_relations<T, I>
  copy(const signed_feature_relations<T, I> &artifact) {
    return artifact;
  }

  template <class T, class I>
  static auto &crossings(signed_feature_relations<T, I> &artifact) {
    return artifact.crossings_;
  }

  template <class T, class I>
  static auto &symbolic_eligibility(
      signed_feature_relations<T, I> &artifact) {
    return artifact.symbolic_eligibility_;
  }

  template <class T, class I>
  static auto &coplanar_event_nodes(
      signed_feature_relations<T, I> &artifact) {
    return artifact.coplanar_event_nodes_;
  }

  template <class T, class I>
  static auto &coplanar_oriented_arcs(
      signed_feature_relations<T, I> &artifact) {
    return artifact.coplanar_oriented_arcs_;
  }

  template <class T, class I>
  static auto &coplanar_overlap_components(
      signed_feature_relations<T, I> &artifact) {
    return artifact.coplanar_overlap_components_;
  }

  template <class T, class I>
  static auto &dispositions(signed_feature_relations<T, I> &artifact) {
    return artifact.candidate_dispositions_;
  }

  template <class T, class I>
  static auto &canonical_bytes(signed_feature_relations<T, I> &artifact) {
    return artifact.canonical_bytes_;
  }

  template <class T, class I>
  static void set_owner(signed_feature_relations<T, I> &artifact,
                        context_owner_token owner) {
    artifact.owner_ = std::move(owner);
  }

  template <class T, class I>
  static void repair_codec(signed_feature_relations<T, I> &artifact) {
    artifact.canonical_bytes_ = encode_signed_feature_relations(artifact);
    artifact.digest_ = sha256::digest(artifact.canonical_bytes_);
  }
};

} // namespace ygor::mesh_boolean::bounded

namespace {

bounded::relation_capabilities capabilities(
    built_fixture &fixture, bounded::resource_manager *resources = nullptr) {
  bounded::relation_capabilities out;
  out.owner = fixture.predecessor.context.owner;
  out.resources = resources ? resources : fixture.predecessor.resources.get();
  return out;
}

std::shared_ptr<const bounded::signed_feature_relations<double, std::uint32_t>>
build_artifact(built_fixture &fixture,
               bounded::resource_manager *resources = nullptr) {
  auto result = bounded::build_signed_feature_relations(
      fixture.predecessor.context, *fixture.predecessor.precision,
      fixture.artifact, capabilities(fixture, resources));
  if (!result.has_value())
    throw std::runtime_error(diagnostic(*result.error()));
  return *result.value();
}

built_fixture overlapping_fixture() {
  return broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(0.5, 0.25, 0.125, 1.5, 1.25, 1.125));
}

built_fixture symbolic_fixture() {
  return broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(0.25, 0.25, 1.0, 0.75, 0.75, 2.0));
}

void require_no_live_resources(const bounded::resource_manager &resources,
                               const char *message) {
  const auto snapshot = resources.snapshot();
  for (const auto &counter : snapshot)
    if (counter.reserved != 0 || counter.committed != 0)
      throw std::runtime_error(message);
}

void test_empty_artifact_and_decode() {
  auto fixture = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(4.0, 4.0, 4.0, 5.0, 5.0, 5.0));
  require(fixture.artifact->candidates().empty(),
          "empty relation fixture must have no candidates");
  bounded::resource_manager resources(
      resource_policy::conservative_defaults());
  const auto artifact = build_artifact(fixture, &resources);
  require(artifact->request_graph().requests.empty() &&
              artifact->relations().empty() && artifact->constructions().empty() &&
              artifact->candidate_dispositions().empty(),
          "empty candidates publish a canonical empty relation artifact");
  require(artifact->source_edge_stage() && artifact->source_edge_facet_stage() &&
              artifact->source_facet_stage() && artifact->coplanar_overlay_stage(),
          "empty artifact retains all verified detailed stages");
  bounded_boolean_error verify_error;
  require(bounded::verify_signed_feature_relations(*artifact, verify_error),
          "empty relation artifact independently verifies");

  auto decode_fixture = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(4.0, 4.0, 4.0, 5.0, 5.0, 5.0));
  bounded::resource_manager decode_resources(
      resource_policy::conservative_defaults());
  auto decoded = bounded::decode_signed_feature_relations(
      artifact->canonical_bytes(), decode_fixture.predecessor.context,
      *decode_fixture.predecessor.precision, decode_fixture.artifact,
      capabilities(decode_fixture, &decode_resources));
  if (!decoded.has_value())
    throw std::runtime_error(diagnostic(*decoded.error()));
  require((*decoded.value())->canonical_bytes() == artifact->canonical_bytes(),
          "empty relation decode reproduces canonical bytes");
}

void test_nonempty_determinism_and_decode() {
  auto first_fixture = overlapping_fixture();
  auto second_fixture = overlapping_fixture();
  bounded::resource_manager first_resources(
      resource_policy::conservative_defaults());
  bounded::resource_manager second_resources(
      resource_policy::conservative_defaults());
  const auto first = build_artifact(first_fixture, &first_resources);
  const auto second = build_artifact(second_fixture, &second_resources);

  require(!first->relations().empty() &&
              first->candidate_dispositions().size() ==
                  first_fixture.artifact->candidates().size(),
          "nonempty artifact publishes relations and complete dispositions");
  require(first->canonical_bytes() == second->canonical_bytes() &&
              first->digest() == second->digest(),
          "different runtime owner anchors produce identical relation semantics");
  require(first->statistics().persistent_bytes != 0 &&
              first->statistics().verifier_work_units != 0 &&
              first->verification_evidence().graph_reconstructed &&
              first->verification_evidence().owner_exclusion_checked &&
              first->verification_evidence().selection_boundary_checked &&
              first->verification_evidence().candidate_dispositions_complete,
          "published relation artifact carries complete verification evidence");

  auto owner_changed = bounded::relation_artifact_test_access::copy(*first);
  const auto original_bytes = bounded::encode_signed_feature_relations(owner_changed);
  bounded::relation_artifact_test_access::set_owner(
      owner_changed, bounded::context_owner_token::create());
  require(bounded::encode_signed_feature_relations(owner_changed) ==
              original_bytes,
          "runtime owner anchor is absent from relation semantic bytes");

  auto decode_fixture = overlapping_fixture();
  bounded::resource_manager decode_resources(
      resource_policy::conservative_defaults());
  auto decoded = bounded::decode_signed_feature_relations(
      first->canonical_bytes(), decode_fixture.predecessor.context,
      *decode_fixture.predecessor.precision, decode_fixture.artifact,
      capabilities(decode_fixture, &decode_resources));
  if (!decoded.has_value())
    throw std::runtime_error(diagnostic(*decoded.error()));
  require((*decoded.value())->digest() == first->digest(),
          "nonempty relation decode rebuilds and independently verifies artifact");
}

void test_matched_mutation_rejection() {
  auto fixture = overlapping_fixture();
  bounded::resource_manager resources(
      resource_policy::conservative_defaults());
  const auto artifact = build_artifact(fixture, &resources);
  bounded_boolean_error error;

  auto disposition_mutation =
      bounded::relation_artifact_test_access::copy(*artifact);
  require(!disposition_mutation.candidate_dispositions().empty(),
          "mutation fixture requires candidate dispositions");
  auto &dispositions =
      bounded::relation_artifact_test_access::dispositions(disposition_mutation);
  dispositions.front().candidate = bounded::candidate_id(
      disposition_mutation.candidate_dispositions().size());
  bounded::relation_artifact_test_access::repair_codec(disposition_mutation);
  require(!bounded::verify_signed_feature_relations(disposition_mutation, error),
          "matched candidate-disposition mutation is rejected");

  if (!artifact->crossings().empty()) {
    auto crossing_mutation =
        bounded::relation_artifact_test_access::copy(*artifact);
    auto &crossing =
        bounded::relation_artifact_test_access::crossings(crossing_mutation)
            .front();
    crossing.numeric_crossing = crossing.numeric_crossing == 1 ? -1 : 1;
    bounded::relation_artifact_test_access::repair_codec(crossing_mutation);
    error = bounded_boolean_error{};
    require(!bounded::verify_signed_feature_relations(crossing_mutation, error),
            "matched crossing multiplicity mutation is rejected");

    auto fan_mutation =
        bounded::relation_artifact_test_access::copy(*artifact);
    auto &fan =
        bounded::relation_artifact_test_access::crossings(fan_mutation).front();
    ++fan.source_fan_group_size;
    bounded::relation_artifact_test_access::repair_codec(fan_mutation);
    error = bounded_boolean_error{};
    require(!bounded::verify_signed_feature_relations(fan_mutation, error),
            "matched source-fan cardinality mutation is rejected");
  }

  auto symbolic_source = symbolic_fixture();
  bounded::resource_manager symbolic_resources(
      resource_policy::conservative_defaults());
  const auto symbolic_artifact =
      build_artifact(symbolic_source, &symbolic_resources);
  require(!symbolic_artifact->symbolic_eligibility().empty() &&
              symbolic_artifact->symbolic_eligibility().size() ==
                  symbolic_artifact->symbolic_decisions().size(),
          "qualified exact ties publish complete symbolic evidence and decisions");
  bool saw_facet_lineage = false;
  bool saw_coincident_contract = false;
  for (const auto &eligibility :
       symbolic_artifact->symbolic_eligibility()) {
    require(eligibility.exact_relation ==
                    bounded::exact_relation_status::exact_zero &&
                eligibility.reason !=
                    bounded::symbolic_eligibility_reason::none &&
                eligibility.evidence_formula_version != 0 &&
                eligibility.exact_lineage_tie &&
                eligibility.structural_category_eligible &&
                eligibility.tolerance_compatible &&
                !eligibility.separated_realizations_possible &&
                eligibility.owner_is_original_source_feature,
            "symbolic eligibility retains qualified exact and structural evidence");
    saw_facet_lineage =
        saw_facet_lineage ||
        eligibility.reason ==
            bounded::symbolic_eligibility_reason::coplanar_source_facet_lineage;
    saw_coincident_contract =
        saw_coincident_contract ||
        eligibility.reason ==
            bounded::symbolic_eligibility_reason::coincident_source_contract;
  }
  require(saw_facet_lineage && saw_coincident_contract,
          "symbolic fixture covers support and overlay eligibility categories");

  std::size_t expected_nodes = 0;
  std::size_t expected_arcs = 0;
  std::size_t expected_components = 0;
  for (const auto &overlay :
       symbolic_artifact->coplanar_overlay_stage()->overlays) {
    expected_nodes += overlay.event_nodes.size();
    expected_arcs += overlay.oriented_arcs.size();
    expected_components += overlay.overlap_components.size();
  }
  require(expected_nodes != 0 && expected_arcs != 0 &&
              expected_components != 0 &&
              symbolic_artifact->coplanar_event_nodes().size() ==
                  expected_nodes &&
              symbolic_artifact->coplanar_oriented_arcs().size() ==
                  expected_arcs &&
              symbolic_artifact->coplanar_overlap_components().size() ==
                  expected_components &&
              symbolic_artifact->statistics().coplanar_event_node_count ==
                  expected_nodes &&
              symbolic_artifact->statistics().coplanar_oriented_arc_count ==
                  expected_arcs &&
              symbolic_artifact->statistics()
                      .coplanar_overlap_component_count == expected_components,
          "coplanar topology is published as complete first-class artifact tables");

  auto topology_decode_fixture = symbolic_fixture();
  bounded::resource_manager topology_decode_resources(
      resource_policy::conservative_defaults());
  auto topology_decoded = bounded::decode_signed_feature_relations(
      symbolic_artifact->canonical_bytes(),
      topology_decode_fixture.predecessor.context,
      *topology_decode_fixture.predecessor.precision,
      topology_decode_fixture.artifact,
      capabilities(topology_decode_fixture, &topology_decode_resources));
  require(topology_decoded.has_value() &&
              (*topology_decoded.value())->canonical_bytes() ==
                  symbolic_artifact->canonical_bytes() &&
              (*topology_decoded.value())->coplanar_event_nodes().size() ==
                  expected_nodes &&
              (*topology_decoded.value())->coplanar_oriented_arcs().size() ==
                  expected_arcs &&
              (*topology_decoded.value())->coplanar_overlap_components().size() ==
                  expected_components,
          "coplanar topology codec rebuilds identical first-class tables");

  auto node_mutation =
      bounded::relation_artifact_test_access::copy(*symbolic_artifact);
  auto &node = bounded::relation_artifact_test_access::coplanar_event_nodes(
      node_mutation).front();
  node.sheet_mask ^= 1U;
  bounded::relation_artifact_test_access::repair_codec(node_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(node_mutation, error),
          "matched coplanar event-node mutation is independently rejected");

  auto arc_mutation =
      bounded::relation_artifact_test_access::copy(*symbolic_artifact);
  auto &arc = bounded::relation_artifact_test_access::coplanar_oriented_arcs(
      arc_mutation).front();
  arc.start_node = arc.end_node;
  bounded::relation_artifact_test_access::repair_codec(arc_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(arc_mutation, error),
          "matched coplanar oriented-arc mutation is independently rejected");

  auto component_mutation =
      bounded::relation_artifact_test_access::copy(*symbolic_artifact);
  auto &component =
      bounded::relation_artifact_test_access::coplanar_overlap_components(
          component_mutation).front();
  component.closed = !component.closed;
  bounded::relation_artifact_test_access::repair_codec(component_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(component_mutation, error),
          "matched coplanar component mutation is independently rejected");

  auto symbolic_mutation =
      bounded::relation_artifact_test_access::copy(*symbolic_artifact);
  auto &symbolic = bounded::relation_artifact_test_access::symbolic_eligibility(
      symbolic_mutation).front();
  symbolic.rounded_nominal_zero = !symbolic.rounded_nominal_zero;
  bounded::relation_artifact_test_access::repair_codec(symbolic_mutation);
  error = bounded_boolean_error{};
  require(!bounded::verify_signed_feature_relations(symbolic_mutation, error),
          "matched symbolic evidence mutation is independently rejected");

  auto trailing = artifact->canonical_bytes();
  trailing.push_back(0);
  auto decode_fixture = overlapping_fixture();
  bounded::resource_manager decode_resources(
      resource_policy::conservative_defaults());
  auto decoded = bounded::decode_signed_feature_relations(
      trailing, decode_fixture.predecessor.context,
      *decode_fixture.predecessor.precision, decode_fixture.artifact,
      capabilities(decode_fixture, &decode_resources));
  require(!decoded.has_value() &&
              decoded.error()->subcode ==
                  static_cast<std::uint32_t>(bounded::relation_subcode::codec_error),
          "relation decode rejects trailing data before publication");
  require_no_live_resources(
      decode_resources,
      "failed relation decode must not reserve or commit resources");
}

void test_resource_boundary_and_cancellation() {
  auto reference_fixture = overlapping_fixture();
  bounded::resource_manager reference_resources(
      resource_policy::conservative_defaults());
  const auto reference = build_artifact(reference_fixture, &reference_resources);
  const auto reference_snapshot = reference_resources.snapshot();
  const auto persistent_used =
      reference_snapshot[static_cast<std::size_t>(
          bounded::resource_kind::persistent_bytes)]
          .committed;
  require(persistent_used == reference->statistics().persistent_bytes &&
              persistent_used > 0,
          "relation persistent accounting is exact and nonzero");

  auto limited_fixture = overlapping_fixture();
  auto limited_policy = resource_policy::conservative_defaults();
  limited_policy.persistent_bytes.hard = persistent_used - 1;
  limited_policy.persistent_bytes.advisory = persistent_used - 1;
  bounded::resource_manager limited_resources(limited_policy);
  auto limited = bounded::build_signed_feature_relations(
      limited_fixture.predecessor.context,
      *limited_fixture.predecessor.precision, limited_fixture.artifact,
      capabilities(limited_fixture, &limited_resources));
  require(!limited.has_value() &&
              limited.error()->category ==
                  bounded_boolean_error_category::resource_limit,
          "relation persistent limit-minus-one fails transactionally");
  require_no_live_resources(
      limited_resources,
      "resource-limited relation build must release every lease");

  auto cancelled_fixture = overlapping_fixture();
  bounded::resource_manager cancelled_resources(
      resource_policy::conservative_defaults());
  bounded_boolean_cancellation_source source;
  auto token = source.token();
  source.request_cancel(7);
  auto cancelled_caps = capabilities(cancelled_fixture, &cancelled_resources);
  cancelled_caps.cancellation = &token;
  auto cancelled = bounded::build_signed_feature_relations(
      cancelled_fixture.predecessor.context,
      *cancelled_fixture.predecessor.precision, cancelled_fixture.artifact,
      cancelled_caps);
  require(!cancelled.has_value() &&
              cancelled.error()->category ==
                  bounded_boolean_error_category::cancelled,
          "pre-cancelled relation build publishes nothing");
  require_no_live_resources(
      cancelled_resources,
      "cancelled relation build must release every lease");

  bounded::resource_manager retry_resources(
      resource_policy::conservative_defaults());
  const auto retry = build_artifact(cancelled_fixture, &retry_resources);
  require(retry->canonical_bytes() == reference->canonical_bytes(),
          "retry after cancellation reproduces canonical relation bytes");
}

} // namespace

int main() {
  try {
    test_empty_artifact_and_decode();
    test_nonempty_determinism_and_decode();
    test_matched_mutation_rejection();
    test_resource_boundary_and_cancellation();
    std::cout << "Component 07 final artifact qualification checks passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
