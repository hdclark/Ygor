
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <vector>

#include <YgorMath.h>
#include <YgorMathDelaunayViaVoronoi.h>

#include "doctest/doctest.h"

namespace {

template <class T, class I>
void require_mesh_is_triangular(const fv_surface_mesh<T, I> &mesh){
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3);
        for(const auto idx : face){
            REQUIRE(idx < mesh.vertices.size());
        }
        const auto &a = mesh.vertices.at(face.at(0));
        const auto &b = mesh.vertices.at(face.at(1));
        const auto &c = mesh.vertices.at(face.at(2));
        REQUIRE(orient_sign(vec2<T>(a.x, a.y), vec2<T>(b.x, b.y), vec2<T>(c.x, c.y)) != 0);
    }
}

template <class T, class I>
void require_empty_circumcircles(const fv_surface_mesh<T, I> &mesh){
    for(const auto &face : mesh.faces){
        const auto a = vec2<T>(mesh.vertices.at(face.at(0)).x, mesh.vertices.at(face.at(0)).y);
        const auto b = vec2<T>(mesh.vertices.at(face.at(1)).x, mesh.vertices.at(face.at(1)).y);
        const auto c = vec2<T>(mesh.vertices.at(face.at(2)).x, mesh.vertices.at(face.at(2)).y);
        for(size_t i = 0; i < mesh.vertices.size(); ++i){
            if((i == face.at(0)) || (i == face.at(1)) || (i == face.at(2))){
                continue;
            }
            const auto p = vec2<T>(mesh.vertices.at(i).x, mesh.vertices.at(i).y);
            REQUIRE(incircle_sign(a, b, c, p) <= 0);
        }
    }
}

size_t convex_hull_size(const std::vector<vec2<double>> &verts){
    std::vector<vec2<double>> pts = verts;
    std::sort(pts.begin(), pts.end(), [](const auto &a, const auto &b){
        if(a.x != b.x){
            return a.x < b.x;
        }
        return a.y < b.y;
    });

    std::vector<vec2<double>> hull;
    hull.reserve(pts.size() * 2);
    for(const auto &p : pts){
        while((hull.size() >= 2)
           && (orient_sign(hull.at(hull.size() - 2), hull.at(hull.size() - 1), p) <= 0)){
            hull.pop_back();
        }
        hull.push_back(p);
    }
    const auto lower_size = hull.size();
    for(auto it = pts.rbegin(); it != pts.rend(); ++it){
        while((hull.size() > lower_size)
           && (orient_sign(hull.at(hull.size() - 2), hull.at(hull.size() - 1), *it) <= 0)){
            hull.pop_back();
        }
        hull.push_back(*it);
    }
    hull.pop_back();
    return hull.size();
}

} // namespace

