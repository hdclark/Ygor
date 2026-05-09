//YgorMeshesAdaptivePredicates.h - Written by hal clark in 2026.

// Adaptive predicates are used to work around the limitations of floating-point arithmetic
// when evaluating geometric relationships such as orientation and in/out tests. The routines
// in this file closely follow the robust expansion-arithmetic approach described in:
//
//   Shewchuk JR. Robust adaptive floating-point geometric predicates.
//   In Proceedings of the twelfth annual symposium on Computational geometry;
//   1996 May 1. pp. 141-150.
//
// The goal is to stay in native floating point when the sign of a determinant is obviously
// correct, and to fall back to expansion arithmetic only when the configuration is nearly
// degenerate.

#pragma once
#ifndef YGOR_MESHES_ADAPTIVE_PREDICATES_HDR_GRD_H
#define YGOR_MESHES_ADAPTIVE_PREDICATES_HDR_GRD_H

namespace adaptive_predicate {

// Compute the exact floating-point product decomposition a*b = hi + lo.
// Useful when assembling exact determinant expansions from ordinary products.
template <class T>
void two_product(T a, T b, T &hi, T &lo);

// Compute the exact floating-point sum decomposition a+b = hi + lo.
// Useful when merging partial determinant terms without losing the roundoff residue.
template <class T>
void two_sum(T a, T b, T &hi, T &lo);

// Add the scalar b to the expansion e[0:elen), writing the resulting expansion to h.
// Useful when a determinant term must be appended to an already accumulated expansion.
template <class T>
int grow_expansion(int elen, const T *e, T b, T *h);

// Add the expansions e[0:elen) and f[0:flen), writing the result to h.
// Useful when combining several exact sub-determinants into one larger exact determinant.
template <class T>
int expansion_sum(int elen, const T *e, int flen, const T *f, T *h);

// Multiply the expansion e[0:elen) by the scalar b, writing the result to h.
// Useful when a determinant cofactor must scale an exact lower-dimensional minor.
template <class T>
int scale_expansion(int elen, const T *e, T b, T *h);

// Remove exact zero entries from e[0:elen) and write the compacted expansion to h.
// Useful before reading the most significant expansion component as the determinant sign.
template <class T>
int compress(int elen, const T *e, T *h);

// Return a quick floating-point estimate of the value represented by e[0:elen).
// Useful when an approximate magnitude is needed without collapsing the exact expansion logic.
template <class T>
T estimate(int elen, const T *e);

// Robust 3D orientation predicate for four points pa, pb, pc, and pd.
//
// Arguments:
//   pa, pb, pc - vertices defining the reference oriented plane.
//   pd         - query point tested against that plane.
//
// Returns a positive value when pd lies on one side of the oriented plane through
// (pa, pb, pc), a negative value on the opposite side, and zero when the four
// points are coplanar. This is useful when building convex hulls, classifying
// tetrahedra, or rejecting coplanar degeneracies in 3D mesh algorithms.
//
// This routine performs the full expansion-arithmetic evaluation described by
// Shewchuk (1996), so it is appropriate when near-degenerate configurations must
// still produce a trustworthy sign.
template <class T>
T orient3d_adaptive(const T *pa, const T *pb, const T *pc, const T *pd);

// Fast 3D orientation predicate with adaptive fallback.
//
// Arguments match orient3d_adaptive. This routine first evaluates the determinant
// in ordinary floating point and only falls back to orient3d_adaptive when the
// determinant is too small relative to a conservative roundoff bound. This is the
// routine to call for general-purpose 3D orientation tests.
template <class T>
T orient3d(const T *pa, const T *pb, const T *pc, const T *pd);

// Robust insphere predicate for five 3D points pa, pb, pc, pd, and pe.
//
// Arguments:
//   pa, pb, pc, pd - points defining the reference sphere.
//   pe             - query point tested against that sphere.
//
// The sign indicates whether pe is inside, outside, or on the sphere that passes
// through pa, pb, pc, and pd, up to the usual dependence on the orientation of
// the tetrahedron (pa, pb, pc, pd). This is useful for Delaunay tetrahedralization,
// circumsphere tests, and robust local-topology decisions in 3D.
//
// This routine performs the full expansion-arithmetic evaluation of the lifted
// determinant so that near-cospherical inputs still produce a trustworthy sign.
template <class T>
T insphere_adaptive(const T *pa, const T *pb, const T *pc, const T *pd, const T *pe);

// Fast insphere predicate with adaptive fallback.
//
// Arguments match insphere_adaptive. This routine uses an inexpensive floating-point
// determinant when the sign is clearly separated from roundoff noise and otherwise
// falls back to insphere_adaptive.
template <class T>
T insphere(const T *pa, const T *pb, const T *pc, const T *pd, const T *pe);

} // namespace adaptive_predicate

#endif // YGOR_MESHES_ADAPTIVE_PREDICATES_HDR_GRD_H
