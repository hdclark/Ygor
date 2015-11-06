//YgorImagesIO.cc - Routines for writing and reading images.
//
#include <iostream>
#include <memory>
#include <algorithm>
#include <list>
#include <vector>
#include <functional>
#include <fstream>
#include <type_traits>
#include <cinttypes>
#include <experimental/optional>
#include <experimental/any>

#include "YgorMisc.h"
//#include "YgorMath.h"
//#include "YgorStats.h"    //For Stats::Mean().
//#include "YgorPlot.h"

#include "YgorString.h"
#include "YgorImages.h"
#include "YgorImagesIO.h"


#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    #define YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
#endif


/*
//Dump raw pixel data to a file.
//
// NOTE: This data can be converted to whatever using ImageMagick ala:
//       convert -size 256x256 -depth `sizeof(T)` -define quantum:format=double -type grayscale \
//                input.gray   -depth 16 ... out.jpg
//       where the number of channels, pixel size, etc.. are recorded in other places (no header).
//       At the time of writing, this does not work when T = uint32_t, and no info is produced as 
//       to why. It seems the ImageMagick QuantumDepth might be compiled-in too low, but it is not
//       itself smart enough to realize there is an issue ... even though you are specifing the
//       number of bits directly ...
//
//       For floating point numbers (or floating point numbers packed into uint32_t), use 
//       quantum:format=floating-point.
//
// NOTE: ImageMagick annoyingly seems to silently choke on uint32_t per channel, producing no 
//       output and reporting no errors. Be aware that you will have to find another converter, or
//       will later have to do it yourself. The appeal of writing in that format would probably only
//       be to make saving to, and later loading from, raw files easier.
template <class T,class R> bool planar_image<T,R>::Dump_Pixels(const std::string &filename) const {

    //Check if it exists. If it does, we will refuse to overwrite it.
    {
        std::ifstream FI(filename);
        if(FI.good()) return false;
    }

    //Now try write the file. There is a race condition that could result in an overwrite, but I'm not
    // sure how I could deal with this.
    const auto num_of_T = this->rows * this->columns * this->channels;
    const auto num_of_bytes = sizeof(T) * num_of_T;
    std::ofstream FO(filename, std::ios::out | std::ios::binary);
    FO.write((char *)(this->data.get()), static_cast<std::streamsize>(num_of_bytes));

    //Check if it was successful, clean up, and carry on.
    if(!FO.good()) return false;
    FO.close();
    return true;
}
#ifdef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::Dump_Pixels(const std::string &filename) const;
    template bool planar_image<uint16_t,double>::Dump_Pixels(const std::string &filename) const;
    template bool planar_image<uint32_t,double>::Dump_Pixels(const std::string &filename) const;
    template bool planar_image<uint64_t,double>::Dump_Pixels(const std::string &filename) const;
    template bool planar_image<float   ,double>::Dump_Pixels(const std::string &filename) const;
#endif

//Dump pixel data statically-cast to doubles to a file.
//
// NOTE: This data can be converted to whatever using ImageMagick ala:
//       convert -size 256x256 -depth 64 -define quantum:format=double -type grayscale \
//                input.gray   -depth 16 ... out.jpg
//       where the number of channels, pixel size, etc.. are recorded in other places (no header).
//       At the time of writing, this does not seem to work, and no info is produced as to why...
//
// NOTE: ImageMagick annoyingly seems to silently choke on uint32_t per channel, producing no 
//       output and reporting no errors. Be aware that you will have to find another converter, or
//       will later have to do it yourself. The appeal of writing in that format would probably only
//       be to make saving to, and later loading from, raw files easier.
template <class T,class R> bool planar_image<T,R>::Dump_d64_Pixels(const std::string &filename) const {

    //Check if it exists. If it does, we will refuse to overwrite it.
    {
        std::ifstream FI(filename);
        if(FI.good()) return false;
    }

    //Now try write the file. There is a race condition that could result in an overwrite, but I'm not
    // sure how I could deal with this.
    std::ofstream FO(filename, std::ios::out | std::ios::binary);

    for(auto row = 0; row < this->rows; ++row){
        for(auto col = 0; col < this->columns; ++col){
            for(auto chan = 0; chan < this->channels; ++chan){
                auto val = static_cast<double>(this->value(row, col, chan));
                FO.write(reinterpret_cast<const char*>(&val), sizeof(val));
            }
        }
    }

    //Check if it was successful, clean up, and carry on.
    if(!FO.good()) return false;
    FO.close();
    return true;
}
#ifdef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::Dump_d64_Pixels(const std::string &filename) const;
    template bool planar_image<uint16_t,double>::Dump_d64_Pixels(const std::string &filename) const;
    template bool planar_image<uint32_t,double>::Dump_d64_Pixels(const std::string &filename) const;
    template bool planar_image<uint64_t,double>::Dump_d64_Pixels(const std::string &filename) const;
    template bool planar_image<float   ,double>::Dump_d64_Pixels(const std::string &filename) const;
#endif

//Dump pixel data scaled to maximally fill the uint16_t range (per channel) to a file without a header.
//
// NOTE: This data can be converted to whatever using ImageMagick ala:
//       convert -size 256x256 -depth 16 -define quantum:format=unsigned -type grayscale \
//                input.gray   -depth 16 ... out.jpg
//       where the number of channels, pixel size, etc.. are recorded in other places (no header).
//
// NOTE: ImageMagick annoyingly seems to silently choke on uint32_t per channel, producing no 
//       output and reporting no errors. Be aware that you will have to find another converter, or
//       will later have to do it yourself. The appeal of writing in that format would probably only
//       be to make saving to, and later loading from, raw files easier.
template <class T,class R> bool planar_image<T,R>::Dump_u16_scale_Pixels(const std::string &filename, bool AutoScaleToFillRange) const {

    //Check if it exists. If it does, we will refuse to overwrite it.
    {
        std::ifstream FI(filename);
        if(FI.good()) return false;
    }

    //Now try write the file. There is a race condition that could result in an overwrite, but I'm not
    // sure how I could deal with this.
    std::ofstream FO(filename, std::ios::out | std::ios::binary);

    //Find the minimum and maximum pixel intensities.
    std::vector<T> min(this->channels, std::numeric_limits<T>::max());
    std::vector<T> max(this->channels, std::numeric_limits<T>::min());
 
    if(AutoScaleToFillRange){
        for(auto row = 0; row < this->rows; ++row){
            for(auto col = 0; col < this->columns; ++col){
                for(auto chan = 0; chan < this->channels; ++chan){
                    const auto val = this->value(row, col, chan);
                    if(min[chan] > val) min[chan] = val;
                    if(max[chan] < val) max[chan] = val;
                }
            }
        }

        //Ensure that there is at least one non-zero pixel for each case. If not, just reset.
        for(auto chan = 0; chan < this->channels; ++chan){
            if(min[chan] == std::numeric_limits<T>::max()) min[chan] = std::numeric_limits<T>::min();
            if(max[chan] == std::numeric_limits<T>::min()) max[chan] = std::numeric_limits<T>::max();
        }
    }else{
        //Just fill the min and max with the theoretical min and max.
        for(auto chan = 0; chan < this->channels; ++chan){
            min[chan] = std::numeric_limits<T>::min();
            max[chan] = std::numeric_limits<T>::max();
        }
    }

    //Now walk over the pixels, scaling to maximally fill the range of u16 as we go.
    for(auto row = 0; row < this->rows; ++row){
        for(auto col = 0; col < this->columns; ++col){
            for(auto chan = 0; chan < this->channels; ++chan){
                const auto val = this->value(row, col, chan);
                const auto clamped_num = static_cast<double>(val - min[chan]);
                const auto clamped_den = static_cast<double>(max[chan] - min[chan]);
                const auto newmax = static_cast<double>(std::numeric_limits<uint16_t>::max());
                const auto scaled = static_cast<uint16_t>( newmax * clamped_num / clamped_den );
                FO.write(reinterpret_cast<const char*>(&scaled), sizeof(scaled));
            }
        }
    }

    //Check if it was successful, clean up, and carry on.
    if(!FO.good()) return false;
    FO.close();
    return true;
}
#ifdef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::Dump_u16_scale_Pixels(const std::string &filename, bool ) const;
    template bool planar_image<uint16_t,double>::Dump_u16_scale_Pixels(const std::string &filename, bool ) const;
    template bool planar_image<uint32_t,double>::Dump_u16_scale_Pixels(const std::string &filename, bool ) const;
    template bool planar_image<uint64_t,double>::Dump_u16_scale_Pixels(const std::string &filename, bool ) const;
    template bool planar_image<float   ,double>::Dump_u16_scale_Pixels(const std::string &filename, bool ) const;
#endif
*/


