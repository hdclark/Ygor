
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include <YgorMath.h>
#include <YgorMathTriangulateSeidels.h>

#include "doctest/doctest.h"

namespace {

template <class I>
using edge_type = std::pair<I, I>;

template <class I>
edge_type<I> make_edge(I a, I b){
    if(b < a){
        std::swap(a, b);
    }
    return std::make_pair(a, b);
}

template <class T>
std::vector<vec2<T>> normalized_loop(std::vector<vec2<T>> poly){
    if((poly.size() >= 2) && (poly.front().x == poly.back().x) && (poly.front().y == poly.back().y)){
        poly.pop_back();
    }
    return poly;
}

template <class T, class I>
std::set<edge_type<I>> collect_triangle_edges(const fv_surface_mesh<T, I> &mesh){
    std::set<edge_type<I>> edges;
    for(const auto &face : mesh.faces){
        if(face.size() != 3){
            continue;
        }
        edges.insert(make_edge(face.at(0), face.at(1)));
        edges.insert(make_edge(face.at(1), face.at(2)));
        edges.insert(make_edge(face.at(2), face.at(0)));
    }
    return edges;
}

template <class T>
bool inside_odd_parity_region(const std::vector<std::vector<vec2<T>>> &loops,
                              const vec2<T> &p){
    size_t winding_parity = 0;
    for(const auto &loop_in : loops){
        const auto loop = normalized_loop(loop_in);
        if(::point_in_polygon_or_on_boundary(loop, p)){
            ++winding_parity;
        }
    }
    return (winding_parity % 2) == 1;
}

template <class T, class I>
void require_all_faces_are_triangles(const fv_surface_mesh<T, I> &mesh){
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3);
    }
}

template <class T, class I>
void require_triangle_centroids_inside_odd_parity_region(const fv_surface_mesh<T, I> &mesh,
                                                         const std::vector<std::vector<vec2<T>>> &loops){
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3);
        const auto &a = mesh.vertices.at(face.at(0));
        const auto &b = mesh.vertices.at(face.at(1));
        const auto &c = mesh.vertices.at(face.at(2));
        const vec2<T> centroid((a.x + b.x + c.x) / static_cast<T>(3),
                               (a.y + b.y + c.y) / static_cast<T>(3));
        REQUIRE(inside_odd_parity_region(loops, centroid));
    }
}

template <class T, class I>
void require_loop_edges_are_mesh_edges(const fv_surface_mesh<T, I> &mesh,
                                       const std::vector<std::vector<vec2<T>>> &loops){
    std::set<edge_type<I>> expected;
    I offset = 0;
    for(const auto &loop_in : loops){
        const auto loop = normalized_loop(loop_in);
        for(size_t i = 0; i < loop.size(); ++i){
            expected.insert(make_edge(static_cast<I>(offset + static_cast<I>(i)),
                                      static_cast<I>(offset + static_cast<I>((i + 1) % loop.size()))));
        }
        offset += static_cast<I>(loop.size());
    }

    const auto mesh_edges = collect_triangle_edges(mesh);
    for(const auto &edge : expected){
        REQUIRE(mesh_edges.count(edge) == 1);
    }
}

} // namespace

