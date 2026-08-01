
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include <YgorMath.h>
#include <YgorMathVoronoi.h>

#include "doctest/doctest.h"

namespace {

template <class T, class I>
void require_edge_bisects_sites(const std::vector<vec2<T>> &sites,
                                const VoronoiDiagram2<T, I> &diagram,
                                const typename VoronoiDiagram2<T, I>::Edge &edge,
                                T tol){
    const auto &a = sites.at(static_cast<size_t>(edge.left_site));
    const auto &b = sites.at(static_cast<size_t>(edge.right_site));

    const auto da = edge.sample_point.distance(a);
    const auto db = edge.sample_point.distance(b);
    REQUIRE(da == doctest::Approx(db).epsilon(static_cast<double>(tol * 16)));

    const auto baseline = b - a;
    REQUIRE(std::abs(edge.direction.Dot(baseline)) <= tol * 64);

    if(edge.vertex0.has_value()){
        const auto &v = diagram.vertices.at(static_cast<size_t>(edge.vertex0.value())).position;
        REQUIRE(v.distance(a) == doctest::Approx(v.distance(b)).epsilon(static_cast<double>(tol * 16)));
    }
    if(edge.vertex1.has_value()){
        const auto &v = diagram.vertices.at(static_cast<size_t>(edge.vertex1.value())).position;
        REQUIRE(v.distance(a) == doctest::Approx(v.distance(b)).epsilon(static_cast<double>(tol * 16)));
    }
}

template <class T>
std::vector<std::vector<vec2<T>>> singleton_site_polygons(const std::vector<vec2<T>> &sites){
    std::vector<std::vector<vec2<T>>> out;
    out.reserve(sites.size());
    for(const auto &site : sites){
        out.push_back({ site });
    }
    return out;
}

} // namespace

