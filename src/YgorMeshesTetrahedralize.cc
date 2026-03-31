//YgorMeshesTetrahedralize.cc - Convert polyhedral surface meshes into tetrahedral meshes
//                              via octree-based spatial decomposition.
//
#include <cstdint>
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <array>
#include <utility>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <functional>
#include <limits>
#include <any>

#include "YgorDefinitions.h"
#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorIndexOctree.h"

#include "YgorMeshesTetrahedralize.h"


// ============================================================
// Internal helper types and functions (not exported).
// ============================================================
namespace {

// A cell in the adaptive octree decomposition.
// Identified by its integer coordinates (ix,iy,iz) at a given refinement level.
// Cell size = domain_size / 2^level.
struct decomp_cell {
    int64_t level;
    int64_t ix, iy, iz;

    bool operator<(const decomp_cell &o) const {
        return std::tie(level, ix, iy, iz) < std::tie(o.level, o.ix, o.iy, o.iz);
    }
    bool operator==(const decomp_cell &o) const {
        return (level == o.level) && (ix == o.ix) && (iy == o.iy) && (iz == o.iz);
    }
};


// Determine if a triangle (defined by three vertices) intersects an axis-aligned box.
//
// Uses the separating-axis theorem (SAT) with the 13 potential separating axes:
//   3 box face normals, 1 triangle normal, 9 cross products of box edges x triangle edges.
template <class T>
static bool
triangle_intersects_aabb(const vec3<T> &v0, const vec3<T> &v1, const vec3<T> &v2,
                         const vec3<T> &box_min, const vec3<T> &box_max){
    // Translate so the box centre is at the origin.
    const auto centre = (box_min + box_max) * static_cast<T>(0.5);
    const auto half   = (box_max - box_min) * static_cast<T>(0.5);

    const vec3<T> a = v0 - centre;
    const vec3<T> b = v1 - centre;
    const vec3<T> c = v2 - centre;

    // Triangle edges.
    const vec3<T> e0 = b - a;
    const vec3<T> e1 = c - b;
    const vec3<T> e2 = a - c;

    // SAT helper: test separation along axis 'ax'. Returns true if separated.
    auto separated = [&](const vec3<T> &ax) -> bool {
        const T pa = a.Dot(ax);
        const T pb = b.Dot(ax);
        const T pc = c.Dot(ax);
        const T lo = std::min({pa, pb, pc});
        const T hi = std::max({pa, pb, pc});

        const T r = half.x * std::abs(ax.x) + half.y * std::abs(ax.y) + half.z * std::abs(ax.z);
        return (lo > r) || (hi < -r);
    };

    // 1. Box face normals (x, y, z axes).
    if(separated(vec3<T>(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0)))) return false;
    if(separated(vec3<T>(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0)))) return false;
    if(separated(vec3<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)))) return false;

    // 2. Triangle face normal.
    const auto tri_normal = e0.Cross(e1);
    if(separated(tri_normal)) return false;

    // 3. Cross products of box edges x triangle edges (9 axes).
    const vec3<T> box_axes[3] = {
        { static_cast<T>(1), static_cast<T>(0), static_cast<T>(0) },
        { static_cast<T>(0), static_cast<T>(1), static_cast<T>(0) },
        { static_cast<T>(0), static_cast<T>(0), static_cast<T>(1) }
    };
    const vec3<T> tri_edges[3] = { e0, e1, e2 };

    for(int i = 0; i < 3; ++i){
        for(int j = 0; j < 3; ++j){
            const auto ax = box_axes[i].Cross(tri_edges[j]);
            // Skip degenerate axis (edge parallel to box axis).
            if(ax.sq_length() < std::numeric_limits<T>::min()) continue;
            if(separated(ax)) return false;
        }
    }

    return true; // No separating axis found: intersection.
}


