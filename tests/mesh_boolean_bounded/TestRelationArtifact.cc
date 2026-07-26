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
  }

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
