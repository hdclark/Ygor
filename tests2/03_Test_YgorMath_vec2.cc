
#include <limits>

#include <YgorMath.h>

#include <doctest/doctest.h>


TEST_CASE( "vec2 constructors are valid" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    SUBCASE("finite negative values"){
        vec2<double> v(-1.0, -1.0);
        REQUIRE( v.x == -1.0 );
        REQUIRE( v.y == -1.0 );
    }
    SUBCASE("zeros"){
        vec2<double> v(0.0, 0.0);
        REQUIRE( v.x == 0.0 );
        REQUIRE( v.y == 0.0 );
    }
    SUBCASE("implicit converted zeros"){
        vec2<double> v(0.0f, 0.0f);
        REQUIRE( v.x == 0.0 );
        REQUIRE( v.y == 0.0 );
    }
    SUBCASE("single NaN value"){
        vec2<double> v(1.0, nan);
        REQUIRE( v.x == 1.0 );
        REQUIRE( !std::isfinite(v.y) );
    }
    SUBCASE("two NaN values"){
        vec2<double> v(nan, nan);
        REQUIRE( !std::isfinite(v.x) );
        REQUIRE( !std::isfinite(v.y) );
    }
    SUBCASE("single infinite value"){
        vec2<double> v(1.0, inf);
        REQUIRE( v.x == 1.0 );
        REQUIRE( !std::isfinite(v.y) );
    }
    SUBCASE("two infinite values"){
        vec2<double> v(inf, inf);
        REQUIRE( !std::isfinite(v.x) );
        REQUIRE( !std::isfinite(v.y) );
    }
}

