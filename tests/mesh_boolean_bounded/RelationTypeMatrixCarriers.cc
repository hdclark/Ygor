#include "YgorMath.h"

#include <array>
#include <cstdint>

// Focused Component 07 type/index qualification carriers.  The normative
// relation targets link only the strict bounded provider and therefore must
// supply the trivial construction/copy/assignment operations of the public
// mesh carrier themselves.  These definitions are kept byte-for-byte
// equivalent to the corresponding YgorMath.cc templates for every supported
// T in {float, double} and I in {uint32_t, uint64_t} so the test does not link
// unrelated legacy geometry, Boolean, imaging, statistics, and I/O providers.
namespace {

template <class T> struct carrier_for {
  using vec = vec3<T>;
};

} // namespace

// --- vec3 explicit specializations ----------------------------------------
template <> vec3<float>::vec3() : x(0.0f), y(0.0f), z(0.0f) {}
template <>
vec3<float>::vec3(float a, float b, float c) : x(a), y(b), z(c) {}
template <>
vec3<float>::vec3(const vec3<float> &in) : x(in.x), y(in.y), z(in.z) {}
template <>
vec3<float> &vec3<float>::operator=(const vec3<float> &rhs) {
  if (this != &rhs) {
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;
  }
  return *this;
}

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

// --- fv_surface_mesh explicit specializations -----------------------------
#define YGOR_DEFINE_MESH_CARRIER(T, I)                                        \
  template <> fv_surface_mesh<T, I>::fv_surface_mesh() {}                     \
  template <> fv_surface_mesh<T, I>::fv_surface_mesh(                         \
      const fv_surface_mesh<T, I> &in)                                        \
      : vertices(in.vertices), vertex_normals(in.vertex_normals),             \
        vertex_colours(in.vertex_colours), faces(in.faces),                   \
        involved_faces(in.involved_faces), metadata(in.metadata) {}           \
  template <> fv_surface_mesh<T, I> &                                         \
  fv_surface_mesh<T, I>::operator=(const fv_surface_mesh<T, I> &rhs) {        \
    if (this != &rhs) {                                                       \
      vertices = rhs.vertices;                                                \
      vertex_normals = rhs.vertex_normals;                                    \
      vertex_colours = rhs.vertex_colours;                                    \
      faces = rhs.faces;                                                      \
      involved_faces = rhs.involved_faces;                                    \
      metadata = rhs.metadata;                                                \
    }                                                                         \
    return *this;                                                             \
  }

YGOR_DEFINE_MESH_CARRIER(float, std::uint32_t)
YGOR_DEFINE_MESH_CARRIER(float, std::uint64_t)
YGOR_DEFINE_MESH_CARRIER(double, std::uint32_t)
YGOR_DEFINE_MESH_CARRIER(double, std::uint64_t)

#undef YGOR_DEFINE_MESH_CARRIER
