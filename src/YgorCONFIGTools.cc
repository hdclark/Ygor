//YgorCONFIGTools.cc - A collection of routines for parsing/reading configuration files.

#include <fstream>
#include <regex>   //C++11 support is currently incomplete for regex!
#include <string>
#include <vector>
//#include <boost/regex.hpp> //Be sure to compile with -lboost_regex using this library.

//const std::regex regex_first_non_whitespace_is_pound( R"***(^[[:blank:]]*#)***", std::regex::perl | std::regex::icase );
const std::regex regex_first_non_whitespace_is_pound( R"***(^[[:space:]]*#)***", std::regex::ECMAScript | std::regex::icase );
//const std::regex regex_contains_a_colon( R"***(:)***", std::regex::basic | std::regex::icase );

#include "YgorCONFIGTools.h"    
#include "YgorFilesDirs.h"    //Needed for Does_File_Exist_And_Can_Be_Read(..)
#include "YgorMisc.h"         //Needed for FUNCINFO/FUNCWARN/FUNCERR macro-functions.
#include "YgorString.h"       //Needed for SplitStringToVector(...), Canonicalize_String(...).


//A simple config file is one where:
//  1) Each line is self-contained. There is no positional awareness (at least, no structural awareness.)
//  2) Each comment line begins with optional whitespace and non-optional '#'. Non-comment lines with a '#' is OK.
//  3) Each line is tokenized on some character (typically ':'.)
std::vector<std::vector<std::string>> Tokenize_Simple_Config_File(std::string filename_in, std::string linetoken){
    std::vector<std::vector<std::string>> out;

    //Check if the file exists AND can be read. If it cannot, send out an empty vector.
    if( !Does_File_Exist_And_Can_Be_Read(filename_in) ) return out;
    std::fstream FI(filename_in.c_str(), std::ifstream::in);

    //Set up the regex we will need.
    //const std::regex regex_first_non_whitespace_is_pound( R"***(^[[:blank:]]*#)***", std::regex::perl | std::regex::icase );
    const std::regex regex_contains_a_linetoken( linetoken, std::regex::basic | std::regex::icase );
  
    std::string rawline;
    std::vector<std::string> tokenizedline;
    while(!((getline(FI,rawline)).eof()) || !rawline.empty()){
        if( std::regex_search(rawline, regex_first_non_whitespace_is_pound)){ //If the first non-whitespace character is #, then ignore.
            rawline.clear();
            continue;
        }
//        if(  std::regex_search(rawline, regex_first_non_whitespace_is_pound) ) continue;   //If the first non-whitespace character is #, then ignore.
//        if( !std::regex_search(rawline, regex_contains_a_linetoken) ) continue;            //If the line does not contain :, then ignore.
        //Split the line on the given linetoken.
        tokenizedline = SplitStringToVector(rawline, linetoken, 'd');  //'d' for "drop the delimiter."
        for(auto & s_it : tokenizedline){
            //Chomp off extra whitespace (all from front, all from back, shorten whitespace within to a single space.)
            Canonicalize_String(s_it, CANONICALIZE::TRIM);
        }

        //Push back the strings.
        out.push_back( tokenizedline );
        rawline.clear();
    }
    FI.close();
    return out;
}

bool Write_Config_File(const std::vector<std::vector<std::string>> &inmem, std::string filename, std::string linetoken){
    const std::string serialized = Serialize_Simple_Config_File( inmem, linetoken );
    return WriteStringToFile(serialized, filename);
}

//This function will take an in-memory config file and will serialize it. This is useful for, say, transmitting across a network.
//
//NOTE: Although the output is a serial, single string, the string will still contain newlines and other structural characters.
//NOTE: In the output, lines are separated by newlines, as they are on file.
//NOTE: The data will be treated as text. No compression or binary serialization is performed.
std::string Serialize_Simple_Config_File(const std::vector<std::vector<std::string>> &deserialized, std::string linetoken){
    std::string out;
    for(const auto & ds_it : deserialized){
        if(ds_it.size() != 0){
            //We know there is at least one item on this line.
            out += *(ds_it.begin());
            //Dump each additional item on this line. Each comes with a preceeding linetoken.
            for(auto l_it = ++(ds_it.begin()); l_it != ds_it.end(); ++l_it){
                out += linetoken;
                out += *l_it;
            }
            //Indicate the line ending.
            out += '\n';
        }
    }
    return out;
}

