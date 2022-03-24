
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

    SUBCASE("operator cast to num_array"){
        affine_transform<double> A;
        auto B = static_cast<num_array<double>>(A);
        REQUIRE( B.read_coeff(0,0) == 1.0 );
        REQUIRE( B.read_coeff(1,1) == 1.0 );
        REQUIRE( B.read_coeff(2,2) == 1.0 );
        REQUIRE( B.read_coeff(3,3) == 1.0 );

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

    REQUIRE( A.coeff(1,0) == 0.0 );
    REQUIRE( A.coeff(2,0) == 0.0 );
    REQUIRE( A.coeff(2,1) == 0.0 );
    REQUIRE( A.coeff(0,1) == 0.0 );
    REQUIRE( A.coeff(0,2) == 0.0 );
    REQUIRE( A.coeff(0,3) == 0.0 );
    REQUIRE( A.coeff(1,2) == 0.0 );
    REQUIRE( A.coeff(1,3) == 0.0 );
    REQUIRE( A.coeff(2,3) == 0.0 );

    REQUIRE_THROWS( A.coeff(3,3) == 1.0 );
    REQUIRE_THROWS( A.coeff(3,0) == 0.0 );
    REQUIRE_THROWS( A.coeff(3,1) == 0.0 );
    REQUIRE_THROWS( A.coeff(3,2) == 0.0 );


    REQUIRE( A.read_coeff(0,0) == 1.0 );
    REQUIRE( A.read_coeff(1,1) == 1.0 );
    REQUIRE( A.read_coeff(2,2) == 1.0 );
    REQUIRE( A.read_coeff(3,3) == 1.0 );

    REQUIRE( A.read_coeff(1,0) == 0.0 );
    REQUIRE( A.read_coeff(2,0) == 0.0 );
    REQUIRE( A.read_coeff(3,0) == 0.0 );
    REQUIRE( A.read_coeff(2,1) == 0.0 );
    REQUIRE( A.read_coeff(3,1) == 0.0 );
    REQUIRE( A.read_coeff(3,2) == 0.0 );

    REQUIRE( A.read_coeff(0,1) == 0.0 );
    REQUIRE( A.read_coeff(0,2) == 0.0 );
    REQUIRE( A.read_coeff(0,3) == 0.0 );
    REQUIRE( A.read_coeff(1,2) == 0.0 );
    REQUIRE( A.read_coeff(1,3) == 0.0 );
    REQUIRE( A.read_coeff(2,3) == 0.0 );
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
        X.coeff(0,3) = 2.0;
        X.coeff(1,3) = -3.0;
        X.coeff(2,3) = 7.0;
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
            XY.coeff(1,0) = 1.0;
            XY.coeff(0,1) = 1.0;
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
            XZ.coeff(2,0) = 1.0;
            XZ.coeff(0,2) = 1.0;
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
            YZ.coeff(2,1) = 1.0;
            YZ.coeff(1,2) = 1.0;
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

TEST_CASE( "affine_transform factories" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();
    const auto eps = 10.0 * std::sqrt( std::numeric_limits<double>::epsilon() );

    const vec3<double> x_unit(1.0, 0.0, 0.0);
    const vec3<double> y_unit(0.0, 1.0, 0.0);
    const vec3<double> z_unit(0.0, 0.0, 1.0);

    SUBCASE( "affine_translate" ){
        const vec3<double> shift(1.5, 2.5, -3.5);
        const vec3<double> shift_inf(1.5, 2.5, inf);
        const vec3<double> shift_nan(nan, 2.5, 3.5);

        REQUIRE_THROWS( affine_translate<double>(shift_inf) );
        REQUIRE_THROWS( affine_translate<double>(shift_nan) );
        
        {
            auto A = affine_translate<double>(shift);
            vec3<double> x(1.0, 2.0, 3.0);
            const auto orig_x = x;
            A.apply_to(x);
            REQUIRE(x == (orig_x + shift));
        }
    }

    SUBCASE( "affine_scale" ){
        const vec3<double> centre(1.5, 2.5, -3.5);
        const vec3<double> centre_inf(1.5, 2.5, inf);
        const vec3<double> centre_nan(nan, 2.5, 3.5);

        REQUIRE_THROWS( affine_scale<double>(centre_inf, 1.0) );
        REQUIRE_THROWS( affine_scale<double>(centre_nan, 1.0) );
        REQUIRE_THROWS( affine_scale<double>(centre, nan) );
        REQUIRE_THROWS( affine_scale<double>(centre, inf) );
        
        {
            vec3<double> x(1.0, 2.0, 3.0);
            const auto orig_x = x;

            // These should combine to form an identity transformation.
            affine_scale<double>(centre *  1.0, 3.0/1.0).apply_to(x);
            affine_scale<double>(centre *  1.0, 1.0/3.0).apply_to(x);

            affine_scale<double>(centre * -1.0, 1.0/2.5).apply_to(x);
            affine_scale<double>(centre * -1.0, 2.5/1.0).apply_to(x);

            affine_scale<double>(centre *  2.3, 0.5/1.0).apply_to(x);
            affine_scale<double>(centre *  2.3, 1.0/0.5).apply_to(x);
            REQUIRE(orig_x.distance(x) < eps);
        }
    }

    SUBCASE( "affine_mirror" ){
        const vec3<double> centre(1.5, 2.5, -3.5);
        const vec3<double> centre_inf(1.5, 2.5, inf);
        const vec3<double> centre_nan(nan, 2.5, 3.5);

        const auto unit = vec3<double>(1.5, 2.5, -3.5).unit();;
        const vec3<double> unit_inf(1.5, 2.5, inf);
        const vec3<double> unit_nan(nan, 2.5, 3.5);

        REQUIRE_THROWS( affine_mirror<double>( plane<double>(unit, centre_inf) ) );
        REQUIRE_THROWS( affine_mirror<double>( plane<double>(unit, centre_nan) ) );
        REQUIRE_THROWS( affine_mirror<double>( plane<double>(unit_inf, centre) ) );
        REQUIRE_THROWS( affine_mirror<double>( plane<double>(unit_nan, centre) ) );
        
        {
            vec3<double> x(1.0, 2.0, 3.0);
            const auto orig_x = x;

            // These should combine to form an identity transformation.
            affine_mirror<double>(plane<double>(unit.rotate_around_x( 0.0 ), centre)).apply_to(x);
            affine_mirror<double>(plane<double>(unit.rotate_around_x( 0.0 ), centre)).apply_to(x);

            affine_mirror<double>(plane<double>(unit.rotate_around_y( 3.1 ), centre)).apply_to(x);
            affine_mirror<double>(plane<double>(unit.rotate_around_y( 3.1 ), centre)).apply_to(x);
                                                           
            affine_mirror<double>(plane<double>(unit.rotate_around_x( 1.3 ), centre)).apply_to(x);
            affine_mirror<double>(plane<double>(unit.rotate_around_x( 1.3 ), centre)).apply_to(x);
                                                           
            affine_mirror<double>(plane<double>(unit.rotate_around_z( 0.2 ), centre)).apply_to(x);
            affine_mirror<double>(plane<double>(unit.rotate_around_z( 0.2 ), centre)).apply_to(x);

            // Mirroring should not be altered by the sign of the plane's normal.
            affine_mirror<double>(plane<double>(unit * -1.0, centre)).apply_to(x);
            affine_mirror<double>(plane<double>(unit *  1.0, centre)).apply_to(x);
            REQUIRE(orig_x.distance(x) < eps);
        }
    }

    SUBCASE( "affine_rotate" ){
        const vec3<double> centre(1.5, 2.5, -3.5);
        const vec3<double> centre_inf(1.5, 2.5, inf);
        const vec3<double> centre_nan(nan, 2.5, 3.5);

        const auto unit = vec3<double>(1.5, 2.5, -3.5).unit();;
        const vec3<double> unit_inf(1.5, 2.5, inf);
        const vec3<double> unit_nan(nan, 2.5, 3.5);

        const auto angle = 3.45;

        REQUIRE_THROWS( affine_rotate<double>( unit, centre_inf, angle) );
        REQUIRE_THROWS( affine_rotate<double>( unit, centre_nan, angle) );
        REQUIRE_THROWS( affine_rotate<double>( unit_inf, centre, angle) );
        REQUIRE_THROWS( affine_rotate<double>( unit_nan, centre, angle) );
        REQUIRE_THROWS( affine_rotate<double>( unit, centre, inf) );
        REQUIRE_THROWS( affine_rotate<double>( unit, centre, nan) );
        
        {
            vec3<double> x(1.0, 2.0, 3.0);
            const auto orig_x = x;

            // These should combine to form an identity transformation.
            affine_rotate<double>(unit.rotate_around_x( 0.0 ), centre.rotate_around_y( 0.5 ),  1.23).apply_to(x);
            affine_rotate<double>(unit.rotate_around_x( 0.0 ), centre.rotate_around_y( 0.5 ), -1.23).apply_to(x);

            affine_rotate<double>(unit.rotate_around_y( 3.1 ), centre.rotate_around_x( 1.5 ), -2.34).apply_to(x);
            affine_rotate<double>(unit.rotate_around_y( 3.1 ), centre.rotate_around_x( 1.5 ),  2.34).apply_to(x);
                                             
            affine_rotate<double>(unit.rotate_around_x( 1.3 ), centre.rotate_around_z( 2.1 ),  3.45).apply_to(x);
            affine_rotate<double>(unit.rotate_around_x( 1.3 ), centre.rotate_around_z( 2.1 ), -3.45).apply_to(x);
                                             
            affine_rotate<double>(unit.rotate_around_z( 0.2 ), centre.rotate_around_x( 4.5 ), -4.56).apply_to(x);
            affine_rotate<double>(unit.rotate_around_z( 0.2 ), centre.rotate_around_x( 4.5 ),  4.56).apply_to(x);

            REQUIRE(orig_x.distance(x) < eps);
        }
    }
}

TEST_CASE( "affine_transform invert" ){
    SUBCASE( "identity" ){
        affine_transform<double> A;

        // Ensure input is an identity matrix.
        REQUIRE( A.read_coeff(0,0) == 1.0 );
        REQUIRE( A.read_coeff(1,1) == 1.0 );
        REQUIRE( A.read_coeff(2,2) == 1.0 );
        REQUIRE( A.read_coeff(3,3) == 1.0 );

        REQUIRE( A.read_coeff(1,0) == 0.0 );
        REQUIRE( A.read_coeff(2,0) == 0.0 );
        REQUIRE( A.read_coeff(3,0) == 0.0 );
        REQUIRE( A.read_coeff(2,1) == 0.0 );
        REQUIRE( A.read_coeff(3,1) == 0.0 );
        REQUIRE( A.read_coeff(3,2) == 0.0 );

        REQUIRE( A.read_coeff(0,1) == 0.0 );
        REQUIRE( A.read_coeff(0,2) == 0.0 );
        REQUIRE( A.read_coeff(0,3) == 0.0 );
        REQUIRE( A.read_coeff(1,2) == 0.0 );
        REQUIRE( A.read_coeff(1,3) == 0.0 );
        REQUIRE( A.read_coeff(2,3) == 0.0 );

        // Inverse should be an identity too.
        const auto B = A.invert();
        REQUIRE( B.read_coeff(0,0) == 1.0 );
        REQUIRE( B.read_coeff(1,1) == 1.0 );
        REQUIRE( B.read_coeff(2,2) == 1.0 );
        REQUIRE( B.read_coeff(3,3) == 1.0 );

        REQUIRE( B.read_coeff(1,0) == 0.0 );
        REQUIRE( B.read_coeff(2,0) == 0.0 );
        REQUIRE( B.read_coeff(3,0) == 0.0 );
        REQUIRE( B.read_coeff(2,1) == 0.0 );
        REQUIRE( B.read_coeff(3,1) == 0.0 );
        REQUIRE( B.read_coeff(3,2) == 0.0 );

        REQUIRE( B.read_coeff(0,1) == 0.0 );
        REQUIRE( B.read_coeff(0,2) == 0.0 );
        REQUIRE( B.read_coeff(0,3) == 0.0 );
        REQUIRE( B.read_coeff(1,2) == 0.0 );
        REQUIRE( B.read_coeff(1,3) == 0.0 );
        REQUIRE( B.read_coeff(2,3) == 0.0 );
    }

    const auto eps = 10.0 * std::sqrt( std::numeric_limits<double>::epsilon() );

    const auto max_diff = [](const affine_transform<double>& A,
                             const affine_transform<double>& B){
        double max_diff = 0.0;
        for(long int r = 0; r < 4; ++r){
            for(long int c = 0; c < 4; ++c){
                const auto diff = std::abs(B.read_coeff(r,c) - A.read_coeff(r,c));
                if(diff < max_diff) max_diff = diff;
            }
        }
        return max_diff;
    };

    SUBCASE( "inverse is cyclic (translation-only)" ){
        const auto A = affine_translate<double>( vec3<double>( 1.0, 2.5, 5.5 ) );
        const auto B = A.invert().invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }

    SUBCASE( "inverse is cyclic (scale-only)" ){
        const auto A = affine_scale<double>( vec3<double>( 1.0, 2.5, 5.5 ), 0.5 );
        const auto B = A.invert().invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }

    SUBCASE( "inverse is cyclic (mirror-only)" ){
        const auto p = plane<double>( vec3<double>( -1.0, 2.5, -5.5 ).unit(),
                                      vec3<double>( 1.0, 2.5, 5.5 ) );
        const auto A = affine_mirror<double>( p );
        const auto B = A.invert().invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }

    SUBCASE( "inverse is cyclic (rotate-only)" ){
        const auto A = affine_rotate<double>( vec3<double>( 1.0, 2.5, 5.5 ),
                                              vec3<double>( -1.0, 2.5, -5.5 ).unit(),
                                              1.23 );
        const auto B = A.invert().invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }

    SUBCASE( "inverse is cyclic (arbitrary)" ){
        affine_transform<double> A;
        A.coeff(0,0) = 1.0;
        A.coeff(1,1) = 2.0;
        A.coeff(2,2) = 3.0;

        A.coeff(1,0) = 4.0;
        A.coeff(2,0) = 5.0;
        A.coeff(2,1) = 6.0;
        A.coeff(0,1) = 7.0;
        A.coeff(0,2) = 8.0;
        A.coeff(0,3) = 9.0;
        A.coeff(1,2) = 10.0;
        A.coeff(1,3) = 11.0;
        A.coeff(2,3) = 12.0;

        const auto B = A.invert().invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }

    // For orthonormal rotation matrices, the transpose should be the same as the inverse.
    SUBCASE( "inverse of rotation-only affine matrix is same as transpose" ){
        auto X = vec3<double>( 1.0, 2.0,-3.0 ).unit();
        auto Y = vec3<double>(-1.0, 2.0, 3.0 ).unit();
        auto Z = vec3<double>( 1.0,-2.0, 3.0 ).unit();
        X.GramSchmidt_orthogonalize(Y, Z);
        X = X.unit();
        Y = Y.unit();
        Z = Z.unit();

        affine_transform<double> A;
        A.coeff(0,0) = X.x;
        A.coeff(1,0) = X.y;
        A.coeff(2,0) = X.z;

        A.coeff(0,1) = Y.x;
        A.coeff(1,1) = Y.y;
        A.coeff(2,1) = Y.z;

        A.coeff(0,2) = Z.x;
        A.coeff(1,2) = Z.y;
        A.coeff(2,2) = Z.z;

        const auto B = A.invert();
        const auto C = static_cast<affine_transform<double>>( static_cast<num_array<double>>(A).transpose() );
        const auto md = max_diff(B, C);
        REQUIRE( md < eps );
    }

    SUBCASE( "attempted inverse of a singular matrix throws" ){
        affine_transform<double> A;
        A.coeff(0,0) = 0.0;
        A.coeff(1,1) = 0.0;
        A.coeff(2,2) = 0.0;

        A.coeff(1,0) = 0.0;
        A.coeff(2,0) = 0.0;
        A.coeff(2,1) = 0.0;
        A.coeff(0,1) = 0.0;
        A.coeff(0,2) = 0.0;

        A.coeff(0,3) = 0.0;
        A.coeff(1,2) = 0.0;
        A.coeff(1,3) = 0.0;
        A.coeff(2,3) = 0.0;

        REQUIRE_THROWS(A.invert());
    }

}


/*
Currently outstanding:
    void apply_to(point_set<T> &in) const;
    bool write_to( std::ostream &os ) const;
    bool read_from( std::istream &is );
*/
