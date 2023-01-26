//YgorImagesIO.h - Self-contained routines for writing and reading images.

#pragma once
#ifndef YGOR_IMAGES_IO_HDR_GRD_H
#define YGOR_IMAGES_IO_HDR_GRD_H

#include <string>
#include <istream>
#include <ostream>
#include <functional>
#include <list>
#include <utility>

#include "YgorDefinitions.h"
#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"

#include "YgorImages.h"


// --------------

// This class finds a minimally-lossy compression conversion from numbers in a floating-point type (e.g., float, double)
// to a smaller integer type (e.g., int8_t, int16_t, uint32_t, ...) that can be (approximately) reconstituted using a
// common, static, linear transformation.
//
// This class is designed to be used when writing images with floating-point pixel intensities to a file type that
// requires integer-valued pixel intensities, but which can also embed a linear transformation to help
// reconstitute/convert to the original type and range.
template <class T_domain, class T_range>
class linear_compress_numeric {
    public:
        using domain_t = T_domain;            // The inputs, often a floating-point type.
        using range_t  = T_range;             // The outputs, often an integer type, i.e., the 'compressed' values.
        using intermediate_t = long double;   // Should be floating point and wider-or-same as range and domain types.

    private:
        intermediate_t slope;
        intermediate_t inv_slope;
        intermediate_t intercept;

        T_domain domain_min;
        T_domain domain_max;

    public:
        linear_compress_numeric();

        // Find a transformation from the domain type ("A", e.g., float) to another type ("B", e.g., int16_t) by finding
        // a linear transformation (i.e., slope and intercept) that maps the full range of type B to the specified
        // domain of type "A".
        void optimize(T_domain domain_min = std::numeric_limits<T_domain>::lowest(),
                      T_domain domain_max = std::numeric_limits<T_domain>::max() );

        // Apply the inverse of the linear transformation to the domain type. If the range type "B" is less precise than
        // type "A" then this will compress the inputs in a lossy way.
        T_range compress(T_domain in_val) const;

        // Apply the linear transformation to recover the original domain type, approximately.
        //
        // Either throws when decompresses outside of domain, or clamps output to the domain.
        T_domain decompress(T_range in_val,
                            bool clamp_to_domain = false) const;

        // Apply the linear transformation to recover the original domain type, approximately, but do so by first
        // converting the transformation to a given type. This is useful to simulate how a given input will
        // round-trip when using a limited-precision copy of the transformation.
        //
        // Either throws when decompresses outside of domain, or clamps output to the domain.
        template<class Y>
        Y decompress_as(T_range in_val) const;

        // Retrieve the transformation.
        intermediate_t get_slope(void) const;
        intermediate_t get_intercept(void) const;
};


// --------------

//Dump raw pixel data to file as type T.
template <class T, class R>
bool
Dump_Pixels(const planar_image<T,R> &img,
            const std::string &filename,
            YgorEndianness destendian = YgorEndianness::Little);

//This enum is used to specify which type of pixel scaling should be used.
enum YgorImageIOPixelScaling {
    None,         // No scaling performed.
    TypeMinMax    // Scaling using the min and max of the origin and destination types.
};

//Dump raw pixel data to file, but static_cast to type Y and possibly autoscaled to fill
// the range of type Y (scaling proportionally to the min and max of type T).
template <class T, class R, class Y>
bool
Dump_Casted_Scaled_Pixels(const planar_image<T,R> &img,
                          const std::string &filename,
                          YgorImageIOPixelScaling scaling = YgorImageIOPixelScaling::None,
                          YgorEndianness destendian = YgorEndianness::Little);

// --------------

//Write pixels, other image class members, and key-value metadata to a FITSv4.0 formatted file.
//
// NOTE: Do not alter the endianness unless you're certain what you're doing! FITS files
//       are conventionally written in big-endian.
//
template <class T, class R>
bool 
WriteToFITS(const planar_image<T,R> &imgcoll, 
            const std::string &filename, 
            YgorEndianness destendian = YgorEndianness::Big);

template <class T, class R>
bool 
WriteToFITS(const planar_image_collection<T,R> &imgcoll, 
            const std::string &filename, 
            YgorEndianness destendian = YgorEndianness::Big);

template <class T, class R>
bool 
WriteToFITS(const std::list<std::reference_wrapper<const planar_image<T,R>>> &img_refws, 
            std::ostream &os,
            YgorEndianness destendian = YgorEndianness::Big);

//Read pixels, other class members, and key-value metadata from a FITSv4.0 formatted file.
//
// NOTE: Do not alter the endianness unless you're certain what you're doing! FITS files
//       are conventionally written in big-endian.
//
// NOTE: Non-standard keywords are required to be present to support round-trip.
//       Compare with the output of the FITS writer above to see which keywords.
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
template <class T, class R>
planar_image_collection<T,R> 
ReadFromFITS(const std::string &filename, 
             YgorEndianness destend = YgorEndianness::Big);

template <class T, class R>
planar_image_collection<T,R> 
ReadFromFITS(std::istream &is,
             YgorEndianness destend = YgorEndianness::Big);

#endif
