#include <iostream>
#include <string>
#include <list>
#include <memory>
#include <cstdint>           //Needed for intmax_t.

#include "YgorFilesDirs.h"


int main(int argc, char **argv){

    auto names = Get_List_of_File_and_Dir_Names_in_Dir("./");

    std::cout << "There are " << names.size() << " file/directory names in the ./ directory." << std::endl;

    for(auto i=names.begin(); i!=names.end(); i++){
        std::cout << *i;
        if(Does_File_Exist_And_Can_Be_Read(*i)){
            std::cout << " is a file and can be read." << std::endl;
        }else if(Does_Dir_Exist_And_Can_Be_Read(*i)){
            std::cout << " is a directory and can be read." << std::endl;
        }else{
            std::cout << " cannot be read - it might be a file or a directory." << std::endl;
        }
      
    }

    

    //Test the binary file reading routine. 
    //NOTE: argv[0] is the name of this binary executable!
    intmax_t size;

    //This call should not fail.
    auto representation_A = Load_Binary_File<unsigned char>( argv[0], &size );   
    if(representation_A != nullptr){
        std::cout << "Successfully loaded binary file \"" << argv[0] << "\" to memory as an array of unsigned chars. ";
        std::cout << "It is of size " << size*sizeof(unsigned char) << " bytes, or " << size << " unsigned chars." << std::endl;
    }

    //Now try load the data as an array of floats. This call *might* fail because the size of the binary file might
    // not be divisible by sizeof(float). This is OK! We will issue a warning if it is expected.
    // 
    if( (size*sizeof(unsigned char)) % sizeof(float) != 0 ){
        std::cout << "WARNING: The following test will (rightfully) fail. This is OK! See the source for more info." << std::endl;
    }
    auto representation_B = Load_Binary_File<float>( argv[0], &size );      

    if(representation_B != nullptr){
        std::cout << "Successfully loaded binary file \"" << argv[0] << "\" to memory as an array of floats. ";
        std::cout << "It is of size " << size*sizeof(float) << " bytes, or " << size << " floats." << std::endl;
    }



    std::cout << "-----------------------------------" << std::endl;
    std::string FIFO_filename("/tmp/test_fifo_filename");
    if(!CreateFIFOFile(FIFO_filename)){
        std::cout << "Unable to create FIFO file to handle client data. Bailing!" << std::endl;
        return false;
    }


    return 0;
}
