#pragma once

#include "CanonicalBytes.h"
#include "ContractVersions.h"
#include "FiniteInterval.h"
#include "Identity.h"
#include "Resources.h"
#include "../YgorMeshesBooleanBounded.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <tuple>
#include <vector>

namespace ygor::mesh_boolean::bounded {

enum class source_geometry_basis_kind : std::uint8_t {
  nominal_embedded = 1,
  constructed_coherent_realization = 2,
};

enum class source_triangle_edge_role : std::uint8_t {
  source_boundary = 1,
  facet_internal_diagonal = 2,
};

enum class source_triangulation_provider_kind : std::uint8_t {
  full_rescan_reference_v1 = 1,
  indexed_dependency_v1 = 2,
};

enum class source_candidate_disposition : std::uint8_t {
  eligible = 1,
  definitely_rejected = 2,
  uncertainty_rejected = 3,
};

enum class bounded_planar_sign : std::int8_t {
  negative = -1,
  uncertain = 0,
  positive = 1,
};

enum class bounded_segment_relation : std::uint8_t {
  definitely_disjoint = 1,
  endpoint_contact = 2,
  proper_crossing = 3,
  collinear_overlap = 4,
  uncertain = 5,
};

enum class source_triangulation_checkpoint : std::uint32_t {
  contract_validation = 1,
  geometry_basis_validation = 2,
  preflight = 3,
  projection = 4,
  active_ring = 5,
  initial_candidates = 6,
  ear_loop = 7,
  final_triangle = 8,
  provenance = 9,
  combinatorial_coverage = 10,
  geometric_coverage = 11,
  canonicalization = 12,
  cross_facet = 13,
  independent_verification = 14,
  encoding = 15,
  commit = 16,
};

enum class source_triangulation_subcode : std::uint32_t {
  unsupported_policy = 40001,
  wrong_owner = 40002,
  wrong_operand = 40003,
  predecessor_digest_mismatch = 40004,
  topology_only_predecessor = 40005,
  geometry_basis_missing = 40006,
  geometry_basis_mismatch = 40007,
  count_overflow = 40008,
  malformed_reference = 40009,
  projected_point_unavailable = 40101,
  projection_mismatch = 40102,
  active_ring_corrupt = 40103,
  repeated_source_vertex = 40104,
  boundary_lookup_mismatch = 40105,
  ear_orientation_uncertain = 40201,
  local_cone_uncertain = 40202,
  diagonal_relation_uncertain = 40203,
  point_on_diagonal = 40204,
  point_in_ear = 40205,
  no_certified_ear = 40206,
  no_legal_ear = 40207,
  dependency_closure = 40208,
  provider_trace_mismatch = 40209,
  work_guard = 40210,
  final_triangle_invalid = 40211,
  count_mismatch = 40301,
  source_boundary_mismatch = 40302,
  diagonal_pair_mismatch = 40303,
  role_contradiction = 40304,
  provenance_incomplete = 40305,
  dual_disconnected = 40306,
  triangle_outside = 40307,
  forbidden_overlap = 40308,
  witness_coverage = 40309,
  area_mismatch = 40310,
  canonical_collision = 40311,
  owner_in_semantics = 40312,
  verifier_rejection = 40313,
  resource_preflight = 40401,
  cancelled = 40402,
};

inline bounded_boolean_error source_triangulation_error(
    operand_id operand, source_triangulation_subcode subcode,
    bounded_boolean_error_category category, const char *summary,
    source_triangulation_checkpoint checkpoint) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 4;
  error.stage = static_cast<std::uint16_t>(
      operand == operand_id::a ? stage_id::source_triangulation_a
                               : stage_id::source_triangulation_b);
  error.checkpoint = static_cast<std::uint32_t>(checkpoint);
  error.summary = summary;
  return error;
}

struct source_triangle_key final {
  operand_id operand = operand_id::a;
  std::uint64_t facet = 0;
  std::array<std::uint64_t, 3> vertices{};
  std::uint16_t policy_version = contract_versions::source_triangulation_policy;
  friend bool operator<(const source_triangle_key &a,
                        const source_triangle_key &b) noexcept {
    return std::tie(a.operand, a.facet, a.vertices, a.policy_version) <
           std::tie(b.operand, b.facet, b.vertices, b.policy_version);
  }
  friend bool operator==(const source_triangle_key &a,
                         const source_triangle_key &b) noexcept {
    return a.operand == b.operand && a.facet == b.facet &&
           a.vertices == b.vertices && a.policy_version == b.policy_version;
  }
};

