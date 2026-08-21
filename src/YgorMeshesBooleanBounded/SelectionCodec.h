#pragma once

#include "RetainedSurfaceComplex.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct selection_codec_access;
struct selection_codec_limits final {
  std::uint64_t maximum_section_bytes = (std::uint64_t{1} << 34);
  std::uint64_t reserved = 0;
};

namespace selection_codec_detail {

inline void encode_side_occupancy(canonical_writer &w,
                                  const selection_side_occupancy &value) {
  w.boolean(value.a_negative);
  w.boolean(value.b_negative);
  w.boolean(value.a_positive);
  w.boolean(value.b_positive);
}

inline void encode_disposition_key(canonical_writer &w,
                                   const selection_disposition_key &key) {
  w.u8(static_cast<std::uint8_t>(key.source_operand));
  w.u64(key.atom_ordinal);
  w.u64(key.shell);
  w.u64(key.source_facet);
  encode_side_occupancy(w, key.occupancy);
  w.u8(static_cast<std::uint8_t>(key.operation));
  w.u16(key.truth_version);
  w.u16(key.symbolic_version);
  w.u16(key.schema_version);
}

inline void encode_descriptor(canonical_writer &w,
                              const surface_occurrence_descriptor &value) {
  w.u8(static_cast<std::uint8_t>(value.source_operand));
  w.u64(value.source_shell);
  w.u64(value.source_facet);
  w.u64(value.sheet_owner_lineage);
  w.u64(value.retained_use_lineage);
  w.u32(value.multiplicity_occurrence);
  w.u8(static_cast<std::uint8_t>(value.result_side_transition));
  w.u64(value.start_sector_lineage);
  w.u64(value.end_sector_lineage);
  w.u8(static_cast<std::uint8_t>(value.separation));
  w.u64(value.separation_lineage);
  w.u16(value.schema_version);
}

inline void encode_carrier(canonical_writer &w,
                           const selection_carrier_identity &value) {
  w.u8(static_cast<std::uint8_t>(value.kind));
  w.u8(static_cast<std::uint8_t>(value.source_operand));
  w.u64(value.lineage);
  w.u64(value.span_lineage);
  w.u8(static_cast<std::uint8_t>(value.separation));
  w.u64(value.separation_lineage);
  w.u16(value.schema_version);
}

inline void encode_slot_key(canonical_writer &w,
                            const edge_occurrence_slot_key &key) {
  encode_carrier(w, key.carrier);
  w.u64(key.endpoint_domains[0]);
  w.u64(key.endpoint_domains[1]);
  w.u64(key.output_slot_discriminator);
  encode_descriptor(w, key.expected_pair.first);
  encode_descriptor(w, key.expected_pair.second);
  w.u16(key.schema_version);
}

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

} // namespace selection_codec_detail

