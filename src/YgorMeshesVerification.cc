//YgorMeshesVerification.cc - Written by hal clark in 2026.

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorMeshesVerification.h"

template <class T>
bool
TriangleIsDegenerate(const vec3<T> &a, const vec3<T> &b, const vec3<T> &c){
    // Project the face onto the YZ, ZX, and XY planes, checking the x-component, y-component, and z-component of the
    // cross product separately.
    //
    // If any projection has non-zero area, the 3D triangle has non-zero area.
    const auto a_yz = vec2<T>(a.y, a.z);
    const auto b_yz = vec2<T>(b.y, b.z);
    const auto c_yz = vec2<T>(c.y, c.z);
    
    const auto a_zx = vec2<T>(a.z, a.x);
    const auto b_zx = vec2<T>(b.z, b.x);
    const auto c_zx = vec2<T>(c.z, c.x);
    
    const auto a_xy = vec2<T>(a.x, a.y);
    const auto b_xy = vec2<T>(b.x, b.y);
    const auto c_xy = vec2<T>(c.x, c.y);
    
    // Only if all three planar projections are strictly zero, the vertices are perfectly collinear and the triangle
    // is degenerate.
    return (orient_sign(a_yz, b_yz, c_yz) == static_cast<T>(0)) 
        && (orient_sign(a_zx, b_zx, c_zx) == static_cast<T>(0))
        && (orient_sign(a_xy, b_xy, c_xy) == static_cast<T>(0));
}

template <class T, class I>
bool
HasOnlyFiniteVertices(const fv_surface_mesh<T, I> &mesh) {
    for(const auto &v : mesh.vertices) {
        if(!v.isfinite()) return false;
    }
    return true;
}


template <class T, class I>
bool
IsTriangularMesh(const fv_surface_mesh<T, I> &mesh) {
    for(const auto &face : mesh.faces) {
        if(face.size() != 3UL) return false;
    }
    return true;
}


template <class T, class I>
bool
HasValidFaceIndices(const fv_surface_mesh<T, I> &mesh) {
    for(const auto &face : mesh.faces) {
        for(const auto vi : face) {
            if(vi >= mesh.vertices.size()) return false;
        }
    }
    return true;
}


template <class T, class I>
bool
HasNoDegenerateFaces(const fv_surface_mesh<T, I> &mesh,
                     bool allow_zero_area) {


    for(const auto &face : mesh.faces){
        if(face.size() < 3UL) return false;

        const auto &v0 = mesh.vertices.at(face.at(0));
        const auto &v1 = mesh.vertices.at(face.at(1));
        const auto &v2 = mesh.vertices.at(face.at(2));
        if(face[0] == face[1] || face[1] == face[2] || face[2] == face[0]){
            return false;
        }

        if(!allow_zero_area){
            if(TriangleIsDegenerate(v0, v1, v2)){
                return false;
            }
        }
    }
    return true;
}


template <class T, class I>
bool
IsClosedManifold(const fv_surface_mesh<T, I> &mesh) {
    if(!IsTriangularMesh(mesh)) return false;
    if(!HasValidFaceIndices(mesh)) return false;

    const auto info = ClassifyEdges<T, I>(mesh);
    return (info.boundary_edges == 0UL)
        && (info.nonmanifold_edges == 0UL)
        && (info.unique_edges > 0UL);
}


template <class T, class I>
EdgeCountInfo<I>
ClassifyEdges(const fv_surface_mesh<T, I> &mesh) {
    EdgeCountInfo<I> info;
    if(mesh.faces.empty()) return info;

    std::map<undirected_edge_t<I>, size_t> edge_counts;
    for(const auto &face : mesh.faces) {
        const size_t n = face.size();
        for(size_t i = 0; i < n; ++i) {
            const I a = face[i];
            const I b = face[(i + 1UL) % n];
            edge_counts[make_undirected_edge(a, b)] += 1UL;
        }
    }

    for(const auto &[edge, count] : edge_counts) {
        (void)edge;
        ++info.unique_edges;
        if(count == 1UL) {
            ++info.boundary_edges;
        } else if(count == 2UL) {
            ++info.manifold_edges;
        } else {
            ++info.nonmanifold_edges;
        }
    }
    return info;
}


template <class T, class I>
bool
HasConsistentOrientation(const fv_surface_mesh<T, I> &mesh) {
    if(!IsTriangularMesh(mesh)) return false;

    std::map<std::pair<I, I>, std::vector<I>> directed;
    for(I f = 0; f < static_cast<I>(mesh.faces.size()); ++f) {
        const auto &face = mesh.faces[f];
        for(size_t i = 0; i < face.size(); ++i) {
            const I a = face[i];
            const I b = face[(i + 1UL) % face.size()];
            directed[{a, b}].push_back(f);
        }
    }

    std::map<undirected_edge_t<I>, std::vector<std::pair<I, I>>> undirected;
    for(const auto &entry : directed) {
        const auto &edge = entry.first;
        const auto &face_indices = entry.second;
        for(const auto &f_idx : face_indices) {
            undirected[make_undirected_edge(edge.first, edge.second)].push_back(edge);
        }
    }

    for(const auto &[key, edges] : undirected) {
        (void)key;
        if(edges.size() != 2UL) continue;
        if(edges[0].first == edges[1].first) return false;
    }
    return true;
}


