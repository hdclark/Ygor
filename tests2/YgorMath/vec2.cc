
#include <limits>
#include <utility>
#include <iostream>

#include <YgorMath.h>

#include "doctest/doctest.h"


TEST_CASE( "vec2 constructors" ){
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

    SUBCASE("copy constructor"){
        vec2<double> v(-1.0, 0.0);
        vec2<double> w(v);
        REQUIRE( v.x == w.x );
        REQUIRE( v.y == w.y );
    }

    SUBCASE("std::array constructor"){
        std::array<double,2> w{{ -1.0, 1.0 }};
        vec2<double> v(w);
        REQUIRE( v.x == -1.0 );
        REQUIRE( v.y == 1.0 );
    }
}


TEST_CASE( "vec2 operators" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();

    vec2<double> A(-1.0, 0.0);
    SUBCASE("operator== and operator!="){
        SUBCASE("finite"){
            vec2<double> B(-1.0, 0.0);
            vec2<double> C( 0.0, 0.0);
            REQUIRE( A == B );
            REQUIRE( A != C );
        }
        SUBCASE("nonfinite"){
            vec2<double> B(-1.0, nan);
            vec2<double> C( nan, nan);
            REQUIRE( A != B );
            REQUIRE( A != C );
            REQUIRE( B != B );
            REQUIRE( B != C );
            REQUIRE( C != C );
        }
    }

    SUBCASE("operator="){
        vec2<double> B = A;
        REQUIRE( A.x == B.x );
        REQUIRE( A.y == B.y );
    }

    SUBCASE("operator= with an implicit conversion via std::array"){
        vec2<double> v = {{ -1.0, 1.0 }};
        REQUIRE( v.x == -1.0 );
        REQUIRE( v.y == 1.0 );
    }

    SUBCASE("operator+"){
        vec2<double> B = A + A + vec2<double>(10.0, 20.0);
        REQUIRE( B.x == 8.0 );
        REQUIRE( B.y == 20.0 );

        B = B + vec2<double>(10.0, 20.0);
        REQUIRE( B.x == 18.0 );
        REQUIRE( B.y == 40.0 );
    }

    SUBCASE("operator+="){
        vec2<double> B(10.0, 20.0);
        B += A;
        REQUIRE( B.x == 9.0 );
        REQUIRE( B.y == 20.0 );

        B += B;
        REQUIRE( B.x == 18.0 );
        REQUIRE( B.y == 40.0 );

        B += B + B;
        REQUIRE( B.x ==  54.0 );
        REQUIRE( B.y == 120.0 );
    }

    SUBCASE("operator-"){
        vec2<double> B = A - A - vec2<double>(10.0, 20.0);
        REQUIRE( B.x == -10.0 );
        REQUIRE( B.y == -20.0 );

        B = B - vec2<double>(1.0, 2.0);
        REQUIRE( B.x == -11.0 );
        REQUIRE( B.y == -22.0 );
    }

    SUBCASE("operator-="){
        vec2<double> B(10.0, 20.0);
        B -= A;
        REQUIRE( B.x == 11.0 );
        REQUIRE( B.y == 20.0 );

        B -= B;
        REQUIRE( B.x == 0.0 );
        REQUIRE( B.y == 0.0 );

        B -= A - B - B;
        REQUIRE( A == B * -1.0 );
    }

    SUBCASE("operator<"){
        vec2<double> B = A;
        vec2<double> C(A.x - 1.0, A.y);
        vec2<double> D(A.x, A.y - 1.0);

        REQUIRE( !(B < A) );

        REQUIRE( C < A );
        REQUIRE( !(A < C) );

        REQUIRE( D < A );
        REQUIRE( !(A < D) );
    }

    A = vec2<double>(40.0, 80.0);
    SUBCASE("operator* and operator*= with a scalar"){
        vec2<double> B = A * -2.0;

        REQUIRE( B.x == A.x * -2.0 );
        REQUIRE( B.y == A.y * -2.0 );

        B *= -2.0;

        REQUIRE( B.x == A.x * -2.0 * -2.0 );
        REQUIRE( B.y == A.y * -2.0 * -2.0 );
    }

    SUBCASE("operator/ and operator/= with a scalar"){
        vec2<double> B = A / -2.0;

        REQUIRE( B.x == A.x / -2.0 );
        REQUIRE( B.y == A.y / -2.0 );

        B /= -2.0;

        REQUIRE( B.x == A.x / -2.0 / -2.0 );
        REQUIRE( B.y == A.y / -2.0 / -2.0 );
    }

    SUBCASE("operator[]"){
        A.x = 1.0;
        A.y = 0.0;
        REQUIRE( A[0] == A.x );
        REQUIRE( A[1] == A.y );
    }

    SUBCASE("cast operator to std::array"){
        SUBCASE("explicit casts"){
            vec2<double> v(-1.0, 1.0);
            auto w = static_cast< std::array<double,2> >(v);
            REQUIRE( v.x == -1.0 );
            REQUIRE( v.y == 1.0 );

            REQUIRE( v.x == w.at(0) );
            REQUIRE( v.y == w.at(1) );
        }
        SUBCASE("implicit casts"){
            vec2<double> v(-1.0, 1.0);
            std::array<double,2> w{{ -10.0, 11.0}};

            std::array<double,2> z = v + w;
            REQUIRE( z.at(0) == (v.x + w.at(0)) );
            REQUIRE( z.at(1) == (v.y + w.at(1)) );
        }
    }
}


