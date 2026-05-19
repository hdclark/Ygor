//YgorMathMonotoneDecomposition.cc

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMathMonotoneDecomposition.h"

namespace {

template <class T>
bool is_finite_2d(const vec2<T> &v){
    return std::isfinite(v.x) && std::isfinite(v.y);
}

template <class T>
bool same_xy(const vec2<T> &a, const vec2<T> &b){
    return (a.x == b.x) && (a.y == b.y);
}

inline bool monotone_fail(std::string *diag, const std::string &msg){
    if(diag != nullptr){
        *diag = msg;
    }
    YLOGDEBUG(msg);
    return false;
}

template <class T>
struct NormalizedPolygon {
    size_t polygon_index = 0;
    bool interior = true;
    size_t parent = std::numeric_limits<size_t>::max();
    long double area_abs = 0.0L;
    std::vector<size_t> original_indices;
    std::vector<size_t> children;
};

template <class T>
long double polygon_signed_area_ld(const std::vector<vec2<T>> &verts,
                                   const std::vector<size_t> &poly){
    long double area = 0.0L;
    for(size_t i = 0; i < poly.size(); ++i){
        const auto &a = verts.at(poly.at(i));
        const auto &b = verts.at(poly.at((i + 1) % poly.size()));
        area += (static_cast<long double>(a.x) * static_cast<long double>(b.y))
              - (static_cast<long double>(b.x) * static_cast<long double>(a.y));
    }
    return area / 2.0L;
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
bool event_higher(const vec2<T> &a, const vec2<T> &b){
    return (a.y > b.y) || ((a.y == b.y) && (a.x < b.x));
}

template <class T>
bool event_lower(const vec2<T> &a, const vec2<T> &b){
    return event_higher(b, a);
}

bool are_adjacent_positions(size_t a, size_t b, size_t n){
    if(n < 2){
        return false;
    }
    return ((a + 1) % n == b) || ((b + 1) % n == a);
}

template <class T>
bool validate_polygon_edges(const std::vector<vec2<T>> &verts,
                            const std::vector<size_t> &poly,
                            std::string *diag){
    const auto n = poly.size();
    for(size_t i = 0; i < n; ++i){
        const auto a_idx = poly.at(i);
        const auto b_idx = poly.at((i + 1) % n);
        const auto &a = verts.at(a_idx);
        const auto &b = verts.at(b_idx);
        if(same_xy(a, b)){
            return monotone_fail(diag, "Polygon contains a zero-length edge between consecutive vertices.");
        }

        for(size_t j = i + 1; j < n; ++j){
            const auto c_idx = poly.at(j);
            const auto d_idx = poly.at((j + 1) % n);
            const bool shares_endpoint = (a_idx == c_idx) || (a_idx == d_idx)
                                      || (b_idx == c_idx) || (b_idx == d_idx);
            const bool adjacent = shares_endpoint
                               && ((j == i + 1) || ((i == 0) && (j + 1 == n)));
            if(adjacent){
                continue;
            }

            const auto &c = verts.at(c_idx);
            const auto &d = verts.at(d_idx);
            if(segments_intersect_beyond_shared_endpoints(a, b, c, d)
            || point_on_open_segment(a, c, d)
            || point_on_open_segment(b, c, d)
            || point_on_open_segment(c, a, b)
            || point_on_open_segment(d, a, b)){
                return monotone_fail(diag, "Polygon edges intersect or overlap beyond adjacent shared endpoints.");
            }
        }
    }
    return true;
}

template <class T>
bool validate_polygon_pair(const std::vector<vec2<T>> &lhs_verts,
                           const std::vector<size_t> &lhs_poly,
                           const std::vector<vec2<T>> &rhs_verts,
                           const std::vector<size_t> &rhs_poly,
                           std::string *diag){
    for(size_t i = 0; i < lhs_poly.size(); ++i){
        const auto &a = lhs_verts.at(lhs_poly.at(i));
        const auto &b = lhs_verts.at(lhs_poly.at((i + 1) % lhs_poly.size()));
        for(size_t j = 0; j < rhs_poly.size(); ++j){
            const auto &c = rhs_verts.at(rhs_poly.at(j));
            const auto &d = rhs_verts.at(rhs_poly.at((j + 1) % rhs_poly.size()));
            if(segments_intersect_beyond_shared_endpoints(a, b, c, d)
            || point_on_closed_segment(a, c, d)
            || point_on_closed_segment(b, c, d)
            || point_on_closed_segment(c, a, b)
            || point_on_closed_segment(d, a, b)){
                return monotone_fail(diag, "Distinct polygons intersect or touch; nested loops must be strictly disjoint.");
            }
        }
    }
    return true;
}

template <class T>
bool normalize_polygon(const std::vector<std::vector<vec2<T>>> &all_verts,
                       size_t polygon_index,
                       NormalizedPolygon<T> &out,
                       std::string *diag){
    const auto &verts = all_verts.at(polygon_index);
    out = NormalizedPolygon<T>();
    out.polygon_index = polygon_index;

    if(verts.empty()){
        return monotone_fail(diag, "Polygon " + std::to_string(polygon_index) + " contains no vertices.");
    }

    for(size_t i = 0; i < verts.size(); ++i){
        if(!is_finite_2d(verts.at(i))){
            return monotone_fail(diag, "Polygon " + std::to_string(polygon_index)
                                      + " contains a non-finite vertex.");
        }
    }

    std::vector<size_t> poly;
    poly.reserve(verts.size());
    for(size_t i = 0; i < verts.size(); ++i){
        if(poly.empty() || !same_xy(verts.at(poly.back()), verts.at(i))){
            poly.push_back(i);
        }
    }
    if((poly.size() >= 2) && same_xy(verts.at(poly.front()), verts.at(poly.back()))){
        poly.pop_back();
    }
    if(poly.size() < 3){
        return monotone_fail(diag, "Polygon " + std::to_string(polygon_index)
                                  + " does not contain at least three distinct vertices after removing repeated endpoints.");
    }

    bool changed = true;
    while(changed){
        changed = false;
        if(poly.size() < 3){
            break;
        }
        for(size_t i = 0; i < poly.size(); ++i){
            const auto prev = poly.at((i + poly.size() - 1) % poly.size());
            const auto cur  = poly.at(i);
            const auto next = poly.at((i + 1) % poly.size());
            if((orient_sign(verts.at(prev), verts.at(cur), verts.at(next)) == 0)
            && point_on_closed_segment(verts.at(cur), verts.at(prev), verts.at(next))){
                poly.erase(poly.begin() + static_cast<std::ptrdiff_t>(i));
                changed = true;
                break;
            }
        }
    }

    if(poly.size() < 3){
        return monotone_fail(diag, "Polygon " + std::to_string(polygon_index)
                                  + " became degenerate after removing duplicate and collinear vertices.");
    }

    std::set<std::pair<T, T>> unique_xy;
    for(const auto idx : poly){
        const auto &v = verts.at(idx);
        const auto key = std::make_pair(v.x, v.y);
        if(!unique_xy.insert(key).second){
            return monotone_fail(diag, "Polygon " + std::to_string(polygon_index)
                                      + " reuses a vertex coordinate, which would violate the simple-polygon assumption.");
        }
    }

    if(!validate_polygon_edges(verts, poly, diag)){
        return false;
    }

    const auto area = polygon_signed_area_ld(verts, poly);
    if(area == 0.0L){
        return monotone_fail(diag, "Polygon " + std::to_string(polygon_index)
                                  + " has zero signed area after normalization.");
    }
    if(area < 0.0L){
        std::reverse(poly.begin(), poly.end());
    }

    out.original_indices = std::move(poly);
    return true;
}

enum class VertexType {
    Start,
    End,
    Split,
    Merge,
    Regular
};

template <class T>
struct SweepEdge {
    size_t upper = 0;
    size_t lower = 0;

    bool operator==(const SweepEdge &rhs) const {
        return (upper == rhs.upper) && (lower == rhs.lower);
    }
};

struct SweepQuery {
    size_t vertex = 0;
};

template <class T>
struct WorkingPolygon {
    bool interior = true;
    std::vector<std::pair<size_t, size_t>> refs;
    std::vector<vec2<T>> coords;
};

template <class T>
bool point_in_polygon_or_on_boundary_robust(const std::vector<vec2<T>> &polygon,
                                            const vec2<T> &p){
    if(polygon.empty()){
        throw std::invalid_argument("Polygon contains no vertices, cannot continue");
    }

    for(size_t i = 0; i < polygon.size(); ++i){
        if(point_on_closed_segment(p, polygon.at(i), polygon.at((i + 1) % polygon.size()))){
            return true;
        }
    }

    bool inside = false;
    for(size_t i = 0; i < polygon.size(); ++i){
        const auto &a = polygon.at(i);
        const auto &b = polygon.at((i + 1) % polygon.size());
        if((a.y > p.y) == (b.y > p.y)){
            continue;
        }

        const auto o = orient_sign(a, b, p);
        if((o == 0)
        || ((b.y > a.y) && (o > 0))
        || ((b.y < a.y) && (o < 0))){
            inside = !inside;
        }
    }
    return inside;
}

template <class T>
bool point_in_polygon_or_on_boundary_robust(const std::vector<vec2<T>> &verts,
                                            const std::vector<size_t> &polygon,
                                            const vec2<T> &p){
    std::vector<vec2<T>> coords;
    coords.reserve(polygon.size());
    for(const auto idx : polygon){
        coords.push_back(verts.at(idx));
    }
    return point_in_polygon_or_on_boundary_robust(coords, p);
}

template <class T>
bool edge_left_of_vertex(const WorkingPolygon<T> &poly,
                         const SweepEdge<T> &edge,
                         size_t vertex_pos){
    const auto sign = orient_sign(poly.coords.at(edge.upper),
                                  poly.coords.at(edge.lower),
                                  poly.coords.at(vertex_pos));
    if(sign == 0){
        // This should only happen for explicitly guarded degeneracies (for example, if a future change relaxes the
        // "no touching" validation), so fall back to a deterministic ordering based on the edge key.
        return std::tie(edge.upper, edge.lower) < std::make_tuple(vertex_pos, vertex_pos);
    }
    return sign > 0;
}

template <class T>
bool edge_less(const WorkingPolygon<T> &poly,
               const SweepEdge<T> &lhs,
               const SweepEdge<T> &rhs){
    if(lhs == rhs){
        return false;
    }

    const auto &lhs_upper = poly.coords.at(lhs.upper);
    const auto &lhs_lower = poly.coords.at(lhs.lower);
    const auto &rhs_upper = poly.coords.at(rhs.upper);
    const auto &rhs_lower = poly.coords.at(rhs.lower);

    if(lhs.upper == rhs.upper){
        const auto sign = orient_sign(lhs_upper, lhs_lower, rhs_lower);
        if(sign != 0){
            return sign > 0;
        }
        return std::tie(lhs.lower, lhs.upper) < std::tie(rhs.lower, rhs.upper);
    }

    if(event_higher(lhs_upper, rhs_upper)){
        const auto sign = orient_sign(lhs_upper, lhs_lower, rhs_upper);
        if(sign != 0){
            return sign > 0;
        }
        return std::tie(lhs.upper, lhs.lower) < std::tie(rhs.upper, rhs.lower);
    }

    const auto sign = orient_sign(rhs_upper, rhs_lower, lhs_upper);
    if(sign != 0){
        return sign < 0;
    }
    return std::tie(lhs.upper, lhs.lower) < std::tie(rhs.upper, rhs.lower);
}

template <class T>
struct SweepStatusLess {
    using is_transparent = void;

    const WorkingPolygon<T> *poly = nullptr;

    bool operator()(const SweepEdge<T> &lhs, const SweepEdge<T> &rhs) const {
        return edge_less(*poly, lhs, rhs);
    }

    bool operator()(const SweepEdge<T> &lhs, const SweepQuery &rhs) const {
        return edge_left_of_vertex(*poly, lhs, rhs.vertex);
    }

    bool operator()(const SweepQuery &lhs, const SweepEdge<T> &rhs) const {
        return !edge_left_of_vertex(*poly, rhs, lhs.vertex);
    }
};

template <class T>
std::pair<size_t, size_t> edge_key(const SweepEdge<T> &edge){
    return std::make_pair(edge.upper, edge.lower);
}

template <class T>
bool make_sweep_edge(const WorkingPolygon<T> &poly,
                     size_t a,
                     size_t b,
                     SweepEdge<T> &out){
    const auto &va = poly.coords.at(a);
    const auto &vb = poly.coords.at(b);
    if(va.y == vb.y){
        return false;
    }
    if(event_higher(va, vb)){
        out = SweepEdge<T>{a, b};
    }else{
        out = SweepEdge<T>{b, a};
    }
    return true;
}

template <class T>
VertexType classify_vertex(const WorkingPolygon<T> &poly,
                           size_t pos){
    const auto n = poly.coords.size();
    const auto prev = (pos + n - 1) % n;
    const auto next = (pos + 1) % n;
    const auto &v_prev = poly.coords.at(prev);
    const auto &v_cur  = poly.coords.at(pos);
    const auto &v_next = poly.coords.at(next);

    const auto turn = orient_sign(v_prev, v_cur, v_next);
    if(turn == 0){
        throw std::runtime_error("Normalized polygon still contains a collinear triple; cannot classify sweep-line vertex.");
    }

    const bool prev_above = event_higher(v_prev, v_cur);
    const bool next_above = event_higher(v_next, v_cur);
    const bool prev_below = event_lower(v_prev, v_cur);
    const bool next_below = event_lower(v_next, v_cur);

    if(prev_below && next_below){
        return (turn > 0) ? VertexType::Start : VertexType::Split;
    }
    if(prev_above && next_above){
        return (turn > 0) ? VertexType::End : VertexType::Merge;
    }
    return VertexType::Regular;
}

template <class T>
bool is_interior_to_right(const WorkingPolygon<T> &poly, size_t pos){
    const auto next = (pos + 1) % poly.coords.size();
    return event_higher(poly.coords.at(pos), poly.coords.at(next));
}

template <class T>
bool add_diagonal(size_t a,
                  size_t b,
                  size_t n,
                  std::set<std::pair<size_t, size_t>> &diagonals){
    if((a == b) || are_adjacent_positions(a, b, n)){
        return true;
    }
    if(b < a){
        std::swap(a, b);
    }
    diagonals.insert(std::make_pair(a, b));
    return true;
}

template <class T>
bool decompose_polygon_sweep(const WorkingPolygon<T> &poly,
                             std::set<std::pair<size_t, size_t>> &diagonals,
                             std::string *diag){
    diagonals.clear();

    SweepStatusLess<T> less;
    less.poly = &poly;
    std::set<SweepEdge<T>, SweepStatusLess<T>> status(less);
    std::map<std::pair<size_t, size_t>, size_t> helpers;
    std::vector<VertexType> types(poly.coords.size(), VertexType::Regular);

    for(size_t i = 0; i < poly.coords.size(); ++i){
        const auto prev = (i + poly.coords.size() - 1) % poly.coords.size();
        const auto next = (i + 1) % poly.coords.size();
        if(orient_sign(poly.coords.at(prev), poly.coords.at(i), poly.coords.at(next)) == 0){
            return monotone_fail(diag, "Normalized polygon still contains a collinear triple; cannot classify sweep-line vertices robustly.");
        }
        types.at(i) = classify_vertex(poly, i);
    }

    std::vector<size_t> events(poly.coords.size());
    for(size_t i = 0; i < events.size(); ++i){
        events.at(i) = i;
    }
    std::sort(events.begin(), events.end(),
              [&](size_t lhs, size_t rhs){
                  const auto &a = poly.coords.at(lhs);
                  const auto &b = poly.coords.at(rhs);
                  if(a.y != b.y){
                      return a.y > b.y;
                  }
                  if(a.x != b.x){
                      return a.x < b.x;
                  }
                  return lhs < rhs;
              });

    for(const auto pos : events){
        const auto prev = (pos + poly.coords.size() - 1) % poly.coords.size();
        const auto next = (pos + 1) % poly.coords.size();

        SweepEdge<T> prev_edge{};
        const bool has_prev_edge = make_sweep_edge(poly, prev, pos, prev_edge);
        SweepEdge<T> next_edge{};
        const bool has_next_edge = make_sweep_edge(poly, pos, next, next_edge);

        const auto maybe_connect_helper = [&](const SweepEdge<T> &edge) -> bool {
            const auto helper_it = helpers.find(edge_key(edge));
            if(helper_it == helpers.end()){
                return true;
            }
            const auto helper_pos = helper_it->second;
            if(types.at(helper_pos) == VertexType::Merge){
                return add_diagonal<T>(pos, helper_pos, poly.coords.size(), diagonals);
            }
            return true;
        };

        const auto find_left_edge = [&]() -> typename std::set<SweepEdge<T>, SweepStatusLess<T>>::iterator {
            const auto right_it = status.lower_bound(SweepQuery{ pos });
            if(right_it == status.begin()){
                return status.end();
            }
            return std::prev(right_it);
        };

        switch(types.at(pos)){
            case VertexType::Start:
                if(has_next_edge){
                    status.insert(next_edge);
                    helpers[edge_key(next_edge)] = pos;
                }
                break;

            case VertexType::End:
                if(has_prev_edge){
                    if(!maybe_connect_helper(prev_edge)){
                        return false;
                    }
                    status.erase(prev_edge);
                    helpers.erase(edge_key(prev_edge));
                }
                break;

            case VertexType::Split: {
                const auto left_it = find_left_edge();
                if(left_it == status.end()){
                    return monotone_fail(diag, "Failed to locate the sweep-line status edge directly left of a split vertex.");
                }
                if(!add_diagonal<T>(pos, helpers.at(edge_key(*left_it)), poly.coords.size(), diagonals)){
                    return false;
                }
                helpers[edge_key(*left_it)] = pos;
                if(has_next_edge){
                    status.insert(next_edge);
                    helpers[edge_key(next_edge)] = pos;
                }
                break;
            }

            case VertexType::Merge: {
                if(has_prev_edge){
                    if(!maybe_connect_helper(prev_edge)){
                        return false;
                    }
                    status.erase(prev_edge);
                    helpers.erase(edge_key(prev_edge));
                }
                const auto left_it = find_left_edge();
                if(left_it == status.end()){
                    return monotone_fail(diag, "Failed to locate the sweep-line status edge directly left of a merge vertex.");
                }
                const auto helper_pos = helpers.at(edge_key(*left_it));
                if(types.at(helper_pos) == VertexType::Merge){
                    if(!add_diagonal<T>(pos, helper_pos, poly.coords.size(), diagonals)){
                        return false;
                    }
                }
                helpers[edge_key(*left_it)] = pos;
                break;
            }

            case VertexType::Regular:
                if(is_interior_to_right(poly, pos)){
                    if(has_prev_edge){
                        if(!maybe_connect_helper(prev_edge)){
                            return false;
                        }
                        status.erase(prev_edge);
                        helpers.erase(edge_key(prev_edge));
                    }
                    if(has_next_edge){
                        status.insert(next_edge);
                        helpers[edge_key(next_edge)] = pos;
                    }
                }else{
                    const auto left_it = find_left_edge();
                    if(left_it == status.end()){
                        return monotone_fail(diag, "Failed to locate the sweep-line status edge directly left of a regular vertex.");
                    }
                    const auto helper_pos = helpers.at(edge_key(*left_it));
                    if(types.at(helper_pos) == VertexType::Merge){
                        if(!add_diagonal<T>(pos, helper_pos, poly.coords.size(), diagonals)){
                            return false;
                        }
                    }
                    helpers[edge_key(*left_it)] = pos;
                }
                break;
        }
    }

    return true;
}

template <class T>
bool sort_neighbors_ccw(const WorkingPolygon<T> &poly,
                        size_t origin,
                        std::vector<size_t> &neighbors,
                        std::string *diag){
    std::sort(neighbors.begin(), neighbors.end(),
              [&](size_t lhs, size_t rhs){
                  const auto &o = poly.coords.at(origin);
                  const auto &a = poly.coords.at(lhs);
                  const auto &b = poly.coords.at(rhs);
                  const auto dx_a = a.x - o.x;
                  const auto dy_a = a.y - o.y;
                  const auto dx_b = b.x - o.x;
                  const auto dy_b = b.y - o.y;
                  const bool upper_a = (dy_a > 0) || ((dy_a == 0) && (dx_a > 0));
                  const bool upper_b = (dy_b > 0) || ((dy_b == 0) && (dx_b > 0));
                  if(upper_a != upper_b){
                      return upper_a && !upper_b;
                  }
                  const auto sign = orient_sign(o, a, b);
                  if(sign != 0){
                      return sign > 0;
                  }
                  const auto dist_a = (dx_a * dx_a) + (dy_a * dy_a);
                  const auto dist_b = (dx_b * dx_b) + (dy_b * dy_b);
                  if(dist_a != dist_b){
                      return dist_a < dist_b;
                  }
                  return lhs < rhs;
              });

    for(size_t i = 1; i < neighbors.size(); ++i){
        if(neighbors.at(i) == neighbors.at(i - 1)){
            return monotone_fail(diag, "Face extraction produced duplicate adjacency entries while sorting vertex stars.");
        }
    }
    return true;
}

template <class T>
bool is_y_monotone_cycle(const WorkingPolygon<T> &poly,
                         const std::vector<size_t> &cycle){
    if(cycle.size() < 3){
        return false;
    }

    auto higher_pos = [&](size_t lhs, size_t rhs){
        return event_higher(poly.coords.at(cycle.at(lhs)), poly.coords.at(cycle.at(rhs)));
    };

    size_t top = 0;
    size_t bottom = 0;
    for(size_t i = 1; i < cycle.size(); ++i){
        if(higher_pos(i, top)){
            top = i;
        }
        if(higher_pos(bottom, i)){
            bottom = i;
        }
    }

    auto chain_is_monotone = [&](size_t step) -> bool {
        size_t cur = top;
        while(cur != bottom){
            const auto next = (cur + step) % cycle.size();
            if(event_higher(poly.coords.at(cycle.at(next)), poly.coords.at(cycle.at(cur)))){
                return false;
            }
            cur = next;
        }
        return true;
    };

    return chain_is_monotone(1) && chain_is_monotone(cycle.size() - 1);
}

template <class T>
bool extract_monotone_faces(const WorkingPolygon<T> &poly,
                            const std::set<std::pair<size_t, size_t>> &diagonals,
                            std::vector<std::vector<size_t>> &faces,
                            std::string *diag){
    faces.clear();
    std::vector<std::vector<size_t>> adjacency(poly.coords.size());
    for(size_t i = 0; i < poly.coords.size(); ++i){
        const auto j = (i + 1) % poly.coords.size();
        adjacency.at(i).push_back(j);
        adjacency.at(j).push_back(i);
    }
    for(const auto &[a, b] : diagonals){
        adjacency.at(a).push_back(b);
        adjacency.at(b).push_back(a);
    }

    for(size_t i = 0; i < adjacency.size(); ++i){
        if(!sort_neighbors_ccw(poly, i, adjacency.at(i), diag)){
            return false;
        }
    }

    std::set<std::pair<size_t, size_t>> visited_halfedges;
    const auto max_steps = (poly.coords.size() + diagonals.size()) * 2 + 1;
    for(size_t u = 0; u < adjacency.size(); ++u){
        for(const auto v : adjacency.at(u)){
            const auto start = std::make_pair(u, v);
            if(visited_halfedges.count(start) != 0){
                continue;
            }

            std::vector<size_t> cycle;
            size_t cur_u = u;
            size_t cur_v = v;
            while(true){
                if(!visited_halfedges.insert(std::make_pair(cur_u, cur_v)).second){
                    return monotone_fail(diag, "Halfedge traversal revisited a directed edge before closing a face cycle.");
                }
                cycle.push_back(cur_u);
                const auto &nbrs = adjacency.at(cur_v);
                const auto it = std::find(nbrs.begin(), nbrs.end(), cur_u);
                if(it == nbrs.end()){
                    return monotone_fail(diag, "Halfedge traversal lost the reverse edge while extracting monotone faces.");
                }
                const auto idx = static_cast<size_t>(std::distance(nbrs.begin(), it));
                const auto next = nbrs.at((idx + nbrs.size() - 1) % nbrs.size());
                cur_u = cur_v;
                cur_v = next;
                if(std::make_pair(cur_u, cur_v) == start){
                    break;
                }
                if(cycle.size() > max_steps){
                    return monotone_fail(diag, "Halfedge traversal exceeded its guard while extracting monotone faces.");
                }
            }

            if(cycle.size() < 3){
                continue;
            }

            long double area = 0.0L;
            for(size_t i = 0; i < cycle.size(); ++i){
                const auto &a = poly.coords.at(cycle.at(i));
                const auto &b = poly.coords.at(cycle.at((i + 1) % cycle.size()));
                area += (static_cast<long double>(a.x) * static_cast<long double>(b.y))
                      - (static_cast<long double>(b.x) * static_cast<long double>(a.y));
            }
            area /= 2.0L;
            if(area <= 0.0L){
                continue;
            }
            if(!is_y_monotone_cycle(poly, cycle)){
                return monotone_fail(diag, "Sweep-line decomposition produced a bounded face that is not y-monotone.");
            }
            faces.push_back(std::move(cycle));
        }
    }

    if(faces.empty()){
        return monotone_fail(diag, "Sweep-line decomposition did not yield any bounded monotone faces.");
    }
    return true;
}

template <class I>
void rotate_face_to_smallest(std::vector<std::pair<I, I>> &face){
    if(face.empty()){
        return;
    }
    const auto it = std::min_element(face.begin(), face.end());
    std::rotate(face.begin(), it, face.end());
}

template <class T, class I>
bool convert_faces(const WorkingPolygon<T> &poly,
                   const std::vector<std::vector<size_t>> &faces,
                   std::vector<monotone_t<I>> &out,
                   std::string *diag){
    for(const auto &face : faces){
        monotone_t<I> piece;
        piece.interior = poly.interior;
        piece.vertices.reserve(face.size());
        for(const auto pos : face){
            const auto polygon_index = static_cast<I>(poly.refs.at(pos).first);
            const auto vertex_index = static_cast<I>(poly.refs.at(pos).second);
            piece.vertices.emplace_back(polygon_index, vertex_index);
        }
        rotate_face_to_smallest<I>(piece.vertices);
        if(piece.vertices.size() < 3){
            return monotone_fail(diag, "Converted monotone face contains fewer than three vertices.");
        }
        out.push_back(std::move(piece));
    }
    return true;
}

template <class T>
bool build_polygon_tree(const std::vector<std::vector<vec2<T>>> &all_verts,
                        std::vector<NormalizedPolygon<T>> &polys){
    for(auto &poly : polys){
        poly.parent = std::numeric_limits<size_t>::max();
        poly.children.clear();
        poly.area_abs = std::abs(polygon_signed_area_ld(all_verts.at(poly.polygon_index),
                                                        poly.original_indices));
    }

    for(size_t i = 0; i < polys.size(); ++i){
        const auto &sample = all_verts.at(polys.at(i).polygon_index).at(polys.at(i).original_indices.front());
        size_t parent = std::numeric_limits<size_t>::max();
        long double parent_area = std::numeric_limits<long double>::max();
        for(size_t j = 0; j < polys.size(); ++j){
            if(i == j){
                continue;
            }
            if(point_in_polygon_or_on_boundary_robust(all_verts.at(polys.at(j).polygon_index),
                                                      polys.at(j).original_indices,
                                                      sample)
            && (polys.at(j).area_abs < parent_area)){
                parent = j;
                parent_area = polys.at(j).area_abs;
            }
        }
        polys.at(i).parent = parent;
    }

    for(size_t i = 0; i < polys.size(); ++i){
        if(polys.at(i).parent != std::numeric_limits<size_t>::max()){
            polys.at(polys.at(i).parent).children.push_back(i);
        }
    }

    for(size_t i = 0; i < polys.size(); ++i){
        size_t depth = 0;
        for(size_t cur = polys.at(i).parent;
            cur != std::numeric_limits<size_t>::max();
            cur = polys.at(cur).parent){
            ++depth;
        }
        polys.at(i).interior = ((depth % 2) == 0);
    }
    return true;
}

template <class T>
WorkingPolygon<T> make_working_polygon(const std::vector<std::vector<vec2<T>>> &verts,
                                       const NormalizedPolygon<T> &poly){
    WorkingPolygon<T> working;
    working.interior = poly.interior;
    working.refs.reserve(poly.original_indices.size());
    working.coords.reserve(poly.original_indices.size());
    for(const auto idx : poly.original_indices){
        working.refs.emplace_back(poly.polygon_index, idx);
        working.coords.push_back(verts.at(poly.polygon_index).at(idx));
    }
    return working;
}

template <class T>
bool bridge_crosses_cycle(const std::vector<vec2<T>> &cycle,
                          size_t skip_vertex_pos,
                          const vec2<T> &a,
                          const vec2<T> &b){
    for(size_t i = 0; i < cycle.size(); ++i){
        const auto j = (i + 1) % cycle.size();
        const bool incident = (skip_vertex_pos != std::numeric_limits<size_t>::max())
                           && ((i == skip_vertex_pos) || (j == skip_vertex_pos));
        if(incident){
            continue;
        }

        const auto &c = cycle.at(i);
        const auto &d = cycle.at(j);
        if(segments_intersect_beyond_shared_endpoints(a, b, c, d)
        || point_on_open_segment(a, c, d)
        || point_on_open_segment(b, c, d)
        || point_on_open_segment(c, a, b)
        || point_on_open_segment(d, a, b)){
            return true;
        }
    }
    return false;
}

template <class T>
bool bridge_makes_collinear_splice(const WorkingPolygon<T> &working,
                                   size_t working_pos,
                                   const WorkingPolygon<T> &hole,
                                   size_t hole_pos){
    const auto working_n = working.coords.size();
    const auto hole_n = hole.coords.size();
    const auto &p = working.coords.at(working_pos);
    const auto &h = hole.coords.at(hole_pos);
    const auto &working_prev = working.coords.at((working_pos + working_n - 1) % working_n);
    const auto &working_next = working.coords.at((working_pos + 1) % working_n);
    const auto &hole_prev = hole.coords.at((hole_pos + hole_n - 1) % hole_n);
    const auto &hole_next = hole.coords.at((hole_pos + 1) % hole_n);

    return (orient_sign(working_prev, p, h) == 0)
        || (orient_sign(h, p, working_next) == 0)
        || (orient_sign(p, h, hole_prev) == 0)
        || (orient_sign(hole_next, h, p) == 0);
}

template <class T>
size_t rightmost_vertex(const WorkingPolygon<T> &poly){
    size_t best = 0;
    for(size_t i = 1; i < poly.coords.size(); ++i){
        const auto &cand = poly.coords.at(i);
        const auto &cur = poly.coords.at(best);
        if((cand.x > cur.x) || ((cand.x == cur.x) && (cand.y < cur.y))){
            best = i;
        }
    }
    return best;
}

template <class T>
bool find_bridge(const WorkingPolygon<T> &working,
                 const std::vector<WorkingPolygon<T>> &holes,
                 size_t hole_index,
                 size_t &working_pos,
                 size_t &hole_pos){
    // Following the standard hole-joining preprocessing described by de Berg et al. and Held, we connect a loop to each
    // directly nested child by duplicating a visible bridge segment as a zero-width corridor. The implementation below
    // keeps the predicates robust by validating each candidate using adaptive orientation/segment tests only.
    bool found = false;
    long double best_len = std::numeric_limits<long double>::max();

    const auto &hole = holes.at(hole_index);
    for(size_t i = 0; i < working.coords.size(); ++i){
        const auto &a = working.coords.at(i);
        for(size_t j = 0; j < hole.coords.size(); ++j){
            const auto &b = hole.coords.at(j);
            if(bridge_makes_collinear_splice(working, i, hole, j)){
                continue;
            }
            if(bridge_crosses_cycle(working.coords, i, a, b)){
                continue;
            }

            bool blocked = false;
            for(size_t k = 0; k < holes.size(); ++k){
                const auto skip = (k == hole_index) ? j : std::numeric_limits<size_t>::max();
                if(bridge_crosses_cycle(holes.at(k).coords, skip, a, b)){
                    blocked = true;
                    break;
                }
            }
            if(blocked){
                continue;
            }

            const vec2<T> midpoint((a.x + b.x) / static_cast<T>(2),
                                   (a.y + b.y) / static_cast<T>(2));
            if(!point_in_polygon_or_on_boundary_robust(working.coords, midpoint)){
                continue;
            }
            bool midpoint_in_hole = false;
            for(const auto &other_hole : holes){
                if(point_in_polygon_or_on_boundary_robust(other_hole.coords, midpoint)){
                    midpoint_in_hole = true;
                    break;
                }
            }
            if(midpoint_in_hole){
                continue;
            }

            const auto dx = static_cast<long double>(a.x) - static_cast<long double>(b.x);
            const auto dy = static_cast<long double>(a.y) - static_cast<long double>(b.y);
            const auto len = (dx * dx) + (dy * dy);
            if((!found) || (len < best_len)
            // Equal-length visible bridges are geometrically interchangeable here, so fall back to the position pair to
            // keep the preprocessing deterministic across runs and standard-library implementations.
            || ((len == best_len) && (std::make_pair(i, j) < std::make_pair(working_pos, hole_pos)))){
                found = true;
                best_len = len;
                working_pos = i;
                hole_pos = j;
            }
        }
    }
    return found;
}

template <class T>
void splice_hole(WorkingPolygon<T> &working,
                 const WorkingPolygon<T> &hole,
                 size_t working_pos,
                 size_t hole_pos){
    WorkingPolygon<T> merged;
    merged.interior = working.interior;
    merged.refs.reserve(working.refs.size() + hole.refs.size() + 2);
    merged.coords.reserve(working.coords.size() + hole.coords.size() + 2);
    const auto hole_n = hole.refs.size();

    for(size_t i = 0; i <= working_pos; ++i){
        merged.refs.push_back(working.refs.at(i));
        merged.coords.push_back(working.coords.at(i));
    }
    for(size_t offset = 0; offset < hole_n; ++offset){
        const auto idx = (hole_pos + ((hole_n - offset) % hole_n)) % hole_n;
        merged.refs.push_back(hole.refs.at(idx));
        merged.coords.push_back(hole.coords.at(idx));
    }
    merged.refs.push_back(hole.refs.at(hole_pos));
    merged.coords.push_back(hole.coords.at(hole_pos));
    merged.refs.push_back(working.refs.at(working_pos));
    merged.coords.push_back(working.coords.at(working_pos));
    for(size_t i = working_pos + 1; i < working.refs.size(); ++i){
        merged.refs.push_back(working.refs.at(i));
        merged.coords.push_back(working.coords.at(i));
    }

    working = std::move(merged);
}

template <class T>
bool build_region_boundary(const std::vector<std::vector<vec2<T>>> &verts,
                           const std::vector<NormalizedPolygon<T>> &normalized,
                           size_t polygon_pos,
                           WorkingPolygon<T> &working,
                           std::string *diag){
    working = make_working_polygon(verts, normalized.at(polygon_pos));
    std::vector<WorkingPolygon<T>> holes;
    holes.reserve(normalized.at(polygon_pos).children.size());
    for(const auto child_pos : normalized.at(polygon_pos).children){
        holes.push_back(make_working_polygon(verts, normalized.at(child_pos)));
    }

    while(!holes.empty()){
        // The rightmost-child ordering mirrors the usual polygon-with-holes bridge insertion strategy and helps keep the
        // temporary corridors from interfering with not-yet-processed children.
        size_t selected_hole = 0;
        for(size_t i = 1; i < holes.size(); ++i){
            const auto cand = rightmost_vertex(holes.at(i));
            const auto cur = rightmost_vertex(holes.at(selected_hole));
            const auto &cand_v = holes.at(i).coords.at(cand);
            const auto &cur_v = holes.at(selected_hole).coords.at(cur);
            if((cand_v.x > cur_v.x) || ((cand_v.x == cur_v.x) && (cand_v.y < cur_v.y))){
                selected_hole = i;
            }
        }

        size_t working_pos = 0;
        size_t hole_pos = 0;
        if(!find_bridge(working, holes, selected_hole, working_pos, hole_pos)){
            return monotone_fail(diag, "Failed to construct a non-intersecting bridge between a polygon and one of its directly nested children.");
        }
        splice_hole(working, holes.at(selected_hole), working_pos, hole_pos);
        holes.erase(holes.begin() + static_cast<std::ptrdiff_t>(selected_hole));
    }
    return true;
}

template <class T, class I>
bool decompose_all_polygons(const std::vector<std::vector<vec2<T>>> &verts,
                            std::vector<monotone_t<I>> &out,
                            std::string *diag){
    out.clear();
    std::vector<NormalizedPolygon<T>> normalized;
    normalized.reserve(verts.size());
    for(size_t polygon_index = 0; polygon_index < verts.size(); ++polygon_index){
        NormalizedPolygon<T> poly;
        if(!normalize_polygon(verts, polygon_index, poly, diag)){
            return false;
        }
        normalized.push_back(std::move(poly));
    }

    for(size_t i = 0; i < normalized.size(); ++i){
        for(size_t j = i + 1; j < normalized.size(); ++j){
            if(!validate_polygon_pair(verts.at(normalized.at(i).polygon_index),
                                      normalized.at(i).original_indices,
                                      verts.at(normalized.at(j).polygon_index),
                                      normalized.at(j).original_indices,
                                      diag)){
                return false;
            }
        }
    }

    if(!build_polygon_tree(verts, normalized)){
        return false;
    }

    for(size_t poly_pos = 0; poly_pos < normalized.size(); ++poly_pos){
        WorkingPolygon<T> working;
        if(!build_region_boundary(verts, normalized, poly_pos, working, diag)){
            return false;
        }

        std::set<std::pair<size_t, size_t>> diagonals;
        if(!decompose_polygon_sweep(working, diagonals, diag)){
            return false;
        }

        std::vector<std::vector<size_t>> faces;
        if(!extract_monotone_faces(working, diagonals, faces, diag)){
            return false;
        }

        if(!convert_faces<T, I>(working, faces, out, diag)){
            return false;
        }
    }

    return true;
}

} // namespace

template <class T, class I>
std::vector<monotone_t<I>>
Monotone_Decomposition_2(const std::vector<std::vector<vec2<T>>> &verts){
    std::vector<monotone_t<I>> output;
    if(verts.empty()){
        YLOGDEBUG("Monotone decomposition received empty input");
        return output;
    }

    if(verts.size() > static_cast<size_t>(std::numeric_limits<I>::max())){
        const auto msg = "Monotone decomposition polygon count exceeds the selected output index type.";
        YLOGWARN(msg);
        throw std::invalid_argument(msg);
    }
    for(size_t i = 0; i < verts.size(); ++i){
        if(verts.at(i).size() > static_cast<size_t>(std::numeric_limits<I>::max())){
            const auto msg = "Monotone decomposition vertex count exceeds the selected output index type for polygon "
                           + std::to_string(i) + ".";
            YLOGWARN(msg);
            throw std::invalid_argument(msg);
        }
    }

    std::string diag;
    if(!decompose_all_polygons<T, I>(verts, output, &diag)){
        YLOGWARN(diag);
        throw std::invalid_argument(diag);
    }

    YLOGDEBUG("Monotone decomposition produced " << output.size() << " monotone polygon(s)");
    return output;
}

#ifndef YGOR_MATH_MONOTONE_DECOMPOSITION_DISABLE_ALL_SPECIALIZATIONS
    template std::vector<monotone_t<uint32_t>> Monotone_Decomposition_2<float , uint32_t>(const std::vector<std::vector<vec2<float >>> &);
    template std::vector<monotone_t<uint64_t>> Monotone_Decomposition_2<float , uint64_t>(const std::vector<std::vector<vec2<float >>> &);

    template std::vector<monotone_t<uint32_t>> Monotone_Decomposition_2<double, uint32_t>(const std::vector<std::vector<vec2<double>>> &);
    template std::vector<monotone_t<uint64_t>> Monotone_Decomposition_2<double, uint64_t>(const std::vector<std::vector<vec2<double>>> &);
#endif
