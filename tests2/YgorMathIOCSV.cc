
#include <cmath>
#include <limits>
#include <sstream>
#include <string>

#include <YgorMath.h>
#include <YgorMathIOCSV.h>

#include "doctest/doctest.h"


TEST_CASE( "ReadNumArrayFromCSV basic CSV" ){
    std::stringstream ss("1.0,2.0,3.0\n4.0,5.0,6.0\n");

    auto result = ReadNumArrayFromCSV<double>(ss);
    REQUIRE( result.data.num_rows() == 2 );
    REQUIRE( result.data.num_cols() == 3 );
    REQUIRE( result.data.read_coeff(0, 0) == doctest::Approx(1.0) );
    REQUIRE( result.data.read_coeff(0, 2) == doctest::Approx(3.0) );
    REQUIRE( result.data.read_coeff(1, 0) == doctest::Approx(4.0) );
    REQUIRE( result.data.read_coeff(1, 2) == doctest::Approx(6.0) );
}


TEST_CASE( "ReadNumArrayFromCSV TSV auto-detection" ){
    std::stringstream ss("1.0\t2.0\t3.0\n4.0\t5.0\t6.0\n");

    auto result = ReadNumArrayFromCSV<double>(ss);
    REQUIRE( result.data.num_rows() == 2 );
    REQUIRE( result.data.num_cols() == 3 );
    REQUIRE( result.data.read_coeff(0, 0) == doctest::Approx(1.0) );
    REQUIRE( result.data.read_coeff(1, 2) == doctest::Approx(6.0) );
}


TEST_CASE( "ReadNumArrayFromCSV with header" ){
    std::stringstream ss("x,y,z\n1.0,2.0,3.0\n4.0,5.0,6.0\n");

    auto result = ReadNumArrayFromCSV<double>(ss, true);
    REQUIRE( result.data.num_rows() == 2 );
    REQUIRE( result.data.num_cols() == 3 );
    REQUIRE( result.data.read_coeff(0, 0) == doctest::Approx(1.0) );
}


TEST_CASE( "ReadNumArrayFromCSV NaN handling" ){
    std::stringstream ss("1.0,nan,NaN\n,NAN,2.0\n");

    auto result = ReadNumArrayFromCSV<double>(ss);
    REQUIRE( result.data.num_rows() == 2 );
    REQUIRE( result.data.num_cols() == 3 );
    REQUIRE( result.data.read_coeff(0, 0) == doctest::Approx(1.0) );
    REQUIRE( std::isnan(result.data.read_coeff(0, 1)) );
    REQUIRE( std::isnan(result.data.read_coeff(0, 2)) );
    REQUIRE( std::isnan(result.data.read_coeff(1, 0)) );  // empty cell -> NaN
    REQUIRE( std::isnan(result.data.read_coeff(1, 1)) );
    REQUIRE( result.data.read_coeff(1, 2) == doctest::Approx(2.0) );
}


TEST_CASE( "ReadNumArrayFromCSV infinity handling" ){
    std::stringstream ss("inf,-inf,+inf\n");

    auto result = ReadNumArrayFromCSV<double>(ss);
    REQUIRE( result.data.num_rows() == 1 );
    REQUIRE( result.data.num_cols() == 3 );
    REQUIRE( std::isinf(result.data.read_coeff(0, 0)) );
    REQUIRE( result.data.read_coeff(0, 0) > 0.0 );
    REQUIRE( std::isinf(result.data.read_coeff(0, 1)) );
    REQUIRE( result.data.read_coeff(0, 1) < 0.0 );
    REQUIRE( std::isinf(result.data.read_coeff(0, 2)) );
    REQUIRE( result.data.read_coeff(0, 2) > 0.0 );
}


