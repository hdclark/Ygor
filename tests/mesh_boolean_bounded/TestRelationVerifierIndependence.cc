#include "BroadPhaseFixtures.h"
#include "YgorMeshesBooleanBounded/RelationBuild.h"
#include "YgorMeshesBooleanBounded/RelationReplay.h"

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace bounded = ygor::mesh_boolean::bounded;
using broad_phase_tests::diagnostic;

namespace ygor::mesh_boolean::bounded {

struct relation_artifact_test_access final {
  template <class T, class I>
  static const auto &source_edge_stage(
      const signed_feature_relations<T, I> &artifact) {
    return artifact.source_edge_stage_;
  }

  template <class T, class I>
  static const auto &source_edge_facet_stage(
      const signed_feature_relations<T, I> &artifact) {
    return artifact.source_edge_facet_stage_;
  }

  template <class T, class I>
  static const auto &source_facet_stage(
      const signed_feature_relations<T, I> &artifact) {
    return artifact.source_facet_stage_;
  }

  template <class T, class I>
  static const auto &coplanar_overlay_stage(
      const signed_feature_relations<T, I> &artifact) {
    return artifact.coplanar_overlay_stage_;
  }

  template <class T, class I>
  static signed_feature_relations<T, I>
  copy(const signed_feature_relations<T, I> &artifact) {
    return artifact;
  }

  template <class T, class I>
  static void replace_producer_stages(
      signed_feature_relations<T, I> &artifact,
      std::shared_ptr<const candidate_source_edge_relation_stage<T>> edge,
      std::shared_ptr<const candidate_source_edge_facet_relation_stage<T>>
          edge_facet,
      std::shared_ptr<const candidate_source_facet_relation_stage<T>> facet,
      std::shared_ptr<const candidate_coplanar_overlay_stage<T>> coplanar) {
    artifact.source_edge_stage_ = std::move(edge);
    artifact.source_edge_facet_stage_ = std::move(edge_facet);
    artifact.source_facet_stage_ = std::move(facet);
    artifact.coplanar_overlay_stage_ = std::move(coplanar);
  }

  template <class T, class I>
  static void set_public_status(signed_feature_relations<T, I> &artifact,
                                const relation_request_key &key,
                                feature_relation_status status) {
    for (auto &relation : artifact.relations_) {
      if (relation.producer.ordinal() >= artifact.request_graph_.requests.size())
        continue;
      if (artifact.request_graph_.requests[relation.producer.ordinal()].key == key) {
        relation.status = status;
        return;
      }
    }
    throw std::runtime_error("mutation source relation is absent");
  }

  template <class T, class I>
  static void repair_replay_and_codec(signed_feature_relations<T, I> &artifact) {
    relation_capabilities capabilities;
    capabilities.owner = artifact.owner_;
    bounded_boolean_error error;
    if (!build_relation_replay_bundle(artifact, capabilities, error))
      throw std::runtime_error("unable to rebuild relation replay mutation");
    artifact.canonical_bytes_ = encode_signed_feature_relations(artifact);
    artifact.digest_ = sha256::digest(artifact.canonical_bytes_);
  }
};

} // namespace ygor::mesh_boolean::bounded

