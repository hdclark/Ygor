#include <iostream>
#include <string>
#include <list>
#include <memory>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorFilesDirs.h"


int main(int argc, char **argv){

    if(argc < 2) YLOGERR("This program requires a (test, dummy) file as input. It will output a copy to /tmp/filesdirstest2.out");
    const std::string FilenameIn(argv[1]);
    const std::string FilenameOut("/tmp/filesdirstest2.out");

    //Test the full-path expansion function.
    const auto fullpath = Fully_Expand_Filename(FilenameIn);
    YLOGINFO("The full path of the input filename is '" << fullpath << "'");
    return 0;

    //Test the Get_Piece_of_Binary_File() function by copying a file, chunk by chunk, to a duplicate in /tmp/
    if(!Does_File_Exist_And_Can_Be_Read(FilenameIn)) YLOGERR("Unable to open input file! Bailing");
    const auto T = Size_of_File(FilenameIn);
    const long long int N = 512; //Copy it 512 bytes at a time.

    long long int n = 0; //Number of bytes copied.
    while(n < T){
        const long long int c = (n+N) > T ? (T-n) : N; //Number of bytes to copy. Is N or less (on the last cycle.)

        const auto mem = Get_Piece_of_Binary_File<char>(FilenameIn, n, c); //Current position, number of bytes desired.

        std::string temp(mem.get(), c);
        AppendStringToFile(temp,FilenameOut);
        n += c;
    }
    return 0;
}
