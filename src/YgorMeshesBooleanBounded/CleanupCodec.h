#pragma once

#include "CleanedTriangleManifold.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct cleanup_codec_access;

namespace cleanup_codec_detail {

inline void encode_digest_bytes(canonical_writer &w,
                                const bounded_boolean_digest &digest) {
  for (const auto byte : digest.bytes)
    w.u8(byte);
}

inline void encode_vertex_key(canonical_writer &w,
                              const cleaned_vertex_key &key) {
  w.u64(key.component11_occurrence);
  w.u16(key.schema_version);
}

inline void encode_edge_key(canonical_writer &w, const cleaned_edge_key &key) {
  w.u64(key.endpoints[0]);
  w.u64(key.endpoints[1]);
  w.u16(key.schema_version);
}

inline void encode_triangle_key(canonical_writer &w,
                                const cleaned_triangle_key &key) {
  w.u64(key.corners[0]);
  w.u64(key.corners[1]);
  w.u64(key.corners[2]);
  w.u16(key.schema_version);
}

template <class T>
inline void encode_range(canonical_writer &w, const std::vector<T> &values) {
  w.u64(values.size());
  for (const auto &value : values)
    w.u64(value);
}

} // namespace cleanup_codec_detail

template <class T>
bool encode_cleaned_triangle_manifold(
    const cleaned_triangle_manifold<T> &artifact, std::vector<std::uint8_t> &bytes,
    cleanup_codec_limits limits, bounded_boolean_error &error) {
  using namespace cleanup_codec_detail;
  canonical_writer w;

  // Section 0: magic / header / owner / operation / predecessor digests.
  w.u32(0x434c4e31); // "CLN1"
  w.u16(artifact.schema_version());
  w.u16(artifact.provider_version());
  w.u16(artifact.codec_version());
  w.u16(artifact.verifier_version());
  w.u8(static_cast<std::uint8_t>(artifact.provider()));
  w.u8(static_cast<std::uint8_t>(artifact.verification()));
  w.u8(static_cast<std::uint8_t>(artifact.operation()));
  w.u8(static_cast<std::uint8_t>(artifact.status()));
  encode_digest_bytes(w, artifact.context_digest());
  encode_digest_bytes(w, artifact.precision_digest());
  encode_digest_bytes(w, artifact.polygonal_digest());
  encode_digest_bytes(w, artifact.triangulated_digest());

  // Section 1: vertices.
  w.u64(artifact.vertices().size());
  for (const auto &record : artifact.vertices()) {
    w.u64(record.canonical_id);
    encode_vertex_key(w, record.key);
    w.u64(record.component11_occurrence);
    for (const auto value : record.nominal_bits)
      w.u64(value);
    for (const auto value : record.lower_bits)
      w.u64(value);
    for (const auto value : record.upper_bits)
      w.u64(value);
    w.u64(record.radial_error_bits);
    w.u64(record.link_begin);
    w.u64(record.link_count);
    w.u64(record.component);
  }

  // Section 2: paired edges and halfedges.
  w.u64(artifact.paired_edges().size());
  for (const auto &record : artifact.paired_edges()) {
    w.u64(record.canonical_id);
    encode_edge_key(w, record.key);
    w.u64(record.halfedges[0]);
    w.u64(record.halfedges[1]);
    w.u64(record.endpoints[0]);
    w.u64(record.endpoints[1]);
    w.u64(record.triangles[0]);
    w.u64(record.triangles[1]);
  }
  w.u64(artifact.halfedges().size());
  for (const auto &record : artifact.halfedges()) {
    w.u64(record.canonical_id);
    w.u64(record.pair);
    w.u64(record.origin);
    w.u64(record.destination);
    w.u64(record.edge);
    w.u64(record.triangle);
    w.u64(record.next);
  }

  // Section 3: triangles and components.
  w.u64(artifact.triangles().size());
  for (const auto &record : artifact.triangles()) {
    w.u64(record.canonical_id);
    encode_triangle_key(w, record.key);
    w.u64(record.corners[0]);
    w.u64(record.corners[1]);
    w.u64(record.corners[2]);
    w.u64(record.halfedges[0]);
    w.u64(record.halfedges[1]);
    w.u64(record.halfedges[2]);
    w.u64(record.component);
    w.u8(static_cast<std::uint8_t>(record.category));
  }
  w.u64(artifact.components().size());
  for (const auto &record : artifact.components()) {
    w.u64(record.canonical_id);
    w.u64(record.triangles_begin);
    w.u64(record.triangles_count);
    w.u64(record.vertex_count);
    w.u64(record.edge_count);
    w.u64(record.face_count);
    w.u64(static_cast<std::uint64_t>(record.euler_chi));
  }

  // Section 4: obligation dispositions and action log.
  w.u64(artifact.obligation_dispositions().size());
  for (const auto &record : artifact.obligation_dispositions()) {
    w.u64(record.canonical_id);
    w.u8(static_cast<std::uint8_t>(record.obligation_class));
    w.u64(record.source);
    w.u8(static_cast<std::uint8_t>(record.disposition));
  }
  w.u64(artifact.actions().size());
  for (const auto &record : artifact.actions()) {
    w.u64(record.canonical_id);
    w.u8(record.action_class);
    w.u8(record.motion_class);
    w.u64(record.certificate);
  }

  // Section 5: reverse maps.
  encode_range(w, artifact.vertex_by_occurrence());
  encode_range(w, artifact.triangle_by_source());
  encode_range(w, artifact.link_index());
  encode_range(w, artifact.component_triangle_index());

  // Section 6: statistics.
  w.u64(artifact.statistics().vertex_count);
  w.u64(artifact.statistics().paired_edge_count);
  w.u64(artifact.statistics().halfedge_count);
  w.u64(artifact.statistics().triangle_count);
  w.u64(artifact.statistics().component_count);
  w.u64(artifact.statistics().obligation_count);
  w.u64(artifact.statistics().action_count);
  w.u64(artifact.statistics().verifier_work_units);

  bytes = w.take();
  if (bytes.size() > limits.maximum_section_bytes) {
    error = cleanup_error(cleanup_subcode::codec_error,
                          bounded_boolean_error_category::index_overflow,
                          "cleanup canonical bytes exceed the configured limit",
                          cleanup_checkpoint::canonical_encoding);
    return false;
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
