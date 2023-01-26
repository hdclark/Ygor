#include <string>

#include "YgorMisc.h"   //Needed for FUNCWARN, FUNCERR, FUNCINFO macros.
#include "YgorLog.h"

int main(int argc, char **argv){

    FUNCINFO("Hopefully this test will work");
    FUNCWARN("It might not");

    const std::string piped = Execute_Command_In_Pipe("ls -lash /usr/bin/* /etc/* /tmp/*");
    
    FUNCINFO("Dumping piped output now:");
    std::cout << piped << std::endl;    

    FUNCINFO("It probably worked!");

    return 0;
}
