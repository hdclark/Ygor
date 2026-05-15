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
// The input vertices are vec2's representing planar x-y coordinates.
//
// Returns an fv_surface_mesh containing the triangulation as faces. The mesh is not watertight, and represents a
// 'patch' of surface. Vertices are copied to the mesh, so no input vertices are modified.
//
// Invalid input (for example, fewer than 3 vertices, non-finite coordinates, or collinear/coincident geometry) and
// unexpected triangulation failures are reported by throwing an exception with a diagnostic message.
//
// The implementation follows an approach inspired primarily by:
//  - Edelsbrunner H, Seidel R. Voronoi diagrams and arrangements. In Proceedings of the first annual symposium on
//    Computational geometry 1985 Jun 1 (pp. 251-262).
//
// Algorithm overview: this implementation generates a Delaunay triangulation by 'lifting' vec2 inputs into vec3 via
// normalizing and assigning a z coordinate placed on paraboloid z = x*x + y*y, computing the 3D convex hull, and
// extracting the lower part of the hull (which is the unconstrained Delaunay triangulation).
//
// Other relevant references:
//  - Bowyer A. The Computer Journal. 1981;24(2):162-166.
//  - Watson DF. The Computer Journal. 1981;24(2):167-172.
//
// Robust orientation and in-circle sign tests are evaluated using adaptive arithmetic.
template <class T, class I>
fv_surface_mesh<T, I>
Delaunay_Triangulation_2(const std::vector<vec2<T>> &verts);


#endif // YGOR_MATH_DELAUNAY_H_
