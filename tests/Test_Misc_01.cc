#include <string>

#include "YgorMisc.h"   //Needed for FUNCWARN, FUNCERR, FUNCINFO macros.
#include "YgorLog.h"

int main(int argc, char **argv){

    YLOGINFO("Hopefully this test will work");
    YLOGWARN("It might not");

    const std::string piped = Execute_Command_In_Pipe("ls -lash /usr/bin/* /etc/* /tmp/*");
    
    YLOGINFO("Dumping piped output now:");
    std::cout << piped << std::endl;    

    YLOGINFO("It probably worked!");

    return 0;
}