TEST_CASE( "Voronoi_Diagram_2 function" ){
    const auto eps = static_cast<double>(std::sqrt(std::numeric_limits<double>::epsilon()));

    SUBCASE("fewer than 2 sites is rejected"){
        std::vector<vec2<double>> sites0;
        REQUIRE_THROWS( Voronoi_Diagram_2<double, uint32_t>(sites0) );

        std::vector<vec2<double>> sites1 {{ vec2<double>(0.0, 0.0) }};
        REQUIRE_THROWS( Voronoi_Diagram_2<double, uint32_t>(sites1) );
    }

    SUBCASE("non-finite and duplicate sites are rejected"){
        std::vector<vec2<double>> nonfinite {{
            vec2<double>(0.0, 0.0),
            vec2<double>(std::numeric_limits<double>::infinity(), 0.0) }};
        REQUIRE_THROWS( Voronoi_Diagram_2<double, uint32_t>(nonfinite) );

        std::vector<vec2<double>> duplicate {{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(0.0, 0.0) }};
        REQUIRE_THROWS( Voronoi_Diagram_2<double, uint32_t>(duplicate) );
    }

    SUBCASE("two sites produce one bisector edge and no Voronoi vertices"){
        std::vector<vec2<double>> sites {{
            vec2<double>(0.0, 0.0),
            vec2<double>(2.0, 0.0) }};
        const auto diagram = Voronoi_Diagram_2<double, uint32_t>(sites);

        REQUIRE( diagram.vertices.empty() );
        REQUIRE( diagram.edges.size() == 1 );
        REQUIRE( diagram.cell_edges.size() == 2 );
        REQUIRE( diagram.cell_edges[0].size() == 1 );
        REQUIRE( diagram.cell_edges[1].size() == 1 );

        const auto &edge = diagram.edges.front();
        CHECK_FALSE( edge.vertex0.has_value() );
        CHECK_FALSE( edge.vertex1.has_value() );
        require_edge_bisects_sites(sites, diagram, edge, static_cast<double>(eps));
        REQUIRE( edge.sample_point.x == doctest::Approx(1.0) );
    }

    SUBCASE("three non-collinear sites produce one Voronoi vertex and three rays"){
        std::vector<vec2<double>> sites {{
            vec2<double>(0.0, 0.0),
            vec2<double>(2.0, 0.0),
            vec2<double>(1.0, 2.0) }};
        const auto diagram = Voronoi_Diagram_2<double, uint32_t>(sites);

        REQUIRE( diagram.vertices.size() == 1 );
        REQUIRE( diagram.edges.size() == 3 );
        REQUIRE( diagram.cell_edges.size() == 3 );
        for(const auto &cell : diagram.cell_edges){
            REQUIRE( cell.size() == 2 );
        }

        const auto &vertex = diagram.vertices.front();
        REQUIRE( vertex.incident_sites.size() == 3 );
        const auto d0 = vertex.position.distance(sites[0]);
        const auto d1 = vertex.position.distance(sites[1]);
        const auto d2 = vertex.position.distance(sites[2]);
        REQUIRE( d0 == doctest::Approx(d1).epsilon(eps * 16) );
        REQUIRE( d0 == doctest::Approx(d2).epsilon(eps * 16) );

        for(const auto &edge : diagram.edges){
            const bool has_vertex = edge.vertex0.has_value() || edge.vertex1.has_value();
            REQUIRE( has_vertex );
            require_edge_bisects_sites(sites, diagram, edge, static_cast<double>(eps));
        }
    }

    SUBCASE("two highest sites on a horizontal row still produce a valid three-ray vertex"){
        std::vector<vec2<double>> sites {{
            vec2<double>(0.0, 2.0),
            vec2<double>(2.0, 2.0),
            vec2<double>(1.0, 0.0) }};
        const auto diagram = Voronoi_Diagram_2<double, uint32_t>(sites);

        REQUIRE( diagram.vertices.size() == 1 );
        REQUIRE( diagram.edges.size() == 3 );
        REQUIRE( diagram.cell_edges.size() == 3 );
        for(const auto &cell : diagram.cell_edges){
            REQUIRE( cell.size() == 2 );
        }

        const auto &vertex = diagram.vertices.front();
        const auto d0 = vertex.position.distance(sites[0]);
        const auto d1 = vertex.position.distance(sites[1]);
        const auto d2 = vertex.position.distance(sites[2]);
        REQUIRE( d0 == doctest::Approx(d1).epsilon(eps * 16) );
        REQUIRE( d0 == doctest::Approx(d2).epsilon(eps * 16) );

        for(const auto &edge : diagram.edges){
            const bool has_vertex = edge.vertex0.has_value() || edge.vertex1.has_value();
            REQUIRE( has_vertex );
            require_edge_bisects_sites(sites, diagram, edge, static_cast<double>(eps));
        }
    }

    SUBCASE("collinear sites produce only unbounded bisectors"){
        std::vector<vec2<double>> sites {{
            vec2<double>(-2.0, 0.0),
            vec2<double>(0.0, 0.0),
            vec2<double>(3.0, 0.0) }};
        const auto diagram = Voronoi_Diagram_2<double, uint32_t>(sites);

        REQUIRE( diagram.vertices.empty() );
        REQUIRE( diagram.edges.size() == 2 );
        REQUIRE( diagram.cell_edges.size() == 3 );
        REQUIRE( diagram.cell_edges[0].size() == 1 );
        REQUIRE( diagram.cell_edges[1].size() == 2 );
        REQUIRE( diagram.cell_edges[2].size() == 1 );

        for(const auto &edge : diagram.edges){
            CHECK_FALSE( edge.vertex0.has_value() );
            CHECK_FALSE( edge.vertex1.has_value() );
            require_edge_bisects_sites(sites, diagram, edge, static_cast<double>(eps));
        }
    }

    SUBCASE("translated and tiny input remains numerically stable"){
        std::vector<vec2<double>> sites {{
            vec2<double>(1.0e9 + 1.0e-6, -2.0e9 + 2.0e-6),
            vec2<double>(1.0e9 + 5.0e-6, -2.0e9 + 3.0e-6),
            vec2<double>(1.0e9 + 3.0e-6, -2.0e9 + 7.0e-6) }};
        const auto diagram = Voronoi_Diagram_2<double, uint64_t>(sites);

        REQUIRE( diagram.vertices.size() == 1 );
        REQUIRE( diagram.edges.size() == 3 );
        for(const auto &edge : diagram.edges){
            require_edge_bisects_sites(sites, diagram, edge, 1.0e-4);
        }
    }

    SUBCASE("four sites can produce a bounded Voronoi edge"){
        std::vector<vec2<double>> sites {{
            vec2<double>(-2.0, -2.0),
            vec2<double>(-2.0, -1.0),
            vec2<double>(-2.0, 0.0),
            vec2<double>(-1.0, -2.0) }};
        const auto diagram = Voronoi_Diagram_2<double, uint32_t>(sites);

        REQUIRE( diagram.vertices.size() >= 2 );
        REQUIRE( diagram.edges.size() >= 5 );

        bool found_bounded_edge = false;
        for(const auto &edge : diagram.edges){
            require_edge_bisects_sites(sites, diagram, edge, static_cast<double>(eps));
            if(edge.vertex0.has_value() && edge.vertex1.has_value()){
                found_bounded_edge = true;
            }
        }
        REQUIRE( found_bounded_edge );
    }
}