TEST_CASE("Triangulate_Seidels_2 function"){
    SUBCASE("empty input returns an empty mesh"){
        const std::vector<std::vector<vec2<double>>> closed_polygons;
        const auto mesh = Triangulate_Seidels_2<double, uint32_t>(closed_polygons);
        REQUIRE(mesh.vertices.empty());
        REQUIRE(mesh.faces.empty());
    }

    SUBCASE("explicit closure vertex is accepted for a single triangle"){
        const std::vector<std::vector<vec2<double>>> closed_polygons{{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(0.0, 1.0),
            vec2<double>(0.0, 0.0)
        }};
        const auto mesh = Triangulate_Seidels_2<double, uint32_t>(closed_polygons);
        REQUIRE(mesh.vertices.size() == 3);
        REQUIRE(mesh.faces.size() == 1);
        require_all_faces_are_triangles(mesh);
        require_triangle_centroids_inside_odd_parity_region(mesh, closed_polygons);
    }

    SUBCASE("polygon with a hole is triangulated without assuming winding order"){
        const std::vector<std::vector<vec2<double>>> closed_polygons{
            {
                vec2<double>(0.0, 0.0),
                vec2<double>(0.0, 6.0),
                vec2<double>(6.0, 6.0),
                vec2<double>(6.0, 0.0)
            },
            {
                vec2<double>(2.0, 2.0),
                vec2<double>(4.0, 2.0),
                vec2<double>(4.0, 4.0),
                vec2<double>(2.0, 4.0)
            }
        };
        const auto mesh = Triangulate_Seidels_2<double, uint32_t>(closed_polygons);
        REQUIRE(mesh.vertices.size() == 8);
        REQUIRE(mesh.faces.size() == 8);
        require_all_faces_are_triangles(mesh);
        require_loop_edges_are_mesh_edges(mesh, closed_polygons);
        require_triangle_centroids_inside_odd_parity_region(mesh, closed_polygons);
    }

    SUBCASE("nested polygons of arbitrary depth use odd-even parity"){
        const std::vector<std::vector<vec2<double>>> closed_polygons{
            {
                vec2<double>(0.0, 0.0),
                vec2<double>(10.0, 0.0),
                vec2<double>(10.0, 10.0),
                vec2<double>(0.0, 10.0)
            },
            {
                vec2<double>(8.0, 2.0),
                vec2<double>(2.0, 2.0),
                vec2<double>(2.0, 8.0),
                vec2<double>(8.0, 8.0)
            },
            {
                vec2<double>(3.0, 3.0),
                vec2<double>(7.0, 3.0),
                vec2<double>(7.0, 7.0),
                vec2<double>(3.0, 7.0)
            },
            {
                vec2<double>(6.0, 4.0),
                vec2<double>(4.0, 4.0),
                vec2<double>(4.0, 6.0),
                vec2<double>(6.0, 6.0)
            }
        };
        const auto mesh = Triangulate_Seidels_2<double, uint32_t>(closed_polygons);
        REQUIRE(mesh.vertices.size() == 16);
        REQUIRE(mesh.faces.size() == 16);
        require_all_faces_are_triangles(mesh);
        require_loop_edges_are_mesh_edges(mesh, closed_polygons);
        require_triangle_centroids_inside_odd_parity_region(mesh, closed_polygons);
    }

    SUBCASE("self-intersecting loops are rejected"){
        const std::vector<std::vector<vec2<double>>> closed_polygons{{
            vec2<double>(0.0, 0.0),
            vec2<double>(2.0, 2.0),
            vec2<double>(0.0, 2.0),
            vec2<double>(2.0, 0.0)
        }};
        REQUIRE_THROWS(Triangulate_Seidels_2<double, uint32_t>(closed_polygons));
    }

    SUBCASE("touching loop boundaries are rejected"){
        const std::vector<std::vector<vec2<double>>> closed_polygons{
            {
                vec2<double>(0.0, 0.0),
                vec2<double>(6.0, 0.0),
                vec2<double>(6.0, 6.0),
                vec2<double>(0.0, 6.0)
            },
            {
                vec2<double>(2.0, 0.0),
                vec2<double>(4.0, 0.0),
                vec2<double>(4.0, 2.0),
                vec2<double>(2.0, 2.0)
            }
        };
        REQUIRE_THROWS(Triangulate_Seidels_2<double, uint32_t>(closed_polygons));
    }

    SUBCASE("float coordinates and uint64_t indices are supported"){
        const std::vector<std::vector<vec2<float>>> closed_polygons{
            {
                vec2<float>(0.0f, 0.0f),
                vec2<float>(3.0f, 0.0f),
                vec2<float>(0.0f, 2.0f)
            }
        };
        const auto mesh = Triangulate_Seidels_2<float, uint64_t>(closed_polygons);
        REQUIRE(mesh.vertices.size() == 3);
        REQUIRE(mesh.faces.size() == 1);
        require_all_faces_are_triangles(mesh);
    }
}
