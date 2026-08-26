//YgorMeshesHalfEdge.h - Written by hal clark in 2026.
//
// Half-edge (doubly-connected edge list) surface mesh representation.
//
// The half-edge data structure provides efficient adjacency queries for
// manifold (or manifold-with-boundary) triangle meshes.  Each directed edge
// is stored as a "half-edge" that knows its origin vertex, incident face,
// opposite (twin) half-edge, and the next/previous half-edges in the face
// loop.
//
// Bidirectional conversion routines are provided so users can conveniently
// convert between this representation and the face-vertex list
// representation (fv_surface_mesh).
//

#pragma once
#ifndef YGOR_MESHES_HALF_EDGE_HDR_GRD_H
#define YGOR_MESHES_HALF_EDGE_HDR_GRD_H

#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// A single directed half-edge record.
template <class I>
struct half_edge_t {
    I vertex; // Origin vertex of this half-edge.
    I face;   // Incident face (sentinel if this is a boundary half-edge).
    I twin;   // Opposite half-edge (sentinel if no twin exists).
    I next;   // Next half-edge in the face/boundary loop.
    I prev;   // Previous half-edge in the face/boundary loop.
};


// Half-edge surface mesh.
//
// Template parameter T is the floating-point scalar type (float or double),
// and I is the unsigned integer index type (uint32_t or uint64_t).
//
// The mesh may represent manifold surfaces, manifold surfaces with boundary,
// or be empty.  Non-manifold meshes cannot be faithfully represented and
// construction from such a mesh will throw.
template <class T, class I>
class he_surface_mesh {
    public:
        // Sentinel value indicating "no element" for index type I.
        static constexpr I sentinel = std::numeric_limits<I>::max();

        std::vector<vec3<T>> vertices;           // Vertex positions.
        std::vector<vec3<T>> vertex_normals;     // Optional per-vertex normals.
                                                 // Should always be empty OR have
                                                 // the same size as this->vertices.

        std::vector<uint32_t> vertex_colours;    // Optional per-vertex RGBA colour.
                                                 // Should always be empty OR have
                                                 // the same size as this->vertices.

        std::vector<I> vertex_halfedges;         // One outgoing half-edge per vertex.
                                                 // Should have the same size as
                                                 // this->vertices.

        std::vector<half_edge_t<I>> halfedges;   // All half-edges.
        std::vector<I> face_halfedges;           // One boundary half-edge per face.

        std::map<std::string,std::string> metadata; // User-defined metadata.


        //Constructors.
        he_surface_mesh();
        he_surface_mesh(const he_surface_mesh &in);

        //Member functions.
        he_surface_mesh & operator= (const he_surface_mesh &);
        bool operator==(const he_surface_mesh &) const;
        bool operator!=(const he_surface_mesh &) const;

        void swap(he_surface_mesh &);

        // The 'default' way to pack/unpack/interpret 8-bit colour data. Provided here to avoid rewriting everywhere.
        uint32_t pack_RGBA32_colour(std::array<uint8_t,4>) const;
        std::array<uint8_t,4> unpack_RGBA32_colour(uint32_t) const;

        // Disregards face orientation; area is always positive. Individual faces can be selected, or negative to select
        // all faces.
        T surface_area(int64_t n = -1) const;

        // Re-compute this->vertex_normals using the current face orientations.
        void compute_vertex_normals(void);

        // Element counts.
        I num_vertices(void) const;
        I num_faces(void) const;
        I num_halfedges(void) const;
        I num_edges(void) const;

        // Adjacency queries.
        std::vector<I> vertex_faces(I vert) const;       // Faces incident on a vertex.
        std::vector<I> vertex_neighbours(I vert) const;   // Adjacent vertices.
        std::vector<I> face_vertices(I face) const;       // Vertices bounding a face.
        std::vector<I> face_neighbours(I face) const;     // Faces sharing an edge.

        // Boundary queries.
        bool is_boundary_vertex(I vert) const;
        bool is_boundary_halfedge(I he) const;

        // Conversion to face-vertex list representation.
        fv_surface_mesh<T,I> convert_to_fv_surface_mesh(void) const;

        //Checks if the key is present without inspecting the value.
        bool MetadataKeyPresent(std::string key) const;

        //Attempts to cast the value if present. Optional is disengaged if key is missing or cast fails.
        template <class U> std::optional<U> GetMetadataValueAs(std::string key) const;
};


// Conversion from face-vertex list to half-edge representation.
//
// The input mesh should be manifold (or manifold with boundary) with
// consistent face orientation.  Non-manifold edges will cause this
// routine to throw.
template <class T, class I>
he_surface_mesh<T,I>
convert_fv_to_he_surface_mesh(const fv_surface_mesh<T,I> &fvsm);


#endif // YGOR_MESHES_HALF_EDGE_HDR_GRD_H
