//YgorMeshesAdaptivePredicates.cc - Written by hal clark in 2026.

#include <cmath>
#include <limits>

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

// Split a floating-point number into high and low parts such that
// a = ahi + alo  and  |ahi|, |alo| <= |a|.
template <class T>
inline void split(T a, T &ahi, T &alo){
    T c = splitter<T>() * a;
    T abig = c - a;
    ahi = c - abig;
    alo = a - ahi;
}

} // namespace detail

template <class T>
void two_product(T a, T b, T &hi, T &lo){
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
    hi = a + b;
    T bvirt = hi - a;
    T avirt = hi - bvirt;
    T bround = b - bvirt;
    T around = a - avirt;
    lo = around + bround;
}

template <class T>
int grow_expansion(int elen, const T *e, T b, T *h){
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
    T Q = static_cast<T>(0);
    for(int i = 0; i < elen; ++i){
        Q += e[i];
    }
    return Q;
}

template <class T>
T orient3d_adaptive(const T *pa, const T *pb, const T *pc, const T *pd){
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

#if defined(__GNUC__)
#pragma GCC pop_options
#endif

} // namespace adaptive_predicate
