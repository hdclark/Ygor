//YgorMeshesTetrahedralize.h - Written by hal clark in 2026.
//
// Routines for converting polyhedral surface meshes into tetrahedral (volumetric) meshes
// using octree-based spatial decomposition.
//

#pragma once

#ifndef YGOR_MESHES_TETRAHEDRALIZE_H_
#define YGOR_MESHES_TETRAHEDRALIZE_H_

#include <cstdint>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// Convert a polyhedral surface mesh into a tetrahedral (volumetric) mesh using octree-based decomposition.
//
// This function generates a conforming tetrahedral mesh of the interior volume enclosed by the input surface
// mesh. The algorithm proceeds as follows:
//
//   1. An adaptive octree is built around the surface mesh bounding box.
//      Cells intersecting the surface are recursively refined up to the specified maximum depth.
//   2. The octree is 2:1 balanced so that adjacent leaf cells differ by at most one refinement level.
//   3. Each leaf cell is classified as interior, exterior, or boundary using ray casting.
//   4. Interior and boundary cells are decomposed into tetrahedra using a body-centred approach that
//      naturally handles transitions between cells at different refinement levels.
//   5. Duplicate vertices are merged and degenerate tetrahedra are removed.
//
// The resulting mesh is suitable for finite-element analysis.
//
// Note: The input surface mesh should represent a closed (watertight), consistently-oriented surface.
//       Open or self-intersecting surfaces may produce unexpected results.
//
// Note: The 'max_depth' parameter controls mesh resolution. Each additional level of depth doubles the
//       number of cells along each axis, significantly increasing the output mesh size. Typical values
//       are 3--6.
//
// Note: The 'boundary_scale' parameter controls how much the bounding box is expanded relative to the
//       mesh extent. A small expansion (e.g., 0.05 = 5%) helps avoid numerical issues at the boundary.
//
// Note: This function uses the Ygor octree for spatial indexing of surface triangles, which accelerates
//       the adaptive refinement and inside/outside classification steps.
//
template <class T, class I>
fv_tet_mesh<T, I>
tetrahedral_mesh_from_surface_mesh(const fv_surface_mesh<T, I> &surface,
                                   int64_t max_depth = 5,
                                   T boundary_scale = static_cast<T>(0.05));

#endif // YGOR_MESHES_TETRAHEDRALIZE_H_
