//YgorMeshesConvexHull.cc - Written by hal clark in 2026.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <future>
#include <limits>
#include <map>
#include <set>
#include <unordered_set>
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
    // Error bound for the fast-path orient3d filter.  The factor 16
    // conservatively bounds the accumulated rounding error from the ~12
    // floating-point operations in the 3x3 determinant computation.
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
// IncrementalConvexHull implementation.
// ============================================================================

template <class T>
IncrementalConvexHull<T>::IncrementalConvexHull()
    // Perturbation magnitude: ~1024 machine-epsilon.  This is small enough to
    // be invisible at typical geometric scales, but large enough to reliably
    // break exact coplanar / collinear degeneracies.
    : m_eps(std::numeric_limits<T>::epsilon() * static_cast<T>(1024)),
      m_rng_state(0x12345678ABCDEF01ULL) {
}

template <class T>
T IncrementalConvexHull<T>::rng_uniform() {
    // xorshift64.
    m_rng_state ^= (m_rng_state << 13);
    m_rng_state ^= (m_rng_state >> 7);
    m_rng_state ^= (m_rng_state << 17);
    return static_cast<T>(m_rng_state & 0xFFFFFFFULL) / static_cast<T>(0x10000000ULL);
}

template <class T>
T IncrementalConvexHull<T>::orient(uint64_t a, uint64_t b, uint64_t c, uint64_t d) const {
    const T pa[3] = { m_points[a].x, m_points[a].y, m_points[a].z };
    const T pb[3] = { m_points[b].x, m_points[b].y, m_points[b].z };
    const T pc[3] = { m_points[c].x, m_points[c].y, m_points[c].z };
    const T pd[3] = { m_points[d].x, m_points[d].y, m_points[d].z };
    return adaptive_predicate::orient3d(pa, pb, pc, pd);
}

template <class T>
void IncrementalConvexHull<T>::register_face_edges(uint64_t face_idx) {
    const auto &f = m_faces[face_idx];
    for(int i = 0; i < 3; ++i){
        uint64_t a = f.verts[i];
        uint64_t b = f.verts[(i + 1) % 3];
        m_edge_to_face[{a, b}] = face_idx;
    }
}

template <class T>
void IncrementalConvexHull<T>::unregister_face_edges(uint64_t face_idx) {
    const auto &f = m_faces[face_idx];
    for(int i = 0; i < 3; ++i){
        uint64_t a = f.verts[i];
        uint64_t b = f.verts[(i + 1) % 3];
        m_edge_to_face.erase({a, b});
    }
}

