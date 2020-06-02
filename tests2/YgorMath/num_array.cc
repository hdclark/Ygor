
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
        for(long int rows = 0; rows < 10; rows += 3){
            for(long int cols = 0; cols < 10; cols += 3){
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
}

TEST_CASE( "num_array other member functions" ){
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
}

/*
    Currently outstanding:
        bool write_to( std::ostream &os ) const;
        bool read_from( std::istream &is );
*/