struct source_diagonal_key final {
  operand_id operand = operand_id::a;
  std::uint64_t facet = 0;
  std::array<std::uint64_t, 2> endpoints{};
  std::uint16_t policy_version = contract_versions::source_triangulation_policy;
  friend bool operator<(const source_diagonal_key &a,
                        const source_diagonal_key &b) noexcept {
    return std::tie(a.operand, a.facet, a.endpoints, a.policy_version) <
           std::tie(b.operand, b.facet, b.endpoints, b.policy_version);
  }
  friend bool operator==(const source_diagonal_key &a,
                         const source_diagonal_key &b) noexcept {
    return a.operand == b.operand && a.facet == b.facet &&
           a.endpoints == b.endpoints && a.policy_version == b.policy_version;
  }
};

struct source_ear_candidate_key final {
  operand_id operand = operand_id::a;
  std::uint64_t facet = 0;
  std::array<std::uint64_t, 3> triangle{};
  std::array<std::uint64_t, 2> diagonal{};
  std::uint64_t corner_vertex = 0;
  std::uint16_t policy_version = contract_versions::source_triangulation_policy;
  friend bool operator<(const source_ear_candidate_key &a,
                        const source_ear_candidate_key &b) noexcept {
    return std::tie(a.operand, a.facet, a.triangle, a.diagonal,
                    a.corner_vertex, a.policy_version) <
           std::tie(b.operand, b.facet, b.triangle, b.diagonal,
                    b.corner_vertex, b.policy_version);
  }
  friend bool operator==(const source_ear_candidate_key &a,
                         const source_ear_candidate_key &b) noexcept {
    return a.operand == b.operand && a.facet == b.facet &&
           a.triangle == b.triangle && a.diagonal == b.diagonal &&
           a.corner_vertex == b.corner_vertex &&
           a.policy_version == b.policy_version;
  }
};

template <class T> struct projected_source_point final {
  std::uint64_t source_vertex = 0;
  std::uint64_t source_corner = 0;
  std::array<T, 2> nominal{};
  std::array<finite_interval<T>, 2> enclosure{};
};

template <class T> struct source_orientation_evidence final {
  finite_interval<T> determinant{};
  std::int8_t exact_sign = 0;
  bounded_planar_sign bounded_sign = bounded_planar_sign::uncertain;
  std::uint16_t formula_version = contract_versions::bounded_source_polygon_kernel;
};

struct facet_geometry_basis_ref final {
  source_geometry_basis_kind kind = source_geometry_basis_kind::nominal_embedded;
  operand_id operand = operand_id::a;
  std::uint64_t facet = 0;
  std::uint64_t ring = 0;
  std::uint64_t shell = 0;
  std::uint8_t dropped_axis = 0;
  std::array<std::uint64_t, 3> support_vertices{};
  bounded_boolean_digest predecessor_digest{};
  bounded_boolean_digest precision_digest{};
  bounded_boolean_digest basis_digest{};
};

template <class T> struct source_triangle_vertex_ref final {
  std::uint64_t source_vertex = 0;
  std::uint64_t shell = 0;
  std::array<std::uint64_t, 3> nominal_bits{};
  std::array<T, 3> lower{};
  std::array<T, 3> upper{};
  T radial_error = T(0);
  std::uint64_t presentation_vertex = 0;
  std::vector<std::uint64_t> incident_triangles;
};

struct source_triangle_edge_use final {
  source_triangle_edge_role role = source_triangle_edge_role::source_boundary;
  std::uint64_t origin = 0;
  std::uint64_t destination = 0;
  std::uint64_t triangle = 0;
  std::uint8_t local_slot = 0;
  std::uint64_t facet = 0;
  std::uint64_t ring = 0;
  std::uint64_t shell = 0;
  std::uint64_t source_directed_use = 0;
  std::uint64_t source_undirected_edge = 0;
  std::uint64_t source_corner = 0;
  std::uint64_t reciprocal_source_use = 0;
  std::uint64_t diagonal = 0;
  std::uint64_t opposite_edge_use = 0;
  bool source_feature_eligible() const noexcept {
    return role == source_triangle_edge_role::source_boundary;
  }
  bool source_edge_candidate_eligible() const noexcept {
    return role == source_triangle_edge_role::source_boundary;
  }
  bool symbolic_owner_eligible() const noexcept {
    return role == source_triangle_edge_role::source_boundary;
  }
  bool classification_barrier() const noexcept {
    return role == source_triangle_edge_role::source_boundary;
  }
};

