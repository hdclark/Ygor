#pragma once

#include "../YgorMeshesBooleanBounded.h"

#include <cstddef>
#include <cstring>

namespace ygor::mesh_boolean::bounded {
template<class T, class I>
class public_mesh_read_view {
  public:
    explicit public_mesh_read_view(const fv_surface_mesh<T,I> &mesh) noexcept : mesh_(&mesh) {}
    std::size_t vertex_count() const noexcept { return mesh_->vertices.size(); }
    std::size_t face_count() const noexcept { return mesh_->faces.size(); }
    const vec3<T> *vertex(std::size_t i) const noexcept { return i < vertex_count() ? &mesh_->vertices[i] : nullptr; }
    std::size_t ring_size(std::size_t i) const noexcept { return i < face_count() ? mesh_->faces[i].size() : 0; }
    const I *index(std::size_t face, std::size_t item) const noexcept {
        return face < face_count() && item < mesh_->faces[face].size() ? &mesh_->faces[face][item] : nullptr;
    }
  private:
    const fv_surface_mesh<T,I> *mesh_;
};
}
