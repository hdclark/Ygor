//YgorTAR.h - A collection of routines for writing and reading TAR files.

#include <stddef.h>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <memory>
#include <string>
#include <cstring> // For std::memcpy
#include <limits>
#include <array>
#include <functional>

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

ustar_writer::ustar_writer(std::ostream &os) : blocks_written(0L),
                                               os(os) { }

void 
ustar_writer::add_file(std::istream &is,
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

ustar_writer::~ustar_writer(){
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


// Callback-based TAR file reader; user-provided functor called once per file.
//
// This routine will be able to handle (a subset of the functionality of) files in the following formats:
// - 'ustar' -- POSIX IEEE P1003.1-1990 format TAR files.
// - 'gnu'   -- the default format produced by GNU tar between versions 1.12 and 1.13.25.
// - 'posix' -- (aka 'pax') the default format produced by GNU tar after version 1.13.25, but long filenames
//               will be parsed as though the long filenames are separate files.
//
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
                                   std::string fprefix)> file_handler ){

    if(!file_handler){
        throw std::invalid_argument("User-provided functor is invalid. Cannot continue.");
    }

    const auto octal_string_to_signed_long_int = [](std::string in) -> long int {
        size_t pos;
        return std::stol(in, &pos, 8);
    };

    long int invalid_headers = 0;

    // Loop over the contents of the TAR file.
    while(true){

        // Finished parsing file when two invalid or empty headers are encountered.
        if(invalid_headers >= 2) break;

        // Read in enough data to populate the header (512 bytes).
        ustar_header ustar = {}; // Aggregate initialization, should be all-zero IFF POD.
        nullify_all(ustar); // Explicitly zero'd incase the ustar struct is modified (e.g., in case constructor added).
        //is.read(reinterpret_cast<char *>(&ustar), 512);
        if( false
        || !is.read(reinterpret_cast<char *>(ustar.fname.data()),   ustar.fname.size()  )
        || !is.read(reinterpret_cast<char *>(ustar.fmode.data()),   ustar.fmode.size()  )
        || !is.read(reinterpret_cast<char *>(ustar.fuser.data()),   ustar.fuser.size()  )
        || !is.read(reinterpret_cast<char *>(ustar.fgroup.data()),  ustar.fgroup.size() )
        || !is.read(reinterpret_cast<char *>(ustar.fsize.data()),   ustar.fsize.size()  )
        || !is.read(reinterpret_cast<char *>(ustar.ftime.data()),   ustar.ftime.size()  )
        || !is.read(reinterpret_cast<char *>(ustar.chksum.data()),  ustar.chksum.size() )
        || !is.read(reinterpret_cast<char *>(ustar.ftype.data()),   ustar.ftype.size()  )
        || !is.read(reinterpret_cast<char *>(ustar.flname.data()),  ustar.flname.size() )
        || !is.read(reinterpret_cast<char *>(ustar.ustari.data()),  ustar.ustari.size() )
        || !is.read(reinterpret_cast<char *>(ustar.ustarv.data()),  ustar.ustarv.size() )
        || !is.read(reinterpret_cast<char *>(ustar.o_name.data()),  ustar.o_name.size() )
        || !is.read(reinterpret_cast<char *>(ustar.g_name.data()),  ustar.g_name.size() )
        || !is.read(reinterpret_cast<char *>(ustar.d_major.data()), ustar.d_major.size())
        || !is.read(reinterpret_cast<char *>(ustar.d_minor.data()), ustar.d_minor.size())
        || !is.read(reinterpret_cast<char *>(ustar.fprefix.data()), ustar.fprefix.size())
        || !is.read(reinterpret_cast<char *>(ustar.padding.data()), ustar.padding.size()) ){
            // Incomplete header, parse error, or no more files remaining.
            ++invalid_headers;
            continue;
        }

        // Extract strings.
        std::string fname  (reinterpret_cast<char *>(ustar.fname.data()),   ustar.fname.size()  );
        std::string fmode  (reinterpret_cast<char *>(ustar.fmode.data()),   ustar.fmode.size()  );
        std::string fuser  (reinterpret_cast<char *>(ustar.fuser.data()),   ustar.fuser.size()  );
        std::string fgroup (reinterpret_cast<char *>(ustar.fgroup.data()),  ustar.fgroup.size() );
        std::string fsize  (reinterpret_cast<char *>(ustar.fsize.data()),   ustar.fsize.size()  );
        std::string ftime  (reinterpret_cast<char *>(ustar.ftime.data()),   ustar.ftime.size()  );
        std::string chksum (reinterpret_cast<char *>(ustar.chksum.data()),  ustar.chksum.size() );
        std::string ftype  (reinterpret_cast<char *>(ustar.ftype.data()),   ustar.ftype.size()  );
        std::string flname (reinterpret_cast<char *>(ustar.flname.data()),  ustar.flname.size() );
        std::string ustari (reinterpret_cast<char *>(ustar.ustari.data()),  ustar.ustari.size() );
        std::string ustarv (reinterpret_cast<char *>(ustar.ustarv.data()),  ustar.ustarv.size() );
        std::string o_name (reinterpret_cast<char *>(ustar.o_name.data()),  ustar.o_name.size() );
        std::string g_name (reinterpret_cast<char *>(ustar.g_name.data()),  ustar.g_name.size() );
        std::string d_major(reinterpret_cast<char *>(ustar.d_major.data()), ustar.d_major.size());
        std::string d_minor(reinterpret_cast<char *>(ustar.d_minor.data()), ustar.d_minor.size());
        std::string fprefix(reinterpret_cast<char *>(ustar.fprefix.data()), ustar.fprefix.size());
        std::string padding(reinterpret_cast<char *>(ustar.padding.data()), ustar.padding.size());

        const auto crop_at_first_null = [](std::string &in) -> void {
            in.erase(std::find(in.begin(), in.end(), '\0'), in.end());
            return;
        };

        crop_at_first_null(fname  );
        crop_at_first_null(fmode  );
        crop_at_first_null(fuser  );
        crop_at_first_null(fgroup );
        crop_at_first_null(fsize  );
        crop_at_first_null(ftime  );
        crop_at_first_null(chksum );
        crop_at_first_null(ftype  );
        crop_at_first_null(flname );
        crop_at_first_null(ustari );
        crop_at_first_null(ustarv );
        crop_at_first_null(o_name );
        crop_at_first_null(g_name );
        crop_at_first_null(d_major);
        crop_at_first_null(d_minor);
        crop_at_first_null(fprefix);
        crop_at_first_null(padding);

        // Skip if header is empty.
        //
        // Two of these indicate the end of further records.
        if(true
        && fname  .empty()
        && fmode  .empty()
        && fuser  .empty()
        && fgroup .empty()
        && fsize  .empty()
        && ftime  .empty()
        && chksum .empty()
        && ftype  .empty()
        && flname .empty()
        && ustari .empty()
        && ustarv .empty()
        && o_name .empty()
        && g_name .empty()
        && d_major.empty()
        && d_minor.empty()
        && fprefix.empty()
        && padding.empty() ){
            ++invalid_headers;
            continue;
        }
        
        // Assume the record is not empty, and therefore a valid record.

        // Determine whether the record is sound and we support the file type.
        if(false){
        }else if( (ustari == "ustar") && (ustarv == "00") ){ // 'ustar\0' and '  '.
            // 'ustar' POSIX IEEE P1003.1-1990 format. Nothing needs to be done...
        }else if( (ustari == "ustar ") && (ustarv == " ") ){ // 'ustar ' and ' \0'. 
            // 'gnu' format: incompatible extensions with ustar and posix/pax formats.
            //
            // Differences in the header start at byte 345, replacing the ustar 'prefix' and 'padding' fields with
            // the atime[12], ctime[12], offset[12], longnames[4], padding[1], sparse[96], realsize[12],
            // isextended[1], and padding[17].
            //
            // Since this metadata is not supported, we simply ensure none of the data layout modifying extensions are
            // actually being used.
            fprefix.clear();
            padding.clear();

            std::string gnu_extensions(reinterpret_cast<char *>(ustar.fprefix.data()) + 12 + 12, 12 + 4 + 1 + 96 + 12 + 1);
            crop_at_first_null(gnu_extensions);
            if(!gnu_extensions.empty()){
                throw std::runtime_error("Unsupported GNU extensions encountered. Refusing to continue.");
            }
        }else{
            throw std::runtime_error("Unrecognized TAR format. Refusing to continue.");
        }

        const long int fsize_l = octal_string_to_signed_long_int(fsize);
        if( (fsize_l < 0) 
        // Ustar file format limited to 8GB per individual file.
        ||  ( (ustari == "ustar") && (ustarv == "00") && (8'589'934'591 < fsize_l) )
        // GNU can be larger, but the highest bit indicates the size is stored elsewhere.
        ||  ( (ustari == "ustar ") && (ustarv == " ") && (4'294'967'295 < fsize_l) ) ){
            throw std::runtime_error("Unsupported encapsulated file size. Refusing to continue.");
        }

        long int ftime_l = 0;
        if(!ftime.empty()){
            try{
                ftime_l = octal_string_to_signed_long_int(ftime);
            }catch(const std::exception &){
                //Consider file invalid if one of the parameters is malformed.
                ++invalid_headers;
                continue;
            }
        }

        // Re-compute the checksum.
        {
            const auto chksum_existing = ustar.chksum;
            ustar.chksum.fill(' ');
            compute_checksum(ustar);
            const auto chksum_recalc = ustar.chksum;
            if(chksum_existing != chksum_recalc){
                ++invalid_headers;
                continue;
            }
        }

        const auto is_regular_file = (ftype == "0"_s) || ftype.empty();

        // Trust the header is valid and has been parsed correctly.

        // Marshall the file contents into a separate stream so we don't have to trust the file handler functor to consume
        // only the indicated number of bytes.
        std::stringstream ss;
        {
            auto total_bytes_remaining = static_cast<std::streamsize>(fsize_l);
            std::array<char, 512> buf;
            while(true){
                const auto bytes_to_read = std::min<std::streamsize>(buf.size(), total_bytes_remaining);
                is.read(buf.data(), bytes_to_read);
                const auto last_bytes_read = is.gcount(); // Note: can be less than requested.
                ss.write(buf.data(), last_bytes_read);
                total_bytes_remaining -= last_bytes_read;
                if(total_bytes_remaining == 0) break;
                if(!is.good()) throw std::runtime_error("Insufficient data available in archive; it is likely invalid.");
            }
        }
        ss.flush();

        // Invoke the user functor.
        if(is_regular_file){
            file_handler(ss,
                         fname,
                         fsize_l,
                         fmode,
                         fuser,
                         fgroup,
                         ftime_l,
                         o_name,
                         g_name,
                         fprefix);
        }

        // Read the remaining padding bytes.
        const auto rem = std::ldiv(fsize_l, 512L).rem;
        const auto zero_pad_bytes = (rem == 0L ? 0L : 512L - rem);
        for(long int i = 0L; i < zero_pad_bytes; ++i) is.get();
    }

    return;
}

