
#include <limits>
#include <utility>
#include <iostream>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <vector>

#include <YgorMath.h>
#include <YgorMeshesOrient.h>

#include "doctest/doctest.h"


// Helper: verify that every face is a triangle with valid vertex indices.
template <class T, class I>
static void check_mesh_valid(const fv_surface_mesh<T, I> &mesh) {
    for(const auto &f : mesh.faces){
        REQUIRE(f.size() == 3UL);
        for(auto vi : f){
            REQUIRE(vi < mesh.vertices.size());
        }
    }
}

// Helper: verify that every shared (manifold) edge is traversed in opposite
// directions by its two adjacent faces — the hallmark of consistent orientation.
template <class T, class I>
static void check_consistent_orientation(const fv_surface_mesh<T, I> &mesh) {
    // Build directed-edge -> face map.
    std::map<std::pair<I,I>, I> directed;
    for(I f = 0; f < static_cast<I>(mesh.faces.size()); ++f){
        const auto &face = mesh.faces[f];
        for(size_t i = 0; i < face.size(); ++i){
            const auto a = face[i];
            const auto b = face[(i + 1UL) % face.size()];
            directed[{a, b}] = f;
        }
    }
    // For each undirected edge shared by exactly two faces, the two directed
    // half-edges should go in opposite directions ((a,b) and (b,a)).
    std::map<std::pair<I,I>, std::vector<std::pair<I,I>>> undirected;
    for(const auto &[edge, f_idx] : directed){
        auto key = std::make_pair(std::min(edge.first, edge.second),
                                  std::max(edge.first, edge.second));
        undirected[key].push_back(edge);
    }
    for(const auto &[key, edges] : undirected){
        if(edges.size() != 2UL) continue; // boundary or non-manifold
        // Consistent: (a,b) and (b,a) → directions differ.
        REQUIRE(edges[0].first != edges[1].first);
        REQUIRE(edges[0].second != edges[1].second);
    }
}


TEST_CASE( "YgorMeshesOrient" ){

    // --- Shared vertex data ------------------------------------------------
    const vec3<double> p0(0.0, 0.0, 0.0);
    const vec3<double> p1(1.0, 0.0, 0.0);
    const vec3<double> p2(0.0, 1.0, 0.0);
    const vec3<double> p3(0.0, 0.0, 1.0);

    SUBCASE("OrientFaces: empty mesh"){
        fv_surface_mesh<double, uint64_t> mesh;
        REQUIRE(OrientFaces(mesh) == true);
    }

    SUBCASE("OrientFaces: single triangle is unmodified"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = { p0, p1, p2 };
        mesh.faces = {{ 0, 1, 2 }};

        REQUIRE(OrientFaces(mesh) == true);
        REQUIRE(mesh.faces.size() == 1UL);
        check_mesh_valid(mesh);
    }

    SUBCASE("OrientFaces: closed tetrahedron with one inverted face"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = { p0, p1, p2, p3 };
        // Three faces oriented consistently, fourth flipped.
        mesh.faces = {
            { 0, 1, 2 },
            { 0, 3, 1 },
            { 1, 3, 2 },
            { 0, 2, 3 }  // intentionally reversed
        };

        REQUIRE(OrientFaces(mesh) == true);
        check_mesh_valid(mesh);
        check_consistent_orientation(mesh);
    }

    SUBCASE("OrientFaces: all faces already consistent"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = { p0, p1, p2, p3 };
        // Consistently-wound tetrahedron.
        mesh.faces = {
            { 0, 2, 1 },
            { 0, 1, 3 },
            { 1, 2, 3 },
            { 0, 3, 2 }
        };

        REQUIRE(OrientFaces(mesh) == true);
        check_mesh_valid(mesh);
        check_consistent_orientation(mesh);
    }

    SUBCASE("OrientFaces: two disconnected triangles"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(5.0, 0.0, 0.0),
            vec3<double>(6.0, 0.0, 0.0),
            vec3<double>(5.0, 1.0, 0.0)
        };
        mesh.faces = {
            { 0, 1, 2 },
            { 3, 4, 5 }
        };

        REQUIRE(OrientFaces(mesh) == true);
        check_mesh_valid(mesh);
    }

    SUBCASE("OrientFaces: closed cube (all faces consistent after orient)"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(1.0, 1.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(0.0, 0.0, 1.0),
            vec3<double>(1.0, 0.0, 1.0),
            vec3<double>(1.0, 1.0, 1.0),
            vec3<double>(0.0, 1.0, 1.0)
        };
        // 12 triangles forming a closed cube; some intentionally mis-wound.
        mesh.faces = {
            // bottom (-z)
            { 0, 2, 1 }, { 0, 3, 2 },
            // top (+z)
            { 4, 5, 6 }, { 4, 6, 7 },
            // front (-y)
            { 0, 1, 5 }, { 0, 5, 4 },
            // back (+y)
            { 2, 3, 7 }, { 2, 7, 6 },
            // left (-x)
            { 0, 4, 7 }, { 0, 7, 3 },
            // right (+x)  -- intentionally flipped
            { 1, 6, 5 }, { 1, 2, 6 }
        };

        REQUIRE(OrientFaces(mesh) == true);
        check_mesh_valid(mesh);
        check_consistent_orientation(mesh);
    }

    SUBCASE("OrientFaces: degenerate (zero-area) face is tolerated"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.5, 0.0, 0.0),  // collinear → degenerate
            vec3<double>(0.0, 1.0, 0.0)
        };
        mesh.faces = {
            { 0, 1, 3 },
            { 0, 1, 2 }  // degenerate triangle
        };

        // Should not crash; the degenerate face may be ignored in seed selection.
        REQUIRE(OrientFaces(mesh) == true);
    }

    SUBCASE("OrientFaces: open mesh (boundary edges) with inconsistent face"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.5, 1.0, 0.0),
            vec3<double>(1.5, 1.0, 0.0)
        };
        // Two triangles sharing edge (1,2). Second face intentionally reversed.
        mesh.faces = {
            { 0, 1, 2 },
            { 1, 3, 2 }   // reversed → should share edge (1,2) in same direction
        };

        REQUIRE(OrientFaces(mesh) == true);
        check_mesh_valid(mesh);
        check_consistent_orientation(mesh);
    }

    SUBCASE("OrientFaces: float specialization"){
        fv_surface_mesh<float, uint32_t> mesh;
        mesh.vertices = {
            vec3<float>(0.0f, 0.0f, 0.0f),
            vec3<float>(1.0f, 0.0f, 0.0f),
            vec3<float>(0.0f, 1.0f, 0.0f),
            vec3<float>(0.0f, 0.0f, 1.0f)
        };
        mesh.faces = {
            { 0, 1, 2 },
            { 0, 3, 1 },
            { 1, 3, 2 },
            { 0, 2, 3 }
        };

        REQUIRE(OrientFaces(mesh) == true);
        check_mesh_valid(mesh);
        check_consistent_orientation(mesh);
    }

    SUBCASE("OrientFaces: octahedron with several flipped faces"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            vec3<double>( 1.0,  0.0,  0.0),
            vec3<double>(-1.0,  0.0,  0.0),
            vec3<double>( 0.0,  1.0,  0.0),
            vec3<double>( 0.0, -1.0,  0.0),
            vec3<double>( 0.0,  0.0,  1.0),
            vec3<double>( 0.0,  0.0, -1.0)
        };
        // Consistent winding for the 8 faces of an octahedron, then flip a few.
        mesh.faces = {
            { 0, 2, 4 },
            { 2, 1, 4 },
            { 1, 3, 4 },
            { 3, 0, 4 },
            { 2, 0, 5 },
            { 1, 2, 5 },  // flipped
            { 3, 1, 5 },
            { 0, 3, 5 }   // flipped
        };

        REQUIRE(OrientFaces(mesh) == true);
        check_mesh_valid(mesh);
        check_consistent_orientation(mesh);
    }
}


