#pragma once

#include "BroadPhaseCodec.h"
#include "BroadPhasePreflight.h"
#include "PrecisionContext.h"
#include "RankMortonKey.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <deque>
#include <numeric>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {
namespace broad_phase_verifier_detail {

template <class T, class I>
bool reconstruct_primitive_table(
    const canonical_halfedge_operand<T, I> &source,
    const context_owner_token &owner,
    broad_phase_primitive_table<T> &out) {
  if (!owner.anchor || !source.owner().same_owner(owner) ||
      source.verification() !=
          canonical_halfedge_verification_disposition::independently_verified)
    return false;
  out = broad_phase_primitive_table<T>{};
  out.operand = source.operand();
  out.predecessor_digest = source.digest();
  out.source_semantic_digest = source.source_semantic_digest();
  out.exact_topology_digest = source.exact_topology_digest();
  out.geometry_attachment_digest = source.geometry_attachment_digest();
  out.precision_attachment_digest = source.precision_attachment_digest();
  try {
    out.edges.reserve(source.edges().size());
    out.triangles.reserve(source.triangles().size());
  } catch (...) {
    return false;
  }

  for (std::uint64_t ordinal = 0; ordinal < source.edges().size(); ++ordinal) {
    const auto *edge = source.edge(manifold_edge_id{ordinal}, owner);
    if (!edge || edge->canonical_id != ordinal || !edge->bound.valid() ||
        edge->key.operand != source.operand() ||
        edge->endpoints[0] >= source.vertices().size() ||
        edge->endpoints[1] >= source.vertices().size() ||
        edge->representative >= source.halfedges().size() ||
        edge->halfedges[0] >= source.halfedges().size() ||
        edge->halfedges[1] >= source.halfedges().size() ||
        edge->triangles[0] >= source.triangles().size() ||
        edge->triangles[1] >= source.triangles().size())
      return false;
    const auto *v0 = source.vertex(manifold_vertex_id{edge->endpoints[0]}, owner);
    const auto *v1 = source.vertex(manifold_vertex_id{edge->endpoints[1]}, owner);
    const auto *representative =
        source.halfedge(manifold_halfedge_id{edge->representative}, owner);
    const auto *h0 = source.halfedge(manifold_halfedge_id{edge->halfedges[0]}, owner);
    const auto *h1 = source.halfedge(manifold_halfedge_id{edge->halfedges[1]}, owner);
    if (!v0 || !v1 || !representative || !h0 || !h1 ||
        representative->edge != ordinal || h0->pair != edge->halfedges[1] ||
        h1->pair != edge->halfedges[0] ||
        !exact_bound_equal(canonical_bound_hull(v0->bound, v1->bound),
                           edge->bound))
      return false;
    const auto role = source.operand() == operand_id::a
                          ? directed_candidate_role::a_edge_b_triangle
                          : directed_candidate_role::b_edge_a_triangle;
    if (candidate_domain_v1(role, edge->edge_class).disposition !=
        candidate_domain_disposition::included)
      return false;

    broad_phase_edge_primitive<T> primitive;
    primitive.id = broad_phase_edge_primitive_id{ordinal};
    primitive.ordinal = ordinal;
    primitive.operand = source.operand();
    primitive.edge = manifold_edge_id{ordinal};
    primitive.semantic_key = edge->key;
    primitive.edge_class = edge->edge_class;
    primitive.representative = manifold_halfedge_id{edge->representative};
    primitive.halfedges = {manifold_halfedge_id{edge->halfedges[0]},
                           manifold_halfedge_id{edge->halfedges[1]}};
    primitive.endpoints = {manifold_vertex_id{edge->endpoints[0]},
                           manifold_vertex_id{edge->endpoints[1]}};
    primitive.incident_triangles = {
        manifold_triangle_id{edge->triangles[0]},
        manifold_triangle_id{edge->triangles[1]}};
    primitive.source_facets = edge->facets;
    primitive.source_undirected_edge = edge->source_undirected_edge;
    primitive.source_directed_uses = edge->source_directed_uses;
    primitive.source_facet = edge->source_facet;
    primitive.source_diagonal = edge->source_diagonal;
    primitive.source_feature_owner = edge->source_feature_owner;
    primitive.symbolic_contact_owner = edge->symbolic_contact_owner;
    primitive.classification_barrier_inside_source_facet =
        edge->classification_barrier_inside_source_facet;
    primitive.retained_surface_feature = edge->retained_surface_feature;
    primitive.bound = edge->bound;
    primitive.endpoint_bits = canonical_bound_endpoint_bits(edge->bound);
    primitive.geometry_attachment_digest = source.geometry_attachment_digest();
    primitive.precision_attachment_digest = source.precision_attachment_digest();
    primitive.inclusion = candidate_domain_disposition::included;
    out.edges.push_back(std::move(primitive));
  }

  for (std::uint64_t ordinal = 0; ordinal < source.triangles().size(); ++ordinal) {
    const auto *triangle = source.triangle(manifold_triangle_id{ordinal}, owner);
    if (!triangle || triangle->canonical_id != ordinal || !triangle->bound.valid() ||
        triangle->key.operand != source.operand())
      return false;
    canonical_bound3<T> expected;
    bool initialized = false;
    for (std::size_t slot = 0; slot < 3; ++slot) {
      if (triangle->vertices[slot] >= source.vertices().size() ||
          triangle->halfedges[slot] >= source.halfedges().size())
        return false;
      const auto *vertex =
          source.vertex(manifold_vertex_id{triangle->vertices[slot]}, owner);
      if (!vertex || !vertex->bound.valid())
        return false;
      expected = initialized ? canonical_bound_hull(expected, vertex->bound)
                             : vertex->bound;
      initialized = true;
    }
    if (!initialized || !exact_bound_equal(expected, triangle->bound))
      return false;

    broad_phase_triangle_primitive<T> primitive;
    primitive.id = broad_phase_triangle_primitive_id{ordinal};
    primitive.ordinal = ordinal;
    primitive.operand = source.operand();
    primitive.triangle = manifold_triangle_id{ordinal};
    primitive.semantic_key = triangle->key;
    for (std::size_t slot = 0; slot < 3; ++slot) {
      primitive.vertices[slot] = manifold_vertex_id{triangle->vertices[slot]};
      primitive.halfedges[slot] = manifold_halfedge_id{triangle->halfedges[slot]};
    }
    primitive.source_triangle = triangle->source_triangle;
    primitive.source_facet = triangle->source_facet;
    primitive.ring = triangle->ring;
    primitive.shell = triangle->shell;
    primitive.facet_group = triangle->facet_group;
    primitive.shell_group = triangle->shell_group;
    primitive.basis = triangle->basis;
    primitive.orientation = triangle->orientation;
    primitive.bound = triangle->bound;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      primitive.axis_keys[axis].lower =
          finite_total_order_key(triangle->bound.axes[axis].lower());
      primitive.axis_keys[axis].upper =
          finite_total_order_key(triangle->bound.axes[axis].upper());
    }
    primitive.geometry_attachment_digest = source.geometry_attachment_digest();
    primitive.precision_attachment_digest = source.precision_attachment_digest();
    out.triangles.push_back(std::move(primitive));
  }
  return true;
}

