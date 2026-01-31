//YgorMathContourConnectivity.h

#ifndef YGOR_MATH_CONTOUR_CONNECTIVITY_H_
#define YGOR_MATH_CONTOUR_CONNECTIVITY_H_

#include "YgorMath.h"

//---------------------------------------------------------------------------------------------------------------------------
//---------- contour connectivity: triangulate the gap between two planar contour collections on offset planes -------------
//---------------------------------------------------------------------------------------------------------------------------
//
// This function computes the triangulation between two sets of planar contours that lie on parallel, offset planes.
// The input consists of two contour_collections: 'top' and 'bottom', where all contours within each collection 
// are coincident on a common plane. The planes are assumed to be coplanar (parallel) but separated by an 
// arbitrary distance.
//
// The function projects all vertices onto a common 2D plane and then computes a triangulation that connects the
// vertices of the top collection to the vertices of the bottom collection, creating a surface mesh that bridges
// the gap between the two planes.
//
// Key features:
// - Handles multiple contours within each collection (polygons with holes)
// - Respects contour orientation: opposing orientations denote holes
// - Does not assume the polygons are simple or monotone
// - The output mesh is not watertight, but represents surface patches bridging the planes
//
// Parameters:
// - top_cc: contour_collection with all contours lying on the 'top' plane
// - bottom_cc: contour_collection with all contours lying on the 'bottom' plane
// - distance_eps: tolerance for distance comparisons (default: 1E-6)
//
// Returns:
// - fv_surface_mesh containing the triangulated surface bridging the two planes
//
// Throws:
// - std::invalid_argument if either collection is empty
// - std::runtime_error if triangulation fails
//

template <class T>
fv_surface_mesh<T, uint64_t>
Triangulate_Planar_Contour_Connectivity(
    const contour_collection<T> &top_cc,
    const contour_collection<T> &bottom_cc,
    T distance_eps = static_cast<T>(1E-6)
);

#endif // YGOR_MATH_CONTOUR_CONNECTIVITY_H_
