#pragma once

#include "CanonicalBytes.h"
#include "ContractVersions.h"
#include "FiniteInterval.h"
#include "Identity.h"
#include "Resources.h"
#include "SourceTriangulationTypes.h"
#include "../YgorMeshesBooleanBounded.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

struct manifold_vertex_tag;
struct manifold_triangle_tag;
struct manifold_halfedge_tag;
struct manifold_edge_tag;
struct vertex_fan_tag;
struct source_facet_group_tag;
struct source_shell_group_tag;
struct canonical_geometry_attachment_tag;
struct canonical_halfedge_evidence_tag;

using manifold_vertex_id = strong_id<manifold_vertex_tag>;
using manifold_triangle_id = strong_id<manifold_triangle_tag>;
using manifold_halfedge_id = strong_id<manifold_halfedge_tag>;
using manifold_edge_id = strong_id<manifold_edge_tag>;
using vertex_fan_id = strong_id<vertex_fan_tag>;
using source_facet_group_id = strong_id<source_facet_group_tag>;
using source_shell_group_id = strong_id<source_shell_group_tag>;
using canonical_geometry_attachment_id = strong_id<canonical_geometry_attachment_tag>;
using canonical_halfedge_evidence_id = strong_id<canonical_halfedge_evidence_tag>;

inline constexpr std::uint64_t canonical_invalid_ordinal =
    std::numeric_limits<std::uint64_t>::max();

enum class canonical_edge_class : std::uint8_t {
  source_edge = 1,
  facet_internal_diagonal = 2,
};

enum class canonical_halfedge_verification_disposition : std::uint8_t {
  unverified = 0,
  independently_verified = 1,
};

enum class canonical_halfedge_checkpoint : std::uint32_t {
  contract_validation = 1,
  predecessor_validation = 2,
  represented_domain_preflight = 3,
  resource_reservation = 4,
  vertex_proposals = 5,
  triangle_rotation = 6,
  halfedge_cycles = 7,
  pairing_keys = 8,
  pairing_sort = 9,
  source_edge_validation = 10,
  diagonal_validation = 11,
  edge_assignment = 12,
  incidence_grouping = 13,
  fan_traversal = 14,
  facet_groups = 15,
  shell_groups = 16,
  geometry_attachments = 17,
  producer_verification = 18,
  canonical_encoding = 19,
  independent_verification = 20,
  resource_reconciliation = 21,
  commit = 22,
};

enum class canonical_halfedge_subcode : std::uint32_t {
  unsupported_version = 50001,
  wrong_owner = 50002,
  wrong_operand = 50003,
  predecessor_digest_mismatch = 50004,
  predecessor_not_verified = 50005,
  represented_domain_mismatch = 50006,
  isolated_vertex_leakage = 50007,
  count_overflow = 50008,
  resource_preflight = 50009,
  cancelled = 50010,
  malformed_reference = 50011,
  duplicate_semantic_key = 50012,
  triangle_rotation_mismatch = 50013,
  triangle_cycle_mismatch = 50014,
  pairing_run_size = 50015,
  pairing_endpoint_mismatch = 50016,
  source_edge_provenance_mismatch = 50017,
  diagonal_provenance_mismatch = 50018,
  pair_reciprocity_mismatch = 50019,
  fan_transition_mismatch = 50020,
  fan_coverage_mismatch = 50021,
  facet_group_mismatch = 50022,
  shell_group_mismatch = 50023,
  geometry_basis_mismatch = 50024,
  bound_invalid = 50025,
  owner_in_semantics = 50026,
  internal_diagonal_semantic_contamination = 50027,
  candidate_visibility_conflation = 50028,
  canonical_bytes_mismatch = 50029,
  verifier_rejection = 50030,
  transaction_failure = 50031,
};

inline bounded_boolean_error canonical_halfedge_error(
    operand_id operand, canonical_halfedge_subcode subcode,
    bounded_boolean_error_category category, const char *summary,
    canonical_halfedge_checkpoint checkpoint) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 5;
  error.stage = static_cast<std::uint16_t>(
      operand == operand_id::a ? stage_id::canonical_halfedge_a
                               : stage_id::canonical_halfedge_b);
  error.checkpoint = static_cast<std::uint32_t>(checkpoint);
  error.summary = summary;
  return error;
}

