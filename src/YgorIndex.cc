//YgorIndex.cc.

#include <algorithm>   //Needed for std::reverse.
#include <any>
#include <cmath>       //Needed for fabs, signbit, sqrt, etc...
#include <limits>      //Needed for std::numeric_limits::max().
#include <memory>
#include <utility>     //Needed for std::pair.
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorIndex.h"

//#ifndef YGOR_INDEX_DISABLE_ALL_SPECIALIZATIONS
//    #define YGOR_INDEX_DISABLE_ALL_SPECIALIZATIONS
//#endif

namespace {

template <class T>
int compare_x(const T &lhs, const T &rhs){
    return orient_sign(vec3<T>(rhs, static_cast<T>(0), static_cast<T>(0)),
                       vec3<T>(rhs, static_cast<T>(1), static_cast<T>(0)),
                       vec3<T>(rhs, static_cast<T>(0), static_cast<T>(1)),
                       vec3<T>(lhs, static_cast<T>(0), static_cast<T>(0)));
}

template <class T>
int compare_y(const T &lhs, const T &rhs){
    return orient_sign(vec3<T>(static_cast<T>(0), rhs, static_cast<T>(0)),
                       vec3<T>(static_cast<T>(0), rhs, static_cast<T>(1)),
                       vec3<T>(static_cast<T>(1), rhs, static_cast<T>(0)),
                       vec3<T>(static_cast<T>(0), lhs, static_cast<T>(0)));
}

template <class T>
int compare_z(const T &lhs, const T &rhs){
    return orient_sign(vec3<T>(static_cast<T>(0), static_cast<T>(0), rhs),
                       vec3<T>(static_cast<T>(1), static_cast<T>(0), rhs),
                       vec3<T>(static_cast<T>(0), static_cast<T>(1), rhs),
                       vec3<T>(static_cast<T>(0), static_cast<T>(0), lhs));
}

template <class T>
int compare_coord(const T &lhs, const T &rhs, int axis){
    if(axis == 0) return compare_x(lhs, rhs);
    if(axis == 1) return compare_y(lhs, rhs);
    return compare_z(lhs, rhs);
}

template <class T>
bool coord_less(const T &lhs, const T &rhs, int axis){
    return compare_coord(lhs, rhs, axis) > 0;
}

template <class T>
bool coord_less_equal(const T &lhs, const T &rhs, int axis){
    return compare_coord(lhs, rhs, axis) >= 0;
}

template <class T>
bool coord_greater_equal(const T &lhs, const T &rhs, int axis){
    return compare_coord(lhs, rhs, axis) <= 0;
}

template <class T>
bool coord_equal(const T &lhs, const T &rhs, int axis){
    return compare_coord(lhs, rhs, axis) == 0;
}

template <class T>
T axis_min(const T &lhs, const T &rhs, int axis){
    return coord_less(lhs, rhs, axis) ? lhs : rhs;
}

template <class T>
T axis_max(const T &lhs, const T &rhs, int axis){
    return coord_less(lhs, rhs, axis) ? rhs : lhs;
}

template <class T>
vec3<T> canonical_min(const vec3<T> &a, const vec3<T> &b){
    return vec3<T>(axis_min(a.x, b.x, 0),
                   axis_min(a.y, b.y, 1),
                   axis_min(a.z, b.z, 2));
}

template <class T>
vec3<T> canonical_max(const vec3<T> &a, const vec3<T> &b){
    return vec3<T>(axis_max(a.x, b.x, 0),
                   axis_max(a.y, b.y, 1),
                   axis_max(a.z, b.z, 2));
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------------
//------------------------- Shared spatial index types for rtree, cells_index, and octree -----------------------------------
//---------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------ index_entry --------------------------------------------------------

template <class T>
index_entry<T>::index_entry() : box(), aux_data() { }

template <class T>
index_entry<T>::index_entry(const index_bbox<T> &b) : box(b), aux_data() { }

template <class T>
index_entry<T>::index_entry(const index_bbox<T> &b, std::any data) : box(b), aux_data(std::move(data)) { }

template <class T>
index_entry<T>::index_entry(const vec3<T> &p) : box(p, p), aux_data() { }

template <class T>
index_entry<T>::index_entry(const vec3<T> &p, std::any data) : box(p, p), aux_data(std::move(data)) { }

template <class T>
bool index_entry<T>::operator==(const index_entry &other) const {
    return box == other.box;
}

//------------------------------------------------------ index_bbox ---------------------------------------------------------

template <class T>
index_bbox<T>::index_bbox() : min(vec3<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0))),
                              max(vec3<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0))) { }

