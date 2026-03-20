
#include <string>

#include <YgorBase64.h>

#include "doctest/doctest.h"


TEST_CASE( "Base64 encode and decode" ){

    const auto test_pair = [](const std::string &orig, const std::string &ref_enc){
        const auto enc = Base64::EncodeFromString(orig);
        REQUIRE( enc == ref_enc );

        const auto dec = Base64::DecodeToString(enc);
        REQUIRE( dec == orig );
    };

    SUBCASE("single character"){ test_pair("a", "YQ=="); }
    SUBCASE("two characters"){ test_pair("ab", "YWI="); }
    SUBCASE("three digit number"){ test_pair("123", "MTIz"); }
    SUBCASE("three characters"){ test_pair("abc", "YWJj"); }
    SUBCASE("four characters"){ test_pair("abcd", "YWJjZA=="); }
    SUBCASE("five characters"){ test_pair("abcde", "YWJjZGU="); }
    SUBCASE("six characters"){ test_pair("abcdef", "YWJjZGVm"); }
    SUBCASE("seven characters"){ test_pair("abcdefg", "YWJjZGVmZw=="); }

    SUBCASE("string with spaces"){
        test_pair("something with spaces", "c29tZXRoaW5nIHdpdGggc3BhY2Vz");
    }
    SUBCASE("string with tabs"){
        test_pair("something with \t tabs", "c29tZXRoaW5nIHdpdGggCSB0YWJz");
    }
    SUBCASE("string with newlines"){
        test_pair("something with \n newlines", "c29tZXRoaW5nIHdpdGggCiBuZXdsaW5lcw==");
    }
    SUBCASE("string with punctuation"){
        test_pair(R"***(something with ,.<>/?;:"[]{}()!@#$^&*`~|\ lots of punctuation)***",
                  "c29tZXRoaW5nIHdpdGggLC48Pi8/OzoiW117fSgpIUAjJF4mKmB+fFwgbG90cyBvZiBwdW5jdHVhdGlvbg==");
    }
    SUBCASE("leading space"){
        test_pair(" something with leading space", "IHNvbWV0aGluZyB3aXRoIGxlYWRpbmcgc3BhY2U=");
    }
    SUBCASE("trailing space"){
        test_pair("something with trailing space ", "c29tZXRoaW5nIHdpdGggdHJhaWxpbmcgc3BhY2Ug");
    }
    SUBCASE("long lowercase sentence"){
        test_pair("the quick brown fox jumped over the lazy dog. 1234567890.",
                  "dGhlIHF1aWNrIGJyb3duIGZveCBqdW1wZWQgb3ZlciB0aGUgbGF6eSBkb2cuIDEyMzQ1Njc4OTAu");
    }
    SUBCASE("long uppercase sentence"){
        test_pair("THE QUICK BROWN FOX JUMPED OVER THE LAZY DOG.",
                  "VEhFIFFVSUNLIEJST1dOIEZPWCBKVU1QRUQgT1ZFUiBUSEUgTEFaWSBET0cu");
    }
}
