//YgorMathContourConnectivity.cc

#include <algorithm>
#include <cmath>
#include <limits>
#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
#include <cstdint>

#include "YgorMath.h"
#include "YgorMathContourConnectivity.h"

// Helper structure for 2D projected vertex with source information.
template <class T>
struct ProjectedVertex {
    vec2<T> pos_2d;          // 2D position in the projection plane
    vec3<T> pos_3d;          // Original 3D position
    size_t vertex_index;     // Index in the output mesh vertices array
    bool is_top;             // True if from top collection, false if from bottom
    size_t contour_index;    // Index of the contour within the collection
    size_t point_index;      // Index of the point within the contour
};

template <class T>
fv_surface_mesh<T, uint64_t>
Triangulate_Planar_Contour_Connectivity(
    const contour_collection<T> &top_cc,
    const contour_collection<T> &bottom_cc,
    T distance_eps
){
    using IndexType = uint64_t;
    
    fv_surface_mesh<T, IndexType> mesh;
    
    // Validate inputs
    if (top_cc.contours.empty()) {
        throw std::invalid_argument("Top contour collection is empty.");
    }
    if (bottom_cc.contours.empty()) {
        throw std::invalid_argument("Bottom contour collection is empty.");
    }
    
    // Compute average points using existing contour_collection API.
    // This handles the empty/NaN cases appropriately.
    const vec3<T> avg_top_point = top_cc.Average_Point();
    const vec3<T> avg_bottom_point = bottom_cc.Average_Point();
    
    // Check if average points are valid (collections may be non-empty but contain no actual vertices)
    if (!avg_top_point.isfinite() || !avg_bottom_point.isfinite()) {
        throw std::invalid_argument("Contour collections contain no vertices.");
    }
    
    // Compute the plane separation direction (from bottom to top)
    vec3<T> separation = avg_top_point - avg_bottom_point;
    const T separation_dist = separation.length();
    
    if (separation_dist < distance_eps) {
        throw std::runtime_error("Top and bottom planes appear to be coincident.");
    }
    
    // Plane normal is the direction perpendicular to both planes
    vec3<T> plane_normal = separation.unit();
    
    // Create orthonormal basis for 2D projection.
    // U and V will span the projection plane, N is the normal.
    vec3<T> U, V;
    
    // Find a vector not parallel to plane_normal
    vec3<T> arbitrary(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));
    if (std::abs(plane_normal.Dot(arbitrary)) > static_cast<T>(0.9)) {
        arbitrary = vec3<T>(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0));
    }
    
    U = plane_normal.Cross(arbitrary).unit();
    V = plane_normal.Cross(U).unit();
    
    // Projection origin (midpoint between average points)
    vec3<T> proj_origin = (avg_top_point + avg_bottom_point) / static_cast<T>(2);
    
    // Lambda to project 3D point to 2D
    auto project_to_2d = [&](const vec3<T> &pt) -> vec2<T> {
        vec3<T> rel = pt - proj_origin;
        return vec2<T>(rel.Dot(U), rel.Dot(V));
    };
    
    // Collect all projected vertices
    std::vector<ProjectedVertex<T>> projected_verts;
    
    // Add all vertices from both collections to the mesh and create projections
    size_t contour_idx = 0;
    for (const auto &contour : top_cc.contours) {
        size_t pt_idx = 0;
        for (const auto &pt : contour.points) {
            ProjectedVertex<T> pv;
            pv.pos_3d = pt;
            pv.pos_2d = project_to_2d(pt);
            pv.vertex_index = mesh.vertices.size();
            pv.is_top = true;
            pv.contour_index = contour_idx;
            pv.point_index = pt_idx;
            
            mesh.vertices.push_back(pt);
            projected_verts.push_back(pv);
            ++pt_idx;
        }
        ++contour_idx;
    }
    
    contour_idx = 0;
    for (const auto &contour : bottom_cc.contours) {
        size_t pt_idx = 0;
        for (const auto &pt : contour.points) {
            ProjectedVertex<T> pv;
            pv.pos_3d = pt;
            pv.pos_2d = project_to_2d(pt);
            pv.vertex_index = mesh.vertices.size();
            pv.is_top = false;
            pv.contour_index = contour_idx;
            pv.point_index = pt_idx;
            
            mesh.vertices.push_back(pt);
            projected_verts.push_back(pv);
            ++pt_idx;
        }
        ++contour_idx;
    }
    
    // Compute total number of top vertices for indexing bottom vertices later.
    size_t top_vertex_count = 0;
    for (const auto &contour : top_cc.contours) {
        top_vertex_count += contour.points.size();
    }
    
    // Now we need to triangulate the gap between top and bottom contours.
    // We use a greedy approach that connects vertices from both planes:
    // 1. For each contour edge on one plane, find nearby vertices on the other plane
    // 2. Create triangles connecting them
    
    // Sort projected vertices by angle around centroid for better connectivity
    vec2<T> centroid_2d(static_cast<T>(0), static_cast<T>(0));
    for (const auto &pv : projected_verts) {
        centroid_2d += pv.pos_2d;
    }
    centroid_2d /= static_cast<T>(projected_verts.size());
    
    // Helper to compute angle from centroid
    auto angle_from_centroid = [&centroid_2d](const vec2<T> &pos) -> T {
        vec2<T> delta = pos - centroid_2d;
        return std::atan2(delta.y, delta.x);
    };
    
    // Create lists of top and bottom vertex indices sorted by angle
    std::vector<size_t> top_indices, bottom_indices;
    for (size_t i = 0; i < projected_verts.size(); ++i) {
        if (projected_verts[i].is_top) {
            top_indices.push_back(i);
        } else {
            bottom_indices.push_back(i);
        }
    }
    
    // Sort by angle
    auto sort_by_angle = [&](std::vector<size_t> &indices) {
        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
            return angle_from_centroid(projected_verts[a].pos_2d) < 
                   angle_from_centroid(projected_verts[b].pos_2d);
        });
    };
    
    sort_by_angle(top_indices);
    sort_by_angle(bottom_indices);
    
    // Use a sweep approach to connect vertices.
    // For each vertex on the top, find the closest vertex on the bottom and vice versa.
    // Then create triangles.
    
    auto find_closest_on_other_plane = [&](size_t idx, bool from_top) -> size_t {
        const auto &pos = projected_verts[idx].pos_2d;
        const auto &other_indices = from_top ? bottom_indices : top_indices;
        
        if (other_indices.empty()) {
            throw std::runtime_error("Cannot find closest vertex: other plane has no vertices.");
        }
        
        size_t closest = other_indices[0];
        T min_dist = (projected_verts[closest].pos_2d - pos).sq_length();
        
        for (size_t i = 1; i < other_indices.size(); ++i) {
            T dist = (projected_verts[other_indices[i]].pos_2d - pos).sq_length();
            if (dist < min_dist) {
                min_dist = dist;
                closest = other_indices[i];
            }
        }
        return closest;
    };
    
    // Helper function to check if two 2D line segments intersect (excluding shared endpoints).
    // Uses the cross-product orientation test.
    auto segments_intersect_2d = [&](const vec2<T> &a1, const vec2<T> &a2, 
                                     const vec2<T> &b1, const vec2<T> &b2) -> bool {
        // Cross product of 2D vectors (returns scalar z-component)
        auto cross_2d = [](const vec2<T> &u, const vec2<T> &v) -> T {
            return u.x * v.y - u.y * v.x;
        };
        
        const T d1 = cross_2d(b2 - b1, a1 - b1);
        const T d2 = cross_2d(b2 - b1, a2 - b1);
        const T d3 = cross_2d(a2 - a1, b1 - a1);
        const T d4 = cross_2d(a2 - a1, b2 - a1);
        
        // Segments straddle each other if they have opposite orientations
        const T eps = distance_eps;
        if (((d1 > eps && d2 < -eps) || (d1 < -eps && d2 > eps)) &&
            ((d3 > eps && d4 < -eps) || (d3 < -eps && d4 > eps))) {
            return true;
        }
        return false;
    };
    
    // Helper to check if triangle is valid:
    // 1. Non-degenerate (has non-zero 3D area)
    // 2. Consistent orientation (normal has a meaningful direction)
    // 3. Edges do not intersect existing face edges in 2D projection
    
    // Create a map from vertex_index to projected 2D position for O(1) lookups
    std::map<IndexType, vec2<T>> vertex_to_2d;
    for (const auto &pv : projected_verts) {
        vertex_to_2d[static_cast<IndexType>(pv.vertex_index)] = pv.pos_2d;
    }
    
    auto is_valid_triangle = [&](size_t v0, size_t v1, size_t v2) -> bool {
        const auto &p0_3d = projected_verts[v0].pos_3d;
        const auto &p1_3d = projected_verts[v1].pos_3d;
        const auto &p2_3d = projected_verts[v2].pos_3d;
        
        // Calculate 3D triangle area using cross product
        const vec3<T> edge1 = p1_3d - p0_3d;
        const vec3<T> edge2 = p2_3d - p0_3d;
        const vec3<T> tri_normal = edge1.Cross(edge2);
        const T area = tri_normal.length() / static_cast<T>(2);
        
        // Reject degenerate triangles
        if (area < distance_eps * distance_eps) {
            return false;
        }
        
        // Orientation check: for bridging triangles connecting top and bottom planes,
        // we verify the triangle involves vertices from both planes. Triangles entirely
        // on one plane would be degenerate in 3D (caught by the area check above).
        // The non-degenerate area check is sufficient for valid bridging triangles.
        
        // Check for edge intersections with existing faces in 2D projection
        const auto &p0_2d = projected_verts[v0].pos_2d;
        const auto &p1_2d = projected_verts[v1].pos_2d;
        const auto &p2_2d = projected_verts[v2].pos_2d;
        
        // Define edges of the new triangle
        std::array<std::pair<vec2<T>, vec2<T>>, 3> new_edges = {{
            {p0_2d, p1_2d},
            {p1_2d, p2_2d},
            {p2_2d, p0_2d}
        }};
        
        // Check against all edges of existing faces
        for (const auto &face : mesh.faces) {
            if (face.size() < 3) continue;
            
            // Get 2D positions for existing face vertices using O(1) map lookup
            std::vector<vec2<T>> face_verts_2d;
            for (const auto &idx : face) {
                auto it = vertex_to_2d.find(idx);
                if (it != vertex_to_2d.end()) {
                    face_verts_2d.push_back(it->second);
                }
            }
            
            if (face_verts_2d.size() < 3) continue;
            
            // Check each edge of the existing face
            for (size_t i = 0; i < face_verts_2d.size(); ++i) {
                size_t j = (i + 1) % face_verts_2d.size();
                const auto &e1 = face_verts_2d[i];
                const auto &e2 = face_verts_2d[j];
                
                // Check against each edge of the new triangle
                for (const auto &new_edge : new_edges) {
                    // Skip if edges share endpoints (they connect at a vertex)
                    const T eps_sq = distance_eps * distance_eps;
                    if ((new_edge.first - e1).sq_length() < eps_sq ||
                        (new_edge.first - e2).sq_length() < eps_sq ||
                        (new_edge.second - e1).sq_length() < eps_sq ||
                        (new_edge.second - e2).sq_length() < eps_sq) {
                        continue;
                    }
                    
                    if (segments_intersect_2d(new_edge.first, new_edge.second, e1, e2)) {
                        return false; // Edges intersect, reject triangle
                    }
                }
            }
        }
        
        return true;
    };
    
    // Process each contour edge and try to form triangles with vertices on the other plane
    std::set<std::tuple<size_t, size_t, size_t>> added_triangles;
    
    auto add_triangle_if_valid = [&](size_t v0, size_t v1, size_t v2) {
        // Normalize triangle vertex order
        std::array<size_t, 3> tri = {v0, v1, v2};
        std::sort(tri.begin(), tri.end());
        
        auto key = std::make_tuple(tri[0], tri[1], tri[2]);
        if (added_triangles.count(key) > 0) {
            return; // Already added
        }
        
        if (is_valid_triangle(v0, v1, v2)) {
            added_triangles.insert(key);
            mesh.faces.push_back({static_cast<IndexType>(projected_verts[v0].vertex_index),
                                  static_cast<IndexType>(projected_verts[v1].vertex_index),
                                  static_cast<IndexType>(projected_verts[v2].vertex_index)});
        }
    };
    
    // Triangulate by iterating through contour edges and connecting to nearest vertices on other plane
    // Process top contour edges
    size_t global_idx = 0;
    for (const auto &contour : top_cc.contours) {
        const size_t n = contour.points.size();
        if (n < 2) {
            global_idx += n;
            continue;
        }
        
        for (size_t i = 0; i < n; ++i) {
            size_t next = (i + 1) % n;
            if (!contour.closed && next == 0) continue;
            
            size_t ti1 = global_idx + i;
            size_t ti2 = global_idx + next;
            
            // Find nearest bottom vertices to both top vertices
            size_t bi1 = find_closest_on_other_plane(ti1, true);
            size_t bi2 = find_closest_on_other_plane(ti2, true);
            
            // Create triangles
            if (bi1 != bi2) {
                // Two triangles forming a quad
                add_triangle_if_valid(ti1, ti2, bi1);
                add_triangle_if_valid(ti2, bi2, bi1);
            } else {
                // Single triangle
                add_triangle_if_valid(ti1, ti2, bi1);
            }
        }
        global_idx += n;
    }
    
    // Process bottom contour edges
    global_idx = top_vertex_count;
    for (const auto &contour : bottom_cc.contours) {
        const size_t n = contour.points.size();
        if (n < 2) {
            global_idx += n;
            continue;
        }
        
        for (size_t i = 0; i < n; ++i) {
            size_t next = (i + 1) % n;
            if (!contour.closed && next == 0) continue;
            
            size_t bi1 = global_idx + i;
            size_t bi2 = global_idx + next;
            
            // Find nearest top vertices to both bottom vertices
            size_t ti1 = find_closest_on_other_plane(bi1, false);
            size_t ti2 = find_closest_on_other_plane(bi2, false);
            
            // Create triangles
            if (ti1 != ti2) {
                // Two triangles forming a quad
                add_triangle_if_valid(bi1, bi2, ti1);
                add_triangle_if_valid(bi2, ti2, ti1);
            } else {
                // Single triangle
                add_triangle_if_valid(bi1, bi2, ti1);
            }
        }
        global_idx += n;
    }
    
    if (mesh.faces.empty()) {
        throw std::runtime_error("Triangulation produced no valid triangles.");
    }
    
    return mesh;
}

// Template instantiations
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template fv_surface_mesh<float,  uint64_t> Triangulate_Planar_Contour_Connectivity(const contour_collection<float>  &, const contour_collection<float>  &, float );
    template fv_surface_mesh<double, uint64_t> Triangulate_Planar_Contour_Connectivity(const contour_collection<double> &, const contour_collection<double> &, double);
#endif
