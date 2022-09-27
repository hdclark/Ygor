
#include <limits>
#include <utility>
#include <iostream>
#include <array>
#include <type_traits>

#include <YgorMath.h>

#include "doctest/doctest.h"


TEST_CASE( "vec3 class layout" ){
    SUBCASE("is_standard_layout"){
        REQUIRE(std::is_standard_layout<vec3<double>>::value);
        REQUIRE(std::is_standard_layout<vec3<float >>::value);
    }
    SUBCASE("no padding is present and no additional members have been added"){
        const auto s_vd = sizeof(vec3<double>);
        const auto s_cd = sizeof(double);
        const auto s_vf = sizeof(vec3<float>);
        const auto s_cf = sizeof(float);
        REQUIRE(s_vd == 3*s_cd);
        REQUIRE(s_vf == 3*s_cf);
    }
    SUBCASE("arrays of vec3s have no padding between elements"){
        const auto a_vd = std::alignment_of< vec3<double> >::value;
        const auto a_vf = std::alignment_of< vec3<float > >::value;
        const bool d = (8 == a_vd) || (4 == a_vd); // depends on compiler and architecture.
        REQUIRE(d);
        REQUIRE(4 == a_vf);
    }

    SUBCASE("arrays of vec3s can be addressed as an array of raw coordinates"){
        std::array<vec3<double>, 2> ad;
        ad[0] = vec3<double>(1.23, 2.34, 3.45);
        ad[1] = vec3<double>(4.56, 5.67, 6.78);
        auto pd = reinterpret_cast<double*>(&(ad[0]));
        REQUIRE(*(pd + 0) == 1.23);
        REQUIRE(*(pd + 1) == 2.34);
        REQUIRE(*(pd + 2) == 3.45);
        REQUIRE(*(pd + 3) == 4.56);
        REQUIRE(*(pd + 4) == 5.67);
        REQUIRE(*(pd + 5) == 6.78);

        std::array<vec3<float>, 2> af;
        af[0] = vec3<float>(1.23f, 2.34f, 3.45f);
        af[1] = vec3<float>(4.56f, 5.67f, 6.78f);
        auto pf = reinterpret_cast<float*>(&(af[0]));
        REQUIRE(*(pf + 0) == 1.23f);
        REQUIRE(*(pf + 1) == 2.34f);
        REQUIRE(*(pf + 2) == 3.45f);
        REQUIRE(*(pf + 3) == 4.56f);
        REQUIRE(*(pf + 4) == 5.67f);
        REQUIRE(*(pf + 5) == 6.78f);
    }
}

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

    SUBCASE("std::array constructor"){
        std::array<double,3> w{{ -1.0, 1.0, 2.0 }};
        vec3<double> v(w);
        REQUIRE( v.x == -1.0 );
        REQUIRE( v.y == 1.0 );
        REQUIRE( v.z == 2.0 );
    }
}


