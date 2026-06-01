//YgorMeshesBoolean2.h - Written by hal clark 2026.

#pragma once
#ifndef YGOR_MESHES_BOOLEAN2_H_
#define YGOR_MESHES_BOOLEAN2_H_

#include <cstdint>

#include "YgorDefinitions.h"
#include "YgorMath.h"


enum class MeshBooleanOperation2 {
    Union,
    Intersection,
    Exclusion,
    Subtraction
};


// Perform an exact-style surface Boolean by intersecting, splitting, classifying,
// and reassembling the boundary triangles of two closed triangular surface meshes.
//
// The inputs may each represent multiple disconnected closed components bundled into
// a single fv_surface_mesh. The implementation uses adaptive predicates for robust
// orientation tests, R-tree broad-phase acceleration, constrained triangulation for
// face retriangulation, and ray-based inside/outside classification.
//
// snap_eps controls vertex snapping for newly generated intersection points. Passing
// zero selects an automatic mesh-scale tolerance.
template <class T, class I>
fv_surface_mesh<T, I>
BooleanMeshOp2(const fv_surface_mesh<T, I> &lhs,
               const fv_surface_mesh<T, I> &rhs,
               MeshBooleanOperation2 op,
               T snap_eps = static_cast<T>(0));


template <class T, class I>
fv_surface_mesh<T, I>
BooleanUnion2(const fv_surface_mesh<T, I> &lhs,
              const fv_surface_mesh<T, I> &rhs,
              T snap_eps = static_cast<T>(0));


template <class T, class I>
fv_surface_mesh<T, I>
BooleanIntersection2(const fv_surface_mesh<T, I> &lhs,
                     const fv_surface_mesh<T, I> &rhs,
                     T snap_eps = static_cast<T>(0));


template <class T, class I>
fv_surface_mesh<T, I>
BooleanExclusion2(const fv_surface_mesh<T, I> &lhs,
                  const fv_surface_mesh<T, I> &rhs,
                  T snap_eps = static_cast<T>(0));


template <class T, class I>
fv_surface_mesh<T, I>
BooleanSubtraction2(const fv_surface_mesh<T, I> &lhs,
                    const fv_surface_mesh<T, I> &rhs,
                    T snap_eps = static_cast<T>(0));


#endif // YGOR_MESHES_BOOLEAN2_H_
