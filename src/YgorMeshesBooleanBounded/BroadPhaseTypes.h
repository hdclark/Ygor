#pragma once

#include "CanonicalHalfedgeTypes.h"
#include "CheckedArithmetic.h"
#include "Context.h"
#include "FloatingBits.h"
#include "Identity.h"
#include "Resources.h"

#include <array>
#include <cstdint>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct broad_phase_edge_primitive_tag;
struct broad_phase_triangle_primitive_tag;
struct broad_phase_node_tag;
struct overlap_witness_tag;
struct candidate_partition_tag;
struct broad_phase_verifier_evidence_tag;

using broad_phase_edge_primitive_id = strong_id<broad_phase_edge_primitive_tag>;
using broad_phase_triangle_primitive_id = strong_id<broad_phase_triangle_primitive_tag>;
using broad_phase_node_id = strong_id<broad_phase_node_tag>;
using overlap_witness_id = strong_id<overlap_witness_tag>;
using candidate_partition_id = strong_id<candidate_partition_tag>;
using broad_phase_verifier_evidence_id = strong_id<broad_phase_verifier_evidence_tag>;

inline constexpr std::uint64_t broad_phase_invalid_ordinal =
    std::numeric_limits<std::uint64_t>::max();
inline constexpr std::uint32_t broad_phase_leaf_capacity_v1 = 8;
inline constexpr std::uint64_t broad_phase_partition_capacity_v1 = 4096;

// These values are part of the replay and canonical-byte contract.
enum class directed_candidate_role : std::uint8_t {
  a_edge_b_triangle = 1,
  b_edge_a_triangle = 2,
};

enum class broad_phase_relation_family : std::uint8_t {
  canonical_edge_source_triangle = 1,
};

enum class topological_filter_reason : std::uint8_t {
  not_filtered = 1,
  same_operand_incident_triangle = 2,
  policy_excluded_internal_diagonal = 3,
};

enum class hierarchy_node_kind : std::uint8_t {
  leaf = 1,
  internal = 2,
};

enum class broad_phase_provider_kind : std::uint8_t {
  rank_morton_triangle_aabb_hierarchy_v1 = 1,
};

enum class candidate_domain_kind : std::uint8_t {
  all_canonical_edges_against_all_opposite_source_triangles_v1 = 1,
};

enum class broad_phase_verification_disposition : std::uint8_t {
  unverified = 0,
  independently_verified = 1,
};

enum class axis_overlap_category : std::uint8_t {
  left_definitely_below_right = 1,
  right_definitely_below_left = 2,
  closed_overlap = 3,
};

enum class candidate_domain_disposition : std::uint8_t {
  included = 1,
  excluded = 2,
};

enum class broad_phase_checkpoint : std::uint32_t {
  context_capability_validation = 1,
  predecessor_validation = 2,
  provider_policy_validation = 3,
  representability_preflight = 4,
  fixed_resource_reservation = 5,
  operand_a_primitive_construction = 6,
  operand_b_primitive_construction = 7,
  independent_bound_reconstruction = 8,
  operand_a_dense_ranking = 9,
  operand_a_spatial_order = 10,
  operand_a_hierarchy = 11,
  operand_a_producer_verification = 12,
  operand_b_dense_ranking = 13,
  operand_b_spatial_order = 14,
  operand_b_hierarchy = 15,
  operand_b_producer_verification = 16,
  a_edge_b_triangle_count = 17,
  b_edge_a_triangle_count = 18,
  prefix_and_candidate_reservation = 19,
  a_edge_b_triangle_emit = 20,
  b_edge_a_triangle_emit = 21,
  count_emit_reconciliation = 22,
  candidate_witness_validation = 23,
  complete_key_canonicalization = 24,
  candidate_id_assignment = 25,
  partition_construction = 26,
  producer_artifact_checks = 27,
  canonical_encoding = 28,
  independent_provider_reconstruction = 29,
  independent_breadth_first_verification = 30,
  exhaustive_all_pairs_verification = 31,
  codec_digest_resource_reconciliation = 32,
  transaction_commit = 33,
};

