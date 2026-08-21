#pragma once

#include "PolygonalOutputComplex.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct output_topology_codec_access;

namespace output_topology_codec_detail {

inline void encode_digest_bytes(canonical_writer &w,
                                const bounded_boolean_digest &digest) {
  for (const auto byte : digest.bytes)
    w.u8(byte);
}

inline void encode_vertex_key(canonical_writer &w,
                              const output_vertex_key &key) {
  w.u64(key.endpoint_domain);
  w.u64(key.predecessor_occurrence);
  w.u16(key.schema_version);
}

inline void encode_region_key(canonical_writer &w,
                              const output_face_region_key &key) {
  w.u8(static_cast<std::uint8_t>(key.source_operand));
  w.u64(key.retained_use);
  w.u64(key.source_facet);
  w.u64(key.support_lineage);
  w.u64(key.positive_area_component);
  w.u16(key.schema_version);
}

inline void encode_paired_edge_key(canonical_writer &w,
                                   const paired_edge_key &key) {
  w.u64(key.planned_edge);
  w.u64(key.endpoint_domains[0]);
  w.u64(key.endpoint_domains[1]);
  w.u64(key.zero_measure_lineage);
  w.u16(key.schema_version);
}

inline void encode_halfedge_key(canonical_writer &w,
                                const output_halfedge_key &key) {
  w.u64(key.paired_edge);
  w.u64(key.incidence);
  w.u8(static_cast<std::uint8_t>(key.direction));
  w.u16(key.schema_version);
}

inline void encode_cycle_key(canonical_writer &w, const face_cycle_key &key) {
  w.u64(key.region);
  w.u64(key.rotated_halfedges.size());
  for (const auto &halfedge : key.rotated_halfedges)
    encode_halfedge_key(w, halfedge);
  w.u16(key.schema_version);
}

inline void encode_contour_key(canonical_writer &w, const contour_key &key) {
  w.u64(key.region);
  w.u64(key.cycle);
  w.u8(static_cast<std::uint8_t>(key.role));
  w.u16(key.schema_version);
}

inline void encode_witness_key(canonical_writer &w,
                               const contour_witness_key &key) {
  w.u64(key.region);
  w.u64(key.cycle);
  w.u8(static_cast<std::uint8_t>(key.source));
  w.u64(key.atom);
  w.u16(key.schema_version);
}

template <class T>
inline void encode_range(canonical_writer &w, const std::vector<T> &values) {
  w.u64(values.size());
  for (const auto &value : values)
    w.u64(value);
}

} // namespace output_topology_codec_detail

