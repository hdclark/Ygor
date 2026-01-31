
#include <limits>
#include <utility>
#include <iostream>
#include <cmath>
#include <cstdint>
#include <vector>
#include <list>

#include <YgorMath.h>

#include "doctest/doctest.h"


TEST_CASE( "Triangulate_Contours function" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("empty contour collection returns empty mesh"){
        contour_collection<double> cc;

        auto mesh = Triangulate_Contours<double, uint32_t>(cc);

        REQUIRE( mesh.vertices.size() == 0 );
        REQUIRE( mesh.faces.size() == 0 );
    }

    SUBCASE("single contour with fewer than 3 points returns empty mesh"){
        contour_collection<double> cc;
        contour_of_points<double> cop;
        cop.closed = true;
        cop.points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        cc.contours.push_back(cop);

        auto mesh = Triangulate_Contours<double, uint32_t>(cc);

        // Not enough points to form a triangle.
        REQUIRE( mesh.faces.size() == 0 );
    }

    SUBCASE("triangular contour produces single triangle"){
        contour_collection<double> cc;
        contour_of_points<double> cop;
        cop.closed = true;
        cop.points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(0.5, 1.0, 0.0));
        cc.contours.push_back(cop);

        auto mesh = Triangulate_Contours<double, uint32_t>(cc);

        REQUIRE( mesh.vertices.size() == 3 );
        REQUIRE( mesh.faces.size() == 1 );
        REQUIRE( mesh.faces[0].size() == 3 );
    }

    SUBCASE("square contour produces two triangles"){
        contour_collection<double> cc;
        contour_of_points<double> cop;
        cop.closed = true;
        cop.points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(1.0, 1.0, 0.0));
        cop.points.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        cc.contours.push_back(cop);

        auto mesh = Triangulate_Contours<double, uint32_t>(cc);

        REQUIRE( mesh.vertices.size() == 4 );
        REQUIRE( mesh.faces.size() == 2 );
        for(const auto &face : mesh.faces){
            REQUIRE( face.size() == 3 );
        }
    }

    SUBCASE("pentagon contour produces three triangles"){
        contour_collection<double> cc;
        contour_of_points<double> cop;
        cop.closed = true;
        // Regular pentagon.
        const double pi = std::acos(-1.0);
        for(int i = 0; i < 5; ++i){
            const double angle = 2.0 * pi * static_cast<double>(i) / 5.0 - pi / 2.0;
            cop.points.emplace_back(vec3<double>(std::cos(angle), std::sin(angle), 0.0));
        }
        cc.contours.push_back(cop);

        auto mesh = Triangulate_Contours<double, uint32_t>(cc);

        REQUIRE( mesh.vertices.size() == 5 );
        REQUIRE( mesh.faces.size() == 3 ); // Pentagon = 5 - 2 = 3 triangles.
        for(const auto &face : mesh.faces){
            REQUIRE( face.size() == 3 );
        }
    }

    SUBCASE("concave L-shaped polygon is correctly triangulated"){
        contour_collection<double> cc;
        contour_of_points<double> cop;
        cop.closed = true;
        // L-shape (concave hexagon).
        cop.points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(2.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(2.0, 1.0, 0.0));
        cop.points.emplace_back(vec3<double>(1.0, 1.0, 0.0));
        cop.points.emplace_back(vec3<double>(1.0, 2.0, 0.0));
        cop.points.emplace_back(vec3<double>(0.0, 2.0, 0.0));
        cc.contours.push_back(cop);

        auto mesh = Triangulate_Contours<double, uint32_t>(cc);

        REQUIRE( mesh.vertices.size() == 6 );
        REQUIRE( mesh.faces.size() == 4 ); // Hexagon = 6 - 2 = 4 triangles.
        for(const auto &face : mesh.faces){
            REQUIRE( face.size() == 3 );
        }

        // Verify all triangles have positive area (valid triangulation).
        for(const auto &face : mesh.faces){
            const auto &v0 = mesh.vertices[face[0]];
            const auto &v1 = mesh.vertices[face[1]];
            const auto &v2 = mesh.vertices[face[2]];
            const auto cross = (v1 - v0).Cross(v2 - v0);
            REQUIRE( cross.length() > eps );
        }
    }

    SUBCASE("contour on tilted plane is correctly projected and triangulated"){
        contour_collection<double> cc;
        contour_of_points<double> cop;
        cop.closed = true;

        // Square on a plane tilted 45 degrees.
        cop.points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(1.0, 0.0, 1.0));
        cop.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        cop.points.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        cc.contours.push_back(cop);

        auto mesh = Triangulate_Contours<double, uint32_t>(cc);

        REQUIRE( mesh.vertices.size() == 4 );
        REQUIRE( mesh.faces.size() == 2 );

        // Check that vertices are coplanar (projected onto best-fit plane).
        if(mesh.vertices.size() >= 3){
            const auto v01 = mesh.vertices[1] - mesh.vertices[0];
            const auto v02 = mesh.vertices[2] - mesh.vertices[0];
            const auto normal = v01.Cross(v02).unit();

            for(size_t i = 3; i < mesh.vertices.size(); ++i){
                const auto v0i = mesh.vertices[i] - mesh.vertices[0];
                const double dist = std::abs(v0i.Dot(normal));
                REQUIRE( dist < eps );
            }
        }
    }

    SUBCASE("triangulation surface area equals original contour area"){
        contour_collection<double> cc;
        contour_of_points<double> cop;
        cop.closed = true;
        // Unit square.
        cop.points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(1.0, 1.0, 0.0));
        cop.points.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        cc.contours.push_back(cop);

        auto mesh = Triangulate_Contours<double, uint32_t>(cc);

        REQUIRE( mesh.faces.size() == 2 );

        // Compute total surface area of triangles.
        double total_area = 0.0;
        for(const auto &face : mesh.faces){
            const auto &v0 = mesh.vertices[face[0]];
            const auto &v1 = mesh.vertices[face[1]];
            const auto &v2 = mesh.vertices[face[2]];
            const auto cross = (v1 - v0).Cross(v2 - v0);
            total_area += 0.5 * cross.length();
        }

        // Unit square area = 1.0.
        REQUIRE( std::abs(total_area - 1.0) < eps );
    }

    SUBCASE("multiple contours are triangulated together"){
        contour_collection<double> cc;

        // Outer square.
        contour_of_points<double> outer;
        outer.closed = true;
        outer.points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        outer.points.emplace_back(vec3<double>(4.0, 0.0, 0.0));
        outer.points.emplace_back(vec3<double>(4.0, 4.0, 0.0));
        outer.points.emplace_back(vec3<double>(0.0, 4.0, 0.0));
        cc.contours.push_back(outer);

        // Inner square (hole) - opposite orientation.
        contour_of_points<double> inner;
        inner.closed = true;
        inner.points.emplace_back(vec3<double>(1.0, 1.0, 0.0));
        inner.points.emplace_back(vec3<double>(1.0, 3.0, 0.0));
        inner.points.emplace_back(vec3<double>(3.0, 3.0, 0.0));
        inner.points.emplace_back(vec3<double>(3.0, 1.0, 0.0));
        cc.contours.push_back(inner);

        auto mesh = Triangulate_Contours<double, uint32_t>(cc);

        REQUIRE( mesh.vertices.size() == 8 );
        // With bridging, we should get a connected polygon that triangulates correctly.
        REQUIRE( mesh.faces.size() >= 6 ); // At minimum, some triangles should be created.
    }

    SUBCASE("clockwise contour is handled correctly"){
        contour_collection<double> cc;
        contour_of_points<double> cop;
        cop.closed = true;
        // Clockwise square.
        cop.points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        cop.points.emplace_back(vec3<double>(1.0, 1.0, 0.0));
        cop.points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        cc.contours.push_back(cop);

        auto mesh = Triangulate_Contours<double, uint32_t>(cc);

        REQUIRE( mesh.vertices.size() == 4 );
        REQUIRE( mesh.faces.size() == 2 );

        // Verify surface area is correct.
        double total_area = 0.0;
        for(const auto &face : mesh.faces){
            const auto &v0 = mesh.vertices[face[0]];
            const auto &v1 = mesh.vertices[face[1]];
            const auto &v2 = mesh.vertices[face[2]];
            const auto cross = (v1 - v0).Cross(v2 - v0);
            total_area += 0.5 * cross.length();
        }
        REQUIRE( std::abs(total_area - 1.0) < eps );
    }

    SUBCASE("contour with collinear points is handled"){
        contour_collection<double> cc;
        contour_of_points<double> cop;
        cop.closed = true;
        // Square with an extra collinear point on one edge.
        cop.points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(0.5, 0.0, 0.0)); // Collinear point.
        cop.points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(1.0, 1.0, 0.0));
        cop.points.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        cc.contours.push_back(cop);

        auto mesh = Triangulate_Contours<double, uint32_t>(cc);

        REQUIRE( mesh.vertices.size() == 5 );
        REQUIRE( mesh.faces.size() >= 2 ); // Should handle collinear points gracefully.
    }

    SUBCASE("star-shaped concave polygon is correctly triangulated"){
        contour_collection<double> cc;
        contour_of_points<double> cop;
        cop.closed = true;

        // 5-pointed star.
        const double pi = std::acos(-1.0);
        const double outer_r = 2.0;
        const double inner_r = 0.8;
        for(int i = 0; i < 5; ++i){
            // Outer point.
            const double outer_angle = 2.0 * pi * static_cast<double>(i) / 5.0 - pi / 2.0;
            cop.points.emplace_back(vec3<double>(outer_r * std::cos(outer_angle), 
                                                  outer_r * std::sin(outer_angle), 0.0));
            // Inner point.
            const double inner_angle = outer_angle + pi / 5.0;
            cop.points.emplace_back(vec3<double>(inner_r * std::cos(inner_angle), 
                                                  inner_r * std::sin(inner_angle), 0.0));
        }
        cc.contours.push_back(cop);

        auto mesh = Triangulate_Contours<double, uint32_t>(cc);

        REQUIRE( mesh.vertices.size() == 10 );
        REQUIRE( mesh.faces.size() == 8 ); // 10-gon = 10 - 2 = 8 triangles.
        for(const auto &face : mesh.faces){
            REQUIRE( face.size() == 3 );
        }
    }

    SUBCASE("hexagon with float type works correctly"){
        contour_collection<float> cc;
        contour_of_points<float> cop;
        cop.closed = true;

        // Regular hexagon.
        const float pi = std::acos(-1.0f);
        for(int i = 0; i < 6; ++i){
            const float angle = 2.0f * pi * static_cast<float>(i) / 6.0f;
            cop.points.emplace_back(vec3<float>(std::cos(angle), std::sin(angle), 0.0f));
        }
        cc.contours.push_back(cop);

        auto mesh = Triangulate_Contours<float, uint32_t>(cc);

        REQUIRE( mesh.vertices.size() == 6 );
        REQUIRE( mesh.faces.size() == 4 ); // Hexagon = 6 - 2 = 4 triangles.
    }

    SUBCASE("uint64_t index type works correctly"){
        contour_collection<double> cc;
        contour_of_points<double> cop;
        cop.closed = true;
        cop.points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        cop.points.emplace_back(vec3<double>(0.5, 1.0, 0.0));
        cc.contours.push_back(cop);

        auto mesh = Triangulate_Contours<double, uint64_t>(cc);

        REQUIRE( mesh.vertices.size() == 3 );
        REQUIRE( mesh.faces.size() == 1 );
        REQUIRE( mesh.faces[0].size() == 3 );
    }
}

