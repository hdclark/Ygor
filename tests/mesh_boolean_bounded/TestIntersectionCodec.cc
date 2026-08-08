#include "YgorMeshesBooleanBounded/IntersectionCodec.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

using namespace ygor::mesh_boolean::bounded;

namespace ygor::mesh_boolean::bounded {

struct intersection_artifact_test_access final {
  template <class T, class I>
  static std::vector<std::uint8_t> &bytes(
      canonical_intersection_complex<T, I> &artifact) {
    return artifact.canonical_bytes_;
  }

  template <class T, class I>
  static bounded_boolean_digest &digest(
      canonical_intersection_complex<T, I> &artifact) {
    return artifact.digest_;
  }

  template <class T, class I>
  static bounded_boolean_digest &section_digest(
      canonical_intersection_complex<T, I> &artifact, std::size_t ordinal) {
    return artifact.section_digests_[ordinal];
  }

  template <class T, class I>
  static intersection_statistics &statistics(
      canonical_intersection_complex<T, I> &artifact) {
    return artifact.statistics_;
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

template <class T, class I>
canonical_intersection_complex<T, I> make_artifact(
    const intersection_canonicalization_header &expected_header) {
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
  canonical_intersection_complex<T, I> artifact;
  require(canonicalize_intersection_tables(
      expected_header, interning, coordinates, incidence, source_edges,
      transverse, coplanar, aggregates, descriptors, artifact, error));
  return artifact;
}

template <class T, class I>
canonical_intersection_complex<T, I> make_empty_artifact(
    const intersection_canonicalization_header &expected_header) {
  event_interning_tables interning;
  event_coordinate_tables coordinates;
  event_incidence_tables incidence;
  source_edge_arrangement_tables source_edges;
  transverse_carrier_arrangement_tables transverse;
  coplanar_carrier_arrangement_tables coplanar;
  intersection_aggregate_tables aggregates;
  intersection_descriptor_tables descriptors;
  bounded_boolean_error error;
  canonical_intersection_complex<T, I> artifact;
  require(canonicalize_intersection_tables(
      expected_header, interning, coordinates, incidence, source_edges,
      transverse, coplanar, aggregates, descriptors, artifact, error));
  return artifact;
}

} // namespace

int main() {
  const auto expected_header = header();
  const intersection_codec_limits limits;
  bounded_boolean_error error;

  auto artifact =
      make_artifact<double, std::uint64_t>(expected_header);
  require(artifact.canonical_bytes().empty());
  require(refresh_intersection_codec(artifact, limits, error));
  require(!artifact.canonical_bytes().empty());
  require(artifact.statistics().canonical_bytes ==
          artifact.canonical_bytes().size());
  require(verify_intersection_codec(artifact, limits, error));

  const auto first_bytes = artifact.canonical_bytes();
  const auto first_digest = artifact.digest();
  const auto first_sections = artifact.section_digests();
  require(refresh_intersection_codec(artifact, limits, error));
  require(artifact.canonical_bytes() == first_bytes);
  require(artifact.digest() == first_digest);
  require(artifact.section_digests() == first_sections);

  canonical_intersection_complex<double, std::uint64_t> decoded;
  require(decode_intersection_complex_private(
      artifact.canonical_bytes(), expected_header, limits, decoded, error));
  require(decoded.canonical_bytes() == artifact.canonical_bytes());
  require(decoded.digest() == artifact.digest());
  require(decoded.section_digests() == artifact.section_digests());
  require(decoded.ordering_certificates().size() ==
          artifact.ordering_certificates().size());
  require(decoded.ordering_certificates()[0].id ==
          artifact.ordering_certificates()[0].id);
  require(decoded.ordering_certificates()[1].id ==
          artifact.ordering_certificates()[1].id);
  require(decoded.descriptors().size() == artifact.descriptors().size());
  require(decoded.descriptors()[0].id == artifact.descriptors()[0].id);
  require(decoded.statistics().persistent_bytes ==
          artifact.statistics().persistent_bytes);
  require(decoded.statistics().canonical_bytes ==
          artifact.statistics().canonical_bytes);
  require(decoded.statistics().ordering_certificate_count ==
          artifact.statistics().ordering_certificate_count);
  require(decoded.statistics().descriptor_count ==
          artifact.statistics().descriptor_count);
  require(verify_intersection_codec(decoded, limits, error));

  auto corrupted = artifact.canonical_bytes();
  require(corrupted.size() > 100);
  corrupted[80] ^= 0x01;
  auto unchanged = decoded;
  require(!decode_intersection_complex_private(
      corrupted, expected_header, limits, decoded, error));
  require(decoded.canonical_bytes() == unchanged.canonical_bytes());
  require(decoded.digest() == unchanged.digest());

  corrupted = artifact.canonical_bytes();
  corrupted.back() ^= 0x80;
  require(!decode_intersection_complex_private(
      corrupted, expected_header, limits, decoded, error));

  corrupted = artifact.canonical_bytes();
  corrupted.push_back(0);
  require(!decode_intersection_complex_private(
      corrupted, expected_header, limits, decoded, error));

  corrupted = artifact.canonical_bytes();
  corrupted.pop_back();
  require(!decode_intersection_complex_private(
      corrupted, expected_header, limits, decoded, error));

  corrupted = artifact.canonical_bytes();
  corrupted[4] ^= 0x01;
  require(!decode_intersection_complex_private(
      corrupted, expected_header, limits, decoded, error));

  auto wrong_header = expected_header;
  wrong_header.context_digest.bytes[0] ^= 0x01;
  require(!decode_intersection_complex_private(
      artifact.canonical_bytes(), wrong_header, limits, decoded, error));

  auto tiny_limits = limits;
  tiny_limits.maximum_bytes = artifact.canonical_bytes().size() - 1;
  require(!verify_intersection_codec(artifact, tiny_limits, error));
  require(!decode_intersection_complex_private(
      artifact.canonical_bytes(), expected_header, tiny_limits, decoded,
      error));

  auto malformed_limits = limits;
  malformed_limits.reserved32 = 1;
  const auto encoded_before_failure = artifact.canonical_bytes();
  require(!refresh_intersection_codec(artifact, malformed_limits, error));
  require(artifact.canonical_bytes() == encoded_before_failure);

  auto mutated = artifact;
  intersection_artifact_test_access::bytes(mutated)[80] ^= 0x01;
  require(!verify_intersection_codec(mutated, limits, error));

  mutated = artifact;
  intersection_artifact_test_access::digest(mutated).bytes[0] ^= 0x01;
  require(!verify_intersection_codec(mutated, limits, error));

  mutated = artifact;
  intersection_artifact_test_access::section_digest(mutated, 0).bytes[0] ^=
      0x01;
  require(!verify_intersection_codec(mutated, limits, error));

  mutated = artifact;
  ++intersection_artifact_test_access::statistics(mutated).canonical_bytes;
  require(!verify_intersection_codec(mutated, limits, error));

  const auto empty_header = header();
  auto empty = make_empty_artifact<float, std::uint32_t>(empty_header);
  require(empty.statistics().persistent_bytes == 0);
  require(refresh_intersection_codec(empty, limits, error));
  require(verify_intersection_codec(empty, limits, error));
  canonical_intersection_complex<float, std::uint32_t> empty_decoded;
  require(decode_intersection_complex_private(
      empty.canonical_bytes(), empty_header, limits, empty_decoded, error));
  require(empty_decoded.canonical_bytes() == empty.canonical_bytes());
}
