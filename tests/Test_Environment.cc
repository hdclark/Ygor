#include <iostream>
#include <utility>

#include "YgorMisc.h"
#include "YgorEnvironment.h"


int main(int argc, char **argv){

    auto resp = (Is_X_Running_And_In_Focus() ? "yes" : "no");
    FUNCINFO("Is X running and is currently in focus? " << resp );

    auto dimens = Get_Framebuffer_Pixel_Dimensions(0,false);
    FUNCINFO("Framebuffer visible screen dimensions are W,H = " << dimens.first << "," << dimens.second );

    FUNCINFO("Total system RAM is: " << Amount_Of_Total_Memory_MB() );
    FUNCINFO("Total unclaimed RAM is: " << Amount_Of_Totally_Free_Memory_MB() );


    //Test sending a simple email.
/*
DEPRECATED!

    if(!Send_Simple_SMTP_Email("hdeanclark@gmail.com", "Testing the simple email routine", "This is my test email.\nThis is a new line.\n    This is an indented newline.")){
        FUNCWARN("Email was unable to be sent");
    }
*/
    return 0;
}
