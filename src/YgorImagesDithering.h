//YgorImagesDithering.h - Error-diffusion dithering routines.

#pragma once
#ifndef YGOR_IMAGES_DITHERING_HDR_GRD_H
#define YGOR_IMAGES_DITHERING_HDR_GRD_H

#include <cstdint>
#include <limits>
#include <set>

#include "YgorImages.h"

// Apply Floyd-Steinberg error diffusion dithering to specified channels of a planar image.
template <class T, class R>
void
Floyd_Steinberg_Dither(planar_image<T,R> &img,
                       std::set<int64_t> chnls = {},
                       T low = std::numeric_limits<T>::lowest(),
                       T high = std::numeric_limits<T>::max(),
                       long double threshold = std::numeric_limits<long double>::quiet_NaN());

#endif // YGOR_IMAGES_DITHERING_HDR_GRD_H
