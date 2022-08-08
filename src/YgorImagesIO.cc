//YgorImagesIO.cc - Routines for writing and reading images.
//
#include <algorithm>
#include <array>
#include <cinttypes>
#include <optional>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <istream>
#include <ostream>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <functional>

#include "YgorDefinitions.h"
#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorImages.h"
#include "YgorString.h"

#include "YgorImagesIO.h"

//#ifndef YGOR_IMAGES_IO_DISABLE_ALL_SPECIALIZATIONS
//    #define YGOR_IMAGES_IO_DISABLE_ALL_SPECIALIZATIONS
//#endif


//Dump raw pixel data to file as type T.
//
// NOTE: This data can be converted to whatever using ImageMagick ala:
//       convert -size `W`x`H` -depth `sizeof(T)` -define quantum:format=`T` -type grayscale in.gray -depth 16 ... out.jpg
//       where the number of channels, pixel size, etc.. are recorded in other places (no header).
//       At the time of writing, this does not work when T = uint32_t, and no info is produced as 
//       to why. It seems the ImageMagick QuantumDepth might be compiled-in too low, but it is not
//       itself smart enough to realize there is an issue ... even though you are specifing the
//       number of bits directly ...
//
//       For floating point numbers (or situations where you've got floating point numbers packed
//       into uint32_t or other weird ideas), use `quantum:format=floating-point`. Doubles can use
//       `quantum:format=double`.
//
// NOTE: ImageMagick annoyingly seems to silently choke on uint32_t per channel, producing no 
//       output and reporting no errors. Be aware that you will have to find another converter, or
//       will later have to do it yourself. The appeal of writing in that format would probably only
//       be to make saving to, and later loading from, raw files easier.
//
//       Ideally, you should write to a more structured format, like FITS or TIFF.
//
// NOTE: Will not overwrite existing files, except if there is a race condition between check and
//       writing.
//
template <class T, class R>
bool
Dump_Pixels(const planar_image<T,R> &img,
            const std::string &filename,
            YgorEndianness destendian){

    //Check if the file exists. If it does, we will refuse to overwrite it.
    {
        std::ifstream FI(filename, std::ios::in | std::ios::binary);
        if(FI.good()) return false;
    }

    //Work out the endianness of this machine and the destination file.
    const auto MachineEndianness = YgorEndianness::Host;
    const auto UsingLittleEndian = (MachineEndianness == YgorEndianness::Little);
    const auto UsingBigEndian = (MachineEndianness == YgorEndianness::Big);

    const bool WriteLittleEndian = (destendian == YgorEndianness::Little);
    const bool WriteBigEndian    = (destendian == YgorEndianness::Big);
    if( (!WriteLittleEndian && !WriteBigEndian) || (WriteLittleEndian && WriteBigEndian) ){
        throw std::runtime_error("Cannot determine which endianness to write!");
        // NOTE: This routine needs to be modified to handle additional endian-types if the above throws.
    }

    std::ofstream FO(filename, std::ios::out | std::ios::binary);
    if(!FO) return false;

    for(auto row = 0; row < img.rows; ++row){
        for(auto col = 0; col < img.columns; ++col){
            for(auto chan = 0; chan < img.channels; ++chan){
                T val = img.value(row, col, chan);

                unsigned char *c = reinterpret_cast<unsigned char *>(&val); //c[0] to c[sizeof(T)-1] only!
                if( (UsingLittleEndian && WriteBigEndian) || (UsingBigEndian && WriteLittleEndian) ){
                    //Reverse the order of the bytes written to the stream.
                    for(size_t i = 0; i <= (sizeof(T)-1); ++i) FO.put(c[sizeof(T)-1-i]);
                }else if( (UsingLittleEndian && WriteLittleEndian) || (UsingBigEndian && WriteBigEndian) ){
                    //Keep the byte order unaltered.
                    FO.write( reinterpret_cast<char *>( &c[0] ), sizeof(T) );
                }
            }
        }
    }

    //Check if it was successful, clean up, and carry on.
    if(!FO.good()) return false;
    FO.close();
    return true;
}
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool Dump_Pixels(const planar_image<uint8_t ,double> &, const std::string &, YgorEndianness);
    template bool Dump_Pixels(const planar_image<uint16_t,double> &, const std::string &, YgorEndianness);
    template bool Dump_Pixels(const planar_image<uint32_t,double> &, const std::string &, YgorEndianness);
    template bool Dump_Pixels(const planar_image<uint64_t,double> &, const std::string &, YgorEndianness);
    template bool Dump_Pixels(const planar_image<float   ,double> &, const std::string &, YgorEndianness);
    template bool Dump_Pixels(const planar_image<double  ,double> &, const std::string &, YgorEndianness);
#endif


