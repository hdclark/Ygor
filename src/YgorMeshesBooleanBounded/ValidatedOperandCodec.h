#pragma once

#include "CanonicalBytes.h"
#include "ValidatedOperand.h"

namespace ygor::mesh_boolean::bounded {
inline void encode_digest(canonical_writer &w, const bounded_boolean_digest &d) {
  for (auto byte : d.bytes)
    w.u8(byte);
}

template <class T, class I>
std::vector<std::uint8_t>
encode_validated_operand_semantic(const validated_operand<T, I> &a) {
  canonical_writer w;
  w.u32(0x324f4759U);
  w.u16(contract_versions::input_validation_provider);
  w.u16(contract_versions::normalized_ring_schema);
  w.u16(contract_versions::source_incidence_schema);
  w.u16(contract_versions::vertex_link_schema);
  w.u16(contract_versions::source_topology_canonicalizer);
  w.u16(contract_versions::presentation_correspondence_schema);
  w.u16(contract_versions::input_facet_geometry_schema);
  w.u16(contract_versions::input_facet_geometry_provider);
  w.u16(contract_versions::coherent_realization_schema);
  w.u16(contract_versions::coherent_realization_provider);
  w.u16(contract_versions::input_geometry_relations);
  w.u16(contract_versions::shell_pair_relation_schema);
  w.u16(contract_versions::shell_pair_relation_provider);
  w.u16(contract_versions::shell_semantics_schema);
  w.u16(contract_versions::shell_semantics_provider);
  w.u16(contract_versions::input_geometry_assessment_schema);
  w.u16(contract_versions::input_geometry_assessment_provider);
  w.u16(contract_versions::validated_operand);
  w.u16(contract_versions::validated_operand_codec);
  w.u16(contract_versions::validated_operand_verifier);
  w.u8(static_cast<std::uint8_t>(a.operand()));
  w.u8(sizeof(T));
  w.u8(sizeof(I));
  w.u8(static_cast<std::uint8_t>(a.certificate()));
  w.u64(a.vertices().size());
  for (const auto &v : a.vertices()) {
    w.u64(v.canonical_id);
    for (auto b : v.coordinate_bits)
      w.u64(b);
  }
  w.u64(a.bounded_vertices().size());
  for (const auto &e : a.bounded_vertices()) {
    w.u64(e.vertex);
    for (auto x : e.lower)
      w.floating(x);
    for (auto x : e.upper)
      w.floating(x);
    w.floating(e.radial_error);
  }
  w.u64(a.facets().size());
  for (const auto &f : a.facets()) {
    w.u64(f.canonical_id);
    w.u64(f.vertices.size());
    for (auto v : f.vertices)
      w.u64(v);
    for (auto v : f.support_vertices)
      w.u64(v);
    w.u64(f.shell);
    w.u8(f.dropped_axis);
    for (auto x : f.support_plane)
      w.long_floating(x);
    w.long_floating(f.projected_area);
    w.u64(f.decomposition.size());
    for (const auto &t : f.decomposition)
      for (auto v : t)
        w.u64(v);
  }
  w.u64(a.directed_uses().size());
  for (const auto &e : a.directed_uses()) {
    w.u64(e.origin);
    w.u64(e.destination);
    w.u64(e.facet);
    w.u64(e.corner);
    w.u64(e.reciprocal);
    w.u64(e.undirected_edge);
  }
  w.u64(a.edges().size());
  for (const auto &e : a.edges()) {
    w.u64(e.canonical_id);
    w.u64(e.low);
    w.u64(e.high);
    for (auto u : e.uses)
      w.u64(u);
  }
  w.u64(a.vertex_links().size());
  for (const auto &l : a.vertex_links()) {
    w.u64(l.vertex);
    w.u64(l.cyclic_facets.size());
    for (auto f : l.cyclic_facets)
      w.u64(f);
  }
  w.u64(a.edge_wedges().size());
  for (const auto &r : a.edge_wedges()) {
    w.u64(r.edge);
    w.u64(r.facets[0]);
    w.u64(r.facets[1]);
    w.boolean(r.locally_embedded);
  }
  w.u64(a.vertex_stars().size());
  for (const auto &r : a.vertex_stars()) {
    w.u64(r.vertex);
    w.u64(r.incident_facets);
    w.boolean(r.closed_disk);
  }
  w.u64(a.relations().size());
  for (const auto &r : a.relations()) {
    w.u64(r.facet_a);
    w.u64(r.triangle_a);
    w.u64(r.facet_b);
    w.u64(r.triangle_b);
    w.u8(static_cast<std::uint8_t>(r.relation));
    w.u8(r.shared_dimension);
    w.boolean(r.uncertainty_separated);
  }
  w.u64(a.shell_pairs().size());
  for (const auto &p : a.shell_pairs()) {
    w.u64(p.shell_a);
    w.u64(p.shell_b);
    w.u8(static_cast<std::uint8_t>(p.relation));
  }
  w.u64(a.shells().size());
  for (const auto &s : a.shells()) {
    w.u64(s.canonical_id);
    w.u64(s.facets.size());
    for (auto f : s.facets)
      w.u64(f);
    w.u8(static_cast<std::uint8_t>(s.intrinsic_orientation));
    w.u64(static_cast<std::uint64_t>(s.parent));
    w.u32(s.depth);
    w.u8(static_cast<std::uint8_t>(s.material_side));
    w.u8(static_cast<std::uint8_t>(s.empty_side));
    w.long_floating(s.signed_volume);
    w.long_floating(s.volume_uncertainty);
  }
  w.u64(a.statistics().triangle_pairs);
  w.u64(a.statistics().relation_calls);
  w.u64(a.statistics().canonical_branches);
  return w.take();
}

template <class T, class I>
std::vector<std::uint8_t>
encode_validated_operand_source_presentation(const validated_operand<T, I> &a) {
  canonical_writer w;
  w.u32(0x32504759U);
  w.u16(contract_versions::validated_operand);
  w.u16(contract_versions::validated_operand_codec);
  w.u16(contract_versions::presentation_correspondence_schema);
  w.u16(contract_versions::source_snapshot);
  w.u16(contract_versions::precision_codec);
  w.u16(contract_versions::context);
  encode_digest(w, a.source_digest());
  encode_digest(w, a.precision_digest());
  encode_digest(w, a.context_digest());
  w.u64(a.vertices().size());
  for (const auto &v : a.vertices()) {
    w.u64(v.canonical_id);
    w.u64(v.presentation_vertex);
  }
  w.u64(a.facets().size());
  for (const auto &f : a.facets()) {
    w.u64(f.canonical_id);
    w.u64(f.presentation_facet);
  }
  w.u64(a.presentation_normalization().size());
  for (const auto &p : a.presentation_normalization()) {
    w.u64(p.source_position);
    w.u64(p.source_facet);
    w.u64(p.canonical_facet);
    w.u8(static_cast<std::uint8_t>(p.action));
    w.u64(p.retained_corner);
  }
  return w.take();
}
} // namespace ygor::mesh_boolean::bounded
