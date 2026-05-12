//YgorMeshesAdaptivePredicates.cc - Written by hal clark in 2026.
//
// These adaptive predicates closely follow the expansion-arithmetic strategy from:
//
//   Shewchuk JR. Robust adaptive floating-point geometric predicates.
//   In Proceedings of the twelfth annual symposium on Computational geometry;
//   1996 May 1. pp. 141-150.
//
// The implementation intentionally mirrors the paper's building blocks: exact two-term
// decompositions, expansion summation, and adaptive fall-backs around determinant signs.

#include <algorithm>
#include <cmath>
#include <limits>

#include "YgorMathArbPrec.h"
#include "YgorMeshesAdaptivePredicates.h"

namespace adaptive_predicate {

#if defined(__GNUC__)
#pragma GCC push_options
#pragma GCC optimize ("no-fast-math")
#endif

namespace detail {

// splitter is (2^ceil(p/2) + 1) where p is the number of significand bits.
// For double (p = 53): splitter = 2^27 + 1 = 134217729.
// For float  (p = 24): splitter = 2^12 + 1 = 4097.
template <class T>
constexpr T splitter(){
    constexpr int p = std::numeric_limits<T>::digits; // significand bits
    constexpr int s = (p + 1) / 2;
    T val = static_cast<T>(1);
    for(int i = 0; i < s; ++i) val *= static_cast<T>(2);
    return val + static_cast<T>(1);
}

// Split the scalar a into high and low limbs ahi and alo so that
// a = ahi + alo exactly. Shewchuk uses this split before forming exact
// products, because it exposes the roundoff that would otherwise be lost.
//
// Arguments:
//   a        - scalar to split into two half-size limbs.
//   ahi, alo - outputs receiving the leading and trailing limbs.
//
// This helper is useful anywhere an exact product is needed, such as the
// oriented-volume and insphere determinants below.
template <class T>
inline void split(T a, T &ahi, T &alo){
    T c = splitter<T>() * a;
    T abig = c - a;
    ahi = c - abig;
    alo = a - ahi;
}

template <class T>
void negate_expansion(int elen, const T *e, T *h){
    for(int i = 0; i < elen; ++i){
        h[i] = -e[i];
    }
}

template <class T>
int det2_expansion(T a11, T a12, T a21, T a22, T *h){
    T p1_hi, p1_lo;
    two_product(a11, a22, p1_hi, p1_lo);
    const T p1[2] = { p1_lo, p1_hi };

    T p2_hi, p2_lo;
    two_product(a21, a12, p2_hi, p2_lo);
    const T neg_p2[2] = { -p2_lo, -p2_hi };

    const int hlen = expansion_sum(2, p1, 2, neg_p2, h);
    T compressed[4];
    const int compressed_len = compress(hlen, h, compressed);
    std::copy(compressed, compressed + compressed_len, h);
    return compressed_len;
}

template <class T>
int det3_expansion(const T m[3][3], T *h){
    T minor1[4];
    const int minor1_len = det2_expansion(m[1][1], m[1][2], m[2][1], m[2][2], minor1);
    T term1[16];
    const int term1_len = scale_expansion(minor1_len, minor1, m[0][0], term1);

    T minor2[4];
    const int minor2_len = det2_expansion(m[1][0], m[1][2], m[2][0], m[2][2], minor2);
    T neg_minor2[4];
    negate_expansion(minor2_len, minor2, neg_minor2);
    T term2[16];
    const int term2_len = scale_expansion(minor2_len, neg_minor2, m[0][1], term2);

    T minor3[4];
    const int minor3_len = det2_expansion(m[1][0], m[1][1], m[2][0], m[2][1], minor3);
    T term3[16];
    const int term3_len = scale_expansion(minor3_len, minor3, m[0][2], term3);

    T sum12[32];
    const int sum12_len = expansion_sum(term1_len, term1, term2_len, term2, sum12);
    const int hlen = expansion_sum(sum12_len, sum12, term3_len, term3, h);

    T compressed[64];
    const int compressed_len = compress(hlen, h, compressed);
    std::copy(compressed, compressed + compressed_len, h);
    return compressed_len;
}

template <class T>
int det4_expansion(const T m[4][4], T *h){
    T term1[64];
    int term1_len;
    {
        const T minor[3][3] = {
            { m[1][1], m[1][2], m[1][3] },
            { m[2][1], m[2][2], m[2][3] },
            { m[3][1], m[3][2], m[3][3] }
        };
        T det_minor[64];
        const int det_minor_len = det3_expansion(minor, det_minor);
        term1_len = scale_expansion(det_minor_len, det_minor, m[0][0], term1);
    }

    T term2[64];
    int term2_len;
    {
        const T minor[3][3] = {
            { m[1][0], m[1][2], m[1][3] },
            { m[2][0], m[2][2], m[2][3] },
            { m[3][0], m[3][2], m[3][3] }
        };
        T det_minor[64];
        const int det_minor_len = det3_expansion(minor, det_minor);
        T neg_det_minor[64];
        negate_expansion(det_minor_len, det_minor, neg_det_minor);
        term2_len = scale_expansion(det_minor_len, neg_det_minor, m[0][1], term2);
    }

    T term3[64];
    int term3_len;
    {
        const T minor[3][3] = {
            { m[1][0], m[1][1], m[1][3] },
            { m[2][0], m[2][1], m[2][3] },
            { m[3][0], m[3][1], m[3][3] }
        };
        T det_minor[64];
        const int det_minor_len = det3_expansion(minor, det_minor);
        term3_len = scale_expansion(det_minor_len, det_minor, m[0][2], term3);
    }

    T term4[64];
    int term4_len;
    {
        const T minor[3][3] = {
            { m[1][0], m[1][1], m[1][2] },
            { m[2][0], m[2][1], m[2][2] },
            { m[3][0], m[3][1], m[3][2] }
        };
        T det_minor[64];
        const int det_minor_len = det3_expansion(minor, det_minor);
        T neg_det_minor[64];
        negate_expansion(det_minor_len, det_minor, neg_det_minor);
        term4_len = scale_expansion(det_minor_len, neg_det_minor, m[0][3], term4);
    }

    T sum12[128];
    const int sum12_len = expansion_sum(term1_len, term1, term2_len, term2, sum12);
    T sum123[192];
    const int sum123_len = expansion_sum(sum12_len, sum12, term3_len, term3, sum123);
    const int hlen = expansion_sum(sum123_len, sum123, term4_len, term4, h);

    T compressed[256];
    const int compressed_len = compress(hlen, h, compressed);
    std::copy(compressed, compressed + compressed_len, h);
    return compressed_len;
}

template <class T>
T det3_fast(const T m[3][3]){
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1])
         - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
         + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

template <class T>
T det4_fast(const T m[4][4]){
    const T minor1[3][3] = {
        { m[1][1], m[1][2], m[1][3] },
        { m[2][1], m[2][2], m[2][3] },
        { m[3][1], m[3][2], m[3][3] }
    };
    const T minor2[3][3] = {
        { m[1][0], m[1][2], m[1][3] },
        { m[2][0], m[2][2], m[2][3] },
        { m[3][0], m[3][2], m[3][3] }
    };
    const T minor3[3][3] = {
        { m[1][0], m[1][1], m[1][3] },
        { m[2][0], m[2][1], m[2][3] },
        { m[3][0], m[3][1], m[3][3] }
    };
    const T minor4[3][3] = {
        { m[1][0], m[1][1], m[1][2] },
        { m[2][0], m[2][1], m[2][2] },
        { m[3][0], m[3][1], m[3][2] }
    };
    return m[0][0] * det3_fast(minor1)
         - m[0][1] * det3_fast(minor2)
         + m[0][2] * det3_fast(minor3)
         - m[0][3] * det3_fast(minor4);
}

} // namespace detail

