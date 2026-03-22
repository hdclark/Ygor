//YgorMeshesOrient.h - Written by hal clark in 2026.

#pragma once
#ifndef YGOR_MESHES_ORIENT_HDR_GRD_H
#define YGOR_MESHES_ORIENT_HDR_GRD_H

#include <cstdint>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// Robustly orient faces in a face-vertex surface mesh so that adjacent faces
// have a consistent winding order and, where possible, outward-pointing normals.
//
// The algorithm proceeds as follows:
//
//   1. Partitioning: The mesh is decomposed into connected manifold patches.
//      Non-manifold edges (shared by more than two faces) act as patch
//      boundaries, and disconnected components are handled independently.
//
//   2. Local Consistency (BFS Propagation): Within each manifold patch, a
//      breadth-first traversal propagates orientation from a seed face to all
//      reachable faces via shared manifold edges.
//
//   3. Seed Selection (Bounding-Box Heuristic): The seed face for each patch
//      is chosen as the face whose centroid is closest to one of the six axis-
//      aligned bounding-box faces. Its orientation is set so the face normal
//      points outward relative to the corresponding bounding-box face normal.
//
//   4. Global Consistency (Ray Casting): For disconnected patches, a ray is
//      cast from each patch's representative face centroid along its normal.
//      If the ray crosses an odd number of faces of other patches, the patch
//      is considered to be oriented inward and is flipped.
//
//   5. Degenerate-Face Handling: Zero-area faces are detected and skipped
//      during seed selection and propagation to avoid numerical drift.
//
// eps controls duplicate-vertex tolerance when constructing edge adjacency.
//
// Returns true when orientation was successfully applied. Returns false only
// when the mesh contains contradictory adjacency within a manifold patch
// (e.g. a Möbius-strip-like topology).
template <class T, class I>
bool
OrientFaces(fv_surface_mesh<T,I> &fvsm,
            T eps = static_cast<T>(1E-6));

#endif // YGOR_MESHES_ORIENT_HDR_GRD_H
