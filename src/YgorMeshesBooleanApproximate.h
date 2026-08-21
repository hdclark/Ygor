#pragma once
#ifndef YGOR_MESHES_BOOLEAN_APPROXIMATE_H_
#define YGOR_MESHES_BOOLEAN_APPROXIMATE_H_

#include "YgorMeshesBooleanExactResult.h"
#include "YgorMeshesBooleanRealization.h"

#include <functional>

namespace ygor {
namespace mesh_boolean {

constexpr std::uint16_t certified_approximate_embedding_schema = 1;
constexpr std::uint16_t certified_approximate_embedding_checker_version = 1;
constexpr std::uint32_t approximate_search_exhausted_subcode = 1;

struct approximate_verifier_budget {
  explicit approximate_verifier_budget(const realization_search_policy &p)
      : limits(p) {}

  bool work(std::uint64_t amount, const char *key) {
    return charge(verifier_work, amount, limits.max_verifier_work, key);
  }
  bool records(std::uint64_t amount, const char *key) {
    return charge(verifier_records, amount, limits.max_verifier_records, key);
  }
  bool bytes(std::uint64_t amount, const char *key) {
    return charge(verifier_bytes, amount, limits.max_verifier_bytes, key);
  }
  bool predicate(const char *key) {
    if (failed())
      return false;
    if (predicate_checks == limits.max_predicate_checks) {
      failure_key = key;
      return false;
    }
    if (!work(1, "approximate_verifier.work_limit"))
      return false;
    ++predicate_checks;
    return true;
  }
  bool failed() const noexcept { return failure_key != nullptr; }

