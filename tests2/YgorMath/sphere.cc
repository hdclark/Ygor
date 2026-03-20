
#include <limits>
#include <cmath>
#include <vector>

#include <YgorMath.h>

#include "doctest/doctest.h"


TEST_CASE( "Sphere_Orthogonal_Regression" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("fit sphere to cube corners"){
        std::vector< vec3<double> > thepoints{
                     vec3<double>( -1.0, -1.0, -1.0 ),
                     vec3<double>( -1.0, -1.0,  1.0 ),
                     vec3<double>( -1.0,  1.0, -1.0 ),
                     vec3<double>( -1.0,  1.0,  1.0 ),
                     vec3<double>(  1.0, -1.0, -1.0 ),
                     vec3<double>(  1.0, -1.0,  1.0 ),
                     vec3<double>(  1.0,  1.0, -1.0 ),
                     vec3<double>(  1.0,  1.0,  1.0 ) };

        const auto thesphere = Sphere_Orthogonal_Regression(thepoints);
        REQUIRE( thesphere.C_0.distance(vec3<double>(0.0, 0.0, 0.0)) < eps );

        const auto R = std::sqrt(3.0);
        REQUIRE( std::abs(thesphere.r_0 - R) < eps );
    }

    SUBCASE("fit sphere to elongated shape"){
        std::vector< vec3<double> > thepoints{
                     vec3<double>( -2.0, -2.0, -2.0 ),
                     vec3<double>( -1.0, -1.0,  1.0 ),
                     vec3<double>( -1.0,  1.0, -1.0 ),
                     vec3<double>( -1.0,  1.0,  1.0 ),
                     vec3<double>(  1.0, -1.0, -1.0 ),
                     vec3<double>(  1.0, -1.0,  1.0 ),
                     vec3<double>(  1.0,  1.0, -1.0 ),
                     vec3<double>(  2.0,  2.0,  2.0 ) };

        const auto thesphere = Sphere_Orthogonal_Regression(thepoints);
        REQUIRE( thesphere.C_0.distance(vec3<double>(0.0, 0.0, 0.0)) < eps );

        const auto R = std::sqrt(3.0)*(3.0/4.0) + std::sqrt(12.0)*(1.0/4.0);
        REQUIRE( std::abs(thesphere.r_0 - R) < eps );
    }

    SUBCASE("fit sphere to square corners"){
        std::vector< vec3<double> > thepoints{
                     vec3<double>( -1.0, -1.0,  0.0 ),
                     vec3<double>( -1.0,  1.0,  0.0 ),
                     vec3<double>(  1.0, -1.0,  0.0 ),
                     vec3<double>(  1.0,  1.0,  0.0 ) };

        const auto thesphere = Sphere_Orthogonal_Regression(thepoints);
        REQUIRE( thesphere.C_0.distance(vec3<double>(0.0, 0.0, 0.0)) < eps );

        const auto R = std::sqrt(2.0);
        REQUIRE( std::abs(thesphere.r_0 - R) < eps );
    }

    SUBCASE("fit sphere to duplicate points"){
        std::vector< vec3<double> > thepoints{
                     vec3<double>( -2.0, 0.0, 0.0 ),
                     vec3<double>( -2.0, 0.0, 0.0 ),
                     vec3<double>(  2.0, 0.0, 0.0 ),
                     vec3<double>(  2.0, 0.0, 0.0 ) };

        const auto thesphere = Sphere_Orthogonal_Regression(thepoints);
        REQUIRE( thesphere.C_0.distance(vec3<double>(0.0, 0.0, 0.0)) < eps );

        const auto R = 2.0;
        REQUIRE( std::abs(thesphere.r_0 - R) < eps );
    }

    SUBCASE("degenerate data should throw"){
        std::vector< vec3<double> > thepoints{
                     vec3<double>( 1.0, 1.0, 1.0 ),
                     vec3<double>( 1.0, 1.0, 1.0 ),
                     vec3<double>( 1.0, 1.0, 1.0 ),
                     vec3<double>( 1.0, 1.0, 1.0 ) };

        REQUIRE_THROWS( Sphere_Orthogonal_Regression(thepoints) );
    }
}
