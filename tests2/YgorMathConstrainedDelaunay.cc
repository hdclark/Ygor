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

    SUBCASE("two vertices with one constrained edge yields edge-only mesh"){
        const std::vector<vec3<double>> verts{{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 1} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == 2);
        REQUIRE(mesh.faces.size() == 1);
        REQUIRE(mesh.faces.at(0).size() == 2);
        REQUIRE(make_edge(mesh.faces.at(0).at(0), mesh.faces.at(0).at(1)) == make_edge<uint32_t>(0, 1));
    }

    SUBCASE("triangle boundary constraints are emitted before the triangle"){
        const std::vector<vec3<double>> verts{{
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0)
        }};
        const std::vector<std::vector<uint32_t>> edges{{ {0, 1}, {1, 2}, {2, 0} }};
        const auto mesh = Constrained_Delaunay_Triangulation_2<double, uint32_t>(verts, edges);

        REQUIRE(mesh.vertices.size() == 3);
        REQUIRE(mesh.faces.size() == 4);
        for(size_t i = 0; i < edges.size(); ++i){
            REQUIRE(mesh.faces.at(i).size() == 2);
            REQUIRE(make_edge(mesh.faces.at(i).at(0), mesh.faces.at(i).at(1))
                    == make_edge(edges.at(i).at(0), edges.at(i).at(1)));
        }
        REQUIRE(mesh.faces.back().size() == 3);
        require_triangle_edges_are_listed(mesh);
    }

    SUBCASE("constraint insertion forces a non-Delaunay diagonal to appear"){
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
        REQUIRE(mesh.faces.at(0).size() == 2);
        REQUIRE(make_edge(mesh.faces.at(0).at(0), mesh.faces.at(0).at(1)) == make_edge<uint32_t>(0, 2));
        REQUIRE(std::count_if(mesh.faces.begin(), mesh.faces.end(),
                              [](const std::vector<uint32_t> &face){ return face.size() == 3; }) >= 3);
        require_triangle_edges_are_listed(mesh);
        require_non_constraint_edges_are_locally_delaunay(mesh, { make_edge<uint32_t>(0, 2) });
    }

    SUBCASE("polygon constraints preserve user edge ordering and generated edges follow them"){
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
        REQUIRE(mesh.faces.size() >= edges.size() + 3);
        for(size_t i = 0; i < edges.size(); ++i){
            REQUIRE(mesh.faces.at(i).size() == 2);
            REQUIRE(make_edge(mesh.faces.at(i).at(0), mesh.faces.at(i).at(1))
                    == make_edge(edges.at(i).at(0), edges.at(i).at(1)));
        }
        for(size_t i = edges.size(); i < mesh.faces.size(); ++i){
            if(mesh.faces.at(i).size() == 2){
                REQUIRE(std::none_of(edges.begin(), edges.end(),
                                     [&](const std::vector<uint32_t> &edge){
                                         return make_edge(edge.at(0), edge.at(1))
                                             == make_edge(mesh.faces.at(i).at(0), mesh.faces.at(i).at(1));
                                     }));
            }
        }
        require_triangle_edges_are_listed(mesh);
        require_non_constraint_edges_are_locally_delaunay(mesh,
            { make_edge<uint32_t>(0, 1), make_edge<uint32_t>(1, 2), make_edge<uint32_t>(2, 3),
              make_edge<uint32_t>(3, 4), make_edge<uint32_t>(4, 0) });
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
        REQUIRE(mesh.faces.size() >= 3);
        REQUIRE(mesh.faces.at(0).size() == 2);
        REQUIRE(make_edge(mesh.faces.at(0).at(0), mesh.faces.at(0).at(1)) == make_edge<uint64_t>(0, 2));
        require_triangle_edges_are_listed(mesh);
    }
}
