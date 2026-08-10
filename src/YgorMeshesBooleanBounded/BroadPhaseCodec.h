#pragma once

#include "CanonicalBytes.h"
#include "CanonicalCandidateStream.h"
#include "Sha256.h"

#include <array>
#include <cstdint>
#include <vector>

namespace ygor::mesh_boolean::bounded {
namespace broad_phase_codec_detail {

inline void write_digest(canonical_writer &writer,
                         const bounded_boolean_digest &digest) {
  for (const auto byte : digest.bytes)
    writer.u8(byte);
}

inline bool read_digest(canonical_reader &reader,
                        bounded_boolean_digest &digest) {
  for (auto &byte : digest.bytes)
    if (!reader.u8(byte))
      return false;
  return true;
}

inline bool validate_section_header(const std::vector<std::uint8_t> &bytes,
                                    std::uint32_t magic,
                                    std::uint16_t version) {
  canonical_reader reader(bytes);
  std::uint32_t observed_magic = 0;
  std::uint16_t observed_version = 0;
  return reader.u32(observed_magic) && reader.u16(observed_version) &&
         observed_magic == magic && observed_version == version;
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
  for (const auto &edge_use : key.rotation.edge_uses)
    write_edge_use_key(writer, edge_use);
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

inline void write_candidate_key(canonical_writer &writer,
                                const canonical_candidate_key &key) {
  writer.u8(static_cast<std::uint8_t>(key.role));
  writer.u8(static_cast<std::uint8_t>(key.family));
  write_edge_key(writer, key.edge);
  write_triangle_key(writer, key.triangle);
  writer.u8(static_cast<std::uint8_t>(key.edge_class));
  writer.u16(key.domain_policy_version);
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

inline void write_morton(canonical_writer &writer,
                         const rank_morton_key &key) {
  writer.u16(key.active_rank_bits);
  writer.u64(key.words.size());
  for (const auto word : key.words)
    writer.u64(word);
}

template <class T>
inline void write_edge_primitive(canonical_writer &writer,
                                 const broad_phase_edge_primitive<T> &edge) {
  writer.u64(edge.id.ordinal());
  writer.u64(edge.ordinal);
  writer.u8(static_cast<std::uint8_t>(edge.operand));
  writer.u64(edge.edge.ordinal());
  write_edge_key(writer, edge.semantic_key);
  writer.u8(static_cast<std::uint8_t>(edge.edge_class));
  writer.u64(edge.representative.ordinal());
  for (const auto halfedge : edge.halfedges)
    writer.u64(halfedge.ordinal());
  for (const auto endpoint : edge.endpoints)
    writer.u64(endpoint.ordinal());
  for (const auto triangle : edge.incident_triangles)
    writer.u64(triangle.ordinal());
  for (const auto facet : edge.source_facets)
    writer.u64(facet);
  writer.u64(edge.source_undirected_edge);
  for (const auto use : edge.source_directed_uses)
    writer.u64(use);
  writer.u64(edge.source_facet);
  writer.u64(edge.source_diagonal);
  writer.boolean(edge.source_feature_owner);
  writer.boolean(edge.symbolic_contact_owner);
  writer.boolean(edge.classification_barrier_inside_source_facet);
  writer.boolean(edge.retained_surface_feature);
  write_bound(writer, edge.bound);
  for (const auto &axis : edge.endpoint_bits) {
    writer.u64(static_cast<std::uint64_t>(axis[0]));
    writer.u64(static_cast<std::uint64_t>(axis[1]));
  }
  write_digest(writer, edge.geometry_attachment_digest);
  write_digest(writer, edge.precision_attachment_digest);
  writer.u8(static_cast<std::uint8_t>(edge.inclusion));
  writer.u32(edge.reserved);
}

template <class T>
inline void write_triangle_primitive(
    canonical_writer &writer,
    const broad_phase_triangle_primitive<T> &triangle) {
  writer.u64(triangle.id.ordinal());
  writer.u64(triangle.ordinal);
  writer.u8(static_cast<std::uint8_t>(triangle.operand));
  writer.u64(triangle.triangle.ordinal());
  write_triangle_key(writer, triangle.semantic_key);
  for (const auto vertex : triangle.vertices)
    writer.u64(vertex.ordinal());
  for (const auto halfedge : triangle.halfedges)
    writer.u64(halfedge.ordinal());
  writer.u64(triangle.source_triangle);
  writer.u64(triangle.source_facet);
  writer.u64(triangle.ring);
  writer.u64(triangle.shell);
  writer.u64(triangle.facet_group);
  writer.u64(triangle.shell_group);
  writer.u8(static_cast<std::uint8_t>(triangle.basis.kind));
  writer.u8(static_cast<std::uint8_t>(triangle.basis.operand));
  writer.u64(triangle.basis.facet);
  writer.u64(triangle.basis.ring);
  writer.u64(triangle.basis.shell);
  writer.u8(triangle.basis.dropped_axis);
  for (const auto support : triangle.basis.support_vertices)
    writer.u64(support);
  write_digest(writer, triangle.basis.predecessor_digest);
  write_digest(writer, triangle.basis.precision_digest);
  write_digest(writer, triangle.basis.basis_digest);
  writer.floating(triangle.orientation.determinant.lower());
  writer.floating(triangle.orientation.determinant.upper());
  writer.u8(static_cast<std::uint8_t>(triangle.orientation.exact_sign));
  writer.u8(static_cast<std::uint8_t>(triangle.orientation.bounded_sign));
  writer.u16(triangle.orientation.formula_version);
  write_bound(writer, triangle.bound);
  for (const auto &axis : triangle.axis_keys) {
    writer.u64(static_cast<std::uint64_t>(axis.lower));
    writer.u64(static_cast<std::uint64_t>(axis.upper));
  }
  for (const auto rank : triangle.dense_ranks)
    writer.u64(rank);
  write_morton(writer, triangle.morton);
  writer.u64(triangle.spatial_ordinal);
  write_digest(writer, triangle.geometry_attachment_digest);
  write_digest(writer, triangle.precision_attachment_digest);
  writer.u32(triangle.reserved);
}

template <class T>
inline void write_hierarchy_node(canonical_writer &writer,
                                 const triangle_aabb_hierarchy_node<T> &node) {
  writer.u64(node.id.ordinal());
  writer.u64(node.ordinal);
  writer.u8(static_cast<std::uint8_t>(node.operand));
  writer.u8(static_cast<std::uint8_t>(node.kind));
  write_bound(writer, node.bound);
  writer.u64(node.first_spatial_primitive);
  writer.u64(node.subtree_primitive_count);
  writer.u32(node.level_from_leaves);
  writer.u32(node.height);
  writer.u8(static_cast<std::uint8_t>(node.key.operand));
  writer.u64(node.key.first_spatial_primitive);
  writer.u64(node.key.subtree_primitive_count);
  writer.u8(static_cast<std::uint8_t>(node.key.kind));
  writer.u32(node.key.level_from_leaves);
  writer.u16(node.key.layout_version);
  writer.u64(node.left.ordinal());
  writer.u64(node.right.ordinal());
  writer.u32(node.reserved);
}

inline void write_count_plan(canonical_writer &writer,
                             const broad_phase_count_plan &plan) {
  writer.u8(static_cast<std::uint8_t>(plan.role));
  writer.u64(plan.edge.ordinal());
  writer.u64(plan.plan_ordinal);
  writer.u64(plan.node_tests);
  writer.u64(plan.leaf_visits);
  writer.u64(plan.primitive_tests);
  writer.u64(plan.definite_prunes);
  writer.u64(plan.retained_overlaps);
  writer.u64(plan.candidate_count);
  writer.u64(plan.output_prefix);
  writer.u64(plan.work_units);
  writer.u32(plan.reserved);
}

template <class T>
inline void write_witness(canonical_writer &writer,
                          const broad_phase_overlap_witness<T> &witness) {
  writer.u64(witness.id.ordinal());
  write_candidate_key(writer, witness.key.candidate);
  writer.u64(witness.key.admitting_leaf);
  writer.u64(witness.key.triangle_spatial_slot);
  for (const auto axis : witness.key.axes)
    writer.u8(static_cast<std::uint8_t>(axis));
  writer.u8(static_cast<std::uint8_t>(witness.role));
  writer.u64(witness.edge.ordinal());
  writer.u64(witness.triangle.ordinal());
  writer.u64(witness.admitting_leaf.ordinal());
  writer.u64(witness.triangle_spatial_slot);
  for (const auto axis : witness.relation.axes)
    writer.u8(static_cast<std::uint8_t>(axis));
  writer.boolean(witness.relation.definitely_separated);
  writer.u8(static_cast<std::uint8_t>(witness.filter_reason));
  writer.u64(witness.count_plan_ordinal);
  writer.u16(witness.provider_version);
  writer.u16(witness.formula_version);
  writer.u32(witness.reserved);
}

template <class T>
inline void write_candidate(canonical_writer &writer,
                            const canonical_candidate_record<T> &candidate) {
  writer.u64(candidate.id.ordinal());
  writer.u64(candidate.ordinal);
  write_candidate_key(writer, candidate.key);
  writer.u8(static_cast<std::uint8_t>(candidate.role));
  writer.u8(static_cast<std::uint8_t>(candidate.family));
  writer.u64(candidate.edge.ordinal());
  writer.u64(candidate.triangle.ordinal());
  writer.u8(static_cast<std::uint8_t>(candidate.edge_class));
  writer.u64(candidate.witness.ordinal());
  writer.u8(static_cast<std::uint8_t>(candidate.filter_reason));
  writer.u16(candidate.domain_policy_version);
  writer.u16(candidate.provider_version);
  writer.u32(candidate.reserved);
}

inline void write_statistics(canonical_writer &writer,
                             const broad_phase_statistics &statistics) {
  const auto write_pair = [&](const auto &values) {
    writer.u64(values[0]);
    writer.u64(values[1]);
  };
  write_pair(statistics.edge_counts);
  write_pair(statistics.source_edge_counts);
  write_pair(statistics.internal_diagonal_counts);
  write_pair(statistics.triangle_counts);
  write_pair(statistics.distinct_rank_counts_x);
  write_pair(statistics.distinct_rank_counts_y);
  write_pair(statistics.distinct_rank_counts_z);
  write_pair(statistics.leaf_counts);
  write_pair(statistics.node_counts);
  write_pair(statistics.hierarchy_heights);
  write_pair(statistics.role_node_tests);
  write_pair(statistics.role_leaf_visits);
  write_pair(statistics.role_primitive_tests);
  write_pair(statistics.role_definite_prunes);
  write_pair(statistics.role_candidates);
  write_pair(statistics.role_emit_node_tests);
  write_pair(statistics.role_emit_primitive_tests);
  writer.u64(statistics.duplicate_candidates);
  writer.u64(statistics.candidate_count);
  writer.u64(statistics.partition_count);
  writer.u64(statistics.maximum_producer_stack);
  writer.u64(statistics.maximum_verifier_queue);
  writer.u64(statistics.producer_work_units);
  writer.u64(statistics.verifier_work_units);
  writer.u64(statistics.persistent_bytes);
  writer.u64(statistics.canonical_bytes);
}

} // namespace broad_phase_codec_detail

template <class T>
std::vector<std::uint8_t> encode_broad_phase_primitive_tables(
    const std::array<broad_phase_primitive_table<T>, 2> &tables) {
  canonical_writer writer;
  writer.u32(0x36504259U); // YBP6
  writer.u16(1);
  for (const auto &table : tables) {
    writer.u8(static_cast<std::uint8_t>(table.operand));
    broad_phase_codec_detail::write_digest(writer, table.predecessor_digest);
    broad_phase_codec_detail::write_digest(writer, table.source_semantic_digest);
    broad_phase_codec_detail::write_digest(writer, table.exact_topology_digest);
    broad_phase_codec_detail::write_digest(writer,
                                           table.geometry_attachment_digest);
    broad_phase_codec_detail::write_digest(writer,
                                           table.precision_attachment_digest);
    writer.u64(table.edges.size());
    for (const auto &edge : table.edges)
      broad_phase_codec_detail::write_edge_primitive(writer, edge);
    writer.u64(table.triangles.size());
    for (const auto &triangle : table.triangles)
      broad_phase_codec_detail::write_triangle_primitive(writer, triangle);
  }
  return writer.take();
}

template <class T>
std::vector<std::uint8_t> encode_triangle_aabb_hierarchy(
    const triangle_aabb_hierarchy<T> &hierarchy) {
  canonical_writer writer;
  writer.u32(0x36484259U); // YBH6
  writer.u16(hierarchy.provider_version);
  writer.u16(hierarchy.layout_version);
  writer.u8(static_cast<std::uint8_t>(hierarchy.operand));
  writer.u32(hierarchy.leaf_capacity);
  writer.u64(hierarchy.spatial_primitives.size());
  for (const auto primitive : hierarchy.spatial_primitives)
    writer.u64(primitive);
  writer.u64(hierarchy.nodes.size());
  for (const auto &node : hierarchy.nodes)
    broad_phase_codec_detail::write_hierarchy_node(writer, node);
  writer.u64(hierarchy.root);
  writer.u64(hierarchy.leaf_count);
  writer.u64(hierarchy.height);
  writer.u32(hierarchy.reserved);
  return writer.take();
}

template <class T>
std::vector<std::uint8_t> encode_broad_phase_candidates(
    const std::vector<broad_phase_count_plan> &count_plans,
    const std::vector<broad_phase_overlap_witness<T>> &witnesses,
    const std::vector<canonical_candidate_record<T>> &candidates) {
  canonical_writer writer;
  writer.u32(0x36434259U); // YBC6
  writer.u16(1);
  writer.u64(count_plans.size());
  for (const auto &plan : count_plans)
    broad_phase_codec_detail::write_count_plan(writer, plan);
  writer.u64(witnesses.size());
  for (const auto &witness : witnesses)
    broad_phase_codec_detail::write_witness(writer, witness);
  writer.u64(candidates.size());
  for (const auto &candidate : candidates)
    broad_phase_codec_detail::write_candidate(writer, candidate);
  return writer.take();
}

inline std::vector<std::uint8_t> encode_broad_phase_partitions(
    const std::vector<canonical_candidate_partition> &partitions) {
  canonical_writer writer;
  writer.u32(0x36524259U); // YBR6
  writer.u16(1);
  writer.u64(partitions.size());
  for (const auto &partition : partitions) {
    writer.u64(partition.id.ordinal());
    writer.u64(partition.ordinal);
    writer.u64(partition.begin);
    writer.u64(partition.count);
    writer.u64(partition.maximum_records);
    writer.u32(partition.reserved);
  }
  return writer.take();
}

inline std::vector<std::uint8_t> encode_broad_phase_evidence(
    const broad_phase_statistics &statistics,
    const broad_phase_verification_evidence &evidence) {
  canonical_writer writer;
  writer.u32(0x36454259U); // YBE6
  writer.u16(evidence.verifier_version);
  writer.u64(evidence.id.ordinal());
  writer.boolean(evidence.primitive_reconstruction_complete);
  writer.boolean(evidence.rank_reconstruction_complete);
  writer.boolean(evidence.hierarchy_reconstruction_complete);
  writer.boolean(evidence.breadth_first_candidate_set_complete);
  writer.boolean(evidence.exhaustive_all_pairs_performed);
  writer.boolean(evidence.exhaustive_all_pairs_complete);
  writer.u64(evidence.breadth_first_candidate_count);
  writer.u64(evidence.exhaustive_candidate_count);
  writer.u64(evidence.verifier_work_units);
  writer.u64(evidence.maximum_queue);
  broad_phase_codec_detail::write_digest(writer, evidence.candidate_set_digest);
  writer.u32(evidence.reserved);
  broad_phase_codec_detail::write_statistics(writer, statistics);
  return writer.take();
}

template <class T, class I>
std::vector<std::uint8_t> encode_canonical_candidate_stream(
    const canonical_candidate_stream<T, I> &artifact) {
  const auto primitive_bytes =
      encode_broad_phase_primitive_tables<T>(artifact.primitive_tables_);
  const auto hierarchy_a =
      encode_triangle_aabb_hierarchy<T>(artifact.hierarchies_[0]);
  const auto hierarchy_b =
      encode_triangle_aabb_hierarchy<T>(artifact.hierarchies_[1]);
  const auto candidate_bytes = encode_broad_phase_candidates<T>(
      artifact.count_plans_, artifact.witnesses_, artifact.candidates_);
  const auto partition_bytes =
      encode_broad_phase_partitions(artifact.partitions_);
  const auto evidence_bytes = encode_broad_phase_evidence(
      artifact.statistics_, artifact.verification_evidence_);

  canonical_writer writer;
  writer.u32(0x36424759U); // YGB6
  writer.u16(artifact.schema_version_);
  writer.u16(artifact.provider_version_);
  writer.u16(artifact.domain_policy_version_);
  writer.u16(artifact.codec_version_);
  writer.u16(artifact.verifier_version_);
  writer.u8(static_cast<std::uint8_t>(artifact.provider_));
  writer.u8(static_cast<std::uint8_t>(artifact.candidate_domain_));
  writer.u8(static_cast<std::uint8_t>(artifact.verification_));
  writer.u32(broad_phase_leaf_capacity_v1);
  writer.u64(broad_phase_partition_capacity_v1);
  broad_phase_codec_detail::write_digest(writer, artifact.predecessor_digest_);
  broad_phase_codec_detail::write_digest(writer, artifact.precision_digest_);
  broad_phase_codec_detail::write_digest(writer,
                                         artifact.primitive_tables_digest_);
  broad_phase_codec_detail::write_digest(writer, artifact.hierarchy_digests_[0]);
  broad_phase_codec_detail::write_digest(writer, artifact.hierarchy_digests_[1]);
  broad_phase_codec_detail::write_digest(writer, artifact.candidate_digest_);
  broad_phase_codec_detail::write_digest(writer, artifact.partition_digest_);
  broad_phase_codec_detail::write_digest(writer, artifact.evidence_digest_);
  if (!writer.sized_bytes(primitive_bytes) ||
      !writer.sized_bytes(hierarchy_a) || !writer.sized_bytes(hierarchy_b) ||
      !writer.sized_bytes(candidate_bytes) ||
      !writer.sized_bytes(partition_bytes) ||
      !writer.sized_bytes(evidence_bytes))
    return {};
  return writer.take();
}

struct broad_phase_codec_access final {
  template <class T, class I>
  static bool refresh(canonical_candidate_stream<T, I> &artifact,
                      bounded_boolean_error &error) {
    const auto primitive_bytes =
        encode_broad_phase_primitive_tables<T>(artifact.primitive_tables_);
    const auto hierarchy_a =
        encode_triangle_aabb_hierarchy<T>(artifact.hierarchies_[0]);
    const auto hierarchy_b =
        encode_triangle_aabb_hierarchy<T>(artifact.hierarchies_[1]);
    const auto candidate_bytes = encode_broad_phase_candidates<T>(
        artifact.count_plans_, artifact.witnesses_, artifact.candidates_);
    const auto partition_bytes =
        encode_broad_phase_partitions(artifact.partitions_);
    artifact.statistics_.canonical_bytes = 0;
    auto evidence_bytes = encode_broad_phase_evidence(
        artifact.statistics_, artifact.verification_evidence_);
    if ((artifact.primitive_tables_[0].edges.size() != 0 ||
         artifact.primitive_tables_[1].edges.size() != 0 ||
         artifact.primitive_tables_[0].triangles.size() != 0 ||
         artifact.primitive_tables_[1].triangles.size() != 0) &&
        primitive_bytes.empty()) {
      error = broad_phase_error(broad_phase_subcode::codec_error,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase primitive encoding failed",
                                broad_phase_checkpoint::canonical_encoding);
      return false;
    }
    artifact.primitive_tables_digest_ = sha256::digest(primitive_bytes);
    artifact.hierarchy_digests_[0] = sha256::digest(hierarchy_a);
    artifact.hierarchy_digests_[1] = sha256::digest(hierarchy_b);
    artifact.candidate_digest_ = sha256::digest(candidate_bytes);
    artifact.partition_digest_ = sha256::digest(partition_bytes);
    artifact.evidence_digest_ = sha256::digest(evidence_bytes);
    artifact.canonical_bytes_ = encode_canonical_candidate_stream(artifact);
    if (artifact.canonical_bytes_.empty()) {
      error = broad_phase_error(broad_phase_subcode::codec_error,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase complete encoding failed",
                                broad_phase_checkpoint::canonical_encoding);
      return false;
    }
    artifact.statistics_.canonical_bytes = artifact.canonical_bytes_.size();
    evidence_bytes = encode_broad_phase_evidence(
        artifact.statistics_, artifact.verification_evidence_);
    artifact.evidence_digest_ = sha256::digest(evidence_bytes);
    artifact.canonical_bytes_ = encode_canonical_candidate_stream(artifact);
    if (artifact.canonical_bytes_.empty() ||
        artifact.canonical_bytes_.size() != artifact.statistics_.canonical_bytes) {
      error = broad_phase_error(broad_phase_subcode::codec_error,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase canonical byte size did not stabilize",
                                broad_phase_checkpoint::canonical_encoding);
      return false;
    }
    artifact.digest_ = sha256::digest(artifact.canonical_bytes_);
    return true;
  }
};

template <class T, class I>
bool verify_broad_phase_codec(
    const canonical_candidate_stream<T, I> &artifact,
    bounded_boolean_error &error) {
  if (encode_broad_phase_primitive_tables<T>(artifact.primitive_tables_) ==
          std::vector<std::uint8_t>{} &&
      (!artifact.primitive_tables_[0].edges.empty() ||
       !artifact.primitive_tables_[1].edges.empty() ||
       !artifact.primitive_tables_[0].triangles.empty() ||
       !artifact.primitive_tables_[1].triangles.empty())) {
    error = broad_phase_error(broad_phase_subcode::codec_error,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase primitive re-encoding failed",
                              broad_phase_checkpoint::codec_digest_resource_reconciliation);
    return false;
  }
  if (sha256::digest(encode_broad_phase_primitive_tables<T>(
          artifact.primitive_tables_)) != artifact.primitive_tables_digest_ ||
      sha256::digest(encode_triangle_aabb_hierarchy<T>(
          artifact.hierarchies_[0])) != artifact.hierarchy_digests_[0] ||
      sha256::digest(encode_triangle_aabb_hierarchy<T>(
          artifact.hierarchies_[1])) != artifact.hierarchy_digests_[1] ||
      sha256::digest(encode_broad_phase_candidates<T>(
          artifact.count_plans_, artifact.witnesses_, artifact.candidates_)) !=
          artifact.candidate_digest_ ||
      sha256::digest(encode_broad_phase_partitions(artifact.partitions_)) !=
          artifact.partition_digest_ ||
      sha256::digest(encode_broad_phase_evidence(
          artifact.statistics_, artifact.verification_evidence_)) !=
          artifact.evidence_digest_ ||
      encode_canonical_candidate_stream(artifact) != artifact.canonical_bytes_ ||
      sha256::digest(artifact.canonical_bytes_) != artifact.digest_) {
    error = broad_phase_error(broad_phase_subcode::digest_mismatch,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase canonical bytes or digest mismatch",
                              broad_phase_checkpoint::codec_digest_resource_reconciliation);
    return false;
  }
  return true;
}

struct canonical_candidate_stream_envelope final {
  std::uint16_t schema_version = 0;
  std::uint16_t provider_version = 0;
  std::uint16_t domain_policy_version = 0;
  std::uint16_t codec_version = 0;
  std::uint16_t verifier_version = 0;
  broad_phase_provider_kind provider =
      broad_phase_provider_kind::rank_morton_triangle_aabb_hierarchy_v1;
  candidate_domain_kind candidate_domain =
      candidate_domain_kind::all_canonical_edges_against_all_opposite_source_triangles_v1;
  broad_phase_verification_disposition verification =
      broad_phase_verification_disposition::unverified;
  bounded_boolean_digest predecessor_digest{};
  bounded_boolean_digest precision_digest{};
  bounded_boolean_digest primitive_tables_digest{};
  std::array<bounded_boolean_digest, 2> hierarchy_digests{};
  bounded_boolean_digest candidate_digest{};
  bounded_boolean_digest partition_digest{};
  bounded_boolean_digest evidence_digest{};
  std::array<std::vector<std::uint8_t>, 6> sections{};
};

inline bool parse_canonical_candidate_stream_envelope(
    const std::vector<std::uint8_t> &bytes, std::uint64_t maximum_bytes,
    canonical_candidate_stream_envelope &envelope,
    bounded_boolean_error &error) {
  envelope = canonical_candidate_stream_envelope{};
  if (bytes.size() > maximum_bytes) {
    error = broad_phase_error(broad_phase_subcode::resource_preflight,
                              bounded_boolean_error_category::resource_limit,
                              "broad-phase encoded artifact exceeds configured limit",
                              broad_phase_checkpoint::canonical_encoding);
    return false;
  }
  canonical_reader reader(bytes);
  std::uint32_t magic = 0;
  std::uint8_t provider = 0, domain = 0, verification = 0;
  std::uint32_t leaf_capacity = 0;
  std::uint64_t partition_capacity = 0;
  if (!reader.u32(magic) || magic != 0x36424759U ||
      !reader.u16(envelope.schema_version) ||
      !reader.u16(envelope.provider_version) ||
      !reader.u16(envelope.domain_policy_version) ||
      !reader.u16(envelope.codec_version) ||
      !reader.u16(envelope.verifier_version) || !reader.u8(provider) ||
      !reader.u8(domain) || !reader.u8(verification) ||
      !reader.u32(leaf_capacity) || !reader.u64(partition_capacity) ||
      !broad_phase_codec_detail::read_digest(reader, envelope.predecessor_digest) ||
      !broad_phase_codec_detail::read_digest(reader, envelope.precision_digest) ||
      !broad_phase_codec_detail::read_digest(reader, envelope.primitive_tables_digest) ||
      !broad_phase_codec_detail::read_digest(reader, envelope.hierarchy_digests[0]) ||
      !broad_phase_codec_detail::read_digest(reader, envelope.hierarchy_digests[1]) ||
      !broad_phase_codec_detail::read_digest(reader, envelope.candidate_digest) ||
      !broad_phase_codec_detail::read_digest(reader, envelope.partition_digest) ||
      !broad_phase_codec_detail::read_digest(reader, envelope.evidence_digest)) {
    error = broad_phase_error(broad_phase_subcode::codec_error,
                              bounded_boolean_error_category::input_contract_error,
                              "broad-phase encoded artifact header is truncated",
                              broad_phase_checkpoint::canonical_encoding);
    return false;
  }
  envelope.provider = static_cast<broad_phase_provider_kind>(provider);
  envelope.candidate_domain = static_cast<candidate_domain_kind>(domain);
  envelope.verification =
      static_cast<broad_phase_verification_disposition>(verification);
  if (envelope.schema_version != 1 || envelope.provider_version != 1 ||
      envelope.domain_policy_version != 1 || envelope.codec_version != 1 ||
      envelope.verifier_version != 1 ||
      envelope.provider != broad_phase_provider_kind::rank_morton_triangle_aabb_hierarchy_v1 ||
      envelope.candidate_domain != candidate_domain_kind::all_canonical_edges_against_all_opposite_source_triangles_v1 ||
      envelope.verification != broad_phase_verification_disposition::independently_verified ||
      leaf_capacity != broad_phase_leaf_capacity_v1 ||
      partition_capacity != broad_phase_partition_capacity_v1) {
    error = broad_phase_error(broad_phase_subcode::unsupported_version,
                              bounded_boolean_error_category::input_contract_error,
                              "broad-phase encoded artifact uses an unsupported contract",
                              broad_phase_checkpoint::canonical_encoding);
    return false;
  }
  for (auto &section : envelope.sections)
    if (!reader.sized_bytes(section, maximum_bytes)) {
      error = broad_phase_error(broad_phase_subcode::codec_error,
                                bounded_boolean_error_category::input_contract_error,
                                "broad-phase encoded artifact section is malformed",
                                broad_phase_checkpoint::canonical_encoding);
      return false;
    }
  if (!reader.complete() ||
      !broad_phase_codec_detail::validate_section_header(envelope.sections[0], 0x36504259U, 1) ||
      !broad_phase_codec_detail::validate_section_header(envelope.sections[1], 0x36484259U, 1) ||
      !broad_phase_codec_detail::validate_section_header(envelope.sections[2], 0x36484259U, 1) ||
      !broad_phase_codec_detail::validate_section_header(envelope.sections[3], 0x36434259U, 1) ||
      !broad_phase_codec_detail::validate_section_header(envelope.sections[4], 0x36524259U, 1) ||
      !broad_phase_codec_detail::validate_section_header(envelope.sections[5], 0x36454259U, 1) ||
      sha256::digest(envelope.sections[0]) != envelope.primitive_tables_digest ||
      sha256::digest(envelope.sections[1]) != envelope.hierarchy_digests[0] ||
      sha256::digest(envelope.sections[2]) != envelope.hierarchy_digests[1] ||
      sha256::digest(envelope.sections[3]) != envelope.candidate_digest ||
      sha256::digest(envelope.sections[4]) != envelope.partition_digest ||
      sha256::digest(envelope.sections[5]) != envelope.evidence_digest) {
    error = broad_phase_error(broad_phase_subcode::digest_mismatch,
                              bounded_boolean_error_category::input_contract_error,
                              "broad-phase encoded artifact section verification failed",
                              broad_phase_checkpoint::canonical_encoding);
    return false;
  }
  return true;
}


} // namespace ygor::mesh_boolean::bounded
