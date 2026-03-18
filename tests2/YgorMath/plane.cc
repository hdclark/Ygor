
#include <limits>
#include <cmath>
#include <vector>

#include <YgorMath.h>

#include "doctest/doctest.h"


TEST_CASE( "Plane_Orthogonal_Regression" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("fit plane to z=5 points"){
        std::vector< vec3<double> > thepoints{
                     vec3<double>( 1.0, 1.0, 5.0 ),
                     vec3<double>( 1.0, 2.0, 5.0 ),
                     vec3<double>( 2.0, 1.0, 5.0 ) };

        const auto theplane = Plane_Orthogonal_Regression(thepoints);
        REQUIRE( std::abs(theplane.N_0.x) < eps );
        REQUIRE( std::abs(theplane.N_0.y) < eps );
        REQUIRE( std::abs(std::abs(theplane.N_0.z) - 1.0) < eps );
        REQUIRE( std::abs(theplane.R_0.z - 5.0) < eps );
    }

    SUBCASE("fit plane to x=5 points"){
        std::vector< vec3<double> > thepoints{
                     vec3<double>( 5.0, 1.0, 1.0 ),
                     vec3<double>( 5.0, 2.0, 1.0 ),
                     vec3<double>( 5.0, 1.0, 2.0 ) };

        const auto theplane = Plane_Orthogonal_Regression(thepoints);
        REQUIRE( std::abs(std::abs(theplane.N_0.x) - 1.0) < eps );
        REQUIRE( std::abs(theplane.N_0.y) < eps );
        REQUIRE( std::abs(theplane.N_0.z) < eps );
        REQUIRE( std::abs(theplane.R_0.x - 5.0) < eps );
    }

    SUBCASE("fit plane to y=5 points"){
        std::vector< vec3<double> > thepoints{
                     vec3<double>( 1.0, 5.0, 1.0 ),
                     vec3<double>( 2.0, 5.0, 1.0 ),
                     vec3<double>( 1.0, 5.0, 2.0 ) };

        const auto theplane = Plane_Orthogonal_Regression(thepoints);
        REQUIRE( std::abs(theplane.N_0.x) < eps );
        REQUIRE( std::abs(std::abs(theplane.N_0.y) - 1.0) < eps );
        REQUIRE( std::abs(theplane.N_0.z) < eps );
        REQUIRE( std::abs(theplane.R_0.y - 5.0) < eps );
    }

    SUBCASE("co-linear data should throw"){
        std::vector< vec3<double> > thepoints{
                     vec3<double>( 0.0, 0.0, 0.0 ),
                     vec3<double>( 1.0, 1.0, 1.0 ),
                     vec3<double>( 2.0, 2.0, 2.0 ) };

        REQUIRE_THROWS( Plane_Orthogonal_Regression(thepoints) );
    }

    SUBCASE("cube data may throw or produce ambiguous result"){
        std::vector< vec3<double> > thepoints{
                     vec3<double>( 0.0, 0.0, 0.0 ),
                     vec3<double>( 5.0, 0.0, 0.0 ),
                     vec3<double>( 0.0, 5.0, 0.0 ),
                     vec3<double>( 5.0, 5.0, 0.0 ),
                     vec3<double>( 0.0, 0.0, 5.0 ),
                     vec3<double>( 5.0, 0.0, 5.0 ),
                     vec3<double>( 0.0, 5.0, 5.0 ),
                     vec3<double>( 5.0, 5.0, 5.0 ) };

        // Cube data is ambiguous; the routine may throw or return an arbitrary plane.
        // The original test noted this was implementation-dependent.
        try{
            const auto theplane = Plane_Orthogonal_Regression(thepoints);
            // If it doesn't throw, just verify the normal is a unit vector.
            REQUIRE( std::abs(theplane.N_0.length() - 1.0) < eps );
        }catch(const std::exception &){
            // Throwing is also acceptable.
            REQUIRE( true );
        }
    }
}