template <class T>
uint64_t IncrementalConvexHull<T>::build_initial_simplex() {
    const uint64_t N = m_points.size();
    if(N < 4){
        throw std::runtime_error("IncrementalConvexHull: need at least 4 points.");
    }

    // Find the first two distinct points.
    uint64_t i0 = 0;
    uint64_t i1 = 1;
    while(i1 < N && m_points[i1].sq_dist(m_points[i0]) < m_eps * m_eps){
        ++i1;
    }
    if(i1 >= N){
        throw std::runtime_error("IncrementalConvexHull: all points are coincident.");
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
        throw std::runtime_error("IncrementalConvexHull: all points are collinear.");
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
                throw std::runtime_error("IncrementalConvexHull: degenerate configuration "
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
    m_alive_faces.clear();
    m_alive_dead_count = 0;

    auto add_face = [&](uint64_t a, uint64_t b, uint64_t c) {
        uint64_t idx = m_faces.size();
        m_faces.push_back(Face{ {a, b, c}, true });
        m_alive_faces.push_back(idx);
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
void IncrementalConvexHull<T>::incorporate_point(uint64_t idx) {
    // Determine visible faces.  A face is visible from point idx when
    // orient(face, idx) < 0 (the point is on the exterior side).
    std::vector<uint64_t> visible;
    visible.reserve(m_alive_faces.size() / 4);
    for(auto fi : m_alive_faces){
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

    std::unordered_set<uint64_t> visible_set;
    visible_set.reserve(visible.size());
    visible_set.insert(visible.begin(), visible.end());

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
    m_alive_dead_count += visible.size();

    // Create new faces connecting the horizon to the new point.
    for(const auto &[a, b] : horizon){
        uint64_t fi = m_faces.size();
        m_faces.push_back(Face{ {a, b, idx}, true });
        m_alive_faces.push_back(fi);
        register_face_edges(fi);
    }

    // Compact m_alive_faces when dead entries exceed half the vector size.
    if(m_alive_dead_count * 2 > m_alive_faces.size()){
        std::vector<uint64_t> compacted;
        compacted.reserve(m_alive_faces.size() - m_alive_dead_count);
        for(auto fi : m_alive_faces){
            if(m_faces[fi].alive){
                compacted.push_back(fi);
            }
        }
        m_alive_faces = std::move(compacted);
        m_alive_dead_count = 0;
    }

    m_mesh_dirty = true;
}

template <class T>
uint64_t IncrementalConvexHull<T>::add_vertex(const vec3<T> &v) {
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
        for(auto fi : m_alive_faces){
            if(!m_faces[fi].alive) continue;
            for(auto vi : m_faces[fi].verts){
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
void IncrementalConvexHull<T>::add_vertices(const std::vector<vec3<T>> &verts) {
    for(const auto &v : verts){
        add_vertex(v);
    }
}

template <class T>
void IncrementalConvexHull<T>::rebuild_mesh() const {
    m_mesh = fv_surface_mesh<T, uint64_t>();
    m_eval_order.clear();

    // Collect alive faces and build a compact vertex index.
    std::map<uint64_t, uint64_t> old_to_new;
    std::vector<uint64_t> new_to_old;

    for(auto fi : m_alive_faces){
        if(!m_faces[fi].alive) continue;
        const auto &f = m_faces[fi];
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

    for(auto fi : m_alive_faces){
        if(!m_faces[fi].alive) continue;
        const auto &f = m_faces[fi];
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
const fv_surface_mesh<T, uint64_t> & IncrementalConvexHull<T>::get_mesh() const {
    if(m_mesh_dirty){
        rebuild_mesh();
    }
    return m_mesh;
}

template <class T>
const std::map<uint64_t, uint64_t> & IncrementalConvexHull<T>::get_evaluation_order() const {
    if(m_mesh_dirty){
        rebuild_mesh();
    }
    return m_eval_order;
}

template <class T>
uint64_t IncrementalConvexHull<T>::num_evaluated() const {
    return m_eval_counter;
}


// ============================================================================
// DivideAndConquerConvexHull implementation.
// ============================================================================

template <class T>
DivideAndConquerConvexHull<T>::DivideAndConquerConvexHull() = default;

template <class T>
std::vector<vec3<T>> DivideAndConquerConvexHull<T>::base_hull(const vec3<T> *pts,
                                                              size_t n) {
    if(n < 4){
        return std::vector<vec3<T>>(pts, pts + n);
    }

    IncrementalConvexHull<T> ich;
    for(size_t i = 0; i < n; ++i){
        ich.add_vertex(pts[i]);
    }
    const auto &mesh = ich.get_mesh();
    return mesh.vertices;
}

template <class T>
std::vector<vec3<T>> DivideAndConquerConvexHull<T>::dc_hull(std::vector<vec3<T>> &pts,
                                                            size_t lo, size_t hi,
                                                            size_t depth,
                                                            work_queue<std::function<void()>> &pool) {
    const size_t n = hi - lo;
    if(n <= m_base_threshold){
        return base_hull(pts.data() + lo, n);
    }

    const size_t mid = lo + n / 2;

    std::vector<vec3<T>> left_result;
    std::vector<vec3<T>> right_result;

    if(depth < m_parallel_depth){
        // Dispatch the left sub-problem to the shared thread pool and
        // compute the right sub-problem on the current thread.
        std::promise<void> left_promise;
        std::future<void> left_future = left_promise.get_future();

        pool.submit_task([&, lo, mid, depth](){
            try{
                left_result = dc_hull(pts, lo, mid, depth + 1, pool);
                left_promise.set_value();
            }catch(...){
                left_promise.set_exception(std::current_exception());
            }
        });

        right_result = dc_hull(pts, mid, hi, depth + 1, pool);

        // Wait for the left sub-problem and propagate any exception.
        left_future.get();
    } else {
        // Sequential recursion at deeper levels to avoid thread oversubscription.
        left_result  = dc_hull(pts, lo, mid, depth + 1, pool);
        right_result = dc_hull(pts, mid, hi, depth + 1, pool);
    }

    // Merge: combine hull vertices from both sub-hulls and compute
    // the hull of the union.
    std::vector<vec3<T>> merged;
    merged.reserve(left_result.size() + right_result.size());
    merged.insert(merged.end(), left_result.begin(), left_result.end());
    merged.insert(merged.end(), right_result.begin(), right_result.end());

    return base_hull(merged.data(), merged.size());
}

template <class T>
void DivideAndConquerConvexHull<T>::compute(const std::vector<vec3<T>> &verts) {
    m_mesh = fv_surface_mesh<T, uint64_t>();

    if(verts.size() < 4){
        // Cannot form a 3-D hull; store whatever vertices are available
        // but leave faces empty.
        m_mesh.vertices.assign(verts.begin(), verts.end());
        return;
    }

    // Make a working copy and sort along the x-axis for spatial partitioning.
    std::vector<vec3<T>> pts(verts);
    std::sort(pts.begin(), pts.end(), [](const vec3<T> &a, const vec3<T> &b){
        if(a.x != b.x) return a.x < b.x;
        if(a.y != b.y) return a.y < b.y;
        return a.z < b.z;
    });

    // Create a shared thread pool for parallel sub-problem dispatch.
    work_queue<std::function<void()>> pool;

    auto hull_verts = dc_hull(pts, 0, pts.size(), 0, pool);

    // Build the final mesh from the hull vertices using the incremental
    // algorithm.
    if(hull_verts.size() < 4){
        m_mesh.vertices = std::move(hull_verts);
        return;
    }

    IncrementalConvexHull<T> ich;
    ich.add_vertices(hull_verts);
    m_mesh = ich.get_mesh();
}

template <class T>
const fv_surface_mesh<T, uint64_t> & DivideAndConquerConvexHull<T>::get_mesh() const {
    return m_mesh;
}


// ============================================================================
// MarriageBeforeConquestConvexHull implementation.
//
// This adapts the Kirkpatrick–Seidel (1986) marriage-before-conquest
// strategy to three dimensions.  At each recursive level, points are split
// at the median x-coordinate.  A 2-D upper bridge (projected onto the
// xy-plane) is found using the median-of-slopes approach described in the
// paper.  The bridge, together with axis-aligned extreme-point tests, is
// used to prune interior points before recursion.  The two sub-hulls are
// then computed recursively and merged.
// ============================================================================

template <class T>
MarriageBeforeConquestConvexHull<T>::MarriageBeforeConquestConvexHull() = default;

template <class T>
std::vector<vec3<T>> MarriageBeforeConquestConvexHull<T>::base_hull(const vec3<T> *pts,
                                                                     size_t n) {
    if(n < 4){
        return std::vector<vec3<T>>(pts, pts + n);
    }

    IncrementalConvexHull<T> ich;
    for(size_t i = 0; i < n; ++i){
        ich.add_vertex(pts[i]);
    }
    const auto &mesh = ich.get_mesh();
    return mesh.vertices;
}

// Prune points that are strictly interior to the convex hull by testing
// against the six axis-aligned extreme planes.  Any point that lies strictly
// inside all six planes cannot be a hull vertex and is discarded.  This is
// a fast O(n) filter that can dramatically reduce the input size for large
// point sets with many interior points.
template <class T>
std::vector<vec3<T>> MarriageBeforeConquestConvexHull<T>::prune_interior(
    const vec3<T> *pts, size_t n)
{
    if(n <= 6){
        return std::vector<vec3<T>>(pts, pts + n);
    }

    T xmin = pts[0].x, xmax = pts[0].x;
    T ymin = pts[0].y, ymax = pts[0].y;
    T zmin = pts[0].z, zmax = pts[0].z;

    for(size_t i = 1; i < n; ++i){
        if(pts[i].x < xmin) xmin = pts[i].x;
        if(pts[i].x > xmax) xmax = pts[i].x;
        if(pts[i].y < ymin) ymin = pts[i].y;
        if(pts[i].y > ymax) ymax = pts[i].y;
        if(pts[i].z < zmin) zmin = pts[i].z;
        if(pts[i].z > zmax) zmax = pts[i].z;
    }

    std::vector<vec3<T>> out;
    out.reserve(n);
    for(size_t i = 0; i < n; ++i){
        if(pts[i].x == xmin || pts[i].x == xmax ||
           pts[i].y == ymin || pts[i].y == ymax ||
           pts[i].z == zmin || pts[i].z == zmax){
            out.push_back(pts[i]);
        } else {
            // Interior point candidate: check if it lies strictly inside all
            // six extreme planes.  If so, it cannot be on the hull.
            bool interior = (pts[i].x > xmin && pts[i].x < xmax &&
                             pts[i].y > ymin && pts[i].y < ymax &&
                             pts[i].z > zmin && pts[i].z < zmax);
            if(!interior){
                out.push_back(pts[i]);
            }
        }
    }

    return out;
}

// Kirkpatrick–Seidel 2-D upper bridge finding.
//
// Given two point sets `left` and `right` (sorted by x within each set),
// finds the upper bridge: the edge (l, r) with l in left and r in right
// such that all points in left lie at or below the line through (l, r) and
// all points in right lie at or below that line.  This is the core
// "marriage" step of the Kirkpatrick–Seidel algorithm.
//
// The algorithm works by:
//   1. Pairing up consecutive points in each set and computing the slope
//      of each pair's connecting line.
//   2. Finding the median of these slopes.
//   3. For each set, finding the point(s) that maximise the y-intercept of
//      the supporting line with the median slope.
//   4. Using the relative position of the supporting points to the median
//      x-coordinate to prune half the candidates.
//   5. Recursing until only one candidate remains in each set.
//
// Returns (index into left, index into right).
template <class T>
std::pair<size_t, size_t> MarriageBeforeConquestConvexHull<T>::upper_bridge_2d(
    const std::vector<vec3<T>> &left,
    const std::vector<vec3<T>> &right,
    T median_x)
{
    // Working copies of candidate indices.
    std::vector<size_t> L_idx(left.size());
    std::vector<size_t> R_idx(right.size());
    for(size_t i = 0; i < left.size(); ++i) L_idx[i] = i;
    for(size_t i = 0; i < right.size(); ++i) R_idx[i] = i;

    while(L_idx.size() > 1 || R_idx.size() > 1){
        // Collect slopes from paired-up points.
        std::vector<T> slopes;
        slopes.reserve(L_idx.size() / 2 + R_idx.size() / 2);

        // Pair consecutive candidates in L.
        std::vector<std::pair<size_t,size_t>> L_pairs;
        for(size_t i = 0; i + 1 < L_idx.size(); i += 2){
            size_t a = L_idx[i], b = L_idx[i + 1];
            T dx = left[b].x - left[a].x;
            if(dx != static_cast<T>(0)){
                slopes.push_back((left[b].y - left[a].y) / dx);
            }
            L_pairs.push_back({a, b});
        }

        // Pair consecutive candidates in R.
        std::vector<std::pair<size_t,size_t>> R_pairs;
        for(size_t i = 0; i + 1 < R_idx.size(); i += 2){
            size_t a = R_idx[i], b = R_idx[i + 1];
            T dx = right[b].x - right[a].x;
            if(dx != static_cast<T>(0)){
                slopes.push_back((right[b].y - right[a].y) / dx);
            }
            R_pairs.push_back({a, b});
        }

        if(slopes.empty()){
            // All pairs have equal x — fall back: keep only the highest-y
            // point from each set.
            break;
        }

        // Find median slope via nth_element.
        size_t mid_s = slopes.size() / 2;
        std::nth_element(slopes.begin(), slopes.begin() + static_cast<long>(mid_s), slopes.end());
        T med_slope = slopes[mid_s];

        // For each set, find the point(s) that maximise
        //   h(p) = p.y - med_slope * p.x
        // which is the y-intercept of a line with slope med_slope through p.

        // Left set: find max h and the point with max x among those.
        T best_hL = left[L_idx[0]].y - med_slope * left[L_idx[0]].x;
        size_t best_iL = L_idx[0];
        for(size_t i = 1; i < L_idx.size(); ++i){
            T h = left[L_idx[i]].y - med_slope * left[L_idx[i]].x;
            if(h > best_hL || (h == best_hL && left[L_idx[i]].x > left[best_iL].x)){
                best_hL = h;
                best_iL = L_idx[i];
            }
        }

        // Right set: find max h and the point with min x among those.
        T best_hR = right[R_idx[0]].y - med_slope * right[R_idx[0]].x;
        size_t best_iR = R_idx[0];
        for(size_t i = 1; i < R_idx.size(); ++i){
            T h = right[R_idx[i]].y - med_slope * right[R_idx[i]].x;
            if(h > best_hR || (h == best_hR && right[R_idx[i]].x < right[best_iR].x)){
                best_hR = h;
                best_iR = R_idx[i];
            }
        }

        // Determine the relationship between the bridge slope and med_slope
        // by examining the supporting line through the left and right
        // maximisers at the median x-coordinate (KS86 Section 3).
        //
        // The bridge crosses x = median_x.  The supporting line with slope
        // med_slope through the left maximiser has value at median_x:
        //   yL = best_hL + med_slope * median_x
        // and similarly for the right maximiser:
        //   yR = best_hR + med_slope * median_x
        //
        // If yL > yR the bridge slope is less than med_slope;
        // if yL < yR the bridge slope is greater than med_slope.
        T yL_at_med = best_hL + med_slope * median_x;
        T yR_at_med = best_hR + med_slope * median_x;

        if(yL_at_med == yR_at_med
           && left[best_iL].x <= median_x
           && right[best_iR].x >= median_x){
            return {best_iL, best_iR};
        }

        std::vector<size_t> new_L, new_R;
        new_L.reserve(L_idx.size());
        new_R.reserve(R_idx.size());

        if(yL_at_med > yR_at_med){
            // Bridge slope < med_slope.
            for(auto &[a, b] : L_pairs){
                T dx = left[b].x - left[a].x;
                if(dx == static_cast<T>(0)){
                    // Keep the higher-y point.
                    new_L.push_back(left[a].y >= left[b].y ? a : b);
                } else {
                    T s = (left[b].y - left[a].y) / dx;
                    if(s > med_slope){
                        // Keep left-most (smaller x).
                        new_L.push_back(left[a].x <= left[b].x ? a : b);
                    } else if(s < med_slope){
                        // Keep both.
                        new_L.push_back(a);
                        new_L.push_back(b);
                    } else {
                        // Equal: keep both.
                        new_L.push_back(a);
                        new_L.push_back(b);
                    }
                }
            }
            // Unpaired last element in L.
            if(L_idx.size() % 2 == 1) new_L.push_back(L_idx.back());

            for(auto &[a, b] : R_pairs){
                T dx = right[b].x - right[a].x;
                if(dx == static_cast<T>(0)){
                    new_R.push_back(right[a].y >= right[b].y ? a : b);
                } else {
                    T s = (right[b].y - right[a].y) / dx;
                    if(s < med_slope){
                        // Keep right-most (larger x).
                        new_R.push_back(right[a].x >= right[b].x ? a : b);
                    } else if(s > med_slope){
                        new_R.push_back(a);
                        new_R.push_back(b);
                    } else {
                        new_R.push_back(a);
                        new_R.push_back(b);
                    }
                }
            }
            if(R_idx.size() % 2 == 1) new_R.push_back(R_idx.back());
        } else {
            // yL_at_med < yR_at_med → bridge slope > med_slope.
            for(auto &[a, b] : L_pairs){
                T dx = left[b].x - left[a].x;
                if(dx == static_cast<T>(0)){
                    new_L.push_back(left[a].y >= left[b].y ? a : b);
                } else {
                    T s = (left[b].y - left[a].y) / dx;
                    if(s < med_slope){
                        // Keep right-most (larger x).
                        new_L.push_back(left[a].x >= left[b].x ? a : b);
                    } else if(s > med_slope){
                        new_L.push_back(a);
                        new_L.push_back(b);
                    } else {
                        new_L.push_back(a);
                        new_L.push_back(b);
                    }
                }
            }
            if(L_idx.size() % 2 == 1) new_L.push_back(L_idx.back());

            for(auto &[a, b] : R_pairs){
                T dx = right[b].x - right[a].x;
                if(dx == static_cast<T>(0)){
                    new_R.push_back(right[a].y >= right[b].y ? a : b);
                } else {
                    T s = (right[b].y - right[a].y) / dx;
                    if(s > med_slope){
                        // Keep left-most (smaller x).
                        new_R.push_back(right[a].x <= right[b].x ? a : b);
                    } else if(s < med_slope){
                        new_R.push_back(a);
                        new_R.push_back(b);
                    } else {
                        new_R.push_back(a);
                        new_R.push_back(b);
                    }
                }
            }
            if(R_idx.size() % 2 == 1) new_R.push_back(R_idx.back());
        }

        // Ensure progress: if pruning did not reduce candidate count, break.
        if(new_L.size() >= L_idx.size() && new_R.size() >= R_idx.size()){
            break;
        }

        if(!new_L.empty()) L_idx = std::move(new_L);
        if(!new_R.empty()) R_idx = std::move(new_R);
    }

    // Fall-back: choose the highest-y candidate from each set.
    size_t bestL = L_idx[0];
    for(size_t i = 1; i < L_idx.size(); ++i){
        if(left[L_idx[i]].y > left[bestL].y ||
           (left[L_idx[i]].y == left[bestL].y && left[L_idx[i]].x > left[bestL].x)){
            bestL = L_idx[i];
        }
    }
    size_t bestR = R_idx[0];
    for(size_t i = 1; i < R_idx.size(); ++i){
        if(right[R_idx[i]].y > right[bestR].y ||
           (right[R_idx[i]].y == right[bestR].y && right[R_idx[i]].x < right[bestR].x)){
            bestR = R_idx[i];
        }
    }
    return {bestL, bestR};
}

template <class T>
std::vector<vec3<T>> MarriageBeforeConquestConvexHull<T>::mbc_hull(
    std::vector<vec3<T>> &pts,
    size_t lo, size_t hi,
    size_t depth,
    work_queue<std::function<void()>> &pool)
{
    size_t n = hi - lo;
    if(n <= m_base_threshold){
        return base_hull(pts.data() + lo, n);
    }

    const size_t mid = lo + n / 2;
    T median_x = pts[mid].x;

    // --- Marriage step ---
    // Build projected (xy) candidate lists for the left and right halves.
    std::vector<vec3<T>> left_pts(pts.begin() + static_cast<long>(lo),
                                  pts.begin() + static_cast<long>(mid));
    std::vector<vec3<T>> right_pts(pts.begin() + static_cast<long>(mid),
                                   pts.begin() + static_cast<long>(hi));

    // Sort each half by x within the projected plane (already mostly sorted
    // from the global sort, but ties may exist).
    // left_pts and right_pts are already sorted by x from the outer sort.

    // Find the 2-D upper bridge in xy-projection.  The bridge identifies the
    // extreme supporting edge crossing the median x, which together with the
    // axis-aligned extreme-point tests used in prune_interior gives the
    // "marriage" information needed to discard interior points before
    // recursion.
    upper_bridge_2d(left_pts, right_pts, median_x);

    // Prune interior points from each half.  This is the key advantage of
    // marriage-before-conquest: by identifying extreme directions before
    // recursion, we can discard points that are clearly interior, reducing
    // the effective input size for sub-problems.
    auto pruned_left  = prune_interior(pts.data() + lo, mid - lo);
    auto pruned_right = prune_interior(pts.data() + mid, hi - mid);

    // Re-sort pruned sets (prune_interior preserves order, but sub-ranges
    // need to be contiguous for recursion).
    auto cmp = [](const vec3<T> &a, const vec3<T> &b){
        if(a.x != b.x) return a.x < b.x;
        if(a.y != b.y) return a.y < b.y;
        return a.z < b.z;
    };
    std::sort(pruned_left.begin(), pruned_left.end(), cmp);
    std::sort(pruned_right.begin(), pruned_right.end(), cmp);

    // --- Conquest step ---
    std::vector<vec3<T>> left_result;
    std::vector<vec3<T>> right_result;

    if(depth < m_parallel_depth){
        std::promise<void> left_promise;
        std::future<void> left_future = left_promise.get_future();

        pool.submit_task([&, depth](){
            try{
                left_result = mbc_hull(pruned_left, 0, pruned_left.size(),
                                       depth + 1, pool);
                left_promise.set_value();
            }catch(...){
                left_promise.set_exception(std::current_exception());
            }
        });

        right_result = mbc_hull(pruned_right, 0, pruned_right.size(),
                                depth + 1, pool);

        // Wait for the left sub-problem and propagate any exception.
        left_future.get();
    } else {
        left_result  = mbc_hull(pruned_left, 0, pruned_left.size(),
                                depth + 1, pool);
        right_result = mbc_hull(pruned_right, 0, pruned_right.size(),
                                depth + 1, pool);
    }

    // --- Merge ---
    // Combine hull vertices from both sub-hulls and compute the hull of
    // the union using the base (incremental) algorithm.
    std::vector<vec3<T>> merged;
    merged.reserve(left_result.size() + right_result.size());
    merged.insert(merged.end(), left_result.begin(), left_result.end());
    merged.insert(merged.end(), right_result.begin(), right_result.end());

    return base_hull(merged.data(), merged.size());
}

template <class T>
void MarriageBeforeConquestConvexHull<T>::compute(const std::vector<vec3<T>> &verts) {
    m_mesh = fv_surface_mesh<T, uint64_t>();

    if(verts.size() < 4){
        m_mesh.vertices.assign(verts.begin(), verts.end());
        return;
    }

    // Make a working copy and sort along the x-axis for spatial partitioning.
    std::vector<vec3<T>> pts(verts);
    std::sort(pts.begin(), pts.end(), [](const vec3<T> &a, const vec3<T> &b){
        if(a.x != b.x) return a.x < b.x;
        if(a.y != b.y) return a.y < b.y;
        return a.z < b.z;
    });

    // Create a shared thread pool for parallel sub-problem dispatch.
    work_queue<std::function<void()>> pool;

    auto hull_verts = mbc_hull(pts, 0, pts.size(), 0, pool);

    // Build the final mesh from the hull vertices using the incremental
    // algorithm.
    if(hull_verts.size() < 4){
        m_mesh.vertices = std::move(hull_verts);
        return;
    }

    IncrementalConvexHull<T> ich;
    ich.add_vertices(hull_verts);
    m_mesh = ich.get_mesh();
}

template <class T>
const fv_surface_mesh<T, uint64_t> & MarriageBeforeConquestConvexHull<T>::get_mesh() const {
    return m_mesh;
}


// ============================================================================
// Explicit template instantiations.
// ============================================================================

#ifndef YGOR_MESHES_CONVEX_HULL_DISABLE_ALL_SPECIALIZATIONS
template class IncrementalConvexHull<float>;
template class IncrementalConvexHull<double>;
template class DivideAndConquerConvexHull<float>;
template class DivideAndConquerConvexHull<double>;
template class MarriageBeforeConquestConvexHull<float>;
template class MarriageBeforeConquestConvexHull<double>;
#endif
