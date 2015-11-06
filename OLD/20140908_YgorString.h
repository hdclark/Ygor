//YgorString.h - Convenient functions for dealing with text in strings.
//
//

#ifndef YGOR_STRING_LIBRARY_H_
#define YGOR_STRING_LIBRARY_H_

#include <string>
#include <map>
#include <vector>
#include <utility>
#include <memory>
#include <set>
#include <algorithm>  //Needed for set_intersection(..);

//#include <regex>                    //Fuck you, silent compiler failing.
#include <boost/regex.hpp>


//To turn off user literals, define a macro like so (or pass in to compiler with -DYGOR_STRING_...
// NOTE: This is a compile-time macro (NOT a header-only thing!)
//#define YGOR_STRING_TURN_OFF_USER_LITERALS


#ifndef YGOR_STRING_TURN_OFF_USER_LITERALS
//-------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------ Handy User-Literals ----------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
std::string operator "" _s(const char* str, size_t len);
#endif

//-------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------- String Type Conversions --------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
//Given a string, copy the data into a unique buffer of type T. 
template <class T> std::unique_ptr<T[]> str_to_buf(bool *OK, const std::string &in, uint64_t *size);


//-------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------ Self-contained N-gram routines -----------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
namespace NGRAMS {
    const unsigned char CHARS  = 0x1;
    const unsigned char WORDS  = 0x2;
}
std::map<std::string,float> NGrams_With_Occurence(const std::string &thestring, long int numb_of_ngrams, long int length_of_ngrams, const unsigned char &type);
std::set<std::string> NGrams(const std::string &thestring, long int numb_of_ngrams, long int length_of_ngrams, const unsigned char &type);
std::set<std::string> NGram_Matches(const std::set<std::string> &A, const std::set<std::string> &B);
long int NGram_Match_Count(const std::set<std::string> &A, const std::set<std::string> &B);


//-------------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------- Substring and Subsequence routines ----------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
//Returns the longest common sequential substring. ie. 'ABCDEF' and 'ACDEF' gives 'CDEF'.
std::string LongestCommonSubstring(const std::string &A, const std::string &B);

//Returns the longest common subsequence. ie. 'ABCDEF' and 'ACDEF' gives 'ACDEF'.
std::string LongestCommonSubsequence(const std::string &A, const std::string &B);

//-------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------- Common text transformations ------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
namespace CANONICALIZE {
    const unsigned char TO_UPPER  =  1;  //Uppercase the characters, if possible.
    const unsigned char LOCALIZE  =  2;  //Transform the letters to the current locale.
    const unsigned char TRIM_ENDS =  4;  //Trim whitespace to the left of first, to the right of last char.
    const unsigned char TRIM      =  8;  //Trim the edges and shrink long whitespace to a single space.
    const unsigned char TRIM_ALL  = 16;  //Remove ALL whitespace.
    const unsigned char TO_AZ     = 32;  //Remove all non [ A-Za-z] characters.
    const unsigned char TO_NUM    = 64;  //Remove all non [ 0-9.-] characters.
    const unsigned char TO_NUMAZ  = 128; //Remove all non [ A-Za-z0-9.-] characters.
}
std::string & Canonicalize_String(std::string &, const unsigned char & );
std::string Canonicalize_String2(const std::string &in, const unsigned char &mask);  //<--- prefer this version

std::string Detox_String(const std::string &in); //Attempt to ~intelligently replace non-simple chars like spaces and commas.

//Filesystem-unaware routines. Do not send escaped file/pathnames to these functions. They work on '/'s.
std::string Get_Parent_Directory(const std::string &path); //Removes lowest filename or dir, returns parent dir. File, '.' oblivious!
std::string Get_Bare_Filename(const std::string &fullname); //Returns substr after last '/'. If no '/', returns input string.

//Compare sections of two strings for equality. After safely pulling out the sections, perform canonicalization on the
// segments before comparison. This is useful for ignoring case and whitespace, for example.
bool Safely_Compare_Strings(const std::string &A, unsigned int from_A, unsigned int to_A, const unsigned char mask_A, 
                            const std::string &B, unsigned int from_B, unsigned int to_B, const unsigned char mask_B);


//-------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------ Math expression transformations ----------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
std::string Remove_Unneeded_Surrounding_Parenthesis(const std::string &in, char L = '(', char R = ')');
std::string Wrap_Comma_Separated_Stuff_At_Same_Depth(const std::string &in, char L = '(', char R = ')');
bool Contains_Unmatched_Char_Pairs(const std::string &in, char L = '(', char R = ')');
//bool Contains_Unbalanced_Char_Pairs(...)  ---> detect '([a)]' errors.  TODO

std::string Remove_Preceeding_Chars(const std::string &in, const std::string &chars);
std::string Remove_Trailing_Chars(const std::string &in, const std::string &chars);

//-------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------- Common text conversions --------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
std::string inttostring(long int number);
std::string floattostring(float number);

