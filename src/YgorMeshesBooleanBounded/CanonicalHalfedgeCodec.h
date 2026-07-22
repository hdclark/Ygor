#pragma once

#include "CanonicalHalfedgeOperand.h"
#include "Sha256.h"

#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {

namespace canonical_halfedge_codec_detail {
inline void write_digest(canonical_writer &writer,
                         const bounded_boolean_digest &digest) {
  for (auto byte : digest.bytes)
    writer.u8(byte);
}
inline void write_vertex_key(canonical_writer &writer,
                             const canonical_vertex_key &key) {
  writer.u8(static_cast<std::uint8_t>(key.operand));
  writer.u64(key.source_vertex);
  writer.u32(key.occurrence);
  writer.u16(key.schema_version);
}
inline void write_edge_use_key(canonical_writer &writer,
                               const canonical_edge_use_key &key) {
  writer.u8(static_cast<std::uint8_t>(key.role));
  writer.u64(key.primary);
  writer.u64(key.secondary);
  writer.u64(key.directed_use);
  writer.u64(key.source_corner);
}
inline void write_triangle_key(canonical_writer &writer,
                               const canonical_triangle_key &key) {
  writer.u8(static_cast<std::uint8_t>(key.operand));
  writer.u64(key.source_triangle);
  for (const auto &vertex : key.rotation.vertices)
    write_vertex_key(writer, vertex);
  for (const auto &use : key.rotation.edge_uses)
    write_edge_use_key(writer, use);
  writer.u16(key.schema_version);
}
inline void write_halfedge_key(canonical_writer &writer,
                               const canonical_halfedge_key &key) {
  write_triangle_key(writer, key.triangle);
  writer.u8(key.local_slot);
  writer.u16(key.schema_version);
}
inline void write_edge_key(canonical_writer &writer,
                           const canonical_edge_key &key) {
  writer.u8(static_cast<std::uint8_t>(key.operand));
  writer.u8(static_cast<std::uint8_t>(key.edge_class));
  writer.u64(key.primary);
  writer.u64(key.secondary);
  writer.u16(key.policy_version);
}
template <class T>
inline void write_bound(canonical_writer &writer,
                        const canonical_bound3<T> &bound) {
  writer.u16(bound.schema_version);
  writer.u16(bound.formula_version);
  for (const auto &axis : bound.axes) {
    writer.floating(axis.lower());
    writer.floating(axis.upper());
  }
}
inline void write_u64_vector(canonical_writer &writer,
                             const std::vector<std::uint64_t> &values) {
  writer.u64(values.size());
  for (auto value : values)
    writer.u64(value);
}
} // namespace canonical_halfedge_codec_detail

template <class T, class I>
inline std::vector<std::uint8_t>
encode_canonical_halfedge_source_semantics(
    const canonical_halfedge_operand<T, I> &artifact) {
  using namespace canonical_halfedge_codec_detail;
  canonical_writer writer;
  writer.u32(0x35534843U); // CHS5
  writer.u16(contract_versions::canonical_halfedge_codec);
  writer.u8(static_cast<std::uint8_t>(artifact.operand_));
  write_digest(writer, artifact.validated_operand_digest_);
  write_digest(writer, artifact.source_triangles_->source_semantic_digest());
  writer.u64(artifact.vertices_.size());
  for (const auto &vertex : artifact.vertices_) {
    writer.u64(vertex.canonical_id);
    write_vertex_key(writer, vertex.key);
    writer.u64(vertex.source_vertex);
    writer.u64(vertex.shell);
    for (auto word : vertex.nominal_bits)
      writer.u64(word);
    writer.u64(vertex.presentation_vertex);
  }
  writer.u64(artifact.facet_groups_.size());
  for (const auto &group : artifact.facet_groups_) {
    writer.u64(group.source_facet);
    writer.u64(group.ring);
    writer.u64(group.shell);
    write_u64_vector(writer, group.source_vertices);
    write_digest(writer, group.source_semantic_digest);
  }
  writer.u64(artifact.shell_groups_.size());
  for (const auto &group : artifact.shell_groups_) {
    writer.u64(group.source_shell);
    writer.u8(static_cast<std::uint8_t>(group.intrinsic_orientation));
    writer.u64(static_cast<std::uint64_t>(group.parent));
    writer.u32(group.depth);
    writer.u8(static_cast<std::uint8_t>(group.material_side));
    writer.u8(static_cast<std::uint8_t>(group.empty_side));
    write_u64_vector(writer, group.facets);
    write_u64_vector(writer, group.source_edges);
  }
  return writer.take();
}

