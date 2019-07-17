//YgorMathIOOBJ.h - Written by hal clark in 2019.
//
// Routines for reading and writing simple (ascii) OBJ ("Wavefront Object") files.
//

#pragma once

#include <string>
#include <vector>

#include "YgorMath.h"


// This routine reads an fv_surface_mesh from an OBJ format stream.
//
// Note that this routine does not validate or enforce manifoldness.
template <class S, class T, class I>
bool
ReadFVSMeshFromOBJ(fv_surface_mesh<T,I> &fvsm,
                   S &ios ); // a stream.

// This routine writes an fv_surface_mesh to an OBJ format stream.
//
// Wavefront OBJ files permit the use of relative indices so that models can be combined by concatenating OBJ files
// together. While convenient, not all software supports this convention. Therefore, relative indexing is disabled by
// default.
//
// Note that metadata is currently not written.
template <class S, class T, class I>
bool
WriteFVSMeshToOBJ(const fv_surface_mesh<T,I> &fvsm,
                  S &ios,
                  bool use_relative_indexing = false); // a stream.
