//YgorString.cc.

#ifndef YGORSTRING_CC_COMMON_TEMPLATE_INSTANTIATIONS
    #define YGORSTRING_CC_COMMON_TEMPLATE_INSTANTIATIONS
#endif

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <utility>
#include <memory>
#include <set>
#include <sstream>
#include <iomanip>    //Needed for std::setprecision(...)
#include <algorithm>  //Needed for set_intersection(..), reverse().
#include <random>
#include <iterator>
#include <chrono>     //Needed to provide backup seeds if std::random_device fails.

//For Canonicalization function.
//#include <locale>                     //Needed for std::toupper()
//const std::locale loc;              // The current locale. Needed for locale-dependent string transformations.
#include <boost/algorithm/string.hpp> //A faster way to get a toupper() function (boost::to_upper).


//#include <regex>                    //Fuck you, silent compiler failing.
#include <boost/regex.hpp>

                      //--------------------------------------------
                      //--------------      NOTE       -------------
                      //--------------------------------------------
                      //  Ensure this file is compiled with the    -
                      //  -lregex flag!                            -
                      //--------------------------------------------
                      //--------------------------------------------

#include "YgorString.h" //Includes namespace constants, function decl.'s, etc..
#include "YgorMisc.h"   //Needed for error functions (for debugging) and isininc macro.




//From: http://daringfireball.net/2010/07/improved_regex_for_matching_urls .
// (?i)\b((?:[a-z][\w-]+:(?:/{1,3}|[a-z0-9%])|www\d{0,3}[.]|[a-z0-9.\-]+[.][a-z]{2,4}/)(?:[^\s()<>]+|\(([^\s()<>]+|(\([^\s()<>]+\)))*\))+(?:\(([^\s()<>]+|(\([^\s()<>]+\)))*\)|[^\s`!()\[\]{};:'".,<>??????~@~\?~@~]?~@~X?~@~Y]))
// but note that some characters (inconveniently) need to be escaped in the source.
//const boost::regex regex_all_urls( "\(\?i)\\b\(\(\?:\[a-z]\[\\w-]+:\(\?:/{1,3}|\[a-z0-9%])|www\\d{0,3}\[.]|\[a-z0-9.-]+\[.]\[a-z]{2,4}/)\(\?:\[^\\s\()<>]+|\\\(\(\[^\\s\()<>]+|\(\\\(\[^\\s\()<>]+\\)))*\\))+\(\?:\\\(\(\[^\\s\()<>]+|\(\\\(\[^\\s\()<>]+\\)))*\\)|\[^\\s`!\()\\\[\\]{};:'\\\".,<>\??????~@~\?~@~]?~@~X?~@~Y]))" , boost::regex::icase );
const boost::regex regex_all_urls( R"***((?i)\b((?:[a-z][\w-]+:(?:/{1,3}|[a-z0-9%])|www\d{0,3}[.]|[a-z0-9.\-]+[.][a-z]{2,4}/)(?:[^\s()<>]+|\(([^\s()<>]+|(\([^\s()<>]+\)))*\))+(?:\(([^\s()<>]+|(\([^\s()<>]+\)))*\)|[^\s`!()\[\]{};:'".,<>??????~@~\?~@~]?~@~X?~@~Y])))***", boost::regex::icase );


//(?i)\b((?:https?://|www\d{0,3}[.]|[a-z0-9.\-]+[.][a-z]{2,4}/)(?:[^\s()<>]+|\(([^\s()<>]+|(\([^\s()<>]+\)))*\))+(?:\(([^\s()<>]+|(\([^\s()<>]+\)))*\)|[^\s`!()\[\]{};:'".,<>??????~@~\?~@~]?~@~X?~@~Y]))
//const boost::regex regex_all_http_urls( "\(\?i)\\b\(\(\?:https\?://|www\\d{0,3}\[.]|\[a-z0-9.\\-]+\[.]\[a-z]{2,4}/)\(\?:\[^\\s\()<>]+|\\\(\(\[^\\s\()<>]+|\(\\\(\[^\\s\()<>]+\\)))*\\))+\(\?:\\\(\(\[^\\s\()<>]+|\(\\\(\[^\\s\()<>]+\\)))*\\)|\[^\\s`!\()\\\[\\]{};:'\\\".,<>\??????~@~\?~@~]?~@~X?~@~Y]))" , boost::regex::icase );
const boost::regex regex_all_http_urls( R"***((?i)\b((?:https?://|www\d{0,3}[.]|[a-z0-9.\-]+[.][a-z]{2,4}/)(?:[^\s()<>]+|\(([^\s()<>]+|(\([^\s()<>]+\)))*\))+(?:\(([^\s()<>]+|(\([^\s()<>]+\)))*\)|[^\s`!()\[\]{};:'".,<>??????~@~\?~@~]?~@~X?~@~Y])))***", boost::regex::icase );

const boost::regex regex_a_number( "[0-9.-eE]*", boost::regex::icase );



#ifndef YGOR_STRING_TURN_OFF_USER_LITERALS
//-------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------ Handy User-Literals ----------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
//Handy (char *) to (std::string) user-literal. Use like: 
//     std::string test = "Ygor string"_s + "Other string"_s;
//     test += "foo"_s + "ba\0r"_s + std::string("baz");  //Will give 'fooba\0rbaz'
std::string operator "" _s(const char* str, size_t len){
    return std::string(str,len);
}
#endif

//-------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------- String Type Conversions --------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
//This function will take the string buffer, copy it to a buffer of type T, and pass out ownership of the new buffer.
// The type T need not match the type of std::string's underlying char_t. The onus is on the user to interpret it.
//
//NOTE: The 'size' parameter is filled with the size of data in the array. The units are in T's. In other words,
// the number of *bytes* of data is size*sizeof(T).
template <class T> std::unique_ptr<T[]> str_to_buf(bool *OK, const std::string &in, uint64_t *size){
    std::unique_ptr<T[]> out;
    if(OK != nullptr) *OK = false;
    if(size == nullptr){
        FUNCWARN("Passed a nullptr instead of a space to store buffer size. Bailing");
        if(OK == nullptr) FUNCERR("Cannot signal error through OK ptr. Cannot continue");
        return std::move(out);
    }
    *size = 0U;
    const auto l_size = in.size(); //Size of string in bytes.

    //Check if the data (appears) to be able to be represented by an array of T's. I don't think this
    // will ever be a problem, because std::string's underlying char type is always a single byte.
    // Leaving this here in case the code is adapted to something else... 
    const auto quotient = l_size/sizeof(T);
    if(static_cast<long long int>(quotient*sizeof(T)) != static_cast<long long int>(l_size) ){
        //NOTE: This is probably not a real problem. In principle, we should just allocate a little more room
        // and let the user handle the divisibility issue themselves. However, in the use cases intended for
        // this code, it is probably better to make it harder for the user to make a mistake. If this 
        // functionality is actually needed, consider either a companion function, a settable parameter, or,
        // in a pinch, try adding a few more elements until the numbers work out.
        FUNCWARN("Data cannot be represented as the specified type. File is of size " << l_size << "b and template has size " << sizeof(T) << "b");
        if(OK == nullptr) FUNCERR("Cannot signal error through OK ptr. Cannot continue");
        return std::move(out);
    }

    out.reset( new T [l_size/sizeof(T)] );      //Allocate space for the entire block to be pulled into memory.
    memcpy((void *)(out.get()), (void *)(in.data()), l_size);
    *size = static_cast<uint64_t>(l_size/sizeof(T));
    if(OK != nullptr) *OK = true;
    return std::move(out);
}
#ifdef YGORSTRING_CC_COMMON_TEMPLATE_INSTANTIATIONS
template std::unique_ptr<uint8_t[]>  str_to_buf(bool *OK, const std::string &in, uint64_t *size);
template std::unique_ptr<uint16_t[]> str_to_buf(bool *OK, const std::string &in, uint64_t *size);
template std::unique_ptr<uint32_t[]> str_to_buf(bool *OK, const std::string &in, uint64_t *size);
template std::unique_ptr<uint64_t[]> str_to_buf(bool *OK, const std::string &in, uint64_t *size);
template std::unique_ptr<int8_t[]>   str_to_buf(bool *OK, const std::string &in, uint64_t *size);
template std::unique_ptr<int16_t[]>  str_to_buf(bool *OK, const std::string &in, uint64_t *size);
template std::unique_ptr<int32_t[]>  str_to_buf(bool *OK, const std::string &in, uint64_t *size);
template std::unique_ptr<int64_t[]>  str_to_buf(bool *OK, const std::string &in, uint64_t *size);
template std::unique_ptr<char[]>     str_to_buf(bool *OK, const std::string &in, uint64_t *size);
template std::unique_ptr<float[]>    str_to_buf(bool *OK, const std::string &in, uint64_t *size);
template std::unique_ptr<double[]>   str_to_buf(bool *OK, const std::string &in, uint64_t *size);
#endif

