//YgorBase64.cc - A part of Ygor, 2019. Written by hal clark.

#include <array>
#include <string>

#include "YgorDefinitions.h"
#include "YgorBase64.h"

#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorString.h"


namespace Base64 {

std::string Encode(const std::vector<uint8_t> &in){
    // This routine encodes a string of bytes as base-64 text. The output is padded so that the length is a multiple of
    // 4 characters.
    //
    // Note: This routine will *NOT* encode base-64 encoded URLs, which require passthrough of many ASCII characters.
    //
    // Note: This routine adapted from the public domain implementation at
    // https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64#C++ .
    // See https://web.archive.org/web/20190903021333/https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64
    // for an archive of the implementation.
    //

    static_assert( (YgorEndianness::Host == YgorEndianness::Little),
                   "This routine assumes the host is little-endian. Cannot continue." );

    const static std::array<char, 64> char_table = { 
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
        'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
        's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/' };
    const char pad_char = '=';

    std::string out;
    if(in.empty()) return out;

    out.reserve(((in.size()/3) + (in.size() % 3 > 0)) * 4);
    
    auto it = std::begin(in);

    for(size_t i = 0; i < in.size() / 3; ++i){
        auto s = static_cast<uint32_t>(0);
        s += (*it++) << 16; // Convert to big endian.
        s += (*it++) << 8;
        s += (*it++);

        out.append(1, char_table[(s & 0x00FC0000) >> 18]);
        out.append(1, char_table[(s & 0x0003F000) >> 12]);
        out.append(1, char_table[(s & 0x00000FC0) >> 6]);
        out.append(1, char_table[(s & 0x0000003F)]);
    }

    if(false){
    }else if(in.size() % 3 == 2){
        auto s = static_cast<uint32_t>(0);
        s += (*it++) << 16; // Convert to big endian.
        s += (*it++) << 8;

        out.append(1, char_table[(s & 0x00FC0000) >> 18]);
        out.append(1, char_table[(s & 0x0003F000) >> 12]);
        out.append(1, char_table[(s & 0x00000FC0) >> 6]);
        out.append(1, pad_char);

    }else if(in.size() % 3 == 1){
        auto s = static_cast<uint32_t>(0);
        s += (*it++) << 16; // Convert to big endian.

        out.append(1, char_table[(s & 0x00FC0000) >> 18]);
        out.append(1, char_table[(s & 0x0003F0C0) >> 12]);
        out.append(2, pad_char);
    }

    return out;
}

std::string EncodeFromString(const std::string &in){
    // This routine is convenient for converting text segments into base64. This helps, for example, to ensure that
    // whitespace and non-printable characters in the text segment do not interfere with text layouts when serializing
    // or writing to a structured file.
    //
    // This routine needs a non-owning array pointer; std::span would be nice, but is currently not available. TODO.

    std::vector<uint8_t> shtl;
    shtl.reserve(in.size());
    for(const auto &c : in) shtl.push_back( static_cast<uint8_t>(c) );

    return Encode(shtl);
}

std::vector<uint8_t> Decode(const std::string &in){
    // This routine decodes a string of bytes as base-64 text. The input must be padded so that the length is a multiple of
    // 4 characters, and *only* characters from the base-64 substitution table (see the Encode routine) must be used. If
    // newlines and whitespace are present, they must be filtered out prior to invoking this routine.
    //
    // Note: This routine does *NOT* decode base-64 encoded URLs, which passthrough many ASCII characters.
    //
    // Note: This routine adapted from the public domain implementation at
    // https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64#C++ .
    // See https://web.archive.org/web/20190903021333/https://en.wikibooks.org/wiki/Algorithm_Implementation/Miscellaneous/Base64
    // for an archive of the implementation.
    //
    const char pad_char = '=';

    if(in.length() % 4 != 0){
        throw std::invalid_argument("Input string length not a multiple of 4. Cannot continue.");
    }

    size_t padding = 0;
    if(in.length() != 0){
        if(in.at(in.length()-1) == pad_char) ++padding;
        if(in.at(in.length()-2) == pad_char) ++padding;
    }

    std::vector<uint8_t> out;
    if(in.empty()) return out;
    out.reserve(((in.length() / 4) * 3) - padding);

    auto s = static_cast<uint32_t>(0);

    for(auto it = std::begin(in);  it != std::end(in);  ){
        for(size_t i = 0; i < 4; ++i){
            s <<= 6;

            if(false){
            }else if(*it >= 0x41 && *it <= 0x5A){
                s |= *it - 0x41;
            }else if(*it >= 0x61 && *it <= 0x7A){
                s |= *it - 0x47;
            }else if(*it >= 0x30 && *it <= 0x39){
                s |= *it + 0x04;

            }else if(*it == 0x2B){
                s |= 0x3E;
            }else if(*it == 0x2F){
                s |= 0x3F;

            }else if(*it == pad_char){
                const auto dist = std::distance(it, std::end(in));

                if(false){
                }else if(dist == 1){ // A single padding character.
                    out.push_back((s >> 16) & 0x000000FF);
                    out.push_back((s >> 8 ) & 0x000000FF);
                    return out;

                }else if(dist == 2){ // Two padding characters.
                    out.push_back((s >> 10) & 0x000000FF);
                    return out;

                }else{
                    throw std::invalid_argument("Base64 string contained invalid placement of padding character. Cannot continue.");
                }
            }else{
                //std::cerr << "Misbehaving character 
                throw std::invalid_argument("Invalid character used for base64 encoding. Cannot continue.");
            }
            ++it;
        }

        out.push_back((s >> 16) & 0x000000FF);
        out.push_back((s >> 8 ) & 0x000000FF);
        out.push_back((s      ) & 0x000000FF);
    }

    return out;
}

std::string DecodeToString(const std::string &in){
    // This routine is convenient for decoding base64 strings into strings. This is convenient for serializing std::maps
    // with std::string keys and values.
    //
    // This routine needs a non-owning array pointer; std::span would be nice, but is currently not available. TODO.
    //
    const auto shtl = Decode(in);

    std::string out;
    for(const auto &b : shtl) out.append(1, static_cast<char>(b) );

    return out;
}

} // namespace Base64.

