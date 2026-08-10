#pragma once

#include "IntersectionCanonicalization.h"

#include <array>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct intersection_codec_limits final {
  std::uint64_t maximum_bytes = (std::uint64_t{1} << 35);
  std::uint64_t maximum_records_per_table = (std::uint64_t{1} << 36);
  std::uint64_t maximum_index_entries = (std::uint64_t{1} << 38);
  std::uint64_t maximum_vector_allocations = (std::uint64_t{1} << 20);
  std::uint16_t version = contract_versions::intersection_codec;
  std::uint16_t reserved16 = 0;
  std::uint32_t reserved32 = 0;
};

// Re-encodes all immutable Component 08 tables into eight explicitly framed
// canonical sections, computes per-section and complete SHA-256 digests, and
// publishes bytes/digests/statistics transactionally.
template <class T, class I>
bool refresh_intersection_codec(
    canonical_intersection_complex<T, I> &artifact,
    const intersection_codec_limits &limits,
    bounded_boolean_error &error);

// Reconstructs every section from immutable records and checks the canonical
// bytes, section digests, complete digest, byte count, versions, reserved
// fields, IDs, references, ranges, and table statistics.
template <class T, class I>
bool verify_intersection_codec(
    const canonical_intersection_complex<T, I> &artifact,
    const intersection_codec_limits &limits,
    bounded_boolean_error &error);

// Private fail-closed decode. Owner identity is supplied out of band because a
// context_owner_token is intentionally process-local and is never serialized.
// The encoded header must match every other expectation field exactly. The
// destination is unchanged on any parse, digest, resource, or validation error.
template <class T, class I>
bool decode_intersection_complex_private(
    const std::vector<std::uint8_t> &bytes,
    const intersection_canonicalization_header &expectations,
    const intersection_codec_limits &limits,
    canonical_intersection_complex<T, I> &artifact,
    bounded_boolean_error &error);

#define YGOR_DECLARE_INTERSECTION_CODEC(T, I)                              \
  extern template bool refresh_intersection_codec<T, I>(                  \
      canonical_intersection_complex<T, I> &,                              \
      const intersection_codec_limits &, bounded_boolean_error &);         \
  extern template bool verify_intersection_codec<T, I>(                   \
      const canonical_intersection_complex<T, I> &,                        \
      const intersection_codec_limits &, bounded_boolean_error &);         \
  extern template bool decode_intersection_complex_private<T, I>(         \
      const std::vector<std::uint8_t> &,                                   \
      const intersection_canonicalization_header &,                        \
      const intersection_codec_limits &,                                   \
      canonical_intersection_complex<T, I> &, bounded_boolean_error &)

YGOR_DECLARE_INTERSECTION_CODEC(float, std::uint32_t);
YGOR_DECLARE_INTERSECTION_CODEC(float, std::uint64_t);
YGOR_DECLARE_INTERSECTION_CODEC(double, std::uint32_t);
YGOR_DECLARE_INTERSECTION_CODEC(double, std::uint64_t);

#undef YGOR_DECLARE_INTERSECTION_CODEC

} // namespace ygor::mesh_boolean::bounded