template <class T>
void two_product(T a, T b, T &hi, T &lo){
    // Purpose: exactly decompose a floating-point product into a rounded term hi
    // and a correction term lo. The arguments a and b are the multiplicands.
    // This is the fundamental primitive used when exact 2x2, 3x3, and 4x4
    // determinant expansions are assembled for orientation and insphere tests.
    //
    // Logic: split each input into high/low limbs, compute the rounded product,
    // and then recover the discarded roundoff by subtracting the products of the
    // limb combinations exactly as described by Shewchuk (1996).
    hi = a * b;
    T ahi, alo, bhi, blo;
    detail::split(a, ahi, alo);
    detail::split(b, bhi, blo);
    T err1 = hi - (ahi * bhi);
    T err2 = err1 - (alo * bhi);
    T err3 = err2 - (ahi * blo);
    lo = (alo * blo) - err3;
}

template <class T>
void two_sum(T a, T b, T &hi, T &lo){
    // Purpose: exactly decompose a floating-point sum into hi + lo.
    // The arguments a and b are the two scalar addends, hi receives the rounded
    // sum, and lo receives the exact residual. This is useful whenever two exact
    // expansion terms must be merged without dropping roundoff.
    //
    // Logic: reconstruct which portions of the inputs survived into hi, then
    // recover the lost low-order bits as the residual lo.
    hi = a + b;
    T bvirt = hi - a;
    T avirt = hi - bvirt;
    T bround = b - bvirt;
    T around = a - avirt;
    lo = around + bround;
}

