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
// The input edges are interpreted as undirected constraints and are expected to reference vertex indices using
// two-element vectors. They constrain the triangulation internally, but the returned mesh contains only the generated
// triangular facets.
//
// Invalid or mutually incompatible constraints (for example, out-of-range indices, self-intersections, or a vertex
// laying strictly in the interior of a constrained segment) and unexpected triangulation failures are reported by
// throwing an exception with a diagnostic message.
//
// Vertices are copied to the mesh, so no input vertices are modified.
//
// The implementation follows the constrained Delaunay segment-insertion approach described by:
//  - Chew LP. Constrained delaunay triangulations. Proc. Symp. Comput. Geom. 1987:215-222.
//
// Robust orientation and in-circle sign tests are evaluated using adaptive arithmetic.
template <class T, class I>
fv_surface_mesh<T, I>
Constrained_Delaunay_Triangulation_2(const std::vector<vec2<T>> &verts,
                                     const std::vector<std::vector<I>> &edges);


#endif // YGOR_MATH_CONSTRAINED_DELAUNAY_H_
