#include <iostream>
#include <utility>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorEnvironment.h"


int main(int argc, char **argv){

    auto resp = (Is_X_Running_And_In_Focus() ? "yes" : "no");
    YLOGINFO("Is X running and is currently in focus? " << resp );

    auto dimens = Get_Framebuffer_Pixel_Dimensions(0,false);
    YLOGINFO("Framebuffer visible screen dimensions are W,H = " << dimens.first << "," << dimens.second );

    YLOGINFO("Total system RAM is: " << Amount_Of_Total_Memory_MB() );
    YLOGINFO("Total unclaimed RAM is: " << Amount_Of_Totally_Free_Memory_MB() );

    return 0;
}
