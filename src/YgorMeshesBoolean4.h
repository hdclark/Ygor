//YgorMeshesBoolean4.h - Written by hal clark in 2026.
//
// Surface mesh Boolean engine backed by an explicit split-facet arrangement.
// Inputs are normalized and validated as finite, closed, consistently oriented
// triangular solids; arrangement facets are classified against the opposite
// solid and selected by the requested Boolean operation. Non-empty outputs are
// verified to be finite, triangular, indexed correctly, non-degenerate, closed,
// and consistently oriented before returning.

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
