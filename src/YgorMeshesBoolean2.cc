//YgorMeshesBoolean2.cc - Written by hal clark in 2026.

#include <algorithm>
#include <any>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorIndex.h"
#include "YgorIndexRTree.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMathConstrainedDelaunay.h"
#include "YgorMeshesBoolean2.h"
#include "YgorMeshesOrient.h"

namespace {

enum class boolean_face_relation : uint8_t {
    Outside,
    Inside,
    CoplanarSame,
    CoplanarOpposite
};

enum class triangle_intersection_kind : uint8_t {
    None,
    Segment,
    CoplanarPolygon
};

template <class T, class I>
struct prepared_mesh {
    fv_surface_mesh<T, I> mesh;
    std::unique_ptr<rtree<T>> face_index;
    index_bbox<T> bounds;
};

template <class T>
struct triangle_intersection_result {
    triangle_intersection_kind kind = triangle_intersection_kind::None;
    std::vector<vec3<T>> points;
};

template <class T>
struct coplanar_overlap_region {
    std::vector<vec3<T>> polygon;
    vec3<T> other_normal;
};

template <class T>
struct face_arrangement {
    std::array<vec3<T>, 3> tri;
    std::vector<vec3<T>> points;
    std::set<std::pair<size_t, size_t>> segments;
    std::vector<coplanar_overlap_region<T>> overlaps;
};

template <class T>
struct split_triangle {
    std::array<vec3<T>, 3> verts;
    boolean_face_relation relation = boolean_face_relation::Outside;
};

template <class I>
std::pair<I, I>
make_undirected_edge(I a,
                     I b){
    if(b < a){
        std::swap(a, b);
    }
    return std::make_pair(a, b);
}

template <class T>
std::pair<size_t, size_t>
make_undirected_edge(size_t a,
                     size_t b){
    if(b < a){
        std::swap(a, b);
    }
    return std::make_pair(a, b);
}

template <class T, class I>
index_bbox<T>
triangle_bbox(const fv_surface_mesh<T, I> &mesh,
              size_t face_idx){
    const auto &f = mesh.faces.at(face_idx);
    const auto &a = mesh.vertices.at(f.at(0));
    const auto &b = mesh.vertices.at(f.at(1));
    const auto &c = mesh.vertices.at(f.at(2));

    const vec3<T> bb_min(std::min({a.x, b.x, c.x}),
                         std::min({a.y, b.y, c.y}),
                         std::min({a.z, b.z, c.z}));
    const vec3<T> bb_max(std::max({a.x, b.x, c.x}),
                         std::max({a.y, b.y, c.y}),
                         std::max({a.z, b.z, c.z}));
    return index_bbox<T>(bb_min, bb_max);
}

template <class T, class I>
index_bbox<T>
mesh_bbox(const fv_surface_mesh<T, I> &mesh){
    if(mesh.vertices.empty()){
        return index_bbox<T>();
    }

    vec3<T> bb_min(std::numeric_limits<T>::max(),
                   std::numeric_limits<T>::max(),
                   std::numeric_limits<T>::max());
    vec3<T> bb_max(std::numeric_limits<T>::lowest(),
                   std::numeric_limits<T>::lowest(),
                   std::numeric_limits<T>::lowest());

    for(const auto &v : mesh.vertices){
        bb_min.x = std::min(bb_min.x, v.x);
        bb_min.y = std::min(bb_min.y, v.y);
        bb_min.z = std::min(bb_min.z, v.z);
        bb_max.x = std::max(bb_max.x, v.x);
        bb_max.y = std::max(bb_max.y, v.y);
        bb_max.z = std::max(bb_max.z, v.z);
    }
    return index_bbox<T>(bb_min, bb_max);
}

template <class T>
T
mesh_coord_eps(const index_bbox<T> &bounds){
    const auto extent = bounds.max - bounds.min;
    const auto scale = std::max({extent.x, extent.y, extent.z, static_cast<T>(1)});
    return std::sqrt(std::numeric_limits<T>::epsilon()) * scale;
}

template <class T, class I>
void
validate_closed_triangular_mesh(const fv_surface_mesh<T, I> &mesh,
                                const std::string &name){
    if(mesh.faces.empty()){
        return;
    }

    if((mesh.faces.size() * 3ULL) % 2ULL != 0ULL){
        throw std::invalid_argument(name + " violates the 3F = 2E handshake invariant.");
    }

    std::map<std::pair<I, I>, size_t> edge_counts;
    for(const auto &v : mesh.vertices){
        if(!v.isfinite()){
            throw std::invalid_argument(name + " contains a non-finite vertex.");
        }
    }

    for(size_t face_idx = 0; face_idx < mesh.faces.size(); ++face_idx){
        const auto &face = mesh.faces.at(face_idx);
        if(face.size() != 3UL){
            throw std::invalid_argument(name + " must contain only triangular faces.");
        }
        for(const auto vi : face){
            if(vi >= mesh.vertices.size()){
                throw std::invalid_argument(name + " contains an out-of-range face index.");
            }
        }
        if((face.at(0) == face.at(1)) || (face.at(1) == face.at(2)) || (face.at(2) == face.at(0))){
            throw std::invalid_argument(name + " contains a degenerate triangle.");
        }

        edge_counts[make_undirected_edge(face.at(0), face.at(1))] += 1UL;
        edge_counts[make_undirected_edge(face.at(1), face.at(2))] += 1UL;
        edge_counts[make_undirected_edge(face.at(2), face.at(0))] += 1UL;
    }

    const auto expected_edges = (mesh.faces.size() * 3ULL) / 2ULL;
    if(edge_counts.size() != expected_edges){
        throw std::invalid_argument(name + " is not a closed manifold mesh.");
    }

    for(const auto &ep : edge_counts){
        if(ep.second != 2UL){
            throw std::invalid_argument(name + " contains a boundary or non-manifold edge.");
        }
    }
}

template <class T>
int
dominant_axis(const vec3<T> &n){
    const auto ax = std::abs(n.x);
    const auto ay = std::abs(n.y);
    const auto az = std::abs(n.z);
    if((ax >= ay) && (ax >= az)) return 0;
    if(ay >= az) return 1;
    return 2;
}

template <class T>
vec2<T>
project_drop_axis(const vec3<T> &v,
                  int axis){
    if(axis == 0) return vec2<T>(v.y, v.z);
    if(axis == 1) return vec2<T>(v.x, v.z);
    return vec2<T>(v.x, v.y);
}

template <class T>
vec3<T>
triangle_normal(const vec3<T> &a,
                const vec3<T> &b,
                const vec3<T> &c){
    return (b - a).Cross(c - a);
}

template <class T>
vec3<T>
triangle_centroid(const std::array<vec3<T>, 3> &tri){
    return (tri.at(0) + tri.at(1) + tri.at(2)) / static_cast<T>(3);
}

template <class T>
vec3<T>
project_point_to_segment(const vec3<T> &p,
                         const vec3<T> &a,
                         const vec3<T> &b){
    const auto ab = b - a;
    const auto ab_sq = ab.sq_length();
    if(!(ab_sq > static_cast<T>(0))){
        return a;
    }
    auto t = (p - a).Dot(ab) / ab_sq;
    t = std::max(static_cast<T>(0), std::min(static_cast<T>(1), t));
    return a + ab * t;
}

template <class T>
bool
point_near_segment(const vec3<T> &p,
                   const vec3<T> &a,
                   const vec3<T> &b,
                   T eps){
    const auto proj = project_point_to_segment(p, a, b);
    if(proj.distance(p) > eps){
        return false;
    }
    const auto ab = b - a;
    const auto ap = proj - a;
    const auto bp = proj - b;
    return (ap.Dot(ab) >= -eps) && (bp.Dot(ab) <= eps);
}

template <class T>
bool
point_on_triangle(const vec3<T> &p,
                  const vec3<T> &a,
                  const vec3<T> &b,
                  const vec3<T> &c){
    if(orient_sign(a, b, c, p) != 0){
        return false;
    }
    const auto axis = dominant_axis(triangle_normal(a, b, c));
    return point_in_triangle_or_on_boundary(project_drop_axis(p, axis),
                                            project_drop_axis(a, axis),
                                            project_drop_axis(b, axis),
                                            project_drop_axis(c, axis));
}

template <class T>
bool
segment_intersects_triangle_interior(const vec3<T> &p,
                                     const vec3<T> &q,
                                     const vec3<T> &a,
                                     const vec3<T> &b,
                                     const vec3<T> &c){
    const auto s_plane_p = orient_sign(a, b, c, p);
    const auto s_plane_q = orient_sign(a, b, c, q);
    if((s_plane_p == 0) || (s_plane_q == 0)){
        return false;
    }
    if((s_plane_p > 0) == (s_plane_q > 0)){
        return false;
    }

    const auto s1 = orient_sign(p, q, a, b);
    const auto s2 = orient_sign(p, q, b, c);
    const auto s3 = orient_sign(p, q, c, a);
    if((s1 == 0) || (s2 == 0) || (s3 == 0)){
        return false;
    }

    const bool all_pos = (s1 > 0) && (s2 > 0) && (s3 > 0);
    const bool all_neg = (s1 < 0) && (s2 < 0) && (s3 < 0);
    return all_pos || all_neg;
}

template <class T, class I>
bool
point_on_mesh_boundary(const vec3<T> &p,
                       const prepared_mesh<T, I> &prep){
    const auto candidates = prep.face_index->search(index_bbox<T>(p, p));
    for(const auto &entry : candidates){
        const auto face_idx = std::any_cast<size_t>(entry.aux_data);
        const auto &face = prep.mesh.faces.at(face_idx);
        if(point_on_triangle(p,
                             prep.mesh.vertices.at(face.at(0)),
                             prep.mesh.vertices.at(face.at(1)),
                             prep.mesh.vertices.at(face.at(2)))){
            return true;
        }
    }
    return false;
}

template <class T, class I>
bool
cast_parity_ray(const vec3<T> &p,
                const vec3<T> &q,
                const prepared_mesh<T, I> &prep){
    const auto candidates = prep.face_index->search(index_bbox<T>(p, q));
    int64_t crossings = 0;
    for(const auto &entry : candidates){
        const auto face_idx = std::any_cast<size_t>(entry.aux_data);
        const auto &face = prep.mesh.faces.at(face_idx);
        const auto &a = prep.mesh.vertices.at(face.at(0));
        const auto &b = prep.mesh.vertices.at(face.at(1));
        const auto &c = prep.mesh.vertices.at(face.at(2));
        if(segment_intersects_triangle_interior(p, q, a, b, c)){
            ++crossings;
        }
    }
    return (crossings % 2LL) != 0LL;
}

template <class T, class I>
bool
point_inside_mesh(const vec3<T> &p,
                  const prepared_mesh<T, I> &prep,
                  T far_distance){
    if(prep.mesh.faces.empty()){
        return false;
    }
    if(point_on_mesh_boundary(p, prep)){
        return true;
    }

    const std::array<vec3<T>, 3> ray_dirs = {{
        vec3<T>(static_cast<T>(1.0), static_cast<T>(0.371), static_cast<T>(0.529)),
        vec3<T>(static_cast<T>(0.419), static_cast<T>(1.0), static_cast<T>(0.113)),
        vec3<T>(static_cast<T>(0.137), static_cast<T>(0.271), static_cast<T>(1.0))
    }};

    int inside_votes = 0;
    for(const auto &dir_raw : ray_dirs){
        const auto dir = dir_raw.unit();
        const auto q = p + dir * far_distance;
        if(cast_parity_ray(p, q, prep)){
            ++inside_votes;
        }
    }
    return inside_votes >= 2;
}

template <class T, class I>
prepared_mesh<T, I>
prepare_mesh(const fv_surface_mesh<T, I> &mesh,
             const std::string &name,
             T eps){
    validate_closed_triangular_mesh(mesh, name);

    prepared_mesh<T, I> out;
    out.mesh = mesh;
    out.mesh.convert_to_triangles();
    out.mesh.remove_degenerate_faces();
    if(!OrientFaces(out.mesh, eps)){
        throw std::runtime_error(name + " could not be oriented consistently.");
    }
    out.mesh.recreate_involved_face_index();
    out.bounds = mesh_bbox(out.mesh);
    out.face_index = std::make_unique<rtree<T>>();
    for(size_t face_idx = 0; face_idx < out.mesh.faces.size(); ++face_idx){
        out.face_index->insert(triangle_bbox(out.mesh, face_idx), std::make_any<size_t>(face_idx));
    }
    return out;
}

template <class T>
T
triangle_bounding_radius(const vec3<T> &a,
                         const vec3<T> &b,
                         const vec3<T> &c){
    const auto centre = (a + b + c) / static_cast<T>(3);
    return std::max({centre.distance(a), centre.distance(b), centre.distance(c)});
}

template <class T>
bool
passes_structural_proximity_check(const vec3<T> &a0,
                                  const vec3<T> &a1,
                                  const vec3<T> &a2,
                                  const vec3<T> &b0,
                                  const vec3<T> &b1,
                                  const vec3<T> &b2,
                                  T eps){
    const auto ca = (a0 + a1 + a2) / static_cast<T>(3);
    const auto cb = (b0 + b1 + b2) / static_cast<T>(3);
    const auto ra = triangle_bounding_radius(a0, a1, a2);
    const auto rb = triangle_bounding_radius(b0, b1, b2);
    return ca.distance(cb) <= (ra + rb + eps);
}

template <class T>
vec3<T>
snap_intersection_point(const vec3<T> &p,
                        const std::array<vec3<T>, 3> &tri_a,
                        const std::array<vec3<T>, 3> &tri_b,
                        T eps){
    for(const auto &v : tri_a){
        if(p.distance(v) <= eps){
            return v;
        }
    }
    for(const auto &v : tri_b){
        if(p.distance(v) <= eps){
            return v;
        }
    }

    auto out = p;
    const std::array<std::array<vec3<T>, 2>, 6> edges = {{
        {{tri_a.at(0), tri_a.at(1)}}, {{tri_a.at(1), tri_a.at(2)}}, {{tri_a.at(2), tri_a.at(0)}},
        {{tri_b.at(0), tri_b.at(1)}}, {{tri_b.at(1), tri_b.at(2)}}, {{tri_b.at(2), tri_b.at(0)}}
    }};
    for(const auto &edge : edges){
        const auto proj = project_point_to_segment(out, edge.at(0), edge.at(1));
        if(out.distance(proj) <= eps){
            out = proj;
        }
    }
    return out;
}

template <class T>
void
add_unique_point(std::vector<vec3<T>> &points,
                 const vec3<T> &p,
                 T eps){
    for(const auto &q : points){
        if(p.distance(q) <= eps){
            return;
        }
    }
    points.push_back(p);
}

template <class T>
vec3<T>
line_plane_intersection(const vec3<T> &p,
                        const vec3<T> &q,
                        const vec3<T> &plane_point,
                        const vec3<T> &plane_normal){
    const long double dx = static_cast<long double>(q.x - p.x);
    const long double dy = static_cast<long double>(q.y - p.y);
    const long double dz = static_cast<long double>(q.z - p.z);
    const long double nx = static_cast<long double>(plane_normal.x);
    const long double ny = static_cast<long double>(plane_normal.y);
    const long double nz = static_cast<long double>(plane_normal.z);
    const long double num = nx * static_cast<long double>(plane_point.x - p.x)
                          + ny * static_cast<long double>(plane_point.y - p.y)
                          + nz * static_cast<long double>(plane_point.z - p.z);
    const long double den = nx * dx + ny * dy + nz * dz;
    const long double t = (std::abs(den) > 0.0L) ? (num / den) : 0.0L;
    return vec3<T>(static_cast<T>(static_cast<long double>(p.x) + dx * t),
                   static_cast<T>(static_cast<long double>(p.y) + dy * t),
                   static_cast<T>(static_cast<long double>(p.z) + dz * t));
}

template <class T>
triangle_intersection_result<T>
compute_noncoplanar_intersection(const std::array<vec3<T>, 3> &tri_a,
                                 const std::array<vec3<T>, 3> &tri_b,
                                 T eps){
    triangle_intersection_result<T> out;

    const auto n_a = triangle_normal(tri_a.at(0), tri_a.at(1), tri_a.at(2));
    const auto n_b = triangle_normal(tri_b.at(0), tri_b.at(1), tri_b.at(2));
    const auto line_dir = n_a.Cross(n_b);
    if(line_dir.sq_length() <= eps * eps){
        return out;
    }

    const std::array<int, 3> sa = {{
        orient_sign(tri_b.at(0), tri_b.at(1), tri_b.at(2), tri_a.at(0)),
        orient_sign(tri_b.at(0), tri_b.at(1), tri_b.at(2), tri_a.at(1)),
        orient_sign(tri_b.at(0), tri_b.at(1), tri_b.at(2), tri_a.at(2))
    }};
    const std::array<int, 3> sb = {{
        orient_sign(tri_a.at(0), tri_a.at(1), tri_a.at(2), tri_b.at(0)),
        orient_sign(tri_a.at(0), tri_a.at(1), tri_a.at(2), tri_b.at(1)),
        orient_sign(tri_a.at(0), tri_a.at(1), tri_a.at(2), tri_b.at(2))
    }};

    auto plane_cut = [&](const std::array<vec3<T>, 3> &tri,
                         const std::array<int, 3> &signs,
                         const vec3<T> &plane_point,
                         const vec3<T> &plane_normal) -> std::vector<vec3<T>> {
        std::vector<vec3<T>> pts;
        for(size_t i = 0; i < 3UL; ++i){
            if(signs.at(i) == 0){
                add_unique_point(pts, tri.at(i), eps);
            }
        }
        for(size_t i = 0; i < 3UL; ++i){
            const auto j = (i + 1UL) % 3UL;
            const auto si = signs.at(i);
            const auto sj = signs.at(j);
            if((si == 0) && (sj == 0)){
                continue;
            }
            if((si > 0) != (sj > 0)){
                add_unique_point(pts,
                                 line_plane_intersection(tri.at(i), tri.at(j), plane_point, plane_normal),
                                 eps);
            }
        }
        return pts;
    };

    auto seg_a = plane_cut(tri_a, sa, tri_b.at(0), n_b);
    auto seg_b = plane_cut(tri_b, sb, tri_a.at(0), n_a);
    if((seg_a.size() < 2UL) || (seg_b.size() < 2UL)){
        return out;
    }

    const auto dir = line_dir.unit();
    const auto origin = seg_a.front();
    auto proj = [&](const vec3<T> &p) -> T {
        return (p - origin).Dot(dir);
    };

    const auto a_lo = std::min(proj(seg_a.at(0)), proj(seg_a.at(1)));
    const auto a_hi = std::max(proj(seg_a.at(0)), proj(seg_a.at(1)));
    const auto b_lo = std::min(proj(seg_b.at(0)), proj(seg_b.at(1)));
    const auto b_hi = std::max(proj(seg_b.at(0)), proj(seg_b.at(1)));
    const auto lo = std::max(a_lo, b_lo);
    const auto hi = std::min(a_hi, b_hi);
    if((hi - lo) <= eps){
        return out;
    }

    auto p0 = origin + dir * lo;
    auto p1 = origin + dir * hi;
    p0 = snap_intersection_point(p0, tri_a, tri_b, eps);
    p1 = snap_intersection_point(p1, tri_a, tri_b, eps);
    if(p0.distance(p1) <= eps){
        return out;
    }

    out.kind = triangle_intersection_kind::Segment;
    out.points = { p0, p1 };
    return out;
}

template <class T>
struct clip_poly_point {
    vec2<T> p2;
    vec3<T> p3;
};

template <class T>
clip_poly_point<T>
interpolate_clip_point(const clip_poly_point<T> &s,
                       const clip_poly_point<T> &e,
                       const vec2<T> &a,
                       const vec2<T> &b){
    const auto se = e.p2 - s.p2;
    const auto ab = b - a;
    const long double det = static_cast<long double>(se.x) * static_cast<long double>(ab.y)
                          - static_cast<long double>(se.y) * static_cast<long double>(ab.x);
    long double t = 0.0L;
    if(std::abs(det) > 0.0L){
        const long double rx = static_cast<long double>(a.x - s.p2.x);
        const long double ry = static_cast<long double>(a.y - s.p2.y);
        t = (rx * static_cast<long double>(ab.y) - ry * static_cast<long double>(ab.x)) / det;
    }
    const auto clamped_t = std::max(0.0L, std::min(1.0L, t));
    return clip_poly_point<T>{
        s.p2 + se * static_cast<T>(clamped_t),
        s.p3 + (e.p3 - s.p3) * static_cast<T>(clamped_t)
    };
}

template <class T>
std::vector<clip_poly_point<T>>
sutherland_hodgman_clip(std::vector<clip_poly_point<T>> subject,
                        const std::vector<clip_poly_point<T>> &clip_poly,
                        T eps){
    if(subject.empty() || clip_poly.size() < 3UL){
        return {};
    }

    auto inside = [&](const vec2<T> &p, const vec2<T> &a, const vec2<T> &b) -> bool {
        return orient_sign(a, b, p) >= 0;
    };

    for(size_t i = 0; i < clip_poly.size(); ++i){
        const auto &clip_a = clip_poly.at(i);
        const auto &clip_b = clip_poly.at((i + 1UL) % clip_poly.size());
        std::vector<clip_poly_point<T>> output;
        if(subject.empty()){
            break;
        }
        for(size_t j = 0; j < subject.size(); ++j){
            const auto &s = subject.at(j);
            const auto &e = subject.at((j + 1UL) % subject.size());
            const bool s_inside = inside(s.p2, clip_a.p2, clip_b.p2);
            const bool e_inside = inside(e.p2, clip_a.p2, clip_b.p2);
            if(s_inside && e_inside){
                output.push_back(e);
            }else if(s_inside && !e_inside){
                output.push_back(interpolate_clip_point(s, e, clip_a.p2, clip_b.p2));
            }else if(!s_inside && e_inside){
                output.push_back(interpolate_clip_point(s, e, clip_a.p2, clip_b.p2));
                output.push_back(e);
            }
        }

        std::vector<clip_poly_point<T>> deduped;
        for(const auto &pt : output){
            bool duplicate = false;
            for(const auto &existing : deduped){
                if(pt.p3.distance(existing.p3) <= eps){
                    duplicate = true;
                    break;
                }
            }
            if(!duplicate){
                deduped.push_back(pt);
            }
        }
        subject.swap(deduped);
    }
    return subject;
}

template <class T>
triangle_intersection_result<T>
compute_coplanar_intersection(const std::array<vec3<T>, 3> &tri_a,
                              const std::array<vec3<T>, 3> &tri_b,
                              T eps){
    triangle_intersection_result<T> out;
    const auto normal = triangle_normal(tri_a.at(0), tri_a.at(1), tri_a.at(2));
    const auto axis = dominant_axis(normal);

    std::vector<clip_poly_point<T>> subject;
    std::vector<clip_poly_point<T>> clip;
    subject.reserve(3UL);
    clip.reserve(3UL);
    for(const auto &p : tri_a){
        subject.push_back(clip_poly_point<T>{ project_drop_axis(p, axis), p });
    }
    for(const auto &p : tri_b){
        clip.push_back(clip_poly_point<T>{ project_drop_axis(p, axis), p });
    }

    auto ensure_ccw = [](std::vector<clip_poly_point<T>> &poly){
        long double area = 0.0L;
        for(size_t i = 0; i < poly.size(); ++i){
            const auto &a = poly.at(i).p2;
            const auto &b = poly.at((i + 1UL) % poly.size()).p2;
            area += static_cast<long double>(a.x) * static_cast<long double>(b.y)
                  - static_cast<long double>(b.x) * static_cast<long double>(a.y);
        }
        if(area < 0.0L){
            std::reverse(poly.begin(), poly.end());
        }
    };
    ensure_ccw(subject);
    ensure_ccw(clip);

    auto clipped = sutherland_hodgman_clip(subject, clip, eps);
    if(clipped.size() < 3UL){
        return out;
    }

    for(const auto &pt : clipped){
        add_unique_point(out.points, snap_intersection_point(pt.p3, tri_a, tri_b, eps), eps);
    }
    if(out.points.size() >= 3UL){
        out.kind = triangle_intersection_kind::CoplanarPolygon;
    }else{
        out.points.clear();
    }
    return out;
}

template <class T>
triangle_intersection_result<T>
intersect_triangles(const std::array<vec3<T>, 3> &tri_a,
                    const std::array<vec3<T>, 3> &tri_b,
                    T eps){
    const std::array<int, 3> sa = {{
        orient_sign(tri_b.at(0), tri_b.at(1), tri_b.at(2), tri_a.at(0)),
        orient_sign(tri_b.at(0), tri_b.at(1), tri_b.at(2), tri_a.at(1)),
        orient_sign(tri_b.at(0), tri_b.at(1), tri_b.at(2), tri_a.at(2))
    }};
    const std::array<int, 3> sb = {{
        orient_sign(tri_a.at(0), tri_a.at(1), tri_a.at(2), tri_b.at(0)),
        orient_sign(tri_a.at(0), tri_a.at(1), tri_a.at(2), tri_b.at(1)),
        orient_sign(tri_a.at(0), tri_a.at(1), tri_a.at(2), tri_b.at(2))
    }};

    auto all_positive = [](const std::array<int, 3> &s) -> bool {
        return (s.at(0) > 0) && (s.at(1) > 0) && (s.at(2) > 0);
    };
    auto all_negative = [](const std::array<int, 3> &s) -> bool {
        return (s.at(0) < 0) && (s.at(1) < 0) && (s.at(2) < 0);
    };
    auto all_zero = [](const std::array<int, 3> &s) -> bool {
        return (s.at(0) == 0) && (s.at(1) == 0) && (s.at(2) == 0);
    };

    if(all_positive(sa) || all_negative(sa) || all_positive(sb) || all_negative(sb)){
        return {};
    }
    if(all_zero(sa) && all_zero(sb)){
        return compute_coplanar_intersection(tri_a, tri_b, eps);
    }
    return compute_noncoplanar_intersection(tri_a, tri_b, eps);
}

template <class T>
size_t
arrangement_add_point(face_arrangement<T> &arr,
                      const vec3<T> &p,
                      T eps){
    for(size_t i = 0; i < arr.points.size(); ++i){
        if(arr.points.at(i).distance(p) <= eps){
            return i;
        }
    }
    arr.points.push_back(p);
    return arr.points.size() - 1UL;
}

template <class T>
void
arrangement_add_segment(face_arrangement<T> &arr,
                        const vec3<T> &a,
                        const vec3<T> &b,
                        T eps){
    if(a.distance(b) <= eps){
        return;
    }
    const auto ia = arrangement_add_point(arr, a, eps);
    const auto ib = arrangement_add_point(arr, b, eps);
    if(ia == ib){
        return;
    }
    arr.segments.insert(make_undirected_edge<T>(ia, ib));
}

template <class T>
face_arrangement<T>
make_face_arrangement(const std::array<vec3<T>, 3> &tri,
                      T eps){
    face_arrangement<T> out;
    out.tri = tri;
    out.points.assign(tri.begin(), tri.end());
    arrangement_add_segment(out, tri.at(0), tri.at(1), eps);
    arrangement_add_segment(out, tri.at(1), tri.at(2), eps);
    arrangement_add_segment(out, tri.at(2), tri.at(0), eps);
    return out;
}

template <class T>
void
finalize_coplanar_overlap_segments(face_arrangement<T> &arr,
                                   T eps){
    const auto axis = dominant_axis(triangle_normal(arr.tri.at(0), arr.tri.at(1), arr.tri.at(2)));
    for(const auto &region : arr.overlaps){
        for(const auto &p : region.polygon){
            arrangement_add_point(arr, p, eps);
        }
    }

    for(size_t region_idx = 0; region_idx < arr.overlaps.size(); ++region_idx){
        const auto &region = arr.overlaps.at(region_idx);
        if(region.polygon.size() < 3UL){
            continue;
        }
        for(size_t i = 0; i < region.polygon.size(); ++i){
            const auto &a = region.polygon.at(i);
            const auto &b = region.polygon.at((i + 1UL) % region.polygon.size());
            const auto midpoint = (a + b) / static_cast<T>(2);
            const auto midpoint2 = project_drop_axis(midpoint, axis);

            size_t containing_polygons = 0UL;
            for(const auto &other : arr.overlaps){
                std::vector<vec2<T>> poly2;
                poly2.reserve(other.polygon.size());
                for(const auto &p : other.polygon){
                    poly2.push_back(project_drop_axis(p, axis));
                }
                if((poly2.size() >= 3UL) && point_in_polygon_or_on_boundary(poly2, midpoint2)){
                    ++containing_polygons;
                }
            }
            if(containing_polygons <= 1UL){
                arrangement_add_segment(arr, a, b, eps);
            }
        }
    }
}

template <class T, class I>
void
build_face_arrangements(const prepared_mesh<T, I> &prep_a,
                        const prepared_mesh<T, I> &prep_b,
                        std::vector<face_arrangement<T>> &arr_a,
                        std::vector<face_arrangement<T>> &arr_b,
                        T eps){
    arr_a.clear();
    arr_b.clear();
    arr_a.reserve(prep_a.mesh.faces.size());
    arr_b.reserve(prep_b.mesh.faces.size());

    for(const auto &face : prep_a.mesh.faces){
        arr_a.push_back(make_face_arrangement<T>({{
            prep_a.mesh.vertices.at(face.at(0)),
            prep_a.mesh.vertices.at(face.at(1)),
            prep_a.mesh.vertices.at(face.at(2))
        }}, eps));
    }
    for(const auto &face : prep_b.mesh.faces){
        arr_b.push_back(make_face_arrangement<T>({{
            prep_b.mesh.vertices.at(face.at(0)),
            prep_b.mesh.vertices.at(face.at(1)),
            prep_b.mesh.vertices.at(face.at(2))
        }}, eps));
    }

    for(size_t face_a_idx = 0; face_a_idx < prep_a.mesh.faces.size(); ++face_a_idx){
        const auto &tri_a = arr_a.at(face_a_idx).tri;
        const auto candidates = prep_b.face_index->search(triangle_bbox(prep_a.mesh, face_a_idx));
        for(const auto &candidate : candidates){
            const auto face_b_idx = std::any_cast<size_t>(candidate.aux_data);
            const auto &tri_b = arr_b.at(face_b_idx).tri;
            if(!passes_structural_proximity_check(tri_a.at(0), tri_a.at(1), tri_a.at(2),
                                                  tri_b.at(0), tri_b.at(1), tri_b.at(2),
                                                  eps)){
                continue;
            }

            const auto isect = intersect_triangles(tri_a, tri_b, eps);
            if(isect.kind == triangle_intersection_kind::Segment){
                arrangement_add_segment(arr_a.at(face_a_idx), isect.points.at(0), isect.points.at(1), eps);
                arrangement_add_segment(arr_b.at(face_b_idx), isect.points.at(0), isect.points.at(1), eps);
            }else if(isect.kind == triangle_intersection_kind::CoplanarPolygon){
                auto region_a = coplanar_overlap_region<T>{ isect.points, triangle_normal(tri_b.at(0), tri_b.at(1), tri_b.at(2)) };
                auto region_b = coplanar_overlap_region<T>{ isect.points, triangle_normal(tri_a.at(0), tri_a.at(1), tri_a.at(2)) };
                arr_a.at(face_a_idx).overlaps.push_back(region_a);
                arr_b.at(face_b_idx).overlaps.push_back(region_b);
            }
        }
    }

    for(auto &arr : arr_a){
        finalize_coplanar_overlap_segments(arr, eps);
    }
    for(auto &arr : arr_b){
        finalize_coplanar_overlap_segments(arr, eps);
    }
}

template <class T, class OtherI>
boolean_face_relation
classify_split_triangle(const face_arrangement<T> &arr,
                        const std::array<vec3<T>, 3> &tri,
                        const prepared_mesh<T, OtherI> &other_prep,
                        T far_distance){
    const auto normal = triangle_normal(arr.tri.at(0), arr.tri.at(1), arr.tri.at(2));
    const auto axis = dominant_axis(normal);
    const auto centroid3 = triangle_centroid(tri);
    const auto centroid2 = project_drop_axis(centroid3, axis);

    for(const auto &overlap : arr.overlaps){
        std::vector<vec2<T>> poly2;
        poly2.reserve(overlap.polygon.size());
        for(const auto &p : overlap.polygon){
            poly2.push_back(project_drop_axis(p, axis));
        }
        if((poly2.size() >= 3UL) && point_in_polygon_or_on_boundary(poly2, centroid2)){
            return (normal.Dot(overlap.other_normal) >= static_cast<T>(0))
                ? boolean_face_relation::CoplanarSame
                : boolean_face_relation::CoplanarOpposite;
        }
    }

    const bool inside = point_inside_mesh(centroid3, other_prep, far_distance);
    return inside ? boolean_face_relation::Inside : boolean_face_relation::Outside;
}

template <class T, class I, class OtherI>
std::vector<split_triangle<T>>
split_face_with_arrangement(const face_arrangement<T> &arr,
                            const prepared_mesh<T, OtherI> &other_prep,
                            T far_distance){
    const auto normal = triangle_normal(arr.tri.at(0), arr.tri.at(1), arr.tri.at(2));
    const auto axis = dominant_axis(normal);

    std::vector<vec2<T>> verts2;
    verts2.reserve(arr.points.size());
    for(const auto &p : arr.points){
        verts2.push_back(project_drop_axis(p, axis));
    }

    std::set<std::pair<size_t, size_t>> refined_segments;
    for(const auto &seg : arr.segments){
        std::vector<size_t> on_segment = { seg.first, seg.second };
        const auto segment_eps = std::max(static_cast<T>(1),
                                          arr.points.at(seg.first).distance(arr.points.at(seg.second)))
                               * std::sqrt(std::numeric_limits<T>::epsilon()) * static_cast<T>(8);
        for(size_t i = 0; i < arr.points.size(); ++i){
            if((i == seg.first) || (i == seg.second)){
                continue;
            }
            if(point_near_segment(arr.points.at(i),
                                  arr.points.at(seg.first),
                                  arr.points.at(seg.second),
                                  segment_eps)){
                on_segment.push_back(i);
            }
        }

        const auto origin = arr.points.at(seg.first);
        const auto dir = arr.points.at(seg.second) - origin;
        const auto dir_sq = dir.sq_length();
        std::sort(on_segment.begin(), on_segment.end(), [&](size_t lhs, size_t rhs){
            const auto lhs_t = (dir_sq > static_cast<T>(0))
                ? (arr.points.at(lhs) - origin).Dot(dir) / dir_sq
                : static_cast<T>(0);
            const auto rhs_t = (dir_sq > static_cast<T>(0))
                ? (arr.points.at(rhs) - origin).Dot(dir) / dir_sq
                : static_cast<T>(0);
            return lhs_t < rhs_t;
        });
        on_segment.erase(std::unique(on_segment.begin(), on_segment.end()), on_segment.end());
        for(size_t i = 1; i < on_segment.size(); ++i){
            if(on_segment.at(i - 1) != on_segment.at(i)){
                refined_segments.insert(make_undirected_edge<T>(on_segment.at(i - 1), on_segment.at(i)));
            }
        }
    }

    std::vector<std::vector<size_t>> edges;
    edges.reserve(refined_segments.size());
    for(const auto &seg : refined_segments){
        edges.push_back({ seg.first, seg.second });
    }

    auto build_boundary_chain = [&](size_t a_idx, size_t b_idx) -> std::vector<size_t> {
        std::vector<size_t> chain = { a_idx, b_idx };
        for(size_t i = 0; i < arr.points.size(); ++i){
            if((i == a_idx) || (i == b_idx)){
                continue;
            }
            if(point_near_segment(arr.points.at(i), arr.points.at(a_idx), arr.points.at(b_idx),
                                  std::max(static_cast<T>(1), arr.points.at(a_idx).distance(arr.points.at(b_idx)))
                                  * std::sqrt(std::numeric_limits<T>::epsilon()) * static_cast<T>(8))){
                chain.push_back(i);
            }
        }
        const auto origin = arr.points.at(a_idx);
        const auto dir = arr.points.at(b_idx) - origin;
        const auto dir_sq = dir.sq_length();
        std::sort(chain.begin(), chain.end(), [&](size_t lhs, size_t rhs){
            const auto lhs_t = (dir_sq > static_cast<T>(0))
                ? (arr.points.at(lhs) - origin).Dot(dir) / dir_sq
                : static_cast<T>(0);
            const auto rhs_t = (dir_sq > static_cast<T>(0))
                ? (arr.points.at(rhs) - origin).Dot(dir) / dir_sq
                : static_cast<T>(0);
            return lhs_t < rhs_t;
        });
        chain.erase(std::unique(chain.begin(), chain.end()), chain.end());
        return chain;
    };

    const auto edge01 = build_boundary_chain(0UL, 1UL);
    const auto edge12 = build_boundary_chain(1UL, 2UL);
    const auto edge20 = build_boundary_chain(2UL, 0UL);
    std::vector<size_t> boundary_cycle;
    boundary_cycle.insert(boundary_cycle.end(), edge01.begin(), edge01.end() - 1);
    boundary_cycle.insert(boundary_cycle.end(), edge12.begin(), edge12.end() - 1);
    boundary_cycle.insert(boundary_cycle.end(), edge20.begin(), edge20.end() - 1);
    const std::array<vec2<T>, 3> tri2 = {{
        project_drop_axis(arr.tri.at(0), axis),
        project_drop_axis(arr.tri.at(1), axis),
        project_drop_axis(arr.tri.at(2), axis)
    }};

    std::set<std::pair<size_t, size_t>> boundary_segments;
    for(size_t i = 0; i < boundary_cycle.size(); ++i){
        boundary_segments.insert(make_undirected_edge<T>(boundary_cycle.at(i),
                                                         boundary_cycle.at((i + 1UL) % boundary_cycle.size())));
    }

    std::vector<std::pair<size_t, size_t>> internal_segments;
    for(const auto &seg : refined_segments){
        if(boundary_segments.count(seg) == 0UL){
            internal_segments.push_back(seg);
        }
    }

    auto emit_simple_polygons = [&](const std::vector<std::vector<size_t>> &polygons) -> std::vector<split_triangle<T>> {
        std::vector<split_triangle<T>> simple_out;
        for(const auto &poly : polygons){
            if(poly.size() < 3UL){
                continue;
            }
            for(size_t i = 1; (i + 1UL) < poly.size(); ++i){
                const auto ia = poly.at(0);
                const auto ib = poly.at(i);
                const auto ic = poly.at(i + 1UL);
                const auto &a2 = verts2.at(ia);
                const auto &b2 = verts2.at(ib);
                const auto &c2 = verts2.at(ic);
                if(orient_sign(a2, b2, c2) == 0){
                    continue;
                }
                const auto centroid2 = (a2 + b2 + c2) / static_cast<T>(3);
                if(!point_in_triangle_or_on_boundary(centroid2, tri2.at(0), tri2.at(1), tri2.at(2))){
                    continue;
                }
                std::array<vec3<T>, 3> tri = {{ arr.points.at(ia), arr.points.at(ib), arr.points.at(ic) }};
                split_triangle<T> st;
                st.verts = tri;
                st.relation = classify_split_triangle(arr, tri, other_prep, far_distance);
                simple_out.push_back(st);
            }
        }
        return simple_out;
    };

    if(internal_segments.empty()){
        return emit_simple_polygons({ boundary_cycle });
    }
    if(internal_segments.size() == 1UL){
        const auto u = internal_segments.front().first;
        const auto v = internal_segments.front().second;
        const auto it_u = std::find(boundary_cycle.begin(), boundary_cycle.end(), u);
        const auto it_v = std::find(boundary_cycle.begin(), boundary_cycle.end(), v);
        if((it_u != boundary_cycle.end()) && (it_v != boundary_cycle.end()) && (it_u != it_v)){
            const auto pos_u = static_cast<size_t>(std::distance(boundary_cycle.begin(), it_u));
            const auto pos_v = static_cast<size_t>(std::distance(boundary_cycle.begin(), it_v));
            std::vector<size_t> poly1;
            std::vector<size_t> poly2;
            for(size_t i = pos_u;; i = (i + 1UL) % boundary_cycle.size()){
                poly1.push_back(boundary_cycle.at(i));
                if(i == pos_v) break;
            }
            for(size_t i = pos_v;; i = (i + 1UL) % boundary_cycle.size()){
                poly2.push_back(boundary_cycle.at(i));
                if(i == pos_u) break;
            }
            return emit_simple_polygons({ poly1, poly2 });
        }
    }

    fv_surface_mesh<T, size_t> tri_mesh;
    try{
        tri_mesh = Constrained_Delaunay_Triangulation_2<T, size_t>(verts2, edges, false);
    }catch(const std::exception &e){
        std::ostringstream oss;
        oss << "Constrained triangulation failed for BooleanMeshOp2 face: " << e.what() << " points=";
        for(size_t i = 0; i < verts2.size(); ++i){
            oss << " [" << i << ":" << verts2.at(i).x << "," << verts2.at(i).y << "]";
        }
        oss << " segments=";
        for(const auto &edge : edges){
            oss << " (" << edge.at(0) << "," << edge.at(1) << ")";
        }
        throw std::runtime_error(oss.str());
    }
    std::vector<split_triangle<T>> out;
    for(const auto &face : tri_mesh.faces){
        if(face.size() != 3UL){
            continue;
        }
        const auto ia = static_cast<size_t>(face.at(0));
        const auto ib = static_cast<size_t>(face.at(1));
        const auto ic = static_cast<size_t>(face.at(2));
        if((ia == ib) || (ib == ic) || (ic == ia)){
            continue;
        }
        const auto &a2 = verts2.at(ia);
        const auto &b2 = verts2.at(ib);
        const auto &c2 = verts2.at(ic);
        if(orient_sign(a2, b2, c2) == 0){
            continue;
        }

        const auto centroid2 = (a2 + b2 + c2) / static_cast<T>(3);
        if(!point_in_triangle_or_on_boundary(centroid2, tri2.at(0), tri2.at(1), tri2.at(2))){
            continue;
        }

        std::array<vec3<T>, 3> tri = {{ arr.points.at(ia), arr.points.at(ib), arr.points.at(ic) }};
        split_triangle<T> st;
        st.verts = tri;
        st.relation = classify_split_triangle(arr, tri, other_prep, far_distance);
        out.push_back(st);
    }
    return out;
}

template <class T>
bool
include_face_from_a(boolean_face_relation relation,
                    MeshBooleanOperation2 op){
    switch(op){
        case MeshBooleanOperation2::Union:
            return (relation == boolean_face_relation::Outside)
                || (relation == boolean_face_relation::CoplanarSame);
        case MeshBooleanOperation2::Intersection:
            return (relation == boolean_face_relation::Inside)
                || (relation == boolean_face_relation::CoplanarSame);
        case MeshBooleanOperation2::Subtraction:
            return (relation == boolean_face_relation::Outside)
                || (relation == boolean_face_relation::CoplanarOpposite);
        case MeshBooleanOperation2::Exclusion:
            return (relation == boolean_face_relation::Outside)
                || (relation == boolean_face_relation::Inside);
    }
    return false;
}

template <class T>
bool
include_face_from_b(boolean_face_relation relation,
                    MeshBooleanOperation2 op){
    switch(op){
        case MeshBooleanOperation2::Union:
            return relation == boolean_face_relation::Outside;
        case MeshBooleanOperation2::Intersection:
            return relation == boolean_face_relation::Inside;
        case MeshBooleanOperation2::Subtraction:
            return relation == boolean_face_relation::Inside;
        case MeshBooleanOperation2::Exclusion:
            return (relation == boolean_face_relation::Outside)
                || (relation == boolean_face_relation::Inside);
    }
    return false;
}

template <class T, class I>
void
append_triangle(fv_surface_mesh<T, I> &mesh,
                const std::array<vec3<T>, 3> &tri){
    const auto base = static_cast<I>(mesh.vertices.size());
    mesh.vertices.push_back(tri.at(0));
    mesh.vertices.push_back(tri.at(1));
    mesh.vertices.push_back(tri.at(2));
    mesh.faces.push_back({ base, static_cast<I>(base + 1), static_cast<I>(base + 2) });
}

template <class T, class I>
void
deduplicate_faces(fv_surface_mesh<T, I> &mesh){
    std::set<std::array<I, 3>> seen;
    std::vector<std::vector<I>> filtered;
    filtered.reserve(mesh.faces.size());
    for(const auto &face : mesh.faces){
        if(face.size() != 3UL){
            continue;
        }
        auto key = std::array<I, 3>{{ face.at(0), face.at(1), face.at(2) }};
        std::sort(key.begin(), key.end());
        if(seen.insert(key).second){
            filtered.push_back(face);
        }
    }
    mesh.faces.swap(filtered);
}

template <class T, class I>
bool
extract_axis_aligned_box(const fv_surface_mesh<T, I> &mesh,
                         vec3<T> &bb_min,
                         vec3<T> &bb_max,
                         T eps){
    if(mesh.faces.size() != 12UL){
        return false;
    }
    bb_min = mesh_bbox(mesh).min;
    bb_max = mesh_bbox(mesh).max;
    if((bb_max.x - bb_min.x <= eps) || (bb_max.y - bb_min.y <= eps) || (bb_max.z - bb_min.z <= eps)){
        return false;
    }

    std::set<std::array<int, 3>> corners;
    for(const auto &v : mesh.vertices){
        auto classify = [&](T value, T lo, T hi) -> int {
            if(std::abs(value - lo) <= eps) return 0;
            if(std::abs(value - hi) <= eps) return 1;
            return -1;
        };
        const auto cx = classify(v.x, bb_min.x, bb_max.x);
        const auto cy = classify(v.y, bb_min.y, bb_max.y);
        const auto cz = classify(v.z, bb_min.z, bb_max.z);
        if((cx < 0) || (cy < 0) || (cz < 0)){
            return false;
        }
        corners.insert({{ cx, cy, cz }});
    }
    if(corners.size() != 8UL){
        return false;
    }

    for(const auto &face : mesh.faces){
        if(face.size() != 3UL){
            return false;
        }
        const auto &a = mesh.vertices.at(face.at(0));
        const auto &b = mesh.vertices.at(face.at(1));
        const auto &c = mesh.vertices.at(face.at(2));
        const bool on_x = ((std::abs(a.x - b.x) <= eps) && (std::abs(b.x - c.x) <= eps)
                        && ((std::abs(a.x - bb_min.x) <= eps) || (std::abs(a.x - bb_max.x) <= eps)));
        const bool on_y = ((std::abs(a.y - b.y) <= eps) && (std::abs(b.y - c.y) <= eps)
                        && ((std::abs(a.y - bb_min.y) <= eps) || (std::abs(a.y - bb_max.y) <= eps)));
        const bool on_z = ((std::abs(a.z - b.z) <= eps) && (std::abs(b.z - c.z) <= eps)
                        && ((std::abs(a.z - bb_min.z) <= eps) || (std::abs(a.z - bb_max.z) <= eps)));
        if(!(on_x || on_y || on_z)){
            return false;
        }
    }
    return true;
}

template <class T>
bool
box_contains_point(const vec3<T> &bb_min,
                   const vec3<T> &bb_max,
                   const vec3<T> &p){
    return (p.x >= bb_min.x) && (p.x <= bb_max.x)
        && (p.y >= bb_min.y) && (p.y <= bb_max.y)
        && (p.z >= bb_min.z) && (p.z <= bb_max.z);
}

template <class T, class I>
fv_surface_mesh<T, I>
exact_axis_aligned_box_boolean(const vec3<T> &lhs_min,
                               const vec3<T> &lhs_max,
                               const vec3<T> &rhs_min,
                               const vec3<T> &rhs_max,
                               MeshBooleanOperation2 op){
    std::vector<T> xs = { lhs_min.x, lhs_max.x, rhs_min.x, rhs_max.x };
    std::vector<T> ys = { lhs_min.y, lhs_max.y, rhs_min.y, rhs_max.y };
    std::vector<T> zs = { lhs_min.z, lhs_max.z, rhs_min.z, rhs_max.z };
    auto uniq = [](std::vector<T> &vals){
        std::sort(vals.begin(), vals.end());
        vals.erase(std::unique(vals.begin(), vals.end()), vals.end());
    };
    uniq(xs);
    uniq(ys);
    uniq(zs);

    const size_t nx = (xs.size() > 1UL) ? (xs.size() - 1UL) : 0UL;
    const size_t ny = (ys.size() > 1UL) ? (ys.size() - 1UL) : 0UL;
    const size_t nz = (zs.size() > 1UL) ? (zs.size() - 1UL) : 0UL;
    std::vector<uint8_t> occupied(nx * ny * nz, 0U);
    auto index = [&](size_t ix, size_t iy, size_t iz) -> size_t {
        return (ix * ny + iy) * nz + iz;
    };
    for(size_t ix = 0; ix < nx; ++ix){
        for(size_t iy = 0; iy < ny; ++iy){
            for(size_t iz = 0; iz < nz; ++iz){
                const vec3<T> centre((xs.at(ix) + xs.at(ix + 1UL)) / static_cast<T>(2),
                                     (ys.at(iy) + ys.at(iy + 1UL)) / static_cast<T>(2),
                                     (zs.at(iz) + zs.at(iz + 1UL)) / static_cast<T>(2));
                const bool in_lhs = box_contains_point(lhs_min, lhs_max, centre);
                const bool in_rhs = box_contains_point(rhs_min, rhs_max, centre);
                bool keep = false;
                switch(op){
                    case MeshBooleanOperation2::Union: keep = in_lhs || in_rhs; break;
                    case MeshBooleanOperation2::Intersection: keep = in_lhs && in_rhs; break;
                    case MeshBooleanOperation2::Subtraction: keep = in_lhs && !in_rhs; break;
                    case MeshBooleanOperation2::Exclusion: keep = (in_lhs != in_rhs); break;
                }
                occupied.at(index(ix, iy, iz)) = keep ? 1U : 0U;
            }
        }
    }

    fv_surface_mesh<T, I> out;
    std::map<vec3<T>, I> vertex_map;
    auto get_vertex = [&](const vec3<T> &p) -> I {
        const auto it = vertex_map.find(p);
        if(it != vertex_map.end()){
            return it->second;
        }
        const auto idx = static_cast<I>(out.vertices.size());
        out.vertices.push_back(p);
        vertex_map.emplace(p, idx);
        return idx;
    };
    auto emit_face = [&](const std::array<vec3<T>, 4> &quad) {
        const auto i0 = get_vertex(quad.at(0));
        const auto i1 = get_vertex(quad.at(1));
        const auto i2 = get_vertex(quad.at(2));
        const auto i3 = get_vertex(quad.at(3));
        out.faces.push_back({ i0, i1, i2 });
        out.faces.push_back({ i0, i2, i3 });
    };

    for(size_t ix = 0; ix < nx; ++ix){
        for(size_t iy = 0; iy < ny; ++iy){
            for(size_t iz = 0; iz < nz; ++iz){
                if(occupied.at(index(ix, iy, iz)) == 0U){
                    continue;
                }
                const auto x0 = xs.at(ix);
                const auto x1 = xs.at(ix + 1UL);
                const auto y0 = ys.at(iy);
                const auto y1 = ys.at(iy + 1UL);
                const auto z0 = zs.at(iz);
                const auto z1 = zs.at(iz + 1UL);
                const auto occ = [&](int dx, int dy, int dz) -> bool {
                    const auto nx_i = static_cast<int>(ix) + dx;
                    const auto ny_i = static_cast<int>(iy) + dy;
                    const auto nz_i = static_cast<int>(iz) + dz;
                    if((nx_i < 0) || (ny_i < 0) || (nz_i < 0)
                    || (nx_i >= static_cast<int>(nx))
                    || (ny_i >= static_cast<int>(ny))
                    || (nz_i >= static_cast<int>(nz))){
                        return false;
                    }
                    return occupied.at(index(static_cast<size_t>(nx_i),
                                             static_cast<size_t>(ny_i),
                                             static_cast<size_t>(nz_i))) != 0U;
                };

                if(!occ(-1, 0, 0)) emit_face({{ {x0,y0,z0}, {x0,y0,z1}, {x0,y1,z1}, {x0,y1,z0} }});
                if(!occ( 1, 0, 0)) emit_face({{ {x1,y0,z0}, {x1,y1,z0}, {x1,y1,z1}, {x1,y0,z1} }});
                if(!occ(0, -1, 0)) emit_face({{ {x0,y0,z0}, {x1,y0,z0}, {x1,y0,z1}, {x0,y0,z1} }});
                if(!occ(0,  1, 0)) emit_face({{ {x0,y1,z0}, {x0,y1,z1}, {x1,y1,z1}, {x1,y1,z0} }});
                if(!occ(0, 0, -1)) emit_face({{ {x0,y0,z0}, {x0,y1,z0}, {x1,y1,z0}, {x1,y0,z0} }});
                if(!occ(0, 0,  1)) emit_face({{ {x0,y0,z1}, {x1,y0,z1}, {x1,y1,z1}, {x0,y1,z1} }});
            }
        }
    }
    if(!out.faces.empty()){
        OrientFaces(out, std::sqrt(std::numeric_limits<T>::epsilon()) * static_cast<T>(8));
    }
    out.recreate_involved_face_index();
    return out;
}

template <class T, class I>
fv_surface_mesh<T, I>
boolean_mesh_op_impl(const fv_surface_mesh<T, I> &lhs,
                     const fv_surface_mesh<T, I> &rhs,
                     MeshBooleanOperation2 op,
                     T snap_eps){
    const auto lhs_bounds = mesh_bbox(lhs);
    const auto rhs_bounds = mesh_bbox(rhs);
    auto all_bounds = lhs_bounds;
    all_bounds.expand(rhs_bounds);
    const auto eps = (snap_eps > static_cast<T>(0)) ? snap_eps : mesh_coord_eps(all_bounds);

    const auto prep_a = prepare_mesh(lhs, "lhs", eps);
    const auto prep_b = prepare_mesh(rhs, "rhs", eps);

    vec3<T> lhs_box_min, lhs_box_max, rhs_box_min, rhs_box_max;
    if(extract_axis_aligned_box(prep_a.mesh, lhs_box_min, lhs_box_max, eps)
    && extract_axis_aligned_box(prep_b.mesh, rhs_box_min, rhs_box_max, eps)){
        return exact_axis_aligned_box_boolean<T, I>(lhs_box_min, lhs_box_max,
                                                    rhs_box_min, rhs_box_max,
                                                    op);
    }

    try{
        std::vector<face_arrangement<T>> arr_a;
        std::vector<face_arrangement<T>> arr_b;
        build_face_arrangements(prep_a, prep_b, arr_a, arr_b, eps);

        const auto extent = all_bounds.max - all_bounds.min;
        const auto far_distance = static_cast<T>(4) * std::max({extent.x, extent.y, extent.z, static_cast<T>(1)});

        fv_surface_mesh<T, I> out;
        for(const auto &arr : arr_a){
            const auto pieces = split_face_with_arrangement<T, I, I>(arr, prep_b, far_distance);
            for(const auto &piece : pieces){
                if(include_face_from_a<T>(piece.relation, op)){
                    append_triangle(out, piece.verts);
                }
            }
        }
        for(const auto &arr : arr_b){
            const auto pieces = split_face_with_arrangement<T, I, I>(arr, prep_a, far_distance);
            for(const auto &piece : pieces){
                if(include_face_from_b<T>(piece.relation, op)){
                    append_triangle(out, piece.verts);
                }
            }
        }

        if(out.faces.empty()){
            return out;
        }

        out.merge_duplicate_vertices(eps * static_cast<T>(32));
        deduplicate_faces(out);
        out.remove_degenerate_faces();
        out.remove_disconnected_vertices();
        out.recreate_involved_face_index();
        validate_closed_triangular_mesh(out, "BooleanMeshOp2 output");
        if(!OrientFaces(out, eps * static_cast<T>(8))){
            throw std::runtime_error("BooleanMeshOp2 produced an inconsistent boundary mesh.");
        }
        out.recreate_involved_face_index();
        return out;
    }catch(const std::exception &e){
        YLOGWARN("BooleanMeshOp2 falling back to legacy BooleanMeshOp: " << e.what());
        throw;
    }
}

} // namespace


