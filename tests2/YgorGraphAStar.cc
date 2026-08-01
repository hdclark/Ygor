#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include <YgorGraphAStar.h>
#include <YgorImages.h>
#include <YgorMath.h>

#include "doctest/doctest.h"

namespace {

struct Grid_Point {
    int x = 0;
    int y = 0;

    bool operator==(const Grid_Point &rhs) const {
        return (x == rhs.x) && (y == rhs.y);
    }
};

struct Grid_Point_Hash {
    std::size_t operator()(const Grid_Point &p) const {
        const auto hx = std::hash<int>{}(p.x);
        const auto hy = std::hash<int>{}(p.y);
        return hx ^ (hy + 0x9e3779b9U + (hx << 6U) + (hx >> 2U));
    }
};

bool contains_point(const std::vector<Grid_Point> &points, const Grid_Point &needle){
    for(const auto &p : points){
        if(p == needle) return true;
    }
    return false;
}

} // namespace

TEST_CASE( "A_Star_Search_grid_adaptor" ){

    const Grid_Point start{0, 0};
    const Grid_Point goal{4, 4};
    const std::vector<Grid_Point> blocked{
        Grid_Point{1, 0}, Grid_Point{1, 1}, Grid_Point{1, 2}, Grid_Point{1, 3}
    };

    auto neighbours = [&](const Grid_Point &p) -> std::vector<Grid_Point> {
        std::vector<Grid_Point> out;
        const std::array<Grid_Point, 4> deltas{{ Grid_Point{1, 0}, Grid_Point{-1, 0},
                                                 Grid_Point{0, 1}, Grid_Point{0, -1} }};
        for(const auto &d : deltas){
            const Grid_Point q{p.x + d.x, p.y + d.y};
            if((0 <= q.x) && (q.x < 5) && (0 <= q.y) && (q.y < 5) && !contains_point(blocked, q)){
                out.push_back(q);
            }
        }
        return out;
    };

    auto travel_cost = [](const Grid_Point &, const Grid_Point &) -> double { return 1.0; };
    auto manhattan = [](const Grid_Point &a, const Grid_Point &b) -> double {
        return static_cast<double>(std::abs(a.x - b.x) + std::abs(a.y - b.y));
    };

    const auto result = YgorGraphAStar::A_Star_Search<Grid_Point>(
        start, goal, neighbours, travel_cost, manhattan, Grid_Point_Hash{});

    REQUIRE( result.success );
    REQUIRE( result.cost == doctest::Approx(8.0) );
    REQUIRE( result.path.size() == 9U );
    REQUIRE( result.path.front() == start );
    REQUIRE( result.path.back() == goal );
    for(const auto &p : blocked){
        REQUIRE( !contains_point(result.path, p) );
    }
}



TEST_CASE( "A_Star_Search_planar_image_pixel_intensity_cost" ){

    planar_image<double, double> img;
    img.init_buffer(3, 3, 1);

    // Low-intensity pixels form the preferred path around the bright centre pixel.
    img.data = {
        1.0, 1.0, 1.0,
        9.0, 9.0, 1.0,
        1.0, 1.0, 1.0
    };

    const Grid_Point start{0, 0};
    const Grid_Point goal{2, 2};

    auto neighbours = [&](const Grid_Point &p) -> std::vector<Grid_Point> {
        std::vector<Grid_Point> out;
        const std::array<Grid_Point, 4> deltas{{ Grid_Point{1, 0}, Grid_Point{-1, 0},
                                                 Grid_Point{0, 1}, Grid_Point{0, -1} }};
        for(const auto &d : deltas){
            const Grid_Point q{p.x + d.x, p.y + d.y};
            if((0 <= q.x) && (q.x < img.columns) && (0 <= q.y) && (q.y < img.rows)){
                out.push_back(q);
            }
        }
        return out;
    };

    auto travel_cost = [&](const Grid_Point &, const Grid_Point &to) -> double {
        return img.value(to.y, to.x, 0);
    };
    auto manhattan = [](const Grid_Point &a, const Grid_Point &b) -> double {
        return static_cast<double>(std::abs(a.x - b.x) + std::abs(a.y - b.y));
    };

    const auto result = YgorGraphAStar::A_Star_Search<Grid_Point>(
        start, goal, neighbours, travel_cost, manhattan, Grid_Point_Hash{});

    REQUIRE( result.success );
    REQUIRE( result.cost == doctest::Approx(4.0) );
    REQUIRE( result.path == std::vector<Grid_Point>{ Grid_Point{0, 0}, Grid_Point{1, 0}, Grid_Point{2, 0},
                                                     Grid_Point{2, 1}, Grid_Point{2, 2} } );
}

