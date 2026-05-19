//YgorMathMonotoneDecomposition.h

#pragma once
#ifndef YGOR_MATH_MONOTONE_DECOMPOSITION_H_
#define YGOR_MATH_MONOTONE_DECOMPOSITION_H_

#include <cstddef>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// Decompose one or more closed simple polygons into y-monotone polygons.
//
// Each input polygon is supplied as a list of vec2 vertices. The polygons are allowed to be nested, but they must not
// self-intersect, mutually intersect, or touch. The implementation first classifies each polygon according to odd-even
// nesting depth and then decomposes each boundary loop independently into y-monotone pieces using the sweep-line vertex
// taxonomy and helper-edge mechanism described by:
//  - de Berg M, Cheong O, van Kreveld M, Overmars M. Computational Geometry: Algorithms and Applications.
//    3rd ed. Springer; 2008. Chapter 3.
//
// Robust geometric predicates are evaluated through the adaptive-orientation wrappers in YgorMath, which in turn follow:
//  - Shewchuk JR. Robust adaptive floating-point geometric predicates. Proc 12th ACM Symp Comput Geom. 1996:141-150.
//
// The output polygons do not copy coordinates; instead they reference the input as (polygon_index, vertex_index) pairs.
// The 'interior' flag uses odd-even nesting parity so that outer shells are marked interior=true, holes interior=false,
// islands inside holes interior=true, and so on.
template <class I>
struct monotone_t {
    bool interior = true;
    std::vector<std::pair<I, I>> vertices;
};

template <class T, class I = uint64_t>
std::vector<monotone_t<I>>
Monotone_Decomposition_2(const std::vector<std::vector<vec2<T>>> &verts);


#endif // YGOR_MATH_MONOTONE_DECOMPOSITION_H_
