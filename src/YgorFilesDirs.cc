//YgorFilesDirs.cc - Routines for interacting with files and directories. 
//

#include <dirent.h>    //Needed for working with directories in C/UNIX.
#include <sys/stat.h>  //Needed for mknod (used for FIFOs, etc..), stat(...)
#include <unistd.h>    //Needed for access(...)
#include <algorithm>
#include <cstdint>     //For intmax_t
#include <cstdio>      //Needed for remove(...)
#include <cstdlib>
#include <ctime>       //Needed for gen_time_random(), time_t.
#include <fstream>     //Needed for fstream (for file checking.)
#include <iomanip>
#include <iterator>
#include <list>
#include <memory>      //Needed for unique_ptrs.
#include <string>

#include "External/MD5/md5.h" //Needed for MD5_of_File(...)

#include "YgorDefinitions.h"
#include "YgorFilesDirs.h"
#include "YgorMisc.h"    //Needed for FUNCINFO,FUNCWARN,FUNCERR macros.
#include "YgorString.h"  //Needed for Xtostring routine.

//#ifndef YGORFILESDIRS_DISABLE_ALL_SPECIALIZATIONS
//    #define YGORFILESDIRS_DISABLE_ALL_SPECIALIZATIONS
//#endif

//--------------------------------------------------------------------------------------------------------
//---------------------------------------------- Files Only ----------------------------------------------
//--------------------------------------------------------------------------------------------------------
bool Does_File_Exist_And_Can_Be_Read(const std::string &filename){
    //Alternative: if(access(filename.c_str(), F_OK) == -1) return false;
    //             else return true;     (but this depends on unistd.h)

    //First check if it a directory (because below will return 'true' if the filename is a directory!)
    if(Does_Dir_Exist_And_Can_Be_Read(filename)){
        return false;
    }
    std::fstream FI(filename.c_str(), std::ifstream::in);
    FI.close();
    if( FI.fail() ){
        return false;
    }
    return true;
}

//Returns the first filename which exists and is available to be read, or an empty string.
std::string Get_First_Filename_Which_Exists_And_Can_Be_Read(const std::list<std::string> &filenames){
    for(const auto & filename : filenames){
        if(Does_File_Exist_And_Can_Be_Read(filename)) return filename;
    }
    return std::string("");
}

//Returns the size of the file in bytes without having to open/seek/close it. 
//off_t Size_of_File(const std::string &filename){
intmax_t Size_of_File(const std::string &filename){
    //Note: This may fail on files > 2GB even if the machine is >32 bit due to stat() using off_t.
    // You can check this by ensuring the off_t typedef is not stuck at 32 bit, though I suspect
    // it is unlikely this would happen...
    //
    //Note: Try replacing this with stat64() so that we get off64_t instead!
    struct stat buf;
    if( ::stat(filename.c_str(), &buf) != 0 ) return 0;
    const auto size = static_cast<intmax_t>(buf.st_size);
    return size;
}

//Returns last time of access, or (std::time_t)(-1) if an error occurs. 
// Should work for both files and directories.
//
//For directories, access/modification of files under directory *should* 
// cascade up to alter the access/modification time of the parent directory. 
// This only goes ONE level up. The absence of this alteration does not imply 
// nothing changed (just that it wasn't recorded).
//
//NOTE: From stat(2) man page:
//   Not all of the Linux file systems implement all of the time fields.  Some file system types allow mounting in such a way
//   that file and/or directory accesses do not cause an update of the st_atime field.  (See noatime, nodiratime,  and  relatime
//   in  mount(8),  and  related  information  in  mount(2).)  In addition, st_atime is not updated if a file is opened with the
//   O_NOATIME; see open(2).
//
//   The field st_atime is changed by file accesses, for example, by execve(2), mknod(2), pipe(2), utime(2) and read(2) (of more
//   than zero bytes).  Other routines, like mmap(2), may or may not update st_atime.
//
//   The  field st_mtime is changed by file modifications, for example, by mknod(2), truncate(2), utime(2) and write(2) (of more
//   than zero bytes).  Moreover, st_mtime of a directory is changed by the creation or deletion of  files  in  that  directory.
//   The st_mtime field is not changed for changes in owner, group, hard link count, or mode.
std::time_t Last_Access_Time(const std::string &pathorfilename){
    struct stat buf;
    if( ::stat(pathorfilename.c_str(), &buf) != 0 ) return (std::time_t)(-1);
    return buf.st_atime;
}

