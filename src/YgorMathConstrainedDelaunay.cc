//YgorMathConstrainedDelaunay.cc

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorMathConstrainedDelaunay.h"
#include "YgorMeshesConvexHull.h"

namespace {

struct CDT_Edge {
    size_t a;
    size_t b;

    bool operator<(const CDT_Edge &other) const {
        return (a < other.a) || ((a == other.a) && (b < other.b));
    }

    bool operator==(const CDT_Edge &other) const {
        return (a == other.a) && (b == other.b);
    }
};

struct CDT_Triangle {
    size_t a;
    size_t b;
    size_t c;
};

inline CDT_Edge make_edge(size_t a, size_t b) {
    if(b < a){
        std::swap(a, b);
    }
    return CDT_Edge{a, b};
}

template <class T>
bool is_finite_2d(const vec3<T> &v) {
    return std::isfinite(v.x) && std::isfinite(v.y);
}

template <class T>
T coord_eps(const vec3<T> &a, const vec3<T> &b) {
    const auto scale = std::max({std::abs(a.x), std::abs(a.y), std::abs(b.x), std::abs(b.y), static_cast<T>(1)});
    return std::sqrt(std::numeric_limits<T>::epsilon()) * scale;
}

template <class T>
T orient2d(const vec3<T> &a, const vec3<T> &b, const vec3<T> &c) {
    const std::array<T, 3> pa{{ a.x, a.y, static_cast<T>(0) }};
    const std::array<T, 3> pb{{ b.x, b.y, static_cast<T>(0) }};
    const std::array<T, 3> pc{{ c.x, c.y, static_cast<T>(0) }};
    const std::array<T, 3> pd{{ static_cast<T>(0), static_cast<T>(0), static_cast<T>(1) }};
    return -adaptive_predicate::orient3d(pa.data(), pb.data(), pc.data(), pd.data());
}

template <class T>
T incircle2d(const vec3<T> &a, const vec3<T> &b, const vec3<T> &c, const vec3<T> &d) {
    const std::array<T, 3> pa{{ a.x, a.y, a.x * a.x + a.y * a.y }};
    const std::array<T, 3> pb{{ b.x, b.y, b.x * b.x + b.y * b.y }};
    const std::array<T, 3> pc{{ c.x, c.y, c.x * c.x + c.y * c.y }};
    const std::array<T, 3> pd{{ d.x, d.y, d.x * d.x + d.y * d.y }};

    auto det = adaptive_predicate::orient3d(pa.data(), pb.data(), pc.data(), pd.data());
    if(orient2d(a, b, c) < static_cast<T>(0)){
        det = -det;
    }
    return det;
}

template <class T>
bool point_on_closed_segment(const vec3<T> &p, const vec3<T> &a, const vec3<T> &b) {
    if(orient2d(a, b, p) != static_cast<T>(0)){
        return false;
    }

    const auto eps = coord_eps(a, b);
    return (std::min(a.x, b.x) - eps <= p.x) && (p.x <= std::max(a.x, b.x) + eps)
        && (std::min(a.y, b.y) - eps <= p.y) && (p.y <= std::max(a.y, b.y) + eps);
}

template <class T>
bool same_xy(const vec3<T> &a, const vec3<T> &b) {
    return (a.x == b.x) && (a.y == b.y);
}

template <class T>
bool point_on_open_segment(const vec3<T> &p, const vec3<T> &a, const vec3<T> &b) {
    if(!point_on_closed_segment(p, a, b)){
        return false;
    }
    return !same_xy(p, a) && !same_xy(p, b);
}

template <class T>
bool segments_intersect_beyond_shared_endpoints(const vec3<T> &a, const vec3<T> &b,
                                                const vec3<T> &c, const vec3<T> &d) {
    if(point_on_open_segment(a, c, d) || point_on_open_segment(b, c, d)
    || point_on_open_segment(c, a, b) || point_on_open_segment(d, a, b)){
        return true;
    }

    const auto o1 = orient2d(a, b, c);
    const auto o2 = orient2d(a, b, d);
    const auto o3 = orient2d(c, d, a);
    const auto o4 = orient2d(c, d, b);

    return (((o1 < static_cast<T>(0)) && (o2 > static_cast<T>(0)))
         || ((o1 > static_cast<T>(0)) && (o2 < static_cast<T>(0))))
        && (((o3 < static_cast<T>(0)) && (o4 > static_cast<T>(0)))
         || ((o3 > static_cast<T>(0)) && (o4 < static_cast<T>(0))));
}

template <class T>
bool point_in_triangle_or_on_boundary(const vec3<T> &p,
                                      const vec3<T> &a,
                                      const vec3<T> &b,
                                      const vec3<T> &c) {
    const auto o = orient2d(a, b, c);
    if(o == static_cast<T>(0)){
        return false;
    }

    const auto o1 = orient2d(a, b, p);
    const auto o2 = orient2d(b, c, p);
    const auto o3 = orient2d(c, a, p);

    if(o > static_cast<T>(0)){
        return (o1 >= static_cast<T>(0))
            && (o2 >= static_cast<T>(0))
            && (o3 >= static_cast<T>(0));
    }

    return (o1 <= static_cast<T>(0))
        && (o2 <= static_cast<T>(0))
        && (o3 <= static_cast<T>(0));
}

template <class T>
bool make_ccw_triangle(const std::vector<vec3<T>> &verts,
                       size_t a,
                       size_t b,
                       size_t c,
                       CDT_Triangle &out) {
    const auto o = orient2d(verts.at(a), verts.at(b), verts.at(c));
    if(o > static_cast<T>(0)){
        out = CDT_Triangle{a, b, c};
        return true;
    }
    if(o < static_cast<T>(0)){
        out = CDT_Triangle{a, c, b};
        return true;
    }
    return false;
}

template <class T>
void prune_triangles(const std::vector<vec3<T>> &verts,
                     std::vector<CDT_Triangle> &triangles) {
    std::set<std::array<size_t, 3>> seen;
    std::vector<CDT_Triangle> filtered;
    filtered.reserve(triangles.size());

    for(const auto &tri : triangles){
        if((tri.a == tri.b) || (tri.b == tri.c) || (tri.c == tri.a)){
            continue;
        }
        if(orient2d(verts.at(tri.a), verts.at(tri.b), verts.at(tri.c)) == static_cast<T>(0)){
            continue;
        }
        auto key = std::array<size_t, 3>{{ tri.a, tri.b, tri.c }};
        std::sort(key.begin(), key.end());
        if(seen.insert(key).second){
            filtered.push_back(tri);
        }
    }

    triangles.swap(filtered);
}

template <class T>
std::vector<CDT_Triangle> build_delaunay_triangles(const std::vector<vec3<T>> &verts) {
    std::vector<CDT_Triangle> output;

    T min_x = std::numeric_limits<T>::max();
    T max_x = std::numeric_limits<T>::lowest();
    T min_y = std::numeric_limits<T>::max();
    T max_y = std::numeric_limits<T>::lowest();
    size_t n_finite = 0;
    for(const auto &v : verts){
        if(!is_finite_2d(v)){
            continue;
        }
        ++n_finite;
        min_x = std::min(min_x, v.x);
        max_x = std::max(max_x, v.x);
        min_y = std::min(min_y, v.y);
        max_y = std::max(max_y, v.y);
    }
    if(n_finite < 3){
        return output;
    }

    auto delta = std::max(max_x - min_x, max_y - min_y);
    if(delta <= static_cast<T>(0)){
        delta = static_cast<T>(1);
    }

    const auto mid_x = (min_x + max_x) / static_cast<T>(2);
    const auto mid_y = (min_y + max_y) / static_cast<T>(2);

    std::vector<vec3<T>> all_verts;
    all_verts.reserve(verts.size() + 3);
    all_verts.emplace_back(mid_x - static_cast<T>(20) * delta, mid_y - delta, static_cast<T>(0));
    all_verts.emplace_back(mid_x + static_cast<T>(20) * delta, mid_y - delta, static_cast<T>(0));
    all_verts.emplace_back(mid_x, mid_y + static_cast<T>(20) * delta, static_cast<T>(0));
    all_verts.insert(all_verts.end(), verts.begin(), verts.end());

    struct WorkingTriangle {
        size_t a;
        size_t b;
        size_t c;
        bool bad = false;
    };

    std::vector<WorkingTriangle> triangles;
    triangles.push_back(WorkingTriangle{0, 1, 2, false});

    for(size_t i = 3; i < all_verts.size(); ++i){
        const auto &p = all_verts.at(i);
        if(!is_finite_2d(p)){
            continue;
        }

        for(auto &tri : triangles){
            if(tri.bad){
                continue;
            }
            if(incircle2d(all_verts.at(tri.a), all_verts.at(tri.b), all_verts.at(tri.c), p) > static_cast<T>(0)){
                tri.bad = true;
            }
        }

        std::map<CDT_Edge, size_t> edge_count;
        for(const auto &tri : triangles){
            if(!tri.bad){
                continue;
            }
            ++edge_count[make_edge(tri.a, tri.b)];
            ++edge_count[make_edge(tri.b, tri.c)];
            ++edge_count[make_edge(tri.c, tri.a)];
        }

        triangles.erase(std::remove_if(triangles.begin(), triangles.end(),
                                       [](const WorkingTriangle &tri){ return tri.bad; }),
                        triangles.end());

        for(const auto &[edge, count] : edge_count){
            if(count != 1){
                continue;
            }

            CDT_Triangle next_tri{};
            if(make_ccw_triangle(all_verts, edge.a, edge.b, i, next_tri)){
                WorkingTriangle next;
                next.a = next_tri.a;
                next.b = next_tri.b;
                next.c = next_tri.c;
                next.bad = false;
                triangles.push_back(next);
            }
        }
    }

    for(const auto &tri : triangles){
        if((tri.a < 3) || (tri.b < 3) || (tri.c < 3)){
            continue;
        }
        CDT_Triangle out{};
        if(make_ccw_triangle(all_verts, tri.a, tri.b, tri.c, out)){
            out.a -= 3;
            out.b -= 3;
            out.c -= 3;
            output.push_back(out);
        }
    }

    prune_triangles(verts, output);
    return output;
}

template <class T, class I>
bool collect_user_constraints(const std::vector<vec3<T>> &verts,
                              const std::vector<std::vector<I>> &edges,
                              std::vector<std::pair<size_t, size_t>> &constraints) {
    constraints.clear();
    std::set<CDT_Edge> seen;

    for(const auto &edge : edges){
        if(edge.size() != 2){
            return false;
        }

        const auto a = static_cast<size_t>(edge.at(0));
        const auto b = static_cast<size_t>(edge.at(1));
        if((a >= verts.size()) || (b >= verts.size()) || (a == b)){
            return false;
        }
        if(!is_finite_2d(verts.at(a)) || !is_finite_2d(verts.at(b)) || same_xy(verts.at(a), verts.at(b))){
            return false;
        }

        const auto edge_key = make_edge(a, b);
        if(!seen.insert(edge_key).second){
            continue;
        }

        for(size_t i = 0; i < verts.size(); ++i){
            if((i == a) || (i == b) || !is_finite_2d(verts.at(i))){
                continue;
            }
            if(point_on_open_segment(verts.at(i), verts.at(a), verts.at(b))){
                return false;
            }
        }

        for(const auto &[c, d] : constraints){
            const bool shares_a = (a == c) || (a == d);
            const bool shares_b = (b == c) || (b == d);
            if(shares_a && shares_b){
                continue;
            }
            if(segments_intersect_beyond_shared_endpoints(verts.at(a), verts.at(b),
                                                          verts.at(c), verts.at(d))){
                return false;
            }
        }

        constraints.emplace_back(a, b);
    }

    return true;
}

inline std::array<CDT_Edge, 3> triangle_edges(const CDT_Triangle &tri) {
    return std::array<CDT_Edge, 3>{{
        make_edge(tri.a, tri.b),
        make_edge(tri.b, tri.c),
        make_edge(tri.c, tri.a)
    }};
}

inline bool triangle_has_edge(const CDT_Triangle &tri, const CDT_Edge &edge) {
    const auto edges = triangle_edges(tri);
    return std::find(edges.begin(), edges.end(), edge) != edges.end();
}

inline size_t opposite_vertex(const CDT_Triangle &tri, const CDT_Edge &edge) {
    if((tri.a != edge.a) && (tri.a != edge.b)) return tri.a;
    if((tri.b != edge.a) && (tri.b != edge.b)) return tri.b;
    return tri.c;
}

bool triangulation_has_edge(const std::vector<CDT_Triangle> &triangles,
                            size_t a,
                            size_t b) {
    const auto edge = make_edge(a, b);
    return std::any_of(triangles.begin(), triangles.end(),
                       [&](const CDT_Triangle &tri){ return triangle_has_edge(tri, edge); });
}

bool walk_boundary_chain(size_t start,
                        size_t stop,
                        size_t first,
                        const std::map<size_t, std::vector<size_t>> &adjacency,
                        std::vector<size_t> &chain) {
    chain.clear();
    chain.push_back(start);

    size_t prev = start;
    size_t cur = first;
    std::set<size_t> visited;
    visited.insert(start);

    while(true){
        if(!visited.insert(cur).second && (cur != stop)){
            return false;
        }
        chain.push_back(cur);
        if(cur == stop){
            return true;
        }

        const auto it = adjacency.find(cur);
        if((it == adjacency.end()) || (it->second.size() != 2)){
            return false;
        }

        const auto &nbrs = it->second;
        size_t next = std::numeric_limits<size_t>::max();
        if(nbrs.at(0) != prev){
            next = nbrs.at(0);
        }else if(nbrs.at(1) != prev){
            next = nbrs.at(1);
        }
        if((next == std::numeric_limits<size_t>::max()) || (next == start)){
            return false;
        }
        prev = cur;
        cur = next;
    }
}

template <class T>
T polygon_signed_area(const std::vector<vec3<T>> &verts,
                      const std::vector<size_t> &poly) {
    T area = static_cast<T>(0);
    for(size_t i = 0; i < poly.size(); ++i){
        const auto &a = verts.at(poly.at(i));
        const auto &b = verts.at(poly.at((i + 1) % poly.size()));
        area += (a.x * b.y) - (b.x * a.y);
    }
    return area / static_cast<T>(2);
}

template <class T>
bool triangulate_polygon_ear_clip(const std::vector<vec3<T>> &verts,
                                  const std::vector<size_t> &chain,
                                  std::set<CDT_Edge> &boundary_edges,
                                  std::vector<CDT_Triangle> &triangles) {
    boundary_edges.clear();
    triangles.clear();

    if(chain.size() < 2){
        return false;
    }

    for(size_t i = 0; i < chain.size(); ++i){
        boundary_edges.insert(make_edge(chain.at(i), chain.at((i + 1) % chain.size())));
    }

    if(chain.size() < 3){
        return true;
    }

    auto poly = chain;
    const auto area = polygon_signed_area(verts, poly);
    if(area == static_cast<T>(0)){
        return true;
    }
    const auto poly_sign = (area > static_cast<T>(0)) ? 1 : -1;

    while(poly.size() > 3){
        bool clipped = false;
        const auto n = poly.size();
        for(size_t i = 0; i < n; ++i){
            const auto prev = poly.at((i + n - 1) % n);
            const auto cur  = poly.at(i);
            const auto next = poly.at((i + 1) % n);
            const auto o = orient2d(verts.at(prev), verts.at(cur), verts.at(next));
            if((poly_sign > 0 && o <= static_cast<T>(0))
            || (poly_sign < 0 && o >= static_cast<T>(0))){
                continue;
            }

            bool contains_other = false;
            for(const auto idx : poly){
                if((idx == prev) || (idx == cur) || (idx == next)){
                    continue;
                }
                if(point_in_triangle_or_on_boundary(verts.at(idx), verts.at(prev), verts.at(cur), verts.at(next))){
                    contains_other = true;
                    break;
                }
            }
            if(contains_other){
                continue;
            }

            CDT_Triangle tri{};
            if(make_ccw_triangle(verts, prev, cur, next, tri)){
                triangles.push_back(tri);
            }
            poly.erase(poly.begin() + static_cast<std::ptrdiff_t>(i));
            clipped = true;
            break;
        }

        if(!clipped){
            return false;
        }
    }

    if(poly.size() == 3){
        CDT_Triangle tri{};
        if(make_ccw_triangle(verts, poly.at(0), poly.at(1), poly.at(2), tri)){
            triangles.push_back(tri);
        }
    }

    prune_triangles(verts, triangles);
    return true;
}

template <class T>
bool diagonal_crosses_polygon_boundary(const std::vector<vec3<T>> &verts,
                                       size_t a,
                                       size_t b,
                                       const std::set<CDT_Edge> &boundary_edges) {
    const auto diag = make_edge(a, b);
    if(boundary_edges.count(diag) != 0){
        return true;
    }

    for(const auto &edge : boundary_edges){
        if((edge.a == a) || (edge.a == b) || (edge.b == a) || (edge.b == b)){
            continue;
        }
        if(segments_intersect_beyond_shared_endpoints(verts.at(a), verts.at(b),
                                                      verts.at(edge.a), verts.at(edge.b))){
            return true;
        }
    }
    return false;
}

template <class T>
bool legalize_polygon_triangulation(const std::vector<vec3<T>> &verts,
                                    const std::set<CDT_Edge> &boundary_edges,
                                    std::vector<CDT_Triangle> &triangles) {
    const auto max_iters = triangles.size() * triangles.size() * 8 + 16;
    size_t iter = 0;
    bool changed = true;

    while(changed && (iter++ < max_iters)){
        changed = false;

        std::map<CDT_Edge, std::vector<size_t>> edge_to_triangles;
        for(size_t i = 0; i < triangles.size(); ++i){
            for(const auto &edge : triangle_edges(triangles.at(i))){
                edge_to_triangles[edge].push_back(i);
            }
        }

        for(const auto &[edge, incident] : edge_to_triangles){
            if((incident.size() != 2) || (boundary_edges.count(edge) != 0)){
                continue;
            }

            const auto t0 = incident.at(0);
            const auto t1 = incident.at(1);
            const auto w = opposite_vertex(triangles.at(t0), edge);
            const auto x = opposite_vertex(triangles.at(t1), edge);
            if((w == x) || diagonal_crosses_polygon_boundary(verts, w, x, boundary_edges)){
                continue;
            }

            if(incircle2d(verts.at(edge.a), verts.at(edge.b), verts.at(w), verts.at(x)) <= static_cast<T>(0)){
                continue;
            }

            CDT_Triangle first{};
            CDT_Triangle second{};
            if(!make_ccw_triangle(verts, w, x, edge.a, first)
            || !make_ccw_triangle(verts, x, w, edge.b, second)){
                continue;
            }

            triangles.at(t0) = first;
            triangles.at(t1) = second;
            changed = true;
            break;
        }
    }

    if(iter > max_iters){
        return false;
    }

    prune_triangles(verts, triangles);
    return true;
}

template <class T>
bool constrain_edge(const std::vector<vec3<T>> &verts,
                    size_t a,
                    size_t b,
                    std::vector<CDT_Triangle> &triangles) {
    if(triangulation_has_edge(triangles, a, b)){
        return true;
    }

    std::set<size_t> removed;
    for(size_t i = 0; i < triangles.size(); ++i){
        const auto &tri = triangles.at(i);
        const auto tri_edges = triangle_edges(tri);
        for(const auto &edge : tri_edges){
            if(segments_intersect_beyond_shared_endpoints(verts.at(a), verts.at(b),
                                                          verts.at(edge.a), verts.at(edge.b))){
                removed.insert(i);
                break;
            }
        }
    }

    if(removed.empty()){
        return false;
    }

    std::map<CDT_Edge, size_t> boundary_counts;
    for(const auto idx : removed){
        for(const auto &edge : triangle_edges(triangles.at(idx))){
            ++boundary_counts[edge];
        }
    }

    std::vector<CDT_Triangle> retained;
    retained.reserve(triangles.size());
    for(size_t i = 0; i < triangles.size(); ++i){
        if(removed.count(i) == 0){
            retained.push_back(triangles.at(i));
        }
    }

    std::map<size_t, std::vector<size_t>> adjacency;
    for(const auto &[edge, count] : boundary_counts){
        if(count != 1){
            continue;
        }
        adjacency[edge.a].push_back(edge.b);
        adjacency[edge.b].push_back(edge.a);
    }

    const auto it_a = adjacency.find(a);
    const auto it_b = adjacency.find(b);
    if((it_a == adjacency.end()) || (it_b == adjacency.end())
    || (it_a->second.size() != 2) || (it_b->second.size() != 2)){
        return false;
    }

    std::vector<size_t> chain0;
    std::vector<size_t> chain1;
    if(!walk_boundary_chain(a, b, it_a->second.at(0), adjacency, chain0)
    || !walk_boundary_chain(a, b, it_a->second.at(1), adjacency, chain1)){
        return false;
    }

    std::set<CDT_Edge> boundary0;
    std::set<CDT_Edge> boundary1;
    std::vector<CDT_Triangle> tris0;
    std::vector<CDT_Triangle> tris1;
    if(!triangulate_polygon_ear_clip(verts, chain0, boundary0, tris0)
    || !triangulate_polygon_ear_clip(verts, chain1, boundary1, tris1)
    || !legalize_polygon_triangulation(verts, boundary0, tris0)
    || !legalize_polygon_triangulation(verts, boundary1, tris1)){
        return false;
    }

    retained.insert(retained.end(), tris0.begin(), tris0.end());
    retained.insert(retained.end(), tris1.begin(), tris1.end());
    prune_triangles(verts, retained);

    triangles.swap(retained);
    return triangulation_has_edge(triangles, a, b) || triangles.empty();
}

} // namespace


