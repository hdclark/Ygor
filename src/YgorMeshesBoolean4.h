//YgorMeshesBoolean4.h - Written by hal clark in 2026.
//
// Surface mesh Boolean engine backed by the BSP tree volume class.
// Two closed triangular surface meshes are independently converted into
// bsp_tree_volume objects, the requested Boolean operation is performed
// on the BSP trees, and the result is converted back to a surface mesh.

#pragma once
#ifndef YGOR_MESHES_BOOLEAN4_H_
#define YGOR_MESHES_BOOLEAN4_H_

#include <cstdint>

#include "YgorDefinitions.h"
#include "YgorMath.h"


enum class MeshBooleanOperation4 {
    Union,
    Intersection,
    Exclusion,
    Subtraction
};


template <class T, class I>
fv_surface_mesh<T, I>
BooleanMeshOp4(const fv_surface_mesh<T, I> &lhs,
               const fv_surface_mesh<T, I> &rhs,
               MeshBooleanOperation4 op);


template <class T, class I>
fv_surface_mesh<T, I>
BooleanUnion4(const fv_surface_mesh<T, I> &lhs,
              const fv_surface_mesh<T, I> &rhs);


template <class T, class I>
fv_surface_mesh<T, I>
BooleanIntersection4(const fv_surface_mesh<T, I> &lhs,
                     const fv_surface_mesh<T, I> &rhs);


template <class T, class I>
fv_surface_mesh<T, I>
BooleanExclusion4(const fv_surface_mesh<T, I> &lhs,
                  const fv_surface_mesh<T, I> &rhs);


template <class T, class I>
fv_surface_mesh<T, I>
BooleanSubtraction4(const fv_surface_mesh<T, I> &lhs,
                    const fv_surface_mesh<T, I> &rhs);


#endif // YGOR_MESHES_BOOLEAN4_H_