// Determine if a ray from 'origin' in direction +X intersects a triangle (v0,v1,v2).
// Returns true if an intersection is found, and sets 't' to the distance along the ray.
// Uses the Möller–Trumbore algorithm.
template <class T>
static bool
ray_intersects_triangle(const vec3<T> &origin,
                        const vec3<T> &v0, const vec3<T> &v1, const vec3<T> &v2,
                        T &t){
    const T eps = std::numeric_limits<T>::epsilon() * static_cast<T>(100);
    const vec3<T> dir(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0));

    const auto e1 = v1 - v0;
    const auto e2 = v2 - v0;

    const auto h = dir.Cross(e2);
    const auto a_det = e1.Dot(h);

    if(std::abs(a_det) < eps) return false; // Ray parallel to triangle.

    const auto f = static_cast<T>(1) / a_det;
    const auto s = origin - v0;
    const auto u = f * s.Dot(h);
    if(u < static_cast<T>(0) || u > static_cast<T>(1)) return false;

    const auto q = s.Cross(e1);
    const auto v = f * dir.Dot(q);
    if(v < static_cast<T>(0) || (u + v) > static_cast<T>(1)) return false;

    t = f * e2.Dot(q);
    return (t > eps);
}


// Point-in-closed-surface test using ray casting along +X.
// Returns true if the point is inside the surface (odd number of intersections).
template <class T, class I>
static bool
point_inside_surface(const vec3<T> &pt,
                     const fv_surface_mesh<T, I> &surface,
                     const octree<T> &tri_index,
                     T domain_extent){
    // Query all triangles whose centroid is within a slab encompassing the entire domain in X.
    const auto search_box = index_bbox<T>(
        vec3<T>(pt.x - domain_extent, pt.y - domain_extent, pt.z - domain_extent),
        vec3<T>(pt.x + domain_extent * static_cast<T>(2), pt.y + domain_extent, pt.z + domain_extent));

    const auto entries = tri_index.search(search_box);

    int64_t crossings = 0;
    for(const auto &entry : entries){
        // Recover triangle index stored in aux_data.
        const auto face_idx = std::any_cast<size_t>(entry.aux_data);
        const auto &face = surface.faces[face_idx];
        if(face.size() < 3) continue;

        T t = static_cast<T>(0);
        if(ray_intersects_triangle(pt,
                                   surface.vertices.at(face[0]),
                                   surface.vertices.at(face[1]),
                                   surface.vertices.at(face[2]),
                                   t)){
            ++crossings;
        }
    }
    return (crossings % 2) != 0;
}

} // anonymous namespace


