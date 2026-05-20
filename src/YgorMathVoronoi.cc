//YgorMathVoronoi.cc

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMathVoronoi.h"

namespace {

template <class T>
bool is_finite_2d(const vec2<T> &v){
    return std::isfinite(v.x) && std::isfinite(v.y);
}

template <class T>
struct Normalization2 {
    vec2<T> center;
    T scale = static_cast<T>(1);
};

template <class T>
Normalization2<T> compute_normalization(const std::vector<vec2<T>> &verts){
    T min_x = std::numeric_limits<T>::max();
    T max_x = std::numeric_limits<T>::lowest();
    T min_y = std::numeric_limits<T>::max();
    T max_y = std::numeric_limits<T>::lowest();

    for(const auto &vert : verts){
        min_x = std::min(min_x, vert.x);
        max_x = std::max(max_x, vert.x);
        min_y = std::min(min_y, vert.y);
        max_y = std::max(max_y, vert.y);
    }

    const T half = static_cast<T>(0.5);
    const T span_x = (max_x * half - min_x * half) * static_cast<T>(2);
    const T span_y = (max_y * half - min_y * half) * static_cast<T>(2);

    Normalization2<T> norm;
    norm.center = vec2<T>(min_x * half + max_x * half,
                          min_y * half + max_y * half);
    norm.scale = std::max(span_x, span_y);

    if(!is_finite_2d(norm.center) || !std::isfinite(norm.scale)){
        throw std::invalid_argument("Voronoi normalization overflowed for finite input");
    }

    if(!(norm.scale > static_cast<T>(0))){
        norm.scale = static_cast<T>(1);
    }
    return norm;
}

template <class T>
vec2<T> normalize_point(const vec2<T> &v, const Normalization2<T> &norm){
    return (v - norm.center) / norm.scale;
}

template <class T>
vec2<T> denormalize_point(const vec2<T> &v, const Normalization2<T> &norm){
    return v * norm.scale + norm.center;
}

template <class T>
vec2<T> denormalize_direction(const vec2<T> &v, const Normalization2<T> &norm){
    return v * norm.scale;
}

template <class T>
vec2<T> perp_ccw(const vec2<T> &v){
    return vec2<T>(-v.y, v.x);
}

template <class T>
T sq_dist(const vec2<T> &a, const vec2<T> &b){
    const auto dx = a.x - b.x;
    const auto dy = a.y - b.y;
    return dx * dx + dy * dy;
}


struct PrecisePoint2 {
    long double x = 0.0L;
    long double y = 0.0L;
};

struct VertexCellKey {
    int64_t x = 0;
    int64_t y = 0;

    bool operator==(const VertexCellKey &rhs) const {
        return (x == rhs.x) && (y == rhs.y);
    }
};

struct VertexCellKeyHash {
    size_t operator()(const VertexCellKey &key) const noexcept {
        const auto hx = std::hash<int64_t>{}(key.x);
        const auto hy = std::hash<int64_t>{}(key.y);
        // 64-bit golden-ratio constant, commonly used to mix partially correlated hash inputs.
        return hx ^ (hy + 0x9e3779b97f4a7c15ULL + (hx << 6U) + (hx >> 2U));
    }
};

template <class T>
vec2<T> representative_site_point(const std::vector<vec2<T>> &poly){
    if(poly.empty()){
        throw std::invalid_argument("Triangulate_Voronoi requires every site polygon to contain at least one vertex.");
    }
    for(const auto &v : poly){
        if(!is_finite_2d(v)){
            throw std::invalid_argument("Triangulate_Voronoi requires every site polygon vertex to be finite.");
        }
    }

    // Single-point and segment footprints already provide a natural dual-vertex placement without needing a polygon-area
    // centroid estimate.
    if(poly.size() == 1){
        return poly.front();
    }
    if(poly.size() == 2){
        return (poly.front() + poly.back()) / static_cast<T>(2);
    }

    const auto &origin = poly.front();
    long double twice_area = 0.0L;
    long double cx_times_area = 0.0L;
    long double cy_times_area = 0.0L;
    for(size_t i = 1; (i + 1) < poly.size(); ++i){
        const auto &a = poly.at(i);
        const auto &b = poly.at(i + 1);
        if(orient_sign(origin, a, b) == 0){
            continue;
        }
        const auto cross = (static_cast<long double>(a.x) - static_cast<long double>(origin.x))
                         * (static_cast<long double>(b.y) - static_cast<long double>(origin.y))
                         - (static_cast<long double>(a.y) - static_cast<long double>(origin.y))
                         * (static_cast<long double>(b.x) - static_cast<long double>(origin.x));
        twice_area += cross;
        cx_times_area += (static_cast<long double>(origin.x)
                       +  static_cast<long double>(a.x)
                       +  static_cast<long double>(b.x)) * cross;
        cy_times_area += (static_cast<long double>(origin.y)
                       +  static_cast<long double>(a.y)
                       +  static_cast<long double>(b.y)) * cross;
    }
    if(std::abs(twice_area) > std::numeric_limits<long double>::epsilon()){
        const auto inv = 1.0L / (static_cast<long double>(3) * twice_area);
        return vec2<T>(static_cast<T>(cx_times_area * inv),
                       static_cast<T>(cy_times_area * inv));
    }

    vec2<T> mean(static_cast<T>(0), static_cast<T>(0));
    for(const auto &v : poly){
        mean += v;
    }
    return mean / static_cast<T>(poly.size());
}

template <class T, class I>
class FortuneVoronoiBuilder {
    public:
        explicit FortuneVoronoiBuilder(const std::vector<vec2<T>> &verts)
            : m_input_sites(verts),
              m_norm(compute_normalization(verts)){
            m_sites.reserve(verts.size());
            for(const auto &vert : verts){
                m_sites.push_back(normalize_point(vert, m_norm));
            }
            m_output.cell_edges.resize(verts.size());
        }

        VoronoiDiagram2<T, I> build(){
            validate_input();
            if(all_sites_collinear()){
                build_collinear_diagram();
                finalize_output();
                return m_output;
            }
            enqueue_site_events();

            while(!m_events.empty()){
                auto *event = m_events.top();
                m_events.pop();
                if((event == nullptr) || !event->valid){
                    continue;
                }

                if(event->type == EventType::Site){
                    handle_site_event(event->site_index);
                }else{
                    handle_circle_event(event);
                }
            }

            finalize_active_edges();
            finalize_output();
            return m_output;
        }

