#include <stddef.h>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <string>
#include <limits>

#include "YgorDefinitions.h"
#include "YgorString.h"
#include "YgorMisc.h"
#include "YgorTAR.h"

// Misc helper routines.
static 
std::string
pad_left_zeros(std::string in, long int desired_length){
    while(static_cast<long int>(in.length()) < desired_length) in = "0"_s + in;
    return in;
}

static
std::string
as_octal_signed(long int n){
    std::stringstream ss;
    ss << std::oct << n;
    return ss.str();
}

static
std::string
as_octal_unsigned(unsigned long int n){
    std::stringstream ss;
    ss << std::oct << n;
    return ss.str();
}

// Set all members to contain all-NULLs.
void nullify_all(ustar_header &a){
    a.fname.fill('\0');
    a.fmode.fill('\0');
    a.fuser.fill('\0');
    a.fgroup.fill('\0');
    a.fsize.fill('\0');
    a.ftime.fill('\0');
    a.chksum.fill('\0');
    a.ftype.fill('\0');
    a.flname.fill('\0');
    a.ustari.fill('\0');
    a.ustarv.fill('\0');
    a.o_name.fill('\0');
    a.g_name.fill('\0');
    a.d_major.fill('\0');
    a.d_minor.fill('\0');
    a.fprefix.fill('\0');
    a.padding.fill('\0');
    return;
}

// Update the header checksum member.
void compute_checksum(ustar_header &a){
    unsigned long int sum = 0;

    a.chksum.fill(' ');
 
    for(const auto &b : a.fname)   sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.fmode)   sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.fuser)   sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.fgroup)  sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.fsize)   sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.ftime)   sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.chksum)  sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.ftype)   sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.flname)  sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.ustari)  sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.ustarv)  sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.o_name)  sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.g_name)  sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.d_major) sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.d_minor) sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.fprefix) sum += static_cast<unsigned long int>(b);
    for(const auto &b : a.padding) sum += static_cast<unsigned long int>(b);

    auto chksum = pad_left_zeros(as_octal_unsigned(sum), 6);
    chksum += '\0';
    std::memcpy(a.chksum.data(), chksum.data(), 7);

    return;
}

ustar_archive_writer::ustar_archive_writer(std::ostream &os) : blocks_written(0L), os(os) { }

