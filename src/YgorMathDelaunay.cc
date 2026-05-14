//YgorMathDelaunay.cc.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMathDelaunay.h"
#include "YgorMeshesAdaptivePredicates.h"
#include "YgorMeshesConvexHull.h"

//#ifndef YGOR_MATH_DELAUNAY_DISABLE_ALL_SPECIALIZATIONS
//     #define YGOR_MATH_DELAUNAY_DISABLE_ALL_SPECIALIZATIONS
//#endif

namespace {

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

// A triangle is represented by three vertex indices.
struct DelaunayTriangle {
    size_t a;
    size_t b;
    size_t c;
    bool bad = false;
};

// An edge is represented by two vertex indices; equality is independent of order.
struct DelaunayEdge {
    size_t a;
    size_t b;

    bool operator==(const DelaunayEdge &other) const {
        return (a == other.a) && (b == other.b);
    }
};

inline DelaunayEdge make_edge(size_t a, size_t b){
    if(b < a){
        std::swap(a, b);
    }
    return DelaunayEdge{a, b};
}

// Hash function for Edge to enable use in unordered containers.
// Order-independent: hash(p1, p2) == hash(p2, p1).
struct EdgeHash {
    std::size_t operator()(const DelaunayEdge &edge) const {
        return std::hash<size_t>()(edge.a) ^ (std::hash<size_t>()(edge.b) << 1);
    }
};

template <class T>
bool make_ccw_triangle(const std::vector<vec2<T>> &verts,
                       size_t a,
                       size_t b,
                       size_t c,
                       DelaunayTriangle &out){
    const auto sign = orient_sign(verts.at(a), verts.at(b), verts.at(c));
    if(sign > 0){
        out = DelaunayTriangle{a, b, c, false};
        return true;
    }
    if(sign < 0){
        out = DelaunayTriangle{a, c, b, false};
        return true;
    }
    return false;
}

template <class T>
void prune_triangles(const std::vector<vec2<T>> &verts,
                     std::vector<DelaunayTriangle> &triangles){
    std::set<std::array<size_t, 3>> seen;
    std::vector<DelaunayTriangle> filtered;
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
bool is_lower_lifted_face(const vec3<T> &a,
                          const vec3<T> &b,
                          const vec3<T> &c,
                          const vec3<T> &below_point){
    const std::array<T, 3> pa{{ a.x, a.y, a.z }};
    const std::array<T, 3> pb{{ b.x, b.y, b.z }};
    const std::array<T, 3> pc{{ c.x, c.y, c.z }};
    const std::array<T, 3> pd{{ below_point.x, below_point.y, below_point.z }};
    return adaptive_predicate::orient3d(pa.data(), pb.data(), pc.data(), pd.data()) < static_cast<T>(0);
}

template <class T>
std::vector<DelaunayTriangle> build_delaunay_triangles(const std::vector<vec2<T>> &verts){
    std::vector<DelaunayTriangle> triangles;
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
        return triangles;
    }
    if(unique_verts.size() == 3){
        DelaunayTriangle tri{};
        if(make_ccw_triangle(verts,
                             unique_to_original.at(std::make_pair(unique_verts.at(0).x, unique_verts.at(0).y)),
                             unique_to_original.at(std::make_pair(unique_verts.at(1).x, unique_verts.at(1).y)),
                             unique_to_original.at(std::make_pair(unique_verts.at(2).x, unique_verts.at(2).y)),
                             tri)){
            triangles.push_back(tri);
        }
        return triangles;
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

    IncrementalConvexHull<T> hull;
    hull.add_vertices(lifted_verts);

    const auto &hull_mesh = hull.get_mesh();
    const auto &eval_order = hull.get_evaluation_order();
    const auto below_z = min_z - static_cast<T>(2);
    const vec3<T> below_point(static_cast<T>(0),
                              static_cast<T>(0),
                              below_z);

    triangles.reserve(hull_mesh.faces.size());
    for(const auto &face : hull_mesh.faces){
        if(face.size() != 3){
            continue;
        }
        const auto ia = eval_order.at(face.at(0));
        const auto ib = eval_order.at(face.at(1));
        const auto ic = eval_order.at(face.at(2));
        if(!is_lower_lifted_face(lifted_verts.at(ia), lifted_verts.at(ib), lifted_verts.at(ic), below_point)){
            continue;
        }

        DelaunayTriangle tri{};
        if(make_ccw_triangle(verts,
                             unique_to_original.at(std::make_pair(unique_verts.at(ia).x, unique_verts.at(ia).y)),
                             unique_to_original.at(std::make_pair(unique_verts.at(ib).x, unique_verts.at(ib).y)),
                             unique_to_original.at(std::make_pair(unique_verts.at(ic).x, unique_verts.at(ic).y)),
                             tri)){
            triangles.push_back(tri);
        }
    }

    prune_triangles(verts, triangles);
    return triangles;
}

} // namespace

// 2D Delaunay triangulation using the incremental Bowyer-Watson algorithm.
// 
// The input is a collection of vec2<T> representing 2D points on the x-y plane.
//
// Returns an fv_surface_mesh<T, I> containing the triangulation as faces.
//
// References:
//  - Bowyer A. Computing dirichlet tessellations. The Computer Journal. 1981;24(2):162-166.
//  - Watson DF. Computing the n-dimensional Delaunay tessellation with application to Voronoi polytopes.
//    The Computer Journal. 1981;24(2):167-172.
template <class T, class I>
fv_surface_mesh<T, I>
Delaunay_Triangulation_2(const std::vector<vec2<T>> &verts) {
    const auto N_verts = verts.size();

    // Need at least 3 vertices to form a triangle.
    if(N_verts < 3){
        YLOGWARN("Refusing Delaunay triangulation of " << N_verts << " vertex/vertices; at least 3 are required");
        throw std::invalid_argument("Delaunay triangulation requires at least 3 vertices.");
    }

    // Compute bounding box for all vertices.
    T min_x = std::numeric_limits<T>::max();
    T max_x = std::numeric_limits<T>::lowest();
    T min_y = std::numeric_limits<T>::max();
    T max_y = std::numeric_limits<T>::lowest();

    // Ensure there are >=3 finite vertices.
    size_t N_finite_verts = 0;
    size_t first_nonfinite_index = N_verts;
    for(size_t i = 0; i < verts.size(); ++i){
        const auto &v = verts.at(i);
        if(!is_finite_2d(v)){
            if(first_nonfinite_index == N_verts){
                first_nonfinite_index = i;
            }
            continue;
        }
        ++N_finite_verts;
        min_x = std::min(min_x, v.x);
        max_x = std::max(max_x, v.x);
        min_y = std::min(min_y, v.y);
        max_y = std::max(max_y, v.y);
    }
    if(first_nonfinite_index != N_verts){
        YLOGWARN("Refusing Delaunay triangulation because vertex " << first_nonfinite_index
                 << " is not finite: (" << verts.at(first_nonfinite_index).x
                 << ", " << verts.at(first_nonfinite_index).y << ")");
        throw std::invalid_argument("Delaunay triangulation requires all vertex coordinates to be finite.");
    }
    if(N_finite_verts < 3){
        YLOGWARN("Refusing Delaunay triangulation because only " << N_finite_verts << " finite vertices were provided");
        throw std::invalid_argument("Delaunay triangulation requires at least 3 finite vertices.");
    }

    // Check for duplicate vertices.
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
        YLOGWARN("Delaunay triangulation received " << duplicate_vertices
                 << " pair(s) of duplicate vertices; degenerate duplicates will be ignored");
    }