template <class T>
int grow_expansion(int elen, const T *e, T b, T *h){
    // Purpose: append the scalar b to the expansion e[0:elen) and return the
    // resulting expansion length. The inputs are the source expansion e, its
    // length elen, the scalar b, and the output buffer h.
    //
    // Use this when a determinant has mostly been accumulated as an expansion
    // and one more scalar term must be inserted exactly.
    //
    // Logic: feed the scalar through the existing expansion one term at a time,
    // preserving the roundoff residue from every exact two_sum.
    T Q = b;
    for(int i = 0; i < elen; ++i){
        T Qnew, hh;
        two_sum(Q, e[i], Qnew, hh);
        h[i] = hh;
        Q = Qnew;
    }
    h[elen] = Q;
    return elen + 1;
}

template <class T>
int expansion_sum(int elen, const T *e, int flen, const T *f, T *h){
    // Purpose: form the exact sum of expansions e[0:elen) and f[0:flen).
    // The arguments are the two input expansions, their lengths, and the output
    // buffer h. This is useful when exact lower-dimensional minors are combined
    // into a larger determinant expansion.
    //
    // Logic: start from one expansion and insert every term from the other with
    // repeated exact summation so each roundoff residue is preserved.
    for(int i = 0; i < elen; ++i) h[i] = e[i];
    int hlen = elen;
    for(int i = 0; i < flen; ++i){
        T Q = f[i];
        for(int j = 0; j < hlen; ++j){
            T Qnew, hh;
            two_sum(Q, h[j], Qnew, hh);
            h[j] = hh;
            Q = Qnew;
        }
        h[hlen] = Q;
        ++hlen;
    }
    return hlen;
}

template <class T>
int scale_expansion(int elen, const T *e, T b, T *h){
    // Purpose: multiply the expansion e[0:elen) by the scalar b.
    // The arguments are the source expansion e, its length elen, the scaling
    // factor b, and the output buffer h. This is useful when a determinant
    // cofactor scales an exact minor, such as in Laplace expansion.
    //
    // Logic: compute each product exactly with two_product, then fold the
    // residuals back together with exact summation so the final expansion
    // still represents the exact mathematical product.
    T bhi, blo;
    detail::split(b, bhi, blo);
    int hlen = 0;

    T Q, hh;
    two_product(e[0], b, Q, hh);
    if(hh != static_cast<T>(0)){
        h[hlen++] = hh;
    }
    for(int i = 1; i < elen; ++i){
        T Ti, ti;
        two_product(e[i], b, Ti, ti);
        T Qnew, hh2;
        two_sum(Q, ti, Qnew, hh2);
        if(hh2 != static_cast<T>(0)){
            h[hlen++] = hh2;
        }
        T Qnew2, hh3;
        two_sum(Qnew, Ti, Qnew2, hh3);
        if(hh3 != static_cast<T>(0)){
            h[hlen++] = hh3;
        }
        Q = Qnew2;
    }
    if(Q != static_cast<T>(0) || hlen == 0){
        h[hlen++] = Q;
    }
    return hlen;
}