template <class T>
index_bbox<T>::index_bbox(const vec3<T> &min_corner, const vec3<T> &max_corner) 
    : min(canonical_min(min_corner, max_corner)),
      max(canonical_max(min_corner, max_corner)) { }

template <class T>
T index_bbox<T>::volume() const {
    const auto dx = max.x - min.x;
    const auto dy = max.y - min.y;
    const auto dz = max.z - min.z;
    return dx * dy * dz;
}

template <class T>
T index_bbox<T>::surface_area() const {
    const auto dx = max.x - min.x;
    const auto dy = max.y - min.y;
    const auto dz = max.z - min.z;
    return static_cast<T>(2) * (dx * dy + dy * dz + dz * dx);
}

template <class T>
T index_bbox<T>::margin() const {
    const auto dx = max.x - min.x;
    const auto dy = max.y - min.y;
    const auto dz = max.z - min.z;
    return dx + dy + dz;
}

template <class T>
bool index_bbox<T>::has_extent() const {
    return !(coord_equal(min.x, max.x, 0) &&
             coord_equal(min.y, max.y, 1) &&
             coord_equal(min.z, max.z, 2));
}

template <class T>
bool index_bbox<T>::contains(const index_bbox &other) const {
    return coord_greater_equal(other.min.x, min.x, 0) &&
           coord_less_equal(other.max.x, max.x, 0) &&
           coord_greater_equal(other.min.y, min.y, 1) &&
           coord_less_equal(other.max.y, max.y, 1) &&
           coord_greater_equal(other.min.z, min.z, 2) &&
           coord_less_equal(other.max.z, max.z, 2);
}

template <class T>
bool index_bbox<T>::contains(const vec3<T> &point) const {
    return coord_greater_equal(point.x, min.x, 0) &&
           coord_less_equal(point.x, max.x, 0) &&
           coord_greater_equal(point.y, min.y, 1) &&
           coord_less_equal(point.y, max.y, 1) &&
           coord_greater_equal(point.z, min.z, 2) &&
           coord_less_equal(point.z, max.z, 2);
}

template <class T>
bool index_bbox<T>::intersects(const index_bbox &other) const {
    return !(coord_less(max.x, other.min.x, 0) ||
             coord_less(other.max.x, min.x, 0) ||
             coord_less(max.y, other.min.y, 1) ||
             coord_less(other.max.y, min.y, 1) ||
             coord_less(max.z, other.min.z, 2) ||
             coord_less(other.max.z, min.z, 2));
}

template <class T>
index_bbox<T> index_bbox<T>::union_with(const index_bbox &other) const {
    index_bbox result;
    result.min.x = axis_min(min.x, other.min.x, 0);
    result.min.y = axis_min(min.y, other.min.y, 1);
    result.min.z = axis_min(min.z, other.min.z, 2);
    result.max.x = axis_max(max.x, other.max.x, 0);
    result.max.y = axis_max(max.y, other.max.y, 1);
    result.max.z = axis_max(max.z, other.max.z, 2);
    return result;
}

template <class T>
index_bbox<T> index_bbox<T>::intersection_with(const index_bbox &other) const {
    index_bbox result;
    result.min.x = axis_max(min.x, other.min.x, 0);
    result.min.y = axis_max(min.y, other.min.y, 1);
    result.min.z = axis_max(min.z, other.min.z, 2);
    result.max.x = axis_min(max.x, other.max.x, 0);
    result.max.y = axis_min(max.y, other.max.y, 1);
    result.max.z = axis_min(max.z, other.max.z, 2);
    return result;
}

