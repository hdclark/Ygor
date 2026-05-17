//YgorMathTriangulateSeidels.cc.

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMathConstrainedDelaunay.h"
#include "YgorMathTriangulateSeidels.h"

namespace {

template <class T>
bool is_finite_2d(const vec2<T> &v){
    return std::isfinite(v.x) && std::isfinite(v.y);
}

template <class T>
bool same_xy(const vec2<T> &a, const vec2<T> &b){
    return (a.x == b.x) && (a.y == b.y);
}

template <class T>
bool has_non_collinear_triplet(const std::vector<vec2<T>> &poly){
    if(poly.size() < 3){
        return false;
    }
    for(size_t i = 0; i < poly.size(); ++i){
        for(size_t j = i + 1; j < poly.size(); ++j){
            if(same_xy(poly.at(i), poly.at(j))){
                continue;
            }
            for(size_t k = j + 1; k < poly.size(); ++k){
                if(orient_sign(poly.at(i), poly.at(j), poly.at(k)) != 0){
                    return true;
                }
            }
        }
    }
    return false;
}

template <class T>
std::vector<vec2<T>> normalize_closed_polygon(const std::vector<vec2<T>> &in){
    auto out = in;
    if(out.size() >= 2 && same_xy(out.front(), out.back())){
        out.pop_back();
    }
    return out;
}

template <class T>
std::string point_to_string(const vec2<T> &p){
    return "(" + std::to_string(p.x) + ", " + std::to_string(p.y) + ")";
}

template <class T>
void validate_polygon_loop(const std::vector<vec2<T>> &poly,
                           size_t poly_idx){
    if(poly.empty()){
        YLOGWARN("Polygon loop " << poly_idx << " is empty.");
        throw std::invalid_argument("Closed polygon input contains an empty loop.");
    }
    if(poly.size() < 3){
        YLOGWARN("Polygon loop " << poly_idx << " contains only " << poly.size() << " unique vertex/vertices.");
        throw std::invalid_argument("Closed polygon input requires at least three unique vertices per loop.");
    }

    for(size_t i = 0; i < poly.size(); ++i){
        const auto &v = poly.at(i);
        if(!is_finite_2d(v)){
            YLOGWARN("Polygon loop " << poly_idx << " contains non-finite vertex " << i << ": "
                     << point_to_string(v));
            throw std::invalid_argument("Closed polygon input requires all coordinates to be finite.");
        }
    }

    for(size_t i = 0; i < poly.size(); ++i){
        const auto next = (i + 1) % poly.size();
        if(same_xy(poly.at(i), poly.at(next))){
            YLOGWARN("Polygon loop " << poly_idx << " contains a zero-length edge between vertices "
                     << i << " and " << next << ".");
            throw std::invalid_argument("Closed polygon input contains a zero-length edge.");
        }
    }

    for(size_t i = 0; i < poly.size(); ++i){
        for(size_t j = i + 1; j < poly.size(); ++j){
            if(same_xy(poly.at(i), poly.at(j))){
                YLOGWARN("Polygon loop " << poly_idx << " repeats vertex coordinates at indices "
                         << i << " and " << j << ": " << point_to_string(poly.at(i)));
                throw std::invalid_argument("Closed polygon input contains repeated non-closure vertices.");
            }
        }
    }

    if(!has_non_collinear_triplet(poly)){
        YLOGWARN("Polygon loop " << poly_idx << " does not contain a non-collinear triplet.");
        throw std::invalid_argument("Closed polygon input contains a degenerate loop with zero enclosed area.");
    }
}

template <class T>
void validate_polygon_arrangement(const std::vector<std::vector<vec2<T>>> &polys){
    for(size_t poly_idx = 0; poly_idx < polys.size(); ++poly_idx){
        const auto &poly = polys.at(poly_idx);
        for(size_t edge_i = 0; edge_i < poly.size(); ++edge_i){
            const auto edge_i_next = (edge_i + 1) % poly.size();
            for(size_t poly_j = poly_idx; poly_j < polys.size(); ++poly_j){
                const auto &other = polys.at(poly_j);
                for(size_t edge_j = 0; edge_j < other.size(); ++edge_j){
                    const auto edge_j_next = (edge_j + 1) % other.size();

                    if((poly_idx == poly_j) && (edge_i == edge_j)){
                        continue;
                    }
                    if(poly_idx == poly_j){
                        if(edge_i == edge_j_next){
                            continue;
                        }
                        if(edge_i_next == edge_j){
                            continue;
                        }
                    }

                    const auto &a = poly.at(edge_i);
                    const auto &b = poly.at(edge_i_next);
                    const auto &c = other.at(edge_j);
                    const auto &d = other.at(edge_j_next);

                    if(segments_intersect_beyond_shared_endpoints(a, b, c, d)){
                        YLOGWARN("Polygon loop arrangement contains an edge intersection between loop "
                                 << poly_idx << " edge " << edge_i << " and loop " << poly_j
                                 << " edge " << edge_j << ".");
                        throw std::invalid_argument("Closed polygon input contains intersecting boundary edges.");
                    }

                    const bool same_segment = (same_xy(a, c) && same_xy(b, d))
                                           || (same_xy(a, d) && same_xy(b, c));
                    if(same_segment){
                        YLOGWARN("Polygon loop arrangement contains duplicate boundary segments between loop "
                                 << poly_idx << " edge " << edge_i << " and loop " << poly_j
                                 << " edge " << edge_j << ".");
                        throw std::invalid_argument("Closed polygon input contains duplicate boundary segments.");
                    }
                }
            }
        }
    }

    for(size_t poly_idx = 0; poly_idx < polys.size(); ++poly_idx){
        const auto &poly = polys.at(poly_idx);
        for(size_t vertex_idx = 0; vertex_idx < poly.size(); ++vertex_idx){
            const auto &p = poly.at(vertex_idx);
            for(size_t other_idx = 0; other_idx < polys.size(); ++other_idx){
                const auto &other = polys.at(other_idx);
                for(size_t edge_idx = 0; edge_idx < other.size(); ++edge_idx){
                    const auto next = (edge_idx + 1) % other.size();
                    if((poly_idx == other_idx)
                    && ((vertex_idx == edge_idx) || (vertex_idx == next))){
                        continue;
                    }

                    if(point_on_open_segment(p, other.at(edge_idx), other.at(next))){
                        YLOGWARN("Polygon loop " << poly_idx << " vertex " << vertex_idx
                                 << " lies on the open boundary edge " << edge_idx
                                 << " of loop " << other_idx << ".");
                        throw std::invalid_argument("Closed polygon input contains touching boundaries or a vertex-on-edge degeneracy.");
                    }
                }
            }
        }
    }
}

template <class T, class I>
void flatten_closed_polygons(const std::vector<std::vector<vec2<T>>> &closed_polygons,
                             std::vector<vec2<T>> &verts,
                             std::vector<std::vector<I>> &edges){
    verts.clear();
    edges.clear();

    std::map<std::pair<T, T>, size_t> vertex_to_index;
    for(const auto &poly : closed_polygons){
        for(const auto &v : poly){
            const auto key = std::make_pair(v.x, v.y);
            if(vertex_to_index.count(key) != 0){
                YLOGWARN("Closed polygon input reuses the boundary vertex " << point_to_string(v)
                         << " in multiple places.");
                throw std::invalid_argument("Closed polygon input contains duplicate boundary vertices across polygon loops.");
            }
            const auto idx = verts.size();
            vertex_to_index[key] = idx;
            verts.push_back(v);
        }
    }

    size_t offset = 0;
    for(const auto &poly : closed_polygons){
        for(size_t i = 0; i < poly.size(); ++i){
            const auto a = static_cast<I>(offset + i);
            const auto b = static_cast<I>(offset + ((i + 1) % poly.size()));
            edges.push_back({ a, b });
        }
        offset += poly.size();
    }
}

} // namespace

