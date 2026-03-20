#include <iostream>
#include <string>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorString.h"



int main(int argc, char **argv){

    for(auto i=0; i<100; ++i){
        YLOGINFO("Some random strings: '" << Generate_Random_String_of_Length(100) << "'");
    }

    return 0;
}