TEST_CASE("Delaunay_Triangulation_2_via_Voronoi function"){
    SUBCASE("fewer than 3 vertices is rejected"){
        REQUIRE_THROWS(Delaunay_Triangulation_2_via_Voronoi<double, uint32_t>({}));
        REQUIRE_THROWS(Delaunay_Triangulation_2_via_Voronoi<double, uint32_t>({ vec2<double>(0.0, 0.0) }));
        REQUIRE_THROWS(Delaunay_Triangulation_2_via_Voronoi<double, uint32_t>({
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0) }));
    }

    SUBCASE("non-finite coordinates are rejected"){
        const auto nan_val = std::numeric_limits<double>::quiet_NaN();
        const auto inf_val = std::numeric_limits<double>::infinity();
        REQUIRE_THROWS(Delaunay_Triangulation_2_via_Voronoi<double, uint32_t>({
            vec2<double>(nan_val, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(0.0, 1.0) }));
        REQUIRE_THROWS(Delaunay_Triangulation_2_via_Voronoi<double, uint32_t>({
            vec2<double>(0.0, 0.0),
            vec2<double>(inf_val, 0.0),
            vec2<double>(0.0, 1.0) }));
        REQUIRE_THROWS(Delaunay_Triangulation_2_via_Voronoi<double, uint32_t>({
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(0.0, -inf_val) }));
    }

    SUBCASE("simple triangle produces one face"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(0.25, 1.0) }};
        const auto mesh = Delaunay_Triangulation_2_via_Voronoi<double, uint32_t>(verts);
        REQUIRE(mesh.vertices.size() == 3);
        REQUIRE(mesh.faces.size() == 1);
        require_mesh_is_triangular(mesh);
    }

    SUBCASE("square produces two faces"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(1.0, 1.0),
            vec2<double>(0.0, 1.0) }};
        const auto mesh = Delaunay_Triangulation_2_via_Voronoi<double, uint32_t>(verts);
        REQUIRE(mesh.vertices.size() == 4);
        REQUIRE(mesh.faces.size() == 2);
        require_mesh_is_triangular(mesh);
        require_empty_circumcircles(mesh);
    }

    SUBCASE("duplicate inputs remain harmless"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(1.0, 1.0),
            vec2<double>(0.0, 1.0),
            vec2<double>(0.0, 1.0) }};
        const auto mesh = Delaunay_Triangulation_2_via_Voronoi<double, uint32_t>(verts);
        REQUIRE(mesh.vertices.size() == verts.size());
        REQUIRE(mesh.faces.size() == 2);
        require_mesh_is_triangular(mesh);
    }

    SUBCASE("collinear vertices are rejected"){
        const std::vector<vec2<double>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(2.0, 0.0),
            vec2<double>(3.0, 0.0) }};
        REQUIRE_THROWS(Delaunay_Triangulation_2_via_Voronoi<double, uint32_t>(verts));
    }

    SUBCASE("regular cocircular polygons are triangulated deterministically"){
        const auto pi = std::acos(-1.0);
        for(size_t n = 4; n <= 16; ++n){
            std::vector<vec2<double>> verts;
            verts.reserve(n);
            for(size_t i = 0; i < n; ++i){
                const auto angle = (2.0 * pi * static_cast<double>(i)) / static_cast<double>(n);
                verts.emplace_back(std::cos(angle), std::sin(angle));
            }
            const auto mesh = Delaunay_Triangulation_2_via_Voronoi<double, uint32_t>(verts);
            REQUIRE(mesh.vertices.size() == n);
            REQUIRE(mesh.faces.size() == n - 2);
            require_mesh_is_triangular(mesh);
        }
    }

    SUBCASE("non-degenerate random inputs satisfy the Delaunay triangulation count"){
        std::mt19937 rng(20260517);
        std::uniform_real_distribution<double> dist(-10.0, 10.0);
        std::vector<vec2<double>> verts;
        verts.reserve(24);
        while(verts.size() < 24){
            const vec2<double> candidate(dist(rng) + static_cast<double>(verts.size()) * 1.0e-4,
                                          dist(rng) - static_cast<double>(verts.size()) * 2.0e-4);
            bool duplicate = false;
            for(const auto &existing : verts){
                if((existing.x == candidate.x) && (existing.y == candidate.y)){
                    duplicate = true;
                    break;
                }
            }
            if(!duplicate){
                verts.push_back(candidate);
            }
        }

        const auto via_voronoi = Delaunay_Triangulation_2_via_Voronoi<double, uint32_t>(verts);
        require_mesh_is_triangular(via_voronoi);
        require_empty_circumcircles(via_voronoi);
        REQUIRE(via_voronoi.faces.size() == ((2 * verts.size()) - 2 - convex_hull_size(verts)));
    }

    SUBCASE("small offset geometry remains triangulable"){
        std::vector<vec2<double>> verts;
        constexpr size_t n = 20;
        constexpr double radius = 1.0e-9;
        constexpr double x_offset = 123.456;
        constexpr double y_offset = -987.654;
        const auto pi = std::acos(-1.0);
        for(size_t i = 0; i < n; ++i){
            const auto angle = (2.0 * pi * static_cast<double>(i)) / static_cast<double>(n);
            verts.emplace_back(x_offset + radius * std::cos(angle),
                               y_offset + radius * std::sin(angle));
        }
        const auto mesh = Delaunay_Triangulation_2_via_Voronoi<double, uint32_t>(verts);
        REQUIRE(mesh.vertices.size() == n);
        REQUIRE(mesh.faces.size() == n - 2);
        require_mesh_is_triangular(mesh);
    }
}