//Returns last time of modification, or (std::time_t)(-1) if an error occurs.
// Should work for both files and directories. See comment about time issues
// above Last_Access_Time().
//
//For directories, access/modification of files under directory *should* 
// cascade up to alter the access/modification time of the parent directory. 
// This only goes ONE level up. The absence of this alteration does not imply 
// nothing changed (just that it wasn't recorded).
std::time_t Last_Modification_Time(const std::string &pathorfilename){
    struct stat buf;
    if( ::stat(pathorfilename.c_str(), &buf) != 0 ) return (std::time_t)(-1);
    return buf.st_mtime;
}


//Returns a string containing an md5, or an empty string on failure.
std::string MD5_of_File(const std::string &filename){
    //NOTE: This could be made more sane by computing small chunks of the file,
    // instead of doing it all at once. See RFC 1321 (MD5) for a reference
    // implementation which includes a routine for doing this. I think it 
    // could be re-implemented quite easily here.
    MD5::Context working;
    MD5::Init(&working);

    intmax_t thesize(0);
    auto inmem = Load_Binary_File<unsigned char>(filename, &thesize);
    if(thesize == 0) return std::string(); //Error.

    MD5::Update(&working, reinterpret_cast<void *>(inmem.get()), thesize);

    unsigned char *result = new unsigned char[16+1];
    result[16] = '\0';

    MD5::Final(result, &working);

    //Print is as 2-digit hex.
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for(int i = 0; i < 16; ++i) ss << std::setw(2) << static_cast<unsigned>(result[i]);

    delete[] result;
    return ss.str();
}

//Gets from byte N to byte N+L (inclusive) from a file. Should only use for large files, because it is opened/closed each time.
//template <class T> std::unique_ptr<T[]> Get_Piece_of_Binary_File(const std::string &filename_in, std::ios::pos_type N, std::ios::pos_type L){
template <class T> std::unique_ptr<T[]> Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L){
    std::ifstream in(filename_in.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if(!in.is_open()) return nullptr;

    intmax_t l_size = static_cast<intmax_t>(in.tellg());  //Grab the size of the binary file in bytes.
    if(l_size < (N+L)) return nullptr;

    auto mem = std::make_unique<T[]>(L/sizeof(T));

    in.seekg(0,std::ios::beg);
    in.seekg(N);
    in.read(reinterpret_cast<char*>(mem.get()), static_cast<std::streamsize>(L));
    return mem; 
}
#ifndef YGORFILESDIRS_DISABLE_ALL_SPECIALIZATIONS
/*
template std::unique_ptr<unsigned char[]> Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
template std::unique_ptr<char[]>          Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
template std::unique_ptr<float[]>         Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
template std::unique_ptr<double[]>        Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
*/

template std::unique_ptr<uint8_t[]>       Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
template std::unique_ptr<uint16_t[]>      Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
template std::unique_ptr<uint32_t[]>      Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
template std::unique_ptr<uint64_t[]>      Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
template std::unique_ptr<int8_t[]>        Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
template std::unique_ptr<int16_t[]>       Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
template std::unique_ptr<int32_t[]>       Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
template std::unique_ptr<int64_t[]>       Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
template std::unique_ptr<char[]>          Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
template std::unique_ptr<float[]>         Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
template std::unique_ptr<double[]>        Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L);
#endif


bool Does_Fifo_Exist(const std::string &filename){
    //NOTE: I *think* this is essentially identical to Does_File_Exist_And_Can_Be_Read(...), but it might
    // be safer to separate these functions until I know for certainty (specifically, will ..._File_... block?)
    if(access(filename.c_str(), F_OK) == -1) return false;
    return true;
}

std::string Get_Unique_Filename(const std::string &prefix, const long int len, const std::string &suffix /* = "" */){
    std::string out; //, temp;
    do{
        const std::string random_chars = Generate_Random_String_of_Length(len);
//        gen_random(&temp, len);
        out = prefix + random_chars + suffix;
    }while( Does_File_Exist_And_Can_Be_Read(out) );
    return out;
}