template <class T, class I>
inline std::vector<std::uint8_t>
encode_canonical_halfedge_exact_topology(
    const canonical_halfedge_operand<T, I> &artifact) {
  using namespace canonical_halfedge_codec_detail;
  canonical_writer writer;
  writer.u32(0x35544843U); // CHT5
  writer.u16(contract_versions::canonical_halfedge_codec);
  writer.u8(static_cast<std::uint8_t>(artifact.operand_));
  writer.u64(artifact.triangles_.size());
  for (const auto &triangle : artifact.triangles_) {
    writer.u64(triangle.canonical_id);
    write_triangle_key(writer, triangle.key);
    writer.u64(triangle.source_triangle);
    writer.u64(triangle.source_facet);
    writer.u64(triangle.ring);
    writer.u64(triangle.shell);
    for (auto value : triangle.vertices)
      writer.u64(value);
    for (auto value : triangle.source_vertices)
      writer.u64(value);
    for (auto value : triangle.halfedges)
      writer.u64(value);
    writer.u64(triangle.facet_group);
    writer.u64(triangle.shell_group);
  }
  writer.u64(artifact.halfedges_.size());
  for (const auto &halfedge : artifact.halfedges_) {
    writer.u64(halfedge.canonical_id);
    write_halfedge_key(writer, halfedge.key);
    writer.u64(halfedge.triangle);
    writer.u8(halfedge.local_slot);
    writer.u64(halfedge.origin);
    writer.u64(halfedge.destination);
    writer.u64(halfedge.source_origin);
    writer.u64(halfedge.source_destination);
    writer.u64(halfedge.next);
    writer.u64(halfedge.previous);
    writer.u64(halfedge.pair);
    writer.u64(halfedge.edge);
    writer.u8(static_cast<std::uint8_t>(halfedge.edge_class));
    writer.u64(halfedge.source_facet);
    writer.u64(halfedge.ring);
    writer.u64(halfedge.shell);
    writer.u64(halfedge.source_directed_use);
    writer.u64(halfedge.source_undirected_edge);
    writer.u64(halfedge.source_corner);
    writer.u64(halfedge.source_diagonal);
    writer.u64(halfedge.predecessor_edge_use);
  }
  writer.u64(artifact.edges_.size());
  for (const auto &edge : artifact.edges_) {
    writer.u64(edge.canonical_id);
    write_edge_key(writer, edge.key);
    writer.u8(static_cast<std::uint8_t>(edge.edge_class));
    for (auto value : edge.endpoints)
      writer.u64(value);
    for (auto value : edge.halfedges)
      writer.u64(value);
    for (auto value : edge.triangles)
      writer.u64(value);
    for (auto value : edge.facets)
      writer.u64(value);
    writer.u64(edge.representative);
    writer.u64(edge.source_undirected_edge);
    for (auto value : edge.source_directed_uses)
      writer.u64(value);
    writer.u64(edge.source_facet);
    writer.u64(edge.source_diagonal);
    writer.boolean(edge.source_feature_owner);
    writer.boolean(edge.symbolic_contact_owner);
    writer.boolean(edge.classification_barrier_inside_source_facet);
    writer.boolean(edge.retained_surface_feature);
  }
  writer.u64(artifact.fans_.size());
  for (const auto &fan : artifact.fans_) {
    writer.u64(fan.canonical_id);
    writer.u64(fan.vertex);
    write_u64_vector(writer, fan.outgoing_halfedges);
  }
  writer.u64(artifact.facet_groups_.size());
  for (const auto &group : artifact.facet_groups_) {
    writer.u64(group.canonical_id);
    writer.u64(group.source_facet);
    write_u64_vector(writer, group.vertices);
    write_u64_vector(writer, group.triangles);
    write_u64_vector(writer, group.internal_edges);
    write_u64_vector(writer, group.boundary_halfedges);
    write_digest(writer, group.exact_triangulation_digest);
  }
  writer.u64(artifact.shell_groups_.size());
  for (const auto &group : artifact.shell_groups_) {
    writer.u64(group.canonical_id);
    writer.u64(group.source_shell);
    write_u64_vector(writer, group.vertices);
    write_u64_vector(writer, group.facets);
    write_u64_vector(writer, group.triangles);
    write_u64_vector(writer, group.edges);
  }
  write_u64_vector(writer, artifact.source_vertex_to_vertex_);
  write_u64_vector(writer, artifact.vertex_to_source_vertex_);
  write_u64_vector(writer, artifact.source_triangle_to_triangle_);
  write_u64_vector(writer, artifact.triangle_to_source_triangle_);
  write_u64_vector(writer, artifact.source_directed_use_to_halfedge_);
  write_u64_vector(writer, artifact.source_edge_to_edge_);
  write_u64_vector(writer, artifact.source_diagonal_to_edge_);
  write_u64_vector(writer, artifact.source_facet_to_group_);
  write_u64_vector(writer, artifact.source_shell_to_group_);
  return writer.take();
}

