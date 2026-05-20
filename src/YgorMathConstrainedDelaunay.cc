//YgorMathConstrainedDelaunay.cc.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <limits>
#include <map>
#include <sstream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMathConstrainedDelaunay.h"
#include "YgorMeshesAdaptivePredicates.h"
#include "YgorMeshesConvexHull.h"

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

inline bool cdt_fail(std::string *diag, const std::string &msg){
    if(diag != nullptr){
        *diag = msg;
    }
    YLOGDEBUG(msg);
    return false;
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
bool has_non_collinear_triplet(const std::vector<vec2<T>> &verts){
    size_t a = verts.size();
    for(size_t i = 0; i < verts.size(); ++i){
        if(is_finite_2d(verts.at(i))){
            a = i;
            break;
        }
    }
    if(a == verts.size()){
        return false;
    }
    size_t b = verts.size();
    for(size_t i = a + 1; i < verts.size(); ++i){
        if(is_finite_2d(verts.at(i)) && !same_xy(verts.at(a), verts.at(i))){
            b = i;
            break;
        }
    }
    if(b == verts.size()){
        return false;
    }

    for(size_t i = b + 1; i < verts.size(); ++i){
        if(!is_finite_2d(verts.at(i))){
            continue;
        }
        if(orient_sign(verts.at(a), verts.at(b), verts.at(i)) != 0){
            return true;
        }
    }
    return false;
}

template <class T>
long double polygon_signed_area_ld(const std::vector<vec2<T>> &verts,
                                   const std::vector<size_t> &poly);

inline std::vector<size_t> normalize_cycle_vertices(const std::vector<size_t> &cycle);

template <class T>
bool make_ccw_triangle(const std::vector<vec2<T>> &verts,
                       size_t a,
                       size_t b,
                       size_t c,
                       CDT_Triangle &out){
    const auto o = orient_sign(verts.at(a), verts.at(b), verts.at(c));
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
        if(orient_sign(verts.at(tri.a), verts.at(tri.b), verts.at(tri.c)) == 0){
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
    std::map<std::pair<T, T>, size_t> unique_to_original;
    std::vector<vec2<T>> unique_verts;
    unique_verts.reserve(verts.size());

    T min_x = std::numeric_limits<T>::max();
    T max_x = std::numeric_limits<T>::lowest();
    T min_y = std::numeric_limits<T>::max();
    T max_y = std::numeric_limits<T>::lowest();

    for(size_t i = 0; i < verts.size(); ++i){
        const auto &vert = verts.at(i);
        const auto key = std::make_pair(vert.x, vert.y);
        if(unique_to_original.emplace(key, i).second){
            unique_verts.push_back(vert);
            min_x = std::min(min_x, vert.x);
            max_x = std::max(max_x, vert.x);
            min_y = std::min(min_y, vert.y);
            max_y = std::max(max_y, vert.y);
        }
    }
    if(unique_verts.size() < 3){
        return output;
    }
    if(unique_verts.size() == 3){
        CDT_Triangle tri{};
        if(make_ccw_triangle(verts,
                             unique_to_original.at(std::make_pair(unique_verts.at(0).x, unique_verts.at(0).y)),
                             unique_to_original.at(std::make_pair(unique_verts.at(1).x, unique_verts.at(1).y)),
                             unique_to_original.at(std::make_pair(unique_verts.at(2).x, unique_verts.at(2).y)),
                             tri)){
            output.push_back(tri);
        }
        return output;
    }

    const auto lift_center_x = (min_x + max_x) / static_cast<T>(2);
    const auto lift_center_y = (min_y + max_y) / static_cast<T>(2);
    const auto lift_scale = std::max(max_x - min_x, max_y - min_y);
    const auto inv_lift_scale = (lift_scale > static_cast<T>(0)) ? (static_cast<T>(1) / lift_scale)
                                                                 : static_cast<T>(1);
    T min_z = std::numeric_limits<T>::max();
    std::vector<vec3<T>> lifted_verts;
    lifted_verts.reserve(unique_verts.size());
    for(const auto &vert : unique_verts){
        const auto dx = (vert.x - lift_center_x) * inv_lift_scale;
        const auto dy = (vert.y - lift_center_y) * inv_lift_scale;
        const auto z = dx * dx + dy * dy;
        lifted_verts.emplace_back(dx, dy, z);
        min_z = std::min(min_z, z);
    }

    IncrementalConvexHull<T> hull(IncrementalConvexHull<T>::PerturbationMode::ZOnly);
    hull.add_vertices(lifted_verts);

    const auto &hull_mesh = hull.get_mesh();
    const auto &eval_order = hull.get_evaluation_order();
    const vec3<T> below_point(static_cast<T>(0),
                              static_cast<T>(0),
                              min_z - static_cast<T>(2));

    for(const auto &face : hull_mesh.faces){
        if(face.size() != 3){
            continue;
        }

        const auto ia = eval_order.at(face.at(0));
        const auto ib = eval_order.at(face.at(1));
        const auto ic = eval_order.at(face.at(2));

        const std::array<T, 3> pa{{ lifted_verts.at(ia).x, lifted_verts.at(ia).y, lifted_verts.at(ia).z }};
        const std::array<T, 3> pb{{ lifted_verts.at(ib).x, lifted_verts.at(ib).y, lifted_verts.at(ib).z }};
        const std::array<T, 3> pc{{ lifted_verts.at(ic).x, lifted_verts.at(ic).y, lifted_verts.at(ic).z }};
        const std::array<T, 3> pd{{ below_point.x, below_point.y, below_point.z }};
        if(adaptive_predicate::orient3d(pa.data(), pb.data(), pc.data(), pd.data()) >= static_cast<T>(0)){
            continue;
        }

        CDT_Triangle tri{};
        if(make_ccw_triangle(verts,
                             unique_to_original.at(std::make_pair(unique_verts.at(ia).x, unique_verts.at(ia).y)),
                             unique_to_original.at(std::make_pair(unique_verts.at(ib).x, unique_verts.at(ib).y)),
                             unique_to_original.at(std::make_pair(unique_verts.at(ic).x, unique_verts.at(ic).y)),
                             tri)){
            output.push_back(tri);
        }
    }

    prune_triangles(verts, output);
    return output;
}

template <class T, class I>
bool collect_user_constraints(const std::vector<vec2<T>> &verts,
                              const std::vector<std::vector<I>> &edges,
                              std::vector<std::pair<size_t, size_t>> &constraints,
                              std::string *diag){
    constraints.clear();
    std::set<CDT_Edge> seen;

    for(size_t edge_idx = 0; edge_idx < edges.size(); ++edge_idx){
        const auto &edge = edges.at(edge_idx);
        if(edge.size() != 2){
            return cdt_fail(diag, "Constraint " + std::to_string(edge_idx)
                                 + " does not contain exactly two vertex indices.");
        }

        const auto a = static_cast<size_t>(edge.at(0));
        const auto b = static_cast<size_t>(edge.at(1));
        if((a >= verts.size()) || (b >= verts.size()) || (a == b)){
            return cdt_fail(diag, "Constraint " + std::to_string(edge_idx)
                                 + " references invalid vertex indices (" + std::to_string(a)
                                 + ", " + std::to_string(b) + ").");
        }
        if(!is_finite_2d(verts.at(a)) || !is_finite_2d(verts.at(b)) || same_xy(verts.at(a), verts.at(b))){
            return cdt_fail(diag, "Constraint " + std::to_string(edge_idx)
                                 + " has coincident or non-finite endpoints.");
        }

        const auto edge_key = make_edge(a, b);
        if(!seen.insert(edge_key).second){
            return cdt_fail(diag, "Constraint " + std::to_string(edge_idx)
                                 + " duplicates an existing constrained edge.");
        }

        for(size_t i = 0; i < verts.size(); ++i){
            if((i == a) || (i == b) || !is_finite_2d(verts.at(i))){
                continue;
            }
            if(point_on_open_segment(verts.at(i), verts.at(a), verts.at(b))){
                return cdt_fail(diag, "Vertex " + std::to_string(i)
                                     + " lies strictly inside constrained segment ("
                                     + std::to_string(a) + ", " + std::to_string(b) + ").");
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
                return cdt_fail(diag, "Constraint (" + std::to_string(a) + ", " + std::to_string(b)
                                     + ") intersects constraint (" + std::to_string(c) + ", "
                                     + std::to_string(d) + ") beyond a shared endpoint.");
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
                            std::map<size_t, size_t> &component_outer_face,
                            std::string *diag){
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
                    return cdt_fail(diag, "Constraint face reconstruction lost halfedge ordering at vertex "
                                         + std::to_string(cur) + ".");
                }

                const auto idx = static_cast<size_t>(std::distance(ordered.begin(), it));
                const auto next = ordered.at((idx + ordered.size() - 1) % ordered.size());
                prev = cur;
                cur = next;

                if((prev == start_halfedge.first) && (cur == start_halfedge.second)){
                    break;
                }
                if(face.cycle.size() > max_face_steps){
                    return cdt_fail(diag, "Constraint face reconstruction exceeded the traversal guard while tracing a boundary cycle.");
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
                                          std::vector<CDT_Triangle> &triangles,
                                          std::string *diag){
    std::vector<CDT_ConstraintFace> faces;
    std::map<std::pair<size_t, size_t>, size_t> halfedge_to_face;
    std::map<size_t, size_t> component_outer_face;
    if(!build_constraint_faces(verts, constraints, faces, halfedge_to_face, component_outer_face, diag)){
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
            return cdt_fail(diag, "Constraint halfedge (" + std::to_string(a) + ", " + std::to_string(b)
                                 + ") was not matched to both incident constraint faces.");
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
                    return cdt_fail(diag, "Constraint face parity propagation became inconsistent; the constrained arrangement is not a valid planar subdivision.");
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

inline bool triangle_has_vertex(const CDT_Triangle &tri, size_t vertex){
    return (tri.a == vertex) || (tri.b == vertex) || (tri.c == vertex);
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

template <class T>
bool build_planar_edge_faces(const std::vector<vec2<T>> &verts,
                             const std::vector<CDT_Edge> &edges,
                             std::vector<std::vector<size_t>> &faces,
                             std::map<std::pair<size_t, size_t>, size_t> &halfedge_to_face,
                             std::string *diag){
    faces.clear();
    halfedge_to_face.clear();

    std::map<size_t, std::vector<size_t>> adjacency;
    for(const auto &edge : edges){
        adjacency[edge.a].push_back(edge.b);
        adjacency[edge.b].push_back(edge.a);
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

    std::set<std::pair<size_t, size_t>> visited_halfedges;
    const auto max_face_steps = edges.size() * 2U + 1U;
    for(const auto &[start, nbrs] : adjacency){
        for(const auto first : nbrs){
            const auto start_halfedge = std::make_pair(start, first);
            if(visited_halfedges.count(start_halfedge) != 0U){
                continue;
            }

            std::vector<size_t> face;
            size_t prev = start;
            size_t cur = first;
            while(true){
                const auto halfedge = std::make_pair(prev, cur);
                visited_halfedges.insert(halfedge);
                halfedge_to_face[halfedge] = faces.size();
                face.push_back(prev);

                const auto &ordered = adjacency.at(cur);
                const auto it = std::find(ordered.begin(), ordered.end(), prev);
                if(it == ordered.end()){
                    return cdt_fail(diag, "Constraint cavity face reconstruction lost halfedge ordering at vertex "
                                         + std::to_string(cur) + ".");
                }

                const auto idx = static_cast<size_t>(std::distance(ordered.begin(), it));
                const auto next = ordered.at((idx + ordered.size() - 1U) % ordered.size());
                prev = cur;
                cur = next;

                if((prev == start_halfedge.first) && (cur == start_halfedge.second)){
                    break;
                }
                if(face.size() > max_face_steps){
                    return cdt_fail(diag, "Constraint cavity face reconstruction exceeded the traversal guard while tracing a boundary cycle.");
                }
            }

            faces.push_back(std::move(face));
        }
    }

    return true;
}

inline bool rotate_cycle_to_halfedge(const std::vector<size_t> &cycle,
                                     size_t a,
                                     size_t b,
                                     std::vector<size_t> &rotated,
                                     std::string *diag){
    if(cycle.size() < 2U){
        return cdt_fail(diag, "Constraint cavity face reconstruction produced a degenerate cycle.");
    }

    for(size_t i = 0; i < cycle.size(); ++i){
        if((cycle.at(i) != a) || (cycle.at((i + 1U) % cycle.size()) != b)){
            continue;
        }

        rotated.clear();
        rotated.reserve(cycle.size());
        for(size_t j = 0; j < cycle.size(); ++j){
            rotated.push_back(cycle.at((i + j) % cycle.size()));
        }
        return true;
    }

    return cdt_fail(diag, "Constraint cavity face reconstruction could not rotate a face to the requested constrained halfedge.");
}

inline std::string format_cycle(const std::vector<size_t> &cycle){
    std::ostringstream out;
    out << "[";
    for(size_t i = 0; i < cycle.size(); ++i){
        if(i != 0U){
            out << ", ";
        }
        out << cycle.at(i);
    }
    out << "]";
    return out.str();
}

inline bool split_cycle_across_constrained_edge(const std::vector<size_t> &cycle,
                                                size_t a,
                                                size_t b,
                                                std::vector<size_t> &first,
                                                std::vector<size_t> &second){
    first.clear();
    second.clear();

    const auto normalized = normalize_cycle_vertices(cycle);
    if(normalized.size() < 4U){
        return false;
    }

    std::vector<size_t> oriented;
    oriented.reserve(normalized.size());
    for(size_t start = 0; start < normalized.size(); ++start){
        if((normalized.at(start) != a) || (normalized.at((start + 1U) % normalized.size()) != b)){
            continue;
        }
        for(size_t i = 0; i < normalized.size(); ++i){
            oriented.push_back(normalized.at((start + i) % normalized.size()));
        }
        break;
    }
    if(oriented.empty()){
        return false;
    }

    for(size_t i = 2U; (i + 1U) < oriented.size(); ++i){
        if((oriented.at(i) != b) || (oriented.at(i + 1U) != a)){
            continue;
        }

        first = { a, b };
        first.insert(first.end(),
                     oriented.begin() + static_cast<std::ptrdiff_t>(2U),
                     oriented.begin() + static_cast<std::ptrdiff_t>(i));

        second = { b, a };
        second.insert(second.end(),
                      oriented.begin() + static_cast<std::ptrdiff_t>(i + 2U),
                      oriented.end());
        return (first.size() >= 3U) && (second.size() >= 3U);
    }

    return false;
}

inline std::vector<size_t> normalize_cycle_vertices(const std::vector<size_t> &cycle){
    std::vector<size_t> normalized;
    normalized.reserve(cycle.size());
    for(const auto vertex : cycle){
        if(normalized.empty() || (normalized.back() != vertex)){
            normalized.push_back(vertex);
        }
    }
    if((normalized.size() >= 2U) && (normalized.front() == normalized.back())){
        normalized.pop_back();
    }
    return normalized;
}

void split_pinched_cycle(const std::vector<size_t> &cycle,
                         std::vector<std::vector<size_t>> &pieces){
    const auto normalized = normalize_cycle_vertices(cycle);
    if(normalized.size() < 3U){
        return;
    }

    std::map<size_t, size_t> first_position;
    for(size_t i = 0; i < normalized.size(); ++i){
        const auto vertex = normalized.at(i);
        const auto inserted = first_position.emplace(vertex, i);
        if(inserted.second){
            continue;
        }

        const auto start = inserted.first->second;
        std::vector<size_t> first_piece(normalized.begin() + static_cast<std::ptrdiff_t>(start),
                                        normalized.begin() + static_cast<std::ptrdiff_t>(i));
        std::vector<size_t> second_piece;
        second_piece.reserve(normalized.size() - (i - start) + 1U);
        second_piece.insert(second_piece.end(),
                            normalized.begin(),
                            normalized.begin() + static_cast<std::ptrdiff_t>(start + 1U));
        second_piece.insert(second_piece.end(),
                            normalized.begin() + static_cast<std::ptrdiff_t>(i),
                            normalized.end());

        split_pinched_cycle(first_piece, pieces);
        split_pinched_cycle(second_piece, pieces);
        return;
    }

    pieces.push_back(normalized);
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
                                  std::vector<CDT_Triangle> &triangles,
                                  std::string *diag){
    boundary_edges.clear();
    triangles.clear();

    if(chain.size() < 2){
        return cdt_fail(diag, "Constraint cavity polygon has fewer than two vertices.");
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
            const auto o = orient_sign(verts.at(prev), verts.at(cur), verts.at(next));
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
            return cdt_fail(diag, "Ear-clipping failed because no valid ear could be found in the constraint cavity polygon.");
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

    const auto corner_turn = poly_sign * orient_sign(verts.at(prev), verts.at(cur), verts.at(next));
    if(corner_turn > 0){
        return (poly_sign * orient_sign(verts.at(cur), verts.at(other), verts.at(prev)) > 0)
            && (poly_sign * orient_sign(verts.at(other), verts.at(cur), verts.at(next)) > 0);
    }

    return !((poly_sign * orient_sign(verts.at(cur), verts.at(other), verts.at(next)) >= 0)
          && (poly_sign * orient_sign(verts.at(other), verts.at(cur), verts.at(prev)) >= 0));
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
                                    std::vector<CDT_Triangle> &triangles,
                                    std::string *diag){
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

            if(incircle_sign(verts.at(edge.a), verts.at(edge.b), verts.at(w), verts.at(x)) <= 0){
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
        return cdt_fail(diag, "Constraint cavity edge legalization exceeded the iteration guard.");
    }

    prune_triangles(verts, triangles);
    return true;
}

template <class T>
bool triangulate_pinched_polygon(const std::vector<vec2<T>> &verts,
                                 const std::vector<size_t> &polygon,
                                 std::vector<CDT_Triangle> &triangles,
                                 std::string *diag){
    triangles.clear();

    const auto normalized_polygon = normalize_cycle_vertices(polygon);
    std::vector<std::vector<size_t>> pieces;
    split_pinched_cycle(normalized_polygon, pieces);
    if(pieces.empty()){
        return cdt_fail(diag, "Constraint cavity polygon collapsed while splitting repeated boundary vertices.");
    }

    for(const auto &piece : pieces){
        if(piece.size() < 3U){
            continue;
        }

        std::set<CDT_Edge> boundary_edges;
        std::vector<CDT_Triangle> piece_triangles;
        if(!triangulate_polygon_ear_clip(verts, piece, boundary_edges, piece_triangles, diag)
        || !legalize_polygon_triangulation(verts, piece, boundary_edges, piece_triangles, diag)){
            return false;
        }
        triangles.insert(triangles.end(), piece_triangles.begin(), piece_triangles.end());
    }

    if((normalized_polygon.size() >= 2U)
    && !triangulation_has_edge(triangles, normalized_polygon.at(0), normalized_polygon.at(1))){
        const auto a = normalized_polygon.at(0);
        const auto b = normalized_polygon.at(1);
        std::map<size_t, size_t> counts;
        for(const auto &piece : pieces){
            for(const auto vertex : piece){
                if((vertex != a) && (vertex != b)){
                    ++counts[vertex];
                }
            }
        }

        size_t shared_bridge_vertex = CDT_INVALID_VERTEX_INDEX;
        for(const auto vertex : normalized_polygon){
            if((counts.count(vertex) != 0U) && (counts.at(vertex) > 1U)){
                shared_bridge_vertex = vertex;
                break;
            }
        }

        const auto append_bridge_triangle = [&](size_t bridge_vertex){
            CDT_Triangle bridge_triangle{};
            if(make_ccw_triangle(verts, a, b, bridge_vertex, bridge_triangle)){
                triangles.push_back(bridge_triangle);
                return true;
            }
            return false;
        };

        if(shared_bridge_vertex != CDT_INVALID_VERTEX_INDEX){
            append_bridge_triangle(shared_bridge_vertex);
        }else{
            for(const auto &piece : pieces){
                const bool has_a = std::find(piece.begin(), piece.end(), a) != piece.end();
                const bool has_b = std::find(piece.begin(), piece.end(), b) != piece.end();
                if((piece.size() < 3U) || (has_a == has_b)){
                    continue;
                }

                const auto endpoint = has_a ? a : b;
                std::vector<size_t> rotated;
                rotated.reserve(piece.size());
                const auto it = std::find(piece.begin(), piece.end(), endpoint);
                const auto offset = static_cast<size_t>(std::distance(piece.begin(), it));
                for(size_t i = 0; i < piece.size(); ++i){
                    rotated.push_back(piece.at((offset + i) % piece.size()));
                }
                append_bridge_triangle(rotated.at(1));
            }
        }
    }

    prune_triangles(verts, triangles);
    return !triangles.empty();
}

template <class T>
bool retriangulate_visible_face(const std::vector<vec2<T>> &verts,
                                size_t a,
                                size_t b,
                                std::vector<CDT_Triangle> &triangles,
                                std::string *diag){
    std::map<CDT_Edge, std::vector<size_t>> edge_to_triangles;
    std::vector<CDT_Edge> edges;
    edges.reserve(triangles.size() * 3U + 1U);
    for(size_t i = 0; i < triangles.size(); ++i){
        for(const auto &edge : triangle_edges(triangles.at(i))){
            auto &incident = edge_to_triangles[edge];
            if(incident.empty()){
                edges.push_back(edge);
            }
            incident.push_back(i);
        }
    }

    edges.push_back(make_edge(a, b));

    std::vector<std::vector<size_t>> faces;
    std::map<std::pair<size_t, size_t>, size_t> halfedge_to_face;
    if(!build_planar_edge_faces(verts, edges, faces, halfedge_to_face, diag)){
        return false;
    }

    const auto face_ab_it = halfedge_to_face.find(std::make_pair(a, b));
    const auto face_ba_it = halfedge_to_face.find(std::make_pair(b, a));
    if((face_ab_it == halfedge_to_face.end()) || (face_ba_it == halfedge_to_face.end())){
        return cdt_fail(diag, "Visible-face retriangulation could not identify both halfedge faces incident to the constrained segment.");
    }

    std::vector<size_t> cycle_ab;
    std::vector<size_t> cycle_ba;
    if(!rotate_cycle_to_halfedge(faces.at(face_ab_it->second), a, b, cycle_ab, diag)
    || !rotate_cycle_to_halfedge(faces.at(face_ba_it->second), b, a, cycle_ba, diag)){
        return false;
    }

    std::vector<size_t> poly0;
    std::vector<size_t> poly1;
    if(!split_cycle_across_constrained_edge(cycle_ab, a, b, poly0, poly1)
    && !split_cycle_across_constrained_edge(cycle_ba, b, a, poly0, poly1)){
        return false;
    }

    std::vector<CDT_Triangle> retained;
    retained.reserve(triangles.size());
    for(const auto &tri : triangles){
        const auto &pa = verts.at(tri.a);
        const auto &pb = verts.at(tri.b);
        const auto &pc = verts.at(tri.c);
        const vec2<T> centroid((pa.x + pb.x + pc.x) / static_cast<T>(3),
                               (pa.y + pb.y + pc.y) / static_cast<T>(3));
        const auto in_poly0 = point_in_polygon_or_on_boundary(verts, poly0, centroid);
        const auto in_poly1 = point_in_polygon_or_on_boundary(verts, poly1, centroid);
        if(!(in_poly0 || in_poly1)){
            retained.push_back(tri);
        }
    }

    const auto append_polygon = [&](const std::vector<size_t> &polygon,
                                    std::vector<CDT_Triangle> &out) -> bool {
        std::set<CDT_Edge> boundary_edges;
        std::vector<CDT_Triangle> polygon_triangles;
        if(!triangulate_polygon_ear_clip(verts, polygon, boundary_edges, polygon_triangles, diag)
        || !legalize_polygon_triangulation(verts, polygon, boundary_edges, polygon_triangles, diag)){
            return false;
        }
        out.insert(out.end(), polygon_triangles.begin(), polygon_triangles.end());
        return true;
    };

    std::vector<CDT_Triangle> replacement;
    if(!append_polygon(poly0, replacement)
    || !append_polygon(poly1, replacement)){
        return false;
    }

    retained.insert(retained.end(), replacement.begin(), replacement.end());
    prune_triangles(verts, retained);
    if(!triangulation_has_edge(retained, a, b)){
        return cdt_fail(diag, "Visible-face retriangulation did not recover constrained edge ("
                             + std::to_string(a) + ", " + std::to_string(b) + ").");
    }
    triangles.swap(retained);
    return true;
}

template <class T>
bool retriangulate_boundary_strip(const std::vector<vec2<T>> &verts,
                                  size_t a,
                                  size_t b,
                                  std::vector<CDT_Triangle> &triangles,
                                  std::string *diag){
    std::map<CDT_Edge, std::vector<size_t>> edge_to_triangles;
    for(size_t i = 0; i < triangles.size(); ++i){
        for(const auto &edge : triangle_edges(triangles.at(i))){
            edge_to_triangles[edge].push_back(i);
        }
    }

    std::vector<CDT_Edge> boundary_edges_list;
    boundary_edges_list.reserve(edge_to_triangles.size() + 1U);
    for(const auto &[edge, incident] : edge_to_triangles){
        if(incident.size() != 1){
            continue;
        }
        boundary_edges_list.push_back(edge);
    }

    boundary_edges_list.push_back(make_edge(a, b));

    std::vector<std::vector<size_t>> boundary_faces;
    std::map<std::pair<size_t, size_t>, size_t> halfedge_to_face;
    if(!build_planar_edge_faces(verts, boundary_edges_list, boundary_faces, halfedge_to_face, diag)){
        return false;
    }

    const auto face_ab_it = halfedge_to_face.find(std::make_pair(a, b));
    const auto face_ba_it = halfedge_to_face.find(std::make_pair(b, a));
    if((face_ab_it == halfedge_to_face.end()) || (face_ba_it == halfedge_to_face.end())){
        return cdt_fail(diag, "Constraint boundary-strip retriangulation could not identify both boundary faces incident to the constrained segment.");
    }

    std::vector<size_t> cycle0;
    std::vector<size_t> cycle1;
    if(!rotate_cycle_to_halfedge(boundary_faces.at(face_ab_it->second), a, b, cycle0, diag)
    || !rotate_cycle_to_halfedge(boundary_faces.at(face_ba_it->second), b, a, cycle1, diag)){
        return false;
    }

    const auto cycle_to_boundary_chain = [](const std::vector<size_t> &cycle) {
        std::vector<size_t> chain;
        chain.reserve(cycle.size());
        chain.push_back(cycle.front());
        for(size_t i = cycle.size(); i > 2U; --i){
            chain.push_back(cycle.at(i - 1U));
        }
        chain.push_back(cycle.at(1));
        return chain;
    };

    const auto chain0 = cycle_to_boundary_chain(cycle0);
    const auto chain1 = cycle_to_boundary_chain(cycle1);
    const auto choose_chain = [&verts](const std::vector<size_t> &lhs,
                                       const std::vector<size_t> &rhs) -> const std::vector<size_t>& {
        if(lhs.size() < 3U){
            return rhs;
        }
        if(rhs.size() < 3U){
            return lhs;
        }
        const auto lhs_area = std::abs(polygon_signed_area_ld(verts, lhs));
        const auto rhs_area = std::abs(polygon_signed_area_ld(verts, rhs));
        return (lhs_area <= rhs_area) ? lhs : rhs;
    };
    const auto &boundary_chain = choose_chain(chain0, chain1);
    if(boundary_chain.size() < 3U){
        return cdt_fail(diag, "Constraint boundary-strip retriangulation found a degenerate boundary chain. chain0="
                             + format_cycle(chain0) + " chain1=" + format_cycle(chain1));
    }

    std::set<size_t> removed;
    std::vector<size_t> strip_polygon;
    strip_polygon.reserve(boundary_chain.size() * 2U);
    strip_polygon.push_back(boundary_chain.front());
    for(size_t i = 0; (i + 1U) < boundary_chain.size(); ++i){
        const auto edge = make_edge(boundary_chain.at(i), boundary_chain.at(i + 1U));
        const auto incident_it = edge_to_triangles.find(edge);
        if((incident_it == edge_to_triangles.end()) || (incident_it->second.size() != 1U)){
            return cdt_fail(diag, "Constraint boundary-strip retriangulation lost a unique incident triangle on the triangulation boundary.");
        }
        const auto tri_idx = incident_it->second.front();
        removed.insert(tri_idx);
        strip_polygon.push_back(opposite_vertex(triangles.at(tri_idx), edge));
        strip_polygon.push_back(boundary_chain.at(i + 1U));
    }

    std::vector<size_t> simplified;
    simplified.reserve(strip_polygon.size());
    for(const auto vertex : strip_polygon){
        if(simplified.empty() || (simplified.back() != vertex)){
            simplified.push_back(vertex);
        }
    }
    if((simplified.size() >= 2U) && (simplified.front() == simplified.back())){
        simplified.pop_back();
    }
    bool changed = true;
    while(changed && (simplified.size() >= 3U)){
        changed = false;
        for(size_t i = 0; i < simplified.size(); ++i){
            const auto prev = simplified.at((i + simplified.size() - 1U) % simplified.size());
            const auto cur = simplified.at(i);
            const auto next = simplified.at((i + 1U) % simplified.size());
            const bool removable = (cur != a) && (cur != b);
            if(removable
            && ((prev == cur) || (cur == next)
            || (orient_sign(verts.at(prev), verts.at(cur), verts.at(next)) == 0))){
                simplified.erase(simplified.begin() + static_cast<std::ptrdiff_t>(i));
                changed = true;
                break;
            }
        }
    }
    if((std::find(simplified.begin(), simplified.end(), a) == simplified.end())
    || (std::find(simplified.begin(), simplified.end(), b) == simplified.end())){
        return cdt_fail(diag, "Constraint boundary-strip retriangulation removed a constrained endpoint from the replacement polygon.");
    }
    if(simplified.size() < 3U){
        return cdt_fail(diag, "Constraint boundary-strip retriangulation collapsed the replacement polygon below three vertices.");
    }

    std::vector<CDT_Triangle> retained;
    retained.reserve(triangles.size());
    for(size_t i = 0; i < triangles.size(); ++i){
        if(removed.count(i) == 0U){
            retained.push_back(triangles.at(i));
        }
    }

    std::set<CDT_Edge> boundary_edges;
    std::vector<CDT_Triangle> strip_triangles;
    if(!triangulate_polygon_ear_clip(verts, simplified, boundary_edges, strip_triangles, diag)
    || !legalize_polygon_triangulation(verts, simplified, boundary_edges, strip_triangles, diag)){
        return false;
    }

    retained.insert(retained.end(), strip_triangles.begin(), strip_triangles.end());
    prune_triangles(verts, retained);
    triangles.swap(retained);
    return true;
}

template <class T>
bool constrain_edge(const std::vector<vec2<T>> &verts,
                    size_t a,
                    size_t b,
                    std::vector<CDT_Triangle> &triangles,
                    std::string *diag){
    // Each retry either succeeds or replaces a boundary strip with a triangulation that exposes a
    // smaller cavity for the requested segment. A guard tied to the current triangle count prevents
    // accidental non-termination if a future change breaks that monotone progress.
    for(size_t attempt = 0; attempt < (triangles.size() + CDT_LEGALIZATION_GUARD_BIAS); ++attempt){
        if(triangulation_has_edge(triangles, a, b)){
            return true;
        }

        std::map<CDT_Edge, std::vector<size_t>> edge_to_triangles;
        for(size_t i = 0; i < triangles.size(); ++i){
            for(const auto &edge : triangle_edges(triangles.at(i))){
                edge_to_triangles[edge].push_back(i);
            }
        }

        bool flipped_crossing_edge = false;
        for(const auto &[edge, incident] : edge_to_triangles){
            if((incident.size() != 2U)
            || !segments_intersect_beyond_shared_endpoints(verts.at(a), verts.at(b),
                                                           verts.at(edge.a), verts.at(edge.b))){
                continue;
            }

            const auto t0 = incident.at(0);
            const auto t1 = incident.at(1);
            const auto w = opposite_vertex(triangles.at(t0), edge);
            const auto x = opposite_vertex(triangles.at(t1), edge);
            if(segments_intersect_beyond_shared_endpoints(verts.at(a), verts.at(b),
                                                          verts.at(w), verts.at(x))){
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
            prune_triangles(verts, triangles);
            flipped_crossing_edge = true;
            break;
        }
        if(flipped_crossing_edge){
            continue;
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
            if(retriangulate_visible_face(verts, a, b, triangles, diag)){
                continue;
            }
            if(!retriangulate_boundary_strip(verts, a, b, triangles, diag)){
                return cdt_fail(diag, "No triangles crossed constrained segment (" + std::to_string(a)
                                     + ", " + std::to_string(b) + "), but the segment was not present in the triangulation.");
            }
            continue;
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

        std::vector<CDT_Edge> cavity_edges;
        cavity_edges.reserve(boundary_counts.size() + 1U);
        for(const auto &[edge, count] : boundary_counts){
            if(count != 1){
                continue;
            }
            cavity_edges.push_back(edge);
        }

        const auto constrained_edge = make_edge(a, b);
        cavity_edges.push_back(constrained_edge);

        std::vector<std::vector<size_t>> cavity_faces;
        std::map<std::pair<size_t, size_t>, size_t> halfedge_to_face;
        if(!build_planar_edge_faces(verts, cavity_edges, cavity_faces, halfedge_to_face, diag)){
            return false;
        }

        std::vector<size_t> chain0;
        std::vector<size_t> chain1;
        const auto face0_it = halfedge_to_face.find(std::make_pair(a, b));
        const auto face1_it = halfedge_to_face.find(std::make_pair(b, a));
        if((face0_it == halfedge_to_face.end()) || (face1_it == halfedge_to_face.end())){
            return cdt_fail(diag, "Constraint cavity for segment (" + std::to_string(a) + ", " + std::to_string(b)
                                 + ") could not identify both faces incident to the constrained segment.");
        }
        if(!rotate_cycle_to_halfedge(cavity_faces.at(face0_it->second), a, b, chain0, diag)
        || !rotate_cycle_to_halfedge(cavity_faces.at(face1_it->second), b, a, chain1, diag)){
            return false;
        }
        std::vector<CDT_Triangle> tris0;
        std::vector<CDT_Triangle> tris1;
        if(!triangulate_pinched_polygon(verts, chain0, tris0, diag)){
            return cdt_fail(diag, "Constraint cavity polygon for segment (" + std::to_string(a) + ", "
                                  + std::to_string(b) + ") could not be ear-clipped on one side: "
                                  + format_cycle(chain0));
        }
        if(!triangulate_pinched_polygon(verts, chain1, tris1, diag)){
            return cdt_fail(diag, "Constraint cavity polygon for segment (" + std::to_string(a) + ", "
                                  + std::to_string(b) + ") could not be ear-clipped on the opposite side: "
                                  + format_cycle(chain1));
        }

        retained.insert(retained.end(), tris0.begin(), tris0.end());
        retained.insert(retained.end(), tris1.begin(), tris1.end());
        prune_triangles(verts, retained);
        triangles.swap(retained);
    }

    return cdt_fail(diag, "Constraint insertion for segment (" + std::to_string(a) + ", "
                         + std::to_string(b) + ") exceeded the insertion retry guard.");
}

} // namespace


template <class T, class I>
fv_surface_mesh<T, I>
Constrained_Delaunay_Triangulation_2(const std::vector<vec2<T>> &verts,
                                     const std::vector<std::vector<I>> &edges){
    fv_surface_mesh<T, I> mesh;
    if(verts.empty() && edges.empty()){
        YLOGDEBUG("Constrained Delaunay triangulation received empty input");
        return mesh;
    }

    size_t first_nonfinite_index = verts.size();
    for(size_t i = 0; i < verts.size(); ++i){
        if(!is_finite_2d(verts.at(i))){
            first_nonfinite_index = i;
            break;
        }
    }
    if(first_nonfinite_index != verts.size()){
        YLOGWARN("Refusing constrained Delaunay triangulation because vertex " << first_nonfinite_index
                 << " is not finite: (" << verts.at(first_nonfinite_index).x
                 << ", " << verts.at(first_nonfinite_index).y << ")");
        throw std::invalid_argument("Constrained Delaunay triangulation requires all vertex coordinates to be finite.");
    }

    size_t duplicate_vertices = 0;
    {
        // TODO: would it be better to hash/index here so that all verts **within some eps** are treated as duplicate?
        // Maybe OK to just ask the user to provide a problem-specific eps??
        std::map<std::pair<T, T>, size_t> vertex_counts;
        for(const auto &vert : verts){
            ++vertex_counts[std::make_pair(vert.x, vert.y)];
        }
        for(const auto &entry : vertex_counts){
            const size_t count = entry.second;
            if(count > 1){
                duplicate_vertices += (count * (count - 1)) / 2;
            }
        }
    }
    if(duplicate_vertices != 0){
        YLOGWARN("Constrained Delaunay triangulation received " << duplicate_vertices
                 << " pair(s) of duplicate vertices; degenerate duplicates can destabilize constraint insertion");
    }

    std::vector<std::pair<size_t, size_t>> constraints;
    std::string diag;
    if(!collect_user_constraints(verts, edges, constraints, &diag)){
        YLOGWARN(diag);
        throw std::invalid_argument(diag);
    }
    YLOGDEBUG("Constrained Delaunay triangulation input: vertices=" << verts.size()
              << ", constraints=" << constraints.size());

    mesh.vertices.reserve(verts.size());
    for(const auto &vert : verts){
        mesh.vertices.emplace_back(vert.x, vert.y, static_cast<T>(0));
    }

    const auto has_closed_region = [&constraints](){
        if(constraints.size() < 3){
            return false;
        }
        std::map<size_t, size_t> degree;
        for(const auto &[a, b] : constraints){
            ++degree[a];
            ++degree[b];
        }
        return std::all_of(degree.begin(), degree.end(),
                           [](const auto &entry){ return entry.second == 2; });
    }();

    if(has_closed_region && !has_non_collinear_triplet(verts)){
        YLOGWARN("Refusing constrained Delaunay triangulation because the constrained region is degenerate");
        throw std::invalid_argument("Constrained Delaunay triangulation requires at least one non-collinear triplet of vertices for a closed constrained region.");
    }

    auto triangles = build_delaunay_triangles(verts);
    if(has_closed_region && triangles.empty()){
        YLOGDEBUG("Initial unconstrained triangulation produced no triangles for a closed constrained region");
        throw std::runtime_error("Constrained Delaunay triangulation could not build an initial triangulation for the provided constrained region.");
    }
    for(const auto &[a, b] : constraints){
        if(!triangles.empty() && !constrain_edge(verts, a, b, triangles, &diag)){
            YLOGWARN(diag);
            throw std::runtime_error(diag);
        }
    }

    prune_triangles(verts, triangles);
    if(!retain_triangles_in_constraint_faces(verts, constraints, triangles, &diag)){
        YLOGWARN(diag);
        throw std::runtime_error(diag);
    }
    prune_triangles(verts, triangles);

    if(has_closed_region && triangles.empty()){
        const auto msg = "Constrained Delaunay triangulation produced no interior triangles for a closed constrained region.";
        YLOGWARN(msg);
        throw std::runtime_error(msg);
    }

    for(const auto &tri : triangles){
        mesh.faces.push_back({ static_cast<I>(tri.a), static_cast<I>(tri.b), static_cast<I>(tri.c) });
    }

    YLOGDEBUG("Constrained Delaunay triangulation produced " << mesh.faces.size() << " triangle(s)");
    return mesh;
}

#ifndef YGOR_MATH_CONSTRAINED_DELAUNAY_DISABLE_ALL_SPECIALIZATIONS
    template fv_surface_mesh<float , uint32_t> Constrained_Delaunay_Triangulation_2(const std::vector<vec2<float >> &, const std::vector<std::vector<uint32_t>> &);
    template fv_surface_mesh<float , uint64_t> Constrained_Delaunay_Triangulation_2(const std::vector<vec2<float >> &, const std::vector<std::vector<uint64_t>> &);

    template fv_surface_mesh<double, uint32_t> Constrained_Delaunay_Triangulation_2(const std::vector<vec2<double>> &, const std::vector<std::vector<uint32_t>> &);
    template fv_surface_mesh<double, uint64_t> Constrained_Delaunay_Triangulation_2(const std::vector<vec2<double>> &, const std::vector<std::vector<uint64_t>> &);
#endif
