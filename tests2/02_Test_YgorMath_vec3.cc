
#include <limits>

#include <YgorMath.h>

#include "doctest/doctest.h"


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
}


TEST_CASE( "vec3 member functions" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

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

        REQUIRE( (std::abs(x_unit.angle(y_unit, &OK) - M_PI * 0.5) < eps) );
        REQUIRE( OK );
        REQUIRE( (std::abs(x_unit.angle(z_unit, &OK) - M_PI * 0.5) < eps) );
        REQUIRE( OK );
        REQUIRE( (std::abs(y_unit.angle(z_unit, &OK) - M_PI * 0.5) < eps) );
        REQUIRE( OK );

        REQUIRE( (std::abs(x_unit.angle(x_unit * -1.0, &OK) - M_PI) < eps) );
        REQUIRE( OK );
        REQUIRE( (std::abs(y_unit.angle(y_unit * -1.0, &OK) - M_PI) < eps) );
        REQUIRE( OK );
        REQUIRE( (std::abs(z_unit.angle(z_unit * -1.0, &OK) - M_PI) < eps) );
        REQUIRE( OK );

        REQUIRE( (std::abs((x_unit + z_unit).angle((x_unit + z_unit) * -1.0, &OK) - M_PI) < eps) );
        REQUIRE( OK );
        REQUIRE( (std::abs((x_unit + y_unit).angle((x_unit + y_unit) * -1.0, &OK) - M_PI) < eps) );
        REQUIRE( OK );
        REQUIRE( (std::abs((y_unit + x_unit).angle((y_unit + x_unit) * -1.0, &OK) - M_PI) < eps) );
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
}

//        vec3<T> rotate_around_x(T angle_rad) const; // Rotate by some angle (in radians, [0,2*pi]) around a cardinal axis.
//        vec3<T> rotate_around_y(T angle_rad) const;
//        vec3<T> rotate_around_z(T angle_rad) const;
//
//        vec3<T> rotate_around_unit(const vec3<T> &axis, T angle_rad) const; // Rotate around the given unit vector by
//                                                                            // some angle (in radians, [0,2*pi]).
//
//        bool GramSchmidt_orthogonalize(vec3<T> &, vec3<T> &) const; //Using *this as seed, orthogonalize (n.b. not orthonormalize) the inputs.
//
//        std::string to_string(void) const;
//        vec3<T> from_string(const std::string &in); //Sets *this and returns a copy.
//
//        //Friends.
//        template<class Y> friend std::ostream & operator << (std::ostream &, const vec3<Y> &); // ---> Overloaded stream operators.
//        template<class Y> friend std::istream & operator >> (std::istream &, vec3<Y> &);

