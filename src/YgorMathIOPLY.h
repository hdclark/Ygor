//YgorMathIOPLY.h - Written by hal clark in 2020.
//
// Routines for reading and writing simple PLY ("Polygon file format") files.
//

#pragma once

#include <string>
#include <vector>
#include <istream>
#include <ostream>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// This routine reads an fv_surface_mesh from an ASCII PLY format stream.
//
// Note that this routine does not validate or enforce manifoldness.
//
// Note that a subset of PLY features are supported. In particular, custom/referenced materials are not supported.
template <class T, class I>
bool
ReadFVSMeshFromASCIIPLY(fv_surface_mesh<T,I> &fvsm,
                        std::istream &is );

// This routine writes an fv_surface_mesh to an ASCII PLY format stream.
//
// Note that metadata can not be written.
template <class T, class I>
bool
WriteFVSMeshToASCIIPLY(const fv_surface_mesh<T,I> &fvsm,
                       std::ostream &os );

