//YgorMeshesOrient.cc - Written by hal clark in 2026.

#include <list>
#include <array>
#include <set>
#include <map>
#include <queue>
#include <cmath>
#include <unordered_map>
#include <stdexcept>
#include <utility>
#include <algorithm>
#include <limits>
#include <vector>
#include <numeric>
#include <functional>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorIndex.h"
#include "YgorIndexCells.h"

#include "YgorMeshesOrient.h"

//#ifndef YGOR_MESHES_ORIENT_DISABLE_ALL_SPECIALIZATIONS
//    #define YGOR_MESHES_ORIENT_DISABLE_ALL_SPECIALIZATIONS
//#endif

namespace {

// Map each vertex to a canonical representative using spatial proximity.
template <class T, class I>
std::vector<I>
VertexRepresentativeMap(const fv_surface_mesh<T,I> &fvsm,
                        T eps){
    const auto N = fvsm.vertices.size();
    std::vector<I> out;
    out.reserve(N);

    if(!(static_cast<T>(0) < eps)){
        for(size_t i = 0UL; i < N; ++i){
            out.emplace_back(static_cast<I>(i));
        }
        return out;
    }

    cells_index<T> spatial_idx(eps);
    const auto eps_sq = eps * eps;
    for(size_t i = 0UL; i < N; ++i){
        I rep = static_cast<I>(i);
        const auto &v_i = fvsm.vertices[i];

        if(v_i.isfinite()){
            auto best_j = std::numeric_limits<size_t>::max();
            auto nearby = spatial_idx.search_radius(v_i, eps);
            for(const auto &e : nearby){
                const auto j = std::any_cast<size_t>(e.aux_data);
                const auto d = (v_i - fvsm.vertices[j]).sq_length();
                if(d <= eps_sq){
                    best_j = std::min(best_j, j);
                }
            }

            if(best_j != std::numeric_limits<size_t>::max()){
                rep = out[best_j];
            }else{
                spatial_idx.insert(v_i, i);
            }
        }

        out.emplace_back(rep);
    }
    return out;
}

// Undirected edge type and helpers.
template <class I>
using undirected_edge_t = std::pair<I,I>;

template <class I>
undirected_edge_t<I>
make_undirected_edge(I a, I b){
    return { std::min<I>(a,b), std::max<I>(a,b) };
}

// Per-face-edge record.
template <class I>
struct face_edge_ref {
    I face;
    I edge;
    I rep_A;
    I rep_B;
};

// Build edge adjacency: undirected edge -> list of face_edge_ref.
template <class T, class I>
std::map< undirected_edge_t<I>, std::vector<face_edge_ref<I>> >
BuildEdgeMap(const fv_surface_mesh<T,I> &fvsm,
             const std::vector<I> &vert_rep){
    std::map< undirected_edge_t<I>, std::vector<face_edge_ref<I>> > edge_map;

    for(size_t f_n = 0UL; f_n < fvsm.faces.size(); ++f_n){
        const auto &face = fvsm.faces[f_n];
        if(face.size() < 2UL) continue;

        for(size_t e_n = 0UL; e_n < face.size(); ++e_n){
            const auto e_m = (e_n + 1UL) % face.size();
            const auto v_A = face[e_n];
            const auto v_B = face[e_m];
            if(v_A == v_B) continue;

            const auto rep_A = vert_rep[v_A];
            const auto rep_B = vert_rep[v_B];
            if(rep_A == rep_B) continue;

            edge_map[make_undirected_edge(rep_A, rep_B)].push_back(
                { static_cast<I>(f_n), static_cast<I>(e_n), rep_A, rep_B });
        }
    }

    return edge_map;
}

// Return +1 if (a,b) matches the canonical edge key ordering, -1 otherwise.
template <class I>
int
edge_direction_for_key(const undirected_edge_t<I> &k, I a, I b){
    return ( (k.first == a) && (k.second == b) ) ? 1 : -1;
}

// Compute face normal (un-normalised) for the given face.
template <class T, class I>
vec3<T>
face_normal(const fv_surface_mesh<T,I> &fvsm,
            size_t f_idx){
    const auto &fv = fvsm.faces[f_idx];
    if(fv.size() < 3UL){
        return vec3<T>(static_cast<T>(0),
                       static_cast<T>(0),
                       static_cast<T>(0));
    }

    // Compute an area-weighted polygon normal by summing triangle-fan cross products.
    // This reduces to the original behaviour for triangular faces.
    const auto &A = fvsm.vertices[fv[0]];
    vec3<T> n(static_cast<T>(0),
              static_cast<T>(0),
              static_cast<T>(0));

    for(size_t i = 1UL; i + 1UL < fv.size(); ++i){
        const auto &B = fvsm.vertices[fv[i]];
        const auto &C = fvsm.vertices[fv[i + 1UL]];
        n += (B - A).Cross(C - A);
    }

    return n;
}

// Compute face centroid.
template <class T, class I>
vec3<T>
face_centroid(const fv_surface_mesh<T,I> &fvsm,
              size_t f_idx){
    const auto &fv = fvsm.faces[f_idx];
    vec3<T> c(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
    if(fv.empty()) return c;
    for(const auto &v : fv){
        c += fvsm.vertices[v];
    }
    return c * (static_cast<T>(1) / static_cast<T>(fv.size()));
}

// Ray–triangle intersection (Möller–Trumbore).
// Returns true if the ray from origin along dir intersects the triangle, and sets t.
template <class T>
bool
ray_triangle_intersect(const vec3<T> &origin,
                       const vec3<T> &dir,
                       const vec3<T> &v0,
                       const vec3<T> &v1,
                       const vec3<T> &v2,
                       T &t){
    const T eps = std::numeric_limits<T>::epsilon() * static_cast<T>(100);
    const auto e1 = v1 - v0;
    const auto e2 = v2 - v0;
    const auto h = dir.Cross(e2);
    const T a = e1.Dot(h);
    if(std::abs(a) < eps) return false;

    const T f = static_cast<T>(1) / a;
    const auto s = origin - v0;
    const T u = f * s.Dot(h);
    if(u < static_cast<T>(0) || u > static_cast<T>(1)) return false;

    const auto q = s.Cross(e1);
    const T v = f * dir.Dot(q);
    if(v < static_cast<T>(0) || (u + v) > static_cast<T>(1)) return false;

    t = f * e2.Dot(q);
    return (t > eps);
}

} // namespace


template <class T, class I>
bool
OrientFaces(fv_surface_mesh<T,I> &fvsm,
            T eps){

    if(fvsm.faces.empty()) return true;

    const auto N_faces = fvsm.faces.size();

    // -----------------------------------------------------------------------
    // 1. Build edge adjacency.
    // -----------------------------------------------------------------------
    const auto vert_rep = VertexRepresentativeMap(fvsm, eps);
    const auto edge_map = BuildEdgeMap(fvsm, vert_rep);

    // Identify manifold edges (shared by exactly 2 faces) and non-manifold
    // edges (shared by > 2 faces). Boundary edges (1 face) are fine.
    // Build face adjacency restricted to manifold edges.
    std::vector<std::vector<I>> face_adj(N_faces);

    // For each manifold edge connecting two faces, record whether their
    // traversal directions match or oppose.  If the two faces traverse the
    // undirected edge in the same direction they need to be on *different*
    // sides (one must be flipped relative to the other).
    std::map< std::pair<I,I>, bool > need_flip;

    for(const auto &ep : edge_map){
        if(ep.second.size() != 2UL) continue; // skip non-manifold & boundary

        const auto &a = ep.second[0UL];
        const auto &b = ep.second[1UL];

        face_adj[a.face].emplace_back(b.face);
        face_adj[b.face].emplace_back(a.face);

        const auto d_a = edge_direction_for_key(ep.first, a.rep_A, a.rep_B);
        const auto d_b = edge_direction_for_key(ep.first, b.rep_A, b.rep_B);

        // If both faces traverse the edge in the same direction relative to
        // the canonical key, they are inconsistent and one must be flipped.
        const bool require_flip = (d_a == d_b);
        need_flip[{ a.face, b.face }] = require_flip;
        need_flip[{ b.face, a.face }] = require_flip;
    }

    // -----------------------------------------------------------------------
    // 2. BFS within each manifold patch to assign consistent orientation.
    //    side[f] = 0 means keep original winding; side[f] = 1 means flip.
    // -----------------------------------------------------------------------
    std::vector<int8_t> side(N_faces, -1);    // -1 = unvisited
    std::vector<std::vector<size_t>> patches;  // list of face indices per patch

    for(size_t f_0 = 0UL; f_0 < N_faces; ++f_0){
        if(side[f_0] != -1) continue;

        side[f_0] = 0; // arbitrary seed side
        std::queue<I> q;
        q.push(static_cast<I>(f_0));
        patches.emplace_back();

        bool contradiction = false;

        while(!q.empty()){
            const auto f = q.front();
            q.pop();

            patches.back().push_back(static_cast<size_t>(f));

            for(const auto n : face_adj[f]){
                auto it = need_flip.find({ f, n });
                if(it == need_flip.end()) continue;
                const auto req = it->second;
                const auto needed_side = req ? static_cast<int8_t>(1 - side[f])
                                              : static_cast<int8_t>(side[f]);
                if(side[n] == -1){
                    side[n] = needed_side;
                    q.push(n);
                }else if(side[n] != needed_side){
                    contradiction = true;
                    // Continue BFS to assign all faces anyway.
                }
            }
        }

        if(contradiction) return false;
    }

    // -----------------------------------------------------------------------
    // 3. Bounding-box seed heuristic: For each patch, pick the face whose
    //    centroid is closest to one of the 6 axis-aligned bounding-box faces,
    //    and orient it so its normal points outward, according to that
    //    bounding-box face's normal. Then propagate that choice to the rest
    //    of the patch.
    // -----------------------------------------------------------------------
    // Compute AABB.
    vec3<T> bb_min( std::numeric_limits<T>::max(),
                    std::numeric_limits<T>::max(),
                    std::numeric_limits<T>::max());
    vec3<T> bb_max( std::numeric_limits<T>::lowest(),
                    std::numeric_limits<T>::lowest(),
                    std::numeric_limits<T>::lowest());
    for(const auto &v : fvsm.vertices){
        if(!v.isfinite()) continue;
        if(v.x < bb_min.x) bb_min.x = v.x;
        if(v.y < bb_min.y) bb_min.y = v.y;
        if(v.z < bb_min.z) bb_min.z = v.z;
        if(v.x > bb_max.x) bb_max.x = v.x;
        if(v.y > bb_max.y) bb_max.y = v.y;
        if(v.z > bb_max.z) bb_max.z = v.z;
    }

    // Six axis-aligned outward normals for the bounding box faces.
    const std::array<vec3<T>,6> bb_normals = {{
        vec3<T>(static_cast<T>(-1), static_cast<T>( 0), static_cast<T>( 0)),  // -x face
        vec3<T>(static_cast<T>( 1), static_cast<T>( 0), static_cast<T>( 0)),  // +x face
        vec3<T>(static_cast<T>( 0), static_cast<T>(-1), static_cast<T>( 0)),  // -y face
        vec3<T>(static_cast<T>( 0), static_cast<T>( 1), static_cast<T>( 0)),  // +y face
        vec3<T>(static_cast<T>( 0), static_cast<T>( 0), static_cast<T>(-1)),  // -z face
        vec3<T>(static_cast<T>( 0), static_cast<T>( 0), static_cast<T>( 1))   // +z face
    }};

    // Six bounding-box face reference points (centre of each face).
    const auto bb_cx = (bb_min.x + bb_max.x) * static_cast<T>(0.5);
    const auto bb_cy = (bb_min.y + bb_max.y) * static_cast<T>(0.5);
    const auto bb_cz = (bb_min.z + bb_max.z) * static_cast<T>(0.5);

    const std::array<vec3<T>,6> bb_face_pts = {{
        vec3<T>(bb_min.x, bb_cy,    bb_cz   ),
        vec3<T>(bb_max.x, bb_cy,    bb_cz   ),
        vec3<T>(bb_cx,    bb_min.y, bb_cz   ),
        vec3<T>(bb_cx,    bb_max.y, bb_cz   ),
        vec3<T>(bb_cx,    bb_cy,    bb_min.z),
        vec3<T>(bb_cx,    bb_cy,    bb_max.z)
    }};

    // For each patch, choose the seed face and determine whether the patch
    // needs to be globally flipped.
    std::vector<bool> patch_flip(patches.size(), false);

    for(size_t p = 0UL; p < patches.size(); ++p){
        // Find the face in this patch whose centroid is closest to a bounding-
        // box face, skipping degenerate faces.
        T best_dist = std::numeric_limits<T>::max();
        size_t best_face = patches[p].front();
        size_t best_bb = 0UL;

        for(const auto f : patches[p]){
            const auto fn = face_normal(fvsm, f);
            if(fn.sq_length() <= static_cast<T>(0)) continue; // degenerate

            const auto fc = face_centroid(fvsm, f);
            for(size_t b = 0UL; b < 6UL; ++b){
                const auto d = (fc - bb_face_pts[b]).sq_length();
                if(d < best_dist){
                    best_dist = d;
                    best_face = f;
                    best_bb = b;
                }
            }
        }

        // Determine the current effective normal after the BFS assignment.
        auto fn = face_normal(fvsm, best_face);
        if(side[best_face] == 1){
            fn = fn * static_cast<T>(-1);
        }

        // The face should point outward, i.e. in the same half-space as the
        // bounding-box outward normal.
        if(fn.Dot(bb_normals[best_bb]) < static_cast<T>(0)){
            patch_flip[p] = true;
        }
    }

    // Apply patch flips: toggle the side assignment for every face in flipped patches.
    for(size_t p = 0UL; p < patches.size(); ++p){
        if(!patch_flip[p]) continue;
        for(const auto f : patches[p]){
            side[f] = static_cast<int8_t>(1 - side[f]);
        }
    }

    // -----------------------------------------------------------------------
    // 4. Ray-casting for global consistency between disconnected patches.
    //    For each patch, cast a ray from a representative face along its
    //    (post-correction) normal. Count how many faces of *other* patches
    //    the ray intersects. If odd, the patch is likely inside-out relative
    //    to the rest of the mesh, so flip it.
    // -----------------------------------------------------------------------
    if(patches.size() > 1UL){
        // Pre-compute per-face effective normals and centroids for ray testing.
        // Also build a face-to-patch index.
        std::vector<size_t> face_patch(N_faces, 0UL);
        for(size_t p = 0UL; p < patches.size(); ++p){
            for(const auto f : patches[p]){
                face_patch[f] = p;
            }
        }

        // For each patch, pick a non-degenerate representative.
        struct patch_rep {
            size_t face;
            vec3<T> centroid;
            vec3<T> normal;
        };
        std::vector<patch_rep> reps;
        reps.reserve(patches.size());

        for(size_t p = 0UL; p < patches.size(); ++p){
            patch_rep pr;
            pr.face = patches[p].front();
            bool found = false;
            for(const auto f : patches[p]){
                auto fn = face_normal(fvsm, f);
                if(side[f] == 1) fn = fn * static_cast<T>(-1);
                if(fn.sq_length() > static_cast<T>(0)){
                    const auto len = fn.length();
                    pr.face = f;
                    pr.centroid = face_centroid(fvsm, f);
                    pr.normal = fn * (static_cast<T>(1) / len);
                    found = true;
                    break;
                }
            }
            if(!found){
                pr.centroid = face_centroid(fvsm, pr.face);
                pr.normal = vec3<T>(static_cast<T>(0),
                                     static_cast<T>(0),
                                     static_cast<T>(1));
            }
            reps.push_back(pr);
        }

        // Cast ray from each patch representative and count intersections
        // with faces of other patches.
        for(size_t p = 0UL; p < patches.size(); ++p){
            const auto &origin = reps[p].centroid;
            const auto &dir    = reps[p].normal;

            size_t hit_count = 0UL;
            for(size_t f = 0UL; f < N_faces; ++f){
                if(face_patch[f] == p) continue;

                const auto &fv = fvsm.faces[f];
                if(fv.size() < 3UL) continue;

                // Triangulate the face (fan from first vertex) for ray test.
                for(size_t i = 1UL; (i + 1UL) < fv.size(); ++i){
                    T t = static_cast<T>(0);
                    if(ray_triangle_intersect(origin, dir,
                                              fvsm.vertices[fv[0]],
                                              fvsm.vertices[fv[i]],
                                              fvsm.vertices[fv[i + 1UL]],
                                              t)){
                        ++hit_count;
                    }
                }
            }

            // Odd number of hits ⇒ the ray enters (or exits) the volume an
            // unbalanced number of times, suggesting the normal points inward.
            if((hit_count % 2UL) == 1UL){
                for(const auto f : patches[p]){
                    side[f] = static_cast<int8_t>(1 - side[f]);
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // 5. Apply the orientation decisions: reverse the winding of faces whose
    //    side == 1.
    // -----------------------------------------------------------------------
    for(size_t f = 0UL; f < N_faces; ++f){
        if(side[f] == 1){
            std::reverse(std::begin(fvsm.faces[f]), std::end(fvsm.faces[f]));
        }
    }

    return true;
}
#ifndef YGOR_MESHES_ORIENT_DISABLE_ALL_SPECIALIZATIONS
    template bool
    OrientFaces(fv_surface_mesh<float,uint32_t> &, float);
    template bool
    OrientFaces(fv_surface_mesh<float,uint64_t> &, float);

    template bool
    OrientFaces(fv_surface_mesh<double,uint32_t> &, double);
    template bool
    OrientFaces(fv_surface_mesh<double,uint64_t> &, double);
#endif
