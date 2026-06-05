//YgorMeshesVerification.h - Written by hal clark in 2026.
//
// Consistency checks for mManifoldness, closedness, and orientation for fv_surface_mesh's.

#pragma once
#ifndef YGOR_MESHES_VERIFICATION_HDR_GRD_H
#define YGOR_MESHES_VERIFICATION_HDR_GRD_H

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"


template <class I>
using undirected_edge_t = std::pair<I, I>;

template <class I>
undirected_edge_t<I>
make_undirected_edge(I a, I b) {
    return { std::min<I>(a, b), std::max<I>(a, b) };
}

template <class T>
bool
TriangleIsDegenerate(const vec3<T> &a, const vec3<T> &b, const vec3<T> &c);

template <class T, class I>
bool
HasOnlyFiniteVertices(const fv_surface_mesh<T, I> &mesh);

template <class T, class I>
bool
IsTriangularMesh(const fv_surface_mesh<T, I> &mesh);

template <class T, class I>
bool
HasValidFaceIndices(const fv_surface_mesh<T, I> &mesh);

template <class T, class I>
bool
HasNoDegenerateFaces(const fv_surface_mesh<T, I> &mesh,
                     bool allow_zero_area = false);

template <class T, class I>
bool
IsClosedManifold(const fv_surface_mesh<T, I> &mesh);


template <class I>
struct EdgeCountInfo {
    size_t unique_edges = 0UL;
    size_t boundary_edges = 0UL;
    size_t nonmanifold_edges = 0UL;
    size_t manifold_edges = 0UL;
};

template <class T, class I>
EdgeCountInfo<I>
ClassifyEdges(const fv_surface_mesh<T, I> &mesh);


template <class T, class I>
bool
HasConsistentOrientation(const fv_surface_mesh<T, I> &mesh);


template <class T, class I>
bool
ValidateClosedTriangularMesh(const fv_surface_mesh<T, I> &mesh,
                             const std::string &name,
                             bool throw_on_failure = true);


#endif // YGOR_MESHES_VERIFICATION_HDR_GRD_H
