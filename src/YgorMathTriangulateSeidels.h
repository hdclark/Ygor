//YgorMathTriangulateSeidels.h.

#pragma once
#ifndef YGOR_MATH_TRIANGULATE_SEIDELS_H_
#define YGOR_MATH_TRIANGULATE_SEIDELS_H_

#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// Perform 2D constrained triangulation of one or more closed polygon loops.
//
// The input is a collection of closed polygon boundaries in the x-y plane.  Each
// loop may be wound either clockwise or counter-clockwise; region membership is
// determined by odd/even nesting depth rather than by winding convention so the
// caller does not need to normalize polygon orientation first.
//
// Loops are expected to be simple, pairwise non-intersecting, and topologically
// compatible with a polygonal odd-parity region.  Degenerate or ambiguous inputs
// (for example, non-finite coordinates, repeated non-closure vertices, zero-length
// edges, self-intersections, touching boundaries, or loops without any
// non-collinear triplet) are rejected with an exception and a diagnostic log
// message.
//
// The returned fv_surface_mesh contains only triangular faces spanning the bounded
// odd-parity regions induced by the input loops; vertices are copied into the mesh.
//
// Relevant references for polygon-triangulation background and the odd/even
// region interpretation used here:
//  - Seidel R. A simple and fast incremental randomized algorithm for computing
//    trapezoidal decompositions and for triangulating polygons. Computational
//    Geometry. 1991;1(1):51-64.
//  - de Berg M, van Kreveld M, Overmars M, Schwarzkopf O. Computational Geometry:
//    Algorithms and Applications. 2nd ed. Springer; 2000. Chapter 3.
//
// The current implementation keeps the robust closed-polygon preprocessing local
// and then triangulates the validated odd-parity region through a self-contained
// trapezoidal decomposition followed by direct triangulation of the resulting
// monotone trapezoids.  Robust orientation, point-in-polygon, and
// segment-intersection tests are delegated to the adaptive arithmetic predicates
// wrapped in YgorMath.
template <class T, class I>
fv_surface_mesh<T, I>
Triangulate_Seidels_2(const std::vector<std::vector<vec2<T>>> &closed_polygons);

template <class T, class I>
inline fv_surface_mesh<T, I>
Triangulate_Polygons_Seidel_2(const std::vector<std::vector<vec2<T>>> &closed_polygons){
    return Triangulate_Seidels_2<T, I>(closed_polygons);
}

template <class T, class I>
inline fv_surface_mesh<T, I>
Constrained_Triangulation_Seidel_2(const std::vector<std::vector<vec2<T>>> &closed_polygons){
    return Triangulate_Seidels_2<T, I>(closed_polygons);
}


#endif // YGOR_MATH_TRIANGULATE_SEIDELS_H_