template <class T>
int compress(int elen, const T *e, T *h){
    // Purpose: remove exact zero terms from the expansion e[0:elen).
    // The arguments are the source expansion e, its length elen, and the output
    // buffer h. This is useful immediately before reading the most significant
    // term to determine the sign of an exact determinant.
    //
    // Logic: copy only non-zero components; if every component is zero, keep a
    // single zero so callers still receive a valid one-term expansion.
    int hlen = 0;
    for(int i = 0; i < elen; ++i){
        if(e[i] != static_cast<T>(0)){
            h[hlen++] = e[i];
        }
    }
    if(hlen == 0){
        h[0] = static_cast<T>(0);
        return 1;
    }
    return hlen;
}

template <class T>
T estimate(int elen, const T *e){
    // Purpose: quickly estimate the value of the expansion e[0:elen).
    // The arguments are the expansion length elen and the expansion terms e.
    // This is useful for inexpensive magnitude estimates, debugging, or other
    // situations where the exact expansion is already available but only a fast
    // scalar approximation is needed.
    //
    // Logic: sum the components directly; the result is not exact, but it is
    // cheap and often sufficient for approximate magnitude comparisons.
    T Q = static_cast<T>(0);
    for(int i = 0; i < elen; ++i){
        Q += e[i];
    }
    return Q;
}

template <class T>
T orient3d_adaptive(const T *pa, const T *pb, const T *pc, const T *pd){
    // Purpose: evaluate the signed 3D orientation determinant exactly enough to
    // remain trustworthy even for nearly coplanar inputs. The arguments pa, pb,
    // pc, and pd point to three-coordinate arrays representing four 3D points.
    // This routine is useful in convex hull construction, tetrahedral quality
    // checks, and any geometric predicate where coplanarity must be detected
    // robustly rather than by heuristic tolerances.
    //
    // Logic: translate the determinant so pd is at the origin, form the three
    // 2x2 minors of the projected coordinates exactly, scale those minors by the
    // remaining z-components, and then add the resulting expansions. This is the
    // standard expansion-based evaluation from Shewchuk (1996).
    T adx = pa[0] - pd[0];
    T ady = pa[1] - pd[1];
    T adz = pa[2] - pd[2];
    T bdx = pb[0] - pd[0];
    T bdy = pb[1] - pd[1];
    T bdz = pb[2] - pd[2];
    T cdx = pc[0] - pd[0];
    T cdy = pc[1] - pd[1];
    T cdz = pc[2] - pd[2];

    T bdxcdy_hi, bdxcdy_lo;
    two_product(bdx, cdy, bdxcdy_hi, bdxcdy_lo);
    T cdxbdy_hi, cdxbdy_lo;
    two_product(cdx, bdy, cdxbdy_hi, cdxbdy_lo);

    T cdxady_hi, cdxady_lo;
    two_product(cdx, ady, cdxady_hi, cdxady_lo);
    T adxcdy_hi, adxcdy_lo;
    two_product(adx, cdy, adxcdy_hi, adxcdy_lo);

    T adxbdy_hi, adxbdy_lo;
    two_product(adx, bdy, adxbdy_hi, adxbdy_lo);
    T bdxady_hi, bdxady_lo;
    two_product(bdx, ady, bdxady_hi, bdxady_lo);

    T bc[4];
    T bc_tmp[4];
    bc_tmp[0] = bdxcdy_lo;
    bc_tmp[1] = bdxcdy_hi;
    T neg_cdxbdy[2] = { -cdxbdy_lo, -cdxbdy_hi };
    int bclen = expansion_sum(2, bc_tmp, 2, neg_cdxbdy, bc);

    T ca[4];
    T ca_tmp[4];
    ca_tmp[0] = cdxady_lo;
    ca_tmp[1] = cdxady_hi;
    T neg_adxcdy[2] = { -adxcdy_lo, -adxcdy_hi };
    int calen = expansion_sum(2, ca_tmp, 2, neg_adxcdy, ca);

    T ab[4];
    T ab_tmp[4];
    ab_tmp[0] = adxbdy_lo;
    ab_tmp[1] = adxbdy_hi;
    T neg_bdxady[2] = { -bdxady_lo, -bdxady_hi };
    int ablen = expansion_sum(2, ab_tmp, 2, neg_bdxady, ab);

    T aterm[16];
    int atermlen = scale_expansion(bclen, bc, adz, aterm);

    T bterm[16];
    int btermlen = scale_expansion(calen, ca, bdz, bterm);

    T cterm[16];
    int ctermlen = scale_expansion(ablen, ab, cdz, cterm);

    T sum1[32];
    int sum1len = expansion_sum(atermlen, aterm, btermlen, bterm, sum1);

    T result[64];
    int resultlen = expansion_sum(sum1len, sum1, ctermlen, cterm, result);

    T compressed[64];
    const int compressed_len = compress(resultlen, result, compressed);
    return compressed[compressed_len - 1];
}