//std::string Get_Unique_Sequential_Filename(const std::string &prefix){
std::string Get_Unique_Sequential_Filename(const std::string &prefix, const long int n_of_digit_pads /* = */, const std::string &suffix /* = "" */){
    std::string out, temp;
    //This function appends numbers to the back of a filename until a unique one is found (or we exceed max.)
    // The first output in a sequence will NOT have a number at all. In the future, the appended characters may
    // be alphanumeric.
    
    //If we don't pad we can have a numberless case. This is handled here.
    if( !Does_File_Exist_And_Can_Be_Read(prefix + std::string(n_of_digit_pads,'0') + suffix) ){
        return (prefix + std::string(n_of_digit_pads,'0') + suffix);
    }

    //General case.
    for(long int i = 0; i < 1E7; i++){
        std::string thenumb = Xtostring<long int>(i);
        if(static_cast<long int>(thenumb.size()) < n_of_digit_pads){
            thenumb = std::string(n_of_digit_pads - static_cast<long int>(thenumb.size()), '0') + thenumb;
        }

        out = prefix + thenumb + suffix;
        if(!Does_File_Exist_And_Can_Be_Read(out)) return out;
    }
    FUNCERR("Unable to find a sequential filename");
    return out;
}

//Expand a filename/partial path/full path/symbolic link/./.. into a full path. Empty string is returned on fail.
std::string Fully_Expand_Filename(const std::string &filename){
    std::string out;
    char *expanded = reinterpret_cast<char *>(realpath(filename.c_str(), nullptr));
    if(expanded == nullptr) return out;
    out.insert(out.size(), expanded); 
    free(reinterpret_cast<void *>(expanded));
    return out;
}

//Determines if a path is below another path. Useful for specifying whitelists of files/directories.
//
//Note: Returns 'true' on success, 'false' on everything else (including all errors).
bool File_Is_Recursively_Within_Directory(const std::string &filename_in, const std::string &pathname_in){
    const auto A = Fully_Expand_Filename(filename_in);
    const auto B = Fully_Expand_Filename(pathname_in);
    if(A.empty() || B.empty()) return false;  //An error in the input or resolving the file/pathname.

    if(A.size() < B.size()) return false; 
    //Primitively examine the directory filename. Compare letter for letter with the filename until the directory name 
    // terminates.
    for(size_t i = 0; i != B.size(); ++i){
        if(B[i] != A[i]) return false;
    }
    return true; //We have to believe the file is recursively contained within the directory now.
}

