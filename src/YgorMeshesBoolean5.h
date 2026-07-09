//YgorMeshesBoolean5.h - Written by hal clark in 2026.
//
// Surface mesh Boolean engine backed by an explicit split-facet arrangement.
// Inputs are normalized and validated as finite, closed, consistently oriented
// triangular solids; arrangement facets are classified against the opposite
// solid and selected by the requested Boolean operation. Non-empty outputs are
// verified to be finite, triangular, indexed correctly, non-degenerate, closed,
// and consistently oriented before returning.

#pragma once
#ifndef YGOR_MESHES_BOOLEAN5_H_
#define YGOR_MESHES_BOOLEAN5_H_

#include <cstdint>

#include "YgorDefinitions.h"
#include "YgorMath.h"


enum class MeshBooleanOperation5 {
    Union,
    Intersection,
    Exclusion,
    Subtraction
};


template <class T, class I>
fv_surface_mesh<T, I>
BooleanMeshOp5(const fv_surface_mesh<T, I> &lhs,
               const fv_surface_mesh<T, I> &rhs,
               MeshBooleanOperation5 op);


template <class T, class I>
fv_surface_mesh<T, I>
BooleanUnion5(const fv_surface_mesh<T, I> &lhs,
              const fv_surface_mesh<T, I> &rhs);


template <class T, class I>
fv_surface_mesh<T, I>
BooleanIntersection5(const fv_surface_mesh<T, I> &lhs,
                     const fv_surface_mesh<T, I> &rhs);


template <class T, class I>
fv_surface_mesh<T, I>
BooleanExclusion5(const fv_surface_mesh<T, I> &lhs,
                  const fv_surface_mesh<T, I> &rhs);


template <class T, class I>
fv_surface_mesh<T, I>
BooleanSubtraction5(const fv_surface_mesh<T, I> &lhs,
                    const fv_surface_mesh<T, I> &rhs);


#endif // YGOR_MESHES_BOOLEAN5_H_
