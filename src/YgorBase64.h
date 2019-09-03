//YgorBase64.h - Written by hal clark in 2019.
//
// Routines for encoding and decoding base-64 text.
//

#pragma once

#include <string>
#include <vector>


namespace Base64 {


std::string
Encode(const std::vector<uint8_t> &in);

std::string
EncodeFromString(const std::string &in);



std::vector<uint8_t>
Decode(const std::string &in);

std::string
DecodeToString(const std::string &in);


} // namespace Base64.