template <class T, class I>
fv_surface_mesh<T, I>
BooleanMeshOp2(const fv_surface_mesh<T, I> &lhs,
               const fv_surface_mesh<T, I> &rhs,
               MeshBooleanOperation2 op,
               T snap_eps){
    return boolean_mesh_op_impl(lhs, rhs, op, snap_eps);
}

template <class T, class I>
fv_surface_mesh<T, I>
BooleanUnion2(const fv_surface_mesh<T, I> &lhs,
              const fv_surface_mesh<T, I> &rhs,
              T snap_eps){
    return BooleanMeshOp2(lhs, rhs, MeshBooleanOperation2::Union, snap_eps);
}

template <class T, class I>
fv_surface_mesh<T, I>
BooleanIntersection2(const fv_surface_mesh<T, I> &lhs,
                     const fv_surface_mesh<T, I> &rhs,
                     T snap_eps){
    return BooleanMeshOp2(lhs, rhs, MeshBooleanOperation2::Intersection, snap_eps);
}

template <class T, class I>
fv_surface_mesh<T, I>
BooleanExclusion2(const fv_surface_mesh<T, I> &lhs,
                  const fv_surface_mesh<T, I> &rhs,
                  T snap_eps){
    return BooleanMeshOp2(lhs, rhs, MeshBooleanOperation2::Exclusion, snap_eps);
}