TEST_CASE( "Triangulate_Voronoi function" ){
    const auto eps = static_cast<double>(std::sqrt(std::numeric_limits<double>::epsilon()));

    SUBCASE("three Voronoi sites produce one dual triangle on z equals zero"){
        const std::vector<vec2<double>> sites {{
            vec2<double>(0.0, 0.0),
            vec2<double>(2.0, 0.0),
            vec2<double>(1.0, 2.0) }};
        const auto diagram = Voronoi_Diagram_2<double, uint32_t>(sites);
        const auto mesh = Triangulate_Voronoi<double, uint32_t>(singleton_site_polygons(sites), diagram);

        REQUIRE( mesh.vertices.size() == sites.size() );
        REQUIRE( mesh.faces.size() == 1 );
        REQUIRE( mesh.faces.front().size() == 3 );
        for(size_t i = 0; i < sites.size(); ++i){
            REQUIRE( mesh.vertices.at(i).x == doctest::Approx(sites.at(i).x) );
            REQUIRE( mesh.vertices.at(i).y == doctest::Approx(sites.at(i).y) );
            REQUIRE( mesh.vertices.at(i).z == doctest::Approx(0.0) );
        }

        const auto &face = mesh.faces.front();
        const auto sign = orient_sign(vec2<double>(mesh.vertices.at(face.at(0)).x, mesh.vertices.at(face.at(0)).y),
                                      vec2<double>(mesh.vertices.at(face.at(1)).x, mesh.vertices.at(face.at(1)).y),
                                      vec2<double>(mesh.vertices.at(face.at(2)).x, mesh.vertices.at(face.at(2)).y));
        REQUIRE( sign > 0 );
    }

    SUBCASE("multiple Voronoi vertices produce multiple dual triangles"){
        const std::vector<vec2<double>> sites {{
            vec2<double>(-2.0, -2.0),
            vec2<double>(-2.0, -1.0),
            vec2<double>(-2.0, 0.0),
            vec2<double>(-1.0, -2.0) }};
        const auto diagram = Voronoi_Diagram_2<double, uint32_t>(sites);
        const auto mesh = Triangulate_Voronoi<double, uint32_t>(singleton_site_polygons(sites), diagram);

        REQUIRE( mesh.vertices.size() == sites.size() );
        REQUIRE( mesh.faces.size() == diagram.vertices.size() );
        for(const auto &face : mesh.faces){
            REQUIRE( face.size() == 3 );
            for(const auto idx : face){
                REQUIRE( static_cast<size_t>(idx) < mesh.vertices.size() );
                REQUIRE( mesh.vertices.at(static_cast<size_t>(idx)).z == doctest::Approx(0.0).epsilon(eps) );
            }
            const auto sign = orient_sign(vec2<double>(mesh.vertices.at(face.at(0)).x, mesh.vertices.at(face.at(0)).y),
                                          vec2<double>(mesh.vertices.at(face.at(1)).x, mesh.vertices.at(face.at(1)).y),
                                          vec2<double>(mesh.vertices.at(face.at(2)).x, mesh.vertices.at(face.at(2)).y));
            REQUIRE( sign > 0 );
        }
    }

    SUBCASE("site polygon count must match the Voronoi site count"){
        const std::vector<vec2<double>> sites {{
            vec2<double>(0.0, 0.0),
            vec2<double>(2.0, 0.0),
            vec2<double>(1.0, 2.0) }};
        const auto diagram = Voronoi_Diagram_2<double, uint32_t>(sites);

        std::vector<std::vector<vec2<double>>> bad_polys{
            { vec2<double>(0.0, 0.0) },
            { vec2<double>(2.0, 0.0) }
        };
        REQUIRE_THROWS( Triangulate_Voronoi<double, uint32_t>(bad_polys, diagram) );
    }
}