template <class T, class I>
fv_surface_mesh<T, I>
Triangulate_Seidels_2(const std::vector<std::vector<vec2<T>>> &closed_polygons){
    fv_surface_mesh<T, I> mesh;
    if(closed_polygons.empty()){
        YLOGDEBUG("Seidel polygon triangulation received empty input");
        return mesh;
    }

    std::vector<std::vector<vec2<T>>> normalized;
    normalized.reserve(closed_polygons.size());
    for(size_t i = 0; i < closed_polygons.size(); ++i){
        auto poly = normalize_closed_polygon(closed_polygons.at(i));
        validate_polygon_loop(poly, i);
        normalized.emplace_back(std::move(poly));
    }

    validate_polygon_arrangement(normalized);

    std::vector<vec2<T>> verts;
    std::vector<std::vector<I>> edges;
    flatten_closed_polygons<T, I>(normalized, verts, edges);

    YLOGDEBUG("Seidel polygon triangulation input: loops=" << normalized.size()
              << ", vertices=" << verts.size()
              << ", edges=" << edges.size());

    try{
        mesh = Constrained_Delaunay_Triangulation_2<T, I>(verts, edges);
    }catch(const std::exception &e){
        YLOGWARN("Seidel polygon triangulation failed: " << e.what());
        throw;
    }

    if(!verts.empty() && mesh.faces.empty()){
        const auto msg = "Seidel polygon triangulation produced no triangles for a non-empty polygon arrangement.";
        YLOGWARN(msg);
        throw std::runtime_error(msg);
    }

    YLOGDEBUG("Seidel polygon triangulation produced " << mesh.faces.size() << " triangle(s)");
    return mesh;
}

#ifndef YGOR_MATH_TRIANGULATE_SEIDELS_DISABLE_ALL_SPECIALIZATIONS
    template fv_surface_mesh<float , uint32_t> Triangulate_Seidels_2(const std::vector<std::vector<vec2<float >>> &);
    template fv_surface_mesh<float , uint64_t> Triangulate_Seidels_2(const std::vector<std::vector<vec2<float >>> &);

    template fv_surface_mesh<double, uint32_t> Triangulate_Seidels_2(const std::vector<std::vector<vec2<double>>> &);
    template fv_surface_mesh<double, uint64_t> Triangulate_Seidels_2(const std::vector<std::vector<vec2<double>>> &);
#endif