    private:
        enum class EventType {
            Site,
            Circle,
        };

        struct Event;

        struct Node {
            Node *parent = nullptr;
            Node *left = nullptr;
            Node *right = nullptr;

            size_t site_index = 0;
            Node *prev = nullptr;
            Node *next = nullptr;
            // Active breakpoint edge between this arc and its immediate left neighbor.
            std::optional<size_t> left_edge_index;
            // Detached edge that resumes once this temporary arc collapses after a breakpoint site insertion.
            std::optional<size_t> pending_edge_index;
            Event *circle_event = nullptr;
            uint64_t priority = 0;
            size_t subtree_size = 1;
        };

        struct Event {
            EventType type = EventType::Site;
            long double y = 0.0L;
            long double x = 0.0L;
            size_t site_index = 0;
            Node *arc = nullptr;
            PrecisePoint2 center;
            bool valid = true;
            uint64_t sequence = 0;
        };

        struct EventCompare {
            bool operator()(const Event *lhs, const Event *rhs) const {
                if(lhs->y != rhs->y){
                    return lhs->y < rhs->y;
                }
                if(lhs->type != rhs->type){
                    return lhs->type == EventType::Circle;
                }
                if(lhs->x != rhs->x){
                    return lhs->x > rhs->x;
                }
                return lhs->sequence > rhs->sequence;
            }
        };

        struct InternalVertex {
            vec2<T> position;
            std::vector<size_t> incident_sites;
        };

        struct InternalEdge {
            size_t left_site = 0;
            size_t right_site = 0;
            std::optional<size_t> vertex0;
            std::optional<size_t> vertex1;
            vec2<T> sample_point;
            vec2<T> direction;
        };

        const std::vector<vec2<T>> &m_input_sites;
        std::vector<vec2<T>> m_sites;
        Normalization2<T> m_norm;
        VoronoiDiagram2<T, I> m_output;

        Node *m_root = nullptr;
        uint64_t m_next_sequence = 0;
        uint64_t m_next_priority = 0;

        std::vector<std::unique_ptr<Node>> m_node_storage;
        std::vector<std::unique_ptr<Event>> m_event_storage;
        std::priority_queue<Event*, std::vector<Event*>, EventCompare> m_events;
        std::vector<InternalVertex> m_vertices;
        std::vector<InternalEdge> m_edges;
        std::unordered_map<VertexCellKey, std::vector<size_t>, VertexCellKeyHash> m_vertex_grid;

        // Fortune's event queue depends on strict y-ordering; this inflated machine-epsilon guard suppresses
        // circle events whose computed bottoms are numerically indistinguishable from the current directrix.
        static constexpr long double event_guard_factor = 1024.0L;
        static constexpr long double vertex_merge_tolerance_factor = 64.0L;
        static constexpr long double ray_sample_guard_factor = 64.0L;

        void validate_input(){
            if(m_input_sites.size() < 2){
                YLOGWARN("Refusing Voronoi diagram of " << m_input_sites.size() << " site(s); at least 2 are required");
                throw std::invalid_argument("Voronoi diagram requires at least 2 sites.");
            }

            size_t first_nonfinite = m_input_sites.size();
            for(size_t i = 0; i < m_input_sites.size(); ++i){
                if(!is_finite_2d(m_input_sites.at(i))){
                    first_nonfinite = i;
                    break;
                }
            }
            if(first_nonfinite != m_input_sites.size()){
                YLOGWARN("Refusing Voronoi diagram because site " << first_nonfinite
                         << " is not finite: (" << m_input_sites.at(first_nonfinite).x
                         << ", " << m_input_sites.at(first_nonfinite).y << ")");
                throw std::invalid_argument("Voronoi diagram requires all site coordinates to be finite.");
            }

            std::map<std::pair<T, T>, size_t> unique_sites;
            for(size_t i = 0; i < m_input_sites.size(); ++i){
                const auto &site = m_input_sites.at(i);
                const auto key = std::make_pair(site.x, site.y);
                const auto [it, inserted] = unique_sites.emplace(key, i);
                if(!inserted){
                    YLOGWARN("Refusing Voronoi diagram because site " << i
                             << " duplicates site " << it->second << " at ("
                             << site.x << ", " << site.y << ")");
                    throw std::invalid_argument("Voronoi diagram requires all input sites to be distinct.");
                }
            }

            T min_x = std::numeric_limits<T>::max();
            T max_x = std::numeric_limits<T>::lowest();
            T min_y = std::numeric_limits<T>::max();
            T max_y = std::numeric_limits<T>::lowest();
            for(const auto &site : m_input_sites){
                min_x = std::min(min_x, site.x);
                max_x = std::max(max_x, site.x);
                min_y = std::min(min_y, site.y);
                max_y = std::max(max_y, site.y);
            }
            YLOGDEBUG("Voronoi diagram input: sites=" << m_input_sites.size()
                      << ", bbox=[(" << min_x << ", " << min_y << "), ("
                      << max_x << ", " << max_y << ")]");
        }

        bool all_sites_collinear() const {
            if(m_sites.size() < 3){
                return true;
            }
            size_t a = 0;
            size_t b = 1;
            for(size_t i = 2; i < m_sites.size(); ++i){
                if(orient_sign(m_sites.at(a), m_sites.at(b), m_sites.at(i)) != 0){
                    return false;
                }
            }
            return true;
        }

        // Fortune's tree-of-parabolas formulation degenerates when every site is collinear because no genuine
        // circle events exist; in that case the Voronoi diagram is simply the set of perpendicular bisectors between
        // consecutive sites ordered along the supporting line.
        void build_collinear_diagram(){
            std::vector<size_t> order(m_sites.size());
            for(size_t i = 0; i < order.size(); ++i){
                order[i] = i;
            }

            const auto axis = m_sites.at(1) - m_sites.at(0);
            std::sort(order.begin(), order.end(), [&](size_t lhs, size_t rhs){
                const auto pl = m_sites.at(lhs).Dot(axis);
                const auto pr = m_sites.at(rhs).Dot(axis);
                if(pl != pr){
                    return pl < pr;
                }
                return lhs < rhs;
            });

            for(size_t i = 0; (i + 1) < order.size(); ++i){
                (void)create_edge(order.at(i), order.at(i + 1));
            }
        }

