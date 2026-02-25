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

//---------------------------------------------------------------------------------------------------------------------------
//------------------------- Shared spatial index types for rtree, cells_index, and octree -----------------------------------
//---------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------ index_entry --------------------------------------------------------

template <class T>
index_entry<T>::index_entry() : point(), aux_data() { }

template <class T>
index_entry<T>::index_entry(const vec3<T> &p) : point(p), aux_data() { }

template <class T>
index_entry<T>::index_entry(const vec3<T> &p, std::any data) : point(p), aux_data(std::move(data)) { }

template <class T>
bool index_entry<T>::operator==(const index_entry &other) const {
    return point == other.point;
}

//------------------------------------------------------ index_bbox ---------------------------------------------------------

template <class T>
index_bbox<T>::index_bbox() : min(vec3<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0))),
                              max(vec3<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0))) { }

template <class T>
index_bbox<T>::index_bbox(const vec3<T> &min_corner, const vec3<T> &max_corner) 
    : min(min_corner), max(max_corner) { }

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
bool index_bbox<T>::contains(const vec3<T> &point) const {
    return (point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z);
}

template <class T>
bool index_bbox<T>::intersects(const index_bbox &other) const {
    return !(max.x < other.min.x || min.x > other.max.x ||
             max.y < other.min.y || min.y > other.max.y ||
             max.z < other.min.z || min.z > other.max.z);
}

template <class T>
index_bbox<T> index_bbox<T>::union_with(const index_bbox &other) const {
    index_bbox result;
    result.min.x = std::min(min.x, other.min.x);
    result.min.y = std::min(min.y, other.min.y);
    result.min.z = std::min(min.z, other.min.z);
    result.max.x = std::max(max.x, other.max.x);
    result.max.y = std::max(max.y, other.max.y);
    result.max.z = std::max(max.z, other.max.z);
    return result;
}

template <class T>
index_bbox<T> index_bbox<T>::intersection_with(const index_bbox &other) const {
    index_bbox result;
    result.min.x = std::max(min.x, other.min.x);
    result.min.y = std::max(min.y, other.min.y);
    result.min.z = std::max(min.z, other.min.z);
    result.max.x = std::min(max.x, other.max.x);
    result.max.y = std::min(max.y, other.max.y);
    result.max.z = std::min(max.z, other.max.z);
    return result;
}

template <class T>
T index_bbox<T>::volume_increase(const index_bbox &other) const {
    const auto combined = union_with(other);
    return combined.volume() - volume();
}

template <class T>
void index_bbox<T>::expand(const index_bbox &other) {
    min.x = std::min(min.x, other.min.x);
    min.y = std::min(min.y, other.min.y);
    min.z = std::min(min.z, other.min.z);
    max.x = std::max(max.x, other.max.x);
    max.y = std::max(max.y, other.max.y);
    max.z = std::max(max.z, other.max.z);
}

template <class T>
void index_bbox<T>::expand(const vec3<T> &point) {
    min.x = std::min(min.x, point.x);
    min.y = std::min(min.y, point.y);
    min.z = std::min(min.z, point.z);
    max.x = std::max(max.x, point.x);
    max.y = std::max(max.y, point.y);
    max.z = std::max(max.z, point.z);
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