template <class T, class I>
bool encode_polygonal_output_complex(
    const polygonal_output_complex<T, I> &artifact,
    std::vector<std::uint8_t> &bytes, output_topology_codec_limits limits,
    bounded_boolean_error &error) {
  using namespace output_topology_codec_detail;
  canonical_writer w;

  // Section 0: magic / header / owner / operation / predecessor digests.
  w.u32(0x4f545031); // "OTP1"
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
  encode_digest_bytes(w, artifact.relation_digest());
  encode_digest_bytes(w, artifact.intersection_digest());
  encode_digest_bytes(w, artifact.classification_digest());
  encode_digest_bytes(w, artifact.retained_digest());

  // Section 1: incidence audits and zero-measure support evidence.
  w.u64(artifact.incidence_audits().size());
  for (const auto &record : artifact.incidence_audits()) {
    w.u64(record.canonical_id);
    w.u64(record.retained_incidence);
    w.u64(record.retained_use);
    w.u64(record.start_domain);
    w.u64(record.end_domain);
    w.u8(static_cast<std::uint8_t>(record.disposition));
    w.u64(record.planned_edge);
    w.u64(record.continuation);
    w.u64(record.consuming_output_entity);
  }
  w.u64(artifact.zero_measure_supports().size());
  for (const auto &record : artifact.zero_measure_supports()) {
    w.u64(record.canonical_id);
    w.u64(record.retained_use);
    w.u64(record.attachment_lineage);
    for (const auto value : record.nominal_bits)
      w.u64(value);
    for (const auto value : record.lower_bits)
      w.u64(value);
    for (const auto value : record.upper_bits)
      w.u64(value);
    w.boolean(record.no_topology_proof);
  }

  // Section 2: coordinate references and vertex occurrences.
  w.u64(artifact.coordinate_references().size());
  for (const auto &record : artifact.coordinate_references()) {
    w.u64(record.canonical_id);
    w.u8(static_cast<std::uint8_t>(record.source));
    w.u8(static_cast<std::uint8_t>(record.source_operand));
    w.u64(record.source_lineage);
    w.u64(record.construction_ledger);
    for (const auto value : record.nominal_bits)
      w.u64(value);
    for (const auto value : record.lower_bits)
      w.u64(value);
    for (const auto value : record.upper_bits)
      w.u64(value);
    w.u64(record.radial_error_bits);
  }
  w.u64(artifact.vertex_occurrences().size());
  for (const auto &record : artifact.vertex_occurrences()) {
    w.u64(record.canonical_id);
    encode_vertex_key(w, record.key);
    w.u64(record.endpoint_domain);
    w.u64(record.coordinate_reference);
    w.u64(record.ports_begin);
    w.u64(record.ports_count);
    w.u64(record.representative_port);
    w.u64(record.predecessor_occurrence);
  }

  // Section 3: regions, members, continuations, and boundary darts.
  w.u64(artifact.face_regions().size());
  for (const auto &record : artifact.face_regions()) {
    w.u64(record.canonical_id);
    encode_region_key(w, record.key);
    w.u64(record.members_begin);
    w.u64(record.members_count);
    w.u64(record.boundary_darts_begin);
    w.u64(record.boundary_darts_count);
    w.u64(record.continuation_members_begin);
    w.u64(record.continuation_members_count);
    w.boolean(record.positive_area);
    w.u64(record.outer_contour);
  }
  w.u64(artifact.region_members().size());
  for (const auto &record : artifact.region_members()) {
    w.u64(record.canonical_id);
    w.u64(record.retained_use);
  }
  w.u64(artifact.continuation_consumptions().size());
  for (const auto &record : artifact.continuation_consumptions()) {
    w.u64(record.canonical_id);
    w.u64(record.continuation);
    w.u64(record.forward_region);
    w.u64(record.reverse_region);
  }
  w.u64(artifact.boundary_darts().size());
  for (const auto &record : artifact.boundary_darts()) {
    w.u64(record.canonical_id);
    w.u64(record.incidence);
    w.u64(record.halfedge);
    w.u64(record.face_next);
    w.u64(record.face_prev);
    w.u64(record.region);
  }

  // Section 4: paired edges and halfedges.
  w.u64(artifact.paired_edges().size());
  for (const auto &record : artifact.paired_edges()) {
    w.u64(record.canonical_id);
    encode_paired_edge_key(w, record.key);
    w.u64(record.planned_edge);
    w.u64(record.halfedge0);
    w.u64(record.halfedge1);
    w.u8(static_cast<std::uint8_t>(record.role));
    w.u8(static_cast<std::uint8_t>(record.endpoint_relation));
    w.u64(record.zero_measure_boundary);
  }
  w.u64(artifact.halfedges().size());
  for (const auto &record : artifact.halfedges()) {
    w.u64(record.canonical_id);
    encode_halfedge_key(w, record.key);
    w.u64(record.pair);
    w.u64(record.origin);
    w.u64(record.destination);
    w.u64(record.successor);
    w.u64(record.predecessor);
    w.u64(record.region);
    w.u64(record.cycle);
    w.u64(record.incidence);
    w.u64(record.paired_edge);
    w.u8(static_cast<std::uint8_t>(record.direction));
    w.u64(record.endpoint_fan_begin);
    w.u64(record.endpoint_fan_count);
  }
  w.u64(artifact.endpoint_fan_refs().size());
  for (const auto &record : artifact.endpoint_fan_refs()) {
    w.u64(record.canonical_id);
    w.u64(record.port);
    w.u8(static_cast<std::uint8_t>(record.role));
  }

  // Section 5: cycles, contour nodes, and witnesses.
  w.u64(artifact.face_cycles().size());
  for (const auto &record : artifact.face_cycles()) {
    w.u64(record.canonical_id);
    encode_cycle_key(w, record.key);
    w.u64(record.region);
    w.u64(record.refs_begin);
    w.u64(record.refs_count);
    w.u64(record.start_halfedge);
    w.u8(static_cast<std::uint8_t>(record.category));
  }
  w.u64(artifact.cycle_halfedge_refs().size());
  for (const auto &record : artifact.cycle_halfedge_refs()) {
    w.u64(record.canonical_id);
    w.u64(record.halfedge);
  }
  w.u64(artifact.contour_nodes().size());
  for (const auto &record : artifact.contour_nodes()) {
    w.u64(record.canonical_id);
    encode_contour_key(w, record.key);
    w.u64(record.cycle);
    w.u64(record.region);
    w.u8(static_cast<std::uint8_t>(record.role));
    w.u64(record.witness);
  }
  w.u64(artifact.contour_witnesses().size());
  for (const auto &record : artifact.contour_witnesses()) {
    w.u64(record.canonical_id);
    encode_witness_key(w, record.key);
    w.u64(record.region);
    w.u64(record.cycle);
    w.u64(record.atom);
    for (const auto value : record.nominal_bits)
      w.u64(value);
    for (const auto value : record.enclosure_bits)
      w.u64(value);
    w.boolean(record.strict_side);
  }

  // Section 6: zero-measure boundaries and admissibility evidence.
  w.u64(artifact.zero_measure_boundaries().size());
  for (const auto &record : artifact.zero_measure_boundaries()) {
    w.u64(record.canonical_id);
    w.u64(record.paired_edge);
    for (const auto value : record.nominal_bits)
      w.u64(value);
    for (const auto value : record.lower_bits)
      w.u64(value);
    for (const auto value : record.upper_bits)
      w.u64(value);
    w.u64(record.lower_length_bits);
    w.u64(record.upper_length_bits);
  }
  w.u64(artifact.admissibility_evidence().size());
  for (const auto &record : artifact.admissibility_evidence()) {
    w.u64(record.canonical_id);
    w.u8(static_cast<std::uint8_t>(record.disposition));
    w.u64(record.subject_a);
    w.u64(record.subject_b);
    w.u64(record.lineage);
  }

  // Section 7: carrier balance audits and vertex-link evidence.
  w.u64(artifact.carrier_balance_audits().size());
  for (const auto &record : artifact.carrier_balance_audits()) {
    w.u64(record.canonical_id);
    w.u64(record.carrier_lineage);
    w.u64(record.members_begin);
    w.u64(record.members_count);
    w.boolean(record.balanced);
  }
  w.u64(artifact.vertex_link_evidence().size());
  for (const auto &record : artifact.vertex_link_evidence()) {
    w.u64(record.canonical_id);
    w.u64(record.vertex_occurrence);
    w.u64(record.outgoing_begin);
    w.u64(record.outgoing_count);
  }

  // Section 8: reverse maps and member ranges.
  encode_range(w, artifact.audit_by_incidence());
  encode_range(w, artifact.halfedge_by_incidence());
  encode_range(w, artifact.occurrence_by_endpoint_domain());
  encode_range(w, artifact.region_by_retained_use());
  encode_range(w, artifact.region_member_index());
  encode_range(w, artifact.cycle_halfedge_index());
  encode_range(w, artifact.outgoing_halfedges());
  encode_range(w, artifact.carrier_balance_members());

  // Section 9: statistics.
  w.u64(artifact.statistics().incidence_audit_count);
  w.u64(artifact.statistics().zero_measure_support_count);
  w.u64(artifact.statistics().coordinate_reference_count);
  w.u64(artifact.statistics().vertex_occurrence_count);
  w.u64(artifact.statistics().region_count);
  w.u64(artifact.statistics().region_member_count);
  w.u64(artifact.statistics().continuation_consumption_count);
  w.u64(artifact.statistics().boundary_dart_count);
  w.u64(artifact.statistics().paired_edge_count);
  w.u64(artifact.statistics().halfedge_count);
  w.u64(artifact.statistics().endpoint_fan_ref_count);
  w.u64(artifact.statistics().face_cycle_count);
  w.u64(artifact.statistics().cycle_halfedge_ref_count);
  w.u64(artifact.statistics().contour_node_count);
  w.u64(artifact.statistics().contour_witness_count);
  w.u64(artifact.statistics().zero_measure_boundary_count);
  w.u64(artifact.statistics().admissibility_evidence_count);
  w.u64(artifact.statistics().carrier_balance_audit_count);
  w.u64(artifact.statistics().vertex_link_evidence_count);
  w.u64(artifact.statistics().verifier_work_units);

  bytes = w.take();
  if (bytes.size() > limits.maximum_section_bytes) {
    error = output_topology_error(
        output_topology_subcode::byte_count_overflow,
        bounded_boolean_error_category::index_overflow,
        "output topology canonical bytes exceed the configured limit",
        output_topology_checkpoint::canonical_encoding);
    return false;
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
