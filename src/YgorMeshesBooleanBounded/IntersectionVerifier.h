#pragma once

#include "IntersectionCodec.h"
#include "SignedFeatureRelations.h"

#include <cstdint>

namespace ygor::mesh_boolean::bounded {

struct intersection_verifier_limits final {
  std::uint64_t maximum_work_units = (std::uint64_t{1} << 40);
  std::uint64_t maximum_pair_checks = (std::uint64_t{1} << 36);
  std::uint16_t version = contract_versions::intersection_verifier;
  bool exhaustive_test_only = false;
  std::uint8_t reserved8 = 0;
  std::uint32_t reserved32 = 0;
};

// Independently reconstructs the canonical event/occurrence partition and all
// retained reverse mappings from the immutable Component 07 predecessor, then
// audits the published source-edge, carrier, overlap, aggregate, descriptor,
// topology, statistics, codec, and digest state with traversal/control flow
// separate from the Component 08 producers. The artifact is never modified.
template <class T, class I>
bool verify_intersection_complex_independent(
    const signed_feature_relations<T, I> &relations,
    const canonical_intersection_complex<T, I> &artifact,
    const intersection_codec_limits &codec_limits,
    const intersection_verifier_limits &limits,
    intersection_verification_evidence &evidence,
    bounded_boolean_error &error);

// Transactionally publishes independently-verified evidence. The destination
// is unchanged on any reconstruction, resource, codec, or final self-check
// failure. Repeated finalization is idempotent and byte-stable.
template <class T, class I>
bool finalize_intersection_complex_verification(
    const signed_feature_relations<T, I> &relations,
    canonical_intersection_complex<T, I> &artifact,
    const intersection_codec_limits &codec_limits,
    const intersection_verifier_limits &limits,
    bounded_boolean_error &error);

// Decodes into private storage, then runs the same independent reconstruction
// used for producer publication before exposing the result. Encoded artifacts
// that have not yet published verifier evidence are finalized transactionally;
// already-verified artifacts must reproduce their stored evidence exactly.
template <class T, class I>
bool decode_intersection_complex_verified_private(
    const std::vector<std::uint8_t> &bytes,
    const intersection_canonicalization_header &expectations,
    const signed_feature_relations<T, I> &relations,
    const intersection_codec_limits &codec_limits,
    const intersection_verifier_limits &verifier_limits,
    canonical_intersection_complex<T, I> &artifact,
    bounded_boolean_error &error);

#define YGOR_DECLARE_INTERSECTION_VERIFIER(T, I)                            \
  extern template bool verify_intersection_complex_independent<T, I>(       \
      const signed_feature_relations<T, I> &,                               \
      const canonical_intersection_complex<T, I> &,                         \
      const intersection_codec_limits &,                                    \
      const intersection_verifier_limits &,                                 \
      intersection_verification_evidence &, bounded_boolean_error &);       \
  extern template bool finalize_intersection_complex_verification<T, I>(    \
      const signed_feature_relations<T, I> &,                               \
      canonical_intersection_complex<T, I> &,                               \
      const intersection_codec_limits &,                                    \
      const intersection_verifier_limits &, bounded_boolean_error &);       \
  extern template bool decode_intersection_complex_verified_private<T, I>( \
      const std::vector<std::uint8_t> &,                                    \
      const intersection_canonicalization_header &,                         \
      const signed_feature_relations<T, I> &,                               \
      const intersection_codec_limits &,                                    \
      const intersection_verifier_limits &,                                \
      canonical_intersection_complex<T, I> &, bounded_boolean_error &)

YGOR_DECLARE_INTERSECTION_VERIFIER(float, std::uint32_t);
YGOR_DECLARE_INTERSECTION_VERIFIER(float, std::uint64_t);
YGOR_DECLARE_INTERSECTION_VERIFIER(double, std::uint32_t);
YGOR_DECLARE_INTERSECTION_VERIFIER(double, std::uint64_t);

#undef YGOR_DECLARE_INTERSECTION_VERIFIER

} // namespace ygor::mesh_boolean::bounded
