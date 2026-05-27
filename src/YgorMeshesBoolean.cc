//YgorMeshesBoolean.cc - Written by hal clark in 2026.

#include <algorithm>
#include <any>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorIndex.h"
#include "YgorIndexRTree.h"
#include "YgorMath.h"
#include "YgorMeshesBoolean.h"
#include "YgorMeshesOrient.h"

namespace {

template <class T, class I>
struct prepared_mesh {
    fv_surface_mesh<T, I> mesh;
    std::unique_ptr<rtree<T>> face_index;
    index_bbox<T> bounds;
};

template <class I>
std::pair<I, I>
make_undirected_edge(I a, I b){
    if(b < a){
        std::swap(a, b);
    }
    return std::make_pair(a, b);
}

template <class T, class I>
index_bbox<T>
triangle_bbox(const fv_surface_mesh<T, I> &mesh,
              size_t face_idx){
    const auto &f = mesh.faces.at(face_idx);
    const auto &a = mesh.vertices.at(f.at(0));
    const auto &b = mesh.vertices.at(f.at(1));
    const auto &c = mesh.vertices.at(f.at(2));

    vec3<T> bb_min(std::min({a.x, b.x, c.x}),
                   std::min({a.y, b.y, c.y}),
                   std::min({a.z, b.z, c.z}));
    vec3<T> bb_max(std::max({a.x, b.x, c.x}),
                   std::max({a.y, b.y, c.y}),
                   std::max({a.z, b.z, c.z}));
    return index_bbox<T>(bb_min, bb_max);
}

template <class T, class I>
index_bbox<T>
mesh_bbox(const fv_surface_mesh<T, I> &mesh){
    if(mesh.vertices.empty()){
        return index_bbox<T>();
    }

    vec3<T> bb_min(std::numeric_limits<T>::max(),
                   std::numeric_limits<T>::max(),
                   std::numeric_limits<T>::max());
    vec3<T> bb_max(std::numeric_limits<T>::lowest(),
                   std::numeric_limits<T>::lowest(),
                   std::numeric_limits<T>::lowest());

    for(const auto &v : mesh.vertices){
        bb_min.x = std::min(bb_min.x, v.x);
        bb_min.y = std::min(bb_min.y, v.y);
        bb_min.z = std::min(bb_min.z, v.z);
        bb_max.x = std::max(bb_max.x, v.x);
        bb_max.y = std::max(bb_max.y, v.y);
        bb_max.z = std::max(bb_max.z, v.z);
    }

    return index_bbox<T>(bb_min, bb_max);
}

template <class T, class I>
void
validate_closed_triangular_mesh(const fv_surface_mesh<T, I> &mesh,
                                const std::string &name){
    if(mesh.faces.empty()){
        return;
    }

    std::map<std::pair<I, I>, size_t> edge_counts;
    for(const auto &v : mesh.vertices){
        if(!v.isfinite()){
            throw std::invalid_argument(name + " contains a non-finite vertex.");
        }
    }

    for(size_t face_idx = 0; face_idx < mesh.faces.size(); ++face_idx){
        const auto &face = mesh.faces.at(face_idx);
        if(face.size() != 3UL){
            throw std::invalid_argument(name + " must contain only triangular faces.");
        }
        for(const auto vi : face){
            if(vi >= mesh.vertices.size()){
                throw std::invalid_argument(name + " contains an out-of-range face index.");
            }
        }
        if((face[0] == face[1]) || (face[1] == face[2]) || (face[2] == face[0])){
            throw std::invalid_argument(name + " contains a degenerate triangle.");
        }

        edge_counts[make_undirected_edge(face[0], face[1])] += 1UL;
        edge_counts[make_undirected_edge(face[1], face[2])] += 1UL;
        edge_counts[make_undirected_edge(face[2], face[0])] += 1UL;
    }

    if((mesh.faces.size() % 2UL) != 0UL){
        throw std::invalid_argument(name + " violates the manifold edge handshake invariant.");
    }

    const auto expected_edges = (mesh.faces.size() * 3UL) / 2UL;
    if(edge_counts.size() != expected_edges){
        throw std::invalid_argument(name + " is not a closed manifold mesh.");
    }

    for(const auto &ep : edge_counts){
        if(ep.second != 2UL){
            throw std::invalid_argument(name + " contains a boundary or non-manifold edge.");
        }
    }
}

template <class T>
T
mesh_coord_eps(const index_bbox<T> &bounds){
    const auto extent = bounds.max - bounds.min;
    const auto scale = std::max({extent.x, extent.y, extent.z, static_cast<T>(1)});
    return std::sqrt(std::numeric_limits<T>::epsilon()) * scale;
}

template <class T>
int
dominant_axis(const vec3<T> &n){
    const auto ax = std::abs(n.x);
    const auto ay = std::abs(n.y);
    const auto az = std::abs(n.z);
    if((ax >= ay) && (ax >= az)) return 0;
    if(ay >= az) return 1;
    return 2;
}

template <class T>
vec2<T>
project_drop_axis(const vec3<T> &v,
                  int axis){
    if(axis == 0) return vec2<T>(v.y, v.z);
    if(axis == 1) return vec2<T>(v.x, v.z);
    return vec2<T>(v.x, v.y);
}

template <class T>
bool
point_on_triangle_boundary(const vec3<T> &p,
                           const vec3<T> &a,
                           const vec3<T> &b,
                           const vec3<T> &c){
    if(orient_sign(a, b, c, p) != 0){
        return false;
    }

    const auto normal = (b - a).Cross(c - a);
    const auto axis = dominant_axis(normal);
    const auto pp = project_drop_axis(p, axis);
    const auto pa = project_drop_axis(a, axis);
    const auto pb = project_drop_axis(b, axis);
    const auto pc = project_drop_axis(c, axis);

    if(!point_in_triangle_or_on_boundary(pp, pa, pb, pc)){
        return false;
    }

    return point_on_closed_segment(pp, pa, pb)
        || point_on_closed_segment(pp, pb, pc)
        || point_on_closed_segment(pp, pc, pa);
}

template <class T>
bool
segment_intersects_triangle_interior(const vec3<T> &p,
                                     const vec3<T> &q,
                                     const vec3<T> &a,
                                     const vec3<T> &b,
                                     const vec3<T> &c){
    const auto s_plane_p = orient_sign(a, b, c, p);
    const auto s_plane_q = orient_sign(a, b, c, q);

    if((s_plane_p == 0) || (s_plane_q == 0)){
        return false;
    }
    if((s_plane_p > 0) == (s_plane_q > 0)){
        return false;
    }

    const auto s1 = orient_sign(p, q, a, b);
    const auto s2 = orient_sign(p, q, b, c);
    const auto s3 = orient_sign(p, q, c, a);

    if((s1 == 0) || (s2 == 0) || (s3 == 0)){
        return false;
    }

    const bool all_pos = (s1 > 0) && (s2 > 0) && (s3 > 0);
    const bool all_neg = (s1 < 0) && (s2 < 0) && (s3 < 0);
    return all_pos || all_neg;
}

template <class T, class I>
bool
point_on_mesh_boundary(const vec3<T> &p,
                       const prepared_mesh<T, I> &prep){
    const auto candidates = prep.face_index->search(index_bbox<T>(p, p));
    for(const auto &entry : candidates){
        const auto face_idx = std::any_cast<size_t>(entry.aux_data);
        const auto &face = prep.mesh.faces.at(face_idx);
        if(point_on_triangle_boundary(p,
                                      prep.mesh.vertices.at(face.at(0)),
                                      prep.mesh.vertices.at(face.at(1)),
                                      prep.mesh.vertices.at(face.at(2)))){
            return true;
        }
    }
    return false;
}

template <class T, class I>
bool
cast_parity_ray(const vec3<T> &p,
                const vec3<T> &q,
                const prepared_mesh<T, I> &prep){
    const auto query_box = index_bbox<T>(p, q);
    const auto candidates = prep.face_index->search(query_box);

    int64_t crossings = 0;
    for(const auto &entry : candidates){
        const auto face_idx = std::any_cast<size_t>(entry.aux_data);
        const auto &face = prep.mesh.faces.at(face_idx);
        const auto &a = prep.mesh.vertices.at(face.at(0));
        const auto &b = prep.mesh.vertices.at(face.at(1));
        const auto &c = prep.mesh.vertices.at(face.at(2));

        if(segment_intersects_triangle_interior(p, q, a, b, c)){
            ++crossings;
        }
    }

    return (crossings % 2LL) != 0LL;
}

template <class T, class I>
bool
point_inside_mesh(const vec3<T> &p,
                  const prepared_mesh<T, I> &prep,
                  T far_distance){
    if(prep.mesh.faces.empty()){
        return false;
    }
    if(point_on_mesh_boundary(p, prep)){
        return true;
    }

    const std::array<vec3<T>, 3> ray_dirs = {{
        vec3<T>(static_cast<T>(1.0), static_cast<T>(0.371), static_cast<T>(0.529)),
        vec3<T>(static_cast<T>(0.419), static_cast<T>(1.0), static_cast<T>(0.113)),
        vec3<T>(static_cast<T>(0.137), static_cast<T>(0.271), static_cast<T>(1.0))
    }};

    int inside_votes = 0;
    for(const auto &dir_raw : ray_dirs){
        const auto dir = dir_raw.unit();
        const auto q = p + dir * far_distance;
        if(cast_parity_ray(p, q, prep)){
            ++inside_votes;
        }
    }

    return inside_votes >= 2;
}

template <class T>
bool
triangle_intersects_aabb(const vec3<T> &v0,
                         const vec3<T> &v1,
                         const vec3<T> &v2,
                         const vec3<T> &box_min,
                         const vec3<T> &box_max){
    const auto centre = (box_min + box_max) * static_cast<T>(0.5);
    const auto half = (box_max - box_min) * static_cast<T>(0.5);

    const vec3<T> a = v0 - centre;
    const vec3<T> b = v1 - centre;
    const vec3<T> c = v2 - centre;

    const vec3<T> e0 = b - a;
    const vec3<T> e1 = c - b;
    const vec3<T> e2 = a - c;

    auto separated = [&](const vec3<T> &axis) -> bool {
        const auto pa = a.Dot(axis);
        const auto pb = b.Dot(axis);
        const auto pc = c.Dot(axis);
        const auto lo = std::min({pa, pb, pc});
        const auto hi = std::max({pa, pb, pc});
        const auto r = half.x * std::abs(axis.x)
                     + half.y * std::abs(axis.y)
                     + half.z * std::abs(axis.z);
        return (lo > r) || (hi < -r);
    };

    if(separated(vec3<T>(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0)))) return false;
    if(separated(vec3<T>(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0)))) return false;
    if(separated(vec3<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1)))) return false;

    const auto tri_normal = e0.Cross(e1);
    if(separated(tri_normal)) return false;

    const vec3<T> box_axes[3] = {
        vec3<T>(static_cast<T>(1), static_cast<T>(0), static_cast<T>(0)),
        vec3<T>(static_cast<T>(0), static_cast<T>(1), static_cast<T>(0)),
        vec3<T>(static_cast<T>(0), static_cast<T>(0), static_cast<T>(1))
    };
    const vec3<T> tri_edges[3] = { e0, e1, e2 };

    for(int i = 0; i < 3; ++i){
        for(int j = 0; j < 3; ++j){
            const auto axis = box_axes[i].Cross(tri_edges[j]);
            if(axis.sq_length() <= std::numeric_limits<T>::min()){
                continue;
            }
            if(separated(axis)) return false;
        }
    }

    return true;
}