template <class T>
T orient3d(const T *pa, const T *pb, const T *pc, const T *pd){
    // Purpose: provide a fast general-purpose 3D orientation test. The point
    // arguments match orient3d_adaptive and are useful anywhere the sign of an
    // oriented tetrahedron volume is needed, such as face visibility tests.
    //
    // Logic: compute the determinant in ordinary floating point, estimate a
    // conservative roundoff envelope, and return immediately when the sign is
    // clearly reliable. Only near-degenerate cases fall back to the exact
    // expansion arithmetic in orient3d_adaptive.
    T adx = pa[0] - pd[0];
    T ady = pa[1] - pd[1];
    T adz = pa[2] - pd[2];
    T bdx = pb[0] - pd[0];
    T bdy = pb[1] - pd[1];
    T bdz = pb[2] - pd[2];
    T cdx = pc[0] - pd[0];
    T cdy = pc[1] - pd[1];
    T cdz = pc[2] - pd[2];

    T det = adx * (bdy * cdz - bdz * cdy)
          + bdx * (cdy * adz - cdz * ady)
          + cdx * (ady * bdz - adz * bdy);

    T permanent = std::abs(adx) * (std::abs(bdy * cdz) + std::abs(bdz * cdy))
                + std::abs(bdx) * (std::abs(cdy * adz) + std::abs(cdz * ady))
                + std::abs(cdx) * (std::abs(ady * bdz) + std::abs(adz * bdy));

    T eps = std::numeric_limits<T>::epsilon();
    T errbound = static_cast<T>(16) * eps * permanent;

    if(std::abs(det) > errbound){
        return det;
    }

    return orient3d_adaptive(pa, pb, pc, pd);
}

