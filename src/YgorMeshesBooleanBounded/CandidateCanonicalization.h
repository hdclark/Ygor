#pragma once

#include "CanonicalCandidateStream.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T>
bool canonicalize_broad_phase_candidates(
    std::vector<broad_phase_candidate_discovery<T>> discoveries,
    const std::array<broad_phase_primitive_table<T>, 2> &primitive_tables,
    const std::array<triangle_aabb_hierarchy<T>, 2> &hierarchies,
    const broad_phase_capabilities &capabilities,
    std::vector<broad_phase_overlap_witness<T>> &witnesses,
    std::vector<canonical_candidate_record<T>> &candidates,
    std::vector<canonical_candidate_partition> &partitions,
    broad_phase_statistics &statistics,
    bounded_boolean_error &error) {
  for (std::uint64_t ordinal = 0; ordinal < discoveries.size(); ++ordinal) {
    const auto &record = discoveries[ordinal];
    if (!valid_directed_candidate_role(record.role) ||
        record.family !=
            broad_phase_relation_family::canonical_edge_source_triangle ||
        record.key.role != record.role || record.key.family != record.family ||
        record.domain_policy_version != capabilities.domain_policy_version ||
        record.provider_version != capabilities.provider_version ||
        record.reserved != 0 || record.witness.reserved != 0 ||
        record.witness.filter_reason != topological_filter_reason::not_filtered ||
        record.witness.role != record.role ||
        record.witness.edge != record.edge ||
        record.witness.triangle != record.triangle ||
        record.witness.key.candidate != record.key) {
      error = broad_phase_error(broad_phase_subcode::malformed_overlap_witness,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase candidate or overlap witness is malformed",
                                broad_phase_checkpoint::candidate_witness_validation);
      error.witnesses[0] = ordinal;
      error.witness_count = 1;
      return false;
    }
    const auto edge_slot = operand_slot(role_edge_operand(record.role));
    const auto triangle_slot = operand_slot(role_triangle_operand(record.role));
    if (record.edge.ordinal() >= primitive_tables[edge_slot].edges.size() ||
        record.triangle.ordinal() >=
            primitive_tables[triangle_slot].triangles.size()) {
      error = broad_phase_error(broad_phase_subcode::candidate_key_mismatch,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase candidate references an invalid primitive",
                                broad_phase_checkpoint::candidate_witness_validation);
      return false;
    }
    const auto &edge = primitive_tables[edge_slot].edges[record.edge.ordinal()];
    const auto &triangle =
        primitive_tables[triangle_slot].triangles[record.triangle.ordinal()];
    canonical_candidate_key expected;
    expected.role = record.role;
    expected.family =
        broad_phase_relation_family::canonical_edge_source_triangle;
    expected.edge = edge.semantic_key;
    expected.triangle = triangle.semantic_key;
    expected.edge_class = edge.edge_class;
    expected.domain_policy_version = capabilities.domain_policy_version;
    if (!(expected == record.key) || record.edge_class != edge.edge_class ||
        record.key.edge_class != edge.edge_class ||
        !candidate_role_matches_operands(record.role, edge.operand,
                                         triangle.operand)) {
      error = broad_phase_error(broad_phase_subcode::candidate_key_mismatch,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase candidate semantic key mismatch",
                                broad_phase_checkpoint::candidate_witness_validation);
      return false;
    }
    const auto &hierarchy = hierarchies[triangle_slot];
    const auto leaf_id = record.witness.admitting_leaf.ordinal();
    if (leaf_id >= hierarchy.nodes.size() ||
        hierarchy.nodes[leaf_id].kind != hierarchy_node_kind::leaf ||
        record.witness.triangle_spatial_slot >=
            hierarchy.spatial_primitives.size() ||
        hierarchy.spatial_primitives[record.witness.triangle_spatial_slot] !=
            record.triangle.ordinal()) {
      error = broad_phase_error(broad_phase_subcode::malformed_overlap_witness,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase overlap witness does not identify its triangle",
                                broad_phase_checkpoint::candidate_witness_validation);
      return false;
    }
    const auto &leaf = hierarchy.nodes[leaf_id];
    const auto leaf_end =
        leaf.first_spatial_primitive + leaf.subtree_primitive_count;
    if (leaf_end < leaf.first_spatial_primitive ||
        record.witness.triangle_spatial_slot < leaf.first_spatial_primitive ||
        record.witness.triangle_spatial_slot >= leaf_end) {
      error = broad_phase_error(broad_phase_subcode::malformed_overlap_witness,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase overlap witness slot is outside its leaf",
                                broad_phase_checkpoint::candidate_witness_validation);
      return false;
    }
    const auto relation = classify_closed_bound_relation(edge.bound, triangle.bound);
    if (relation.definitely_separated) {
      error = broad_phase_error(
          broad_phase_subcode::definitely_separated_pair_emitted,
          bounded_boolean_error_category::internal_invariant_error,
          "broad-phase emitted a definitely separated pair",
          broad_phase_checkpoint::candidate_witness_validation);
      return false;
    }
    if (relation.axes != record.witness.relation.axes ||
        relation.definitely_separated !=
            record.witness.relation.definitely_separated ||
        record.witness.key.axes != relation.axes ||
        record.witness.key.admitting_leaf != leaf_id ||
        record.witness.key.triangle_spatial_slot !=
            record.witness.triangle_spatial_slot) {
      error = broad_phase_error(broad_phase_subcode::malformed_overlap_witness,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase overlap witness comparison evidence mismatch",
                                broad_phase_checkpoint::candidate_witness_validation);
      return false;
    }
  }

  std::sort(discoveries.begin(), discoveries.end(),
            [](const auto &a, const auto &b) {
              return std::tie(a.key, a.witness.key) <
                     std::tie(b.key, b.witness.key);
            });
  for (std::size_t i = 1; i < discoveries.size(); ++i) {
    if (discoveries[i - 1].key == discoveries[i].key) {
      statistics.duplicate_candidates += 1;
      error = broad_phase_error(broad_phase_subcode::duplicate_candidate_key,
                                bounded_boolean_error_category::internal_invariant_error,
                                "V1 broad-phase discovered a duplicate semantic candidate",
                                broad_phase_checkpoint::complete_key_canonicalization);
      error.witnesses[0] = i - 1;
      error.witnesses[1] = i;
      error.witness_count = 2;
      return false;
    }
  }

  try {
    witnesses.clear();
    candidates.clear();
    partitions.clear();
    witnesses.reserve(discoveries.size());
    candidates.reserve(discoveries.size());
  } catch (...) {
    error = broad_phase_error(broad_phase_subcode::resource_preflight,
                              bounded_boolean_error_category::resource_limit,
                              "broad-phase canonical candidate allocation failed",
                              broad_phase_checkpoint::prefix_and_candidate_reservation);
    return false;
  }

  for (std::uint64_t ordinal = 0; ordinal < discoveries.size(); ++ordinal) {
    auto witness = discoveries[ordinal].witness;
    witness.id = overlap_witness_id{ordinal};
    canonical_candidate_record<T> candidate;
    candidate.id = candidate_id{ordinal};
    candidate.ordinal = ordinal;
    candidate.key = discoveries[ordinal].key;
    candidate.role = discoveries[ordinal].role;
    candidate.family = discoveries[ordinal].family;
    candidate.edge = discoveries[ordinal].edge;
    candidate.triangle = discoveries[ordinal].triangle;
    candidate.edge_class = discoveries[ordinal].edge_class;
    candidate.witness = witness.id;
    candidate.filter_reason = topological_filter_reason::not_filtered;
    candidate.domain_policy_version = capabilities.domain_policy_version;
    candidate.provider_version = capabilities.provider_version;
    witnesses.push_back(std::move(witness));
    candidates.push_back(std::move(candidate));
  }

  const std::uint64_t partition_count =
      candidates.empty()
          ? 0
          : (static_cast<std::uint64_t>(candidates.size()) +
             capabilities.partition_capacity - 1) /
                capabilities.partition_capacity;
  try {
    partitions.reserve(static_cast<std::size_t>(partition_count));
  } catch (...) {
    error = broad_phase_error(broad_phase_subcode::resource_preflight,
                              bounded_boolean_error_category::resource_limit,
                              "broad-phase partition allocation failed",
                              broad_phase_checkpoint::partition_construction);
    return false;
  }
  for (std::uint64_t ordinal = 0; ordinal < partition_count; ++ordinal) {
    canonical_candidate_partition partition;
    partition.id = candidate_partition_id{ordinal};
    partition.ordinal = ordinal;
    if (!checked_multiply<std::uint64_t>(ordinal,
                                          capabilities.partition_capacity,
                                          partition.begin)) {
      error = broad_phase_error(broad_phase_subcode::count_overflow,
                                bounded_boolean_error_category::index_overflow,
                                "broad-phase partition offset overflow",
                                broad_phase_checkpoint::partition_construction);
      return false;
    }
    partition.count = std::min<std::uint64_t>(
        capabilities.partition_capacity,
        static_cast<std::uint64_t>(candidates.size()) - partition.begin);
    partition.maximum_records = capabilities.partition_capacity;
    partitions.push_back(partition);
  }
  statistics.candidate_count = candidates.size();
  statistics.partition_count = partitions.size();
  return true;
}

} // namespace ygor::mesh_boolean::bounded
