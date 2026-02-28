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
// The input is a collection of vec3<T> where the z-component is expected to be zero (2D points in a plane).
// The triangulation is performed on the x-y plane only; the z-coordinate is ignored.
//
// Returns an fv_surface_mesh<T, I> containing the triangulation as faces.
//
// References:
//  - Bowyer A. Computing dirichlet tessellations. The Computer Journal. 1981;24(2):162-166.
//  - Watson DF. Computing the n-dimensional Delaunay tessellation with application to Voronoi polytopes.
//    The Computer Journal. 1981;24(2):167-172.
template <class T, class I>
fv_surface_mesh<T, I>
Delaunay_Triangulation_2(const std::vector<vec3<T>> &verts) {

    fv_surface_mesh<T, I> mesh;
    const auto N_verts = verts.size();

    // Need at least 3 vertices to form a triangle.
    if(N_verts < 3){
        return mesh;
    }

    const auto machine_eps = std::sqrt( std::numeric_limits<T>::epsilon() );

    // Helper to compute circumcircle for a triangle (2D).
    // Returns the circumcenter (x, y) and squared radius.
    const auto compute_circumcircle = [&](const vec3<T> &A, const vec3<T> &B, const vec3<T> &C) 
        -> std::tuple<T, T, T> {
        // Using the formula for circumcenter of a triangle:
        // D = 2*(Ax*(By-Cy) + Bx*(Cy-Ay) + Cx*(Ay-By))
        const auto D = static_cast<T>(2) * (A.x * (B.y - C.y) + B.x * (C.y - A.y) + C.x * (A.y - B.y));

        // Use scale-aware tolerance for the determinant.
        const auto max_coord = std::max({std::abs(A.x), std::abs(B.x), std::abs(C.x),
                                         std::abs(A.y), std::abs(B.y), std::abs(C.y),
                                         static_cast<T>(1)});
        const auto det_eps = machine_eps * max_coord * max_coord;
        if(std::abs(D) < det_eps){
            // Degenerate triangle (collinear points).
            return std::make_tuple(std::numeric_limits<T>::quiet_NaN(),
                                   std::numeric_limits<T>::quiet_NaN(),
                                   std::numeric_limits<T>::quiet_NaN());
        }

        const auto A_sq = A.x * A.x + A.y * A.y;
        const auto B_sq = B.x * B.x + B.y * B.y;
        const auto C_sq = C.x * C.x + C.y * C.y;

        const auto cx = (A_sq * (B.y - C.y) + B_sq * (C.y - A.y) + C_sq * (A.y - B.y)) / D;
        const auto cy = (A_sq * (C.x - B.x) + B_sq * (A.x - C.x) + C_sq * (B.x - A.x)) / D;

        const auto r_sq = (A.x - cx) * (A.x - cx) + (A.y - cy) * (A.y - cy);
        return std::make_tuple(cx, cy, r_sq);
    };

    // Helper to check if a point is strictly inside the circumcircle of a triangle.
    // Points on the circle boundary are not considered "inside".
    const auto point_in_circumcircle = [&](const vec3<T> &P, const vec3<T> &A, const vec3<T> &B, const vec3<T> &C) -> bool {
        const auto [cx, cy, r_sq] = compute_circumcircle(A, B, C);
        if(!std::isfinite(cx) || !std::isfinite(cy) || !std::isfinite(r_sq)){
            return false;
        }
        const auto dist_sq = (P.x - cx) * (P.x - cx) + (P.y - cy) * (P.y - cy);
        // Use a small scale-aware tolerance to handle numerical precision.
        const auto r_tol = r_sq * machine_eps;
        return dist_sq < (r_sq - r_tol);
    };

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
    const vec3<T> super_A(mid_x - static_cast<T>(20) * delta, mid_y - delta, static_cast<T>(0));
    const vec3<T> super_B(mid_x + static_cast<T>(20) * delta, mid_y - delta, static_cast<T>(0));
    const vec3<T> super_C(mid_x, mid_y + static_cast<T>(20) * delta, static_cast<T>(0));

    // We store all vertices including super-triangle vertices.
    // Indices 0, 1, 2 are super-triangle vertices.
    std::vector<vec3<T>> all_verts;
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
            if(point_in_circumcircle(P, A, B, C)){
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
    mesh.vertices = verts;

    // Adjust triangle indices: subtract 3 because we removed super-triangle vertices.
    for(const auto &tri : triangles){
        const auto &A = all_verts[tri.a];
        const auto &B = all_verts[tri.b];
        const auto &C = all_verts[tri.c];
        const auto area2 = (B.x - A.x) * (C.y - A.y) - (B.y - A.y) * (C.x - A.x);
        if(std::abs(area2) <= machine_eps){
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
    template fv_surface_mesh<float , uint32_t> Delaunay_Triangulation_2(const std::vector<vec3<float >> &);
    template fv_surface_mesh<float , uint64_t> Delaunay_Triangulation_2(const std::vector<vec3<float >> &);

    template fv_surface_mesh<double, uint32_t> Delaunay_Triangulation_2(const std::vector<vec3<double>> &);
    template fv_surface_mesh<double, uint64_t> Delaunay_Triangulation_2(const std::vector<vec3<double>> &);
#endif
