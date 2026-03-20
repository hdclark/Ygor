
#include <limits>
#include <cmath>
#include <sstream>
#include <string>

#include <YgorMath.h>

#include "doctest/doctest.h"


TEST_CASE( "samples_1D metadata serialization" ){
    using namespace std::string_literals;

    samples_1D<double> buffa;
    for(double x = 0.0; x < 10.0; x += 0.15){
        const double f = std::sin(x) * std::exp(-x/2.0);
        const double df = f * 0.05;
        const double dx = x * 0.005;
        buffa.push_back(x, dx, f, df);
    }

    // Inject metadata.
    const auto key_A = "key_A";
    const auto val_A = "val_A";

    const auto key_B = "key\tB";
    const auto val_B = "val_B";

    const auto key_C = "key_C";
    const auto val_C = "val\tC";

    const auto key_D = " very long key D with nasty characters \t \n \0 "s;
    const auto val_D = " very long val D with nasty characters \t \n \0 "s;

    buffa.metadata[key_A] = val_A;
    buffa.metadata[key_B] = val_B;
    buffa.metadata[key_C] = val_C;
    buffa.metadata[key_D] = val_D;

    SUBCASE("operator<< and operator>> round-trip preserves metadata"){
        samples_1D<double> buffb;

        std::stringstream ss;
        ss << buffa;
        ss >> buffb;

        REQUIRE( buffb.metadata[key_A] == val_A );
        REQUIRE( buffb.metadata[key_B] == val_B );
        REQUIRE( buffb.metadata[key_C] == val_C );
        REQUIRE( buffb.metadata[key_D] == val_D );
    }

    SUBCASE("Write_To_Stream and Read_From_Stream round-trip preserves metadata"){
        samples_1D<double> buffb;

        std::stringstream ss;
        buffa.Write_To_Stream(ss);
        buffb.Read_From_Stream(ss);

        REQUIRE( buffb.metadata[key_A] == val_A );
        REQUIRE( buffb.metadata[key_B] == val_B );
        REQUIRE( buffb.metadata[key_C] == val_C );
        REQUIRE( buffb.metadata[key_D] == val_D );
    }
}


TEST_CASE( "samples_1D median" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("median of 6 elements"){
        samples_1D<double> test;
        test.push_back(  1.0, 0.0,  2.0, 0.0 );
        test.push_back(  2.0, 0.0, 14.0, 0.0 );
        test.push_back(  4.0, 0.0,  2.0, 0.0 );
        test.push_back(  7.0, 0.0,  5.0, 0.0 );
        test.push_back(  1.0, 0.0,  8.0, 0.0 );
        test.push_back(  6.0, 0.0,  1.0, 0.0 );

        // median of 1, 2, 4, 7, 1, 6  = 3
        // median of 2, 14, 2, 5, 8, 1 = 3.5
        REQUIRE( std::abs(test.Median_x()[0] - 3.0) < eps );
        REQUIRE( std::abs(test.Median_y()[0] - 3.5) < eps );
    }

    SUBCASE("median of 7 elements"){
        samples_1D<double> test;
        test.push_back(  1.0, 0.0,  2.0, 0.0 );
        test.push_back(  2.0, 0.0, 14.0, 0.0 );
        test.push_back(  4.0, 0.0,  2.0, 0.0 );
        test.push_back( 53.0, 0.0, 53.0, 0.0 );
        test.push_back(  7.0, 0.0,  5.0, 0.0 );
        test.push_back(  1.0, 0.0,  8.0, 0.0 );
        test.push_back(  6.0, 0.0,  1.0, 0.0 );

        // median of 1, 2, 4, 53, 7, 1, 6  = 4
        // median of 2, 14, 2, 53, 5, 8, 1 = 5
        REQUIRE( std::abs(test.Median_x()[0] - 4.0) < eps );
        REQUIRE( std::abs(test.Median_y()[0] - 5.0) < eps );
    }
}
