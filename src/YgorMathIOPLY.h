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


// This routine reads an fv_surface_mesh from a PLY format stream. Both ASCII and little-endian binary encodings are
// supported.
//
// Note: Reading arbitrary metadata is written as a comment and will be base64-encoded if necessary.
//
// Note: This routine does not validate or enforce manifoldness. Point clouds are valid.
//
// Note: Only a *subset* of PLY features are supported. In particular, custom/referenced materials are not supported.
//
template <class T, class I>
bool
ReadFVSMeshFromPLY(fv_surface_mesh<T,I> &fvsm,
                   std::istream &is );


// This routine writes an fv_surface_mesh to an ASCII or binary encoded PLY format stream (little-endian).
//
// Note: Binary files are wrought with problems:
//
//       1. They can be non-portable due to incompatibilities in data type representations on host and recipient,
//
//       2. Binary PLY files have a strange 'half-binary' structure with a text header binary body. On systems where
//          there are differences in how text and binary streams are handled, it can be problematic to try switch modes.
//          Opening in text mode might work, but can lead to inconsistencies that depend on *where* the file was
//          generated.
//
//       3. Debugging is much harder, especially since the binary body is just a block of numbers. If the count is wrong
//          or unexpected, the recipient likely won't be able extract any usable data. In particular, binary files
//          should not be used for archival purposes!
//
//       Binary files are mostly useful when performance or storage size are important AND you control both sender and
//       recipient machines (or they are the same).
//
// Note: Reading arbitrary metadata is written as a comment and will be base64-encoded if necessary.
//
// Note: This routine does not validate or enforce manifoldness. Point clouds are valid.
//
// Note: Only basic PLY features are used (vertex and vertex_index elements).
//
template <class T, class I>
bool
WriteFVSMeshToPLY(const fv_surface_mesh<T,I> &fvsm,
                  std::ostream &os,
                  bool as_binary = false);

