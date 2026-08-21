#pragma once

#include "CandidateDomainPolicy.h"
#include "TriangleAABBHierarchy.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct broad_phase_count_plan final {
  directed_candidate_role role = directed_candidate_role::a_edge_b_triangle;
  broad_phase_edge_primitive_id edge{0};
  std::uint64_t plan_ordinal = 0;
  std::uint64_t node_tests = 0;
  std::uint64_t leaf_visits = 0;
  std::uint64_t primitive_tests = 0;
  std::uint64_t definite_prunes = 0;
  std::uint64_t retained_overlaps = 0;
  std::uint64_t candidate_count = 0;
  std::uint64_t output_prefix = 0;
  std::uint64_t work_units = 0;
  std::uint32_t reserved = 0;
};

template <class T> struct broad_phase_overlap_witness final {
  overlap_witness_id id{0};
  overlap_witness_key key{};
  directed_candidate_role role = directed_candidate_role::a_edge_b_triangle;
  broad_phase_edge_primitive_id edge{0};
  broad_phase_triangle_primitive_id triangle{0};
  broad_phase_node_id admitting_leaf{0};
  std::uint64_t triangle_spatial_slot = 0;
  broad_phase_bound_relation<T> relation{};
  topological_filter_reason filter_reason =
      topological_filter_reason::not_filtered;
  std::uint64_t count_plan_ordinal = 0;
  std::uint16_t provider_version = 1;
  std::uint16_t formula_version = 1;
  std::uint32_t reserved = 0;
};

template <class T> struct broad_phase_candidate_discovery final {
  canonical_candidate_key key{};
  directed_candidate_role role = directed_candidate_role::a_edge_b_triangle;
  broad_phase_relation_family family =
      broad_phase_relation_family::canonical_edge_source_triangle;
  broad_phase_edge_primitive_id edge{0};
  broad_phase_triangle_primitive_id triangle{0};
  canonical_edge_class edge_class = canonical_edge_class::source_edge;
  broad_phase_overlap_witness<T> witness{};
  std::uint16_t domain_policy_version = 1;
  std::uint16_t provider_version = 1;
  std::uint32_t reserved = 0;
};

