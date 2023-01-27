#include <iostream>
#include <string>
#include <list>
#include <memory>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorFilesDirs.h"


int main(int argc, char **argv){

    if(argc < 2) YLOGERR("This program creates a directory like 'mkdir -p'. Bailing");
    const std::string dirname(argv[1]);

    if(!Create_Dir_and_Necessary_Parents(dirname)){
        YLOGERR("Not able to create directory '" << dirname << "'");
    }else{
        YLOGINFO("Successfully created directory '" << dirname << "' and any necessary parents");
    }
    return 0;
}
