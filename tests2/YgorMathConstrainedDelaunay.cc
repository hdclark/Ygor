
#include <array>
#include <algorithm>
#include <limits>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "YgorMathConstrainedDelaunay.h"
#include "doctest/doctest.h"

using namespace ygor_test_constrained_delaunay;

TEST_CASE( "Constrained_Delaunay_Triangulation_2 function" ){
    SUBCASE("empty input returns empty mesh"){
        const std::vector<vec3<double>> verts;
        const std::vector<std::vector<uint32_t>> edges;
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);
        REQUIRE(mesh.vertices.empty());
        REQUIRE(mesh.faces.empty());
    }

    SUBCASE("two vertices with one constrained edge yields no faces"){
        const std::vector<vec3<double>> verts{{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 1} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == 2);
        REQUIRE(mesh.faces.empty());
    }

    SUBCASE("triangle boundary constraints produce only the triangle"){
        const std::vector<vec3<double>> verts{{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 1}, {1, 2}, {2, 0} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == 3);
        REQUIRE(mesh.faces.size() == 1);
        require_all_faces_are_triangles(mesh);
        require_constraints_are_triangle_edges(mesh,
            { make_edge<uint32_t>(0, 1), make_edge<uint32_t>(1, 2), make_edge<uint32_t>(0, 2) });
    }

    SUBCASE("constraint insertion keeps only triangles and preserves the constrained edge"){
        const std::vector<vec3<double>> verts{{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(2.0, 0.0, 0.0),
            vec3<double>(2.0, 2.0, 0.0),
            vec3<double>(0.0, 2.0, 0.0),
            vec3<double>(1.1, 0.8, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 2} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == verts.size());
        REQUIRE(mesh.faces.size() >= 3);
        require_all_faces_are_triangles(mesh);
        require_constraints_are_triangle_edges(mesh, { make_edge<uint32_t>(0, 2) });
        require_non_constraint_edges_are_locally_delaunay(mesh, { make_edge<uint32_t>(0, 2) });
    }

    SUBCASE("polygon boundary constraints produce only triangle faces"){
        const std::vector<vec3<double>> verts{{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(2.0, 0.0, 0.0),
            vec3<double>(3.0, 1.0, 0.0),
            vec3<double>(1.5, 2.5, 0.0),
            vec3<double>(0.0, 1.0, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == verts.size());
        REQUIRE(mesh.faces.size() == 3);
        require_all_faces_are_triangles(mesh);
        require_constraints_are_triangle_edges(mesh,
            { make_edge<uint32_t>(0, 1), make_edge<uint32_t>(1, 2), make_edge<uint32_t>(2, 3),
              make_edge<uint32_t>(3, 4), make_edge<uint32_t>(4, 0) });
        require_non_constraint_edges_are_locally_delaunay(mesh,
            { make_edge<uint32_t>(0, 1), make_edge<uint32_t>(1, 2), make_edge<uint32_t>(2, 3),
              make_edge<uint32_t>(3, 4), make_edge<uint32_t>(4, 0) });
    }

    SUBCASE("non-convex constrained polygon stays inside the constrained region"){
        const std::vector<vec3<double>> verts{{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(6.0, 1.0, 0.0),
            vec3<double>(-3.0, 4.0, 0.0),
            vec3<double>(-3.0, 1.0, 0.0),
            vec3<double>(-2.0, -1.0, 0.0),
            vec3<double>(-2.0, -5.0, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 0} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == verts.size());
        REQUIRE(mesh.faces.size() == verts.size() - 2);
        require_all_faces_are_triangles(mesh);
        require_constraints_are_triangle_edges(mesh,
            { make_edge<uint32_t>(0, 1), make_edge<uint32_t>(1, 2), make_edge<uint32_t>(2, 3),
              make_edge<uint32_t>(3, 4), make_edge<uint32_t>(4, 5), make_edge<uint32_t>(5, 0) });
        require_triangle_centroids_within_polygon(mesh, verts);
    }

    SUBCASE("closed constrained regions are still filtered when extra constraints touch the boundary"){
        const std::vector<vec3<double>> verts{{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(2.0, 0.0, 0.0),
            vec3<double>(2.0, 2.0, 0.0),
            vec3<double>(0.0, 2.0, 0.0),
            vec3<double>(1.0, 1.0, 0.0),
            vec3<double>(3.0, 1.0, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}
        }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == verts.size());
        REQUIRE(mesh.faces.size() == 4);
        require_all_faces_are_triangles(mesh);
        require_constraints_are_triangle_edges(mesh,
            { make_edge<uint32_t>(0, 1), make_edge<uint32_t>(1, 2), make_edge<uint32_t>(2, 3),
              make_edge<uint32_t>(3, 0), make_edge<uint32_t>(0, 4) });
        require_triangle_centroids_within_polygon(mesh,
            { verts.at(0), verts.at(1), verts.at(2), verts.at(3) });
        for(const auto &face : mesh.faces){
            REQUIRE(std::find(face.begin(), face.end(), uint32_t{5}) == face.end());
        }
    }

    SUBCASE("crossing constraints are rejected"){
        const std::vector<vec3<double>> verts{{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(1.0, 1.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 2}, {1, 3} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);
        REQUIRE(mesh.vertices.empty());
        REQUIRE(mesh.faces.empty());
    }

    SUBCASE("constraint passing through another vertex is rejected"){
        const std::vector<vec3<double>> verts{{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(2.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 2} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);
        REQUIRE(mesh.vertices.empty());
        REQUIRE(mesh.faces.empty());
    }

    SUBCASE("malformed constraints are rejected"){
        const std::vector<vec3<double>> verts{{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 1, 2} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);
        REQUIRE(mesh.vertices.empty());
        REQUIRE(mesh.faces.empty());
    }

    SUBCASE("out-of-range constraints are rejected"){
        const std::vector<vec3<double>> verts{{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 3} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);
        REQUIRE(mesh.vertices.empty());
        REQUIRE(mesh.faces.empty());
    }

    SUBCASE("self-edge constraints are rejected"){
        const std::vector<vec3<double>> verts{{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {1, 1} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);
        REQUIRE(mesh.vertices.empty());
        REQUIRE(mesh.faces.empty());
    }

    SUBCASE("duplicate constraints are rejected even when reversed"){
        const std::vector<vec3<double>> verts{{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 1}, {1, 0} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);
        REQUIRE(mesh.vertices.empty());
        REQUIRE(mesh.faces.empty());
    }

    SUBCASE("float coordinates and uint64_t indices are supported"){
        const std::vector<vec3<float>> verts{{
            vec3<float>(0.0f, 0.0f, 0.0f),
            vec3<float>(1.0f, 0.0f, 0.0f),
            vec3<float>(0.0f, 1.0f, 0.0f),
            vec3<float>(1.0f, 1.0f, 0.0f)
        }};
        const std::vector<std::vector<uint64_t>> edges{{ {0, 2} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<float, uint64_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == 4);
        REQUIRE(mesh.faces.size() == 2);
        require_all_faces_are_triangles(mesh);
        require_constraints_are_triangle_edges(mesh, { make_edge<uint64_t>(0, 2) });
    }
}

TEST_CASE("adaptive orient3d keeps near-degenerate signs under the normal build flags"){
    const std::array<double, 3> a{{ 0.27046243662747216, -0.82109361271069092,  0.11235779824475989 }};
    const std::array<double, 3> b{{ 0.57930393901296728, -0.55673265201320743, -0.16266294128208603 }};
    const std::array<double, 3> c{{-0.50044415316658108, -0.41627067894555503,  0.60647264433458070 }};
    const std::array<double, 3> d{{ 0.11644074082461947, -0.59803231455648442,  0.18538916709908485 }};

    REQUIRE(adaptive_predicate::orient3d(a.data(), b.data(), c.data(), d.data()) < 0.0);
}