//-------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------ Self-contained N-gram routines -----------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
//Note: By "N-gram", here we mean that a string like "some string" would be split into the following N-grams:
//                     "some string"   --->   {som, ome, str, tri, rin, ing}
// note that these N-grams do NOT cross whitespace. Other than splitting words, whitespace is ignored.
// To have N-grams which WOULD cross whitespace (but still ignore it,) one will need to move the internal
// while loop outside of the outer loop for these routines (but it shouldn't be too difficult to adjust!) 
//
std::map<std::string,float> NGrams_With_Occurence(const std::string &thestring, long int numb_of_ngrams, long int length_of_ngrams, const unsigned char &type){
    //Pass in numb_of_ngrams = -1 to generate ALL ngrams.
    //Note that the computation is truncated when numb_of_ngrams has been reached - there is *no* selectivity in the output upon truncation.
    std::map<std::string,float> output;
    long int ngrams_generated = 0;
    std::stringstream inss(thestring);
    std::string theword;
    while( inss.good() ){
        inss >> theword;

        if( (type & NGRAMS::CHARS) == NGRAMS::CHARS){   //Character N-grams. These are of a (user) specified length.
            while( static_cast<long int>(theword.size()) >= length_of_ngrams ){
                if( (ngrams_generated >= numb_of_ngrams) && (numb_of_ngrams != -1) ) return output;

                //Push back the current Ngram. Remove the first character for the next cycle. 
                ++ngrams_generated;
                output[theword.substr(0,length_of_ngrams)] += 1.0f;
                theword.erase( theword.begin() );
            }

        }else if((type & NGRAMS::WORDS) == NGRAMS::WORDS){ //Word N-grams. These are NOT of a (user) specified length.
            if(ngrams_generated <= numb_of_ngrams){
                if(theword.size() > 0){
                    output[theword] += 1.0f;
                    ++ngrams_generated;
                }
            }else{
                return output;
            }
        }
    }
    return output;
}

//TODO:  fix the numb_of_ngrams ignoring in the NGRAMS::WORDS case. (and for the above function.)

std::set<std::string> NGrams(const std::string &thestring, long int numb_of_ngrams, long int length_of_ngrams, const unsigned char &type){
    //Pass in numb_of_ngrams = -1 to generate ALL ngrams.
    //Note that the computation is truncated when numb_of_ngrams has been reached - there is *no* selectivity in the output upon truncation.
    std::set<std::string> output;
    long int ngrams_generated = 0;
    std::stringstream inss(thestring);
    std::string theword;
    while( inss.good() ){
        inss >> theword;
        if( (type & NGRAMS::CHARS) == NGRAMS::CHARS){   //Character N-grams. These are of a (user) specified length.
            while( static_cast<long int>(theword.size()) >= length_of_ngrams ){
                if( (ngrams_generated >= numb_of_ngrams) && (numb_of_ngrams != -1) ) return output;

                //Push back the current Ngram. Remove the first character for the next cycle. 
                ++ngrams_generated;
                output.insert( theword.substr(0,length_of_ngrams) );
                theword.erase( theword.begin() );
            }

        }else if((type & NGRAMS::WORDS) == NGRAMS::WORDS){ //Word N-grams. These are NOT of a (user) specified length.
            if(ngrams_generated <= numb_of_ngrams){
                if(theword.size() > 0){
                    output.insert(theword);
                    ++ngrams_generated;
                }
            }else{
                return output;
            }
        }
    }
    return output;
}

std::set<std::string> NGram_Matches(const std::set<std::string> &A, const std::set<std::string> &B){
    std::set<std::string> output;
    std::set_intersection(A.cbegin(), A.cend(), B.cbegin(), B.cend(), std::inserter(output,output.begin()));  
    return output;
}

long int NGram_Match_Count(const std::set<std::string> &A, const std::set<std::string> &B){
    return static_cast<long int>( (NGram_Matches(A,B)).size() );
}

//-------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------- Substring and Subsequence routines ----------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
//Returns the longest common sequential substring. ie. 'ABCDEF' and 'ACDEF' gives 'CDEF'.
std::string LongestCommonSubstring(const std::string &A, const std::string &B){


FUNCWARN("NOTE: IN GENERAL THERE ARE MORE THAN ONE LONGEST COMMON SUBSTRINGS. THIS FUNCTION NEEDS TO BE FIXED!");


// Examples:
//  "ABCDEF_FEDCBA" and "CDEFED" could return "CDE" or "FED". Both are substrings and have equal length.
//  "ABCDEFABC" and "ABC" will uniquely return "ABC". But there are two such substrings. This may be relevant.
//
// The code below finds one of the longest substrings (I think).
//
// You should return a list/vector of substrings. Maybe sort them alphabetically. Do not unique them.
// Provide a few additional routines for 
//   (1) get only the first encountered longest.
//   (2) get the alphanumerically-sorted first.
//   (3) get the length of the longest substring. (This will be unique and can make direct use of this code if needed.)
//
// Might as well tackle LongestCommonSubsequence() while you're working on this function.
// See it below. It has the same issues. It might be even worse to figure out, though.

    if(A.empty() || B.empty()) return "";
    if(B.size() > A.size()) return LongestCommonSubstring(B,A); //Less memory usage.

    std::vector<std::string> curr(B.size(),""), prev(B.size(),""), forswap;
    std::string out;
    for(size_t i = 0; i < A.size(); ++i){
        for(size_t j = 0; j < B.size(); ++j){
            if(A[i] != B[j]){
                if(out.size() < curr[j].size()) out = curr[j];
                curr[j].clear();
            }else{
                if(i == 0 || j == 0){
                    curr[j].clear();
                }else{
                    curr[j] = prev[j-1];
                }
                curr[j] += A[i];
            }
        }
        forswap = std::move(curr);
        curr = std::move(prev);
        prev = std::move(forswap);
    }

    for(size_t i = 0; i < curr.size(); ++i){
        if(out.size() < curr[i].size()) out = curr[i];
        if(out.size() < prev[i].size()) out = prev[i];
    }
    return out;
}

//Returns the longest common subsequence. ie. 'ABCDEF' and 'ACDEF' gives 'ACDEF'.
std::string LongestCommonSubsequence(const std::string &A, const std::string &B){

    FUNCERR("HAVE NOT YET IMPLEMENTED THIS ROUTINE!");

    //NOTE: there is no single longest common subsequence. See LongestCommonSubstring().


    return "";
}