std::vector<std::vector<std::string>> Deserialize_Simple_Config_File(const std::string &serialized, std::string linetoken){
    std::vector<std::vector<std::string>> out;
    std::stringstream ss(serialized);

    //Set up the regex we will need.
    //const std::regex regex_first_non_whitespace_is_pound( R"***(^[[:blank:]]*#)***", std::regex::perl | std::regex::icase );
    const std::regex regex_contains_a_linetoken( linetoken, std::regex::basic | std::regex::icase );
    std::string rawline;
    std::vector<std::string> tokenizedline;
    while(!((getline(ss,rawline)).eof()) || !rawline.empty()){
        if( std::regex_search(rawline, regex_first_non_whitespace_is_pound)){ //If the first non-whitespace character is #, then ignore.
            rawline.clear();
            continue;
        }
//        if( !std::regex_search(rawline, regex_contains_a_linetoken) ) continue;          //If the line does not contain the linetoken, then ignore.
        //Split the line on the given linetoken.
        tokenizedline = SplitStringToVector(rawline, linetoken, 'd');  //'d' for "drop the delimiter."
        for(auto & s_it : tokenizedline){
            //Chomp off extra whitespace (all from front, all from back, shorten whitespace within to a single space.)
            Canonicalize_String(s_it, CANONICALIZE::TRIM);
        }
        //Push back the strings.
        out.push_back(tokenizedline);
        rawline.clear();
    }
    return out;
}

//This function will take an in-memory config file and will serialize it twice, producing a truly-serial output (no newlines.)
//
//This is useful for dumping from one program to another (i.e. using std::cin to collect a serialized config file as a single input.)
//
//NOTE: This routine cannot handle content which contains newlines!
std::vector<std::vector<std::string>> Double_Deserialize_Simple_Config_File(const std::string &serialized, std::string separator, std::string linetoken){
    if(separator == linetoken) FUNCERR("This function requires a different separator than the linetoken");
    //const auto serial = ReplaceAllInstances(serialized, linetoken, R"***(\n)***");
    const auto serial = ReplaceAllInstances(serialized, linetoken, "\n");
    return Deserialize_Simple_Config_File(serial, separator);
}

std::string Double_Serialize_Simple_Config_File(const std::vector<std::vector<std::string>> &deserialized, std::string separator, std::string linetoken){
    if(separator == linetoken) FUNCERR("This function requires a different separator than the linetoken");
    for(const auto & r_it : deserialized){
        for(auto c_it = r_it.begin(); c_it != r_it.end(); ++c_it){
            if(c_it->find('\n') != std::string::npos) FUNCERR("This routine cannot handle content which contains newlines. Please encode or remove them before calling this function");
        }
    }
    const auto serial = Serialize_Simple_Config_File(deserialized, separator); //Still contains newlines.
    //return ReplaceAllInstances(serial, R"***(\n)***", linetoken);
    return ReplaceAllInstances(serial, "\n", linetoken);
}



//Operations on in-memory configs. The burden is on the user to ensure the proper method is used for the config.
// For instance, some routines assume a simple 'a = b' format. Using these on a 'a = b = c' format, for instance,
// may produce unreliable output.
bool YgorCONFIGTools_Is_X_Present_on_Leftmost(const std::vector<std::vector<std::string>> &conf, const std::string &X){
    //This function assumes very little. It should work on any config file. 
    //
    //If the given string X is present on any leftmost line, we return 'true'. If it cannot be found, we return 'false.'
    //
    //NOTE: The string comparison is exact and is whitespace, case, etc.. dependant.
    for(const auto & conf_it : conf){
        if(conf_it.size() != 0){
            if(X == *(conf_it.begin())) return true;
        }
    }
    return false;
}

std::string YgorCONFIGTools_Given_X_Get_First_B_When_XB(const std::vector<std::vector<std::string>> &conf, const std::string &X){
    //This function only looks at the first two items of a line. Lines with less items are ignored. Lines with more are
    // treated as though they only have two items (the rest of the line is ignored.)
    //
    //If the given string X is present on any leftmost line, we return the second item. If it cannot be found, we return an
    // empty string.
    //
    //NOTE: The string comparison is exact and is whitespace, case, etc.. dependant.
    for(const auto & conf_it : conf){
        if(conf_it.size() > 1){
            if(X == *(conf_it.begin())) return *(++(conf_it.begin()));
        }
    }
    return std::string();
}



std::vector<std::vector<std::string>> YgorCONFIGTools_Get_All_Lines_With_X_Present_on_Leftmost(const std::vector<std::vector<std::string>> &conf, const std::string &X){
    std::vector<std::vector<std::string>> out;

    for(const auto & l_it : conf){
        if(l_it.size() != 0){
            if(X == *(l_it.begin())) out.push_back( l_it );
        }
    }

    return out;
}

