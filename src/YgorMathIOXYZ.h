//YgorMathIOXYZ.h - Written by hal clark in 2019.
//
// Routines for reading and writing simple (ascii) XYZ point cloud files.
//

#pragma once

#include <string>
#include <vector>
#include <istream>
#include <ostream>

#include "YgorMath.h"

template <class T> class vec3;


// This routine reads an point_set from an XYZ format stream.
template <class T>
bool
ReadPointSetFromXYZ(point_set<T> &ps,
                    std::istream &is );

// This routine writes an point_set to an XYZ format stream.
//
// Note that metadata is currently not written.
template <class T>
bool
WritePointSetToXYZ(const point_set<T> &ps,
                   std::ostream &os );