template <class T>
T insphere_adaptive(const T *pa, const T *pb, const T *pc, const T *pd, const T *pe){
    // Purpose: robustly determine the sign of the lifted 4x4 insphere determinant
    // for the five points pa, pb, pc, pd, and pe. The first four points define
    // the sphere and pe is the query point. This is useful in Delaunay
    // tetrahedralization and other circumsphere-based 3D topology tests.
    //
    // Logic: translate all coordinates so pe becomes the origin, lift each point
    // with its squared distance to pe, then evaluate the resulting 4x4 determinant
    // exactly through recursive expansion arithmetic. This mirrors the lifted
    // determinant formulation described by Shewchuk (1996).
    const T aex = pa[0] - pe[0];
    const T aey = pa[1] - pe[1];
    const T aez = pa[2] - pe[2];
    const T bex = pb[0] - pe[0];
    const T bey = pb[1] - pe[1];
    const T bez = pb[2] - pe[2];
    const T cex = pc[0] - pe[0];
    const T cey = pc[1] - pe[1];
    const T cez = pc[2] - pe[2];
    const T dex = pd[0] - pe[0];
    const T dey = pd[1] - pe[1];
    const T dez = pd[2] - pe[2];

    const T alift = aex * aex + aey * aey + aez * aez;
    const T blift = bex * bex + bey * bey + bez * bez;
    const T clift = cex * cex + cey * cey + cez * cez;
    const T dlift = dex * dex + dey * dey + dez * dez;

    const T m[4][4] = {
        { aex, aey, aez, alift },
        { bex, bey, bez, blift },
        { cex, cey, cez, clift },
        { dex, dey, dez, dlift }
    };

    T result[256];
    const int result_len = detail::det4_expansion(m, result);
    T compressed[256];
    const int compressed_len = compress(result_len, result, compressed);
    return compressed[compressed_len - 1];
}

template <class T>
T insphere(const T *pa, const T *pb, const T *pc, const T *pd, const T *pe){
    // Purpose: provide a fast general-purpose 3D insphere test. The point
    // arguments match insphere_adaptive and the routine is useful for deciding
    // whether a point lies inside a tetrahedron's circumsphere.
    //
    // Logic: evaluate the lifted determinant in native floating point first and
    // compare it with a conservative magnitude estimate. When the sign is clearly
    // separated from roundoff noise, return immediately; otherwise fall back to
    // the exact expansion-based insphere_adaptive routine.
    const T aex = pa[0] - pe[0];
    const T aey = pa[1] - pe[1];
    const T aez = pa[2] - pe[2];
    const T bex = pb[0] - pe[0];
    const T bey = pb[1] - pe[1];
    const T bez = pb[2] - pe[2];
    const T cex = pc[0] - pe[0];
    const T cey = pc[1] - pe[1];
    const T cez = pc[2] - pe[2];
    const T dex = pd[0] - pe[0];
    const T dey = pd[1] - pe[1];
    const T dez = pd[2] - pe[2];

    const T alift = aex * aex + aey * aey + aez * aez;
    const T blift = bex * bex + bey * bey + bez * bez;
    const T clift = cex * cex + cey * cey + cez * cez;
    const T dlift = dex * dex + dey * dey + dez * dez;

    const T m[4][4] = {
        { aex, aey, aez, alift },
        { bex, bey, bez, blift },
        { cex, cey, cez, clift },
        { dex, dey, dez, dlift }
    };
    const T minor1[3][3] = {
        { bey, bez, blift },
        { cey, cez, clift },
        { dey, dez, dlift }
    };
    const T minor2[3][3] = {
        { bex, bez, blift },
        { cex, cez, clift },
        { dex, dez, dlift }
    };
    const T minor3[3][3] = {
        { bex, bey, blift },
        { cex, cey, clift },
        { dex, dey, dlift }
    };
    const T minor4[3][3] = {
        { bex, bey, bez },
        { cex, cey, cez },
        { dex, dey, dez }
    };

    const T det = detail::det4_fast(m);
    const T permanent = std::abs(aex) * std::abs(detail::det3_fast(minor1))
                      + std::abs(aey) * std::abs(detail::det3_fast(minor2))
                      + std::abs(aez) * std::abs(detail::det3_fast(minor3))
                      + std::abs(alift) * std::abs(detail::det3_fast(minor4));

    const T errbound = static_cast<T>(64) * std::numeric_limits<T>::epsilon() * permanent;
    if(std::abs(det) > errbound){
        return det;
    }

    return insphere_adaptive(pa, pb, pc, pd, pe);
}