template <class T>
T index_bbox<T>::volume_increase(const index_bbox &other) const {
    const auto combined = union_with(other);
    return combined.volume() - volume();
}

template <class T>
void index_bbox<T>::expand(const index_bbox &other) {
    min.x = axis_min(min.x, other.min.x, 0);
    min.y = axis_min(min.y, other.min.y, 1);
    min.z = axis_min(min.z, other.min.z, 2);
    max.x = axis_max(max.x, other.max.x, 0);
    max.y = axis_max(max.y, other.max.y, 1);
    max.z = axis_max(max.z, other.max.z, 2);
}

template <class T>
void index_bbox<T>::expand(const vec3<T> &point) {
    min.x = axis_min(min.x, point.x, 0);
    min.y = axis_min(min.y, point.y, 1);
    min.z = axis_min(min.z, point.z, 2);
    max.x = axis_max(max.x, point.x, 0);
    max.y = axis_max(max.y, point.y, 1);
    max.z = axis_max(max.z, point.z, 2);
}

template <class T>
vec3<T> index_bbox<T>::center() const {
    return vec3<T>((min.x + max.x) / static_cast<T>(2),
                   (min.y + max.y) / static_cast<T>(2),
                   (min.z + max.z) / static_cast<T>(2));
}

template <class T>
vec3<T> index_bbox<T>::closest_point(const vec3<T> &point) const {
    vec3<T> result = point;

    if(coord_less(result.x, min.x, 0)) result.x = min.x;
    else if(coord_less(max.x, result.x, 0)) result.x = max.x;

    if(coord_less(result.y, min.y, 1)) result.y = min.y;
    else if(coord_less(max.y, result.y, 1)) result.y = max.y;

    if(coord_less(result.z, min.z, 2)) result.z = min.z;
    else if(coord_less(max.z, result.z, 2)) result.z = max.z;

    return result;
}

template <class T>
T index_bbox<T>::squared_distance_to(const vec3<T> &point) const {
    const auto closest = closest_point(point);
    return (closest - point).sq_length();
}

template <class T>
bool index_bbox<T>::isfinite() const {
    return min.isfinite() && max.isfinite();
}

template <class T>
bool index_bbox<T>::operator==(const index_bbox &other) const {
    return coord_equal(min.x, other.min.x, 0) &&
           coord_equal(min.y, other.min.y, 1) &&
           coord_equal(min.z, other.min.z, 2) &&
           coord_equal(max.x, other.max.x, 0) &&
           coord_equal(max.y, other.max.y, 1) &&
           coord_equal(max.z, other.max.z, 2);
}

//------------------------------------------------------ index_node_base ----------------------------------------------------

template <class T>
index_node_base<T>::index_node_base() : parent(nullptr) { }

//------------------------------------------------------ index_internal_node ------------------------------------------------

template <class T>
index_internal_node<T>::index_internal_node() : index_node_base<T>() { }

template <class T>
bool index_internal_node<T>::is_leaf() const {
    return false;
}

//------------------------------------------------------ index_leaf_node ----------------------------------------------------

template <class T>
index_leaf_node<T>::index_leaf_node() : index_node_base<T>() { }

template <class T>
bool index_leaf_node<T>::is_leaf() const {
    return true;
}

#ifndef YGOR_INDEX_DISABLE_ALL_SPECIALIZATIONS
    template struct index_entry<float>;
    template struct index_entry<double>;

    template struct index_bbox<float>;
    template struct index_bbox<double>;

    template struct index_node_base<float>;
    template struct index_node_base<double>;

    template struct index_internal_node<float>;
    template struct index_internal_node<double>;

    template struct index_leaf_node<float>;
    template struct index_leaf_node<double>;
#endif