TEST_CASE( "vec3 operators" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();

    vec3<double> A(-1.0, 0.0, 1.0);
    SUBCASE("operator== and operator!="){
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

    SUBCASE("operator= with an implicit conversion via std::array"){
        vec3<double> v = {{ -1.0, 1.0, 2.0 }};
        REQUIRE( v.x == -1.0 );
        REQUIRE( v.y == 1.0 );
        REQUIRE( v.z == 2.0 );
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

    SUBCASE("operator<"){
        vec3<double> B = A;
        vec3<double> C(A.x - 1.0, A.y, A.z);
        vec3<double> D(A.x, A.y - 1.0, A.z);
        vec3<double> E(A.x, A.y, A.z - 1.0);

        REQUIRE( !(B < A) );

        REQUIRE( C < A );
        REQUIRE( !(A < C) );

        REQUIRE( D < A );
        REQUIRE( !(A < D) );

        REQUIRE( E < A );
        REQUIRE( !(A < E) );
    }

    A = vec3<double>(40.0, 80.0, 160.0);
    SUBCASE("operator* and operator*= with a scalar"){
        vec3<double> B = A * -2.0;

        REQUIRE( B.x == A.x * -2.0 );
        REQUIRE( B.y == A.y * -2.0 );
        REQUIRE( B.z == A.z * -2.0 );

        B *= -2.0;

        REQUIRE( B.x == A.x * -2.0 * -2.0 );
        REQUIRE( B.y == A.y * -2.0 * -2.0 );
        REQUIRE( B.z == A.z * -2.0 * -2.0 );
    }

    SUBCASE("operator/ and operator/= with a scalar"){
        vec3<double> B = A / -2.0;

        REQUIRE( B.x == A.x / -2.0 );
        REQUIRE( B.y == A.y / -2.0 );
        REQUIRE( B.z == A.z / -2.0 );

        B /= -2.0;

        REQUIRE( B.x == A.x / -2.0 / -2.0 );
        REQUIRE( B.y == A.y / -2.0 / -2.0 );
        REQUIRE( B.z == A.z / -2.0 / -2.0 );
    }

    SUBCASE("operator[]"){
        REQUIRE( A[0] == A.x );
        REQUIRE( A[1] == A.y );
        REQUIRE( A[2] == A.z );
    }

    SUBCASE("cast operator to std::array"){
        SUBCASE("explicit casts"){
            vec3<double> v(-1.0, 1.0, 2.0);
            auto w = static_cast< std::array<double,3> >(v);
            REQUIRE( v.x == -1.0 );
            REQUIRE( v.y == 1.0 );
            REQUIRE( v.z == 2.0 );

            REQUIRE( v.x == w.at(0) );
            REQUIRE( v.y == w.at(1) );
            REQUIRE( v.z == w.at(2) );
        }
        SUBCASE("implicit casts"){
            vec3<double> v(-1.0, 1.0, 2.0);
            std::array<double,3> w{{ -10.0, 11.0, 22.0 }};

            std::array<double,3> z = v + w;
            REQUIRE( z.at(0) == (v.x + w.at(0)) );
            REQUIRE( z.at(1) == (v.y + w.at(1)) );
            REQUIRE( z.at(2) == (v.z + w.at(2)) );
        }
    }
}


TEST_CASE( "vec3 member functions" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );
    const auto pi = std::acos(-1.0);

    const vec3<double> x_unit(1.0, 0.0, 0.0);
    const vec3<double> y_unit(0.0, 1.0, 0.0);
    const vec3<double> z_unit(0.0, 0.0, 1.0);
    const vec3<double> zero(0.0, 0.0, 0.0);

    SUBCASE("isfinite with NaNs"){
        const vec3<double> A( 1.0, 1.0, 1.0);
        const vec3<double> B( nan, 1.0, 1.0);
        const vec3<double> C( nan, nan, 1.0);
        const vec3<double> D( nan, nan, nan);
        const vec3<double> E( nan, 1.0, nan);
        const vec3<double> F( 1.0, 1.0, nan);
        const vec3<double> G( 1.0, nan, nan);

        REQUIRE(  A.isfinite() );
        REQUIRE( !B.isfinite() );
        REQUIRE( !C.isfinite() );
        REQUIRE( !D.isfinite() );
        REQUIRE( !E.isfinite() );
        REQUIRE( !F.isfinite() );
        REQUIRE( !G.isfinite() );
    }
    SUBCASE("isfinite with infs"){
        const vec3<double> A( 1.0, 1.0, 1.0);
        const vec3<double> B( inf, 1.0, 1.0);
        const vec3<double> C( inf, inf, 1.0);
        const vec3<double> D( inf, inf, inf);
        const vec3<double> E( inf, 1.0, inf);
        const vec3<double> F( 1.0, 1.0, inf);
        const vec3<double> G( 1.0, inf, inf);

        REQUIRE(  A.isfinite() );
        REQUIRE( !B.isfinite() );
        REQUIRE( !C.isfinite() );
        REQUIRE( !D.isfinite() );
        REQUIRE( !E.isfinite() );
        REQUIRE( !F.isfinite() );
        REQUIRE( !G.isfinite() );
    }

    SUBCASE("dot product"){
        REQUIRE(  x_unit.Dot(x_unit) == 1.0 );
        REQUIRE(  y_unit.Dot(y_unit) == 1.0 );
        REQUIRE(  z_unit.Dot(z_unit) == 1.0 );

        REQUIRE(  x_unit.Dot(x_unit * -1.0) == -1.0 );
        REQUIRE(  y_unit.Dot(y_unit * -1.0) == -1.0 );
        REQUIRE(  z_unit.Dot(z_unit * -1.0) == -1.0 );

        REQUIRE(  x_unit.Dot(y_unit) == 0.0 );
        REQUIRE(  x_unit.Dot(z_unit) == 0.0 );
        REQUIRE(  y_unit.Dot(z_unit) == 0.0 );

        REQUIRE(  x_unit.Dot(y_unit * -1.0) == 0.0 );
        REQUIRE(  x_unit.Dot(z_unit * -1.0) == 0.0 );
        REQUIRE(  y_unit.Dot(z_unit * -1.0) == 0.0 );

        REQUIRE(  x_unit.Dot(x_unit + y_unit + z_unit) == 1.0 );
        REQUIRE(  y_unit.Dot(x_unit + y_unit + z_unit) == 1.0 );
        REQUIRE(  z_unit.Dot(x_unit + y_unit + z_unit) == 1.0 );
    }

    SUBCASE("cross product"){
        REQUIRE(  x_unit.Cross(x_unit) == zero );
        REQUIRE(  y_unit.Cross(y_unit) == zero );
        REQUIRE(  z_unit.Cross(z_unit) == zero );

        REQUIRE(  x_unit.Cross(y_unit) ==  z_unit );
        REQUIRE(  x_unit.Cross(z_unit) ==  y_unit * -1.0 );
        REQUIRE(  y_unit.Cross(z_unit) ==  x_unit );

        REQUIRE(  y_unit.Cross(x_unit) ==  z_unit * -1.0 );
        REQUIRE(  z_unit.Cross(x_unit) ==  y_unit );
        REQUIRE(  z_unit.Cross(y_unit) ==  x_unit * -1.0 );

        REQUIRE(  y_unit.Cross(x_unit * -1.0) == z_unit );
        REQUIRE(  z_unit.Cross(x_unit * -1.0) == y_unit * -1.0 );
        REQUIRE(  z_unit.Cross(y_unit * -1.0) == x_unit );
    }

    SUBCASE("mask product"){
        const vec3<double> A(10.0, 20.0, 30.0);
        const vec3<double> B(2.0, 3.0, 4.0);
        const auto C = A.Mask(B);
        REQUIRE(  C.x == (A.x * B.x) );
        REQUIRE(  C.y == (A.y * B.y) );
        REQUIRE(  C.z == (A.z * B.z) );
    }

    SUBCASE("unit"){
        REQUIRE( x_unit.unit() == x_unit );
        REQUIRE( y_unit.unit() == y_unit );
        REQUIRE( z_unit.unit() == z_unit );

        const auto A = vec3<double>(10.0, -20.0, 30.0).unit();
        const auto A_length = std::sqrt(A.x * A.x + A.y * A.y + A.z * A.z);
        REQUIRE( (A_length - 1.0) < eps );

        const auto B = zero.unit();
        REQUIRE( !std::isfinite(B.x) );
        REQUIRE( !std::isfinite(B.y) );
        REQUIRE( !std::isfinite(B.z) );
    }

    SUBCASE("length"){
        REQUIRE( x_unit.length() == 1.0 );
        REQUIRE( y_unit.length() == 1.0 );
        REQUIRE( z_unit.length() == 1.0 );

        REQUIRE( zero.length() == 0.0 );

        const auto A = vec3<double>(10.0, -20.0, 30.0);
        REQUIRE( std::abs(A.length() - std::sqrt(1400.0)) < eps );
    }

    SUBCASE("sq_length"){
        REQUIRE( x_unit.sq_length() == 1.0 );
        REQUIRE( y_unit.sq_length() == 1.0 );
        REQUIRE( z_unit.sq_length() == 1.0 );

        REQUIRE( zero.sq_length() == 0.0 );

        const auto A = vec3<double>(10.0, -20.0, 30.0);
        REQUIRE( std::abs(A.sq_length() - 1400.0) < eps );
    }

    SUBCASE("distance"){
        REQUIRE( zero.distance(zero) == 0.0 );

        REQUIRE( x_unit.distance(x_unit) == 0.0 );
        REQUIRE( y_unit.distance(y_unit) == 0.0 );
        REQUIRE( z_unit.distance(z_unit) == 0.0 );

        REQUIRE( x_unit.distance(zero) == 1.0 );
        REQUIRE( y_unit.distance(zero) == 1.0 );
        REQUIRE( z_unit.distance(zero) == 1.0 );

        REQUIRE( std::abs(x_unit.distance(y_unit) - std::sqrt(2.0)) < eps );
        REQUIRE( std::abs(x_unit.distance(z_unit) - std::sqrt(2.0)) < eps );
        REQUIRE( std::abs(y_unit.distance(z_unit) - std::sqrt(2.0)) < eps );

        REQUIRE( std::abs(x_unit.distance(y_unit * -1.0) - std::sqrt(2.0)) < eps );
        REQUIRE( std::abs(x_unit.distance(z_unit * -1.0) - std::sqrt(2.0)) < eps );
        REQUIRE( std::abs(y_unit.distance(z_unit * -1.0) - std::sqrt(2.0)) < eps );
    }

    SUBCASE("sq_dist"){
        REQUIRE( zero.sq_dist(zero) == 0.0 );

        REQUIRE( x_unit.sq_dist(x_unit) == 0.0 );
        REQUIRE( y_unit.sq_dist(y_unit) == 0.0 );
        REQUIRE( z_unit.sq_dist(z_unit) == 0.0 );

        REQUIRE( x_unit.sq_dist(zero) == 1.0 );
        REQUIRE( y_unit.sq_dist(zero) == 1.0 );
        REQUIRE( z_unit.sq_dist(zero) == 1.0 );

        REQUIRE( std::abs(x_unit.sq_dist(y_unit) - 2.0) < eps );
        REQUIRE( std::abs(x_unit.sq_dist(z_unit) - 2.0) < eps );
        REQUIRE( std::abs(y_unit.sq_dist(z_unit) - 2.0) < eps );

        REQUIRE( std::abs(x_unit.sq_dist(y_unit * -1.0) - 2.0) < eps );
        REQUIRE( std::abs(x_unit.sq_dist(z_unit * -1.0) - 2.0) < eps );
        REQUIRE( std::abs(y_unit.sq_dist(z_unit * -1.0) - 2.0) < eps );
    }

    SUBCASE("angle"){
        bool OK = true;

        REQUIRE( (x_unit.angle(x_unit, &OK) == 0.0) );
        REQUIRE( OK );
        REQUIRE( (y_unit.angle(y_unit, &OK) == 0.0) );
        REQUIRE( OK );
        REQUIRE( (z_unit.angle(z_unit, &OK) == 0.0) );
        REQUIRE( OK );

        REQUIRE( (std::abs(x_unit.angle(y_unit, &OK) - pi * 0.5) < eps) );
        REQUIRE( OK );
        REQUIRE( (std::abs(x_unit.angle(z_unit, &OK) - pi * 0.5) < eps) );
        REQUIRE( OK );
        REQUIRE( (std::abs(y_unit.angle(z_unit, &OK) - pi * 0.5) < eps) );
        REQUIRE( OK );

        REQUIRE( (std::abs(x_unit.angle(x_unit * -1.0, &OK) - pi) < eps) );
        REQUIRE( OK );
        REQUIRE( (std::abs(y_unit.angle(y_unit * -1.0, &OK) - pi) < eps) );
        REQUIRE( OK );
        REQUIRE( (std::abs(z_unit.angle(z_unit * -1.0, &OK) - pi) < eps) );
        REQUIRE( OK );

        REQUIRE( (std::abs((x_unit + z_unit).angle((x_unit + z_unit) * -1.0, &OK) - pi) < eps) );
        REQUIRE( OK );
        REQUIRE( (std::abs((x_unit + y_unit).angle((x_unit + y_unit) * -1.0, &OK) - pi) < eps) );
        REQUIRE( OK );
        REQUIRE( (std::abs((y_unit + x_unit).angle((y_unit + x_unit) * -1.0, &OK) - pi) < eps) );
        REQUIRE( OK );

        x_unit.angle(zero, &OK);
        REQUIRE( !OK );
        y_unit.angle(zero, &OK);
        REQUIRE( !OK );
        z_unit.angle(zero, &OK);
        REQUIRE( !OK );
        zero.angle(zero, &OK);
        REQUIRE( !OK );
    }

    SUBCASE("zero"){
        REQUIRE( zero == vec3<double>().zero() );
        REQUIRE( zero == x_unit.zero() );
        REQUIRE( zero != x_unit );
    }

    SUBCASE("rotate_around_x, rotate_around_y, rotate_around_z"){
        const double agreement = 1.0 - eps;

        // Correct orientations.
        REQUIRE( agreement < x_unit.rotate_around_x(0.0).Dot(x_unit) );
        REQUIRE( agreement < x_unit.rotate_around_x(0.1).Dot(x_unit) );
        REQUIRE( agreement < x_unit.rotate_around_x(1.0).Dot(x_unit) );

        REQUIRE( agreement < y_unit.rotate_around_y(0.0).Dot(y_unit) );
        REQUIRE( agreement < y_unit.rotate_around_y(0.1).Dot(y_unit) );
        REQUIRE( agreement < y_unit.rotate_around_y(1.0).Dot(y_unit) );

        REQUIRE( agreement < z_unit.rotate_around_z(0.0).Dot(z_unit) );
        REQUIRE( agreement < z_unit.rotate_around_z(0.1).Dot(z_unit) );
        REQUIRE( agreement < z_unit.rotate_around_z(1.0).Dot(z_unit) );

        REQUIRE( agreement < x_unit.rotate_around_y(2.0 * pi).Dot(x_unit) );
        REQUIRE( agreement < x_unit.rotate_around_z(2.0 * pi).Dot(x_unit) );
        REQUIRE( agreement < y_unit.rotate_around_x(2.0 * pi).Dot(y_unit) );
        REQUIRE( agreement < y_unit.rotate_around_y(2.0 * pi).Dot(y_unit) );
        REQUIRE( agreement < z_unit.rotate_around_x(2.0 * pi).Dot(z_unit) );
        REQUIRE( agreement < z_unit.rotate_around_y(2.0 * pi).Dot(z_unit) );

        REQUIRE( agreement < x_unit.rotate_around_y(8.0 * pi).Dot(x_unit) );
        REQUIRE( agreement < x_unit.rotate_around_z(8.0 * pi).Dot(x_unit) );
        REQUIRE( agreement < y_unit.rotate_around_x(8.0 * pi).Dot(y_unit) );
        REQUIRE( agreement < y_unit.rotate_around_y(8.0 * pi).Dot(y_unit) );
        REQUIRE( agreement < z_unit.rotate_around_x(8.0 * pi).Dot(z_unit) );
        REQUIRE( agreement < z_unit.rotate_around_y(8.0 * pi).Dot(z_unit) );

        REQUIRE( agreement < x_unit.rotate_around_z(0.5 * pi).Dot(y_unit) );
        REQUIRE( agreement < y_unit.rotate_around_z(1.5 * pi).Dot(x_unit) );
        REQUIRE( agreement < z_unit.rotate_around_y(0.5 * pi).Dot(x_unit) );
        REQUIRE( agreement < x_unit.rotate_around_y(1.5 * pi).Dot(z_unit) );
        REQUIRE( agreement < y_unit.rotate_around_x(0.5 * pi).Dot(z_unit) );
        REQUIRE( agreement < z_unit.rotate_around_x(1.5 * pi).Dot(y_unit) );

        // Maintains length when rotated.
        REQUIRE( (x_unit.rotate_around_z(1.0).length() - 1.0) < eps );
        REQUIRE( (y_unit.rotate_around_z(1.0).length() - 1.0) < eps );

        REQUIRE( (x_unit.rotate_around_y(1.0).length() - 1.0) < eps );
        REQUIRE( (z_unit.rotate_around_y(1.0).length() - 1.0) < eps );

        REQUIRE( (y_unit.rotate_around_x(1.0).length() - 1.0) < eps );
        REQUIRE( (z_unit.rotate_around_x(1.0).length() - 1.0) < eps );

        REQUIRE( ((x_unit * -20.0).rotate_around_z(1.0).length() - 20.0) < eps );
        REQUIRE( ((y_unit * -20.0).rotate_around_z(1.0).length() - 20.0) < eps );

        REQUIRE( ((x_unit * -20.0).rotate_around_y(1.0).length() - 20.0) < eps );
        REQUIRE( ((z_unit * -20.0).rotate_around_y(1.0).length() - 20.0) < eps );

        REQUIRE( ((y_unit * -20.0).rotate_around_x(1.0).length() - 20.0) < eps );
        REQUIRE( ((z_unit * -20.0).rotate_around_x(1.0).length() - 20.0) < eps );
    }

    SUBCASE("rotate_around_unit"){
        const double agreement = 1.0 - eps;

        // Correct orientations.
        REQUIRE( agreement < x_unit.rotate_around_unit(x_unit, 0.0).Dot(x_unit) );
        REQUIRE( agreement < x_unit.rotate_around_unit(x_unit, 0.1).Dot(x_unit) );
        REQUIRE( agreement < x_unit.rotate_around_unit(x_unit, 1.0).Dot(x_unit) );

        REQUIRE( agreement < y_unit.rotate_around_unit(y_unit, 0.0).Dot(y_unit) );
        REQUIRE( agreement < y_unit.rotate_around_unit(y_unit, 0.1).Dot(y_unit) );
        REQUIRE( agreement < y_unit.rotate_around_unit(y_unit, 1.0).Dot(y_unit) );

        REQUIRE( agreement < z_unit.rotate_around_unit(z_unit, 0.0).Dot(z_unit) );
        REQUIRE( agreement < z_unit.rotate_around_unit(z_unit, 0.1).Dot(z_unit) );
        REQUIRE( agreement < z_unit.rotate_around_unit(z_unit, 1.0).Dot(z_unit) );

        REQUIRE( agreement < x_unit.rotate_around_unit(y_unit, 2.0 * pi).Dot(x_unit) );
        REQUIRE( agreement < x_unit.rotate_around_unit(z_unit, 2.0 * pi).Dot(x_unit) );
        REQUIRE( agreement < y_unit.rotate_around_unit(x_unit, 2.0 * pi).Dot(y_unit) );
        REQUIRE( agreement < y_unit.rotate_around_unit(y_unit, 2.0 * pi).Dot(y_unit) );
        REQUIRE( agreement < z_unit.rotate_around_unit(x_unit, 2.0 * pi).Dot(z_unit) );
        REQUIRE( agreement < z_unit.rotate_around_unit(y_unit, 2.0 * pi).Dot(z_unit) );

        REQUIRE( agreement < x_unit.rotate_around_unit(y_unit, 8.0 * pi).Dot(x_unit) );
        REQUIRE( agreement < x_unit.rotate_around_unit(z_unit, 8.0 * pi).Dot(x_unit) );
        REQUIRE( agreement < y_unit.rotate_around_unit(x_unit, 8.0 * pi).Dot(y_unit) );
        REQUIRE( agreement < y_unit.rotate_around_unit(y_unit, 8.0 * pi).Dot(y_unit) );
        REQUIRE( agreement < z_unit.rotate_around_unit(x_unit, 8.0 * pi).Dot(z_unit) );
        REQUIRE( agreement < z_unit.rotate_around_unit(y_unit, 8.0 * pi).Dot(z_unit) );

        REQUIRE( agreement < x_unit.rotate_around_unit(z_unit, 0.5 * pi).Dot(y_unit) );
        REQUIRE( agreement < y_unit.rotate_around_unit(z_unit, 1.5 * pi).Dot(x_unit) );
        REQUIRE( agreement < z_unit.rotate_around_unit(y_unit, 0.5 * pi).Dot(x_unit) );
        REQUIRE( agreement < x_unit.rotate_around_unit(y_unit, 1.5 * pi).Dot(z_unit) );
        REQUIRE( agreement < y_unit.rotate_around_unit(x_unit, 0.5 * pi).Dot(z_unit) );
        REQUIRE( agreement < z_unit.rotate_around_unit(x_unit, 1.5 * pi).Dot(y_unit) );

        // Rotation about an arbitrary axis is 2pi cyclical.
        REQUIRE( x_unit.rotate_around_unit(y_unit + z_unit, 2.0 * pi).distance(x_unit) < eps );
        REQUIRE( y_unit.rotate_around_unit(x_unit + z_unit, 2.0 * pi).distance(y_unit) < eps );
        REQUIRE( z_unit.rotate_around_unit(x_unit + y_unit, 2.0 * pi).distance(z_unit) < eps );

        // Rotation about an arbitrary axis reproduces simple unit vector decompositions.
        REQUIRE( (x_unit + y_unit + z_unit).rotate_around_unit(z_unit, 0.5 * pi).distance((x_unit * -1.0) + (y_unit *  1.0) + (z_unit *  1.0)) < eps );
        REQUIRE( (x_unit + y_unit + z_unit).rotate_around_unit(z_unit, 1.0 * pi).distance((x_unit * -1.0) + (y_unit * -1.0) + (z_unit *  1.0)) < eps );
        REQUIRE( (x_unit + y_unit + z_unit).rotate_around_unit(z_unit, 1.5 * pi).distance((x_unit *  1.0) + (y_unit * -1.0) + (z_unit *  1.0)) < eps );
        REQUIRE( (x_unit + y_unit + z_unit).rotate_around_unit(z_unit, 2.0 * pi).distance((x_unit *  1.0) + (y_unit *  1.0) + (z_unit *  1.0)) < eps );

        REQUIRE( (x_unit + y_unit + z_unit).rotate_around_unit(y_unit, 0.5 * pi).distance((x_unit *  1.0) + (y_unit *  1.0) + (z_unit * -1.0)) < eps );
        REQUIRE( (x_unit + y_unit + z_unit).rotate_around_unit(y_unit, 1.0 * pi).distance((x_unit * -1.0) + (y_unit *  1.0) + (z_unit * -1.0)) < eps );
        REQUIRE( (x_unit + y_unit + z_unit).rotate_around_unit(y_unit, 1.5 * pi).distance((x_unit * -1.0) + (y_unit *  1.0) + (z_unit *  1.0)) < eps );
        REQUIRE( (x_unit + y_unit + z_unit).rotate_around_unit(y_unit, 2.0 * pi).distance((x_unit *  1.0) + (y_unit *  1.0) + (z_unit *  1.0)) < eps );

        REQUIRE( (x_unit + y_unit + z_unit).rotate_around_unit(x_unit, 0.5 * pi).distance((x_unit *  1.0) + (y_unit * -1.0) + (z_unit *  1.0)) < eps );
        REQUIRE( (x_unit + y_unit + z_unit).rotate_around_unit(x_unit, 1.0 * pi).distance((x_unit *  1.0) + (y_unit * -1.0) + (z_unit * -1.0)) < eps );
        REQUIRE( (x_unit + y_unit + z_unit).rotate_around_unit(x_unit, 1.5 * pi).distance((x_unit *  1.0) + (y_unit *  1.0) + (z_unit * -1.0)) < eps );
        REQUIRE( (x_unit + y_unit + z_unit).rotate_around_unit(x_unit, 2.0 * pi).distance((x_unit *  1.0) + (y_unit *  1.0) + (z_unit *  1.0)) < eps );

        // Negative rotations are supported.
        REQUIRE( x_unit.rotate_around_unit(y_unit + z_unit, -2.0 * pi).distance(x_unit) < eps );
        REQUIRE( y_unit.rotate_around_unit(x_unit + z_unit, -2.0 * pi).distance(y_unit) < eps );
        REQUIRE( z_unit.rotate_around_unit(x_unit + y_unit, -2.0 * pi).distance(z_unit) < eps );

        // Negative rotations can also be accomplished by negating the axis vector..
        REQUIRE( x_unit.rotate_around_unit((y_unit + z_unit) * -1.0, 2.0 * pi).distance(x_unit) < eps );
        REQUIRE( y_unit.rotate_around_unit((x_unit + z_unit) * -1.0, 2.0 * pi).distance(y_unit) < eps );
        REQUIRE( z_unit.rotate_around_unit((x_unit + y_unit) * -1.0, 2.0 * pi).distance(z_unit) < eps );

        // Maintains length when rotated.
        REQUIRE( (x_unit.rotate_around_unit(z_unit, 1.0).length() - 1.0) < eps );
        REQUIRE( (y_unit.rotate_around_unit(z_unit, 1.0).length() - 1.0) < eps );

        REQUIRE( (x_unit.rotate_around_unit(y_unit, 1.0).length() - 1.0) < eps );
        REQUIRE( (z_unit.rotate_around_unit(y_unit, 1.0).length() - 1.0) < eps );

        REQUIRE( (y_unit.rotate_around_unit(x_unit, 1.0).length() - 1.0) < eps );
        REQUIRE( (z_unit.rotate_around_unit(x_unit, 1.0).length() - 1.0) < eps );

        REQUIRE( ((x_unit * -20.0).rotate_around_unit(z_unit, 1.0).length() - 20.0) < eps );
        REQUIRE( ((y_unit * -20.0).rotate_around_unit(z_unit, 1.0).length() - 20.0) < eps );

        REQUIRE( ((x_unit * -20.0).rotate_around_unit(y_unit, 1.0).length() - 20.0) < eps );
        REQUIRE( ((z_unit * -20.0).rotate_around_unit(y_unit, 1.0).length() - 20.0) < eps );

        REQUIRE( ((y_unit * -20.0).rotate_around_unit(x_unit, 1.0).length() - 20.0) < eps );
        REQUIRE( ((z_unit * -20.0).rotate_around_unit(x_unit, 1.0).length() - 20.0) < eps );

        // Maintains length when the axis is not a unit vector.
        REQUIRE( (x_unit.rotate_around_unit(y_unit + z_unit, 1.0).length() - 1.0) < eps );
        REQUIRE( (y_unit.rotate_around_unit(x_unit + z_unit, 1.0).length() - 1.0) < eps );
        REQUIRE( (z_unit.rotate_around_unit(x_unit + y_unit, 1.0).length() - 1.0) < eps );
    }

    SUBCASE("GramSchmidt_orthogonalize"){
        bool OK;
        vec3<double> A, B, C;
        
        // Orthonormal vectors remain orthonormal.
        A = x_unit;
        B = y_unit;
        C = z_unit;
        OK = A.GramSchmidt_orthogonalize(B, C);
        REQUIRE(OK);
        REQUIRE( A.distance(x_unit) < eps );
        REQUIRE( B.distance(y_unit) < eps );
        REQUIRE( C.distance(z_unit) < eps );

        A = x_unit * -1.0;
        B = y_unit *  1.0;
        C = z_unit * -1.0;
        OK = A.GramSchmidt_orthogonalize(C, B);
        REQUIRE(OK);
        REQUIRE( A.distance(x_unit * -1.0) < eps );
        REQUIRE( B.distance(y_unit *  1.0) < eps );
        REQUIRE( C.distance(z_unit * -1.0) < eps );
        
        A = x_unit;
        B = y_unit;
        C = z_unit;
        OK = B.GramSchmidt_orthogonalize(A, C);
        REQUIRE(OK);
        REQUIRE( A.distance(x_unit) < eps );
        REQUIRE( B.distance(y_unit) < eps );
        REQUIRE( C.distance(z_unit) < eps );

        A = x_unit;
        B = y_unit;
        C = z_unit;
        OK = B.GramSchmidt_orthogonalize(C, A);
        REQUIRE(OK);
        REQUIRE( A.distance(x_unit) < eps );
        REQUIRE( B.distance(y_unit) < eps );
        REQUIRE( C.distance(z_unit) < eps );

        
        A = x_unit;
        B = y_unit;
        C = z_unit;
        OK = C.GramSchmidt_orthogonalize(A, B);
        REQUIRE(OK);
        REQUIRE( A.distance(x_unit) < eps );
        REQUIRE( B.distance(y_unit) < eps );
        REQUIRE( C.distance(z_unit) < eps );

        A = x_unit;
        B = y_unit;
        C = z_unit;
        OK = C.GramSchmidt_orthogonalize(B, A);
        REQUIRE(OK);
        REQUIRE( A.distance(x_unit) < eps );
        REQUIRE( B.distance(y_unit) < eps );
        REQUIRE( C.distance(z_unit) < eps );

        // Vector lengths are maintained, i.e., orthonormalization is *not* performed.
        A = x_unit *   2.0;
        B = y_unit * -10.0;
        C = z_unit *  50.0;
        OK = C.GramSchmidt_orthogonalize(B, A);
        REQUIRE(OK);
        REQUIRE( A.distance(x_unit *   2.0) < eps );
        REQUIRE( B.distance(y_unit * -10.0) < eps );
        REQUIRE( C.distance(z_unit *  50.0) < eps );

        A = x_unit *   2.0E10;
        B = y_unit * -10.0E-5;
        C = z_unit *  50.0;
        OK = C.GramSchmidt_orthogonalize(B, A);
        REQUIRE(OK);
        REQUIRE( A.distance(x_unit *   2.0E10) < eps );
        REQUIRE( B.distance(y_unit * -10.0E-5) < eps );
        REQUIRE( C.distance(z_unit *  50.0   ) < eps );

        // Inputs that do not span the basis will fail to orthogonalize.
        A = x_unit;
        B = x_unit;
        C = z_unit;
        OK = A.GramSchmidt_orthogonalize(B, C);
        REQUIRE(!OK);

        A = y_unit;
        B = y_unit;
        C = z_unit;
        OK = A.GramSchmidt_orthogonalize(B, C);
        REQUIRE(!OK);

        A = x_unit;
        B = x_unit;
        C = x_unit;
        OK = A.GramSchmidt_orthogonalize(B, C);
        REQUIRE(!OK);

        A = y_unit;
        B = y_unit;
        C = x_unit;
        OK = A.GramSchmidt_orthogonalize(B, C);
        REQUIRE(!OK);

        A = vec3<double>().zero();
        B = y_unit;
        C = z_unit;
        OK = A.GramSchmidt_orthogonalize(B, C);
        REQUIRE(!OK);

        A = x_unit;
        B = vec3<double>().zero();
        C = z_unit;
        OK = A.GramSchmidt_orthogonalize(B, C);
        REQUIRE(!OK);

        A = x_unit;
        B = y_unit;
        C = vec3<double>().zero();
        OK = A.GramSchmidt_orthogonalize(B, C);
        REQUIRE(!OK);

        // Non-finite inputs cannot reasonably be orthogonalized, in general, so should be rejected.
        A = x_unit * inf;
        B = x_unit;
        C = z_unit;
        OK = A.GramSchmidt_orthogonalize(B, C);
        REQUIRE(!OK);

        A = x_unit;
        B = x_unit * inf;
        C = z_unit;
        OK = A.GramSchmidt_orthogonalize(B, C);
        REQUIRE(!OK);

        A = x_unit;
        B = x_unit;
        C = z_unit * inf;
        OK = A.GramSchmidt_orthogonalize(B, C);
        REQUIRE(!OK);

        A = x_unit * nan;
        B = x_unit;
        C = z_unit;
        OK = A.GramSchmidt_orthogonalize(B, C);
        REQUIRE(!OK);

        A = x_unit;
        B = x_unit * nan;
        C = z_unit;
        OK = A.GramSchmidt_orthogonalize(B, C);
        REQUIRE(!OK);

        A = x_unit;
        B = x_unit;
        C = z_unit * nan;
        OK = A.GramSchmidt_orthogonalize(B, C);
        REQUIRE(!OK);
    }

    SUBCASE("to_num_array"){
        auto A = x_unit.to_num_array().to_vec3();
        auto B = y_unit.to_num_array().to_vec3();
        auto C = z_unit.to_num_array().to_vec3();

        REQUIRE(A == x_unit);
        REQUIRE(B == y_unit);
        REQUIRE(C == z_unit);

        const auto D = (x_unit * 1.0 + y_unit * 2.0 - z_unit * 3.0);
        const auto E = D.to_num_array().to_vec3();
        REQUIRE(D == E);

        const auto F = vec3<double>(1.0, eps, inf);
        const auto G = F.to_num_array().to_vec3();
        REQUIRE(F == G);
    }

    SUBCASE("to_homogeneous_num_array"){
        auto A = x_unit.to_homogeneous_num_array().hnormalize_to_vec3();
        auto B = y_unit.to_homogeneous_num_array().hnormalize_to_vec3();
        auto C = z_unit.to_homogeneous_num_array().hnormalize_to_vec3();

        REQUIRE(A == x_unit);
        REQUIRE(B == y_unit);
        REQUIRE(C == z_unit);

        const auto D = (x_unit * 1.0 + y_unit * 2.0 - z_unit * 3.0);
        const auto E = D.to_homogeneous_num_array().hnormalize_to_vec3();
        REQUIRE(D == E);

        const auto F = vec3<double>(1.0, eps, inf);
        const auto G = F.to_homogeneous_num_array().hnormalize_to_vec3();
        REQUIRE(F == G);
    }

    SUBCASE("to_string and from_string"){
        vec3<double> A;

        REQUIRE( A.from_string( x_unit.to_string() ).distance(x_unit) < eps );
        REQUIRE( A.from_string( y_unit.to_string() ).distance(y_unit) < eps );
        REQUIRE( A.from_string( z_unit.to_string() ).distance(z_unit) < eps );

        const vec3<double> B( std::numeric_limits<double>::min(),
                              std::numeric_limits<double>::denorm_min(),
                              1.0E308 ); 
        A.from_string( B.to_string() );
        REQUIRE( A.x == B.x );
        REQUIRE( A.y == B.y );
        REQUIRE( A.z == B.z );

        // Values expected to be rounded should be exactly reproduced.
        const vec3<double> C( 1.234567E-307,
                              std::nexttoward( 0.0,  1.0),
                              std::nexttoward( 0.0, -1.0) );
        A.from_string( C.to_string() );
        REQUIRE( A.x == C.x );
        REQUIRE( A.y == C.y );
        REQUIRE( A.z == C.z );

        // Non-finites should be correctly parsed by the locale.
        const vec3<double> D(  nan,
                               inf,
                              -inf );
        A.from_string( D.to_string() );
        REQUIRE( std::isnan(D.x));
        REQUIRE( D.y ==  inf );
        REQUIRE( D.z == -inf );

        // Extreme values should round-trip exactly, without any loss of precision.
        const vec3<double> E( std::numeric_limits<double>::max(),
                              std::numeric_limits<double>::lowest(),
                              1.1234567890123456789 );
        A.from_string( E.to_string() );

        REQUIRE( A.x == E.x );
        REQUIRE( A.y == E.y );
        REQUIRE( A.z == E.z );

        // Preceding whitespace should be ignored.
        A.from_string( std::string("    ") + E.to_string() );
        REQUIRE( A.x == E.x );
        REQUIRE( A.y == E.y );
        REQUIRE( A.z == E.z );

        // Invalid values should throw.
        REQUIRE_THROWS( A.from_string( "(1.0,2.0)" ) );
        //REQUIRE_THROWS( A.from_string( "(1.0,2.0,3.0,1.0)" ) );
        //REQUIRE_THROWS( A.from_string( "(1.0,2.0,3.0),1.0" ) );
        REQUIRE_THROWS( A.from_string( "1.0,2.0,3.0)" ) );
        REQUIRE_THROWS( A.from_string( "(1.0,2.0,xyz)" ) );
        REQUIRE_THROWS( A.from_string( "(1.0 2.0 3.0)" ) );
    }

    SUBCASE("operator<< and operator>>"){
        vec3<double> A;

        const auto round_trip_vec3 = [&A](const vec3<double> &x) -> void {
            std::stringstream ss;
            ss << x;
            ss >> A;
            return;
        };
        const auto parse_from_string = [&A](const std::string &x) -> bool {
            std::stringstream ss;
            ss << x;
            ss >> A;
            return true;
        };

        round_trip_vec3(x_unit);
        REQUIRE( A.distance(x_unit) < eps );
        round_trip_vec3(y_unit);
        REQUIRE( A.distance(y_unit) < eps );
        round_trip_vec3(z_unit);
        REQUIRE( A.distance(z_unit) < eps );

        const vec3<double> B( std::numeric_limits<double>::min(),
                              std::numeric_limits<double>::denorm_min(),
                              1.0E308 ); 
        round_trip_vec3(B);
        REQUIRE( A.x == B.x );
        REQUIRE( A.y == B.y );
        REQUIRE( A.z == B.z );

        // Values expected to be rounded should be exactly reproduced.
        const vec3<double> C( 1.234567E-307,
                              std::nexttoward( 0.0,  1.0),
                              std::nexttoward( 0.0, -1.0) );
        round_trip_vec3(C);
        REQUIRE( A.x == C.x );
        REQUIRE( A.y == C.y );
        REQUIRE( A.z == C.z );

        // Non-finites should be correctly parsed by the locale.
        const vec3<double> D(  nan,
                               inf,
                              -inf );
        round_trip_vec3(D);
        REQUIRE( std::isnan(D.x));
        REQUIRE( D.y ==  inf );
        REQUIRE( D.z == -inf );

        // Extreme values should round-trip exactly, without any loss of precision.
        const vec3<double> E( std::numeric_limits<double>::max(),
                              std::numeric_limits<double>::lowest(),
                              1.1234567890123456789 );
        round_trip_vec3(E);
        REQUIRE( A.x == E.x );
        REQUIRE( A.y == E.y );
        REQUIRE( A.z == E.z );

        // Invalid values should throw.
        REQUIRE_THROWS( parse_from_string("(1.0,2.0)") );
        //REQUIRE_THROWS( parse_from_string("(1.0,2.0,3.0,1.0)") );
        //REQUIRE_THROWS( parse_from_string( "(1.0,2.0,3.0),1.0") );
        REQUIRE_THROWS( parse_from_string( "1.0,2.0,3.0)") );
        REQUIRE_THROWS( parse_from_string( "(1.0,2.0,xyz)") );
        REQUIRE_THROWS( parse_from_string( "(1.0 2.0 3.0)") );
    }
}

TEST_CASE( "vec3 affiliated free functions" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );
    const auto pi = std::acos(-1.0);

    const vec3<double> x_unit(1.0, 0.0, 0.0);
    const vec3<double> y_unit(0.0, 1.0, 0.0);
    const vec3<double> z_unit(0.0, 0.0, 1.0);
    const vec3<double> zeros(0.0, 0.0, 0.0);

    SUBCASE("rotate_unit_vector_in_plane"){
        // Some simple null rotation cases.
        REQUIRE( rotate_unit_vector_in_plane(x_unit, 0.0*pi, 0.0*pi).distance(x_unit) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit, 0.0*pi, 0.0*pi).distance(y_unit) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit, 0.0*pi, 0.0*pi).distance(z_unit) < eps );

        REQUIRE( rotate_unit_vector_in_plane(x_unit, 2.0*pi, 0.0*pi).distance(x_unit) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit, 2.0*pi, 0.0*pi).distance(y_unit) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit, 2.0*pi, 0.0*pi).distance(z_unit) < eps );

        REQUIRE( rotate_unit_vector_in_plane(x_unit, 0.0*pi, 1.0*pi).distance(x_unit) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit, 0.0*pi, 1.0*pi).distance(y_unit) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit, 0.0*pi, 1.0*pi).distance(z_unit) < eps );

        REQUIRE( rotate_unit_vector_in_plane(x_unit, 2.0*pi, 1.0*pi).distance(x_unit) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit, 2.0*pi, 1.0*pi).distance(y_unit) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit, 2.0*pi, 1.0*pi).distance(z_unit) < eps );

        // Around x axis.
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  0.0*pi,  0.0*pi ).distance(x_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  0.5*pi,  0.0*pi ).distance(z_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  1.0*pi,  0.0*pi ).distance(x_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  1.5*pi,  0.0*pi ).distance(z_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  2.0*pi,  0.0*pi ).distance(x_unit *  1.0) < eps );

        REQUIRE( rotate_unit_vector_in_plane(x_unit,  0.0*pi,  0.5*pi ).distance(x_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  0.5*pi,  0.5*pi ).distance(y_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  1.0*pi,  0.5*pi ).distance(x_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  1.5*pi,  0.5*pi ).distance(y_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  2.0*pi,  0.5*pi ).distance(x_unit *  1.0) < eps );

        REQUIRE( rotate_unit_vector_in_plane(x_unit,  0.0*pi,  1.0*pi ).distance(x_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  0.5*pi,  1.0*pi ).distance(z_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  1.0*pi,  1.0*pi ).distance(x_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  1.5*pi,  1.0*pi ).distance(z_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  2.0*pi,  1.0*pi ).distance(x_unit *  1.0) < eps );

        REQUIRE( rotate_unit_vector_in_plane(x_unit,  0.0*pi,  1.5*pi ).distance(x_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  0.5*pi,  1.5*pi ).distance(y_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  1.0*pi,  1.5*pi ).distance(x_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  1.5*pi,  1.5*pi ).distance(y_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(x_unit,  2.0*pi,  1.5*pi ).distance(x_unit *  1.0) < eps );

        // Around y axis.
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  0.0*pi,  0.0*pi ).distance(y_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  0.5*pi,  0.0*pi ).distance(z_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  1.0*pi,  0.0*pi ).distance(y_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  1.5*pi,  0.0*pi ).distance(z_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  2.0*pi,  0.0*pi ).distance(y_unit *  1.0) < eps );

        REQUIRE( rotate_unit_vector_in_plane(y_unit,  0.0*pi,  0.5*pi ).distance(y_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  0.5*pi,  0.5*pi ).distance(x_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  1.0*pi,  0.5*pi ).distance(y_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  1.5*pi,  0.5*pi ).distance(x_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  2.0*pi,  0.5*pi ).distance(y_unit *  1.0) < eps );

        REQUIRE( rotate_unit_vector_in_plane(y_unit,  0.0*pi,  1.0*pi ).distance(y_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  0.5*pi,  1.0*pi ).distance(z_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  1.0*pi,  1.0*pi ).distance(y_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  1.5*pi,  1.0*pi ).distance(z_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  2.0*pi,  1.0*pi ).distance(y_unit *  1.0) < eps );

        REQUIRE( rotate_unit_vector_in_plane(y_unit,  0.0*pi,  1.5*pi ).distance(y_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  0.5*pi,  1.5*pi ).distance(x_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  1.0*pi,  1.5*pi ).distance(y_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  1.5*pi,  1.5*pi ).distance(x_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(y_unit,  2.0*pi,  1.5*pi ).distance(y_unit *  1.0) < eps );

        // Around z axis.
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  0.0*pi,  0.0*pi ).distance(z_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  0.5*pi,  0.0*pi ).distance(x_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  1.0*pi,  0.0*pi ).distance(z_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  1.5*pi,  0.0*pi ).distance(x_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  2.0*pi,  0.0*pi ).distance(z_unit *  1.0) < eps );

        REQUIRE( rotate_unit_vector_in_plane(z_unit,  0.0*pi,  0.5*pi ).distance(z_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  0.5*pi,  0.5*pi ).distance(y_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  1.0*pi,  0.5*pi ).distance(z_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  1.5*pi,  0.5*pi ).distance(y_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  2.0*pi,  0.5*pi ).distance(z_unit *  1.0) < eps );

        REQUIRE( rotate_unit_vector_in_plane(z_unit,  0.0*pi,  1.0*pi ).distance(z_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  0.5*pi,  1.0*pi ).distance(x_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  1.0*pi,  1.0*pi ).distance(z_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  1.5*pi,  1.0*pi ).distance(x_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  2.0*pi,  1.0*pi ).distance(z_unit *  1.0) < eps );

        REQUIRE( rotate_unit_vector_in_plane(z_unit,  0.0*pi,  1.5*pi ).distance(z_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  0.5*pi,  1.5*pi ).distance(y_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  1.0*pi,  1.5*pi ).distance(z_unit * -1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  1.5*pi,  1.5*pi ).distance(y_unit *  1.0) < eps );
        REQUIRE( rotate_unit_vector_in_plane(z_unit,  2.0*pi,  1.5*pi ).distance(z_unit *  1.0) < eps );
    }

    SUBCASE("Evolve_x_v_over_T_via_F"){
        vec3<double> x_0(0.0, 0.0, 0.0);
        vec3<double> v_0(0.0, 0.0, 0.0);
        std::tuple<vec3<double>,vec3<double>> x_and_v_initial{x_0, v_0};

        // Null evolution.
        const auto Fnull = [](vec3<double>, double) -> vec3<double> {
            return vec3<double>(0.0, 0.0, 0.0);
        };

        const auto x_and_v_final_Fnull = Evolve_x_v_over_T_via_F(x_and_v_initial, Fnull, 1.0, 10'000);

        REQUIRE( std::get<0>(x_and_v_final_Fnull).x == 0.0 );
        REQUIRE( std::get<0>(x_and_v_final_Fnull).y == 0.0 );
        REQUIRE( std::get<0>(x_and_v_final_Fnull).z == 0.0 );

        REQUIRE( std::get<1>(x_and_v_final_Fnull).x == 0.0 );
        REQUIRE( std::get<1>(x_and_v_final_Fnull).y == 0.0 );
        REQUIRE( std::get<1>(x_and_v_final_Fnull).z == 0.0 );

        // Evolution under x only.
        const auto Fx = [](vec3<double>, double) -> vec3<double> {
            return vec3<double>(1.0, 0.0, 0.0);
        };

        const auto x_and_v_final_Fx = Evolve_x_v_over_T_via_F(x_and_v_initial, Fx, 1.0, 10'000);

        REQUIRE( std::abs( std::get<0>(x_and_v_final_Fx).x - 0.5 ) < 1E-3 );
        REQUIRE( std::get<0>(x_and_v_final_Fx).y == 0.0 );
        REQUIRE( std::get<0>(x_and_v_final_Fx).z == 0.0 );

        REQUIRE( std::abs( std::get<1>(x_and_v_final_Fx).x - 1.0 ) < 1E-3 );
        REQUIRE( std::get<1>(x_and_v_final_Fx).y == 0.0 );
        REQUIRE( std::get<1>(x_and_v_final_Fx).z == 0.0 );

        // Evolution under y only.
        const auto Fy = [](vec3<double>, double) -> vec3<double> {
            return vec3<double>(0.0, 1.0, 0.0);
        };

        const auto x_and_v_final_Fy = Evolve_x_v_over_T_via_F(x_and_v_initial, Fy, 1.0, 10'000);

        REQUIRE( std::get<0>(x_and_v_final_Fy).x == 0.0 );
        REQUIRE( std::abs( std::get<0>(x_and_v_final_Fy).y - 0.5 ) < 1E-3 );
        REQUIRE( std::get<0>(x_and_v_final_Fy).z == 0.0 );

        REQUIRE( std::get<1>(x_and_v_final_Fy).x == 0.0 );
        REQUIRE( std::abs( std::get<1>(x_and_v_final_Fy).y - 1.0 ) < 1E-3 );
        REQUIRE( std::get<1>(x_and_v_final_Fy).z == 0.0 );

        // Evolution under z only.
        const auto Fz = [](vec3<double>, double) -> vec3<double> {
            return vec3<double>(0.0, 0.0, 1.0);
        };

        const auto x_and_v_final_Fz = Evolve_x_v_over_T_via_F(x_and_v_initial, Fz, 1.0, 10'000);

        REQUIRE( std::get<0>(x_and_v_final_Fz).x == 0.0 );
        REQUIRE( std::get<0>(x_and_v_final_Fz).y == 0.0 );
        REQUIRE( std::abs( std::get<0>(x_and_v_final_Fz).z - 0.5 ) < 1E-3 );

        REQUIRE( std::get<1>(x_and_v_final_Fz).x == 0.0 );
        REQUIRE( std::get<1>(x_and_v_final_Fz).y == 0.0 );
        REQUIRE( std::abs( std::get<1>(x_and_v_final_Fz).z - 1.0 ) < 1E-3 );

        // Evolution under x, y, and z.
        const auto Fxyz = [](vec3<double>, double) -> vec3<double> {
            return vec3<double>(1.0, 2.0, 3.0);
        };

        const auto x_and_v_final_Fxyz = Evolve_x_v_over_T_via_F(x_and_v_initial, Fxyz, 1.0, 10'000);

        REQUIRE( std::abs( std::get<0>(x_and_v_final_Fxyz).x - 0.5 ) < 1E-3 );
        REQUIRE( std::abs( std::get<0>(x_and_v_final_Fxyz).y - 1.0 ) < 1E-3 );
        REQUIRE( std::abs( std::get<0>(x_and_v_final_Fxyz).z - 1.5 ) < 1E-3 );

        REQUIRE( std::abs( std::get<1>(x_and_v_final_Fxyz).x - 1.0 ) < 1E-4 );
        REQUIRE( std::abs( std::get<1>(x_and_v_final_Fxyz).y - 2.0 ) < 1E-4 );
        REQUIRE( std::abs( std::get<1>(x_and_v_final_Fxyz).z - 3.0 ) < 1E-4 );

        // Invalid arguments are rejected.
        std::function<vec3<double>(vec3<double> x, double T)> Finvalid;
        REQUIRE_THROWS( Evolve_x_v_over_T_via_F(x_and_v_initial, Fxyz, 1.0,  0) );
        REQUIRE_THROWS( Evolve_x_v_over_T_via_F(x_and_v_initial, Fxyz, 1.0, -1) );
        REQUIRE_THROWS( Evolve_x_v_over_T_via_F(x_and_v_initial, Finvalid, 1.0, 100) );
    }
}

// Note: The following function is a link-time check. At runtime the code unconditionally passes.
//       The optimization level is overridden to avoid the code being optimized away.
//       Use of this pragma (at time of writing) cannot appear inside a function.
#pragma GCC push_options
#pragma GCC optimize("O0")
static bool instantiate_vec3_types(){
    vec3<float   > v0;
    vec3<double  > v1;
    vec3<uint8_t > v2;
    vec3<uint16_t> v3;
    vec3<uint32_t> v4;
    vec3<uint64_t> v5;
    vec3<int8_t  > v6;
    vec3<int16_t > v7;
    vec3<int32_t > v8;
    vec3<int64_t > v9;
    return true;
}
#pragma GCC pop_options

TEST_CASE( "vec3 with integer type template specializations are available" ){
    REQUIRE(instantiate_vec3_types());
}

