//YgorMeshesIntersections.h - Written by hal clark in 2026.

#pragma once
#ifndef YGOR_MESHES_INTERSECTIONS_H_
#define YGOR_MESHES_INTERSECTIONS_H_

#include <cstdint>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"


enum class fv_surface_mesh_intersection_primitive_type : uint8_t {
    Vertex,
    Edge,
    Face
};

enum class fv_surface_mesh_intersection_geometry_type : uint8_t {
    Point,
    Segment,
    Polygon
};

template <class I>
struct fv_surface_mesh_intersection_primitive {
    fv_surface_mesh_intersection_primitive_type type = fv_surface_mesh_intersection_primitive_type::Vertex;
    std::vector<I> indices;
};

template <class T>
struct fv_surface_mesh_intersection_geometry {
    fv_surface_mesh_intersection_geometry_type type = fv_surface_mesh_intersection_geometry_type::Point;
    std::vector<vec3<T>> vertices;
};

template <class T, class I>
struct fv_surface_mesh_intersection {
    I lhs_face_index = static_cast<I>(0);
    I rhs_face_index = static_cast<I>(0);
    std::vector<fv_surface_mesh_intersection_primitive<I>> lhs_primitives;
    std::vector<fv_surface_mesh_intersection_primitive<I>> rhs_primitives;
    fv_surface_mesh_intersection_geometry<T> lhs_geometry;
    fv_surface_mesh_intersection_geometry<T> rhs_geometry;
};


// Extract an ordered list of pairwise surface intersections between two meshes.
//
// Each record links a face from lhs with a face from rhs and reports the precise
// coincident point, segment, or polygon together with the participating
// vertices/edges/faces in both meshes.
//
// Throws std::invalid_argument if either mesh contains non-finite vertices,
// out-of-range face indices, or face indices that cannot be represented by I.
//
// The implementation uses adaptive predicates for orientation tests and
// automatically chooses a mesh-scale snapping tolerance when snap_eps <= 0.
template <class T, class I>
std::vector<fv_surface_mesh_intersection<T, I>>
FindIntersections(const fv_surface_mesh<T, I> &lhs,
                  const fv_surface_mesh<T, I> &rhs,
                  T snap_eps = static_cast<T>(0));

#endif // YGOR_MESHES_INTERSECTIONS_H_