namespace broad_phase_traverse_detail {

inline bool checked_increment(std::uint64_t &value) noexcept {
  if (value == std::numeric_limits<std::uint64_t>::max())
    return false;
  ++value;
  return true;
}

inline bool checked_accumulate(std::uint64_t value,
                               std::uint64_t &total) noexcept {
  return checked_add<std::uint64_t>(total, value, total);
}

inline std::size_t role_slot(directed_candidate_role role) noexcept {
  return role == directed_candidate_role::a_edge_b_triangle ? 0 : 1;
}

template <class T, class Consumer>
bool traverse_one_edge(
    directed_candidate_role role,
    const broad_phase_edge_primitive<T> &edge,
    const std::vector<broad_phase_triangle_primitive<T>> &triangles,
    const triangle_aabb_hierarchy<T> &hierarchy,
    const broad_phase_capabilities &capabilities,
    broad_phase_count_plan &plan,
    Consumer &&consumer,
    bounded_boolean_error &error) {
  if (!candidate_role_matches_operands(role, edge.operand, hierarchy.operand) ||
      edge.inclusion != candidate_domain_disposition::included ||
      !edge.bound.valid()) {
    error = broad_phase_error(broad_phase_subcode::malformed_role,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase traversal role or edge primitive is malformed",
                              broad_phase_checkpoint::candidate_witness_validation);
    return false;
  }
  if (hierarchy.empty())
    return true;
  if (hierarchy.root >= hierarchy.nodes.size()) {
    error = broad_phase_error(broad_phase_subcode::malformed_root,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase traversal hierarchy root is malformed",
                              broad_phase_checkpoint::candidate_witness_validation);
    return false;
  }

  std::vector<std::uint64_t> stack;
  try {
    stack.reserve(static_cast<std::size_t>(hierarchy.height + 1));
    stack.push_back(hierarchy.root);
  } catch (...) {
    error = broad_phase_error(broad_phase_subcode::resource_preflight,
                              bounded_boolean_error_category::resource_limit,
                              "broad-phase traversal stack allocation failed",
                              broad_phase_checkpoint::fixed_resource_reservation);
    return false;
  }

  std::uint64_t poll_counter = 0;
  while (!stack.empty()) {
    const auto node_id = stack.back();
    stack.pop_back();
    if (node_id >= hierarchy.nodes.size() ||
        !checked_increment(plan.node_tests) ||
        !checked_increment(plan.work_units)) {
      error = broad_phase_error(broad_phase_subcode::traversal_limit,
                                bounded_boolean_error_category::resource_limit,
                                "broad-phase node traversal limit exceeded",
                                broad_phase_checkpoint::a_edge_b_triangle_count);
      return false;
    }
    if (plan.work_units > capabilities.maximum_work_units) {
      error = broad_phase_error(broad_phase_subcode::traversal_limit,
                                bounded_boolean_error_category::resource_limit,
                                "broad-phase work limit exceeded",
                                broad_phase_checkpoint::a_edge_b_triangle_count);
      return false;
    }
    if (++poll_counter >= capabilities.cancellation_poll_interval) {
      poll_counter = 0;
      if (broad_phase_cancelled(capabilities)) {
        error = broad_phase_error(broad_phase_subcode::cancelled,
                                  bounded_boolean_error_category::cancelled,
                                  "broad-phase traversal cancelled",
                                  broad_phase_checkpoint::a_edge_b_triangle_count);
        return false;
      }
    }

    const auto &node = hierarchy.nodes[node_id];
    if (!node.bound.valid()) {
      error = broad_phase_error(broad_phase_subcode::malformed_bound,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase hierarchy node bound is invalid",
                                broad_phase_checkpoint::candidate_witness_validation);
      return false;
    }
    const auto node_relation = classify_closed_bound_relation(edge.bound, node.bound);
    if (node_relation.definitely_separated) {
      if (!checked_increment(plan.definite_prunes)) {
        error = broad_phase_error(broad_phase_subcode::traversal_limit,
                                  bounded_boolean_error_category::index_overflow,
                                  "broad-phase prune counter overflow",
                                  broad_phase_checkpoint::a_edge_b_triangle_count);
        return false;
      }
      continue;
    }

    if (node.kind == hierarchy_node_kind::internal) {
      const auto left = node.left.ordinal();
      const auto right = node.right.ordinal();
      if (left >= hierarchy.nodes.size() || right >= hierarchy.nodes.size() ||
          left >= node_id || right >= node_id || left == right) {
        error = broad_phase_error(broad_phase_subcode::hierarchy_cycle,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "broad-phase traversal encountered an invalid child",
                                  broad_phase_checkpoint::candidate_witness_validation);
        return false;
      }
      try {
        stack.push_back(right);
        stack.push_back(left);
      } catch (...) {
        error = broad_phase_error(broad_phase_subcode::resource_preflight,
                                  bounded_boolean_error_category::resource_limit,
                                  "broad-phase traversal stack growth failed",
                                  broad_phase_checkpoint::fixed_resource_reservation);
        return false;
      }
      continue;
    }
    if (node.kind != hierarchy_node_kind::leaf ||
        node.subtree_primitive_count == 0 ||
        node.subtree_primitive_count > broad_phase_leaf_capacity_v1) {
      error = broad_phase_error(broad_phase_subcode::malformed_leaf,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase traversal encountered a malformed leaf",
                                broad_phase_checkpoint::candidate_witness_validation);
      return false;
    }
    if (!checked_increment(plan.leaf_visits)) {
      error = broad_phase_error(broad_phase_subcode::traversal_limit,
                                bounded_boolean_error_category::index_overflow,
                                "broad-phase leaf counter overflow",
                                broad_phase_checkpoint::a_edge_b_triangle_count);
      return false;
    }
    const auto end = node.first_spatial_primitive + node.subtree_primitive_count;
    if (end < node.first_spatial_primitive ||
        end > hierarchy.spatial_primitives.size()) {
      error = broad_phase_error(broad_phase_subcode::malformed_leaf,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase leaf range is invalid",
                                broad_phase_checkpoint::candidate_witness_validation);
      return false;
    }
    for (std::uint64_t spatial = node.first_spatial_primitive; spatial < end;
         ++spatial) {
      if (!checked_increment(plan.primitive_tests) ||
          !checked_increment(plan.work_units) ||
          plan.work_units > capabilities.maximum_work_units) {
        error = broad_phase_error(broad_phase_subcode::traversal_limit,
                                  bounded_boolean_error_category::resource_limit,
                                  "broad-phase primitive traversal limit exceeded",
                                  broad_phase_checkpoint::a_edge_b_triangle_count);
        return false;
      }
      const auto primitive_id = hierarchy.spatial_primitives[spatial];
      if (primitive_id >= triangles.size()) {
        error = broad_phase_error(broad_phase_subcode::malformed_leaf,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "broad-phase leaf references an invalid triangle",
                                  broad_phase_checkpoint::candidate_witness_validation);
        return false;
      }
      const auto &triangle = triangles[primitive_id];
      if (triangle.operand != hierarchy.operand || !triangle.bound.valid()) {
        error = broad_phase_error(broad_phase_subcode::malformed_triangle_primitive,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "broad-phase triangle primitive is malformed",
                                  broad_phase_checkpoint::candidate_witness_validation);
        return false;
      }
      const auto relation = classify_closed_bound_relation(edge.bound, triangle.bound);
      if (relation.definitely_separated) {
        if (!checked_increment(plan.definite_prunes)) {
          error = broad_phase_error(broad_phase_subcode::traversal_limit,
                                    bounded_boolean_error_category::index_overflow,
                                    "broad-phase primitive prune counter overflow",
                                    broad_phase_checkpoint::a_edge_b_triangle_count);
          return false;
        }
        continue;
      }
      if (!checked_increment(plan.retained_overlaps) ||
          !checked_increment(plan.candidate_count) ||
          plan.candidate_count > capabilities.maximum_candidates) {
        error = broad_phase_error(broad_phase_subcode::traversal_limit,
                                  bounded_boolean_error_category::resource_limit,
                                  "broad-phase candidate limit exceeded",
                                  broad_phase_checkpoint::a_edge_b_triangle_count);
        return false;
      }
      canonical_candidate_key key;
      key.role = role;
      key.family = broad_phase_relation_family::canonical_edge_source_triangle;
      key.edge = edge.semantic_key;
      key.triangle = triangle.semantic_key;
      key.edge_class = edge.edge_class;
      key.domain_policy_version = capabilities.domain_policy_version;
      overlap_witness_key witness_key;
      witness_key.candidate = key;
      witness_key.admitting_leaf = node_id;
      witness_key.triangle_spatial_slot = spatial;
      witness_key.axes = relation.axes;
      if (!consumer(triangle, node, spatial, relation, key, witness_key))
        return false;
    }
  }
  return true;
}

} // namespace broad_phase_traverse_detail