template <class T, class I>
inline std::vector<std::uint8_t>
encode_canonical_halfedge_geometry_attachments(
    const canonical_halfedge_operand<T, I> &artifact) {
  using namespace canonical_halfedge_codec_detail;
  canonical_writer writer;
  writer.u32(0x35474843U); // CHG5
  writer.u16(contract_versions::canonical_halfedge_codec);
  writer.u8(static_cast<std::uint8_t>(artifact.operand_));
  write_digest(writer, artifact.precision_digest_);
  writer.u64(artifact.vertices_.size());
  for (const auto &vertex : artifact.vertices_) {
    writer.u64(vertex.canonical_id);
    writer.u8(static_cast<std::uint8_t>(vertex.geometry_basis));
    for (auto value : vertex.committed_point)
      writer.floating(value);
    for (auto value : vertex.lower)
      writer.floating(value);
    for (auto value : vertex.upper)
      writer.floating(value);
    writer.floating(vertex.radial_error);
    write_bound(writer, vertex.bound);
  }
  writer.u64(artifact.triangles_.size());
  for (const auto &triangle : artifact.triangles_) {
    writer.u64(triangle.canonical_id);
    writer.u8(static_cast<std::uint8_t>(triangle.basis.kind));
    writer.u64(triangle.basis.facet);
    writer.u64(triangle.basis.ring);
    writer.u64(triangle.basis.shell);
    writer.u8(triangle.basis.dropped_axis);
    for (auto value : triangle.basis.support_vertices)
      writer.u64(value);
    write_digest(writer, triangle.basis.predecessor_digest);
    write_digest(writer, triangle.basis.precision_digest);
    write_digest(writer, triangle.basis.basis_digest);
    writer.u8(static_cast<std::uint8_t>(triangle.orientation.bounded_sign));
    writer.u8(static_cast<std::uint8_t>(triangle.orientation.exact_sign));
    writer.u16(triangle.orientation.formula_version);
    write_bound(writer, triangle.bound);
  }
  writer.u64(artifact.edges_.size());
  for (const auto &edge : artifact.edges_) {
    writer.u64(edge.canonical_id);
    write_bound(writer, edge.bound);
  }
  writer.u64(artifact.facet_groups_.size());
  for (const auto &group : artifact.facet_groups_) {
    writer.u64(group.canonical_id);
    write_bound(writer, group.bound);
  }
  writer.u64(artifact.shell_groups_.size());
  for (const auto &group : artifact.shell_groups_) {
    writer.u64(group.canonical_id);
    write_bound(writer, group.bound);
  }
  return writer.take();
}

