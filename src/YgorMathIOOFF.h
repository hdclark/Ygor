//YgorMathIOOFF.h - Written by hal clark in 2017, 2019.
//
// Routines for reading and writing simple (ascii) OFF ("Object File Format") files.
//

#pragma once

#include <string>
#include <vector>
#include <istream>
#include <ostream>

#include "YgorMath.h"

template <class T> class line_segment;
template <class T> class vec3;


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


// This routine reads an fv_surface_mesh from an OFF format stream.
//
// Note that OFF files can contain lines, unconnected vertices, and other non-polyhedron (non-manifold) elements.
// This routine does not validate or enforce manifoldness.
template <class T, class I>
bool
ReadFVSMeshFromOFF(fv_surface_mesh<T,I> &fvsm,
                   std::istream &is );

// This routine writes an fv_surface_mesh to an OFF format stream.
//
// Note that metadata is currently not written.
template <class T, class I>
bool
WriteFVSMeshToOFF(const fv_surface_mesh<T,I> &fvsm,
                  std::ostream &os );
