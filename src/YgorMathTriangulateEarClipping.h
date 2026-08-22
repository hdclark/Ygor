//YgorMathTriangulateEarClipping.h

#pragma once
#ifndef YGOR_MATH_TRIANGULATE_EAR_CLIPPING_H_
#define YGOR_MATH_TRIANGULATE_EAR_CLIPPING_H_

#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"

// Perform 2D triangulation of closed polygon boundaries on a planar surface.
//
// The input is interpreted as a collection of closed polygon rings in planar x-y coordinates.
// Each ring may be oriented arbitrarily. Nested rings are interpreted with the even-odd rule, so
// an outer boundary may contain holes, islands, and deeper alternating nesting.
//
// The returned mesh contains only triangular facets embedded in z=0. Invalid input (for example,
// non-finite coordinates, self-intersections, touching/intersecting rings, or zero-area rings) and
// unexpected triangulation failures are reported by throwing an exception with a diagnostic message.
//
// The implementation follows a sweep-line monotone decomposition pipeline and then clips ears from
// the resulting monotone pieces. The implementation is based primarily on:
//  - Garey MR, Johnson DS, Preparata FP, Tarjan RE. Triangulating a simple polygon.
//    Information Processing Letters. 1978;7(4):175-179.
//  - de Berg M, Cheong O, van Kreveld M, Overmars M. Computational Geometry: Algorithms and
//    Applications. 3rd ed. Springer; 2008. Chapter 3.
//  - Meisters GH. Polygons have ears. The American Mathematical Monthly. 1975;82(6):648-651.
//
// Robust orientation and point-on-segment tests are evaluated using adaptive arithmetic.
template <class T, class I>
fv_surface_mesh<T, I>
Triangulate_Ear_Clipping_2(const std::vector<std::vector<vec2<T>>> &closed_polygons);

#endif // YGOR_MATH_TRIANGULATE_EAR_CLIPPING_H_
