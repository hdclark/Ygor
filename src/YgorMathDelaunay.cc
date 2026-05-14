//YgorMathDelaunay.cc.

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMathDelaunay.h"

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

template <class T>
uint64_t hash_coord_bits(T value){
    typename std::conditional<(sizeof(T) <= sizeof(uint32_t)), uint32_t, uint64_t>::type bits = 0;
    std::memcpy(&bits, &value, sizeof(T));
    return static_cast<uint64_t>(bits);
}

inline uint64_t mix_perturbation_bits(uint64_t state){
    state ^= (state >> 33);
    state *= 0xff51afd7ed558ccdULL;
    state ^= (state >> 33);
    state *= 0xc4ceb9fe1a85ec53ULL;
    state ^= (state >> 33);
    return state;
}

template <class T>
T step_towards(T value, bool toward_positive, unsigned steps){
    const auto limit = toward_positive ? std::numeric_limits<T>::infinity()
                                       : -std::numeric_limits<T>::infinity();
    for(unsigned i = 0; i < steps; ++i){
        value = std::nextafter(value, limit);
    }
    return value;
}

template <class T>
std::vector<vec2<T>> make_working_vertices(const std::vector<vec2<T>> &verts){
    std::vector<vec2<T>> working = verts;
    for(auto &vert : working){
        if(!is_finite_2d(vert)){
            continue;
        }
        auto seed = mix_perturbation_bits(hash_coord_bits(vert.x) ^ (hash_coord_bits(vert.y) << 1));
        const auto x_steps = static_cast<unsigned>((seed & 0x3ULL) + 1ULL);
        seed >>= 2;
        const auto y_steps = static_cast<unsigned>((seed & 0x3ULL) + 1ULL);
        seed >>= 2;
        const bool x_positive = (seed & 0x1ULL) != 0;
        seed >>= 1;
        const bool y_positive = (seed & 0x1ULL) != 0;

        vert.x = step_towards(vert.x, x_positive, x_steps);
        vert.y = step_towards(vert.y, y_positive, y_steps);
    }
    return working;
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
    const auto machine_eps = std::sqrt( std::numeric_limits<T>::epsilon() ) * 10.0;
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

    const auto working_verts = make_working_vertices(verts);

    // Create a super-triangle that encompasses all vertices.
    // Make the super-triangle large enough to contain all points.
    const auto dx = max_x - min_x;
    const auto dy = max_y - min_y;
    auto delta = std::max(dx, dy);

    // Handle degenerate case where all vertices are at the same location (or all non-finite).
    // Use a minimum non-zero delta to create a valid super-triangle.
    if(delta < machine_eps){
        YLOGWARN("Delaunay triangulation bounding box is numerically degenerate; falling back to a unit super-triangle scale");
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
    for(const auto &v : working_verts){
        all_verts.push_back(v);
    }

    // Start with just the super-triangle.
    std::vector<DelaunayTriangle> triangles;
    triangles.push_back(DelaunayTriangle{0, 1, 2, false});

    // Add each vertex one at a time.
    for(size_t i = 3; i < all_verts.size(); ++i){
        const auto &P = all_verts[i];

        // Skip non-finite points.
        if(!std::isfinite(P.x) || !std::isfinite(P.y)){
            continue;
        }

        // Find all triangles whose circumcircle contains this point.
        for(auto &tri : triangles){
            if(tri.bad){
                continue;
            }
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
        std::unordered_map<DelaunayEdge, size_t, EdgeHash> edge_count;
        for(const auto &tri : triangles){
            if(!tri.bad){
                continue;
            }
            // Count each edge of this triangle.
            ++edge_count[make_edge(tri.a, tri.b)];
            ++edge_count[make_edge(tri.b, tri.c)];
            ++edge_count[make_edge(tri.c, tri.a)];
        }

        // Boundary edges are those that appear exactly once.
        std::vector<DelaunayEdge> polygon;
        for(const auto &[edge, count] : edge_count){
            if(count == 1){
                polygon.push_back(edge);
            }
        }
        if(polygon.empty()){
            YLOGWARN("Failed insertion of vertex " << (i - 3)
                     << " because no Bowyer-Watson cavity boundary edges were found");
            throw std::runtime_error("Bowyer-Watson insertion failed: no cavity boundary edges were found for an inserted vertex");
        }

        // Remove the bad triangles.
        triangles.erase(
            std::remove_if(triangles.begin(), triangles.end(),
                           [](const DelaunayTriangle &t){ return t.bad; }),
            triangles.end());

        // Re-triangulate the polygon hole by connecting each edge to the new point.
        for(const auto &edge : polygon){
            DelaunayTriangle next{};
            if(make_ccw_triangle(all_verts, edge.a, edge.b, i, next)){
                triangles.push_back(next);
            }
        }
    }

    // Remove triangles that share vertices with the super-triangle.
    triangles.erase(
        std::remove_if(triangles.begin(), triangles.end(),
                       [](const DelaunayTriangle &t){
                            return (t.a < 3) || (t.b < 3) || (t.c < 3);
                        }),
        triangles.end());
    prune_triangles(all_verts, triangles);

    if(triangles.empty()){
        YLOGDEBUG("Delaunay triangulation did not produce any finite triangles after removing the super-triangle");
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
        mesh.faces.push_back({ static_cast<I>(tri.a - 3),
                               static_cast<I>(tri.b - 3),
                               static_cast<I>(tri.c - 3) });
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