//Write pixels and metadata to a FITS formatted file.
//
// NOTE: You can easily examine such files like so: `fold -w 80 theimage.fit | less`
template <class T, class R>
bool WriteToFITS(const planar_image<T,R> &img, const std::string &filename, YgorImageIOEndianness userE){

    if((img.rows*img.columns*img.channels) < 0){
        throw std::runtime_error("Attempted to store unitialized or nonsenical image data");
        //NOTE: It is valid in FITS to store zero or more pixels.
    }

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
    volatile uint32_t EndianScape = static_cast<uint32_t>(1);
    volatile uint8_t *EndianCheck = reinterpret_cast<volatile uint8_t *>(&EndianScape);
    const bool UsingLittleEndian = (EndianCheck[0] == static_cast<uint8_t>(1)); // "LSB".
    const bool UsingBigEndian = (EndianCheck[sizeof(uint32_t)-1] == static_cast<uint8_t>(1)); // "MSB".
    if( (!UsingLittleEndian && !UsingBigEndian) || (UsingLittleEndian && UsingBigEndian) ){
        throw std::runtime_error("Cannot determine machine's endianness!");
        // NOTE: This portion could be controlled with a compile-time macro, using Boost.Endian, or using
        //       platform-specific headers/libraries.
    }

    //Determine which endianness to use. The FITS standard way is big endian, but some folks prefer to write
    // as little endian for whatever reason. The default is big endian.
    const bool WriteLittleEndian = (userE == YgorImageIOEndianness::Little);
    const bool WriteBigEndian    = (  (userE == YgorImageIOEndianness::Big)
                                   || (userE == YgorImageIOEndianness::Default) );
    if( (!WriteLittleEndian && !WriteBigEndian) || (WriteLittleEndian && WriteBigEndian) ){
        throw std::runtime_error("Cannot determine which endianness to write!");
        // NOTE: This routine needs to be modified to handle additional endian-types if the above throws.
    }


    //Write the header for the primary HDU (header-data unit). 
    typedef std::array<char,80> card_t; //Each entry in the header.
    typedef std::array<card_t,36> header_t; //The header has 36 cards, and thus 80*36 = 2880 bytes.
 
    auto pack_into_card = [](card_t &acard, const std::string &in) -> void {
        if(in.size() > acard.size()) throw std::runtime_error("A single card cannot accommodate this header");
        std::copy(std::begin(in), std::end(in), std::begin(acard));
        return; 
    };


    std::ofstream FO(filename, std::fstream::out);
    if(!FO.good()) return false;

    //Add the primary header.
    try{
        size_t i = 0;
        header_t primary_header;
        for(auto & card : primary_header) card.fill(' ');

        pack_into_card(primary_header.at(i++), std::string("SIMPLE  = T"));
        if(std::is_floating_point<T>::value){
            pack_into_card(primary_header.at(i++), std::string("BITPIX  = -") + std::to_string(8*sizeof(T)));
        }else{
            pack_into_card(primary_header.at(i++), std::string("BITPIX  = ") + std::to_string(8*sizeof(T)));
        }
        if(img.channels == 1){
            pack_into_card(primary_header.at(i++), std::string("NAXIS   = 2"));
            pack_into_card(primary_header.at(i++), std::string("NAXIS1  = ") + std::to_string(img.columns));
            pack_into_card(primary_header.at(i++), std::string("NAXIS2  = ") + std::to_string(img.rows));
        }else{
            pack_into_card(primary_header.at(i++), std::string("NAXIS   = 3"));
            pack_into_card(primary_header.at(i++), std::string("NAXIS1  = ") + std::to_string(img.columns));
            pack_into_card(primary_header.at(i++), std::string("NAXIS2  = ") + std::to_string(img.rows));
            pack_into_card(primary_header.at(i++), std::string("NAXIS3  = ") + std::to_string(img.channels));
        }

        if((img.rows*img.columns*img.channels) > 0){
            pack_into_card(primary_header.at(i++), std::string("BZERO   = 0.0"));
            pack_into_card(primary_header.at(i++), std::string("BSCALE  = 1.0"));

            auto minmax = img.minmax();
            pack_into_card(primary_header.at(i++), std::string("DATAMIN = ") + std::to_string(static_cast<double>(minmax.first)));
            pack_into_card(primary_header.at(i++), std::string("DATAMAX = ") + std::to_string(static_cast<double>(minmax.second)));
        }

        if(WriteLittleEndian){
            pack_into_card(primary_header.at(i++), std::string("BYTEORDR= LITTLE_ENDIAN"));
        }else{
            pack_into_card(primary_header.at(i++), std::string("BYTEORDR= BIG_ENDIAN"));
        }

        pack_into_card(primary_header.at(i++), std::string("YGORPXLX= ") + std::to_string(img.pxl_dx));
        pack_into_card(primary_header.at(i++), std::string("YGORPXLY= ") + std::to_string(img.pxl_dy));
        pack_into_card(primary_header.at(i++), std::string("YGORPXLZ= ") + std::to_string(img.pxl_dz));

        pack_into_card(primary_header.at(i++), std::string("YGORANKR= ") + img.anchor.to_string());
        pack_into_card(primary_header.at(i++), std::string("YGOROFST= ") + img.offset.to_string());

        pack_into_card(primary_header.at(i++), std::string("YGORROWU= ") + img.row_unit.to_string());
        pack_into_card(primary_header.at(i++), std::string("YGORCOLU= ") + img.col_unit.to_string());

        pack_into_card(primary_header.at(i++), std::string("HISTORY This file was created with libygor's YgorImages"));
        //pack_into_card(primary_header.at(i++), std::string("COMMENT ..."));

        pack_into_card(primary_header.at(i++), std::string("END"));

        for(const auto &acard : primary_header){
            for(const auto &achar : acard){
                FO << achar;
            }
        }
    }catch(const std::exception &e){
        return false;
    }

    //Add the image data.
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
                        for(size_t i = 0; i <= (sizeof(T)-1); ++i) FO.put(c[sizeof(T)-1-i]);
                    }else if( (UsingLittleEndian && WriteLittleEndian) || (UsingBigEndian && WriteBigEndian) ){
                        //Keep the byte order unaltered.
                        for(size_t i = 0; i <= (sizeof(T)-1); ++i) FO.put(c[i]);
                    }
                }
            }
        }
        for(long int i = 0; i < BytesToPad; ++i) FO.put(static_cast<unsigned char>(0));

    }catch(const std::exception &e){
        return false;
    }

    FO.close();
    return true;
}
#ifdef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool WriteToFITS(const planar_image<uint8_t ,double> &, const std::string &, YgorImageIOEndianness);
    template bool WriteToFITS(const planar_image<uint16_t,double> &, const std::string &, YgorImageIOEndianness);
    template bool WriteToFITS(const planar_image<uint32_t,double> &, const std::string &, YgorImageIOEndianness);
    template bool WriteToFITS(const planar_image<uint64_t,double> &, const std::string &, YgorImageIOEndianness);
    template bool WriteToFITS(const planar_image<float   ,double> &, const std::string &, YgorImageIOEndianness);