        void enqueue_site_events(){
            std::vector<size_t> order(m_sites.size());
            for(size_t i = 0; i < order.size(); ++i){
                order[i] = i;
            }
            std::sort(order.begin(), order.end(), [&](size_t lhs, size_t rhs){
                const auto &a = m_sites.at(lhs);
                const auto &b = m_sites.at(rhs);
                if(a.y != b.y){
                    return a.y > b.y;
                }
                if(a.x != b.x){
                    return a.x < b.x;
                }
                return lhs < rhs;
            });

            for(const auto idx : order){
                auto *event = make_event();
                event->type = EventType::Site;
                event->site_index = idx;
                event->x = static_cast<long double>(m_sites.at(idx).x);
                event->y = static_cast<long double>(m_sites.at(idx).y);
                m_events.push(event);
            }
        }

        // Standard splitmix64 scrambling, used here to turn sequential node ids into treap priorities.
        static uint64_t splitmix64(uint64_t value){
            value += 0x9e3779b97f4a7c15ULL;
            value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
            value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
            return value ^ (value >> 31U);
        }

        Node* make_arc(size_t site_index){
            auto node = std::make_unique<Node>();
            node->site_index = site_index;
            node->priority = splitmix64(m_next_priority++);
            auto *ptr = node.get();
            m_node_storage.push_back(std::move(node));
            return ptr;
        }

        Event* make_event(){
            auto event = std::make_unique<Event>();
            event->sequence = m_next_sequence++;
            auto *ptr = event.get();
            m_event_storage.push_back(std::move(event));
            return ptr;
        }

        static size_t subtree_size(const Node *node){
            return (node == nullptr) ? 0U : node->subtree_size;
        }

        static void update_node(Node *node){
            if(node == nullptr){
                return;
            }
            node->subtree_size = subtree_size(node->left) + subtree_size(node->right) + 1U;
            if(node->left != nullptr){
                node->left->parent = node;
            }
            if(node->right != nullptr){
                node->right->parent = node;
            }
        }

        static Node* merge_treaps(Node *lhs, Node *rhs){
            if(lhs == nullptr){
                if(rhs != nullptr){
                    rhs->parent = nullptr;
                }
                return rhs;
            }
            if(rhs == nullptr){
                lhs->parent = nullptr;
                return lhs;
            }

            if(lhs->priority < rhs->priority){
                lhs->right = merge_treaps(lhs->right, rhs);
                update_node(lhs);
                lhs->parent = nullptr;
                return lhs;
            }

            rhs->left = merge_treaps(lhs, rhs->left);
            update_node(rhs);
            rhs->parent = nullptr;
            return rhs;
        }

        static std::pair<Node*, Node*> split_treap(Node *root, size_t left_count){
            if(root == nullptr){
                return { nullptr, nullptr };
            }

            if(subtree_size(root->left) >= left_count){
                auto halves = split_treap(root->left, left_count);
                root->left = halves.second;
                update_node(root);
                if(halves.first != nullptr){
                    halves.first->parent = nullptr;
                }
                root->parent = nullptr;
                return { halves.first, root };
            }

            auto halves = split_treap(root->right, left_count - subtree_size(root->left) - 1U);
            root->right = halves.first;
            update_node(root);
            if(halves.second != nullptr){
                halves.second->parent = nullptr;
            }
            root->parent = nullptr;
            return { root, halves.second };
        }

        static Node* leftmost_arc(Node *root){
            auto *node = root;
            while((node != nullptr) && (node->left != nullptr)){
                node = node->left;
            }
            return node;
        }

        size_t arc_rank(const Node *node) const {
            size_t rank = subtree_size(node->left);
            auto *cur = node;
            while(cur->parent != nullptr){
                if(cur->parent->right == cur){
                    rank += subtree_size(cur->parent->left) + 1U;
                }
                cur = cur->parent;
            }
            return rank;
        }

        void replace_arc(Node *arc, const std::vector<Node*> &replacement){
            const auto rank = arc_rank(arc);
            auto left_and_tail = split_treap(m_root, rank);
            auto arc_and_right = split_treap(left_and_tail.second, 1U);
            (void)arc_and_right.first;

            Node *middle = nullptr;
            for(auto *node : replacement){
                node->parent = nullptr;
                node->left = nullptr;
                node->right = nullptr;
                node->subtree_size = 1U;
                middle = merge_treaps(middle, node);
            }

            m_root = merge_treaps(merge_treaps(left_and_tail.first, middle), arc_and_right.second);
            if(m_root != nullptr){
                m_root->parent = nullptr;
            }
        }

        void remove_arc(Node *arc){
            const auto rank = arc_rank(arc);
            auto left_and_tail = split_treap(m_root, rank);
            auto arc_and_right = split_treap(left_and_tail.second, 1U);
            (void)arc_and_right.first;
            m_root = merge_treaps(left_and_tail.first, arc_and_right.second);
            if(m_root != nullptr){
                m_root->parent = nullptr;
            }
        }

        void insert_arc_before(Node *arc, Node *inserted){
            const auto rank = arc_rank(arc);
            auto halves = split_treap(m_root, rank);
            inserted->parent = nullptr;
            inserted->left = nullptr;
            inserted->right = nullptr;
            inserted->subtree_size = 1U;
            m_root = merge_treaps(merge_treaps(halves.first, inserted), halves.second);
            if(m_root != nullptr){
                m_root->parent = nullptr;
            }
        }

        void reset_edge_geometry(InternalEdge &edge) const {
            const auto midpoint = (m_sites.at(edge.left_site) + m_sites.at(edge.right_site)) / static_cast<T>(2);
            edge.sample_point = midpoint;
            edge.direction = perp_ccw(m_sites.at(edge.right_site) - m_sites.at(edge.left_site));
            if(edge.direction.sq_length() == static_cast<T>(0)){
                edge.direction = vec2<T>(static_cast<T>(0), static_cast<T>(1));
            }
        }