  realization_search_policy limits;
  std::uint64_t verifier_work = 0;
  std::uint64_t verifier_records = 0;
  std::uint64_t verifier_bytes = 0;
  std::uint64_t predicate_checks = 0;
  const char *failure_key = nullptr;

private:
  bool charge(std::uint64_t &value, std::uint64_t amount,
              std::uint64_t limit, const char *key) {
    if (failed())
      return false;
    if (amount > limit || value > limit - amount) {
      failure_key = key;
      return false;
    }
    value += amount;
    return true;
  }
};

struct approximate_decode_limits {
  std::uint64_t max_record_bytes = 128U * 1024U * 1024U;
  std::uint64_t max_exact_result_bytes = 64U * 1024U * 1024U;
  std::uint64_t max_output_bytes = 64U * 1024U * 1024U;
  std::uint64_t max_certificate_bytes = 64U * 1024U * 1024U;
  std::uint64_t max_entities = 4U * 1024U * 1024U;
  std::uint64_t max_references = 32U * 1024U * 1024U;
  std::uint64_t max_exact_integer_bytes = 8U * 1024U * 1024U;
};

enum class approximate_obligation_kind : std::uint8_t {
  occurrence_isomorphic_topology,
  distinct_required_vertices,
  selected_edge_order,
  required_incidence,
  triangle_noncollapse,
  triangle_orientation,
  support_plane_deviation,
  prohibited_nonadjacent_intersection,
  vertex_link_order,
  edge_radial_order,
  defining_relation,
  global_embedding
};

enum class approximate_relaxed_relation_kind : std::uint8_t {
  target_coordinate,
  original_vertex_coordinate,
  support_plane,
  defining_relation
};

template <class T> struct approximate_vertex_evidence {
  selected_vertex_id selected;
  exact_point3 exact_target;
  std::array<coordinate_bits<T>, 3> output_bits;
  exact_point3 displacement;
  exact_scalar squared_displacement;
  std::array<std::uint32_t, 3> accepted_axis_ranks{{0, 0, 0}};
  std::uint64_t accepted_candidate_rank = 0;
  bool original_vertex = false;
};

struct approximate_triangle_evidence {
  realization_triangle_id triangle;
  selected_patch_id patch;
  std::array<selected_vertex_id, 3> vertices;
  projection_axis projection = projection_axis::drop_z;
  exact_sign exact_orientation = exact_sign::positive;
  std::array<exact_scalar, 3> support_plane_deviations;
  std::array<exact_scalar, 3> normalized_squared_support_plane_deviations;
};

struct approximate_occurrence_map {
  selected_vertex_occurrence_id occurrence;
  selected_vertex_id output_vertex;
  std::vector<selected_halfedge_id> exact_cyclic_halfedges;
  std::vector<realization_triangle_id> output_cyclic_triangles;
};

struct approximate_patch_adjacency {
  selected_edge_id edge;
  std::vector<selected_patch_id> exact_radial_patches;
  std::vector<realization_triangle_id> output_incident_triangles;
};

struct approximate_obligation_evidence {
  approximate_obligation_kind kind =
      approximate_obligation_kind::global_embedding;
  std::vector<std::uint64_t> vertices;
  std::vector<std::uint64_t> triangles;
  std::vector<std::uint64_t> exact_entities;
  bool passed = false;
  exact_scalar measured_value;
  exact_scalar allowed_value;
};

struct approximate_relaxed_relation {
  approximate_relaxed_relation_kind kind =
      approximate_relaxed_relation_kind::target_coordinate;
  std::uint64_t exact_entity = 0;
  std::uint8_t axis = 0;
  exact_scalar exact_value;
  exact_scalar emitted_value;
  exact_scalar absolute_deviation;
  exact_scalar allowed_deviation;
  std::optional<defining_relation_id> defining_relation;
  defining_relation_kind relation_kind =
      defining_relation_kind::coordinate_equality;
  std::uint16_t relation_formula_version = 0;
  exact_sign relation_expected = exact_sign::zero;
  exact_scalar exact_residual;
  exact_scalar emitted_residual;
};

struct approximate_defining_relation_evidence {
  selected_vertex_id vertex;
  defining_relation_id relation;
  defining_relation_kind kind = defining_relation_kind::coordinate_equality;
  std::uint16_t formula_version = 1;
  exact_sign expected = exact_sign::zero;
  exact_scalar exact_residual;
  exact_scalar emitted_residual;
  exact_scalar normalized_squared_residual_change;
  exact_scalar allowed_squared_residual_change;
  bool exact = false;
  bool passed = false;
};

struct approximate_constraint_component {
  std::uint64_t id = 0;
  std::vector<std::uint64_t> variables;
  std::vector<std::uint64_t> variable_order;
  std::vector<std::uint64_t> obligations;
  std::vector<std::uint64_t> accepted_ranks;
  std::vector<std::uint64_t> rejected_prefix_witnesses;
  std::uint64_t visited_nodes = 0;
  std::uint64_t complete_assignments = 0;
  digest graph_digest;
  digest transcript_digest;
};

struct approximate_search_transcript {
  std::uint16_t candidate_generation_version = 1;
  std::uint32_t candidate_ulp_radius = 0;
  std::uint64_t candidate_cap = 0;
  std::uint64_t candidate_evaluation_limit = 0;
  std::uint64_t candidate_evaluations = 0;
  std::uint64_t search_node_limit = 0;
  std::uint64_t obligation_limit = 0;
  std::uint64_t triangle_pair_limit = 0;
  std::uint64_t predicate_check_limit = 0;
  std::uint64_t predicate_checks = 0;
  std::uint64_t verifier_work_limit = 0;
  std::uint64_t verifier_record_limit = 0;
  std::uint64_t verifier_byte_limit = 0;
  std::uint64_t generated_axis_candidates = 0;
  std::uint64_t generated_point_candidates = 0;
  std::uint64_t visited_nodes = 0;
  std::uint64_t complete_assignments = 0;
  std::vector<std::uint64_t> accepted_candidate_ranks;
  digest candidate_domain_digest;
  digest transcript_digest;
};

template <class T, class I> struct certified_approximate_certificate {
  std::uint16_t schema = certified_approximate_embedding_schema;
  coordinate_tag coordinate = exact_result_coordinate_type<T>();
  index_tag index = exact_result_index_type<I>();
  digest exact_result_digest;
  digest selected_boundary_digest;
  digest policy_digest;
  product_realization_policy policy;
  std::uint64_t exact_vertex_occurrences = 0;
  std::uint64_t exact_edges = 0;
  std::uint64_t exact_halfedges = 0;
  std::uint64_t exact_cycles = 0;
  std::uint64_t exact_patches = 0;
  std::vector<approximate_vertex_evidence<T>> vertices;
  std::vector<approximate_triangle_evidence> triangles;
  std::vector<approximate_occurrence_map> occurrence_maps;
  std::vector<approximate_patch_adjacency> patch_adjacency;
  std::vector<approximate_obligation_evidence> obligations;
  std::vector<approximate_relaxed_relation> relaxed_relations;
  std::vector<approximate_defining_relation_evidence> defining_relations;
  std::vector<approximate_constraint_component> components;
  approximate_search_transcript search;
  exact_scalar maximum_squared_vertex_displacement;
  std::array<exact_scalar, 3> maximum_axis_displacements;
  exact_scalar maximum_support_plane_deviation;
  exact_scalar maximum_normalized_squared_support_plane_deviation;
  std::vector<std::uint8_t> canonical_bytes;
  digest certificate_digest;
};

digest approximate_realization_policy_digest(
    const product_realization_policy &);

product_status_or<std::vector<approximate_triangle_evidence>>
reconstruct_authorized_approximate_triangulation(
    const exact_stratified_boundary &,
    const std::function<bool(std::uint64_t, std::uint64_t, std::uint64_t,
                             std::uint64_t)> & = {}) noexcept;

namespace detail {
template <class T, class I>
product_status_or<bool> verify_certified_approximate_embedding_with_budget(
    const exact_result_handle &, const product_realization_policy &,
    const boolean_success<T, I> &,
    const certified_approximate_certificate<T, I> &,
    approximate_verifier_budget &) noexcept;
}

template <class T, class I>
product_status_or<std::vector<std::uint8_t>>
encode_certified_approximate_embedding(
    const exact_result_handle &, const product_realization_policy &,
    const boolean_success<T, I> &,
    const certified_approximate_certificate<T, I> &);

template <class T, class I>
void canonicalize_certified_approximate_certificate(
    certified_approximate_certificate<T, I> &);

template <class T, class I>
product_status_or<bool> verify_serialized_certified_approximate_embedding(
    const std::vector<std::uint8_t> &, const product_realization_policy &,
    const approximate_decode_limits & = {}) noexcept;

template <class T, class I>
product_status_or<certified_mesh_payload<T, I>>
realize_certified_approximate_embedding(
    boolean_context<T, I> &, const exact_result_handle &,
    const backend_identity &, const product_realization_policy &);

#define YGOR_APPROXIMATE_EXTERN(T, I)                                          \
  extern template product_status_or<certified_mesh_payload<T, I>>             \
  realize_certified_approximate_embedding(                                    \
      boolean_context<T, I> &, const exact_result_handle &,                   \
      const backend_identity &, const product_realization_policy &);           \
  extern template product_status_or<bool>                                     \
  verify_certified_approximate_embedding(                                     \
      const exact_result_handle &, const product_realization_policy &,        \
      const boolean_success<T, I> &,                                          \
      const certified_approximate_certificate<T, I> &) noexcept;              \
  extern template product_status_or<std::vector<std::uint8_t>>                \
  encode_certified_approximate_embedding(                                     \
      const exact_result_handle &, const product_realization_policy &,        \
      const boolean_success<T, I> &,                                          \
      const certified_approximate_certificate<T, I> &);                       \
  extern template void canonicalize_certified_approximate_certificate(        \
      certified_approximate_certificate<T, I> &);                             \
  extern template product_status_or<bool>                                     \
  verify_serialized_certified_approximate_embedding<T, I>(                    \
      const std::vector<std::uint8_t> &, const product_realization_policy &,  \
      const approximate_decode_limits &) noexcept
YGOR_APPROXIMATE_EXTERN(float, std::uint32_t);
YGOR_APPROXIMATE_EXTERN(float, std::uint64_t);
YGOR_APPROXIMATE_EXTERN(double, std::uint32_t);
YGOR_APPROXIMATE_EXTERN(double, std::uint64_t);
#undef YGOR_APPROXIMATE_EXTERN

} // namespace mesh_boolean
} // namespace ygor

#endif