//-------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------- Common text transformations ------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
//This function transforms a given string into the 'canonical' format. This *should* involve handling
// spurious whitespace and transforming to a specific encoding. Future work can fine-tune this, though.
// All strings 'from the wild' are passed through this filter.
std::string & Canonicalize_String(std::string &in, const unsigned char &opts){
    //This function can be used as a filter OR as a pass-in-and-act-on-it function.

    //Transform to all upper case.
    if( (opts & CANONICALIZE::TO_UPPER) == CANONICALIZE::TO_UPPER ){
        //for (size_t i=0; i<in.length(); ++i) in[i] = toupper(in[i],loc);  // using the C++ <locale> way.
        boost::to_upper(in);  // Using the Boost way.
    }

    //Localization, transformation to particular encoding.
    if( (opts & CANONICALIZE::LOCALIZE) == CANONICALIZE::LOCALIZE ){
        // ...
    }


    //Whitespace filter. Works for beginning, end, and interim whitespaces.
    if( (opts & CANONICALIZE::TRIM) == CANONICALIZE::TRIM ){
        //stringstream >> will spit out an empty string if there is trailing space. This routine *requires* such space!
        std::stringstream ss(in+" ");
        if(!ss.good()) return in;
        in.clear();
        std::string temp;
        ss >> temp;
        do{
            if(!temp.empty()) in += temp;
            ss >> temp;
            if(!temp.empty() && ss.good()) in += " ";
        }while(ss.good());
    }

    //Whitespace filter. Works for beginning, end, and interim whitespaces.
    if( (opts & CANONICALIZE::TRIM_ENDS) == CANONICALIZE::TRIM_ENDS ){
        const std::string whitespace(" \t"); //\f\v\n\r"); //Consider vertical tabs and newlines too? Don't assume so...

        //Trailing.
        const auto last_non = in.find_last_not_of(whitespace);
        if(last_non != std::string::npos){
            in.erase(last_non+1);
        }else{ //All whitespace!
            in.clear();
        }

        //Preceeding.
        const auto first_non = in.find_first_not_of(whitespace);
        if(first_non != std::string::npos){
            in.erase(0,first_non);
        }else{ //All whitespace!
            in.clear();
        }
    }

    //Remove ALL whitespace, leaving a single, long, whitespace-less string (in the order they originally were in.)
    if( (opts & CANONICALIZE::TRIM_ALL) == CANONICALIZE::TRIM_ALL ){
        std::stringstream inss(in + " ");
        in.clear();
        std::string theword;
        while(inss.good()){
            inss >> theword;
            if(!theword.empty() && inss.good()) in += theword;
        }
    }

    //Remove all non-[ A-Za-z] characters.
    if( (opts & CANONICALIZE::TO_AZ) == CANONICALIZE::TO_AZ ){
        for(auto it = in.begin(); it != in.end(); ++it) if( (*it) != ' '){
            if( !(isininc('A',*it,'Z') || isininc('a',*it,'z')) ){ // !( ((*it) >= 'A') && ((*it) <= 'Z') )  && !( ((*it) >= 'a') && ((*it) <= 'z') ) ){
                in.erase(it);
                --it;
            }
        }
    }

    //Remove all non-[ 0-9-.] characters.
    if( (opts & CANONICALIZE::TO_NUM) == CANONICALIZE::TO_NUM ){
        for(auto it = in.begin(); it != in.end(); ++it) if( (*it) != ' '){
            if(!( isininc('1',*it,'9') || (*it == '0') || (*it == '.') || (*it == '-')  )){
                in.erase(it);
                --it;
            }
        }
    }

    //Remove all non-[ A-Za-z0-9-.]
    if( (opts & CANONICALIZE::TO_NUMAZ) == CANONICALIZE::TO_NUMAZ ){
        for(auto it = in.begin(); it != in.end(); ++it) if( (*it) != ' '){
            if(!( isininc('1',*it,'9') || (*it == '0') || (*it == '.') || (*it == '-') || isininc('A',*it,'Z') || isininc('a',*it,'z') )){
                in.erase(it);
                --it;
            }
        }
    }

    //Common replacements (ie. a '_' instead of a ' ', etc..)

    // ...

    return in;
}

//Prefer this function to the other. Eventually, move the other function here and remove it.
// I don't think it works out being any faster to deal with references because of r-values.
std::string Canonicalize_String2(const std::string &in, const unsigned char &mask){
    std::string temp(in);
    Canonicalize_String(temp, mask);
    return temp;
}

//Attempt to ~intelligently replace non-simple chars like spaces and commas.
//
//NOTE: This function is incomplete because it should be added to as annoyances are found.
//NOTE: Do NOT depend on any specific format of string being emitted from this function, 
//      except that they will probably be suitable for a filename and easy bash interpretation.
std::string Detox_String(const std::string &in){
    std::string out(in);

    //Replace annoying characters with underscores.
    for(auto it = out.begin(); it != out.end(); ++it){
        if((*it == ',') || (*it == ' ') ||
           (*it == '\'') || (*it == '\"') ||
            (*it == '(') || (*it == ')') ||
            (*it == '{') || (*it == '}') ||
            (*it == '|') || (*it == '+') ||
            (*it == '~') || (*it == '!') ||
            (*it == '#') || (*it == '@') ||
            (*it == '$') || (*it == '%') ||
            (*it == '&') || (*it == '^') ||
            (*it == ':') || (*it == ';') ||
            (*it == '<') || (*it == '>') ||
            (*it == '?') || (*it == '*') || (*it == '`') ||
        (*it == '\\') || (*it == '/')) *it = '_';
    }

    if(out.size() <= 1) return out;

    //Condense all underscore runs to a single underscore.
    auto it1 = out.begin();
    while(it1 != out.end()){
        auto it2 = std::next(it1,1);
        if((*it1 == '_') && (*it2 == '_')){
            out.erase(it2);
        }else{
            ++it1;
        }
    }
    return out;
}

//Removes lowest filename or dir (via '/'), returns parent directory 
// with trailing slash intact.
//
//NOTE: No check is performed to see if EITHER given or outgoing paths
//       are legitimate. 
//NOTE: On failure, an empty string is returned.
//NOTE: This routine does not know about the filesystem, nor '.', './',
//       or '../'. If these are required, explicitly provide them!
//       (If they were added, an arbitrary number of '../' could be 
//       added - probably not useful for most purposes.) 
//NOTE: If the full path is required, but input is not controlled, try
//       using `basename` or similar (See YgorFilesDirs). ((This will 
//       only work when the file/path in question actually exists.))
//
//EXAMPLES:
//    '/some/file'  --> '/some/'
//    '/some/path/' --> '/some/'
//    'some/path/'  --> 'some/'
//    '/some/'      --> '/'
//    '/'           --> ''  (failure - no parent)
//    'file'        --> ''  (failure - we don't do '.' or '..')
//    '../file'     --> '../'
//    './file'      --> './'
//    '../../../'   --> '../../'
//    './.././'     --> './../'
std::string Get_Parent_Directory(const std::string &path){
    //Trim the edges. Probably not strictly needed...
    std::string out(Canonicalize_String2(path,CANONICALIZE::TRIM_ENDS));
    if(out.empty()) return out; //Failure.    

    //Reverse the string.
    std::reverse(out.begin(), out.end());
   
    //If the first char is a slash, remove if.
    if(out[0] == '/') out.erase(0,1);
    if(out.empty()) return out; //Failure.    

    //Remove everything up until the first '/' is found.
    const auto first = out.find('/');
    if(first != std::string::npos){
        out.erase(0,first); //Leaves '/' intact.
    }else{ //No slash found. This is a failure.
        out.clear();
    }
    if(out.empty()) return out; //Failure.    

    //Reverse the string.
    std::reverse(out.begin(), out.end());
    return out;
}

