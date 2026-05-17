//YgorMathDelaunayViaVoronoi.cc.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMathDelaunay.h"
#include "YgorMathDelaunayViaVoronoi.h"

namespace {

// Allow a few extra half-edge traversals beyond the nominal 2*m walk so the
// face-recovery loop can safely step past a repeated seed half-edge and still
// close the cycle before declaring the topology inconsistent.
constexpr size_t FORTUNE_FACE_GUARD_BIAS = 8;

struct FortuneTriangle {
    size_t a;
    size_t b;
    size_t c;
};

template <class T>
bool is_finite_2d(const vec2<T> &v){
    return std::isfinite(v.x) && std::isfinite(v.y);
}

template <class T>
bool same_xy(const vec2<T> &a, const vec2<T> &b){
    return (a.x == b.x) && (a.y == b.y);
}

template <class T>
bool has_non_collinear_triplet(const std::vector<vec2<T>> &verts){
    size_t a = verts.size();
    for(size_t i = 0; i < verts.size(); ++i){
        if(is_finite_2d(verts.at(i))){
            a = i;
            break;
        }
    }
    if(a == verts.size()){
        return false;
    }

    size_t b = verts.size();
    for(size_t i = a + 1; i < verts.size(); ++i){
        if(is_finite_2d(verts.at(i)) && !same_xy(verts.at(a), verts.at(i))){
            b = i;
            break;
        }
    }
    if(b == verts.size()){
        return false;
    }

    for(size_t i = b + 1; i < verts.size(); ++i){
        if(!is_finite_2d(verts.at(i))){
            continue;
        }
        if(orient_sign(verts.at(a), verts.at(b), verts.at(i)) != 0){
            return true;
        }
    }
    return false;
}

template <class T>
bool make_ccw_triangle(const std::vector<vec2<T>> &verts,
                       size_t a,
                       size_t b,
                       size_t c,
                       FortuneTriangle &out){
    const auto sign = orient_sign(verts.at(a), verts.at(b), verts.at(c));
    if(sign > 0){
        out = FortuneTriangle{a, b, c};
        return true;
    }
    if(sign < 0){
        out = FortuneTriangle{a, c, b};
        return true;
    }
    return false;
}

template <class T>
long double signed_area_ld(const std::vector<vec2<T>> &verts,
                           const std::vector<size_t> &poly){
    long double area = 0.0L;
    for(size_t i = 0; i < poly.size(); ++i){
        const auto &a = verts.at(poly.at(i));
        const auto &b = verts.at(poly.at((i + 1) % poly.size()));
        area += static_cast<long double>(a.x) * static_cast<long double>(b.y)
              - static_cast<long double>(a.y) * static_cast<long double>(b.x);
    }
    return area * 0.5L;
}

template <class T>
bool is_convex_polygon(const std::vector<vec2<T>> &verts,
                       const std::vector<size_t> &poly){
    if(poly.size() < 3){
        return false;
    }
    int sign = 0;
    for(size_t i = 0; i < poly.size(); ++i){
        const auto o = orient_sign(verts.at(poly.at(i)),
                                   verts.at(poly.at((i + 1) % poly.size())),
                                   verts.at(poly.at((i + 2) % poly.size())));
        if(o == 0){
            continue;
        }
        if(sign == 0){
            sign = o;
            continue;
        }
        if(sign != o){
            return false;
        }
    }
    return true;
}

template <class T>
void prune_triangles(const std::vector<vec2<T>> &verts,
                     std::vector<FortuneTriangle> &triangles){
    std::set<std::array<size_t, 3>> seen;
    std::vector<FortuneTriangle> filtered;
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

template <class T>
bool triangulate_simple_polygon(const std::vector<vec2<T>> &verts,
                                const std::vector<size_t> &poly_in,
                                std::vector<FortuneTriangle> &triangles){
    if(poly_in.size() < 3){
        return false;
    }

    std::vector<size_t> poly = poly_in;
    if(signed_area_ld(verts, poly) < 0.0L){
        std::reverse(poly.begin(), poly.end());
    }

    if(is_convex_polygon(verts, poly)){
        for(size_t i = 1; (i + 1) < poly.size(); ++i){
            FortuneTriangle tri{};
            if(make_ccw_triangle(verts, poly.front(), poly.at(i), poly.at(i + 1), tri)){
                triangles.push_back(tri);
            }
        }
        return true;
    }

    std::vector<size_t> remaining = poly;
    while(remaining.size() > 3){
        bool ear_found = false;
        for(size_t i = 0; i < remaining.size(); ++i){
            const auto prev = remaining.at((i + remaining.size() - 1) % remaining.size());
            const auto curr = remaining.at(i);
            const auto next = remaining.at((i + 1) % remaining.size());
            if(orient_sign(verts.at(prev), verts.at(curr), verts.at(next)) <= 0){
                continue;
            }

            bool blocked = false;
            for(size_t j = 0; j < remaining.size(); ++j){
                const auto probe = remaining.at(j);
                if((probe == prev) || (probe == curr) || (probe == next)){
                    continue;
                }
                if(point_in_triangle_or_on_boundary(verts.at(probe),
                                                    verts.at(prev),
                                                    verts.at(curr),
                                                    verts.at(next))){
                    blocked = true;
                    break;
                }
            }
            if(blocked){
                continue;
            }

            FortuneTriangle tri{};
            if(make_ccw_triangle(verts, prev, curr, next, tri)){
                triangles.push_back(tri);
            }
            remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
            ear_found = true;
            break;
        }
        if(!ear_found){
            return false;
        }
    }

    FortuneTriangle last{};
    return make_ccw_triangle(verts, remaining.at(0), remaining.at(1), remaining.at(2), last)
        ? (triangles.push_back(last), true)
        : false;
}

template <class T>
std::vector<size_t> convex_hull_indices(const std::vector<vec2<T>> &verts){
    std::vector<size_t> order(verts.size());
    for(size_t i = 0; i < verts.size(); ++i){
        order.at(i) = i;
    }
    std::sort(order.begin(),
              order.end(),
              [&verts](size_t lhs, size_t rhs){
                  const auto &a = verts.at(lhs);
                  const auto &b = verts.at(rhs);
                  if(a.x != b.x){
                      return a.x < b.x;
                  }
                  if(a.y != b.y){
                      return a.y < b.y;
                  }
                  return lhs < rhs;
              });

    std::vector<size_t> hull;
    hull.reserve(order.size() * 2);
    for(const auto idx : order){
        while((hull.size() >= 2)
           && (orient_sign(verts.at(hull.at(hull.size() - 2)),
                           verts.at(hull.at(hull.size() - 1)),
                           verts.at(idx)) <= 0)){
            hull.pop_back();
        }
        hull.push_back(idx);
    }

    const auto lower_size = hull.size();
    for(auto it = order.rbegin(); it != order.rend(); ++it){
        const auto idx = *it;
        while((hull.size() > lower_size)
           && (orient_sign(verts.at(hull.at(hull.size() - 2)),
                           verts.at(hull.at(hull.size() - 1)),
                           verts.at(idx)) <= 0)){
            hull.pop_back();
        }
        hull.push_back(idx);
    }

    if(!hull.empty()){
        hull.pop_back();
    }
    return hull;
}

template <class T>
std::vector<size_t> radial_order_indices(const std::vector<vec2<T>> &verts){
    long double cx = 0.0L;
    long double cy = 0.0L;
    for(const auto &vert : verts){
        cx += static_cast<long double>(vert.x);
        cy += static_cast<long double>(vert.y);
    }
    cx /= static_cast<long double>(verts.size());
    cy /= static_cast<long double>(verts.size());

    std::vector<size_t> order(verts.size());
    for(size_t i = 0; i < verts.size(); ++i){
        order.at(i) = i;
    }
    std::sort(order.begin(),
              order.end(),
              [&verts, cx, cy](size_t lhs, size_t rhs){
                  const auto angle_l = std::atan2(static_cast<long double>(verts.at(lhs).y) - cy,
                                                  static_cast<long double>(verts.at(lhs).x) - cx);
                  const auto angle_r = std::atan2(static_cast<long double>(verts.at(rhs).y) - cy,
                                                  static_cast<long double>(verts.at(rhs).x) - cx);
                  if(angle_l != angle_r){
                      return angle_l < angle_r;
                  }
                  return lhs < rhs;
              });
    return order;
}

template <class T>
bool legalize_triangles(const std::vector<vec2<T>> &verts,
                        std::vector<FortuneTriangle> &triangles){
    const auto flip_guard = triangles.size() * triangles.size() + FORTUNE_FACE_GUARD_BIAS;
    for(size_t iter = 0; iter < flip_guard; ++iter){
        bool flipped = false;
        std::map<std::pair<size_t, size_t>, std::vector<size_t>> edge_to_triangles;
        for(size_t ti = 0; ti < triangles.size(); ++ti){
            const auto &tri = triangles.at(ti);
            edge_to_triangles[{ std::min(tri.a, tri.b), std::max(tri.a, tri.b) }].push_back(ti);
            edge_to_triangles[{ std::min(tri.b, tri.c), std::max(tri.b, tri.c) }].push_back(ti);
            edge_to_triangles[{ std::min(tri.c, tri.a), std::max(tri.c, tri.a) }].push_back(ti);
        }

        for(const auto &[edge, incident] : edge_to_triangles){
            if(incident.size() != 2){
                continue;
            }
            const auto &lhs = triangles.at(incident.at(0));
            const auto &rhs = triangles.at(incident.at(1));
            const auto opposite_lhs = (lhs.a != edge.first && lhs.a != edge.second) ? lhs.a
                                    : (lhs.b != edge.first && lhs.b != edge.second) ? lhs.b
                                                                                    : lhs.c;
            const auto opposite_rhs = (rhs.a != edge.first && rhs.a != edge.second) ? rhs.a
                                    : (rhs.b != edge.first && rhs.b != edge.second) ? rhs.b
                                                                                    : rhs.c;
            if(opposite_lhs == opposite_rhs){
                continue;
            }
            if(incircle_sign(verts.at(lhs.a), verts.at(lhs.b), verts.at(lhs.c), verts.at(opposite_rhs)) <= 0){
                continue;
            }

            FortuneTriangle tri_a{};
            FortuneTriangle tri_b{};
            if(!make_ccw_triangle(verts, opposite_lhs, opposite_rhs, edge.first, tri_a)
            || !make_ccw_triangle(verts, opposite_lhs, opposite_rhs, edge.second, tri_b)){
                return false;
            }
            triangles.at(incident.at(0)) = tri_a;
            triangles.at(incident.at(1)) = tri_b;
            flipped = true;
            break;
        }

        if(!flipped){
            prune_triangles(verts, triangles);
            return true;
        }
    }
    return false;
}

template <class T>
class FortuneSweepBuilder {
    public:
        FortuneSweepBuilder(const std::vector<vec2<T>> &scaled_verts,
                            const std::vector<size_t> &unique_to_original)
            : m_scaled_verts(scaled_verts),
              m_unique_to_original(unique_to_original),
              m_rng_state(0x9E3779B97F4A7C15ULL),
              // The sweep operates on normalized O(1) coordinates, so a small
              // multiple of machine epsilon is large enough to break exact event
              // ties while remaining far below any geometric feature scale.
              m_geom_eps(static_cast<long double>(std::numeric_limits<T>::epsilon()) * 64.0L){
        }

        std::vector<FortuneTriangle> build(){
            if(m_scaled_verts.size() < 3){
                return {};
            }
            if(m_scaled_verts.size() == 3){
                FortuneTriangle tri{};
                std::vector<FortuneTriangle> out;
                if(make_ccw_triangle(m_scaled_verts, 0, 1, 2, tri)){
                    out.push_back(tri);
                }
                return out;
            }

            initialize_site_order();
            process_events();
            auto triangles = recover_faces_as_triangles();
            validate_local_delaunay(triangles);
            return triangles;
        }

    private:
        struct CircleEvent;

        struct Arc {
            explicit Arc(size_t in_site, uint64_t in_priority)
                : site(in_site), priority(in_priority) {
            }

            size_t site;
            Arc *prev = nullptr;
            Arc *next = nullptr;
            CircleEvent *circle = nullptr;

            Arc *left = nullptr;
            Arc *right = nullptr;
            Arc *parent = nullptr;
            uint64_t priority = 0;
            size_t subtree_size = 1;
        };

        struct CircleEvent {
            CircleEvent(Arc *in_arc,
                        Arc *in_left,
                        Arc *in_right,
                        long double in_x,
                        long double in_y,
                        size_t in_serial)
                : arc(in_arc), left(in_left), right(in_right), x(in_x), y(in_y), serial(in_serial) {
            }

            Arc *arc = nullptr;
            Arc *left = nullptr;
            Arc *right = nullptr;
            long double x = 0.0L;
            long double y = 0.0L;
            bool valid = true;
            size_t serial = 0;
        };

        struct CircleEventCompare {
            bool operator()(const CircleEvent *lhs, const CircleEvent *rhs) const {
                if(lhs->y != rhs->y){
                    return lhs->y < rhs->y;
                }
                if(lhs->x != rhs->x){
                    return lhs->x > rhs->x;
                }
                return lhs->serial > rhs->serial;
            }
        };

        const std::vector<vec2<T>> &m_scaled_verts;
        const std::vector<size_t> &m_unique_to_original;
        std::vector<size_t> m_site_order;
        size_t m_next_site = 0;
        Arc *m_root = nullptr;
        uint64_t m_rng_state;
        size_t m_event_serial = 0;
        long double m_directrix = std::numeric_limits<long double>::infinity();
        const long double m_geom_eps;
        std::vector<std::unique_ptr<Arc>> m_arc_storage;
        std::vector<std::unique_ptr<CircleEvent>> m_event_storage;
        std::priority_queue<CircleEvent *, std::vector<CircleEvent *>, CircleEventCompare> m_circle_events;
        std::set<std::pair<size_t, size_t>> m_edges;

        static size_t node_size(const Arc *n){
            return (n != nullptr) ? n->subtree_size : 0;
        }

        static void update_node(Arc *n){
            if(n == nullptr){
                return;
            }
            if(n->left != nullptr){
                n->left->parent = n;
            }
            if(n->right != nullptr){
                n->right->parent = n;
            }
            n->subtree_size = 1 + node_size(n->left) + node_size(n->right);
        }

        static Arc *merge(Arc *a, Arc *b){
            if(a == nullptr){
                if(b != nullptr){
                    b->parent = nullptr;
                }
                return b;
            }
            if(b == nullptr){
                a->parent = nullptr;
                return a;
            }
            if(a->priority > b->priority){
                a->right = merge(a->right, b);
                update_node(a);
                a->parent = nullptr;
                return a;
            }
            b->left = merge(a, b->left);
            update_node(b);
            b->parent = nullptr;
            return b;
        }

        static void split(Arc *root, size_t left_count, Arc *&left, Arc *&right){
            if(root == nullptr){
                left = nullptr;
                right = nullptr;
                return;
            }
            const auto root_left_count = node_size(root->left);
            if(left_count <= root_left_count){
                split(root->left, left_count, left, root->left);
                update_node(root);
                right = root;
                if(left != nullptr){
                    left->parent = nullptr;
                }
                right->parent = nullptr;
                return;
            }
            split(root->right, left_count - root_left_count - 1, root->right, right);
            update_node(root);
            left = root;
            if(right != nullptr){
                right->parent = nullptr;
            }
            left->parent = nullptr;
        }

        size_t rank_of(const Arc *node) const {
            size_t rank = node_size(node->left);
            auto *curr = node;
            while(curr->parent != nullptr){
                if(curr == curr->parent->right){
                    rank += 1 + node_size(curr->parent->left);
                }
                curr = curr->parent;
            }
            return rank;
        }

        Arc *allocate_arc(size_t site){
            m_rng_state ^= (m_rng_state << 13);
            m_rng_state ^= (m_rng_state >> 7);
            m_rng_state ^= (m_rng_state << 17);
            m_arc_storage.emplace_back(std::make_unique<Arc>(site, m_rng_state));
            return m_arc_storage.back().get();
        }

        CircleEvent *allocate_circle_event(Arc *arc,
                                           Arc *left,
                                           Arc *right,
                                           long double x,
                                           long double y){
            m_event_storage.emplace_back(std::make_unique<CircleEvent>(arc, left, right, x, y, m_event_serial++));
            return m_event_storage.back().get();
        }

        void initialize_site_order(){
            m_site_order.resize(m_scaled_verts.size());
            for(size_t i = 0; i < m_scaled_verts.size(); ++i){
                m_site_order.at(i) = i;
            }
            std::sort(m_site_order.begin(),
                      m_site_order.end(),
                      [this](size_t lhs, size_t rhs){
                          const auto &a = m_scaled_verts.at(lhs);
                          const auto &b = m_scaled_verts.at(rhs);
                          if(a.y != b.y){
                              return a.y > b.y;
                          }
                          if(a.x != b.x){
                              return a.x < b.x;
                          }
                          return m_unique_to_original.at(lhs) < m_unique_to_original.at(rhs);
                      });
        }

        void process_events(){
            while((m_next_site < m_site_order.size()) || !m_circle_events.empty()){
                while(!m_circle_events.empty() && !m_circle_events.top()->valid){
                    m_circle_events.pop();
                }

                const auto has_site = (m_next_site < m_site_order.size());
                const auto has_circle = !m_circle_events.empty();
                if(!has_site && !has_circle){
                    break;
                }

                bool process_site = has_site;
                if(has_site && has_circle){
                    const auto &site = m_scaled_verts.at(m_site_order.at(m_next_site));
                    const auto *circle = m_circle_events.top();
                    if(circle->y > static_cast<long double>(site.y) + m_geom_eps){
                        process_site = false;
                    }else if(std::abs(circle->y - static_cast<long double>(site.y)) <= m_geom_eps){
                        process_site = (static_cast<long double>(site.x) <= circle->x + m_geom_eps);
                    }
                }else if(!has_site){
                    process_site = false;
                }

                if(process_site){
                    const auto site_idx = m_site_order.at(m_next_site++);
                    m_directrix = static_cast<long double>(m_scaled_verts.at(site_idx).y);
                    handle_site_event(site_idx);
                }else{
                    auto *circle = m_circle_events.top();
                    m_circle_events.pop();
                    if(!circle->valid){
                        continue;
                    }
                    m_directrix = circle->y;
                    handle_circle_event(circle);
                }
            }
        }

        long double breakpoint_x(size_t left_site, size_t right_site, long double directrix) const {
            const auto &p = m_scaled_verts.at(left_site);
            const auto &q = m_scaled_verts.at(right_site);
            const auto py = static_cast<long double>(p.y);
            const auto qy = static_cast<long double>(q.y);
            const auto px = static_cast<long double>(p.x);
            const auto qx = static_cast<long double>(q.x);

            const auto dp = py - directrix;
            const auto dq = qy - directrix;
            if(std::abs(dp) <= m_geom_eps && std::abs(dq) <= m_geom_eps){
                return (px + qx) * 0.5L;
            }
            if(std::abs(dp) <= m_geom_eps){
                return px;
            }
            if(std::abs(dq) <= m_geom_eps){
                return qx;
            }
            if(py == qy){
                return (px + qx) * 0.5L;
            }

            const auto z0 = 2.0L * dp;
            const auto z1 = 2.0L * dq;
            const auto a = (1.0L / z0) - (1.0L / z1);
            const auto b = -2.0L * ((px / z0) - (qx / z1));
            const auto c = ((px * px) + (py * py) - (directrix * directrix)) / z0
                         - ((qx * qx) + (qy * qy) - (directrix * directrix)) / z1;
            auto disc = (b * b) - (4.0L * a * c);
            if(disc < 0.0L && disc > -m_geom_eps){
                disc = 0.0L;
            }
            if(disc < 0.0L){
                std::ostringstream oss;
                oss << "Voronoi sweep encountered a negative breakpoint discriminant (" << disc << ")";
                throw std::runtime_error(oss.str());
            }
            const auto root_disc = std::sqrt(disc);
            const auto x1 = (-b - root_disc) / (2.0L * a);
            const auto x2 = (-b + root_disc) / (2.0L * a);
            return (py < qy) ? std::max(x1, x2) : std::min(x1, x2);
        }

        Arc *find_arc_above(T x) const {
            auto *curr = m_root;
            const auto probe_x = static_cast<long double>(x);
            while(curr != nullptr){
                const auto left_bound = (curr->prev != nullptr)
                                      ? breakpoint_x(curr->prev->site, curr->site, m_directrix)
                                      : -std::numeric_limits<long double>::infinity();
                const auto right_bound = (curr->next != nullptr)
                                       ? breakpoint_x(curr->site, curr->next->site, m_directrix)
                                       : std::numeric_limits<long double>::infinity();
                if(probe_x < left_bound){
                    curr = curr->left;
                }else if(probe_x > right_bound){
                    curr = curr->right;
                }else{
                    return curr;
                }
            }
            return nullptr;
        }

        void invalidate_circle_event(Arc *arc){
            if((arc != nullptr) && (arc->circle != nullptr)){
                arc->circle->valid = false;
                arc->circle = nullptr;
            }
        }

        void insert_after(Arc *existing, Arc *node){
            Arc *left = nullptr;
            Arc *right = nullptr;
            split(m_root, rank_of(existing) + 1, left, right);
            m_root = merge(merge(left, node), right);
        }

        void erase_arc(Arc *node){
            Arc *left = nullptr;
            Arc *mid = nullptr;
            Arc *right = nullptr;
            split(m_root, rank_of(node), left, right);
            split(right, 1, mid, right);
            (void)mid;
            m_root = merge(left, right);
            node->left = nullptr;
            node->right = nullptr;
            node->parent = nullptr;
            node->subtree_size = 1;
        }

        void add_edge(size_t a, size_t b){
            if(a == b){
                return;
            }
            if(b < a){
                std::swap(a, b);
            }
            m_edges.insert({a, b});
        }

        bool compute_circle_event_geometry(size_t a,
                                           size_t b,
                                           size_t c,
                                           long double &center_x,
                                           long double &event_y) const {
            const auto &pa = m_scaled_verts.at(a);
            const auto &pb = m_scaled_verts.at(b);
            const auto &pc = m_scaled_verts.at(c);
            if(orient_sign(pa, pb, pc) >= 0){
                return false;
            }

            const auto ax = static_cast<long double>(pa.x);
            const auto ay = static_cast<long double>(pa.y);
            const auto bx = static_cast<long double>(pb.x);
            const auto by = static_cast<long double>(pb.y);
            const auto cx = static_cast<long double>(pc.x);
            const auto cy = static_cast<long double>(pc.y);
            const auto d = 2.0L * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
            if(std::abs(d) <= m_geom_eps){
                return false;
            }

            center_x = (((ax * ax) + (ay * ay)) * (by - cy)
                      + ((bx * bx) + (by * by)) * (cy - ay)
                      + ((cx * cx) + (cy * cy)) * (ay - by)) / d;
            const auto center_y = (((ax * ax) + (ay * ay)) * (cx - bx)
                                 + ((bx * bx) + (by * by)) * (ax - cx)
                                 + ((cx * cx) + (cy * cy)) * (bx - ax)) / d;
            const auto radius = std::hypotl(center_x - ax, center_y - ay);
            event_y = center_y - radius;
            if(event_y >= m_directrix - m_geom_eps){
                return false;
            }
            return true;
        }

        void schedule_circle_event(Arc *arc){
            if((arc == nullptr) || (arc->prev == nullptr) || (arc->next == nullptr)){
                return;
            }
            if((arc->prev->site == arc->site) || (arc->site == arc->next->site) || (arc->prev->site == arc->next->site)){
                return;
            }

            long double event_x = 0.0L;
            long double event_y = 0.0L;
            if(!compute_circle_event_geometry(arc->prev->site, arc->site, arc->next->site, event_x, event_y)){
                return;
            }
            auto *event = allocate_circle_event(arc, arc->prev, arc->next, event_x, event_y);
            arc->circle = event;
            m_circle_events.push(event);
        }

        void handle_site_event(size_t site_idx){
            if(m_root == nullptr){
                m_root = allocate_arc(site_idx);
                return;
            }

            auto *arc = find_arc_above(m_scaled_verts.at(site_idx).x);
            if(arc == nullptr){
                throw std::runtime_error("Voronoi sweep could not locate a beach-line arc for a site event.");
            }

            invalidate_circle_event(arc);
            auto *right_copy = allocate_arc(arc->site);
            auto *new_arc = allocate_arc(site_idx);
            auto *old_next = arc->next;

            arc->next = new_arc;
            new_arc->prev = arc;
            new_arc->next = right_copy;
            right_copy->prev = new_arc;
            right_copy->next = old_next;
            if(old_next != nullptr){
                old_next->prev = right_copy;
            }

            insert_after(arc, new_arc);
            insert_after(new_arc, right_copy);

            add_edge(arc->site, site_idx);

            schedule_circle_event(arc);
            schedule_circle_event(right_copy);
        }

        void handle_circle_event(CircleEvent *event){
            auto *arc = event->arc;
            if((arc == nullptr) || (arc->circle != event) || !event->valid){
                return;
            }
            auto *left = arc->prev;
            auto *right = arc->next;
            if((left == nullptr) || (right == nullptr) || (left != event->left) || (right != event->right)){
                return;
            }

            add_edge(left->site, arc->site);
            add_edge(arc->site, right->site);
            add_edge(left->site, right->site);

            invalidate_circle_event(left);
            invalidate_circle_event(arc);
            invalidate_circle_event(right);

            left->next = right;
            right->prev = left;
            erase_arc(arc);

            schedule_circle_event(left);
            schedule_circle_event(right);
        }

        std::vector<FortuneTriangle> recover_faces_as_triangles() const {
            std::vector<std::vector<size_t>> adjacency(m_scaled_verts.size());
            for(const auto &[a, b] : m_edges){
                adjacency.at(a).push_back(b);
                adjacency.at(b).push_back(a);
            }
            for(size_t i = 0; i < adjacency.size(); ++i){
                auto &nbrs = adjacency.at(i);
                std::sort(nbrs.begin(),
                          nbrs.end(),
                          [this, i](size_t lhs, size_t rhs){
                              const auto &origin = m_scaled_verts.at(i);
                              const auto &a = m_scaled_verts.at(lhs);
                              const auto &b = m_scaled_verts.at(rhs);
                              const auto angle_a = std::atan2(static_cast<long double>(a.y - origin.y),
                                                              static_cast<long double>(a.x - origin.x));
                              const auto angle_b = std::atan2(static_cast<long double>(b.y - origin.y),
                                                              static_cast<long double>(b.x - origin.x));
                              if(angle_a != angle_b){
                                  return angle_a < angle_b;
                              }
                              return lhs < rhs;
                          });
                nbrs.erase(std::unique(nbrs.begin(), nbrs.end()), nbrs.end());
            }

            std::set<std::pair<size_t, size_t>> visited_halfedges;
            std::vector<FortuneTriangle> triangles;
            const auto face_guard = (m_edges.size() * 2) + FORTUNE_FACE_GUARD_BIAS;

            for(const auto &[start_u, start_v] : m_edges){
                for(const auto seed : { std::make_pair(start_u, start_v), std::make_pair(start_v, start_u) }){
                    if(visited_halfedges.count(seed) != 0U){
                        continue;
                    }

                    std::vector<size_t> face;
                    auto u = seed.first;
                    auto v = seed.second;
                    const auto seed_u = u;
                    const auto seed_v = v;

                    for(size_t steps = 0; steps < face_guard; ++steps){
                        if(!visited_halfedges.insert({u, v}).second){
                            break;
                        }
                        face.push_back(u);
                        const auto &nbrs = adjacency.at(v);
                        const auto it = std::find(nbrs.begin(), nbrs.end(), u);
                        if(it == nbrs.end()){
                            throw std::runtime_error("Voronoi sweep produced a non-manifold adjacency relation.");
                        }
                        const auto pos = static_cast<size_t>(std::distance(nbrs.begin(), it));
                        const auto next_pos = (pos + nbrs.size() - 1) % nbrs.size();
                        const auto w = nbrs.at(next_pos);
                        u = v;
                        v = w;
                        if((u == seed_u) && (v == seed_v)){
                            break;
                        }
                        if(steps + 1 == face_guard){
                            throw std::runtime_error("Voronoi sweep face recovery exceeded its iteration guard.");
                        }
                    }

                    if(face.size() < 3){
                        continue;
                    }
                    if(signed_area_ld(m_scaled_verts, face) <= 0.0L){
                        continue;
                    }
                    if(!triangulate_simple_polygon(m_scaled_verts, face, triangles)){
                        throw std::runtime_error("Voronoi sweep could not triangulate a recovered planar face.");
                    }
                }
            }

            prune_triangles(m_scaled_verts, triangles);
            const auto hull = convex_hull_indices(m_scaled_verts);
            if(hull.size() == m_scaled_verts.size()){
                const auto expected_hull_tris = (hull.size() >= 2) ? (hull.size() - 2) : 0;
                if(triangles.size() != expected_hull_tris){
                    const auto hull_cycle = radial_order_indices(m_scaled_verts);
                    std::vector<FortuneTriangle> hull_triangles;
                    if(triangulate_simple_polygon(m_scaled_verts, hull_cycle, hull_triangles)){
                        if(!legalize_triangles(m_scaled_verts, hull_triangles)){
                            throw std::runtime_error("Voronoi sweep could not legalize a hull-only triangulation.");
                        }
                        prune_triangles(m_scaled_verts, hull_triangles);
                        if(!hull_triangles.empty()){
                            return hull_triangles;
                        }
                    }
                }
            }
            {
                // Euler's formula for planar triangulations: F = 2V - 2 - h,
                // where V is the vertex count and h is the convex hull size.
                const auto n_unique = m_scaled_verts.size();
                const auto h_hull = hull.size();
                const auto expected_tris = (n_unique >= 3) ? (2 * n_unique - 2 - h_hull) : static_cast<size_t>(0);
                if(triangles.size() != expected_tris){
                    std::ostringstream oss;
                    oss << "Voronoi sweep face recovery produced " << triangles.size()
                        << " triangle(s) but expected " << expected_tris
                        << " (n=" << n_unique << ", h=" << h_hull << ")";
                    throw std::runtime_error(oss.str());
                }
            }
            return triangles;
        }

        void validate_local_delaunay(const std::vector<FortuneTriangle> &triangles) const {
            std::map<std::pair<size_t, size_t>, std::vector<size_t>> edge_to_opposites;
            for(const auto &tri : triangles){
                edge_to_opposites[{ std::min(tri.a, tri.b), std::max(tri.a, tri.b) }].push_back(tri.c);
                edge_to_opposites[{ std::min(tri.b, tri.c), std::max(tri.b, tri.c) }].push_back(tri.a);
                edge_to_opposites[{ std::min(tri.c, tri.a), std::max(tri.c, tri.a) }].push_back(tri.b);
            }
            for(const auto &tri : triangles){
                for(const auto &entry : std::array<std::pair<std::pair<size_t, size_t>, size_t>, 3>{{
                        {{ std::min(tri.a, tri.b), std::max(tri.a, tri.b) }, tri.c},
                        {{ std::min(tri.b, tri.c), std::max(tri.b, tri.c) }, tri.a},
                        {{ std::min(tri.c, tri.a), std::max(tri.c, tri.a) }, tri.b} }}){
                    const auto &adjacent = edge_to_opposites.at(entry.first);
                    if(adjacent.size() == 1){
                        continue;
                    }
                    if(adjacent.size() != 2){
                        std::ostringstream oss;
                        oss << "Voronoi sweep produced a non-manifold edge between unique vertices "
                            << entry.first.first << " and " << entry.first.second
                            << " with " << adjacent.size() << " incident triangles";
                        throw std::runtime_error(oss.str());
                    }
                    const auto other = (adjacent.at(0) == entry.second) ? adjacent.at(1) : adjacent.at(0);
                    if(incircle_sign(m_scaled_verts.at(tri.a),
                                     m_scaled_verts.at(tri.b),
                                     m_scaled_verts.at(tri.c),
                                     m_scaled_verts.at(other)) > 0){
                        std::ostringstream oss;
                        oss << "Voronoi sweep produced a non-Delaunay edge between unique vertices "
                            << entry.first.first << " and " << entry.first.second;
                        throw std::runtime_error(oss.str());
                    }
                }
            }
        }
};

template <class T>
std::vector<FortuneTriangle> build_voronoi_delaunay_triangles(const std::vector<vec2<T>> &verts,
                                                              std::vector<size_t> &unique_to_original){
    std::map<std::pair<T, T>, size_t> unique_lookup;
    std::vector<vec2<T>> unique_verts;
    unique_verts.reserve(verts.size());
    unique_to_original.clear();
    unique_to_original.reserve(verts.size());

    T min_x = std::numeric_limits<T>::max();
    T max_x = std::numeric_limits<T>::lowest();
    T min_y = std::numeric_limits<T>::max();
    T max_y = std::numeric_limits<T>::lowest();

    for(size_t i = 0; i < verts.size(); ++i){
        const auto &vert = verts.at(i);
        const auto key = std::make_pair(vert.x, vert.y);
        if(unique_lookup.emplace(key, unique_verts.size()).second){
            unique_verts.push_back(vert);
            unique_to_original.push_back(i);
            min_x = std::min(min_x, vert.x);
            max_x = std::max(max_x, vert.x);
            min_y = std::min(min_y, vert.y);
            max_y = std::max(max_y, vert.y);
        }
    }

    if(unique_verts.size() < 3){
        return {};
    }
    if(unique_verts.size() == 3){
        FortuneTriangle tri{};
        std::vector<FortuneTriangle> out;
        if(make_ccw_triangle(unique_verts, 0, 1, 2, tri)){
            out.push_back(tri);
        }
        return out;
    }

    const auto center_x = (min_x + max_x) / static_cast<T>(2);
    const auto center_y = (min_y + max_y) / static_cast<T>(2);
    const auto scale = std::max(max_x - min_x, max_y - min_y);
    const auto inv_scale = (scale > static_cast<T>(0)) ? (static_cast<T>(1) / scale) : static_cast<T>(1);

    std::vector<vec2<T>> scaled_verts;
    scaled_verts.reserve(unique_verts.size());
    for(const auto &vert : unique_verts){
        scaled_verts.emplace_back((vert.x - center_x) * inv_scale,
                                  (vert.y - center_y) * inv_scale);
    }

    FortuneSweepBuilder<T> builder(scaled_verts, unique_to_original);
    return builder.build();
}

} // namespace


template <class T, class I>
fv_surface_mesh<T, I>
Delaunay_Triangulation_2_via_Voronoi(const std::vector<vec2<T>> &verts) {
    const auto N_verts = verts.size();

    if(N_verts < 3){
        YLOGWARN("Refusing Voronoi-based Delaunay triangulation of " << N_verts
                 << " vertex/vertices; at least 3 are required");
        throw std::invalid_argument("Voronoi-based Delaunay triangulation requires at least 3 vertices.");
    }

    T min_x = std::numeric_limits<T>::max();
    T max_x = std::numeric_limits<T>::lowest();
    T min_y = std::numeric_limits<T>::max();
    T max_y = std::numeric_limits<T>::lowest();

    size_t N_finite_verts = 0;
    size_t first_nonfinite_index = N_verts;
    for(size_t i = 0; i < verts.size(); ++i){
        const auto &v = verts.at(i);
        if(!is_finite_2d(v)){
            if(first_nonfinite_index == N_verts){
                first_nonfinite_index = i;
            }
            continue;
        }
        ++N_finite_verts;
        min_x = std::min(min_x, v.x);
        max_x = std::max(max_x, v.x);
        min_y = std::min(min_y, v.y);
        max_y = std::max(max_y, v.y);
    }
    if(first_nonfinite_index != N_verts){
        YLOGWARN("Refusing Voronoi-based Delaunay triangulation because vertex " << first_nonfinite_index
                 << " is not finite: (" << verts.at(first_nonfinite_index).x
                 << ", " << verts.at(first_nonfinite_index).y << ")");
        throw std::invalid_argument("Voronoi-based Delaunay triangulation requires all vertex coordinates to be finite.");
    }
    if(N_finite_verts < 3){
        YLOGWARN("Refusing Voronoi-based Delaunay triangulation because only " << N_finite_verts
                 << " finite vertices were provided");
        throw std::invalid_argument("Voronoi-based Delaunay triangulation requires at least 3 finite vertices.");
    }

    size_t duplicate_vertices = 0;
    {
        std::map<std::pair<T, T>, size_t> vertex_counts;
        for(const auto &vert : verts){
            ++vertex_counts[std::make_pair(vert.x, vert.y)];
        }
        for(const auto &entry : vertex_counts){
            const size_t count = entry.second;
            if(count > 1){
                duplicate_vertices += (count * (count - 1)) / 2;
            }
        }
    }
    if(duplicate_vertices != 0){
        YLOGWARN("Voronoi-based Delaunay triangulation received " << duplicate_vertices
                 << " pair(s) of duplicate vertices; degenerate duplicates will be ignored");
    }

    if(!has_non_collinear_triplet(verts)){
        YLOGWARN("Refusing Voronoi-based Delaunay triangulation because all finite vertices are collinear or coincident");
        throw std::invalid_argument("Voronoi-based Delaunay triangulation requires at least one non-collinear triplet of vertices.");
    }

    YLOGDEBUG("Voronoi-based Delaunay triangulation input: vertices=" << N_verts
              << ", bbox=[(" << min_x << ", " << min_y << "), (" << max_x << ", " << max_y << ")]");

    std::vector<size_t> unique_to_original;
    std::vector<FortuneTriangle> triangles;
    try{
        triangles = build_voronoi_delaunay_triangles(verts, unique_to_original);
    }catch(const std::runtime_error &e){
        YLOGWARN("Voronoi sweep encountered an unresolved degenerate configuration; "
                 "falling back to lifting-based Delaunay triangulation. Reason: " << e.what());
        return Delaunay_Triangulation_2<T, I>(verts);
    }
    if(triangles.empty()){
        YLOGDEBUG("Voronoi-based Delaunay triangulation did not produce any finite triangles");
        throw std::runtime_error("Voronoi-based Delaunay triangulation failed to produce any triangles for the provided vertices.");
    }

    fv_surface_mesh<T, I> mesh;
    mesh.vertices.reserve(verts.size());
    for(const auto &vert : verts){
        mesh.vertices.emplace_back(vert.x, vert.y, static_cast<T>(0));
    }
    for(const auto &tri : triangles){
        mesh.faces.push_back({ static_cast<I>(unique_to_original.at(tri.a)),
                               static_cast<I>(unique_to_original.at(tri.b)),
                               static_cast<I>(unique_to_original.at(tri.c)) });
    }
    if(mesh.faces.empty()){
        YLOGDEBUG("All Voronoi-based Delaunay faces were pruned as degenerate");
        throw std::runtime_error("Voronoi-based Delaunay triangulation failed because every candidate triangle was degenerate.");
    }

    YLOGDEBUG("Voronoi-based Delaunay triangulation produced " << mesh.faces.size() << " triangle(s)");
    return mesh;
}
#ifndef YGOR_MATH_DELAUNAY_VIA_VORONOI_DISABLE_ALL_SPECIALIZATIONS
    template fv_surface_mesh<float , uint32_t> Delaunay_Triangulation_2_via_Voronoi(const std::vector<vec2<float >> &);
    template fv_surface_mesh<float , uint64_t> Delaunay_Triangulation_2_via_Voronoi(const std::vector<vec2<float >> &);

    template fv_surface_mesh<double, uint32_t> Delaunay_Triangulation_2_via_Voronoi(const std::vector<vec2<double>> &);
    template fv_surface_mesh<double, uint64_t> Delaunay_Triangulation_2_via_Voronoi(const std::vector<vec2<double>> &);
#endif