template <class T, class I>
bool
cell_touches_mesh(const vec3<T> &cell_min,
                  const vec3<T> &cell_max,
                  const prepared_mesh<T, I> &prep){
    const auto candidates = prep.face_index->search(index_bbox<T>(cell_min, cell_max));
    for(const auto &entry : candidates){
        const auto face_idx = std::any_cast<size_t>(entry.aux_data);
        const auto &face = prep.mesh.faces.at(face_idx);
        if(triangle_intersects_aabb(prep.mesh.vertices.at(face.at(0)),
                                    prep.mesh.vertices.at(face.at(1)),
                                    prep.mesh.vertices.at(face.at(2)),
                                    cell_min,
                                    cell_max)){
            return true;
        }
    }
    return false;
}

template <class T, class I>
prepared_mesh<T, I>
prepare_mesh(const fv_surface_mesh<T, I> &input,
             const std::string &name){
    prepared_mesh<T, I> out;
    out.mesh = input;
    out.mesh.convert_to_triangles();
    out.mesh.remove_degenerate_faces();
    out.face_index = std::make_unique<rtree<T>>();

    if(out.mesh.faces.empty()){
        out.bounds = mesh_bbox(out.mesh);
        return out;
    }

    validate_closed_triangular_mesh(out.mesh, name);
    out.bounds = mesh_bbox(out.mesh);

    const auto orient_eps = mesh_coord_eps(out.bounds);
    if(!OrientFaces(out.mesh, orient_eps)){
        throw std::invalid_argument(name + " could not be consistently oriented.");
    }

    out.mesh.recreate_involved_face_index();
    for(size_t face_idx = 0; face_idx < out.mesh.faces.size(); ++face_idx){
        out.face_index->insert(triangle_bbox(out.mesh, face_idx), face_idx);
    }
    return out;
}

