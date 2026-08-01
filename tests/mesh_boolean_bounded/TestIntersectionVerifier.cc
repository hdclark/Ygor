#include "BroadPhaseFixtures.h"
#include "YgorMeshesBooleanBounded/CoplanarCarrierArrangements.h"
#include "YgorMeshesBooleanBounded/EventCoordinates.h"
#include "YgorMeshesBooleanBounded/EventIncidence.h"
#include "YgorMeshesBooleanBounded/EventInterning.h"
#include "YgorMeshesBooleanBounded/IntersectionAggregation.h"
#include "YgorMeshesBooleanBounded/IntersectionDescriptors.h"
#include "YgorMeshesBooleanBounded/IntersectionVerifier.h"
#include "YgorMeshesBooleanBounded/RelationBuild.h"
#include "YgorMeshesBooleanBounded/SourceEdgeArrangements.h"
#include "YgorMeshesBooleanBounded/TransverseCarrierArrangements.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace bounded = ygor::mesh_boolean::bounded;
using broad_phase_tests::require;

namespace ygor::mesh_boolean::bounded {

struct intersection_artifact_test_access final {
  template <class T, class I>
  static auto &intervals(canonical_intersection_complex<T, I> &artifact) {
    return artifact.source_edge_intervals_;
  }
  template <class T, class I>
  static auto &sequences(canonical_intersection_complex<T, I> &artifact) {
    return artifact.source_edge_sequences_;
  }
  template <class T, class I>
  static auto &descriptors(canonical_intersection_complex<T, I> &artifact) {
    return artifact.descriptors_;
  }
  template <class T, class I>
  static auto &bytes(canonical_intersection_complex<T, I> &artifact) {
    return artifact.canonical_bytes_;
  }
  template <class T, class I>
  static auto &digest(canonical_intersection_complex<T, I> &artifact) {
    return artifact.digest_;
  }
  template <class T, class I>
  static auto &reserved(canonical_intersection_complex<T, I> &artifact) {
    return artifact.reserved_;
  }
  template <class T, class I>
  static auto &verification(canonical_intersection_complex<T, I> &artifact) {
    return artifact.verification_;
  }
  template <class T, class I>
  static auto &evidence(canonical_intersection_complex<T, I> &artifact) {
    return artifact.verification_evidence_;
  }
  template <class T, class I>
  static auto &statistics(canonical_intersection_complex<T, I> &artifact) {
    return artifact.statistics_;
  }
};

} // namespace ygor::mesh_boolean::bounded

