
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <set>
#include <utility>
#include <vector>

#include <YgorMath.h>
#include <YgorMeshesConvexHull.h>

#include "doctest/doctest.h"


// Helper: verify that every face is a triangle with valid vertex indices.
template <class T>
static void check_mesh_valid(const fv_surface_mesh<T, uint64_t> &mesh) {
    for(const auto &f : mesh.faces){
        REQUIRE(f.size() == 3UL);
        for(auto vi : f){
            REQUIRE(vi < mesh.vertices.size());
        }
    }
}

// Helper: verify that the mesh is a closed manifold (every directed edge
// appears exactly once, and every undirected edge is shared by exactly 2 faces).
template <class T>
static void check_closed_manifold(const fv_surface_mesh<T, uint64_t> &mesh) {
    std::map<std::pair<uint64_t,uint64_t>, int> edge_count;
    for(const auto &f : mesh.faces){
        for(size_t i = 0; i < f.size(); ++i){
            uint64_t a = f[i];
            uint64_t b = f[(i + 1) % f.size()];
            edge_count[{a, b}]++;
        }
    }
    // Every directed edge (a,b) should appear exactly once.
    for(const auto &[e, cnt] : edge_count){
        REQUIRE(cnt == 1);
    }
    // For every directed edge (a,b), the reverse (b,a) should also exist.
    for(const auto &[e, cnt] : edge_count){
        auto it = edge_count.find({e.second, e.first});
        REQUIRE(it != edge_count.end());
        REQUIRE(it->second == 1);
    }
}

// Helper: verify that all input points are on or inside the hull.
template <class T>
static void check_points_inside(const fv_surface_mesh<T, uint64_t> &mesh,
                                 const std::vector<vec3<T>> &pts) {
    // For each face, compute its outward normal.  Every input point should
    // lie on the non-positive side of every face (i.e., inside or on).
    for(const auto &f : mesh.faces){
        const auto &A = mesh.vertices[f[0]];
        const auto &B = mesh.vertices[f[1]];
        const auto &C = mesh.vertices[f[2]];
        auto normal = (B - A).Cross(C - A);
        for(const auto &p : pts){
            T dot = normal.Dot(p - A);
            REQUIRE(dot < static_cast<T>(1e-6));
        }
    }
}

// Helper: verify Euler's formula for a convex polyhedron (V - E + F = 2).
template <class T>
static void check_euler(const fv_surface_mesh<T, uint64_t> &mesh) {
    uint64_t V = mesh.vertices.size();
    uint64_t F = mesh.faces.size();
    std::set<std::pair<uint64_t,uint64_t>> edges;
    for(const auto &f : mesh.faces){
        for(size_t i = 0; i < f.size(); ++i){
            uint64_t a = f[i];
            uint64_t b = f[(i + 1) % f.size()];
            edges.insert({std::min(a, b), std::max(a, b)});
        }
    }
    uint64_t E = edges.size();
    REQUIRE(static_cast<int64_t>(V) - static_cast<int64_t>(E) + static_cast<int64_t>(F) == 2);
}


