//YgorMathIOSTL.h - Written by hal clark in 2020.
//
// Routines for reading and writing simple STL ("Stereolithography") files.
//

#pragma once

#include <string>
#include <vector>
#include <istream>
#include <ostream>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// This routine reads an fv_surface_mesh from an ASCII STL format stream.
//
// Note that this routine does not validate or enforce manifoldness.
template <class T, class I>
bool
ReadFVSMeshFromASCIISTL(fv_surface_mesh<T,I> &fvsm,
                        std::istream &is );

// This routine writes an fv_surface_mesh to an ASCII STL format stream.
//
// Note that metadata can not be written.
template <class T, class I>
bool
WriteFVSMeshToASCIISTL(const fv_surface_mesh<T,I> &fvsm,
                       std::ostream &os );

// This routine writes an fv_surface_mesh to a binary STL format stream.
//
// Note that metadata can not be written.
template <class T, class I>
bool
WriteFVSMeshToBinarySTL(const fv_surface_mesh<T,I> &fvsm,
                        std::ostream &os );