        void retarget_active_edge(size_t edge_index, size_t left_site, size_t right_site){
            auto &edge = m_edges.at(edge_index);
            if(edge.vertex0.has_value() || edge.vertex1.has_value()){
                throw std::runtime_error("Voronoi sweep-line tried to retarget a finished edge during a same-level site insertion.");
            }
            edge.left_site = left_site;
            edge.right_site = right_site;
            reset_edge_geometry(edge);
        }

        size_t create_edge(size_t left_site, size_t right_site){
            InternalEdge edge;
            edge.left_site = left_site;
            edge.right_site = right_site;
            reset_edge_geometry(edge);
            m_edges.push_back(edge);
            return m_edges.size() - 1;
        }

        void invalidate_circle_event(Node *arc){
            if((arc != nullptr) && (arc->circle_event != nullptr)){
                arc->circle_event->valid = false;
                arc->circle_event = nullptr;
            }
        }

        long double evaluate_parabola_y(size_t site_index, long double x, long double sweep_y) const {
            const auto &site = m_sites.at(site_index);
            const auto denom = static_cast<long double>(2) * (static_cast<long double>(site.y) - sweep_y);
            if(!(denom > static_cast<long double>(0))){
                return std::numeric_limits<long double>::infinity();
            }
            const auto dx = x - static_cast<long double>(site.x);
            const auto yy = static_cast<long double>(site.y);
            return (dx * dx + yy * yy - sweep_y * sweep_y) / denom;
        }

        // The beach line is the lower envelope of the site parabolas, so the active breakpoint is the intersection
        // with the smaller y-value among the two analytic roots (Fortune, 1987; de Berg et al., 2008).
        long double breakpoint_x(size_t left_site, size_t right_site, long double sweep_y) const {
            const auto &p = m_sites.at(left_site);
            const auto &q = m_sites.at(right_site);
            const auto py = static_cast<long double>(p.y);
            const auto qy = static_cast<long double>(q.y);
            const auto px = static_cast<long double>(p.x);
            const auto qx = static_cast<long double>(q.x);

            if(py == qy){
                return (px + qx) / static_cast<long double>(2);
            }

            const auto z0 = static_cast<long double>(2) * (py - sweep_y);
            const auto z1 = static_cast<long double>(2) * (qy - sweep_y);
            if(!(z0 > static_cast<long double>(0)) || !(z1 > static_cast<long double>(0))){
                return (px + qx) / static_cast<long double>(2);
            }

            const auto a = static_cast<long double>(1) / z0 - static_cast<long double>(1) / z1;
            const auto b = -static_cast<long double>(2) * (px / z0 - qx / z1);
            const auto c = (px * px + py * py - sweep_y * sweep_y) / z0
                         - (qx * qx + qy * qy - sweep_y * sweep_y) / z1;

            if(std::abs(a) <= event_tolerance()){
                if(std::abs(b) <= event_tolerance()){
                    return (px + qx) / static_cast<long double>(2);
                }
                return -c / b;
            }

            auto disc = b * b - static_cast<long double>(4) * a * c;
            if((disc < static_cast<long double>(0)) && (std::abs(disc) <= event_tolerance())){
                disc = static_cast<long double>(0);
            }
            if(disc < static_cast<long double>(0)){
                return (px + qx) / static_cast<long double>(2);
            }

            const auto root = std::sqrt(disc);
            const auto x0 = (-b - root) / (static_cast<long double>(2) * a);
            const auto x1 = (-b + root) / (static_cast<long double>(2) * a);
            const auto y0 = evaluate_parabola_y(left_site, x0, sweep_y);
            const auto y1 = evaluate_parabola_y(left_site, x1, sweep_y);
            return (y0 < y1) ? x0 : x1;
        }

        PrecisePoint2 breakpoint_point(size_t left_site, size_t right_site, long double sweep_y) const {
            const auto x = breakpoint_x(left_site, right_site, sweep_y);
            return PrecisePoint2{x, evaluate_parabola_y(left_site, x, sweep_y)};
        }

        Node* find_arc_above(long double x, long double sweep_y) const {
            auto *node = m_root;
            while(node != nullptr){
                if((node->prev != nullptr)
                && (x < breakpoint_x(node->prev->site_index, node->site_index, sweep_y))){
                    node = node->left;
                    continue;
                }
                if((node->next != nullptr)
                && !(x < breakpoint_x(node->site_index, node->next->site_index, sweep_y))){
                    node = node->right;
                    continue;
                }
                return node;
            }
            return nullptr;
        }

        long double event_tolerance() const {
            return event_guard_factor * std::numeric_limits<long double>::epsilon();
        }

        bool compute_circumcenter(const vec2<T> &a,
                                  const vec2<T> &b,
                                  const vec2<T> &c,
                                  PrecisePoint2 &center,
                                  long double &radius) const {
            const long double ax = static_cast<long double>(a.x) - static_cast<long double>(b.x);
            const long double ay = static_cast<long double>(a.y) - static_cast<long double>(b.y);
            const long double cx = static_cast<long double>(c.x) - static_cast<long double>(b.x);
            const long double cy = static_cast<long double>(c.y) - static_cast<long double>(b.y);

            const auto denom = static_cast<long double>(2) * (ax * cy - ay * cx);
            if(std::abs(denom) <= event_tolerance()){
                return false;
            }

            const auto a_len = ax * ax + ay * ay;
            const auto c_len = cx * cx + cy * cy;
            const auto ux = (a_len * cy - c_len * ay) / denom;
            const auto uy = (ax * c_len - cx * a_len) / denom;
            center = PrecisePoint2{static_cast<long double>(b.x) + ux,
                                    static_cast<long double>(b.y) + uy};
            const auto dx = center.x - static_cast<long double>(a.x);
            const auto dy = center.y - static_cast<long double>(a.y);
            radius = std::sqrt(dx * dx + dy * dy);
            return std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(radius);
        }

        static VertexCellKey vertex_cell_key_for(const PrecisePoint2 &center, long double tol){
            return VertexCellKey{
                static_cast<int64_t>(std::floor(center.x / tol)),
                static_cast<int64_t>(std::floor(center.y / tol))
            };
        }

