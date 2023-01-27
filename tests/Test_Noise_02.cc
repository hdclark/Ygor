#include <iostream>
#include <cstddef>
#include <climits>
#include <bitset>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorNoise.h"



int main(int argc, char **argv){

    //Practice with bitwise manipulation.
    uint64_t a = 3;
    YLOGINFO("a is initially " << std::bitset<64>(a)); 

    YLOGINFO("Left 1, 8 times");
    a = PER_BYTE_BITWISE_ROT_L(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));


    YLOGINFO("Right 1, 8 times");
    a = PER_BYTE_BITWISE_ROT_R(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 1);
    YLOGINFO("a is currently " << std::bitset<64>(a));


    YLOGINFO("Left 3, 8 times");
    a = PER_BYTE_BITWISE_ROT_L(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));


    YLOGINFO("Right 3, 8 times");
    a = PER_BYTE_BITWISE_ROT_R(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 3);
    YLOGINFO("a is currently " << std::bitset<64>(a));



    YLOGINFO("Left -1, 8 times");
    a = PER_BYTE_BITWISE_ROT_L(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));


    YLOGINFO("Right -1, 8 times");
    a = PER_BYTE_BITWISE_ROT_R(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, -1);
    YLOGINFO("a is currently " << std::bitset<64>(a));


    YLOGINFO("Left -3, 8 times");
    a = PER_BYTE_BITWISE_ROT_L(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_L(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));


    YLOGINFO("Right -3, 8 times");
    a = PER_BYTE_BITWISE_ROT_R(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, -3);
    YLOGINFO("a is currently " << std::bitset<64>(a));


    YLOGINFO("Right 65, 8 times");
    a = PER_BYTE_BITWISE_ROT_R(a, 65);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 65);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 65);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 65);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 65);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 65);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 65);
    YLOGINFO("a is currently " << std::bitset<64>(a));
    a = PER_BYTE_BITWISE_ROT_R(a, 65);
    YLOGINFO("a is currently " << std::bitset<64>(a));


    return 0;
}