#endif


//Read pixels and metadata from a FITS formatted file.
template <class T, class R>
planar_image<T,R> 
ReadFromFITS(const std::string &filename, YgorImageIOEndianness userE){

    planar_image<T,R> img;

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
    volatile uint32_t EndianScape = static_cast<uint32_t>(1);
    volatile uint8_t *EndianCheck = reinterpret_cast<volatile uint8_t *>(&EndianScape);
    const bool UsingLittleEndian = (EndianCheck[0] == static_cast<uint8_t>(1)); // "LSB".
    const bool UsingBigEndian = (EndianCheck[sizeof(uint32_t)-1] == static_cast<uint8_t>(1)); // "MSB".
    if( (!UsingLittleEndian && !UsingBigEndian) || (UsingLittleEndian && UsingBigEndian) ){
        throw std::runtime_error("Cannot determine machine's endianness!");
        // NOTE: This portion could be controlled with a compile-time macro, using Boost.Endian, or using
        //       platform-specific headers/libraries.
    }

    //Determine which endianness to use. The FITS standard way is big endian, but some folks prefer to write
    // as little endian for whatever reason. The default is big endian.
    bool ReadLittleEndian = (userE == YgorImageIOEndianness::Little);
    bool ReadBigEndian    = (   (userE == YgorImageIOEndianness::Big)
                                   || (userE == YgorImageIOEndianness::Default) );
    if( (!ReadLittleEndian && !ReadBigEndian) || (ReadLittleEndian && ReadBigEndian) ){
        throw std::runtime_error("Cannot determine which endianness to read!");
        // NOTE: This routine needs to be modified to handle additional endian-types if the above throws.
    }

    typedef std::array<char,80> card_t; //Each entry in the header.
    typedef std::array<card_t,36> header_t; //The header has 36 cards, and thus 80*36 = 2880 bytes.
 
    //Start reading the file. It must have a length that is a multiple of 2880 bytes to be valid.
    std::ifstream FI(filename, std::ifstream::in);
    if(!FI){
        throw std::runtime_error("Could not open file.");
    }
    FI.seekg(0, FI.end);
    const auto FileLength = static_cast<intmax_t>(FI.tellg());
    FI.seekg(0, FI.beg);

    if( (FileLength < static_cast<intmax_t>(2880))
    ||  (std::imaxdiv(FileLength, static_cast<intmax_t>(2880)).rem != 0) ){
        throw std::runtime_error("File is not a valid FITS file. The length is not a multiple of 2880 bytes.");
    }

    //Read in the primary header; the first 2880 bytes.
    header_t primary_header;
    for(auto & acard : primary_header) acard.fill(' ');

    for(auto & acard : primary_header){
        FI.read(acard.data(),acard.size());
        if(FI.gcount() != acard.size()) throw std::runtime_error("Not enough data to read full card.");
    }

    //Decipher metadata needed to continue reading. Separate the key and parse the value and then use the value
    // for something.
    std::set<std::string> Encountered;
    std::map<std::string,double> NumericValue; // FITS requires merely 'floating point'.
    std::multimap<std::string,std::string> StringValue;

    for(auto & acard : primary_header){
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
        const std::string theval = Canonicalize_String2(std::string(itB, itC), CANONICALIZE::TRIM_ENDS);
        // NOTE: the value might still contain quotation marks

        if(thekey == "SIMPLE"){
            Encountered.insert(thekey);
            if(theval != "T") throw std::runtime_error("Not a valid FITS file.");

        }else if( (thekey == "BITPIX")   //Floating-point valued keys.
              ||  (thekey == "NAXIS")
              ||  (thekey == "NAXIS1")
              ||  (thekey == "NAXIS2")
              ||  (thekey == "NAXIS3")
              ||  (thekey == "BZERO")
              ||  (thekey == "BSCALE") 
              ||  (thekey == "YGORPXLX")
              ||  (thekey == "YGORPXLY")
              ||  (thekey == "YGORPXLZ") ){
            Encountered.insert(thekey);
            NumericValue[thekey] = std::stod(theval);  

        }else if( (thekey == "BYTEORDR") //Text valued keys, or keys to be transformed later.
              ||  (thekey == "COMMENT")
              ||  (thekey == "HISTORY")
              ||  (thekey == "YGORANKR")
              ||  (thekey == "YGOROFST")
              ||  (thekey == "YGORROWU")
              ||  (thekey == "YGORCOLU") ){
            Encountered.insert(thekey);
            StringValue.emplace(std::make_pair(thekey, theval)); //Ordering unchanged >=C++11.

        }else if(thekey == "END"){  //Empty valued keys.
            Encountered.insert(thekey);

        }else{  //Unrecognized keys.
            Encountered.insert(thekey);
            StringValue.emplace(std::make_pair(thekey, theval)); //Ordering unchanged >=C++11.
        }
    }

    //Check that all necessary keys have been encountered.
    {
        const auto count = (  Encountered.count("SIMPLE")
                            + Encountered.count("BITPIX")
                            + Encountered.count("NAXIS")
                            + Encountered.count("NAXIS1") //Ygor: NAXIS{1,2} required, NAXIS3 optional.
                            + Encountered.count("NAXIS2") );
        if(count != 5){
            throw std::runtime_error("Primary header is missing information. Cannot read image");
        }
    }

    //Get necessary information from the metadata. Prune items not needing to be stored in image metadata.
    auto purge_encounter = [&Encountered, &StringValue, &NumericValue](const std::string &akey) -> void {
                               StringValue.erase(akey);
                               NumericValue.erase(akey);
                               Encountered.erase(akey);
                               return;
                           };

    purge_encounter("SIMPLE");

    const auto BitsPerPixel = static_cast<int>(std::floor( std::abs( NumericValue["BITPIX"] ) ) );
    const auto PixelsAreFloatingPoint = (NumericValue["BITPIX"] < 0);
    purge_encounter("BITPIX");

    {
        const auto NAxes = static_cast<long int>(std::floor(NumericValue["NAXIS"]));
        if(NAxes == 2){
            img.init_buffer(
                 static_cast<long int>(std::floor(NumericValue["NAXIS2"])), //Rows.
                 static_cast<long int>(std::floor(NumericValue["NAXIS1"])),1); //Columns, Channels..
        }else if(NAxes == 3){
            if(1 != Encountered.count("NAXIS3")){
                throw std::runtime_error("Missing FITS key-value 'NAXIS3'.");
            }
            img.init_buffer(
                 static_cast<long int>(std::floor(NumericValue["NAXIS2"])), //Rows.
                 static_cast<long int>(std::floor(NumericValue["NAXIS1"])), //Columns.
                 static_cast<long int>(std::floor(NumericValue["NAXIS3"]))); //Channels.
        }else{
            throw std::runtime_error("Cannot handle a FITS file with the number of axes specified.");
        }
        purge_encounter("NAXIS");
        purge_encounter("NAXIS1");
        purge_encounter("NAXIS2");
        purge_encounter("NAXIS3");
    }    

    {
        const auto count = (  Encountered.count("YGORPXLX") 
                            + Encountered.count("YGORPXLY")
                            + Encountered.count("YGORPXLZ")
                            + Encountered.count("YGORANKR")
                            + Encountered.count("YGOROFST") );
        if(count == 5){
            img.init_spatial( static_cast<R>(NumericValue["YGORPXLX"]),
                              static_cast<R>(NumericValue["YGORPXLY"]), 
                              static_cast<R>(NumericValue["YGORPXLZ"]),
                              vec3<R>().from_string(StringValue.lower_bound("YGORANKR")->second),
                              vec3<R>().from_string(StringValue.lower_bound("YGOROFST")->second) );
        }else if(count != 0){
            throw std::runtime_error("Ygor spatial information only partially specified. Cannot reconstitute image");
        }
        purge_encounter("YGORPXLX");
        purge_encounter("YGORPXLY");
        purge_encounter("YGORPXLZ");
        purge_encounter("YGORANKR");
        purge_encounter("YGOROFST");
    }

    {
        const auto count = (  Encountered.count("YGORROWU") 
                            + Encountered.count("YGORCOLU") );
        if(count == 2){
            img.init_orientation( vec3<R>().from_string(StringValue.lower_bound("YGORROWU")->second),
                                  vec3<R>().from_string(StringValue.lower_bound("YGORCOLU")->second) );
        }else if(count != 0){
            throw std::runtime_error("Ygor orientation information only partially specified. Cannot reconstitute image");
        }
        purge_encounter("YGORROWU");
        purge_encounter("YGORCOLU");
    }

    const auto BZero = NumericValue["BZERO"];
    const auto BScale = NumericValue["BSCALE"];
    purge_encounter("BZERO");
    purge_encounter("BSCALE");

    if(1 == Encountered.count("BYTEORDR")){
        const auto lkey = "BYTEORDR";
        auto it = StringValue.lower_bound(lkey);
        if(it == std::end(StringValue)) throw std::runtime_error("Programming error: missing string.");

        if(it->second == "LITTLE_ENDIAN"){
            ReadLittleEndian = true;
            ReadBigEndian = false;
        }else if(it->second == "BIG_ENDIAN"){
            ReadLittleEndian = false;
            ReadBigEndian = true;
        }else{
            throw std::runtime_error("Endianess specified in FITS file is not recognized.");
        }
        purge_encounter(lkey);
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
                        //FI.get(c[sizeof(T)-1-i]);
                        FI.read( reinterpret_cast<char *>( &c[sizeof(T)-1-i] ), 1);
                    }
                }else if( (UsingLittleEndian && ReadLittleEndian) || (UsingBigEndian && ReadBigEndian) ){
                    //Keep the byte order unaltered.
                    FI.read( reinterpret_cast<char *>( &c[0] ), sizeof(T) );
                    //for(size_t i = 0; i <= (sizeof(T)-1); ++i){
                    //    FI.get(c[i]);
                }

                const auto scaled = static_cast<T>(BZero + BScale * val);
                img.reference(img.rows-1-row, col, chan) = scaled; //FITS files define row zero at bottom left, so flip.
            }
        }
    }
    for(long int i = 0; i < BytesOfPad; ++i) FI.get();

    //Check if there is another HDU to read.
    if((BytesToRead + BytesOfPad + 2880) != FileLength){
         throw std::runtime_error("There is additional data to be read. This is not yet supported"); 
    }

    return img;
}
#ifdef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template planar_image<uint8_t ,double> ReadFromFITS(const std::string &, YgorImageIOEndianness);
    template planar_image<uint16_t,double> ReadFromFITS(const std::string &, YgorImageIOEndianness);
    template planar_image<uint32_t,double> ReadFromFITS(const std::string &, YgorImageIOEndianness);
    template planar_image<uint64_t,double> ReadFromFITS(const std::string &, YgorImageIOEndianness);
    template planar_image<float   ,double> ReadFromFITS(const std::string &, YgorImageIOEndianness);