enum class broad_phase_subcode : std::uint32_t {
  unsupported_version = 60001,
  wrong_owner = 60002,
  wrong_operand = 60003,
  predecessor_digest_mismatch = 60004,
  predecessor_not_verified = 60005,
  malformed_role = 60006,
  count_overflow = 60007,
  pair_product_overflow = 60008,
  node_count_overflow = 60009,
  byte_count_overflow = 60010,
  resource_preflight = 60011,
  cancelled = 60012,
  malformed_edge_primitive = 60013,
  malformed_triangle_primitive = 60014,
  malformed_bound = 60015,
  bound_reconstruction_mismatch = 60016,
  domain_inclusion_mismatch = 60017,
  internal_diagonal_contamination = 60018,
  duplicate_edge_representative = 60019,
  malformed_endpoint_key = 60020,
  malformed_dense_rank = 60021,
  malformed_rank_morton_key = 60022,
  malformed_spatial_order = 60023,
  malformed_leaf = 60024,
  malformed_internal_node = 60025,
  malformed_root = 60026,
  hierarchy_containment_failure = 60027,
  hierarchy_cycle = 60028,
  traversal_limit = 60029,
  emit_count_mismatch = 60030,
  malformed_overlap_witness = 60031,
  definitely_separated_pair_emitted = 60032,
  nonseparated_pair_omitted = 60033,
  candidate_key_mismatch = 60034,
  duplicate_candidate_key = 60035,
  candidate_order_mismatch = 60036,
  candidate_id_mismatch = 60037,
  partition_mismatch = 60038,
  codec_error = 60039,
  digest_mismatch = 60040,
  independent_provider_mismatch = 60041,
  independent_candidate_set_mismatch = 60042,
  exhaustive_false_negative = 60043,
  counter_mismatch = 60044,
  resource_reconciliation = 60045,
  transaction_failure = 60046,
  owner_in_semantics = 60047,
  internal_invariant = 60048,
};

inline bounded_boolean_error broad_phase_error(
    broad_phase_subcode subcode, bounded_boolean_error_category category,
    const char *summary, broad_phase_checkpoint checkpoint) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 6;
  error.stage = static_cast<std::uint16_t>(stage_id::broad_phase);
  error.checkpoint = static_cast<std::uint32_t>(checkpoint);
  error.summary = summary;
  return error;
}

inline constexpr std::size_t operand_slot(operand_id operand) noexcept {
  return operand == operand_id::a ? std::size_t{0} : std::size_t{1};
}

inline constexpr operand_id opposite_operand(operand_id operand) noexcept {
  return operand == operand_id::a ? operand_id::b : operand_id::a;
}

inline constexpr operand_id role_edge_operand(directed_candidate_role role) noexcept {
  return role == directed_candidate_role::a_edge_b_triangle ? operand_id::a
                                                             : operand_id::b;
}

inline constexpr operand_id role_triangle_operand(directed_candidate_role role) noexcept {
  return opposite_operand(role_edge_operand(role));
}

template <class T> struct axis_endpoint_key final {
  floating_uint_t<T> lower = 0;
  floating_uint_t<T> upper = 0;

  friend bool operator<(const axis_endpoint_key &a,
                        const axis_endpoint_key &b) noexcept {
    return std::tie(a.lower, a.upper) < std::tie(b.lower, b.upper);
  }
  friend bool operator==(const axis_endpoint_key &a,
                         const axis_endpoint_key &b) noexcept {
    return a.lower == b.lower && a.upper == b.upper;
  }
  friend bool operator!=(const axis_endpoint_key &a,
                         const axis_endpoint_key &b) noexcept {
    return !(a == b);
  }
};

struct rank_morton_key final {
  std::uint16_t active_rank_bits = 0;
  std::vector<std::uint64_t> words;

  friend bool operator<(const rank_morton_key &a,
                        const rank_morton_key &b) noexcept {
    return std::tie(a.active_rank_bits, a.words) <
           std::tie(b.active_rank_bits, b.words);
  }
  friend bool operator==(const rank_morton_key &a,
                         const rank_morton_key &b) noexcept {
    return a.active_rank_bits == b.active_rank_bits && a.words == b.words;
  }
  friend bool operator!=(const rank_morton_key &a,
                         const rank_morton_key &b) noexcept {
    return !(a == b);
  }
};