struct canonical_vertex_key final {
  operand_id operand = operand_id::a;
  std::uint64_t source_vertex = 0;
  std::uint32_t occurrence = 0;
  std::uint16_t schema_version = contract_versions::canonical_halfedge_vertex_schema;
  friend bool operator<(const canonical_vertex_key &a,
                        const canonical_vertex_key &b) noexcept {
    return std::tie(a.operand, a.source_vertex, a.occurrence, a.schema_version) <
           std::tie(b.operand, b.source_vertex, b.occurrence, b.schema_version);
  }
  friend bool operator==(const canonical_vertex_key &a,
                         const canonical_vertex_key &b) noexcept {
    return std::tie(a.operand, a.source_vertex, a.occurrence, a.schema_version) ==
           std::tie(b.operand, b.source_vertex, b.occurrence, b.schema_version);
  }
};

struct canonical_edge_use_key final {
  source_triangle_edge_role role = source_triangle_edge_role::source_boundary;
  std::uint64_t primary = 0;
  std::uint64_t secondary = 0;
  std::uint64_t directed_use = 0;
  std::uint64_t source_corner = 0;
  friend bool operator<(const canonical_edge_use_key &a,
                        const canonical_edge_use_key &b) noexcept {
    return std::tie(a.role, a.primary, a.secondary, a.directed_use,
                    a.source_corner) <
           std::tie(b.role, b.primary, b.secondary, b.directed_use,
                    b.source_corner);
  }
  friend bool operator==(const canonical_edge_use_key &a,
                         const canonical_edge_use_key &b) noexcept {
    return std::tie(a.role, a.primary, a.secondary, a.directed_use,
                    a.source_corner) ==
           std::tie(b.role, b.primary, b.secondary, b.directed_use,
                    b.source_corner);
  }
};

struct canonical_triangle_rotation_key final {
  std::array<canonical_vertex_key, 3> vertices{};
  std::array<canonical_edge_use_key, 3> edge_uses{};
  friend bool operator<(const canonical_triangle_rotation_key &a,
                        const canonical_triangle_rotation_key &b) noexcept {
    return std::tie(a.vertices, a.edge_uses) <
           std::tie(b.vertices, b.edge_uses);
  }
  friend bool operator==(const canonical_triangle_rotation_key &a,
                         const canonical_triangle_rotation_key &b) noexcept {
    return a.vertices == b.vertices && a.edge_uses == b.edge_uses;
  }
};

struct canonical_triangle_key final {
  operand_id operand = operand_id::a;
  std::uint64_t source_triangle = 0;
  canonical_triangle_rotation_key rotation{};
  std::uint16_t schema_version = contract_versions::canonical_halfedge_triangle_schema;
  friend bool operator<(const canonical_triangle_key &a,
                        const canonical_triangle_key &b) noexcept {
    return std::tie(a.operand, a.source_triangle, a.rotation,
                    a.schema_version) <
           std::tie(b.operand, b.source_triangle, b.rotation,
                    b.schema_version);
  }
  friend bool operator==(const canonical_triangle_key &a,
                         const canonical_triangle_key &b) noexcept {
    return std::tie(a.operand, a.source_triangle, a.rotation,
                    a.schema_version) ==
           std::tie(b.operand, b.source_triangle, b.rotation,
                    b.schema_version);
  }
};

struct canonical_halfedge_key final {
  canonical_triangle_key triangle{};
  std::uint8_t local_slot = 0;
  std::uint16_t schema_version = contract_versions::canonical_halfedge_halfedge_schema;
  friend bool operator<(const canonical_halfedge_key &a,
                        const canonical_halfedge_key &b) noexcept {
    return std::tie(a.triangle, a.local_slot, a.schema_version) <
           std::tie(b.triangle, b.local_slot, b.schema_version);
  }
  friend bool operator==(const canonical_halfedge_key &a,
                         const canonical_halfedge_key &b) noexcept {
    return std::tie(a.triangle, a.local_slot, a.schema_version) ==
           std::tie(b.triangle, b.local_slot, b.schema_version);
  }
};