TEST_CASE( "vec2 member functions" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    const vec2<double> x_unit(1.0, 0.0);
    const vec2<double> y_unit(0.0, 1.0);
    const vec2<double> zero(0.0, 0.0);

    SUBCASE("isfinite with NaNs"){
        const vec2<double> A( 1.0, 1.0 );
        const vec2<double> B( nan, 1.0 );
        const vec2<double> C( nan, nan );
        const vec2<double> D( 1.0, nan );

        REQUIRE(  A.isfinite() );
        REQUIRE( !B.isfinite() );
        REQUIRE( !C.isfinite() );
        REQUIRE( !D.isfinite() );
    }
    SUBCASE("isfinite with infs"){
        const vec2<double> A( 1.0, 1.0 );
        const vec2<double> B( inf, 1.0 );
        const vec2<double> C( inf, inf );
        const vec2<double> D( 1.0, inf );

        REQUIRE(  A.isfinite() );
        REQUIRE( !B.isfinite() );
        REQUIRE( !C.isfinite() );
        REQUIRE( !D.isfinite() );
    }

    SUBCASE("dot product"){
        REQUIRE(  x_unit.Dot(x_unit) == 1.0 );
        REQUIRE(  y_unit.Dot(y_unit) == 1.0 );

        REQUIRE(  x_unit.Dot(x_unit * -1.0) == -1.0 );
        REQUIRE(  y_unit.Dot(y_unit * -1.0) == -1.0 );

        REQUIRE(  x_unit.Dot(y_unit) == 0.0 );

        REQUIRE(  x_unit.Dot(y_unit * -1.0) == 0.0 );
    }

    SUBCASE("mask product"){
        const vec2<double> A(10.0, 20.0);
        const vec2<double> B(2.0, 3.0);
        const auto C = A.Mask(B);
        REQUIRE(  C.x == (A.x * B.x) );
        REQUIRE(  C.y == (A.y * B.y) );
    }

    SUBCASE("unit"){
        REQUIRE( x_unit.unit() == x_unit );
        REQUIRE( y_unit.unit() == y_unit );

        const auto A = vec2<double>(10.0, -20.0).unit();
        const auto A_length = std::sqrt(A.x * A.x + A.y * A.y);
        REQUIRE( (A_length - 1.0) < eps );

        const auto B = zero.unit();
        REQUIRE( !std::isfinite(B.x) );
        REQUIRE( !std::isfinite(B.y) );
    }

    SUBCASE("length"){
        REQUIRE( x_unit.length() == 1.0 );
        REQUIRE( y_unit.length() == 1.0 );

        REQUIRE( zero.length() == 0.0 );

        const auto A = vec2<double>(10.0, -20.0);
        REQUIRE( std::abs(A.length() - std::sqrt(500.0)) < eps );
    }

    SUBCASE("sq_length"){
        REQUIRE( x_unit.sq_length() == 1.0 );
        REQUIRE( y_unit.sq_length() == 1.0 );

        REQUIRE( zero.sq_length() == 0.0 );

        const auto A = vec2<double>(10.0, -20.0);
        REQUIRE( std::abs(A.sq_length() - 500.0) < eps );
    }

    SUBCASE("distance"){
        REQUIRE( zero.distance(zero) == 0.0 );

        REQUIRE( x_unit.distance(x_unit) == 0.0 );
        REQUIRE( y_unit.distance(y_unit) == 0.0 );

        REQUIRE( x_unit.distance(zero) == 1.0 );
        REQUIRE( y_unit.distance(zero) == 1.0 );

        REQUIRE( std::abs(x_unit.distance(y_unit) - std::sqrt(2.0)) < eps );

        REQUIRE( std::abs(x_unit.distance(y_unit * -1.0) - std::sqrt(2.0)) < eps );
    }

    SUBCASE("sq_dist"){
        REQUIRE( zero.sq_dist(zero) == 0.0 );

        REQUIRE( x_unit.sq_dist(x_unit) == 0.0 );
        REQUIRE( y_unit.sq_dist(y_unit) == 0.0 );

        REQUIRE( x_unit.sq_dist(zero) == 1.0 );
        REQUIRE( y_unit.sq_dist(zero) == 1.0 );

        REQUIRE( std::abs(x_unit.sq_dist(y_unit) - 2.0) < eps );

        REQUIRE( std::abs(x_unit.sq_dist(y_unit * -1.0) - 2.0) < eps );
    }

    SUBCASE("zero"){
        REQUIRE( zero == vec2<double>().zero() );
        REQUIRE( zero == x_unit.zero() );
        REQUIRE( zero != x_unit );
    }

    SUBCASE("rotate_around_x, rotate_around_y, rotate_around_z"){
        const double agreement = 1.0 - eps;

        // Correct orientations.
        REQUIRE( agreement < x_unit.rotate_around_z(2.0 * M_PI).Dot(x_unit) );
        REQUIRE( agreement < x_unit.rotate_around_z(8.0 * M_PI).Dot(x_unit) );
        REQUIRE( agreement < x_unit.rotate_around_z(0.5 * M_PI).Dot(y_unit) );
        REQUIRE( agreement < y_unit.rotate_around_z(1.5 * M_PI).Dot(x_unit) );

        // Maintains length when rotated.
        REQUIRE( (x_unit.rotate_around_z(1.0).length() - 1.0) < eps );
        REQUIRE( (y_unit.rotate_around_z(1.0).length() - 1.0) < eps );

        REQUIRE( ((x_unit * -20.0).rotate_around_z(1.0).length() - 20.0) < eps );
        REQUIRE( ((y_unit * -20.0).rotate_around_z(1.0).length() - 20.0) < eps );
    }

    SUBCASE("to_string and from_string"){
        vec2<double> A;

        REQUIRE( A.from_string( x_unit.to_string() ).distance(x_unit) < eps );
        REQUIRE( A.from_string( y_unit.to_string() ).distance(y_unit) < eps );

        // Precision should be maintained for small numbers.
        const vec2<double> B( std::numeric_limits<double>::min(),
                              std::numeric_limits<double>::denorm_min() );
        A.from_string( B.to_string() );
        REQUIRE( A.x == B.x );
        REQUIRE( A.y == B.y );

        const vec2<double> C( -1.234567E-307,
                               1.234567E-307 );
        A.from_string( C.to_string() );
        REQUIRE( A.x == C.x );
        REQUIRE( A.y == C.y );

        // Precision should be maintained for 'large' numbers.
        const vec2<double> D( -1.0E308,
                               1.0E308 );
        A.from_string( D.to_string() );
        REQUIRE( A.x == D.x );
        REQUIRE( A.y == D.y );

        // Values expected to be rounded should be exactly reproduced.
        const vec2<double> E( std::nexttoward( 0.0,  1.0),
                              std::nexttoward( 0.0, -1.0) );
        A.from_string( E.to_string() );
        REQUIRE( A.x == E.x );
        REQUIRE( A.y == E.y );

        // Non-finites should be correctly parsed by the locale.
        const vec2<double> F(  nan,
                               inf );
        A.from_string( F.to_string() );
        REQUIRE( std::isnan(F.x));
        REQUIRE( F.y == inf );

        const vec2<double> G( -inf,
                               nan );
        A.from_string( G.to_string() );
        REQUIRE( G.x == -inf );
        REQUIRE( std::isnan(G.y));

        // Extreme values should round-trip exactly, without any loss of precision.
        const vec2<double> H( std::numeric_limits<double>::max(),
                              std::numeric_limits<double>::lowest() );
        A.from_string( H.to_string() );
        REQUIRE( A.x == H.x );
        REQUIRE( A.y == H.y );

        const vec2<double> I( 2.3456789012345678901,
                              1.1234567890123456789 );
        A.from_string( I.to_string() );
        REQUIRE( A.x == I.x );
        REQUIRE( A.y == I.y );

        // Invalid values should throw.
        REQUIRE_THROWS( A.from_string( "(1.0)" ) );
        //REQUIRE_THROWS( A.from_string( "(1.0,2.0,1.0)" ) );
        REQUIRE_THROWS( A.from_string( "(1.0,2.0),1.0" ) );
        REQUIRE_THROWS( A.from_string( "1.0,2.0)" ) );
        REQUIRE_THROWS( A.from_string( "(1.0,xyz)" ) );
        REQUIRE_THROWS( A.from_string( "(1.0 2.0)" ) );
    }

    SUBCASE("operator<< and operator>>"){
        vec2<double> A;

        const auto round_trip_vec2 = [&A](const vec2<double> &x) -> void {
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

        round_trip_vec2(x_unit);
        REQUIRE( A.distance(x_unit) < eps );
        round_trip_vec2(y_unit);
        REQUIRE( A.distance(y_unit) < eps );

        // Precision should be maintained for small numbers.
        const vec2<double> B( std::numeric_limits<double>::min(),
                              std::numeric_limits<double>::denorm_min() );
        round_trip_vec2(B);
        REQUIRE( A.x == B.x );
        REQUIRE( A.y == B.y );

        const vec2<double> C( -1.234567E-307,
                               1.234567E-307 );
        round_trip_vec2(C);
        REQUIRE( A.x == C.x );
        REQUIRE( A.y == C.y );

        // Precision should be maintained for 'large' numbers.
        const vec2<double> D( -1.0E308,
                               1.0E308 );
        round_trip_vec2(D);
        REQUIRE( A.x == D.x );
        REQUIRE( A.y == D.y );

        // Values expected to be rounded should be exactly reproduced.
        const vec2<double> E( std::nexttoward( 0.0,  1.0),
                              std::nexttoward( 0.0, -1.0) );
        round_trip_vec2(E);
        REQUIRE( A.x == E.x );
        REQUIRE( A.y == E.y );

        // Non-finites should be correctly parsed by the locale.
        const vec2<double> F(  nan,
                               inf );
        round_trip_vec2(F);
        REQUIRE( std::isnan(F.x));
        REQUIRE( F.y == inf );

        const vec2<double> G( -inf,
                               nan );
        round_trip_vec2(G);
        REQUIRE( G.x == -inf );
        REQUIRE( std::isnan(G.y));

        // Extreme values should round-trip exactly, without any loss of precision.
        const vec2<double> H( std::numeric_limits<double>::max(),
                              std::numeric_limits<double>::lowest() );
        round_trip_vec2(H);
        REQUIRE( A.x == H.x );
        REQUIRE( A.y == H.y );

        const vec2<double> I( 2.3456789012345678901,
                              1.1234567890123456789 );
        round_trip_vec2(I);
        REQUIRE( A.x == I.x );
        REQUIRE( A.y == I.y );

        // Invalid values should throw.
        REQUIRE_THROWS( parse_from_string( "(1.0)" ) );
        //REQUIRE_THROWS( parse_from_string( "(1.0,2.0,1.0)" ) );
        REQUIRE_THROWS( parse_from_string( "(1.0,2.0),1.0" ) );
        REQUIRE_THROWS( parse_from_string( "1.0,2.0)" ) );
        REQUIRE_THROWS( parse_from_string( "(1.0,xyz)" ) );
        REQUIRE_THROWS( parse_from_string( "(1.0 2.0)" ) );
    }
}

