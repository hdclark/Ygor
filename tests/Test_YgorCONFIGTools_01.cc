
#include <iostream>
#include <string>
#include <vector>

#include "YgorCONFIGTools.h"

/*

    This source file will be read in as a config file. 
    All lines without the given delimiter will be ignored.
    We will only see valid lines. Here are some potentially
    valid lines.

    Post-processing should be used to further validate the 
    lines.

    # This is a comment.
    # This is also a comment, even though it holds a :.
    This is NOT a comment.
    The (empty) line below this is also not a comment.

    The next line is not a comment.
    item 1 : item 2
    Nor is the next line.
    item a : item b :      item c
    item 1 of 3:item 2 of 3:item 3 of 3

    #End of the config file. Happy post-processing!

*/



int main(int argc, char **argv){

    //Tokenize the file. An error will produce an empty container.
    auto out = Tokenize_Simple_Config_File("Test_YgorCONFIGTools.cc", ":"); 

    if(out.size() == 0){
        std::cout << "The config file encountered serious errors (does the file exist?)" << std::endl;
        return -1;
    }
    for(auto v_it = out.begin(); v_it != out.end(); ++v_it){
        std::cout << "Tokenized line contains: ";
        for(auto s_it = v_it->begin(); s_it != v_it->end(); ++s_it){
            std::cout << "'" << *s_it << "' ";
        }
        std::cout << std::endl;
    }


    //Test the serialization routine.
    const std::string serialized = Serialize_Simple_Config_File(out, "<THE_DELIMITER>");

    std::cout <<  "--------------------------------------------------------------------------" << std::endl;
    std::cout << "The serialized config file is: " << std::endl;
    std::cout <<  serialized << std::endl;
    std::cout <<  "--------------------------------------------------------------------------" << std::endl;

    //Test the deserialization routine.
    auto reout = Deserialize_Simple_Config_File( serialized, "<THE_DELIMITER>");

    if(reout.size() == 0){
        std::cout << "Config deserialization encountered errors" << std::endl;
        return -1;
    }
    for(auto v_it = reout.begin(); v_it != reout.end(); ++v_it){
        std::cout << "Tokenized line contains: ";
        for(auto s_it = v_it->begin(); s_it != v_it->end(); ++s_it){
            std::cout << "'" << *s_it << "' ";
        }
        std::cout << std::endl;
    }


    return 0;
}
