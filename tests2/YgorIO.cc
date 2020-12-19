
#include <iostream>
#include <sstream>

#include <YgorMisc.h>
#include <YgorIO.h>

#include "doctest/doctest.h"


TEST_CASE( "YgorIO read_binary and write_binary" ){
    std::stringstream ss;

    SUBCASE("uint8_t"){
        const uint8_t v1 = 0x12;
        uint8_t v2 = 0;

        REQUIRE(ygor::io::write_binary<uint8_t,YgorEndianness::Little>(ss, v1));
        REQUIRE(ygor::io::read_binary<uint8_t,YgorEndianness::Little>(ss, v2));
        REQUIRE(v1 == v2);

        REQUIRE(ygor::io::write_binary<uint8_t,YgorEndianness::Big>(ss, v1));
        REQUIRE(ygor::io::read_binary<uint8_t,YgorEndianness::Big>(ss, v2));
        REQUIRE(v1 == v2);

        REQUIRE(ygor::io::write_binary<uint8_t,YgorEndianness::Little>(ss, v1));
        REQUIRE(ygor::io::read_binary<uint8_t,YgorEndianness::Big>(ss, v2));
        REQUIRE(v1 == v2); // Note: valid only for 1-byte objects or palindromes!

        REQUIRE(ygor::io::write_binary<uint8_t,YgorEndianness::Big>(ss, v1));
        REQUIRE(ygor::io::read_binary<uint8_t,YgorEndianness::Little>(ss, v2));
        REQUIRE(v1 == v2); // Note: valid only for 1-byte objects or palindromes!
    }

    SUBCASE("uint16_t"){
        const uint16_t v1 = 0x1234;
        uint16_t v2 = 0;

        REQUIRE(ygor::io::write_binary<uint16_t,YgorEndianness::Little>(ss, v1));
        REQUIRE(ygor::io::read_binary<uint16_t,YgorEndianness::Little>(ss, v2));
        REQUIRE(v1 == v2);

        REQUIRE(ygor::io::write_binary<uint16_t,YgorEndianness::Big>(ss, v1));
        REQUIRE(ygor::io::read_binary<uint16_t,YgorEndianness::Big>(ss, v2));
        REQUIRE(v1 == v2);

        REQUIRE(ygor::io::write_binary<uint16_t,YgorEndianness::Little>(ss, v1));
        REQUIRE(ygor::io::read_binary<uint16_t,YgorEndianness::Big>(ss, v2));
        REQUIRE(v1 != v2);

        REQUIRE(ygor::io::write_binary<uint16_t,YgorEndianness::Big>(ss, v1));
        REQUIRE(ygor::io::read_binary<uint16_t,YgorEndianness::Little>(ss, v2));
        REQUIRE(v1 != v2);
    }

    SUBCASE("uint64_t"){
        const uint64_t v1 = 0x1234567890ABCDEF;
        uint64_t v2 = 0;

        REQUIRE(ygor::io::write_binary<uint64_t,YgorEndianness::Little>(ss, v1));
        REQUIRE(ygor::io::read_binary<uint64_t,YgorEndianness::Little>(ss, v2));
        REQUIRE(v1 == v2);

        REQUIRE(ygor::io::write_binary<uint64_t,YgorEndianness::Big>(ss, v1));
        REQUIRE(ygor::io::read_binary<uint64_t,YgorEndianness::Big>(ss, v2));
        REQUIRE(v1 == v2);

        REQUIRE(ygor::io::write_binary<uint64_t,YgorEndianness::Little>(ss, v1));
        REQUIRE(ygor::io::read_binary<uint64_t,YgorEndianness::Big>(ss, v2));
        REQUIRE(v1 != v2);

        REQUIRE(ygor::io::write_binary<uint64_t,YgorEndianness::Big>(ss, v1));
        REQUIRE(ygor::io::read_binary<uint64_t,YgorEndianness::Little>(ss, v2));
        REQUIRE(v1 != v2);
    }

    SUBCASE("int64_t"){
        const int64_t v1 = 0x1234567890ABCDEF;
        int64_t v2 = 0;

        REQUIRE(ygor::io::write_binary<int64_t,YgorEndianness::Little>(ss, v1));
        REQUIRE(ygor::io::read_binary<int64_t,YgorEndianness::Little>(ss, v2));
        REQUIRE(v1 == v2);

        REQUIRE(ygor::io::write_binary<int64_t,YgorEndianness::Big>(ss, v1));
        REQUIRE(ygor::io::read_binary<int64_t,YgorEndianness::Big>(ss, v2));
        REQUIRE(v1 == v2);

        REQUIRE(ygor::io::write_binary<int64_t,YgorEndianness::Little>(ss, v1));
        REQUIRE(ygor::io::read_binary<int64_t,YgorEndianness::Big>(ss, v2));
        REQUIRE(v1 != v2);

        REQUIRE(ygor::io::write_binary<int64_t,YgorEndianness::Big>(ss, v1));
        REQUIRE(ygor::io::read_binary<int64_t,YgorEndianness::Little>(ss, v2));
        REQUIRE(v1 != v2);
    }

    SUBCASE("float"){
        const float v1 = 12345678.90123f;
        float v2 = 0;

        REQUIRE(ygor::io::write_binary<float,YgorEndianness::Little>(ss, v1));
        REQUIRE(ygor::io::read_binary<float,YgorEndianness::Little>(ss, v2));
        REQUIRE(v1 == v2);

        REQUIRE(ygor::io::write_binary<float,YgorEndianness::Big>(ss, v1));
        REQUIRE(ygor::io::read_binary<float,YgorEndianness::Big>(ss, v2));
        REQUIRE(v1 == v2);

        REQUIRE(ygor::io::write_binary<float,YgorEndianness::Little>(ss, v1));
        REQUIRE(ygor::io::read_binary<float,YgorEndianness::Big>(ss, v2));
        REQUIRE(v1 != v2);

        REQUIRE(ygor::io::write_binary<float,YgorEndianness::Big>(ss, v1));
        REQUIRE(ygor::io::read_binary<float,YgorEndianness::Little>(ss, v2));
        REQUIRE(v1 != v2);
    }

    SUBCASE("double"){
        const double v1 = 12345678.90123;
        double v2 = 0;

        REQUIRE(ygor::io::write_binary<double,YgorEndianness::Little>(ss, v1));
        REQUIRE(ygor::io::read_binary<double,YgorEndianness::Little>(ss, v2));
        REQUIRE(v1 == v2);

        REQUIRE(ygor::io::write_binary<double,YgorEndianness::Big>(ss, v1));
        REQUIRE(ygor::io::read_binary<double,YgorEndianness::Big>(ss, v2));
        REQUIRE(v1 == v2);

        REQUIRE(ygor::io::write_binary<double,YgorEndianness::Little>(ss, v1));
        REQUIRE(ygor::io::read_binary<double,YgorEndianness::Big>(ss, v2));
        REQUIRE(v1 != v2);

        REQUIRE(ygor::io::write_binary<double,YgorEndianness::Big>(ss, v1));
        REQUIRE(ygor::io::read_binary<double,YgorEndianness::Little>(ss, v2));
        REQUIRE(v1 != v2);
    }

    SUBCASE("std::array<char,3>"){
        const std::array<char,3> v1 = {{ 'a', 'b', 'c' }};

        const auto host_endianness = YgorEndianness::Host;
        const auto other_endianness = (host_endianness == YgorEndianness::Little) ? YgorEndianness::Big 
                                                                                  : YgorEndianness::Little;

        SUBCASE("string data written as-is when writing with host's endianness"){
            REQUIRE(ygor::io::write_binary<std::array<char,3>,host_endianness>(ss, v1));
            REQUIRE(ss.str() == "abc");
        }
        SUBCASE("string data flipped when NOT writing with host's endianness"){
            REQUIRE(ygor::io::write_binary<std::array<char,3>,other_endianness>(ss, v1));
            REQUIRE(ss.str() == "cba");
        }
    }

}