template void two_product(float, float, float&, float&);
template void two_product(double, double, double&, double&);
template void two_sum(float, float, float&, float&);
template void two_sum(double, double, double&, double&);
template int grow_expansion(int, const float*, float, float*);
template int grow_expansion(int, const double*, double, double*);
template int expansion_sum(int, const float*, int, const float*, float*);
template int expansion_sum(int, const double*, int, const double*, double*);
template int scale_expansion(int, const float*, float, float*);
template int scale_expansion(int, const double*, double, double*);
template int compress(int, const float*, float*);
template int compress(int, const double*, double*);
template float estimate(int, const float*);
template double estimate(int, const double*);
template float orient3d_adaptive(const float*, const float*, const float*, const float*);
template double orient3d_adaptive(const double*, const double*, const double*, const double*);
template float orient3d(const float*, const float*, const float*, const float*);
template double orient3d(const double*, const double*, const double*, const double*);
template float insphere_adaptive(const float*, const float*, const float*, const float*, const float*);
template double insphere_adaptive(const double*, const double*, const double*, const double*, const double*);
template float insphere(const float*, const float*, const float*, const float*, const float*);
template double insphere(const double*, const double*, const double*, const double*, const double*);

#if defined(__GNUC__)
#pragma GCC pop_options
#endif

template <>
ArbPrec orient3d_adaptive<ArbPrec>(const ArbPrec *pa, const ArbPrec *pb, const ArbPrec *pc, const ArbPrec *pd){
    const ArbPrec adx = pa[0] - pd[0];
    const ArbPrec ady = pa[1] - pd[1];
    const ArbPrec adz = pa[2] - pd[2];
    const ArbPrec bdx = pb[0] - pd[0];
    const ArbPrec bdy = pb[1] - pd[1];
    const ArbPrec bdz = pb[2] - pd[2];
    const ArbPrec cdx = pc[0] - pd[0];
    const ArbPrec cdy = pc[1] - pd[1];
    const ArbPrec cdz = pc[2] - pd[2];

    const ArbPrec m[3][3] = {
        { adx, ady, adz },
        { bdx, bdy, bdz },
        { cdx, cdy, cdz }
    };
    return detail::det3_fast(m);
}

template <>
ArbPrec orient3d<ArbPrec>(const ArbPrec *pa, const ArbPrec *pb, const ArbPrec *pc, const ArbPrec *pd){
    return orient3d_adaptive<ArbPrec>(pa, pb, pc, pd);
}

template <>
ArbPrec insphere_adaptive<ArbPrec>(const ArbPrec *pa,
                                   const ArbPrec *pb,
                                   const ArbPrec *pc,
                                   const ArbPrec *pd,
                                   const ArbPrec *pe){
    const ArbPrec aex = pa[0] - pe[0];
    const ArbPrec aey = pa[1] - pe[1];
    const ArbPrec aez = pa[2] - pe[2];
    const ArbPrec bex = pb[0] - pe[0];
    const ArbPrec bey = pb[1] - pe[1];
    const ArbPrec bez = pb[2] - pe[2];
    const ArbPrec cex = pc[0] - pe[0];
    const ArbPrec cey = pc[1] - pe[1];
    const ArbPrec cez = pc[2] - pe[2];
    const ArbPrec dex = pd[0] - pe[0];
    const ArbPrec dey = pd[1] - pe[1];
    const ArbPrec dez = pd[2] - pe[2];

    const ArbPrec m[4][4] = {
        { aex, aey, aez, aex * aex + aey * aey + aez * aez },
        { bex, bey, bez, bex * bex + bey * bey + bez * bez },
        { cex, cey, cez, cex * cex + cey * cey + cez * cez },
        { dex, dey, dez, dex * dex + dey * dey + dez * dez }
    };
    return detail::det4_fast(m);
}

template <>
ArbPrec insphere<ArbPrec>(const ArbPrec *pa,
                          const ArbPrec *pb,
                          const ArbPrec *pc,
                          const ArbPrec *pd,
                          const ArbPrec *pe){
    return insphere_adaptive<ArbPrec>(pa, pb, pc, pd, pe);
}

} // namespace adaptive_predicate
