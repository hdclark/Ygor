//YgorMeshesAdaptivePredicates.h - Written by hal clark in 2026.

// Adaptive predicates are used to try work around the limitations of floating-point types
// when working with geometric objects to determine spatial relationships (e.g., orientation,
// relative position, inside/outside assessment) without resorting to arbitrary-precision
// arithmetic. The key benefit is the adaptive predicates are typically faster than arbitrary
// precision arithmetic, only resorting to more precise computations when needed.

#pragma once
#ifndef YGOR_MESHES_ADAPTIVE_PREDICATES_HDR_GRD_H
#define YGOR_MESHES_ADAPTIVE_PREDICATES_HDR_GRD_H

namespace adaptive_predicate {

// Two-product: computes a*b = hi + lo exactly in floating point.
template <class T>
void two_product(T a, T b, T &hi, T &lo);

// Two-sum: computes a+b = hi + lo exactly in floating point.
template <class T>
void two_sum(T a, T b, T &hi, T &lo);

// Grow-expansion: adds a scalar b to an existing expansion e (length elen),
// writing the result into h. Returns the new length.
template <class T>
int grow_expansion(int elen, const T *e, T b, T *h);

// Expansion-sum: adds two expansions e and f, writing result to h.
// Returns the new length.
template <class T>
int expansion_sum(int elen, const T *e, int flen, const T *f, T *h);

// Scale-expansion: multiplies an expansion e by a scalar b, writing result
// to h.  Returns the new length.
template <class T>
int scale_expansion(int elen, const T *e, T b, T *h);

// Compress: removes zero entries from an expansion e, writing result to h.
// Returns the new length.
template <class T>
int compress(int elen, const T *e, T *h);

// Estimate: returns a fast floating-point approximation of the expansion sum.
template <class T>
T estimate(int elen, const T *e);

// orient3d_adaptive: robust orientation predicate for four 3D points.
//
// Returns a positive value when d is below the plane of (a,b,c) (oriented
// counter-clockwise when viewed from above), negative when above, and zero
// when coplanar. Handles all degeneracies via exact arithmetic.
template <class T>
T orient3d_adaptive(const T *pa, const T *pb, const T *pc, const T *pd);

// orient3d: fast-path with adaptive fallback.
template <class T>
T orient3d(const T *pa, const T *pb, const T *pc, const T *pd);

} // namespace adaptive_predicate

#endif // YGOR_MESHES_ADAPTIVE_PREDICATES_HDR_GRD_H
