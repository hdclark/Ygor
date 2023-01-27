//Test_Environment.cc

#include <iostream>
#include <string>
#include <utility>     //Needed for std::pair.

#include "YgorMisc.h"    //Needed for Execute_Command_In_Pipe.
#include "YgorLog.h"
#include "YgorString.h"  //Needed for Xtostring.

#include "YgorEnvironment.h"


int main(int argc, char **argv){

    decltype(Get_Framebuffer_Pixel_Dimensions(0)) dims;

    dims = Get_Framebuffer_Pixel_Dimensions(0);
    YLOGINFO("The visible fb0 dimensions are " << dims.first << " and " << dims.second);
    dims = Get_Framebuffer_Pixel_Dimensions(0, true);
    YLOGINFO("The virtual fb0 dimensions are " << dims.first << " and " << dims.second);


    dims = Get_Framebuffer_Pixel_Dimensions(1, false);
    YLOGINFO("The visible fb1 dimensions are " << dims.first << " and " << dims.second);
    dims = Get_Framebuffer_Pixel_Dimensions(1, true);
    YLOGINFO("The virtual fb1 dimensions are " << dims.first << " and " << dims.second);


    dims = Get_Framebuffer_Pixel_Dimensions(2);
    YLOGINFO("The visible fb2 dimensions are " << dims.first << " and " << dims.second);
    dims = Get_Framebuffer_Pixel_Dimensions(2, true);
    YLOGINFO("The virtual fb2 dimensions are " << dims.first << " and " << dims.second);


    return 0;
}
