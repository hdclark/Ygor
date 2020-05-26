
#include <limits>
#include <utility>
#include <iostream>

#include <YgorMath.h>

#include "doctest/doctest.h"


TEST_CASE( "line constructors" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    const vec3<double> x_unit(1.0, 0.0, 0.0);
    const vec3<double> y_unit(0.0, 1.0, 0.0);
    const vec3<double> z_unit(0.0, 0.0, 1.0);
    const vec3<double> zero(0.0, 0.0, 0.0);

    SUBCASE("cartersian-aligned unit inputs"){
        line<double> A(zero, x_unit);
        REQUIRE( A.R_0 == zero );
        REQUIRE( A.U_0 == x_unit );

        line<double> B(zero, x_unit * -1.0);
        REQUIRE( B.R_0 == zero );
        REQUIRE( B.U_0 == x_unit * -1.0 );

        line<double> C(zero, y_unit);
        REQUIRE( C.R_0 == zero );
        REQUIRE( C.U_0 == y_unit );

        line<double> D(zero, y_unit * -1.0);
        REQUIRE( D.R_0 == zero );
        REQUIRE( D.U_0 == y_unit * -1.0 );

        line<double> E(zero, z_unit);
        REQUIRE( E.R_0 == zero );
        REQUIRE( E.U_0 == z_unit );

        line<double> F(zero, z_unit * -1.0);
        REQUIRE( F.R_0 == zero );
        REQUIRE( F.U_0 == z_unit * -1.0 );
    }

    SUBCASE("sums of cartersian-aligned unit inputs"){
        line<double> A(x_unit * -1.0,
                       x_unit *  1.0);
        REQUIRE( A.R_0 == x_unit * -1.0 );
        REQUIRE( A.U_0 == x_unit *  1.0 );

        line<double> B(x_unit *  1.0, 
                       x_unit * -1.0);
        REQUIRE( B.R_0 == x_unit *  1.0 );
        REQUIRE( B.U_0 == x_unit * -1.0 );

        line<double> C(y_unit * -1.0,
                       y_unit *  1.0);
        REQUIRE( C.R_0 == y_unit * -1.0 );
        REQUIRE( C.U_0 == y_unit *  1.0 );

        line<double> D(y_unit *  1.0, 
                       y_unit * -1.0);
        REQUIRE( D.R_0 == y_unit *  1.0 );
        REQUIRE( D.U_0 == y_unit * -1.0 );

        line<double> E(z_unit * -1.0,
                       z_unit *  1.0);
        REQUIRE( E.R_0 == z_unit * -1.0 );
        REQUIRE( E.U_0 == z_unit *  1.0 );

        line<double> F(z_unit *  1.0, 
                       z_unit * -1.0);
        REQUIRE( F.R_0 == z_unit *  1.0 );
        REQUIRE( F.U_0 == z_unit * -1.0 );

        line<double> G((x_unit * -1.0 + y_unit * -1.0 + z_unit * -1.0),
                       zero);
        REQUIRE( G.R_0 == (x_unit * -1.0 + y_unit * -1.0 + z_unit * -1.0) );
        REQUIRE( G.U_0 == (x_unit *  1.0 + y_unit *  1.0 + z_unit *  1.0).unit() );

        line<double> H((x_unit * -1.0 + y_unit * -1.0 + z_unit * -1.0),
                       (x_unit *  1.0 + y_unit *  1.0 + z_unit *  1.0));
        REQUIRE( H.R_0 == (x_unit * -1.0 + y_unit * -1.0 + z_unit * -1.0) );
        REQUIRE( H.U_0 == (x_unit *  1.0 + y_unit *  1.0 + z_unit *  1.0).unit() );
    }

    SUBCASE("NaN inputs"){
        const vec3<double> vA(nan, 0.0, 0.0);
        const vec3<double> vB(1.0, 1.0, 1.0);
        REQUIRE_THROWS( line<double>(vA, vB) );
    
        const vec3<double> vC(0.0, nan, 0.0);
        const vec3<double> vD(1.0, 1.0, 1.0);
        REQUIRE_THROWS( line<double>(vC, vD) );
    
        const vec3<double> vE(0.0, 0.0, nan);
        const vec3<double> vF(0.0, 0.0, 1.0);
        REQUIRE_THROWS( line<double>(vE, vF) );
    
        const vec3<double> vG(1.0, 1.0, 1.0);
        const vec3<double> vH(nan, 0.0, 0.0);
        REQUIRE_THROWS( line<double>(vG, vH) );
    
        const vec3<double> vI(1.0, 1.0, 1.0);
        const vec3<double> vJ(0.0, nan, 0.0);
        REQUIRE_THROWS( line<double>(vI, vJ) );
    
        const vec3<double> vK(0.0, 0.0, nan);
        const vec3<double> vL(0.0, 0.0, 1.0);
        REQUIRE_THROWS( line<double>(vK, vL) );
    }

    SUBCASE("infinite inputs"){
        const vec3<double> vA(inf, 0.0, 0.0);
        const vec3<double> vB(1.0, 1.0, 1.0);
        REQUIRE_THROWS( line<double>(vA, vB) );
    
        const vec3<double> vC(0.0, inf, 0.0);
        const vec3<double> vD(1.0, 1.0, 1.0);
        REQUIRE_THROWS( line<double>(vC, vD) );
    
        const vec3<double> vE(0.0, 0.0, inf);
        const vec3<double> vF(0.0, 0.0, 1.0);
        REQUIRE_THROWS( line<double>(vE, vF) );
    
        const vec3<double> vG(1.0, 1.0, 1.0);
        const vec3<double> vH(inf, 0.0, 0.0);
        REQUIRE_THROWS( line<double>(vG, vH) );
    
        const vec3<double> vI(1.0, 1.0, 1.0);
        const vec3<double> vJ(0.0, inf, 0.0);
        REQUIRE_THROWS( line<double>(vI, vJ) );
    
        const vec3<double> vK(0.0, 0.0, inf);
        const vec3<double> vL(0.0, 0.0, 1.0);
        REQUIRE_THROWS( line<double>(vK, vL) );
    }

    SUBCASE("degenerate inputs"){
        REQUIRE_THROWS( line<double>(zero, zero) );

        REQUIRE_THROWS( line<double>(x_unit, x_unit) );
        REQUIRE_THROWS( line<double>(y_unit, y_unit) );
        REQUIRE_THROWS( line<double>(z_unit, z_unit) );

        REQUIRE_THROWS( line<double>(x_unit * -1.0, x_unit * -1.0) );
        REQUIRE_THROWS( line<double>(y_unit * -1.0, y_unit * -1.0) );
        REQUIRE_THROWS( line<double>(z_unit * -1.0, z_unit * -1.0) );
    }
}

