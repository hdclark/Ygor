//YgorIndex.h

#ifndef YGOR_INDEX_H_
#define YGOR_INDEX_H_

#include <stddef.h>
#include <any>
#include <array>
#include <cmath>
#include <complex>
#include <optional>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"


//---------------------------------------------------------------------------------------------------------------------------
//------------------------- Shared spatial index types for rtree, cells_index, and octree -----------------------------------
//---------------------------------------------------------------------------------------------------------------------------

// Forward declarations.
template <class T> struct index_node_base;
template <class T> struct index_internal_node;
template <class T> struct index_leaf_node;
template <class T> struct index_bbox;

// An entry in a spatial index consisting of a spatial point and optional auxiliary data.
template <class T> struct index_entry {
    vec3<T> point;
    std::any aux_data;

    index_entry();
    index_entry(const vec3<T> &p);
    index_entry(const vec3<T> &p, std::any data);

    bool operator==(const index_entry &other) const;
};

// A bounding box in 3D space defined by min and max corners.
template <class T> struct index_bbox {
    vec3<T> min;
    vec3<T> max;

    index_bbox();
    index_bbox(const vec3<T> &min_corner, const vec3<T> &max_corner);

    // Compute the volume of the bounding box.
    T volume() const;

    // Compute the surface area of the bounding box.
    T surface_area() const;

    // Compute the margin (sum of edge lengths) of the bounding box.
    T margin() const;

    // Check if this bbox contains a point.
    bool contains(const vec3<T> &point) const;

    // Check if this bbox intersects another bbox.
    bool intersects(const index_bbox &other) const;

    // Compute the union of two bounding boxes.
    index_bbox union_with(const index_bbox &other) const;

    // Compute the intersection of two bounding boxes.
    index_bbox intersection_with(const index_bbox &other) const;

    // Compute the increase in volume if this bbox is expanded to include another.
    T volume_increase(const index_bbox &other) const;

    // Expand this bbox to include another bbox.
    void expand(const index_bbox &other);

    // Expand this bbox to include a point.
    void expand(const vec3<T> &point);
};

// Base class for all nodes in a spatial index.
template <class T> struct index_node_base {
    index_bbox<T> bounds;
    index_internal_node<T>* parent;  // Raw pointer for parent (ownership is top-down)

    index_node_base();
    virtual ~index_node_base() = default;
    virtual bool is_leaf() const = 0;
};

// Internal node containing references to child nodes.
template <class T> struct index_internal_node : public index_node_base<T> {
    std::vector<std::unique_ptr<index_node_base<T>>> children;

    index_internal_node();
    bool is_leaf() const override;
};

// Leaf node containing actual data entries.
template <class T> struct index_leaf_node : public index_node_base<T> {
    std::vector<index_entry<T>> entries;

    index_leaf_node();
    bool is_leaf() const override;
};

#endif // YGOR_INDEX_H_