template <class T, class I>
bool encode_retained_surface_complex(const retained_surface_complex<T, I> &artifact,
                                     std::vector<std::uint8_t> &bytes,
                                     selection_codec_limits limits,
                                     bounded_boolean_error &error) {
  using namespace selection_codec_detail;
  canonical_writer w;

  // Section 0: magic / header / owner / operation / predecessor digests.
  w.u32(0x53454c31); // "SEL1"
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

  // Section 1: dispositions, side tuples, truth records.
  w.u64(artifact.dispositions().size());
  for (const auto &record : artifact.dispositions()) {
    w.u64(record.canonical_id);
    encode_disposition_key(w, record.key);
    w.u64(record.atom);
    w.u64(record.side_tuple);
    w.u64(record.truth);
    w.u8(static_cast<std::uint8_t>(record.disposition));
    w.u64(record.retained_use);
    w.u64(record.sheet_cell);
    w.u64(record.multiplicity);
    w.boolean(record.source_orientation_reversed);
  }
  w.u64(artifact.side_tuples().size());
  for (const auto &record : artifact.side_tuples()) {
    w.u64(record.canonical_id);
    w.u64(record.atom);
    w.u8(static_cast<std::uint8_t>(record.source_operand));
    w.u64(record.source_shell);
    encode_side_occupancy(w, record.occupancy);
    for (const auto origin : record.origins)
      w.u8(static_cast<std::uint8_t>(origin));
    w.boolean(record.coincident);
    w.u64(record.sheet_cell);
  }
  w.u64(artifact.truth_records().size());
  for (const auto &record : artifact.truth_records()) {
    w.u64(record.canonical_id);
    w.u64(record.side_tuple);
    w.u64(record.atom);
    w.u8(static_cast<std::uint8_t>(record.operation));
    w.boolean(record.result_negative);
    w.boolean(record.result_positive);
    w.boolean(record.retain);
    w.u8(static_cast<std::uint8_t>(record.orientation));
    w.u8(record.multiplicity);
    w.u8(record.owner_priority);
    w.u16(record.truth_version);
  }

  // Section 2: sheet cells, members, owners, suppressions, multiplicities.
  w.u64(artifact.sheet_cells().size());
  for (const auto &record : artifact.sheet_cells()) {
    w.u64(record.canonical_id);
    w.u8(static_cast<std::uint8_t>(record.relation));
    w.u64(record.support_lineage);
    w.boolean(record.opposite_orientation);
    w.u8(static_cast<std::uint8_t>(record.symbolic_owner));
    w.u64(record.members_begin);
    w.u64(record.members_count);
    w.u64(record.owner_atom);
    w.u64(record.owner_decision);
    w.u64(record.multiplicity);
    w.boolean(record.cancelled);
    w.boolean(record.internal);
    w.boolean(record.distinct_occurrences);
  }
  w.u64(artifact.coincidence_members().size());
  for (const auto &record : artifact.coincidence_members()) {
    w.u64(record.canonical_id);
    w.u64(record.sheet_cell);
    w.u64(record.atom);
    w.u8(static_cast<std::uint8_t>(record.source_operand));
    w.u64(record.source_facet);
  }
  w.u64(artifact.owner_decisions().size());
  for (const auto &record : artifact.owner_decisions()) {
    w.u64(record.canonical_id);
    w.u64(record.sheet_cell);
    w.u64(record.owner_atom);
    w.u8(static_cast<std::uint8_t>(record.owner_operand));
    for (const auto component : record.rank_components)
      w.u64(component);
    w.u64(record.symbolic_rule_ordinal);
  }
  w.u64(artifact.suppressions().size());
  for (const auto &record : artifact.suppressions()) {
    w.u64(record.canonical_id);
    w.u64(record.atom);
    w.u8(static_cast<std::uint8_t>(record.reason));
    w.u64(record.owner_atom);
  }
  w.u64(artifact.multiplicities().size());
  for (const auto &record : artifact.multiplicities()) {
    w.u64(record.canonical_id);
    w.u64(record.sheet_cell);
    w.u32(record.occurrence_count);
    w.u32(record.occurrence_slot);
    w.u64(record.separation_lineage);
  }

  // Section 3: retained uses and provenance.
  w.u64(artifact.retained_uses().size());
  for (const auto &record : artifact.retained_uses()) {
    w.u64(record.canonical_id);
    w.u64(record.atom);
    w.u8(static_cast<std::uint8_t>(record.source_operand));
    w.u64(record.source_shell);
    w.u64(record.source_facet);
    w.boolean(record.preserve_source_orientation);
    encode_side_occupancy(w, record.result_occupancy);
    w.u64(record.sheet_owner_lineage);
    w.u64(record.multiplicity_occurrence);
    w.u8(static_cast<std::uint8_t>(record.separation));
    w.u64(record.separation_lineage);
    w.u64(record.provenance);
  }
  w.u64(artifact.retained_provenance().size());
  for (const auto &record : artifact.retained_provenance()) {
    w.u64(record.canonical_id);
    w.u64(record.retained_use);
    w.u64(record.atom);
    w.u8(static_cast<std::uint8_t>(record.source_operand));
    w.u64(record.source_shell);
    w.u64(record.source_facet);
    w.u64(record.source_triangle);
  }

  // Section 4: incidences and continuations.
  w.u64(artifact.incidences().size());
  for (const auto &record : artifact.incidences()) {
    w.u64(record.canonical_id);
    w.u64(record.retained_use);
    w.u64(record.atom);
    w.u64(record.start_domain);
    w.u64(record.end_domain);
    encode_carrier(w, record.carrier);
    w.u8(static_cast<std::uint8_t>(record.direction));
    w.u64(record.start_sector_lineage);
    w.u64(record.end_sector_lineage);
    encode_descriptor(w, record.descriptor);
    encode_descriptor(w, record.expected_opposite);
    w.u8(static_cast<std::uint8_t>(record.disposition));
    w.u64(record.continuation);
    w.u64(record.planned_edge);
    w.u64(record.carrier_balance);
  }
  w.u64(artifact.continuations().size());
  for (const auto &record : artifact.continuations()) {
    w.u64(record.canonical_id);
    w.u64(record.forward_incidence);
    w.u64(record.reverse_incidence);
    w.u64(record.seam_lineage);
  }

  // Section 5: edge mate groups, planned edges, carrier balance.
  w.u64(artifact.edge_mate_groups().size());
  for (const auto &record : artifact.edge_mate_groups()) {
    w.u64(record.canonical_id);
    encode_slot_key(w, record.slot);
    w.u64(record.forward_incidence);
    w.u64(record.reverse_incidence);
  }
  w.u64(artifact.planned_edges().size());
  for (const auto &record : artifact.planned_edges()) {
    w.u64(record.canonical_id);
    w.u64(record.mate_group);
    w.u64(record.start_domain);
    w.u64(record.end_domain);
    encode_descriptor(w, record.expected_pair.first);
    encode_descriptor(w, record.expected_pair.second);
    w.boolean(record.cross_operand);
    w.boolean(record.cross_owner);
  }
  w.u64(artifact.carrier_balances().size());
  for (const auto &record : artifact.carrier_balances()) {
    w.u64(record.canonical_id);
    encode_carrier(w, record.carrier);
    w.u64(record.members_begin);
    w.u64(record.members_count);
    w.boolean(record.balanced);
  }

  // Section 6: endpoint domains, ports, arcs, link components, occurrences.
  w.u64(artifact.endpoint_domains().size());
  for (const auto &record : artifact.endpoint_domains()) {
    w.u64(record.canonical_id);
    w.u8(static_cast<std::uint8_t>(record.kind));
    w.u8(static_cast<std::uint8_t>(record.source_operand));
    w.u64(record.lineage);
    w.u64(record.event_occurrence);
    w.u8(static_cast<std::uint8_t>(record.separation));
    w.u64(record.separation_lineage);
  }
  w.u64(artifact.local_ports().size());
  for (const auto &record : artifact.local_ports()) {
    w.u64(record.canonical_id);
    w.u64(record.endpoint_domain);
    w.u64(record.incidence);
    w.u64(record.retained_use);
    w.u8(static_cast<std::uint8_t>(record.role));
    w.u64(record.face_corner_arc);
    w.u64(record.edge_mate_arc);
  }
  w.u64(artifact.face_corner_arcs().size());
  for (const auto &record : artifact.face_corner_arcs()) {
    w.u64(record.canonical_id);
    w.u64(record.in_port);
    w.u64(record.out_port);
    w.u64(record.retained_use);
  }
  w.u64(artifact.edge_mate_arcs().size());
  for (const auto &record : artifact.edge_mate_arcs()) {
    w.u64(record.canonical_id);
    w.u64(record.first_port);
    w.u64(record.second_port);
    w.u64(record.planned_edge);
  }
  w.u64(artifact.link_components().size());
  for (const auto &record : artifact.link_components()) {
    w.u64(record.canonical_id);
    w.u64(record.endpoint_domain);
    w.u64(record.members_begin);
    w.u64(record.members_count);
    w.u64(record.vertex_occurrence);
  }
  w.u64(artifact.vertex_occurrences().size());
  for (const auto &record : artifact.vertex_occurrences()) {
    w.u64(record.canonical_id);
    w.u64(record.endpoint_domain);
    w.u64(record.cycle_begin);
    w.u64(record.cycle_count);
    w.u64(record.representative_port);
  }

  // Section 7: reverse maps and member ranges.
  encode_range(w, artifact.disposition_by_atom());
  encode_range(w, artifact.retained_use_by_atom());
  encode_range(w, artifact.cell_members());
  encode_range(w, artifact.balance_members());
  encode_range(w, artifact.link_cycle_ports());
  encode_range(w, artifact.incidence_by_retained_use());
  encode_range(w, artifact.port_by_endpoint_domain());

  // Section 8: statistics.
  w.u64(artifact.statistics().atom_count);
  w.u64(artifact.statistics().disposition_count);
  w.u64(artifact.statistics().side_tuple_count);
  w.u64(artifact.statistics().sheet_cell_count);
  w.u64(artifact.statistics().member_count);
  w.u64(artifact.statistics().owner_decision_count);
  w.u64(artifact.statistics().suppression_count);
  w.u64(artifact.statistics().multiplicity_count);
  w.u64(artifact.statistics().retained_use_count);
  w.u64(artifact.statistics().incidence_count);
  w.u64(artifact.statistics().continuation_count);
  w.u64(artifact.statistics().edge_occurrence_count);
  w.u64(artifact.statistics().carrier_balance_count);
  w.u64(artifact.statistics().endpoint_domain_count);
  w.u64(artifact.statistics().local_port_count);
  w.u64(artifact.statistics().face_corner_arc_count);
  w.u64(artifact.statistics().edge_mate_arc_count);
  w.u64(artifact.statistics().link_component_count);
  w.u64(artifact.statistics().vertex_occurrence_count);
  w.u64(artifact.statistics().verifier_work_units);

  bytes = w.take();
  if (bytes.size() > limits.maximum_section_bytes) {
    error = selection_error(selection_subcode::byte_count_overflow,
                            bounded_boolean_error_category::index_overflow,
                            "selection canonical bytes exceed the configured limit",
                            selection_checkpoint::canonical_encoding);
    return false;
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