namespace {

using relation_type = bounded::signed_feature_relations<double, std::uint32_t>;
using artifact_type = bounded::canonical_intersection_complex<double, std::uint32_t>;

std::shared_ptr<const relation_type> make_relations(
    broad_phase_tests::built_fixture &fixture) {
  bounded::resource_manager resources(resource_policy::conservative_defaults());
  bounded::relation_capabilities capabilities;
  capabilities.owner = fixture.predecessor.context.owner;
  capabilities.resources = &resources;
  auto outcome = bounded::build_signed_feature_relations(
      fixture.predecessor.context, *fixture.predecessor.precision,
      fixture.artifact, capabilities);
  require(outcome.has_value(), "Component 08 verifier fixture relation build failed");
  return *outcome.value();
}

std::vector<bounded::source_edge_domain_record> source_domains(
    const relation_type &relations) {
  std::vector<bounded::source_edge_domain_record> domains;
  for (const auto operand : {bounded::operand_id::a, bounded::operand_id::b}) {
    const auto &table = relations.candidates()->primitive_table(operand);
    for (const auto &edge : table.edges) {
      if (edge.edge_class != bounded::canonical_edge_class::source_edge ||
          !edge.source_feature_owner)
        continue;
      bounded::source_edge_domain_record domain;
      domain.source_edge.operand = operand;
      domain.source_edge.kind = bounded::relation_feature_kind::source_edge;
      domain.source_edge.primary = edge.semantic_key.primary;
      domain.source_edge.secondary = edge.semantic_key.secondary;
      domain.start_vertex.operand = operand;
      domain.start_vertex.kind = bounded::relation_feature_kind::source_vertex;
      domain.start_vertex.primary = edge.semantic_key.primary;
      domain.end_vertex.operand = operand;
      domain.end_vertex.kind = bounded::relation_feature_kind::source_vertex;
      domain.end_vertex.primary = edge.semantic_key.secondary;
      domains.push_back(domain);
    }
  }
  std::sort(domains.begin(), domains.end(), [](const auto &a, const auto &b) {
    return a.source_edge < b.source_edge;
  });
  return domains;
}

bounded::intersection_canonicalization_header header(
    const relation_type &relations) {
  bounded::intersection_canonicalization_header value;
  value.owner = relations.owner();
  value.operation = relations.operation();
  value.context_digest = relations.context_digest();
  value.precision_digest = relations.precision_digest();
  value.relation_digest = relations.digest();
  value.source_semantic_digests[0] =
      relations.candidates()->primitive_table(bounded::operand_id::a)
          .source_semantic_digest;
  value.source_semantic_digests[1] =
      relations.candidates()->primitive_table(bounded::operand_id::b)
          .source_semantic_digest;
  value.exact_triangulation_digests[0] =
      relations.candidates()->primitive_table(bounded::operand_id::a)
          .exact_topology_digest;
  value.exact_triangulation_digests[1] =
      relations.candidates()->primitive_table(bounded::operand_id::b)
          .exact_topology_digest;
  return value;
}

artifact_type make_artifact(const relation_type &relations) {
  std::vector<bounded::normalized_event_seed_proposal> proposals;
  bounded_boolean_error error;
  require(bounded::normalize_event_seed_records(
              relations.event_seeds(), relations.constructions(), proposals,
              error),
          "Component 08 verifier fixture normalization failed");

  bounded::event_interning_tables interning;
  require(bounded::intern_normalized_event_seeds(proposals, interning, error),
          "Component 08 verifier fixture interning failed");
  bounded::event_coordinate_tables coordinates;
  require(bounded::attach_event_coordinates(
              proposals, relations.constructions(),
              relations.construction_ledger(), interning, coordinates, error),
          "Component 08 verifier fixture coordinate attachment failed");
  bounded::event_incidence_tables incidence;
  require(bounded::build_event_incidence(relations, interning, incidence, error),
          "Component 08 verifier fixture incidence failed");

  std::vector<bounded::source_edge_membership_proposal> memberships;
  require(bounded::collect_source_edge_membership_proposals(
              relations.event_seeds(), relations.interval_evidence(),
              interning, incidence, memberships, error),
          "Component 08 verifier fixture source memberships failed");
  bounded::source_edge_arrangement_tables source_edges;
  require(bounded::build_source_edge_arrangements<double>(
              source_domains(relations), memberships, source_edges, error),
          "Component 08 verifier fixture source arrangements failed");

  bounded::transverse_carrier_arrangement_tables transverse;
  require(bounded::build_transverse_carrier_arrangements<double>(
              {}, {}, {}, transverse, error),
          "Component 08 verifier fixture transverse arrangements failed");
  bounded::coplanar_carrier_arrangement_tables coplanar;
  require(bounded::build_coplanar_carrier_arrangements<double>(
              {}, {}, {}, {}, coplanar, error),
          "Component 08 verifier fixture coplanar arrangements failed");

  bounded::intersection_aggregate_tables aggregates;
  require(bounded::build_intersection_aggregates(
              relations.event_seeds(), interning, incidence, source_edges,
              transverse, coplanar, aggregates, error),
          "Component 08 verifier fixture aggregation failed");
  bounded::intersection_descriptor_tables base_descriptors;
  require(bounded::build_intersection_descriptors(
              relations.event_seeds(), interning, incidence, source_edges,
              transverse, coplanar, aggregates, base_descriptors, error),
          "Component 08 verifier fixture base descriptors failed");
  bounded::intersection_descriptor_tables descriptors;
  require(bounded::extend_intersection_descriptors_with_source_topology(
              *relations.candidates()->manifolds(), relations.crossings(),
              relations.event_seeds(), interning, incidence, base_descriptors,
              descriptors, error),
          "Component 08 verifier fixture topology descriptors failed");

  artifact_type artifact;
  require(bounded::canonicalize_intersection_tables(
              header(relations), interning, coordinates, incidence,
              source_edges, transverse, coplanar, aggregates, descriptors,
              artifact, error),
          "Component 08 verifier fixture canonicalization failed");
  require(bounded::refresh_intersection_codec(
              artifact, bounded::intersection_codec_limits{}, error),
          "Component 08 verifier fixture codec failed");
  return artifact;
}

void reset_unverified(artifact_type &artifact) {
  bounded::intersection_artifact_test_access::verification(artifact) =
      bounded::intersection_verification_disposition::not_verified;
  bounded::intersection_artifact_test_access::evidence(artifact) = {};
  bounded::intersection_artifact_test_access::statistics(artifact)
      .verifier_work_units = 0;
}

template <class Mutation>
void require_repaired_mutation_rejected(const relation_type &relations,
                                        const artifact_type &verified,
                                        Mutation mutation) {
  auto candidate = verified;
  reset_unverified(candidate);
  mutation(candidate);
  bounded_boolean_error error;
  const bounded::intersection_codec_limits codec_limits;
  require(bounded::refresh_intersection_codec(candidate, codec_limits, error),
          "mutation must have a repaired canonical codec");
  bounded::intersection_verification_evidence evidence;
  require(!bounded::verify_intersection_complex_independent(
              relations, candidate, codec_limits,
              bounded::intersection_verifier_limits{}, evidence, error),
          "independent verifier accepted a repaired semantic mutation");
}

} // namespace

