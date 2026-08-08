#pragma once

#include "EventCoordinates.h"
#include "IntersectionDescriptors.h"

#include <array>
#include <cstdint>

namespace ygor::mesh_boolean::bounded {

struct intersection_canonicalization_header final {
  context_owner_token owner{};
  boolean_operation operation = boolean_operation::set_union;
  bounded_boolean_digest context_digest{};
  bounded_boolean_digest precision_digest{};
  bounded_boolean_digest relation_digest{};
  std::array<bounded_boolean_digest, 2> source_semantic_digests{};
  std::array<bounded_boolean_digest, 2> exact_triangulation_digests{};
  std::uint16_t schema_version =
      contract_versions::intersection_artifact_schema;
  std::uint16_t provider_version = contract_versions::intersection_provider;
  std::uint16_t semantic_policy_version =
      contract_versions::intersection_semantic_policy;
  std::uint16_t codec_version = contract_versions::intersection_codec;
  std::uint16_t verifier_version = contract_versions::intersection_verifier;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

// Copies all already-canonical Component 08 provider tables into one private
// immutable artifact layout. This phase preserves every table-owned CSR/index
// array, assigns the one shared ordering-certificate ID domain, computes exact
// table statistics, and publishes only after the complete copy validates.
// Encoding, section digests, replay, and final independent verification remain
// later transactional phases.
template <class T, class I>
bool canonicalize_intersection_tables(
    const intersection_canonicalization_header &header,
    const event_interning_tables &interning,
    const event_coordinate_tables &coordinates,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    const intersection_descriptor_tables &descriptors,
    canonical_intersection_complex<T, I> &artifact,
    bounded_boolean_error &error);

// Reconstructs the complete table-owned artifact projection independently and
// checks every record, index vector, certificate remap, statistic, version,
// owner, and zero/unverified publication field.
template <class T, class I>
bool verify_intersection_canonicalization(
    const intersection_canonicalization_header &header,
    const event_interning_tables &interning,
    const event_coordinate_tables &coordinates,
    const event_incidence_tables &incidence,
    const source_edge_arrangement_tables &source_edges,
    const transverse_carrier_arrangement_tables &transverse,
    const coplanar_carrier_arrangement_tables &coplanar,
    const intersection_aggregate_tables &aggregates,
    const intersection_descriptor_tables &descriptors,
    const canonical_intersection_complex<T, I> &artifact,
    bounded_boolean_error &error);

#define YGOR_DECLARE_INTERSECTION_CANONICALIZATION(T, I)                    \
  extern template bool canonicalize_intersection_tables<T, I>(             \
      const intersection_canonicalization_header &,                         \
      const event_interning_tables &, const event_coordinate_tables &,      \
      const event_incidence_tables &,                                       \
      const source_edge_arrangement_tables &,                               \
      const transverse_carrier_arrangement_tables &,                        \
      const coplanar_carrier_arrangement_tables &,                          \
      const intersection_aggregate_tables &,                                \
      const intersection_descriptor_tables &,                              \
      canonical_intersection_complex<T, I> &, bounded_boolean_error &);     \
  extern template bool verify_intersection_canonicalization<T, I>(         \
      const intersection_canonicalization_header &,                         \
      const event_interning_tables &, const event_coordinate_tables &,      \
      const event_incidence_tables &,                                       \
      const source_edge_arrangement_tables &,                               \
      const transverse_carrier_arrangement_tables &,                        \
      const coplanar_carrier_arrangement_tables &,                          \
      const intersection_aggregate_tables &,                                \
      const intersection_descriptor_tables &,                              \
      const canonical_intersection_complex<T, I> &, bounded_boolean_error &)

YGOR_DECLARE_INTERSECTION_CANONICALIZATION(float, std::uint32_t);
YGOR_DECLARE_INTERSECTION_CANONICALIZATION(float, std::uint64_t);
YGOR_DECLARE_INTERSECTION_CANONICALIZATION(double, std::uint32_t);
YGOR_DECLARE_INTERSECTION_CANONICALIZATION(double, std::uint64_t);

#undef YGOR_DECLARE_INTERSECTION_CANONICALIZATION

} // namespace ygor::mesh_boolean::bounded