        void merge_vertex_sites(InternalVertex &vertex, const std::array<size_t, 3> &sites){
            for(const auto site : sites){
                if(std::find(vertex.incident_sites.begin(),
                             vertex.incident_sites.end(),
                             site) == vertex.incident_sites.end()){
                    vertex.incident_sites.push_back(site);
                }
            }
            std::sort(vertex.incident_sites.begin(), vertex.incident_sites.end());
        }

        size_t add_vertex(const PrecisePoint2 &center, std::array<size_t, 3> sites){
            const auto tol = vertex_merge_tolerance_factor
                           * std::sqrt(static_cast<long double>(std::numeric_limits<T>::epsilon()));
            const auto base_cell = vertex_cell_key_for(center, tol);

            for(int64_t dy = -1; dy <= 1; ++dy){
                for(int64_t dx = -1; dx <= 1; ++dx){
                    const VertexCellKey key{base_cell.x + dx, base_cell.y + dy};
                    const auto grid_it = m_vertex_grid.find(key);
                    if(grid_it == m_vertex_grid.end()){
                        continue;
                    }

                    for(const auto idx : grid_it->second){
                        const auto ddx = static_cast<long double>(m_vertices.at(idx).position.x) - center.x;
                        const auto ddy = static_cast<long double>(m_vertices.at(idx).position.y) - center.y;
                        if((ddx * ddx + ddy * ddy) <= (tol * tol)){
                            merge_vertex_sites(m_vertices.at(idx), sites);
                            return idx;
                        }
                    }
                }
            }

            InternalVertex vert;
            vert.position = vec2<T>(static_cast<T>(center.x), static_cast<T>(center.y));
            vert.incident_sites.assign(sites.begin(), sites.end());
            std::sort(vert.incident_sites.begin(), vert.incident_sites.end());
            vert.incident_sites.erase(std::unique(vert.incident_sites.begin(), vert.incident_sites.end()),
                                      vert.incident_sites.end());
            m_vertices.push_back(vert);
            const auto index = m_vertices.size() - 1;
            m_vertex_grid[base_cell].push_back(index);
            return index;
        }

        void finish_edge(size_t edge_index, size_t vertex_index){
            auto &edge = m_edges.at(edge_index);
            if(!edge.vertex0.has_value()){
                edge.vertex0 = vertex_index;
                return;
            }
            if(!edge.vertex1.has_value() && (edge.vertex0.value() != vertex_index)){
                edge.vertex1 = vertex_index;
            }
        }

        // Circle events are only valid for clockwise triples of consecutive arcs in a top-to-bottom sweep. The
        // orientation decision uses Shewchuk-style adaptive predicates through orient_sign so that nearly collinear
        // triples do not spuriously generate unstable Voronoi vertices.
        void schedule_circle_event(Node *arc, long double current_sweep_y){
            if((arc == nullptr) || (arc->prev == nullptr) || (arc->next == nullptr)){
                return;
            }
            if((arc->prev->site_index == arc->site_index) || (arc->next->site_index == arc->site_index)){
                return;
            }
            if(arc->prev->site_index == arc->next->site_index){
                return;
            }

            const auto sign = orient_sign(m_sites.at(arc->prev->site_index),
                                          m_sites.at(arc->site_index),
                                          m_sites.at(arc->next->site_index));
            if(sign >= 0){
                return;
            }

            PrecisePoint2 center;
            long double radius = 0.0L;
            if(!compute_circumcenter(m_sites.at(arc->prev->site_index),
                                     m_sites.at(arc->site_index),
                                     m_sites.at(arc->next->site_index),
                                     center,
                                     radius)){
                return;
            }
            const auto center_point = vec2<T>(static_cast<T>(center.x), static_cast<T>(center.y));
            if(!is_finite_2d(center_point)
            || (incircle_sign(m_sites.at(arc->prev->site_index),
                              m_sites.at(arc->site_index),
                              m_sites.at(arc->next->site_index),
                              center_point) <= 0)){
                return;
            }

            const auto event_y = center.y - radius;
            const auto guard = ray_sample_guard_factor
                             * std::sqrt(static_cast<long double>(std::numeric_limits<T>::epsilon()));
            if(!(event_y < (current_sweep_y - guard))){
                return;
            }

            auto *event = make_event();
            event->type = EventType::Circle;
            event->arc = arc;
            event->center = center;
            event->x = center.x;
            event->y = event_y;
            arc->circle_event = event;
            m_events.push(event);
        }

        bool same_level_sites(size_t lhs, size_t rhs) const {
            return std::abs(static_cast<long double>(m_sites.at(lhs).y) - static_cast<long double>(m_sites.at(rhs).y))
                <= event_tolerance();
        }

        long double site_breakpoint_tolerance() const {
            return ray_sample_guard_factor
                 * std::sqrt(static_cast<long double>(std::numeric_limits<T>::epsilon()));
        }

