
#include <limits>
#include <utility>
#include <iostream>

#include <YgorMath.h>

#include "doctest/doctest.h"


TEST_CASE( "num_array constructors" ){
    SUBCASE("default constructor produces a zero-sized matrix"){
        num_array<double> A;
        REQUIRE(A.num_rows() == 0);
        REQUIRE(A.num_cols() == 0);
        REQUIRE(A.size() == 0);
    }

    SUBCASE("sized constructor refuses invalid dimensions"){
        REQUIRE_THROWS( num_array<double>( 0, 2) );
        REQUIRE_THROWS( num_array<double>( 1, 0) );
        REQUIRE_THROWS( num_array<double>( 0, 0) );
        REQUIRE_THROWS( num_array<double>(-1, 2) );
        REQUIRE_THROWS( num_array<double>( 2,-1) );
        REQUIRE_THROWS( num_array<double>(-1,-1) );
        REQUIRE_THROWS( num_array<double>(1'000'000'000'000, 1'000'000'000'000) );
    }

    SUBCASE("sized constructor produces a zeroed matrix by default"){
        num_array<double> A(2, 2);
        REQUIRE(A.read_coeff(0,0) == 0.0);
        REQUIRE(A.read_coeff(0,1) == 0.0);
        REQUIRE(A.read_coeff(0,0) == 0.0);
        REQUIRE(A.read_coeff(1,1) == 0.0);
    }

    SUBCASE("sized constructor accepts optional coefficient value"){
        num_array<double> A(2, 2, 73.0);
        REQUIRE(A.read_coeff(0,0) == 73.0);
        REQUIRE(A.read_coeff(0,1) == 73.0);
        REQUIRE(A.read_coeff(0,0) == 73.0);
        REQUIRE(A.read_coeff(1,1) == 73.0);
    }

    SUBCASE("sized constructor sizes are honoured"){
        for(long int rows = 1; rows < 11; rows += 3){
            for(long int cols = 1; cols < 11; cols += 3){
                num_array<double> A(rows, cols);
                REQUIRE(A.size() == A.num_rows() * A.num_cols());
                REQUIRE(A.size() == rows * cols);
            }
        }
    }

    SUBCASE("copy constructor copies correctly"){
        for(long int rows = 2; rows < 12; rows += 3){
            for(long int cols = 3; cols < 13; cols += 3){
                num_array<double> A(rows, cols);
                A.coeff(0, 0) = 1.0;
                A.coeff(rows-1, 0) = 2.0;
                A.coeff(0, cols-1) = 3.0;

                num_array<double> B(A);
                REQUIRE(B.read_coeff(0,0) == 1.0);
                REQUIRE(B.read_coeff(rows-1,0) == 2.0);
                REQUIRE(B.read_coeff(0,cols-1) == 3.0);
                REQUIRE(A == B);
            }
        }
    }

    SUBCASE("operator cast to affine_transform"){
        auto A = num_array<double>().iota(4, 4, 1.0);

        REQUIRE_THROWS( static_cast<affine_transform<double>>(A) );

        A.coeff(3,0) = 0.0;
        A.coeff(3,1) = 0.0;
        A.coeff(3,2) = 0.0;
        A.coeff(3,3) = 1.0;

        auto B = static_cast<affine_transform<double>>(A);

        // Note: assumes iota is column-major (i.e., fills columns fully before moving to next row).
        REQUIRE( B.read_coeff(0,0) ==  1.0 );
        REQUIRE( B.read_coeff(1,0) ==  2.0 );
        REQUIRE( B.read_coeff(2,0) ==  3.0 );
        REQUIRE( B.read_coeff(3,0) ==  0.0 );

        REQUIRE( B.read_coeff(0,1) ==  5.0 );
        REQUIRE( B.read_coeff(1,1) ==  6.0 );
        REQUIRE( B.read_coeff(2,1) ==  7.0 );
        REQUIRE( B.read_coeff(3,1) ==  0.0 );

        REQUIRE( B.read_coeff(0,2) ==  9.0 );
        REQUIRE( B.read_coeff(1,2) == 10.0 );
        REQUIRE( B.read_coeff(2,2) == 11.0 );
        REQUIRE( B.read_coeff(3,2) ==  0.0 );

        REQUIRE( B.read_coeff(0,3) == 13.0 );
        REQUIRE( B.read_coeff(1,3) == 14.0 );
        REQUIRE( B.read_coeff(2,3) == 15.0 );
        REQUIRE( B.read_coeff(3,3) ==  1.0 );
    }
}

TEST_CASE( "num_array assignment operators" ){
    SUBCASE("operator="){
        num_array<double> L1(10,10);
        L1.coeff(0,0) = 2.0;
        num_array<double> L2;
        REQUIRE( L1 != L2 );
        L2 = L1;
        REQUIRE( L1 == L2 );
        L2 = L2;
        REQUIRE( L1 == L2 );
        L2.coeff(0,0) = 1.0;
        REQUIRE( L1 != L2 );
        L2 = num_array<double>(5,5);
        REQUIRE( L1 != L2 );
        L1 = num_array<double>(5,5);
        REQUIRE( L1 == L2 );
    }
}