inline rank_morton_key slow_interleave(
    const std::array<std::uint64_t, 3> &ranks,
    std::uint16_t width) {
  rank_morton_key result;
  result.active_rank_bits = width;
  const std::size_t bit_count = static_cast<std::size_t>(width) * 3;
  result.words.assign((bit_count + 63) / 64, 0);
  std::size_t destination = 0;
  for (std::uint16_t bit = width; bit > 0; --bit) {
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const bool set = (ranks[axis] & (std::uint64_t{1} << (bit - 1))) != 0;
      if (set) {
        const auto word = destination / 64;
        const auto offset = static_cast<unsigned>(63 - (destination % 64));
        result.words[word] |= std::uint64_t{1} << offset;
      }
      ++destination;
    }
  }
  return result;
}

template <class T>
bool reconstruct_ranks_and_order(
    std::vector<broad_phase_triangle_primitive<T>> &triangles,
    std::array<std::uint64_t, 3> &distinct,
    std::vector<std::uint64_t> &spatial_order) {
  distinct = {};
  spatial_order.clear();
  if (triangles.empty())
    return true;
  std::uint64_t maximum_rank = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    std::vector<std::pair<axis_endpoint_key<T>, std::uint64_t>> ordered;
    try {
      ordered.reserve(triangles.size());
    } catch (...) {
      return false;
    }
    for (std::uint64_t ordinal = 0; ordinal < triangles.size(); ++ordinal)
      ordered.push_back({triangles[ordinal].axis_keys[axis], ordinal});
    std::sort(ordered.begin(), ordered.end(), [&](const auto &a, const auto &b) {
      if (a.first != b.first)
        return a.first < b.first;
      return triangles[a.second].semantic_key < triangles[b.second].semantic_key;
    });
    std::uint64_t rank = 0;
    for (std::size_t position = 0; position < ordered.size(); ++position) {
      if (position != 0 && ordered[position - 1].first != ordered[position].first)
        ++rank;
      triangles[ordered[position].second].dense_ranks[axis] = rank;
    }
    distinct[axis] = rank + 1;
    maximum_rank = std::max(maximum_rank, rank);
  }
  const auto width = broad_phase_rank_bit_width(maximum_rank);
  for (auto &triangle : triangles)
    triangle.morton = slow_interleave(triangle.dense_ranks, width);
  try {
    spatial_order.resize(triangles.size());
  } catch (...) {
    return false;
  }
  std::iota(spatial_order.begin(), spatial_order.end(), std::uint64_t{0});
  std::sort(spatial_order.begin(), spatial_order.end(), [&](auto a, auto b) {
    const auto &left = triangles[a];
    const auto &right = triangles[b];
    return std::tie(left.morton, left.axis_keys[0], left.axis_keys[1],
                    left.axis_keys[2], left.semantic_key) <
           std::tie(right.morton, right.axis_keys[0], right.axis_keys[1],
                    right.axis_keys[2], right.semantic_key);
  });
  for (std::uint64_t position = 0; position < spatial_order.size(); ++position)
    triangles[spatial_order[position]].spatial_ordinal = position;
  return true;
}

