//YgorIO.h - Written by hal clark in 2020.
//
// This file contains sub-routines for performing IO.
//

#pragma once

#include <iostream>

#include "YgorDefinitions.h"
#include "YgorMisc.h"
#include "YgorIO.h"

namespace ygor {
namespace io {


// Write multi-byte binary data to a stream.
// Returns false if any write fails.
// Endian conversion is handled if needed. 
//
// Note: On some platforms, the stream must be opened in binary mode.
//       This avoids, e.g., line-ending conversions.
template <class T, YgorEndianness destination_endianness>
inline
bool
write_binary(std::ostream& os,
             const T& val){

    static_assert( (   (YgorEndianness::Host == YgorEndianness::Little)
                    || (YgorEndianness::Host == YgorEndianness::Big)    ),
                   "This routine requires the host to be either little- or big-endian." );
    static_assert( (   (destination_endianness == YgorEndianness::Little)
                    || (destination_endianness == YgorEndianness::Big)    ),
                   "This routine requires the write style to be either little- or big-endian." );

    constexpr auto UsingLittleEndian = (YgorEndianness::Host == YgorEndianness::Little);
    constexpr auto UsingBigEndian    = (YgorEndianness::Host == YgorEndianness::Big);

    constexpr bool WriteLittleEndian = (destination_endianness == YgorEndianness::Little);
    constexpr bool WriteBigEndian    = (destination_endianness == YgorEndianness::Big);

    const unsigned char *c = reinterpret_cast<const unsigned char *>(&val); //c[0] to c[sizeof(T)-1] only.
    bool ret = true;

    // Write style matches host storage style. Keep the byte order unaltered.
    if constexpr (false){
    }else if constexpr ( (UsingLittleEndian && WriteLittleEndian)
                      || (UsingBigEndian && WriteBigEndian) ){
        if(!os.write( reinterpret_cast<const char *>( &c[0] ), sizeof(T) )){
            ret = false;
        }

    // Write style does not match host storage style. Reverse the order of bytes written.
    }else if constexpr ( (UsingLittleEndian && WriteBigEndian)
                      || (UsingBigEndian && WriteLittleEndian) ){
        for(size_t i = 0; i <= (sizeof(T)-1); ++i){
            if(ret && !os.put(c[sizeof(T)-1-i])){
                ret = false;
            }
        }
    }
    return ret;
}


// Read multi-byte binary data from a stream.
// Returns false if any read fails.
// Endian conversion is handled if needed.
//
// Note: On some platforms, the stream must be opened in binary mode.
//       This avoids, e.g., line-ending conversions.
template <class T, YgorEndianness source_endianness>
inline
bool
read_binary(std::istream& is,
            T& val){

    static_assert( (   (YgorEndianness::Host == YgorEndianness::Little)
                    || (YgorEndianness::Host == YgorEndianness::Big)    ),
                   "This routine requires the host to be either little- or big-endian." );
    static_assert( (   (source_endianness == YgorEndianness::Little)
                    || (source_endianness == YgorEndianness::Big)    ),
                   "This routine requires the read style to be either little- or big-endian." );

    constexpr auto UsingLittleEndian = (YgorEndianness::Host == YgorEndianness::Little);
    constexpr auto UsingBigEndian    = (YgorEndianness::Host == YgorEndianness::Big);

    constexpr bool ReadLittleEndian = (source_endianness == YgorEndianness::Little);
    constexpr bool ReadBigEndian    = (source_endianness == YgorEndianness::Big);

    uint8_t *c = reinterpret_cast<uint8_t *>(&val); //c[0] to c[sizeof(T)-1] only.
    bool ret = true;

    // Read style matches host storage style. Keep the byte order unaltered.
    if constexpr (false){
    }else if constexpr ( (UsingLittleEndian && ReadLittleEndian)
                      || (UsingBigEndian    && ReadBigEndian) ){
        if(!is.read( reinterpret_cast<char *>( &c[0] ), sizeof(T) )){
            ret = false;
        }

    // Read style does not match host storage style. Reverse the order of bytes written.
    }else if constexpr ( (UsingLittleEndian && ReadBigEndian)
                      || (UsingBigEndian    && ReadLittleEndian) ){
        for(size_t i = 0; i <= (sizeof(T)-1); ++i){
            if(ret && !is.read( reinterpret_cast<char *>( &c[sizeof(T)-1-i] ), 1)){
                ret = false;
            }
        }
    }

    return ret;
}

} // namespace io.
} // namespace ygor.


