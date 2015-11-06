//Test_FilesDirs4.cc

#include <iostream>
#include <string>
#include <list>
#include <memory>
#include <chrono>
#include <thread>

#include "YgorMisc.h"
#include "YgorFilesDirs.h"


int main(int argc, char **argv){
    const std::string Info("This program periodically checks a file and a directory for changes. Pass in both.");

    if(argc < 3) FUNCERR(Info + " Bailing");
    const std::string filename(argv[1]);
    const std::string dirname(argv[2]);

    if(!Does_File_Exist_And_Can_Be_Read(filename)) FUNCERR(Info + " Bailing");
    if(!Does_Dir_Exist_And_Can_Be_Read(dirname)) FUNCERR(Info + " Bailing");

    std::cout << " ------ File Times -------       ------ Dir Times -------" << std::endl;
    std::cout << "  Access    Modification          Access    Modification" << std::endl;
    while(true){
        std::cout << " " << Last_Access_Time(filename);
        std::cout << "     " << Last_Modification_Time(filename);
        std::cout << "         ";
        std::cout << " " << Last_Access_Time(dirname);
        std::cout << "     " << Last_Modification_Time(dirname);
        std::cout << std::endl;

        std::this_thread::sleep_for( std::chrono::seconds(1) );
    }

    return 0;
}