template <class T>
bool reconstruct_hierarchy(
    operand_id operand,
    const std::vector<broad_phase_triangle_primitive<T>> &triangles,
    const std::vector<std::uint64_t> &spatial_order,
    triangle_aabb_hierarchy<T> &out) {
  out = triangle_aabb_hierarchy<T>{};
  out.operand = operand;
  if (triangles.empty())
    return spatial_order.empty();
  if (triangles.size() != spatial_order.size())
    return false;
  std::deque<std::uint64_t> current;
  for (std::uint64_t begin = 0; begin < triangles.size();
       begin += broad_phase_leaf_capacity_v1) {
    const auto count = std::min<std::uint64_t>(
        broad_phase_leaf_capacity_v1,
        static_cast<std::uint64_t>(triangles.size()) - begin);
    canonical_bound3<T> bound;
    bool initialized = false;
    for (std::uint64_t offset = 0; offset < count; ++offset) {
      const auto primitive = spatial_order[begin + offset];
      if (primitive >= triangles.size())
        return false;
      bound = initialized ? canonical_bound_hull(bound, triangles[primitive].bound)
                          : triangles[primitive].bound;
      initialized = true;
    }
    triangle_aabb_hierarchy_node<T> leaf;
    leaf.id = broad_phase_node_id{out.nodes.size()};
    leaf.ordinal = out.nodes.size();
    leaf.operand = operand;
    leaf.kind = hierarchy_node_kind::leaf;
    leaf.bound = bound;
    leaf.first_spatial_primitive = begin;
    leaf.subtree_primitive_count = count;
    leaf.level_from_leaves = 0;
    leaf.height = 1;
    leaf.key = {operand, begin, count, hierarchy_node_kind::leaf, 0, 1};
    out.nodes.push_back(leaf);
    current.push_back(leaf.ordinal);
  }
  out.spatial_primitives = spatial_order;
  out.leaf_count = current.size();
  while (current.size() > 1) {
    std::deque<std::uint64_t> next;
    while (!current.empty()) {
      const auto left_id = current.front();
      current.pop_front();
      if (current.empty()) {
        next.push_back(left_id);
        break;
      }
      const auto right_id = current.front();
      current.pop_front();
      const auto &left = out.nodes[left_id];
      const auto &right = out.nodes[right_id];
      if (left.first_spatial_primitive + left.subtree_primitive_count !=
          right.first_spatial_primitive)
        return false;
      triangle_aabb_hierarchy_node<T> parent;
      parent.id = broad_phase_node_id{out.nodes.size()};
      parent.ordinal = out.nodes.size();
      parent.operand = operand;
      parent.kind = hierarchy_node_kind::internal;
      parent.bound = canonical_bound_hull(left.bound, right.bound);
      parent.first_spatial_primitive = left.first_spatial_primitive;
      parent.subtree_primitive_count =
          left.subtree_primitive_count + right.subtree_primitive_count;
      parent.level_from_leaves =
          std::max(left.level_from_leaves, right.level_from_leaves) + 1;
      parent.height = std::max(left.height, right.height) + 1;
      parent.left = broad_phase_node_id{left_id};
      parent.right = broad_phase_node_id{right_id};
      parent.key = {operand, parent.first_spatial_primitive,
                    parent.subtree_primitive_count,
                    hierarchy_node_kind::internal,
                    parent.level_from_leaves, 1};
      out.nodes.push_back(parent);
      next.push_back(parent.ordinal);
    }
    current.swap(next);
  }
  out.root = current.front();
  out.height = out.nodes[out.root].height;
  return true;
}