TEST_CASE( "A_Star_Search_surface_mesh_vertices_euclidean_cost" ){

    fv_surface_mesh<double, uint64_t> mesh;
    mesh.vertices = {
        vec3<double>(0.0, 0.0, 0.0),
        vec3<double>(1.0, 0.0, 0.0),
        vec3<double>(2.0, 0.0, 0.0),
        vec3<double>(0.0, 10.0, 0.0)
    };
    mesh.faces = {
        {0, 1, 3},
        {1, 2, 3}
    };

    auto neighbours = [&](uint64_t v) -> std::vector<uint64_t> {
        std::vector<uint64_t> out;
        for(const auto &face : mesh.faces){
            bool face_contains_v = false;
            for(const auto candidate : face){
                if(candidate == v) face_contains_v = true;
            }
            if(face_contains_v){
                for(const auto candidate : face){
                    if(candidate == v) continue;
                    if(std::find(out.begin(), out.end(), candidate) == out.end()) out.push_back(candidate);
                }
            }
        }
        return out;
    };

    auto euclidean_distance = [&](uint64_t a, uint64_t b) -> double {
        return mesh.vertices.at(a).distance(mesh.vertices.at(b));
    };
    auto heuristic = euclidean_distance;

    const auto result = YgorGraphAStar::A_Star_Search<uint64_t>(uint64_t{0}, uint64_t{2}, neighbours, euclidean_distance, heuristic);

    REQUIRE( result.success );
    REQUIRE( result.cost == doctest::Approx(2.0) );
    REQUIRE( result.path == std::vector<uint64_t>{0, 1, 2} );
}

TEST_CASE( "A_Star_Search_weighted_graph_prefers_lower_cost" ){

    const std::unordered_map<int, std::vector<int>> graph{
        {0, {1, 2}},
        {1, {3}},
        {2, {3}},
        {3, {}}
    };
    const std::unordered_map<int, double> edge_cost{
        {1, 100.0}, // 0 -> 1 encoded as 0*10+1
        {2, 1.0},   // 0 -> 2
        {13, 1.0},  // 1 -> 3
        {23, 1.0}   // 2 -> 3
    };

    auto neighbours = [&](int n) -> const std::vector<int>& { return graph.at(n); };
    auto travel_cost = [&](int a, int b) -> double { return edge_cost.at((10 * a) + b); };
    auto heuristic = [](int, int) -> double { return 0.0; };

    const auto result = YgorGraphAStar::A_Star_Search<int>(0, 3, neighbours, travel_cost, heuristic);

    REQUIRE( result.success );
    REQUIRE( result.cost == doctest::Approx(2.0) );
    REQUIRE( result.path == std::vector<int>{0, 2, 3} );
}

TEST_CASE( "A_Star_Search_reports_unreachable_goal" ){

    const std::unordered_map<int, std::vector<int>> graph{
        {0, {1}},
        {1, {}},
        {2, {}}
    };

    auto neighbours = [&](int n) -> const std::vector<int>& { return graph.at(n); };
    auto travel_cost = [](int, int) -> double { return 1.0; };
    auto heuristic = [](int, int) -> double { return 0.0; };

    const auto result = YgorGraphAStar::A_Star_Search<int>(0, 2, neighbours, travel_cost, heuristic);

    REQUIRE( !result.success );
    REQUIRE( std::isinf(result.cost) );
    REQUIRE( result.path.empty() );
}

TEST_CASE( "A_Star_Search_rejects_invalid_costs" ){

    auto neighbours = [](int) -> std::vector<int> { return std::vector<int>{1}; };
    auto negative_travel_cost = [](int, int) -> double { return -1.0; };
    auto heuristic = [](int, int) -> double { return 0.0; };

    REQUIRE_THROWS_AS( YgorGraphAStar::A_Star_Search<int>(0, 1, neighbours, negative_travel_cost, heuristic),
                       std::runtime_error );
}
