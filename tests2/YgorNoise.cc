
#include <cstdint>
#include <bitset>

#include <YgorMisc.h>

#include "doctest/doctest.h"


TEST_CASE( "PER_BYTE_BITWISE_ROT_L and PER_BYTE_BITWISE_ROT_R" ){

    SUBCASE("rotate left by 1 produces expected bit pattern"){
        // a = 3 = 0b00000011 in the lowest byte.
        uint64_t a = 3;
        a = PER_BYTE_BITWISE_ROT_L(a, 1);
        // 0b00000011 rotated left 1 within a byte -> 0b00000110 = 6
        REQUIRE( (a & 0xFF) == 6 );
    }

    SUBCASE("rotate right by 1 produces expected bit pattern"){
        uint64_t a = 6;
        a = PER_BYTE_BITWISE_ROT_R(a, 1);
        REQUIRE( (a & 0xFF) == 3 );
    }

    SUBCASE("rotate left by 8 returns original value"){
        uint64_t a = 3;
        for(int i = 0; i < 8; ++i){
            a = PER_BYTE_BITWISE_ROT_L(a, 1);
        }
        REQUIRE( a == 3 );
    }

    SUBCASE("rotate right by 8 returns original value"){
        uint64_t a = 3;
        for(int i = 0; i < 8; ++i){
            a = PER_BYTE_BITWISE_ROT_R(a, 1);
        }
        REQUIRE( a == 3 );
    }

    SUBCASE("left then right by same amount restores original"){
        uint64_t a = 3;
        for(int i = 0; i < 8; ++i){
            a = PER_BYTE_BITWISE_ROT_L(a, 1);
        }
        for(int i = 0; i < 8; ++i){
            a = PER_BYTE_BITWISE_ROT_R(a, 1);
        }
        REQUIRE( a == 3 );
    }

    SUBCASE("rotate left by 3 then right by 3 restores original"){
        uint64_t a = 3;
        for(int i = 0; i < 8; ++i){
            a = PER_BYTE_BITWISE_ROT_L(a, 3);
        }
        for(int i = 0; i < 8; ++i){
            a = PER_BYTE_BITWISE_ROT_R(a, 3);
        }
        REQUIRE( a == 3 );
    }

    SUBCASE("rotate left by 3 produces expected bit pattern"){
        // 0b00000011 rotated left 3 within a byte -> 0b00011000 = 24
        uint64_t a = 3;
        a = PER_BYTE_BITWISE_ROT_L(a, 3);
        REQUIRE( (a & 0xFF) == 24 );
    }

    SUBCASE("rotate right by 3 produces expected bit pattern"){
        uint64_t a = 24;
        a = PER_BYTE_BITWISE_ROT_R(a, 3);
        REQUIRE( (a & 0xFF) == 3 );
    }
}
