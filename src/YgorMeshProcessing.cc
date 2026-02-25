//YgorMeshProcessing.cc - Written by hal clark in 2026.
//
// Surface mesh processing routines.
//

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

#include "YgorDefinitions.h"
#include "YgorMath.h"

#include "YgorMeshProcessing.h"

namespace {

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

    struct cell_t {
        int64_t x;
        int64_t y;
        int64_t z;
        bool operator==(const cell_t &rhs) const {
            return (x == rhs.x) && (y == rhs.y) && (z == rhs.z);
        }
    };
    struct cell_hash_t {
        std::size_t operator()(const cell_t &c) const {
            const auto h0 = std::hash<int64_t>{}(c.x);
            const auto h1 = std::hash<int64_t>{}(c.y);
            const auto h2 = std::hash<int64_t>{}(c.z);
            return h0 ^ (h1 << 1U) ^ (h2 << 2U);
        }
    };
    const auto make_cell = [&](const vec3<T> &v) -> cell_t {
        const auto inv_eps = static_cast<T>(1) / eps;
        return { static_cast<int64_t>(std::floor(v.x * inv_eps)),
                 static_cast<int64_t>(std::floor(v.y * inv_eps)),
                 static_cast<int64_t>(std::floor(v.z * inv_eps)) };
    };

    std::unordered_map<cell_t, std::vector<I>, cell_hash_t> bins;

    const auto eps_sq = eps * eps;
    for(size_t i = 0UL; i < N; ++i){
        I rep = static_cast<I>(i);
        const auto &v_i = fvsm.vertices[i];

        if(v_i.isfinite()){
            const auto c_i = make_cell(v_i);
            auto best_j = std::numeric_limits<size_t>::max();

            for(int64_t dx = -1L; dx <= 1L; ++dx){
                for(int64_t dy = -1L; dy <= 1L; ++dy){
                    for(int64_t dz = -1L; dz <= 1L; ++dz){
                        const cell_t c_n = { c_i.x + dx, c_i.y + dy, c_i.z + dz };
                        const auto it = bins.find(c_n);
                        if(it == std::end(bins)) continue;

                        for(const auto j_i : it->second){
                            const auto j = static_cast<size_t>(j_i);
                            const auto d = (v_i - fvsm.vertices[j]).sq_length();
                            if(d <= eps_sq){
                                best_j = std::min(best_j, j);
                            }
                        }
                    }
                }
            }

            if(best_j != std::numeric_limits<size_t>::max()){
                rep = out[best_j];
            }else{
                bins[c_i].emplace_back(static_cast<I>(i));
            }
        }

        out.emplace_back(rep);
    }
    return out;
}

template <class I>
struct face_edge_ref {
    I face;
    I edge;
    I vert_A;
    I vert_B;
    I rep_A;
    I rep_B;
};

template <class I>
using undirected_edge_t = std::pair<I,I>;

template <class I>
undirected_edge_t<I>
make_undirected_edge(I a, I b){
    return { std::min<I>(a,b), std::max<I>(a,b) };
}

template <class I>
int
edge_direction_for_key(const undirected_edge_t<I> &k,
                       I a,
                       I b){
    return ( (k.first == a) && (k.second == b) ) ? 1 : -1;
}

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

            edge_map[make_undirected_edge(rep_A, rep_B)].push_back({ static_cast<I>(f_n),
                                                                      static_cast<I>(e_n),
                                                                      v_A,
                                                                      v_B,
                                                                      rep_A,
                                                                      rep_B });
        }
    }

    return edge_map;
}

} // namespace


template <class T, class I>
fv_surface_mesh_hole_chains<I>
FindBoundaryChains(const fv_surface_mesh<T,I> &fvsm,
                   T eps){
    fv_surface_mesh_hole_chains<I> out;

    if(fvsm.vertices.empty() || fvsm.faces.empty()) return out;

    const auto vert_rep = VertexRepresentativeMap(fvsm, eps);
    const auto edge_map = BuildEdgeMap(fvsm, vert_rep);

    std::vector<face_edge_ref<I>> boundary_edges;
    for(const auto &ep : edge_map){
        if(ep.second.size() == 1UL){
            boundary_edges.emplace_back(ep.second.front());
        }else if(2UL < ep.second.size()){
            out.has_nonmanifold_edges = true;
        }
    }

    if(boundary_edges.empty()) return out;

    std::multimap<I,size_t> boundary_starts;
    for(size_t i = 0UL; i < boundary_edges.size(); ++i){
        boundary_starts.insert({ boundary_edges[i].rep_A, i });
    }

    std::vector<bool> used(boundary_edges.size(), false);
    for(size_t i = 0UL; i < boundary_edges.size(); ++i){
        if(used[i]) continue;

        fv_surface_mesh_hole_chain<I> chain;

        size_t curr_i = i;
        const auto start_rep = boundary_edges[curr_i].rep_A;
        while(!used[curr_i]){
            used[curr_i] = true;
            const auto &curr = boundary_edges[curr_i];

            chain.vertices.emplace_back(curr.vert_A);
            chain.faces.emplace_back(curr.face);
            chain.face_edges.emplace_back(curr.edge);

            const auto next_rep = curr.rep_B;
            if(next_rep == start_rep){
                chain.is_closed = true;
                break;
            }

            const auto eq = boundary_starts.equal_range(next_rep);
            size_t candidate_i = std::numeric_limits<size_t>::max();
            size_t n_open = 0UL;
            for(auto it = eq.first; it != eq.second; ++it){
                if(!used[it->second]){
                    candidate_i = it->second;
                    ++n_open;
                }
            }
            if(n_open == 0UL){
                break;
            }
            if(1UL < n_open) out.has_ambiguous_boundary = true;

            curr_i = candidate_i;
        }

        out.chains.emplace_back(chain);
    }

    return out;
}
#ifndef YGORMESHPROCESSING_DISABLE_ALL_SPECIALIZATIONS
    template fv_surface_mesh_hole_chains<uint32_t>
    FindBoundaryChains(const fv_surface_mesh<float,uint32_t> &, float);
    template fv_surface_mesh_hole_chains<uint64_t>
    FindBoundaryChains(const fv_surface_mesh<float,uint64_t> &, float);

    template fv_surface_mesh_hole_chains<uint32_t>
    FindBoundaryChains(const fv_surface_mesh<double,uint32_t> &, double);
    template fv_surface_mesh_hole_chains<uint64_t>
    FindBoundaryChains(const fv_surface_mesh<double,uint64_t> &, double);