//Dump raw pixel data to file, but static_cast to type Y and possibly autoscaled to fill
// the range of type Y (e.g., scaling proportionally to the min and max of type T).
template <class T, class R, class Y>
bool
Dump_Casted_Scaled_Pixels(const planar_image<T,R> &img,
                          const std::string &filename,
                          YgorImageIOPixelScaling scaling,
                          YgorEndianness destendian){

    //Check if the file exists. If it does, we will refuse to overwrite it.
    {
        std::ifstream FI(filename, std::ios::in | std::ios::binary);
        if(FI.good()) return false;
    }

    //Work out the endianness of this machine and the destination file.
    const auto MachineEndianness = YgorEndianness::Host;
    const auto UsingLittleEndian = (MachineEndianness == YgorEndianness::Little);
    const auto UsingBigEndian = (MachineEndianness == YgorEndianness::Big);

    const bool WriteLittleEndian = (destendian == YgorEndianness::Little);
    const bool WriteBigEndian    = (destendian == YgorEndianness::Big);
    if( (!WriteLittleEndian && !WriteBigEndian) || (WriteLittleEndian && WriteBigEndian) ){
        throw std::runtime_error("Cannot determine which endianness to write!");
        // NOTE: This routine needs to be modified to handle additional endian-types if the above throws.
    }

    std::ofstream FO(filename, std::ios::out | std::ios::binary);
    if(!FO) return false;

    for(auto row = 0; row < img.rows; ++row){
        for(auto col = 0; col < img.columns; ++col){
            for(auto chan = 0; chan < img.channels; ++chan){
                const T valT = img.value(row, col, chan);

                //Cast to the new type.
                Y valY = static_cast<Y>(valT);
 
                //Scale and cast the value instead, iff the user wants to.
                if(scaling == YgorImageIOPixelScaling::TypeMinMax){
                    valY = Ygor_Scale_With_Type_Range<Y,T>(valT);
                }

                unsigned char *c = reinterpret_cast<unsigned char *>(&valY); //c[0] to c[sizeof(Y)-1] only!
                if( (UsingLittleEndian && WriteBigEndian) || (UsingBigEndian && WriteLittleEndian) ){
                    //Reverse the order of the bytes written to the stream.
                    for(size_t i = 0; i <= (sizeof(Y)-1); ++i) FO.put(c[sizeof(Y)-1-i]);
                }else if( (UsingLittleEndian && WriteLittleEndian) || (UsingBigEndian && WriteBigEndian) ){
                    //Keep the byte order unaltered.
                    FO.write( reinterpret_cast<char *>( &c[0] ), sizeof(Y) );
                }
            }
        }
    }

    //Check if it was successful, clean up, and carry on.
    if(!FO.good()) return false;
    FO.close();
    return true;
}
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool Dump_Casted_Scaled_Pixels<uint8_t ,double, uint8_t >(const planar_image<uint8_t ,double> &, const std::string &, 
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<uint16_t,double, uint8_t >(const planar_image<uint16_t,double> &, const std::string &, 
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<uint32_t,double, uint8_t >(const planar_image<uint32_t,double> &, const std::string &, 
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<uint64_t,double, uint8_t >(const planar_image<uint64_t,double> &, const std::string &, 
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<float   ,double, uint8_t >(const planar_image<float   ,double> &, const std::string &, 
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<double  ,double, uint8_t >(const planar_image<double  ,double> &, const std::string &, 
                                                    YgorImageIOPixelScaling, YgorEndianness);

    template bool Dump_Casted_Scaled_Pixels<uint8_t ,double, uint16_t>(const planar_image<uint8_t ,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<uint16_t,double, uint16_t>(const planar_image<uint16_t,double> &, const std::string &,  
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<uint32_t,double, uint16_t>(const planar_image<uint32_t,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<uint64_t,double, uint16_t>(const planar_image<uint64_t,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<float   ,double, uint16_t>(const planar_image<float   ,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<double  ,double, uint16_t>(const planar_image<double  ,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);

    template bool Dump_Casted_Scaled_Pixels<uint8_t ,double, float   >(const planar_image<uint8_t ,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<uint16_t,double, float   >(const planar_image<uint16_t,double> &, const std::string &,  
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<uint32_t,double, float   >(const planar_image<uint32_t,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<uint64_t,double, float   >(const planar_image<uint64_t,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<float   ,double, float   >(const planar_image<float   ,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<double  ,double, float   >(const planar_image<double  ,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);

    template bool Dump_Casted_Scaled_Pixels<uint8_t ,double, double  >(const planar_image<uint8_t ,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<uint16_t,double, double  >(const planar_image<uint16_t,double> &, const std::string &,  
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<uint32_t,double, double  >(const planar_image<uint32_t,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<uint64_t,double, double  >(const planar_image<uint64_t,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<float   ,double, double  >(const planar_image<float   ,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);
    template bool Dump_Casted_Scaled_Pixels<double  ,double, double  >(const planar_image<double  ,double> &, const std::string &,    
                                                    YgorImageIOPixelScaling, YgorEndianness);
#endif


//Write pixels and metadata to a FITS formatted file.
//
// NOTE: You can easily examine such files like so: `fold -w 80 theimage.fit | less`
//       Each block corresponds to exactly 36 lines, each with 80 characters.
template <class T, class R>
bool WriteToFITS(const planar_image<T,R> &img, const std::string &filename, YgorEndianness userE){
    std::list<std::reference_wrapper<const planar_image<T,R>>> imgs_refws;
    imgs_refws.emplace_back( std::ref(img) );

    std::ofstream os(filename, std::ios::out | std::ios::binary);
    if(!os.good()){
        FUNCWARN("Unable to open file");
        return false;
    }

    return WriteToFITS<T,R>(imgs_refws, os, userE);
}
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool WriteToFITS(const planar_image<uint8_t ,double> &, const std::string &, YgorEndianness);
    template bool WriteToFITS(const planar_image<uint16_t,double> &, const std::string &, YgorEndianness);
    template bool WriteToFITS(const planar_image<uint32_t,double> &, const std::string &, YgorEndianness);
    template bool WriteToFITS(const planar_image<uint64_t,double> &, const std::string &, YgorEndianness);
    template bool WriteToFITS(const planar_image<float   ,double> &, const std::string &, YgorEndianness);
    template bool WriteToFITS(const planar_image<double  ,double> &, const std::string &, YgorEndianness);
#endif

template <class T, class R>
bool WriteToFITS(const planar_image_collection<T,R> &imgcoll, const std::string &filename, YgorEndianness userE){
    std::list<std::reference_wrapper<const planar_image<T,R>>> imgs_refws;
    for(const auto& img : imgcoll.images){
        imgs_refws.emplace_back( std::ref(img) );
    }

    std::ofstream os(filename, std::ios::out | std::ios::binary);
    if(!os.good()){
        FUNCWARN("Unable to open file");
        return false;
    }

    return WriteToFITS<T,R>(imgs_refws, os, userE);
}
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool WriteToFITS(const planar_image_collection<uint8_t ,double> &, const std::string &, YgorEndianness);
    template bool WriteToFITS(const planar_image_collection<uint16_t,double> &, const std::string &, YgorEndianness);
    template bool WriteToFITS(const planar_image_collection<uint32_t,double> &, const std::string &, YgorEndianness);
    template bool WriteToFITS(const planar_image_collection<uint64_t,double> &, const std::string &, YgorEndianness);
    template bool WriteToFITS(const planar_image_collection<float   ,double> &, const std::string &, YgorEndianness);
    template bool WriteToFITS(const planar_image_collection<double  ,double> &, const std::string &, YgorEndianness);
#endif

template <class T, class R>
bool WriteToFITS(const std::list<std::reference_wrapper<const planar_image<T,R>>> &imgs_refws, 
                 std::ostream &os,
                 YgorEndianness userE ){

    //Verify we conform to the FITS standard types.
    if(std::is_floating_point<T>::value){
        if(!std::numeric_limits<T>::is_iec559) throw std::runtime_error("Refusing to write non-IEEE754 floating point values");
    }else if(std::is_integral<T>::value){
        if((!std::is_signed<T>::value) && ((sizeof(T)) != 1)){
            throw std::runtime_error("FITS cannot store unsigned integral pixel values when they hold >8 bits."
                                     " You'll need to convert to a supported pixel format, or modify the routine"
                                     " to use the BSCALE and BZERO FITS keys to offset pixel values by half the"
                                     " unsigned max.");
        }else if((sizeof(T)) > 8){
            throw std::runtime_error("FITS cannot store signed integral pixels that are larger than 64 bits.");
        }
    }else{
        throw std::runtime_error("FITS cannot store non-integral, non-floating point types.");
    }

    //Check if we are on a big-endian (i.e., "MSB") or little-endian ("LSB") machine.
    const auto UsingEndian = YgorEndianness::Host;
    const bool UsingLittleEndian = (UsingEndian == YgorEndianness::Little);
    const bool UsingBigEndian = (UsingEndian == YgorEndianness::Big);

    //Determine which endianness to use. The FITS standard way is big endian, but some folks prefer to write
    // as little endian for whatever reason. The default is big endian.
    const bool WriteLittleEndian = (userE == YgorEndianness::Little);
    const bool WriteBigEndian    = (userE == YgorEndianness::Big);
    if( (!WriteLittleEndian && !WriteBigEndian) || (WriteLittleEndian && WriteBigEndian) ){
        throw std::runtime_error("Cannot determine which endianness to write!");
        // NOTE: This routine needs to be modified to handle additional endian-types if the above throws.
    }

    typedef std::array<char,80> card_t; //Each entry in the header.
    typedef std::array<card_t,36> header_t; //The header has 36 cards, and thus 80*36 = 2880 bytes.

    auto pack_into_card = [](card_t &acard, const std::string &in) -> void {
        if(in.size() > acard.size()){
            FUNCWARN("Note: line is: '" << in << "' which has length " << in.size());
            throw std::runtime_error("A single card cannot accommodate this header");
        }
        std::copy(std::begin(in), std::end(in), std::begin(acard));
        return; 
    };

    auto left_pad = [](size_t width, char pad, const std::string &in) -> std::string {
        if(width < in.size()){
            throw std::runtime_error("Unable to pad string because string is already longer than desired width"); 
        }
        return std::string(width - in.size(), pad) + in;
    };
    auto fwnum = [&](const std::string &in) -> std::string {
        // Fixed-width number, right justified in bytes 11 through 30.
        return left_pad(20UL, ' ', in);
    };
    auto escape_string = [](const std::string& in) -> std::string {
        // FITS string type requires wrapping in single quotation marks.
        // Any single quotation marks appearing inside should be escaped by doubling.
        // Note that the longest string we can handle is 68 chars long. Beyond this, the 'long string' convention must
        // be used (used below for metadata).
        std::string out;
        for(const auto& c : in){
            out += c;
            if(c == '\'') out += c;
        }
        return "'" + out + "'";
    };


    // ----

    const auto N_imgs = static_cast<long int>(imgs_refws.size());
    if(N_imgs < 1L){
        // It's valid to create a header-only file, but I'm not sure why it would be needed.
        // It might require a special-case and code duplication below, so I'll disallow it for now.
        throw std::invalid_argument("No images provided, refusing to write empty FITS file");
    }
 
    bool emit_primary_header = true; // Only for the first image.
    for(const auto& img_refw : imgs_refws){
        const auto& img = img_refw.get();

        if((img.rows*img.columns*img.channels) < 0){
            throw std::runtime_error("Attempted to store unitialized or nonsenical image data");
            //NOTE: It is valid in FITS to store zero or more pixels.
        }

        //Add the primary header and additional metadata headers if needed.
        try{
            size_t i = 0UL;
            header_t header;

            auto flush_and_reset = [&]() -> void {
                // If any data has been written, emit the header.
                if(0UL < i){
                    for(const auto &acard : header){
                        for(const auto &achar : acard){
                            os.write( reinterpret_cast<const char *>( &achar ), sizeof(char) );
                        }
                    }
                }

                // Reset the header and card count.
                for(auto & card : header) card.fill(' ');
                i = 0UL;

                return;
            };
            flush_and_reset();

            // Handle single-line and multi-line headers by automatically splitting into pieces when needed.
            // This routine will also flush the card to disk when needed.
            auto pack_multiline = [&](const std::string &key, std::string val) -> void {
               bool first = true;

               // Break into 68 or 67 character chunks (68 or less does not require a CONTINUE, but >68 will require a
               // continue with 67 characters and a trailing '&'). Add each chunk one at a time.
               while(!val.empty()){
                   const std::string keyword = (first) ? key : "CONTINUE  ";

                   const bool guaranteed_to_be_too_long = (68UL < val.size());
                   std::string es = (guaranteed_to_be_too_long) ? "" : escape_string(val);
                   if( !guaranteed_to_be_too_long
                   && (es.size() < 70UL) ){
                       pack_into_card(header.at(i++), keyword + es);
                       val.clear();
                       
                   }else{
                       // Escaping can lengthen a string if there are enclosed single quotation marks, so iteratively
                       // escape until the escaped string will fit in a single FITS card. It should also NOT split the
                       // string on a double-quotation boundary to simplify un-escaping by the reader.
                       auto count = std::min(static_cast<size_t>(67UL), static_cast<size_t>(val.size()));
                       std::string l_es;
                       for( ; 0UL < count; --count){
                           l_es = escape_string(val.substr(0UL,count) + std::string("&"));

                           const auto N_l_es = l_es.size();
                           const bool valid_size = (N_l_es <= 70UL);

                           bool valid_split = true;
                           if( 4UL < N_l_es ){
                               // The following summarizes what '' splits we're checking for here:
                               //     '...X'&' <-- invalid because the '' was cut-off.
                               //     '...''&' <-- valid, '' not cut-off.
                               //     '...'X&' <-- valid, '' not cut-off.
                               //     '...XX&' <-- valid, nothing to cut-off.
                               const bool quote_in_ult = (l_es[N_l_es - 3UL] == '\'');
                               const bool quote_in_penult = (l_es[N_l_es - 4UL] == '\'');
                               if(quote_in_ult){
                                   valid_split = false;
                               }
                               if(quote_in_ult && quote_in_penult){
                                   valid_split = true;
                               }
                           }

                           if(valid_size && valid_split) break;
                       }
                       // Note: is count == 1 then there might be a pathological string. We attempt to emit whatever is in
                       // te escape string buffer to make forward progress.

                       pack_into_card(header.at(i++), keyword + l_es);
                       val = val.substr(count);
                   }

                   first = false;
                   if(i == 36UL) flush_and_reset();
               }
           };

            // NOTE: the order of the following keywords is fixed. Do not rearrange until consulting the specification.
            if(emit_primary_header){
                pack_into_card(header.at(i++), std::string("SIMPLE  = ") + fwnum("T"));
            }else{
                pack_into_card(header.at(i++), std::string("XTENSION= 'IMAGE   '")); // Note: whitespace is significant.
            }

            if(std::is_floating_point<T>::value){
                pack_into_card(header.at(i++), std::string("BITPIX  = ") + fwnum(std::string("-") + std::to_string(8*sizeof(T))) );
            }else{
                pack_into_card(header.at(i++), std::string("BITPIX  = ") + fwnum(std::to_string(8*sizeof(T))) );
            }
            if(img.channels == 1){
                pack_into_card(header.at(i++), std::string("NAXIS   = ") + fwnum("2"));
                pack_into_card(header.at(i++), std::string("NAXIS1  = ") + fwnum(std::to_string(img.columns)));
                pack_into_card(header.at(i++), std::string("NAXIS2  = ") + fwnum(std::to_string(img.rows)));
            }else{
                pack_into_card(header.at(i++), std::string("NAXIS   = ") + fwnum("3"));
                pack_into_card(header.at(i++), std::string("NAXIS1  = ") + fwnum(std::to_string(img.columns)));
                pack_into_card(header.at(i++), std::string("NAXIS2  = ") + fwnum(std::to_string(img.rows)));
                pack_into_card(header.at(i++), std::string("NAXIS3  = ") + fwnum(std::to_string(img.channels)));
            }

            if(!emit_primary_header){
                // Keywords needed to support the IMAGE extension.
                pack_into_card(header.at(i++), std::string("PCOUNT  = ") + fwnum("0")); // # of bytes, for binary blob extensions.
                pack_into_card(header.at(i++), std::string("GCOUNT  = ") + fwnum("1")); // historical (random groups).
            }

            // Emitted file will not contain any trailing headers or extensions.
            if(emit_primary_header){
                const auto extend_str = (N_imgs == 1L) ? std::string("F") : std::string("T");
                pack_into_card(header.at(i++), std::string("EXTEND  = ") + fwnum(extend_str));
            }

            if((img.rows*img.columns*img.channels) > 0){
                pack_into_card(header.at(i++), std::string("BZERO   = ") + fwnum("0.0"));
                pack_into_card(header.at(i++), std::string("BSCALE  = ") + fwnum("1.0"));

                auto minmax = img.minmax();
                pack_into_card(header.at(i++), std::string("DATAMIN = ") + fwnum(std::to_string(static_cast<double>(minmax.first))));
                pack_into_card(header.at(i++), std::string("DATAMAX = ") + fwnum(std::to_string(static_cast<double>(minmax.second))));
            }

            // Warns the reader that the OGIP long string continuation syntax might be used for metadata.
            pack_into_card(header.at(i++), std::string("LONGSTRN= 'OGIP 1.0'"));
            pack_into_card(header.at(i++), std::string("COMMENT   This FITS file may contain long string keyword values that are"));
            pack_into_card(header.at(i++), std::string("COMMENT   continued over multiple keywords. This convention uses the '&'"));
            pack_into_card(header.at(i++), std::string("COMMENT   character at the end of a string which is then continued"));
            pack_into_card(header.at(i++), std::string("COMMENT   on subsequent keywords whose name = 'CONTINUE'."));

            if(WriteLittleEndian){
                pack_into_card(header.at(i++), std::string("BYTEORDR= 'LITTLE_ENDIAN'"));
            }else{
                pack_into_card(header.at(i++), std::string("BYTEORDR= 'BIG_ENDIAN'"));
            }

            pack_into_card(header.at(i++), "YGORPXLX= "_s + std::to_string(img.pxl_dx));
            pack_into_card(header.at(i++), "YGORPXLY= "_s + std::to_string(img.pxl_dy));
            pack_into_card(header.at(i++), "YGORPXLZ= "_s + std::to_string(img.pxl_dz));

            pack_multiline("YGORANKR= "_s, img.anchor.to_string());
            pack_multiline("YGOROFST= "_s, img.offset.to_string());

            pack_multiline("YGORROWU= "_s, img.row_unit.to_string());
            pack_multiline("YGORCOLU= "_s, img.col_unit.to_string());

            pack_multiline("HISTORY   "_s, "Created with libygor (https://github.com/hdclark/Ygor).");

            for(const auto& p : img.metadata){
               // Serialize the key-value pair.
               std::string s = encode_metadata_kv_pair(p);
               pack_multiline("YGORMETA= "_s, s);
            }

            pack_into_card(header.at(i++), std::string("END"));
            flush_and_reset();

        }catch(const std::exception &e){
            FUNCWARN("Encountered error while writing FITS header: '" << e.what() << "'");
            return false;
        }

        // Write pixel data.
        try{
            const auto BytesToWrite = sizeof(T)*img.rows*img.columns*img.channels;
            const auto BytesOver = std::ldiv(BytesToWrite, 2880);
            const auto BytesToPad = ((BytesOver.rem == 0) ? 0 : (2880 - BytesOver.rem));

            for(auto chan = 0; chan < img.channels; ++chan){
                for(auto row = 0; row < img.rows; ++row){
                    for(auto col = 0; col < img.columns; ++col){
                        T val = img.value(img.rows-1-row, col, chan); //FITS files define row zero at bottom left, so flip.
     
                        unsigned char *c = reinterpret_cast<unsigned char *>(&val); //c[0] to c[sizeof(T)-1] only!
                        if( (UsingLittleEndian && WriteBigEndian) || (UsingBigEndian && WriteLittleEndian) ){
                            //Reverse the order of the bytes written to the stream.
                            for(size_t i = 0; i <= (sizeof(T)-1); ++i) os.put(c[sizeof(T)-1-i]);
                        }else if( (UsingLittleEndian && WriteLittleEndian) || (UsingBigEndian && WriteBigEndian) ){
                            //Keep the byte order unaltered.
                            os.write( reinterpret_cast<char *>( &c[0] ), sizeof(T) );
                        }
                    }
                }
            }
            for(long int i = 0; i < BytesToPad; ++i) os.put(static_cast<unsigned char>(0));

        }catch(const std::exception &e){
            FUNCWARN("Encountered error while writing FITS pixel data: '" << e.what() << "'");
            return false;
        }

        emit_primary_header = false;
    }

    os.flush();
    return true;
}
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool WriteToFITS(const std::list<std::reference_wrapper<const planar_image<uint8_t ,double>>> &, std::ostream &, YgorEndianness);
    template bool WriteToFITS(const std::list<std::reference_wrapper<const planar_image<uint16_t,double>>> &, std::ostream &, YgorEndianness);
    template bool WriteToFITS(const std::list<std::reference_wrapper<const planar_image<uint32_t,double>>> &, std::ostream &, YgorEndianness);
    template bool WriteToFITS(const std::list<std::reference_wrapper<const planar_image<uint64_t,double>>> &, std::ostream &, YgorEndianness);
    template bool WriteToFITS(const std::list<std::reference_wrapper<const planar_image<float   ,double>>> &, std::ostream &, YgorEndianness);
    template bool WriteToFITS(const std::list<std::reference_wrapper<const planar_image<double  ,double>>> &, std::ostream &, YgorEndianness);
#endif


//Read pixels and metadata from a FITS formatted file.
template <class T, class R>
planar_image_collection<T,R> 
ReadFromFITS(const std::string &filename, YgorEndianness userE){
    std::ifstream is(filename, std::ios::in | std::ios::binary);
    if(!is){
        throw std::runtime_error("Could not open file.");
    }
    return ReadFromFITS<T,R>(is, userE);
}
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template planar_image_collection<uint8_t ,double> ReadFromFITS(const std::string &, YgorEndianness);
    template planar_image_collection<uint16_t,double> ReadFromFITS(const std::string &, YgorEndianness);
    template planar_image_collection<uint32_t,double> ReadFromFITS(const std::string &, YgorEndianness);
    template planar_image_collection<uint64_t,double> ReadFromFITS(const std::string &, YgorEndianness);
    template planar_image_collection<float   ,double> ReadFromFITS(const std::string &, YgorEndianness);
    template planar_image_collection<double  ,double> ReadFromFITS(const std::string &, YgorEndianness);
#endif

template <class T, class R>
planar_image_collection<T,R> 
ReadFromFITS(std::istream &is, YgorEndianness userE){
    planar_image_collection<T,R> imgcoll;

    //Verify we conform to the FITS standard types.
    if(std::is_floating_point<T>::value){
        if(!std::numeric_limits<T>::is_iec559) throw std::runtime_error("Refusing to read non-IEEE754 floating point values");
    }else if(std::is_integral<T>::value){
        if((!std::is_signed<T>::value) && ((sizeof(T)) != 1)){
            throw std::runtime_error("FITS cannot store unsigned integral pixel values when they hold >8 bits."
                                     " You'll need to convert to a supported pixel format, or modify the routine"
                                     " to use the BSCALE and BZERO FITS keys to offset pixel values by half the" 
                                     " unsigned max.");
        }else if((sizeof(T)) > 8){
            throw std::runtime_error("FITS cannot store signed integral pixels that are larger than 64 bits.");
        }
    }else{
        throw std::runtime_error("FITS cannot store non-integral, non-floating point types.");
    }

    //Check if we are on a big-endian (i.e., "MSB") or little-endian ("LSB") machine.
    const auto UsingEndian = YgorEndianness::Host;
    const bool UsingLittleEndian = (UsingEndian == YgorEndianness::Little);
    const bool UsingBigEndian = (UsingEndian == YgorEndianness::Big);

    //Determine which endianness to use. The FITS standard way is big endian, but some folks prefer to write
    // as little endian for whatever reason. The default is big endian.
    bool ReadLittleEndian = (userE == YgorEndianness::Little);
    bool ReadBigEndian    = (userE == YgorEndianness::Big);
    if( (!ReadLittleEndian && !ReadBigEndian) || (ReadLittleEndian && ReadBigEndian) ){
        throw std::runtime_error("Cannot determine which endianness to read!");
        // NOTE: This routine needs to be modified to handle additional endian-types if the above throws.
    }

    typedef std::array<char,80> card_t; //Each entry in the header.
    typedef std::array<card_t,36> header_t; //The header has 36 cards, and thus 80*36 = 2880 bytes.

    bool primary_header = true;

    while(true){
        imgcoll.images.emplace_back();
        auto& img = imgcoll.images.back();

        //Decipher metadata needed to continue reading. Separate the key and parse the value and then use the value
        // for something.
        std::set<std::string> Encountered;
        std::map<std::string,double> NumericValue; // FITS requires merely 'floating point'.
        std::multimap<std::string,std::string> StringValue;

        std::string previous_key; // Used to support 'CONTINUE' keyword.
        bool previous_was_string = false;
        bool done_reading_header = false;
        long int number_of_headers = 0;
        bool found_magic = false; // SIMPLE or XTENSION keyword are required to appear first.

        // Read in the primary header, the first 2880 bytes, and continue reading header blocks until we encounter
        // the END statement.
        while(!done_reading_header){
            header_t l_header;
            for(auto & acard : l_header) acard.fill(' ');

            for(auto & acard : l_header){
                is.read(acard.data(),acard.size());
                if(static_cast<long long int>(is.gcount()) != static_cast<long long int>(acard.size())){
                    throw std::runtime_error("Not enough data to read full card.");
                }
            }

            for(auto & acard : l_header){
                //Keys can be up to eight chars long and must be composed of only [A-Z] [0-9] [-] or [_] chars.
                auto itA = std::begin(acard); //Will point to the character *after* the key's last character.
                {
                    size_t di = 0;
                    for( ; itA != std::end(acard) ; ++di, ++itA){
                        if(    (di > 7)
                           || !( isininc('A',*itA,'Z')
                                 || isininc('0',*itA,'9')
                                 || (*itA == '-') 
                                 || (*itA == '_') ) ) break;
                    }
                }
                const std::string thekey(std::begin(acard), itA);

                // Check for magic numbers that confirm we're reading a FITS file.
                if(!found_magic){
                    if(primary_header){
                        //The first keyword in the first FITS file is required to be 'SIMPLE', so treat it like a magic number.
                        if(thekey != "SIMPLE") throw std::runtime_error("Missing FITS magic numbers; not a valid FITS file.");
                    }else{
                        //Likewise for extensions -- after the primary header the first keyword should be 'XTENSION'.
                        if(thekey != "XTENSION") throw std::runtime_error("Missing FITS extension magic numbers; not a valid FITS file.");
                    }
                    found_magic = true;
                }

                if(thekey.empty()) continue; //Either empty or not valid. Skip it either way.

                //The remaining chars may be preceeded by zero or more spaces, a '=', and one more space. 
                // Iff this pattern holds, then iterate past them.
                auto itB = itA; //Will point to the character *after* the '= ', or will be == itA if N/A.
                {
                    size_t di = std::distance(std::begin(acard),itB);
                    for( ; itB != std::end(acard) ; ++di, ++itB){
                        if(di > 9) break; //Made to the end OK.
                        if(di == 8){
                            if(*itB != '='){ //Must be a '=' to conform.
                                itB = itA;
                                break;
                            }
                        }else{
                            if(*itB != ' '){ //Must be a ' ' to conform.
                                itB = itA;
                                break;
                            }
                        }
                    }
                }

                //There are also possibly comments preceeded by a '/'. Disregard them if needed.
                auto itC = itB; //Will point to the comment indicator '/' or the end if none are found.
                {
                    bool MidQuote = false;
                    for( ; itC != std::end(acard) ; ++itC){        
                        if(*itC == '\'') MidQuote = !MidQuote;
                        if((*itC == '/') && !MidQuote) break;
                    }
                }
                std::string theval = Canonicalize_String2(std::string(itB, itC), CANONICALIZE::TRIM_ENDS);
                
                // Handle string escapes.
                //
                // Note that strings are necessarily wrapped in single quotation marks and any internal quotation marks are
                // doubled.
                const auto unescape_iff_string = [](std::string in) -> std::string {
                    if( (1UL < in.size())
                    &&  (in.front() == '\'')
                    &&  (in.back() == '\'') ){
                        // Chop off single quotation marks iff present at both beginning and end of a string.
                        in = in.substr(1UL, in.size() - 2UL);

                        // De-duplicate doubled quotation marks, which are how single quotation marks are escaped.
                        size_t p = 0UL;
                        while(true){
                            p = in.find("''", p);
                            if(p == std::string::npos) break;
                            in.erase(p, 1UL); // Erase one quotation mark.
                            ++p; // Skip past the second to avoid accidentally removing it if beside another.
                        }
                    }
                    return in;
                };
                theval = unescape_iff_string(theval);

                // Strip any whitespace inside quotes.
                // This is needed to deal with 'fixed-width' strings, like the XTENSION keyword's value.
                theval = Canonicalize_String2(theval, CANONICALIZE::TRIM_ENDS);

                if(thekey == "SIMPLE"){
                    Encountered.insert(thekey);
                    StringValue.emplace(std::make_pair(thekey, theval)); //Ordering unchanged >=C++11.

                    if(theval != "T") throw std::runtime_error("Not a valid FITS file.");

                    previous_was_string = false;
                    previous_key = thekey;
                
                }else if(thekey == "XTENSION"){
                    Encountered.insert(thekey);
                    StringValue.emplace(std::make_pair(thekey, theval)); //Ordering unchanged >=C++11.

                    if(theval != "IMAGE"){
                        throw std::runtime_error("FITS extension not supported. Currently only 'IMAGE' is supported");
                    }

                    previous_was_string = false;
                    previous_key = thekey;

                }else if( (thekey == "BITPIX")   //Floating-point valued keys.
                      ||  (thekey == "NAXIS")
                      ||  (thekey == "NAXIS1")
                      ||  (thekey == "NAXIS2")
                      ||  (thekey == "NAXIS3")
                      ||  (thekey == "BZERO")
                      ||  (thekey == "BSCALE") 
                      ||  (thekey == "PCOUNT") 
                      ||  (thekey == "GCOUNT")
                      ||  (thekey == "YGORPXLX")
                      ||  (thekey == "YGORPXLY")
                      ||  (thekey == "YGORPXLZ") ){
                    Encountered.insert(thekey);
                    NumericValue[thekey] = std::stod(theval);  

                    previous_was_string = false;
                    previous_key = thekey;

                }else if( (thekey == "LONGSTRN") //Text valued keys, or keys to be transformed later.
                      ||  (thekey == "EXTEND")
                      ||  (thekey == "BYTEORDR")
                      ||  (thekey == "COMMENT")
                      ||  (thekey == "HISTORY")
                      ||  (thekey == "YGORANKR")
                      ||  (thekey == "YGOROFST")
                      ||  (thekey == "YGORROWU")
                      ||  (thekey == "YGORCOLU")
                      ||  (thekey == "YGORMETA") ){
                    Encountered.insert(thekey);
                    StringValue.emplace(std::make_pair(thekey, theval)); //Ordering unchanged >=C++11.

                    previous_was_string = true;
                    previous_key = thekey;

                }else if(thekey == "CONTINUE"){
                    // Continue the most recent keyword's value, to support oversized values.
                    if(previous_key.empty()) throw std::runtime_error("Encountered CONTINUE with no preceding keyword to continue");
                    if(!previous_was_string) throw std::runtime_error("Refusing to CONTINUE a non-string key");

                    // Identify the preceding keyword's content.
                    const auto end = std::end(StringValue);
                    auto next_it = StringValue.upper_bound(previous_key);
                    if(next_it == end) throw std::runtime_error("Encountered CONTINUE keyword without preceding keyword content");
                    auto s_it = std::prev(next_it);
                    if(s_it == end) throw std::logic_error("No preceding keyword content found");

                    // Look for the '&' character in the previous string's content.
                    // It should be present, but if it isn't, then continue with the CONTINUE concatenation.
                    auto pos_ampersand = s_it->second.rfind("&");
                    if(pos_ampersand != std::string::npos){
                        s_it->second.resize( pos_ampersand );
                    }

                    // Add the CONTINUE keyword's contents to the previous contents.
                    s_it->second += theval;

                }else if(thekey == "END"){  //Empty valued keys.
                    Encountered.insert(thekey);

                    previous_was_string = false;
                    previous_key = thekey;

                    done_reading_header = true;
                    break; // Stop processing any more keys after END.

                }else{  //Unrecognized keys.
                    Encountered.insert(thekey);
                    StringValue.emplace(std::make_pair(thekey, theval)); //Ordering unchanged >=C++11.

                    previous_was_string = true;
                    previous_key = thekey;
                }
            }

            primary_header = false;

            // Check if we are stuck in a loop waiting for a missing END keyword.
            if(1000L < number_of_headers){
                throw std::runtime_error("Encountered more than 1000 FITS headers; refusing to continue");
            }
            ++number_of_headers;
        }

        //Get necessary information from the metadata. Prune items not needing to be stored in image metadata.
        auto purge_encounter = [&Encountered, &StringValue, &NumericValue](const std::string &akey) -> void {
            StringValue.erase(akey);
            NumericValue.erase(akey);
            Encountered.erase(akey);
            return;
        };

        auto get_as_str = [&Encountered, &StringValue, &NumericValue](const std::string &akey) -> std::optional<std::string> {
            std::optional<std::string> out;

            if(0UL != Encountered.count(akey)){
                if(auto it = StringValue.lower_bound(akey); (it != std::end(StringValue))) out = it->second;
            }
            return out;
        };

        auto get_as_long = [&Encountered, &StringValue, &NumericValue](const std::string &akey) -> std::optional<long int> {
            std::optional<long int> out;

            if( (0UL != Encountered.count(akey))
            &&  (0UL != NumericValue.count(akey)) ){
                out = static_cast<long int>(std::floor(NumericValue[akey]));
            }
            return out;
        };

        auto get_as_double = [&Encountered, &StringValue, &NumericValue](const std::string &akey) -> std::optional<double> {
            std::optional<double> out;

            if( (0UL != Encountered.count(akey))
            &&  (0UL != NumericValue.count(akey)) ){
                out = static_cast<double>(NumericValue[akey]);
            }
            return out;
        };

        //Check that all necessary keys have been encountered.
        const auto simple_opt = get_as_str("SIMPLE");
        const auto xtension_opt = get_as_str("XTENSION");
        const bool has_end = (Encountered.count("END") != 0UL);

        if(simple_opt){
            // This is a primary header.
            if( (simple_opt.value() != "T")
            ||  !get_as_long("BITPIX")
            ||  !get_as_long("NAXIS")
            ||  !get_as_long("NAXIS1")
            ||  !get_as_long("NAXIS2")
            ||  !has_end ){
                throw std::runtime_error("Primary FITS header is missing necessary information");
            }
            purge_encounter("SIMPLE");

        }else if(xtension_opt){
            // This is an extension header.
            if( xtension_opt.value() != "IMAGE" ){
                throw std::runtime_error("FITS extension not supported. Currently only 'IMAGE' is supported");
            }
            if( !get_as_long("BITPIX")
            ||  !get_as_long("NAXIS")
            ||  !get_as_long("NAXIS1")
            ||  !get_as_long("NAXIS2")
            ||  !get_as_long("PCOUNT")
            ||  !get_as_long("GCOUNT")
            ||  !has_end ){
                throw std::runtime_error("FITS IMAGE extension header is missing necessary information");
            }

            if( (get_as_long("PCOUNT").value() != 0L)
            ||  (get_as_long("GCOUNT").value() != 1L) ){
                throw std::runtime_error("FITS IMAGE extension header is non-conforming");
            }
            purge_encounter("XTENSION");

        }else{
            throw std::runtime_error("Unknown FITS header is missing necessary information");
        }


        const auto bitpix_opt = get_as_long("BITPIX");
        const auto BitsPerPixel = std::abs( bitpix_opt.value() );
        const auto PixelsAreFloatingPoint = ( bitpix_opt.value() < 0 );
        purge_encounter("BITPIX");

        {
            const auto NAxes = get_as_long("NAXIS").value();
            const auto naxis1_opt = get_as_long("NAXIS1");
            const auto naxis2_opt = get_as_long("NAXIS2");
            const auto naxis3_opt = get_as_long("NAXIS3");

            if(NAxes == 2L){
                img.init_buffer(
                    naxis2_opt.value(), // Rows.
                    naxis1_opt.value(), // Columns.
                    1L); // Channels.
            }else if(NAxes == 3L){
                if(!naxis3_opt) throw std::runtime_error("Missing expected FITS key-value 'NAXIS3'");
                img.init_buffer(
                    naxis2_opt.value(),  // Rows.
                    naxis1_opt.value(),  // Columns.
                    naxis3_opt.value()); // Channels.
            }else{
                throw std::runtime_error("Cannot handle a FITS file with the number of axes specified.");
            }
            purge_encounter("NAXIS");
            purge_encounter("NAXIS1");
            purge_encounter("NAXIS2");
            purge_encounter("NAXIS3");
        }    

        {
            const auto ygorpxlx_opt = get_as_double("YGORPXLX");
            const auto ygorpxly_opt = get_as_double("YGORPXLY");
            const auto ygorpxlz_opt = get_as_double("YGORPXLZ");

            const auto ygorankr_opt = get_as_str("YGORANKR");
            const auto ygorofst_opt = get_as_str("YGOROFST");

            if( !ygorpxlx_opt || !ygorpxly_opt || !ygorpxlz_opt
            ||  !ygorankr_opt || !ygorofst_opt ){
                throw std::runtime_error("Custom Ygor spatial information incomplete or missing. Cannot reconstitute image");
            }
            img.init_spatial( ygorpxlx_opt.value(),
                              ygorpxly_opt.value(),
                              ygorpxlz_opt.value(),
                              vec3<R>().from_string(ygorankr_opt.value()),
                              vec3<R>().from_string(ygorofst_opt.value()) );

            purge_encounter("YGORPXLX");
            purge_encounter("YGORPXLY");
            purge_encounter("YGORPXLZ");
            purge_encounter("YGORANKR");
            purge_encounter("YGOROFST");
        }

        {
            const auto ygorrowu_opt = get_as_str("YGORROWU");
            const auto ygorcolu_opt = get_as_str("YGORCOLU");
            if(!ygorrowu_opt || !ygorcolu_opt){
                throw std::runtime_error("Custom Ygor orientation information incomplete or missing. Cannot reconstitute image");
            }
            img.init_orientation( vec3<R>().from_string(ygorrowu_opt.value()),
                                  vec3<R>().from_string(ygorcolu_opt.value()) );
            purge_encounter("YGORROWU");
            purge_encounter("YGORCOLU");
        }

        const auto extend_opt = get_as_str("EXTEND");
        const bool stream_might_contain_extensions = (extend_opt) ? (extend_opt.value() != "F") : true;
        purge_encounter("EXTEND");

        const auto BZero = get_as_double("BZERO").value_or(0.0);
        const auto BScale = get_as_double("BSCALE").value_or(1.0);
        purge_encounter("BZERO");
        purge_encounter("BSCALE");

        const auto byteordr_opt = get_as_str("BYTEORDR");
        if(byteordr_opt){
            if(byteordr_opt.value() == "LITTLE_ENDIAN"){
                ReadLittleEndian = true;
                ReadBigEndian = false;
            }else if(byteordr_opt.value() == "BIG_ENDIAN"){
                ReadLittleEndian = false;
                ReadBigEndian = true;
            }else{
                throw std::runtime_error("Endianess specified in FITS file is not recognized.");
            }
            purge_encounter("BYTEORDR");
        }

        purge_encounter("DATAMIN");  // Could be used for scaling and windowing, but ... meh.
        purge_encounter("DATAMAX");

        //Verify that the metadata is (1) reasonable, and (2) we can use it to create a ygor image.
        if( !(    (BitsPerPixel == 8 ) || (BitsPerPixel == 16) 
               || (BitsPerPixel == 32) || (BitsPerPixel == 64) ) ){
            throw std::runtime_error("Encountered a non-standard bit size.");
        }
        if( PixelsAreFloatingPoint && !( (BitsPerPixel == 32) || (BitsPerPixel == 64) ) ){
            throw std::runtime_error("Only 32 and 64 bit floating point types are recognized by this reader");
        }
        if( (sizeof(T)*8) != BitsPerPixel ){
            throw std::runtime_error("Number of bits in FITS pixels and type T do not match.");
        }
        if( PixelsAreFloatingPoint != std::is_floating_point<T>::value ){
            throw std::runtime_error("FITS pixels and type T pixels do not share floating point status.");
        }
        if(   (img.rows < 1) || (img.columns < 1) || (img.channels < 1)
           || (img.rows*img.columns*img.channels*sizeof(T) > 100'000'000'000) ){ //Arbitrary 100GB limit...
            throw std::runtime_error("FITS file image dimensions are suspect. Refusing to continue");
        }

        //Extract image metadata key-value pairs.
        {
            //const auto end = std::end(StringValue);
            auto it_range = StringValue.equal_range("YGORMETA");
            for(auto p_it = it_range.first; p_it != it_range.second; ++p_it){
                auto kvp = decode_metadata_kv_pair( p_it->second );
                if(kvp){
                    img.metadata[kvp.value().first] = kvp.value().second;
                }
            }
        }

        //Read in pixel data.
        const auto BytesToRead = sizeof(T)*img.rows*img.columns*img.channels;
        const auto BytesOver = std::ldiv(BytesToRead, 2880);
        const auto BytesOfPad = ((BytesOver.rem == 0) ? 0 : (2880 - BytesOver.rem));

        for(auto chan = 0; chan < img.channels; ++chan){
            for(auto row = 0; row < img.rows; ++row){
                for(auto col = 0; col < img.columns; ++col){
                    T val;
                    uint8_t *c = reinterpret_cast<uint8_t *>(&val); //c[0] to c[sizeof(T)-1] only!

                    if( (UsingLittleEndian && ReadBigEndian) || (UsingBigEndian && ReadLittleEndian) ){
                        //Reverse the order of the bytes read from the stream.
                        for(size_t i = 0; i <= (sizeof(T)-1); ++i){
                            is.read( reinterpret_cast<char *>( &c[sizeof(T)-1-i] ), 1);
                        }
                    }else if( (UsingLittleEndian && ReadLittleEndian) || (UsingBigEndian && ReadBigEndian) ){
                        //Keep the byte order unaltered.
                        is.read( reinterpret_cast<char *>( &c[0] ), sizeof(T) );
                    }

                    const auto scaled = static_cast<T>(BZero + BScale * val);
                    img.reference(img.rows-1-row, col, chan) = scaled; //FITS files define row zero at bottom left, so flip.
                }
            }
        }
        for(long int i = 0; i < BytesOfPad; ++i) is.get();


        // The stream might now be exhausted, which would be perfect. However, it's possible that one or more extensions follow.
        // See the FITSv4.0 specification, section 7. The magic header for such headers is 'XTENSION'.
        if(!stream_might_contain_extensions) break;

        {
            // If there is ANY data remaining, and that the first char is 'X', I think it's safe to assume there is an
            // extension present. At least, it seems worthwhile to attempt to parse an extension.
            const char c = is.peek();
            if(c != 'X') break;
        }
    }

    return imgcoll;
}
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template planar_image_collection<uint8_t ,double> ReadFromFITS(std::istream &, YgorEndianness);
    template planar_image_collection<uint16_t,double> ReadFromFITS(std::istream &, YgorEndianness);
    template planar_image_collection<uint32_t,double> ReadFromFITS(std::istream &, YgorEndianness);
    template planar_image_collection<uint64_t,double> ReadFromFITS(std::istream &, YgorEndianness);
    template planar_image_collection<float   ,double> ReadFromFITS(std::istream &, YgorEndianness);
    template planar_image_collection<double  ,double> ReadFromFITS(std::istream &, YgorEndianness);
#endif