struct hierarchy_node_key final {
  operand_id operand = operand_id::a;
  std::uint64_t first_spatial_primitive = 0;
  std::uint64_t subtree_primitive_count = 0;
  hierarchy_node_kind kind = hierarchy_node_kind::leaf;
  std::uint32_t level_from_leaves = 0;
  std::uint16_t layout_version = 1;

  friend bool operator<(const hierarchy_node_key &a,
                        const hierarchy_node_key &b) noexcept {
    return std::tie(a.operand, a.first_spatial_primitive,
                    a.subtree_primitive_count, a.kind, a.level_from_leaves,
                    a.layout_version) <
           std::tie(b.operand, b.first_spatial_primitive,
                    b.subtree_primitive_count, b.kind, b.level_from_leaves,
                    b.layout_version);
  }
  friend bool operator==(const hierarchy_node_key &a,
                         const hierarchy_node_key &b) noexcept {
    return std::tie(a.operand, a.first_spatial_primitive,
                    a.subtree_primitive_count, a.kind, a.level_from_leaves,
                    a.layout_version) ==
           std::tie(b.operand, b.first_spatial_primitive,
                    b.subtree_primitive_count, b.kind, b.level_from_leaves,
                    b.layout_version);
  }
};

struct canonical_candidate_key final {
  directed_candidate_role role = directed_candidate_role::a_edge_b_triangle;
  broad_phase_relation_family family =
      broad_phase_relation_family::canonical_edge_source_triangle;
  canonical_edge_key edge{};
  canonical_triangle_key triangle{};
  canonical_edge_class edge_class = canonical_edge_class::source_edge;
  std::uint16_t domain_policy_version = 1;

  friend bool operator<(const canonical_candidate_key &a,
                        const canonical_candidate_key &b) noexcept {
    return std::tie(a.role, a.family, a.edge, a.triangle, a.edge_class,
                    a.domain_policy_version) <
           std::tie(b.role, b.family, b.edge, b.triangle, b.edge_class,
                    b.domain_policy_version);
  }
  friend bool operator==(const canonical_candidate_key &a,
                         const canonical_candidate_key &b) noexcept {
    return std::tie(a.role, a.family, a.edge, a.triangle, a.edge_class,
                    a.domain_policy_version) ==
           std::tie(b.role, b.family, b.edge, b.triangle, b.edge_class,
                    b.domain_policy_version);
  }
  friend bool operator!=(const canonical_candidate_key &a,
                         const canonical_candidate_key &b) noexcept {
    return !(a == b);
  }
};

struct overlap_witness_key final {
  canonical_candidate_key candidate{};
  std::uint64_t admitting_leaf = 0;
  std::uint64_t triangle_spatial_slot = 0;
  std::array<axis_overlap_category, 3> axes{
      axis_overlap_category::closed_overlap,
      axis_overlap_category::closed_overlap,
      axis_overlap_category::closed_overlap};

  friend bool operator<(const overlap_witness_key &a,
                        const overlap_witness_key &b) noexcept {
    return std::tie(a.candidate, a.admitting_leaf, a.triangle_spatial_slot,
                    a.axes) <
           std::tie(b.candidate, b.admitting_leaf, b.triangle_spatial_slot,
                    b.axes);
  }
  friend bool operator==(const overlap_witness_key &a,
                         const overlap_witness_key &b) noexcept {
    return std::tie(a.candidate, a.admitting_leaf, a.triangle_spatial_slot,
                    a.axes) ==
           std::tie(b.candidate, b.admitting_leaf, b.triangle_spatial_slot,
                    b.axes);
  }
};

template <class T> struct broad_phase_bound_relation final {
  std::array<axis_overlap_category, 3> axes{
      axis_overlap_category::closed_overlap,
      axis_overlap_category::closed_overlap,
      axis_overlap_category::closed_overlap};
  bool definitely_separated = false;
};