void 
ustar_archive_writer::add_file(std::istream &is,
                               std::string fname,
                               long int fsize,
                               std::string fmode,
                               std::string fuser,
                               std::string fgroup,
                               long int ftime,
                               std::string o_name,
                               std::string g_name,
                               std::string fprefix ){

    ustar_header ustar = {}; // Aggregate initialization, should be all-zero IFF POD.
    nullify_all(ustar); // Explicitly zero'd incase the ustar struct is modified (e.g., in case constructor added).

    // Validate elements before writing anything or touching the input file stream.
    if(static_cast<size_t>(100) < fname.size()){
        throw std::invalid_argument("File name too long. Consider using prefix field if possible. Refusing to continue.");
    }
    if(static_cast<size_t>(155) < fprefix.size()){
        throw std::invalid_argument("File prefix too long. Refusing to continue.");
    }

    // ... Validate all user-provided entries, both the size and the contents (e.g., numbers, contains NULLs, etc.) ... 
    // TODO
    // ...
    

    // If necessary, determine how long the file is.
    //
    // Note: this step will fail if the stream is not seekable, but in such cases the user will need to provide the file
    // size directly -- possibly by streaming into a std::stringstream and counting the total size directly.
    //
    if(fsize <= 0L){
        // Determine how many bytes will be read by ... reading them all, counting, and reseting the stream position.
        is.ignore( std::numeric_limits<std::streamsize>::max() );
        const std::streamsize bytes_available = is.gcount();
        is.clear(); // Reset the EOF bit.
        is.seekg( 0UL, std::ios_base::beg );
        if(!is) throw std::runtime_error("Unable to seek input stream. Please provide file size explicitly. Refusing to continue.");
        fsize = static_cast<long int>(bytes_available);
    }

    // Add metadata to the header struct.
    std::memcpy(ustar.fname.data(),   fname.data(),   std::min<unsigned long int>(100, fname.size())  );

    if(!fmode.empty()){
        fmode = pad_left_zeros(fmode, 7);
        std::memcpy(ustar.fmode.data(),   fmode.data(),   std::min<unsigned long int>(  7, fmode.size())  );
    }

    if(!fuser.empty()){
        fuser = pad_left_zeros(fuser, 7);
        std::memcpy(ustar.fuser.data(),   fuser.data(),   std::min<unsigned long int>(  7, fuser.size())  );
    }

    if(!fgroup.empty()){
        fgroup = pad_left_zeros(fgroup, 7);
        std::memcpy(ustar.fgroup.data(),  fgroup.data(),  std::min<unsigned long int>(  7, fgroup.size()) );
    }

    const auto fsize_str = pad_left_zeros(as_octal_signed(fsize), 11);
    std::memcpy(ustar.fsize.data(),   fsize_str.data(),   std::min<unsigned long int>( 11, fsize_str.size())  );

    if(0L <= ftime){
        const auto ftime_str = pad_left_zeros(as_octal_signed(ftime), 11);
        std::memcpy(ustar.ftime.data(),   ftime_str.data(),   std::min<unsigned long int>( 11, ftime_str.size())  );
    }

    const auto ftype = "0"_s; // Normal file type.
    std::memcpy(ustar.ftype.data(),   ftype.data(),   std::min<unsigned long int>(  1, ftype.size())  );

    const auto flname = ""_s;
    std::memcpy(ustar.flname.data(),  flname.data(),  std::min<unsigned long int>(100, flname.size()) );

    const auto ustari = "ustar\0"_s;
    std::memcpy(ustar.ustari.data(),  ustari.data(),  std::min<unsigned long int>(  6, ustari.size()) );

    const auto ustarv = "00"_s;
    std::memcpy(ustar.ustarv.data(),  ustarv.data(),  std::min<unsigned long int>(  2, ustarv.size()) );

    std::memcpy(ustar.o_name.data(),  o_name.data(),  std::min<unsigned long int>( 31, o_name.size())  );

    std::memcpy(ustar.g_name.data(),  g_name.data(),  std::min<unsigned long int>( 31, g_name.size())  );

    std::string d_major = ""_s; // If made accessible, this should be provided as an octal number.
    if(!d_major.empty()){
        d_major = pad_left_zeros(d_major, 7);
        std::memcpy(ustar.d_major.data(), d_major.data(), std::min<unsigned long int>(  7, d_major.size()));
    }

    std::string d_minor = ""_s; // If made accessible, this should be provided as an octal number.
    if(!d_minor.empty()){
        d_minor = pad_left_zeros(d_minor, 7);
        std::memcpy(ustar.d_minor.data(), d_minor.data(), std::min<unsigned long int>(  7, d_minor.size()));
    }

    std::memcpy(ustar.fprefix.data(), fprefix.data(), std::min<unsigned long int>(155, fprefix.size()));

    compute_checksum(ustar);

    // Emit the header.
    //ofs.write(reinterpret_cast<char *>(&ustar), 512);
    os.write(reinterpret_cast<const char *>(ustar.fname.data()),   ustar.fname.size()  );
    os.write(reinterpret_cast<const char *>(ustar.fmode.data()),   ustar.fmode.size()  );
    os.write(reinterpret_cast<const char *>(ustar.fuser.data()),   ustar.fuser.size()  );
    os.write(reinterpret_cast<const char *>(ustar.fgroup.data()),  ustar.fgroup.size() );
    os.write(reinterpret_cast<const char *>(ustar.fsize.data()),   ustar.fsize.size()  );
    os.write(reinterpret_cast<const char *>(ustar.ftime.data()),   ustar.ftime.size()  );
    os.write(reinterpret_cast<const char *>(ustar.chksum.data()),  ustar.chksum.size() );
    os.write(reinterpret_cast<const char *>(ustar.ftype.data()),   ustar.ftype.size()  );
    os.write(reinterpret_cast<const char *>(ustar.flname.data()),  ustar.flname.size() );
    os.write(reinterpret_cast<const char *>(ustar.ustari.data()),  ustar.ustari.size() );
    os.write(reinterpret_cast<const char *>(ustar.ustarv.data()),  ustar.ustarv.size() );
    os.write(reinterpret_cast<const char *>(ustar.o_name.data()),  ustar.o_name.size() );
    os.write(reinterpret_cast<const char *>(ustar.g_name.data()),  ustar.g_name.size() );
    os.write(reinterpret_cast<const char *>(ustar.d_major.data()), ustar.d_major.size());
    os.write(reinterpret_cast<const char *>(ustar.d_minor.data()), ustar.d_minor.size());
    os.write(reinterpret_cast<const char *>(ustar.fprefix.data()), ustar.fprefix.size());
    os.write(reinterpret_cast<const char *>(ustar.padding.data()), ustar.padding.size());
    ++this->blocks_written;

    // Transfer the file data and top-up to the next 512 segment.
    std::streamsize total_file_bytes_written = 0;
    {
        std::array<char, 512> buf;
        while(true){
            is.read(buf.data(), buf.size());
            const auto last_bytes_read = is.gcount(); // Can be less than buffer size.
            os.write(buf.data(), last_bytes_read);
            total_file_bytes_written += last_bytes_read;
            if(static_cast<size_t>(last_bytes_read) < buf.size()) break;
        }
    }
    if(total_file_bytes_written != static_cast<std::streamsize>(fsize)){
        throw std::invalid_argument("Incorrect file size provided. Refusing to continue.");
    }

    const auto zero_pad_bytes = static_cast<size_t>( (fsize % 512) == 0 ? 0 : 512 - (fsize % 512) );
    for(size_t i = 0; i < zero_pad_bytes; ++i) os.put('\0');
    this->blocks_written += (fsize + 512) / 512;

    os.flush();
    if(!os) throw std::runtime_error("Unable to write file to TAR. Refusing to continue.");

    return;
}



ustar_archive_writer::~ustar_archive_writer(){
    // Write two full empty records to signify the end.
    for(size_t i = 0; i < 512; ++i) os.put('\0');
    for(size_t i = 0; i < 512; ++i) os.put('\0');
    this->blocks_written += 2;

    // Pad the number of blocks to be a multiple of 20, which is conventional.
    const auto zero_pad_blocks = ( (this->blocks_written % 20) == 0 ? 0 : 20 - (this->blocks_written % 20) );
    for(long int j = 0; j < zero_pad_blocks; ++j){
        for(size_t i = 0; i < 512; ++i) os.put('\0');
    }

    if(!os){
        FUNCWARN("File stream not in good state after emitting TAR archive");
        // Note: We cannot throw in the destructor, so the best we can do is warn about it.
        //
        // We could alternatively terminate, but that may not be unacceptable.
        // On the other hand, it may be unacceptable to continue the program.
        // Many RAII classes encounter this issue, and the STL classes tend to ignore errors in destructors, so this is
        // what we'll go with for now.
    }
    os.flush();
}

