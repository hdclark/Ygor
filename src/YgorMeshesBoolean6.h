//YgorMeshesBoolean6.h - Written by hal clark in 2026.

#pragma once
#ifndef YGOR_MESHES_BOOLEAN6_H_
#define YGOR_MESHES_BOOLEAN6_H_

#include <cstdint>

#include "YgorDefinitions.h"
#include "YgorMath.h"


enum class MeshBooleanOperation6 {
    Union,
    Intersection,
    Exclusion,
    Subtraction
};


template <class T, class I>
fv_surface_mesh<T, I>
BooleanMeshOp6(const fv_surface_mesh<T, I> &lhs,
               const fv_surface_mesh<T, I> &rhs,
               MeshBooleanOperation6 op);


template <class T, class I>
fv_surface_mesh<T, I>
BooleanUnion6(const fv_surface_mesh<T, I> &lhs,
              const fv_surface_mesh<T, I> &rhs);


template <class T, class I>
fv_surface_mesh<T, I>
BooleanIntersection6(const fv_surface_mesh<T, I> &lhs,
                     const fv_surface_mesh<T, I> &rhs);


template <class T, class I>
fv_surface_mesh<T, I>
BooleanExclusion6(const fv_surface_mesh<T, I> &lhs,
                  const fv_surface_mesh<T, I> &rhs);


template <class T, class I>
fv_surface_mesh<T, I>
BooleanSubtraction6(const fv_surface_mesh<T, I> &lhs,
                    const fv_surface_mesh<T, I> &rhs);


#endif // YGOR_MESHES_BOOLEAN6_H_
