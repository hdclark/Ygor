
#include <limits>

#include <YgorMath.h>

#include <doctest/doctest.h>


TEST_CASE( "vec3 constructors" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    SUBCASE("finite negative values"){
        vec3<double> v(-1.0, -1.0, -1.0);
        REQUIRE( v.x == -1.0 );
        REQUIRE( v.y == -1.0 );
        REQUIRE( v.z == -1.0 );
    }
    SUBCASE("zeros"){
        vec3<double> v(0.0, 0.0, 0.0);
        REQUIRE( v.x == 0.0 );
        REQUIRE( v.y == 0.0 );
        REQUIRE( v.z == 0.0 );
    }
    SUBCASE("implicit converted zeros"){
        vec3<double> v(0.0f, 0.0f, 0.0f);
        REQUIRE( v.x == 0.0 );
        REQUIRE( v.y == 0.0 );
        REQUIRE( v.z == 0.0 );
    }
    SUBCASE("single NaN value"){
        vec3<double> v(1.0, nan, 1.0);
        REQUIRE( v.x == 1.0 );
        REQUIRE( !std::isfinite(v.y) );
        REQUIRE( v.z == 1.0 );
    }
    SUBCASE("two NaN values"){
        vec3<double> v(nan, nan, 1.0);
        REQUIRE( !std::isfinite(v.x) );
        REQUIRE( !std::isfinite(v.y) );
        REQUIRE( v.z == 1.0 );
    }
    SUBCASE("three NaN values"){
        vec3<double> v(nan, nan, nan);
        REQUIRE( !std::isfinite(v.x) );
        REQUIRE( !std::isfinite(v.y) );
        REQUIRE( !std::isfinite(v.z) );
    }
    SUBCASE("single infinite value"){
        vec3<double> v(1.0, inf, 1.0);
        REQUIRE( v.x == 1.0 );
        REQUIRE( !std::isfinite(v.y) );
        REQUIRE( v.z == 1.0 );
    }
    SUBCASE("two infinite values"){
        vec3<double> v(inf, inf, 1.0);
        REQUIRE( !std::isfinite(v.x) );
        REQUIRE( !std::isfinite(v.y) );
        REQUIRE( v.z == 1.0 );
    }
    SUBCASE("three infinite values"){
        vec3<double> v(inf, inf, inf);
        REQUIRE( !std::isfinite(v.x) );
        REQUIRE( !std::isfinite(v.y) );
        REQUIRE( !std::isfinite(v.z) );
    }

    SUBCASE("copy constructor"){
        vec3<double> v(-1.0, 0.0, 1.0);
        vec3<double> w(v);
        REQUIRE( v.x == w.x );
        REQUIRE( v.y == w.y );
        REQUIRE( v.z == w.z );
    }
}


TEST_CASE( "vec3 operators" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    vec3<double> A(-1.0, 0.0, 1.0);
    SUBCASE("operator=="){
        SUBCASE("finite"){
            vec3<double> B(-1.0, 0.0, 1.0);
            vec3<double> C( 0.0, 0.0, 0.0);
            REQUIRE( A == B );
            REQUIRE( A != C );
        }
        SUBCASE("nonfinite"){
            vec3<double> B(-1.0, 0.0, nan);
            vec3<double> C( nan, nan, nan);
            REQUIRE( A != B );
            REQUIRE( A != C );
            REQUIRE( B != B );
            REQUIRE( B != C );
            REQUIRE( C != C );
        }
    }

    SUBCASE("operator="){
        vec3<double> B = A;
        REQUIRE( A.x == B.x );
        REQUIRE( A.y == B.y );
        REQUIRE( A.z == B.z );
    }

    SUBCASE("operator+"){
        vec3<double> B = A + A + vec3<double>(10.0, 20.0, 30.0);
        REQUIRE( B.x == 8.0 );
        REQUIRE( B.y == 20.0 );
        REQUIRE( B.z == 32.0 );

        B = B + vec3<double>(10.0, 20.0, 30.0);
        REQUIRE( B.x == 18.0 );
        REQUIRE( B.y == 40.0 );
        REQUIRE( B.z == 62.0 );
    }

    SUBCASE("operator+="){
        vec3<double> B(10.0, 20.0, 30.0);
        B += A;
        REQUIRE( B.x == 9.0 );
        REQUIRE( B.y == 20.0 );
        REQUIRE( B.z == 31.0 );

        B += B;
        REQUIRE( B.x == 18.0 );
        REQUIRE( B.y == 40.0 );
        REQUIRE( B.z == 62.0 );

        B += B + B;
        REQUIRE( B.x ==  54.0 );
        REQUIRE( B.y == 120.0 );
        REQUIRE( B.z == 186.0 );
    }

    SUBCASE("operator-"){
        vec3<double> B = A - A - vec3<double>(10.0, 20.0, 30.0);
        REQUIRE( B.x == -10.0 );
        REQUIRE( B.y == -20.0 );
        REQUIRE( B.z == -30.0 );

        B = B - vec3<double>(1.0, 2.0, 3.0);
        REQUIRE( B.x == -11.0 );
        REQUIRE( B.y == -22.0 );
        REQUIRE( B.z == -33.0 );
    }

    SUBCASE("operator-="){
        vec3<double> B(10.0, 20.0, 30.0);
        B -= A;
        REQUIRE( B.x == 11.0 );
        REQUIRE( B.y == 20.0 );
        REQUIRE( B.z == 29.0 );

        B -= B;
        REQUIRE( B.x == 0.0 );
        REQUIRE( B.y == 0.0 );
        REQUIRE( B.z == 0.0 );

        B -= A - B - B;
        REQUIRE( A == B * -1.0 );
    }
}


