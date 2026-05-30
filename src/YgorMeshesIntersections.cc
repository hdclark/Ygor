//YgorMeshesIntersections.cc - Written by hal clark in 2026.

#include <algorithm>
#include <any>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorIndexRTree.h"
#include "YgorMath.h"
#include "YgorMeshesIntersections.h"

template <class T>
int orient_sign(const vec3<T> &a, const vec3<T> &b, const vec3<T> &c, const vec3<T> &d);

template <class T>
bool point_on_closed_segment(const vec3<T> &p, const vec3<T> &a, const vec3<T> &b);

template <class T>
bool point_on_open_segment(const vec3<T> &p, const vec3<T> &a, const vec3<T> &b);

template <class T>
int orient_sign(const vec2<T> &a, const vec2<T> &b, const vec2<T> &c);

template <class T>
bool point_on_closed_segment(const vec2<T> &p, const vec2<T> &a, const vec2<T> &b);

template <class T>
bool point_in_triangle_or_on_boundary(const vec2<T> &p,
                                      const vec2<T> &a,
                                      const vec2<T> &b,
                                      const vec2<T> &c);

namespace {

enum class triangle_intersection_kind : uint8_t {
    None,
    Point,
    Segment,
    Polygon
};

template <class T, class I>
struct mesh_intersection_triangle {
    I face_index = static_cast<I>(0);
    std::array<I, 3> vertex_indices;
    std::array<vec3<T>, 3> verts;
    index_bbox<T> bbox;
};

template <class T>
struct triangle_intersection_result {
    triangle_intersection_kind kind = triangle_intersection_kind::None;
    std::vector<vec3<T>> points;
};

template <class T>
struct projected_point {
    vec2<T> p2;
    vec3<T> p3;
};

template <class I>
I
checked_index_cast(size_t n){
    if constexpr ((sizeof(I) < sizeof(size_t)) || std::numeric_limits<I>::is_signed){
        if(n > static_cast<size_t>(std::numeric_limits<I>::max())){
            throw std::invalid_argument("Surface mesh contains an index that cannot be represented by the mesh index type.");
        }
    }
    return static_cast<I>(n);
}

inline bool
lexicographically_less(const vec3<float> &lhs,
                       const vec3<float> &rhs){
    if(lhs.x != rhs.x) return lhs.x < rhs.x;
    if(lhs.y != rhs.y) return lhs.y < rhs.y;
    return lhs.z < rhs.z;
}

inline bool
lexicographically_less(const vec3<double> &lhs,
                       const vec3<double> &rhs){
    if(lhs.x != rhs.x) return lhs.x < rhs.x;
    if(lhs.y != rhs.y) return lhs.y < rhs.y;
    return lhs.z < rhs.z;
}

template <class T>
index_bbox<T>
triangle_bbox(const std::array<vec3<T>, 3> &tri){
    const vec3<T> bb_min(std::min({tri.at(0).x, tri.at(1).x, tri.at(2).x}),
                         std::min({tri.at(0).y, tri.at(1).y, tri.at(2).y}),
                         std::min({tri.at(0).z, tri.at(1).z, tri.at(2).z}));
    const vec3<T> bb_max(std::max({tri.at(0).x, tri.at(1).x, tri.at(2).x}),
                         std::max({tri.at(0).y, tri.at(1).y, tri.at(2).y}),
                         std::max({tri.at(0).z, tri.at(1).z, tri.at(2).z}));
    return index_bbox<T>(bb_min, bb_max);
}

template <class T>
vec3<T>
triangle_normal(const std::array<vec3<T>, 3> &tri){
    return (tri.at(1) - tri.at(0)).Cross(tri.at(2) - tri.at(0));
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
    return a + (ab * t);
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
        {{ tri_a.at(0), tri_a.at(1) }}, {{ tri_a.at(1), tri_a.at(2) }}, {{ tri_a.at(2), tri_a.at(0) }},
        {{ tri_b.at(0), tri_b.at(1) }}, {{ tri_b.at(1), tri_b.at(2) }}, {{ tri_b.at(2), tri_b.at(0) }}
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
    for(const auto &existing : points){
        if(existing.distance(p) <= eps){
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
projected_point<T>
segment_line_intersection_2d(const projected_point<T> &a0,
                             const projected_point<T> &a1,
                             const projected_point<T> &b0,
                             const projected_point<T> &b1){
    const auto sa = a1.p2 - a0.p2;
    const auto sb = b1.p2 - b0.p2;
    const long double det = static_cast<long double>(sa.x) * static_cast<long double>(sb.y)
                          - static_cast<long double>(sa.y) * static_cast<long double>(sb.x);

    long double t = 0.0L;
    if(std::abs(det) > 0.0L){
        const long double rx = static_cast<long double>(b0.p2.x - a0.p2.x);
        const long double ry = static_cast<long double>(b0.p2.y - a0.p2.y);
        t = (rx * static_cast<long double>(sb.y) - ry * static_cast<long double>(sb.x)) / det;
    }
    t = std::max(0.0L, std::min(1.0L, t));
    return projected_point<T>{
        a0.p2 + (sa * static_cast<T>(t)),
        a0.p3 + ((a1.p3 - a0.p3) * static_cast<T>(t))
    };
}

template <class T>
std::vector<vec3<T>>
coplanar_segment_intersections(const projected_point<T> &a0,
                               const projected_point<T> &a1,
                               const projected_point<T> &b0,
                               const projected_point<T> &b1,
                               T eps){
    std::vector<vec3<T>> out;

    const auto o1 = orient_sign(a0.p2, a1.p2, b0.p2);
    const auto o2 = orient_sign(a0.p2, a1.p2, b1.p2);
    const auto o3 = orient_sign(b0.p2, b1.p2, a0.p2);
    const auto o4 = orient_sign(b0.p2, b1.p2, a1.p2);

    if(((o1 > 0) != (o2 > 0)) && ((o3 > 0) != (o4 > 0))){
        add_unique_point(out, segment_line_intersection_2d(a0, a1, b0, b1).p3, eps);
        return out;
    }

    if((o1 == 0) && point_on_closed_segment(b0.p2, a0.p2, a1.p2)){
        add_unique_point(out, b0.p3, eps);
    }
    if((o2 == 0) && point_on_closed_segment(b1.p2, a0.p2, a1.p2)){
        add_unique_point(out, b1.p3, eps);
    }
    if((o3 == 0) && point_on_closed_segment(a0.p2, b0.p2, b1.p2)){
        add_unique_point(out, a0.p3, eps);
    }
    if((o4 == 0) && point_on_closed_segment(a1.p2, b0.p2, b1.p2)){
        add_unique_point(out, a1.p3, eps);
    }
    return out;
}

template <class T>
void
canonicalize_segment(std::vector<vec3<T>> &points){
    if((points.size() == 2UL) && lexicographically_less(points.at(1), points.at(0))){
        std::swap(points.at(0), points.at(1));
    }
}

template <class T>
void
canonicalize_polygon(std::vector<vec3<T>> &points,
                     const vec3<T> &normal){
    if(points.size() < 3UL){
        return;
    }

    const auto axis = dominant_axis(normal);
    vec3<T> centroid(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
    for(const auto &p : points){
        centroid += p;
    }
    centroid /= static_cast<T>(points.size());
    const auto c2 = project_drop_axis(centroid, axis);

    std::sort(points.begin(), points.end(), [&](const vec3<T> &lhs, const vec3<T> &rhs){
        const auto pl = project_drop_axis(lhs, axis) - c2;
        const auto pr = project_drop_axis(rhs, axis) - c2;
        const auto al = std::atan2(static_cast<long double>(pl.y), static_cast<long double>(pl.x));
        const auto ar = std::atan2(static_cast<long double>(pr.y), static_cast<long double>(pr.x));
        if(al != ar){
            return al < ar;
        }
        return lexicographically_less(lhs, rhs);
    });

    auto best_it = points.begin();
    for(auto it = points.begin() + 1; it != points.end(); ++it){
        if(lexicographically_less(*it, *best_it)){
            best_it = it;
        }
    }
    std::rotate(points.begin(), best_it, points.end());
}

template <class T>
triangle_intersection_result<T>
classify_intersection_points(std::vector<vec3<T>> points,
                             const vec3<T> &normal,
                             T eps){
    triangle_intersection_result<T> out;
    if(points.empty()){
        return out;
    }

    std::vector<vec3<T>> deduped;
    for(const auto &p : points){
        add_unique_point(deduped, p, eps);
    }
    if(deduped.empty()){
        return out;
    }
    if(deduped.size() == 1UL){
        out.kind = triangle_intersection_kind::Point;
        out.points = deduped;
        return out;
    }

    size_t a = 0;
    size_t b = 1;
    T farthest_sq = deduped.at(0).sq_dist(deduped.at(1));
    for(size_t i = 0; i < deduped.size(); ++i){
        for(size_t j = i + 1UL; j < deduped.size(); ++j){
            const auto dist_sq = deduped.at(i).sq_dist(deduped.at(j));
            if(dist_sq > farthest_sq){
                farthest_sq = dist_sq;
                a = i;
                b = j;
            }
        }
    }

    if(!(farthest_sq > eps * eps)){
        out.kind = triangle_intersection_kind::Point;
        out.points = { deduped.at(a) };
        return out;
    }

    const auto axis = dominant_axis(normal);
    const auto pa = project_drop_axis(deduped.at(a), axis);
    const auto pb = project_drop_axis(deduped.at(b), axis);
    bool collinear = true;
    for(const auto &p : deduped){
        if(orient_sign(pa, pb, project_drop_axis(p, axis)) != 0){
            collinear = false;
            break;
        }
    }

    if(collinear){
        const auto dir = deduped.at(b) - deduped.at(a);
        auto param = [&](const vec3<T> &p) -> T {
            return (p - deduped.at(a)).Dot(dir);
        };

        auto minmax = std::minmax_element(deduped.begin(), deduped.end(), [&](const vec3<T> &lhs, const vec3<T> &rhs){
            return param(lhs) < param(rhs);
        });
        out.kind = triangle_intersection_kind::Segment;
        out.points = { *minmax.first, *minmax.second };
        canonicalize_segment(out.points);
        if(out.points.at(0).distance(out.points.at(1)) <= eps){
            out.kind = triangle_intersection_kind::Point;
            out.points.resize(1UL);
        }
        return out;
    }

    out.kind = triangle_intersection_kind::Polygon;
    out.points = deduped;
    canonicalize_polygon(out.points, normal);
    return out;
}

template <class T>
triangle_intersection_result<T>
compute_coplanar_intersection(const std::array<vec3<T>, 3> &tri_a,
                              const std::array<vec3<T>, 3> &tri_b,
                              T eps){
    const auto normal = triangle_normal(tri_a);
    const auto axis = dominant_axis(normal);

    const std::array<projected_point<T>, 3> a = {{
        { project_drop_axis(tri_a.at(0), axis), tri_a.at(0) },
        { project_drop_axis(tri_a.at(1), axis), tri_a.at(1) },
        { project_drop_axis(tri_a.at(2), axis), tri_a.at(2) }
    }};
    const std::array<projected_point<T>, 3> b = {{
        { project_drop_axis(tri_b.at(0), axis), tri_b.at(0) },
        { project_drop_axis(tri_b.at(1), axis), tri_b.at(1) },
        { project_drop_axis(tri_b.at(2), axis), tri_b.at(2) }
    }};

    std::vector<vec3<T>> points;
    for(size_t i = 0; i < 3UL; ++i){
        if(point_in_triangle_or_on_boundary(a.at(i).p2, b.at(0).p2, b.at(1).p2, b.at(2).p2)){
            add_unique_point(points, tri_a.at(i), eps);
        }
        if(point_in_triangle_or_on_boundary(b.at(i).p2, a.at(0).p2, a.at(1).p2, a.at(2).p2)){
            add_unique_point(points, tri_b.at(i), eps);
        }
    }

    for(size_t i = 0; i < 3UL; ++i){
        const size_t inext = (i + 1UL) % 3UL;
        for(size_t j = 0; j < 3UL; ++j){
            const size_t jnext = (j + 1UL) % 3UL;
            const auto edge_points = coplanar_segment_intersections(a.at(i), a.at(inext), b.at(j), b.at(jnext), eps);
            for(const auto &p : edge_points){
                add_unique_point(points, snap_intersection_point(p, tri_a, tri_b, eps), eps);
            }
        }
    }

    for(auto &p : points){
        p = snap_intersection_point(p, tri_a, tri_b, eps);
    }
    return classify_intersection_points(points, normal, eps);
}

template <class T>
triangle_intersection_result<T>
compute_noncoplanar_intersection(const std::array<vec3<T>, 3> &tri_a,
                                 const std::array<vec3<T>, 3> &tri_b,
                                 T eps){
    triangle_intersection_result<T> out;

    const auto n_a = triangle_normal(tri_a);
    const auto n_b = triangle_normal(tri_b);
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
    if(seg_a.empty() || seg_b.empty()){
        return out;
    }

    const auto dir = line_dir.unit();
    const auto origin = seg_a.front();
    auto proj = [&](const vec3<T> &p) -> T {
        return (p - origin).Dot(dir);
    };

    const auto a_minmax = std::minmax_element(seg_a.begin(), seg_a.end(), [&](const vec3<T> &lhs, const vec3<T> &rhs){
        return proj(lhs) < proj(rhs);
    });
    const auto b_minmax = std::minmax_element(seg_b.begin(), seg_b.end(), [&](const vec3<T> &lhs, const vec3<T> &rhs){
        return proj(lhs) < proj(rhs);
    });
    const auto a_lo = proj(*a_minmax.first);
    const auto a_hi = proj(*a_minmax.second);
    const auto b_lo = proj(*b_minmax.first);
    const auto b_hi = proj(*b_minmax.second);
    const auto lo = std::max(a_lo, b_lo);
    const auto hi = std::min(a_hi, b_hi);
    if(hi < (lo - eps)){
        return out;
    }

    auto p0 = origin + (dir * lo);
    auto p1 = origin + (dir * hi);
    p0 = snap_intersection_point(p0, tri_a, tri_b, eps);
    p1 = snap_intersection_point(p1, tri_a, tri_b, eps);

    if(p0.distance(p1) <= eps){
        out.kind = triangle_intersection_kind::Point;
        out.points = { p0 };
        return out;
    }

    out.kind = triangle_intersection_kind::Segment;
    out.points = { p0, p1 };
    canonicalize_segment(out.points);
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

template <class T, class I>
std::vector<mesh_intersection_triangle<T, I>>
collect_intersection_triangles(const fv_surface_mesh<T, I> &mesh){
    std::vector<mesh_intersection_triangle<T, I>> out;
    for(const auto &v : mesh.vertices){
        if(!v.isfinite()){
            throw std::invalid_argument("Surface mesh contains a non-finite vertex.");
        }
    }

    for(size_t face_idx = 0; face_idx < mesh.faces.size(); ++face_idx){
        const auto &face = mesh.faces.at(face_idx);
        if(face.size() < 3UL){
            continue;
        }
        for(const auto vi : face){
            if(static_cast<size_t>(vi) >= mesh.vertices.size()){
                throw std::invalid_argument("Surface mesh contains an out-of-range face index.");
            }
        }
        for(size_t i = 1UL; i + 1UL < face.size(); ++i){
            const std::array<I, 3> vertex_indices = {{
                face.at(0),
                face.at(i),
                face.at(i + 1UL)
            }};
            const std::array<vec3<T>, 3> tri = {{
                mesh.vertices.at(static_cast<size_t>(vertex_indices.at(0))),
                mesh.vertices.at(static_cast<size_t>(vertex_indices.at(1))),
                mesh.vertices.at(static_cast<size_t>(vertex_indices.at(2)))
            }};
            if(!(triangle_normal(tri).sq_length() > static_cast<T>(0))){
                continue;
            }

            mesh_intersection_triangle<T, I> mt;
            mt.face_index = checked_index_cast<I>(face_idx);
            mt.vertex_indices = vertex_indices;
            mt.verts = tri;
            mt.bbox = triangle_bbox(tri);
            out.push_back(mt);
        }
    }
    return out;
}

template <class T, class I>
T
mesh_intersection_eps(const fv_surface_mesh<T, I> &lhs,
                      const fv_surface_mesh<T, I> &rhs,
                      T snap_eps){
    if(snap_eps > static_cast<T>(0)){
        return snap_eps;
    }

    T scale = static_cast<T>(1);
    const auto accumulate_scale = [&scale](const std::vector<vec3<T>> &verts){
        for(const auto &v : verts){
            scale = std::max(scale, std::abs(v.x));
            scale = std::max(scale, std::abs(v.y));
            scale = std::max(scale, std::abs(v.z));
        }
    };
    accumulate_scale(lhs.vertices);
    accumulate_scale(rhs.vertices);
    return std::sqrt(std::numeric_limits<T>::epsilon()) * scale;
}

template <class I>
void
sort_and_dedup_primitives(std::vector<fv_surface_mesh_intersection_primitive<I>> &primitives){
    std::sort(primitives.begin(), primitives.end(), [](const auto &lhs, const auto &rhs){
        if(lhs.type != rhs.type){
            return static_cast<int>(lhs.type) < static_cast<int>(rhs.type);
        }
        return lhs.indices < rhs.indices;
    });
    primitives.erase(std::unique(primitives.begin(), primitives.end(), [](const auto &lhs, const auto &rhs){
        return (lhs.type == rhs.type) && (lhs.indices == rhs.indices);
    }), primitives.end());
}

template <class T, class I>
std::vector<fv_surface_mesh_intersection_primitive<I>>
collect_triangle_primitives(const mesh_intersection_triangle<T, I> &tri,
                            const fv_surface_mesh_intersection_geometry<T> &geometry,
                            T eps){
    std::vector<fv_surface_mesh_intersection_primitive<I>> out;
    out.push_back(fv_surface_mesh_intersection_primitive<I>{
        fv_surface_mesh_intersection_primitive_type::Face,
        { tri.face_index }
    });

    for(size_t i = 0; i < 3UL; ++i){
        for(const auto &p : geometry.vertices){
            if(p.distance(tri.verts.at(i)) <= eps){
                out.push_back(fv_surface_mesh_intersection_primitive<I>{
                    fv_surface_mesh_intersection_primitive_type::Vertex,
                    { tri.vertex_indices.at(i) }
                });
                break;
            }
        }
    }

    auto add_edge_if_active = [&](size_t i, size_t j){
        const auto &a = tri.verts.at(i);
        const auto &b = tri.verts.at(j);
        bool active = false;

        for(const auto &p : geometry.vertices){
            if(point_on_open_segment(p, a, b)){
                active = true;
                break;
            }
        }

        if(!active && (geometry.vertices.size() >= 2UL)){
            const size_t edge_count = (geometry.type == fv_surface_mesh_intersection_geometry_type::Polygon)
                                    ? geometry.vertices.size()
                                    : (geometry.vertices.size() - 1UL);
            for(size_t k = 0; k < edge_count; ++k){
                const auto &p0 = geometry.vertices.at(k);
                const auto &p1 = geometry.vertices.at((k + 1UL) % geometry.vertices.size());
                if((p0.distance(p1) <= eps)){
                    continue;
                }
                if(point_on_closed_segment(p0, a, b) && point_on_closed_segment(p1, a, b)){
                    active = true;
                    break;
                }
            }
        }

        if(active){
            auto edge = std::vector<I>{ tri.vertex_indices.at(i), tri.vertex_indices.at(j) };
            std::sort(edge.begin(), edge.end());
            out.push_back(fv_surface_mesh_intersection_primitive<I>{
                fv_surface_mesh_intersection_primitive_type::Edge,
                edge
            });
        }
    };

    add_edge_if_active(0UL, 1UL);
    add_edge_if_active(1UL, 2UL);
    add_edge_if_active(2UL, 0UL);
    sort_and_dedup_primitives(out);
    return out;
}

template <class T, class I>
bool
intersection_less(const fv_surface_mesh_intersection<T, I> &lhs,
                  const fv_surface_mesh_intersection<T, I> &rhs){
    if(lhs.lhs_face_index != rhs.lhs_face_index){
        return lhs.lhs_face_index < rhs.lhs_face_index;
    }
    if(lhs.rhs_face_index != rhs.rhs_face_index){
        return lhs.rhs_face_index < rhs.rhs_face_index;
    }
    if(lhs.lhs_geometry.type != rhs.lhs_geometry.type){
        return static_cast<int>(lhs.lhs_geometry.type) < static_cast<int>(rhs.lhs_geometry.type);
    }
    if(lhs.lhs_geometry.vertices != rhs.lhs_geometry.vertices){
        return lhs.lhs_geometry.vertices < rhs.lhs_geometry.vertices;
    }

    const auto prim_equal = [](const fv_surface_mesh_intersection_primitive<I> &a,
                               const fv_surface_mesh_intersection_primitive<I> &b){
        return (a.type == b.type) && (a.indices == b.indices);
    };
    const auto prim_less = [](const fv_surface_mesh_intersection_primitive<I> &a,
                              const fv_surface_mesh_intersection_primitive<I> &b){
        if(a.type != b.type){
            return static_cast<int>(a.type) < static_cast<int>(b.type);
        }
        return a.indices < b.indices;
    };
    const auto prims_equal = [&](const std::vector<fv_surface_mesh_intersection_primitive<I>> &a,
                                 const std::vector<fv_surface_mesh_intersection_primitive<I>> &b){
        return (a.size() == b.size()) && std::equal(a.begin(), a.end(), b.begin(), prim_equal);
    };

    if(!prims_equal(lhs.lhs_primitives, rhs.lhs_primitives)){
        return std::lexicographical_compare(lhs.lhs_primitives.begin(), lhs.lhs_primitives.end(),
                                            rhs.lhs_primitives.begin(), rhs.lhs_primitives.end(),
                                            prim_less);
    }
    return std::lexicographical_compare(lhs.rhs_primitives.begin(), lhs.rhs_primitives.end(),
                                        rhs.rhs_primitives.begin(), rhs.rhs_primitives.end(),
                                        prim_less);
}

template <class T, class I>
bool
intersections_equal(const fv_surface_mesh_intersection<T, I> &lhs,
                    const fv_surface_mesh_intersection<T, I> &rhs){
    const auto primitive_equal = [](const fv_surface_mesh_intersection_primitive<I> &lp,
                                    const fv_surface_mesh_intersection_primitive<I> &rp){
        return (lp.type == rp.type) && (lp.indices == rp.indices);
    };
    return (lhs.lhs_face_index == rhs.lhs_face_index)
        && (lhs.rhs_face_index == rhs.rhs_face_index)
        && (lhs.lhs_geometry.type == rhs.lhs_geometry.type)
        && (lhs.lhs_geometry.vertices == rhs.lhs_geometry.vertices)
        && (lhs.lhs_primitives.size() == rhs.lhs_primitives.size())
        && (lhs.rhs_primitives.size() == rhs.rhs_primitives.size())
        && std::equal(lhs.lhs_primitives.begin(), lhs.lhs_primitives.end(), rhs.lhs_primitives.begin(), primitive_equal)
        && std::equal(lhs.rhs_primitives.begin(), lhs.rhs_primitives.end(), rhs.rhs_primitives.begin(), primitive_equal);
}

template <class T, class I>
fv_surface_mesh_intersection_geometry<T>
make_geometry(const triangle_intersection_result<T> &isect){
    fv_surface_mesh_intersection_geometry<T> out;
    if(isect.kind == triangle_intersection_kind::Point){
        out.type = fv_surface_mesh_intersection_geometry_type::Point;
    }else if(isect.kind == triangle_intersection_kind::Segment){
        out.type = fv_surface_mesh_intersection_geometry_type::Segment;
    }else{
        out.type = fv_surface_mesh_intersection_geometry_type::Polygon;
    }
    out.vertices = isect.points;
    return out;
}

} // namespace

template <class T, class I>
std::vector<fv_surface_mesh_intersection<T, I>>
FindIntersections(const fv_surface_mesh<T, I> &lhs,
                  const fv_surface_mesh<T, I> &rhs,
                  T snap_eps){
    std::vector<fv_surface_mesh_intersection<T, I>> out;

    const auto eps = mesh_intersection_eps(lhs, rhs, snap_eps);
    const auto lhs_tris = collect_intersection_triangles(lhs);
    const auto rhs_tris = collect_intersection_triangles(rhs);
    if(lhs_tris.empty() || rhs_tris.empty()){
        return out;
    }

    rtree<T> rhs_index;
    for(size_t rhs_idx = 0; rhs_idx < rhs_tris.size(); ++rhs_idx){
        rhs_index.insert(rhs_tris.at(rhs_idx).bbox, std::make_any<size_t>(rhs_idx));
    }

    for(const auto &lhs_tri : lhs_tris){
        const auto candidates = rhs_index.search(lhs_tri.bbox);
        for(const auto &candidate : candidates){
            const auto rhs_idx = std::any_cast<size_t>(candidate.aux_data);
            const auto &rhs_tri = rhs_tris.at(rhs_idx);
            const auto isect = intersect_triangles(lhs_tri.verts, rhs_tri.verts, eps);
            if(isect.kind == triangle_intersection_kind::None){
                continue;
            }

            fv_surface_mesh_intersection<T, I> record;
            record.lhs_face_index = lhs_tri.face_index;
            record.rhs_face_index = rhs_tri.face_index;
            record.lhs_geometry = make_geometry<T, I>(isect);
            record.rhs_geometry = record.lhs_geometry;
            record.lhs_primitives = collect_triangle_primitives(lhs_tri, record.lhs_geometry, eps);
            record.rhs_primitives = collect_triangle_primitives(rhs_tri, record.rhs_geometry, eps);
            out.push_back(record);
        }
    }

    std::sort(out.begin(), out.end(), intersection_less<T, I>);
    out.erase(std::unique(out.begin(), out.end(), intersections_equal<T, I>), out.end());
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::vector<fv_surface_mesh_intersection<float , uint32_t>>
        FindIntersections(const fv_surface_mesh<float , uint32_t> &, const fv_surface_mesh<float , uint32_t> &, float );
    template std::vector<fv_surface_mesh_intersection<float , uint64_t>>
        FindIntersections(const fv_surface_mesh<float , uint64_t> &, const fv_surface_mesh<float , uint64_t> &, float );

    template std::vector<fv_surface_mesh_intersection<double, uint32_t>>
        FindIntersections(const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &, double);
    template std::vector<fv_surface_mesh_intersection<double, uint64_t>>
        FindIntersections(const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &, double);
#endif
