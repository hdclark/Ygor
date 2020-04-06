//YgorTAR.h - A collection of routines for writing and reading TAR files.


#ifndef YGOR_TAR_H_
#define YGOR_TAR_H_

#include <cstdint>
#include <iosfwd>
#include <string>
#include <array>
#include <functional>

#include "YgorDefinitions.h"

// Representation of the 'Unix Standard TAR' file format (i.e., 'ustar', as per POSIX IEEE P1003.1-1990).
struct ustar_header {
    std::array<uint8_t, 100> fname;    // Unprefixed filename with null termination IFF room available.
    std::array<uint8_t,   8> fmode;    // File permissions, as octal string.
    std::array<uint8_t,   8> fuser;    // File user id, as octal string.
    std::array<uint8_t,   8> fgroup;   // File group id, as octal string.
    std::array<uint8_t,  12> fsize;    // File size in bytes, as octal string.
    std::array<uint8_t,  12> ftime;    // Modification time in UNIX time, as octal string.
    std::array<uint8_t,   8> chksum;   // Header checksum, assuming self is 8 spaces, as octal string.
    std::array<uint8_t,   1> ftype;    // File type. 0 for normal file.
    std::array<uint8_t, 100> flname;   // Linked file name, if file type is a link and null terminated IFF room available.
    std::array<uint8_t,   6> ustari;   // Ustar identifier. Should contain "ustar\0".
    std::array<uint8_t,   2> ustarv;   // Version of ustar being used. Should contain "00".
    std::array<uint8_t,  32> o_name;   // Owner user name. Null terminated.
    std::array<uint8_t,  32> g_name;   // Owner group name. Null terminated.
    std::array<uint8_t,   8> d_major;  // Device major number, as octal string.
    std::array<uint8_t,   8> d_minor;  // Device minor number, as octal string.
    std::array<uint8_t, 155> fprefix;  // File prefix with null termination IFF room available.
    std::array<uint8_t,  12> padding;  // Padding to ensure header is 512 bytes.
};

// Set all members to contain all-NULLs.
void nullify_all(ustar_header &);

// Update the header checksum member.
void compute_checksum(ustar_header &);

// RAII manager class that simplifies constructing TAR format for the user.
class ustar_writer {
    private:
        long int blocks_written; // The total number of 512-byte blocks written.
                                 // Customarily, the final number of blocks will be a multiple of 20.

        std::ostream &os;

    public:
        ustar_writer(std::ostream &os); // Note: stream scope must outlast this instance!

        void add_file(std::istream &is,            // Contents of the file. If no file size provided, this stream must be seekable!
                      std::string fname,           // e.g., 'afile.txt'.
                      long int fsize      = -1UL,  // File size in bytes. If negative, attempt to seek. If positive then write and confirm the size.
                      std::string fmode   = "644", // Specifying "" results in annoying files when unpacked.
                      std::string fuser   = "",    // ID, e.g., "1000".
                      std::string fgroup  = "",    // ID, e.g., "1000".
                      long int ftime      = -1UL,  // UNIX time, in seconds. If negative, time is omitted.
                      std::string o_name  = "",    // Owner name, e.g., "user".
                      std::string g_name  = "",    // Owner name, e.g., "users".
                      std::string fprefix = "" );  // Prefix directory, e.g., "/path/to/a/directory".

        ~ustar_writer(); // Finalizes file and flushes stream.
};

// Callback-based TAR file reader; user-provided functor called once per file.
void read_ustar(std::istream &is,
                std::function<void(std::istream &is,
                                   std::string fname,
                                   long int fsize,
                                   std::string fmode,
                                   std::string fuser,
                                   std::string fgroup,
                                   long int ftime,
                                   std::string o_name,
                                   std::string g_name,
                                   std::string fprefix)> file_handler );

#endif