//Returns substr after last '/'. If no '/', returns input string.
//
//NOTE: This routine knows nothing about the filesystem. The file need not exist, 
// and the path could be bogus; this routine doesn't care. It just works on '/'s.
std::string Get_Bare_Filename(const std::string &fullname){
    //Get the location of the last '/'.
    const size_t indx_last = fullname.find_last_of('/');

    if(indx_last == std::string::npos) return fullname;
    return fullname.substr(indx_last + 1);
}


//Compare sections of two strings for equality. After safely pulling out the sections, perform canonicalization on the
// segments before comparison. This is useful for ignoring case and whitespace, for example.
//
//NOTE: The from_X and to_X numbers are zero-based. They can safely be out of range; the comparison will 
//      safely fail.
//
//NOTE: If the user asks for a range, both strings MUST satisfy the range in order to attempt comparison.
//      For example, if A = "abc" and B = "abc" but the user has both from_X = 0 and both to_X = 3, then this
//      function will return FALSE. Why? Because the user specified that the matching section must be exactly
//      four characters in length (i.e., the 0th, 1st, 2nd, and 3rd characters).
//
//NOTE: Be careful with the from_X and to_X numbers. They are applied AFTER the canononicalization!
//
bool Safely_Compare_Strings(const std::string &A, unsigned int from_A, unsigned int to_A, const unsigned char mask_A, 
                            const std::string &B, unsigned int from_B, unsigned int to_B, const unsigned char mask_B){

    const std::string AA = Canonicalize_String2(A,mask_A);
    const std::string BB = Canonicalize_String2(B,mask_B);

    if( (to_A == from_A) && (to_B == from_B) ) return true; //Both are the empty string ... they match!
    if( (to_A < from_A) || (to_B < from_B) ) return false;
    if( (to_A + 1) > AA.size() ) return false;
    if( (to_B + 1) > BB.size() ) return false;

    const std::string AAA = AA.substr(from_A, (to_A - from_A) + 1);
    const std::string BBB = BB.substr(from_B, (to_B - from_B) + 1);

    return (AAA == BBB);
}



//-------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------ Math expression transformations ----------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
//This function removes redundant parenthesis on the outer edges of an expression.
// eg.   ((((x+(y+z)))))   --->    x+(y+z)
// eg.      (a+b)(a+b)     --->  (a+b)(a+b)
std::string Remove_Unneeded_Surrounding_Parenthesis(const std::string &in, char L, char R){
    if(in.empty()) return in;
    std::string out(in);
    while( (out.front() == L) && (out.back() == R) ){
        //We use depths or 'levels' to denote how deep we are nested.
        // If the level goes below 0, we cannot chop off outer parenthesis.
        //
        //Note: level == openparens-1.
        long int level(0);
        for(auto it = ++(out.begin()); (it != --(out.end())) && (level >= 0); ++it){
            if(*it == L) ++level;
            if(*it == R) --level;
        }
        if(level < 0) break;
        out = out.substr(1, (out.size()-1)-1);
    }
    return std::move(out);
}

//This function explicitly wraps objects separated by commas into parenthesis-delimited blocks.
// Only touches commas at the top-level parenthesis depth level.
// eg.    (1+1-2,atan2(x,y),z)   --->   (1+1-2),(atan2(x,y)),(z)
// eg.     1+1-2,atan2(x,y),z    --->   (1+1-2),(atan2(x,y)),(z)
// eg.   ((1+1-2,atan2(x,y),z))  --->   (1+1-2),(atan2(x,y)),(z)
std::string Wrap_Comma_Separated_Stuff_At_Same_Depth(const std::string &in, char L, char R){
    //Troublesome expressions like: if(1+1-2,x,y) will get split up into incorrect trees
    // unless the commas are enclosed with parenthesis. 
    //
    //This function scans the input and tries to wrap objects separated by commas (at 
    // the same depth) in parenthesis.
    std::string out;
    const std::string inn(Remove_Unneeded_Surrounding_Parenthesis(in, L, R));

    long int parendepth(0);
    bool foundcommas = false;
    for(auto it = inn.begin(); it != inn.end(); ++it){
        if(*it == L)  ++parendepth;
        if(*it == R)  --parendepth;
        if((*it == ',') && (parendepth == 0)){
            out += std::string(1,R) + ","_s + std::string(1,L);
            foundcommas = true;
        }else{
            out += *it;
        }
    }
    if(foundcommas) return std::string(1,L) + out + std::string(1,R);
    return std::move(out);
}


//Looks for unmatched pairs of input. Typically used with '(' and ')', or other parenthesis. 
// Will work for quotations too.
//
// eg.    ((x+y))  ---> false
// eg.    )(x+y)(  ---> false
// eg.    [(x+y])  ---> false
// eg.    ((x+y)   ---> true
//Note: Will not find unpaired/balanced chars like '([x)]'!
bool Contains_Unmatched_Char_Pairs(const std::string &in, char L, char R){
  long int parens(0);
  for(auto it = in.begin(); it != in.end(); ++it){
      if( *it == L ) ++parens;
      if( *it == R ) --parens;
  }
  return (parens != 0);
}

//Removes all occurences of any of the characters in `chars' from the front of `in'. 
// Removal stops as soon as a non-`chars' character is encountered.
std::string Remove_Preceeding_Chars(const std::string &in, const std::string &chars){
    std::string out(in);
    std::reverse(out.begin(),out.end()); //Reverse the input. Preceeding => trailing.
    out = Remove_Trailing_Chars(out,chars);
    std::reverse(out.begin(),out.end()); //Restore the original order, sans any preceeding chars.
    return out;
}

//Removes all occurences of any of the characters in `chars' from the back of `in'. 
// Removal stops as soon as a non-`chars' character is encountered.
std::string Remove_Trailing_Chars(const std::string &in, const std::string &chars){
    std::string out(in);
    for(auto it = out.rbegin(); it != out.rend();   ){
        //Look for the current character in the blacklist (i.e., `chars').
        if(chars.find(*it) != std::string::npos){
            //This is a character which should be trimmed.
            out.pop_back(); //out.erase(it);
            it = out.rbegin();
        }else{
            //This is the first non`chars' character. Time to bail.
            break;
        }
    }
    return out;
}



//-------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------- Common text conversions --------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
std::string inttostring(long int number){
    std::stringstream ss;
    ss << number;
    return ss.str();
}

std::string floattostring(float number){
    std::stringstream ss;
    ss << number;
    return ss.str();
}

template <class T>  std::string Xtostring(T numb){
    std::stringstream ss;
    ss << numb;
    return ss.str();
}
#ifdef YGORSTRING_CC_COMMON_TEMPLATE_INSTANTIATIONS
    template std::string Xtostring<int>(int);
    template std::string Xtostring<long int>(long int);
    template std::string Xtostring<long long int>(long long int);
    template std::string Xtostring<unsigned int>(unsigned int);
    template std::string Xtostring<unsigned long int>(unsigned long int);
    template std::string Xtostring<unsigned long long int>(unsigned long long int);
    template std::string Xtostring<float>(float);
    template std::string Xtostring<double>(double);

    template<> std::string Xtostring<std::string>(std::string in){ return in; }
#endif