template <class T>
bool count_directed_candidates(
    directed_candidate_role role,
    const std::vector<broad_phase_edge_primitive<T>> &edges,
    const std::vector<broad_phase_triangle_primitive<T>> &triangles,
    const triangle_aabb_hierarchy<T> &hierarchy,
    const broad_phase_capabilities &capabilities,
    std::vector<broad_phase_count_plan> &plans,
    broad_phase_statistics &statistics,
    bounded_boolean_error &error) {
  if (!valid_directed_candidate_role(role) ||
      role_edge_operand(role) != (edges.empty() ? role_edge_operand(role)
                                                : edges.front().operand) ||
      role_triangle_operand(role) != hierarchy.operand) {
    error = broad_phase_error(broad_phase_subcode::malformed_role,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase count role is malformed",
                              broad_phase_checkpoint::a_edge_b_triangle_count);
    return false;
  }
  const auto role_index = broad_phase_traverse_detail::role_slot(role);
  for (std::uint64_t edge_index = 0; edge_index < edges.size(); ++edge_index) {
    broad_phase_count_plan plan;
    plan.role = role;
    plan.edge = broad_phase_edge_primitive_id{edge_index};
    plan.plan_ordinal = plans.size();
    const auto no_op = [](const auto &, const auto &, std::uint64_t, const auto &,
                          const auto &, const auto &) { return true; };
    if (!broad_phase_traverse_detail::traverse_one_edge(
            role, edges[edge_index], triangles, hierarchy, capabilities, plan,
            no_op, error))
      return false;
    if (!broad_phase_traverse_detail::checked_accumulate(
            plan.node_tests, statistics.role_node_tests[role_index]) ||
        !broad_phase_traverse_detail::checked_accumulate(
            plan.leaf_visits, statistics.role_leaf_visits[role_index]) ||
        !broad_phase_traverse_detail::checked_accumulate(
            plan.primitive_tests, statistics.role_primitive_tests[role_index]) ||
        !broad_phase_traverse_detail::checked_accumulate(
            plan.definite_prunes, statistics.role_definite_prunes[role_index]) ||
        !broad_phase_traverse_detail::checked_accumulate(
            plan.candidate_count, statistics.role_candidates[role_index]) ||
        !broad_phase_traverse_detail::checked_accumulate(
            plan.work_units, statistics.producer_work_units)) {
      error = broad_phase_error(broad_phase_subcode::counter_mismatch,
                                bounded_boolean_error_category::index_overflow,
                                "broad-phase count statistics overflow",
                                broad_phase_checkpoint::count_emit_reconciliation);
      return false;
    }
    plans.push_back(plan);
  }
  return true;
}

