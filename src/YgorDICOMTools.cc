#include <stddef.h>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "YgorDICOMTools.h"
#include "YgorFilesDirs.h"    //Needed for Does_File_Exist_And_Can_Be_Read(..)
#include "YgorMisc.h"


//NOTE: This routine should NOT be used for actually parsing the file - this is designed to 
// be as quick as possible for simply checking if the file is DICOM or not. See the more appropriate
// validation routine for an alternative for parsing the file.
bool Is_File_A_DICOM_File(const std::string &filename_in){
    if( !Does_File_Exist_And_Can_Be_Read(filename_in) ) return false;

    //As far as I can tell, all valid DICOM files have a 128-byte '\0\0\0...' header followed
    // by 'DICM'. This is all we will check (at the moment?)

    //Open the file and prepare to read (a part of it) into memory.
    std::ifstream in(filename_in.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if(!in.is_open()) return false;

    const long int check_size = 128+4;            //The amount of bytes we want to read in.
    std::ifstream::pos_type l_size = in.tellg();  //Grab the size of the binary file.

    //If the file is smaller than the header, we know immediately it is not DICOM.
    if( check_size > static_cast<long int>(l_size) ) return false;

    std::unique_ptr<char[]> mem ( new char [check_size] );
//    mem.reset( new char [check_size] );
    in.seekg(0, std::ios::beg);                   //Seek back to the beginning.
    in.read((char *)(mem.get()), check_size);
    in.close();

    //Now check the contents of the data we've read in.
    long int i = 0;
    for( ; i < 128; i++) if(mem[i] != '\0') return false;
    if(mem[i++] != 'D') return false;
    if(mem[i++] != 'I') return false;
    if(mem[i++] != 'C') return false;
    if(mem[i]   != 'M') return false;

    //If we make it here, it is probably the case that 
    return true;
}


std::basic_string<unsigned char>  Simple_DICOM_Header( void ){
    std::basic_string<unsigned char> out;
    for(size_t i=0; i<128; ++i) out += static_cast<unsigned char>('\0');

    out += static_cast<unsigned char>('D');
    out += static_cast<unsigned char>('I');
    out += static_cast<unsigned char>('C');
    out += static_cast<unsigned char>('M');
    return out;
}


bool Is_Common_ASCII( const unsigned char &in ){     return isininc((unsigned char)(32),in,(unsigned char)(126)); }
bool operator==( const large &L, const large &R ){   return L.i == R.i;  }
bool operator==( const small &L, const small &R ){   return L.i == R.i;  }

std::ostream & operator<<( std::ostream &out, const large &in ){

    //Dump as a single four-byte integer.
    out << std::setfill(' ') << std::setw(10) << in.i;

    //Dump as 4 one-byte integers.
    out << " (";
    out << std::setfill('0') << std::setw(3) << (unsigned int)(in.c[0]) << ",";
    out << std::setfill('0') << std::setw(3) << (unsigned int)(in.c[1]) << ",";
    out << std::setfill('0') << std::setw(3) << (unsigned int)(in.c[2]) << ",";
    out << std::setfill('0') << std::setw(3) << (unsigned int)(in.c[3]) << ")";

    //Dump as 2 two-byte integers (in hex.)
    out << " (";
    out << std::setfill('0') << std::setw(4) << std::hex << in.s[0].i << ",";
    out << std::setfill('0') << std::setw(4) << std::hex << in.s[1].i << ")";

    out << std::dec;
    return out;
}

std::ostream & operator<<( std::ostream &out, const small &in ){

    //Dump as a single two-byte integer.
    out << std::setfill(' ') << std::setw(8) << in.i;

    //Dump as 2 one-byte integers.
    out  << " (";
    out << std::setfill('0') << std::setw(3) << (unsigned int)(in.c[0]) << ",";
    out << std::setfill('0') << std::setw(3) << (unsigned int)(in.c[1]) << ")";

    //Dump as a single two-byte integer (in hex.)
    out << " (";
    out << std::setfill('0') << std::setw(4) << std::hex << in.i << ")";

    out << std::dec;
    return out;
}

std::ostream & operator<<( std::ostream &out, const std::basic_string<unsigned char> &in ){
    for(unsigned char x : in) out << (char)x;
    return out;
}

std::ostream & operator<<( std::ostream &out, const piece &in ){
    //Print the simple parts.
    out << "A = " << in.A << ",  B(first) = " << in.B.s[0] << ",  B(secnd) = " << in.B.s[1];

    //Print the data.
    if(in.data.size() != 0){
        out << ",  data = \"";
        out << in.data;
        out << "\"";
    }
    return out;
}

//This is a sort of heuristic function - it attempts to 'guess' whether or not the given B denotes (only) the size of a pieces' data.
// Some elements require an extra 4 bytes (a pseudo 'C' parameter) to denote the size of the accompanying data.
bool Does_A_B_Not_Denote_A_Size( const large &A, const large &B ){

    //Special cases with precedent, found via trial and error.
    // -> return "false" to force the "B" to be equal to the size of the data.
    // -> return "true"  to check (elsewhere) whether the last two bytes of B are the size, or the next four bytes are the size.

    if( A.i == static_cast<uint32_t>(65538)       ) return true;  

    if( A.i == static_cast<uint32_t>(5255174)     ) return false;    //Original: Found in RS files - I think these exclusively denote stringified contour data ( "1.23/2.34/-5.56/1.24/2.45/..." )  Typically from the 'BODY' contour.
    if( A.i == static_cast<uint32_t>(3758161918)  ) return false;    //Original: Found in RS files - I think these are 'elements with a length of data' or something.

    if( A.i == static_cast<uint32_t>(1081312)     ) return false;    //Found in CT files - Used to denote the image data, I think.

    if( A.i == static_cast<uint32_t>(1191942)     ) return false;    //Found in RS files - GDCMdump gives "RT Referenced Study Sequence"
    if( A.i == static_cast<uint32_t>(4206598)     ) return false;    //Found in RS files - GDCMdump gives "Contour Sequence"


if( A.i == static_cast<uint32_t>(1060870)     ) return false;    //Found in RS files - GDCMdump gives "Referenced Frame of Reference Sequence"
if( A.i == static_cast<uint32_t>(3747846)     ) return false;    //Found in RS files - GDCMdump gives "ROI Contour Sequence"
if( A.i == static_cast<uint32_t>(1454086)     ) return false;    //Found in RS files - GDCMdump gives "Contour Image Sequence"

//Untested - likely matches.
if( A.i == static_cast<uint32_t>(1323014)     ) return false;    //Found in RS files - 3006,0014 





    //General scheme: Do the first two bytes of B look like a two-char identifier? (ie. UL, RS, DE, etc..)
    if( Is_Common_ASCII(B.c[0])  &&  Is_Common_ASCII(B.c[1])  ){
        return true;
    }
    return false;
}

//This is a sort of heuristic function - it attempts to 'guess' whether or not the given B contains a two-byte 'data length' integer.
// This is a typical thing for a few elements near the header, but is rarely (if ever?) found in the body of a DICOM file. 
bool Do_Last_Two_Bytes_of_B_Denote_A_Size( const large &A, const large &B ){
    //These items have been found to have the last two bytes of B denote the size of the data.
    //
    //This is a whitelist.
    if( 
        (A.i == static_cast<uint32_t>(2))            //Original: Determined using RTSTRUCT files.
        ||
        (A.i == static_cast<uint32_t>(131074))       //Original: Determined using RTSTRUCT files.
        ||
        (A.i == static_cast<uint32_t>(196610))       //Original: Determined using RTSTRUCT files.
        ||
        (A.i == static_cast<uint32_t>(1048578))      //Original: Determined using RTSTRUCT files.
        ||
        (A.i == static_cast<uint32_t>(1179650))      //Original: Determined using RTSTRUCT files.
      ) return true;
    return false;
}

//This is a sort of heuristic function - it attempts to 'guess' whether or not the given A and B will be followed by a four byte 
// 'data length' integer. This is a fairly rare case, it seems.
bool Do_Next_Four_Bytes_Denote_A_Size( const large &A, const large &B ){
    //These items have been found to have the next four bytes denote the size of the data.
    //
    //This is a whitelist.
    if(  
        (A.i == static_cast<uint32_t>(65538))        //Original: Determined using RTSTRUCT files.
//||
//(A.i == static_cast<uint32_t>(1081312))

      ) return true;
    return false;
}

//This is a sort of heuristic function - it attempts to 'guess' whether or not the given element contains data which should be 
// expanded into children. Such data either can or cannot: some data is data like "a string" and some contains nested elements.
bool Can_This_Elements_Data_Be_Delineated( const piece &in ){
    if(
          //These are required to be true for any element.   
          //
          //NOTE: All other logic blocks must && with this one. 
            (
//                (in.data.size() != 0)     //This stops from attempting to expand elements with less than 8 bytes (ie. not enough room for an "A" and a "B"!)
//            &&
                (in.data.size() >= 8)     //This stops from attempting to expand elements with less than 8 bytes (ie. not enough room for an "A" and a "B"!)
            &&
                (in.child.size() == 0)    //This means it has already been delineated - we do not want to (possibly over-) write data in this case.
            )
          //These are basic filters. We can not whitelist, but we can override elements within this block.
          // For example, when A = 3758161918, the first char of the data is NOT common ASCII, but this 
          // criteria works well for most other blocks. 
          &&
            (
                !Is_Common_ASCII(in.data[0])
            ||
                ( in.A.i == static_cast<uint32_t>(3758161918) )
            )
          //Exceptions.These items *appear* to have valid data, but do not.
          //
          //NOTE: This is a blacklist!
          &&
            (
                ( in.A.i != static_cast<uint32_t>(65538) )         //Original: Found in RTSTRUCT file.
            &&
                ( in.A.i != static_cast<uint32_t>(2) )             //Original: Found in RTSTRUCT file.
            &&
                ( in.A.i != static_cast<uint32_t>(1081312) )       //Found in CT file -> holds image data.
            )
      ) return true;
    return false;
}


//Given iterators to the loaded file, we attempt to locate the 'DICM' where we expect it.
// If it is found, an iterator to the memory immediately after the 'DICM' is returned, otherwise
// we simply return end.
unsigned char * Validate_DICOM_Format(const unsigned char *begin, const unsigned char *end){
    //If the memory is too small, we couldn't possibly find it.
    if( std::distance(begin, end) < 4 ) return (unsigned char *)(end);

    large shuttle;

    //We continually read until we have read 4 consecutive non-\0 characters. Check that they are 'DICM'. 
    unsigned char *space = (unsigned char *)(begin)+4, *dummy;
    large target;     target.c[0] = 'D';     target.c[1] = 'I';     target.c[2] = 'C';     target.c[3] = 'M';
    for( ; space != end; space++){
        dummy = space - 4;
        shuttle.c[0] = *(dummy++);
        shuttle.c[1] = *(dummy++);
        shuttle.c[2] = *(dummy++);
        shuttle.c[3] = *(dummy);

        if( (shuttle.c[0] != '\0') && (shuttle.c[1] != '\0') && (shuttle.c[2] != '\0') && (shuttle.c[3] != '\0') ){
            if(shuttle.i == target.i){
                //Found it!
                return space;
            }else{
                //Reaching this point may not be considered a failure - there may be 'noise' prior to reaching the 'DICM', but this
                // assumes that there is nothing in the file (other that \0's) prior to the 'DICM'.
                FUNCWARN("Unable to find 'DICM' in the memory region supplied");
                return (unsigned char *)(end);
            }
        }
    }
    return (unsigned char *)(end);
}


//Breaks a piece of memory into a sequence of 'pieces' (which are specific to the DICOM RTSTRUCT format.)
// The region of memory is not directly passed in, but rather is passed in via ~iterators to the beginning 
// and end of the memory block.
//
//NOTE: The data passed to this routine *needs* to be free of the 'DICM' magic file number.
std::vector<piece> Parse_Binary_File(const unsigned char *begin, const unsigned char *end){
    std::vector<piece> out;

    std::basic_string<unsigned char> buff; //Stores temporary data.
    large shuttle;

    //We do a read-interpret-read_data loop until we are at the end.
    auto i = begin;
    while(i < end){
        piece outgoing;

        //Clear our storage from last time.
        buff.clear();
 
        //Read in four bytes. This is the identifier for whatever we are about to read.
        shuttle.c[0] = *(i++);  shuttle.c[1] = *(i++); shuttle.c[2] = *(i++);  shuttle.c[3] = *(i++);
        outgoing.A = shuttle;

        //Read in the next four bytes. Do so two at a time. These vary in interpretation.
        shuttle.c[0] = *(i++);  shuttle.c[1] = *(i++); shuttle.c[2] = *(i++);  shuttle.c[3] = *(i++);
        outgoing.B = shuttle;

        //We now determine whether there is any data associated with this item or not. Some items *appear* to have data associated
        // with them, but in fact do not. Such an item is the 'UL' item (File Meta Information Group Length.)
        long int amount = 0; 

        //This criteria is a very brittle part of this program. It tries to guess which data needs to be delineated and
        // broken into children. It uses heuristics which are built by patching previously encountered warnings and failures!
        //
        //There are three OBSERVED possibilities here:
        // 1) The size is all 4 bytes of B (most common, espescially after the first few items near the header (compatability mode or something?) )
        // 2) The last two bytes of B denote the size. I've only seen this for a handful of elements, so I've used a whitelist function for it.
        // 3) The next 4 bytes (not yet read) must contain the size. This is the most rare. As far as I know, only one element does this

        if( Does_A_B_Not_Denote_A_Size( outgoing.A, outgoing.B ) ){
            //Now there are TWO OBSERVED possibilities:
            // 2) the last two bytes of B denote the size.
            if( Do_Last_Two_Bytes_of_B_Denote_A_Size( outgoing.A, outgoing.B ) ){
                //Grab the size from the last two bytes.
                amount = static_cast<long int>(outgoing.B.s[1].i);

            // 3) the next 4 bytes (not yet read) must contain the size.
            }else if( Do_Next_Four_Bytes_Denote_A_Size( outgoing.A, outgoing.B ) ){
                //Read ahead 4 more bytes to get the size.
                shuttle.c[0] = *(i++);  shuttle.c[1] = *(i++); shuttle.c[2] = *(i++);  shuttle.c[3] = *(i++);
                amount = static_cast<long int>(shuttle.i);

            // The OTHER possibility - we encounter an element which may need to be whitelisted as either of the THREE above possibilities!
            }else{
                FUNCWARN("Encountered an item which we have not previously encountered: A = " << outgoing.A << " and B = " << outgoing.B << ".");
                FUNCWARN("  Please determine how to read the element and add it to the appropriate whitelist function.");
                FUNCWARN("  Guessing how the item should be treated. Search the source for tag [ WWWW1 ] for more info.");

                //NOTE: The below code is a poor attempt at GUESSING how the item works. Expect errors. It is not necessarily (likely) how the item should be treated.
                //Attempt to "guess" the second probability, because it seems like a more common channel.. The appropriate whitelist in this case is Do_Last_Two_Bytes_of_B_Denote_A_Size(...)
                FUNCWARN("  If no additional warnings/errors are encountered, consider adding this item to the appropriate whitelist (and then test it!)");
                amount = static_cast<long int>(outgoing.B.s[1].i);
            }

        //Otherwise the entire 4 bytes denote the number of bytes to read in.
        // This is case 1).
        }else{
            amount = static_cast<long int>(shuttle.i);
        }

        //Perform a sanity check - is there enough data left to make this data sane?
        // If there is not, it is *not* safe to continue processing data. We *have* to return (with a warning.)
        const long int total_remaining_space = static_cast<long int>( std::distance(i,end) );
        if( total_remaining_space < amount ){
            FUNCWARN("We have interpreted an instruction to read memory of capacity beyond what we have loaded into memory during parsing. This _may_ or _may not_ be an error");
            FUNCWARN("  NOTE: The heuristic we use to 'guess' the proper size to load in can get snagged on elements with a large size. Try whitelisting the A value in the various whitelists");
            FUNCWARN("  NOTE: This element had A = " << outgoing.A << " and B = " << outgoing.B << ". We have attempted to read " << amount << " bytes when there was  " << total_remaining_space << " space remaining");
            return out;
        }

        outgoing.data_size = amount;

        //If there is data to be read in, do so.
        for(long int j=0; j<amount; ++j){
            buff.push_back( *(i++) );
        }

        outgoing.data = buff;

        out.push_back(outgoing);
    }

    return out;
}


//This routine walks over a chunk of de-lineated data and de-lineates all children data streams.
// It is called recursively until all elements have been delineated.
void Delineate_Children(std::vector<piece> &in){
    for(auto & i : in){
        //If there is data in the string buffer, and there is not already a child node, and the data *appears*
        // to be able to be expanded, then we attempt to do so.
        if( Can_This_Elements_Data_Be_Delineated( i ) ){
            i.child = Parse_Binary_File(  i.data.c_str(), i.data.c_str() + i.data.size() );

            //Now call this routine with the recently created data.
            Delineate_Children( i.child );

        //Otherwise, if there is already a child, then check it.
        }else if( (i.child.size() != 0) ){
            Delineate_Children( i.child );

        //Otherwise, this element has no data to process, has no attached nodes, and is fully expanded.
        //Simply move on to the next element.
        }else{
            continue;
        }
    }
    return;
}


//Dumps the entire vector, recursively dumping all children when they are encountered. Prefixes the data
// with spaces to denote the depth of the node.
void Dump_Children(std::ostream & out, const std::vector<piece> &in, const std::string space){ //NOTE: space defaults to ""
    for(const auto & i : in){
        //Print out the identity of this piece. If there is no child node, print the data.
        out << space;
        out << "A = " << i.A << " ";
        out << "B = " << i.B << " ";
        out << "S = " << std::setfill(' ') << std::setw(9) << i.data_size << " ";
        if(i.child.size() != 0){
            out << " [HAS_CHILD] " << std::endl;
            Dump_Children(out, i.child, (space + "    "));
        }else if(i.data.size() == 0){
            out << " [_NO_DATA_] " << std::endl;
        }else{
            out << " data = \"" << i.data  << "\"" << std::endl;
        }
    }
    return;
}

void Dump_Children(std::ostream & out, const std::vector<piece *> &in, const std::string space){ //NOTE: space defaults to ""
    for(auto i : in) Dump_Children(out, { *i } ); //Vector initializer to turn out of the element.
    return;
}


//Given a vector of nodes, we cycle through (recursively) and pick out the elements which match the key at a given depth.
// For instance, if we want to look for the (fictional) element   { 123, 456, 789 } then at depth 0 we look for all elements
// corresponding to 123, recurse and look for 456, and then recurse and look for 789. 
//
//NOTE: Only the final element(s) are returned. They are NOT embedded at the depth we find them.
//NOTE: Wildcards ("0") can be at any depth.
//NOTE: we do not make "in" const because we do not want const pointers to the data - we want it to be mutable!
void Get_Elements(std::vector<piece *> &out, std::vector<piece> &in, const std::vector<uint32_t> &key, const uint32_t depth ){ //NOTE: depth defaults to 0
    for(auto & i : in){

        //If we have found a match,
        if((i.A.i == key[depth]) || (key[depth] == 0)){

            //If this is the final element in the key, then we simply collect the piece and move on.
            if( (depth + 1) == key.size() ){
                out.push_back( &i );

            //Otherwise, we have to see if the current element has any children nodes. If it does, recurse, otherwise continue.
            }else if( i.child.size() != 0 ){
                Get_Elements(out, i.child, key, depth+1);
            }

        //Otherwise, just carry on.
        }else{
            //Do nothing...
        }
    }
    return;
}


//This function strips out data from nodes which have children and resets the "data_size" member to -1.
void Prep_Children_For_Recompute_Children_Data_Size( std::vector<piece> &in ){
    for(auto & i : in){
        //NOTE: Do NOT adjust the B parameter here - even if it contains the size info. This will be done when it is needed!

        //Reset the size of each node. This indicates that we need to recompute it.
        i.data_size = -1;

        //If there are children nodes, we clear the data and recurse.
        if( i.child.size() != 0 ){
            i.data.clear();
            Prep_Children_For_Recompute_Children_Data_Size( i.child );

        //If there are not children nodes, we (probably, might?) have data attached. We do nothing.
        }else{
            //Do nothing...
        }
    }
    return;
}


//This function is used for repacking purposes. It recurses through a vector to give the TOTAL size of all children (including the 
// A, B, and data parts of each!)
//
//NOTE: The unit of "size" is bytes.
//NOTE: This size includes the "A" and "B" parts of the children nodes.
//NOTE: The return value of this function will be the total length of the 
long int Recompute_Children_Data_Size( std::vector<piece> &in ){
    long int upward = 0;

    //Cycle through the vector.
    for(auto & i : in){

        //Check if this node has children. If it does, we recurse and determine the size of all child elements.
        if( i.child.size() != 0 ){
            i.data_size = Recompute_Children_Data_Size( i.child );

        //If it does not, we update the local "data_size" element and add it to "upward" along with the additional 8 bytes for this nodes "A" and "B".
        }else{
            i.data_size = (long int)(i.data.size());
        }

        //Check if B includes the size of the element. If it does not, then we have to take into account the extra 4 bytes required to append the
        // size immediately after B (when we eventually write the data...)
        if( Does_A_B_Not_Denote_A_Size( i.A, i.B ) ){

            //If the last two bytes of B denote the size, we have only 8 bytes of data (no extra size 4 bytes.)
            if( Do_Last_Two_Bytes_of_B_Denote_A_Size( i.A, i.B ) ){
                upward += i.data_size + (long int)(2*sizeof(uint32_t));

            //Otherwise, if we require an extra four bytes then we have to add this to the size of this elements memory footprint. 
            }else if( Do_Next_Four_Bytes_Denote_A_Size( i.A, i.B ) ){
                upward += i.data_size + (long int)(2*sizeof(uint32_t)) + (long int)(1*sizeof(uint32_t));

            //This is the case where we do not know enough about the tags to tell either way (safely.) Issue a warning and pick a method to try it.
            }else{
                FUNCWARN("Attempting to determine the size of the memory layout of an element A = " << i.A << " and B = " << i.B << " which is unfamiliar (not on a whitelist.)");
                FUNCWARN("  Please determine how to read the element and add it to the appropriate whitelist function.");
                FUNCWARN("  Guessing how the item should be treated. Search the source for tag [ WWWW2 ] for more info.");

                //GUESSING that the last two bytes of B denote the size! See ~10 lines above. The appropriate whitelist is Do_Last_Two_Bytes_of_B_Denote_A_Size(...).
                FUNCWARN("  Guessing a default layout. If this works, please add it to the appropriate whitelist (and test it!)");
                upward += i.data_size + (long int)(2*sizeof(uint32_t));
            }

        //Otherwise, B denotes the size. We have a simple layout.
        }else{
            upward += i.data_size + (long int)(2*sizeof(uint32_t));
        }

    }
    return upward;

}


//This function will take a vector of nodes and children and will repack each node. 
//
//NOTE: In order to fully pack an entire vector, just create a phony node and attach the vector to be its' child.
//
void Repack_Nodes( const std::vector<piece> &in, std::basic_string<unsigned char> &out ){

    //Cycle through the vector.
    for(const auto & i : in){

        //Dump the "A" part.
        out.push_back( i.A.c[0] );
        out.push_back( i.A.c[1] );
        out.push_back( i.A.c[2] );
        out.push_back( i.A.c[3] );


        //Check if B includes the size of the element. If it does not, then we have to take into account the extra 4 bytes required to append the size.
        if( Does_A_B_Not_Denote_A_Size( i.A, i.B ) ){

            //If the last two bytes of B denote the size, then we need to copy part of B and write the size. This is the second-most common scenario, and
            // appears to only happen near the header.
            if( Do_Last_Two_Bytes_of_B_Denote_A_Size( i.A, i.B ) ){
                out.push_back( i.B.c[0] );
                out.push_back( i.B.c[1] );

                small temp;
                temp.i = static_cast<uint16_t>( i.data_size );
                out.push_back( temp.c[0] );
                out.push_back( temp.c[1] );


            //Otherwise, if we require an extra four bytes then we simply copy over the B and then write the size.
            // This appears to be very rare, and only happens near the header.
            }else if( Do_Next_Four_Bytes_Denote_A_Size( i.A, i.B ) ){
                out.push_back( i.B.c[0] );
                out.push_back( i.B.c[1] );
                out.push_back( i.B.c[2] );
                out.push_back( i.B.c[3] );

                large temp;
                temp.i = static_cast<uint32_t>(i.data_size);
                out.push_back( temp.c[0] );
                out.push_back( temp.c[1] );
                out.push_back( temp.c[2] );
                out.push_back( temp.c[3] );

            //This is the case where we do not know enough about the tags to tell either way (safely.) Issue a warning and pick a method to try it.
            }else{
                FUNCWARN("Attempting to flatten an element A = " << i.A << " and B = " << i.B << " which is unfamiliar (not on a whitelist.)");
                FUNCWARN("  Please determine how to read the element and add it to the appropriate whitelist function.");
                FUNCWARN("  Guessing how the item should be treated. Search the source for tag [ WWWW3 ] for more info.");

                //GUESSING that the last two bytes of B denote the size! See ~10 lines above. The appropriate whitelist is Do_Last_Two_Bytes_of_B_Denote_A_Size(...).
                FUNCWARN("  Guessing a default behaviour. If this works, please add it to the appropriate whitelist.");

                out.push_back( i.B.c[0] );
                out.push_back( i.B.c[1] );

                small temp;
                temp.i = static_cast<uint16_t>( i.data_size );
                out.push_back( temp.c[0] );
                out.push_back( temp.c[1] );

            }

        //Otherwise, B denotes (only) the size. This is the most common scenario (in the body of the file.)
        }else{
            large temp;
            temp.i = static_cast<uint32_t>(i.data_size);
            out.push_back( temp.c[0] );
            out.push_back( temp.c[1] );
            out.push_back( temp.c[2] );
            out.push_back( temp.c[3] );
        }


        //Now check if there are any children nodes and dump them.
        if( i.child.size() != 0 ){
            Repack_Nodes( i.child, out );
 
        //Otherwise, dump the payload string. NOTE: Some data elements are empty - this is OK.
        }else{
            out += i.data;
        }
    }
    return;
}