template <class T, class I>
fv_surface_mesh<T, I>
copy_mesh_for_empty_case(const fv_surface_mesh<T, I> &input){
    fv_surface_mesh<T, I> out(input);
    out.convert_to_triangles();
    out.remove_degenerate_faces();
    out.remove_disconnected_vertices();
    if(!out.faces.empty()){
        const auto bounds = mesh_bbox(out);
        const auto orient_eps = mesh_coord_eps(bounds);
        if(!OrientFaces(out, orient_eps)){
            throw std::invalid_argument("empty-case mesh could not be consistently oriented.");
        }
        out.recreate_involved_face_index();
        out.compute_vertex_normals();
    }
    return out;
}

template <class T>
bool
eval_boolean(bool inside_lhs,
             bool inside_rhs,
             MeshBooleanOperation op){
    if(op == MeshBooleanOperation::Union) return inside_lhs || inside_rhs;
    if(op == MeshBooleanOperation::Intersection) return inside_lhs && inside_rhs;
    if(op == MeshBooleanOperation::Exclusion) return inside_lhs != inside_rhs;
    return inside_lhs && !inside_rhs;
}

template <class T, class I>
fv_surface_mesh<T, I>
emit_boundary_mesh(const std::vector<uint8_t> &occupied,
                   size_t resolution,
                   const vec3<T> &domain_min,
                   T h){
    fv_surface_mesh<T, I> out;
    if(occupied.empty()){
        return out;
    }

    auto linear_index = [resolution](size_t i, size_t j, size_t k) -> size_t {
        return (k * resolution + j) * resolution + i;
    };
    auto cell_is_occupied = [&](int64_t i, int64_t j, int64_t k) -> bool {
        if((i < 0) || (j < 0) || (k < 0)) return false;
        if(i >= static_cast<int64_t>(resolution)
        || j >= static_cast<int64_t>(resolution)
        || k >= static_cast<int64_t>(resolution)){
            return false;
        }
        return occupied.at(linear_index(static_cast<size_t>(i),
                                        static_cast<size_t>(j),
                                        static_cast<size_t>(k))) != 0U;
    };

    std::map<std::tuple<int64_t, int64_t, int64_t>, I> vertex_map;
    auto vertex_for = [&](int64_t i, int64_t j, int64_t k) -> I {
        const auto key = std::make_tuple(i, j, k);
        const auto it = vertex_map.find(key);
        if(it != vertex_map.end()){
            return it->second;
        }

        const auto idx = static_cast<I>(out.vertices.size());
        out.vertices.emplace_back(domain_min.x + h * static_cast<T>(i),
                                  domain_min.y + h * static_cast<T>(j),
                                  domain_min.z + h * static_cast<T>(k));
        vertex_map.emplace(key, idx);
        return idx;
    };

    auto emit_quad = [&](I a, I b, I c, I d) {
        out.faces.push_back({ a, b, c });
        out.faces.push_back({ a, c, d });
    };

    for(size_t k = 0; k < resolution; ++k){
        for(size_t j = 0; j < resolution; ++j){
            for(size_t i = 0; i < resolution; ++i){
                if(!occupied.at(linear_index(i, j, k))){
                    continue;
                }

                const auto v000 = vertex_for(static_cast<int64_t>(i),     static_cast<int64_t>(j),     static_cast<int64_t>(k));
                const auto v100 = vertex_for(static_cast<int64_t>(i) + 1, static_cast<int64_t>(j),     static_cast<int64_t>(k));
                const auto v010 = vertex_for(static_cast<int64_t>(i),     static_cast<int64_t>(j) + 1, static_cast<int64_t>(k));
                const auto v110 = vertex_for(static_cast<int64_t>(i) + 1, static_cast<int64_t>(j) + 1, static_cast<int64_t>(k));
                const auto v001 = vertex_for(static_cast<int64_t>(i),     static_cast<int64_t>(j),     static_cast<int64_t>(k) + 1);
                const auto v101 = vertex_for(static_cast<int64_t>(i) + 1, static_cast<int64_t>(j),     static_cast<int64_t>(k) + 1);
                const auto v011 = vertex_for(static_cast<int64_t>(i),     static_cast<int64_t>(j) + 1, static_cast<int64_t>(k) + 1);
                const auto v111 = vertex_for(static_cast<int64_t>(i) + 1, static_cast<int64_t>(j) + 1, static_cast<int64_t>(k) + 1);

                if(!cell_is_occupied(static_cast<int64_t>(i) - 1, static_cast<int64_t>(j), static_cast<int64_t>(k))){
                    emit_quad(v000, v001, v011, v010);
                }
                if(!cell_is_occupied(static_cast<int64_t>(i) + 1, static_cast<int64_t>(j), static_cast<int64_t>(k))){
                    emit_quad(v100, v110, v111, v101);
                }
                if(!cell_is_occupied(static_cast<int64_t>(i), static_cast<int64_t>(j) - 1, static_cast<int64_t>(k))){
                    emit_quad(v000, v100, v101, v001);
                }
                if(!cell_is_occupied(static_cast<int64_t>(i), static_cast<int64_t>(j) + 1, static_cast<int64_t>(k))){
                    emit_quad(v010, v011, v111, v110);
                }
                if(!cell_is_occupied(static_cast<int64_t>(i), static_cast<int64_t>(j), static_cast<int64_t>(k) - 1)){
                    emit_quad(v000, v010, v110, v100);
                }
                if(!cell_is_occupied(static_cast<int64_t>(i), static_cast<int64_t>(j), static_cast<int64_t>(k) + 1)){
                    emit_quad(v001, v101, v111, v011);
                }
            }
        }
    }

    out.remove_disconnected_vertices();
    out.recreate_involved_face_index();
    out.compute_vertex_normals();
    return out;
}