//Loads a binary file into memory, closes the file, and passes the data back.
//
//NOTE: The 'size' parameter is filled with the size of data in the array. The units are in T's. In other words,
// the number of *bytes* of data is size*sizeof(T).
template <class T> std::unique_ptr<T[]> Load_Binary_File(const std::string &filename_in, intmax_t *size){
    if(size == nullptr) return nullptr;
    (*size) = 0;  //Just in case we run into an error and have to exit prematurely..

    //Load the file into memory.
    std::ifstream in(filename_in.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    if(!in.is_open()){
         FUNCWARN("Unable to load file. Does it exist? Do you have permissions to open it?");
         return nullptr;
    }

    //std::ifstream::pos_type l_size = in.tellg();  //Grab the size of the binary file in bytes.
    intmax_t l_size = static_cast<intmax_t>(in.tellg());  //Grab the size of the binary file in bytes.
    //const auto l_size = in.tellg();  //Grab the size of the binary file in bytes.

    //Check if the file is OK!

    // ... 
   

    //Check if the data (appears) to be able to be represented by an array of T's. If not, we have to 
    // abort or risk filling the buffer incorrectly.
    {
      const auto A = l_size/sizeof(T);  //Safer to test this way than with modulus!
      if( static_cast<intmax_t>(A * sizeof(T)) != static_cast<intmax_t>(l_size) ){
          FUNCWARN("This data does not appear to be able to be represented as the requested data type.");
          FUNCWARN(" The file is of size " << l_size << " bytes, which is not divisible by the size of the template type (size = " << sizeof(T) << " bytes.)");
          return nullptr;
      }
    }

    auto mem = std::make_unique<T[]>( l_size/sizeof(T) ); //Allocate space for the entire block to be pulled into memory.
    in.seekg(0, std::ios::beg);  //Seek back to the beginning.
    in.read((char *)(mem.get()), static_cast<std::streamsize>(l_size));  //Read the entire file in (in one go.)
    in.close();

    (*size) = static_cast<intmax_t>(l_size/sizeof(T));
    return mem;
}
#ifndef YGORFILESDIRS_DISABLE_ALL_SPECIALIZATIONS
/*
template std::unique_ptr<unsigned char[]> Load_Binary_File(const std::string &filename_in, intmax_t *size);
template std::unique_ptr<char[]>          Load_Binary_File(const std::string &filename_in, intmax_t *size);
template std::unique_ptr<float[]>         Load_Binary_File(const std::string &filename_in, intmax_t *size);
template std::unique_ptr<double[]>        Load_Binary_File(const std::string &filename_in, intmax_t *size);
*/
template std::unique_ptr<uint8_t[]>  Load_Binary_File(const std::string &filename_in, intmax_t *size);
template std::unique_ptr<uint16_t[]> Load_Binary_File(const std::string &filename_in, intmax_t *size);
template std::unique_ptr<uint32_t[]> Load_Binary_File(const std::string &filename_in, intmax_t *size);
template std::unique_ptr<uint64_t[]> Load_Binary_File(const std::string &filename_in, intmax_t *size);
template std::unique_ptr<int8_t[]>   Load_Binary_File(const std::string &filename_in, intmax_t *size);
template std::unique_ptr<int16_t[]>  Load_Binary_File(const std::string &filename_in, intmax_t *size);
template std::unique_ptr<int32_t[]>  Load_Binary_File(const std::string &filename_in, intmax_t *size);
template std::unique_ptr<int64_t[]>  Load_Binary_File(const std::string &filename_in, intmax_t *size);
template std::unique_ptr<char[]>     Load_Binary_File(const std::string &filename_in, intmax_t *size);
template std::unique_ptr<float[]>    Load_Binary_File(const std::string &filename_in, intmax_t *size);
template std::unique_ptr<double[]>   Load_Binary_File(const std::string &filename_in, intmax_t *size);
#endif

//Write a block of memory to file. "size" is the NUMBER OF T to read. If sizeof(T) != 8, then this is != bytes!
//Returns true on success.
template <class T> bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<T[]> in, intmax_t size){
    if(size <= 0) return false;
    std::ofstream FO(filename_out.c_str(), std::ios::out | std::ios::binary);
    FO.write((char *)(in.get()), static_cast<std::streamsize>(size*sizeof(T)));
    //Check for errors, or will it except?
    FO.close();
    return true;
}
#ifndef YGORFILESDIRS_DISABLE_ALL_SPECIALIZATIONS
/*
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<unsigned char[]> in, intmax_t size);
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<char[]> in, intmax_t size);
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<float[]> in, intmax_t size);
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<double[]> in, intmax_t size);
*/
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<uint8_t[]> in, intmax_t size);
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<uint16_t[]> in, intmax_t size);
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<uint32_t[]> in, intmax_t size);
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<uint64_t[]> in, intmax_t size);
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<int8_t[]> in, intmax_t size);
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<int16_t[]> in, intmax_t size);
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<int32_t[]> in, intmax_t size);
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<int64_t[]> in, intmax_t size);
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<char[]> in, intmax_t size);
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<float[]> in, intmax_t size);
template bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<double[]> in, intmax_t size);
#endif

//Append a block of memory to file. "size" is the NUMBER OF T to read. If sizeof(T) != 8, then this is != bytes!
//Returns true on success.
template <class T> bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<T[]> in, intmax_t size){
    if(size <= 0) return false;
    std::ofstream FO(filename_out.c_str(), std::ios::out | std::ios::binary | std::ios::app);
    FO.write((char *)(in.get()), static_cast<std::streamsize>(size*sizeof(T)));
    //Check for errors, or will it except?
    FO.close();
    return true;
}
#ifndef YGORFILESDIRS_DISABLE_ALL_SPECIALIZATIONS
/*
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<unsigned char[]> in, intmax_t size);
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<char[]> in, intmax_t size);
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<float[]> in, intmax_t size);
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<double[]> in, intmax_t size);
*/
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<uint8_t[]> in, intmax_t size);
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<uint16_t[]> in, intmax_t size);
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<uint32_t[]> in, intmax_t size);
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<uint64_t[]> in, intmax_t size);
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<int8_t[]> in, intmax_t size);
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<int16_t[]> in, intmax_t size);
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<int32_t[]> in, intmax_t size);
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<int64_t[]> in, intmax_t size);
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<char[]> in, intmax_t size);
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<float[]> in, intmax_t size);
template bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<double[]> in, intmax_t size);
#endif


