
#include <string>
#include <cstdint>

#include <YgorAlgorithms.h>
#include <YgorString.h>

#include "doctest/doctest.h"


TEST_CASE( "Consistent_Hash_64" ){

    SUBCASE("known hash values"){
        REQUIRE( Consistent_Hash_64("Some string")       == 3924287415333687545ULL   );
        REQUIRE( Consistent_Hash_64("Some string 1")     == 1340806366846037431ULL   );
        REQUIRE( Consistent_Hash_64("Some string 2")     == 16715263229586335810ULL  );
        REQUIRE( Consistent_Hash_64("Some other string") == 11153072457922119554ULL  );
        REQUIRE( Consistent_Hash_64("S")                 == 4082407551497703653ULL   );
        REQUIRE( Consistent_Hash_64(" ")                 == 6640659725864518894ULL   );
    }
}


static bool Verify_MD5(const std::string &key, const std::string &expected_hash){
    std::string computed_hash;
    uint64_t str_sz;
    bool l_OK;

    auto buff = str_to_buf<uint8_t>(&l_OK, key, &str_sz);
    if(!l_OK) return false;

    buff = MD5_Hash(&l_OK, std::move(buff), str_sz, &computed_hash);
    if(!l_OK) return false;

    return (computed_hash == expected_hash);
}


TEST_CASE( "MD5_Hash" ){

    SUBCASE("RFC 1321 test vectors"){
        REQUIRE( Verify_MD5("",                "d41d8cd98f00b204e9800998ecf8427e") );
        REQUIRE( Verify_MD5("a",               "0cc175b9c0f1b6a831c399e269772661") );
        REQUIRE( Verify_MD5("abc",             "900150983cd24fb0d6963f7d28e17f72") );
        REQUIRE( Verify_MD5("message digest",  "f96b697d7cb7938d525a2f31aaf161d0") );
        REQUIRE( Verify_MD5("abcdefghijklmnopqrstuvwxyz",
                             "c3fcd3d76192e4007dfb496cca67e13b") );
        REQUIRE( Verify_MD5("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
                             "d174ab98d277d9f5a5611c2c9f419d9f") );
        REQUIRE( Verify_MD5("12345678901234567890123456789012345678901234567890123456789012345678901234567890",
                             "57edf4a22be3c955ac49da2e2107b67a") );
    }

    SUBCASE("additional test strings"){
        REQUIRE( Verify_MD5("Some text",
                             "9db5682a4d778ca2cb79580bdb67083f") );
        REQUIRE( Verify_MD5("The quick brown fox jumps over the lazy dog",
                             "9e107d9d372bb6826bd81d3542a419d6") );
        REQUIRE( Verify_MD5("The quick brown fox jumps over the lazy dog.",
                             "e4d909c290d0fb1ca068ffaddf22cbd0") );
        REQUIRE( Verify_MD5("Writing tests",
                             "cc554b23ef0cdcdc244978dfbd605a86") );
        REQUIRE( Verify_MD5("Kind of sucks...",
                             "05c07dcbff9e889b409101710e238e56") );
        REQUIRE( Verify_MD5("...but is probably necessary!",
                             "be164b7e20919dd5e84f79b28c0364d0") );
    }
}