        void handle_breakpoint_site_event(Node *left_arc,
                                          Node *right_arc,
                                          size_t site_index,
                                          long double directrix){
            if((left_arc == nullptr) || (right_arc == nullptr) || !right_arc->left_edge_index.has_value()){
                throw std::runtime_error("Voronoi sweep-line could not split a site event that landed on a breakpoint.");
            }

            auto *outer_left = left_arc->prev;
            auto *outer_right = right_arc->next;
            invalidate_circle_event(outer_left);
            invalidate_circle_event(left_arc);
            invalidate_circle_event(right_arc);
            invalidate_circle_event(outer_right);

            auto *mid_arc = make_arc(site_index);
            mid_arc->left_edge_index = create_edge(left_arc->site_index, site_index);
            mid_arc->pending_edge_index = right_arc->left_edge_index;
            right_arc->left_edge_index = create_edge(site_index, right_arc->site_index);

            mid_arc->prev = left_arc;
            mid_arc->next = right_arc;
            left_arc->next = mid_arc;
            right_arc->prev = mid_arc;
            insert_arc_before(right_arc, mid_arc);

            PrecisePoint2 center;
            long double radius = 0.0L;
            if(compute_circumcenter(m_sites.at(left_arc->site_index),
                                    m_sites.at(mid_arc->site_index),
                                    m_sites.at(right_arc->site_index),
                                    center,
                                    radius)){
                const auto center_point = vec2<T>(static_cast<T>(center.x), static_cast<T>(center.y));
                const auto event_y = center.y - radius;
                if(is_finite_2d(center_point)
                && (incircle_sign(m_sites.at(left_arc->site_index),
                                  m_sites.at(mid_arc->site_index),
                                  m_sites.at(right_arc->site_index),
                                  center_point) > 0)
                && (std::abs(event_y - static_cast<long double>(m_sites.at(site_index).y))
                    <= site_breakpoint_tolerance())){
                    const auto vertex_index = add_vertex(center,
                                                         {{ left_arc->site_index,
                                                            mid_arc->site_index,
                                                            right_arc->site_index }});
                    finish_edge(mid_arc->pending_edge_index.value(), vertex_index);
                    finish_edge(mid_arc->left_edge_index.value(), vertex_index);
                    finish_edge(right_arc->left_edge_index.value(), vertex_index);

                    left_arc->next = right_arc;
                    right_arc->prev = left_arc;
                    right_arc->left_edge_index = mid_arc->pending_edge_index;
                    remove_arc(mid_arc);

                    schedule_circle_event(outer_left, directrix);
                    schedule_circle_event(left_arc, directrix);
                    schedule_circle_event(right_arc, directrix);
                    schedule_circle_event(outer_right, directrix);
                    return;
                }
            }

            schedule_circle_event(outer_left, directrix);
            schedule_circle_event(left_arc, directrix);
            schedule_circle_event(mid_arc, directrix);
            schedule_circle_event(right_arc, directrix);
            schedule_circle_event(outer_right, directrix);
        }

        void handle_same_level_site_event(Node *arc, size_t site_index, long double directrix){
            auto *prev_arc = arc->prev;
            auto *next_arc = arc->next;

            invalidate_circle_event(prev_arc);
            invalidate_circle_event(next_arc);

            const bool insert_left = (m_sites.at(site_index).x < m_sites.at(arc->site_index).x);
            auto *left_arc = make_arc(insert_left ? site_index : arc->site_index);
            auto *right_arc = make_arc(insert_left ? arc->site_index : site_index);
            right_arc->left_edge_index = create_edge(left_arc->site_index, right_arc->site_index);

            left_arc->left_edge_index = arc->left_edge_index;
            left_arc->prev = prev_arc;
            if(prev_arc != nullptr){
                prev_arc->next = left_arc;
            }
            left_arc->next = right_arc;

            right_arc->prev = left_arc;
            right_arc->next = next_arc;
            if(next_arc != nullptr){
                next_arc->prev = right_arc;
            }

            if(insert_left){
                if((prev_arc != nullptr) && left_arc->left_edge_index.has_value()){
                    retarget_active_edge(left_arc->left_edge_index.value(), prev_arc->site_index, left_arc->site_index);
                }else{
                    left_arc->left_edge_index.reset();
                }
            }else if((next_arc != nullptr) && next_arc->left_edge_index.has_value()){
                retarget_active_edge(next_arc->left_edge_index.value(), right_arc->site_index, next_arc->site_index);
            }

            replace_arc(arc, { left_arc, right_arc });

            schedule_circle_event(left_arc, directrix);
            schedule_circle_event(right_arc, directrix);
            schedule_circle_event(left_arc->prev, directrix);
            schedule_circle_event(right_arc->next, directrix);
        }

        void handle_site_event(size_t site_index){
            const auto directrix = std::nextafter(static_cast<long double>(m_sites.at(site_index).y),
                                                  -std::numeric_limits<long double>::infinity());
            if(m_root == nullptr){
                m_root = make_arc(site_index);
                return;
            }

            auto *arc = find_arc_above(static_cast<long double>(m_sites.at(site_index).x), directrix);
            if(arc == nullptr){
                throw std::runtime_error("Voronoi sweep-line could not locate a beach-line arc for a site event.");
            }
            auto *prev_arc = arc->prev;
            auto *next_arc = arc->next;
            invalidate_circle_event(arc);
            invalidate_circle_event(prev_arc);
            invalidate_circle_event(next_arc);

            const auto site_x = static_cast<long double>(m_sites.at(site_index).x);
            const auto split_tol = site_breakpoint_tolerance();
            if((prev_arc != nullptr) && arc->left_edge_index.has_value()
            && (std::abs(site_x - breakpoint_x(prev_arc->site_index, arc->site_index, directrix)) <= split_tol)){
                handle_breakpoint_site_event(prev_arc, arc, site_index, directrix);
                return;
            }
            if((next_arc != nullptr) && next_arc->left_edge_index.has_value()
            && (std::abs(site_x - breakpoint_x(arc->site_index, next_arc->site_index, directrix)) <= split_tol)){
                handle_breakpoint_site_event(arc, next_arc, site_index, directrix);
                return;
            }

            if(same_level_sites(arc->site_index, site_index)){
                handle_same_level_site_event(arc, site_index, directrix);
                return;
            }

            const auto edge_index = create_edge(arc->site_index, site_index);
            auto *left_arc = make_arc(arc->site_index);
            auto *mid_arc = make_arc(site_index);
            auto *right_arc = make_arc(arc->site_index);
            left_arc->left_edge_index = arc->left_edge_index;
            mid_arc->left_edge_index = edge_index;
            right_arc->left_edge_index = edge_index;

            left_arc->prev = prev_arc;
            if(left_arc->prev != nullptr){
                left_arc->prev->next = left_arc;
            }
            left_arc->next = mid_arc;

            mid_arc->prev = left_arc;
            mid_arc->next = right_arc;

            right_arc->prev = mid_arc;
            right_arc->next = next_arc;
            if(right_arc->next != nullptr){
                right_arc->next->prev = right_arc;
            }

            replace_arc(arc, { left_arc, mid_arc, right_arc });

            schedule_circle_event(left_arc, directrix);
            schedule_circle_event(right_arc, directrix);
            schedule_circle_event(left_arc->prev, directrix);
            schedule_circle_event(right_arc->next, directrix);
        }

