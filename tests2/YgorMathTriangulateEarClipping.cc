
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include <YgorMath.h>
#include <YgorMathTriangulateEarClipping.h>

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
vec2<T> as_vec2(const vec3<T> &v){
    return vec2<T>(v.x, v.y);
}

template <class T, class I>
void require_all_faces_are_triangles(const fv_surface_mesh<T, I> &mesh){
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3);
    }
}

template <class T, class I>
void require_edges_are_manifold(const fv_surface_mesh<T, I> &mesh){
    std::map<edge_type<I>, size_t> counts;
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3);
        ++counts[make_edge(face.at(0), face.at(1))];
        ++counts[make_edge(face.at(1), face.at(2))];
        ++counts[make_edge(face.at(2), face.at(0))];
    }
    for(const auto &[unused_edge, count] : counts){
        (void)unused_edge;
        REQUIRE(count <= 2);
    }
}

template <class T>
T polygon_signed_area(const std::vector<vec2<T>> &poly){
    T area = static_cast<T>(0);
    for(size_t i = 0; i < poly.size(); ++i){
        const auto &a = poly.at(i);
        const auto &b = poly.at((i + 1) % poly.size());
        area += (a.x * b.y) - (b.x * a.y);
    }
    return area / static_cast<T>(2);
}

template <class T, class I>
T mesh_area(const fv_surface_mesh<T, I> &mesh){
    T total = static_cast<T>(0);
    for(const auto &face : mesh.faces){
        const auto &a = mesh.vertices.at(face.at(0));
        const auto &b = mesh.vertices.at(face.at(1));
        const auto &c = mesh.vertices.at(face.at(2));
        total += std::abs((b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y)) / static_cast<T>(2);
    }
    return total;
}

template <class T>
bool point_in_even_odd_region(const std::vector<std::vector<vec2<T>>> &polygons,
                              const vec2<T> &p){
    int parity = 0;
    for(const auto &poly : polygons){
        if(::point_in_polygon_or_on_boundary(poly, p)){
            parity ^= 1;
        }
    }
    return parity != 0;
}

template <class T, class I>
void require_triangle_centroids_within_arrangement(const fv_surface_mesh<T, I> &mesh,
                                                   const std::vector<std::vector<vec2<T>>> &polygons){
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3);
        const auto &a = mesh.vertices.at(face.at(0));
        const auto &b = mesh.vertices.at(face.at(1));
        const auto &c = mesh.vertices.at(face.at(2));
        const vec2<T> centroid((a.x + b.x + c.x) / static_cast<T>(3),
                               (a.y + b.y + c.y) / static_cast<T>(3));
        REQUIRE(point_in_even_odd_region(polygons, centroid));
    }
}

} // namespace