template <class T> struct source_triangle_record final {
  std::uint64_t canonical_id = 0;
  source_triangle_key key{};
  std::uint64_t ring = 0;
  std::uint64_t shell = 0;
  std::array<std::uint64_t, 3> vertices{};
  std::array<std::uint64_t, 3> edge_uses{};
  facet_geometry_basis_ref basis{};
  source_orientation_evidence<T> orientation{};
};

struct source_internal_diagonal_record final {
  std::uint64_t canonical_id = 0;
  source_diagonal_key key{};
  std::uint64_t ring = 0;
  std::uint64_t shell = 0;
  std::array<std::uint64_t, 2> triangle_uses{};
  std::array<std::uint64_t, 2> edge_uses{};
  bool source_feature_eligible = false;
  bool source_edge_candidate_eligible = false;
  bool symbolic_owner_eligible = false;
  bool classification_barrier = false;
};

struct source_triangulation_witness_record final {
  std::uint64_t facet = 0;
  std::uint64_t ordinal = 0;
  std::uint64_t containing_triangle = 0;
  std::array<std::uint64_t, 3> source_vertices{};
};

struct source_facet_triangulation_record final {
  std::uint64_t facet = 0;
  std::uint64_t ring = 0;
  std::uint64_t shell = 0;
  std::vector<std::uint64_t> source_vertices;
  std::vector<std::uint64_t> triangles;
  std::vector<std::uint64_t> diagonals;
  std::vector<source_triangulation_witness_record> producer_witnesses;
  std::vector<source_triangulation_witness_record> verifier_witnesses;
  bounded_boolean_digest semantic_digest{};
  bounded_boolean_digest exact_digest{};
};

struct source_candidate_dependency_record final {
  source_ear_candidate_key key{};
  source_candidate_disposition disposition =
      source_candidate_disposition::definitely_rejected;
  std::vector<std::uint64_t> active_vertices;
  std::vector<std::array<std::uint64_t, 2>> active_segments;
  std::vector<std::uint64_t> point_blockers;
  std::vector<std::array<std::uint64_t, 2>> segment_blockers;
  std::uint64_t generation = 0;
};

struct source_ear_trace_step final {
  std::uint64_t facet = 0;
  std::uint64_t generation = 0;
  std::vector<std::uint64_t> active_ring;
  std::vector<source_candidate_dependency_record> candidates;
  source_ear_candidate_key selected{};
  bool selected_valid = false;
};

struct source_triangulation_statistics final {
  std::uint64_t projected_points = 0;
  std::uint64_t candidate_evaluations = 0;
  std::uint64_t full_rescans = 0;
  std::uint64_t dependency_records = 0;
  std::uint64_t segment_relations = 0;
  std::uint64_t point_relations = 0;
  std::uint64_t triangles = 0;
  std::uint64_t diagonals = 0;
  std::uint64_t pair_audits = 0;
  std::uint64_t witness_audits = 0;
  std::uint64_t verifier_work = 0;
  std::uint64_t work_units = 0;
  std::uint64_t persistent_bytes = 0;
};

struct source_triangulation_capabilities final {
  std::uint16_t version = contract_versions::source_triangulation_provider;
  source_triangulation_provider_kind provider =
      source_triangulation_provider_kind::indexed_dependency_v1;
  context_owner_token owner{};
  const bounded_boolean_cancellation_token *cancellation = nullptr;
  resource_manager *resources = nullptr;
  std::uint64_t maximum_ring_size = 4096;
  std::uint64_t maximum_witnesses_per_facet = 64;
  std::uint64_t maximum_work_units = (std::uint64_t{1} << 34);
  bool compare_with_reference = false;
  std::uint32_t reserved = 0;
};

inline std::array<std::uint64_t, 3>
minimum_orientation_preserving_rotation(
    const std::array<std::uint64_t, 3> &vertices) noexcept {
  const std::array<std::array<std::uint64_t, 3>, 3> rotations{{
      vertices,
      {vertices[1], vertices[2], vertices[0]},
      {vertices[2], vertices[0], vertices[1]},
  }};
  return *std::min_element(rotations.begin(), rotations.end());
}

inline std::array<std::uint64_t, 2>
ordered_endpoints(std::uint64_t a, std::uint64_t b) noexcept {
  return a < b ? std::array<std::uint64_t, 2>{a, b}
               : std::array<std::uint64_t, 2>{b, a};
}

} // namespace ygor::mesh_boolean::bounded