#endif


/*

// === 20151001 ===
// Is this still needed / desired? I never made much progress on it. What is the scope? samples_1Ds? Contours? planar_images? ...
//
// It would probably be better to just write a one-off 'write_to_postscript' function as needed.
// I don't think reading arbitrary postscript files is going to be necessary.

//---------------------------------------------------------------------------------------------------------------------------
//--------------------------- postscriptinator: a thin class for generating Postscript images -------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class generates Postscript files from simple data. It was developed to convert the workflow 
// R^3 -> OpenGL Bitmap -> R^3 (using potrace or similar) into just R^3.

template <class R> class postscriptinator {
    R xmin, xmax, ymin, ymax;  //Drawing coordinates.
    R PageW, PageH;            //Page geometry (in [cm]).
    bool Enable_Auto_Sizing;   //Default is yes. Will be flipped to no if size explicitly set by user.
    std::string Definitions;
    std::string Header; //Note: Does NOT hold the '%!PS' at the top!
    std::list<std::string> Stack; //Holds postscript commands. May morph into something more easily parseable (true stack)...
    std::string Footer;

    //Constructor.
    postscriptinator();

    //Methods. 
    void Import_Contour(const contour_of_points<R> &C, const vec3<R> &Proj2);


    std::string Generate_Page_Geom(void) const;
    std::string Assemble(void) const; //Assembles all the pieces into a single string.
};


//---------------------------------------------------------------------------------------------------------------------------
//--------------------------- postscriptinator: a thin class for generating Postscript images -------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class generates Postscript files from simple data. It was developed to convert the workflow 
// R^3 -> OpenGL Bitmap -> R^3 (using potrace or similar) into just R^3.

template <class R> 
postscriptinator<R>::postscriptinator(){
    this->xmin = this->ymin = (R)( 1E99);
    this->xmax = this->ymax = (R)(-1E99);

    this->PageW = 21.0; //[cm]
    this->PageW = 24.0; //[cm]
    this->Enable_Auto_Sizing = true;

    //Set the page dimensions and coordinates.
    // Bottom left corner: (0,0). Top right: (21,24). Units are [cm]. Page size is 8.5"x11".
    this->Header += "%!PS\n";

    //Some definitions which are used to ~compress the resulting file.
    this->Definitions += "\n";
    this->Definitions += "/m {newpath moveto} bind def\n";
    this->Definitions += "/l {lineto} bind def\n";
    this->Definitions += "/cp {closepath} bind def\n";
    this->Definitions += "/s {stroke} bind def\n";
    this->Definitions += "/sg {setgray} bind def\n";
    this->Definitions += "\n";

    //Closing things.
    this->Footer += "showpage\n";
}


template <class R> void postscriptinator<R>::Import_Contour(const contour_of_points<R> &C, const vec3<R> &Proj2){
    //Performs two passes. First is to adjust the min/max values.


FUNCERR("This routine has not yet been written!");

}


template <class R> std::string postscriptinator<R>::Generate_Page_Geom(void) const {
    std::string out;
    //Compute the page dimension/density information.
    //Default is: 72 dpi (ie. 28.3465 dots/cm).
    out += "matrix currentmatrix /originmat exch def\n";
    out += "/umatrix {originmat matrix concatmatrix setmatrix} def\n";
    out += "[28.3465 0 0 28.3465 10.5 100.0] umatrix\n";

    return std::move(out);
}

template <class R> std::string postscriptinator<R>::Assemble(void) const {
    std::string out;
    out += this->Header;
    out += this->Definitions;
    out += this->Generate_Page_Geom();    
    for(auto it = this->Stack.begin(); it != this->Stack.end(); ++it){
        out += *it;
    }
    out += this->Footer;
    return std::move(out);
}

*/



