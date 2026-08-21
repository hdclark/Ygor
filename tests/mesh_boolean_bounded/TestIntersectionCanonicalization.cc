#include "YgorMeshesBooleanBounded/IntersectionCanonicalization.h"

#include <cstdlib>

using namespace ygor::mesh_boolean::bounded;

namespace ygor::mesh_boolean::bounded {

struct intersection_artifact_test_access final {
  template <class T, class I>
  static intersection_descriptor_record &descriptor(
      canonical_intersection_complex<T, I> &artifact, std::size_t ordinal) {
    return artifact.descriptors_[ordinal];
  }

  template <class T, class I>
  static ordering_certificate_record &certificate(
      canonical_intersection_complex<T, I> &artifact, std::size_t ordinal) {
    return artifact.ordering_certificates_[ordinal];
  }

  template <class T, class I>
  static intersection_statistics &statistics(
      canonical_intersection_complex<T, I> &artifact) {
    return artifact.statistics_;
  }

  template <class T, class I>
  static bounded_boolean_digest &section_digest(
      canonical_intersection_complex<T, I> &artifact, std::size_t ordinal) {
    return artifact.section_digests_[ordinal];
  }
};

} // namespace ygor::mesh_boolean::bounded

namespace {

void require(bool condition) {
  if (!condition)
    std::abort();
}

ordering_certificate_record certificate(std::uint64_t ordinal,
                                        std::uint64_t first,
                                        std::uint64_t second) {
  ordering_certificate_record value;
  value.id = ordering_certificate_id{ordinal};
  value.disposition = intersection_order_disposition::definitely_before;
  value.first_parameter = relation_interval_evidence_id{first};
  value.second_parameter = relation_interval_evidence_id{second};
  value.comparison_evidence_lineage = ordinal + 100;
  value.topology_safe = true;
  return value;
}

intersection_canonicalization_header header() {
  intersection_canonicalization_header value;
  value.owner = context_owner_token::create();
  value.context_digest.bytes[0] = 0x11;
  value.precision_digest.bytes[0] = 0x22;
  value.relation_digest.bytes[0] = 0x33;
  value.source_semantic_digests[0].bytes[0] = 0x44;
  value.source_semantic_digests[1].bytes[0] = 0x45;
  value.exact_triangulation_digests[0].bytes[0] = 0x54;
  value.exact_triangulation_digests[1].bytes[0] = 0x55;
  return value;
}

} // namespace

int main() {
  const auto expected_header = header();
  event_interning_tables interning;
  event_coordinate_tables coordinates;
  event_incidence_tables incidence;
  source_edge_arrangement_tables source_edges;
  transverse_carrier_arrangement_tables transverse;
  coplanar_carrier_arrangement_tables coplanar;
  intersection_aggregate_tables aggregates;
  intersection_descriptor_tables descriptors;

  source_edges.ordering_certificates.push_back(certificate(0, 10, 11));
  transverse.ordering_certificates.push_back(certificate(0, 20, 21));

  intersection_descriptor_record descriptor;
  descriptor.id = intersection_descriptor_id{0};
  descriptors.records.push_back(descriptor);

  bounded_boolean_error error;
  canonical_intersection_complex<double, std::uint64_t> artifact;
  require(canonicalize_intersection_tables(
      expected_header, interning, coordinates, incidence, source_edges,
      transverse, coplanar, aggregates, descriptors, artifact, error));
  require(artifact.owner().same_owner(expected_header.owner));
  require(artifact.verification() ==
          intersection_verification_disposition::not_verified);
  require(artifact.ordering_certificates().size() == 2);
  require(artifact.ordering_certificates()[0].id == ordering_certificate_id{0});
  require(artifact.ordering_certificates()[1].id == ordering_certificate_id{1});
  require(artifact.statistics().ordering_certificate_count == 2);
  require(artifact.statistics().descriptor_count == 1);
  require(artifact.statistics().persistent_bytes != 0);
  require(verify_intersection_canonicalization(
      expected_header, interning, coordinates, incidence, source_edges,
      transverse, coplanar, aggregates, descriptors, artifact, error));

  auto mutated = artifact;
  intersection_artifact_test_access::descriptor(mutated, 0)
      .signed_crossing_delta = 1;
  require(!verify_intersection_canonicalization(
      expected_header, interning, coordinates, incidence, source_edges,
      transverse, coplanar, aggregates, descriptors, mutated, error));
  require(error.subcode ==
          static_cast<std::uint32_t>(intersection_subcode::verifier_rejection));

  mutated = artifact;
  intersection_artifact_test_access::certificate(mutated, 1).id =
      ordering_certificate_id{0};
  require(!verify_intersection_canonicalization(
      expected_header, interning, coordinates, incidence, source_edges,
      transverse, coplanar, aggregates, descriptors, mutated, error));

  mutated = artifact;
  ++intersection_artifact_test_access::statistics(mutated).persistent_bytes;
  require(!verify_intersection_canonicalization(
      expected_header, interning, coordinates, incidence, source_edges,
      transverse, coplanar, aggregates, descriptors, mutated, error));

  mutated = artifact;
  intersection_artifact_test_access::section_digest(mutated, 0).bytes[0] = 1;
  require(!verify_intersection_canonicalization(
      expected_header, interning, coordinates, incidence, source_edges,
      transverse, coplanar, aggregates, descriptors, mutated, error));

  auto malformed_header = expected_header;
  malformed_header.reserved32 = 1;
  canonical_intersection_complex<double, std::uint64_t> rejected = artifact;
  require(!canonicalize_intersection_tables(
      malformed_header, interning, coordinates, incidence, source_edges,
      transverse, coplanar, aggregates, descriptors, rejected, error));
  require(rejected.descriptors().size() == artifact.descriptors().size());
  require(rejected.ordering_certificates().size() ==
          artifact.ordering_certificates().size());

  auto malformed_source = source_edges;
  malformed_source.ordering_certificates[0].id = ordering_certificate_id{1};
  require(!canonicalize_intersection_tables(
      expected_header, interning, coordinates, incidence, malformed_source,
      transverse, coplanar, aggregates, descriptors, rejected, error));

  auto malformed_descriptor = descriptors;
  malformed_descriptor.records[0].key.reserved = 1;
  require(!canonicalize_intersection_tables(
      expected_header, interning, coordinates, incidence, source_edges,
      transverse, coplanar, aggregates, malformed_descriptor, rejected,
      error));

  event_interning_tables empty_interning;
  event_coordinate_tables empty_coordinates;
  event_incidence_tables empty_incidence;
  source_edge_arrangement_tables empty_source;
  transverse_carrier_arrangement_tables empty_transverse;
  coplanar_carrier_arrangement_tables empty_coplanar;
  intersection_aggregate_tables empty_aggregates;
  intersection_descriptor_tables empty_descriptors;
  canonical_intersection_complex<float, std::uint32_t> empty_artifact;
  require(canonicalize_intersection_tables(
      expected_header, empty_interning, empty_coordinates, empty_incidence,
      empty_source, empty_transverse, empty_coplanar, empty_aggregates,
      empty_descriptors, empty_artifact, error));
  require(empty_artifact.statistics().persistent_bytes == 0);
  require(verify_intersection_canonicalization(
      expected_header, empty_interning, empty_coordinates, empty_incidence,
      empty_source, empty_transverse, empty_coplanar, empty_aggregates,
      empty_descriptors, empty_artifact, error));
}