#endif


template <class T, class I>
bool
FillBoundaryChainsByZippering(fv_surface_mesh<T,I> &fvsm,
                              const fv_surface_mesh_hole_chains<I> &holes,
                              T eps){
    if(holes.has_nonmanifold_edges) return false;

    const auto eps_sq = eps * eps;
    bool made_changes = false;

    for(const auto &chain : holes.chains){
        if(!chain.is_closed) continue;
        if(chain.vertices.size() < 3UL) continue;

        const auto v_0 = chain.vertices.front();
        if(fvsm.vertices.size() <= static_cast<size_t>(v_0)) return false;

        for(size_t i = 1UL; (i + 1UL) < chain.vertices.size(); ++i){
            const auto v_1 = chain.vertices[i];
            const auto v_2 = chain.vertices[i + 1UL];

            if( (fvsm.vertices.size() <= static_cast<size_t>(v_1))
            ||  (fvsm.vertices.size() <= static_cast<size_t>(v_2)) ) return false;

            if((v_0 == v_1) || (v_0 == v_2) || (v_1 == v_2)) continue;

            const auto s_01 = (fvsm.vertices[v_0] - fvsm.vertices[v_1]).sq_length();
            const auto s_02 = (fvsm.vertices[v_0] - fvsm.vertices[v_2]).sq_length();
            const auto s_12 = (fvsm.vertices[v_1] - fvsm.vertices[v_2]).sq_length();
            if((s_01 <= eps_sq) || (s_02 <= eps_sq) || (s_12 <= eps_sq)) continue;

            fvsm.faces.emplace_back(std::vector<I>{ v_0, v_1, v_2 });
            made_changes = true;
        }
    }

    if(made_changes){
        fvsm.recreate_involved_face_index();
    }

    return true;
}
#ifndef YGORMESHPROCESSING_DISABLE_ALL_SPECIALIZATIONS
    template bool
    FillBoundaryChainsByZippering(fv_surface_mesh<float,uint32_t> &, const fv_surface_mesh_hole_chains<uint32_t> &, float);
    template bool
    FillBoundaryChainsByZippering(fv_surface_mesh<float,uint64_t> &, const fv_surface_mesh_hole_chains<uint64_t> &, float);

    template bool
    FillBoundaryChainsByZippering(fv_surface_mesh<double,uint32_t> &, const fv_surface_mesh_hole_chains<uint32_t> &, double);
    template bool
    FillBoundaryChainsByZippering(fv_surface_mesh<double,uint64_t> &, const fv_surface_mesh_hole_chains<uint64_t> &, double);
#endif


