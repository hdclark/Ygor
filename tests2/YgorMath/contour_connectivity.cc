
#include <limits>
#include <list>
#include <utility>
#include <iostream>
#include <cstdint>
#include <cmath>

#include <YgorMath.h>
#include <YgorMathContourConnectivity.h>

#include "doctest/doctest.h"


TEST_CASE( "Triangulate_Planar_Contour_Connectivity basic tests" ){

    SUBCASE("simple square contours on parallel planes"){
        // Create a simple square contour on the top plane (z = 1)
        contour_collection<double> top_cc;
        top_cc.contours.emplace_back();
        top_cc.contours.back().closed = true;
        top_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(1.0, 0.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(0.0, 1.0, 1.0));

        // Create a matching square contour on the bottom plane (z = 0)
        contour_collection<double> bottom_cc;
        bottom_cc.contours.emplace_back();
        bottom_cc.contours.back().closed = true;
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 1.0, 0.0));

        // Triangulate the gap
        auto mesh = Triangulate_Planar_Contour_Connectivity(top_cc, bottom_cc);

        // Check that we have vertices (4 from top + 4 from bottom = 8)
        REQUIRE(mesh.vertices.size() == 8);

        // Check that we have triangles (should have triangles connecting the two squares)
        REQUIRE(mesh.faces.size() > 0);

        // All faces should be triangles
        for (const auto &face : mesh.faces) {
            REQUIRE(face.size() == 3);
        }

        // Surface area should be positive
        REQUIRE(mesh.surface_area() > 0.0);
    }

    SUBCASE("triangular contours on parallel planes"){
        // Create a simple triangle on the top plane
        contour_collection<double> top_cc;
        top_cc.contours.emplace_back();
        top_cc.contours.back().closed = true;
        top_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 2.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(2.0, 0.0, 2.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.732, 2.0));

        // Create a matching triangle on the bottom plane
        contour_collection<double> bottom_cc;
        bottom_cc.contours.emplace_back();
        bottom_cc.contours.back().closed = true;
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(2.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.732, 0.0));

        auto mesh = Triangulate_Planar_Contour_Connectivity(top_cc, bottom_cc);

        // Check vertices (3 + 3 = 6)
        REQUIRE(mesh.vertices.size() == 6);

        // Check that we have triangles
        REQUIRE(mesh.faces.size() > 0);

        // Surface area should be positive
        REQUIRE(mesh.surface_area() > 0.0);
    }

    SUBCASE("empty top collection throws"){
        contour_collection<double> top_cc;
        
        contour_collection<double> bottom_cc;
        bottom_cc.contours.emplace_back();
        bottom_cc.contours.back().closed = true;
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 0.0));

        REQUIRE_THROWS_AS(
            Triangulate_Planar_Contour_Connectivity(top_cc, bottom_cc),
            std::invalid_argument
        );
    }

    SUBCASE("empty bottom collection throws"){
        contour_collection<double> top_cc;
        top_cc.contours.emplace_back();
        top_cc.contours.back().closed = true;
        top_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        
        contour_collection<double> bottom_cc;

        REQUIRE_THROWS_AS(
            Triangulate_Planar_Contour_Connectivity(top_cc, bottom_cc),
            std::invalid_argument
        );
    }

    SUBCASE("coincident planes throws"){
        contour_collection<double> top_cc;
        top_cc.contours.emplace_back();
        top_cc.contours.back().closed = true;
        top_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.0, 0.0));

        contour_collection<double> bottom_cc;
        bottom_cc.contours.emplace_back();
        bottom_cc.contours.back().closed = true;
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.0, 0.0));

        REQUIRE_THROWS_AS(
            Triangulate_Planar_Contour_Connectivity(top_cc, bottom_cc),
            std::runtime_error
        );
    }

    SUBCASE("collections with empty contour_of_points entries throw"){
        // Create collections that have contours but no actual points
        contour_collection<double> top_cc;
        top_cc.contours.emplace_back();  // Empty contour_of_points
        top_cc.contours.back().closed = true;
        // No points added
        
        contour_collection<double> bottom_cc;
        bottom_cc.contours.emplace_back();
        bottom_cc.contours.back().closed = true;
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 0.0));

        REQUIRE_THROWS_AS(
            Triangulate_Planar_Contour_Connectivity(top_cc, bottom_cc),
            std::invalid_argument
        );
    }

    SUBCASE("multiple contours with some empty entries throw"){
        // Create a collection with one valid contour and one empty
        contour_collection<double> top_cc;
        top_cc.contours.emplace_back();  // First contour is empty
        top_cc.contours.back().closed = true;
        top_cc.contours.emplace_back();  // Second contour also empty
        top_cc.contours.back().closed = true;
        // No points in any contour
        
        contour_collection<double> bottom_cc;
        bottom_cc.contours.emplace_back();
        bottom_cc.contours.back().closed = true;
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.0, 0.0));

        REQUIRE_THROWS_AS(
            Triangulate_Planar_Contour_Connectivity(top_cc, bottom_cc),
            std::invalid_argument
        );
    }
}

