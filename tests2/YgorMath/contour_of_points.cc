
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

