#pragma once
#ifndef YGOR_MESHES_BOOLEAN_OUTPUT_H_
#define YGOR_MESHES_BOOLEAN_OUTPUT_H_

#include "YgorMeshesBooleanRealization.h"

namespace ygor {
namespace mesh_boolean {

constexpr std::uint64_t assembled_output_type_tag = 0x5947424f55543132ULL;
constexpr std::uint16_t assembled_output_schema = 2;

struct result_topology_authorization {
  context_owner_token owner;
  digest selected_digest, topology_certificate_digest, policy_digest;
  selected_boundary_topology topology = selected_boundary_topology::empty;
};

template <class I> struct output_vertex_record {
  output_vertex_id id;
  realization_vertex_id realization;
  I public_index = 0;
  std::vector<std::uint8_t> semantic_token;
};

template <class I> struct output_face_record {
  output_face_id id;
  realization_triangle_id realization;
  output_component_id component;
  std::uint8_t cyclic_rotation = 0;
  std::array<I, 3> public_vertices{{0, 0, 0}};
  std::vector<std::uint8_t> semantic_key;
};

struct output_component_record {
  output_component_id id;
  std::uint64_t first_face = 0, face_count = 0;
  std::vector<std::uint8_t> semantic_key;
};

struct output_assembly_certificate {
  output_assembly_certificate_id id;
  std::uint16_t schema = assembled_output_schema, ordering_version = 1,
                encoding_version = 1;
  std::uint64_t vertices = 0, faces = 0, components = 0, face_indices = 0,
                involved_face_entries = 0, edge_uses = 0;
  digest selected_digest, realized_digest, policy_digest, topology_digest,
      mapping_digest, semantic_digest;
};

template <class T, class I> struct assembled_output : boolean_success<T, I> {
  context_owner_token owner;
  operation selected_operation = operation::regularized_union;
  digest setup_digest, input_a_digest, input_b_digest, selected_digest,
      realized_digest, policy_digest, artifact_digest;
  output_policy policy;
  result_topology_authorization topology_authorization;
  std::shared_ptr<const published_artifact<realized_boundary<T, I>>> realized;
  std::vector<output_vertex_record<I>> vertices;
  std::vector<output_face_record<I>> faces;
  std::vector<output_component_record> components;
  output_assembly_certificate certificate;
  std::vector<std::uint8_t> canonical_bytes, artifact_bytes;
};

status_or<bool> register_boolean_output_verifier(verifier_registry &,
                                                  coordinate_tag, index_tag);

template <class T, class I>
status_or<result_topology_authorization>
authorize_result_topology(boolean_context<T, I> &);

template <class T, class I>
status_or<std::shared_ptr<const published_artifact<assembled_output<T, I>>>>
assemble_boolean_output_artifact(boolean_context<T, I> &);

template <class T, class I>
boolean_result<T, I> assemble_boolean_output(boolean_context<T, I> &);

#define YGOR_OUTPUT_EXTERN(T, I)                                               \
  extern template status_or<result_topology_authorization>                    \
  authorize_result_topology(boolean_context<T, I> &);                         \
  extern template status_or<                                                   \
      std::shared_ptr<const published_artifact<assembled_output<T, I>>>>       \
  assemble_boolean_output_artifact(boolean_context<T, I> &);                   \
  extern template boolean_result<T, I> assemble_boolean_output(                \
      boolean_context<T, I> &)
YGOR_OUTPUT_EXTERN(float, std::uint32_t);
YGOR_OUTPUT_EXTERN(float, std::uint64_t);
YGOR_OUTPUT_EXTERN(double, std::uint32_t);
YGOR_OUTPUT_EXTERN(double, std::uint64_t);
#undef YGOR_OUTPUT_EXTERN

} // namespace mesh_boolean
} // namespace ygor
#endif