template <class T>   std::string XtoPreciseString(T numb){
    std::stringstream ss;
    ss << std::setprecision(500) << numb;
    return ss.str();
}
#ifdef YGORSTRING_CC_COMMON_TEMPLATE_INSTANTIATIONS
    template std::string XtoPreciseString<int>(int);
    template std::string XtoPreciseString<long int>(long int);
    template std::string XtoPreciseString<long long int>(long long int);
    template std::string XtoPreciseString<unsigned int>(unsigned int);
    template std::string XtoPreciseString<unsigned long int>(unsigned long int);
    template std::string XtoPreciseString<unsigned long long int>(unsigned long long int);
    template std::string XtoPreciseString<float>(float);
    template std::string XtoPreciseString<double>(double);

    template<> std::string XtoPreciseString<std::string>(std::string in){ return in; }
#endif

template <class T>   T stringtoX(const std::string &text){
    std::stringstream ss(text);
    T temp;
    ss >> temp;
    return temp;
}
#ifdef YGORSTRING_CC_COMMON_TEMPLATE_INSTANTIATIONS
    template float                    stringtoX<float>(const std::string &);
    template double                   stringtoX<double>(const std::string &);
    template int                      stringtoX<int>(const std::string &);
    template unsigned int             stringtoX<unsigned int>(const std::string &);
    template long int                 stringtoX<long int>(const std::string &);
    template long long int            stringtoX<long long int>(const std::string &);
    template unsigned long int        stringtoX<unsigned long int>(const std::string &);
    template unsigned long long int   stringtoX<unsigned long long int>(const std::string &);

    template<> std::string stringtoX<std::string>(const std::string &in){ return in; }
#endif

template <class T> bool Is_String_An_X(const std::string &text){
    std::stringstream ss(text);
    T d;
    if(ss >> d){
        //Test if there is no more data remaining. If there is, then the string is NOT a 'T'.
        ss.peek();
        if(ss.eof()){
            return true;
        }else{
            return false;
        }
    }
    return false;
}
#ifdef YGORSTRING_CC_COMMON_TEMPLATE_INSTANTIATIONS
    template bool  Is_String_An_X<float>(const std::string &);
    template bool  Is_String_An_X<double>(const std::string &);
    template bool  Is_String_An_X<int>(const std::string &);
    template bool  Is_String_An_X<unsigned int>(const std::string &);
    template bool  Is_String_An_X<long int>(const std::string &);
    template bool  Is_String_An_X<long long int>(const std::string &);
    template bool  Is_String_An_X<unsigned long int>(const std::string &);
    template bool  Is_String_An_X<unsigned long long int>(const std::string &);

    template<> bool Is_String_An_X<std::string>(const std::string &in){ return true; }
#endif

//This is useful for parsing a config file. Provide a key like "aval = " and if the line is "aval = 6" then you'll
// get a 6 back. 
//
//NOTE: This routine will ONLY alter the output's contents on success.
//
//NOTE: This routine will not handle quotation marks. The RHS of the line must be 'bare'.
//
//NOTE: Your choice of canonicalization will be reflected in the output. If you TO_UPPER the input,
//      any output will also be TO_UPPER'd. (Working around this here would be quite tricky. It would
//      be much easier to do in your context-aware code, if required.)
//
//NOTE: The canonicalization will happen ASAP. Ensure your keys reflect this. (i.e., if you are trimming all
//      consecutive whitespace to a single space, ensure your key is written as "your key = " instead of
//      "your key =" - the very last space may be significant, dependending on what you're doing!)
// 
//      For example, passing the key "    someval =  " with flag '::TRIM_ALL' will give identical outcome as
//      passing "someval=". Be aware of this when specifying your keys!
//
template <class T>
bool GetValueIfKeyMatches(T *out, const std::string &key,  const unsigned char key_mask, 
                                  const std::string &line, const unsigned char line_mask){
    if(out == nullptr) FUNCERR("Passed a nullptr - no way return matches. Cannot continue");

//    if(line.size() <= key.size()) return false;
//
// If we do the canonicalization in the comparison, we have to redo it again right after if true.
//    if(!Safely_Compare_Strings(line,0,key.size()-1,line_mask,
//                                key,0,key.size()-1, key_mask) ) return false;

    const std::string key_C = Canonicalize_String2(key,key_mask);
    const std::string lineC = Canonicalize_String2(line,line_mask);

    if(lineC.size() <= key_C.size()) return false;

    if(!Safely_Compare_Strings(lineC,0,key_C.size()-1,0,
                               key_C,0,key_C.size()-1,0) ) return false;

    const std::string remaining_line = lineC.substr(key_C.size());
    if(!Is_String_An_X<T>(remaining_line)) return false;
    *out = stringtoX<T>(remaining_line);
    return true;
}
#ifdef YGORSTRING_CC_COMMON_TEMPLATE_INSTANTIATIONS
    template bool GetValueIfKeyMatches(float                  *, const std::string &, const unsigned char, const std::string &, const unsigned char);
    template bool GetValueIfKeyMatches(double                 *, const std::string &, const unsigned char, const std::string &, const unsigned char);
    template bool GetValueIfKeyMatches(int                    *, const std::string &, const unsigned char, const std::string &, const unsigned char);
    template bool GetValueIfKeyMatches(unsigned int           *, const std::string &, const unsigned char, const std::string &, const unsigned char);
    template bool GetValueIfKeyMatches(long int               *, const std::string &, const unsigned char, const std::string &, const unsigned char);
    template bool GetValueIfKeyMatches(long long int          *, const std::string &, const unsigned char, const std::string &, const unsigned char);
    template bool GetValueIfKeyMatches(unsigned long int      *, const std::string &, const unsigned char, const std::string &, const unsigned char);
    template bool GetValueIfKeyMatches(unsigned long long int *, const std::string &, const unsigned char, const std::string &, const unsigned char);
    template bool GetValueIfKeyMatches(std::string            *, const std::string &, const unsigned char, const std::string &, const unsigned char);
#endif



//This function takes a string (i.e. an array of chars), cycles through looking for un-printable characters, and 
// converts them to hex for easy printing. This is a one-way function, and we cannot go backward! 
// Output is for human consumption only!
//
//NOTE: This function will destroy carriage returns, newlines, tabs, etc... This is intentional.
std::string Convert_Unprintables_to_Hex(const std::string &in){
    std::stringstream ss;
    for(auto it = in.begin(); it != in.end(); ++it){
        int v(static_cast<int>(static_cast<unsigned char>(*it)));
        if(isininc(32,v,126)){
            ss << *it;
        }else{
            ss << "(0x" << std::hex << v << std::dec << ")";
        }
    }
    return ss.str();
}

//This function escapes a chunk of static text. The output is properly quoted with either ' or "s,
// and anything which needs to be escaped will be.
std::string Quote_Static_for_Bash(const std::string &in){
    std::string out("\"");
//    out += "'";
    for(auto it = in.begin(); it != in.end(); ++it){
        if(*it == '\''){
            out += R"***('\'')***";
        }else{
            out += *it;
        }
    }
//    out += "'";
    return out + '"';
}

//This function escapes a chunk of possibly-expandable text. The output is possibly quoted with "s,
// and anything which needs to be escaped will be.
std::string Quote_Expandable_for_Bash(const std::string &in){
    std::string out("\"");
    for(auto it = in.begin(); it != in.end(); ++it){
        if(*it == '"'){
            out += R"***(\")***";
        }else{
            out += *it;
        }
    }
//    out += '"';
    return out + '"';
}


//-------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------- Generic String-related Routines -----------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------

