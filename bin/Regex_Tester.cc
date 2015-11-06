//Regex_Tester.cc - A simple utility program to test a given regex.


#include <string>
#include <vector>

#include "YgorMisc.h"
#include "YgorString.h"
#include "YgorFilesDirs.h"

int main(int argc, char **argv){
    std::string Source;

    //Check to see if a file has been passed, containing the sample text.
    if((argc == 2) && Does_File_Exist_And_Can_Be_Read(argv[1])){
        Source = LoadFileToString(argv[1]);
    }else{
        std::cout << "Input the source text (or representative text to be regexed.)" << std::endl;
        std::cout << "NOTE: Source text can also be passed in as a filename argument." << std::endl;
        std::getline(std::cin, Source);
    }

    std::cout << Source << std::endl;

    while(true){
        std::cout << "Now input the regex. The matches will be shown." << std::endl;

        std::string Regex;
        std::getline(std::cin, Regex);

        const auto matches = GetAllRegex2(Source, Regex);

        std::cout << "Found " << matches.size() << " matches." << std::endl;

        for(auto it=matches.begin(); it!= matches.end(); ++it){
            std::cout << "    " << *it << std::endl;
        }
    }

    return 0;
}
