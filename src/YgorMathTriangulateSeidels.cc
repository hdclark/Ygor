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

    const size_t first_vertex_idx = 0;
    size_t first_distinct_idx = poly.size();
    for(size_t i = 1; i < poly.size(); ++i){
        if(!same_xy(poly.at(first_vertex_idx), poly.at(i))){
            first_distinct_idx = i;
            break;
        }
    }
    if(first_distinct_idx >= poly.size()){
        return false;
    }

    for(size_t i = first_distinct_idx + 1; i < poly.size(); ++i){
        if(orient_sign(poly.at(first_vertex_idx), poly.at(first_distinct_idx), poly.at(i)) != 0){
            return true;
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

template <class T>
bool point_inside_odd_parity_region(const std::vector<std::vector<vec2<T>>> &closed_polygons,
                                    const vec2<T> &p){
    size_t depth = 0;
    for(const auto &poly : closed_polygons){
        if(point_in_polygon_or_on_boundary(poly, p)){
            ++depth;
        }
    }
    return (depth % 2) == 1;
}

template <class T, class I>
void retain_faces_in_odd_parity_region(fv_surface_mesh<T, I> &mesh,
                                       const std::vector<std::vector<vec2<T>>> &closed_polygons){
    std::vector<std::vector<I>> filtered;
    filtered.reserve(mesh.faces.size());
    for(const auto &face : mesh.faces){
        if(face.size() != 3){
            continue;
        }
        const auto &a = mesh.vertices.at(face.at(0));
        const auto &b = mesh.vertices.at(face.at(1));
        const auto &c = mesh.vertices.at(face.at(2));
        const vec2<T> centroid((a.x + b.x + c.x) / static_cast<T>(3),
                               (a.y + b.y + c.y) / static_cast<T>(3));
        if(point_inside_odd_parity_region(closed_polygons, centroid)){
            filtered.push_back(face);
        }
    }
    mesh.faces.swap(filtered);
}

template <class T>
long double polygon_signed_area_ld(const std::vector<vec2<T>> &poly){
    long double area = 0.0L;
    for(size_t i = 0; i < poly.size(); ++i){
        const auto &a = poly.at(i);
        const auto &b = poly.at((i + 1) % poly.size());
        area += (static_cast<long double>(a.x) * static_cast<long double>(b.y))
              - (static_cast<long double>(b.x) * static_cast<long double>(a.y));
    }
    return area / 2.0L;
}

template <class T>
struct SeidelTrapezoidSlice {
    vec2<T> left_low;
    vec2<T> left_high;
    vec2<T> right_high;
    vec2<T> right_low;
};

template <class T>
struct SeidelCrossingSegment {
    vec2<T> left;
    vec2<T> right;
    long double y_mid = 0.0L;
};

template <class T>
T interpolate_y_at_x(const vec2<T> &a,
                     const vec2<T> &b,
                     long double x){
    if(a.x == b.x){
        return a.y;
    }
    const auto dx = static_cast<long double>(b.x) - static_cast<long double>(a.x);

    const auto t = (x - static_cast<long double>(a.x)) / dx;
    const auto y = static_cast<long double>(a.y)
                 + t * (static_cast<long double>(b.y) - static_cast<long double>(a.y));
    return static_cast<T>(y);
}

template <class T>
std::vector<T> collect_sorted_unique_x_events(const std::vector<std::vector<vec2<T>>> &closed_polygons){
    std::vector<T> x_events;
    for(const auto &poly : closed_polygons){
        for(const auto &v : poly){
            x_events.push_back(v.x);
        }
    }

    std::sort(x_events.begin(), x_events.end());
    x_events.erase(std::unique(x_events.begin(), x_events.end()), x_events.end());
    return x_events;
}

template <class T>
std::vector<SeidelTrapezoidSlice<T>>
build_trapezoidal_slices(const std::vector<std::vector<vec2<T>>> &closed_polygons){
    std::vector<SeidelTrapezoidSlice<T>> slices;
    const auto x_events = collect_sorted_unique_x_events(closed_polygons);
    if(x_events.size() < 2){
        return slices;
    }
    constexpr long double order_tol = 64.0L * std::numeric_limits<T>::epsilon();

    // Narkhede and Manocha describe Seidel's polygon triangulation as a trapezoidal decomposition
    // followed by triangulation of the monotone cells.  This implementation keeps that structure
    // local and deterministic by sweeping the already-validated arrangement slab-by-slab between
    // consecutive x-events rather than building a separate randomized search DAG.
    for(size_t xi = 0; (xi + 1) < x_events.size(); ++xi){
        const auto x_left = static_cast<long double>(x_events.at(xi));
        const auto x_right = static_cast<long double>(x_events.at(xi + 1));
        if(std::abs(x_right - x_left) <= order_tol){
            continue;
        }
        const auto x_mid = (x_left + x_right) / 2.0L;

        std::vector<SeidelCrossingSegment<T>> crossings;
        for(const auto &poly : closed_polygons){
            for(size_t i = 0; i < poly.size(); ++i){
                const auto &a = poly.at(i);
                const auto &b = poly.at((i + 1) % poly.size());

                const auto min_x = std::min(static_cast<long double>(a.x), static_cast<long double>(b.x));
                const auto max_x = std::max(static_cast<long double>(a.x), static_cast<long double>(b.x));
                if((min_x >= x_mid) || (max_x <= x_mid) || (min_x == max_x)){
                    continue;
                }

                SeidelCrossingSegment<T> seg;
                seg.left = vec2<T>(static_cast<T>(x_left), interpolate_y_at_x(a, b, x_left));
                seg.right = vec2<T>(static_cast<T>(x_right), interpolate_y_at_x(a, b, x_right));
                seg.y_mid = static_cast<long double>(interpolate_y_at_x(a, b, x_mid));
                crossings.emplace_back(std::move(seg));
            }
        }

        std::sort(crossings.begin(), crossings.end(),
                  [order_tol](const auto &lhs, const auto &rhs){
                      if(std::abs(lhs.y_mid - rhs.y_mid) > order_tol){
                          return lhs.y_mid < rhs.y_mid;
                      }
                      if(std::abs(static_cast<long double>(lhs.left.y) - static_cast<long double>(rhs.left.y)) > order_tol){
                          return lhs.left.y < rhs.left.y;
                      }
                      return lhs.right.y < rhs.right.y;
                  });

        if((crossings.size() % 2) != 0){
            YLOGWARN("Seidel trapezoidal decomposition encountered an odd number of boundary crossings in x-slab "
                     << xi << ".");
            throw std::runtime_error("Seidel polygon triangulation failed while building the trapezoidal decomposition.");
        }

        for(size_t i = 0; i < crossings.size(); i += 2){
            const auto &low = crossings.at(i);
            const auto &high = crossings.at(i + 1);
            if((low.left.y > high.left.y) || (low.right.y > high.right.y)){
                YLOGWARN("Seidel trapezoidal decomposition encountered an invalid crossing order in x-slab "
                         << xi << ".");
                throw std::runtime_error("Seidel polygon triangulation failed because the trapezoidal ordering became inconsistent.");
            }

            slices.push_back(SeidelTrapezoidSlice<T>{
                low.left,
                high.left,
                high.right,
                low.right
            });
        }
    }

    return slices;
}

template <class T>
bool make_ccw_triangle(const std::vector<vec2<T>> &verts,
                       size_t a,
                       size_t b,
                       size_t c,
                       std::vector<size_t> &face){
    const auto o = orient_sign(verts.at(a), verts.at(b), verts.at(c));
    if(o > 0){
        face = { a, b, c };
        return true;
    }
    if(o < 0){
        face = { a, c, b };
        return true;
    }
    return false;
}

template <class T, class I>
fv_surface_mesh<T, I>
triangulate_trapezoidal_slices(const std::vector<std::vector<vec2<T>>> &closed_polygons){
    fv_surface_mesh<T, I> mesh;
    const auto slices = build_trapezoidal_slices(closed_polygons);

    std::vector<vec2<T>> verts2d;
    std::map<std::pair<T, T>, size_t> vertex_to_index;
    auto get_vertex_index = [&](const vec2<T> &v) -> size_t {
        const auto key = std::make_pair(v.x, v.y);
        const auto it = vertex_to_index.find(key);
        if(it != vertex_to_index.end()){
            return it->second;
        }
        const auto idx = verts2d.size();
        vertex_to_index.emplace(key, idx);
        verts2d.push_back(v);
        mesh.vertices.emplace_back(v.x, v.y, static_cast<T>(0));
        return idx;
    };

    for(const auto &slice : slices){
        const auto a = get_vertex_index(slice.left_low);
        const auto b = get_vertex_index(slice.left_high);
        const auto c = get_vertex_index(slice.right_high);
        const auto d = get_vertex_index(slice.right_low);

        std::vector<size_t> tri;
        if(make_ccw_triangle(verts2d, a, b, c, tri)){
            mesh.faces.push_back({
                static_cast<I>(tri.at(0)),
                static_cast<I>(tri.at(1)),
                static_cast<I>(tri.at(2))
            });
        }
        if(make_ccw_triangle(verts2d, a, c, d, tri)){
            mesh.faces.push_back({
                static_cast<I>(tri.at(0)),
                static_cast<I>(tri.at(1)),
                static_cast<I>(tri.at(2))
            });
        }
    }

    return mesh;
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

    YLOGDEBUG("Seidel polygon triangulation input: loops=" << normalized.size()
              << ", boundary vertices=" << [&normalized](){
                    size_t n = 0;
                    for(const auto &poly : normalized){
                        n += poly.size();
                    }
                    return n;
                }()
              << ", absolute loop area sum=" << [&normalized](){
                    long double area = 0.0L;
                    for(const auto &poly : normalized){
                        const auto signed_area = polygon_signed_area_ld(poly);
                        area += std::abs(signed_area);
                    }
                    return area;
                }());

    try{
        mesh = triangulate_trapezoidal_slices<T, I>(normalized);
    }catch(const std::exception &e){
        YLOGWARN("Seidel polygon triangulation failed: " << e.what());
        throw;
    }

    retain_faces_in_odd_parity_region(mesh, normalized);

    if(!normalized.empty() && mesh.faces.empty()){
        const auto msg = "Seidel polygon triangulation produced no triangles for a non-empty polygon arrangement.";
        YLOGWARN(msg);
        throw std::runtime_error(msg);
    }

    YLOGDEBUG("Seidel polygon triangulation produced " << mesh.faces.size() << " triangle(s)");
    return mesh;
}

#ifndef YGOR_MATH_TRIANGULATE_SEIDELS_DISABLE_ALL_SPECIALIZATIONS
    template fv_surface_mesh<float , uint32_t> Triangulate_Seidels_2(const std::vector<std::vector<vec2<float>>> &);
    template fv_surface_mesh<float , uint64_t> Triangulate_Seidels_2(const std::vector<std::vector<vec2<float>>> &);

    template fv_surface_mesh<double, uint32_t> Triangulate_Seidels_2(const std::vector<std::vector<vec2<double>>> &);
    template fv_surface_mesh<double, uint64_t> Triangulate_Seidels_2(const std::vector<std::vector<vec2<double>>> &);
#endif
