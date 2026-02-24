//YgorMeshProcessing4.h - Written by hal clark in 2020.
//
// Routines for mesh processing.
//

#pragma once

#include "YgorDefinitions.h"
#include "YgorMath.h"


// Perform Loop subdivision on a triangle mesh.
//
// Note: This function only operates on triangle meshes. Non-triangle faces will cause an exception.
//
// Note: The mesh is modified in-place.
//
// Note: The number of iterations controls how many times the subdivision is applied.
//       Each iteration increases the face count by a factor of 4.
//
// Note: This implementation handles both closed meshes and meshes with boundary edges.
//       Boundary edges are detected as edges that belong to only one face.
//
// Note: Vertex normals and vertex colours are interpolated to new vertices,
//       but the result may not be accurate for all use cases.
//
template <class T, class I>
void
loop_subdivide(fv_surface_mesh<T,I> &fvsm,
               int64_t iterations = 1);