template <class T>
bool emit_directed_candidates(
    directed_candidate_role role,
    const std::vector<broad_phase_edge_primitive<T>> &edges,
    const std::vector<broad_phase_triangle_primitive<T>> &triangles,
    const triangle_aabb_hierarchy<T> &hierarchy,
    const broad_phase_capabilities &capabilities,
    const std::vector<broad_phase_count_plan> &plans,
    std::uint64_t plan_begin,
    std::vector<broad_phase_candidate_discovery<T>> &output,
    broad_phase_statistics &statistics,
    bounded_boolean_error &error) {
  const auto role_index = broad_phase_traverse_detail::role_slot(role);
  if (plan_begin > plans.size() || edges.size() > plans.size() - plan_begin) {
    error = broad_phase_error(broad_phase_subcode::emit_count_mismatch,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase emit plan range is malformed",
                              broad_phase_checkpoint::count_emit_reconciliation);
    return false;
  }
  for (std::uint64_t edge_index = 0; edge_index < edges.size(); ++edge_index) {
    const auto &expected = plans[plan_begin + edge_index];
    if (expected.role != role || expected.edge.ordinal() != edge_index ||
        expected.output_prefix > output.size() ||
        expected.candidate_count > output.size() - expected.output_prefix) {
      error = broad_phase_error(broad_phase_subcode::emit_count_mismatch,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase emit plan does not match edge",
                                broad_phase_checkpoint::count_emit_reconciliation);
      return false;
    }
    broad_phase_count_plan observed;
    observed.role = role;
    observed.edge = broad_phase_edge_primitive_id{edge_index};
    observed.plan_ordinal = expected.plan_ordinal;
    std::uint64_t written = 0;
    const auto consumer = [&](const broad_phase_triangle_primitive<T> &triangle,
                              const triangle_aabb_hierarchy_node<T> &leaf,
                              std::uint64_t spatial,
                              const broad_phase_bound_relation<T> &relation,
                              const canonical_candidate_key &key,
                              const overlap_witness_key &witness_key) {
      if (written >= expected.candidate_count)
        return false;
      const auto output_index = expected.output_prefix + written;
      broad_phase_candidate_discovery<T> record;
      record.key = key;
      record.role = role;
      record.family = broad_phase_relation_family::canonical_edge_source_triangle;
      record.edge = broad_phase_edge_primitive_id{edge_index};
      record.triangle = triangle.id;
      record.edge_class = edges[edge_index].edge_class;
      record.domain_policy_version = capabilities.domain_policy_version;
      record.provider_version = capabilities.provider_version;
      record.witness.id = overlap_witness_id{output_index};
      record.witness.key = witness_key;
      record.witness.role = role;
      record.witness.edge = record.edge;
      record.witness.triangle = record.triangle;
      record.witness.admitting_leaf = leaf.id;
      record.witness.triangle_spatial_slot = spatial;
      record.witness.relation = relation;
      record.witness.filter_reason = topological_filter_reason::not_filtered;
      record.witness.count_plan_ordinal = expected.plan_ordinal;
      record.witness.provider_version = capabilities.provider_version;
      output[output_index] = std::move(record);
      ++written;
      return true;
    };
    if (!broad_phase_traverse_detail::traverse_one_edge(
            role, edges[edge_index], triangles, hierarchy, capabilities,
            observed, consumer, error)) {
      if (error.subcode == 0) {
        error = broad_phase_error(broad_phase_subcode::emit_count_mismatch,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "broad-phase emit overflowed its assigned range",
                                  broad_phase_checkpoint::count_emit_reconciliation);
      }
      return false;
    }
    if (written != expected.candidate_count ||
        observed.node_tests != expected.node_tests ||
        observed.leaf_visits != expected.leaf_visits ||
        observed.primitive_tests != expected.primitive_tests ||
        observed.definite_prunes != expected.definite_prunes ||
        observed.retained_overlaps != expected.retained_overlaps ||
        observed.candidate_count != expected.candidate_count ||
        observed.work_units != expected.work_units) {
      error = broad_phase_error(broad_phase_subcode::emit_count_mismatch,
                                bounded_boolean_error_category::internal_invariant_error,
                                "broad-phase count and emit traversals disagree",
                                broad_phase_checkpoint::count_emit_reconciliation);
      error.witnesses[0] = expected.plan_ordinal;
      error.witness_count = 1;
      return false;
    }
    if (!broad_phase_traverse_detail::checked_accumulate(
            observed.node_tests, statistics.role_emit_node_tests[role_index]) ||
        !broad_phase_traverse_detail::checked_accumulate(
            observed.primitive_tests,
            statistics.role_emit_primitive_tests[role_index]) ||
        !broad_phase_traverse_detail::checked_accumulate(
            observed.work_units, statistics.producer_work_units)) {
      error = broad_phase_error(broad_phase_subcode::counter_mismatch,
                                bounded_boolean_error_category::index_overflow,
                                "broad-phase emit statistics overflow",
                                broad_phase_checkpoint::count_emit_reconciliation);
      return false;
    }
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