        // When the middle arc disappears, the two converging breakpoint traces meet at a Voronoi vertex and the
        // surviving ancestor breakpoint is retargeted to the newly adjacent arc pair.
        void handle_circle_event(Event *event){
            auto *arc = event->arc;
            if((arc == nullptr) || (arc->circle_event != event)){
                return;
            }

            if((arc->prev == nullptr) || (arc->next == nullptr) || !arc->left_edge_index.has_value()
            || !arc->next->left_edge_index.has_value()){
                arc->circle_event = nullptr;
                return;
            }

            auto *left_arc = arc->prev;
            auto *right_arc = arc->next;
            invalidate_circle_event(left_arc);
            invalidate_circle_event(right_arc);
            invalidate_circle_event(arc);

            const auto vertex_index = add_vertex(event->center,
                                                 {{ left_arc->site_index,
                                                    arc->site_index,
                                                    right_arc->site_index }});
            finish_edge(arc->left_edge_index.value(), vertex_index);
            finish_edge(right_arc->left_edge_index.value(), vertex_index);

            left_arc->next = right_arc;
            right_arc->prev = left_arc;

            auto continuing_edge = arc->pending_edge_index.has_value()
                                 ? arc->pending_edge_index.value()
                                 : create_edge(left_arc->site_index, right_arc->site_index);
            finish_edge(continuing_edge, vertex_index);
            right_arc->left_edge_index = continuing_edge;
            remove_arc(arc);

            schedule_circle_event(left_arc, event->y);
            schedule_circle_event(right_arc, event->y);
        }

        void finalize_active_edges(){
            T min_y = m_sites.front().y;
            T max_y = m_sites.front().y;
            for(const auto &site : m_sites){
                min_y = std::min(min_y, site.y);
                max_y = std::max(max_y, site.y);
            }
            const auto span = std::max<T>(max_y - min_y, static_cast<T>(1));
            const auto final_y = static_cast<long double>(min_y - static_cast<T>(4) * span - static_cast<T>(4));

            std::map<size_t, std::vector<vec2<T>>> samples;
            for(auto *arc = leftmost_arc(m_root); arc != nullptr; arc = arc->next){
                if((arc->prev != nullptr) && arc->left_edge_index.has_value()){
                    const auto sample = breakpoint_point(arc->prev->site_index, arc->site_index, final_y);
                    samples[arc->left_edge_index.value()].emplace_back(static_cast<T>(sample.x),
                                                                        static_cast<T>(sample.y));
                }
            }

            for(size_t i = 0; i < m_edges.size(); ++i){
                auto &edge = m_edges.at(i);
                const auto canonical = perp_ccw(m_sites.at(edge.right_site) - m_sites.at(edge.left_site));
                if(edge.vertex0.has_value() && edge.vertex1.has_value()){
                    const auto p0 = m_vertices.at(edge.vertex0.value()).position;
                    const auto p1 = m_vertices.at(edge.vertex1.value()).position;
                    edge.sample_point = (p0 + p1) / static_cast<T>(2);
                    edge.direction = p1 - p0;
                    if(edge.direction.sq_length() == static_cast<T>(0)){
                        edge.direction = canonical;
                    }
                    continue;
                }

                if(edge.vertex0.has_value() || edge.vertex1.has_value()){
                    const auto vertex_index = edge.vertex0.has_value() ? edge.vertex0.value() : edge.vertex1.value();
                    const auto endpoint = m_vertices.at(vertex_index).position;
                    auto chosen = endpoint + canonical;
                    const auto it = samples.find(i);
                    if(it != samples.end()){
                        for(const auto &candidate : it->second){
                            if(sq_dist(candidate, endpoint) > static_cast<T>(ray_sample_guard_factor)
                                                             * std::numeric_limits<T>::epsilon()){
                                chosen = candidate;
                                break;
                            }
                        }
                    }
                    edge.sample_point = chosen;
                    edge.direction = chosen - endpoint;
                    if(edge.direction.sq_length() == static_cast<T>(0)){
                        edge.direction = canonical;
                    }
                    continue;
                }

                const auto it = samples.find(i);
                if((it != samples.end()) && (it->second.size() >= 2)){
                    edge.sample_point = it->second.front();
                    edge.direction = it->second.back() - it->second.front();
                }else{
                    edge.sample_point = (m_sites.at(edge.left_site) + m_sites.at(edge.right_site)) / static_cast<T>(2);
                    edge.direction = canonical;
                }
                if(edge.direction.sq_length() == static_cast<T>(0)){
                    edge.direction = canonical;
                }
            }
        }

        vec2<T> representative_point_for_edge(const InternalEdge &edge) const {
            if(edge.vertex0.has_value()){
                return m_vertices.at(edge.vertex0.value()).position;
            }
            if(edge.vertex1.has_value()){
                return m_vertices.at(edge.vertex1.value()).position;
            }
            return edge.sample_point;
        }

        void finalize_output(){
            m_output.vertices.clear();
            m_output.vertices.reserve(m_vertices.size());
            m_output.cell_edges.assign(m_input_sites.size(), {});
            for(const auto &vertex : m_vertices){
                typename VoronoiDiagram2<T, I>::Vertex out_vertex;
                out_vertex.position = denormalize_point(vertex.position, m_norm);
                out_vertex.incident_sites.reserve(vertex.incident_sites.size());
                for(const auto site : vertex.incident_sites){
                    out_vertex.incident_sites.push_back(static_cast<I>(site));
                }
                m_output.vertices.push_back(std::move(out_vertex));
            }

            m_output.edges.clear();
            m_output.edges.reserve(m_edges.size());
            for(size_t edge_index = 0; edge_index < m_edges.size(); ++edge_index){
                const auto &edge = m_edges.at(edge_index);
                typename VoronoiDiagram2<T, I>::Edge out_edge;
                out_edge.left_site = static_cast<I>(edge.left_site);
                out_edge.right_site = static_cast<I>(edge.right_site);
                if(edge.vertex0.has_value()){
                    out_edge.vertex0 = static_cast<I>(edge.vertex0.value());
                }
                if(edge.vertex1.has_value()){
                    out_edge.vertex1 = static_cast<I>(edge.vertex1.value());
                }
                out_edge.sample_point = denormalize_point(edge.sample_point, m_norm);
                out_edge.direction = denormalize_direction(edge.direction, m_norm);
                m_output.edges.push_back(out_edge);
                m_output.cell_edges.at(edge.left_site).push_back(static_cast<I>(edge_index));
                m_output.cell_edges.at(edge.right_site).push_back(static_cast<I>(edge_index));
            }

            for(size_t site = 0; site < m_output.cell_edges.size(); ++site){
                auto &edges = m_output.cell_edges.at(site);
                std::sort(edges.begin(), edges.end(), [&](I lhs, I rhs){
                    const auto pl = representative_point_for_edge(m_edges.at(static_cast<size_t>(lhs)));
                    const auto pr = representative_point_for_edge(m_edges.at(static_cast<size_t>(rhs)));
                    const auto &s = m_sites.at(site);
                    const auto al = std::atan2(static_cast<long double>(pl.y - s.y),
                                               static_cast<long double>(pl.x - s.x));
                    const auto ar = std::atan2(static_cast<long double>(pr.y - s.y),
                                               static_cast<long double>(pr.x - s.x));
                    if(al != ar){
                        return al < ar;
                    }
                    return lhs < rhs;
                });
                edges.erase(std::unique(edges.begin(), edges.end()), edges.end());
            }

            YLOGDEBUG("Voronoi diagram produced " << m_output.vertices.size()
                      << " vertex/vertices and " << m_output.edges.size() << " edge(s)");
        }
};

} // namespace