//Load a file, line by line, into a string delimited by '\n'. Will not work with binary files.
std::string LoadFileToString(const std::string &filename_in){
    std::string lines;
    std::fstream thefile(filename_in.c_str(), std::ios::in);

    //If the file is not good, we simply return an empty string.
    if(!thefile.good()){
        return lines;
    }

    std::string filecontents;
    while(!thefile.eof()){
        std::getline(thefile, lines);
        if(!thefile.eof()) filecontents += lines + "\n";
    }

    thefile.close();
    return filecontents;
}

//Load a binary file and copy the memory into a std::string. Involves a copy, so use only for small things.
std::string LoadBinaryFileToString(const std::string &filename_in){
    std::string thedata;
    const auto filesize = Size_of_File(filename_in);
    const auto mem_ptr = Get_Piece_of_Binary_File<char>(filename_in, 0, filesize); //likely a unique_ptr.
    thedata.clear();
    thedata.insert(0,mem_ptr.get(),static_cast<std::string::size_type>(filesize));
//    thedata.insert(thedata.size(), mem_ptr.get(), filesize); //Probably overkill. Inserts it at the end. Should always be at the beginning in this case.
    return thedata;
}

//Load a file into a list with no delimiter.
std::list<std::string> LoadFileToList(const std::string &filename_in){
    std::string lines;
    std::list<std::string> filecontents;
    std::fstream thefile(filename_in.c_str(), std::ios::in);

    //If the file is not good, we simply return an empty string.
    if(!thefile.good()){
        return filecontents;
    }

    while(!thefile.eof()){
        std::getline(thefile, lines);
        if(!thefile.eof()) filecontents.push_back( lines );
    }

    thefile.close();
    return filecontents;
}

//Write a string to a file. No extra formatting is performed. Returns true on success.
bool WriteStringToFile(const std::string &in, const std::string &filename_in, bool overwrite){
    if(!overwrite && Does_File_Exist_And_Can_Be_Read(filename_in)) return false;

    std::fstream thefile(filename_in.c_str(), std::ios::out);
    if(!thefile.good()){
        return false;
    }
    thefile << in;
    thefile.close();
    return true;
}
bool OverwriteStringToFile(const std::string &in, const std::string &filename_in){
    const bool overwrite = true;
    return WriteStringToFile(in,filename_in,overwrite);
}


//Write a binary string to a file. Returns true on success.
//
//NOTE: This will NOT silently overwrite existing files unless they are generated quickly...
bool WriteBinaryStringToFile(const std::string &in, const std::string &filename_in, bool overwrite){
    if(!overwrite && Does_File_Exist_And_Can_Be_Read(filename_in)) return false;

    std::fstream thefile(filename_in.c_str(), std::ios::out | std::ios::binary);
    if(!thefile.good()){
        return false;
    }
    thefile << in;
    thefile.close();
    return true;
}

//Append a string to a file, creating the file if necessary. Returns true on success.
bool AppendStringToFile(const std::string &in, const std::string &filename_in){
    std::fstream thefile(filename_in.c_str(), std::ios::out | std::ios::app);
    if(!thefile.good()){
        return false;
    }
    thefile << in;
    thefile.close();
    return true;
}

//Creates a FIFO at the specified path. User must then read/write to use it.
//
//NOTE: This function assumes the file does not exist. It will not be successful if it does.
//
//NOTE: This function returns 'true' upon success.
bool CreateFIFOFile(const std::string &filename_in){
    const int result = mknod( filename_in.c_str(), S_IRUSR | S_IWUSR | S_IFIFO, 0);
    if(result < 0){
        return false;
    }
    return true;
}

