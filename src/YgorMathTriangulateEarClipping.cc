//YgorMathTriangulateEarClipping.cc.

#include <algorithm>
#include <array>
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
#include "YgorMathTriangulateEarClipping.h"

namespace {

constexpr size_t TE_INVALID_INDEX = std::numeric_limits<size_t>::max();

struct TE_Ring {
    size_t input_polygon_index = 0;
    std::vector<size_t> vertices;
    long double signed_area = 0.0L;
    size_t parent = TE_INVALID_INDEX;
    std::vector<size_t> children;
    size_t depth = 0;
};

struct TE_Diagonal {
    size_t a;
    size_t b;

    bool operator<(const TE_Diagonal &other) const {
        return (a < other.a) || ((a == other.a) && (b < other.b));
    }
};

struct TE_Triangle {
    size_t a;
    size_t b;
    size_t c;
};

struct TE_BridgeChoice {
    size_t outer_pos = TE_INVALID_INDEX;
    size_t hole_pos = TE_INVALID_INDEX;
    long double dist_sq = std::numeric_limits<long double>::infinity();
};

inline TE_Diagonal make_diagonal(size_t a, size_t b){
    if(b < a){
        std::swap(a, b);
    }
    return TE_Diagonal{a, b};
}

inline bool te_fail(std::string *diag, const std::string &msg){
    if(diag != nullptr){
        *diag = msg;
    }
    YLOGDEBUG(msg);
    return false;
}

template <class T>
bool is_finite_2d(const vec2<T> &v){
    return std::isfinite(v.x) && std::isfinite(v.y);
}

template <class T>
bool same_xy(const vec2<T> &a, const vec2<T> &b){
    return (a.x == b.x) && (a.y == b.y);
}

template <class T>
long double sq_dist_ld(const vec2<T> &a, const vec2<T> &b){
    const auto dx = static_cast<long double>(a.x) - static_cast<long double>(b.x);
    const auto dy = static_cast<long double>(a.y) - static_cast<long double>(b.y);
    return (dx * dx) + (dy * dy);
}

template <class T>
long double polygon_signed_area_ld(const std::vector<vec2<T>> &verts,
                                   const std::vector<size_t> &poly){
    const auto &origin = verts.at(poly.front());
    long double area = 0.0L;
    for(size_t i = 0; i < poly.size(); ++i){
        const auto &a = verts.at(poly.at(i));
        const auto &b = verts.at(poly.at((i + 1) % poly.size()));
        const auto ax = static_cast<long double>(a.x) - static_cast<long double>(origin.x);
        const auto ay = static_cast<long double>(a.y) - static_cast<long double>(origin.y);
        const auto bx = static_cast<long double>(b.x) - static_cast<long double>(origin.x);
        const auto by = static_cast<long double>(b.y) - static_cast<long double>(origin.y);
        area += (ax * by) - (bx * ay);
    }
    return area / 2.0L;
}

template <class T>
long double polygon_signed_area_ld_occurrences(const std::vector<vec2<T>> &verts,
                                               const std::vector<size_t> &occurrences,
                                               const std::vector<size_t> &cycle){
    const auto &origin = verts.at(occurrences.at(cycle.front()));
    long double area = 0.0L;
    for(size_t i = 0; i < cycle.size(); ++i){
        const auto &a = verts.at(occurrences.at(cycle.at(i)));
        const auto &b = verts.at(occurrences.at(cycle.at((i + 1) % cycle.size())));
        const auto ax = static_cast<long double>(a.x) - static_cast<long double>(origin.x);
        const auto ay = static_cast<long double>(a.y) - static_cast<long double>(origin.y);
        const auto bx = static_cast<long double>(b.x) - static_cast<long double>(origin.x);
        const auto by = static_cast<long double>(b.y) - static_cast<long double>(origin.y);
        area += (ax * by) - (bx * ay);
    }
    return area / 2.0L;
}

template <class T>
bool make_ccw_triangle(const std::vector<vec2<T>> &verts,
                       size_t a,
                       size_t b,
                       size_t c,
                       TE_Triangle &out){
    const auto o = orient_sign(verts.at(a), verts.at(b), verts.at(c));
    if(o > 0){
        out = TE_Triangle{a, b, c};
        return true;
    }
    if(o < 0){
        out = TE_Triangle{a, c, b};
        return true;
    }
    return false;
}

template <class T>
bool sanitize_polygon_ring(const std::vector<vec2<T>> &input,
                           std::vector<vec2<T>> &sanitized,
                           std::string *diag,
                           size_t polygon_index){
    sanitized.clear();
    sanitized.reserve(input.size());

    for(size_t i = 0; i < input.size(); ++i){
        const auto &v = input.at(i);
        if(!is_finite_2d(v)){
            return te_fail(diag, "Polygon " + std::to_string(polygon_index)
                                 + " contains a non-finite vertex at index " + std::to_string(i) + ".");
        }
        sanitized.push_back(v);
    }

    if(sanitized.size() >= 2 && same_xy(sanitized.front(), sanitized.back())){
        sanitized.pop_back();
    }

    std::vector<vec2<T>> no_duplicates;
    no_duplicates.reserve(sanitized.size());
    for(const auto &v : sanitized){
        if(no_duplicates.empty() || !same_xy(no_duplicates.back(), v)){
            no_duplicates.push_back(v);
        }
    }
    if(no_duplicates.size() >= 2 && same_xy(no_duplicates.front(), no_duplicates.back())){
        no_duplicates.pop_back();
    }
    sanitized.swap(no_duplicates);

    if(sanitized.size() < 3){
        return te_fail(diag, "Polygon " + std::to_string(polygon_index)
                             + " does not contain at least three distinct vertices after removing duplicated endpoints.");
    }

    // Remove consecutive collinear vertices that lie on a closed boundary segment. This directly handles a common
    // degeneracy from real-world contour data while preserving the geometric support of the ring.
    bool changed = true;
    while(changed && (sanitized.size() >= 3)){
        changed = false;
        std::vector<vec2<T>> reduced;
        reduced.reserve(sanitized.size());
        for(size_t i = 0; i < sanitized.size(); ++i){
            const auto &prev = sanitized.at((i + sanitized.size() - 1) % sanitized.size());
            const auto &cur = sanitized.at(i);
            const auto &next = sanitized.at((i + 1) % sanitized.size());
            const bool remove = !same_xy(prev, cur)
                             && !same_xy(cur, next)
                             && (orient_sign(prev, cur, next) == 0)
                             && point_on_closed_segment(cur, prev, next);
            if(remove){
                changed = true;
                continue;
            }
            reduced.push_back(cur);
        }
        sanitized.swap(reduced);
    }

    if(sanitized.size() < 3){
        return te_fail(diag, "Polygon " + std::to_string(polygon_index)
                             + " becomes degenerate after removing collinear vertices.");
    }

    {
        std::map<std::pair<T, T>, size_t> counts;
        for(const auto &v : sanitized){
            const auto key = std::make_pair(v.x, v.y);
            if(++counts[key] > 1){
                return te_fail(diag, "Polygon " + std::to_string(polygon_index)
                                     + " repeats a non-adjacent vertex, which would create a self-touching boundary.");
            }
        }
    }

    const auto area = polygon_signed_area_ld(sanitized,
                                             [&sanitized](){
                                                 std::vector<size_t> idx(sanitized.size());
                                                 for(size_t i = 0; i < sanitized.size(); ++i){
                                                     idx.at(i) = i;
                                                 }
                                                 return idx;
                                             }());
    if(area == 0.0L){
        return te_fail(diag, "Polygon " + std::to_string(polygon_index)
                             + " encloses zero signed area after sanitization.");
    }

    // Guard against self-intersection and against a vertex landing on a non-incident edge.
    for(size_t i = 0; i < sanitized.size(); ++i){
        const auto &a = sanitized.at(i);
        const auto &b = sanitized.at((i + 1) % sanitized.size());
        if(same_xy(a, b)){
            return te_fail(diag, "Polygon " + std::to_string(polygon_index)
                                 + " contains a zero-length edge after sanitization.");
        }
        for(size_t j = 0; j < sanitized.size(); ++j){
            const auto shares_endpoint = (j == i)
                                      || (j == ((i + 1) % sanitized.size()))
                                      || (((j + 1) % sanitized.size()) == i)
                                      || (((j + 1) % sanitized.size()) == ((i + 1) % sanitized.size()));
            if(!shares_endpoint && point_on_open_segment(sanitized.at(j), a, b)){
                return te_fail(diag, "Polygon " + std::to_string(polygon_index)
                                     + " has a vertex lying strictly on a non-incident boundary edge.");
            }
            if(j <= i){
                continue;
            }
            const bool adjacent = shares_endpoint;
            if(adjacent){
                continue;
            }
            const auto &c = sanitized.at(j);
            const auto &d = sanitized.at((j + 1) % sanitized.size());
            if(segments_intersect_beyond_shared_endpoints(a, b, c, d)){
                return te_fail(diag, "Polygon " + std::to_string(polygon_index)
                                     + " self-intersects beyond a shared endpoint.");
            }
        }
    }

    return true;
}

template <class T>
int classify_vertex_against_event_order(const vec2<T> &a, const vec2<T> &b){
    if(a.y > b.y) return 1;
    if(a.y < b.y) return -1;
    if(a.x < b.x) return 1;
    if(a.x > b.x) return -1;
    return 0;
}

template <class T>
bool is_above_event_order(const vec2<T> &a, const vec2<T> &b){
    return classify_vertex_against_event_order(a, b) > 0;
}

template <class T>
bool boundaries_are_disjoint_or_nested(const std::vector<vec2<T>> &verts,
                                       const TE_Ring &lhs,
                                       const TE_Ring &rhs,
                                       std::string *diag){
    for(size_t i = 0; i < lhs.vertices.size(); ++i){
        const auto &a = verts.at(lhs.vertices.at(i));
        const auto &b = verts.at(lhs.vertices.at((i + 1) % lhs.vertices.size()));
        for(size_t j = 0; j < rhs.vertices.size(); ++j){
            const auto &c = verts.at(rhs.vertices.at(j));
            const auto &d = verts.at(rhs.vertices.at((j + 1) % rhs.vertices.size()));
            if(same_xy(a, c) || same_xy(a, d) || same_xy(b, c) || same_xy(b, d)){
                return te_fail(diag, "Polygon boundaries touch at a shared vertex; this degeneracy is not supported.");
            }
            if(point_on_open_segment(a, c, d) || point_on_open_segment(b, c, d)
            || point_on_open_segment(c, a, b) || point_on_open_segment(d, a, b)){
                return te_fail(diag, "Polygon boundaries touch because a vertex lies on another polygon boundary.");
            }
            if(segments_intersect_beyond_shared_endpoints(a, b, c, d)){
                return te_fail(diag, "Polygon boundaries intersect; only disjoint or strictly nested rings are supported.");
            }
        }
    }
    return true;
}

template <class T>
bool point_strictly_inside_ring(const std::vector<vec2<T>> &verts,
                                const TE_Ring &ring,
                                const vec2<T> &p){
    if(!point_in_polygon_or_on_boundary(verts, ring.vertices, p)){
        return false;
    }
    for(size_t i = 0; i < ring.vertices.size(); ++i){
        const auto &a = verts.at(ring.vertices.at(i));
        const auto &b = verts.at(ring.vertices.at((i + 1) % ring.vertices.size()));
        if(point_on_closed_segment(p, a, b)){
            return false;
        }
    }
    return true;
}

template <class T>
bool build_ring_nesting(std::vector<TE_Ring> &rings,
                        const std::vector<vec2<T>> &verts,
                        std::string *diag){
    for(size_t i = 0; i < rings.size(); ++i){
        for(size_t j = i + 1; j < rings.size(); ++j){
            if(!boundaries_are_disjoint_or_nested(verts, rings.at(i), rings.at(j), diag)){
                return false;
            }
        }
    }

    for(size_t child = 0; child < rings.size(); ++child){
        const auto sample = verts.at(rings.at(child).vertices.front());
        size_t parent = TE_INVALID_INDEX;
        long double parent_abs_area = std::numeric_limits<long double>::infinity();
        for(size_t candidate = 0; candidate < rings.size(); ++candidate){
            if(candidate == child){
                continue;
            }
            if(!point_strictly_inside_ring(verts, rings.at(candidate), sample)){
                continue;
            }
            const auto abs_area = std::abs(rings.at(candidate).signed_area);
            if(abs_area < parent_abs_area){
                parent_abs_area = abs_area;
                parent = candidate;
            }
        }
        rings.at(child).parent = parent;
    }

    for(auto &ring : rings){
        ring.children.clear();
        ring.depth = 0;
    }
    for(size_t i = 0; i < rings.size(); ++i){
        const auto parent = rings.at(i).parent;
        if(parent != TE_INVALID_INDEX){
            rings.at(parent).children.push_back(i);
        }
    }
    for(size_t i = 0; i < rings.size(); ++i){
        size_t depth = 0;
        size_t cur = rings.at(i).parent;
        std::set<size_t> seen;
        while(cur != TE_INVALID_INDEX){
            if(!seen.insert(cur).second){
                return te_fail(diag, "Ring nesting became cyclic while classifying polygon containment.");
            }
            ++depth;
            cur = rings.at(cur).parent;
        }
        rings.at(i).depth = depth;
    }

    return true;
}

template <class T>
std::vector<size_t> oriented_ring_vertices(const std::vector<vec2<T>> &verts,
                                           const TE_Ring &ring,
                                           bool want_ccw){
    auto out = ring.vertices;
    const bool is_ccw = (polygon_signed_area_ld(verts, out) > 0.0L);
    if(is_ccw != want_ccw){
        std::reverse(out.begin(), out.end());
    }
    return out;
}

template <class T>
bool segment_intersects_cycle_except_incident(const std::vector<vec2<T>> &verts,
                                              const std::vector<size_t> &cycle,
                                              size_t outer_pos,
                                              const vec2<T> &a,
                                              const vec2<T> &b){
    for(size_t i = 0; i < cycle.size(); ++i){
        const auto ia = cycle.at(i);
        const auto ib = cycle.at((i + 1) % cycle.size());
        if((i == outer_pos) || (((i + 1) % cycle.size()) == outer_pos)){
            continue;
        }
        if(point_on_open_segment(verts.at(ia), a, b) || point_on_open_segment(verts.at(ib), a, b)){
            return true;
        }
        if(segments_intersect_beyond_shared_endpoints(a, b, verts.at(ia), verts.at(ib))){
            return true;
        }
    }
    return false;
}

template <class T>
bool segment_intersects_ring_except_incident(const std::vector<vec2<T>> &verts,
                                             const std::vector<size_t> &ring,
                                             size_t hole_pos,
                                             const vec2<T> &a,
                                             const vec2<T> &b){
    for(size_t i = 0; i < ring.size(); ++i){
        const auto ia = ring.at(i);
        const auto ib = ring.at((i + 1) % ring.size());
        if((i == hole_pos) || (((i + 1) % ring.size()) == hole_pos)){
            continue;
        }
        if(point_on_open_segment(verts.at(ia), a, b) || point_on_open_segment(verts.at(ib), a, b)){
            return true;
        }
        if(segments_intersect_beyond_shared_endpoints(a, b, verts.at(ia), verts.at(ib))){
            return true;
        }
    }
    return false;
}

template <class T>
TE_BridgeChoice choose_visible_bridge(const std::vector<vec2<T>> &verts,
                                      const std::vector<size_t> &cycle,
                                      const std::vector<size_t> &hole,
                                      const std::vector<std::vector<size_t>> &other_holes){
    // Bridge each hole through its right-most vertex. This follows the standard hole-elimination approach discussed in
    // de Berg et al., Chapter 3, but uses an exhaustive visibility search among boundary vertices to remain maintainable
    // in the presence of degenerate real-world geometry.
    size_t hole_pos = 0;
    for(size_t i = 1; i < hole.size(); ++i){
        const auto &best = verts.at(hole.at(hole_pos));
        const auto &cur = verts.at(hole.at(i));
        if((cur.x > best.x) || ((cur.x == best.x) && (cur.y < best.y))){
            hole_pos = i;
        }
    }

    TE_BridgeChoice best_choice;
    const auto &hole_anchor = verts.at(hole.at(hole_pos));
    for(size_t outer_pos = 0; outer_pos < cycle.size(); ++outer_pos){
        const auto &outer_anchor = verts.at(cycle.at(outer_pos));
        if(same_xy(hole_anchor, outer_anchor)){
            continue;
        }
        if(segment_intersects_cycle_except_incident(verts, cycle, outer_pos, hole_anchor, outer_anchor)){
            continue;
        }
        if(segment_intersects_ring_except_incident(verts, hole, hole_pos, hole_anchor, outer_anchor)){
            continue;
        }
        bool intersects_other_hole = false;
        for(const auto &other_hole : other_holes){
            if(&other_hole == &hole){
                continue;
            }
            if(segment_intersects_ring_except_incident(verts, other_hole, TE_INVALID_INDEX, hole_anchor, outer_anchor)){
                intersects_other_hole = true;
                break;
            }
        }
        if(intersects_other_hole){
            continue;
        }

        const auto dist_sq = sq_dist_ld(hole_anchor, outer_anchor);
        if(dist_sq < best_choice.dist_sq){
            best_choice = TE_BridgeChoice{ outer_pos, hole_pos, dist_sq };
        }
    }
    return best_choice;
}

template <class T>
bool merge_hole_into_cycle(const std::vector<vec2<T>> &verts,
                           std::vector<size_t> &cycle,
                           const std::vector<size_t> &hole,
                           const std::vector<std::vector<size_t>> &other_holes,
                           std::string *diag){
    const auto bridge = choose_visible_bridge(verts, cycle, hole, other_holes);
    if((bridge.outer_pos == TE_INVALID_INDEX) || (bridge.hole_pos == TE_INVALID_INDEX)){
        return te_fail(diag, "Unable to find a visible bridge from a hole boundary to its enclosing boundary.");
    }

    std::vector<size_t> merged;
    merged.reserve(cycle.size() + hole.size() + 2);
    for(size_t i = 0; i <= bridge.outer_pos; ++i){
        merged.push_back(cycle.at(i));
    }
    for(size_t offset = 0; offset < hole.size(); ++offset){
        merged.push_back(hole.at((bridge.hole_pos + offset) % hole.size()));
    }
    merged.push_back(hole.at(bridge.hole_pos));
    merged.push_back(cycle.at(bridge.outer_pos));
    for(size_t i = bridge.outer_pos + 1; i < cycle.size(); ++i){
        merged.push_back(cycle.at(i));
    }

    // Reject bridges that accidentally created a self-intersection in the weakly simple boundary.
    for(size_t i = 0; i < merged.size(); ++i){
        const auto &a = verts.at(merged.at(i));
        const auto &b = verts.at(merged.at((i + 1) % merged.size()));
        if(same_xy(a, b)){
            return te_fail(diag, "Hole bridging created a zero-length edge in the merged boundary.");
        }
        for(size_t j = i + 1; j < merged.size(); ++j){
            const bool adjacent = (j == i)
                               || (((j + 1) % merged.size()) == i)
                               || (((i + 1) % merged.size()) == j);
            if(adjacent){
                continue;
            }
            const auto &c = verts.at(merged.at(j));
            const auto &d = verts.at(merged.at((j + 1) % merged.size()));
            if(segments_intersect_beyond_shared_endpoints(a, b, c, d)){
                return te_fail(diag, "Hole bridging introduced a self-intersection in the merged polygon boundary.");
            }
        }
    }

    cycle.swap(merged);
    return true;
}

enum class TE_VertexType {
    start,
    end,
    split,
    merge,
    regular
};

template <class T>
TE_VertexType classify_vertex_type(const std::vector<vec2<T>> &verts,
                                   const std::vector<size_t> &occurrences,
                                   size_t pos){
    const auto n = occurrences.size();
    const auto prev = occurrences.at((pos + n - 1) % n);
    const auto cur = occurrences.at(pos);
    const auto next = occurrences.at((pos + 1) % n);

    const bool prev_above = is_above_event_order(verts.at(prev), verts.at(cur));
    const bool next_above = is_above_event_order(verts.at(next), verts.at(cur));
    const bool prev_below = is_above_event_order(verts.at(cur), verts.at(prev));
    const bool next_below = is_above_event_order(verts.at(cur), verts.at(next));
    const auto turn = orient_sign(verts.at(prev), verts.at(cur), verts.at(next));

    if(prev_below && next_below){
        return (turn > 0) ? TE_VertexType::start : TE_VertexType::split;
    }
    if(prev_above && next_above){
        return (turn > 0) ? TE_VertexType::end : TE_VertexType::merge;
    }
    return TE_VertexType::regular;
}

template <class T>
struct TE_SweepContext {
    const std::vector<vec2<T>> *verts = nullptr;
    const std::vector<size_t> *occurrences = nullptr;
    long double query_y = 0.0L;
    long double query_x = 0.0L;
    size_t query_vertex = TE_INVALID_INDEX;
};

template <class T>
long double edge_x_at_query_y(const TE_SweepContext<T> &ctx, size_t edge_start){
    const auto &occ = *ctx.occurrences;
    const auto &verts = *ctx.verts;
    const auto &a = verts.at(occ.at(edge_start));
    const auto &b = verts.at(occ.at((edge_start + 1) % occ.size()));

    const auto ay = static_cast<long double>(a.y);
    const auto by = static_cast<long double>(b.y);
    const auto ax = static_cast<long double>(a.x);
    const auto bx = static_cast<long double>(b.x);
    if(ay == by){
        return std::min(ax, bx);
    }
    const auto t = (ctx.query_y - ay) / (by - ay);
    return ax + t * (bx - ax);
}

template <class T>
struct TE_StatusComparator {
    const TE_SweepContext<T> *ctx = nullptr;