template <class T>
void append_candidate_key_digest(canonical_writer &writer,
                                 const canonical_candidate_key &key) {
  broad_phase_codec_detail::write_candidate_key(writer, key);
}

template <class T>
bool collect_breadth_first_candidates(
    directed_candidate_role role,
    const broad_phase_primitive_table<T> &edge_table,
    const broad_phase_primitive_table<T> &triangle_table,
    const triangle_aabb_hierarchy<T> &hierarchy,
    std::vector<canonical_candidate_key> &keys,
    std::uint64_t &work,
    std::uint64_t &maximum_queue) {
  if (!candidate_role_matches_operands(role, edge_table.operand,
                                       triangle_table.operand) ||
      triangle_table.operand != hierarchy.operand)
    return false;
  for (const auto &edge : edge_table.edges) {
    if (hierarchy.empty())
      continue;
    std::deque<std::uint64_t> queue;
    queue.push_back(hierarchy.root);
    while (!queue.empty()) {
      maximum_queue = std::max<std::uint64_t>(maximum_queue, queue.size());
      const auto node_id = queue.front();
      queue.pop_front();
      if (node_id >= hierarchy.nodes.size())
        return false;
      ++work;
      const auto &node = hierarchy.nodes[node_id];
      if (classify_closed_bound_relation(edge.bound, node.bound)
              .definitely_separated)
        continue;
      if (node.kind == hierarchy_node_kind::internal) {
        if (node.left.ordinal() >= node_id || node.right.ordinal() >= node_id)
          return false;
        queue.push_back(node.left.ordinal());
        queue.push_back(node.right.ordinal());
        continue;
      }
      const auto end =
          node.first_spatial_primitive + node.subtree_primitive_count;
      if (end > hierarchy.spatial_primitives.size())
        return false;
      for (std::uint64_t position = node.first_spatial_primitive; position < end;
           ++position) {
        ++work;
        const auto triangle_id = hierarchy.spatial_primitives[position];
        if (triangle_id >= triangle_table.triangles.size())
          return false;
        const auto &triangle = triangle_table.triangles[triangle_id];
        if (classify_closed_bound_relation(edge.bound, triangle.bound)
                .definitely_separated)
          continue;
        canonical_candidate_key key;
        key.role = role;
        key.family =
            broad_phase_relation_family::canonical_edge_source_triangle;
        key.edge = edge.semantic_key;
        key.triangle = triangle.semantic_key;
        key.edge_class = edge.edge_class;
        key.domain_policy_version = 1;
        keys.push_back(std::move(key));
      }
    }
  }
  return true;
}

