//YgorMathConstrainedDelaunay.cc.

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

namespace {

constexpr size_t CDT_INVALID_VERTEX_INDEX = std::numeric_limits<size_t>::max();
constexpr size_t CDT_LEGALIZATION_GUARD_SCALE = 8;
constexpr size_t CDT_LEGALIZATION_GUARD_BIAS = 16;

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

struct CDT_ConstraintFace {
    std::vector<size_t> cycle;
    size_t component = 0;
    long double area = 0.0L;
};

inline CDT_Edge make_edge(size_t a, size_t b){
    if(b < a){
        std::swap(a, b);
    }
    return CDT_Edge{a, b};
}

template <class T>
bool is_finite_2d(const vec2<T> &v){
    return std::isfinite(v.x) && std::isfinite(v.y);
}

template <class T>
bool same_xy(const vec2<T> &a, const vec2<T> &b){
    return (a.x == b.x) && (a.y == b.y);
}

template <class T>
long double polygon_signed_area_ld(const std::vector<vec2<T>> &verts,
                                   const std::vector<size_t> &poly);

template <class T>
bool make_ccw_triangle(const std::vector<vec2<T>> &verts,
                       size_t a,
                       size_t b,
                       size_t c,
                       CDT_Triangle &out){
    const auto o = orient2d_sign(verts.at(a), verts.at(b), verts.at(c));
    if(o > 0){
        out = CDT_Triangle{a, b, c};
        return true;
    }
    if(o < 0){
        out = CDT_Triangle{a, c, b};
        return true;
    }
    return false;
}

template <class T>
void prune_triangles(const std::vector<vec2<T>> &verts,
                     std::vector<CDT_Triangle> &triangles){
    std::set<std::array<size_t, 3>> seen;
    std::vector<CDT_Triangle> filtered;
    filtered.reserve(triangles.size());

    for(const auto &tri : triangles){
        if((tri.a == tri.b) || (tri.b == tri.c) || (tri.c == tri.a)){
            continue;
        }
        if(orient2d_sign(verts.at(tri.a), verts.at(tri.b), verts.at(tri.c)) == 0){
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
std::vector<CDT_Triangle> build_delaunay_triangles(const std::vector<vec2<T>> &verts){
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

    std::vector<vec2<T>> all_verts;
    all_verts.reserve(verts.size() + 3);
    all_verts.emplace_back(mid_x - static_cast<T>(20) * delta, mid_y - delta);
    all_verts.emplace_back(mid_x + static_cast<T>(20) * delta, mid_y - delta);
    all_verts.emplace_back(mid_x, mid_y + static_cast<T>(20) * delta);
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
            if(incircle2d_sign(all_verts.at(tri.a), all_verts.at(tri.b), all_verts.at(tri.c), p) > 0){
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
bool collect_user_constraints(const std::vector<vec2<T>> &verts,
                              const std::vector<std::vector<I>> &edges,
                              std::vector<std::pair<size_t, size_t>> &constraints){
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
            return false;
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

template <class T>
bool build_constraint_faces(const std::vector<vec2<T>> &verts,
                            const std::vector<std::pair<size_t, size_t>> &constraints,
                            std::vector<CDT_ConstraintFace> &faces,
                            std::map<std::pair<size_t, size_t>, size_t> &halfedge_to_face,
                            std::map<size_t, size_t> &component_outer_face){
    faces.clear();
    halfedge_to_face.clear();
    component_outer_face.clear();

    std::map<size_t, std::vector<size_t>> adjacency;
    for(const auto &[a, b] : constraints){
        adjacency[a].push_back(b);
        adjacency[b].push_back(a);
    }
    if(adjacency.empty()){
        return true;
    }

    for(auto &[vertex, nbrs] : adjacency){
        std::sort(nbrs.begin(), nbrs.end(),
                  [&](size_t lhs, size_t rhs){
                      const auto &origin = verts.at(vertex);
                      const auto &a = verts.at(lhs);
                      const auto &b = verts.at(rhs);
                      const auto angle_a = std::atan2(a.y - origin.y, a.x - origin.x);
                      const auto angle_b = std::atan2(b.y - origin.y, b.x - origin.x);
                      return angle_a < angle_b;
                  });
    }

    std::map<size_t, size_t> vertex_component;
    std::set<size_t> visited_vertices;
    size_t next_component = 0;
    for(const auto &[start, _] : adjacency){
        if(visited_vertices.count(start) != 0){
            continue;
        }

        std::vector<size_t> pending{ start };
        visited_vertices.insert(start);
        while(!pending.empty()){
            const auto cur = pending.back();
            pending.pop_back();
            vertex_component[cur] = next_component;
            for(const auto nbr : adjacency.at(cur)){
                if(visited_vertices.insert(nbr).second){
                    pending.push_back(nbr);
                }
            }
        }
        ++next_component;
    }

    std::set<std::pair<size_t, size_t>> visited_halfedges;
    const auto max_face_steps = constraints.size() * 2 + 1;
    for(const auto &[start, nbrs] : adjacency){
        for(const auto first : nbrs){
            const auto start_halfedge = std::make_pair(start, first);
            if(visited_halfedges.count(start_halfedge) != 0){
                continue;
            }

            CDT_ConstraintFace face;
            size_t prev = start;
            size_t cur = first;
            while(true){
                const auto halfedge = std::make_pair(prev, cur);
                visited_halfedges.insert(halfedge);
                halfedge_to_face[halfedge] = faces.size();
                face.cycle.push_back(prev);

                const auto &ordered = adjacency.at(cur);
                const auto it = std::find(ordered.begin(), ordered.end(), prev);
                if(it == ordered.end()){
                    return false;
                }

                const auto idx = static_cast<size_t>(std::distance(ordered.begin(), it));
                const auto next = ordered.at((idx + ordered.size() - 1) % ordered.size());
                prev = cur;
                cur = next;

                if((prev == start_halfedge.first) && (cur == start_halfedge.second)){
                    break;
                }
                if(face.cycle.size() > max_face_steps){
                    return false;
                }
            }

            face.component = vertex_component.at(face.cycle.front());
            face.area = polygon_signed_area_ld(verts, face.cycle);
            faces.push_back(std::move(face));
        }
    }

    for(size_t i = 0; i < faces.size(); ++i){
        const auto component = faces.at(i).component;
        const auto outer_it = component_outer_face.find(component);
        if((outer_it == component_outer_face.end())
        || (faces.at(i).area < faces.at(outer_it->second).area)){
            component_outer_face[component] = i;
        }
    }

    return true;
}

template <class T>
bool retain_triangles_in_constraint_faces(const std::vector<vec2<T>> &verts,
                                          const std::vector<std::pair<size_t, size_t>> &constraints,
                                          std::vector<CDT_Triangle> &triangles){
    std::vector<CDT_ConstraintFace> faces;
    std::map<std::pair<size_t, size_t>, size_t> halfedge_to_face;
    std::map<size_t, size_t> component_outer_face;
    if(!build_constraint_faces(verts, constraints, faces, halfedge_to_face, component_outer_face)){
        return false;
    }
    if(faces.empty()){
        return true;
    }

    std::vector<std::vector<size_t>> face_adjacency(faces.size());
    for(const auto &[a, b] : constraints){
        const auto left_it  = halfedge_to_face.find(std::make_pair(a, b));
        const auto right_it = halfedge_to_face.find(std::make_pair(b, a));
        if((left_it == halfedge_to_face.end()) || (right_it == halfedge_to_face.end())){
            return false;
        }
        const auto left_face = left_it->second;
        const auto right_face = right_it->second;
        if(left_face == right_face){
            continue;
        }
        face_adjacency.at(left_face).push_back(right_face);
        face_adjacency.at(right_face).push_back(left_face);
    }

    std::vector<int> face_parity(faces.size(), -1);
    for(const auto &[component, outer_face] : component_outer_face){
        const auto sample_idx = faces.at(outer_face).cycle.front();
        int base_parity = 0;
        for(const auto &[other_component, other_outer_face] : component_outer_face){
            if(other_component == component){
                continue;
            }
            if(point_in_polygon_or_on_boundary(verts, faces.at(other_outer_face).cycle, verts.at(sample_idx))){
                base_parity ^= 1;
            }
        }

        std::vector<size_t> pending{ outer_face };
        face_parity.at(outer_face) = base_parity;
        while(!pending.empty()){
            const auto face_idx = pending.back();
            pending.pop_back();
            for(const auto adjacent_face : face_adjacency.at(face_idx)){
                const auto next_parity = face_parity.at(face_idx) ^ 1;
                if(face_parity.at(adjacent_face) == -1){
                    face_parity.at(adjacent_face) = next_parity;
                    pending.push_back(adjacent_face);
                }else if(face_parity.at(adjacent_face) != next_parity){
                    return false;
                }
            }
        }
    }

    const auto has_closed_region = std::any_of(faces.begin(), faces.end(),
                                               [](const CDT_ConstraintFace &face){ return face.area > 0.0L; });
    if(!has_closed_region){
        return true;
    }

    std::vector<CDT_Triangle> filtered;
    filtered.reserve(triangles.size());
    for(const auto &tri : triangles){
        const auto &a = verts.at(tri.a);
        const auto &b = verts.at(tri.b);
        const auto &c = verts.at(tri.c);
        const vec2<T> centroid((a.x + b.x + c.x) / static_cast<T>(3),
                               (a.y + b.y + c.y) / static_cast<T>(3));

        for(size_t i = 0; i < faces.size(); ++i){
            if((faces.at(i).area <= 0.0L) || (face_parity.at(i) != 1)){
                continue;
            }
            if(point_in_polygon_or_on_boundary(verts, faces.at(i).cycle, centroid)){
                filtered.push_back(tri);
                break;
            }
        }
    }

    triangles.swap(filtered);
    return true;
}

inline std::array<CDT_Edge, 3> triangle_edges(const CDT_Triangle &tri){
    return std::array<CDT_Edge, 3>{{
        make_edge(tri.a, tri.b),
        make_edge(tri.b, tri.c),
        make_edge(tri.c, tri.a)
    }};
}

inline bool triangle_has_edge(const CDT_Triangle &tri, const CDT_Edge &edge){
    const auto edges = triangle_edges(tri);
    return std::find(edges.begin(), edges.end(), edge) != edges.end();
}

inline size_t opposite_vertex(const CDT_Triangle &tri, const CDT_Edge &edge){
    if((tri.a != edge.a) && (tri.a != edge.b)) return tri.a;
    if((tri.b != edge.a) && (tri.b != edge.b)) return tri.b;
    return tri.c;
}

bool triangulation_has_edge(const std::vector<CDT_Triangle> &triangles,
                            size_t a,
                            size_t b){
    const auto edge = make_edge(a, b);
    return std::any_of(triangles.begin(), triangles.end(),
                       [&](const CDT_Triangle &tri){ return triangle_has_edge(tri, edge); });
}

bool walk_boundary_chain(size_t start,
                        size_t stop,
                        size_t first,
                        const std::map<size_t, std::vector<size_t>> &adjacency,
                        std::vector<size_t> &chain){
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
        size_t next = CDT_INVALID_VERTEX_INDEX;
        if(nbrs.at(0) != prev){
            next = nbrs.at(0);
        }else if(nbrs.at(1) != prev){
            next = nbrs.at(1);
        }
        if((next == CDT_INVALID_VERTEX_INDEX) || (next == start)){
            return false;
        }
        prev = cur;
        cur = next;
    }
}

template <class T>
long double polygon_signed_area_ld(const std::vector<vec2<T>> &verts,
                                   const std::vector<size_t> &poly){
    long double area = 0.0L;
    for(size_t i = 0; i < poly.size(); ++i){
        const auto &a = verts.at(poly.at(i));
        const auto &b = verts.at(poly.at((i + 1) % poly.size()));
        area += (static_cast<long double>(a.x) * static_cast<long double>(b.y))
              - (static_cast<long double>(b.x) * static_cast<long double>(a.y));
    }
    return area / 2.0L;
}

template <class T>
bool triangulate_polygon_ear_clip(const std::vector<vec2<T>> &verts,
                                  const std::vector<size_t> &chain,
                                  std::set<CDT_Edge> &boundary_edges,
                                  std::vector<CDT_Triangle> &triangles){
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
    const auto area = polygon_signed_area_ld(verts, poly);
    if(area == 0.0L){
        return true;
    }
    const auto poly_sign = (area > 0.0L) ? 1 : -1;

    while(poly.size() > 3){
        bool clipped = false;
        const auto n = poly.size();
        for(size_t i = 0; i < n; ++i){
            const auto prev = poly.at((i + n - 1) % n);
            const auto cur  = poly.at(i);
            const auto next = poly.at((i + 1) % n);
            const auto o = orient2d_sign(verts.at(prev), verts.at(cur), verts.at(next));
            if((poly_sign > 0 && o <= 0)
            || (poly_sign < 0 && o >= 0)){
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
bool diagonal_crosses_polygon_boundary(const std::vector<vec2<T>> &verts,
                                       size_t a,
                                       size_t b,
                                       const std::set<CDT_Edge> &boundary_edges){
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
bool diagonal_in_polygon_cone(const std::vector<vec2<T>> &verts,
                              const std::vector<size_t> &polygon,
                              size_t vertex_pos,
                              size_t other_pos,
                              int poly_sign){
    const auto n = polygon.size();
    const auto prev = polygon.at((vertex_pos + n - 1) % n);
    const auto cur = polygon.at(vertex_pos);
    const auto next = polygon.at((vertex_pos + 1) % n);
    const auto other = polygon.at(other_pos);

    const auto corner_turn = poly_sign * orient2d_sign(verts.at(prev), verts.at(cur), verts.at(next));
    if(corner_turn > 0){
        return (poly_sign * orient2d_sign(verts.at(cur), verts.at(other), verts.at(prev)) > 0)
            && (poly_sign * orient2d_sign(verts.at(other), verts.at(cur), verts.at(next)) > 0);
    }

    return !((poly_sign * orient2d_sign(verts.at(cur), verts.at(other), verts.at(next)) >= 0)
          && (poly_sign * orient2d_sign(verts.at(other), verts.at(cur), verts.at(prev)) >= 0));
}

template <class T>
bool diagonal_lies_inside_polygon(const std::vector<vec2<T>> &verts,
                                  size_t a,
                                  size_t b,
                                  const std::vector<size_t> &polygon,
                                  const std::set<CDT_Edge> &boundary_edges){
    if((a == b) || diagonal_crosses_polygon_boundary(verts, a, b, boundary_edges)){
        return false;
    }

    std::map<size_t, size_t> positions;
    for(size_t i = 0; i < polygon.size(); ++i){
        positions[polygon.at(i)] = i;
    }

    const auto it_a = positions.find(a);
    const auto it_b = positions.find(b);
    if((it_a == positions.end()) || (it_b == positions.end())){
        return false;
    }

    const auto area = polygon_signed_area_ld(verts, polygon);
    if(area == 0.0L){
        return false;
    }
    const auto poly_sign = (area > 0.0L) ? 1 : -1;

    return diagonal_in_polygon_cone(verts, polygon, it_a->second, it_b->second, poly_sign)
        && diagonal_in_polygon_cone(verts, polygon, it_b->second, it_a->second, poly_sign);
}


template <class T>
bool legalize_polygon_triangulation(const std::vector<vec2<T>> &verts,
                                    const std::vector<size_t> &polygon,
                                    const std::set<CDT_Edge> &boundary_edges,
                                    std::vector<CDT_Triangle> &triangles){
    // Edge legalisation converges quickly for these small cavity polygons; this guard only prevents
    // accidental non-termination if a future change breaks the flip logic.
    const auto max_iters = triangles.size() * triangles.size() * CDT_LEGALIZATION_GUARD_SCALE
                         + CDT_LEGALIZATION_GUARD_BIAS;
    size_t iter = 0;
    bool changed = true;
    bool hit_iteration_limit = false;

    while(changed){
        if(iter++ >= max_iters){
            hit_iteration_limit = true;
            break;
        }
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
            if((w == x) || !diagonal_lies_inside_polygon(verts, w, x, polygon, boundary_edges)){
                continue;
            }

            if(incircle2d_sign(verts.at(edge.a), verts.at(edge.b), verts.at(w), verts.at(x)) <= 0){
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

    if(hit_iteration_limit){
        return false;
    }

    prune_triangles(verts, triangles);
    return true;
}

template <class T>
bool constrain_edge(const std::vector<vec2<T>> &verts,
                    size_t a,
                    size_t b,
                    std::vector<CDT_Triangle> &triangles){
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
    || !legalize_polygon_triangulation(verts, chain0, boundary0, tris0)
    || !legalize_polygon_triangulation(verts, chain1, boundary1, tris1)){
        return false;
    }

    retained.insert(retained.end(), tris0.begin(), tris0.end());
    retained.insert(retained.end(), tris1.begin(), tris1.end());
    prune_triangles(verts, retained);

    if(!triangulation_has_edge(retained, a, b)){
        return false;
    }

    triangles.swap(retained);
    return true;
}

} // namespace


template <class T, class I>
fv_surface_mesh<T, I>
Constrained_Delaunay_Triangulation_2(const std::vector<vec2<T>> &verts,
                                     const std::vector<std::vector<I>> &edges){
    fv_surface_mesh<T, I> mesh;

    std::vector<std::pair<size_t, size_t>> constraints;
    if(!collect_user_constraints(verts, edges, constraints)){
        return mesh;
    }

    mesh.vertices.reserve(verts.size());
    for(const auto &vert : verts){
        mesh.vertices.emplace_back(vert.x, vert.y, static_cast<T>(0));
    }

    auto triangles = build_delaunay_triangles(verts);
    for(const auto &[a, b] : constraints){
        if(!triangles.empty() && !constrain_edge(verts, a, b, triangles)){
            return fv_surface_mesh<T, I>();
        }
    }

    prune_triangles(verts, triangles);
    if(!retain_triangles_in_constraint_faces(verts, constraints, triangles)){
        return fv_surface_mesh<T, I>();
    }
    prune_triangles(verts, triangles);

    for(const auto &tri : triangles){
        mesh.faces.push_back({ static_cast<I>(tri.a), static_cast<I>(tri.b), static_cast<I>(tri.c) });
    }

    return mesh;
}

#ifndef YGOR_MATH_CONSTRAINED_DELAUNAY_DISABLE_ALL_SPECIALIZATIONS
    template fv_surface_mesh<float , uint32_t> Constrained_Delaunay_Triangulation_2(const std::vector<vec2<float >> &, const std::vector<std::vector<uint32_t>> &);
    template fv_surface_mesh<float , uint64_t> Constrained_Delaunay_Triangulation_2(const std::vector<vec2<float >> &, const std::vector<std::vector<uint64_t>> &);

    template fv_surface_mesh<double, uint32_t> Constrained_Delaunay_Triangulation_2(const std::vector<vec2<double>> &, const std::vector<std::vector<uint32_t>> &);
    template fv_surface_mesh<double, uint64_t> Constrained_Delaunay_Triangulation_2(const std::vector<vec2<double>> &, const std::vector<std::vector<uint64_t>> &);
#endif
