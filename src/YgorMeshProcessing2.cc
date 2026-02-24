//YgorMeshProcessing2.cc - Mesh processing algorithms.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "YgorMath.h"
#include "YgorMeshProcessing2.h"

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------- mesh_remesher: implementation ---------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

template <class T, class I>
mesh_remesher<T, I>::mesh_remesher(mesh_type &mesh, T target_edge_length)
    : m_mesh(mesh),
      m_target_edge_length(target_edge_length),
      m_max_edge_length(static_cast<T>(4.0 / 3.0) * target_edge_length),
      m_min_edge_length(static_cast<T>(4.0 / 5.0) * target_edge_length) {
    
    if(target_edge_length <= static_cast<T>(0)) {
        throw std::runtime_error("Target edge length must be positive.");
    }
}

template <class T, class I>
int64_t mesh_remesher<T, I>::remesh_iteration() {
    int64_t total_changes = 0;
    total_changes += split_long_edges();
    total_changes += collapse_short_edges();
    total_changes += flip_edges_for_valence();
    total_changes += tangential_relaxation();
    return total_changes;
}

template <class T, class I>
void mesh_remesher<T, I>::ensure_involved_faces_index() {
    if(m_mesh.involved_faces.size() != m_mesh.vertices.size()) {
        m_mesh.recreate_involved_face_index();
    }
}

template <class T, class I>
std::set<std::pair<I, I>> mesh_remesher<T, I>::get_all_edges() const {
    std::set<std::pair<I, I>> edges;
    for(const auto &face : m_mesh.faces) {
        const auto N = face.size();
        if(N < 3) continue; // Skip degenerate faces.
        for(size_t i = 0; i < N; ++i) {
            I v0 = face[i];
            I v1 = face[(i + 1) % N];
            if(v0 > v1) std::swap(v0, v1);
            edges.emplace(v0, v1);
        }
    }
    return edges;
}

template <class T, class I>
std::vector<I> mesh_remesher<T, I>::get_faces_sharing_edge(I v0, I v1) const {
    std::vector<I> shared_faces;
    for(size_t f_idx = 0; f_idx < m_mesh.faces.size(); ++f_idx) {
        const auto &face = m_mesh.faces[f_idx];
        bool has_v0 = false, has_v1 = false;
        for(const auto &v : face) {
            if(v == v0) has_v0 = true;
            if(v == v1) has_v1 = true;
        }
        if(has_v0 && has_v1) {
            shared_faces.push_back(static_cast<I>(f_idx));
        }
    }
    return shared_faces;
}

template <class T, class I>
I mesh_remesher<T, I>::get_opposite_vertex(I face_idx, I v0, I v1) const {
    const auto &face = m_mesh.faces[face_idx];
    for(const auto &v : face) {
        if(v != v0 && v != v1) return v;
    }
    throw std::runtime_error("Could not find opposite vertex in triangle.");
}

template <class T, class I>
I mesh_remesher<T, I>::vertex_valence(I v_idx) const {
    std::set<I> neighbors;
    for(const auto &face : m_mesh.faces) {
        bool contains_v = false;
        for(const auto &v : face) {
            if(v == v_idx) {
                contains_v = true;
                break;
            }
        }
        if(contains_v) {
            for(const auto &v : face) {
                if(v != v_idx) {
                    neighbors.insert(v);
                }
            }
        }
    }
    return static_cast<I>(neighbors.size());
}

template <class T, class I>
bool mesh_remesher<T, I>::is_boundary_vertex(I v_idx) const {
    // A vertex is on the boundary if any of its edges is shared by only one face.
    std::set<I> neighbors;
    for(const auto &face : m_mesh.faces) {
        bool contains_v = false;
        for(const auto &v : face) {
            if(v == v_idx) {
                contains_v = true;
                break;
            }
        }
        if(contains_v) {
            for(const auto &v : face) {
                if(v != v_idx) {
                    neighbors.insert(v);
                }
            }
        }
    }
    
    for(const auto &neighbor : neighbors) {
        auto shared = get_faces_sharing_edge(v_idx, neighbor);
        if(shared.size() == 1) {
            return true;
        }
    }
    return false;
}

