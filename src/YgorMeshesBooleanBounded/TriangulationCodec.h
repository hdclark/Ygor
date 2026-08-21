#pragma once

#include "TriangulatedOutputComplex.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct triangulation_codec_access;

namespace triangulation_codec_detail {

inline void encode_digest_bytes(canonical_writer &w,
                                const bounded_boolean_digest &digest) {
  for (const auto byte : digest.bytes)
    w.u8(byte);
}

inline void encode_region_key(canonical_writer &w,
                              const triangulation_region_key &key) {
  w.u64(key.component11_region);
  w.u16(key.schema_version);
}

inline void encode_projected_key(canonical_writer &w,
                                 const projected_occurrence_key &key) {
  w.u64(key.region);
  w.u64(key.component11_occurrence);
  w.u16(key.schema_version);
}

inline void encode_diagonal_key(canonical_writer &w,
                                const internal_diagonal_key &key) {
  w.u64(key.region);
  w.u64(key.endpoints[0]);
  w.u64(key.endpoints[1]);
  w.u16(key.schema_version);
}

inline void encode_triangle_key(canonical_writer &w,
                                const output_triangle_key &key) {
  w.u64(key.region);
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

} // namespace triangulation_codec_detail

template <class T, class I>
bool encode_triangulated_output_complex(
    const triangulated_output_complex<T, I> &artifact,
    std::vector<std::uint8_t> &bytes, output_triangulation_codec_limits limits,
    bounded_boolean_error &error) {
  using namespace triangulation_codec_detail;
  canonical_writer w;

  // Section 0: magic / header / owner / operation / predecessor digests.
  w.u32(0x4f545231); // "OTR1"
  w.u16(artifact.schema_version());
  w.u16(artifact.provider_version());
  w.u16(artifact.codec_version());
  w.u16(artifact.verifier_version());
  w.u8(static_cast<std::uint8_t>(artifact.provider()));
  w.u8(static_cast<std::uint8_t>(artifact.verification()));
  w.u8(static_cast<std::uint8_t>(artifact.operation()));
  encode_digest_bytes(w, artifact.context_digest());
  encode_digest_bytes(w, artifact.precision_digest());
  encode_digest_bytes(w, artifact.manifold_digests()[0]);
  encode_digest_bytes(w, artifact.manifold_digests()[1]);
  encode_digest_bytes(w, artifact.intersection_digest());
  encode_digest_bytes(w, artifact.retained_digest());
  encode_digest_bytes(w, artifact.polygonal_digest());

  // Section 1: support frames and projected occurrences.
  w.u64(artifact.support_frames().size());
  for (const auto &record : artifact.support_frames()) {
    w.u64(record.canonical_id);
    w.u64(record.region);
    w.u8(static_cast<std::uint8_t>(record.disposition));
    w.u8(static_cast<std::uint8_t>(record.source_operand));
    w.u64(record.source_facet);
    w.u8(record.dropped_axis);
  }
  w.u64(artifact.projected_occurrences().size());
  for (const auto &record : artifact.projected_occurrences()) {
    w.u64(record.canonical_id);
    encode_projected_key(w, record.key);
    w.u64(record.region);
    w.u64(record.component11_occurrence);
    w.u64(record.nominal_bits[0]);
    w.u64(record.nominal_bits[1]);
    w.u64(record.lower_bits[0]);
    w.u64(record.lower_bits[1]);
    w.u64(record.upper_bits[0]);
    w.u64(record.upper_bits[1]);
  }

  // Section 2: predicate evidence and escalation traces.
  w.u64(artifact.predicate_evidence().size());
  for (const auto &record : artifact.predicate_evidence()) {
    w.u64(record.canonical_id);
    w.u8(static_cast<std::uint8_t>(record.kind));
    w.u64(record.region);
    w.u64(record.operands[0]);
    w.u64(record.operands[1]);
    w.u64(record.operands[2]);
    w.u64(record.operands[3]);
    w.u8(record.operand_count);
    w.u8(static_cast<std::uint8_t>(record.exact_sign));
    w.u64(record.enclosure_lower_bits[0]);
    w.u64(record.enclosure_lower_bits[1]);
    w.u64(record.enclosure_upper_bits[0]);
    w.u64(record.enclosure_upper_bits[1]);
    w.u8(static_cast<std::uint8_t>(record.disposition));
  }
  w.u64(artifact.escalations().size());
  for (const auto &record : artifact.escalations()) {
    w.u64(record.canonical_id);
    w.u64(record.region);
    w.u64(record.corner_occurrence);
    w.u8(record.steps);
    w.u8(static_cast<std::uint8_t>(record.terminal));
  }

  // Section 3: internal diagonals and internal halfedges.
  w.u64(artifact.diagonals().size());
  for (const auto &record : artifact.diagonals()) {
    w.u64(record.canonical_id);
    encode_diagonal_key(w, record.key);
    w.u64(record.region);
    w.u64(record.halfedges[0]);
    w.u64(record.halfedges[1]);
    w.u64(record.endpoints[0]);
    w.u64(record.endpoints[1]);
    w.u8(static_cast<std::uint8_t>(record.state));
  }
  w.u64(artifact.internal_halfedges().size());
  for (const auto &record : artifact.internal_halfedges()) {
    w.u64(record.canonical_id);
    w.u64(record.pair);
    w.u64(record.origin);
    w.u64(record.destination);
    w.u64(record.region);
    w.u64(record.diagonal);
  }

  // Section 4: triangles, corner refs, edge-use refs.
  w.u64(artifact.triangles().size());
  for (const auto &record : artifact.triangles()) {
    w.u64(record.canonical_id);
    encode_triangle_key(w, record.key);
    w.u64(record.region);
    for (const auto corner : record.corners)
      w.u64(corner);
    for (const auto ref : record.corner_refs)
      w.u64(ref);
    for (const auto ref : record.edge_use_refs)
      w.u64(ref);
    w.u8(static_cast<std::uint8_t>(record.category));
  }
  w.u64(artifact.corner_refs().size());
  for (const auto &record : artifact.corner_refs()) {
    w.u64(record.canonical_id);
    w.u64(record.triangle);
    w.u64(record.occurrence);
  }
  w.u64(artifact.edge_use_refs().size());
  for (const auto &record : artifact.edge_use_refs()) {
    w.u64(record.canonical_id);
    w.u64(record.triangle);
    w.boolean(record.is_boundary);
    w.u64(record.halfedge);
  }

  // Section 5: boundary assignments and cleanup certificates.
  w.u64(artifact.boundary_assignments().size());
  for (const auto &record : artifact.boundary_assignments()) {
    w.u64(record.canonical_id);
    w.u64(record.component11_halfedge);
    w.u64(record.triangle);
    w.u8(static_cast<std::uint8_t>(record.kind));
  }
  w.u64(artifact.cleanup_certificates().size());
  for (const auto &record : artifact.cleanup_certificates()) {
    w.u64(record.canonical_id);
    w.u64(record.triangle);
    w.u8(static_cast<std::uint8_t>(record.metric));
    w.u8(static_cast<std::uint8_t>(record.dimension));
    w.u64(record.length_lower_bits[0]);
    w.u64(record.length_lower_bits[1]);
    w.u64(record.length_upper_bits[0]);
    w.u64(record.length_upper_bits[1]);
    w.boolean(record.budget_reserved);
    w.boolean(record.budget_committed);
  }

  // Section 6: coverage certificates.
  w.u64(artifact.coverage_certificates().size());
  for (const auto &record : artifact.coverage_certificates()) {
    w.u64(record.canonical_id);
    w.u64(record.region);
    w.u8(static_cast<std::uint8_t>(record.disposition));
    w.u64(record.vertex_count);
    w.u64(record.edge_count);
    w.u64(record.face_count);
    w.u64(record.hole_count);
    w.u64(record.triangle_begin);
    w.u64(record.triangle_count);
    w.u64(record.diagonal_begin);
    w.u64(record.diagonal_count);
  }

  // Section 7: reverse maps.
  encode_range(w, artifact.boundary_assignment_by_halfedge());
  encode_range(w, artifact.region_triangle_index());
  encode_range(w, artifact.region_diagonal_index());

  // Section 8: statistics.
  w.u64(artifact.statistics().region_count);
  w.u64(artifact.statistics().support_frame_count);
  w.u64(artifact.statistics().projected_occurrence_count);
  w.u64(artifact.statistics().predicate_evidence_count);
  w.u64(artifact.statistics().internal_diagonal_count);
  w.u64(artifact.statistics().internal_halfedge_count);
  w.u64(artifact.statistics().triangle_count);
  w.u64(artifact.statistics().corner_ref_count);
  w.u64(artifact.statistics().edge_use_ref_count);
  w.u64(artifact.statistics().residual_cell_count);
  w.u64(artifact.statistics().cleanup_obligation_count);
  w.u64(artifact.statistics().boundary_assignment_count);
  w.u64(artifact.statistics().coverage_certificate_count);
  w.u64(artifact.statistics().verifier_work_units);

  bytes = w.take();
  if (bytes.size() > limits.maximum_section_bytes) {
    error = output_triangulation_error(
        output_triangulation_subcode::codec_error,
        bounded_boolean_error_category::index_overflow,
        "triangulation canonical bytes exceed the configured limit",
        output_triangulation_checkpoint::canonical_encoding);
    return false;
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