template <class T, class I>
bool
EnsureConsistentFaceOrientation(fv_surface_mesh<T,I> &fvsm,
                                T eps,
                                int64_t *genus){
    if(genus != nullptr) *genus = -1;

    if(fvsm.faces.empty()) return true;

    const auto vert_rep = VertexRepresentativeMap(fvsm, eps);
    const auto edge_map = BuildEdgeMap(fvsm, vert_rep);

    for(const auto &ep : edge_map){
        if(2UL < ep.second.size()) return false;
    }

    const auto N_faces = fvsm.faces.size();
    std::vector<int8_t> side(N_faces, -1);

    std::vector<std::vector<I>> face_adj(N_faces);
    std::map< std::pair<I,I>, bool > same_dir;
    for(const auto &ep : edge_map){
        if(ep.second.size() != 2UL) continue;

        const auto &a = ep.second[0UL];
        const auto &b = ep.second[1UL];

        face_adj[a.face].emplace_back(b.face);
        face_adj[b.face].emplace_back(a.face);

        const auto d_a = edge_direction_for_key(ep.first, a.rep_A, a.rep_B);
        const auto d_b = edge_direction_for_key(ep.first, b.rep_A, b.rep_B);
        const bool require_flip_mismatch = (d_a == d_b);

        same_dir[{ a.face, b.face }] = require_flip_mismatch;
        same_dir[{ b.face, a.face }] = require_flip_mismatch;
    }

    std::vector<std::set<I>> components;
    for(size_t f_0 = 0UL; f_0 < N_faces; ++f_0){
        if(side[f_0] != -1) continue;

        side[f_0] = 0;
        std::queue<I> q;
        q.push(static_cast<I>(f_0));
        components.emplace_back();

        while(!q.empty()){
            const auto f = q.front();
            q.pop();

            components.back().insert(f);

            for(const auto n : face_adj[f]){
                const auto req = same_dir[{ f, n }];
                const auto needed_side = req ? static_cast<int8_t>(1 - side[f])
                                             : static_cast<int8_t>(side[f]);
                if(side[n] == -1){
                    side[n] = needed_side;
                    q.push(n);
                }else if(side[n] != needed_side){
                    return false;
                }
            }
        }
    }

    for(size_t f = 0UL; f < N_faces; ++f){
        if(side[f] == 1){
            std::reverse(std::begin(fvsm.faces[f]), std::end(fvsm.faces[f]));
        }
    }

    if(genus != nullptr){
        int64_t genus_sum = 0;
        for(const auto &comp : components){
            std::set<I> comp_verts;
            std::set<undirected_edge_t<I>> comp_edges;
            std::set<I> comp_faces;

            std::vector<face_edge_ref<I>> boundary;
            for(const auto f : comp){
                comp_faces.insert(f);
                const auto &face = fvsm.faces[f];
                for(size_t i = 0UL; i < face.size(); ++i){
                    const auto v_A = face[i];
                    const auto v_B = face[(i + 1UL) % face.size()];
                    const auto r_A = vert_rep[v_A];
                    const auto r_B = vert_rep[v_B];
                    if(r_A == r_B) continue;

                    comp_verts.insert(r_A);
                    comp_verts.insert(r_B);
                    const auto e = make_undirected_edge(r_A, r_B);
                    comp_edges.insert(e);

                    const auto it = edge_map.find(e);
                    if( (it != std::end(edge_map))
                    &&  (it->second.size() == 1UL) ){
                        boundary.push_back({ f, static_cast<I>(i), v_A, v_B, r_A, r_B });
                    }
                }
            }

            int64_t b = 0;
            if(!boundary.empty()){
                std::multimap<I,size_t> starts;
                for(size_t i = 0UL; i < boundary.size(); ++i){
                    starts.insert({ boundary[i].rep_A, i });
                }

                std::vector<bool> used(boundary.size(), false);
                for(size_t i = 0UL; i < boundary.size(); ++i){
                    if(used[i]) continue;
                    auto curr_i = i;
                    const auto start_rep = boundary[curr_i].rep_A;
                    while(!used[curr_i]){
                        used[curr_i] = true;
                        const auto next_rep = boundary[curr_i].rep_B;
                        if(next_rep == start_rep){
                            ++b;
                            break;
                        }

                        const auto eq = starts.equal_range(next_rep);
                        size_t candidate_i = std::numeric_limits<size_t>::max();
                        for(auto it = eq.first; it != eq.second; ++it){
                            if(!used[it->second]){
                                candidate_i = it->second;
                                break;
                            }
                        }
                        if(candidate_i == std::numeric_limits<size_t>::max()){
                            throw std::runtime_error("Unable to compute genus: encountered an open or malformed boundary chain. Consider refining the mesh, merging duplicate vertices, and removing degenerate/non-manifold faces before retrying.");
                        }
                        curr_i = candidate_i;
                    }
                }
            }

            const int64_t V = static_cast<int64_t>(comp_verts.size());
            const int64_t E = static_cast<int64_t>(comp_edges.size());
            const int64_t F = static_cast<int64_t>(comp_faces.size());
            const int64_t chi = V - E + F;
            const int64_t numer = 2 - b - chi;
            if((numer < 0) || ((numer % 2) != 0)){
                throw std::runtime_error("Unable to compute genus: mesh topology appears inconsistent with an orientable manifold. Consider refining the mesh, repairing boundaries, and removing non-manifold features before retrying.");
            }
            genus_sum += numer / 2;
        }

        *genus = genus_sum;
    }

    return true;
}
#ifndef YGORMESHPROCESSING_DISABLE_ALL_SPECIALIZATIONS
    template bool
    EnsureConsistentFaceOrientation(fv_surface_mesh<float,uint32_t> &, float, int64_t *);
    template bool
    EnsureConsistentFaceOrientation(fv_surface_mesh<float,uint64_t> &, float, int64_t *);

    template bool
    EnsureConsistentFaceOrientation(fv_surface_mesh<double,uint32_t> &, double, int64_t *);
    template bool
    EnsureConsistentFaceOrientation(fv_surface_mesh<double,uint64_t> &, double, int64_t *);
#endif