int main() {
  try {
    auto fixture = broad_phase_tests::build(
        broad_phase_tests::box(),
        broad_phase_tests::box(0.5, 0.25, 0.125, 1.5, 1.25, 1.125));
    bounded::resource_manager resources(
        resource_policy::conservative_defaults());
    bounded::relation_capabilities capabilities;
    capabilities.owner = fixture.predecessor.context.owner;
    capabilities.resources = &resources;
    auto built = bounded::build_signed_feature_relations(
        fixture.predecessor.context, *fixture.predecessor.precision,
        fixture.artifact, capabilities);
    if (!built.has_value())
      throw std::runtime_error(diagnostic(*built.error()));
    const auto &artifact = **built.value();
    auto independent = bounded::relation_artifact_test_access::copy(artifact);
    auto edge = std::make_shared<
        bounded::candidate_source_edge_relation_stage<double>>(
        *bounded::relation_artifact_test_access::source_edge_stage(artifact));
    auto edge_facet = std::make_shared<
        bounded::candidate_source_edge_facet_relation_stage<double>>(
        *bounded::relation_artifact_test_access::source_edge_facet_stage(artifact));
    auto facet = std::make_shared<
        bounded::candidate_source_facet_relation_stage<double>>(
        *bounded::relation_artifact_test_access::source_facet_stage(artifact));
    auto coplanar = std::make_shared<
        bounded::candidate_coplanar_overlay_stage<double>>(
        *bounded::relation_artifact_test_access::coplanar_overlay_stage(artifact));
    edge->evaluation_count += 17;
    edge_facet->evaluation_count += 19;
    facet->evaluation_count += 23;
    coplanar->evaluation_count += 29;
    bounded::relation_artifact_test_access::replace_producer_stages<
        double, std::uint32_t>(
        independent, std::move(edge), std::move(edge_facet), std::move(facet),
        std::move(coplanar));
    bounded::relation_artifact_test_access::repair_replay_and_codec(independent);
    bounded_boolean_error error;
    if (!bounded::verify_signed_feature_relations(independent, error))
      throw std::runtime_error(diagnostic(error));

    const auto require_matched_rejection = [&](const auto &base, auto mutate) {
      auto candidate = bounded::relation_artifact_test_access::copy(base);
      auto candidate_edge = std::make_shared<
          bounded::candidate_source_edge_relation_stage<double>>(
          *bounded::relation_artifact_test_access::source_edge_stage(base));
      auto candidate_edge_facet = std::make_shared<
          bounded::candidate_source_edge_facet_relation_stage<double>>(
          *bounded::relation_artifact_test_access::source_edge_facet_stage(
              base));
      auto candidate_facet = std::make_shared<
          bounded::candidate_source_facet_relation_stage<double>>(
          *bounded::relation_artifact_test_access::source_facet_stage(base));
      auto candidate_coplanar = std::make_shared<
          bounded::candidate_coplanar_overlay_stage<double>>(
          *bounded::relation_artifact_test_access::coplanar_overlay_stage(
              base));
      mutate(candidate, *candidate_edge, *candidate_edge_facet,
             *candidate_facet, *candidate_coplanar);
      for (auto &record : candidate_edge->relations)
        record.semantic_digest = bounded::sha256::digest(
            bounded::encode_source_edge_relation_semantics(record));
      candidate_edge->semantic_digest = bounded::sha256::digest(
          bounded::encode_candidate_source_edge_relation_semantics(
              *candidate_edge));
      for (auto &record : candidate_edge_facet->relations)
        record.semantic_digest = bounded::sha256::digest(
            bounded::encode_source_edge_facet_relation_semantics(record));
      candidate_edge_facet->semantic_digest = bounded::sha256::digest(
          bounded::encode_candidate_source_edge_facet_relation_semantics(
              *candidate_edge_facet));
      for (auto &record : candidate_facet->relations)
        record.semantic_digest = bounded::sha256::digest(
            bounded::encode_source_facet_relation_semantics(record));
      candidate_facet->semantic_digest = bounded::sha256::digest(
          bounded::encode_candidate_source_facet_relation_semantics(
              *candidate_facet));
      for (auto &record : candidate_coplanar->overlays)
        record.semantic_digest = bounded::sha256::digest(
            bounded::encode_coplanar_overlay_semantics(record));
      candidate_coplanar->semantic_digest = bounded::sha256::digest(
          bounded::encode_candidate_coplanar_overlay_semantics(
              *candidate_coplanar));
      bounded::relation_artifact_test_access::replace_producer_stages<
          double, std::uint32_t>(
          candidate, std::move(candidate_edge),
          std::move(candidate_edge_facet), std::move(candidate_facet),
          std::move(candidate_coplanar));
      bounded::relation_artifact_test_access::repair_replay_and_codec(candidate);
      bounded_boolean_error mutation_error;
      if (bounded::verify_signed_feature_relations(candidate, mutation_error))
        throw std::runtime_error(
            "matched producer category/public status mutation survived");
    };

    require_matched_rejection(artifact,
        [](auto &candidate, auto &edge_stage, auto &, auto &, auto &) {
          if (edge_stage.relations.empty())
            throw std::runtime_error("edge mutation fixture is empty");
          auto &record = edge_stage.relations.front();
          const auto &key = edge_stage.request_graph.requests.front().key;
          record.contact = record.contact ==
                                   bounded::source_edge_contact_class::none
                               ? bounded::source_edge_contact_class::proper_crossing
                               : bounded::source_edge_contact_class::none;
          bounded::relation_artifact_test_access::set_public_status(
              candidate, key,
              record.contact == bounded::source_edge_contact_class::none
                  ? bounded::feature_relation_status::definitely_separated
                  : bounded::feature_relation_status::proper_crossing);
        });
    require_matched_rejection(artifact,
        [](auto &candidate, auto &, auto &edge_facet_stage, auto &, auto &) {
          if (edge_facet_stage.relations.empty())
            throw std::runtime_error("edge/facet mutation fixture is empty");
          auto &record = edge_facet_stage.relations.front();
          const auto &key = edge_facet_stage.request_graph.requests.front().key;
          record.contact =
              record.contact == bounded::source_edge_facet_contact_class::none
                  ? bounded::source_edge_facet_contact_class::proper_face_crossing
                  : bounded::source_edge_facet_contact_class::none;
          bounded::relation_artifact_test_access::set_public_status(
              candidate, key,
              record.contact == bounded::source_edge_facet_contact_class::none
                  ? bounded::feature_relation_status::definitely_separated
                  : bounded::feature_relation_status::proper_crossing);
        });
    require_matched_rejection(artifact,
        [](auto &candidate, auto &, auto &, auto &facet_stage, auto &) {
          if (facet_stage.relations.empty())
            throw std::runtime_error("facet mutation fixture is empty");
          auto &record = facet_stage.relations.front();
          const auto &key = facet_stage.request_graph.requests.front().key;
          record.classification =
              record.classification ==
                      bounded::source_facet_support_relation_class::transverse
                  ? bounded::source_facet_support_relation_class::parallel_separated
                  : bounded::source_facet_support_relation_class::transverse;
          bounded::relation_artifact_test_access::set_public_status(
              candidate, key,
              record.classification ==
                      bounded::source_facet_support_relation_class::transverse
                  ? bounded::feature_relation_status::proper_crossing
                  : bounded::feature_relation_status::definitely_separated);
        });
    auto overlay_fixture = broad_phase_tests::build(
        broad_phase_tests::box(),
        broad_phase_tests::box(0.25, 0.25, 1.0, 0.75, 0.75, 2.0));
    bounded::resource_manager overlay_resources(
        resource_policy::conservative_defaults());
    bounded::relation_capabilities overlay_capabilities;
    overlay_capabilities.owner = overlay_fixture.predecessor.context.owner;
    overlay_capabilities.resources = &overlay_resources;
    auto overlay_built = bounded::build_signed_feature_relations(
        overlay_fixture.predecessor.context,
        *overlay_fixture.predecessor.precision, overlay_fixture.artifact,
        overlay_capabilities);
    if (!overlay_built.has_value())
      throw std::runtime_error(diagnostic(*overlay_built.error()));
    const auto &overlay_artifact = **overlay_built.value();
    require_matched_rejection(overlay_artifact,
        [](auto &candidate, auto &, auto &, auto &, auto &coplanar_stage) {
          if (coplanar_stage.overlays.empty())
            throw std::runtime_error("overlay mutation fixture is empty");
          auto &record = coplanar_stage.overlays.front();
          record.classification =
              record.classification == bounded::coplanar_facet_overlay_class::disjoint
                  ? bounded::coplanar_facet_overlay_class::point_contact
                  : bounded::coplanar_facet_overlay_class::disjoint;
          bool updated = false;
          for (const auto &request : candidate.request_graph().requests) {
            if (request.key.family != bounded::relation_request_family::
                                          coplanar_source_facet_overlay ||
                request.key.first != record.facets[0].feature ||
                request.key.second != record.facets[1].feature)
              continue;
            bounded::relation_artifact_test_access::set_public_status(
                candidate, request.key,
                record.classification ==
                        bounded::coplanar_facet_overlay_class::disjoint
                    ? bounded::feature_relation_status::definitely_separated
                    : bounded::feature_relation_status::point_contact);
            updated = true;
            break;
          }
          if (!updated)
            throw std::runtime_error("overlay public relation is absent");
        });
    std::cout << "Component 07 verifier independence mutation passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
