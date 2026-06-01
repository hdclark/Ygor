//YgorMeshesBoolean3.h - Written by GitHub Copilot in 2026.

#pragma once
#ifndef YGOR_MESHES_BOOLEAN3_H_
#define YGOR_MESHES_BOOLEAN3_H_

#include <array>
#include <cstdint>
#include <optional>

#include "YgorDefinitions.h"
#include "YgorMath.h"


enum class MeshBooleanOperation3 {
    Union,
    Intersection,
    Exclusion,
    Subtraction
};


template <class T>
struct MeshBoolean3Options {
    T snap_eps = static_cast<T>(0);
    T fast_path_weld_eps = static_cast<T>(1.0e-7);
};


template <class T>
struct Boolean3Plane {
    plane<T> geometric;
    vec3<T> normal;
    T offset = static_cast<T>(0);
};


template <class T>
struct SymbolicVertex {
    std::array<Boolean3Plane<T>, 3> support_planes;
    mutable std::optional<vec3<T>> evaluated_point;
};


template <class T>
Boolean3Plane<T>
MakeBoolean3Plane(const vec3<T> &a,
                  const vec3<T> &b,
                  const vec3<T> &c);


template <class T>
std::optional<vec3<T>>
EvaluateSymbolicVertex(const SymbolicVertex<T> &vertex);


template <class T>
int
OrientSymbolicVertexAgainstPlane(const SymbolicVertex<T> &vertex,
                                 const Boolean3Plane<T> &plane);


// Perform an exact-style hybrid surface Boolean using adaptive predicates for the
// topological decisions, direct face subdivision on the clean fast path, and a
// localized degeneracy handler for coplanar / edge-touching / vertex-touching
// clusters.
template <class T, class I>
fv_surface_mesh<T, I>
BooleanMeshOp3(const fv_surface_mesh<T, I> &lhs,
               const fv_surface_mesh<T, I> &rhs,
               MeshBooleanOperation3 op,
               MeshBoolean3Options<T> options = {});


template <class T, class I>
fv_surface_mesh<T, I>
BooleanUnion3(const fv_surface_mesh<T, I> &lhs,
              const fv_surface_mesh<T, I> &rhs,
              MeshBoolean3Options<T> options = {});


template <class T, class I>
fv_surface_mesh<T, I>
BooleanIntersection3(const fv_surface_mesh<T, I> &lhs,
                     const fv_surface_mesh<T, I> &rhs,
                     MeshBoolean3Options<T> options = {});


template <class T, class I>
fv_surface_mesh<T, I>
BooleanExclusion3(const fv_surface_mesh<T, I> &lhs,
                  const fv_surface_mesh<T, I> &rhs,
                  MeshBoolean3Options<T> options = {});


template <class T, class I>
fv_surface_mesh<T, I>
BooleanSubtraction3(const fv_surface_mesh<T, I> &lhs,
                    const fv_surface_mesh<T, I> &rhs,
                    MeshBoolean3Options<T> options = {});


#endif // YGOR_MESHES_BOOLEAN3_H_
