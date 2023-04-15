//YgorDICOMTools.h - A collection of routines for parsing/reading/writing/modifying/inspecting DICOM-format files.
//
//NOTE: These still need to be rigorously tested. Do NOT use them for any critical purposes prior to verification!


#ifndef YGOR_DICOMTOOLS_H_
#define YGOR_DICOMTOOLS_H_

#include <cstdint>
#include <iosfwd>
#include <string>

#include "YgorDefinitions.h"

bool Is_File_A_DICOM_File(const std::string &in);




std::basic_string<unsigned char>  Simple_DICOM_Header( void );

union small {
    uint16_t i;
    unsigned char c[2];
};

union large {
    uint32_t i;
    unsigned char c[4];
    small s[2];
};

//class piece;
class piece {
    public:
        large A;
        large B;

        int64_t data_size;
        std::basic_string<unsigned char> data;

        std::vector<piece> child;   //This is the data, delinearized into sequential items.
};


bool Is_Common_ASCII( const unsigned char &in );
bool operator==( const large &L, const large &R );
bool operator==( const small &L, const small &R );

std::ostream & operator<<( std::ostream &out, const large &in );
std::ostream & operator<<( std::ostream &out, const small &in );

class piece;

std::ostream & operator<<( std::ostream &out, const std::basic_string<unsigned char> &in );
std::ostream & operator<<( std::ostream &out, const piece &in );

bool Does_A_B_Not_Denote_A_Size( const large &A, const large &B );
bool Do_Last_Two_Bytes_of_B_Denote_A_Size( const large &A, const large &B );
bool Do_Next_Four_Bytes_Denote_A_Size( const large &A, const large &B );
bool Can_This_Elements_Data_Be_Delineated( const piece &in );

unsigned char * Validate_DICOM_Format(const unsigned char *begin, const unsigned char *end);
std::vector<piece> Parse_Binary_File(const unsigned char *begin, const unsigned char *end);
void Delineate_Children(std::vector<piece> &in);

void Dump_Children(std::ostream & out, const std::vector<piece> &in, const std::string space = ""); //NOTE: space defaults to ""
void Dump_Children(std::ostream & out, const std::vector<piece *> &in, const std::string space = ""); //NOTE: space defaults to ""
void Get_Elements(std::vector<piece *> &out, std::vector<piece> &in, const std::vector<uint32_t> &key, const uint32_t depth = 0); //NOTE: depth defaults to 0
void Prep_Children_For_Recompute_Children_Data_Size( std::vector<piece> &in );

int64_t Recompute_Children_Data_Size( std::vector<piece> &in );
void Repack_Nodes( const std::vector<piece> &in, std::basic_string<unsigned char> &out );

#endif