template <class T, class I>
fv_surface_mesh<T, I>
boolean_mesh_op_impl(const fv_surface_mesh<T, I> &lhs,
                     const fv_surface_mesh<T, I> &rhs,
                     MeshBooleanOperation op,
                     int64_t max_depth,
                     T boundary_scale){
    if((max_depth < 1) || (max_depth > 8)){
        throw std::invalid_argument("max_depth must be between 1 and 8.");
    }
    if(boundary_scale < static_cast<T>(0)){
        throw std::invalid_argument("boundary_scale must be non-negative.");
    }

    if(lhs.faces.empty() && rhs.faces.empty()){
        return fv_surface_mesh<T, I>();
    }
    if(lhs.faces.empty()){
        if(op == MeshBooleanOperation::Intersection || op == MeshBooleanOperation::Subtraction){
            return fv_surface_mesh<T, I>();
        }
        const auto rhs_prep = prepare_mesh(rhs, "rhs mesh");
        return copy_mesh_for_empty_case(rhs_prep.mesh);
    }
    if(rhs.faces.empty()){
        if(op == MeshBooleanOperation::Intersection){
            return fv_surface_mesh<T, I>();
        }
        const auto lhs_prep = prepare_mesh(lhs, "lhs mesh");
        return copy_mesh_for_empty_case(lhs_prep.mesh);
    }

    const auto lhs_prep = prepare_mesh(lhs, "lhs mesh");
    const auto rhs_prep = prepare_mesh(rhs, "rhs mesh");

    auto combined = lhs_prep.bounds.union_with(rhs_prep.bounds);
    const auto extent = combined.max - combined.min;
    T side = std::max({extent.x, extent.y, extent.z});
    if(!(side > static_cast<T>(0))){
        throw std::invalid_argument("Boolean meshes require a non-degenerate bounding domain.");
    }

    const auto expansion = side * boundary_scale;
    const auto centre = (combined.min + combined.max) * static_cast<T>(0.5);
    side += static_cast<T>(2) * expansion;

    const auto domain_min = centre - vec3<T>(side, side, side) * static_cast<T>(0.5);
    const auto resolution = static_cast<size_t>(size_t{1} << static_cast<size_t>(max_depth));
    const auto h = side / static_cast<T>(resolution);
    const auto far_distance = side * static_cast<T>(4);

    std::vector<uint8_t> occupied(resolution * resolution * resolution, 0U);

    auto linear_index = [resolution](size_t i, size_t j, size_t k) -> size_t {
        return (k * resolution + j) * resolution + i;
    };

    for(size_t k = 0; k < resolution; ++k){
        for(size_t j = 0; j < resolution; ++j){
            for(size_t i = 0; i < resolution; ++i){
                const auto cell_min = domain_min + vec3<T>(h * static_cast<T>(i),
                                                           h * static_cast<T>(j),
                                                           h * static_cast<T>(k));
                const auto centre_pt = cell_min + vec3<T>(h, h, h) * static_cast<T>(0.5);

                const auto inside_lhs = point_inside_mesh(centre_pt, lhs_prep, far_distance);
                const auto inside_rhs = point_inside_mesh(centre_pt, rhs_prep, far_distance);

                if(eval_boolean<T>(inside_lhs, inside_rhs, op)){
                    occupied.at(linear_index(i, j, k)) = 1U;
                }
            }
        }
    }

    auto out = emit_boundary_mesh<T, I>(occupied, resolution, domain_min, h);
    out.metadata["BooleanMaxDepth"] = std::to_string(max_depth);
    out.metadata["BooleanBoundaryScale"] = std::to_string(boundary_scale);
    if(op == MeshBooleanOperation::Union){
        out.metadata["BooleanOperation"] = "Union";
    }else if(op == MeshBooleanOperation::Intersection){
        out.metadata["BooleanOperation"] = "Intersection";
    }else if(op == MeshBooleanOperation::Exclusion){
        out.metadata["BooleanOperation"] = "Exclusion";
    }else{
        out.metadata["BooleanOperation"] = "Subtraction";
    }
    return out;
}

} // namespace


