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
    std::vector<I> vertices;   // Ordered boundary vertices.
    std::vector<I> faces;      // Face adjacent to each boundary edge.
    std::vector<I> face_edges; // Edge index in corresponding face.
    bool is_closed = false;
};


template <class I>
struct fv_surface_mesh_hole_chains {
    std::vector<fv_surface_mesh_hole_chain<I>> chains;
    bool has_nonmanifold_edges = false;
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
