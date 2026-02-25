//YgorMeshesHoles.h - Written by hal clark in 2026.

#pragma once
#ifndef YGOR_MESHES_HOLES_HDR_GRD_H
#define YGOR_MESHES_HOLES_HDR_GRD_H

#include <vector>
#include <cstdint>

#include "YgorDefinitions.h"
#include "YgorMath.h"


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


// Detect boundary edge chains ("holes") in a face-vertex surface mesh.
//
// The output contains one chain per connected boundary, with entries aligned so
// that vertices[i] is the first endpoint of the boundary edge associated with
// faces[i]/face_edges[i]. The next endpoint is vertices[(i+1) % N] for closed
// chains, where N = vertices.size().
//
// Vertices separated by <= eps are treated as coincident while building
// adjacency, helping to tolerate small duplicate-vertex perturbations.
//
// This routine does not modify the mesh.
template <class T, class I>
fv_surface_mesh_hole_chains<I>
FindBoundaryChains(const fv_surface_mesh<T,I> &fvsm,
                   T eps = static_cast<T>(1E-6));


// Attempt to fill closed boundary chains by zippering/triangulating each chain.
//
// Chains with fewer than 3 vertices or open chains are ignored. Degenerate
// triangles (duplicate indices or very short edges <= eps) are skipped.
//
// Returns false only when the supplied boundary data is incompatible with the
// mesh (e.g., out-of-range indices) or when non-manifold edges are flagged in
// holes. Returns true otherwise (including "no-op" cases).
template <class T, class I>
bool
FillBoundaryChainsByZippering(fv_surface_mesh<T,I> &fvsm,
                              const fv_surface_mesh_hole_chains<I> &holes,
                              T eps = static_cast<T>(1E-6));


// Ensure adjacent faces are consistently oriented, flipping faces as needed.
//
// Returns false when a consistent orientation cannot be imposed due to
// non-manifoldness or contradictory adjacency constraints.
//
// If genus != nullptr, genus is computed for each connected component and
// summed. Boundary counting assumes manifold boundaries are closed loops. If
// malformed boundaries are encountered, this routine throws with guidance for
// remediation (e.g., refine/clean the mesh).
//
// eps controls duplicate-vertex tolerance when constructing edge adjacency.
template <class T, class I>
bool
EnsureConsistentFaceOrientation(fv_surface_mesh<T,I> &fvsm,
                                T eps = static_cast<T>(1E-6),
                                int64_t *genus = nullptr);

#endif // YGOR_MESHES_HOLES_HDR_GRD_H

