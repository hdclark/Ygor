#include "YgorMath.h"

#include <cstdint>

// The focused bounded-Boolean integration target needs only the public mesh
// carrier's trivial construction/copy operations.  These definitions are kept
// byte-for-byte equivalent to the corresponding YgorMath.cc templates so the
// normative Component 07 test does not need to link unrelated legacy geometry,
// Boolean, imaging, statistics, and I/O providers.
template <> vec3<double>::vec3() : x(0.0), y(0.0), z(0.0) {}
template <>
vec3<double>::vec3(double a, double b, double c) : x(a), y(b), z(c) {}
template <>
vec3<double>::vec3(const vec3<double> &in) : x(in.x), y(in.y), z(in.z) {}
template <>
vec3<double> &vec3<double>::operator=(const vec3<double> &rhs) {
  if (this != &rhs) {
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;
  }
  return *this;
}

template <>
fv_surface_mesh<double, std::uint32_t>::fv_surface_mesh() {}
template <>
fv_surface_mesh<double, std::uint32_t>::fv_surface_mesh(
    const fv_surface_mesh<double, std::uint32_t> &in)
    : vertices(in.vertices), vertex_normals(in.vertex_normals),
      vertex_colours(in.vertex_colours), faces(in.faces),
      involved_faces(in.involved_faces), metadata(in.metadata) {}
template <>
fv_surface_mesh<double, std::uint32_t> &
fv_surface_mesh<double, std::uint32_t>::operator=(
    const fv_surface_mesh<double, std::uint32_t> &rhs) {
  if (this != &rhs) {
    vertices = rhs.vertices;
    vertex_normals = rhs.vertex_normals;
    vertex_colours = rhs.vertex_colours;
    faces = rhs.faces;
    involved_faces = rhs.involved_faces;
    metadata = rhs.metadata;
  }
  return *this;
}
