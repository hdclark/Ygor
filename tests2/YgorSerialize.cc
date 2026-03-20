
#include <string>
#include <list>
#include <vector>
#include <cstdint>
#include <memory>

#include <YgorSerialize.h>

#include "doctest/doctest.h"


TEST_CASE( "SERIALIZE Put and Get round-trip" ){

    SUBCASE("uint8_t round-trip"){
        const uint64_t count = 20;
        uint64_t max = count * sizeof(uint8_t);
        std::unique_ptr<uint8_t[]> mem( new uint8_t[max] );
        uint64_t offset = 0;
        bool OK = false;

        // Write sequential uint8_t values.
        for(uint64_t i = 0; i < count; ++i){
            const auto aval = static_cast<uint8_t>(i);
            mem = SERIALIZE::Put<uint8_t>(&OK, std::move(mem), &offset, max, aval);
            REQUIRE( OK );
        }
        REQUIRE( offset == max );

        // Read them back and verify.
        offset = 0;
        for(uint64_t i = 0; i < count; ++i){
            uint8_t aval = 0;
            mem = SERIALIZE::Get<uint8_t>(&OK, std::move(mem), &offset, max, &aval);
            REQUIRE( OK );
            REQUIRE( aval == static_cast<uint8_t>(i) );
        }
    }

    SUBCASE("double round-trip"){
        const uint64_t count = 20;
        uint64_t max = count * sizeof(double);
        std::unique_ptr<uint8_t[]> mem( new uint8_t[max] );
        uint64_t offset = 0;
        bool OK = false;

        // Write sequential double values.
        for(uint64_t i = 0; i < count; ++i){
            const auto aval = static_cast<double>(i * sizeof(double));
            mem = SERIALIZE::Put<double>(&OK, std::move(mem), &offset, max, aval);
            REQUIRE( OK );
        }
        REQUIRE( offset == max );

        // Read them back and verify.
        offset = 0;
        for(uint64_t i = 0; i < count; ++i){
            double aval = -1.0;
            mem = SERIALIZE::Get<double>(&OK, std::move(mem), &offset, max, &aval);
            REQUIRE( OK );
            REQUIRE( aval == static_cast<double>(i * sizeof(double)) );
        }
    }

    SUBCASE("string round-trip"){
        uint64_t max = 5000;
        std::unique_ptr<uint8_t[]> mem( new uint8_t[max] );
        uint64_t offset = 0;
        bool OK = false;

        const std::basic_string<char> original("This is my test string. I hope it is saved and recovered correctly.");

        mem = SERIALIZE::Put_String<char>(&OK, std::move(mem), &offset, max, original);
        REQUIRE( OK );

        uint64_t actualmax = offset;
        offset = 0;
        std::basic_string<char> recovered;

        mem = SERIALIZE::Get_String<char>(&OK, std::move(mem), &offset, actualmax, &recovered);
        REQUIRE( OK );
        REQUIRE( recovered == original );
    }

    SUBCASE("list<double> round-trip"){
        uint64_t max = 5000;
        std::unique_ptr<uint8_t[]> mem( new uint8_t[max] );
        uint64_t offset = 0;
        bool OK = false;

        const std::list<double> original({1.0, 2.0, 3.0, 4.0, 5.0});

        mem = SERIALIZE::Put_Sequence_Container(&OK, std::move(mem), &offset, max, original);
        REQUIRE( OK );

        uint64_t actualmax = offset;
        offset = 0;
        std::list<double> recovered;

        mem = SERIALIZE::Get_Sequence_Container(&OK, std::move(mem), &offset, actualmax, &recovered);
        REQUIRE( OK );
        REQUIRE( recovered == original );
    }

    SUBCASE("list<double> to vector<double> round-trip"){
        uint64_t max = 5000;
        std::unique_ptr<uint8_t[]> mem( new uint8_t[max] );
        uint64_t offset = 0;
        bool OK = false;

        const std::list<double> original({1.0, 2.0, 3.0, 4.0, 5.0});

        mem = SERIALIZE::Put_Sequence_Container(&OK, std::move(mem), &offset, max, original);
        REQUIRE( OK );

        uint64_t actualmax = offset;
        offset = 0;
        std::vector<double> recovered;

        mem = SERIALIZE::Get_Sequence_Container(&OK, std::move(mem), &offset, actualmax, &recovered);
        REQUIRE( OK );
        REQUIRE( recovered.size() == original.size() );

        auto it = original.begin();
        for(size_t i = 0; i < recovered.size(); ++i, ++it){
            REQUIRE( recovered[i] == *it );
        }
    }
}