template <class T, class I>
fv_surface_mesh<T, I>
BooleanMeshOp(const fv_surface_mesh<T, I> &lhs,
              const fv_surface_mesh<T, I> &rhs,
              MeshBooleanOperation op,
              int64_t max_depth,
              T boundary_scale){
    return boolean_mesh_op_impl(lhs, rhs, op, max_depth, boundary_scale);
}

template <class T, class I>
fv_surface_mesh<T, I>
BooleanUnion(const fv_surface_mesh<T, I> &lhs,
             const fv_surface_mesh<T, I> &rhs,
             int64_t max_depth,
             T boundary_scale){
    return BooleanMeshOp(lhs, rhs, MeshBooleanOperation::Union, max_depth, boundary_scale);
}

template <class T, class I>
fv_surface_mesh<T, I>
BooleanIntersection(const fv_surface_mesh<T, I> &lhs,
                    const fv_surface_mesh<T, I> &rhs,
                    int64_t max_depth,
                    T boundary_scale){
    return BooleanMeshOp(lhs, rhs, MeshBooleanOperation::Intersection, max_depth, boundary_scale);
}

template <class T, class I>
fv_surface_mesh<T, I>
BooleanExclusion(const fv_surface_mesh<T, I> &lhs,
                 const fv_surface_mesh<T, I> &rhs,
                 int64_t max_depth,
                 T boundary_scale){
    return BooleanMeshOp(lhs, rhs, MeshBooleanOperation::Exclusion, max_depth, boundary_scale);
}

