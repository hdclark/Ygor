#pragma once

#include "ClassificationComplex.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

namespace classification_codec_detail {

inline void encode_key(canonical_writer &writer,
                       const classification_atom_key &key) {
  writer.u8(static_cast<std::uint8_t>(key.operand));
  writer.u64(key.shell);
  writer.u64(key.source_facet);
  writer.u64(key.boundary_lineage.size());
  for (const auto value : key.boundary_lineage)
    writer.u64(value);
  writer.u64(key.incident_descriptor_lineage.size());
  for (const auto value : key.incident_descriptor_lineage)
    writer.u64(value);
  writer.u16(key.schema_version);
}

inline void encode_adjacency_key(canonical_writer &writer,
                                 const classification_adjacency_key &key) {
  encode_key(writer, key.source);
  encode_key(writer, key.destination);
  writer.u8(static_cast<std::uint8_t>(key.adjacency_class));
  writer.u8(static_cast<std::uint8_t>(key.eligibility));
  writer.u32(static_cast<std::uint32_t>(key.total_delta));
  writer.u64(key.shell_deltas.size());
  for (const auto &entry : key.shell_deltas) {
    writer.u64(entry.first);
    writer.u32(static_cast<std::uint32_t>(entry.second));
  }
  writer.u64(key.semantic_locus_lineage);
  writer.u64(key.descriptor_lineage);
  writer.u16(key.schema_version);
}

inline void encode_witness(canonical_writer &writer,
                           const classification_witness &witness) {
  for (std::size_t axis = 0; axis < 3; ++axis)
    writer.u64(witness.nominal_bits[axis]);
  for (std::size_t axis = 0; axis < 6; ++axis)
    writer.u64(witness.enclosure_bits[axis]);
}

inline void encode_shell_winding(canonical_writer &writer,
                                 const std::vector<std::pair<std::uint64_t, std::int64_t>> &map) {
  writer.u64(map.size());
  for (const auto &entry : map) {
    writer.u64(entry.first);
    writer.u64(static_cast<std::uint64_t>(entry.second));
  }
}

inline void encode_int32_shell_winding(
    canonical_writer &writer,
    const std::vector<std::pair<std::uint64_t, std::int32_t>> &map) {
  writer.u64(map.size());
  for (const auto &entry : map) {
    writer.u64(entry.first);
    writer.u32(static_cast<std::uint32_t>(entry.second));
  }
}

} // namespace classification_codec_detail