template <class T, class I>
fv_surface_mesh<T, I>
BooleanSubtraction2(const fv_surface_mesh<T, I> &lhs,
                    const fv_surface_mesh<T, I> &rhs,
                    T snap_eps){
    return BooleanMeshOp2(lhs, rhs, MeshBooleanOperation2::Subtraction, snap_eps);
}

#ifndef YGOR_MESHES_BOOLEAN2_DISABLE_ALL_SPECIALIZATIONS
template fv_surface_mesh<float , uint32_t> BooleanMeshOp2(const fv_surface_mesh<float , uint32_t> &, const fv_surface_mesh<float , uint32_t> &, MeshBooleanOperation2, float);
template fv_surface_mesh<float , uint64_t> BooleanMeshOp2(const fv_surface_mesh<float , uint64_t> &, const fv_surface_mesh<float , uint64_t> &, MeshBooleanOperation2, float);
template fv_surface_mesh<double, uint32_t> BooleanMeshOp2(const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &, MeshBooleanOperation2, double);
template fv_surface_mesh<double, uint64_t> BooleanMeshOp2(const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &, MeshBooleanOperation2, double);

template fv_surface_mesh<float , uint32_t> BooleanUnion2(const fv_surface_mesh<float , uint32_t> &, const fv_surface_mesh<float , uint32_t> &, float);
template fv_surface_mesh<float , uint64_t> BooleanUnion2(const fv_surface_mesh<float , uint64_t> &, const fv_surface_mesh<float , uint64_t> &, float);
template fv_surface_mesh<double, uint32_t> BooleanUnion2(const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &, double);
template fv_surface_mesh<double, uint64_t> BooleanUnion2(const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &, double);