TEST_CASE( "compute_vertex_normals" ){
    const double eps = std::sqrt(std::numeric_limits<double>::epsilon());

    SUBCASE("Empty mesh produces empty normals"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.compute_vertex_normals();
        REQUIRE(mesh.vertex_normals.empty());
    }

    SUBCASE("Single triangle: normals match face normal"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0)
        };
        mesh.faces = {{ 0, 1, 2 }};

        mesh.compute_vertex_normals();
        REQUIRE(mesh.vertex_normals.size() == 3UL);

        // Expected face normal is (0,0,1).
        const vec3<double> expected(0.0, 0.0, 1.0);
        for(const auto &n : mesh.vertex_normals){
            REQUIRE(std::abs(n.x - expected.x) < eps);
            REQUIRE(std::abs(n.y - expected.y) < eps);
            REQUIRE(std::abs(n.z - expected.z) < eps);
        }
    }

    SUBCASE("Vertex normals are unit length"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(0.0, 0.0, 1.0)
        };
        mesh.faces = {
            { 0, 2, 1 },
            { 0, 1, 3 },
            { 1, 2, 3 },
            { 0, 3, 2 }
        };

        mesh.compute_vertex_normals();
        REQUIRE(mesh.vertex_normals.size() == 4UL);

        for(const auto &n : mesh.vertex_normals){
            REQUIRE(std::abs(n.length() - 1.0) < eps);
        }
    }

    SUBCASE("Degenerate face does not corrupt normals"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.5, 0.0, 0.0),  // collinear
            vec3<double>(0.0, 1.0, 0.0)
        };
        mesh.faces = {
            { 0, 1, 3 },
            { 0, 1, 2 }   // degenerate
        };

        mesh.compute_vertex_normals();
        REQUIRE(mesh.vertex_normals.size() == 4UL);

        // Vertex 3 sees only the non-degenerate face; its normal should be (0,0,1).
        const auto &n3 = mesh.vertex_normals[3];
        REQUIRE(std::abs(n3.x) < eps);
        REQUIRE(std::abs(n3.y) < eps);
        REQUIRE(std::abs(n3.z - 1.0) < eps);
    }

    SUBCASE("Isolated vertex gets zero normal"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(99.0, 99.0, 99.0)  // isolated
        };
        mesh.faces = {{ 0, 1, 2 }};

        mesh.compute_vertex_normals();
        REQUIRE(mesh.vertex_normals.size() == 4UL);

        const auto &n3 = mesh.vertex_normals[3];
        REQUIRE(std::abs(n3.x) < eps);
        REQUIRE(std::abs(n3.y) < eps);
        REQUIRE(std::abs(n3.z) < eps);
    }

    SUBCASE("Float specialization works"){
        fv_surface_mesh<float, uint32_t> mesh;
        mesh.vertices = {
            vec3<float>(0.0f, 0.0f, 0.0f),
            vec3<float>(1.0f, 0.0f, 0.0f),
            vec3<float>(0.0f, 1.0f, 0.0f)
        };
        mesh.faces = {{ 0, 1, 2 }};

        mesh.compute_vertex_normals();
        REQUIRE(mesh.vertex_normals.size() == 3UL);
        const float f_eps = std::sqrt(std::numeric_limits<float>::epsilon());
        for(const auto &n : mesh.vertex_normals){
            REQUIRE(std::abs(n.length() - 1.0f) < f_eps);
        }
    }
}
