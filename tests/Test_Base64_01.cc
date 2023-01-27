#include <vector>
#include <string>

#include "YgorMisc.h"
#include "YgorLog.h"

#include "YgorBase64.h"

int main(int, char**){

    const auto test_encode_decode = [](const std::string &orig,
                                       const std::string &ref_enc) -> void {
        YLOGINFO("Input string: '" << orig << "'");

        const auto enc = Base64::EncodeFromString(orig);
        YLOGINFO("     Encoded: '" << enc << "'");

        if(enc == ref_enc){
            YLOGINFO("    encoded string matches reference encode correctly");
        }else{
            YLOGWARN("     encode failed");
            for(const auto &c : enc) std::cout << c << " ";
            YLOGERR("done");
        }

        const auto dec = Base64::DecodeToString(enc);
        YLOGINFO("     Decoded: '" << dec << "'");

        if(dec == orig){
            YLOGINFO("    encode-decode matches correctly");
        }else{
            YLOGWARN("     encode-decode failed");
            for(const auto &c : dec) std::cout << c << " ";
            YLOGERR("done");
        }
        return;
    };

    //test_encode_decode("");
    //test_encode_decode(" ");
    test_encode_decode("a", "YQ==");
    test_encode_decode("ab", "YWI=");
    test_encode_decode("123", "MTIz");
    test_encode_decode("abc", "YWJj");
    test_encode_decode("abcd", "YWJjZA==");
    test_encode_decode("abcde", "YWJjZGU=");
    test_encode_decode("abcdef", "YWJjZGVm");
    test_encode_decode("abcdefg", "YWJjZGVmZw==");
    test_encode_decode("something with spaces", "c29tZXRoaW5nIHdpdGggc3BhY2Vz");
    test_encode_decode("something with \t tabs", "c29tZXRoaW5nIHdpdGggCSB0YWJz");
    test_encode_decode("something with \n newlines", "c29tZXRoaW5nIHdpdGggCiBuZXdsaW5lcw==");
    test_encode_decode(R"***(something with ,.<>/?;:"[]{}()!@#$^&*`~|\ lots of punctuation)***", 
                       "c29tZXRoaW5nIHdpdGggLC48Pi8/OzoiW117fSgpIUAjJF4mKmB+fFwgbG90cyBvZiBwdW5jdHVhdGlvbg==");
    test_encode_decode(" something with leading space", "IHNvbWV0aGluZyB3aXRoIGxlYWRpbmcgc3BhY2U=");
    test_encode_decode("something with trailing space ", "c29tZXRoaW5nIHdpdGggdHJhaWxpbmcgc3BhY2Ug");
    test_encode_decode("the quick brown fox jumped over the lazy dog. 1234567890.", 
                       "dGhlIHF1aWNrIGJyb3duIGZveCBqdW1wZWQgb3ZlciB0aGUgbGF6eSBkb2cuIDEyMzQ1Njc4OTAu");
    test_encode_decode("THE QUICK BROWN FOX JUMPED OVER THE LAZY DOG.", 
                       "VEhFIFFVSUNLIEJST1dOIEZPWCBKVU1QRUQgT1ZFUiBUSEUgTEFaWSBET0cu");

    YLOGINFO("All base64 encode and decode tests passed successfully");
    return 0;
}