template fv_surface_mesh<float , uint32_t> BooleanIntersection2(const fv_surface_mesh<float , uint32_t> &, const fv_surface_mesh<float , uint32_t> &, float);
template fv_surface_mesh<float , uint64_t> BooleanIntersection2(const fv_surface_mesh<float , uint64_t> &, const fv_surface_mesh<float , uint64_t> &, float);
template fv_surface_mesh<double, uint32_t> BooleanIntersection2(const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &, double);
template fv_surface_mesh<double, uint64_t> BooleanIntersection2(const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &, double);

template fv_surface_mesh<float , uint32_t> BooleanExclusion2(const fv_surface_mesh<float , uint32_t> &, const fv_surface_mesh<float , uint32_t> &, float);
template fv_surface_mesh<float , uint64_t> BooleanExclusion2(const fv_surface_mesh<float , uint64_t> &, const fv_surface_mesh<float , uint64_t> &, float);
template fv_surface_mesh<double, uint32_t> BooleanExclusion2(const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &, double);
template fv_surface_mesh<double, uint64_t> BooleanExclusion2(const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &, double);

template fv_surface_mesh<float , uint32_t> BooleanSubtraction2(const fv_surface_mesh<float , uint32_t> &, const fv_surface_mesh<float , uint32_t> &, float);
template fv_surface_mesh<float , uint64_t> BooleanSubtraction2(const fv_surface_mesh<float , uint64_t> &, const fv_surface_mesh<float , uint64_t> &, float);
template fv_surface_mesh<double, uint32_t> BooleanSubtraction2(const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &, double);
template fv_surface_mesh<double, uint64_t> BooleanSubtraction2(const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &, double);
#endif
