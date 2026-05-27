//YgorMeshesBoolean.h - Written by hal clark in 2026.

#pragma once
#ifndef YGOR_MESHES_BOOLEAN_H_
#define YGOR_MESHES_BOOLEAN_H_

#include <cstdint>

#include "YgorDefinitions.h"
#include "YgorMath.h"


enum class MeshBooleanOperation {
    Union,
    Intersection,
    Exclusion,
    Subtraction
};


// Perform a volumetric Boolean operation on two closed triangular surface meshes.
//
// The inputs may each represent a group of closed components bundled into a single
// fv_surface_mesh. The meshes are validated, triangulated, and consistently oriented
// before evaluation.
//
// The implementation evaluates the Boolean classification on a Cartesian volumetric
// grid spanning the combined input domain, using exact-predicate ray tests for the
// inside/outside queries and an R-tree over triangle bounding boxes for broad-phase
// acceleration. The output is a closed triangular surface mesh aligned to the final
// volumetric partition.
//
// max_depth controls the grid resolution (2^max_depth cells along each domain axis).
// boundary_scale expands the shared bounding cube by the specified fraction of the
// dominant domain extent.
template <class T, class I>
fv_surface_mesh<T, I>
BooleanMeshOp(const fv_surface_mesh<T, I> &lhs,
              const fv_surface_mesh<T, I> &rhs,
              MeshBooleanOperation op,
              int64_t max_depth = 6,
              T boundary_scale = static_cast<T>(0.05));


template <class T, class I>
fv_surface_mesh<T, I>
BooleanUnion(const fv_surface_mesh<T, I> &lhs,
             const fv_surface_mesh<T, I> &rhs,
             int64_t max_depth = 6,
             T boundary_scale = static_cast<T>(0.05));

template <class T, class I>
fv_surface_mesh<T, I>
BooleanIntersection(const fv_surface_mesh<T, I> &lhs,
                    const fv_surface_mesh<T, I> &rhs,
                    int64_t max_depth = 6,
                    T boundary_scale = static_cast<T>(0.05));

template <class T, class I>
fv_surface_mesh<T, I>
BooleanExclusion(const fv_surface_mesh<T, I> &lhs,
                 const fv_surface_mesh<T, I> &rhs,
                 int64_t max_depth = 6,
                 T boundary_scale = static_cast<T>(0.05));

template <class T, class I>
fv_surface_mesh<T, I>
BooleanSubtraction(const fv_surface_mesh<T, I> &lhs,
                   const fv_surface_mesh<T, I> &rhs,
                   int64_t max_depth = 6,
                   T boundary_scale = static_cast<T>(0.05));


#endif // YGOR_MESHES_BOOLEAN_H_