template <class T, class I>
VoronoiDiagram2<T, I>
Voronoi_Diagram_2(const std::vector<vec2<T>> &verts){
    FortuneVoronoiBuilder<T, I> builder(verts);
    return builder.build();
}

template <class T, class I>
fv_surface_mesh<T, I>
Triangulate_Voronoi(const std::vector<std::vector<vec2<T>>> &verts,
                    const VoronoiDiagram2<T, I> &diagram){
    if(verts.size() != diagram.cell_edges.size()){
        throw std::invalid_argument("Triangulate_Voronoi requires one site polygon per Voronoi cell.");
    }

    fv_surface_mesh<T, I> mesh;
    mesh.vertices.reserve(verts.size());

    std::vector<vec2<T>> representatives;
    representatives.reserve(verts.size());
    for(const auto &poly : verts){
        const auto rep = representative_site_point(poly);
        representatives.push_back(rep);
        mesh.vertices.emplace_back(rep.x, rep.y, static_cast<T>(0));
    }

    std::set<std::array<size_t, 3>> seen_faces;
    for(const auto &vertex : diagram.vertices){
        if(vertex.incident_sites.size() < 3){
            continue;
        }

        std::vector<size_t> sites;
        sites.reserve(vertex.incident_sites.size());
        for(const auto site_index : vertex.incident_sites){
            const auto idx = static_cast<size_t>(site_index);
            if(idx >= representatives.size()){
                throw std::runtime_error("Triangulate_Voronoi encountered a Voronoi vertex referencing an out-of-range site.");
            }
            sites.push_back(idx);
        }
        std::sort(sites.begin(), sites.end());
        sites.erase(std::unique(sites.begin(), sites.end()), sites.end());
        std::vector<std::pair<long double, size_t>> ordered_sites;
        ordered_sites.reserve(sites.size());
        for(const auto site : sites){
            ordered_sites.emplace_back(
                std::atan2(static_cast<long double>(representatives.at(site).y - vertex.position.y),
                           static_cast<long double>(representatives.at(site).x - vertex.position.x)),
                site);
        }
        const auto angle_eps = 16.0L * std::numeric_limits<long double>::epsilon();
        std::sort(ordered_sites.begin(), ordered_sites.end(), [&](const auto &lhs, const auto &rhs){
            if(std::abs(lhs.first - rhs.first) > angle_eps){
                return lhs.first < rhs.first;
            }
            return lhs.second < rhs.second;
        });
        for(size_t i = 0; i < sites.size(); ++i){
            sites.at(i) = ordered_sites.at(i).second;
        }
        if(sites.size() < 3){
            continue;
        }

        const auto anchor = sites.front();
        for(size_t i = 1; (i + 1) < sites.size(); ++i){
            const auto a = anchor;
            const auto b = sites.at(i);
            const auto c = sites.at(i + 1);
            const auto sign = orient_sign(representatives.at(a),
                                          representatives.at(b),
                                          representatives.at(c));
            if(sign == 0){
                continue;
            }

            auto key = std::array<size_t, 3>{{ a, b, c }};
            std::sort(key.begin(), key.end());
            if(!seen_faces.insert(key).second){
                continue;
            }

            if(sign > 0){
                mesh.faces.push_back({ static_cast<I>(a), static_cast<I>(b), static_cast<I>(c) });
            }else{
                mesh.faces.push_back({ static_cast<I>(a), static_cast<I>(c), static_cast<I>(b) });
            }
        }
    }

    return mesh;
}

#ifndef YGOR_MATH_VORONOI_DISABLE_ALL_SPECIALIZATIONS
    template VoronoiDiagram2<float , uint32_t> Voronoi_Diagram_2(const std::vector<vec2<float >> &);
    template VoronoiDiagram2<float , uint64_t> Voronoi_Diagram_2(const std::vector<vec2<float >> &);

    template VoronoiDiagram2<double, uint32_t> Voronoi_Diagram_2(const std::vector<vec2<double>> &);
    template VoronoiDiagram2<double, uint64_t> Voronoi_Diagram_2(const std::vector<vec2<double>> &);

    template fv_surface_mesh<float , uint32_t> Triangulate_Voronoi(const std::vector<std::vector<vec2<float >>> &,
                                                                     const VoronoiDiagram2<float , uint32_t> &);
    template fv_surface_mesh<float , uint64_t> Triangulate_Voronoi(const std::vector<std::vector<vec2<float >>> &,
                                                                     const VoronoiDiagram2<float , uint64_t> &);

    template fv_surface_mesh<double, uint32_t> Triangulate_Voronoi(const std::vector<std::vector<vec2<double>>> &,
                                                                     const VoronoiDiagram2<double, uint32_t> &);
    template fv_surface_mesh<double, uint64_t> Triangulate_Voronoi(const std::vector<std::vector<vec2<double>>> &,
                                                                     const VoronoiDiagram2<double, uint64_t> &);
#endif
