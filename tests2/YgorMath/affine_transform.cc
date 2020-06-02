
#include <limits>
#include <utility>
#include <iostream>

#include <YgorMath.h>

#include "doctest/doctest.h"


TEST_CASE( "affine_transform constructors" ){
    const vec3<double> x_unit(1.0, 0.0, 0.0);
    const vec3<double> y_unit(0.0, 1.0, 0.0);
    const vec3<double> z_unit(0.0, 0.0, 1.0);
    const vec3<double> zero3(0.0, 0.0, 0.0);

    SUBCASE("default constructor is an identity transformation"){
        affine_transform<double> A;
        REQUIRE( A.read_coeff(0,0) == 1.0 );
        REQUIRE( A.read_coeff(1,1) == 1.0 );
        REQUIRE( A.read_coeff(2,2) == 1.0 );
        REQUIRE( A.read_coeff(3,3) == 1.0 );

        REQUIRE( A.read_coeff(0,1) == 0.0 );
        REQUIRE( A.read_coeff(0,2) == 0.0 );
        REQUIRE( A.read_coeff(0,3) == 0.0 );
        REQUIRE( A.read_coeff(1,2) == 0.0 );
        REQUIRE( A.read_coeff(1,3) == 0.0 );
        REQUIRE( A.read_coeff(2,3) == 0.0 );

        REQUIRE( A.read_coeff(1,0) == 0.0 );
        REQUIRE( A.read_coeff(2,0) == 0.0 );
        REQUIRE( A.read_coeff(3,0) == 0.0 );
        REQUIRE( A.read_coeff(2,1) == 0.0 );
        REQUIRE( A.read_coeff(3,1) == 0.0 );
        REQUIRE( A.read_coeff(3,2) == 0.0 );
    }

    SUBCASE("default constructor is an identity transformation"){
        affine_transform<double> A;
        affine_transform<double> B(A);
        REQUIRE( B.read_coeff(0,0) == 1.0 );
        REQUIRE( B.read_coeff(1,1) == 1.0 );
        REQUIRE( B.read_coeff(2,2) == 1.0 );
        REQUIRE( B.read_coeff(0,0) == 1.0 );

        REQUIRE( B.read_coeff(0,1) == 0.0 );
        REQUIRE( B.read_coeff(0,2) == 0.0 );
        REQUIRE( B.read_coeff(0,3) == 0.0 );
        REQUIRE( B.read_coeff(1,2) == 0.0 );
        REQUIRE( B.read_coeff(1,3) == 0.0 );
        REQUIRE( B.read_coeff(2,3) == 0.0 );

        REQUIRE( B.read_coeff(1,0) == 0.0 );
        REQUIRE( B.read_coeff(2,0) == 0.0 );
        REQUIRE( B.read_coeff(3,0) == 0.0 );
        REQUIRE( B.read_coeff(2,1) == 0.0 );
        REQUIRE( B.read_coeff(3,1) == 0.0 );
        REQUIRE( B.read_coeff(3,2) == 0.0 );
    }
}

TEST_CASE( "affine_transform operators" ){
    const vec3<double> x_unit(1.0, 0.0, 0.0);
    const vec3<double> y_unit(0.0, 1.0, 0.0);
    const vec3<double> z_unit(0.0, 0.0, 1.0);
    const vec3<double> zero3(0.0, 0.0, 0.0);

    SUBCASE("operator== and operator!="){
        affine_transform<double> L1;
        affine_transform<double> L2(L1);
        REQUIRE( L1 == L1 );
        REQUIRE( L1 == L2 );

        for(long int i = 0; i < 3; ++i){
            for(long int j = 0; j < 3; ++j){
                affine_transform<double> L3(L1);
                L3.coeff(i,j) = 2.0;
                REQUIRE( L1 != L3 );
            }
        }
    }

    SUBCASE("operator="){
        affine_transform<double> L1;
        L1.coeff(0,0) = 2.0;
        affine_transform<double> L2;
        REQUIRE( L1 != L2 );
        L2 = L1;
        REQUIRE( L1 == L2 );
    }

    SUBCASE("operator<"){
        affine_transform<double> L1;
        L1.coeff(0,0) = 2.0;
        affine_transform<double> L2;
        affine_transform<double> L3 = L1;
        long int i;

        i = 0;
        i += (L1 < L1) ? 1 : 0;
        i += (L1 < L1) ? 1 : 0;
        REQUIRE(i == 0);

        i = 0;
        i += (L1 < L2) ? 1 : 0;
        i += (L2 < L1) ? 1 : 0;
        REQUIRE(i == 1);

        i = 0;
        i += (L1 < L3) ? 1 : 0;
        i += (L3 < L1) ? 1 : 0;
        REQUIRE(i == 0);
    }
}