template <class T, class I>
inline std::vector<std::uint8_t>
encode_canonical_halfedge_complete(
    const canonical_halfedge_operand<T, I> &artifact,
    const std::vector<std::uint8_t> &source,
    const std::vector<std::uint8_t> &topology,
    const std::vector<std::uint8_t> &geometry) {
  using namespace canonical_halfedge_codec_detail;
  canonical_writer writer;
  writer.u32(0x35414843U); // CHA5
  writer.u16(contract_versions::canonical_halfedge_operand_schema);
  writer.u16(contract_versions::canonical_halfedge_provider);
  writer.u16(contract_versions::canonical_halfedge_policy);
  writer.u16(contract_versions::canonical_halfedge_codec);
  writer.u16(contract_versions::canonical_halfedge_verifier);
  writer.u8(static_cast<std::uint8_t>(artifact.operand_));
  writer.u8(static_cast<std::uint8_t>(artifact.verification_));
  writer.u32(0);
  writer.u64(artifact.vertices_.size());
  writer.u64(artifact.triangles_.size());
  writer.u64(artifact.halfedges_.size());
  writer.u64(artifact.edges_.size());
  writer.u64(artifact.fans_.size());
  writer.u64(artifact.facet_groups_.size());
  writer.u64(artifact.shell_groups_.size());
  writer.sized_bytes(source);
  writer.sized_bytes(topology);
  writer.sized_bytes(geometry);
  write_digest(writer, artifact.replay_presentation_digest_);
  return writer.take();
}

template <class T, class I>
inline bool canonical_halfedge_codec_header_valid(
    const std::vector<std::uint8_t> &bytes,
    const canonical_halfedge_operand<T, I> &expected) {
  canonical_reader reader(bytes);
  std::uint32_t magic = 0, reserved = 0;
  std::uint16_t schema = 0, provider = 0, policy = 0, codec = 0, verifier = 0;
  std::uint8_t operand = 0, verification = 0;
  std::uint64_t counts[7]{};
  std::vector<std::uint8_t> section;
  if (!reader.u32(magic) || magic != 0x35414843U ||
      !reader.u16(schema) || schema != contract_versions::canonical_halfedge_operand_schema ||
      !reader.u16(provider) || provider != contract_versions::canonical_halfedge_provider ||
      !reader.u16(policy) || policy != contract_versions::canonical_halfedge_policy ||
      !reader.u16(codec) || codec != contract_versions::canonical_halfedge_codec ||
      !reader.u16(verifier) || verifier != contract_versions::canonical_halfedge_verifier ||
      !reader.u8(operand) || operand != static_cast<std::uint8_t>(expected.operand()) ||
      !reader.u8(verification) || verification != static_cast<std::uint8_t>(canonical_halfedge_verification_disposition::independently_verified) ||
      !reader.u32(reserved) || reserved != 0)
    return false;
  for (auto &count : counts)
    if (!reader.u64(count))
      return false;
  if (counts[0] != expected.vertices().size() ||
      counts[1] != expected.triangles().size() ||
      counts[2] != expected.halfedges().size() ||
      counts[3] != expected.edges().size() ||
      counts[4] != expected.fans().size() ||
      counts[5] != expected.facet_groups().size() ||
      counts[6] != expected.shell_groups().size())
    return false;
  for (int i = 0; i < 3; ++i)
    if (!reader.sized_bytes(section, std::uint64_t{1} << 40))
      return false;
  std::vector<std::uint8_t> digest_bytes;
  return reader.fixed_bytes(32, digest_bytes) && reader.complete();
}

} // namespace ygor::mesh_boolean::bounded
