#include <iostream>
#include <memory>
#include <sstream>
#include <iomanip>

#include "md5.h"

#include <YgorMisc.h>
#include <YgorFilesDirs.h>
#include <YgorString.h>

std::string Compute_MD5_of_File(const std::string &Filename){

    MD5_CTX working;
    MD5_Init(&working);

    long int thesize(0);
    auto inmem = Load_Binary_File<unsigned char>(Filename, &thesize);
//    std::string inmem = LoadBinaryFileToString(Filename);

    MD5_Update(&working, reinterpret_cast<void *>(inmem.get()), thesize);

//    unsigned char result[16+1];
//    result[16] = '\0';
    unsigned char *result = new unsigned char[16+1];
//    result[16] = '\0';
//    std::unique_ptr<unsigned char> result = new unsigned char[16+1];
    result[16] = '\0';
   
    MD5_Final(result, &working);
    const std::string out = reinterpret_cast<const char *>(result);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for(int i = 0; i < 16; ++i){
        ss << std::setw(2) << static_cast<unsigned>(result[i]);
    }  
//    for(int i = 0; i < 16; i++) printf("%02x", result[i]);


    delete[] result;

    return ss.str(); //out;
}


int main(int argc, char **argv){

    FUNCINFO("MD5 of /tmp/sgf_skeleton.csv is " << Compute_MD5_of_File("/tmp/sgf_skeleton.csv")); 
//    Compute_MD5_of_File("/tmp/sgf_skeleton.csv");


    return 0;
}
