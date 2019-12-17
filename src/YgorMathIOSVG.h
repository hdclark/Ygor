//YgorMathIOSVG.h - Written by hal clark in 2019.
//
// Routines for writing simple SVG files.
//

#pragma once

#include <string>
#include <vector>
#include <istream>
#include <ostream>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// This routine writes a planar contours and metadata to an SVG format stream.
template <class T>
bool
WriteCCToSVG(const contour_collection<T> &cc,
             const plane<T> &plane,
             std::ostream &os,
             T extra_space = static_cast<T>(5),     // The nearest separation between the contour and the edge of the viewbox.
             T font_size = static_cast<T>(1),       // The size of the text's font in the same units as the contours (i.e., mm).
             std::string perimeter_text = "");      // Text to render along the contour. Reference metadata keys like '$Key'.

