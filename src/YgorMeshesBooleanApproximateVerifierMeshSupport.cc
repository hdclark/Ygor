#include "YgorMath.h"

template <class T> vec3<T>::vec3() : x(T(0)), y(T(0)), z(T(0)) {}
template <class T> vec3<T>::vec3(T a, T b, T c) : x(a), y(b), z(c) {}
template <class T> vec3<T>::vec3(const vec3<T> &other)
    : x(other.x), y(other.y), z(other.z) {}

template vec3<float>::vec3();
template vec3<float>::vec3(float, float, float);
template vec3<float>::vec3(const vec3<float> &);
template vec3<double>::vec3();
template vec3<double>::vec3(double, double, double);
template vec3<double>::vec3(const vec3<double> &);

template <class T, class I> fv_surface_mesh<T, I>::fv_surface_mesh() {}
template fv_surface_mesh<float, std::uint32_t>::fv_surface_mesh();
template fv_surface_mesh<float, std::uint64_t>::fv_surface_mesh();
template fv_surface_mesh<double, std::uint32_t>::fv_surface_mesh();
template fv_surface_mesh<double, std::uint64_t>::fv_surface_mesh();
