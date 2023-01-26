#include <iostream>
#include <string>
#include <list>
#include <memory>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorFilesDirs.h"


int main(int argc, char **argv){

    if(argc < 3) FUNCERR("This program requires a file and a directory as input. Bailing");
    const std::string Filename(argv[1]);
    const std::string Pathname(argv[2]);

    //Check if the file is recursively within the directory or not.
    std::cout << "The file '" << Filename << "' is ";
    if(!File_Is_Recursively_Within_Directory(Filename,Pathname)){
        std::cout << "not ";
    }
    std::cout << "contained within the directory '" << Pathname << "'" << std::endl;

    return 0;
}