TEST_CASE( "affine_transform accessors" ){
    affine_transform<double> A;

    REQUIRE( A.coeff(0,0) == 1.0 );
    REQUIRE( A.coeff(1,1) == 1.0 );
    REQUIRE( A.coeff(2,2) == 1.0 );

    REQUIRE( A.coeff(0,1) == 0.0 );
    REQUIRE( A.coeff(0,2) == 0.0 );
    REQUIRE( A.coeff(1,2) == 0.0 );
    REQUIRE( A.coeff(1,0) == 0.0 );
    REQUIRE( A.coeff(2,0) == 0.0 );
    REQUIRE( A.coeff(3,0) == 0.0 );
    REQUIRE( A.coeff(2,1) == 0.0 );
    REQUIRE( A.coeff(3,1) == 0.0 );
    REQUIRE( A.coeff(3,2) == 0.0 );

    REQUIRE_THROWS( A.coeff(3,3) == 1.0 );
    REQUIRE_THROWS( A.coeff(0,3) == 0.0 );
    REQUIRE_THROWS( A.coeff(1,3) == 0.0 );
    REQUIRE_THROWS( A.coeff(2,3) == 0.0 );


    REQUIRE( A.read_coeff(0,0) == 1.0 );
    REQUIRE( A.read_coeff(1,1) == 1.0 );
    REQUIRE( A.read_coeff(2,2) == 1.0 );
    REQUIRE( A.read_coeff(3,3) == 1.0 );

    REQUIRE( A.read_coeff(0,1) == 0.0 );
    REQUIRE( A.read_coeff(0,2) == 0.0 );
    REQUIRE( A.read_coeff(0,3) == 0.0 );
    REQUIRE( A.read_coeff(1,2) == 0.0 );
    REQUIRE( A.read_coeff(1,3) == 0.0 );
    REQUIRE( A.read_coeff(2,3) == 0.0 );

    REQUIRE( A.read_coeff(1,0) == 0.0 );
    REQUIRE( A.read_coeff(2,0) == 0.0 );
    REQUIRE( A.read_coeff(3,0) == 0.0 );
    REQUIRE( A.read_coeff(2,1) == 0.0 );
    REQUIRE( A.read_coeff(3,1) == 0.0 );
    REQUIRE( A.read_coeff(3,2) == 0.0 );
}

