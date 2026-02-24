//YgorMeshProcessing.h - Written by hal clark in 2026.
//
// Surface mesh processing routines.
//

#pragma once

#include <vector>
#include <cstdint>

#include "YgorDefinitions.h"

template <class T, class I> class fv_surface_mesh;


template <class I>
struct fv_surface_mesh_hole_chain {
    // Indices of boundary vertices, ordered along the boundary chain.
    std::vector<I> vertices;
    // For each boundary edge (between vertices[i] and vertices[i+1]), the index
    // of the face in the mesh that is adjacent to that edge.
    std::vector<I> faces;
    // For each boundary edge, the local edge index within the corresponding
    // face given by faces[i].
    std::vector<I> face_edges;
    // True if the boundary chain is closed (first and last vertices coincide).
    bool is_closed = false;
};


template <class I>
struct fv_surface_mesh_hole_chains {
    // All detected boundary chains in the surface mesh.
    std::vector<fv_surface_mesh_hole_chain<I>> chains;
    // True if any boundary edge in the mesh is nonmanifold (shared by more
    // than two faces), which may limit the robustness of boundary processing.
    bool has_nonmanifold_edges = false;
    // True if the boundary configuration is ambiguous or inconsistent and
    // cannot be cleanly represented as disjoint boundary chains.
    bool has_ambiguous_boundary = false;
};


template <class T, class I>
fv_surface_mesh_hole_chains<I>
FindBoundaryChains(const fv_surface_mesh<T,I> &fvsm,
                   T eps = static_cast<T>(1E-6));


template <class T, class I>
bool
FillBoundaryChainsByZippering(fv_surface_mesh<T,I> &fvsm,
                              const fv_surface_mesh_hole_chains<I> &holes,
                              T eps = static_cast<T>(1E-6));


template <class T, class I>
bool
EnsureConsistentFaceOrientation(fv_surface_mesh<T,I> &fvsm,
                                T eps = static_cast<T>(1E-6),
                                int64_t *genus = nullptr);
