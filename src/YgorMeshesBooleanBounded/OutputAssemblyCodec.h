#pragma once

#include "AssembledOutputCandidate.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct output_assembly_codec_access;

namespace output_assembly_codec_detail {

inline void encode_digest_bytes(canonical_writer &w,
                                const bounded_boolean_digest &digest) {
  for (const auto byte : digest.bytes)
    w.u8(byte);
}

template <class T>
inline void encode_range(canonical_writer &w, const std::vector<T> &values) {
  w.u64(values.size());
  for (const auto &value : values)
    w.u64(value);
}

} // namespace output_assembly_codec_detail

// Public mesh content domain (Section 17.2 layout).
template <class T, class I>
bool encode_public_mesh_content(const assembled_output_candidate<T, I> &candidate,
                                std::vector<std::uint8_t> &bytes,
                                bounded_boolean_error &error) {
  (void)error;
  using namespace output_assembly_codec_detail;
  canonical_writer w;
  const char magic[] = "YGOR_BOUNDED_OUTPUT_MESH_V1";
  for (const char *p = magic; *p; ++p)
    w.u8(static_cast<std::uint8_t>(*p));
  w.u16(contract_versions::output_assembly_codec);
  w.u16(contract_versions::output_assembly_ordering_schema);
  w.u8(sizeof(T));
  w.u8(sizeof(I));
  w.u8(1); // signed-zero policy: preserve source nominal
  w.u8(static_cast<std::uint8_t>(output_policy_kind::triangulated_oriented_manifold_v1));
  w.u64(candidate.components().size());
  w.u64(candidate.mesh().vertices.size());
  for (const auto &vertex : candidate.mesh().vertices)
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const T value = axis == 0 ? vertex.x : axis == 1 ? vertex.y : vertex.z;
      w.floating(value);
    }
  w.u64(candidate.mesh().faces.size());
  for (const auto &face : candidate.mesh().faces) {
    w.u64(face.size());
    for (const auto index : face)
      w.u64(static_cast<std::uint64_t>(index));
  }
  bytes = w.take();
  return true;
}

// Artifact domain: header, predecessor digests, public content reference,
// maps, reports, and pending statuses.
template <class T, class I>
bool encode_assembled_output_candidate(
    const assembled_output_candidate<T, I> &candidate,
    std::vector<std::uint8_t> &bytes, output_assembly_codec_limits limits,
    bounded_boolean_error &error) {
  using namespace output_assembly_codec_detail;
  canonical_writer w;
  w.u32(0x41534d31); // "ASM1"
  w.u16(candidate.schema_version());
  w.u16(candidate.provider_version());
  w.u16(candidate.codec_version());
  w.u16(candidate.verifier_version());
  w.u8(static_cast<std::uint8_t>(candidate.provider()));
  w.u8(static_cast<std::uint8_t>(candidate.verification()));
  w.u8(static_cast<std::uint8_t>(candidate.operation()));
  w.u8(static_cast<std::uint8_t>(candidate.topology_status()));
  w.u8(static_cast<std::uint8_t>(candidate.geometry_status()));
  encode_digest_bytes(w, candidate.context_digest());
  encode_digest_bytes(w, candidate.precision_digest());
  encode_digest_bytes(w, candidate.cleaned_digest());

  // Public content length and digest.
  w.u64(candidate.public_content_bytes().size());
  for (const auto byte : candidate.public_content_bytes())
    w.u8(byte);

  // Components.
  w.u64(candidate.components().size());
  for (const auto &record : candidate.components()) {
    w.u64(record.canonical_id);
    w.u64(record.vertex_begin);
    w.u64(record.vertex_count);
    w.u64(record.facet_begin);
    w.u64(record.facet_count);
    w.u64(record.edge_count);
    w.u64(static_cast<std::uint64_t>(record.euler_chi));
  }

  // Coordinate copies.
  w.u64(candidate.coordinate_copies().size());
  for (const auto &record : candidate.coordinate_copies()) {
    w.u64(record.canonical_id);
    w.u64(record.public_vertex);
    w.u64(record.cleaned_occurrence);
    for (const auto bit : record.output_bits)
      w.u64(bit);
    w.u8(static_cast<std::uint8_t>(record.disposition));
  }

  // Report.
  w.u64(candidate.report().vertex_count);
  w.u64(candidate.report().edge_count);
  w.u64(candidate.report().facet_count);
  w.u64(candidate.report().component_count);
  w.u64(candidate.report().output_precision_bits);
  w.u64(candidate.report().maximum_authorized_tolerance_bits);

  // Maps.
  encode_range(w, candidate.vertex_by_occurrence());
  encode_range(w, candidate.occurrence_by_vertex());
  encode_range(w, candidate.facet_by_triangle());
  encode_range(w, candidate.triangle_by_facet());

  // Statistics.
  w.u64(candidate.statistics().component_count);
  w.u64(candidate.statistics().vertex_count);
  w.u64(candidate.statistics().facet_count);
  w.u64(candidate.statistics().graph_node_count);
  w.u64(candidate.statistics().refinement_rounds);
  w.u64(candidate.statistics().individualization_branches);
  w.u64(candidate.statistics().verifier_work_units);

  bytes = w.take();
  if (bytes.size() > limits.maximum_section_bytes) {
    error = output_assembly_error(
        output_assembly_subcode::codec_error,
        bounded_boolean_error_category::index_overflow,
        "assembly canonical bytes exceed the configured limit",
        output_assembly_checkpoint::canonical_encoding);
    return false;
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
