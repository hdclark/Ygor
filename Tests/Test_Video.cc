//Test_Video.cc - This is a test file for YgorVIDEOTools.cc.

#include <iostream>
#include <utility>
#include <list>

#include "YgorMisc.h"
#include "YgorVIDEOTools.h"


int main(int argc, char **argv){
    if(argc < 2) FUNCERR("This program requires a filename (video file) as input");

    std::pair<long int,long int> dims = YgorVIDEOTools_Get_Video_Dimensions(argv[1]);

    FUNCINFO("The dimensions were found to be W = " << dims.first << " and H = " << dims.second);

    return 0;
}

