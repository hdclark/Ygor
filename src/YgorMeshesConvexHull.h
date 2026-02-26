//YgorMeshesConvexHull.h - Written by hal clark in 2026.

#pragma once
#ifndef YGOR_MESHES_CONVEX_HULL_HDR_GRD_H
#define YGOR_MESHES_CONVEX_HULL_HDR_GRD_H

#include <array>
#include <cstdint>
#include <map>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// ============================================================================
// Adaptive-precision floating-point predicates.
//
// These routines implement Shewchuk-style adaptive arithmetic for robust
// geometric orientation tests.  They are intentionally kept separate from the
// ConvexHull class so that they can be reused elsewhere.
// ============================================================================

namespace adaptive_predicate {

// Two-product: computes a*b = hi + lo exactly in floating point.
template <class T>
void two_product(T a, T b, T &hi, T &lo);

// Two-sum: computes a+b = hi + lo exactly in floating point.
template <class T>
void two_sum(T a, T b, T &hi, T &lo);

// Grow-expansion: adds a scalar b to an existing expansion e (length elen),
// writing the result into h.  Returns the new length.
template <class T>
int grow_expansion(int elen, const T *e, T b, T *h);

// Expansion-sum: adds two expansions e and f, writing result to h.
// Returns the new length.
template <class T>
int expansion_sum(int elen, const T *e, int flen, const T *f, T *h);

// Scale-expansion: multiplies an expansion e by a scalar b, writing result
// to h.  Returns the new length.
template <class T>
int scale_expansion(int elen, const T *e, T b, T *h);

// Compress: removes zero entries from an expansion e, writing result to h.
// Returns the new length.
template <class T>
int compress(int elen, const T *e, T *h);

// Estimate: returns a fast floating-point approximation of the expansion sum.
template <class T>
T estimate(int elen, const T *e);

// orient3d_adaptive: robust orientation predicate for four 3-D points.
// Returns a positive value when d is below the plane of (a,b,c) (oriented
// counter-clockwise when viewed from above), negative when above, and zero
// when coplanar.  Handles all degeneracies via exact arithmetic.
template <class T>
T orient3d_adaptive(const T *pa, const T *pb, const T *pc, const T *pd);

// orient3d: fast-path with adaptive fallback.
template <class T>
T orient3d(const T *pa, const T *pb, const T *pc, const T *pd);

} // namespace adaptive_predicate


// ============================================================================
// ConvexHull: online 3-D convex hull via a randomized incremental algorithm.
//
// Template parameter T is the floating-point coordinate type (float, double).
//
// Usage:
//     ConvexHull<double> ch;
//     ch.add_vertex(v0);   // vec3<double>
//     ch.add_vertex(v1);
//     ...
//     const auto &mesh = ch.get_mesh();
//
// The resulting hull is stored as an fv_surface_mesh<T, uint64_t>.
//
// A mapping from vertex index in the mesh to the order in which it was
// evaluated is available via get_evaluation_order().  This is useful when
// the hull is computed on top of an existing fv_surface_mesh.
// ============================================================================

template <class T>
class ConvexHull {
    public:
        ConvexHull();

        // Add a single vertex to the hull.  Returns the evaluation-order
        // index assigned to this vertex.
        uint64_t add_vertex(const vec3<T> &v);

        // Convenience: add multiple vertices at once.
        void add_vertices(const std::vector<vec3<T>> &verts);

        // Access the current hull mesh.
        const fv_surface_mesh<T, uint64_t> & get_mesh() const;

        // Map from vertex index in the mesh to evaluation order (i.e., the
        // sequential index at which the vertex was provided to add_vertex).
        const std::map<uint64_t, uint64_t> & get_evaluation_order() const;

        // Return the number of vertices evaluated so far (including interior
        // points that are not on the hull).
        uint64_t num_evaluated() const;

    private:
        // Internal face representation used during incremental construction.
        struct Face {
            std::array<uint64_t, 3> verts; // vertex indices into m_points
            bool alive = true;
        };

        // All points ever added (in internal index order).  m_points stores
        // slightly perturbed copies used for geometric predicates; m_original
        // stores the unperturbed coordinates used in the output mesh.
        std::vector<vec3<T>> m_points;
        std::vector<vec3<T>> m_original;

        // Current list of faces (some may be dead / removed).
        std::vector<Face> m_faces;

        // Adjacency: for each directed edge (a,b), store the face index that
        // owns it.  Used for fast horizon detection.
        std::map<std::pair<uint64_t,uint64_t>, uint64_t> m_edge_to_face;

        // Evaluation order tracking: maps mesh vertex index (after compaction)
        // to the sequential order in which the vertex was provided to add_vertex.
        uint64_t m_eval_counter = 0;
        mutable std::map<uint64_t, uint64_t> m_eval_order;

        // Small perturbation magnitude for degeneracy resolution.
        T m_eps;

        // RNG state for perturbation.
        uint64_t m_rng_state;

        // Pseudo-random number in [0, 1).
        T rng_uniform();

        // Build the initial tetrahedron from the first 4 non-degenerate points.
        // The current implementation always returns 0 and callers ignore this
        // value; it is retained only for compatibility and does not indicate
        // the index of the next point to process.
        uint64_t build_initial_simplex();

        // Add a single point (by internal index) to the existing hull.
        void incorporate_point(uint64_t idx);

        // Robust orientation test wrapping the adaptive predicate.
        T orient(uint64_t a, uint64_t b, uint64_t c, uint64_t d) const;

        // Register / unregister directed-edge adjacency for a face.
        void register_face_edges(uint64_t face_idx);
        void unregister_face_edges(uint64_t face_idx);

        // Compact internal faces into the output mesh.
        void rebuild_mesh() const;

        // The output mesh, rebuilt on demand.
        mutable fv_surface_mesh<T, uint64_t> m_mesh;
        mutable bool m_mesh_dirty = true;
};

#endif // YGOR_MESHES_CONVEX_HULL_HDR_GRD_H