template <class T, class I>
fv_surface_mesh<T, I>
BooleanSubtraction(const fv_surface_mesh<T, I> &lhs,
                   const fv_surface_mesh<T, I> &rhs,
                   int64_t max_depth,
                   T boundary_scale){
    return BooleanMeshOp(lhs, rhs, MeshBooleanOperation::Subtraction, max_depth, boundary_scale);
}

#ifndef YGOR_MESHES_BOOLEAN_DISABLE_ALL_SPECIALIZATIONS
template fv_surface_mesh<float , uint32_t> BooleanMeshOp(const fv_surface_mesh<float , uint32_t> &, const fv_surface_mesh<float , uint32_t> &, MeshBooleanOperation, int64_t, float);
template fv_surface_mesh<float , uint64_t> BooleanMeshOp(const fv_surface_mesh<float , uint64_t> &, const fv_surface_mesh<float , uint64_t> &, MeshBooleanOperation, int64_t, float);
template fv_surface_mesh<double, uint32_t> BooleanMeshOp(const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &, MeshBooleanOperation, int64_t, double);
template fv_surface_mesh<double, uint64_t> BooleanMeshOp(const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &, MeshBooleanOperation, int64_t, double);

template fv_surface_mesh<float , uint32_t> BooleanUnion(const fv_surface_mesh<float , uint32_t> &, const fv_surface_mesh<float , uint32_t> &, int64_t, float);
template fv_surface_mesh<float , uint64_t> BooleanUnion(const fv_surface_mesh<float , uint64_t> &, const fv_surface_mesh<float , uint64_t> &, int64_t, float);
template fv_surface_mesh<double, uint32_t> BooleanUnion(const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &, int64_t, double);
template fv_surface_mesh<double, uint64_t> BooleanUnion(const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &, int64_t, double);