TEST_CASE("Triangulate_Ear_Clipping_2 function"){
    SUBCASE("empty input returns an empty mesh"){
        const std::vector<std::vector<vec2<double>>> polygons;
        const auto mesh = Triangulate_Ear_Clipping_2<double, uint32_t>(polygons);
        REQUIRE(mesh.vertices.empty());
        REQUIRE(mesh.faces.empty());
    }

    SUBCASE("simple convex polygon triangulates to n-2 triangles"){
        const std::vector<std::vector<vec2<double>>> polygons{{
            { vec2<double>(0.0, 0.0), vec2<double>(4.0, 0.0), vec2<double>(4.0, 4.0), vec2<double>(0.0, 4.0) }
        }};
        const auto mesh = Triangulate_Ear_Clipping_2<double, uint32_t>(polygons);
        REQUIRE(mesh.vertices.size() == 4);
        REQUIRE(mesh.faces.size() == 2);
        require_all_faces_are_triangles(mesh);
        require_edges_are_manifold(mesh);
        require_triangle_centroids_within_arrangement(mesh, polygons);
        REQUIRE(mesh_area(mesh) == doctest::Approx(16.0));
    }

    SUBCASE("polygon with a hole triangulates the even-odd interior only"){
        const std::vector<std::vector<vec2<double>>> polygons{{
            { vec2<double>(0.0, 0.0), vec2<double>(4.0, 0.0), vec2<double>(4.0, 4.0), vec2<double>(0.0, 4.0) },
            { vec2<double>(1.0, 1.0), vec2<double>(1.0, 3.0), vec2<double>(3.0, 3.0), vec2<double>(3.0, 1.0) }
        }};
        const auto mesh = Triangulate_Ear_Clipping_2<double, uint32_t>(polygons);
        REQUIRE(mesh.faces.size() == 8);
        require_all_faces_are_triangles(mesh);
        require_edges_are_manifold(mesh);
        require_triangle_centroids_within_arrangement(mesh, polygons);
        REQUIRE(mesh_area(mesh) == doctest::Approx(12.0));
    }

    SUBCASE("arbitrarily nested polygons are handled with even-odd parity"){
        const std::vector<std::vector<vec2<double>>> polygons{{
            { vec2<double>(-10.0, -10.0), vec2<double>(10.0, -10.0), vec2<double>(10.0, 10.0), vec2<double>(-10.0, 10.0) },
            { vec2<double>(-8.0, -8.0), vec2<double>(-8.0, 8.0), vec2<double>(8.0, 8.0), vec2<double>(8.0, -8.0) },
            { vec2<double>(-4.0, -4.0), vec2<double>(4.0, -4.0), vec2<double>(4.0, 4.0), vec2<double>(-4.0, 4.0) },
            { vec2<double>(-2.0, -2.0), vec2<double>(-2.0, 2.0), vec2<double>(2.0, 2.0), vec2<double>(2.0, -2.0) }
        }};
        const auto mesh = Triangulate_Ear_Clipping_2<double, uint32_t>(polygons);
        REQUIRE(mesh.faces.size() == 16);
        require_all_faces_are_triangles(mesh);
        require_edges_are_manifold(mesh);
        require_triangle_centroids_within_arrangement(mesh, polygons);
        REQUIRE(mesh_area(mesh) == doctest::Approx(192.0));
    }

    SUBCASE("very small offset polygon remains triangulable"){
        std::vector<std::vector<vec2<double>>> polygons(1);
        constexpr size_t N = 20;
        constexpr double radius = 1.0e-9;
        constexpr double x_offset = 123.456;
        constexpr double y_offset = -987.654;
        const auto pi = std::acos(-1.0);
        for(size_t i = 0; i < N; ++i){
            const auto angle = (2.0 * pi * static_cast<double>(i)) / static_cast<double>(N);
            polygons.front().emplace_back(x_offset + radius * std::cos(angle),
                                          y_offset + radius * std::sin(angle));
        }
        const auto mesh = Triangulate_Ear_Clipping_2<double, uint32_t>(polygons);
        REQUIRE(mesh.faces.size() == (N - 2));
        require_all_faces_are_triangles(mesh);
        require_edges_are_manifold(mesh);
        require_triangle_centroids_within_arrangement(mesh, polygons);
    }

    SUBCASE("self-intersecting polygon is rejected"){
        const std::vector<std::vector<vec2<double>>> polygons{{
            { vec2<double>(0.0, 0.0), vec2<double>(2.0, 2.0), vec2<double>(0.0, 2.0), vec2<double>(2.0, 0.0) }
        }};
        REQUIRE_THROWS(Triangulate_Ear_Clipping_2<double, uint32_t>(polygons));
    }

    SUBCASE("touching polygons are rejected"){
        const std::vector<std::vector<vec2<double>>> polygons{{
            { vec2<double>(0.0, 0.0), vec2<double>(5.0, 0.0), vec2<double>(5.0, 5.0), vec2<double>(0.0, 5.0) },
            { vec2<double>(5.0, 1.0), vec2<double>(7.0, 1.0), vec2<double>(7.0, 3.0), vec2<double>(5.0, 3.0) }
        }};
        REQUIRE_THROWS(Triangulate_Ear_Clipping_2<double, uint32_t>(polygons));
    }

    SUBCASE("float coordinates and uint64_t indices are supported"){
        const std::vector<std::vector<vec2<float>>> polygons{{
            { vec2<float>(0.0f, 0.0f), vec2<float>(2.0f, 0.0f), vec2<float>(0.0f, 2.0f) }
        }};
        const auto mesh = Triangulate_Ear_Clipping_2<float, uint64_t>(polygons);
        REQUIRE(mesh.vertices.size() == 3);
        REQUIRE(mesh.faces.size() == 1);
        require_all_faces_are_triangles(mesh);
    }
}
