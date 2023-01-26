//Test_TAR_01 - Test TAR-handling routines.

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

int main(int, char **){

    try{
        const std::string fcontents("This is a simple test file. The file length is explicitly provided.");
        const std::string tar_fname("test_output_01.tar");

        // Create a TAR file using a simple text file.
        {
            std::ofstream ofs(tar_fname);
            ustar_writer ustar(ofs);

            std::stringstream ss(fcontents);
            const auto fsize = static_cast<long int>(ss.str().size());
            ustar.add_file(ss, "test_tar_file_01.txt", fsize);
        }

        FUNCINFO("File '" << tar_fname << "' should be validated for conformance"); 

        // Read the TAR file and compare the contents.
        {
            const auto file_handler = [fcontents]( std::istream &is,
                                                   std::string fname,
                                                   long int fsize,
                                                   std::string fmode,
                                                   std::string fuser,
                                                   std::string fgroup,
                                                   long int ftime,
                                                   std::string o_name,
                                                   std::string g_name,
                                                   std::string fprefix) -> void {
                std::stringstream ss;
                ss << is.rdbuf();
                if(ss.str() == fcontents){
                    FUNCINFO("File round-trip OK");
                }else{
                    FUNCERR("File contents did not round-trip");
                }
                return;
            };

            std::ifstream ifs(tar_fname);
            read_ustar(ifs, file_handler);
        }

    }catch(const std::exception &e){
        FUNCERR("Failed when provided explicit file length: " << e.what());
    }

    try{
        const std::string tar_fname("test_output_02.tar");
        const std::string fcontents("This is a simple test file. The file length is not provided so must be determined via seeking.");

        // Create TAR file without explicitly providing the content length.
        {
            std::ofstream ofs(tar_fname);
            ustar_writer ustar(ofs);

            std::stringstream ss(fcontents);
            ustar.add_file(ss, "test_tar_file_02.txt");
        }

        FUNCINFO("File '" << tar_fname << "' should be validated for conformance"); 

        // Read the TAR file and compare the contents.
        {
            const auto file_handler = [fcontents]( std::istream &is,
                                                   std::string fname,
                                                   long int fsize,
                                                   std::string fmode,
                                                   std::string fuser,
                                                   std::string fgroup,
                                                   long int ftime,
                                                   std::string o_name,
                                                   std::string g_name,
                                                   std::string fprefix) -> void {
                std::stringstream ss;
                ss << is.rdbuf();
                if(ss.str() == fcontents){
                    FUNCINFO("File round-trip OK");
                }else{
                    FUNCERR("File contents did not round-trip");
                }
                return;
            };

            std::ifstream ifs(tar_fname);
            read_ustar(ifs, file_handler);
        }

    }catch(const std::exception &e){
        FUNCERR("Failed without explicit file length: " << e.what());
    }


    
    return 0;
}