template fv_surface_mesh<float , uint32_t> BooleanIntersection(const fv_surface_mesh<float , uint32_t> &, const fv_surface_mesh<float , uint32_t> &, int64_t, float);
template fv_surface_mesh<float , uint64_t> BooleanIntersection(const fv_surface_mesh<float , uint64_t> &, const fv_surface_mesh<float , uint64_t> &, int64_t, float);
template fv_surface_mesh<double, uint32_t> BooleanIntersection(const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &, int64_t, double);
template fv_surface_mesh<double, uint64_t> BooleanIntersection(const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &, int64_t, double);

template fv_surface_mesh<float , uint32_t> BooleanExclusion(const fv_surface_mesh<float , uint32_t> &, const fv_surface_mesh<float , uint32_t> &, int64_t, float);
template fv_surface_mesh<float , uint64_t> BooleanExclusion(const fv_surface_mesh<float , uint64_t> &, const fv_surface_mesh<float , uint64_t> &, int64_t, float);
template fv_surface_mesh<double, uint32_t> BooleanExclusion(const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &, int64_t, double);
template fv_surface_mesh<double, uint64_t> BooleanExclusion(const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &, int64_t, double);

template fv_surface_mesh<float , uint32_t> BooleanSubtraction(const fv_surface_mesh<float , uint32_t> &, const fv_surface_mesh<float , uint32_t> &, int64_t, float);
template fv_surface_mesh<float , uint64_t> BooleanSubtraction(const fv_surface_mesh<float , uint64_t> &, const fv_surface_mesh<float , uint64_t> &, int64_t, float);
template fv_surface_mesh<double, uint32_t> BooleanSubtraction(const fv_surface_mesh<double, uint32_t> &, const fv_surface_mesh<double, uint32_t> &, int64_t, double);
template fv_surface_mesh<double, uint64_t> BooleanSubtraction(const fv_surface_mesh<double, uint64_t> &, const fv_surface_mesh<double, uint64_t> &, int64_t, double);
#endif
