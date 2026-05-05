//YgorMathConstrainedDelaunay.h

#pragma once
#ifndef YGOR_MATH_CONSTRAINED_DELAUNAY_H_
#define YGOR_MATH_CONSTRAINED_DELAUNAY_H_

#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// Perform 2D constrained Delaunay triangulation of vertices on a planar surface.
//
// The input vertices are vec3's where the z-component is expected to be zero (or near-zero). The triangulation is
// performed on the x-y plane. The z-component is ignored.
//
// The input edges are interpreted as undirected constraints and are expected to reference vertex indices using
// two-element vectors. Distinct valid user-supplied edges are emitted first in the output mesh as 2-vertex faces,
// followed by any generated triangulation edges, and finally the triangulation triangles.
//
// Invalid or mutually incompatible constraints (for example, out-of-range indices, self-intersections, or a vertex
// lying strictly in the interior of a constrained segment) cause an empty mesh to be returned.
//
// Vertices are copied to the mesh, so no input vertices are modified.
//
// The implementation follows the constrained Delaunay segment-insertion approach described by:
//  - Chew LP. Constrained delaunay triangulations. Proc. Symp. Comput. Geom. 1987:215-222.
// Robust orientation and in-circle predicates are evaluated using the adaptive arithmetic implementation exposed by
// YgorMeshesConvexHull.
template <class T, class I>
fv_surface_mesh<T, I>
Constrained_Delaunay_Triangulation_2(const std::vector<vec3<T>> &verts,
                                     const std::vector<std::vector<I>> &edges);


#endif // YGOR_MATH_CONSTRAINED_DELAUNAY_H_
