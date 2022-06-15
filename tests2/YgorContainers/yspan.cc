
#include <array>
#include <numeric>
#include <vector>
#include <limits>
#include <utility>
#include <iostream>

#include <YgorMath.h>
#include <YgorContainers.h>

#include "doctest/doctest.h"


TEST_CASE( "yspan constructors" ){
    std::array<int32_t, 10> a;
    std::iota( std::begin(a), std::end(a), 0 );

    SUBCASE("default constructor produces an empty span"){
        yspan<int32_t> s;
        REQUIRE(s.size() == 0);
        REQUIRE(s.stride() == 0);
    }

    SUBCASE("constructor refuses invalid dimensions"){
        REQUIRE_THROWS( yspan<int32_t>( nullptr, 1, 1) );
        REQUIRE_THROWS( yspan<int32_t>( &a[0], -1, 1) );
        REQUIRE_THROWS( yspan<int32_t>( &a[0], 1, -1) );
    }

    SUBCASE("constructor correctly constructs"){
        yspan<int32_t> A(&a[0], 2, 3);
        REQUIRE(A.size() == 2);
        REQUIRE(A.stride() == 3);
    }
}

TEST_CASE( "yspan accessors" ){
    std::array<int64_t, 50> a;
    std::iota( std::begin(a), std::end(a), 100L );

    SUBCASE("at() with homogeneous array and without stride"){
        yspan<int64_t> s(&a[0], a.size(), 0);

        REQUIRE_THROWS(s.at(-1));
        REQUIRE(s.at(0) == 100L);
        REQUIRE(s.at(1) == 101L);
        REQUIRE(s.at(49) == 149L);
        REQUIRE_THROWS(s.at(50));
    }
    SUBCASE("at() with homogeneous array and with stride"){
        // Skip every other number.
        yspan<int64_t> s1(&a[0], a.size()/2, sizeof(int64_t));

        REQUIRE_THROWS(s1.at(-1));
        REQUIRE(s1.at(0) == 100L);
        REQUIRE(s1.at(1) == 102L);
        REQUIRE(s1.at(24) == 148L);
        REQUIRE_THROWS(s1.at(25));

        // Skip to every fifth number.
        yspan<int64_t> s2(&a[0], a.size()/5, sizeof(int64_t) * 4);

        REQUIRE_THROWS(s2.at(-1));
        REQUIRE(s2.at(0) == 100L);
        REQUIRE(s2.at(1) == 105L);
        REQUIRE(s2.at(9) == 145L);
        REQUIRE_THROWS(s2.at(10));
    }

    struct comp_t {
        int32_t a;
        float b;
        double c;
        int64_t d;
    };
    std::vector<comp_t> comp;
    for(size_t i = 0; i < 50; ++i){
        comp.emplace_back();
        comp.back().a = 100L + i;
        comp.back().b = 1'000.0f + i;
        comp.back().c = 10'000.0 + i;
        comp.back().d = 100'000L + i;
    }

    SUBCASE("at() with composite array"){
        // "a" element using stride from only sizeof().
        const long int a_stride_1 = sizeof(comp_t) - sizeof(decltype(comp[0].a));
        yspan<int32_t> sa1(&comp[0].a, comp.size(), a_stride_1);

        REQUIRE_THROWS(sa1.at(-1));
        REQUIRE(sa1.at(0) == 100L);
        REQUIRE(sa1.at(1) == 101L);
        REQUIRE(sa1.at(49) == 149L);
        REQUIRE_THROWS(sa1.at(50));

        // "a" element using stride computed from difference of pointers.
        const long int a_stride_2 = static_cast<long int>(
                                      reinterpret_cast<std::uintptr_t>(std::addressof(comp[1].a))
                                      - reinterpret_cast<std::uintptr_t>(std::addressof(comp[0].a)) )
                                  - sizeof(decltype(comp[0].a));
        yspan<int32_t> sa2(&comp[0].a, comp.size(), a_stride_2);

        REQUIRE_THROWS(sa2.at(-1));
        REQUIRE(sa2.at(0) == 100L);
        REQUIRE(sa2.at(1) == 101L);
        REQUIRE(sa2.at(49) == 149L);
        REQUIRE_THROWS(sa2.at(50));

        // "b" element using stride from only sizeof().
        const long int b_stride_1 = sizeof(comp_t) - sizeof(decltype(comp[0].b));
        yspan<float> sb1(&comp[0].b, comp.size(), b_stride_1);

        REQUIRE_THROWS(sb1.at(-1));
        REQUIRE(sb1.at(0) == 1'000.0f);
        REQUIRE(sb1.at(1) == 1'001.0f);
        REQUIRE(sb1.at(49) == 1'049.0f);
        REQUIRE_THROWS(sb1.at(50));

        // "c" element using stride computed from difference of pointers and skipping every other element.
        const long int c_stride_1 = static_cast<long int>(
                                      reinterpret_cast<std::uintptr_t>(std::addressof(comp[2].c))
                                      - reinterpret_cast<std::uintptr_t>(std::addressof(comp[0].c)) )
                                  - sizeof(decltype(comp[0].c));
        yspan<double> sc1(&comp[0].c, comp.size()/2, c_stride_1);

        REQUIRE_THROWS(sc1.at(-1));
        REQUIRE(sc1.at(0) == 10'000.0);
        REQUIRE(sc1.at(1) == 10'002.0);
        REQUIRE(sc1.at(24) == 10'048.0);
        REQUIRE_THROWS(sc1.at(25));

        // "d" element using stride from only sizeof() and skipping to every fifth element.
        const long int d_stride_1 = sizeof(comp_t) * 5L - sizeof(decltype(comp[0].d));
        yspan<int64_t> sd1(&comp[0].d, comp.size()/5, d_stride_1);

        REQUIRE_THROWS(sd1.at(-1));
        REQUIRE(sd1.at(0) == 100'000L);
        REQUIRE(sd1.at(1) == 100'005L);
        REQUIRE(sd1.at(9) == 100'045L);
        REQUIRE_THROWS(sd1.at(10));
    }

    SUBCASE("size"){
        yspan<int64_t> s1(&a[0], 10L, 4);
        REQUIRE(s1.size() == 10L);

        yspan<int64_t> s2(&a[0], 5L, 8);
        REQUIRE(s2.size() == 5L);
    }

    SUBCASE("stride"){
        const auto stride_1 = sizeof(int64_t);
        yspan<int64_t> s1(&a[0], 10L, stride_1);
        REQUIRE(s1.stride() == stride_1);

        const auto stride_2 = sizeof(int64_t) * 4;
        yspan<int64_t> s2(&a[0], 5L, stride_2);
        REQUIRE(s2.stride() == stride_2);
    }

    SUBCASE("writes via at() appear in the underlynig storage"){
        yspan<int64_t> s(&a[0], a.size(), 0);

        REQUIRE(a.at(1) == 101L);
        REQUIRE(s.at(1) == 101L);
        s.at(1) = 123L;
        REQUIRE(a.at(1) == 123L);
        REQUIRE(s.at(1) == 123L);
    }
}

TEST_CASE( "yspan iterators" ){
    std::array<int64_t, 50> a;
    std::iota( std::begin(a), std::end(a), 100L );
    yspan<int64_t> s1(&a[0], a.size(), 0);
    yspan<int64_t> s2(&a[0], a.size()/2, sizeof(int64_t));

    SUBCASE("begin() is visible in global scope"){
        // This is mostly a compile-time check...
        auto it1 = s1.begin();
        auto it2 = std::begin(s1);
        REQUIRE(it1 == it2);
    }

    SUBCASE("end() is visible in global scope"){
        // This is mostly a compile-time check...
        auto it1 = s1.end();
        auto it2 = std::end(s1);
        REQUIRE(it1 == it2);
    }

    SUBCASE("iterator begin() and end() span all elements"){
        auto it1 = s1.begin();
        for(long int i = 0; i < 50; ++i) ++it1;
        REQUIRE(it1 == s1.end());

        for(long int i = 0; i < 50; ++i) --it1;
        REQUIRE(it1 == s1.begin());
    }

    SUBCASE("iterator std::distance() works (operator-)"){
        const auto N_elems_1 = std::distance(s1.begin(), s1.end());
        REQUIRE(N_elems_1 == 50UL);

        const auto N_elems_2 = std::distance(s2.begin(), s2.end());
        REQUIRE(N_elems_2 == 25UL);
    }

    SUBCASE("iterator std::next() and std::prev() work (operator+=)"){
        const auto it1 = std::next( s1.begin(), 50L);
        REQUIRE(it1 == s1.end());

        const auto it2 = std::prev( it1, 50L );
        REQUIRE(it2 == s1.begin());
    }

    SUBCASE("iterator with homogeneous array and without stride"){
        auto it1 = s1.begin();

        REQUIRE(*it1 == 100L);
        ++it1;
        REQUIRE(*it1 == 101L);
        it1++;
        REQUIRE(*it1++ == 102L);
        REQUIRE(*it1 == 103L);

        --it1;
        REQUIRE(*it1 == 102L);
        it1--;
        REQUIRE(*it1-- == 101L);
        REQUIRE(*it1 == 100L);
    }

    SUBCASE("iterator with homogeneous array and with stride"){
        auto it2 = s2.begin();

        REQUIRE(*it2 == 100L);
        ++it2;
        REQUIRE(*it2 == 102L);
        it2++;
        REQUIRE(*it2++ == 104L);
        REQUIRE(*it2 == 106L);

        --it2;
        REQUIRE(*it2 == 104L);
        it2--;
        REQUIRE(*it2-- == 102L);
        REQUIRE(*it2 == 100L);
    }

    struct comp_t {
        int32_t a;
        float b;
        double c;
        int64_t d;
    };
    std::vector<comp_t> comp;
    for(size_t i = 0; i < 50; ++i){
        comp.emplace_back();
        comp.back().a = 100L + i;
        comp.back().b = 1'000.0f + i;
        comp.back().c = 10'000.0 + i;
        comp.back().d = 100'000L + i;
    }
    const long int a_stride = sizeof(comp_t) - sizeof(decltype(comp[0].a));
    yspan<int32_t> s3(&comp[0].a, comp.size(), a_stride);

    SUBCASE("iterator with heterogeneous array and with stride"){
        auto it3 = s3.begin();

        REQUIRE(*it3 == 100L);
        ++it3;
        REQUIRE(*it3 == 101L);
        it3++;
        REQUIRE(*it3++ == 102L);
        REQUIRE(*it3 == 103L);

        --it3;
        REQUIRE(*it3 == 102L);
        it3--;
        REQUIRE(*it3-- == 101L);
        REQUIRE(*it3 == 100L);
    }

}

TEST_CASE( "yspan const iterators" ){
    std::vector<uint16_t> v1;
    for(uint16_t i = 0U; i < 100U; ++i) v1.push_back(i);

    const auto v2 = v1; // make const vector.
    //yspan<uint16_t> s1(v2.data(), v2.size(), 0L); // error, because const disappears.

    yspan<const uint16_t> s2(v2.data(), v2.size(), 0L); // ok, because const propagates.
    auto it2_1 = std::begin(s2);
    auto it2_2 = s2.begin();
    REQUIRE(*it2_1 == *it2_2);
}


/*
TEST_CASE( "yspan assignment operators" ){
    SUBCASE("operator="){
        yspan<double> L1(10,10);
        L1.coeff(0,0) = 2.0;
        yspan<double> L2;
        REQUIRE( L1 != L2 );
        L2 = L1;
        REQUIRE( L1 == L2 );
        L2 = L2;
        REQUIRE( L1 == L2 );
        L2.coeff(0,0) = 1.0;
        REQUIRE( L1 != L2 );
        L2 = yspan<double>(5,5);
        REQUIRE( L1 != L2 );
        L1 = yspan<double>(5,5);
        REQUIRE( L1 == L2 );
    }
}

TEST_CASE( "yspan comparison operators" ){
    SUBCASE("operator== and operator!="){
        // Finite case.
        {
            yspan<double> L1(10,10);
            yspan<double> L2(L1);
            REQUIRE( L1 == L1 );
            REQUIRE( L1 == L2 );

            for(long int row = 0; row < 10; ++row){
                for(long int col = 0; col < 10; ++col){
                    yspan<double> L3(L1);
                    REQUIRE( L1 == L3 );
                    L3.coeff(row,col) = 2.0;
                    REQUIRE( L1 != L3 );
                }
            }

            yspan<double> L4(5,20);
            REQUIRE( L1 != L4 );
            yspan<double> L5(20,5);
            REQUIRE( L1 != L5 );
            REQUIRE( L4 != L5 );
        }

        // Non-finite case.
        {
            auto L1 = yspan<double>().zero(5,2);
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
        yspan<double> L1(10,10);
        L1.coeff(0,0) = 2.0;
        yspan<double> L2;
        yspan<double> L3 = L1;
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

TEST_CASE( "yspan scalar operators" ){
    SUBCASE("operator* and operator/"){
        yspan<double> L1(10,10,1.0);
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
        yspan<double> L1(10,10,1.0);
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

TEST_CASE( "yspan matrix operators" ){
    SUBCASE("operator+"){
        yspan<double> L1(3,3);
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

        yspan<double> L5(3,2);
        yspan<double> L6(2,3);
        yspan<double> L7(2,2);
        REQUIRE_THROWS(L1 + L5);
        REQUIRE_THROWS(L1 + L6);
        REQUIRE_THROWS(L1 + L7);
    }

    SUBCASE("operator-"){
        yspan<double> L1(3,3);
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

        yspan<double> L5(3,2);
        yspan<double> L6(2,3);
        yspan<double> L7(2,2);
        REQUIRE_THROWS(L1 - L5);
        REQUIRE_THROWS(L1 - L6);
        REQUIRE_THROWS(L1 - L7);
    }

    SUBCASE("operator*"){
        // Example 1: an out-of-band generic example worked out by hand.
        {
            yspan<double> L1(2,2);
            L1.coeff(0,0) =  2.0;
            L1.coeff(0,1) =  5.0;
            L1.coeff(1,0) =  7.0;
            L1.coeff(1,1) = 11.0;

            yspan<double> L2(2,1);
            L2.coeff(0,0) = 23.0;
            L2.coeff(1,0) = 57.0;

            auto L3 = L1 * L2;
            REQUIRE( L3.coeff(0,0) == 331.0 );
            REQUIRE( L3.coeff(1,0) == 788.0 );

            REQUIRE_THROWS(L2 * L1);
        }

        // Example 2: identity matrix multiplication.
        {
            const auto L1 = yspan<double>().identity(100);
            REQUIRE( L1 == (L1 * L1 * L1));

            const auto L2 = yspan<double>().iota(100, 100, 0.0);
            const auto L3 = yspan<double>().iota(100, 100, -50.0);
            REQUIRE( L2 == L1 * L2);
            REQUIRE( L2 == L2 * L1);
            REQUIRE( L2 == L1 * L2 * L1);
            REQUIRE( L3 == L1 * L3);
        }

        // Example 3: square matrix using iota.
        //
        // Note: assumes iota is column-major (i.e., fills columns fully before moving to next row).
        {
            const auto L1 = yspan<double>().iota(3,3,0.0);
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
            const auto L1 = yspan<double>().iota(2,3,0.0);
            const auto L2 = yspan<double>().iota(3,2,7.0);

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
            yspan<double> L1(3, 2, 1.0);
            yspan<double> L2(2, 3, 2.0);
            yspan<double> L3(2, 4, 3.0);
            yspan<double> L4(4, 2, 4.0);

            REQUIRE_THROWS(L1 * L1);
            REQUIRE_THROWS(L1 * L2 * L2);
            REQUIRE_THROWS(L1 * L3 * L4 * L1);
        }
    }

    SUBCASE("operator+="){
        yspan<double> L1(3,3);
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

        yspan<double> L5(3,2);
        yspan<double> L6(2,3);
        yspan<double> L7(2,2);
        REQUIRE_THROWS(L1 += L5);
        REQUIRE_THROWS(L1 += L6);
        REQUIRE_THROWS(L1 += L7);
    }

    SUBCASE("operator-="){
        yspan<double> L1(3,3);
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

        yspan<double> L5(3,2);
        yspan<double> L6(2,3);
        yspan<double> L7(2,2);
        REQUIRE_THROWS(L1 -= L5);
        REQUIRE_THROWS(L1 -= L6);
        REQUIRE_THROWS(L1 -= L7);
    }

    SUBCASE("operator*"){
        // Example 1: an out-of-band generic example worked out by hand.
        {
            yspan<double> L1(2,2);
            L1.coeff(0,0) =  2.0;
            L1.coeff(0,1) =  5.0;
            L1.coeff(1,0) =  7.0;
            L1.coeff(1,1) = 11.0;

            yspan<double> L2(2,1);
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
            const auto L1 = yspan<double>().identity(100);
            const auto R1 = (L1 * L1) *= L1;
            REQUIRE( L1 == R1 );

            const auto L2 = yspan<double>().iota(100, 100, 0.0);
            const auto L3 = yspan<double>().iota(100, 100, -50.0);
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
            const auto L1 = yspan<double>().iota(3,3,0.0);
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
            const auto L1 = yspan<double>().iota(2,3,0.0);
            const auto L2 = yspan<double>().iota(3,2,7.0);

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
            yspan<double> L1(3, 2, 1.0);
            yspan<double> L2(2, 3, 2.0);
            yspan<double> L3(2, 4, 3.0);
            yspan<double> L4(4, 2, 4.0);

            REQUIRE_THROWS((L1 * L2) *= L2);
            REQUIRE_THROWS((L1 * L3 * L4) *= L1);
        }
    }
}

TEST_CASE( "yspan accessors" ){
    SUBCASE("index"){
        const auto L1 = yspan<double>().iota(3,3,0.0);
        REQUIRE(L1.index(0,0) == 0);
        REQUIRE(L1.index(2,0) == 2); // Assumes column-major ordering.
        REQUIRE(L1.index(0,2) == 6); // Assumes column-major ordering.
    }

    SUBCASE("num_rows and num_cols"){
        const auto L1 = yspan<double>().iota(3,4,0.0);
        REQUIRE(L1.num_rows() == 3);
        REQUIRE(L1.num_cols() == 4);

        yspan<double> L2(4,9);
        L2 = L1 * L2;
        REQUIRE(L2.num_rows() == 3);
        REQUIRE(L2.num_cols() == 9);
    }

    SUBCASE("size"){
        yspan<double> L1;
        REQUIRE(L1.size() == 0);

        L1 = yspan<double>().iota(3,4,0.0);
        REQUIRE(L1.size() == 12);

        L1 = yspan<double>().iota(4,3,0.0);
        REQUIRE(L1.size() == 12);
    }

    SUBCASE("coeff and read_coeff"){
        auto L1 = yspan<double>().iota(3,3,0.0);

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
        auto L1 = yspan<double>().iota(2,2,0.0);

        REQUIRE( *(std::next(L1.begin(), 0)) == 0.0 );
        REQUIRE( *(std::next(L1.begin(), 1)) == 1.0 );
        REQUIRE( *(std::next(L1.begin(), 2)) == 2.0 );
        REQUIRE( *(std::next(L1.begin(), 3)) == 3.0 );
        REQUIRE( std::distance(L1.begin(), L1.end()) == static_cast<size_t>(4) );
    }

    SUBCASE("cbegin and cend"){
        auto L1 = yspan<double>().iota(2,2,0.0);

        REQUIRE( *(std::next(L1.cbegin(), 0)) == 0.0 );
        REQUIRE( *(std::next(L1.cbegin(), 1)) == 1.0 );
        REQUIRE( *(std::next(L1.cbegin(), 2)) == 2.0 );
        REQUIRE( *(std::next(L1.cbegin(), 3)) == 3.0 );
        REQUIRE( std::distance(L1.cbegin(), L1.cend()) == static_cast<size_t>(4) );
    }
}

TEST_CASE( "yspan other member functions" ){
    const auto inf = std::numeric_limits<double>::infinity();
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    const vec3<double> x_unit(1.0, 0.0, 0.0);
    const vec3<double> y_unit(0.0, 1.0, 0.0);
    const vec3<double> z_unit(0.0, 0.0, 1.0);

    SUBCASE("swap"){
        yspan<double> L1(10,10,1.0);
        yspan<double> L2(5,5,0.0);
        L1.swap(L2);

        REQUIRE(L1.num_rows() == 5);
        REQUIRE(L1.num_cols() == 5);
        REQUIRE(L1.read_coeff(0,0) == 0.0);

        REQUIRE(L2.num_rows() == 10);
        REQUIRE(L2.num_cols() == 10);
        REQUIRE(L2.read_coeff(0,0) == 1.0);
    }

    SUBCASE("yspan factory members" ){
        const auto L1 = yspan<double>().zero(3,3);
        for(long int row = 0; row < L1.num_rows(); ++row){
            for(long int col = 0; col < L1.num_cols(); ++col){
                REQUIRE( L1.read_coeff(row,col) == 0.0 );
            }
        }

        const auto L2 = yspan<double>().identity(3);
        for(long int row = 0; row < L1.num_rows(); ++row){
            for(long int col = 0; col < L1.num_cols(); ++col){
                if(row == col){
                    REQUIRE( L2.read_coeff(row,col) == 1.0 );
                }else{
                    REQUIRE( L2.read_coeff(row,col) == 0.0 );
                }
            }
        }

        const auto L3 = yspan<double>().iota(3,3,0.0);
        for(long int row = 0; row < L1.num_rows(); ++row){
            for(long int col = 0; col < L1.num_cols(); ++col){
                const auto expected_val = static_cast<double>( L3.index(row,col) );
                REQUIRE( L3.read_coeff(row,col) == expected_val );
            }
        }

    }

    SUBCASE("isnan and isfinite"){
        auto L1 = yspan<double>().identity(100);
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
        const auto L1 = yspan<double>().identity(100);
        const auto L2 = yspan<double>().iota(3,3,0.0);
        const auto L3 = yspan<double>().iota(4,4,0.0);
        const auto L4 = yspan<double>().zero(10,10);
        const auto L5 = yspan<double>().iota(5,10,0.0);
        yspan<double> L6;

        REQUIRE(L1.trace() == 100.0);
        REQUIRE(L2.trace() == 12.0);
        REQUIRE(L3.trace() == 30.0);
        REQUIRE(L4.trace() == 0.0);
        REQUIRE_THROWS(L5.trace());
        REQUIRE_THROWS(L6.trace());
    }

    SUBCASE("transpose"){
        auto L1 = yspan<double>().identity(100);
        auto L2 = yspan<double>().zero(1,10);
        auto L3 = yspan<double>().iota(3,2,0.0);

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
        auto A = x_unit.to_yspan().to_vec3();
        auto B = y_unit.to_yspan().to_vec3();
        auto C = z_unit.to_yspan().to_vec3();

        REQUIRE(A == x_unit);
        REQUIRE(B == y_unit);
        REQUIRE(C == z_unit);

        A = x_unit.to_yspan().transpose().to_vec3();
        B = y_unit.to_yspan().transpose().to_vec3();
        C = z_unit.to_yspan().transpose().to_vec3();

        REQUIRE(A == x_unit);
        REQUIRE(B == y_unit);
        REQUIRE(C == z_unit);

        const auto D = (x_unit * 1.0 + y_unit * 2.0 - z_unit * 3.0);
        const auto E = D.to_yspan().to_vec3();
        REQUIRE(D == E);

        const auto F = vec3<double>(1.0, eps, inf);
        const auto G = F.to_yspan().to_vec3();
        REQUIRE(F == G);

        REQUIRE_THROWS(yspan<double>(1,1).to_vec3());
        REQUIRE_THROWS(yspan<double>(3,2).to_vec3());
        REQUIRE_THROWS(yspan<double>(3,3).to_vec3());
        REQUIRE_THROWS(yspan<double>(4,1).to_vec3());
        REQUIRE_THROWS(yspan<double>(1,4).to_vec3());
    }

    SUBCASE("hnormalize_to_vec3"){
        auto A = x_unit.to_homogeneous_yspan().hnormalize_to_vec3();
        auto B = y_unit.to_homogeneous_yspan().hnormalize_to_vec3();
        auto C = z_unit.to_homogeneous_yspan().hnormalize_to_vec3();

        REQUIRE(A == x_unit);
        REQUIRE(B == y_unit);
        REQUIRE(C == z_unit);

        A = x_unit.to_homogeneous_yspan().transpose().hnormalize_to_vec3();
        B = y_unit.to_homogeneous_yspan().transpose().hnormalize_to_vec3();
        C = z_unit.to_homogeneous_yspan().transpose().hnormalize_to_vec3();

        REQUIRE(A == x_unit);
        REQUIRE(B == y_unit);
        REQUIRE(C == z_unit);

        const auto D = (x_unit * 1.0 + y_unit * 2.0 - z_unit * 3.0);
        const auto E = D.to_homogeneous_yspan().hnormalize_to_vec3();
        REQUIRE(D == E);

        const auto F = vec3<double>(-1.0, eps, inf);
        const auto G = F.to_homogeneous_yspan().hnormalize_to_vec3();
        REQUIRE(F == G);

        REQUIRE_THROWS(yspan<double>(1,1).hnormalize_to_vec3());
        REQUIRE_THROWS(yspan<double>(3,2).hnormalize_to_vec3());
        REQUIRE_THROWS(yspan<double>(3,3).hnormalize_to_vec3());
        REQUIRE_THROWS(yspan<double>(4,2).hnormalize_to_vec3());
        REQUIRE_THROWS(yspan<double>(4,4).hnormalize_to_vec3());
        REQUIRE_THROWS(yspan<double>(1,5).hnormalize_to_vec3());
        REQUIRE_THROWS(yspan<double>(5,1).hnormalize_to_vec3());

        auto H = A.to_homogeneous_yspan() * 2.0;
        REQUIRE(H.hnormalize_to_vec3() == A);

        auto J = A.to_homogeneous_yspan() * -73.0;
        REQUIRE(J.hnormalize_to_vec3() == A);
    }
}

TEST_CASE( "yspan write_to and read_from" ){
    SUBCASE("square matrices can be stringified exactly"){
        {
            const auto L1 = yspan<double>().identity(4);
            yspan<double> L2;
            std::stringstream ss;
            L1.write_to(ss);
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
        {
            const auto L1 = yspan<double>().zero(5,5);
            yspan<double> L2;
            std::stringstream ss;
            L1.write_to(ss);
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
        {
            auto L1 = yspan<double>().zero(5,5);
            L1.coeff(2,3) = std::numeric_limits<double>::quiet_NaN();
            L1.coeff(4,1) = -std::numeric_limits<double>::infinity();
            yspan<double> L2;
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
            const auto L1 = yspan<double>().iota(3,3,0.0);
            yspan<double> L2;
            std::stringstream ss;
            L1.write_to(ss);
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
    }
    SUBCASE("rectangular matrices can be stringified exactly"){
        {
            const auto L1 = yspan<double>().zero(5,2);
            yspan<double> L2;
            std::stringstream ss;
            L1.write_to(ss);
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
        {
            auto L1 = yspan<double>().zero(5,2);
            L1.coeff(2,1) = std::numeric_limits<double>::quiet_NaN();
            L1.coeff(4,1) = -std::numeric_limits<double>::infinity();
            yspan<double> L2;
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
            const auto L1 = yspan<double>().iota(3,5,0.0);
            std::stringstream ss;
            L1.write_to(ss);
            yspan<double> L2;
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
        {
            const auto L1 = yspan<double>().iota(5,3,0.0) / 1.0E30;
            std::stringstream ss;
            L1.write_to(ss);
            yspan<double> L2;
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
    }

    SUBCASE("stringification rejects invalid data"){
        {
            const auto L1 = yspan<double>().zero(5,2);
            yspan<double> L2;
            std::stringstream ss;
            ss << "-1 ";
            L1.write_to(ss);
            REQUIRE(!L2.read_from(ss));
        }
        {
            const auto L1 = yspan<double>().zero(5,2);
            yspan<double> L2;
            std::stringstream ss;
            ss << "5 -2 ";
            L1.write_to(ss);
            REQUIRE(!L2.read_from(ss));
        }
        {
            const auto L1 = yspan<double>().zero(5,2);
            yspan<double> L2;
            std::stringstream ss;
            ss << "}";
            L1.write_to(ss);
            REQUIRE(!L2.read_from(ss));
        }
    }

    SUBCASE("stringification ignores irrelevenat data"){
        {
            const auto L1 = yspan<double>().zero(5,2);
            yspan<double> L2;
            std::stringstream ss;
            L1.write_to(ss);
            L1.write_to(ss);
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
        {
            const auto L1 = yspan<double>().zero(5,2);
            yspan<double> L2;
            std::stringstream ss;
            L1.write_to(ss);
            ss << " 5.21 4.3 }";
            L2.read_from(ss);
            REQUIRE(L1 == L2);
        }
    }
}

TEST_CASE( "yspan invert" ){
    const auto eps = 10.0 * std::sqrt( std::numeric_limits<double>::epsilon() );

    // Finds the maximum coefficient-wise deviation between two matrices.
    const auto max_diff = [](const yspan<double>& A,
                             const yspan<double>& B){
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
        const auto A = yspan<double>().identity(1);
        const auto B = A.invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }
    SUBCASE( "2x2 identity" ){
        const auto A = yspan<double>().identity(2);
        const auto B = A.invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }
    SUBCASE( "3x3 identity" ){
        const auto A = yspan<double>().identity(3);
        const auto B = A.invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }
    SUBCASE( "4x4 identity" ){
        const auto A = yspan<double>().identity(4);
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

        auto A = yspan<double>().zero(3,3);
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
        const auto A = yspan<double>().iota(1, 1, 1.0);
        const auto B = A.invert().invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }
    SUBCASE( "2x2 inverse is cyclic (arbitrary)" ){
        const auto A = yspan<double>().iota(2, 2, 1.0);
        const auto B = A.invert().invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }
    SUBCASE( "3x3 inverse is cyclic (arbitrary)" ){
        auto A = yspan<double>().iota(3, 3, 1.0); // is singular.
        A.coeff(2,2) = 1.0; // makes A non-singular.
        const auto B = A.invert().invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }
    SUBCASE( "4x4 inverse is cyclic (arbitrary)" ){
        auto A = yspan<double>().iota(4, 4, 1.0); // is singular.
        A.coeff(2,2) = 1.0; // makes A non-singular.
        A.coeff(3,3) = 1.0; // makes A non-singular.
        const auto B = A.invert().invert();
        const auto md = max_diff(A, B);
        REQUIRE( md < eps );
    }

    // Singular matrices do not have a "regular" inverse.
    SUBCASE( "1x1 singular matrix inversion attempt throws" ){
        const auto A = yspan<double>().zero(1, 1);
        REQUIRE_THROWS(A.invert());
    }
    SUBCASE( "2x2 singular matrix inversion attempt throws" ){
        const auto A = yspan<double>().zero(2, 2);
        REQUIRE_THROWS(A.invert());
    }
    SUBCASE( "3x3 singular matrix inversion attempt throws" ){
        const auto A = yspan<double>().zero(3, 3);
        REQUIRE_THROWS(A.invert());
    }
    SUBCASE( "4x4 singular matrix inversion attempt throws" ){
        const auto A = yspan<double>().zero(4, 4);
        REQUIRE_THROWS(A.invert());
    }

    // Rectangular matrices do not have a "regular" inverse.
    SUBCASE( "1x2 rectangular matrix inversion attempt throws" ){
        const auto A = yspan<double>().iota(1, 2, 1.0);
        REQUIRE_THROWS(A.invert());
    }
    SUBCASE( "1x4 rectangular matrix inversion attempt throws" ){
        const auto A = yspan<double>().iota(1, 4, 1.0);
        REQUIRE_THROWS(A.invert());
    }
    SUBCASE( "4x2 rectangular matrix inversion attempt throws" ){
        const auto A = yspan<double>().iota(4, 2, 1.0);
        REQUIRE_THROWS(A.invert());
    }

    // Empty matrices do not have an inverse.
    SUBCASE( "empty matrix inversion attempt throws" ){
        const yspan<double> A;
        REQUIRE_THROWS(A.invert());
    }

}
*/
