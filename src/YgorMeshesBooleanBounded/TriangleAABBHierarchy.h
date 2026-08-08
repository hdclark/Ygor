#pragma once

#include "RankMortonKey.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

namespace ygor::mesh_boolean::bounded {

template <class T> struct triangle_aabb_hierarchy_node final {
  broad_phase_node_id id{0};
  std::uint64_t ordinal = 0;
  operand_id operand = operand_id::a;
  hierarchy_node_kind kind = hierarchy_node_kind::leaf;
  canonical_bound3<T> bound{};
  std::uint64_t first_spatial_primitive = 0;
  std::uint64_t subtree_primitive_count = 0;
  std::uint32_t level_from_leaves = 0;
  std::uint32_t height = 1;
  hierarchy_node_key key{};
  broad_phase_node_id left{broad_phase_invalid_ordinal};
  broad_phase_node_id right{broad_phase_invalid_ordinal};
  std::uint32_t reserved = 0;
};

template <class T> struct triangle_aabb_hierarchy final {
  operand_id operand = operand_id::a;
  std::uint16_t provider_version = 1;
  std::uint16_t layout_version = 1;
  std::uint32_t leaf_capacity = broad_phase_leaf_capacity_v1;
  std::vector<std::uint64_t> spatial_primitives;
  std::vector<triangle_aabb_hierarchy_node<T>> nodes;
  std::uint64_t root = broad_phase_invalid_ordinal;
  std::uint64_t leaf_count = 0;
  std::uint64_t height = 0;
  std::uint32_t reserved = 0;

  bool empty() const noexcept { return spatial_primitives.empty(); }
};

template <class T>
bool build_triangle_aabb_hierarchy(
    operand_id operand,
    const std::vector<broad_phase_triangle_primitive<T>> &triangles,
    const std::vector<std::uint64_t> &spatial_order,
    triangle_aabb_hierarchy<T> &out, bounded_boolean_error &error) {
  out = triangle_aabb_hierarchy<T>{};
  out.operand = operand;
  if (triangles.size() != spatial_order.size()) {
    error = broad_phase_error(broad_phase_subcode::malformed_spatial_order,
                              bounded_boolean_error_category::internal_invariant_error,
                              "triangle hierarchy received an incomplete spatial order",
                              broad_phase_checkpoint::operand_a_hierarchy);
    return false;
  }
  if (triangles.empty())
    return true;

  std::uint64_t leaf_count =
      (static_cast<std::uint64_t>(triangles.size()) +
       broad_phase_leaf_capacity_v1 - 1) /
      broad_phase_leaf_capacity_v1;
  std::uint64_t node_upper_bound = 0;
  if (!checked_multiply<std::uint64_t>(leaf_count, 2, node_upper_bound) ||
      node_upper_bound == 0) {
    error = broad_phase_error(broad_phase_subcode::node_count_overflow,
                              bounded_boolean_error_category::index_overflow,
                              "triangle hierarchy node count overflow",
                              broad_phase_checkpoint::representability_preflight);
    return false;
  }
  --node_upper_bound;

  try {
    out.spatial_primitives = spatial_order;
    out.nodes.reserve(static_cast<std::size_t>(node_upper_bound));
  } catch (...) {
    error = broad_phase_error(broad_phase_subcode::resource_preflight,
                              bounded_boolean_error_category::resource_limit,
                              "triangle hierarchy allocation failed",
                              broad_phase_checkpoint::fixed_resource_reservation);
    return false;
  }

  std::vector<std::uint64_t> current;
  std::vector<std::uint64_t> next;
  try {
    current.reserve(static_cast<std::size_t>(leaf_count));
    next.reserve(static_cast<std::size_t>((leaf_count + 1) / 2));
  } catch (...) {
    error = broad_phase_error(broad_phase_subcode::resource_preflight,
                              bounded_boolean_error_category::resource_limit,
                              "triangle hierarchy level allocation failed",
                              broad_phase_checkpoint::fixed_resource_reservation);
    return false;
  }

  for (std::uint64_t begin = 0; begin < triangles.size();
       begin += broad_phase_leaf_capacity_v1) {
    const std::uint64_t count = std::min<std::uint64_t>(
        broad_phase_leaf_capacity_v1,
        static_cast<std::uint64_t>(triangles.size()) - begin);
    canonical_bound3<T> bound;
    bool initialized = false;
    for (std::uint64_t offset = 0; offset < count; ++offset) {
      const auto primitive = spatial_order[begin + offset];
      if (primitive >= triangles.size() ||
          triangles[primitive].spatial_ordinal != begin + offset ||
          !triangles[primitive].bound.valid()) {
        error = broad_phase_error(broad_phase_subcode::malformed_spatial_order,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "triangle hierarchy spatial primitive is invalid",
                                  broad_phase_checkpoint::operand_a_hierarchy);
        return false;
      }
      bound = initialized ? canonical_bound_hull(bound, triangles[primitive].bound)
                          : triangles[primitive].bound;
      initialized = true;
    }
    if (!initialized || !bound.valid()) {
      error = broad_phase_error(broad_phase_subcode::malformed_leaf,
                                bounded_boolean_error_category::internal_invariant_error,
                                "triangle hierarchy leaf bound is invalid",
                                broad_phase_checkpoint::operand_a_hierarchy);
      return false;
    }
    triangle_aabb_hierarchy_node<T> node;
    node.ordinal = out.nodes.size();
    node.id = broad_phase_node_id{node.ordinal};
    node.operand = operand;
    node.kind = hierarchy_node_kind::leaf;
    node.bound = bound;
    node.first_spatial_primitive = begin;
    node.subtree_primitive_count = count;
    node.level_from_leaves = 0;
    node.height = 1;
    node.key = {operand, begin, count, hierarchy_node_kind::leaf, 0, 1};
    out.nodes.push_back(node);
    current.push_back(node.ordinal);
  }
  out.leaf_count = current.size();

  while (current.size() > 1) {
    next.clear();
    for (std::size_t i = 0; i < current.size(); i += 2) {
      if (i + 1 == current.size()) {
        next.push_back(current[i]);
        continue;
      }
      const auto left_id = current[i];
      const auto right_id = current[i + 1];
      if (left_id >= out.nodes.size() || right_id >= out.nodes.size() ||
          left_id == right_id) {
        error = broad_phase_error(broad_phase_subcode::malformed_internal_node,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "triangle hierarchy child reference is invalid",
                                  broad_phase_checkpoint::operand_a_hierarchy);
        return false;
      }
      const auto &left = out.nodes[left_id];
      const auto &right = out.nodes[right_id];
      std::uint64_t left_end = 0;
      std::uint64_t total_count = 0;
      if (!checked_add<std::uint64_t>(left.first_spatial_primitive,
                                      left.subtree_primitive_count, left_end) ||
          left_end != right.first_spatial_primitive ||
          !checked_add<std::uint64_t>(left.subtree_primitive_count,
                                      right.subtree_primitive_count,
                                      total_count)) {
        error = broad_phase_error(broad_phase_subcode::malformed_internal_node,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "triangle hierarchy child ranges are not adjacent",
                                  broad_phase_checkpoint::operand_a_hierarchy);
        return false;
      }
      triangle_aabb_hierarchy_node<T> node;
      node.ordinal = out.nodes.size();
      node.id = broad_phase_node_id{node.ordinal};
      node.operand = operand;
      node.kind = hierarchy_node_kind::internal;
      node.bound = canonical_bound_hull(left.bound, right.bound);
      node.first_spatial_primitive = left.first_spatial_primitive;
      node.subtree_primitive_count = total_count;
      node.level_from_leaves =
          std::max(left.level_from_leaves, right.level_from_leaves) + 1;
      node.height = std::max(left.height, right.height) + 1;
      node.left = broad_phase_node_id{left_id};
      node.right = broad_phase_node_id{right_id};
      node.key = {operand, node.first_spatial_primitive,
                  node.subtree_primitive_count, hierarchy_node_kind::internal,
                  node.level_from_leaves, 1};
      if (!node.bound.valid()) {
        error = broad_phase_error(broad_phase_subcode::hierarchy_containment_failure,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "triangle hierarchy parent bound is invalid",
                                  broad_phase_checkpoint::operand_a_hierarchy);
        return false;
      }
      out.nodes.push_back(node);
      next.push_back(node.ordinal);
    }
    current.swap(next);
  }

  out.root = current.front();
  out.height = out.nodes[out.root].height;
  return true;
}

template <class T>
bool verify_triangle_aabb_hierarchy_producer(
    const std::vector<broad_phase_triangle_primitive<T>> &triangles,
    const triangle_aabb_hierarchy<T> &hierarchy,
    bounded_boolean_error &error) {
  if (hierarchy.operand != (triangles.empty() ? hierarchy.operand
                                              : triangles.front().operand) ||
      hierarchy.provider_version != 1 || hierarchy.layout_version != 1 ||
      hierarchy.leaf_capacity != broad_phase_leaf_capacity_v1 ||
      hierarchy.reserved != 0) {
    error = broad_phase_error(broad_phase_subcode::unsupported_version,
                              bounded_boolean_error_category::internal_invariant_error,
                              "triangle hierarchy header is malformed",
                              broad_phase_checkpoint::operand_a_producer_verification);
    return false;
  }
  if (triangles.empty()) {
    if (!hierarchy.spatial_primitives.empty() || !hierarchy.nodes.empty() ||
        hierarchy.root != broad_phase_invalid_ordinal || hierarchy.leaf_count != 0 ||
        hierarchy.height != 0) {
      error = broad_phase_error(broad_phase_subcode::malformed_root,
                                bounded_boolean_error_category::internal_invariant_error,
                                "empty triangle hierarchy is noncanonical",
                                broad_phase_checkpoint::operand_a_producer_verification);
      return false;
    }
    return true;
  }
  if (hierarchy.spatial_primitives.size() != triangles.size() ||
      hierarchy.root >= hierarchy.nodes.size() || hierarchy.leaf_count == 0 ||
      hierarchy.nodes[hierarchy.root].first_spatial_primitive != 0 ||
      hierarchy.nodes[hierarchy.root].subtree_primitive_count != triangles.size() ||
      hierarchy.nodes[hierarchy.root].height != hierarchy.height) {
    error = broad_phase_error(broad_phase_subcode::malformed_root,
                              bounded_boolean_error_category::internal_invariant_error,
                              "triangle hierarchy root is malformed",
                              broad_phase_checkpoint::operand_a_producer_verification);
    return false;
  }
  std::vector<std::uint8_t> primitive_seen(triangles.size(), 0);
  std::uint64_t leaves_seen = 0;
  for (std::uint64_t ordinal = 0; ordinal < hierarchy.nodes.size(); ++ordinal) {
    const auto &node = hierarchy.nodes[ordinal];
    if (node.ordinal != ordinal || node.id.ordinal() != ordinal ||
        node.operand != hierarchy.operand || node.reserved != 0 ||
        !node.bound.valid() || node.key.operand != hierarchy.operand ||
        node.key.first_spatial_primitive != node.first_spatial_primitive ||
        node.key.subtree_primitive_count != node.subtree_primitive_count ||
        node.key.kind != node.kind ||
        node.key.level_from_leaves != node.level_from_leaves ||
        node.key.layout_version != hierarchy.layout_version) {
      error = broad_phase_error(broad_phase_subcode::malformed_internal_node,
                                bounded_boolean_error_category::internal_invariant_error,
                                "triangle hierarchy node metadata is malformed",
                                broad_phase_checkpoint::operand_a_producer_verification);
      error.witnesses[0] = ordinal;
      error.witness_count = 1;
      return false;
    }
    std::uint64_t end = 0;
    if (node.subtree_primitive_count == 0 ||
        !checked_add<std::uint64_t>(node.first_spatial_primitive,
                                    node.subtree_primitive_count, end) ||
        end > hierarchy.spatial_primitives.size()) {
      error = broad_phase_error(broad_phase_subcode::malformed_internal_node,
                                bounded_boolean_error_category::internal_invariant_error,
                                "triangle hierarchy node range is malformed",
                                broad_phase_checkpoint::operand_a_producer_verification);
      return false;
    }
    if (node.kind == hierarchy_node_kind::leaf) {
      ++leaves_seen;
      if (node.level_from_leaves != 0 || node.height != 1 ||
          node.subtree_primitive_count > broad_phase_leaf_capacity_v1 ||
          node.left.ordinal() != broad_phase_invalid_ordinal ||
          node.right.ordinal() != broad_phase_invalid_ordinal) {
        error = broad_phase_error(broad_phase_subcode::malformed_leaf,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "triangle hierarchy leaf payload is malformed",
                                  broad_phase_checkpoint::operand_a_producer_verification);
        return false;
      }
      canonical_bound3<T> expected;
      bool initialized = false;
      for (std::uint64_t position = node.first_spatial_primitive; position < end;
           ++position) {
        const auto primitive = hierarchy.spatial_primitives[position];
        if (primitive >= triangles.size() || primitive_seen[primitive] != 0 ||
            triangles[primitive].spatial_ordinal != position) {
          error = broad_phase_error(broad_phase_subcode::malformed_leaf,
                                    bounded_boolean_error_category::internal_invariant_error,
                                    "triangle hierarchy leaf coverage is malformed",
                                    broad_phase_checkpoint::operand_a_producer_verification);
          return false;
        }
        primitive_seen[primitive] = 1;
        expected = initialized ? canonical_bound_hull(expected, triangles[primitive].bound)
                               : triangles[primitive].bound;
        initialized = true;
      }
      if (!initialized || !exact_bound_equal(expected, node.bound)) {
        error = broad_phase_error(broad_phase_subcode::hierarchy_containment_failure,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "triangle hierarchy leaf bound mismatch",
                                  broad_phase_checkpoint::operand_a_producer_verification);
        return false;
      }
    } else if (node.kind == hierarchy_node_kind::internal) {
      const auto left_id = node.left.ordinal();
      const auto right_id = node.right.ordinal();
      if (left_id >= ordinal || right_id >= ordinal || left_id == right_id ||
          left_id >= hierarchy.nodes.size() || right_id >= hierarchy.nodes.size()) {
        error = broad_phase_error(broad_phase_subcode::hierarchy_cycle,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "triangle hierarchy has a self, forward, or cyclic child",
                                  broad_phase_checkpoint::operand_a_producer_verification);
        return false;
      }
      const auto &left = hierarchy.nodes[left_id];
      const auto &right = hierarchy.nodes[right_id];
      std::uint64_t left_end = 0;
      std::uint64_t combined = 0;
      if (!checked_add<std::uint64_t>(left.first_spatial_primitive,
                                      left.subtree_primitive_count, left_end) ||
          left_end != right.first_spatial_primitive ||
          left.first_spatial_primitive != node.first_spatial_primitive ||
          !checked_add<std::uint64_t>(left.subtree_primitive_count,
                                      right.subtree_primitive_count, combined) ||
          combined != node.subtree_primitive_count ||
          node.height != std::max(left.height, right.height) + 1 ||
          node.level_from_leaves !=
              std::max(left.level_from_leaves, right.level_from_leaves) + 1 ||
          !exact_bound_equal(canonical_bound_hull(left.bound, right.bound),
                             node.bound)) {
        error = broad_phase_error(broad_phase_subcode::malformed_internal_node,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "triangle hierarchy internal payload is malformed",
                                  broad_phase_checkpoint::operand_a_producer_verification);
        return false;
      }
    } else {
      error = broad_phase_error(broad_phase_subcode::malformed_internal_node,
                                bounded_boolean_error_category::internal_invariant_error,
                                "triangle hierarchy node kind is unknown",
                                broad_phase_checkpoint::operand_a_producer_verification);
      return false;
    }
  }
  if (leaves_seen != hierarchy.leaf_count ||
      std::find(primitive_seen.begin(), primitive_seen.end(), std::uint8_t{0}) !=
          primitive_seen.end()) {
    error = broad_phase_error(broad_phase_subcode::malformed_leaf,
                              bounded_boolean_error_category::internal_invariant_error,
                              "triangle hierarchy primitive coverage is incomplete",
                              broad_phase_checkpoint::operand_a_producer_verification);
    return false;
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
