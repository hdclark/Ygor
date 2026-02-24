//YgorMeshProcessing2.h - Mesh processing algorithms.

#ifndef YGOR_MESH_PROCESSING2_H_
#define YGOR_MESH_PROCESSING2_H_

#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include "YgorMath.h"

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------- mesh_remesher: an iterative remeshing algorithm class ----------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
// This class implements the iterative remeshing algorithm described in:
//
//   Botsch M, Kobbelt L. "A remeshing approach to multiresolution modeling."
//   In Proceedings of the 2004 Eurographics/ACM SIGGRAPH symposium on Geometry processing.
//   2004 Jul 8 (pp. 185-192).
//
// The algorithm consists of four key sub-algorithms which are repeatedly applied to improve mesh quality:
//   1. Edge Splitting: Any edge longer than 4/3 * L_target is split at its midpoint.
//   2. Edge Collapsing: Any edge shorter than 4/5 * L_target is collapsed.
//   3. Edge Flipping: Edges are flipped to improve vertex valence (target valence = 6).
//   4. Tangential Relaxation: Vertices are moved tangentially to smooth the mesh.
//
// The mesh is modified in-place and remains valid after every operation.
//
template <class T, class I>
class mesh_remesher {
    public:
        using mesh_type = fv_surface_mesh<T, I>;

        // Constructor: takes a reference to the mesh and target edge length.
        mesh_remesher(mesh_type &mesh, T target_edge_length);

        // The main iterative remeshing routine. Performs all four sub-algorithms.
        // Returns the number of mesh modifications made during this iteration.
        int64_t remesh_iteration();

        // Individual sub-algorithms, accessible for fine-grained control.
        
        // Split edges longer than max_edge_length (4/3 * L_target).
        // Returns the number of edges that were split.
        int64_t split_long_edges();

        // Collapse edges shorter than min_edge_length (4/5 * L_target).
        // Returns the number of edges that were collapsed.
        int64_t collapse_short_edges();

        // Flip edges to improve vertex valence towards the target valence (6).
        // Returns the number of edges that were flipped.
        int64_t flip_edges_for_valence();

        // Perform tangential relaxation to smooth vertex positions.
        // Lambda controls the relaxation strength (0 = no motion, 1 = full motion).
        // Returns the number of vertices that were moved.
        int64_t tangential_relaxation(T lambda = static_cast<T>(0.5));

        // Mesh quality metrics.
        
        // Compute the mean edge length across the entire mesh.
        T mean_edge_length() const;

        // Compute the standard deviation of edge lengths.
        T edge_length_stddev() const;

        // Compute the mean valence across all non-boundary vertices.
        T mean_valence() const;

        // Compute the valence deviation from the ideal (6 for interior vertices).
        T valence_deviation() const;

        // Compute minimum and maximum aspect ratios across all triangles.
        // Aspect ratio = longest edge / altitude to longest edge.
        std::pair<T, T> aspect_ratio_range() const;

        // Set/get parameters.
        void set_target_edge_length(T len);
        T get_target_edge_length() const;
        T get_max_edge_length() const;
        T get_min_edge_length() const;

    private:
        mesh_type &m_mesh;
        T m_target_edge_length;
        T m_max_edge_length;  // 4/3 * L_target
        T m_min_edge_length;  // 4/5 * L_target

        // Internal helper functions.

        // Ensure involved_faces index is up to date.
        void ensure_involved_faces_index();

        // Get all unique edges in the mesh as pairs of vertex indices (smaller index first).
        std::set<std::pair<I, I>> get_all_edges() const;

        // Get the two faces that share an edge (returns empty vector if boundary edge).
        std::vector<I> get_faces_sharing_edge(I v0, I v1) const;

        // Get the opposite vertex of an edge in a triangle.
        I get_opposite_vertex(I face_idx, I v0, I v1) const;

        // Compute the valence (number of adjacent edges) of a vertex.
        I vertex_valence(I v_idx) const;

        // Check if a vertex is on a boundary (part of an edge shared by only one face).
        bool is_boundary_vertex(I v_idx) const;

        // Compute the area-weighted normal at a vertex.
        vec3<T> vertex_normal(I v_idx) const;

        // Compute the centroid of a vertex's one-ring neighborhood.
        vec3<T> one_ring_centroid(I v_idx) const;

        // Check if collapsing an edge would create degenerate geometry.
        bool collapse_would_invert_faces(I v_keep, I v_remove, const vec3<T> &new_pos) const;

        // Perform the actual edge collapse operation.
        void do_collapse_edge(I v_keep, I v_remove);

        // Check if flipping an edge would improve valence.
        bool flip_improves_valence(I v0, I v1, I v_opp_a, I v_opp_b) const;

        // Perform the actual edge flip operation.
        void do_flip_edge(I face_a, I face_b, I v0, I v1, I v_opp_a, I v_opp_b);
};

//---------------------------------------------------------------------------------------------------------------------------

#endif // YGOR_MESH_PROCESSING2_H_