    if(!has_non_collinear_triplet(verts)){
        YLOGWARN("Refusing Delaunay triangulation because all finite vertices are collinear or coincident");
        throw std::invalid_argument("Delaunay triangulation requires at least one non-collinear triplet of vertices.");
    }
    YLOGDEBUG("Delaunay triangulation input: vertices=" << N_verts
              << ", bbox=[(" << min_x << ", " << min_y << "), (" << max_x << ", " << max_y << ")]");

    auto triangles = build_delaunay_triangles(verts);

    if(triangles.empty()){
        YLOGDEBUG("Delaunay triangulation did not produce any finite triangles");
        throw std::runtime_error("Delaunay triangulation failed to produce any triangles for the provided vertices.");
    }

    // Build the output mesh.
    // The mesh vertices should be the original input vertices (not the super-triangle).
    fv_surface_mesh<T, I> mesh;
    mesh.vertices.reserve(verts.size());
    for(const auto &vert : verts){
        mesh.vertices.emplace_back(vert.x, vert.y, static_cast<T>(0));
    }

    // Adjust triangle indices: subtract 3 because we removed super-triangle vertices.
    for(const auto &tri : triangles){
        mesh.faces.push_back({ static_cast<I>(tri.a),
                               static_cast<I>(tri.b),
                               static_cast<I>(tri.c) });
    }
    if(mesh.faces.empty()){
        YLOGDEBUG("All candidate Delaunay faces were pruned as degenerate");
        throw std::runtime_error("Delaunay triangulation failed because every candidate triangle was degenerate.");
    }

    YLOGDEBUG("Delaunay triangulation produced " << mesh.faces.size() << " triangle(s)");
    return mesh;
}
#ifndef YGOR_MATH_DELAUNAY_DISABLE_ALL_SPECIALIZATIONS
    template fv_surface_mesh<float , uint32_t> Delaunay_Triangulation_2(const std::vector<vec2<float >> &);
    template fv_surface_mesh<float , uint64_t> Delaunay_Triangulation_2(const std::vector<vec2<float >> &);

    template fv_surface_mesh<double, uint32_t> Delaunay_Triangulation_2(const std::vector<vec2<double>> &);
    template fv_surface_mesh<double, uint64_t> Delaunay_Triangulation_2(const std::vector<vec2<double>> &);
#endif
