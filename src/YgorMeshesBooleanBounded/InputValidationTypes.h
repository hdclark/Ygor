#pragma once

#include "../YgorMeshesBooleanBounded.h"
#include "ContractVersions.h"
#include "Identity.h"
#include "Resources.h"

#include <cstdint>

namespace ygor::mesh_boolean::bounded {

enum class input_certificate_disposition : std::uint8_t {
  nominal_embedded = 1,
  topology_only_nonpublishable = 3,
};
enum class occupied_side : std::int8_t { negative = -1, positive = 1 };
enum class shell_orientation : std::int8_t { inward = -1, outward = 1 };
enum class ring_position_action : std::uint8_t {
  retained = 1,
  consecutive_duplicate = 2,
  duplicate_closure = 3
};
enum class validation_relation : std::uint8_t {
  definitely_disjoint = 1,
  authorized_shared_vertex = 2,
  authorized_shared_edge = 3,
  point_contact = 4,
  edge_contact = 5,
  face_contact = 6,
  transverse_intersection = 7,
  coplanar_positive_area_overlap = 8,
  whole_patch_coincidence = 9,
  uncertain = 10,
};
enum class shell_pair_relation : std::uint8_t {
  definitely_disjoint = 1,
  a_inside_b = 2,
  b_inside_a = 3,
  authorized_point_contact = 4,
  authorized_edge_contact = 5,
  authorized_face_contact = 6,
};

enum class input_validation_subcode : std::uint32_t {
  wrong_capability = 20001,
  non_finite_coordinate = 20006,
  out_of_range_index = 20007,
  undersized_ring = 20101,
  repeated_ring_vertex = 20102,
  self_edge = 20103,
  open_boundary = 20105,
  non_manifold_edge = 20106,
  same_direction_pair = 20107,
  vertex_link_multiple_cycles = 20110,
  vertex_link_open_chain = 20109,
  vertex_link_degree_mismatch = 20112,
  invalid_automorphism_class = 20203,
  support_plane_unavailable = 20302,
  planarity_exceeded = 20303,
  projected_crossing = 20310,
  facet_orientation_zero = 20314,
  shell_orientation_parity = 20410,
  shell_transverse_intersection = 20402,
  shell_coplanar_overlap = 20403,
  shell_positive_volume_overlap = 20404,
  shell_contact_ambiguous = 20406,
  shell_parent_ambiguous = 20407,
  forbidden_interaction = 20501,
  adjacency_exceeded = 20502,
  collapsed_geometry = 20505,
  uncertainty_incompatible = 20506,
  resource_preflight = 20005,
  cancelled = 20008,
  verifier_rejection = 20601,
};

struct input_validation_capabilities final {
  std::uint16_t version = contract_versions::input_validation_provider;
  context_owner_token owner{};
  const bounded_boolean_cancellation_token *cancellation = nullptr;
  resource_manager *resources = nullptr;
  std::uint32_t reserved = 0;
};

inline bounded_boolean_error
input_validation_error(operand_id operand, input_validation_subcode subcode,
                       bounded_boolean_error_category category,
                       const char *summary, std::uint32_t checkpoint = 0) {
  bounded_boolean_error error;
  error.category = category;
  error.subcode = static_cast<std::uint32_t>(subcode);
  error.component = 2;
  error.stage = static_cast<std::uint16_t>(operand == operand_id::a
                                               ? stage_id::input_validation_a
                                               : stage_id::input_validation_b);
  error.checkpoint = checkpoint;
  error.summary = summary;
  return error;
}

} // namespace ygor::mesh_boolean::bounded
