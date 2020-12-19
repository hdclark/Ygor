//YgorImagesIO.h - Self-contained routines for writing and reading images.

#pragma once
#ifndef YGOR_IMAGES_IO_HDR_GRD_H
#define YGOR_IMAGES_IO_HDR_GRD_H

#include <string>

#include "YgorDefinitions.h"
#include "YgorMisc.h"
#include "YgorMath.h"

#include "YgorImages.h"

//This enum is used to specify which type of pixel scaling should be used.
enum YgorImageIOPixelScaling {
    None,         // No scaling performed.
    TypeMinMax    // Scaling using the min and max of the origin and destination types.
};

// --------------

//Dump raw pixel data to file as type T.
template <class T, class R>
bool
Dump_Pixels(const planar_image<T,R> &img,
            const std::string &filename,
            YgorEndianness destendian = YgorEndianness::Little);

//Dump raw pixel data to file, but static_cast to type Y and possibly autoscaled to fill
// the range of type Y (scaling proportionally to the min and max of type T).
template <class T, class R, class Y>
bool
Dump_Casted_Scaled_Pixels(const planar_image<T,R> &img,
                          const std::string &filename,
                          YgorImageIOPixelScaling scaling = YgorImageIOPixelScaling::None,
                          YgorEndianness destendian = YgorEndianness::Little);

//Write pixels and class members other than the metadata member to a FITS formatted file.
//
// NOTE: Do not alter the endianness unless you're certain what you're doing! FITS files
//       conventionally written in big-endian.
//
// NOTE: The metadata member *could* be serialized and written/read as an extension HDU.
//       This was not done because it was not needed at the time of writing.
//
template <class T, class R>
bool 
WriteToFITS(const planar_image<T,R> &img, 
            const std::string &filename, 
            YgorEndianness destendian = YgorEndianness::Big);

//Read pixels and class members other than the metadata member from a FITS formatted file.
//
// NOTE: Do not alter the endianness unless you're certain what you're doing! FITS files
//       conventionally written in big-endian.
//
// NOTE: If types T and R do not match, pixel values and image class member metadata are 
//       converted using static_cast after reading. Alternatively, the file could be probed
//       before loading happens.
//
//       This routine was written with very basic data interchange between identical instances
//       of the same program in mind. So the image types T and R were known before reading was
//       attempted. Because it is hard to have compile-time templated code react at run-time,
//       it may be possible to return an arbitrary planar_image<T,R> stuffed into a boost::any
//       or std::any. This logic is how Boost.Gil does it. Alternatively, you 
//       could merely assume the largest format possible (T=long double, R=long double), read 
//       the file, and convert after-the-fact. Regardless, this functionality was simply not 
//       needed at the time of writing. 
//
// NOTE: The metadata member *could* be serialized and written/read as an extension HDU.
//       This was not done because it was not needed at the time of writing.
//
template <class T, class R>
planar_image<T,R> 
ReadFromFITS(const std::string &filename, 
             YgorEndianness destend = YgorEndianness::Big);

#endif