template <class T, class I>
bool
ValidateClosedTriangularMesh(const fv_surface_mesh<T, I> &mesh,
                             const std::string &name,
                             bool throw_on_failure) {
    if(mesh.faces.empty()) {
        return true;
    }

    if(!HasOnlyFiniteVertices(mesh)) {
        if(throw_on_failure)
            throw std::invalid_argument(name + " contains a non-finite vertex.");
        return false;
    }

    if(!IsTriangularMesh(mesh)) {
        if(throw_on_failure)
            throw std::invalid_argument(name + " must contain only triangular faces.");
        return false;
    }

    if(!HasValidFaceIndices(mesh)) {
        if(throw_on_failure)
            throw std::invalid_argument(name + " contains an out-of-range face index.");
        return false;
    }

    if(!HasNoDegenerateFaces(mesh)) {
        if(throw_on_failure)
            throw std::invalid_argument(name + " contains a degenerate triangle.");
        return false;
    }

    if((mesh.faces.size() * 3ULL) % 2ULL != 0ULL) {
        if(throw_on_failure)
            throw std::invalid_argument(name + " violates the 3F = 2E handshake invariant.");
        return false;
    }

    const auto info = ClassifyEdges<T, I>(mesh);
    if(info.boundary_edges > 0UL || info.nonmanifold_edges > 0UL) {
        if(throw_on_failure)
            throw std::invalid_argument(name + " is not a closed manifold mesh.");
        return false;
    }

    return true;
}


// Explicit template instantiations.
#ifndef YGOR_MESHES_VERIFICATION_DISABLE_ALL_SPECIALIZATIONS

template bool HasOnlyFiniteVertices      (const fv_surface_mesh<float,  uint32_t> &);
template bool IsTriangularMesh           (const fv_surface_mesh<float,  uint32_t> &);
template bool HasValidFaceIndices        (const fv_surface_mesh<float,  uint32_t> &);
template bool HasNoDegenerateFaces       (const fv_surface_mesh<float,  uint32_t> &, bool);
template bool IsClosedManifold           (const fv_surface_mesh<float,  uint32_t> &);
template EdgeCountInfo<uint32_t> ClassifyEdges(const fv_surface_mesh<float,  uint32_t> &);
template bool HasConsistentOrientation   (const fv_surface_mesh<float,  uint32_t> &);
template bool ValidateClosedTriangularMesh(const fv_surface_mesh<float,  uint32_t> &, const std::string &, bool);

template bool HasOnlyFiniteVertices      (const fv_surface_mesh<float,  uint64_t> &);
template bool IsTriangularMesh           (const fv_surface_mesh<float,  uint64_t> &);
template bool HasValidFaceIndices        (const fv_surface_mesh<float,  uint64_t> &);
template bool HasNoDegenerateFaces       (const fv_surface_mesh<float,  uint64_t> &, bool);
template bool IsClosedManifold           (const fv_surface_mesh<float,  uint64_t> &);
template EdgeCountInfo<uint64_t> ClassifyEdges(const fv_surface_mesh<float,  uint64_t> &);
template bool HasConsistentOrientation   (const fv_surface_mesh<float,  uint64_t> &);
template bool ValidateClosedTriangularMesh(const fv_surface_mesh<float,  uint64_t> &, const std::string &, bool);

template bool HasOnlyFiniteVertices      (const fv_surface_mesh<double, uint32_t> &);
template bool IsTriangularMesh           (const fv_surface_mesh<double, uint32_t> &);
template bool HasValidFaceIndices        (const fv_surface_mesh<double, uint32_t> &);
template bool HasNoDegenerateFaces       (const fv_surface_mesh<double, uint32_t> &, bool);
template bool IsClosedManifold           (const fv_surface_mesh<double, uint32_t> &);
template EdgeCountInfo<uint32_t> ClassifyEdges(const fv_surface_mesh<double, uint32_t> &);
template bool HasConsistentOrientation   (const fv_surface_mesh<double, uint32_t> &);
template bool ValidateClosedTriangularMesh(const fv_surface_mesh<double, uint32_t> &, const std::string &, bool);

template bool HasOnlyFiniteVertices      (const fv_surface_mesh<double, uint64_t> &);
template bool IsTriangularMesh           (const fv_surface_mesh<double, uint64_t> &);
template bool HasValidFaceIndices        (const fv_surface_mesh<double, uint64_t> &);
template bool HasNoDegenerateFaces       (const fv_surface_mesh<double, uint64_t> &, bool);
template bool IsClosedManifold           (const fv_surface_mesh<double, uint64_t> &);
template EdgeCountInfo<uint64_t> ClassifyEdges(const fv_surface_mesh<double, uint64_t> &);
template bool HasConsistentOrientation   (const fv_surface_mesh<double, uint64_t> &);
template bool ValidateClosedTriangularMesh(const fv_surface_mesh<double, uint64_t> &, const std::string &, bool);

#endif // YGOR_MESHES_VERIFICATION_DISABLE_ALL_SPECIALIZATIONS