template <class T>
bool collect_all_pairs_candidates(
    directed_candidate_role role,
    const broad_phase_primitive_table<T> &edges,
    const broad_phase_primitive_table<T> &triangles,
    std::vector<canonical_candidate_key> &keys,
    std::uint64_t &work) {
  for (const auto &edge : edges.edges) {
    for (const auto &triangle : triangles.triangles) {
      ++work;
      if (classify_closed_bound_relation(edge.bound, triangle.bound)
              .definitely_separated)
        continue;
      canonical_candidate_key key;
      key.role = role;
      key.family = broad_phase_relation_family::canonical_edge_source_triangle;
      key.edge = edge.semantic_key;
      key.triangle = triangle.semantic_key;
      key.edge_class = edge.edge_class;
      key.domain_policy_version = 1;
      keys.push_back(std::move(key));
    }
  }
  return true;
}

inline bool validate_partitions(
    const std::vector<canonical_candidate_partition> &partitions,
    std::uint64_t candidate_count, std::uint64_t capacity) {
  if (candidate_count == 0)
    return partitions.empty();
  const auto expected_count = (candidate_count + capacity - 1) / capacity;
  if (partitions.size() != expected_count)
    return false;
  std::uint64_t cursor = 0;
  for (std::uint64_t ordinal = 0; ordinal < partitions.size(); ++ordinal) {
    const auto &partition = partitions[ordinal];
    if (partition.id.ordinal() != ordinal || partition.ordinal != ordinal ||
        partition.begin != cursor || partition.maximum_records != capacity ||
        partition.count == 0 || partition.count > capacity ||
        partition.reserved != 0)
      return false;
    cursor += partition.count;
  }
  return cursor == candidate_count;
}

} // namespace broad_phase_verifier_detail

struct broad_phase_verifier_access final {
  template <class T, class I>
  static bool set_evidence(canonical_candidate_stream<T, I> &artifact,
                           const broad_phase_verification_evidence &evidence) {
    artifact.verification_evidence_ = evidence;
    artifact.verification_ =
        broad_phase_verification_disposition::independently_verified;
    return true;
  }
};

