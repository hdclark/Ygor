//YgorMathDelaunay.h

#pragma once
#ifndef YGOR_MATH_DELAUNAY_H_
#define YGOR_MATH_DELAUNAY_H_

#include <stddef.h>
#include <any>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// Perform 2D Delaunay triangulation of vertices on a planar surface.
//
// The input vertices are vec3's where the z-component is expected to be zero (or near-zero). The triangulation is
// performed on the x-y plane.
//
// Returns an fv_surface_mesh containing the triangulation as faces. The mesh is not watertight, and represents a
// 'patch' of surface. Vertices are copied to the mesh, so no input vertices are modified.
//
// If fewer than 3 vertices are provided, an empty mesh is returned.
//
// Note: This is a 2D incremental Bowyer-Watson triangulation:
//  - Bowyer A. The Computer Journal. 1981;24(2):162-166.
//  - Watson DF. The Computer Journal. 1981;24(2):167-172.
// For 3D surface meshing, consider using Convex_Hull_3.
template <class T, class I>
fv_surface_mesh<T, I>
Delaunay_Triangulation_2(const std::vector<vec3<T>> &verts);


#endif // YGOR_MATH_DELAUNAY_H_