TEST_CASE( "Triangulate_Planar_Contour_Connectivity with multiple contours" ){
    
    SUBCASE("two separate squares on each plane (separate contours)"){
        // Top plane: two separate squares
        contour_collection<double> top_cc;
        
        // First square
        top_cc.contours.emplace_back();
        top_cc.contours.back().closed = true;
        top_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(1.0, 0.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(0.0, 1.0, 1.0));
        
        // Second square (offset)
        top_cc.contours.emplace_back();
        top_cc.contours.back().closed = true;
        top_cc.contours.back().points.emplace_back(vec3<double>(2.0, 0.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(3.0, 0.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(3.0, 1.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(2.0, 1.0, 1.0));

        // Bottom plane: matching squares
        contour_collection<double> bottom_cc;
        
        bottom_cc.contours.emplace_back();
        bottom_cc.contours.back().closed = true;
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        
        bottom_cc.contours.emplace_back();
        bottom_cc.contours.back().closed = true;
        bottom_cc.contours.back().points.emplace_back(vec3<double>(2.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(3.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(3.0, 1.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(2.0, 1.0, 0.0));

        auto mesh = Triangulate_Planar_Contour_Connectivity(top_cc, bottom_cc);

        // 8 vertices on each plane = 16 total
        REQUIRE(mesh.vertices.size() == 16);
        
        // Should have triangles
        REQUIRE(mesh.faces.size() > 0);
        
        // Surface area should be positive
        REQUIRE(mesh.surface_area() > 0.0);
    }

    SUBCASE("polygon with hole pattern (outer and inner contour with opposite orientations)"){
        // Create an outer square on top (counter-clockwise)
        contour_collection<double> top_cc;
        top_cc.contours.emplace_back();
        top_cc.contours.back().closed = true;
        top_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(4.0, 0.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(4.0, 4.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(0.0, 4.0, 1.0));
        
        // Create an inner square (clockwise - denotes hole)
        top_cc.contours.emplace_back();
        top_cc.contours.back().closed = true;
        top_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(1.0, 3.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(3.0, 3.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(3.0, 1.0, 1.0));

        // Bottom plane with matching contours
        contour_collection<double> bottom_cc;
        bottom_cc.contours.emplace_back();
        bottom_cc.contours.back().closed = true;
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(4.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(4.0, 4.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 4.0, 0.0));
        
        bottom_cc.contours.emplace_back();
        bottom_cc.contours.back().closed = true;
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 3.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(3.0, 3.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(3.0, 1.0, 0.0));

        auto mesh = Triangulate_Planar_Contour_Connectivity(top_cc, bottom_cc);

        // 8 vertices on each plane = 16 total
        REQUIRE(mesh.vertices.size() == 16);
        
        // Should have triangles
        REQUIRE(mesh.faces.size() > 0);
        
        // Surface area should be positive
        REQUIRE(mesh.surface_area() > 0.0);
    }
}

TEST_CASE( "Triangulate_Planar_Contour_Connectivity with oblique planes" ){
    
    SUBCASE("planes not aligned with cardinal axes"){
        // Create contours on a plane tilted at 45 degrees
        const double pi = std::acos(-1.0);
        const double angle = pi / 4.0; // 45 degrees
        
        // Top plane points (rotated around Y axis)
        contour_collection<double> top_cc;
        top_cc.contours.emplace_back();
        top_cc.contours.back().closed = true;
        
        // Original square at z=1, rotated 45 degrees around Y
        auto rotate_y = [angle](const vec3<double> &v) {
            return vec3<double>(
                v.x * std::cos(angle) + v.z * std::sin(angle),
                v.y,
                -v.x * std::sin(angle) + v.z * std::cos(angle)
            );
        };
        
        vec3<double> offset_top(0.0, 0.0, 1.0);
        top_cc.contours.back().points.emplace_back(rotate_y(vec3<double>(0.0, 0.0, 0.0) + offset_top));
        top_cc.contours.back().points.emplace_back(rotate_y(vec3<double>(1.0, 0.0, 0.0) + offset_top));
        top_cc.contours.back().points.emplace_back(rotate_y(vec3<double>(1.0, 1.0, 0.0) + offset_top));
        top_cc.contours.back().points.emplace_back(rotate_y(vec3<double>(0.0, 1.0, 0.0) + offset_top));

        // Bottom plane points
        contour_collection<double> bottom_cc;
        bottom_cc.contours.emplace_back();
        bottom_cc.contours.back().closed = true;
        
        vec3<double> offset_bottom(0.0, 0.0, 0.0);
        bottom_cc.contours.back().points.emplace_back(rotate_y(vec3<double>(0.0, 0.0, 0.0) + offset_bottom));
        bottom_cc.contours.back().points.emplace_back(rotate_y(vec3<double>(1.0, 0.0, 0.0) + offset_bottom));
        bottom_cc.contours.back().points.emplace_back(rotate_y(vec3<double>(1.0, 1.0, 0.0) + offset_bottom));
        bottom_cc.contours.back().points.emplace_back(rotate_y(vec3<double>(0.0, 1.0, 0.0) + offset_bottom));

        auto mesh = Triangulate_Planar_Contour_Connectivity(top_cc, bottom_cc);

        // Check vertices
        REQUIRE(mesh.vertices.size() == 8);
        
        // Should have triangles
        REQUIRE(mesh.faces.size() > 0);
        
        // Surface area should be positive
        REQUIRE(mesh.surface_area() > 0.0);
    }
}

TEST_CASE( "Triangulate_Planar_Contour_Connectivity with float type" ){
    
    SUBCASE("basic square contours with float type"){
        contour_collection<float> top_cc;
        top_cc.contours.emplace_back();
        top_cc.contours.back().closed = true;
        top_cc.contours.back().points.emplace_back(vec3<float>(0.0f, 0.0f, 1.0f));
        top_cc.contours.back().points.emplace_back(vec3<float>(1.0f, 0.0f, 1.0f));
        top_cc.contours.back().points.emplace_back(vec3<float>(1.0f, 1.0f, 1.0f));
        top_cc.contours.back().points.emplace_back(vec3<float>(0.0f, 1.0f, 1.0f));

        contour_collection<float> bottom_cc;
        bottom_cc.contours.emplace_back();
        bottom_cc.contours.back().closed = true;
        bottom_cc.contours.back().points.emplace_back(vec3<float>(0.0f, 0.0f, 0.0f));
        bottom_cc.contours.back().points.emplace_back(vec3<float>(1.0f, 0.0f, 0.0f));
        bottom_cc.contours.back().points.emplace_back(vec3<float>(1.0f, 1.0f, 0.0f));
        bottom_cc.contours.back().points.emplace_back(vec3<float>(0.0f, 1.0f, 0.0f));

        auto mesh = Triangulate_Planar_Contour_Connectivity(top_cc, bottom_cc);

        REQUIRE(mesh.vertices.size() == 8);
        REQUIRE(mesh.faces.size() > 0);
        REQUIRE(mesh.surface_area() > 0.0f);
    }
}

TEST_CASE( "Triangulate_Planar_Contour_Connectivity mesh integrity" ){
    
    SUBCASE("all face indices are valid"){
        contour_collection<double> top_cc;
        top_cc.contours.emplace_back();
        top_cc.contours.back().closed = true;
        top_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(1.0, 0.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(0.0, 1.0, 1.0));

        contour_collection<double> bottom_cc;
        bottom_cc.contours.emplace_back();
        bottom_cc.contours.back().closed = true;
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 1.0, 0.0));

        auto mesh = Triangulate_Planar_Contour_Connectivity(top_cc, bottom_cc);

        const size_t n_verts = mesh.vertices.size();
        for (const auto &face : mesh.faces) {
            for (const auto &idx : face) {
                REQUIRE(idx < n_verts);
            }
        }
    }

    SUBCASE("all triangles have non-zero area"){
        contour_collection<double> top_cc;
        top_cc.contours.emplace_back();
        top_cc.contours.back().closed = true;
        top_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(1.0, 0.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        top_cc.contours.back().points.emplace_back(vec3<double>(0.0, 1.0, 1.0));

        contour_collection<double> bottom_cc;
        bottom_cc.contours.emplace_back();
        bottom_cc.contours.back().closed = true;
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(1.0, 1.0, 0.0));
        bottom_cc.contours.back().points.emplace_back(vec3<double>(0.0, 1.0, 0.0));

        auto mesh = Triangulate_Planar_Contour_Connectivity(top_cc, bottom_cc);

        for (size_t i = 0; i < mesh.faces.size(); ++i) {
            double area = mesh.surface_area(static_cast<int64_t>(i));
            REQUIRE(area > 0.0);
        }
    }
}

