//YgorGraphAStar.h - Generic A* graph/path finding helpers.

#ifndef YGOR_GRAPH_ASTAR_H_
#define YGOR_GRAPH_ASTAR_H_

#include <algorithm>
#include <cstddef>
#include <functional>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cmath>

// A small, adaptor-driven A* implementation.  The node type must be hashable
// by Hash and comparable by Equal.  Neighbour, cost, heuristic, and goal-test
// logic are provided by callers so the algorithm can be reused for grids,
// image pixels/voxels, surface meshes, point clouds, and arbitrary graphs.
namespace YgorGraphAStar {

    template <class Node>
    struct Search_Result {
        bool success = false;
        double cost = std::numeric_limits<double>::infinity();
        std::vector<Node> path;
        std::size_t nodes_expanded = 0;
        std::size_t nodes_discovered = 0;
    };

    template <class Node,
              class Neighbour_Func,
              class Cost_Func,
              class Heuristic_Func,
              class Goal_Func,
              class Hash = std::hash<Node>,
              class Equal = std::equal_to<Node>,
              class = typename std::enable_if<!std::is_convertible<Neighbour_Func, Node>::value>::type>
    Search_Result<Node> A_Star_Search(const Node &start,
                                       Neighbour_Func neighbours,
                                       Cost_Func travel_cost,
                                       Heuristic_Func heuristic,
                                       Goal_Func is_goal,
                                       Hash hash = Hash(),
                                       Equal equal = Equal()){

        struct Queue_Record {
            Node node;
            double f_score = std::numeric_limits<double>::infinity();
            double g_score = std::numeric_limits<double>::infinity();
            std::size_t sequence = 0;
        };

        struct Queue_Record_Greater {
            bool operator()(const Queue_Record &l, const Queue_Record &r) const {
                if(l.f_score != r.f_score) return r.f_score < l.f_score;
                if(l.g_score != r.g_score) return r.g_score < l.g_score;
                return r.sequence < l.sequence;
            }
        };

        using Score_Map = std::unordered_map<Node, double, Hash, Equal>;
        using Parent_Map = std::unordered_map<Node, Node, Hash, Equal>;

        auto require_valid_nonnegative = [](double v, const char *name) -> void {
            if(!std::isfinite(v)) throw std::runtime_error(std::string("A* encountered non-finite ") + name);
            if(v < 0.0) throw std::runtime_error(std::string("A* encountered negative ") + name);
        };

        const auto h0 = static_cast<double>(heuristic(start));
        require_valid_nonnegative(h0, "heuristic");

        Score_Map g_score(0, hash, equal);
        Parent_Map came_from(0, hash, equal);
        std::priority_queue<Queue_Record, std::vector<Queue_Record>, Queue_Record_Greater> open_set;
        std::size_t sequence = 0;

        g_score.emplace(start, 0.0);
        open_set.push(Queue_Record{ start, h0, 0.0, sequence++ });

        Search_Result<Node> result;
        result.nodes_discovered = 1;

        while(!open_set.empty()){
            const auto current_record = open_set.top();
            open_set.pop();

            const auto current_best_it = g_score.find(current_record.node);
            if(current_best_it == g_score.end()) continue;
            if(current_record.g_score != current_best_it->second) continue; // stale queue entry.

            if(is_goal(current_record.node)){
                result.success = true;
                result.cost = current_record.g_score;

                Node n = current_record.node;
                result.path.push_back(n);
                while(!equal(n, start)){
                    const auto p_it = came_from.find(n);
                    if(p_it == came_from.end()) throw std::runtime_error("A* parent chain is incomplete");
                    n = p_it->second;
                    result.path.push_back(n);
                }
                std::reverse(result.path.begin(), result.path.end());
                return result;
            }

            ++result.nodes_expanded;

            for(const auto &next : neighbours(current_record.node)){
                const auto step_cost = static_cast<double>(travel_cost(current_record.node, next));
                require_valid_nonnegative(step_cost, "travel cost");
                const auto tentative_g = current_record.g_score + step_cost;
                if(!std::isfinite(tentative_g)) throw std::runtime_error("A* accumulated a non-finite path cost");

                const auto known_it = g_score.find(next);
                if((known_it != g_score.end()) && !(tentative_g < known_it->second)) continue;

                const auto h = static_cast<double>(heuristic(next));
                require_valid_nonnegative(h, "heuristic");

                if(known_it == g_score.end()){
                    g_score.emplace(next, tentative_g);
                    ++result.nodes_discovered;
                }else{
                    known_it->second = tentative_g;
                }
                came_from[next] = current_record.node;
                open_set.push(Queue_Record{ next, tentative_g + h, tentative_g, sequence++ });
            }
        }

        return result;
    }

    template <class Node,
              class Neighbour_Func,
              class Cost_Func,
              class Heuristic_Func,
              class Hash = std::hash<Node>,
              class Equal = std::equal_to<Node>>
    Search_Result<Node> A_Star_Search(const Node &start,
                                       const Node &goal,
                                       Neighbour_Func neighbours,
                                       Cost_Func travel_cost,
                                       Heuristic_Func heuristic,
                                       Hash hash = Hash(),
                                       Equal equal = Equal()){
        auto is_goal = [&](const Node &n) -> bool { return equal(n, goal); };
        auto h = [&](const Node &n) -> double { return static_cast<double>(heuristic(n, goal)); };
        return A_Star_Search<Node, Neighbour_Func, Cost_Func, decltype(h), decltype(is_goal), Hash, Equal>(
            start, neighbours, travel_cost, h, is_goal, hash, equal);
    }

} // namespace YgorGraphAStar

#endif