//Remove a file using the cstdio remove() function. Requires a specific filename.
//
//NOTE: Returns true on success, false on failure.
//
//NOTE: Will not properly handle filenames with NULLs (is that possible?).
bool RemoveFile(const std::string &filename){
    //Explicitly disallow deletion of directories (at least those we have access to).
    if(Does_Dir_Exist_And_Can_Be_Read(filename)){
        return false;
    }

    //This function returns 0 iff the file was removed.
    return (::remove(filename.c_str()) == 0);
}

//Touch a file: write 0 bytes to it, creating if necessary.
bool TouchFile(const std::string &filename){
    const bool overwrite = true;
    return WriteBinaryStringToFile("", filename, overwrite);
}

//Copy a file from source to destination.
bool CopyFile(const std::string &source, const std::string &destination){
    //Copy the file.
    {
      std::ifstream A(source, std::ios::binary);
      if(A.fail()){
          FUNCWARN("Failed to open source file '" << source << "'");
          return false;
      }

      std::ofstream B(destination, std::ios::binary);
      if(B.fail()){
          FUNCWARN("Failed to create/open destination file '" << destination << "'");
          return false;
      }

      B << A.rdbuf();
      if(!B.good()){
          FUNCWARN("After copy, destination file is not std::fstream 'good'");
          return false;
      }
      //if(!A.eof()){
      //    FUNCWARN("After copy, source file appears to not be at EOF");
      //    return false;
      //}
      //return true;
    }

    //Verify it worked.
    return (Size_of_File(source) == Size_of_File(destination));
}

//--------------------------------------------------------------------------------------------------------
//---------------------------------------------- Directories ---------------------------------------------
//--------------------------------------------------------------------------------------------------------


bool Does_Dir_Exist_And_Can_Be_Read(const std::string &dir){
    std::list<std::string> out;
    struct dirent **eps;
    auto one = [](const struct dirent *) -> int { return 1; };
    int n = scandir(dir.c_str(), &eps, one, alphasort);

    return (n >= 0);
}
   

bool Create_Dir_and_Necessary_Parents(const std::string &dir){
    //Supposed to behave like `mkdir -p $dir`.

    if(dir.empty()) return false;

    //If the parent directory does not exist, create it first.
    const auto parent = Get_Parent_Directory(dir);
    if(!parent.empty() && !Does_Dir_Exist_And_Can_Be_Read(parent)){
        if(!Create_Dir_and_Necessary_Parents(parent)){
            //At this point, parent does not exist or is not accessible, and cannot 
            // be created. There is nothing else we can try.
            return false;
        }
    }

    //Permissions: read, write, search permissions for owner and group, and 
    // read, search permissions for others.
    const auto res = mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if(res != 0) return false;

    return true;
}     


bool Append_List_of_File_and_Dir_Names_in_Dir(std::list<std::string> &out, const std::string &dir){
    struct dirent **eps;
    //Lambda for picking out directories/filenames. We want to collect everything, so simply return 1.
    //
    // To discriminate certain attributes, see 
    //   http://www.gnu.org/software/libc/manual/html_node/Directory-Entries.html#Directory-Entries 
    // (where this routine originally came from!)
    auto one = [](const struct dirent *) -> int { return 1; };
    int n = scandir(dir.c_str(), &eps, one, alphasort);
    if(n >= 0){
        for(int cnt = 0; cnt < n; ++cnt){
            const std::string the_name( eps[cnt]->d_name );
            //We choose to ignore the '.' and '..' because it is usually a given 
            // that they exist and will typically not be of use to return. 
            if((the_name != ".") && (the_name != "..")) out.push_back(the_name);
        }
    }else{
        return false;
    }
    return true;;
}
std::list<std::string> Get_List_of_File_and_Dir_Names_in_Dir(const std::string &dir){
    std::list<std::string> out;
    //const auto wasOK = Append_List_of_File_and_Dir_Names_in_Dir(out, dir);
    //if(!wasOK) out.clear();
    Append_List_of_File_and_Dir_Names_in_Dir(out, dir);
    return out;
}