TEST_CASE( "line operators" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    const vec3<double> x_unit(1.0, 0.0, 0.0);
    const vec3<double> y_unit(0.0, 1.0, 0.0);
    const vec3<double> z_unit(0.0, 0.0, 1.0);
    const vec3<double> zero(0.0, 0.0, 0.0);

    SUBCASE("operator== and operator!="){
        line<double> L1( zero,   x_unit );
        line<double> L2( zero,   x_unit );
        line<double> L3( x_unit, y_unit );
        line<double> L4( x_unit, zero   );
        REQUIRE( L1 == L2 );
        REQUIRE( L1 != L3 );
        REQUIRE( L1 != L4 );
    }

    SUBCASE("operator="){
        line<double> L1( zero, x_unit );
        line<double> L2 = L1;
        REQUIRE( L1 == L2 );

        auto L3 = line<double>( zero, y_unit );
        auto L4 = line<double>( zero, z_unit );
        auto L5 = line<double>( zero, x_unit * -1.0 );
        auto L6 = line<double>( zero, y_unit * -1.0 );
        auto L7 = line<double>( zero, z_unit * -1.0 );
        auto L8 = line<double>( x_unit, zero );
        REQUIRE( L1 != L3 );
        REQUIRE( L1 != L4 );
        REQUIRE( L1 != L5 );
        REQUIRE( L1 != L6 );
        REQUIRE( L1 != L7 );
        REQUIRE( L1 != L8 );
    }
}

