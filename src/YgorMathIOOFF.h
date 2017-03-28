//YgorMathIOOFF.h - Written by hal clark in 2017.
//
// This file defines routines for writing YgorMath classes to simple (ascii) OFF ("Object File Format") files.
//

#pragma once

#include <vector>

#include "YgorMath.h"


// This routine writes a collection of vec3<T> without any connectivity between them.
template <class T>
bool
WritePointsToOFF(std::vector<vec3<T>> points,
                 const std::string &filename,
                 const std::string &comment = "");

// This routine writes a line_segment in OFF format.
template <class T>
bool
WriteLineSegmentToOFF(line_segment<T> ls,
                      const std::string &filename,
                      const std::string &comment = "");


