//YgorFilesDirs.h

#ifndef YGOR_FILES_DIRS_HDR_GRD_H
#define YGOR_FILES_DIRS_HDR_GRD_H

#include <sys/stat.h>
#include <cstdint> //For intmax_t
#include <ctime>
#include <fstream>
#include <list>
#include <memory>
#include <sstream>
#include <string>

#include "YgorDefinitions.h"


//--------------------------------------------------------------------------------------------------------
//---------------------------------------------- Files Only ----------------------------------------------
//--------------------------------------------------------------------------------------------------------

//Checks if a file exists and is available to be read. Closes the file in either case.
bool Does_File_Exist_And_Can_Be_Read(const std::string &filename);

//Returns the first filename which exists and is available to be read, or an empty string.
std::string Get_First_Filename_Which_Exists_And_Can_Be_Read(const std::list<std::string> &filenames);

//Returns the size of the file in bytes without having to open/seek/close it. 
//off_t Size_of_File(const std::string &filename);
intmax_t Size_of_File(const std::string &filename);

//Returns last time of access, or (std::time_t)(-1) if an error occurs.
// Should work for both files and directories.
std::time_t Last_Access_Time(const std::string &pathorfilename);

//Returns last time of modification, or (std::time_t)(-1) if an error occurs.
// Should work for both files and directories.
std::time_t Last_Modification_Time(const std::string &pathorfilename);

//Returns a string containing an md5, or an empty string on failure.
std::string MD5_of_File(const std::string &filename);
 
//Gets from byte N to byte N+L from a file. Should only use for large files, because it is opened/closed each time.
template <class T> std::unique_ptr<T[]> Get_Piece_of_Binary_File(const std::string &filename_in, intmax_t N, intmax_t L); //std::ios::pos_type N, std::ios::pos_type L);

//Checks if a FIFO exists.
bool Does_Fifo_Exist(const std::string &filename);

//Generates random filenames, given a prefix filename. This replaces mkstemp, tmpfile, etc.. from unistd
// and other non-portable (non-reliable?) methods (just need to wrap the guts of these functions for other
// systems...)   NOTE: This routine does *not* reserve the name for you!
std::string Get_Unique_Filename(const std::string &prefix, const int64_t len, const std::string &suffix = "");
std::string Get_Unique_Sequential_Filename(const std::string &prefix, const int64_t n_of_zero_pads = 6, const std::string &suffix = "");

//Expand a filename/partial path/full path/symbolic link/./.. into a full path. Empty string is returned on fail.
std::string Fully_Expand_Filename(const std::string &filename);

//Determines if a path is below another path. Useful for specifying whitelists of files/directories.
bool File_Is_Recursively_Within_Directory(const std::string &filename_in, const std::string &pathname_in);

//Reads a file in binary mode and returns a pointer to the data.
template <class T> std::unique_ptr<T[]> Load_Binary_File(const std::string &filename_in, intmax_t *size);

//Writes a block of data to file.
template <class T> bool Write_Binary_File(const std::string &filename_out, std::unique_ptr<T[]> in, intmax_t size);

//Append a block of data to file.
template <class T> bool Append_Binary_File(const std::string &filename_out, std::unique_ptr<T[]> in, intmax_t size);

//Load a file into a string, line by line, delimited (specifically) by '\n'.
std::string LoadFileToString(const std::string &filename_in);

//Load a binary file and copy the memory into a std::string. Involves a copy, so use only for small things.
std::string LoadBinaryFileToString(const std::string &filename_in);

//Load a file into a list with no delimiters.
std::list<std::string> LoadFileToList(const std::string &filename_in);

//Write a string to a file. No extra formatting is performed. Returns true on success.
bool WriteStringToFile(const std::string &in, const std::string &filename, bool overwrite = false);
bool OverwriteStringToFile(const std::string &in, const std::string &filename);

//Write a binary string to a file. Returns true on success.
bool WriteBinaryStringToFile(const std::string &in, const std::string &filename, bool overwrite = false);

//Append a string to file, creating the file if necessary. Returns true on success.
bool AppendStringToFile(const std::string &in, const std::string &filename);

//Creates a FIFO at the specified path. User must then read/write to use it.
bool CreateFIFOFile(const std::string &filename_in);

//Remove a file using the cstdio remove() function. Requires a specific filename.
bool RemoveFile(const std::string &filename);

//Touch a file: write 0 bytes to it, creating if necessary.
bool TouchFile(const std::string &filename);

//Copy a file from source to destination.
bool CopyFile(const std::string &source, const std::string &destination);

//--------------------------------------------------------------------------------------------------------
//---------------------------------------------- Directories ---------------------------------------------
//--------------------------------------------------------------------------------------------------------

//Checks if a directory exists and can be read. 
bool Does_Dir_Exist_And_Can_Be_Read(const std::string &dir);

//Acts like `mkdir -p $dir`. Supply full path if (somehow) in an orphan working directory.
bool Create_Dir_and_Necessary_Parents(const std::string &dir);


//Appends to a list the files and (non-'.' and -'..') dirs under a dir. False on error (or empty dir!).
bool Append_List_of_File_and_Dir_Names_in_Dir(std::list<std::string> &out, const std::string &dir);
//Returns a list of the files and (non-'.' and -'..') dirs under a dir. Empty list on error (or empty dir!).
std::list<std::string> Get_List_of_File_and_Dir_Names_in_Dir(const std::string &dir);

//Same as 'Get_List_of_File_and_Dir_Names_in_Dir()' but with canonical path.
bool Append_List_of_Full_Path_File_and_Dir_Names_in_Dir(std::list<std::string> &out, const std::string &dir);
//Same as 'Get_List_of_File_and_Dir_Names_in_Dir()' but with canonical path.
std::list<std::string> Get_List_of_Full_Path_File_and_Dir_Names_in_Dir(const std::string &dir);

//Returns a list of all accessible files recursively found as a child of the given directory.
std::list<std::string> Get_Recursive_List_of_Full_Path_File_Names_in_Dir(const std::string &dir);


//Returns a list of the files under a dir. False on error (or empty dir!).
bool Append_List_of_File_Names_in_Dir(std::list<std::string> &out, const std::string &dir);
//Returns a list of the files under a dir. Empty list on error (or empty dir!).
std::list<std::string> Get_List_of_File_Names_in_Dir(const std::string &dir);

//Same as 'Get_List_of_File_Names_in_Dir()' but with canonical path.
bool Append_List_of_Full_Path_File_Names_in_Dir(std::list<std::string> &out, const std::string &dir);
//Same as 'Get_List_of_File_Names_in_Dir()' but with canonical path.
std::list<std::string> Get_List_of_Full_Path_File_Names_in_Dir(const std::string &dir);

#endif