    bool operator()(size_t lhs, size_t rhs) const {
        if(lhs == rhs){
            return false;
        }
        const bool lhs_is_query = (lhs == TE_INVALID_INDEX);
        const bool rhs_is_query = (rhs == TE_INVALID_INDEX);
        const auto lhs_x = lhs_is_query ? ctx->query_x : edge_x_at_query_y(*ctx, lhs);
        const auto rhs_x = rhs_is_query ? ctx->query_x : edge_x_at_query_y(*ctx, rhs);
        if(lhs_x < rhs_x) return true;
        if(lhs_x > rhs_x) return false;

        if(lhs_is_query || rhs_is_query){
            return lhs < rhs;
        }

        const auto &occ = *ctx->occurrences;
        const auto &verts = *ctx->verts;
        const auto &la = verts.at(occ.at(lhs));
        const auto &lb = verts.at(occ.at((lhs + 1) % occ.size()));
        const auto &ra = verts.at(occ.at(rhs));
        const auto &rb = verts.at(occ.at((rhs + 1) % occ.size()));
        const auto lhs_top = is_above_event_order(la, lb) ? la : lb;
        const auto lhs_bottom = is_above_event_order(la, lb) ? lb : la;
        const auto rhs_top = is_above_event_order(ra, rb) ? ra : rb;
        const auto rhs_bottom = is_above_event_order(ra, rb) ? rb : ra;

        if(lhs_top.x < rhs_top.x) return true;
        if(lhs_top.x > rhs_top.x) return false;
        if(lhs_bottom.x < rhs_bottom.x) return true;
        if(lhs_bottom.x > rhs_bottom.x) return false;
        return lhs < rhs;
    }
};

template <class T>
bool edge_is_active_below_vertex(const std::vector<vec2<T>> &verts,
                                 const std::vector<size_t> &occurrences,
                                 size_t edge_start,
                                 size_t vertex_pos){
    const auto &a = verts.at(occurrences.at(edge_start));
    const auto &b = verts.at(occurrences.at((edge_start + 1) % occurrences.size()));
    const auto &v = verts.at(occurrences.at(vertex_pos));
    const bool a_is_above_v = is_above_event_order(a, v);
    const bool b_is_above_v = is_above_event_order(b, v);
    const bool v_is_above_a = is_above_event_order(v, a);
    const bool v_is_above_b = is_above_event_order(v, b);
    return (a_is_above_v != b_is_above_v) || ((a == v) && v_is_above_b) || ((b == v) && v_is_above_a);
}

template <class T>
size_t left_edge_of_vertex(const std::set<size_t, TE_StatusComparator<T>> &status,
                           TE_SweepContext<T> &ctx,
                           const std::vector<vec2<T>> &verts,
                           const std::vector<size_t> &occurrences,
                           size_t vertex_pos){
    const auto &v = verts.at(occurrences.at(vertex_pos));
    const auto bbox_scale = std::max(std::abs(static_cast<long double>(v.x)), std::abs(static_cast<long double>(v.y))) + 1.0L;
    ctx.query_y = static_cast<long double>(v.y) - (std::numeric_limits<long double>::epsilon() * bbox_scale * 16.0L);
    ctx.query_x = static_cast<long double>(v.x);
    ctx.query_vertex = vertex_pos;

    const auto it = status.lower_bound(TE_INVALID_INDEX);
    if(it == status.begin()){
        return TE_INVALID_INDEX;
    }
    if(it == status.end()){
        return *std::prev(status.end());
    }
    return *std::prev(it);
}

template <class T>
bool add_diagonal_if_valid(const std::vector<vec2<T>> &verts,
                           const std::vector<size_t> &occurrences,
                           size_t a,
                           size_t b,
                           std::set<TE_Diagonal> &diagonals,
                           std::string *diag){
    if((a == b)
    || (((a + 1) % occurrences.size()) == b)
    || (((b + 1) % occurrences.size()) == a)){
        return true;
    }
    const auto key = make_diagonal(a, b);
    if(diagonals.count(key) != 0){
        return true;
    }
    if(orient_sign(verts.at(occurrences.at(a)), verts.at(occurrences.at(b)),
                   verts.at(occurrences.at((b + 1) % occurrences.size()))) == 0
    && orient_sign(verts.at(occurrences.at((a + occurrences.size() - 1) % occurrences.size())),
                   verts.at(occurrences.at(a)), verts.at(occurrences.at(b))) == 0){
        return te_fail(diag, "Sweep-line monotone decomposition attempted to add a degenerate diagonal.");
    }
    diagonals.insert(key);
    return true;
}

template <class T>
bool decompose_into_monotone_faces(const std::vector<vec2<T>> &verts,
                                   const std::vector<size_t> &occurrences,
                                   std::set<TE_Diagonal> &diagonals,
                                   std::string *diag){
    diagonals.clear();
    if(occurrences.size() < 3){
        return te_fail(diag, "Merged polygon contains fewer than three boundary vertices.");
    }

    std::vector<size_t> events(occurrences.size());
    for(size_t i = 0; i < occurrences.size(); ++i){
        events.at(i) = i;
    }
    std::sort(events.begin(), events.end(),
              [&](size_t lhs, size_t rhs){
                  const auto &a = verts.at(occurrences.at(lhs));
                  const auto &b = verts.at(occurrences.at(rhs));
                  const auto cmp = classify_vertex_against_event_order(a, b);
                  return (cmp > 0) || ((cmp == 0) && (lhs < rhs));
              });

    TE_SweepContext<T> ctx;
    ctx.verts = &verts;
    ctx.occurrences = &occurrences;
    TE_StatusComparator<T> comp;
    comp.ctx = &ctx;
    std::set<size_t, TE_StatusComparator<T>> status(comp);
    std::map<size_t, size_t> helper;

    for(const auto vertex_pos : events){
        const auto type = classify_vertex_type(verts, occurrences, vertex_pos);
        const auto prev_edge = (vertex_pos + occurrences.size() - 1) % occurrences.size();
        const auto next_edge = vertex_pos;
        const auto next_occ = (vertex_pos + 1) % occurrences.size();
        const bool next_below = is_above_event_order(verts.at(occurrences.at(vertex_pos)),
                                                     verts.at(occurrences.at(next_occ)));

        switch(type){
            case TE_VertexType::start:
                status.insert(next_edge);
                helper[next_edge] = vertex_pos;
                break;

            case TE_VertexType::end:
                if((helper.count(prev_edge) != 0)
                && (classify_vertex_type(verts, occurrences, helper.at(prev_edge)) == TE_VertexType::merge)
                && !add_diagonal_if_valid(verts, occurrences, vertex_pos, helper.at(prev_edge), diagonals, diag)){
                    return false;
                }
                helper.erase(prev_edge);
                status.erase(prev_edge);
                break;

            case TE_VertexType::split: {
                const auto left_edge = left_edge_of_vertex(status, ctx, verts, occurrences, vertex_pos);
                if(left_edge == TE_INVALID_INDEX){
                    return te_fail(diag, "Sweep-line monotone decomposition could not locate the edge immediately left of a split vertex.");
                }
                if((helper.count(left_edge) != 0)
                && !add_diagonal_if_valid(verts, occurrences, vertex_pos, helper.at(left_edge), diagonals, diag)){
                    return false;
                }
                helper[left_edge] = vertex_pos;
                status.insert(next_edge);
                helper[next_edge] = vertex_pos;
                break;
            }

            case TE_VertexType::merge: {
                if((helper.count(prev_edge) != 0)
                && (classify_vertex_type(verts, occurrences, helper.at(prev_edge)) == TE_VertexType::merge)
                && !add_diagonal_if_valid(verts, occurrences, vertex_pos, helper.at(prev_edge), diagonals, diag)){
                    return false;
                }
                helper.erase(prev_edge);
                status.erase(prev_edge);

                const auto left_edge = left_edge_of_vertex(status, ctx, verts, occurrences, vertex_pos);
                if(left_edge == TE_INVALID_INDEX){
                    return te_fail(diag, "Sweep-line monotone decomposition could not locate the edge immediately left of a merge vertex.");
                }
                if((helper.count(left_edge) != 0)
                && (classify_vertex_type(verts, occurrences, helper.at(left_edge)) == TE_VertexType::merge)
                && !add_diagonal_if_valid(verts, occurrences, vertex_pos, helper.at(left_edge), diagonals, diag)){
                    return false;
                }
                helper[left_edge] = vertex_pos;
                break;
            }

            case TE_VertexType::regular:
                if(next_below){
                    if((helper.count(prev_edge) != 0)
                    && (classify_vertex_type(verts, occurrences, helper.at(prev_edge)) == TE_VertexType::merge)
                    && !add_diagonal_if_valid(verts, occurrences, vertex_pos, helper.at(prev_edge), diagonals, diag)){
                        return false;
                    }
                    helper.erase(prev_edge);
                    status.erase(prev_edge);
                    status.insert(next_edge);
                    helper[next_edge] = vertex_pos;
                }else{
                    const auto left_edge = left_edge_of_vertex(status, ctx, verts, occurrences, vertex_pos);
                    if(left_edge == TE_INVALID_INDEX){
                        return te_fail(diag, "Sweep-line monotone decomposition could not locate the edge immediately left of a regular vertex.");
                    }
                    if((helper.count(left_edge) != 0)
                    && (classify_vertex_type(verts, occurrences, helper.at(left_edge)) == TE_VertexType::merge)
                    && !add_diagonal_if_valid(verts, occurrences, vertex_pos, helper.at(left_edge), diagonals, diag)){
                        return false;
                    }
                    helper[left_edge] = vertex_pos;
                }
                break;
        }
    }

    return true;
}

template <class T>
bool extract_faces_from_diagonals(const std::vector<vec2<T>> &verts,
                                  const std::vector<size_t> &occurrences,
                                  const std::set<TE_Diagonal> &diagonals,
                                  std::vector<std::vector<size_t>> &faces,
                                  std::string *diag){
    faces.clear();
    std::vector<std::vector<size_t>> adjacency(occurrences.size());
    for(size_t i = 0; i < occurrences.size(); ++i){
        adjacency.at(i).push_back((i + 1) % occurrences.size());
        adjacency.at((i + 1) % occurrences.size()).push_back(i);
    }
    for(const auto &edge : diagonals){
        adjacency.at(edge.a).push_back(edge.b);
        adjacency.at(edge.b).push_back(edge.a);
    }

    for(size_t v = 0; v < adjacency.size(); ++v){
        auto &nbrs = adjacency.at(v);
        std::sort(nbrs.begin(), nbrs.end(),
                  [&](size_t lhs, size_t rhs){
                      const auto &origin = verts.at(occurrences.at(v));
                      const auto &a = verts.at(occurrences.at(lhs));
                      const auto &b = verts.at(occurrences.at(rhs));
                      const auto angle_a = std::atan2(static_cast<long double>(a.y) - static_cast<long double>(origin.y),
                                                      static_cast<long double>(a.x) - static_cast<long double>(origin.x));
                      const auto angle_b = std::atan2(static_cast<long double>(b.y) - static_cast<long double>(origin.y),
                                                      static_cast<long double>(b.x) - static_cast<long double>(origin.x));
                      if(angle_a < angle_b) return true;
                      if(angle_a > angle_b) return false;
                      return lhs < rhs;
                  });
        nbrs.erase(std::unique(nbrs.begin(), nbrs.end()), nbrs.end());
        if(nbrs.size() < 2){
            return te_fail(diag, "Monotone decomposition produced a disconnected boundary vertex while extracting faces.");
        }
    }

    std::set<std::pair<size_t, size_t>> visited_halfedges;
    for(size_t start = 0; start < adjacency.size(); ++start){
        for(const auto first : adjacency.at(start)){
            const auto start_halfedge = std::make_pair(start, first);
            if(visited_halfedges.count(start_halfedge) != 0){
                continue;
            }

            std::vector<size_t> face;
            size_t prev = start;
            size_t cur = first;
            while(true){
                const auto halfedge = std::make_pair(prev, cur);
                if(!visited_halfedges.insert(halfedge).second){
                    break;
                }
                face.push_back(prev);

                const auto &ordered = adjacency.at(cur);
                const auto it = std::find(ordered.begin(), ordered.end(), prev);
                if(it == ordered.end()){
                    return te_fail(diag, "Halfedge traversal lost local angular ordering while extracting monotone faces.");
                }
                const auto idx = static_cast<size_t>(std::distance(ordered.begin(), it));
                const auto next = ordered.at((idx + ordered.size() - 1) % ordered.size());
                prev = cur;
                cur = next;
                if((prev == start_halfedge.first) && (cur == start_halfedge.second)){
                    break;
                }
                if(face.size() > (occurrences.size() + diagonals.size() * 2 + 4)){
                    return te_fail(diag, "Halfedge traversal exceeded the guard while extracting monotone faces.");
                }
            }

            if(face.size() < 3){
                continue;
            }
            const auto area = polygon_signed_area_ld_occurrences(verts, occurrences, face);
            if(area > 0.0L){
                faces.push_back(std::move(face));
            }
        }
    }

    if(faces.empty()){
        return te_fail(diag, "Monotone decomposition produced no positive-area interior faces.");
    }
    return true;
}

template <class T>
bool triangulate_face_ear_clip(const std::vector<vec2<T>> &verts,
                               const std::vector<size_t> &occurrences,
                               const std::vector<size_t> &face,
                               std::vector<TE_Triangle> &triangles,
                               std::string *diag){
    if(face.size() < 3){
        return true;
    }

    auto poly = face;
    const auto signed_area = polygon_signed_area_ld_occurrences(verts, occurrences, poly);
    if(signed_area == 0.0L){
        return te_fail(diag, "Ear clipping encountered a zero-area monotone face.");
    }
    if(signed_area < 0.0L){
        std::reverse(poly.begin(), poly.end());
    }

    while(poly.size() > 3){
        bool clipped = false;
        const auto n = poly.size();
        for(size_t i = 0; i < n; ++i){
            const auto prev_occ = poly.at((i + n - 1) % n);
            const auto cur_occ  = poly.at(i);
            const auto next_occ = poly.at((i + 1) % n);
            const auto prev_idx = occurrences.at(prev_occ);
            const auto cur_idx  = occurrences.at(cur_occ);
            const auto next_idx = occurrences.at(next_occ);
            if(same_xy(verts.at(prev_idx), verts.at(cur_idx))
            || same_xy(verts.at(cur_idx), verts.at(next_idx))
            || same_xy(verts.at(prev_idx), verts.at(next_idx))){
                poly.erase(poly.begin() + static_cast<std::ptrdiff_t>(i));
                clipped = true;
                break;
            }
            if(orient_sign(verts.at(prev_idx), verts.at(cur_idx), verts.at(next_idx)) <= 0){
                continue;
            }

            bool contains_other = false;
            for(const auto test_occ : poly){
                if((test_occ == prev_occ) || (test_occ == cur_occ) || (test_occ == next_occ)){
                    continue;
                }
                const auto test_idx = occurrences.at(test_occ);
                if(same_xy(verts.at(test_idx), verts.at(prev_idx))
                || same_xy(verts.at(test_idx), verts.at(cur_idx))
                || same_xy(verts.at(test_idx), verts.at(next_idx))){
                    continue;
                }
                if(point_in_triangle_or_on_boundary(verts.at(test_idx),
                                                    verts.at(prev_idx),
                                                    verts.at(cur_idx),
                                                    verts.at(next_idx))){
                    contains_other = true;
                    break;
                }
            }
            if(contains_other){
                continue;
            }

            TE_Triangle tri{};
            if(make_ccw_triangle(verts, prev_idx, cur_idx, next_idx, tri)){
                triangles.push_back(tri);
            }
            poly.erase(poly.begin() + static_cast<std::ptrdiff_t>(i));
            clipped = true;
            break;
        }
        if(!clipped){
            return te_fail(diag, "Ear clipping could not find a valid ear in a monotone face.");
        }
    }

    if(poly.size() == 3){
        TE_Triangle tri{};
        if(make_ccw_triangle(verts,
                             occurrences.at(poly.at(0)),
                             occurrences.at(poly.at(1)),
                             occurrences.at(poly.at(2)),
                             tri)){
            triangles.push_back(tri);
        }
    }
    return true;
}

template <class T>
bool triangulate_component(const std::vector<vec2<T>> &verts,
                           const std::vector<size_t> &outer,
                           const std::vector<std::vector<size_t>> &holes,
                           std::vector<TE_Triangle> &triangles,
                           std::string *diag){
    std::vector<size_t> cycle = outer;
    for(size_t h = 0; h < holes.size(); ++h){
        std::vector<std::vector<size_t>> remaining_holes;
        for(size_t other = h + 1; other < holes.size(); ++other){
            remaining_holes.push_back(holes.at(other));
        }
        if(!merge_hole_into_cycle(verts, cycle, holes.at(h), remaining_holes, diag)){
            return false;
        }
    }

    std::set<TE_Diagonal> diagonals;
    if(!decompose_into_monotone_faces(verts, cycle, diagonals, diag)){
        return false;
    }

    std::vector<std::vector<size_t>> faces;
    if(!extract_faces_from_diagonals(verts, cycle, diagonals, faces, diag)){
        return false;
    }

    for(const auto &face : faces){
        if(!triangulate_face_ear_clip(verts, cycle, face, triangles, diag)){
            return false;
        }
    }
    return true;
}

template <class T>
void prune_triangles(const std::vector<vec2<T>> &verts,
                     std::vector<TE_Triangle> &triangles){
    std::set<std::array<size_t, 3>> seen;
    std::vector<TE_Triangle> filtered;
    filtered.reserve(triangles.size());
    for(const auto &tri : triangles){
        if((tri.a == tri.b) || (tri.b == tri.c) || (tri.c == tri.a)){
            continue;
        }
        if(orient_sign(verts.at(tri.a), verts.at(tri.b), verts.at(tri.c)) == 0){
            continue;
        }
        auto key = std::array<size_t, 3>{{ tri.a, tri.b, tri.c }};
        std::sort(key.begin(), key.end());
        if(seen.insert(key).second){
            filtered.push_back(tri);
        }
    }
    triangles.swap(filtered);
}

} // namespace


