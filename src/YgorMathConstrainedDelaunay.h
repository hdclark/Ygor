//YgorMathConstrainedDelaunay.h.

#pragma once
#ifndef YGOR_MATH_CONSTRAINED_DELAUNAY_H_
#define YGOR_MATH_CONSTRAINED_DELAUNAY_H_

#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// Perform 2D constrained Delaunay triangulation of vertices on a planar surface.
//
// The input vertices are vec2's representing planar x-y coordinates.
//
// The input edges are interpreted as undirected constraints and are expected to reference valid vertex indices.
// They constrain the triangulation internally, but the returned mesh contains only the generated
// triangular facets.
//
// Invalid or mutually incompatible constraints (for example, out-of-range indices, self-intersections, or a vertex
// laying strictly in the interior of a constrained segment) and unexpected triangulation failures are reported by
// throwing an exception with a diagnostic message.
//
// Vertices are copied to the mesh, so no input vertices are modified.
//
// retain_only_constraint_faces controls whether triangles are filtered to the bounded regions enclosed by the
// constraints. Most polygon-style callers should leave this enabled. Callers that use constraints only as interior
// split lines inside some separate outer domain can disable it and apply their own domain filtering afterward.
//
// The implementation follows an approach inspired primarily by:
//  - Edelsbrunner H, Seidel R. Voronoi diagrams and arrangements. In Proceedings of the first annual symposium on
//    Computational geometry 1985 Jun 1 (pp. 251-262).
//
// Algorithm overview: this implementation first generates a full unconstrained Delaunay triangulation using the
// 'lifting' method of hoisting vec2 into vec3 via normalizing and assigning a z coordinate placed on paraboloid
// z = x*x + y*y, extracting the lower part of the 3D convex hull (which is the unconstrained Delaunay triangulation),
// and then implementing local mesh adjustments to achieve the input constraints.
//
// Other relevant references:
//  - Chew LP. Constrained delaunay triangulations. Proc. Symp. Comput. Geom. 1987:215-222.
//  - Sloan SW. A fast algorithm for generating constrained Delaunay triangulations.
//    Computers & Structures. 1993 May 3;47(3):441-50.
//
// Robust orientation and in-circle sign tests are evaluated using adaptive arithmetic.
template <class T, class I>
fv_surface_mesh<T, I>
Constrained_Delaunay_Triangulation_2(const std::vector<vec2<T>> &verts,
                                     const std::vector<std::vector<I>> &edges,
                                     bool retain_only_constraint_faces = true);


#endif // YGOR_MATH_CONSTRAINED_DELAUNAY_H_
