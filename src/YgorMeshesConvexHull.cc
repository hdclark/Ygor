//YgorMeshesConvexHull.cc - Written by hal clark in 2026.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorMeshesConvexHull.h"

//#ifndef YGOR_MESHES_CONVEX_HULL_DISABLE_ALL_SPECIALIZATIONS
//    #define YGOR_MESHES_CONVEX_HULL_DISABLE_ALL_SPECIALIZATIONS
//#endif

// ============================================================================
// Adaptive-precision floating-point predicates (Shewchuk-style).
// ============================================================================

namespace adaptive_predicate {

namespace detail {

// splitter is (2^ceil(p/2) + 1) where p is the number of significand bits.
// For double (p = 53): splitter = 2^27 + 1 = 134217729.
// For float  (p = 24): splitter = 2^12 + 1 = 4097.
template <class T>
constexpr T splitter() {
    constexpr int p = std::numeric_limits<T>::digits; // significand bits
    constexpr int s = (p + 1) / 2;
    T val = static_cast<T>(1);
    for(int i = 0; i < s; ++i) val *= static_cast<T>(2);
    return val + static_cast<T>(1);
}

// Split a floating-point number into high and low parts such that
// a = ahi + alo  and  |ahi|, |alo| <= |a|.
template <class T>
inline void split(T a, T &ahi, T &alo) {
    T c = splitter<T>() * a;
    T abig = c - a;
    ahi = c - abig;
    alo = a - ahi;
}

} // namespace detail

template <class T>
void two_product(T a, T b, T &hi, T &lo) {
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
void two_sum(T a, T b, T &hi, T &lo) {
    hi = a + b;
    T bvirt = hi - a;
    T avirt = hi - bvirt;
    T bround = b - bvirt;
    T around = a - avirt;
    lo = around + bround;
}

template <class T>
int grow_expansion(int elen, const T *e, T b, T *h) {
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
int expansion_sum(int elen, const T *e, int flen, const T *f, T *h) {
    // Copy e into h.
    for(int i = 0; i < elen; ++i) h[i] = e[i];
    int hlen = elen;
    for(int i = 0; i < flen; ++i){
        // Grow h by f[i].
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
int scale_expansion(int elen, const T *e, T b, T *h) {
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
int compress(int elen, const T *e, T *h) {
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
T estimate(int elen, const T *e) {
    T Q = static_cast<T>(0);
    for(int i = 0; i < elen; ++i){
        Q += e[i];
    }
    return Q;
}

// orient3d: compute the orientation of four 3-D points.
//
// Returns the sign of the determinant:
//
//   | ax-dx  ay-dy  az-dz |
//   | bx-dx  by-dy  bz-dz |
//   | cx-dx  cy-dy  cz-dz |
//
// which equals (a-d) . ((b-d) x (c-d)).
//
// Note: this equals  -((d-a) . ((b-a) x (c-a))), so the sign is OPPOSITE
// to what one gets by dotting (d-a) with the face normal (b-a)x(c-a).
//
template <class T>
T orient3d_adaptive(const T *pa, const T *pb, const T *pc, const T *pd) {
    T adx = pa[0] - pd[0];
    T ady = pa[1] - pd[1];
    T adz = pa[2] - pd[2];
    T bdx = pb[0] - pd[0];
    T bdy = pb[1] - pd[1];
    T bdz = pb[2] - pd[2];
    T cdx = pc[0] - pd[0];
    T cdy = pc[1] - pd[1];
    T cdz = pc[2] - pd[2];

    // Compute 2x2 minors as exact two-component expansions.
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

    // bc = bdx*cdy - cdx*bdy  (4 components)
    T bc[4];
    T bc_tmp[4];
    bc_tmp[0] = bdxcdy_lo;
    bc_tmp[1] = bdxcdy_hi;
    T neg_cdxbdy[2] = { -cdxbdy_lo, -cdxbdy_hi };
    int bclen = expansion_sum(2, bc_tmp, 2, neg_cdxbdy, bc);

    // ca = cdx*ady - adx*cdy  (4 components)
    T ca[4];
    T ca_tmp[4];
    ca_tmp[0] = cdxady_lo;
    ca_tmp[1] = cdxady_hi;
    T neg_adxcdy[2] = { -adxcdy_lo, -adxcdy_hi };
    int calen = expansion_sum(2, ca_tmp, 2, neg_adxcdy, ca);

    // ab = adx*bdy - bdx*ady  (4 components)
    T ab[4];
    T ab_tmp[4];
    ab_tmp[0] = adxbdy_lo;
    ab_tmp[1] = adxbdy_hi;
    T neg_bdxady[2] = { -bdxady_lo, -bdxady_hi };
    int ablen = expansion_sum(2, ab_tmp, 2, neg_bdxady, ab);

    // det = adz * bc + bdz * ca + cdz * ab.
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

    return estimate(resultlen, result);
}

template <class T>
T orient3d(const T *pa, const T *pb, const T *pc, const T *pd) {
    // Fast path: regular floating-point determinant.
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

    // Compute error bound.
    T permanent = std::abs(adx) * (std::abs(bdy * cdz) + std::abs(bdz * cdy))
                + std::abs(bdx) * (std::abs(cdy * adz) + std::abs(cdz * ady))
                + std::abs(cdx) * (std::abs(ady * bdz) + std::abs(adz * bdy));

    T eps = std::numeric_limits<T>::epsilon();
    T errbound = static_cast<T>(16) * eps * permanent;

    if(std::abs(det) > errbound){
        return det;
    }

    // Fall back to adaptive precision.
    return orient3d_adaptive(pa, pb, pc, pd);
}

// Explicit instantiations.
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

} // namespace adaptive_predicate


// ============================================================================
// ConvexHull implementation.
// ============================================================================

template <class T>
ConvexHull<T>::ConvexHull()
    : m_eps(std::numeric_limits<T>::epsilon() * static_cast<T>(1024)),
      m_rng_state(0x12345678ABCDEF01ULL) {
}

template <class T>
T ConvexHull<T>::rng_uniform() {
    // xorshift64.
    m_rng_state ^= (m_rng_state << 13);
    m_rng_state ^= (m_rng_state >> 7);
    m_rng_state ^= (m_rng_state << 17);
    return static_cast<T>(m_rng_state & 0xFFFFFFFULL) / static_cast<T>(0x10000000ULL);
}

template <class T>
T ConvexHull<T>::orient(uint64_t a, uint64_t b, uint64_t c, uint64_t d) const {
    const T pa[3] = { m_points[a].x, m_points[a].y, m_points[a].z };
    const T pb[3] = { m_points[b].x, m_points[b].y, m_points[b].z };
    const T pc[3] = { m_points[c].x, m_points[c].y, m_points[c].z };
    const T pd[3] = { m_points[d].x, m_points[d].y, m_points[d].z };
    return adaptive_predicate::orient3d(pa, pb, pc, pd);
}

template <class T>
void ConvexHull<T>::register_face_edges(uint64_t face_idx) {
    const auto &f = m_faces[face_idx];
    for(int i = 0; i < 3; ++i){
        uint64_t a = f.verts[i];
        uint64_t b = f.verts[(i + 1) % 3];
        m_edge_to_face[{a, b}] = face_idx;
    }
}

template <class T>
void ConvexHull<T>::unregister_face_edges(uint64_t face_idx) {
    const auto &f = m_faces[face_idx];
    for(int i = 0; i < 3; ++i){
        uint64_t a = f.verts[i];
        uint64_t b = f.verts[(i + 1) % 3];
        m_edge_to_face.erase({a, b});
    }
}

template <class T>
uint64_t ConvexHull<T>::build_initial_simplex() {
    const uint64_t N = m_points.size();
    if(N < 4){
        throw std::runtime_error("ConvexHull: need at least 4 points.");
    }

    // Find the first two distinct points.
    uint64_t i0 = 0;
    uint64_t i1 = 1;
    while(i1 < N && m_points[i1].sq_dist(m_points[i0]) < m_eps * m_eps){
        ++i1;
    }
    if(i1 >= N){
        throw std::runtime_error("ConvexHull: all points are coincident.");
    }

    // Find the first point not collinear with i0, i1.
    uint64_t i2 = i1 + 1;
    while(i2 < N){
        auto cross = (m_points[i1] - m_points[i0]).Cross(m_points[i2] - m_points[i0]);
        if(cross.sq_length() > m_eps * m_eps){
            break;
        }
        ++i2;
    }
    if(i2 >= N){
        throw std::runtime_error("ConvexHull: all points are collinear.");
    }

    // Find the first point not coplanar with i0, i1, i2.
    uint64_t i3 = 0;
    {
        bool found = false;
        for(uint64_t k = 0; k < N; ++k){
            if(k == i0 || k == i1 || k == i2) continue;
            T o = orient(i0, i1, i2, k);
            if(o != static_cast<T>(0)){
                i3 = k;
                found = true;
                break;
            }
        }
        if(!found){
            // All points are coplanar.  Nudge all points by a tiny random
            // amount to break the degeneracy.
            for(uint64_t k = 0; k < N; ++k){
                m_points[k].x += (rng_uniform() - static_cast<T>(0.5)) * m_eps;
                m_points[k].y += (rng_uniform() - static_cast<T>(0.5)) * m_eps;
                m_points[k].z += (rng_uniform() - static_cast<T>(0.5)) * m_eps;
            }
            // Retry.
            for(uint64_t k = 0; k < N; ++k){
                if(k == i0 || k == i1 || k == i2) continue;
                T o = orient(i0, i1, i2, k);
                if(o != static_cast<T>(0)){
                    i3 = k;
                    found = true;
                    break;
                }
            }
            if(!found){
                throw std::runtime_error("ConvexHull: degenerate configuration "
                                         "even after perturbation.");
            }
        }
    }

    // Orient so that orient(i0,i1,i2,i3) > 0.  In our convention, all hull
    // faces are wound so that interior points lie on the positive-orient side
    // and exterior (visible) points lie on the negative-orient side.
    T o = orient(i0, i1, i2, i3);
    if(o < static_cast<T>(0)){
        std::swap(i0, i1);
    }

    m_faces.clear();
    m_edge_to_face.clear();

    auto add_face = [&](uint64_t a, uint64_t b, uint64_t c) {
        uint64_t idx = m_faces.size();
        m_faces.push_back(Face{ {a, b, c}, true });
        register_face_edges(idx);
    };

    // Four faces of the tetrahedron.  Each face is wound so that the
    // opposite vertex lies on the positive-orient side.
    add_face(i0, i1, i2);  // opposite i3
    add_face(i0, i3, i1);  // opposite i2
    add_face(i1, i3, i2);  // opposite i0
    add_face(i0, i2, i3);  // opposite i1

    return 0; // caller processes all points from 0..N
}

template <class T>
void ConvexHull<T>::incorporate_point(uint64_t idx) {
    // Determine visible faces.  A face is visible from point idx when
    // orient(face, idx) < 0 (the point is on the exterior side).
    std::vector<uint64_t> visible;
    visible.reserve(m_faces.size() / 2);
    for(uint64_t fi = 0; fi < m_faces.size(); ++fi){
        if(!m_faces[fi].alive) continue;
        const auto &f = m_faces[fi];
        T o = orient(f.verts[0], f.verts[1], f.verts[2], idx);
        if(o < static_cast<T>(0)){
            visible.push_back(fi);
        }
    }

    if(visible.empty()){
        // Point is inside or on the hull; nothing to do.
        return;
    }

    // Find horizon edges: for each directed edge (a,b) of a visible face,
    // if the adjacent face (owning the reverse edge (b,a)) is non-visible,
    // then (a,b) is a horizon edge.  New face (a, b, idx) will have
    // consistent winding with the surviving hull.
    std::vector<std::pair<uint64_t,uint64_t>> horizon;
    horizon.reserve(visible.size() * 3);

    std::set<uint64_t> visible_set(visible.begin(), visible.end());

    for(auto fi : visible){
        const auto &f = m_faces[fi];
        for(int i = 0; i < 3; ++i){
            uint64_t a = f.verts[i];
            uint64_t b = f.verts[(i + 1) % 3];
            // Check the adjacent face owning reverse edge (b,a).
            auto it = m_edge_to_face.find({b, a});
            if(it != m_edge_to_face.end() &&
               visible_set.find(it->second) == visible_set.end() &&
               m_faces[it->second].alive){
                horizon.emplace_back(a, b);
            }
        }
    }

    // Remove visible faces.
    for(auto fi : visible){
        unregister_face_edges(fi);
        m_faces[fi].alive = false;
    }

    // Create new faces connecting the horizon to the new point.
    for(const auto &[a, b] : horizon){
        uint64_t fi = m_faces.size();
        m_faces.push_back(Face{ {a, b, idx}, true });
        register_face_edges(fi);
    }

    m_mesh_dirty = true;
}

template <class T>
uint64_t ConvexHull<T>::add_vertex(const vec3<T> &v) {
    uint64_t idx = m_points.size();
    uint64_t eval_idx = m_eval_counter++;

    // Store original and perturbed copies.
    m_original.push_back(v);
    vec3<T> perturbed = v;
    perturbed.x += (rng_uniform() - static_cast<T>(0.5)) * m_eps;
    perturbed.y += (rng_uniform() - static_cast<T>(0.5)) * m_eps;
    perturbed.z += (rng_uniform() - static_cast<T>(0.5)) * m_eps;
    m_points.push_back(perturbed);

    m_mesh_dirty = true;

    if(m_points.size() < 4){
        // Not enough points to form a tetrahedron yet.
        return eval_idx;
    }

    if(m_faces.empty()){
        // Build the initial simplex from all available points.
        build_initial_simplex();

        // Identify simplex vertices.
        std::set<uint64_t> simplex_verts;
        for(const auto &f : m_faces){
            if(!f.alive) continue;
            for(auto vi : f.verts){
                simplex_verts.insert(vi);
            }
        }

        // Incorporate all non-simplex points.
        for(uint64_t i = 0; i < m_points.size(); ++i){
            if(simplex_verts.count(i)) continue;
            incorporate_point(i);
        }
    } else {
        // Hull already exists; incorporate the new point.
        incorporate_point(idx);
    }

    return eval_idx;
}

template <class T>
void ConvexHull<T>::add_vertices(const std::vector<vec3<T>> &verts) {
    for(const auto &v : verts){
        add_vertex(v);
    }
}

template <class T>
void ConvexHull<T>::rebuild_mesh() const {
    m_mesh = fv_surface_mesh<T, uint64_t>();
    m_eval_order.clear();

    // Collect alive faces and build a compact vertex index.
    std::map<uint64_t, uint64_t> old_to_new;
    std::vector<uint64_t> new_to_old;

    for(const auto &f : m_faces){
        if(!f.alive) continue;
        for(auto vi : f.verts){
            if(old_to_new.find(vi) == old_to_new.end()){
                uint64_t ni = new_to_old.size();
                old_to_new[vi] = ni;
                new_to_old.push_back(vi);
            }
        }
    }

    // Use original (unperturbed) coordinates in the output mesh.
    m_mesh.vertices.resize(new_to_old.size());
    for(uint64_t i = 0; i < new_to_old.size(); ++i){
        m_mesh.vertices[i] = m_original[new_to_old[i]];
    }

    for(const auto &f : m_faces){
        if(!f.alive) continue;
        std::vector<uint64_t> face_indices;
        face_indices.reserve(3);
        for(auto vi : f.verts){
            face_indices.push_back(old_to_new[vi]);
        }
        m_mesh.faces.push_back(std::move(face_indices));
    }

    // Build the evaluation order map: mesh vertex index -> input order.
    // Since internal index i corresponds to the i-th call to add_vertex,
    // the evaluation order is simply the internal index.
    for(uint64_t i = 0; i < new_to_old.size(); ++i){
        m_eval_order[i] = new_to_old[i];
    }

    m_mesh_dirty = false;
}

template <class T>
const fv_surface_mesh<T, uint64_t> & ConvexHull<T>::get_mesh() const {
    if(m_mesh_dirty){
        rebuild_mesh();
    }
    return m_mesh;
}

template <class T>
const std::map<uint64_t, uint64_t> & ConvexHull<T>::get_evaluation_order() const {
    if(m_mesh_dirty){
        rebuild_mesh();
    }
    return m_eval_order;
}

template <class T>
uint64_t ConvexHull<T>::num_evaluated() const {
    return m_eval_counter;
}


// ============================================================================
// Explicit template instantiations.
// ============================================================================

#ifndef YGOR_MESHES_CONVEX_HULL_DISABLE_ALL_SPECIALIZATIONS
template class ConvexHull<float>;
template class ConvexHull<double>;
#endif