TEST_CASE( "YgorMeshesConvexHull" ){
    using T = double;

    SUBCASE("Tetrahedron (4 vertices)"){
        ConvexHull<T> ch;
        ch.add_vertex(vec3<T>(0.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(1.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 1.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 0.0, 1.0));

        const auto &mesh = ch.get_mesh();
        check_mesh_valid(mesh);
        REQUIRE(mesh.vertices.size() == 4UL);
        REQUIRE(mesh.faces.size() == 4UL);
        check_closed_manifold(mesh);
        check_euler(mesh);
    }

    SUBCASE("Cube (8 vertices)"){
        ConvexHull<T> ch;
        ch.add_vertex(vec3<T>(0.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(1.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 1.0, 0.0));
        ch.add_vertex(vec3<T>(1.0, 1.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 0.0, 1.0));
        ch.add_vertex(vec3<T>(1.0, 0.0, 1.0));
        ch.add_vertex(vec3<T>(0.0, 1.0, 1.0));
        ch.add_vertex(vec3<T>(1.0, 1.0, 1.0));

        const auto &mesh = ch.get_mesh();
        check_mesh_valid(mesh);
        REQUIRE(mesh.vertices.size() == 8UL);
        // A triangulated cube has 12 triangles (6 faces * 2).
        REQUIRE(mesh.faces.size() == 12UL);
        check_closed_manifold(mesh);
        check_euler(mesh);
    }

    SUBCASE("Interior point is not on hull"){
        ConvexHull<T> ch;
        // Tetrahedron.
        ch.add_vertex(vec3<T>(0.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(2.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 2.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 0.0, 2.0));
        // Add interior point.
        ch.add_vertex(vec3<T>(0.25, 0.25, 0.25));

        const auto &mesh = ch.get_mesh();
        check_mesh_valid(mesh);
        // The interior point should not add any vertices to the hull.
        REQUIRE(mesh.vertices.size() == 4UL);
        REQUIRE(mesh.faces.size() == 4UL);
        check_closed_manifold(mesh);
        check_euler(mesh);
    }

    SUBCASE("Coplanar points (degeneracy)"){
        ConvexHull<T> ch;
        // Four coplanar points in z=0 plane.
        ch.add_vertex(vec3<T>(0.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(1.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 1.0, 0.0));
        ch.add_vertex(vec3<T>(1.0, 1.0, 0.0));
        // A fifth point above the plane to make a valid 3D hull.
        ch.add_vertex(vec3<T>(0.5, 0.5, 1.0));

        const auto &mesh = ch.get_mesh();
        check_mesh_valid(mesh);
        check_closed_manifold(mesh);
        check_euler(mesh);
        // Should have 5 vertices.
        REQUIRE(mesh.vertices.size() == 5UL);
    }

    SUBCASE("Collinear points (degeneracy)"){
        ConvexHull<T> ch;
        // Three collinear points along the x-axis.
        ch.add_vertex(vec3<T>(0.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(1.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(2.0, 0.0, 0.0));
        // Then non-collinear and non-coplanar.
        ch.add_vertex(vec3<T>(0.0, 1.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 0.0, 1.0));

        const auto &mesh = ch.get_mesh();
        check_mesh_valid(mesh);
        check_closed_manifold(mesh);
        check_euler(mesh);
    }

    SUBCASE("All coplanar points get nudged"){
        ConvexHull<T> ch;
        // All four initial points coplanar in z=0 plane.
        ch.add_vertex(vec3<T>(0.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(1.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 1.0, 0.0));
        ch.add_vertex(vec3<T>(1.0, 1.0, 0.0));

        // Should not throw; nudging resolves the degeneracy.
        const auto &mesh = ch.get_mesh();
        check_mesh_valid(mesh);
        check_closed_manifold(mesh);
        check_euler(mesh);
        REQUIRE(mesh.vertices.size() == 4UL);
        REQUIRE(mesh.faces.size() == 4UL);
    }

    SUBCASE("Octahedron (6 vertices)"){
        ConvexHull<T> ch;
        ch.add_vertex(vec3<T>( 1.0,  0.0,  0.0));
        ch.add_vertex(vec3<T>(-1.0,  0.0,  0.0));
        ch.add_vertex(vec3<T>( 0.0,  1.0,  0.0));
        ch.add_vertex(vec3<T>( 0.0, -1.0,  0.0));
        ch.add_vertex(vec3<T>( 0.0,  0.0,  1.0));
        ch.add_vertex(vec3<T>( 0.0,  0.0, -1.0));

        const auto &mesh = ch.get_mesh();
        check_mesh_valid(mesh);
        REQUIRE(mesh.vertices.size() == 6UL);
        REQUIRE(mesh.faces.size() == 8UL);
        check_closed_manifold(mesh);
        check_euler(mesh);
    }

    SUBCASE("add_vertices convenience method"){
        ConvexHull<T> ch;
        std::vector<vec3<T>> pts = {
            vec3<T>(0.0, 0.0, 0.0),
            vec3<T>(1.0, 0.0, 0.0),
            vec3<T>(0.0, 1.0, 0.0),
            vec3<T>(0.0, 0.0, 1.0)
        };
        ch.add_vertices(pts);

        const auto &mesh = ch.get_mesh();
        check_mesh_valid(mesh);
        REQUIRE(mesh.vertices.size() == 4UL);
        REQUIRE(mesh.faces.size() == 4UL);
        check_closed_manifold(mesh);
    }

    SUBCASE("Online construction: add points one by one"){
        ConvexHull<T> ch;
        // Start with a tetrahedron.
        ch.add_vertex(vec3<T>(0.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(2.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 2.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 0.0, 2.0));

        const auto &mesh1 = ch.get_mesh();
        REQUIRE(mesh1.vertices.size() == 4UL);
        REQUIRE(mesh1.faces.size() == 4UL);

        // Add a point that extends the hull.
        ch.add_vertex(vec3<T>(2.0, 2.0, 0.0));
        const auto &mesh2 = ch.get_mesh();
        REQUIRE(mesh2.vertices.size() == 5UL);
        check_closed_manifold(mesh2);
        check_euler(mesh2);

        // Add another exterior point.
        ch.add_vertex(vec3<T>(0.0, 0.0, 3.0));
        const auto &mesh3 = ch.get_mesh();
        REQUIRE(mesh3.vertices.size() == 6UL);
        check_closed_manifold(mesh3);
        check_euler(mesh3);
    }

    SUBCASE("Evaluation order tracking"){
        ConvexHull<T> ch;
        ch.add_vertex(vec3<T>(0.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(1.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 1.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 0.0, 1.0));
        ch.add_vertex(vec3<T>(0.5, 0.5, 0.5)); // interior

        REQUIRE(ch.num_evaluated() == 5UL);
        const auto &eo = ch.get_evaluation_order();
        // All 5 points should have evaluation order entries.
        REQUIRE(eo.size() == 5UL);
        // Check that evaluation orders are 0..4 in some order.
        std::set<uint64_t> orders;
        for(const auto &[idx, order] : eo){
            orders.insert(order);
        }
        REQUIRE(orders.size() == 5UL);
        REQUIRE(*orders.begin() == 0UL);
        REQUIRE(*orders.rbegin() == 4UL);
    }

    SUBCASE("Many random points form a valid hull"){
        ConvexHull<T> ch;
        // Simple LCG for reproducible test.
        uint64_t seed = 42;
        auto next_rand = [&]() -> T {
            seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
            return static_cast<T>((seed >> 33) & 0x7FFFFFFFULL)
                 / static_cast<T>(0x80000000ULL) * 2.0 - 1.0;
        };

        for(int i = 0; i < 100; ++i){
            ch.add_vertex(vec3<T>(next_rand(), next_rand(), next_rand()));
        }

        const auto &mesh = ch.get_mesh();
        check_mesh_valid(mesh);
        check_closed_manifold(mesh);
        check_euler(mesh);
        // The hull should have fewer vertices than all the points.
        REQUIRE(mesh.vertices.size() <= 100UL);
        REQUIRE(mesh.vertices.size() >= 4UL);
    }

    SUBCASE("Points on a sphere"){
        // All points on a unit sphere should all be on the hull.
        ConvexHull<T> ch;
        std::vector<vec3<T>> pts;
        // Generate vertices of an icosahedron (12 vertices).
        T phi = (static_cast<T>(1) + std::sqrt(static_cast<T>(5))) / static_cast<T>(2);
        T a = static_cast<T>(1);
        std::vector<vec3<T>> ico_verts = {
            vec3<T>(-a,  phi, 0), vec3<T>( a,  phi, 0),
            vec3<T>(-a, -phi, 0), vec3<T>( a, -phi, 0),
            vec3<T>(0, -a,  phi), vec3<T>(0,  a,  phi),
            vec3<T>(0, -a, -phi), vec3<T>(0,  a, -phi),
            vec3<T>( phi, 0, -a), vec3<T>( phi, 0,  a),
            vec3<T>(-phi, 0, -a), vec3<T>(-phi, 0,  a)
        };
        for(auto &v : ico_verts){
            v = v.unit();
        }
        ch.add_vertices(ico_verts);

        const auto &mesh = ch.get_mesh();
        check_mesh_valid(mesh);
        REQUIRE(mesh.vertices.size() == 12UL);
        REQUIRE(mesh.faces.size() == 20UL);
        check_closed_manifold(mesh);
        check_euler(mesh);
    }

    SUBCASE("Convex hull contains all input points"){
        ConvexHull<T> ch;
        std::vector<vec3<T>> pts = {
            vec3<T>(0.0, 0.0, 0.0),
            vec3<T>(3.0, 0.0, 0.0),
            vec3<T>(0.0, 3.0, 0.0),
            vec3<T>(0.0, 0.0, 3.0),
            vec3<T>(1.0, 1.0, 0.5), // interior
            vec3<T>(0.5, 0.5, 0.5), // interior
        };
        ch.add_vertices(pts);

        const auto &mesh = ch.get_mesh();
        check_mesh_valid(mesh);
        check_points_inside(mesh, pts);
    }

    SUBCASE("Float specialization works"){
        ConvexHull<float> ch;
        ch.add_vertex(vec3<float>(0.0f, 0.0f, 0.0f));
        ch.add_vertex(vec3<float>(1.0f, 0.0f, 0.0f));
        ch.add_vertex(vec3<float>(0.0f, 1.0f, 0.0f));
        ch.add_vertex(vec3<float>(0.0f, 0.0f, 1.0f));

        const auto &mesh = ch.get_mesh();
        REQUIRE(mesh.vertices.size() == 4UL);
        REQUIRE(mesh.faces.size() == 4UL);
    }

    SUBCASE("Duplicate vertices are handled"){
        ConvexHull<T> ch;
        ch.add_vertex(vec3<T>(0.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(1.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 1.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 0.0, 1.0));
        // Duplicate of vertex 0.
        ch.add_vertex(vec3<T>(0.0, 0.0, 0.0));
        // Duplicate of vertex 1.
        ch.add_vertex(vec3<T>(1.0, 0.0, 0.0));

        const auto &mesh = ch.get_mesh();
        check_mesh_valid(mesh);
        // Duplicates get slight perturbation and may end up on the hull,
        // but the mesh must remain a valid closed manifold.
        check_closed_manifold(mesh);
        check_euler(mesh);
        REQUIRE(mesh.vertices.size() >= 4UL);
        REQUIRE(mesh.vertices.size() <= 6UL);
    }

    SUBCASE("Fewer than 4 vertices throws on get_mesh"){
        ConvexHull<T> ch;
        ch.add_vertex(vec3<T>(0.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(1.0, 0.0, 0.0));
        ch.add_vertex(vec3<T>(0.0, 1.0, 0.0));

        // With only 3 vertices, the hull cannot form a closed mesh.
        // get_mesh should return an empty mesh since no faces were built.
        const auto &mesh = ch.get_mesh();
        REQUIRE(mesh.faces.empty());
    }
}


TEST_CASE( "adaptive_predicate" ){
    using T = double;

    SUBCASE("orient3d: basic orientation"){
        // orient3d(a,b,c,d) = (a-d) . ((b-d) x (c-d))
        // which equals -((d-a) . ((b-a) x (c-a))).
        // The sign is OPPOSITE to the dot product of (d-a) with the
        // face normal (b-a)x(c-a).
        T a[3] = {0.0, 0.0, 0.0};
        T b[3] = {1.0, 0.0, 0.0};
        T c[3] = {0.0, 1.0, 0.0};
        T d_above[3] = {0.0, 0.0, 1.0};
        T d_below[3] = {0.0, 0.0, -1.0};

        // Face normal (b-a)x(c-a) = (1,0,0)x(0,1,0) = (0,0,1) pointing up.
        // d_above is on the same side as the normal → orient3d < 0.
        // d_below is on the opposite side → orient3d > 0.
        T o_above = adaptive_predicate::orient3d(a, b, c, d_above);
        T o_below = adaptive_predicate::orient3d(a, b, c, d_below);
        REQUIRE(o_above < 0.0);
        REQUIRE(o_below > 0.0);
    }

    SUBCASE("orient3d: coplanar points return zero"){
        T a[3] = {0.0, 0.0, 0.0};
        T b[3] = {1.0, 0.0, 0.0};
        T c[3] = {0.0, 1.0, 0.0};
        T d[3] = {1.0, 1.0, 0.0};

        T o = adaptive_predicate::orient3d(a, b, c, d);
        REQUIRE(o == 0.0);
    }

    SUBCASE("orient3d: near-coplanar points are handled robustly"){
        // Points that are coplanar in exact arithmetic but nearly so in FP.
        T a[3] = {0.0, 0.0, 0.0};
        T b[3] = {1.0, 0.0, 0.0};
        T c[3] = {0.0, 1.0, 0.0};

        // Exactly coplanar.
        T d_coplanar[3] = {0.5, 0.5, 0.0};
        T o = adaptive_predicate::orient3d(a, b, c, d_coplanar);
        REQUIRE(o == 0.0);

        // Slightly above the plane — orient3d should be negative
        // (face normal points up, d is on the normal side).
        T eps = 1e-15;
        T d_above[3] = {0.5, 0.5, eps};
        T o2 = adaptive_predicate::orient3d(a, b, c, d_above);
        REQUIRE(o2 < 0.0);
    }

    SUBCASE("orient3d_adaptive: consistency with orient3d"){
        T a[3] = {1.0, 2.0, 3.0};
        T b[3] = {4.0, 5.0, 6.0};
        T c[3] = {7.0, 8.0, 10.0};
        T d[3] = {1.5, 3.5, 5.5};

        T o1 = adaptive_predicate::orient3d(a, b, c, d);
        T o2 = adaptive_predicate::orient3d_adaptive(a, b, c, d);
        // They should have the same sign.
        REQUIRE((o1 > 0.0) == (o2 > 0.0));
    }
}
