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
// Note that reading arbitrary metadata is written as a comment and will be base64-encoded if necessary.
//
// Note that this routine does not validate or enforce manifoldness. Point clouds are valid.
//
// Note that a subset of PLY features are supported. In particular, custom/referenced materials are not supported.
template <class T, class I>
bool
ReadFVSMeshFromASCIIPLY(fv_surface_mesh<T,I> &fvsm,
                        std::istream &is );

// This routine writes an fv_surface_mesh to an ASCII PLY format stream.
//
// Note that reading arbitrary metadata is written as a comment and will be base64-encoded if necessary.
//
// Note that this routine does not validate or enforce manifoldness. Point clouds are valid.
//
// Note that only basic PLY features are used (vertex and vertex_index elements).
template <class T, class I>
bool
WriteFVSMeshToASCIIPLY(const fv_surface_mesh<T,I> &fvsm,
                       std::ostream &os );

