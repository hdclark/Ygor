#pragma once

#include "BroadPhasePrimitiveTables.h"
#include "TriangleAABBHierarchy.h"
#include "CanonicalSourceManifolds.h"

#include <cstdint>
#include <limits>

namespace ygor::mesh_boolean::bounded {

inline bool broad_phase_capabilities_valid(
    const broad_phase_capabilities &capabilities) noexcept {
  return capabilities.version == 1 && capabilities.provider_version == 1 &&
         capabilities.domain_policy_version == 1 &&
         capabilities.verifier_version == 1 && capabilities.owner.anchor &&
         capabilities.maximum_entities != 0 &&
         capabilities.maximum_pair_product != 0 &&
         capabilities.maximum_nodes != 0 &&
         capabilities.maximum_candidates != 0 &&
         capabilities.maximum_work_units != 0 &&
         capabilities.maximum_canonical_bytes != 0 &&
         capabilities.partition_capacity == broad_phase_partition_capacity_v1 &&
         capabilities.cancellation_poll_interval != 0 &&
         capabilities.reserved == 0;
}

template <class T, class I>
bool preflight_broad_phase(
    const canonical_source_manifolds<T, I> &manifolds,
    const broad_phase_capabilities &capabilities,
    broad_phase_preflight_counts &counts, bounded_boolean_error &error) {
  counts = broad_phase_preflight_counts{};
  if (!broad_phase_capabilities_valid(capabilities)) {
    error = broad_phase_error(broad_phase_subcode::unsupported_version,
                              bounded_boolean_error_category::input_contract_error,
                              "broad-phase capabilities are unsupported",
                              broad_phase_checkpoint::context_capability_validation);
    return false;
  }
  if (!manifolds.owner().same_owner(capabilities.owner) || !manifolds.a() ||
      !manifolds.b() ||
      !manifolds.a()->owner().same_owner(capabilities.owner) ||
      !manifolds.b()->owner().same_owner(capabilities.owner)) {
    error = broad_phase_error(broad_phase_subcode::wrong_owner,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase source manifold owner mismatch",
                              broad_phase_checkpoint::predecessor_validation);
    return false;
  }
  if (manifolds.a()->operand() != operand_id::a ||
      manifolds.b()->operand() != operand_id::b ||
      manifolds.a()->verification() !=
          canonical_halfedge_verification_disposition::independently_verified ||
      manifolds.b()->verification() !=
          canonical_halfedge_verification_disposition::independently_verified) {
    error = broad_phase_error(broad_phase_subcode::wrong_operand,
                              bounded_boolean_error_category::internal_invariant_error,
                              "broad-phase source manifold operands are malformed",
                              broad_phase_checkpoint::predecessor_validation);
    return false;
  }

  const canonical_halfedge_operand<T, I> *operands[2] = {
      manifolds.a().get(), manifolds.b().get()};
  for (std::size_t slot = 0; slot < 2; ++slot) {
    const auto &operand = *operands[slot];
    counts.edges[slot] = operand.edges().size();
    counts.triangles[slot] = operand.triangles().size();
    for (const auto &edge : operand.edges()) {
      if (edge.edge_class == canonical_edge_class::source_edge)
        ++counts.source_edges[slot];
      else if (edge.edge_class == canonical_edge_class::facet_internal_diagonal)
        ++counts.internal_diagonals[slot];
      else {
        error = broad_phase_error(broad_phase_subcode::malformed_edge_primitive,
                                  bounded_boolean_error_category::internal_invariant_error,
                                  "canonical edge has an unknown broad-phase class",
                                  broad_phase_checkpoint::provider_policy_validation);
        return false;
      }
    }
    if (counts.edges[slot] > capabilities.maximum_entities ||
        counts.triangles[slot] > capabilities.maximum_entities) {
      error = broad_phase_error(broad_phase_subcode::count_overflow,
                                bounded_boolean_error_category::resource_limit,
                                "broad-phase entity count exceeds configured limit",
                                broad_phase_checkpoint::representability_preflight);
      return false;
    }
    counts.leaves[slot] =
        (counts.triangles[slot] + broad_phase_leaf_capacity_v1 - 1) /
        broad_phase_leaf_capacity_v1;
    if (counts.leaves[slot] != 0) {
      if (!checked_multiply<std::uint64_t>(counts.leaves[slot], 2,
                                           counts.node_upper_bounds[slot]) ||
          counts.node_upper_bounds[slot] == 0) {
        error = broad_phase_error(broad_phase_subcode::node_count_overflow,
                                  bounded_boolean_error_category::index_overflow,
                                  "broad-phase hierarchy node count overflow",
                                  broad_phase_checkpoint::representability_preflight);
        return false;
      }
      --counts.node_upper_bounds[slot];
      if (counts.node_upper_bounds[slot] > capabilities.maximum_nodes) {
        error = broad_phase_error(broad_phase_subcode::node_count_overflow,
                                  bounded_boolean_error_category::resource_limit,
                                  "broad-phase hierarchy node limit exceeded",
                                  broad_phase_checkpoint::representability_preflight);
        return false;
      }
    }
  }

  if (!checked_multiply<std::uint64_t>(counts.edges[0], counts.triangles[1],
                                       counts.pair_products[0]) ||
      !checked_multiply<std::uint64_t>(counts.edges[1], counts.triangles[0],
                                       counts.pair_products[1])) {
    error = broad_phase_error(broad_phase_subcode::pair_product_overflow,
                              bounded_boolean_error_category::index_overflow,
                              "broad-phase pair product is not representable",
                              broad_phase_checkpoint::representability_preflight);
    return false;
  }
  if (counts.pair_products[0] > capabilities.maximum_pair_product ||
      counts.pair_products[1] > capabilities.maximum_pair_product) {
    error = broad_phase_error(broad_phase_subcode::pair_product_overflow,
                              bounded_boolean_error_category::resource_limit,
                              "broad-phase pair product exceeds configured limit",
                              broad_phase_checkpoint::representability_preflight);
    return false;
  }

  std::uint64_t fixed_entities = 0;
  std::uint64_t fixed_nodes = 0;
  if (!checked_add<std::uint64_t>(counts.edges[0], counts.edges[1],
                                  fixed_entities) ||
      !checked_add<std::uint64_t>(fixed_entities, counts.triangles[0],
                                  fixed_entities) ||
      !checked_add<std::uint64_t>(fixed_entities, counts.triangles[1],
                                  fixed_entities) ||
      !checked_add<std::uint64_t>(counts.node_upper_bounds[0],
                                  counts.node_upper_bounds[1], fixed_nodes)) {
    error = broad_phase_error(broad_phase_subcode::count_overflow,
                              bounded_boolean_error_category::index_overflow,
                              "broad-phase fixed table count overflow",
                              broad_phase_checkpoint::representability_preflight);
    return false;
  }
  std::uint64_t primitive_bytes = 0;
  std::uint64_t edge_bytes = 0;
  std::uint64_t triangle_bytes = 0;
  std::uint64_t node_bytes = 0;
  std::uint64_t count_plan_bytes = 0;
  std::uint64_t edge_count = 0;
  std::uint64_t triangle_count = 0;
  if (!checked_add<std::uint64_t>(counts.edges[0], counts.edges[1], edge_count) ||
      !checked_add<std::uint64_t>(counts.triangles[0], counts.triangles[1], triangle_count) ||
      !checked_multiply<std::uint64_t>(edge_count, sizeof(broad_phase_edge_primitive<T>), edge_bytes) ||
      !checked_multiply<std::uint64_t>(triangle_count, sizeof(broad_phase_triangle_primitive<T>), triangle_bytes) ||
      !checked_add<std::uint64_t>(edge_bytes, triangle_bytes, primitive_bytes) ||
      !checked_multiply<std::uint64_t>(fixed_nodes,
                                      sizeof(triangle_aabb_hierarchy_node<T>),
                                      node_bytes) ||
      !checked_multiply<std::uint64_t>(edge_count,
                                      std::uint64_t{128}, count_plan_bytes) ||
      !checked_add<std::uint64_t>(primitive_bytes, node_bytes,
                                  counts.fixed_persistent_bytes) ||
      !checked_add<std::uint64_t>(counts.fixed_persistent_bytes,
                                  count_plan_bytes,
                                  counts.fixed_persistent_bytes)) {
    error = broad_phase_error(broad_phase_subcode::byte_count_overflow,
                              bounded_boolean_error_category::index_overflow,
                              "broad-phase fixed byte count overflow",
                              broad_phase_checkpoint::representability_preflight);
    return false;
  }
  counts.fixed_temporary_bytes = counts.fixed_persistent_bytes;
  if (!checked_add<std::uint64_t>(fixed_entities, fixed_nodes, counts.fixed_work_units)) {
    error = broad_phase_error(broad_phase_subcode::count_overflow,
                              bounded_boolean_error_category::index_overflow,
                              "broad-phase fixed work count overflow",
                              broad_phase_checkpoint::representability_preflight);
    return false;
  }
  return true;
}

} // namespace ygor::mesh_boolean::bounded