template <class T>   std::string Xtostring(T numb);
template <class T>   std::string XtoPreciseString(T numb);
template <class T>   T           stringtoX(const std::string &text);
template <class T>   bool        Is_String_An_X(const std::string &text);

//This is useful for parsing a config file. Provide a key like "aval = " and if the line is "aval = 6" then you'll
// get a 6 back.
template <class T>
bool GetValueIfKeyMatches(T *out, const std::string &key , const unsigned char key_mask, 
                                  const std::string &line, const unsigned char line_mask);

std::string Convert_Unprintables_to_Hex(const std::string &in);
std::string Quote_Static_for_Bash(const std::string &in);
std::string Quote_Expandable_for_Bash(const std::string &in);

//-------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------- Generic String-related Routines -----------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
std::string Generate_Random_String_of_Length(long int len);

std::string Lineate_Vector(const std::vector<std::string> &in, const std::string &separator);

//-------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------- Basic parsing routines ---------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
//These were inherited from Project - Rugine for the purposes of using them in Project - DICOMAutomaton. They may need some
// reworking.
//
//    FIXME - Cleanup and move things to the appropriate location!
//
//    FIXME - Time for a total overhaul? There are quite a few inconsistencies, 
//            and I can now ditch boost regex.
//
//NOTE: 'BEHAVIOUR' can be 'd', 'l', or 'r'. See source for info about what these do.
std::vector<std::string> &BYOVectorSplit(const std::string &s, char delim, std::vector<std::string> &elems, char BEHAVIOUR); 
std::vector<std::string> SplitStringToVector(const std::string &s, char delim, char BEHAVIOUR);
std::vector<std::string> SplitVector(const std::vector<std::string> &s, char delim, char BEHAVIOUR);

std::vector<std::string> SplitStringToVector(const std::string &s, const std::string &delim, char BEHAVIOUR);

std::string ReplaceAllInstances(const std::string &in, const std::string &regex, const std::string &replacement);

std::string PurgeCharsFromString(const std::string &in, std::string purge_chars);

std::vector<std::string> GetUrls( std::vector<std::string> &in );
std::vector<std::string> GetUrls( const std::string &in );

std::string GetFirstNumber(std::string &in);

std::string GetFirstRegex(const std::string &source, boost::regex &regex_the_query);
std::string GetFirstRegex(const std::string &source, std::string query);
//std::string GetFirstRegex(std::string source, std::string query);
std::string GetFirstRegex(std::vector<std::string> &source, std::string query);

std::vector<std::string> GetAllRegex(std::vector<std::string> &source, boost::regex &regex_the_query);
std::vector<std::string> GetAllRegex(std::vector<std::string> &source, std::string query);

std::vector<std::string> GetAllRegex(std::string &source, std::string query);   //NOTE: DO NOT USE THIS ONE!
std::vector<std::string> GetAllRegex2(const std::string &source, std::string query);

std::string GetLineNBelow( std::vector<std::string> &source, std::string query, long int N);

std::vector<std::vector<std::string>> GetSubVectorFromTo( std::vector<std::string> &in, std::string from, std::string to);

//-------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------- URL-handling routines ---------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
std::string Basic_Encode_URL(const std::string &in);
std::string Basic_Decode_URL(const std::string &in);
std::vector<std::vector<std::string>> Basic_Decode_Form_URL(const std::string &in); //Used to decode POST/GET HTML form data.

//-------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------- Text Reflow Routines ----------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------
//Breaks a text blob on all "\n\n"s and removes all "\n". Used internally by the other reflow functions.
std::vector<std::string> Break_Text_Into_Paragraphs(const std::string &in); 

/*
//Breaks some reflowed text into a text blob (inserting "\n\n" between paragraphs and "\n" between lines).
std::string Break_Paragraphs_Into_Text(const std::vector<std::string> &in);
*/

//Returns a broken vector which fits within the given max width.
//NOTE: Takes a single line. Knows nothing about paragraphs.
std::vector<std::string> Reflow_Line_to_Fit_Width_Left_Just(const std::string &in, long int W = 80, long int indent = 0);

//Returns a broken vector of possibly multiple paragraphs which fits within the given max width and is indented.
//NOTE: Takes a linear collection of paragraphs. Assumes paragraphs are separated by "\n\n".
std::vector<std::string> Reflow_Text_to_Fit_Width_Left_Just(const std::string &in, long int W = 80, long int indent = 0);

//Returns a broken vector composed of (possibly multiple) paragraphs which are laid side-by-side. 
std::vector<std::string> Reflow_Adjacent_Texts_to_Fit_Width_Left_Just(const std::string &inL, long int WL, long int indentL, 
                                                                      const std::string &sep,
                                                                      const std::string &inR, long int WR, long int indentR);

//Centers a given line to a specified width by padding the left side with ' 's.
std::string Reflow_Line_Align_Center(const std::string &in, long int W = 80);

#endif