template <class T, class I>
bool verify_canonical_candidate_stream(
    const canonical_candidate_stream<T, I> &artifact,
    const boolean_context<T, I> &context,
    const precision_context<T> &precision,
    const broad_phase_capabilities &capabilities,
    broad_phase_verification_evidence &evidence,
    bounded_boolean_error &error,
    bool verify_codec = true) {
  evidence = broad_phase_verification_evidence{};
  evidence.id = broad_phase_verifier_evidence_id{0};
  evidence.verifier_version = capabilities.verifier_version;
  if (!broad_phase_capabilities_valid(capabilities) ||
      artifact.schema_version() != 1 || artifact.provider_version() != 1 ||
      artifact.domain_policy_version() != 1 || artifact.codec_version() != 1 ||
      artifact.verifier_version() != 1 ||
      artifact.provider() !=
          broad_phase_provider_kind::rank_morton_triangle_aabb_hierarchy_v1 ||
      artifact.candidate_domain() !=
          candidate_domain_kind::all_canonical_edges_against_all_opposite_source_triangles_v1 ||
      !context.owner.same_owner(capabilities.owner) ||
      !precision.owner().same_owner(capabilities.owner) ||
      !artifact.owner().same_owner(capabilities.owner) || !artifact.manifolds() ||
      !artifact.manifolds()->owner().same_owner(capabilities.owner) ||
      artifact.predecessor_digest() != artifact.manifolds()->digest() ||
      artifact.precision_digest() != precision.digest()) {
    error = broad_phase_error(broad_phase_subcode::unsupported_version,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase verifier rejected artifact header or owner",
                              broad_phase_checkpoint::independent_provider_reconstruction);
    return false;
  }

  std::array<broad_phase_primitive_table<T>, 2> expected_tables;
  if (!broad_phase_verifier_detail::reconstruct_primitive_table(
          *artifact.manifolds()->a(), capabilities.owner, expected_tables[0]) ||
      !broad_phase_verifier_detail::reconstruct_primitive_table(
          *artifact.manifolds()->b(), capabilities.owner, expected_tables[1])) {
    error = broad_phase_error(broad_phase_subcode::independent_provider_mismatch,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase verifier could not reconstruct primitive tables",
                              broad_phase_checkpoint::independent_provider_reconstruction);
    return false;
  }
  evidence.primitive_reconstruction_complete = true;

  std::array<std::vector<std::uint64_t>, 2> expected_spatial;
  std::array<std::array<std::uint64_t, 3>, 2> distinct{};
  for (std::size_t slot = 0; slot < 2; ++slot) {
    if (!broad_phase_verifier_detail::reconstruct_ranks_and_order(
            expected_tables[slot].triangles, distinct[slot],
            expected_spatial[slot])) {
      error = broad_phase_error(broad_phase_subcode::independent_provider_mismatch,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase verifier could not reconstruct rank-Morton order",
                                broad_phase_checkpoint::independent_provider_reconstruction);
      return false;
    }
  }
  evidence.rank_reconstruction_complete = true;
  if (encode_broad_phase_primitive_tables(expected_tables) !=
      encode_broad_phase_primitive_tables(
          std::array<broad_phase_primitive_table<T>, 2>{
              artifact.primitive_table(operand_id::a),
              artifact.primitive_table(operand_id::b)})) {
    error = broad_phase_error(broad_phase_subcode::independent_provider_mismatch,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase primitive or rank reconstruction mismatch",
                              broad_phase_checkpoint::independent_provider_reconstruction);
    return false;
  }

  std::array<triangle_aabb_hierarchy<T>, 2> expected_hierarchies;
  for (std::size_t slot = 0; slot < 2; ++slot) {
    if (!broad_phase_verifier_detail::reconstruct_hierarchy(
            slot == 0 ? operand_id::a : operand_id::b,
            expected_tables[slot].triangles, expected_spatial[slot],
            expected_hierarchies[slot])) {
      error = broad_phase_error(broad_phase_subcode::independent_provider_mismatch,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase verifier could not reconstruct hierarchy",
                                broad_phase_checkpoint::independent_provider_reconstruction);
      return false;
    }
    if (encode_triangle_aabb_hierarchy(expected_hierarchies[slot]) !=
        encode_triangle_aabb_hierarchy(
            artifact.hierarchy(slot == 0 ? operand_id::a : operand_id::b))) {
      error = broad_phase_error(broad_phase_subcode::independent_provider_mismatch,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase hierarchy reconstruction mismatch",
                                broad_phase_checkpoint::independent_provider_reconstruction);
      return false;
    }
  }
  evidence.hierarchy_reconstruction_complete = true;

  std::vector<canonical_candidate_key> expected_keys;
  std::uint64_t verifier_work = 0;
  std::uint64_t maximum_queue = 0;
  if (!broad_phase_verifier_detail::collect_breadth_first_candidates(
          directed_candidate_role::a_edge_b_triangle, expected_tables[0],
          expected_tables[1], expected_hierarchies[1], expected_keys,
          verifier_work, maximum_queue) ||
      !broad_phase_verifier_detail::collect_breadth_first_candidates(
          directed_candidate_role::b_edge_a_triangle, expected_tables[1],
          expected_tables[0], expected_hierarchies[0], expected_keys,
          verifier_work, maximum_queue)) {
    error = broad_phase_error(broad_phase_subcode::independent_candidate_set_mismatch,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase breadth-first traversal failed",
                              broad_phase_checkpoint::independent_breadth_first_verification);
    return false;
  }
  std::sort(expected_keys.begin(), expected_keys.end());
  if (std::adjacent_find(expected_keys.begin(), expected_keys.end()) !=
      expected_keys.end()) {
    error = broad_phase_error(broad_phase_subcode::duplicate_candidate_key,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase verifier reconstructed duplicate V1 candidates",
                              broad_phase_checkpoint::independent_breadth_first_verification);
    return false;
  }
  if (expected_keys.size() != artifact.candidates().size()) {
    error = broad_phase_error(broad_phase_subcode::independent_candidate_set_mismatch,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase published candidate count mismatch",
                              broad_phase_checkpoint::independent_breadth_first_verification);
    return false;
  }

  std::vector<std::uint64_t> per_plan_counts(artifact.count_plans().size(), 0);
  canonical_writer candidate_set_writer;
  candidate_set_writer.u32(0x36534259U); // YBS6
  candidate_set_writer.u64(expected_keys.size());
  for (std::uint64_t ordinal = 0; ordinal < expected_keys.size(); ++ordinal) {
    const auto &candidate = artifact.candidates()[ordinal];
    if (!(candidate.key == expected_keys[ordinal]) ||
        candidate.id.ordinal() != ordinal || candidate.ordinal != ordinal ||
        candidate.role != candidate.key.role ||
        candidate.family != candidate.key.family ||
        candidate.edge_class != candidate.key.edge_class ||
        candidate.filter_reason != topological_filter_reason::not_filtered ||
        candidate.domain_policy_version != 1 || candidate.provider_version != 1 ||
        candidate.reserved != 0 ||
        candidate.witness.ordinal() >= artifact.witnesses().size()) {
      error = broad_phase_error(broad_phase_subcode::candidate_order_mismatch,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase candidate order, ID, or payload mismatch",
                                broad_phase_checkpoint::independent_breadth_first_verification);
      error.witnesses[0] = ordinal;
      error.witness_count = 1;
      return false;
    }
    const auto &witness = artifact.witnesses()[candidate.witness.ordinal()];
    if (witness.id.ordinal() != ordinal ||
        !(witness.key.candidate == candidate.key) ||
        witness.filter_reason != topological_filter_reason::not_filtered ||
        witness.count_plan_ordinal >= per_plan_counts.size() ||
        witness.reserved != 0) {
      error = broad_phase_error(broad_phase_subcode::malformed_overlap_witness,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase verifier rejected overlap witness",
                                broad_phase_checkpoint::independent_breadth_first_verification);
      return false;
    }
    ++per_plan_counts[witness.count_plan_ordinal];
    broad_phase_verifier_detail::append_candidate_key_digest<T>(
        candidate_set_writer, candidate.key);
  }
  for (std::uint64_t ordinal = 0; ordinal < artifact.count_plans().size(); ++ordinal) {
    const auto &plan = artifact.count_plans()[ordinal];
    if (plan.plan_ordinal != ordinal || plan.reserved != 0 ||
        plan.candidate_count != per_plan_counts[ordinal] ||
        plan.retained_overlaps != plan.candidate_count ||
        (ordinal != 0 &&
         plan.output_prefix !=
             artifact.count_plans()[ordinal - 1].output_prefix +
                 artifact.count_plans()[ordinal - 1].candidate_count)) {
      error = broad_phase_error(broad_phase_subcode::counter_mismatch,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase count plan or prefix mismatch",
                                broad_phase_checkpoint::independent_breadth_first_verification);
      return false;
    }
  }
  if (!artifact.count_plans().empty()) {
    const auto &last = artifact.count_plans().back();
    if (last.output_prefix + last.candidate_count != artifact.candidates().size()) {
      error = broad_phase_error(broad_phase_subcode::counter_mismatch,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase final prefix does not match candidate count",
                                broad_phase_checkpoint::independent_breadth_first_verification);
      return false;
    }
  } else if (!artifact.candidates().empty()) {
    return false;
  }
  if (!broad_phase_verifier_detail::validate_partitions(
          artifact.partitions(), artifact.candidates().size(),
          capabilities.partition_capacity)) {
    error = broad_phase_error(broad_phase_subcode::partition_mismatch,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase partition table mismatch",
                              broad_phase_checkpoint::independent_breadth_first_verification);
    return false;
  }
  evidence.breadth_first_candidate_set_complete = true;
  evidence.breadth_first_candidate_count = expected_keys.size();
  evidence.maximum_queue = maximum_queue;
  evidence.candidate_set_digest = sha256::digest(candidate_set_writer.bytes());
  evidence.verifier_work_units = verifier_work;

  std::uint64_t pair_product_a = 0;
  std::uint64_t pair_product_b = 0;
  const bool pair_products_valid = checked_multiply<std::uint64_t>(
                                       expected_tables[0].edges.size(),
                                       expected_tables[1].triangles.size(),
                                       pair_product_a) &&
                                   checked_multiply<std::uint64_t>(
                                       expected_tables[1].edges.size(),
                                       expected_tables[0].triangles.size(),
                                       pair_product_b);
  const bool require_exhaustive =
      pair_products_valid &&
      (context.options.verification.level ==
           verification_level::exhaustive_diagnostics_v1 ||
       (pair_product_a <= capabilities.exhaustive_pair_threshold &&
        pair_product_b <= capabilities.exhaustive_pair_threshold));
  evidence.exhaustive_all_pairs_performed = require_exhaustive;
  if (require_exhaustive) {
    std::vector<canonical_candidate_key> exhaustive;
    std::uint64_t exhaustive_work = 0;
    broad_phase_verifier_detail::collect_all_pairs_candidates(
        directed_candidate_role::a_edge_b_triangle, expected_tables[0],
        expected_tables[1], exhaustive, exhaustive_work);
    broad_phase_verifier_detail::collect_all_pairs_candidates(
        directed_candidate_role::b_edge_a_triangle, expected_tables[1],
        expected_tables[0], exhaustive, exhaustive_work);
    std::sort(exhaustive.begin(), exhaustive.end());
    if (exhaustive != expected_keys) {
      error = broad_phase_error(broad_phase_subcode::exhaustive_false_negative,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase exhaustive all-pairs oracle mismatch",
                                broad_phase_checkpoint::exhaustive_all_pairs_verification);
      return false;
    }
    evidence.exhaustive_all_pairs_complete = true;
    evidence.exhaustive_candidate_count = exhaustive.size();
    evidence.verifier_work_units += exhaustive_work;
  }
  if (evidence.verifier_work_units > capabilities.maximum_work_units) {
    error = broad_phase_error(broad_phase_subcode::traversal_limit,
                              bounded_boolean_error_category::resource_limit,
                              "broad-phase verifier work limit exceeded",
                              broad_phase_checkpoint::independent_breadth_first_verification);
    return false;
  }
  if (verify_codec && !verify_broad_phase_codec(artifact, error))
    return false;
  return true;
}

} // namespace ygor::mesh_boolean::bounded