// ============================================================
// Main conversion function.
// ============================================================
template <class T, class I>
fv_tet_mesh<T, I>
tetrahedral_mesh_from_surface_mesh(const fv_surface_mesh<T, I> &surface_in,
                                   int64_t max_depth,
                                   T boundary_scale){
    if(max_depth < 1 || max_depth > 10){
        throw std::invalid_argument("max_depth must be between 1 and 10");
    }
    if(boundary_scale < static_cast<T>(0)){
        throw std::invalid_argument("boundary_scale must be non-negative");
    }

    // -----------------------------------------------
    // Step 1. Prepare the surface mesh (triangulate).
    // -----------------------------------------------
    fv_surface_mesh<T, I> surface(surface_in);
    surface.convert_to_triangles();
    surface.remove_degenerate_faces();

    if(surface.faces.empty()){
        throw std::invalid_argument("Surface mesh has no valid faces. Cannot continue.");
    }

    // -----------------------------------------------
    // Step 2. Compute a cubic bounding box.
    // -----------------------------------------------
    vec3<T> bb_min(std::numeric_limits<T>::max(),
                   std::numeric_limits<T>::max(),
                   std::numeric_limits<T>::max());
    vec3<T> bb_max(std::numeric_limits<T>::lowest(),
                   std::numeric_limits<T>::lowest(),
                   std::numeric_limits<T>::lowest());

    for(const auto &v : surface.vertices){
        bb_min.x = std::min(bb_min.x, v.x);
        bb_min.y = std::min(bb_min.y, v.y);
        bb_min.z = std::min(bb_min.z, v.z);
        bb_max.x = std::max(bb_max.x, v.x);
        bb_max.y = std::max(bb_max.y, v.y);
        bb_max.z = std::max(bb_max.z, v.z);
    }

    // Make it a cube (equal side lengths) and expand.
    const auto extent = bb_max - bb_min;
    T side = std::max({extent.x, extent.y, extent.z});
    if(side <= static_cast<T>(0)){
        throw std::invalid_argument("Surface mesh is degenerate (zero extent). Cannot continue.");
    }

    const auto expansion = side * boundary_scale;
    const auto centre = (bb_min + bb_max) * static_cast<T>(0.5);
    side += expansion * static_cast<T>(2);

    const vec3<T> domain_min = centre - vec3<T>(side, side, side) * static_cast<T>(0.5);
    const T domain_size = side;

    // -----------------------------------------------
    // Step 3. Index surface triangles using the Ygor octree.
    // -----------------------------------------------
    // We insert triangle centroids with the face index as auxiliary data.
    octree<T> tri_index;
    for(size_t fi = 0; fi < surface.faces.size(); ++fi){
        const auto &face = surface.faces[fi];
        if(face.size() < 3) continue;
        const auto &va = surface.vertices.at(face[0]);
        const auto &vb = surface.vertices.at(face[1]);
        const auto &vc = surface.vertices.at(face[2]);
        const auto centroid = (va + vb + vc) / static_cast<T>(3);
        tri_index.insert(centroid, static_cast<size_t>(fi));
    }

    // -----------------------------------------------
    // Step 4. Build adaptive octree decomposition.
    // -----------------------------------------------
    // Leaf cells stored in a set for O(log n) lookup/insertion.
    std::set<decomp_cell> leaf_cells;
    leaf_cells.insert({0, 0, 0, 0}); // Root cell.

    // Helper: compute axis-aligned bounding box of a cell.
    auto cell_min = [&](const decomp_cell &c) -> vec3<T> {
        const T cs = domain_size / static_cast<T>(static_cast<int64_t>(1) << c.level);
        return domain_min + vec3<T>(static_cast<T>(c.ix) * cs,
                                    static_cast<T>(c.iy) * cs,
                                    static_cast<T>(c.iz) * cs);
    };
    auto cell_max = [&](const decomp_cell &c) -> vec3<T> {
        const T cs = domain_size / static_cast<T>(static_cast<int64_t>(1) << c.level);
        return domain_min + vec3<T>(static_cast<T>(c.ix + 1) * cs,
                                    static_cast<T>(c.iy + 1) * cs,
                                    static_cast<T>(c.iz + 1) * cs);
    };
    auto cell_centre = [&](const decomp_cell &c) -> vec3<T> {
        return (cell_min(c) + cell_max(c)) * static_cast<T>(0.5);
    };

    // Helper: subdivide a cell into 8 children.
    auto subdivide_cell = [&](const decomp_cell &c) -> std::array<decomp_cell, 8> {
        const int64_t nl = c.level + 1;
        const int64_t bx = c.ix * 2;
        const int64_t by = c.iy * 2;
        const int64_t bz = c.iz * 2;
        return {{
            {nl, bx,   by,   bz  },
            {nl, bx+1, by,   bz  },
            {nl, bx,   by+1, bz  },
            {nl, bx+1, by+1, bz  },
            {nl, bx,   by,   bz+1},
            {nl, bx+1, by,   bz+1},
            {nl, bx,   by+1, bz+1},
            {nl, bx+1, by+1, bz+1}
        }};
    };

    // Iteratively refine cells that intersect surface triangles.
    for(int64_t depth = 0; depth < max_depth; ++depth){
        std::vector<decomp_cell> to_refine;
        for(const auto &c : leaf_cells){
            if(c.level != depth) continue;

            const auto cmin = cell_min(c);
            const auto cmax = cell_max(c);

            // Use the octree to find candidate triangles for this cell.
            // NOTE: A centroid-based radius of one cell diagonal can miss large
            // triangles whose centroids lie outside that radius but still
            // intersect the cell. Use a conservative (effectively unbounded)
            // radius here and rely on the precise AABB intersection test below
            // to filter out non-intersecting triangles.
            const auto cc = cell_centre(c);
            const auto candidates = tri_index.search_radius(
                                        cc,
                                        std::numeric_limits<double>::infinity());

            bool intersects = false;
            for(const auto &entry : candidates){
                const auto face_idx = std::any_cast<size_t>(entry.aux_data);
                const auto &face = surface.faces[face_idx];
                if(face.size() < 3) continue;
                if(triangle_intersects_aabb(surface.vertices.at(face[0]),
                                            surface.vertices.at(face[1]),
                                            surface.vertices.at(face[2]),
                                            cmin, cmax)){
                    intersects = true;
                    break;
                }
            }
            if(intersects){
                to_refine.push_back(c);
            }
        }
        for(const auto &c : to_refine){
            leaf_cells.erase(c);
            for(const auto &child : subdivide_cell(c)){
                leaf_cells.insert(child);
            }
        }
    }

    // -----------------------------------------------
    // Step 5. 2:1 balance the octree.
    // -----------------------------------------------
    // Ensure no face-adjacent leaf cells differ by more than one refinement level.
    // The 6 face-neighbor offsets at the same level.
    auto face_neighbors = [](const decomp_cell &c) -> std::array<decomp_cell, 6> {
        return {{
            {c.level, c.ix-1, c.iy,   c.iz  },
            {c.level, c.ix+1, c.iy,   c.iz  },
            {c.level, c.ix,   c.iy-1, c.iz  },
            {c.level, c.ix,   c.iy+1, c.iz  },
            {c.level, c.ix,   c.iy,   c.iz-1},
            {c.level, c.ix,   c.iy,   c.iz+1}
        }};
    };

    // Helper: find the leaf cell containing a given point.
    // Returns true if found, and sets 'out' to the leaf cell.
    auto find_leaf_at = [&](const vec3<T> &pt, decomp_cell &out) -> bool {
        // Start at the root and descend.
        decomp_cell cur{0, 0, 0, 0};
        for(;;){
            if(leaf_cells.count(cur) != 0){
                out = cur;
                return true;
            }
            if(cur.level >= max_depth) return false;

            const auto cc = cell_centre(cur);
            int64_t bx = cur.ix * 2 + ((pt.x >= cc.x) ? 1 : 0);
            int64_t by = cur.iy * 2 + ((pt.y >= cc.y) ? 1 : 0);
            int64_t bz = cur.iz * 2 + ((pt.z >= cc.z) ? 1 : 0);
            cur = {cur.level + 1, bx, by, bz};
        }
    };

    // Helper: check if a face neighbor position is within the domain.
    auto cell_in_domain = [&](const decomp_cell &c) -> bool {
        const int64_t dim = static_cast<int64_t>(1) << c.level;
        return (c.ix >= 0 && c.ix < dim &&
                c.iy >= 0 && c.iy < dim &&
                c.iz >= 0 && c.iz < dim);
    };

    // Balance by iterating until no more refinements are needed.
    {
        bool changed = true;
        while(changed){
            changed = false;
            std::vector<decomp_cell> to_refine;

            for(const auto &c : leaf_cells){
                if(c.level >= max_depth) continue;

                for(const auto &fn : face_neighbors(c)){
                    if(!cell_in_domain(fn)) continue;

                    // Check if the neighbor position is occupied by a much finer cell.
                    // Find a point inside the neighbor and look up its leaf cell.
                    const auto fn_centre_pt = cell_centre(fn);
                    decomp_cell neighbor_leaf;
                    if(find_leaf_at(fn_centre_pt, neighbor_leaf)){
                        if(neighbor_leaf.level > c.level + 1){
                            to_refine.push_back(c);
                            break; // Only need to refine once.
                        }
                    }
                }
            }

            for(const auto &c : to_refine){
                if(leaf_cells.count(c) == 0) continue; // Already refined.
                leaf_cells.erase(c);
                for(const auto &child : subdivide_cell(c)){
                    leaf_cells.insert(child);
                }
                changed = true;
            }
        }
    }

    // -----------------------------------------------
    // Step 6. Classify cells (inside / outside).
    // -----------------------------------------------
    std::set<decomp_cell> inside_cells;
    for(const auto &c : leaf_cells){
        const auto cc = cell_centre(c);
        if(point_inside_surface(cc, surface, tri_index, domain_size)){
            inside_cells.insert(c);
        }
    }

    if(inside_cells.empty()){
        throw std::runtime_error("No interior cells found. The surface may not be watertight or consistently oriented.");
    }

    // -----------------------------------------------
    // Step 7. Decompose inside cells into tetrahedra.
    // -----------------------------------------------
    // We use a body-centred decomposition that naturally handles transitions between refinement levels.
    //
    // For each face of each cell:
    //   - Determine if the face neighbor is at the same level, a finer level, or absent (boundary).
    //   - Triangulate the face accordingly, then connect each face triangle to the cell body centre.
    //
    // Face triangulation rules:
    //   (a) Same-level or boundary face (4 corners): fan from face centre → 4 triangles.
    //   (b) Coarser-to-finer face (this cell has finer neighbors): the face has 9 vertices
    //       (4 corners + 4 edge midpoints + face centre). Fan from face centre → 8 triangles.
    //   (c) Finer-to-coarser face (this cell is finer, neighbor is coarser): split the face
    //       using the diagonal through the vertex that is the coarse cell's face centre → 2 triangles.
    //       This ensures conformity with rule (b) on the coarser side.
    //
    // Each face triangle + the cell body centre → 1 tetrahedron.
    //
    fv_tet_mesh<T, I> tet_mesh;

    // Global vertex map: position → index. Uses a grid-snapped key to find coincident vertices.
    const T snap_eps = domain_size * std::numeric_limits<T>::epsilon() * static_cast<T>(16);
    std::map<std::tuple<int64_t,int64_t,int64_t>, I> vertex_map;

    auto snap_key = [&](const vec3<T> &p) -> std::tuple<int64_t,int64_t,int64_t> {
        // Snap to a fine grid to identify coincident vertices.
        const T inv_eps = static_cast<T>(1) / snap_eps;
        return { static_cast<int64_t>(std::round((p.x - domain_min.x) * inv_eps)),
                 static_cast<int64_t>(std::round((p.y - domain_min.y) * inv_eps)),
                 static_cast<int64_t>(std::round((p.z - domain_min.z) * inv_eps)) };
    };

    auto get_or_add_vertex = [&](const vec3<T> &p) -> I {
        const auto key = snap_key(p);
        auto it = vertex_map.find(key);
        if(it != vertex_map.end()) return it->second;
        const auto idx = static_cast<I>(tet_mesh.vertices.size());
        tet_mesh.vertices.push_back(p);
        vertex_map[key] = idx;
        return idx;
    };

    // Helper: add a tetrahedron, checking for degeneracy.
    auto add_tet = [&](I a, I b, I c, I d){
        if(a == b || a == c || a == d || b == c || b == d || c == d) return;
        tet_mesh.tetrahedra.push_back({{a, b, c, d}});
    };

    // Helper: determine the face neighbor type for a cell face.
    // Returns:  0 = same-level or boundary (rule a)
    //           1 = this cell is coarser, neighbor is finer (rule b)
    //          -1 = this cell is finer, neighbor is coarser (rule c)
    // For rule (c), sets 'inner_corner_idx' to the index (0..3) of the face corner
    // that corresponds to the coarse cell's face centre.
    auto classify_face = [&](const decomp_cell &c, const decomp_cell &fn,
                             const vec3<T> fc[4],
                             int &inner_corner_idx) -> int {
        inner_corner_idx = -1;

        if(!cell_in_domain(fn)){
            return 0; // Boundary face.
        }

        // Find the leaf cell at the face neighbor centre.
        const auto fn_centre_pt = cell_centre(fn);
        decomp_cell neighbor_leaf;
        if(!find_leaf_at(fn_centre_pt, neighbor_leaf)){
            return 0; // Should not happen; treat as boundary.
        }

        if(neighbor_leaf.level == c.level){
            return 0; // Same level.
        }
        if(neighbor_leaf.level > c.level){
            return 1; // Neighbor is finer (this cell is coarser).
        }

        // Neighbor is coarser (this cell is finer). Determine inner corner.
        // The inner corner is the face corner that coincides with the coarse cell's
        // face centre. Use the actual coarser neighbor leaf bounds to compute this.
        const auto coarse_cmin = cell_min(neighbor_leaf);
        const auto coarse_cmax = cell_max(neighbor_leaf);
        const auto coarse_face_centre = (coarse_cmin + coarse_cmax) * static_cast<T>(0.5);

        // Find the face corner closest to the coarse face centre.
        T best_dist = std::numeric_limits<T>::max();
        inner_corner_idx = 0;
        for(int j = 0; j < 4; ++j){
            const T d = fc[j].sq_dist(coarse_face_centre);
            if(d < best_dist){
                best_dist = d;
                inner_corner_idx = j;
            }
        }

        return -1;
    };

    // Process each inside cell.
    for(const auto &c : inside_cells){
        const auto cmin = cell_min(c);
        const auto cmax = cell_max(c);
        const auto bc = cell_centre(c); // Body centre.
        const I bc_idx = get_or_add_vertex(bc);

        // The 8 corners of the cell.
        //   Corner ordering:
        //     0=(xmin,ymin,zmin), 1=(xmax,ymin,zmin), 2=(xmin,ymax,zmin), 3=(xmax,ymax,zmin)
        //     4=(xmin,ymin,zmax), 5=(xmax,ymin,zmax), 6=(xmin,ymax,zmax), 7=(xmax,ymax,zmax)
        const vec3<T> corners[8] = {
            {cmin.x, cmin.y, cmin.z},
            {cmax.x, cmin.y, cmin.z},
            {cmin.x, cmax.y, cmin.z},
            {cmax.x, cmax.y, cmin.z},
            {cmin.x, cmin.y, cmax.z},
            {cmax.x, cmin.y, cmax.z},
            {cmin.x, cmax.y, cmax.z},
            {cmax.x, cmax.y, cmax.z}
        };

        // The 6 faces, each defined by 4 corner indices (counter-clockwise when viewed from outside).
        // Along with the face-neighbor offset direction.
        struct face_def {
            int ci[4];       // Corner indices (into corners[]).
            decomp_cell fn;  // Face neighbor cell at the same level.
        };
        const face_def face_defs[6] = {
            {{0, 4, 6, 2}, {c.level, c.ix-1, c.iy,   c.iz  }}, // -X face
            {{1, 3, 7, 5}, {c.level, c.ix+1, c.iy,   c.iz  }}, // +X face
            {{0, 1, 5, 4}, {c.level, c.ix,   c.iy-1, c.iz  }}, // -Y face
            {{2, 6, 7, 3}, {c.level, c.ix,   c.iy+1, c.iz  }}, // +Y face
            {{0, 2, 3, 1}, {c.level, c.ix,   c.iy,   c.iz-1}}, // -Z face
            {{4, 5, 7, 6}, {c.level, c.ix,   c.iy,   c.iz+1}}  // +Z face
        };

        for(int fi = 0; fi < 6; ++fi){
            const auto &fd = face_defs[fi];

            // Get face corner positions.
            const vec3<T> fc[4] = {
                corners[fd.ci[0]], corners[fd.ci[1]],
                corners[fd.ci[2]], corners[fd.ci[3]]
            };

            int inner_corner = -1;
            const int face_type = classify_face(c, fd.fn, fc, inner_corner);

            if(face_type == 0){
                // Rule (a): Same level or boundary. Fan from face centre → 4 triangles.
                const auto face_centre = (fc[0] + fc[1] + fc[2] + fc[3]) * static_cast<T>(0.25);
                const I fc_idx = get_or_add_vertex(face_centre);
                const I ci0 = get_or_add_vertex(fc[0]);
                const I ci1 = get_or_add_vertex(fc[1]);
                const I ci2 = get_or_add_vertex(fc[2]);
                const I ci3 = get_or_add_vertex(fc[3]);

                add_tet(bc_idx, ci0, ci1, fc_idx);
                add_tet(bc_idx, ci1, ci2, fc_idx);
                add_tet(bc_idx, ci2, ci3, fc_idx);
                add_tet(bc_idx, ci3, ci0, fc_idx);

            }else if(face_type == 1){
                // Rule (b): This cell is coarser, neighbor is finer.
                // Face has 9 vertices: 4 corners + 4 edge midpoints + face centre.
                const auto face_centre = (fc[0] + fc[1] + fc[2] + fc[3]) * static_cast<T>(0.25);
                const vec3<T> edge_mids[4] = {
                    (fc[0] + fc[1]) * static_cast<T>(0.5),
                    (fc[1] + fc[2]) * static_cast<T>(0.5),
                    (fc[2] + fc[3]) * static_cast<T>(0.5),
                    (fc[3] + fc[0]) * static_cast<T>(0.5)
                };

                const I fc_ctr = get_or_add_vertex(face_centre);
                I ci[4], mi[4];
                for(int j = 0; j < 4; ++j){
                    ci[j] = get_or_add_vertex(fc[j]);
                    mi[j] = get_or_add_vertex(edge_mids[j]);
                }

                // 8-triangle fan from face centre through corners and edge midpoints.
                add_tet(bc_idx, ci[0], mi[0], fc_ctr);
                add_tet(bc_idx, mi[0], ci[1], fc_ctr);
                add_tet(bc_idx, ci[1], mi[1], fc_ctr);
                add_tet(bc_idx, mi[1], ci[2], fc_ctr);
                add_tet(bc_idx, ci[2], mi[2], fc_ctr);
                add_tet(bc_idx, mi[2], ci[3], fc_ctr);
                add_tet(bc_idx, ci[3], mi[3], fc_ctr);
                add_tet(bc_idx, mi[3], ci[0], fc_ctr);

            }else{
                // Rule (c): This cell is finer, neighbor is coarser.
                // inner_corner was already computed by classify_face() using the actual
                // coarser neighbor leaf bounds.

                // Split into 2 triangles using diagonal from inner corner to the opposite corner.
                const int opp = (inner_corner + 2) % 4;
                const int prev = (inner_corner + 3) % 4;
                const int next = (inner_corner + 1) % 4;

                const I ci_inner = get_or_add_vertex(fc[inner_corner]);
                const I ci_opp   = get_or_add_vertex(fc[opp]);
                const I ci_prev  = get_or_add_vertex(fc[prev]);
                const I ci_next  = get_or_add_vertex(fc[next]);

                add_tet(bc_idx, ci_inner, ci_next, ci_opp);
                add_tet(bc_idx, ci_inner, ci_opp,  ci_prev);
            }
        }
    }

    // -----------------------------------------------
    // Step 8. Final cleanup.
    // -----------------------------------------------
    tet_mesh.remove_disconnected_vertices();

    return tet_mesh;
}


// Explicit template instantiations.
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template fv_tet_mesh<float , uint32_t > tetrahedral_mesh_from_surface_mesh(const fv_surface_mesh<float , uint32_t > &, int64_t, float );
    template fv_tet_mesh<float , uint64_t > tetrahedral_mesh_from_surface_mesh(const fv_surface_mesh<float , uint64_t > &, int64_t, float );

    template fv_tet_mesh<double, uint32_t > tetrahedral_mesh_from_surface_mesh(const fv_surface_mesh<double, uint32_t > &, int64_t, double);
    template fv_tet_mesh<double, uint64_t > tetrahedral_mesh_from_surface_mesh(const fv_surface_mesh<double, uint64_t > &, int64_t, double);
#endif