TEST_CASE( "affine_transform appliers" ){
    const vec3<double> x_unit(1.0, 0.0, 0.0);
    const vec3<double> y_unit(0.0, 1.0, 0.0);
    const vec3<double> z_unit(0.0, 0.0, 1.0);
    const vec3<double> zero3(0.0, 0.0, 0.0);

    SUBCASE( "Identity transforms on vec3" ){
        affine_transform<double> I;
        vec3<double> A(x_unit);
        vec3<double> B(y_unit);
        vec3<double> C(z_unit);
        vec3<double> D(zero3);

        I.apply_to(A);
        REQUIRE(A == x_unit);

        I.apply_to(B);
        REQUIRE(B == y_unit);

        I.apply_to(C);
        REQUIRE(C == z_unit);

        I.apply_to(D);
        REQUIRE(D == zero3);
    }

    SUBCASE( "Scale transforms on vec3" ){
        affine_transform<double> X;
        X.coeff(0,0) = 2.0;
        X.coeff(1,1) = 3.0;
        X.coeff(2,2) = -7.0;
        vec3<double> A(x_unit);
        vec3<double> B(y_unit);
        vec3<double> C(z_unit);
        vec3<double> D(zero3);

        X.apply_to(A);
        REQUIRE(A == x_unit * 2.0);

        X.apply_to(B);
        REQUIRE(B == y_unit * 3.0);

        X.apply_to(C);
        REQUIRE(C == z_unit * -7.0);

        X.apply_to(D);
        REQUIRE(D == zero3);
    }

    SUBCASE( "Shift transforms on vec3" ){
        affine_transform<double> X;
        X.coeff(3,0) = 2.0;
        X.coeff(3,1) = -3.0;
        X.coeff(3,2) = 7.0;
        vec3<double> A(x_unit);
        vec3<double> B(y_unit);
        vec3<double> C(z_unit);
        vec3<double> D(zero3);

        X.apply_to(A);
        REQUIRE(A == vec3<double>(3.0, -3.0, 7.0));

        X.apply_to(B);
        REQUIRE(B == vec3<double>(2.0, -2.0, 7.0));

        X.apply_to(C);
        REQUIRE(C == vec3<double>(2.0, -3.0, 8.0));

        X.apply_to(D);
        REQUIRE(D == vec3<double>(2.0, -3.0, 7.0));
    }

    SUBCASE( "Rotation transforms on vec3" ){
        SUBCASE( "XY" ){
            affine_transform<double> XY;
            XY.coeff(0,0) = 0.0;
            XY.coeff(0,1) = 1.0;
            XY.coeff(1,0) = 1.0;
            XY.coeff(1,1) = 0.0;
            vec3<double> A(x_unit);
            vec3<double> B(y_unit);
            vec3<double> C(z_unit);
            vec3<double> D(zero3);

            XY.apply_to(A);
            REQUIRE(A == y_unit);

            XY.apply_to(B);
            REQUIRE(B == x_unit);

            XY.apply_to(C);
            REQUIRE(C == z_unit);

            XY.apply_to(D);
            REQUIRE(D == zero3);
        }

        SUBCASE( "XZ" ){
            affine_transform<double> XZ;
            XZ.coeff(0,0) = 0.0;
            XZ.coeff(0,2) = 1.0;
            XZ.coeff(2,0) = 1.0;
            XZ.coeff(2,2) = 0.0;
            vec3<double> A(x_unit);
            vec3<double> B(y_unit);
            vec3<double> C(z_unit);
            vec3<double> D(zero3);

            XZ.apply_to(A);
            REQUIRE(A == z_unit);

            XZ.apply_to(B);
            REQUIRE(B == y_unit);

            XZ.apply_to(C);
            REQUIRE(C == x_unit);

            XZ.apply_to(D);
            REQUIRE(D == zero3);
        }

        SUBCASE( "YZ" ){
            affine_transform<double> YZ;
            YZ.coeff(1,1) = 0.0;
            YZ.coeff(1,2) = 1.0;
            YZ.coeff(2,1) = 1.0;
            YZ.coeff(2,2) = 0.0;
            vec3<double> A(x_unit);
            vec3<double> B(y_unit);
            vec3<double> C(z_unit);
            vec3<double> D(zero3);

            YZ.apply_to(A);
            REQUIRE(A == x_unit);

            YZ.apply_to(B);
            REQUIRE(B == z_unit);

            YZ.apply_to(C);
            REQUIRE(C == y_unit);

            YZ.apply_to(D);
            REQUIRE(D == zero3);
        }
    }
}

/*
Currently outstanding:
    void apply_to(point_set<T> &in) const;
    bool write_to( std::ostream &os ) const;
    bool read_from( std::istream &is );
*/
