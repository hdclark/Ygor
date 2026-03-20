
#include <limits>
#include <list>
#include <utility>
#include <iostream>

#include <YgorMath.h>

#include "doctest/doctest.h"

TEST_CASE( "contour_of_points::Get_Signed_Area" ){
    contour_of_points<double> cop1;
    cop1.closed = true;
    cop1.points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
    cop1.points.emplace_back(vec3<double>(1.0, 0.0, 0.0));
    cop1.points.emplace_back(vec3<double>(1.0, 1.0, 0.0));
    cop1.points.emplace_back(vec3<double>(0.0, 1.0, 0.0));
    
    SUBCASE("signed area for a planar, cardinal-axes-aligned, convex, simple polygon with positive orientation"){
        const auto sa = cop1.Get_Signed_Area();
        REQUIRE(sa == 1.0);
    }

    contour_of_points<double> cop2;
    cop2.closed = true;
    cop2.points.emplace_back(vec3<double>(0.0, 0.0, 0.0));
    cop2.points.emplace_back(vec3<double>(0.0, 1.0, 0.0));
    cop2.points.emplace_back(vec3<double>(1.0, 1.0, 0.0));
    cop2.points.emplace_back(vec3<double>(1.0, 0.0, 0.0));

    SUBCASE("signed area for a planar, cardinal-axes-aligned, convex, simple polygon with negative orientation"){
        const auto sa = cop2.Get_Signed_Area();
        REQUIRE(sa == -1.0);
    }

    // Same as cop1, but rotated 45 degrees.
    contour_of_points<double> cop3 = cop1;
    vec3<double> rot_axis3(vec3<double>(-1.0, 1.0, 0.0));
    const auto pi = std::acos(-1.0);
    const double rot_ang3 = pi * 45.0 / 360.0; // 45 degrees in radians.
    for(auto &v : cop3.points){
        v = v.rotate_around_unit(rot_axis3, rot_ang3);
    }

    // Same as cop1, but rotated -45 degrees.
    contour_of_points<double> cop4 = cop1;
    for(auto &v : cop4.points){
        v = v.rotate_around_unit(rot_axis3, -rot_ang3);
    }

    SUBCASE("signed area for a planar, cardinal-axes-oblique, convex, simple polygon with positive orientation"){
        const auto sa3 = cop3.Get_Signed_Area();
        REQUIRE(std::abs( sa3 - 1.0 ) < 1E-6 );

        const auto sa4 = cop4.Get_Signed_Area();
        REQUIRE(std::abs( sa4 - 1.0 ) < 1E-6 );
    }

}

TEST_CASE( "contour_of_points integration and utilities" ){

    SUBCASE("textbook line integral Q11 p1107"){
        contour_of_points<double> contour({ vec3<double>(0.0, 0.0, 0.0),
                                            vec3<double>(1.0, 2.0, 3.0) });
        contour.closed = false;
        auto f = [](const vec3<double> &r, const vec3<double> &, const vec3<double> &, const vec3<double> &) -> double {
            const double x = r.x, y = r.y, z = r.z;
            return x * std::exp(y * z);
        };
        const double result = contour.Integrate_Simple_Scalar_Kernel(f);
        const double expected = std::sqrt(14.0) * (std::exp(6.0) - 1.0) / 12.0;
        const double rel_tolerance = 1e-6;
        const double abs_tolerance_min = 1e-9;
        double tolerance = std::abs(expected) * rel_tolerance;
        if (tolerance < abs_tolerance_min) {
            tolerance = abs_tolerance_min;
        }
        REQUIRE(std::abs(result - expected) < tolerance);
    }

    SUBCASE("contour equality"){
        contour_of_points<double> contour({ vec3<double>(1.0/9.0, 10.0, 0.0),
                                            vec3<double>(0.0, 10.0, 0.0),
                                            vec3<double>(0.0, -72.123, 0.0),
                                            vec3<double>(5.0, 0.0, 0.0) });
        contour.closed = true;

        contour_of_points<double> contour2({ vec3<double>(5.0, 10.0, 0.0),
                                             vec3<double>(0.1, 10.0, 0.0),
                                             vec3<double>(0.0, 0.0, 0.0),
                                             vec3<double>(5.0, 0.0, 0.0) });
        contour2.closed = true;

        REQUIRE(contour == contour);
        REQUIRE(contour2 == contour2);
        REQUIRE_FALSE(contour == contour2);
    }

    SUBCASE("string write/load round-trip"){
        contour_of_points<double> contour({ vec3<double>(1.0/9.0, 10.0, 0.0),
                                            vec3<double>(0.0, 10.0, 0.0),
                                            vec3<double>(0.0, -72.123, 0.0),
                                            vec3<double>(5.0, 0.0, 0.0) });
        contour.closed = true;

        auto stringified = contour.write_to_string();
        REQUIRE(contour.load_from_string(stringified));
    }

    SUBCASE("line_segment Sample_With_Spacing"){
        const vec3<double> A(10.0, 0.0, 0.0);
        const vec3<double> B(11.0, 0.0, 0.0);
        const line_segment<double> line(A, B);

        double spacing = 0.3, offset = 0.05, remain = 0.0;
        auto somepoints = line.Sample_With_Spacing(spacing, offset, remain);
        REQUIRE(somepoints.size() == 4);
    }
}