//This function looks very similar to one in YgorMisc.h/cc. It should replace it eventually.
std::string Generate_Random_String_of_Length(long int len){
    std::string out;
    static const std::string alphanum(R"***(0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz)***");
    std::default_random_engine gen;

    //Seed the generator. It would be best to be able to pass in an optional std::seed_sequence. Also, to ensure the
    // quality is sufficient, incorporating the entropy() might be useful (read some issues about the implementation at
    // the moment, though...).
    try{
        std::random_device rd;  //Constructor can fail if many threads create instances (maybe limited fd's?).
        gen.seed(rd()); //Seed with a true random number.
    }catch(const std::exception &){
        // --- Maybe we should issue a warning now? ---
        // Is it necessary for this routine? Do we need truly random? Can we just say 'write your own' if you need
        // something more robust?
        //
        // Not sure that issuing a warning would be good, especially if this routine is mostly failing due to many
        // concurrent threads.

        //unsigned timeseed = std::chrono::system_clock::now().time_since_epoch().count();
        const auto timeseed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        gen.seed(timeseed); //Seed with time. 
    }

    std::uniform_int_distribution<int> dist(0,alphanum.length()-1);
    for(int i=0; i<len; ++i) out += alphanum[dist(gen)];
    return out;
}

//This function will taken a vector of strings (typically, a tokenized string) and lineate it into a single string.
std::string Lineate_Vector(const std::vector<std::string> &in, const std::string &separator){
    std::string out;
    for(auto it = in.begin(); it != in.end(); ){
        out += *it;
        ++it;
        if(it != in.end()) out += separator;
    }
    return out;
}



//-------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------- Basic parsing routines ---------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------


//This function takes a string (s), a delimiter, and a vector (elems) and splits the string into tokens.
//
//Use the function below ("split" with two arguments) for a version which does not require a vector as input,
// but is slower (and more costly.)
//
//The character BEHAVIOUR denotes how the delimiter should be handled. 
//   d - To drop the delimiter altogether, 
//   l - Keep the delimiter on the left side of the break (last character of the line,)
//   r - Put the delimiter on the right side of the break (first character of next line.)
std::vector<std::string> &BYOVectorSplit(const std::string &s, char delim, std::vector<std::string> &elems, char BEHAVIOUR){
    std::stringstream ss(s);
    std::string item;
    size_t count = 0;

    while(std::getline(ss, item, delim)){
        if(item.size() != 0){
            if(BEHAVIOUR == 'd'){
                elems.push_back(item);
            }else if(BEHAVIOUR == 'l'){
                elems.push_back(item + delim);
            }else if(BEHAVIOUR == 'r'){
                if(count == 0){
                    elems.push_back(item);
                }else{
                    elems.push_back(delim + item);
                }
            }
        }
        ++count;
    }
    return elems;
}

std::vector<std::string> SplitStringToVector(const std::string &s, char delim, char BEHAVIOUR){
    std::vector<std::string> elems;
    return BYOVectorSplit(s, delim, elems, BEHAVIOUR);
}

std::vector<std::string> SplitStringToVector(const std::string &s, const std::string &delim, char BEHAVIOUR){
    std::vector<std::string> out;

    //First, we append the delimiter to the end of the string.
    const boost::regex regex( delim );

    //Iterate over all the non-matches. The '-1" means iterate over non-matches.
    boost::sregex_token_iterator iter( s.begin(), s.end(), regex, -1 );
    boost::sregex_token_iterator end;

    //Iterate over matches, storing them if they 
    for( ; iter != end; ++iter ){
        out.push_back( *iter );
    }
    if(out.size() == 0) return out;

    if(BEHAVIOUR == 'd'){
        return out;
    }else if(BEHAVIOUR == 'l'){
        //Append the delimiter to the end of all elements (except the last, where we have inserted an extra one.)
        for(auto it=out.begin(); it!=--(out.end()); ++it) *it += delim;
        return out;
    }else if(BEHAVIOUR == 'r'){
        //Append the delimiter to the beginning of all elements (except the first, where there was no preceeding one.)
        for(auto it=++(out.begin()); it!=out.end(); ++it) *it = delim + *it;
        return out;
    }
 
    FUNCWARN("BEHAVIOUR = " << BEHAVIOUR << " not recognized. Treating as 'd' and continuing");       
    return out;
}

std::vector<std::string> SplitVector(const std::vector<std::string> &s, char delim, char BEHAVIOUR){
    std::vector<std::string> elems;
    for(size_t i=0; i<s.size(); ++i){
        std::vector<std::string> temp = SplitStringToVector(s[i], delim, BEHAVIOUR);
        elems.insert(elems.end(), temp.begin(), temp.end());
    }
    return elems;
}

//This function replaces occurences of some regex in a string with some replacement text.
std::string ReplaceAllInstances(const std::string &in, const std::string &regex, const std::string &replacement){
    const boost::regex boost_regex(regex.c_str(), boost::regex::icase );
    return regex_replace( in, boost_regex, replacement );
}

//This function removes all instances of the input characters from the provided string.
std::string PurgeCharsFromString(const std::string &in, std::string purge_chars){
    size_t i=0;
    std::string out(in);
    do{
        //Notice that for a match to happen it is enough that one of the characters matches 
        // in the string (any of them). To search for an entire sequence of characters use 
        // find instead.
        i = out.find_first_of(purge_chars, i);
        if( i == std::string::npos ) break;

        out.erase( out.begin() + i );
    }while(true);
    return out;
}


std::vector<std::string> GetUrls( std::vector<std::string> &in ){
    std::vector<std::string> urls;
    for(size_t i=0; i<in.size(); ++i){
        boost::sregex_token_iterator iter(in[i].begin(), in[i].end(), regex_all_http_urls, 0);
        boost::sregex_token_iterator end;
        for( ; iter != end; ++iter ) {
            urls.push_back( *iter );
        }
    }
    return urls;
}

std::vector<std::string> GetUrls( const std::string &in ){
    std::vector<std::string> temp;
    temp.push_back(in);
    return GetUrls( temp );
}

std::string GetFirstNumber(std::string &in){
    boost::sregex_token_iterator iter(in.begin(), in.end(), regex_a_number, 0);
    boost::sregex_token_iterator end;

    //--- FYI:
    //Although we only return the first (non-empty) string, we could equally well use this code to iterate
    // through ALL matches found (and thus return a vector or something.)
    for( ; iter != end; ++iter ){
        if(iter->length() != 0){
            return *iter;
        }
    }
    return "-1";
}

std::string GetFirstRegex(const std::string &source, boost::regex &regex_the_query){
    boost::smatch match;
    
    //Returns TRUE if there was a match.
    if(boost::regex_search(source, match, regex_the_query)){ //, boost::match_extra)){
        //Check for sub-matches (ie. parenthesis within the regex query string.)
        //
        //For now, we will do something silly and only return the last sub-match, instead of a list of all submatches.
        // This is a handicap, but in principle one could just redo the regex and iterate over the sub-match part, 
        // eventually picking them all up. This is costly, but easy to think about.
        for(long int i = match.size()-1; i>=0; --i){
            std::string submatch(match[i].first, match[i].second);
            return submatch;
        }
    }  
    return std::string();
}

std::string GetFirstRegex(const std::string &source, std::string query){
    //Make a regex unit from the query.
    boost::regex regex_the_query( query.c_str(), boost::regex::icase );
    return GetFirstRegex(source, regex_the_query);
}

std::string GetFirstRegex(std::vector<std::string> &source, std::string query){
    //Make a regex unit from the query.
    boost::regex regex_the_query( query.c_str(), boost::regex::icase );

    for(size_t i=0; i<source.size(); ++i){
        std::string dumb = GetFirstRegex(source[i], regex_the_query);
        if(dumb != "") return dumb;
    }
    return std::string();
}