template <class T, class I>
vec3<T> mesh_remesher<T, I>::vertex_normal(I v_idx) const {
    vec3<T> normal(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
    
    for(size_t f_idx = 0; f_idx < m_mesh.faces.size(); ++f_idx) {
        const auto &face = m_mesh.faces[f_idx];
        if(face.size() < 3) continue;
        
        bool contains_v = false;
        for(const auto &v : face) {
            if(v == v_idx) {
                contains_v = true;
                break;
            }
        }
        
        if(contains_v) {
            // Compute face normal weighted by face area.
            const auto &p0 = m_mesh.vertices[face[0]];
            const auto &p1 = m_mesh.vertices[face[1]];
            const auto &p2 = m_mesh.vertices[face[2]];
            vec3<T> face_normal = (p1 - p0).Cross(p2 - p0);
            // The magnitude of the cross product is 2x the area, which serves as a weight.
            normal += face_normal;
        }
    }
    
    T len = normal.length();
    if(len > static_cast<T>(1e-10)) {
        normal = normal / len;
    }
    return normal;
}

template <class T, class I>
vec3<T> mesh_remesher<T, I>::one_ring_centroid(I v_idx) const {
    std::set<I> neighbors;
    for(const auto &face : m_mesh.faces) {
        bool contains_v = false;
        for(const auto &v : face) {
            if(v == v_idx) {
                contains_v = true;
                break;
            }
        }
        if(contains_v) {
            for(const auto &v : face) {
                if(v != v_idx) {
                    neighbors.insert(v);
                }
            }
        }
    }
    
    vec3<T> centroid(static_cast<T>(0), static_cast<T>(0), static_cast<T>(0));
    if(neighbors.empty()) {
        return m_mesh.vertices[v_idx];
    }
    
    for(const auto &n : neighbors) {
        centroid += m_mesh.vertices[n];
    }
    centroid = centroid / static_cast<T>(neighbors.size());
    return centroid;
}

template <class T, class I>
bool mesh_remesher<T, I>::collapse_would_invert_faces(I v_keep, I v_remove, const vec3<T> &new_pos) const {
    // Check all faces that reference v_remove (except those that will be deleted).
    for(size_t f_idx = 0; f_idx < m_mesh.faces.size(); ++f_idx) {
        const auto &face = m_mesh.faces[f_idx];
        if(face.size() < 3) continue;
        
        // Check if this face contains v_remove but NOT v_keep (these faces will be modified).
        bool has_v_remove = false;
        bool has_v_keep = false;
        for(const auto &v : face) {
            if(v == v_remove) has_v_remove = true;
            if(v == v_keep) has_v_keep = true;
        }
        
        if(has_v_remove && !has_v_keep) {
            // This face will have v_remove replaced with v_keep.
            // Check if the face normal would flip.
            std::vector<vec3<T>> old_pts, new_pts;
            for(const auto &v : face) {
                old_pts.push_back(m_mesh.vertices[v]);
                if(v == v_remove) {
                    new_pts.push_back(new_pos);
                } else {
                    new_pts.push_back(m_mesh.vertices[v]);
                }
            }
            
            vec3<T> old_normal = (old_pts[1] - old_pts[0]).Cross(old_pts[2] - old_pts[0]);
            vec3<T> new_normal = (new_pts[1] - new_pts[0]).Cross(new_pts[2] - new_pts[0]);
            
            // If the normal flips direction significantly, the collapse would invert the face.
            if(old_normal.Dot(new_normal) <= static_cast<T>(0)) {
                return true;
            }
            
            // Also check if the new face would be degenerate (zero area).
            if(new_normal.length() < static_cast<T>(1e-10) * old_normal.length()) {
                return true;
            }
        }
    }
    return false;
}

template <class T, class I>
void mesh_remesher<T, I>::do_collapse_edge(I v_keep, I v_remove) {
    // Remove faces that have both v_keep and v_remove.
    auto it = m_mesh.faces.begin();
    while(it != m_mesh.faces.end()) {
        bool has_keep = false, has_remove = false;
        for(const auto &v : *it) {
            if(v == v_keep) has_keep = true;
            if(v == v_remove) has_remove = true;
        }
        if(has_keep && has_remove) {
            it = m_mesh.faces.erase(it);
        } else {
            ++it;
        }
    }
    
    // Replace v_remove with v_keep in all remaining faces.
    for(auto &face : m_mesh.faces) {
        for(auto &v : face) {
            if(v == v_remove) v = v_keep;
        }
    }
    
    // Invalidate the involved_faces index.
    m_mesh.involved_faces.clear();
}

template <class T, class I>
bool mesh_remesher<T, I>::flip_improves_valence(I v0, I v1, I v_opp_a, I v_opp_b) const {
    // Current valences.
    I val_v0 = vertex_valence(v0);
    I val_v1 = vertex_valence(v1);
    I val_a = vertex_valence(v_opp_a);
    I val_b = vertex_valence(v_opp_b);
    
    // Target valence is 6 for interior vertices.
    const I target = 6;
    
    // Compute current deviation from target.
    auto deviation_before = std::abs(static_cast<int>(val_v0) - static_cast<int>(target))
                          + std::abs(static_cast<int>(val_v1) - static_cast<int>(target))
                          + std::abs(static_cast<int>(val_a) - static_cast<int>(target))
                          + std::abs(static_cast<int>(val_b) - static_cast<int>(target));
    
    // After flipping:
    // v0 and v1 each lose one edge (to each other), so valence decreases by 1.
    // v_opp_a and v_opp_b each gain one edge (to each other), so valence increases by 1.
    auto deviation_after = std::abs(static_cast<int>(val_v0) - 1 - static_cast<int>(target))
                         + std::abs(static_cast<int>(val_v1) - 1 - static_cast<int>(target))
                         + std::abs(static_cast<int>(val_a) + 1 - static_cast<int>(target))
                         + std::abs(static_cast<int>(val_b) + 1 - static_cast<int>(target));
    
    return deviation_after < deviation_before;
}

template <class T, class I>
void mesh_remesher<T, I>::do_flip_edge(I face_a, I face_b, I v0, I v1, I v_opp_a, I v_opp_b) {
    // The edge (v0, v1) is replaced by edge (v_opp_a, v_opp_b).
    // face_a becomes (v_opp_a, v_opp_b, v0) or similar ordering.
    // face_b becomes (v_opp_a, v_opp_b, v1) or similar ordering.
    
    // We need to preserve winding order to maintain consistent normals.
    // face_a originally has vertices {v0, v1, v_opp_a} in some order.
    // face_b originally has vertices {v0, v1, v_opp_b} in some order.
    
    // Find the correct winding for the new faces.
    auto &fa = m_mesh.faces[face_a];
    auto &fb = m_mesh.faces[face_b];
    
    // Determine original winding of face_a to find the rotation order.
    int v0_idx_a = -1, v1_idx_a = -1;
    for(int i = 0; i < 3; ++i) {
        if(fa[i] == v0) v0_idx_a = i;
        if(fa[i] == v1) v1_idx_a = i;
    }
    
    // If v0 -> v1 is the winding direction in face_a, then v_opp_a -> v0 -> ... 
    // After flip, face_a should be (v_opp_a, v0, v_opp_b) keeping similar winding sense.
    // Actually, let's just set the new faces directly.
    
    // new face_a: (v_opp_a, v_opp_b, v0)
    // new face_b: (v_opp_b, v_opp_a, v1)
    // But we need to preserve winding. Let's determine the correct order.
    
    // Original face_a has (v0, v1, v_opp_a) in some CCW order.
    // If (v0_idx_a + 1) % 3 == v1_idx_a, then the order is v0 -> v1 -> v_opp_a.
    // New face_a should be v_opp_a -> v0 -> v_opp_b to preserve CCW when v0 is shared.
    
    if((v0_idx_a + 1) % 3 == v1_idx_a) {
        // v0 -> v1 in face_a
        fa[0] = v_opp_a;
        fa[1] = v0;
        fa[2] = v_opp_b;
    } else {
        // v1 -> v0 in face_a
        fa[0] = v_opp_a;
        fa[1] = v_opp_b;
        fa[2] = v0;
    }
    
    // For face_b, similar logic.
    int v0_idx_b = -1, v1_idx_b = -1;
    for(int i = 0; i < 3; ++i) {
        if(fb[i] == v0) v0_idx_b = i;
        if(fb[i] == v1) v1_idx_b = i;
    }
    
    if((v0_idx_b + 1) % 3 == v1_idx_b) {
        // v0 -> v1 in face_b
        fb[0] = v_opp_b;
        fb[1] = v1;
        fb[2] = v_opp_a;
    } else {
        // v1 -> v0 in face_b
        fb[0] = v_opp_b;
        fb[1] = v_opp_a;
        fb[2] = v1;
    }
    
    // Invalidate the involved_faces index.
    m_mesh.involved_faces.clear();
}

//---------------------------------------------------------------------------------------------------------------------------
// Split edges longer than max_edge_length.
//---------------------------------------------------------------------------------------------------------------------------
template <class T, class I>
int64_t mesh_remesher<T, I>::split_long_edges() {
    int64_t splits = 0;
    bool changed = true;
    
    while(changed) {
        changed = false;
        auto edges = get_all_edges();
        
        for(const auto &edge : edges) {
            I v0 = edge.first;
            I v1 = edge.second;
            
            const auto &p0 = m_mesh.vertices[v0];
            const auto &p1 = m_mesh.vertices[v1];
            T edge_len = p0.distance(p1);
            
            if(edge_len > m_max_edge_length) {
                // Split this edge by adding a midpoint vertex.
                vec3<T> midpoint = (p0 + p1) / static_cast<T>(2);
                I new_v = static_cast<I>(m_mesh.vertices.size());
                m_mesh.vertices.push_back(midpoint);
                
                // Update vertex normals if they exist.
                if(!m_mesh.vertex_normals.empty()) {
                    // Interpolate normals.
                    vec3<T> avg_normal = (m_mesh.vertex_normals[v0] + m_mesh.vertex_normals[v1]) / static_cast<T>(2);
                    T len = avg_normal.length();
                    if(len > static_cast<T>(1e-10)) {
                        avg_normal = avg_normal / len;
                    }
                    m_mesh.vertex_normals.push_back(avg_normal);
                }
                
                // Update vertex colours if they exist.
                if(!m_mesh.vertex_colours.empty()) {
                    // Interpolate colours (simple average of each channel).
                    auto c0 = m_mesh.unpack_RGBA32_colour(m_mesh.vertex_colours[v0]);
                    auto c1 = m_mesh.unpack_RGBA32_colour(m_mesh.vertex_colours[v1]);
                    std::array<uint8_t, 4> avg_c;
                    for(int i = 0; i < 4; ++i) {
                        avg_c[i] = static_cast<uint8_t>((static_cast<int>(c0[i]) + static_cast<int>(c1[i])) / 2);
                    }
                    m_mesh.vertex_colours.push_back(m_mesh.pack_RGBA32_colour(avg_c));
                }
                
                // Split all faces that contain this edge.
                auto shared_faces = get_faces_sharing_edge(v0, v1);
                
                for(auto f_idx : shared_faces) {
                    const auto face_copy = m_mesh.faces[f_idx];  // Copy since we'll modify.
                    if(face_copy.size() < 3) continue;
                    
                    I v_opp = get_opposite_vertex(f_idx, v0, v1);
                    
                    // Replace the original face with two new faces.
                    // Original: (v0, v1, v_opp) -> (v0, new_v, v_opp) and (new_v, v1, v_opp)
                    // Find the ordering of v0 and v1 in the face to preserve winding.
                    
                    int v0_pos = -1, v1_pos = -1;
                    for(size_t i = 0; i < face_copy.size(); ++i) {
                        if(face_copy[i] == v0) v0_pos = static_cast<int>(i);
                        if(face_copy[i] == v1) v1_pos = static_cast<int>(i);
                    }
                    
                    // Determine winding: does v0 come before v1 in the cyclic order?
                    bool v0_before_v1 = ((v0_pos + 1) % 3 == v1_pos);
                    
                    std::vector<I> face1, face2;
                    if(v0_before_v1) {
                        // Order is v0 -> v1 -> v_opp
                        face1 = {v0, new_v, v_opp};
                        face2 = {new_v, v1, v_opp};
                    } else {
                        // Order is v1 -> v0 -> v_opp
                        face1 = {v1, new_v, v_opp};
                        face2 = {new_v, v0, v_opp};
                    }
                    
                    // Replace the original face with face1.
                    m_mesh.faces[f_idx] = face1;
                    
                    // Add face2 as a new face.
                    m_mesh.faces.push_back(face2);
                }
                
                ++splits;
                changed = true;
                
                // Invalidate index and restart to handle newly created edges.
                m_mesh.involved_faces.clear();
                break;  // Restart the loop with updated edge set.
            }
        }
    }
    
    return splits;
}

//---------------------------------------------------------------------------------------------------------------------------
// Collapse edges shorter than min_edge_length.
//---------------------------------------------------------------------------------------------------------------------------
template <class T, class I>
int64_t mesh_remesher<T, I>::collapse_short_edges() {
    int64_t collapses = 0;
    bool changed = true;
    
    while(changed) {
        changed = false;
        auto edges = get_all_edges();
        
        for(const auto &edge : edges) {
            I v0 = edge.first;
            I v1 = edge.second;
            
            // Skip if either vertex has been invalidated (we don't actually remove vertices,
            // so check if the edge still exists in any face).
            bool edge_exists = false;
            for(const auto &face : m_mesh.faces) {
                bool has_v0 = false, has_v1 = false;
                for(const auto &v : face) {
                    if(v == v0) has_v0 = true;
                    if(v == v1) has_v1 = true;
                }
                if(has_v0 && has_v1) {
                    edge_exists = true;
                    break;
                }
            }
            if(!edge_exists) continue;
            
            const auto &p0 = m_mesh.vertices[v0];
            const auto &p1 = m_mesh.vertices[v1];
            T edge_len = p0.distance(p1);
            
            if(edge_len < m_min_edge_length) {
                // Determine which vertex to keep.
                // Prefer keeping boundary vertices over interior ones.
                bool b0 = is_boundary_vertex(v0);
                bool b1 = is_boundary_vertex(v1);
                
                I v_keep, v_remove;
                vec3<T> new_pos;
                
                if(b0 && !b1) {
                    v_keep = v0;
                    v_remove = v1;
                    new_pos = p0;  // Keep boundary vertex at its position.
                } else if(b1 && !b0) {
                    v_keep = v1;
                    v_remove = v0;
                    new_pos = p1;
                } else {
                    // Both boundary or both interior: keep the one with lower index and use midpoint.
                    v_keep = v0;
                    v_remove = v1;
                    new_pos = (p0 + p1) / static_cast<T>(2);
                }
                
                // Check if collapse would invert any faces.
                if(collapse_would_invert_faces(v_keep, v_remove, new_pos)) {
                    continue;  // Skip this collapse.
                }
                
                // Move the kept vertex to the new position.
                m_mesh.vertices[v_keep] = new_pos;
                
                // Perform the collapse.
                do_collapse_edge(v_keep, v_remove);
                
                ++collapses;
                changed = true;
                break;  // Restart with updated edge set.
            }
        }
    }
    
    return collapses;
}

//---------------------------------------------------------------------------------------------------------------------------
// Flip edges to improve vertex valence.
//---------------------------------------------------------------------------------------------------------------------------
template <class T, class I>
int64_t mesh_remesher<T, I>::flip_edges_for_valence() {
    int64_t flips = 0;
    bool changed = true;
    
    while(changed) {
        changed = false;
        auto edges = get_all_edges();
        
        for(const auto &edge : edges) {
            I v0 = edge.first;
            I v1 = edge.second;
            
            // Get the two faces sharing this edge.
            auto shared_faces = get_faces_sharing_edge(v0, v1);
            
            // Edge flip only works for interior edges (shared by exactly 2 triangles).
            if(shared_faces.size() != 2) continue;
            
            const auto &face_a = m_mesh.faces[shared_faces[0]];
            const auto &face_b = m_mesh.faces[shared_faces[1]];
            
            // Only handle triangular faces.
            if(face_a.size() != 3 || face_b.size() != 3) continue;
            
            I v_opp_a = get_opposite_vertex(shared_faces[0], v0, v1);
            I v_opp_b = get_opposite_vertex(shared_faces[1], v0, v1);
            
            // Check if the edge between v_opp_a and v_opp_b already exists.
            // If so, flipping would create a duplicate edge.
            bool edge_already_exists = false;
            for(const auto &face : m_mesh.faces) {
                bool has_a = false, has_b = false;
                for(const auto &v : face) {
                    if(v == v_opp_a) has_a = true;
                    if(v == v_opp_b) has_b = true;
                }
                if(has_a && has_b) {
                    edge_already_exists = true;
                    break;
                }
            }
            if(edge_already_exists) continue;
            
            // Check if flipping would improve valence.
            if(!flip_improves_valence(v0, v1, v_opp_a, v_opp_b)) continue;
            
            // Check if the flip would create an inverted face.
            // The quadrilateral formed by (v0, v_opp_a, v1, v_opp_b) must be convex.
            const auto &p0 = m_mesh.vertices[v0];
            const auto &p1 = m_mesh.vertices[v1];
            const auto &pa = m_mesh.vertices[v_opp_a];
            const auto &pb = m_mesh.vertices[v_opp_b];
            
            // Compute the normal of the original configuration.
            vec3<T> n_orig = (p1 - p0).Cross(pa - p0);
            
            // Check using the Delaunay criterion approximation:
            // The flip is valid if the opposite vertices can "see" each other.
            // A simple check: compute cross products and ensure consistent orientation.
            vec3<T> quad_normal = n_orig;
            T len = quad_normal.length();
            if(len < static_cast<T>(1e-10)) continue;
            quad_normal = quad_normal / len;
            
            // Check if both new triangles would have normals aligned with the original.
            vec3<T> new_face_a_n = (p0 - pa).Cross(pb - pa);
            vec3<T> new_face_b_n = (p1 - pb).Cross(pa - pb);
            
            if(new_face_a_n.Dot(quad_normal) <= static_cast<T>(0) ||
               new_face_b_n.Dot(quad_normal) <= static_cast<T>(0)) {
                continue;  // Flip would invert a face.
            }
            
            // Perform the flip.
            do_flip_edge(shared_faces[0], shared_faces[1], v0, v1, v_opp_a, v_opp_b);
            
            ++flips;
            changed = true;
            break;  // Restart with updated topology.
        }
    }
    
    return flips;
}

//---------------------------------------------------------------------------------------------------------------------------
// Tangential relaxation (smoothing).
//---------------------------------------------------------------------------------------------------------------------------
template <class T, class I>
int64_t mesh_remesher<T, I>::tangential_relaxation(T lambda) {
    int64_t moved = 0;
    
    // Compute new positions for all non-boundary vertices.
    std::vector<vec3<T>> new_positions(m_mesh.vertices.size());
    std::vector<bool> should_move(m_mesh.vertices.size(), false);
    
    for(size_t v_idx = 0; v_idx < m_mesh.vertices.size(); ++v_idx) {
        I vi = static_cast<I>(v_idx);
        
        // Check if this vertex is referenced by any face.
        bool is_used = false;
        for(const auto &face : m_mesh.faces) {
            for(const auto &v : face) {
                if(v == vi) {
                    is_used = true;
                    break;
                }
            }
            if(is_used) break;
        }
        if(!is_used) {
            new_positions[v_idx] = m_mesh.vertices[v_idx];
            continue;
        }
        
        // Don't move boundary vertices.
        if(is_boundary_vertex(vi)) {
            new_positions[v_idx] = m_mesh.vertices[v_idx];
            continue;
        }
        
        // Compute the centroid of the one-ring neighborhood.
        vec3<T> centroid = one_ring_centroid(vi);
        
        // Compute the vertex normal.
        vec3<T> normal = vertex_normal(vi);
        
        // Project the motion onto the tangent plane.
        const auto &current = m_mesh.vertices[v_idx];
        vec3<T> motion = centroid - current;
        
        // Remove the normal component to get tangential motion.
        vec3<T> tangent_motion = motion - normal * motion.Dot(normal);
        
        // Apply relaxation with the lambda factor.
        new_positions[v_idx] = current + tangent_motion * lambda;
        should_move[v_idx] = (tangent_motion.length() > static_cast<T>(1e-10));
    }
    
    // Apply the new positions.
    for(size_t v_idx = 0; v_idx < m_mesh.vertices.size(); ++v_idx) {
        if(should_move[v_idx]) {
            m_mesh.vertices[v_idx] = new_positions[v_idx];
            ++moved;
        }
    }
    
    return moved;
}

//---------------------------------------------------------------------------------------------------------------------------
// Mesh quality metrics.
//---------------------------------------------------------------------------------------------------------------------------
template <class T, class I>
T mesh_remesher<T, I>::mean_edge_length() const {
    auto edges = get_all_edges();
    if(edges.empty()) return static_cast<T>(0);
    
    T total = static_cast<T>(0);
    for(const auto &edge : edges) {
        total += m_mesh.vertices[edge.first].distance(m_mesh.vertices[edge.second]);
    }
    return total / static_cast<T>(edges.size());
}

template <class T, class I>
T mesh_remesher<T, I>::edge_length_stddev() const {
    auto edges = get_all_edges();
    if(edges.size() < 2) return static_cast<T>(0);
    
    T mean = mean_edge_length();
    T sum_sq = static_cast<T>(0);
    
    for(const auto &edge : edges) {
        T len = m_mesh.vertices[edge.first].distance(m_mesh.vertices[edge.second]);
        T diff = len - mean;
        sum_sq += diff * diff;
    }
    
    return std::sqrt(sum_sq / static_cast<T>(edges.size() - 1));
}

template <class T, class I>
T mesh_remesher<T, I>::mean_valence() const {
    std::set<I> used_vertices;
    for(const auto &face : m_mesh.faces) {
        for(const auto &v : face) {
            used_vertices.insert(v);
        }
    }
    
    if(used_vertices.empty()) return static_cast<T>(0);
    
    T total = static_cast<T>(0);
    int64_t count = 0;
    
    for(const auto &v : used_vertices) {
        if(!is_boundary_vertex(v)) {
            total += static_cast<T>(vertex_valence(v));
            ++count;
        }
    }
    
    if(count == 0) return static_cast<T>(0);
    return total / static_cast<T>(count);
}

template <class T, class I>
T mesh_remesher<T, I>::valence_deviation() const {
    std::set<I> used_vertices;
    for(const auto &face : m_mesh.faces) {
        for(const auto &v : face) {
            used_vertices.insert(v);
        }
    }
    
    if(used_vertices.empty()) return static_cast<T>(0);
    
    const T target = static_cast<T>(6);
    T total_dev = static_cast<T>(0);
    int64_t count = 0;
    
    for(const auto &v : used_vertices) {
        if(!is_boundary_vertex(v)) {
            total_dev += std::abs(static_cast<T>(vertex_valence(v)) - target);
            ++count;
        }
    }
    
    if(count == 0) return static_cast<T>(0);
    return total_dev / static_cast<T>(count);
}

template <class T, class I>
std::pair<T, T> mesh_remesher<T, I>::aspect_ratio_range() const {
    T min_ar = std::numeric_limits<T>::max();
    T max_ar = static_cast<T>(0);
    
    for(const auto &face : m_mesh.faces) {
        if(face.size() != 3) continue;
        
        const auto &p0 = m_mesh.vertices[face[0]];
        const auto &p1 = m_mesh.vertices[face[1]];
        const auto &p2 = m_mesh.vertices[face[2]];
        
        T e0 = p0.distance(p1);
        T e1 = p1.distance(p2);
        T e2 = p2.distance(p0);
        
        // Find longest edge.
        T longest = std::max({e0, e1, e2});
        
        // Compute area.
        T area = (p1 - p0).Cross(p2 - p0).length() / static_cast<T>(2);
        
        // Altitude to longest edge.
        T altitude = static_cast<T>(2) * area / longest;
        
        // Aspect ratio.
        T ar = (altitude > static_cast<T>(1e-10)) ? (longest / altitude) : std::numeric_limits<T>::max();
        
        min_ar = std::min(min_ar, ar);
        max_ar = std::max(max_ar, ar);
    }
    
    if(min_ar == std::numeric_limits<T>::max()) {
        min_ar = static_cast<T>(0);
    }
    
    return {min_ar, max_ar};
}

template <class T, class I>
void mesh_remesher<T, I>::set_target_edge_length(T len) {
    if(len <= static_cast<T>(0)) {
        throw std::runtime_error("Target edge length must be positive.");
    }
    m_target_edge_length = len;
    m_max_edge_length = static_cast<T>(4.0 / 3.0) * len;
    m_min_edge_length = static_cast<T>(4.0 / 5.0) * len;
}

template <class T, class I>
T mesh_remesher<T, I>::get_target_edge_length() const {
    return m_target_edge_length;
}

template <class T, class I>
T mesh_remesher<T, I>::get_max_edge_length() const {
    return m_max_edge_length;
}

template <class T, class I>
T mesh_remesher<T, I>::get_min_edge_length() const {
    return m_min_edge_length;
}

//---------------------------------------------------------------------------------------------------------------------------
// Template instantiations.
//---------------------------------------------------------------------------------------------------------------------------
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template class mesh_remesher<float,  uint32_t>;
    template class mesh_remesher<float,  uint64_t>;
    template class mesh_remesher<double, uint32_t>;
    template class mesh_remesher<double, uint64_t>;
#endif