bool Append_List_of_Full_Path_File_and_Dir_Names_in_Dir(std::list<std::string> &out, const std::string &dir){
    if(dir.empty()) return false;
    const size_t existing = out.size();
    if(!Append_List_of_File_and_Dir_Names_in_Dir(out, dir)) return false;
    
    //Append the passed in directory to the beginning of the filename.
    std::string slasheddir(dir);
    if(*(dir.rbegin()) != '/') slasheddir += "/";
    auto it = std::next(std::begin(out),existing);
    for( ; it != out.end(); ++it) *it = slasheddir + *it;
    return true;
}
std::list<std::string> Get_List_of_Full_Path_File_and_Dir_Names_in_Dir(const std::string &dir){
    std::list<std::string> out;
    //const auto wasOK = Append_List_of_Full_Path_File_and_Dir_Names_in_Dir(out, dir);
    //if(!wasOK) out.clear();
    Append_List_of_Full_Path_File_and_Dir_Names_in_Dir(out, dir);
    return out;
}

std::list<std::string> Get_Recursive_List_of_Full_Path_File_Names_in_Dir(const std::string &dir){
    //Returns a list of all accessible files recursively found as a child of the given directory.
    // If failure is encountered, an empty list is returned.
    std::list<std::string> files;
    std::list<std::string> paths;

    //Seed paths with the specified directories' children, or immediately return.
    if(Append_List_of_Full_Path_File_and_Dir_Names_in_Dir(paths, dir)){
        for(auto it = paths.begin(); it != paths.end();   ){
            //If this is a file, add it to the list of files and remove it.
            if(Does_File_Exist_And_Can_Be_Read(*it)){
                files.push_back(*it);
                it = paths.erase(it);

            //If this is a directory, find children and add them to paths.
            }else if(Does_Dir_Exist_And_Can_Be_Read(*it)){
                if(Append_List_of_Full_Path_File_and_Dir_Names_in_Dir(paths, *it)){
                    it = paths.erase(it);
                }else{
                    files.clear();
                    return files;
                }
            //Otherwise, an error has occurred. Maybe the file/dir is not accessible?
            }else{
                files.clear();
                return files;
            }
        }
    }
    return files;
}


bool Append_List_of_File_Names_in_Dir(std::list<std::string> &out, const std::string &dir){
    struct dirent **eps;
    //Lambda for picking out directories/filenames. Returns 1 only on regular files.
    //
    // To discriminate certain attributes, see 
    //   http://www.gnu.org/software/libc/manual/html_node/Directory-Entries.html#Directory-Entries 
    // (where this routine originally came from!)
    auto one = [](const struct dirent *s) -> int {
        if(s->d_type == DT_REG) return 1; 
        return 0;
    };
    int n = scandir(dir.c_str(), &eps, one, alphasort);
    if(n >= 0){
        for(int cnt = 0; cnt < n; ++cnt){
            const std::string the_name(eps[cnt]->d_name);
            //We choose to ignore the '.' and '..' because it is usually a given 
            // that they exist and will typically not be of use to return. 
            if((the_name != ".") && (the_name != "..")) out.push_back(the_name);
        }
    }else{
        return false;
    }
    return true;
}
std::list<std::string> Get_List_of_File_Names_in_Dir(const std::string &dir){
    std::list<std::string> out;
    const auto wasOK = Append_List_of_File_Names_in_Dir(out, dir);
    if(!wasOK) out.clear();
    return out;
}



bool Append_List_of_Full_Path_File_Names_in_Dir(std::list<std::string> &out, const std::string &dir){
    if(dir.empty()) return false;
    const size_t existing = out.size();
    if(!Append_List_of_File_Names_in_Dir(out, dir)) return false;

    //Append the passed in directory to the beginning of the filename.
    std::string slasheddir(dir);
    if(*(dir.rbegin()) != '/') slasheddir += "/";
    auto it = std::next(std::begin(out),existing);
    for( ; it != out.end(); ++it) *it = slasheddir + *it;
    return true;
}
std::list<std::string> Get_List_of_Full_Path_File_Names_in_Dir(const std::string &dir){
    std::list<std::string> out;
    const auto wasOK = Append_List_of_Full_Path_File_Names_in_Dir(out, dir);
    if(!wasOK) out.clear();
    return out;
}