TEST_CASE( "ReadNumArrayFromCSV non-numeric string mapping" ){
    std::stringstream ss("apple,1.0\nbanana,2.0\napple,3.0\n");

    auto result = ReadNumArrayFromCSV<double>(ss);
    REQUIRE( result.data.num_rows() == 3 );
    REQUIRE( result.data.num_cols() == 2 );

    // "apple" and "banana" should be mapped to distinct integers.
    REQUIRE( result.string_to_int.size() == 2 );
    REQUIRE( result.string_to_int.count("apple") == 1 );
    REQUIRE( result.string_to_int.count("banana") == 1 );
    REQUIRE( result.string_to_int.at("apple") != result.string_to_int.at("banana") );

    // Both "apple" cells should have the same value.
    REQUIRE( result.data.read_coeff(0, 0) == result.data.read_coeff(2, 0) );

    // The numeric columns should still be parsed correctly.
    REQUIRE( result.data.read_coeff(0, 1) == doctest::Approx(1.0) );
    REQUIRE( result.data.read_coeff(1, 1) == doctest::Approx(2.0) );
    REQUIRE( result.data.read_coeff(2, 1) == doctest::Approx(3.0) );

    // Locations should be tracked.
    const int64_t apple_int = result.string_to_int.at("apple");
    REQUIRE( result.int_to_locations.count(apple_int) == 1 );
    REQUIRE( result.int_to_locations.at(apple_int).size() == 2 );

    // The mapping should be bijective.
    REQUIRE( result.int_to_string.at( result.string_to_int.at("apple") ) == "apple" );
    REQUIRE( result.string_to_int.at( result.int_to_string.at(1L) ) == 1L );
}


TEST_CASE( "ReadNumArrayFromCSV custom callback" ){
    std::stringstream ss("hello,1.0\nworld,2.0\n");

    auto cb = [](const std::string &token, int64_t row, int64_t col) -> double {
        return -999.0;  // Map all non-numeric values to -999.
    };

    auto result = ReadNumArrayFromCSV<double>(ss, false, cb);
    REQUIRE( result.data.num_rows() == 2 );
    REQUIRE( result.data.read_coeff(0, 0) == doctest::Approx(-999.0) );
    REQUIRE( result.data.read_coeff(1, 0) == doctest::Approx(-999.0) );
    REQUIRE( result.data.read_coeff(0, 1) == doctest::Approx(1.0) );
}


TEST_CASE( "ReadNumArrayFromCSV empty input" ){
    std::stringstream ss("");
    REQUIRE_THROWS( ReadNumArrayFromCSV<double>(ss) );
}


TEST_CASE( "ReadNumArrayFromCSV inconsistent columns" ){
    std::stringstream ss("1.0,2.0\n1.0,2.0,3.0\n");
    REQUIRE_THROWS( ReadNumArrayFromCSV<double>(ss) );
}


TEST_CASE( "ReadNumArrayFromCSV whitespace trimming" ){
    std::stringstream ss(" 1.0 , 2.0 , 3.0 \n");

    auto result = ReadNumArrayFromCSV<double>(ss);
    REQUIRE( result.data.num_rows() == 1 );
    REQUIRE( result.data.num_cols() == 3 );
    REQUIRE( result.data.read_coeff(0, 0) == doctest::Approx(1.0) );
    REQUIRE( result.data.read_coeff(0, 1) == doctest::Approx(2.0) );
    REQUIRE( result.data.read_coeff(0, 2) == doctest::Approx(3.0) );
}


TEST_CASE( "num_array subarray basic" ){
    num_array<double> m(3, 4, 0.0);
    for(int64_t r = 0; r < 3; ++r){
        for(int64_t c = 0; c < 4; ++c){
            m.coeff(r, c) = static_cast<double>(r * 10 + c);
        }
    }

    SUBCASE("full subarray equals original"){
        auto sub = m.subarray(0, 3, 0, 4);
        REQUIRE( sub.num_rows() == 3 );
        REQUIRE( sub.num_cols() == 4 );
        REQUIRE( sub == m );
    }

    SUBCASE("single row slice"){
        auto row = m.subarray(1, 2, 0, 4);
        REQUIRE( row.num_rows() == 1 );
        REQUIRE( row.num_cols() == 4 );
        REQUIRE( row.read_coeff(0, 0) == doctest::Approx(10.0) );
        REQUIRE( row.read_coeff(0, 3) == doctest::Approx(13.0) );
    }

    SUBCASE("column range slice"){
        auto cols = m.subarray(0, 3, 1, 3);
        REQUIRE( cols.num_rows() == 3 );
        REQUIRE( cols.num_cols() == 2 );
        REQUIRE( cols.read_coeff(0, 0) == doctest::Approx(1.0) );
        REQUIRE( cols.read_coeff(2, 1) == doctest::Approx(22.0) );
    }

    SUBCASE("invalid ranges are rejected"){
        REQUIRE_THROWS( m.subarray(-1, 3, 0, 4) );
        REQUIRE_THROWS( m.subarray(0, 4, 0, 4) );
        REQUIRE_THROWS( m.subarray(0, 3, 0, 5) );
        REQUIRE_THROWS( m.subarray(2, 1, 0, 4) );
        REQUIRE_THROWS( m.subarray(0, 3, 3, 2) );
    }
}
