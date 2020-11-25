//YgorMathIOOBJ.h - Written by hal clark in 2019, 2020.
//
// Routines for reading and writing simple (ascii) OBJ ("Wavefront Object") files.
//

#pragma once

#include <string>
#include <vector>
#include <istream>
#include <ostream>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// This routine reads an fv_surface_mesh from an OBJ format stream.
//
// Note that this routine does not validate or enforce manifoldness.
template <class T, class I>
bool
ReadFVSMeshFromOBJ(fv_surface_mesh<T,I> &fvsm,
                   std::istream &is );

// This routine writes an fv_surface_mesh to an OBJ format stream.
//
// Wavefront OBJ files permit the use of relative indices so that models can be combined by concatenating OBJ files
// together. While convenient, not all software supports this convention. Therefore, relative indexing is disabled by
// default.
//
// Note that metadata is currently not written.
template <class T, class I>
bool
WriteFVSMeshToOBJ(const fv_surface_mesh<T,I> &fvsm,
                  std::ostream &os,
                  bool use_relative_indexing = false);


// This routine reads an point_set from an OBJ format stream. Normals are optionally supported.
//
// Note that this routine will fail if faces or edges are encountered.
template <class T>
bool
ReadPointSetFromOBJ(point_set<T> &ps,
                    std::istream &is );

// This routine writes an point_set to an OBJ format stream. Normals are optionally supported.
//
// Note that metadata is currently not written.
template <class T>
bool
WritePointSetToOBJ(const point_set<T> &ps,
                  std::ostream &os );