int main() {
  auto fixture = broad_phase_tests::build(
      broad_phase_tests::box(),
      broad_phase_tests::box(4.0, 4.0, 4.0, 5.0, 5.0, 5.0));
  const auto relations = make_relations(fixture);
  require(relations->event_seeds().empty(),
          "verifier fixture must isolate empty event lineage");

  auto artifact = make_artifact(*relations);
  const auto unverified_bytes = artifact.canonical_bytes();
  bounded_boolean_error error;
  bounded::intersection_verification_evidence evidence;
  const bounded::intersection_codec_limits codec_limits;
  const bounded::intersection_verifier_limits verifier_limits;
  const bool initial_verified =
      bounded::verify_intersection_complex_independent(
          *relations, artifact, codec_limits, verifier_limits, evidence, error);
  if (!initial_verified)
    std::cerr << "verifier failure: subcode=" << error.subcode
              << " summary=" << error.summary << '\n';
  require(initial_verified,
          "independent verifier rejected canonical unverified artifact");
  require(evidence.seed_regrouped && evidence.incidence_reconstructed &&
              evidence.arrangements_reconstructed &&
              evidence.descriptors_reconstructed && evidence.work_units != 0,
          "independent verifier did not publish complete evidence");
  require(bounded::finalize_intersection_complex_verification(
              *relations, artifact, codec_limits, verifier_limits, error),
          "Component 08 verification finalization failed");
  require(artifact.verification() ==
              bounded::intersection_verification_disposition::
                  independently_verified &&
              artifact.statistics().verifier_work_units ==
                  artifact.verification_evidence().work_units,
          "Component 08 verification publication is incomplete");

  artifact_type decoded_unverified;
  const bool decoded_unverified_ok =
      bounded::decode_intersection_complex_verified_private(
          unverified_bytes, header(*relations), *relations, codec_limits,
          verifier_limits, decoded_unverified, error);
  if (!decoded_unverified_ok)
    std::cerr << "decode failure: subcode=" << error.subcode
              << " summary=" << error.summary << '\n';
  require(decoded_unverified_ok,
          "Component 08 verified decode rejected unverified canonical bytes");
  require(decoded_unverified.verification() ==
              bounded::intersection_verification_disposition::
                  independently_verified &&
              decoded_unverified.canonical_bytes() == artifact.canonical_bytes(),
          "Component 08 verified decode did not publish canonical evidence");

  artifact_type decoded_verified;
  require(bounded::decode_intersection_complex_verified_private(
              artifact.canonical_bytes(), header(*relations), *relations,
              codec_limits, verifier_limits, decoded_verified, error),
          "Component 08 verified decode rejected verified canonical bytes");
  require(decoded_verified.canonical_bytes() == artifact.canonical_bytes() &&
              decoded_verified.verification_evidence().reconstructed_digest ==
                  artifact.verification_evidence().reconstructed_digest,
          "Component 08 verified decode changed a verified artifact");
  const auto bytes = artifact.canonical_bytes();
  require(bounded::finalize_intersection_complex_verification(
              *relations, artifact, codec_limits, verifier_limits, error) &&
              artifact.canonical_bytes() == bytes,
          "Component 08 verification finalization is not idempotent");

  require(!artifact.source_edge_intervals().empty() &&
              !artifact.source_edge_sequences().empty() &&
              !artifact.descriptors().empty(),
          "verifier fixture lacks source topology records");

  require_repaired_mutation_rejected(
      *relations, artifact, [](artifact_type &candidate) {
        ++bounded::intersection_artifact_test_access::intervals(candidate)[0]
              .key.canonical_ordinal;
      });
  require_repaired_mutation_rejected(
      *relations, artifact, [](artifact_type &candidate) {
        ++bounded::intersection_artifact_test_access::sequences(candidate)[0]
              .start.source_vertex.primary;
      });
  require_repaired_mutation_rejected(
      *relations, artifact, [](artifact_type &candidate) {
        auto &descriptors =
            bounded::intersection_artifact_test_access::descriptors(candidate);
        descriptors[0].continuation_allowed =
            !descriptors[0].continuation_allowed;
      });

  bool mutated_internal_diagonal = false;
  for (std::size_t i = 0; i < artifact.descriptors().size(); ++i) {
    if (artifact.descriptors()[i].key.locus !=
        bounded::intersection_descriptor_locus::
            transparent_internal_diagonal_adjacency)
      continue;
    require_repaired_mutation_rejected(
        *relations, artifact, [i](artifact_type &candidate) {
          bounded::intersection_artifact_test_access::descriptors(candidate)[i]
              .selection_consumable = true;
        });
    mutated_internal_diagonal = true;
    break;
  }
  require(mutated_internal_diagonal,
          "verifier fixture lacks transparent internal-diagonal descriptor");

  auto byte_corruption = artifact;
  bounded::intersection_artifact_test_access::bytes(byte_corruption)[80] ^= 1;
  require(!bounded::verify_intersection_complex_independent(
              *relations, byte_corruption, codec_limits, verifier_limits,
              evidence, error),
          "independent verifier accepted byte corruption");

  auto digest_corruption = artifact;
  bounded::intersection_artifact_test_access::digest(digest_corruption)
      .bytes[0] ^= 1;
  require(!bounded::verify_intersection_complex_independent(
              *relations, digest_corruption, codec_limits, verifier_limits,
              evidence, error),
          "independent verifier accepted digest corruption");

  auto forged_evidence = artifact;
  bounded::intersection_artifact_test_access::evidence(forged_evidence)
      .reconstructed_digest.bytes[0] ^= 1;
  require(bounded::refresh_intersection_codec(forged_evidence, codec_limits,
                                              error),
          "forged evidence must retain a repaired canonical codec");
  require(!bounded::verify_intersection_complex_independent(
              *relations, forged_evidence, codec_limits, verifier_limits,
              evidence, error),
          "independent verifier accepted forged evidence");

  auto malformed_limits = verifier_limits;
  malformed_limits.reserved32 = 1;
  require(!bounded::verify_intersection_complex_independent(
              *relations, artifact, codec_limits, malformed_limits, evidence,
              error),
          "independent verifier accepted reserved limit fields");
}