TEST_CASE( "num_array comparison operators" ){
    SUBCASE("operator== and operator!="){
        // Finite case.
        {
            num_array<double> L1(10,10);
            num_array<double> L2(L1);
            REQUIRE( L1 == L1 );
            REQUIRE( L1 == L2 );

            for(long int row = 0; row < 10; ++row){
                for(long int col = 0; col < 10; ++col){
                    num_array<double> L3(L1);
                    REQUIRE( L1 == L3 );
                    L3.coeff(row,col) = 2.0;
                    REQUIRE( L1 != L3 );
                }
            }

            num_array<double> L4(5,20);
            REQUIRE( L1 != L4 );
            num_array<double> L5(20,5);
            REQUIRE( L1 != L5 );
            REQUIRE( L4 != L5 );
        }

        // Non-finite case.
        {
            auto L1 = num_array<double>().zero(5,2);
            L1.coeff(2,1) = std::numeric_limits<double>::quiet_NaN();
            L1.coeff(4,1) = -std::numeric_limits<double>::infinity();
            const auto L2 = L1;
            REQUIRE(!L1.isfinite());
            REQUIRE(!L2.isfinite());

            const auto direct_equality = (L1 == L2); // Should fail due to NaN.
            REQUIRE(!direct_equality);

            std::stringstream ss1;
            std::stringstream ss2;
            L1.write_to(ss1);
            L2.write_to(ss2);
            REQUIRE(ss1.str() == ss2.str()); // Will properly handle NaNs.
        }
    }

    SUBCASE("operator<"){
        num_array<double> L1(10,10);
        L1.coeff(0,0) = 2.0;
        num_array<double> L2;
        num_array<double> L3 = L1;
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

TEST_CASE( "num_array scalar operators" ){
    SUBCASE("operator* and operator/"){
        num_array<double> L1(10,10,1.0);
        auto L2 = (L1 * -2.0) * 3.0;
        auto L3 = (L2 / 2.0) / -3.0;
        REQUIRE(L1 != L2);
        REQUIRE(L1 == L3);

        for(long int row = 0; row < L1.num_rows(); ++row){
            for(long int col = 0; col < L1.num_cols(); ++col){
                REQUIRE( L1.coeff(row,col) == 1.0 );
                REQUIRE( L2.coeff(row,col) == -6.0 );
                REQUIRE( L3.coeff(row,col) == 1.0 );
            }
        }
    }

    SUBCASE("operator*= and operator/="){
        num_array<double> L1(10,10,1.0);
        (L1 *= -2.0) *= 3.0;
        auto L2 = L1;
        (L1 /= 2.0) /= -3.0;
        auto L3 = L1;
        REQUIRE(L1 != L2);
        REQUIRE(L1 == L3);

        for(long int row = 0; row < L1.num_rows(); ++row){
            for(long int col = 0; col < L1.num_cols(); ++col){
                REQUIRE( L1.coeff(row,col) == 1.0 );
                REQUIRE( L2.coeff(row,col) == -6.0 );
                REQUIRE( L3.coeff(row,col) == 1.0 );
            }
        }
    }
}

TEST_CASE( "num_array matrix operators" ){
    SUBCASE("operator+"){
        num_array<double> L1(3,3);
        L1.coeff(0,0) =           1.0;
        L1.coeff(1,0) =         -10.0;
        L1.coeff(2,0) =         100.0;
        L1.coeff(0,1) =      -1'000.0;
        L1.coeff(1,1) =      10'000.0;
        L1.coeff(2,1) =    -100'000.0;
        L1.coeff(0,2) =   1'000'000.0;
        L1.coeff(1,2) = -10'000'000.0;
        L1.coeff(2,2) = 100'000'000.0;

        auto L2 = L1;
        auto L3 = L1 + L1 + L2;
        auto L4 = L1 + L1 + L2 + L1;

        for(long int row = 0; row < L1.num_rows(); ++row){
            for(long int col = 0; col < L1.num_cols(); ++col){
                REQUIRE( L1.coeff(row,col) == L2.coeff(row,col) );
                REQUIRE( 3.0 * L1.coeff(row,col) == L3.coeff(row,col) );
                REQUIRE( 4.0 * L1.coeff(row,col) == L4.coeff(row,col) );
            }
        }

        num_array<double> L5(3,2);
        num_array<double> L6(2,3);
        num_array<double> L7(2,2);
        REQUIRE_THROWS(L1 + L5);
        REQUIRE_THROWS(L1 + L6);
        REQUIRE_THROWS(L1 + L7);
    }

    SUBCASE("operator-"){
        num_array<double> L1(3,3);
        L1.coeff(0,0) =           1.0;
        L1.coeff(1,0) =         -10.0;
        L1.coeff(2,0) =         100.0;
        L1.coeff(0,1) =      -1'000.0;
        L1.coeff(1,1) =      10'000.0;
        L1.coeff(2,1) =    -100'000.0;
        L1.coeff(0,2) =   1'000'000.0;
        L1.coeff(1,2) = -10'000'000.0;
        L1.coeff(2,2) = 100'000'000.0;

        auto L2 = L1;
        auto L3 = L1 - L1 - L2;
        auto L4 = L1 - L1 - L2 - L1;

        for(long int row = 0; row < L1.num_rows(); ++row){
            for(long int col = 0; col < L1.num_cols(); ++col){
                REQUIRE( L1.coeff(row,col) == L2.coeff(row,col) );
                REQUIRE( -1.0 * L1.coeff(row,col) == L3.coeff(row,col) );
                REQUIRE( -2.0 * L1.coeff(row,col) == L4.coeff(row,col) );
            }
        }

        num_array<double> L5(3,2);
        num_array<double> L6(2,3);
        num_array<double> L7(2,2);
        REQUIRE_THROWS(L1 - L5);
        REQUIRE_THROWS(L1 - L6);
        REQUIRE_THROWS(L1 - L7);
    }

    SUBCASE("operator*"){
        // Example 1: an out-of-band generic example worked out by hand.
        {
            num_array<double> L1(2,2);
            L1.coeff(0,0) =  2.0;
            L1.coeff(0,1) =  5.0;
            L1.coeff(1,0) =  7.0;
            L1.coeff(1,1) = 11.0;

            num_array<double> L2(2,1);
            L2.coeff(0,0) = 23.0;
            L2.coeff(1,0) = 57.0;

            auto L3 = L1 * L2;
            REQUIRE( L3.coeff(0,0) == 331.0 );
            REQUIRE( L3.coeff(1,0) == 788.0 );

            REQUIRE_THROWS(L2 * L1);
        }

        // Example 2: identity matrix multiplication.
        {
            const auto L1 = num_array<double>().identity(100);
            REQUIRE( L1 == (L1 * L1 * L1));

            const auto L2 = num_array<double>().iota(100, 100, 0.0);
            const auto L3 = num_array<double>().iota(100, 100, -50.0);
            REQUIRE( L2 == L1 * L2);
            REQUIRE( L2 == L2 * L1);
            REQUIRE( L2 == L1 * L2 * L1);
            REQUIRE( L3 == L1 * L3);
        }

        // Example 3: square matrix using iota.
        //
        // Note: assumes iota is column-major (i.e., fills columns fully before moving to next row).
        {
            const auto L1 = num_array<double>().iota(3,3,0.0);
            const auto L2 = L1 * L1;
            REQUIRE( L2.read_coeff(0,0) ==   15.0 );
            REQUIRE( L2.read_coeff(0,1) ==   42.0 );
            REQUIRE( L2.read_coeff(0,2) ==   69.0 );
            REQUIRE( L2.read_coeff(1,0) ==   18.0 );
            REQUIRE( L2.read_coeff(1,1) ==   54.0 );
            REQUIRE( L2.read_coeff(1,2) ==   90.0 );
            REQUIRE( L2.read_coeff(2,0) ==   21.0 );
            REQUIRE( L2.read_coeff(2,1) ==   66.0 );
            REQUIRE( L2.read_coeff(2,2) ==  111.0 );
        }

        // Example 4: unsquare matrices using iota.
        //
        // Note: assumes iota is column-major (i.e., fills columns fully before moving to next row).
        {
            const auto L1 = num_array<double>().iota(2,3,0.0);
            const auto L2 = num_array<double>().iota(3,2,7.0);

            const auto L3 = L1 * L2;
            const auto L4 = L2 * L1;

            REQUIRE( L3.read_coeff(0,0) ==  52.0 );
            REQUIRE( L3.read_coeff(0,1) ==  70.0 );
            REQUIRE( L3.read_coeff(1,0) ==  76.0 );
            REQUIRE( L3.read_coeff(1,1) == 103.0 );

            REQUIRE( L4.read_coeff(0,0) == 10.0 );
            REQUIRE( L4.read_coeff(0,1) == 44.0 );
            REQUIRE( L4.read_coeff(0,2) == 78.0 );
            REQUIRE( L4.read_coeff(1,0) == 11.0 );
            REQUIRE( L4.read_coeff(1,1) == 49.0 );
            REQUIRE( L4.read_coeff(1,2) == 87.0 );
            REQUIRE( L4.read_coeff(2,0) == 12.0 );
            REQUIRE( L4.read_coeff(2,1) == 54.0 );
            REQUIRE( L4.read_coeff(2,2) == 96.0 );
        }

        // Disallowed invalid combinations.
        {
            num_array<double> L1(3, 2, 1.0);
            num_array<double> L2(2, 3, 2.0);
            num_array<double> L3(2, 4, 3.0);
            num_array<double> L4(4, 2, 4.0);

            REQUIRE_THROWS(L1 * L1);
            REQUIRE_THROWS(L1 * L2 * L2);
            REQUIRE_THROWS(L1 * L3 * L4 * L1);
        }
    }

    SUBCASE("operator+="){
        num_array<double> L1(3,3);
        L1.coeff(0,0) =           1.0;
        L1.coeff(1,0) =         -10.0;
        L1.coeff(2,0) =         100.0;
        L1.coeff(0,1) =      -1'000.0;
        L1.coeff(1,1) =      10'000.0;
        L1.coeff(2,1) =    -100'000.0;
        L1.coeff(0,2) =   1'000'000.0;
        L1.coeff(1,2) = -10'000'000.0;
        L1.coeff(2,2) = 100'000'000.0;

        auto L2 = L1;
        auto L3 = L1;
        L3 += L1;
        L3 += L2;
        auto L4 = ((L1 + L1) += L2) += L1;

        for(long int row = 0; row < L1.num_rows(); ++row){
            for(long int col = 0; col < L1.num_cols(); ++col){
                REQUIRE( L1.coeff(row,col) == L2.coeff(row,col) );
                REQUIRE( 3.0 * L1.coeff(row,col) == L3.coeff(row,col) );
                REQUIRE( 4.0 * L1.coeff(row,col) == L4.coeff(row,col) );
            }
        }

        num_array<double> L5(3,2);
        num_array<double> L6(2,3);
        num_array<double> L7(2,2);
        REQUIRE_THROWS(L1 += L5);
        REQUIRE_THROWS(L1 += L6);
        REQUIRE_THROWS(L1 += L7);
    }

    SUBCASE("operator-="){
        num_array<double> L1(3,3);
        L1.coeff(0,0) =           1.0;
        L1.coeff(1,0) =         -10.0;
        L1.coeff(2,0) =         100.0;
        L1.coeff(0,1) =      -1'000.0;
        L1.coeff(1,1) =      10'000.0;
        L1.coeff(2,1) =    -100'000.0;
        L1.coeff(0,2) =   1'000'000.0;
        L1.coeff(1,2) = -10'000'000.0;
        L1.coeff(2,2) = 100'000'000.0;

        auto L2 = L1;
        auto L3 = L1;
        L3 -= L1;
        L3 -= L2;
        auto L4 = ((L1 - L1) -= L2) -= L1;

        for(long int row = 0; row < L1.num_rows(); ++row){
            for(long int col = 0; col < L1.num_cols(); ++col){
                REQUIRE( L1.coeff(row,col) == L2.coeff(row,col) );
                REQUIRE( -1.0 * L1.coeff(row,col) == L3.coeff(row,col) );
                REQUIRE( -2.0 * L1.coeff(row,col) == L4.coeff(row,col) );
            }
        }

        num_array<double> L5(3,2);
        num_array<double> L6(2,3);
        num_array<double> L7(2,2);
        REQUIRE_THROWS(L1 -= L5);
        REQUIRE_THROWS(L1 -= L6);
        REQUIRE_THROWS(L1 -= L7);
    }

    SUBCASE("operator*"){
        // Example 1: an out-of-band generic example worked out by hand.
        {
            num_array<double> L1(2,2);
            L1.coeff(0,0) =  2.0;
            L1.coeff(0,1) =  5.0;
            L1.coeff(1,0) =  7.0;
            L1.coeff(1,1) = 11.0;

            num_array<double> L2(2,1);
            L2.coeff(0,0) = 23.0;
            L2.coeff(1,0) = 57.0;

            auto L3 = L1;
            L3 *= L2;
            REQUIRE( L3.coeff(0,0) == 331.0 );
            REQUIRE( L3.coeff(1,0) == 788.0 );

            REQUIRE_THROWS(L2 *= L1);
        }

        // Example 2: identity matrix multiplication.
        {
            const auto L1 = num_array<double>().identity(100);
            const auto R1 = (L1 * L1) *= L1;
            REQUIRE( L1 == R1 );

            const auto L2 = num_array<double>().iota(100, 100, 0.0);
            const auto L3 = num_array<double>().iota(100, 100, -50.0);
            const auto R2 = (L1 * L1) *= L2;
            const auto R3 = (L1 * L2) *= L1;
            const auto R4 = (L1 * L2) *= L1;
            const auto R5 = (L1 * L1) *= L3;
            REQUIRE( L2 == R2 );
            REQUIRE( L2 == R3 );
            REQUIRE( L2 == R4 );
            REQUIRE( L3 == R5 );
        }

        // Example 3: square matrix using iota.
        //
        // Note: assumes iota is column-major (i.e., fills columns fully before moving to next row).
        {
            const auto L1 = num_array<double>().iota(3,3,0.0);
            auto L2 = L1;
            L2 *= L1;
            REQUIRE( L2.read_coeff(0,0) ==   15.0 );
            REQUIRE( L2.read_coeff(0,1) ==   42.0 );
            REQUIRE( L2.read_coeff(0,2) ==   69.0 );
            REQUIRE( L2.read_coeff(1,0) ==   18.0 );
            REQUIRE( L2.read_coeff(1,1) ==   54.0 );
            REQUIRE( L2.read_coeff(1,2) ==   90.0 );
            REQUIRE( L2.read_coeff(2,0) ==   21.0 );
            REQUIRE( L2.read_coeff(2,1) ==   66.0 );
            REQUIRE( L2.read_coeff(2,2) ==  111.0 );
        }

        // Example 4: unsquare matrices using iota.
        //
        // Note: assumes iota is column-major (i.e., fills columns fully before moving to next row).
        {
            const auto L1 = num_array<double>().iota(2,3,0.0);
            const auto L2 = num_array<double>().iota(3,2,7.0);

            auto L3 = L1;
            L3 *= L2;
            auto L4 = L2;
            L4 *= L1;

            REQUIRE( L3.read_coeff(0,0) ==  52.0 );
            REQUIRE( L3.read_coeff(0,1) ==  70.0 );
            REQUIRE( L3.read_coeff(1,0) ==  76.0 );
            REQUIRE( L3.read_coeff(1,1) == 103.0 );

            REQUIRE( L4.read_coeff(0,0) == 10.0 );
            REQUIRE( L4.read_coeff(0,1) == 44.0 );
            REQUIRE( L4.read_coeff(0,2) == 78.0 );
            REQUIRE( L4.read_coeff(1,0) == 11.0 );
            REQUIRE( L4.read_coeff(1,1) == 49.0 );
            REQUIRE( L4.read_coeff(1,2) == 87.0 );
            REQUIRE( L4.read_coeff(2,0) == 12.0 );
            REQUIRE( L4.read_coeff(2,1) == 54.0 );
            REQUIRE( L4.read_coeff(2,2) == 96.0 );
        }

        // Disallowed invalid combinations.
        {
            num_array<double> L1(3, 2, 1.0);
            num_array<double> L2(2, 3, 2.0);
            num_array<double> L3(2, 4, 3.0);
            num_array<double> L4(4, 2, 4.0);

            REQUIRE_THROWS((L1 * L2) *= L2);
            REQUIRE_THROWS((L1 * L3 * L4) *= L1);
        }
    }
}

TEST_CASE( "num_array accessors" ){
    SUBCASE("index"){
        const auto L1 = num_array<double>().iota(3,3,0.0);
        REQUIRE(L1.index(0,0) == 0);
        REQUIRE(L1.index(2,0) == 2); // Assumes column-major ordering.
        REQUIRE(L1.index(0,2) == 6); // Assumes column-major ordering.
    }

    SUBCASE("num_rows and num_cols"){
        const auto L1 = num_array<double>().iota(3,4,0.0);
        REQUIRE(L1.num_rows() == 3);
        REQUIRE(L1.num_cols() == 4);

        num_array<double> L2(4,9);
        L2 = L1 * L2;
        REQUIRE(L2.num_rows() == 3);
        REQUIRE(L2.num_cols() == 9);
    }

    SUBCASE("size"){
        num_array<double> L1;
        REQUIRE(L1.size() == 0);

        L1 = num_array<double>().iota(3,4,0.0);
        REQUIRE(L1.size() == 12);

        L1 = num_array<double>().iota(4,3,0.0);
        REQUIRE(L1.size() == 12);
    }

    SUBCASE("coeff and read_coeff"){
        auto L1 = num_array<double>().iota(3,3,0.0);

        REQUIRE(L1.read_coeff(0,0) == 0.0);
        REQUIRE(L1.read_coeff(2,0) == 2.0); // Assumes column-major ordering.
        REQUIRE(L1.read_coeff(0,2) == 6.0); // Assumes column-major ordering.

        REQUIRE(L1.coeff(0,0) == 0.0);
        REQUIRE(L1.coeff(2,0) == 2.0); // Assumes column-major ordering.
        REQUIRE(L1.coeff(0,2) == 6.0); // Assumes column-major ordering.

        L1.coeff(2,0) = 40.0;
        REQUIRE(L1.coeff(2,0) == 40.0);

    }

    SUBCASE("begin and end"){
        auto L1 = num_array<double>().iota(2,2,0.0);

        REQUIRE( *(std::next(L1.begin(), 0)) == 0.0 );
        REQUIRE( *(std::next(L1.begin(), 1)) == 1.0 );
        REQUIRE( *(std::next(L1.begin(), 2)) == 2.0 );
        REQUIRE( *(std::next(L1.begin(), 3)) == 3.0 );
        REQUIRE( std::distance(L1.begin(), L1.end()) == static_cast<size_t>(4) );
    }

    SUBCASE("cbegin and cend"){
        auto L1 = num_array<double>().iota(2,2,0.0);

        REQUIRE( *(std::next(L1.cbegin(), 0)) == 0.0 );
        REQUIRE( *(std::next(L1.cbegin(), 1)) == 1.0 );
        REQUIRE( *(std::next(L1.cbegin(), 2)) == 2.0 );
        REQUIRE( *(std::next(L1.cbegin(), 3)) == 3.0 );
        REQUIRE( std::distance(L1.cbegin(), L1.cend()) == static_cast<size_t>(4) );
    }
}

TEST_CASE( "num_array other member functions" ){
    const auto inf = std::numeric_limits<double>::infinity();
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    const vec3<double> x_unit(1.0, 0.0, 0.0);
    const vec3<double> y_unit(0.0, 1.0, 0.0);
    const vec3<double> z_unit(0.0, 0.0, 1.0);

    SUBCASE("swap"){
        num_array<double> L1(10,10,1.0);
        num_array<double> L2(5,5,0.0);
        L1.swap(L2);

        REQUIRE(L1.num_rows() == 5);
        REQUIRE(L1.num_cols() == 5);
        REQUIRE(L1.read_coeff(0,0) == 0.0);

        REQUIRE(L2.num_rows() == 10);
        REQUIRE(L2.num_cols() == 10);
        REQUIRE(L2.read_coeff(0,0) == 1.0);
    }

    SUBCASE("num_array factory members" ){
        const auto L1 = num_array<double>().zero(3,3);
        for(long int row = 0; row < L1.num_rows(); ++row){
            for(long int col = 0; col < L1.num_cols(); ++col){
                REQUIRE( L1.read_coeff(row,col) == 0.0 );
            }
        }

        const auto L2 = num_array<double>().identity(3);
        for(long int row = 0; row < L1.num_rows(); ++row){
            for(long int col = 0; col < L1.num_cols(); ++col){
                if(row == col){
                    REQUIRE( L2.read_coeff(row,col) == 1.0 );
                }else{
                    REQUIRE( L2.read_coeff(row,col) == 0.0 );
                }
            }
        }

        const auto L3 = num_array<double>().iota(3,3,0.0);
        for(long int row = 0; row < L1.num_rows(); ++row){
            for(long int col = 0; col < L1.num_cols(); ++col){
                const auto expected_val = static_cast<double>( L3.index(row,col) );
                REQUIRE( L3.read_coeff(row,col) == expected_val );
            }
        }

    }

    SUBCASE("isnan and isfinite"){
        auto L1 = num_array<double>().identity(100);
        REQUIRE(!L1.isnan());
        REQUIRE(L1.isfinite());

        L1.coeff(23,46) = std::numeric_limits<double>::quiet_NaN();
        REQUIRE(L1.isnan());
        REQUIRE(!L1.isfinite());

        L1.coeff(23,46) = std::numeric_limits<double>::infinity();
        REQUIRE(!L1.isnan());
        REQUIRE(!L1.isfinite());
    }

    SUBCASE("trace"){
        const auto L1 = num_array<double>().identity(100);
        const auto L2 = num_array<double>().iota(3,3,0.0);
        const auto L3 = num_array<double>().iota(4,4,0.0);
        const auto L4 = num_array<double>().zero(10,10);
        const auto L5 = num_array<double>().iota(5,10,0.0);
        num_array<double> L6;

        REQUIRE(L1.trace() == 100.0);
        REQUIRE(L2.trace() == 12.0);
        REQUIRE(L3.trace() == 30.0);
        REQUIRE(L4.trace() == 0.0);
        REQUIRE_THROWS(L5.trace());
        REQUIRE_THROWS(L6.trace());
    }

    SUBCASE("transpose"){
        auto L1 = num_array<double>().identity(100);
        auto L2 = num_array<double>().zero(1,10);
        auto L3 = num_array<double>().iota(3,2,0.0);

        auto L4 = L1.transpose();
        auto L5 = L2.transpose();
        auto L6 = L3.transpose();

        REQUIRE(L1.transpose() == L1);
        REQUIRE(L1 == L4);

        REQUIRE(L5.num_rows() == 10);
        REQUIRE(L5.num_cols() == 1);

        REQUIRE(L6.read_coeff(0,0) == 0.0); //Assumes coloumn-major ordering.
        REQUIRE(L6.read_coeff(0,1) == 1.0);
        REQUIRE(L6.read_coeff(0,2) == 2.0);
        REQUIRE(L6.read_coeff(1,0) == 3.0);
        REQUIRE(L6.read_coeff(1,1) == 4.0);
        REQUIRE(L6.read_coeff(1,2) == 5.0);
        REQUIRE(L3.transpose().transpose() == L3);
        REQUIRE(L6.transpose() == L3);
    }

    SUBCASE("to_vec3"){
        auto A = x_unit.to_num_array().to_vec3();
        auto B = y_unit.to_num_array().to_vec3();
        auto C = z_unit.to_num_array().to_vec3();

        REQUIRE(A == x_unit);
        REQUIRE(B == y_unit);
        REQUIRE(C == z_unit);

        A = x_unit.to_num_array().transpose().to_vec3();
        B = y_unit.to_num_array().transpose().to_vec3();
        C = z_unit.to_num_array().transpose().to_vec3();

        REQUIRE(A == x_unit);
        REQUIRE(B == y_unit);
        REQUIRE(C == z_unit);

        const auto D = (x_unit * 1.0 + y_unit * 2.0 - z_unit * 3.0);
        const auto E = D.to_num_array().to_vec3();
        REQUIRE(D == E);

        const auto F = vec3<double>(1.0, eps, inf);
        const auto G = F.to_num_array().to_vec3();
        REQUIRE(F == G);

        REQUIRE_THROWS(num_array<double>(1,1).to_vec3());
        REQUIRE_THROWS(num_array<double>(3,2).to_vec3());
        REQUIRE_THROWS(num_array<double>(3,3).to_vec3());
        REQUIRE_THROWS(num_array<double>(4,1).to_vec3());
        REQUIRE_THROWS(num_array<double>(1,4).to_vec3());
    }

    SUBCASE("hnormalize_to_vec3"){
        auto A = x_unit.to_homogeneous_num_array().hnormalize_to_vec3();
        auto B = y_unit.to_homogeneous_num_array().hnormalize_to_vec3();
        auto C = z_unit.to_homogeneous_num_array().hnormalize_to_vec3();

        REQUIRE(A == x_unit);
        REQUIRE(B == y_unit);
        REQUIRE(C == z_unit);

        A = x_unit.to_homogeneous_num_array().transpose().hnormalize_to_vec3();
        B = y_unit.to_homogeneous_num_array().transpose().hnormalize_to_vec3();
        C = z_unit.to_homogeneous_num_array().transpose().hnormalize_to_vec3();

        REQUIRE(A == x_unit);
        REQUIRE(B == y_unit);
        REQUIRE(C == z_unit);

        const auto D = (x_unit * 1.0 + y_unit * 2.0 - z_unit * 3.0);
        const auto E = D.to_homogeneous_num_array().hnormalize_to_vec3();
        REQUIRE(D == E);

        const auto F = vec3<double>(-1.0, eps, inf);
        const auto G = F.to_homogeneous_num_array().hnormalize_to_vec3();
        REQUIRE(F == G);

        REQUIRE_THROWS(num_array<double>(1,1).hnormalize_to_vec3());
        REQUIRE_THROWS(num_array<double>(3,2).hnormalize_to_vec3());
        REQUIRE_THROWS(num_array<double>(3,3).hnormalize_to_vec3());
        REQUIRE_THROWS(num_array<double>(4,2).hnormalize_to_vec3());
        REQUIRE_THROWS(num_array<double>(4,4).hnormalize_to_vec3());
        REQUIRE_THROWS(num_array<double>(1,5).hnormalize_to_vec3());
        REQUIRE_THROWS(num_array<double>(5,1).hnormalize_to_vec3());

        auto H = A.to_homogeneous_num_array() * 2.0;
        REQUIRE(H.hnormalize_to_vec3() == A);

        auto J = A.to_homogeneous_num_array() * -73.0;
        REQUIRE(J.hnormalize_to_vec3() == A);
    }
}

TEST_CASE( "num_array write_to and read_from" ){
    SUBCASE("square matrices can be stringified exactly"){
        {
            const auto L1 = num_array<double>().identity(4);
            num_array<double> L2;
            std::stringstream ss;
            L1.write_to(ss);
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
        {
            const auto L1 = num_array<double>().zero(5,5);
            num_array<double> L2;
            std::stringstream ss;
            L1.write_to(ss);
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
        {
            auto L1 = num_array<double>().zero(5,5);
            L1.coeff(2,3) = std::numeric_limits<double>::quiet_NaN();
            L1.coeff(4,1) = -std::numeric_limits<double>::infinity();
            num_array<double> L2;
            std::stringstream ss1;
            L1.write_to(ss1);
            L2.read_from(ss1);
            const auto direct_equality = (L1 == L2); // Should fail due to NaN.
            REQUIRE(!direct_equality);

            std::stringstream ss2;
            std::stringstream ss3;
            L1.write_to(ss2);
            L2.write_to(ss3);
            REQUIRE(ss2.str() == ss3.str()); // Will properly handle NaNs.
        }
        {
            const auto L1 = num_array<double>().iota(3,3,0.0);
            num_array<double> L2;
            std::stringstream ss;
            L1.write_to(ss);
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
    }
    SUBCASE("rectangular matrices can be stringified exactly"){
        {
            const auto L1 = num_array<double>().zero(5,2);
            num_array<double> L2;
            std::stringstream ss;
            L1.write_to(ss);
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
        {
            auto L1 = num_array<double>().zero(5,2);
            L1.coeff(2,1) = std::numeric_limits<double>::quiet_NaN();
            L1.coeff(4,1) = -std::numeric_limits<double>::infinity();
            num_array<double> L2;
            std::stringstream ss1;
            L1.write_to(ss1);
            L2.read_from(ss1);
            const auto direct_equality = (L1 == L2); // Should fail due to NaN.
            REQUIRE(!direct_equality);

            std::stringstream ss2;
            std::stringstream ss3;
            L1.write_to(ss2);
            L2.write_to(ss3);
            REQUIRE(ss2.str() == ss3.str()); // Will properly handle NaNs.
        }
        {
            const auto L1 = num_array<double>().iota(3,5,0.0);
            std::stringstream ss;
            L1.write_to(ss);
            num_array<double> L2;
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
        {
            const auto L1 = num_array<double>().iota(5,3,0.0) / 1.0E30;
            std::stringstream ss;
            L1.write_to(ss);
            num_array<double> L2;
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
    }

    SUBCASE("stringification rejects invalid data"){
        {
            const auto L1 = num_array<double>().zero(5,2);
            num_array<double> L2;
            std::stringstream ss;
            ss << "-1 ";
            L1.write_to(ss);
            REQUIRE(!L2.read_from(ss));
        }
        {
            const auto L1 = num_array<double>().zero(5,2);
            num_array<double> L2;
            std::stringstream ss;
            ss << "5 -2 ";
            L1.write_to(ss);
            REQUIRE(!L2.read_from(ss));
        }
        {
            const auto L1 = num_array<double>().zero(5,2);
            num_array<double> L2;
            std::stringstream ss;
            ss << "}";
            L1.write_to(ss);
            REQUIRE(!L2.read_from(ss));
        }
    }

    SUBCASE("stringification ignores irrelevenat data"){
        {
            const auto L1 = num_array<double>().zero(5,2);
            num_array<double> L2;
            std::stringstream ss;
            L1.write_to(ss);
            L1.write_to(ss);
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
        {
            const auto L1 = num_array<double>().zero(5,2);
            num_array<double> L2;
            std::stringstream ss;
            L1.write_to(ss);
            ss << " 5.21 4.3 }";
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
    }
}

TEST_CASE( "num_array invert" ){
    const auto eps = 10.0 * std::sqrt( std::numeric_limits<double>::epsilon() );

    // Finds the maximum coefficient-wise deviation between two matrices.
    const auto max_diff = [](const num_array<double>& A,
                             const num_array<double>& B){
        if( (A.num_rows() != B.num_rows())
        ||  (A.num_cols() != B.num_cols() ) ){
            throw std::logic_error("Invalid comparison between matrices of differing sizes");
        }
        double max_diff = 0.0;
        for(long int r = 0; r < A.num_rows(); ++r){
            for(long int c = 0; c < B.num_rows(); ++c){
                const auto diff = std::abs(B.read_coeff(r,c) - A.read_coeff(r,c));
                if(diff < max_diff) max_diff = diff;
            }
        }
        return max_diff;
    };

    // The inverse should be an identity matrix too.
    SUBCASE( "1x1 identity" ){
        const auto A = num_array<double>().identity(1);
        const auto B = A.invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }
    SUBCASE( "2x2 identity" ){
        const auto A = num_array<double>().identity(2);
        const auto B = A.invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }
    SUBCASE( "3x3 identity" ){
        const auto A = num_array<double>().identity(3);
        const auto B = A.invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }
    SUBCASE( "4x4 identity" ){
        const auto A = num_array<double>().identity(4);
        const auto B = A.invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }

    // For orthonormal rotation matrices, the transpose should be the same as the inverse.
    SUBCASE( "3x3 inverse of rotation matrix is same as transpose" ){
        auto X = vec3<double>( 1.0, 2.0,-3.0 ).unit();
        auto Y = vec3<double>(-1.0, 2.0, 3.0 ).unit();
        auto Z = vec3<double>( 1.0,-2.0, 3.0 ).unit();
        X.GramSchmidt_orthogonalize(Y, Z);
        X = X.unit();
        Y = Y.unit();
        Z = Z.unit();

        auto A = num_array<double>().zero(3,3);
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
        const auto C = A.transpose();
        const auto md = max_diff(B, C);
        REQUIRE( md < eps );
    }

    // The inverse of an inverse should give the original matrix (iff the inverse exists).
    SUBCASE( "1x1 inverse is cyclic (arbitrary)" ){
        const auto A = num_array<double>().iota(1, 1, 1.0);
        const auto B = A.invert().invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }
    SUBCASE( "2x2 inverse is cyclic (arbitrary)" ){
        const auto A = num_array<double>().iota(2, 2, 1.0);
        const auto B = A.invert().invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }
    SUBCASE( "3x3 inverse is cyclic (arbitrary)" ){
        auto A = num_array<double>().iota(3, 3, 1.0); // is singular.
        A.coeff(2,2) = 1.0; // makes A non-singular.
        const auto B = A.invert().invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }
    SUBCASE( "4x4 inverse is cyclic (arbitrary)" ){
        auto A = num_array<double>().iota(4, 4, 1.0); // is singular.
        A.coeff(2,2) = 1.0; // makes A non-singular.
        A.coeff(3,3) = 1.0; // makes A non-singular.
        const auto B = A.invert().invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }

    // Singular matrices do not have a "regular" inverse.
    SUBCASE( "1x1 singular matrix inversion attempt throws" ){
        const auto A = num_array<double>().zero(1, 1);
        REQUIRE_THROWS(A.invert());
    }
    SUBCASE( "2x2 singular matrix inversion attempt throws" ){
        const auto A = num_array<double>().zero(2, 2);
        REQUIRE_THROWS(A.invert());
    }
    SUBCASE( "3x3 singular matrix inversion attempt throws" ){
        const auto A = num_array<double>().zero(3, 3);
        REQUIRE_THROWS(A.invert());
    }
    SUBCASE( "4x4 singular matrix inversion attempt throws" ){
        const auto A = num_array<double>().zero(4, 4);
        REQUIRE_THROWS(A.invert());
    }

    // Rectangular matrices do not have a "regular" inverse.
    SUBCASE( "1x2 rectangular matrix inversion attempt throws" ){
        const auto A = num_array<double>().iota(1, 2, 1.0);
        REQUIRE_THROWS(A.invert());
    }
    SUBCASE( "1x4 rectangular matrix inversion attempt throws" ){
        const auto A = num_array<double>().iota(1, 4, 1.0);
        REQUIRE_THROWS(A.invert());
    }
    SUBCASE( "4x2 rectangular matrix inversion attempt throws" ){
        const auto A = num_array<double>().iota(4, 2, 1.0);
        REQUIRE_THROWS(A.invert());
    }

    // Empty matrices do not have an inverse.
    SUBCASE( "empty matrix inversion attempt throws" ){
        const num_array<double> A;
        REQUIRE_THROWS(A.invert());
    }

}
