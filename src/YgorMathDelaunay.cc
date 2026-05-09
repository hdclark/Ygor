//YgorMAthDelaunay.cc.

#include <algorithm>   //Needed for std::reverse.
#include <any>
#include <cmath>       //Needed for fabs, signbit, sqrt, etc...
#include <limits>      //Needed for std::numeric_limits::max().
#include <memory>
#include <tuple>
#include <unordered_map>
#include <utility>     //Needed for std::pair.
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorMathDelaunay.h"

//#ifndef YGOR_MATH_DELAUNAY_DISABLE_ALL_SPECIALIZATIONS
//     #define YGOR_MATH_DELAUNAY_DISABLE_ALL_SPECIALIZATIONS
//#endif


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

    fv_surface_mesh<T, I> mesh;
    const auto N_verts = verts.size();

    // Need at least 3 vertices to form a triangle.
    if(N_verts < 3){
        return mesh;
    }

    const auto machine_eps = std::sqrt( std::numeric_limits<T>::epsilon() );

    // Compute bounding box for all vertices.
    T min_x = std::numeric_limits<T>::max();
    T max_x = std::numeric_limits<T>::lowest();
    T min_y = std::numeric_limits<T>::max();
    T max_y = std::numeric_limits<T>::lowest();

    size_t N_finite_verts = 0;
    for(const auto &v : verts){
        if(!std::isfinite(v.x) || !std::isfinite(v.y)){
            continue;
        }
        ++N_finite_verts;
        min_x = std::min(min_x, v.x);
        max_x = std::max(max_x, v.x);
        min_y = std::min(min_y, v.y);
        max_y = std::max(max_y, v.y);
    }
    if(N_finite_verts < 3){
        return mesh;
    }

    // Create a super-triangle that encompasses all vertices.
    // Make the super-triangle large enough to contain all points.
    const auto dx = max_x - min_x;
    const auto dy = max_y - min_y;
    auto delta = std::max(dx, dy);

    // Handle degenerate case where all vertices are at the same location (or all non-finite).
    // Use a minimum non-zero delta to create a valid super-triangle.
    if(delta < machine_eps){
        delta = static_cast<T>(1);
    }

    const auto mid_x = (min_x + max_x) / static_cast<T>(2);
    const auto mid_y = (min_y + max_y) / static_cast<T>(2);

    // Super-triangle vertices (we make it much larger than necessary for robustness).
    const vec2<T> super_A(mid_x - static_cast<T>(20) * delta, mid_y - delta);
    const vec2<T> super_B(mid_x + static_cast<T>(20) * delta, mid_y - delta);
    const vec2<T> super_C(mid_x, mid_y + static_cast<T>(20) * delta);

    // We store all vertices including super-triangle vertices.
    // Indices 0, 1, 2 are super-triangle vertices.
    std::vector<vec2<T>> all_verts;
    all_verts.reserve(N_verts + 3);
    all_verts.push_back(super_A);
    all_verts.push_back(super_B);
    all_verts.push_back(super_C);
    for(const auto &v : verts){
        all_verts.push_back(v);
    }

    // A triangle is represented by three vertex indices.
    struct Triangle {
        I a, b, c;
        bool bad = false;
    };

    // An edge is represented by two vertex indices; equality is independent of order.
    struct Edge {
        I p1, p2;
        bool operator==(const Edge &other) const {
            return (p1 == other.p1 && p2 == other.p2) || (p1 == other.p2 && p2 == other.p1);
        }
    };

    // Hash function for Edge to enable use in unordered containers.
    // Order-independent: hash(p1, p2) == hash(p2, p1).
    struct EdgeHash {
        std::size_t operator()(const Edge &e) const {
            const auto lo = std::min(e.p1, e.p2);
            const auto hi = std::max(e.p1, e.p2);
            return std::hash<I>()(lo) ^ (std::hash<I>()(hi) << 1);
        }
    };

    // Start with just the super-triangle.
    std::vector<Triangle> triangles;
    triangles.push_back(Triangle{static_cast<I>(0), static_cast<I>(1), static_cast<I>(2), false});

    // Add each vertex one at a time.
    for(size_t i = 3; i < all_verts.size(); ++i){
        const auto &P = all_verts[i];

        // Skip non-finite points.
        if(!std::isfinite(P.x) || !std::isfinite(P.y)){
            continue;
        }

        // Find all triangles whose circumcircle contains this point.
        for(auto &tri : triangles){
            if(tri.bad) continue;
            const auto &A = all_verts[tri.a];
            const auto &B = all_verts[tri.b];
            const auto &C = all_verts[tri.c];
            if(incircle_sign(A, B, C, P) > 0){
                tri.bad = true;
            }
        }

        // Find the boundary edges of the polygon hole created by removing bad triangles.
        // An edge is on the boundary if it appears exactly once among all bad triangles.
        // Using a hash map keeps this step linear in the number of bad triangles.
        std::unordered_map<Edge, size_t, EdgeHash> edge_count;
        for(const auto &tri : triangles){
            if(!tri.bad) continue;

            // Count each edge of this triangle.
            edge_count[Edge{tri.a, tri.b}]++;
            edge_count[Edge{tri.b, tri.c}]++;
            edge_count[Edge{tri.c, tri.a}]++;
        }

        // Boundary edges are those that appear exactly once.
        std::vector<Edge> polygon;
        for(const auto &[edge, count] : edge_count){
            if(count == 1){
                polygon.push_back(edge);
            }
        }

        // Remove the bad triangles.
        triangles.erase(
            std::remove_if(triangles.begin(), triangles.end(),
                           [](const Triangle &t){ return t.bad; }),
            triangles.end());

        // Re-triangulate the polygon hole by connecting each edge to the new point.
        const auto point_idx = static_cast<I>(i);
        for(const auto &edge : polygon){
            triangles.push_back(Triangle{edge.p1, edge.p2, point_idx, false});
        }
    }

    // Remove triangles that share vertices with the super-triangle.
    triangles.erase(
        std::remove_if(triangles.begin(), triangles.end(),
                       [](const Triangle &t){
                            return (t.a < 3) || (t.b < 3) || (t.c < 3);
                        }),
        triangles.end());

    // Build the output mesh.
    // The mesh vertices should be the original input vertices (not the super-triangle).
    mesh.vertices.reserve(verts.size());
    for(const auto &vert : verts){
        mesh.vertices.emplace_back(vert.x, vert.y, static_cast<T>(0));
    }

    // Adjust triangle indices: subtract 3 because we removed super-triangle vertices.
    for(const auto &tri : triangles){
        const auto &A = all_verts[tri.a];
        const auto &B = all_verts[tri.b];
        const auto &C = all_verts[tri.c];
        if(orient_sign(A, B, C) == 0){
            continue;
        }
        std::vector<I> face;
        face.push_back(static_cast<I>(tri.a - 3));
        face.push_back(static_cast<I>(tri.b - 3));
        face.push_back(static_cast<I>(tri.c - 3));
        mesh.faces.push_back(std::move(face));
    }

    return mesh;
}
#ifndef YGOR_MATH_DELAUNAY_DISABLE_ALL_SPECIALIZATIONS
    template fv_surface_mesh<float , uint32_t> Delaunay_Triangulation_2(const std::vector<vec2<float >> &);
    template fv_surface_mesh<float , uint64_t> Delaunay_Triangulation_2(const std::vector<vec2<float >> &);

    template fv_surface_mesh<double, uint32_t> Delaunay_Triangulation_2(const std::vector<vec2<double>> &);
    template fv_surface_mesh<double, uint64_t> Delaunay_Triangulation_2(const std::vector<vec2<double>> &);
#endif
