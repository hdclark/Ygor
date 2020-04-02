//Test_TAR_01 - Test TAR-handling routines.

#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <string>
#include <list>

#include "YgorMath.h"
#include "YgorMisc.h"         //Needed for FUNCINFO, FUNCWARN, FUNCERR macros.
#include "YgorStats.h"        //Needed for Stats:: namespace.
#include "YgorString.h"       //Needed for GetFirstRegex(...)

#include "YgorTAR.h"

int main(int, char **){

    try{
        const std::string tar_fname("test_output_01.tar");
        std::ofstream ofs(tar_fname);
        ustar_archive_writer ustar(ofs);

        std::stringstream ss("This is a simple test file. The file length is explicitly provided.");
        const auto fsize = static_cast<long int>(ss.str().size());
        ustar.add_file(ss, "test_tar_file_01.txt", fsize);

        FUNCINFO("File '" << tar_fname << "' should be validated for conformance"); 
    }catch(const std::exception &e){
        FUNCERR("Failed when provided explicit file length: " << e.what());
    }

    try{
        const std::string tar_fname("test_output_02.tar");
        std::ofstream ofs(tar_fname);
        ustar_archive_writer ustar(ofs);

        std::stringstream ss("This is a simple test file. The file length is not provided so must be determined via seeking.");
        ustar.add_file(ss, "test_tar_file_02.txt");

        FUNCINFO("File '" << tar_fname << "' should be validated for conformance"); 
    }catch(const std::exception &e){
        FUNCERR("Failed without explicit file length: " << e.what());
    }

    
    return 0;
}