template <class T, class I>
bool encode_classification_complex(const classification_complex<T, I> &artifact,
                                   std::vector<std::uint8_t> &out,
                                   const classification_codec_limits &limits,
                                   bounded_boolean_error &error) {
  using namespace classification_codec_detail;
  canonical_writer writer;
  writer.u32(0x434C5359); // "YSCL"
  writer.u16(contract_versions::classification_artifact_schema);
  writer.u16(contract_versions::classification_provider);
  writer.u16(contract_versions::classification_codec);
  writer.u16(contract_versions::classification_verifier);
  writer.u8(static_cast<std::uint8_t>(artifact.provider()));
  writer.u8(static_cast<std::uint8_t>(artifact.verification()));
  writer.u8(static_cast<std::uint8_t>(artifact.operation()));
  for (const auto byte : artifact.context_digest().bytes)
    writer.u8(byte);
  for (const auto byte : artifact.precision_digest().bytes)
    writer.u8(byte);
  for (const auto byte : artifact.relation_digest().bytes)
    writer.u8(byte);
  for (const auto byte : artifact.intersection_digest().bytes)
    writer.u8(byte);
  for (const auto byte : artifact.manifold_digests()[0].bytes)
    writer.u8(byte);
  for (const auto byte : artifact.manifold_digests()[1].bytes)
    writer.u8(byte);

  writer.u64(artifact.atoms().size());
  for (const auto &atom : artifact.atoms()) {
    writer.u64(atom.canonical_id);
    encode_key(writer, atom.key);
    writer.u8(static_cast<std::uint8_t>(atom.operand));
    writer.u64(atom.shell);
    writer.u64(atom.source_facet);
    writer.u64(atom.source_triangles.size());
    for (const auto value : atom.source_triangles)
      writer.u64(value);
    writer.boolean(atom.positive_area);
    encode_witness(writer, atom.witness);
    writer.u64(atom.group);
    writer.u64(atom.side_label);
  }

  writer.u64(artifact.sectors().size());
  for (const auto &sector : artifact.sectors()) {
    writer.u64(sector.canonical_id);
    writer.u8(static_cast<std::uint8_t>(sector.operand));
    writer.u64(sector.source_feature);
    writer.u8(static_cast<std::uint8_t>(sector.contact));
    writer.u64(sector.incident_atoms.size());
    for (const auto value : sector.incident_atoms)
      writer.u64(value);
    writer.boolean(sector.occurrence_separated);
  }

  writer.u64(artifact.occurrences().size());
  for (const auto &occurrence : artifact.occurrences()) {
    writer.u64(occurrence.canonical_id);
    writer.u8(static_cast<std::uint8_t>(occurrence.operand));
    writer.u64(occurrence.event_ordinal);
    writer.u64(occurrence.descriptor_lineage);
    writer.boolean(occurrence.topology_separate);
    writer.u64(occurrence.incident_sectors.size());
    for (const auto value : occurrence.incident_sectors)
      writer.u64(value);
  }

  writer.u64(artifact.adjacency().size());
  for (const auto &adj : artifact.adjacency()) {
    writer.u64(adj.canonical_id);
    encode_adjacency_key(writer, adj.key);
    writer.u64(adj.source_atom);
    writer.u64(adj.destination_atom);
    writer.u64(adj.reverse);
    writer.u64(adj.descriptor_lineage);
  }

  writer.u64(artifact.groups().size());
  for (const auto &group : artifact.groups()) {
    writer.u64(group.canonical_id);
    writer.u64(group.members.size());
    for (const auto value : group.members)
      writer.u64(value);
    writer.u64(group.propagation_component);
    writer.u64(static_cast<std::uint64_t>(group.total_winding));
    encode_shell_winding(writer, group.shell_winding);
  }

  writer.u64(artifact.quotient_edges().size());
  for (const auto &edge : artifact.quotient_edges()) {
    writer.u64(edge.canonical_id);
    writer.u64(edge.source_group);
    writer.u64(edge.destination_group);
    writer.u64(edge.reverse);
    writer.u32(static_cast<std::uint32_t>(edge.total_delta));
    encode_int32_shell_winding(writer, edge.shell_deltas);
    writer.u64(edge.member_adjacency.size());
    for (const auto value : edge.member_adjacency)
      writer.u64(value);
    writer.u8(static_cast<std::uint8_t>(edge.role));
  }

  writer.u64(artifact.propagation_components().size());
  for (const auto &component : artifact.propagation_components()) {
    writer.u64(component.canonical_id);
    writer.u64(component.groups.size());
    for (const auto value : component.groups)
      writer.u64(value);
    writer.u64(component.anchor_group);
    writer.u8(static_cast<std::uint8_t>(component.anchor_source));
  }

  writer.u64(artifact.seed_queries().size());
  for (const auto &query : artifact.seed_queries()) {
    writer.u64(query.canonical_id);
    writer.u64(query.component);
    writer.u64(query.group);
    writer.u64(query.atom);
    writer.u8(static_cast<std::uint8_t>(query.source_kind));
    writer.u64(static_cast<std::uint64_t>(query.total_winding));
    encode_shell_winding(writer, query.shell_winding);
    writer.u64(query.attempts.size());
    for (const auto &attempt : query.attempts) {
      writer.u64(attempt.canonical_id);
      writer.u32(attempt.direction_index);
      writer.u8(static_cast<std::uint8_t>(attempt.disposition));
    }
  }

  writer.u64(artifact.side_labels().size());
  for (const auto &label : artifact.side_labels()) {
    writer.u64(label.canonical_id);
    writer.u64(label.atom);
    writer.u8(static_cast<std::uint8_t>(label.negative_side));
    writer.u8(static_cast<std::uint8_t>(label.positive_side));
    writer.u8(static_cast<std::uint8_t>(label.negative_origin));
    writer.u8(static_cast<std::uint8_t>(label.positive_origin));
    writer.u8(static_cast<std::uint8_t>(label.contact));
    writer.boolean(label.occurrence_separated);
  }

  const auto &statistics = artifact.statistics();
  writer.u64(statistics.triangle_count);
  writer.u64(statistics.atom_count);
  writer.u64(statistics.sector_count);
  writer.u64(statistics.occurrence_count);
  writer.u64(statistics.adjacency_count);
  writer.u64(statistics.union_proposal_count);
  writer.u64(statistics.group_count);
  writer.u64(statistics.quotient_edge_count);
  writer.u64(statistics.propagation_component_count);
  writer.u64(statistics.seed_query_count);
  writer.u64(statistics.seed_attempt_count);
  writer.u64(statistics.seed_hit_count);
  writer.u64(statistics.propagation_assignment_count);
  writer.u64(statistics.side_label_count);
  writer.u64(statistics.verifier_work_units);
  writer.u64(statistics.persistent_bytes);
  writer.u64(statistics.canonical_bytes);

  if (writer.bytes().size() > limits.maximum_section_bytes) {
    error = classification_error(classification_subcode::codec_error,
                                 bounded_boolean_error_category::resource_limit,
                                 "classification canonical bytes exceed the limit",
                                 classification_checkpoint::canonical_encoding);
    return false;
  }
  out = writer.take();
  return true;
}

} // namespace ygor::mesh_boolean::bounded
