#pragma once
#ifndef YGOR_MESHES_BOOLEAN_PREPARATION_INTERNAL_H_
#define YGOR_MESHES_BOOLEAN_PREPARATION_INTERNAL_H_

#include "YgorMeshesBooleanPreparation.h"

namespace ygor {
namespace mesh_boolean {
namespace preparation_detail {

constexpr std::array<char, 8> canonical_mesh_tag{
    {'Y', 'G', 'B', 'P', 'I', 'N', '0', '2'}};

template <class T, class I>
void encode_canonical_mesh(canonical_encoder &e,
                           const fv_surface_mesh<T, I> &mesh) {
  e.byte(sizeof(T));
  e.byte(sizeof(I));
  e.u64(mesh.vertices.size());
  for (const auto &v : mesh.vertices) {
    e.floating(v.x);
    e.floating(v.y);
    e.floating(v.z);
  }
  e.u64(mesh.vertex_normals.size());
  for (const auto &v : mesh.vertex_normals) {
    e.floating(v.x);
    e.floating(v.y);
    e.floating(v.z);
  }
  e.u64(mesh.vertex_colours.size());
  for (auto colour : mesh.vertex_colours) e.u32(colour);
  e.u64(mesh.faces.size());
  for (const auto &face : mesh.faces) {
    e.u64(face.size());
    for (I index : face) e.u64(static_cast<std::uint64_t>(index));
  }
  e.u64(mesh.involved_faces.size());
  for (const auto &faces : mesh.involved_faces) {
    e.u64(faces.size());
    for (I index : faces) e.u64(static_cast<std::uint64_t>(index));
  }
  e.u64(mesh.metadata.size());
  for (const auto &entry : mesh.metadata) {
    e.string(entry.first);
    e.string(entry.second);
  }
}

template <class T, class I>
std::vector<std::uint8_t>
canonical_mesh_bytes(const fv_surface_mesh<T, I> &mesh) {
  canonical_encoder e;
  encode_canonical_mesh(e, mesh);
  return e.bytes();
}

template <class T, class I>
digest canonical_mesh_digest(const fv_surface_mesh<T, I> &mesh) {
  return domain_digest(canonical_mesh_tag, canonical_mesh_bytes(mesh));
}

template <class T, class I>
bool valid_attribute_cardinality(const fv_surface_mesh<T, I> &mesh) {
  return (mesh.vertex_normals.empty() ||
          mesh.vertex_normals.size() == mesh.vertices.size()) &&
         (mesh.vertex_colours.empty() ||
          mesh.vertex_colours.size() == mesh.vertices.size()) &&
         (mesh.involved_faces.empty() ||
          mesh.involved_faces.size() == mesh.vertices.size());
}

template <class T, class I>
bool valid_involved_face_indices(const fv_surface_mesh<T, I> &mesh) {
  for (const auto &indices : mesh.involved_faces)
    for (I index : indices)
      if (static_cast<std::uint64_t>(index) >= mesh.faces.size()) return false;
  return true;
}

} // namespace preparation_detail
} // namespace mesh_boolean
} // namespace ygor
#endif