template <class T, class I>
fv_surface_mesh<T, I>
Triangulate_Ear_Clipping_2(const std::vector<std::vector<vec2<T>>> &closed_polygons){
    fv_surface_mesh<T, I> mesh;
    if(closed_polygons.empty()){
        YLOGDEBUG("Ear-clipping triangulation received empty polygon input");
        return mesh;
    }

    std::vector<vec2<T>> verts;
    std::vector<TE_Ring> rings;
    std::string diag;

    for(size_t polygon_index = 0; polygon_index < closed_polygons.size(); ++polygon_index){
        const auto &poly = closed_polygons.at(polygon_index);
        if(poly.empty()){
            YLOGWARN("Refusing ear-clipping triangulation because polygon " << polygon_index << " is empty");
            throw std::invalid_argument("Ear-clipping triangulation does not accept empty polygon rings.");
        }

        std::vector<vec2<T>> sanitized;
        if(!sanitize_polygon_ring(poly, sanitized, &diag, polygon_index)){
            YLOGWARN(diag);
            throw std::invalid_argument(diag);
        }

        TE_Ring ring;
        ring.input_polygon_index = polygon_index;
        ring.vertices.reserve(sanitized.size());
        for(const auto &v : sanitized){
            ring.vertices.push_back(verts.size());
            verts.push_back(v);
        }
        ring.signed_area = polygon_signed_area_ld(verts, ring.vertices);
        rings.push_back(std::move(ring));
    }

    if(!build_ring_nesting(rings, verts, &diag)){
        YLOGWARN(diag);
        throw std::invalid_argument(diag);
    }

    mesh.vertices.reserve(verts.size());
    for(const auto &v : verts){
        mesh.vertices.emplace_back(v.x, v.y, static_cast<T>(0));
    }

    std::vector<TE_Triangle> triangles;
    for(size_t i = 0; i < rings.size(); ++i){
        if((rings.at(i).depth % 2) != 0){
            continue;
        }

        const auto outer = oriented_ring_vertices(verts, rings.at(i), true);
        std::vector<std::vector<size_t>> holes;
        for(const auto child : rings.at(i).children){
            holes.push_back(oriented_ring_vertices(verts, rings.at(child), false));
        }

        if(!triangulate_component(verts, outer, holes, triangles, &diag)){
            YLOGWARN(diag);
            throw std::runtime_error(diag);
        }
    }

    prune_triangles(verts, triangles);
    if(triangles.empty()){
        const auto msg = "Ear-clipping triangulation produced no interior triangles for the supplied polygon arrangement.";
        YLOGWARN(msg);
        throw std::runtime_error(msg);
    }

    for(const auto &tri : triangles){
        mesh.faces.push_back({ static_cast<I>(tri.a), static_cast<I>(tri.b), static_cast<I>(tri.c) });
    }

    YLOGDEBUG("Ear-clipping triangulation produced " << mesh.faces.size() << " triangle(s)");
    return mesh;
}

#ifndef YGOR_MATH_TRIANGULATE_EAR_CLIPPING_DISABLE_ALL_SPECIALIZATIONS
    template fv_surface_mesh<float , uint32_t> Triangulate_Ear_Clipping_2(const std::vector<std::vector<vec2<float >>> &);
    template fv_surface_mesh<float , uint64_t> Triangulate_Ear_Clipping_2(const std::vector<std::vector<vec2<float >>> &);

    template fv_surface_mesh<double, uint32_t> Triangulate_Ear_Clipping_2(const std::vector<std::vector<vec2<double>>> &);
    template fv_surface_mesh<double, uint64_t> Triangulate_Ear_Clipping_2(const std::vector<std::vector<vec2<double>>> &);
#endif
