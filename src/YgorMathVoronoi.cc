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
#include <string>
#include <tuple>
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
bool same_xy(const vec2<T> &a, const vec2<T> &b){
    return (a.x == b.x) && (a.y == b.y);
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

    Normalization2<T> norm;
    norm.center = vec2<T>((min_x + max_x) / static_cast<T>(2),
                          (min_y + max_y) / static_cast<T>(2));
    norm.scale = std::max(max_x - min_x, max_y - min_y);
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
            enqueue_site_events();

            while(!m_events.empty()){
                auto *event = m_events.top();
                m_events.pop();
                if((event == nullptr) || !event->valid){
                    continue;
                }

                m_sweep_y = event->y;
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
        enum class NodeType {
            Arc,
            Breakpoint,
        };

        enum class EventType {
            Site,
            Circle,
        };

        struct Event;

        struct Node {
            NodeType type = NodeType::Arc;
            Node *parent = nullptr;
            Node *left = nullptr;
            Node *right = nullptr;

            size_t site_index = 0;
            size_t left_site = 0;
            size_t right_site = 0;
            size_t edge_index = 0;

            Node *prev = nullptr;
            Node *next = nullptr;
            Event *circle_event = nullptr;
        };

        struct Event {
            EventType type = EventType::Site;
            long double y = 0.0L;
            long double x = 0.0L;
            size_t site_index = 0;
            Node *arc = nullptr;
            vec2<long double> center;
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
        long double m_sweep_y = std::numeric_limits<long double>::infinity();
        uint64_t m_next_sequence = 0;

        std::vector<std::unique_ptr<Node>> m_node_storage;
        std::vector<std::unique_ptr<Event>> m_event_storage;
        std::priority_queue<Event*, std::vector<Event*>, EventCompare> m_events;
        std::vector<InternalVertex> m_vertices;
        std::vector<InternalEdge> m_edges;

        static constexpr long double event_guard_factor = 1024.0L;

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

        Node* make_arc(size_t site_index){
            auto node = std::make_unique<Node>();
            node->type = NodeType::Arc;
            node->site_index = site_index;
            auto *ptr = node.get();
            m_node_storage.push_back(std::move(node));
            return ptr;
        }

        Node* make_breakpoint(size_t left_site, size_t right_site, size_t edge_index){
            auto node = std::make_unique<Node>();
            node->type = NodeType::Breakpoint;
            node->left_site = left_site;
            node->right_site = right_site;
            node->edge_index = edge_index;
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

        void replace_node(Node *old_node, Node *new_node){
            auto *parent = old_node->parent;
            if(parent == nullptr){
                m_root = new_node;
            }else if(parent->left == old_node){
                parent->left = new_node;
            }else{
                parent->right = new_node;
            }
            if(new_node != nullptr){
                new_node->parent = parent;
            }
        }

        static void attach_children(Node *parent, Node *left_child, Node *right_child){
            parent->left = left_child;
            parent->right = right_child;
            if(left_child != nullptr){
                left_child->parent = parent;
            }
            if(right_child != nullptr){
                right_child->parent = parent;
            }
        }

        static Node* left_breakpoint(Node *arc){
            auto *child = arc;
            auto *parent = arc->parent;
            while((parent != nullptr) && (parent->left == child)){
                child = parent;
                parent = parent->parent;
            }
            return parent;
        }

        static Node* right_breakpoint(Node *arc){
            auto *child = arc;
            auto *parent = arc->parent;
            while((parent != nullptr) && (parent->right == child)){
                child = parent;
                parent = parent->parent;
            }
            return parent;
        }

        size_t create_edge(size_t left_site, size_t right_site){
            InternalEdge edge;
            edge.left_site = left_site;
            edge.right_site = right_site;
            const auto midpoint = (m_sites.at(left_site) + m_sites.at(right_site)) / static_cast<T>(2);
            edge.sample_point = midpoint;
            edge.direction = perp_ccw(m_sites.at(right_site) - m_sites.at(left_site));
            if(edge.direction.sq_length() == static_cast<T>(0)){
                edge.direction = vec2<T>(static_cast<T>(0), static_cast<T>(1));
            }
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

        vec2<long double> breakpoint_point(size_t left_site, size_t right_site, long double sweep_y) const {
            const auto x = breakpoint_x(left_site, right_site, sweep_y);
            return vec2<long double>(x, evaluate_parabola_y(left_site, x, sweep_y));
        }

        Node* find_arc_above(long double x, long double sweep_y) const {
            auto *node = m_root;
            while((node != nullptr) && (node->type == NodeType::Breakpoint)){
                const auto bx = breakpoint_x(node->left_site, node->right_site, sweep_y);
                node = (x < bx) ? node->left : node->right;
            }
            return node;
        }

        long double event_tolerance() const {
            return event_guard_factor * std::numeric_limits<long double>::epsilon();
        }

        bool compute_circumcenter(const vec2<T> &a,
                                  const vec2<T> &b,
                                  const vec2<T> &c,
                                  vec2<long double> &center,
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
            center = vec2<long double>(static_cast<long double>(b.x) + ux,
                                       static_cast<long double>(b.y) + uy);
            const auto dx = center.x - static_cast<long double>(a.x);
            const auto dy = center.y - static_cast<long double>(a.y);
            radius = std::sqrt(dx * dx + dy * dy);
            return std::isfinite(center.x) && std::isfinite(center.y) && std::isfinite(radius);
        }

        size_t add_vertex(const vec2<long double> &center, std::array<size_t, 3> sites){
            const auto tol = static_cast<long double>(64) * std::sqrt(static_cast<long double>(std::numeric_limits<T>::epsilon()));
            for(size_t i = 0; i < m_vertices.size(); ++i){
                const auto dx = static_cast<long double>(m_vertices.at(i).position.x) - center.x;
                const auto dy = static_cast<long double>(m_vertices.at(i).position.y) - center.y;
                if((dx * dx + dy * dy) <= (tol * tol)){
                    for(const auto site : sites){
                        if(std::find(m_vertices.at(i).incident_sites.begin(),
                                     m_vertices.at(i).incident_sites.end(),
                                     site) == m_vertices.at(i).incident_sites.end()){
                            m_vertices.at(i).incident_sites.push_back(site);
                        }
                    }
                    std::sort(m_vertices.at(i).incident_sites.begin(), m_vertices.at(i).incident_sites.end());
                    return i;
                }
            }

            InternalVertex vert;
            vert.position = vec2<T>(static_cast<T>(center.x), static_cast<T>(center.y));
            vert.incident_sites.assign(sites.begin(), sites.end());
            std::sort(vert.incident_sites.begin(), vert.incident_sites.end());
            vert.incident_sites.erase(std::unique(vert.incident_sites.begin(), vert.incident_sites.end()),
                                      vert.incident_sites.end());
            m_vertices.push_back(vert);
            return m_vertices.size() - 1;
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

            vec2<long double> center;
            long double radius = 0.0L;
            if(!compute_circumcenter(m_sites.at(arc->prev->site_index),
                                     m_sites.at(arc->site_index),
                                     m_sites.at(arc->next->site_index),
                                     center,
                                     radius)){
                return;
            }

            const auto event_y = center.y - radius;
            const auto guard = static_cast<long double>(64) * std::sqrt(static_cast<long double>(std::numeric_limits<T>::epsilon()));
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
            invalidate_circle_event(arc);

            const auto edge_index = create_edge(arc->site_index, site_index);
            auto *left_arc = make_arc(arc->site_index);
            auto *mid_arc = make_arc(site_index);
            auto *right_arc = make_arc(arc->site_index);
            auto *right_bp = make_breakpoint(site_index, arc->site_index, edge_index);
            auto *left_bp = make_breakpoint(arc->site_index, site_index, edge_index);

            left_arc->prev = arc->prev;
            if(left_arc->prev != nullptr){
                left_arc->prev->next = left_arc;
            }
            left_arc->next = mid_arc;

            mid_arc->prev = left_arc;
            mid_arc->next = right_arc;

            right_arc->prev = mid_arc;
            right_arc->next = arc->next;
            if(right_arc->next != nullptr){
                right_arc->next->prev = right_arc;
            }

            attach_children(right_bp, mid_arc, right_arc);
            attach_children(left_bp, left_arc, right_bp);
            replace_node(arc, left_bp);

            schedule_circle_event(left_arc, directrix);
            schedule_circle_event(right_arc, directrix);
        }

        void handle_circle_event(Event *event){
            auto *arc = event->arc;
            if((arc == nullptr) || (arc->circle_event != event)){
                return;
            }

            auto *left_bp = ::left_breakpoint(arc);
            auto *right_bp = ::right_breakpoint(arc);
            if((left_bp == nullptr) || (right_bp == nullptr) || (arc->prev == nullptr) || (arc->next == nullptr)){
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
            finish_edge(left_bp->edge_index, vertex_index);
            finish_edge(right_bp->edge_index, vertex_index);

            auto *parent = arc->parent;
            if(parent == nullptr){
                throw std::runtime_error("Voronoi sweep-line encountered a malformed beach-line tree during a circle event.");
            }
            auto *higher = (parent == left_bp) ? right_bp : left_bp;
            auto *sibling = (parent->left == arc) ? parent->right : parent->left;
            replace_node(parent, sibling);

            left_arc->next = right_arc;
            right_arc->prev = left_arc;

            const auto new_edge = create_edge(left_arc->site_index, right_arc->site_index);
            finish_edge(new_edge, vertex_index);
            higher->left_site = left_arc->site_index;
            higher->right_site = right_arc->site_index;
            higher->edge_index = new_edge;

            schedule_circle_event(left_arc, event->y);
            schedule_circle_event(right_arc, event->y);
        }

        void collect_active_breakpoints(Node *node,
                                        long double final_y,
                                        std::map<size_t, std::vector<vec2<T>>> &samples) const {
            if(node == nullptr){
                return;
            }
            if(node->type == NodeType::Breakpoint){
                const auto sample = breakpoint_point(node->left_site, node->right_site, final_y);
                samples[node->edge_index].emplace_back(static_cast<T>(sample.x), static_cast<T>(sample.y));
                collect_active_breakpoints(node->left, final_y, samples);
                collect_active_breakpoints(node->right, final_y, samples);
            }
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
            collect_active_breakpoints(m_root, final_y, samples);

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
                            if(sq_dist(candidate, endpoint) > static_cast<T>(64) * std::numeric_limits<T>::epsilon()){
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

#ifndef YGOR_MATH_VORONOI_DISABLE_ALL_SPECIALIZATIONS
    template VoronoiDiagram2<float , uint32_t> Voronoi_Diagram_2(const std::vector<vec2<float >> &);
    template VoronoiDiagram2<float , uint64_t> Voronoi_Diagram_2(const std::vector<vec2<float >> &);

    template VoronoiDiagram2<double, uint32_t> Voronoi_Diagram_2(const std::vector<vec2<double>> &);
    template VoronoiDiagram2<double, uint64_t> Voronoi_Diagram_2(const std::vector<vec2<double>> &);
#endif