std::vector<std::string> GetAllRegex(std::vector<std::string> &source, boost::regex &regex_the_query){
    FUNCWARN("This function has been found to be invalid - Swap the GetAllRegex2 codes into this code to see if anything changes!");
    #pragma message "Warning - This function has been found to be invalid - Swap the GetAllRegex2 codes into this code to see if anything changes!"

    std::vector<std::string> outgoing;
    const boost::sregex_token_iterator end;

    for(size_t i=0; i<source.size(); ++i){
        //Perform the matching/finding. We get an iterator over matches.
        boost::sregex_token_iterator iter(source[i].begin(), source[i].end(), regex_the_query, 0);

        //Iterate over matches, storing them if they 
        for( ; iter != end; ++iter ){
            if((*iter).length() != 0){
                outgoing.push_back( *iter );
            }
        }
    }
    return outgoing;
}

std::vector<std::string> GetAllRegex(std::vector<std::string> &source, std::string query){
    //Make a regex unit from the query.
    boost::regex regex_the_query( query.c_str(), boost::regex::icase );
    return GetAllRegex(source, regex_the_query);
}

std::vector<std::string> GetAllRegex(std::string &source, std::string query){
    //Make a regex unit from the query.
    boost::regex regex_the_query( query.c_str(), boost::regex::icase );

    //Encapsulate the source string into a vector.
    std::vector<std::string> encapsed_source;
    encapsed_source.push_back( source );

    return GetAllRegex(encapsed_source, regex_the_query);
}



std::vector<std::string> GetAllRegex2(const std::string &source, std::string query){
    std::vector<std::string> out;
    boost::regex regex_the_query( query.c_str(), boost::regex::icase );
    boost::sregex_iterator it(source.begin(), source.end(), regex_the_query);
    boost::sregex_iterator end;
    for( ; it != end; ++it){
        for(decltype((*it).size()) i = 1; i < (*it).size(); ++i){ //The Zeroth contains the entire match (not the matchs enclosed in parenthesis!)
            if((*it)[i].matched) out.push_back( (*it)[i].str());
        }
    }
    return out;
}

//This function goes through a vector of strings, finds a matching line, and returns the Nth line below the match.
std::string GetLineNBelow( std::vector<std::string> &source, std::string query, long int N){
    for(long int i=0; i<static_cast<long int>(source.size()); ++i){
        if( std::string::npos != (source[i]).find(query) ){
            if(((i+N) >= 0) && ((i+N) < static_cast<long int>(source.size()))){
                return source[i+N];
            }
        }
    }
    return std::string();
}


//This function (lazily) matches regex on a vector and returns the elements within the matches (endpoints inclusive.)
//
//Same-line matching is NOT handled properly. It would be better to handle same-line matching with a string input.
std::vector<std::vector<std::string> > GetSubVectorFromTo( std::vector<std::string> &in, std::string from, std::string to){
    std::vector<std::vector<std::string> > matches;  //<--- this is the output. Each element is a subvector of the source between endpoints.
    size_t i = 0;
    const boost::regex regex_from(from.c_str(), boost::regex::icase );
    const boost::regex regex_to(to.c_str(), boost::regex::icase );

    while(i < in.size()){
        std::vector<std::string> subvector;
    
        //Look for the first tag.
        for( ; i<in.size(); ++i){
            if( boost::regex_search(in[i], regex_from) ){
               subvector.push_back( in[i] ); 
               ++i;                                        //NOTE: This routine will not properly handle single-line matches!!!
               break;
            }
        }
    
        //Grab each line until the second tag is found (or the end is hit.)
        for( ; i<in.size(); ++i){
            subvector.push_back( in[i] );
            if( boost::regex_search(in[i], regex_to) ){         
               ++i;                                        //NOTE: This routine will not properly handle single-line matches!!!
               break;
            }
        }
        matches.push_back( subvector );
    }
    return matches;
}


//-------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------- URL-handling routines ---------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------

//Found at http://www.codeguru.com/cpp/cpp/algorithms/strings/article.php/c12759/URI-Encoding-and-Decoding.htm
std::string Basic_Encode_URL(const std::string & in){
   const char DEC2HEX[16 + 1] = "0123456789ABCDEF";
   const unsigned char * pSrc = (const unsigned char *)in.c_str();
   const int SRC_LEN = in.length();
   unsigned char * const pStart = new unsigned char[SRC_LEN * 3];
   unsigned char * pEnd = pStart;
   const unsigned char * const SRC_END = pSrc + SRC_LEN;
   for (; pSrc < SRC_END; ++pSrc)
   {
      if (pSrc != nullptr)
         *pEnd++ = *pSrc;
      else
      {
         // escape this char
         *pEnd++ = '%';
         *pEnd++ = DEC2HEX[*pSrc >> 4];
         *pEnd++ = DEC2HEX[*pSrc & 0x0F];
      }
   }
   std::string sResult((char *)pStart, (char *)pEnd);
   delete [] pStart;
   return sResult;
}


std::string Basic_Decode_URL(const std::string &in){
    std::string out(in);
    // FIXME ??    See: http://www.w3schools.com/tags/ref_entities.asp

    out = ReplaceAllInstances(out, "[%]7C", "|");
    out = ReplaceAllInstances(out, "[%]26", "&");
    out = ReplaceAllInstances(out, "[%]3A", ":");
    out = ReplaceAllInstances(out, "[%]2F", "/");
    out = ReplaceAllInstances(out, "[%]3F", "?");
    out = ReplaceAllInstances(out, "[%]3D", "=");

    out = ReplaceAllInstances(out, "&[Qq][Uu][Oo][Tt];", "\"");
    out = ReplaceAllInstances(out, "&#34;", "\"");

    out = ReplaceAllInstances(out, "&[Aa][Pp][Oo][Ss];", "'");
    out = ReplaceAllInstances(out, "&#39;", "'");

    out = ReplaceAllInstances(out, "&[Aa][Mm][Pp];", "&");
    out = ReplaceAllInstances(out, "&#38;", "&");

    out = ReplaceAllInstances(out, "&[Ll][Tt];", "<");
    out = ReplaceAllInstances(out, "&#60;", "<");

    out = ReplaceAllInstances(out, "&[Gg][Tt];", ">");
    out = ReplaceAllInstances(out, "&#62;", ">");

    //This one should be last of the rule-replacement rules.
    out = ReplaceAllInstances(out, "[%]25"   , "%");

    std::string out2;
    char ch;
    int i, ii;
    for(i=0; i<(int)(out.length()); i++){
        if(int(out[i])==37){
//            sscanf(out.substr(i+1,2).c_str(), "%x", &ii);
            sscanf(out.substr(i+1,2).c_str(), "%d", &ii);
            ch = static_cast<char>(ii);
            out2 += ch;
            i = i+2;
        }else{
            out2 += out[i];
        }
    }
    return out2;
}

//Decode GET/POST form URI-encoding into an explicit vector format. Useful for servers handling form data.
std::vector<std::vector<std::string>> Basic_Decode_Form_URL(const std::string &in){
    std::vector<std::vector<std::string>> out;
    std::vector<std::string> shuttle;    //shuttle drains into out.
    std::string cshuttle;                //cshuttle drains into shuttle.
    const std::string dec = Basic_Decode_URL(in);
    for(auto it = dec.begin(); it != dec.end(); ++it){
        if(*it == '+'){
            cshuttle.push_back(' ');
            continue;
        }
        if(*it == '&'){ //Statement separator.
            if(!cshuttle.empty()) shuttle.push_back(cshuttle);
            cshuttle.clear();

            if(!shuttle.empty()) out.push_back(shuttle);
            shuttle.clear();
            continue;
        }
        if(*it == '='){ //Statement operator.
            if(!cshuttle.empty()) shuttle.push_back(cshuttle);
            cshuttle.clear();
            continue;
        }
        if(*it == '?'){ //Form data opener. I don't think this can occur more than once..
            //Example:  ...somepage.html?a=b&c=d...
            cshuttle.clear();
            continue;
        }
        cshuttle += *it;
    }
    if(!cshuttle.empty()) shuttle.push_back(cshuttle);
    if(!shuttle.empty()) out.push_back(shuttle);
    return out;
}