template <class T, class I>
fv_surface_mesh<T, I>
Constrained_Delaunay_Triangulation_2(const std::vector<vec3<T>> &verts,
                                     const std::vector<std::vector<I>> &edges) {
    fv_surface_mesh<T, I> mesh;

    std::vector<std::pair<size_t, size_t>> constraints;
    if(!collect_user_constraints(verts, edges, constraints)){
        return mesh;
    }

    mesh.vertices = verts;

    auto triangles = build_delaunay_triangles(verts);
    for(const auto &[a, b] : constraints){
        if(!triangles.empty() && !constrain_edge(verts, a, b, triangles)){
            return fv_surface_mesh<T, I>();
        }
    }

    prune_triangles(verts, triangles);

    std::set<CDT_Edge> user_edges;
    for(const auto &[a, b] : constraints){
        user_edges.insert(make_edge(a, b));
        mesh.faces.push_back({ static_cast<I>(a), static_cast<I>(b) });
    }

    std::set<CDT_Edge> triangulation_edges;
    for(const auto &tri : triangles){
        triangulation_edges.insert(make_edge(tri.a, tri.b));
        triangulation_edges.insert(make_edge(tri.b, tri.c));
        triangulation_edges.insert(make_edge(tri.c, tri.a));
    }

    for(const auto &edge : triangulation_edges){
        if(user_edges.count(edge) == 0){
            mesh.faces.push_back({ static_cast<I>(edge.a), static_cast<I>(edge.b) });
        }
    }

    for(const auto &tri : triangles){
        mesh.faces.push_back({ static_cast<I>(tri.a), static_cast<I>(tri.b), static_cast<I>(tri.c) });
    }

    return mesh;
}

#ifndef YGOR_MATH_CONSTRAINED_DELAUNAY_DISABLE_ALL_SPECIALIZATIONS
    template fv_surface_mesh<float , uint32_t> Constrained_Delaunay_Triangulation_2(const std::vector<vec3<float >> &, const std::vector<std::vector<uint32_t>> &);
    template fv_surface_mesh<float , uint64_t> Constrained_Delaunay_Triangulation_2(const std::vector<vec3<float >> &, const std::vector<std::vector<uint64_t>> &);

    template fv_surface_mesh<double, uint32_t> Constrained_Delaunay_Triangulation_2(const std::vector<vec3<double>> &, const std::vector<std::vector<uint32_t>> &);
    template fv_surface_mesh<double, uint64_t> Constrained_Delaunay_Triangulation_2(const std::vector<vec3<double>> &, const std::vector<std::vector<uint64_t>> &);
#endif
