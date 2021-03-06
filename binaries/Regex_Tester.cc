//Regex_Tester.cc - A simple utility program to test a given regex.


#include <iostream>
#include <string>
#include <vector>
#include <regex>

#include "YgorFilesDirs.h"
#include "YgorString.h"


int main(int argc, char **argv){
    std::string Source;

    // Check to see if a file has been passed, containing the sample text.
    if((argc == 2) && Does_File_Exist_And_Can_Be_Read(argv[1])){
        Source = LoadFileToString(argv[1]);

    // Check to see if the sample test has already been provided.
    }else if(argc == 2){
        Source = argv[1];
        
    }else{
        std::cout << "Input the source text (or representative text to be regexed.)" << std::endl;
        std::cout << "NOTE: Source text can also be passed in as a filename argument or as an argument to this program." << std::endl;
        std::getline(std::cin, Source);
    }

    std::cout << Source << std::endl;

    while(true){
        std::cout << "Now input the regex. The matches will be shown." << std::endl;

        std::string regex_str;
        std::getline(std::cin, regex_str);

        const auto subs = GetAllRegex2(Source, regex_str);

        std::regex compiled_regex( regex_str, std::regex::icase );
        const auto w_match = std::regex_match(Source, compiled_regex);

        std::cout << "Global match? --> " << (w_match ? "true" : "false") << std::endl;
        std::cout << "Subs found " << subs.size() << " matches." << std::endl;

        for(const auto & sub : subs){
            std::cout << "    " << sub << std::endl;
        }
    }

    return 0;
}
