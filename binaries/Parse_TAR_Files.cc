//Parse_TAR_Files.cc - Report TAR-handling routines.

#include <iosfwd>
#include <functional>
#include <cmath>
#include <string>
#include <list>

#include "YgorMath.h"
#include "YgorMisc.h"         //Needed for FUNCINFO, FUNCWARN, FUNCERR macros.
#include "YgorLog.h"
#include "YgorStats.h"        //Needed for Stats:: namespace.
#include "YgorString.h"       //Needed for GetFirstRegex(...)

#include "YgorTAR.h"

int main(int argc, char **argv){

    // Print usage if either no arguments are provided, or the first argument starts with a '-'.
    {
        const std::string usage("This program will attempt to parse TAR files using Ygor. You must provide TAR file paths.");

        if(argc == 1){
            std::cerr << usage << std::endl;
            return 0;
        }
        const std::string argv1(argv[1]);
        if(!argv1.empty() && (argv1.at(0) == '-')){
            std::cerr << usage << std::endl;
            return 0;
        }
    }

    // Provide a file handler that will merely list each file encountered in each archive.
    const auto file_handler = []( std::istream & /* is */,
                                  std::string fname,
                                  long int fsize,
                                  std::string /* fmode */,
                                  std::string /* fuser */,
                                  std::string /* fgroup */,
                                  long int /* ftime */,
                                  std::string /* o_name */,
                                  std::string /* g_name */,
                                  std::string fprefix) -> void {
        const auto fullpath = fprefix.empty() ? fname : (fprefix + "/"_s + fname);
        YLOGINFO("Encountered encapsulated file '" << fullpath << "' which comprises " << fsize << " bytes");
        return;
    };

    for(int i = 1; i < argc; ++i){
        const std::string tar_fname(argv[i]);
        std::ifstream ifs(tar_fname);
        read_ustar(ifs, file_handler);
    }

    return 0;
}

