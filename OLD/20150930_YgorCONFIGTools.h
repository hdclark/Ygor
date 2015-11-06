//YgorCONFIGTools.h - A collection of routines for parsing/reading configuration files.

#ifndef CONFIGTOOLS_HDR_GRD_H_
#define CONFIGTOOLS_HDR_GRD_H_

#include <string>
#include <vector>

//A simple config file is one where:
//  1) Each line is self-contained. There is no positional awareness (at least, no structural awareness.)
//  2) Each comment line begins with optional whitespace and non-optional '#'. Non-comment lines with a '#' is OK.
//  3) Each line is separated with some character (typically ':'.) Lines which omit this delimiter are comments.
//       (NOTE: If a structure like 'a = b' is required, simply filter out the lines with one element!)
//  4) Each line logically ends with a line-ending token. This is not changeable, and is always a newline '\n'.
//       (NOTE: This can be overcome be repeated serialization, until a single newline remains!)
//
// The output is a vector of vectors. Each outer vector element represents an individual line. Each inner vector
// element represents a tokenized line. Each string is an individual token. So the following file gets tokenized
// as follows.
//   _____________________________
//  |# some comment, is ignored.  | 
//  |                             | 
//  |# empty lines also ignored.  |       
//  |                             |                                                  { { "flagA", "optionA" },
//  |flagA = optionA              |                                                    { "flagB", "optionB" },
//  |flagB =   optionB            | --> Tokenize_Simple_Config_File(filename,"=") -->  { "flagC", "optionC #hi" }, 
//  |flagC=optionC #hi            |                                                    { "flagD", "1.23" } };
//  |flagD= 1.23                  | 
//  |_____________________________| 
//
// NOTE: Reading the file tokenizes on line endings, and we then further tokenize on the provided separator.
//       So, in reality, the text is 'double tokenized'.
std::vector<std::vector<std::string>> Tokenize_Simple_Config_File(std::string filename, std::string separator);
bool Write_Config_File(const std::vector<std::vector<std::string>> &inmem, std::string filename, std::string separator);


//This is an in-memory counterpart of Tokenize_Simple_Config_File(). Consider the following example string (which 
// may be read in from a file and had the newlines replaced with a '\n' newline character):
//
//     "flagA = optionA\nflagB =   optionB\nflagC=optionC #hi\nflagD= 1.23"
//                                                                                       { { "flagA", "optionA" },
//                                      --> Deserialize_Simple_Config_File(...,"=") -->    { "flagB", "optionB" },
//                                                                                         { "flagC", "optionC #hi" },
//                                                                                         { "flagD", "1.23" } };
//                                                                                       
// NOTE: This routine uses std::getline and assumes logical lines are separated by a '\n' character. If this is not
//       desirable, use the Double_Deserialize_Simple_Config_File() routine.
//
std::vector<std::vector<std::string>> Deserialize_Simple_Config_File(const std::string &serialized, std::string separator);
std::string Serialize_Simple_Config_File(const std::vector<std::vector<std::string>> &deserialized, std::string separator);


//These functions behave like the Deserialize_Simple_Config_File() routines, but do not require newlines to be used.
// Thus, the entire serialized config file data can be, say, passed in a single std::cin call. Example input could be:
//
//                   "Name = Spinach ||| Portion in g = 100 ||| Calories in kcal = 25 |||"
//
// where '|||' takes the place of the newline (aka "linetoken") and '=' is the separator.
//
// NOTE: This routine cannot handle newlines. Internally, all instances of linetokens are simply converted to 
//       '\n' characters before being sent through Deserialize_Simple_Config_File(). This *could* be changed if
//       needed. It shouldn't break any existing code, I think.
std::vector<std::vector<std::string>> Double_Deserialize_Simple_Config_File(const std::string &serialized, std::string separator, std::string linetoken);
std::string Double_Serialize_Simple_Config_File(const std::vector<std::vector<std::string>> &deserialized, std::string separator, std::string linetoken);



//Operations on in-memory configs. The burden is on the user to ensure the proper method is used for the config.
// For instance, some routines assume a simple 'a = b' format. Using these on a 'a = b = c' format, for instance,
// may produce unreliable output.
bool        YgorCONFIGTools_Is_X_Present_on_Leftmost(const std::vector<std::vector<std::string>> &conf, const std::string &X);
std::string YgorCONFIGTools_Given_X_Get_First_B_When_XB(const std::vector<std::vector<std::string>> &conf, const std::string &X);



std::vector<std::vector<std::string>> YgorCONFIGTools_Get_All_Lines_With_X_Present_on_Leftmost(const std::vector<std::vector<std::string>> &conf, const std::string &X);

#endif