struct canonical_edge_key final {
  operand_id operand = operand_id::a;
  canonical_edge_class edge_class = canonical_edge_class::source_edge;
  std::uint64_t primary = 0;
  std::uint64_t secondary = 0;
  std::uint16_t policy_version = contract_versions::canonical_halfedge_pairing_policy;
  friend bool operator<(const canonical_edge_key &a,
                        const canonical_edge_key &b) noexcept {
    return std::tie(a.operand, a.edge_class, a.primary, a.secondary,
                    a.policy_version) <
           std::tie(b.operand, b.edge_class, b.primary, b.secondary,
                    b.policy_version);
  }
  friend bool operator==(const canonical_edge_key &a,
                         const canonical_edge_key &b) noexcept {
    return std::tie(a.operand, a.edge_class, a.primary, a.secondary,
                    a.policy_version) ==
           std::tie(b.operand, b.edge_class, b.primary, b.secondary,
                    b.policy_version);
  }
};

template <class T> struct canonical_bound3 final {
  std::uint16_t schema_version =
      contract_versions::canonical_halfedge_geometry_attachment_schema;
  std::uint16_t formula_version =
      contract_versions::canonical_halfedge_bound_formula;
  std::array<finite_interval<T>, 3> axes{};

  bool valid() const noexcept {
    if (schema_version !=
            contract_versions::canonical_halfedge_geometry_attachment_schema ||
        formula_version != contract_versions::canonical_halfedge_bound_formula)
      return false;
    for (const auto &axis : axes)
      if (!finite_bits(axis.lower()) || !finite_bits(axis.upper()) ||
          finite_numeric_less(axis.upper(), axis.lower()))
        return false;
    return true;
  }
  bool contains(const canonical_bound3 &other) const noexcept {
    if (!valid() || !other.valid())
      return false;
    for (std::size_t axis = 0; axis < 3; ++axis)
      if (finite_numeric_less(other.axes[axis].lower(), axes[axis].lower()) ||
          finite_numeric_less(axes[axis].upper(), other.axes[axis].upper()))
        return false;
    return true;
  }
};

template <class T>
inline canonical_bound3<T> canonical_bound_hull(const canonical_bound3<T> &a,
                                                 const canonical_bound3<T> &b) {
  canonical_bound3<T> out;
  for (std::size_t axis = 0; axis < 3; ++axis)
    out.axes[axis] = interval_hull(a.axes[axis], b.axes[axis]);
  return out;
}

struct canonical_halfedge_statistics final {
  std::uint64_t represented_vertices = 0;
  std::uint64_t isolated_vertices_excluded = 0;
  std::uint64_t triangles = 0;
  std::uint64_t halfedges = 0;
  std::uint64_t source_edges = 0;
  std::uint64_t internal_diagonals = 0;
  std::uint64_t fan_transitions = 0;
  std::uint64_t pairing_comparisons = 0;
  std::uint64_t verifier_work = 0;
  std::uint64_t work_units = 0;
  std::uint64_t persistent_bytes = 0;
};

struct canonical_halfedge_capabilities final {
  std::uint16_t version = contract_versions::canonical_halfedge_provider;
  std::uint16_t policy_version = contract_versions::canonical_halfedge_policy;
  context_owner_token owner{};
  const bounded_boolean_cancellation_token *cancellation = nullptr;
  resource_manager *resources = nullptr;
  std::uint64_t maximum_entities = (std::uint64_t{1} << 34);
  std::uint64_t maximum_work_units = (std::uint64_t{1} << 36);
  std::uint64_t maximum_canonical_bytes = (std::uint64_t{1} << 34);
  std::uint32_t reserved = 0;
};

inline bool canonical_halfedge_cancelled(
    const canonical_halfedge_capabilities &capabilities) noexcept {
  return capabilities.cancellation &&
         capabilities.cancellation->cancellation_requested();
}

} // namespace ygor::mesh_boolean::bounded