template <class T>
inline broad_phase_bound_relation<T>
classify_closed_bound_relation(const canonical_bound3<T> &left,
                               const canonical_bound3<T> &right) noexcept {
  broad_phase_bound_relation<T> result;
  if (!left.valid() || !right.valid()) {
    result.definitely_separated = true;
    return result;
  }
  for (std::size_t axis = 0; axis < 3; ++axis) {
    if (finite_numeric_less(left.axes[axis].upper(),
                            right.axes[axis].lower())) {
      result.axes[axis] = axis_overlap_category::left_definitely_below_right;
      result.definitely_separated = true;
    } else if (finite_numeric_less(right.axes[axis].upper(),
                                   left.axes[axis].lower())) {
      result.axes[axis] = axis_overlap_category::right_definitely_below_left;
      result.definitely_separated = true;
    } else {
      result.axes[axis] = axis_overlap_category::closed_overlap;
    }
  }
  return result;
}

struct broad_phase_capabilities final {
  std::uint16_t version = 1;
  std::uint16_t provider_version = 1;
  std::uint16_t domain_policy_version = 1;
  std::uint16_t verifier_version = 1;
  context_owner_token owner{};
  const bounded_boolean_cancellation_token *cancellation = nullptr;
  resource_manager *resources = nullptr;
  std::uint64_t maximum_entities = (std::uint64_t{1} << 34);
  std::uint64_t maximum_pair_product = (std::uint64_t{1} << 38);
  std::uint64_t maximum_nodes = (std::uint64_t{1} << 35);
  std::uint64_t maximum_candidates = (std::uint64_t{1} << 36);
  std::uint64_t maximum_work_units = (std::uint64_t{1} << 39);
  std::uint64_t maximum_canonical_bytes = (std::uint64_t{1} << 35);
  std::uint64_t exhaustive_pair_threshold = (std::uint64_t{1} << 20);
  std::uint64_t partition_capacity = broad_phase_partition_capacity_v1;
  std::uint32_t cancellation_poll_interval = 1024;
  std::uint32_t reserved = 0;
};

inline bool broad_phase_cancelled(
    const broad_phase_capabilities &capabilities) noexcept {
  return capabilities.cancellation &&
         capabilities.cancellation->cancellation_requested();
}

struct broad_phase_preflight_counts final {
  std::array<std::uint64_t, 2> edges{};
  std::array<std::uint64_t, 2> source_edges{};
  std::array<std::uint64_t, 2> internal_diagonals{};
  std::array<std::uint64_t, 2> triangles{};
  std::array<std::uint64_t, 2> leaves{};
  std::array<std::uint64_t, 2> node_upper_bounds{};
  std::array<std::uint64_t, 2> pair_products{};
  std::uint64_t fixed_temporary_bytes = 0;
  std::uint64_t fixed_persistent_bytes = 0;
  std::uint64_t fixed_work_units = 0;
};

struct broad_phase_statistics final {
  std::array<std::uint64_t, 2> edge_counts{};
  std::array<std::uint64_t, 2> source_edge_counts{};
  std::array<std::uint64_t, 2> internal_diagonal_counts{};
  std::array<std::uint64_t, 2> triangle_counts{};
  std::array<std::uint64_t, 2> distinct_rank_counts_x{};
  std::array<std::uint64_t, 2> distinct_rank_counts_y{};
  std::array<std::uint64_t, 2> distinct_rank_counts_z{};
  std::array<std::uint64_t, 2> leaf_counts{};
  std::array<std::uint64_t, 2> node_counts{};
  std::array<std::uint64_t, 2> hierarchy_heights{};
  std::array<std::uint64_t, 2> role_node_tests{};
  std::array<std::uint64_t, 2> role_leaf_visits{};
  std::array<std::uint64_t, 2> role_primitive_tests{};
  std::array<std::uint64_t, 2> role_definite_prunes{};
  std::array<std::uint64_t, 2> role_candidates{};
  std::array<std::uint64_t, 2> role_emit_node_tests{};
  std::array<std::uint64_t, 2> role_emit_primitive_tests{};
  std::uint64_t duplicate_candidates = 0;
  std::uint64_t candidate_count = 0;
  std::uint64_t partition_count = 0;
  std::uint64_t maximum_producer_stack = 0;
  std::uint64_t maximum_verifier_queue = 0;
  std::uint64_t producer_work_units = 0;
  std::uint64_t verifier_work_units = 0;
  std::uint64_t persistent_bytes = 0;
  std::uint64_t canonical_bytes = 0;
};

} // namespace ygor::mesh_boolean::bounded