//-------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------- Text Reflow Routines ----------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------

//This function takes a chunk of text, removes spurious newlines, and returns long strings delineated into paragraphs.
// It assumes that anything with two or more consecutive newlines is a paragraph boundary.
//NOTE: The vector contains one paragraph per element.
std::vector<std::string> Break_Text_Into_Paragraphs(const std::string &in){
    const auto broken = SplitStringToVector(in, "\n\n", 'd');
    std::vector<std::string> out;

    for(auto s_it = broken.begin(); s_it != broken.end(); ++s_it){
//        out.push_back( PurgeCharsFromString(*s_it, "\n") );   //Assumes that words might be split into parts on the line ending/beginning.
        out.push_back( ReplaceAllInstances(*s_it, "\n", " ") );  //Assumes that words are not split - each line ends with a complete word.
    }
    return out;
}

/*
//Breaks some reflowed text into a text blob (inserting "\n\n" between paragraphs and "\n" between lines).
std::string Break_Paragraphs_Into_Text(const std::vector<std::string> &in){
    std::string out;

    for(auto l_it = in.begin(); l_it != in.end(); ++l_it){
        if(l_it != in.begin()) out += "\n";
        out += *l_it;
    }

    return out;
}
*/

//Returns a broken vector which fits within the given max width.
//NOTE: Takes a single line. Knows nothing about paragraphs.
//NOTE: If a single word is longer than the line length, it will NOT be broken into two parts - it will be 
// longer than the maximum line length!
std::vector<std::string> Reflow_Line_to_Fit_Width_Left_Just(const std::string &in, long int W, long int indent){
    if(/*(indent < 0) ||*/ (W <= 0)) FUNCERR("Given impossible width/indentation values. Cannot proceed");
    if(YGORABS(indent) >= (W-1)) FUNCERR("Given ridiculous width/indentation values. Refusing to proceed");

    std::vector<std::string> out;

    std::stringstream inss(in);
    std::string shtl;
    std::string theindent, negindent;
    if(indent >= 0) theindent = std::string( indent, ' ');
    if(indent <  0) negindent = std::string(-indent, ' '); //Indents everything except the first line.

    bool firstrun = true;
    while( inss.good() ){
        std::string theword;
        inss >> theword;

        if(firstrun == true){
            theword = theindent + theword;
        }
       
        if(firstrun && shtl.empty()){
            shtl += theword;
 
        }else if(static_cast<long int>(shtl.size() + theword.size() + 1) > W){
            out.push_back(shtl);
            shtl.clear();
            shtl += negindent + theword;

//        }else if(firstrun && shtl.empty()){ 
//            shtl += theword;

        }else{
            shtl += " "_s + theword;
        }

        firstrun = false;
    }
    if(!shtl.empty()) out.push_back(shtl);
    return out;
}

//Returns a broken vector of possibly multiple paragraphs which fits within the given max width and is indented.
//NOTE: Takes a linear collection of paragraphs. Assumes paragraphs are separated by "\n\n".
//NOTE: If a single word is longer than the line length, it will NOT be broken into two parts - it will be 
// longer than the maximum line length!
std::vector<std::string> Reflow_Text_to_Fit_Width_Left_Just(const std::string &in, long int W, long int indent){
    std::vector<std::string> out;

    //Break each paragraph into its own single line.
    const auto broken_des = Break_Text_Into_Paragraphs(in);

    //Cycle over each paragraph, reflowing the text and inserting the indent.
    for(auto p_it = broken_des.begin(); p_it != broken_des.end(); ++p_it){
        const auto reflowed = Reflow_Line_to_Fit_Width_Left_Just(*p_it, W, indent);

        //Drop a break (empty horizontal line) between paragraphs if between two paragraphs.
        if(p_it != broken_des.begin()) out.push_back("");

        //Cycle over the lines and push them into the output vector.
        for(auto l_it = reflowed.begin(); l_it != reflowed.end(); ++l_it) out.push_back( *l_it );
    }


    return out;
}


//Returns a broken vector composed of (possibly multiple) paragraphs which are laid side-by-side. 
//NOTE: Takes (two) linear collections of paragraphs. Assumes paragraphs are separated by "\n\n".
//NOTE: If a single word is longer than the line length, it will NOT be broken into two parts - it will be 
// longer than the maximum line length!
//NOTE: The width of the separator is NOT accounted for. This is the user's job.
std::vector<std::string> Reflow_Adjacent_Texts_to_Fit_Width_Left_Just(const std::string &inL, long int WL, long int indentL,
                                                                      const std::string &sep,
                                                                      const std::string &inR, long int WR, long int indentR){
    std::vector<std::string> outL, outR, out;

    //Break each paragraph into its own single line.
    const auto broken_desL = Break_Text_Into_Paragraphs(inL);
    const auto broken_desR = Break_Text_Into_Paragraphs(inR);

    //Cycle over each paragraph, reflowing the text and inserting the indent.
    for(auto p_it = broken_desL.begin(); p_it != broken_desL.end(); ++p_it){
        const auto reflowed = Reflow_Line_to_Fit_Width_Left_Just(*p_it, WL, indentL);

        //Drop a break (empty horizontal line) between paragraphs if between two paragraphs.
        if(p_it != broken_desL.begin()) outL.push_back("");

        //Cycle over the lines and push them into the output vector.
        for(auto l_it = reflowed.begin(); l_it != reflowed.end(); ++l_it) outL.push_back( *l_it );
    }
    for(auto p_it = broken_desR.begin(); p_it != broken_desR.end(); ++p_it){
        const auto reflowed = Reflow_Line_to_Fit_Width_Left_Just(*p_it, WR, indentR);

        //Drop a break (empty horizontal line) between paragraphs if between two paragraphs.
        if(p_it != broken_desR.begin()) outR.push_back("");

        //Cycle over the lines and push them into the output vector.
        for(auto l_it = reflowed.begin(); l_it != reflowed.end(); ++l_it) outR.push_back( *l_it );
    }

    //Assemble the pieces. Ensure the number of rows match for each text block.
    const auto max_rows = (outL.size() > outR.size()) ? outL.size() : outR.size();
    while(outL.size() < max_rows) outL.push_back("");
    while(outR.size() < max_rows) outR.push_back("");

    for(auto it = outL.begin(); it != outL.end(); ++it){
        //Pad the left with spaces.
        while(static_cast<long int>(it->size()) < WL) *it += ' ';
    }
    for(auto itL = outL.begin(), itR = outR.begin(); (itL != outL.end()) && (itR != outR.end()); ++itL, ++itR){
        out.push_back( *itL + sep + *itR );
    }
    return out;
}


//Centers a given line to a specified width by padding the left side with ' 's.
std::string Reflow_Line_Align_Center(const std::string &in, long int W){
    //Check if the string is larger than the specified width. If so, we cannot possibly center it any better :/.
    if(static_cast<long int>(in.size()) > W) return in;

    long int space = (W - static_cast<long int>(in.size()))/2; //+-1 !  :)
    return std::string(space, ' ') + in;
}

