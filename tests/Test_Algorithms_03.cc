/*
#include <cmath>
#include <iostream>
#include <vector>
#include <functional>
#include <list>
#include <limits>
#include <numeric>   //For std::accumulate(...).
#include <map>
#include <string>
#include <tuple>
#include <random>
#include <iomanip> //For the MD5 conversion from uint128_t to std::string.
*/

#include "YgorAlgorithms.h"
#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorStats.h"
#include "YgorString.h"
#include "YgorPlot.h"
#include "YgorTime.h"
#include "YgorFilesDirs.h"

bool Verify_MD5(const std::string &key, const std::string &expected_hash){
    std::string computed_hash;
    uint64_t str_sz;
    bool l_OK;
    
    //Generate a buffer of type T from the incoming key.
    auto buff = str_to_buf<uint8_t>(&l_OK,key,&str_sz);
    if(!l_OK){
        FUNCWARN("Unable to generate buffer from incoming key string '" << key << "'");
        return false;
    }

    //Compute the hash using the buffer.
    buff = MD5_Hash(&l_OK,std::move(buff),str_sz,&computed_hash);
    if(!l_OK){
        FUNCWARN("Unable to compute hash");
        return false;
    }

    //Compare the result to the expected result.
    if(computed_hash != expected_hash){
        FUNCWARN("Computed hash '" << computed_hash << "' and expected hash '" << expected_hash << "' do not agree");
        return false;
    }else{
        FUNCINFO("Computed hash and expected hash '" << expected_hash << "' agree");
   }
    
    return true;
}


int main(int argc, char **argv){

    //Verify the MD5 implementation computes some hashes.
    bool OK = true;

    //The "MD5 test suite" from RFC 1321 appears to be composed of the following.
    OK &= Verify_MD5("", "d41d8cd98f00b204e9800998ecf8427e");
    OK &= Verify_MD5("a", "0cc175b9c0f1b6a831c399e269772661");
    OK &= Verify_MD5("abc", "900150983cd24fb0d6963f7d28e17f72");
    OK &= Verify_MD5("message digest", "f96b697d7cb7938d525a2f31aaf161d0");
    OK &= Verify_MD5("abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b");
    OK &= Verify_MD5("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "d174ab98d277d9f5a5611c2c9f419d9f");
    OK &= Verify_MD5("12345678901234567890123456789012345678901234567890123456789012345678901234567890", "57edf4a22be3c955ac49da2e2107b67a");

    //And here are some others, for S&G's.
    OK &= Verify_MD5("Some text", "9db5682a4d778ca2cb79580bdb67083f");
    OK &= Verify_MD5("The quick brown fox jumps over the lazy dog", "9e107d9d372bb6826bd81d3542a419d6");
    OK &= Verify_MD5("The quick brown fox jumps over the lazy dog.", "e4d909c290d0fb1ca068ffaddf22cbd0");
    OK &= Verify_MD5("Writing tests", "cc554b23ef0cdcdc244978dfbd605a86");
    OK &= Verify_MD5("Kind of sucks...", "05c07dcbff9e889b409101710e238e56");
    OK &= Verify_MD5("...but is probably necessary!", "be164b7e20919dd5e84f79b28c0364d0");

    if(!OK) FUNCERR("Failed!");
    FUNCINFO("Passed! Hashing algorithm *appears* to be OK");
    FUNCWARN("Note - this is *not* proof that the algorithm is OK!");
    return 0;
}
