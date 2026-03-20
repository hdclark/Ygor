
#include <limits>
#include <cmath>

#include <YgorMath.h>

#include "doctest/doctest.h"


TEST_CASE( "point_set Centroid" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("symmetric points around origin"){
        point_set<double> ps;
        ps.points.emplace_back( -1.0,  0.0,  0.0 );
        ps.points.emplace_back(  1.0,  0.0,  0.0 );
        ps.points.emplace_back(  0.0, -1.0,  0.0 );
        ps.points.emplace_back(  0.0,  1.0,  0.0 );
        ps.points.emplace_back(  0.0,  0.0, -1.0 );
        ps.points.emplace_back(  0.0,  0.0,  1.0 );

        const auto c = ps.Centroid();
        REQUIRE( (c - vec3<double>(0.0, 0.0, 0.0)).length() < eps );
    }

    SUBCASE("points around (-1,0,0)"){
        point_set<double> ps;
        ps.points.emplace_back( -2.0,  0.0,  0.0 );
        ps.points.emplace_back(  0.0,  0.0,  0.0 );
        ps.points.emplace_back( -1.0, -1.0,  0.0 );
        ps.points.emplace_back( -1.0,  1.0,  0.0 );
        ps.points.emplace_back( -1.0,  0.0, -1.0 );
        ps.points.emplace_back( -1.0,  0.0,  1.0 );

        const auto c = ps.Centroid();
        REQUIRE( (c - vec3<double>(-1.0, 0.0, 0.0)).length() < eps );
    }

    SUBCASE("points around (5,-5,3)"){
        point_set<double> ps;
        ps.points.emplace_back(  4.0, -5.0,  3.0 );
        ps.points.emplace_back(  6.0, -5.0,  3.0 );
        ps.points.emplace_back(  5.0, -6.0,  3.0 );
        ps.points.emplace_back(  5.0, -4.0,  3.0 );
        ps.points.emplace_back(  5.0, -5.0,  2.0 );
        ps.points.emplace_back(  5.0, -5.0,  4.0 );

        const auto c = ps.Centroid();
        REQUIRE( (c - vec3<double>(5.0, -5.0, 3.0)).length() < eps );
    }
}