TEST_CASE( "plane-plane intersection" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("orthogonal planes at origin"){
        plane<double> PA( vec3<double>(0.0, 0.0, 1.0), vec3<double>(0.0, 0.0, 0.0) );
        plane<double> PB( vec3<double>(0.0, 1.0, 0.0), vec3<double>(0.0, 0.0, 0.0) );

        line<double> intersection;
        REQUIRE( PA.Intersects_With_Plane_Along_Line(PB, intersection) );

        REQUIRE( std::abs(PA.Get_Signed_Distance_To_Point(intersection.R_0)) < eps );
        REQUIRE( std::abs(PA.Get_Signed_Distance_To_Point(intersection.R_0 + intersection.U_0 * 10.0)) < eps );
        REQUIRE( std::abs(PB.Get_Signed_Distance_To_Point(intersection.R_0)) < eps );
        REQUIRE( std::abs(PB.Get_Signed_Distance_To_Point(intersection.R_0 + intersection.U_0 * 10.0)) < eps );
    }

    SUBCASE("two offset planes"){
        plane<double> PA( vec3<double>(0.0, 0.0, 1.0), vec3<double>(1.2, 2.3, 4.5) );
        plane<double> PB( vec3<double>(0.0, 0.1, 0.9).unit(), vec3<double>(5.6, 6.7, 7.8) );

        line<double> intersection;
        REQUIRE( PA.Intersects_With_Plane_Along_Line(PB, intersection) );

        REQUIRE( std::abs(PA.Get_Signed_Distance_To_Point(intersection.R_0)) < eps );
        REQUIRE( std::abs(PA.Get_Signed_Distance_To_Point(intersection.R_0 + intersection.U_0 * 10.0)) < eps );
        REQUIRE( std::abs(PB.Get_Signed_Distance_To_Point(intersection.R_0)) < eps );
        REQUIRE( std::abs(PB.Get_Signed_Distance_To_Point(intersection.R_0 + intersection.U_0 * 10.0)) < eps );
    }

    SUBCASE("parallel planes do not intersect"){
        plane<double> PA( vec3<double>(0.0, 0.0, 1.0), vec3<double>(1.2, 2.3, 4.5) );
        plane<double> PB( vec3<double>(0.0, 0.0, 1.0), vec3<double>(5.6, 6.7, 7.8) );

        line<double> intersection;
        REQUIRE( !PA.Intersects_With_Plane_Along_Line(PB, intersection) );
    }
}


TEST_CASE( "line_segment-plane intersection" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("segment through z=0 plane"){
        line_segment<double> L( vec3<double>(0.0, 0.0, -1.0), vec3<double>(0.0, 0.0, 1.0) );
        plane<double> PA( vec3<double>(0.0, 0.0, 1.0), vec3<double>(0.0, 0.0, 0.0) );

        vec3<double> P;
        REQUIRE( PA.Intersects_With_Line_Segment_Once(L, P) );
        REQUIRE( P.distance(vec3<double>(0.0, 0.0, 0.0)) < eps );
    }

    SUBCASE("segment does not reach diagonal plane at z=1.1"){
        line_segment<double> L( vec3<double>(0.0, 0.0, -1.0), vec3<double>(0.0, 0.0, 1.0) );
        plane<double> PB( vec3<double>(2.0, 2.0, 2.0), vec3<double>(0.0, 0.0, 1.1) );

        vec3<double> P;
        REQUIRE( !PB.Intersects_With_Line_Segment_Once(L, P) );
    }

    SUBCASE("long segment through origin plane"){
        line_segment<double> L( vec3<double>(0.0, 0.0, -100.0), vec3<double>(0.0, 0.0, 0.1) );
        plane<double> PA( vec3<double>(0.0, 1.0, 1.0), vec3<double>(0.0, 0.0, 0.0) );

        vec3<double> P;
        REQUIRE( PA.Intersects_With_Line_Segment_Once(L, P) );
        REQUIRE( P.distance(vec3<double>(0.0, 0.0, 0.0)) < eps );
    }

    SUBCASE("long segment does not reach offset plane"){
        line_segment<double> L( vec3<double>(0.0, 0.0, -100.0), vec3<double>(0.0, 0.0, 0.1) );
        plane<double> PB( vec3<double>(2.0, 2.0, -2.0), vec3<double>(0.0, 0.0, 0.2) );

        vec3<double> P;
        REQUIRE( !PB.Intersects_With_Line_Segment_Once(L, P) );
    }
}
