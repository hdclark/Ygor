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
             std::ostream &os );

