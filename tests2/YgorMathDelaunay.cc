
#include <limits>
#include <utility>
#include <iostream>
#include <random>
#include <cstdint>
#include <algorithm>

#include <YgorMath.h>
#include <YgorMathArbPrec.h>
#include <YgorMathDelaunay.h>

#include "doctest/doctest.h"

struct ArbPrecPrecisionGuard {
    explicit ArbPrecPrecisionGuard(size_t bits)
        : previous_bits(ArbPrec::default_precision_bits()) {
        ArbPrec::set_default_precision_bits(bits);
    }

    ~ArbPrecPrecisionGuard(){
        ArbPrec::set_default_precision_bits(previous_bits);
    }

    size_t previous_bits;
};


TEST_CASE( "Delaunay_Triangulation_2 function" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("fewer than 3 vertices is rejected"){
        // 0 vertices
        std::vector<vec2<double>> verts0;
        REQUIRE_THROWS( Delaunay_Triangulation_2<double, uint32_t>(verts0) );

        // 1 vertex
        std::vector<vec2<double>> verts1 {{ vec2<double>(0.0, 0.0) }};
        REQUIRE_THROWS( Delaunay_Triangulation_2<double, uint32_t>(verts1) );

        // 2 vertices
        std::vector<vec2<double>> verts2 {{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0) }};
        REQUIRE_THROWS( Delaunay_Triangulation_2<double, uint32_t>(verts2) );
    }

    SUBCASE("exactly 3 vertices produces exactly 1 triangle"){
        std::vector<vec2<double>> verts {{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(0.5, 1.0) }};
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        REQUIRE( mesh.vertices.size() == 3 );
        REQUIRE( mesh.faces.size() == 1 );
        REQUIRE( mesh.faces[0].size() == 3 );
    }

    SUBCASE("4 vertices forming a square produces 2 triangles"){
        std::vector<vec2<double>> verts {{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(1.0, 1.0),
            vec2<double>(0.0, 1.0) }};
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        REQUIRE( mesh.vertices.size() == 4 );
        REQUIRE( mesh.faces.size() == 2 );
        for(const auto &f : mesh.faces){
            REQUIRE( f.size() == 3 );
        }
    }

    SUBCASE("5 vertices with one in center produces 4 triangles"){
        // Square with center point.
        std::vector<vec2<double>> verts {{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(1.0, 1.0),
            vec2<double>(0.0, 1.0),
            vec2<double>(0.5, 0.5) }};  // center
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        REQUIRE( mesh.vertices.size() == 5 );
        REQUIRE( mesh.faces.size() == 4 );
        for(const auto &f : mesh.faces){
            REQUIRE( f.size() == 3 );
        }
    }

    SUBCASE("regular grid produces valid triangulation"){
        // A 3x3 grid of points = 9 vertices.
        std::vector<vec2<double>> verts;
        for(int i = 0; i < 3; ++i){
            for(int j = 0; j < 3; ++j){
                verts.emplace_back(vec2<double>(static_cast<double>(i), static_cast<double>(j)));
            }
        }
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        REQUIRE( mesh.vertices.size() == 9 );
        // For a 3x3 grid (2x2 cells), we expect 2*2*2 = 8 triangles.
        REQUIRE( mesh.faces.size() == 8 );
        for(const auto &f : mesh.faces){
            REQUIRE( f.size() == 3 );
        }
    }

    SUBCASE("all triangles have valid indices"){
        std::vector<vec2<double>> verts {{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(1.0, 1.0),
            vec2<double>(0.0, 1.0),
            vec2<double>(0.5, 0.5) }};
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        for(const auto &f : mesh.faces){
            REQUIRE( f.size() == 3 );
            for(const auto &idx : f){
                REQUIRE( idx < mesh.vertices.size() );
            }
        }
    }

    SUBCASE("triangles have non-zero area"){
        std::vector<vec2<double>> verts {{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(1.0, 1.0),
            vec2<double>(0.0, 1.0),
            vec2<double>(0.5, 0.5) }};
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        for(const auto &f : mesh.faces){
            const auto &A = mesh.vertices[f[0]];
            const auto &B = mesh.vertices[f[1]];
            const auto &C = mesh.vertices[f[2]];
            // Compute triangle area using cross product.
            const auto cross = (B - A).Cross(C - A);
            const auto area = cross.length() / 2.0;
            REQUIRE( area > eps );
        }
    }

    SUBCASE("Delaunay property is satisfied (empty circumcircle)"){
        // The Delaunay property: no vertex is strictly inside the circumcircle of any triangle.
        std::vector<vec2<double>> verts {{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(1.0, 1.0),
            vec2<double>(0.0, 1.0),
            vec2<double>(0.5, 0.5) }};
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);

        // Use the same tolerance strategy as the implementation.
        const double machine_eps = std::sqrt( std::numeric_limits<double>::epsilon() );

        // Helper to compute circumcircle.
        auto compute_circumcircle = [&machine_eps](const vec3<double> &A, const vec3<double> &B, const vec3<double> &C)
            -> std::tuple<double, double, double> {
            const double D = 2.0 * (A.x * (B.y - C.y) + B.x * (C.y - A.y) + C.x * (A.y - B.y));
            const double max_coord = std::max({std::abs(A.x), std::abs(B.x), std::abs(C.x),
                                               std::abs(A.y), std::abs(B.y), std::abs(C.y), 1.0});
            const double det_eps = machine_eps * max_coord * max_coord;
            if(std::abs(D) < det_eps){
                return std::make_tuple(std::numeric_limits<double>::quiet_NaN(),
                                       std::numeric_limits<double>::quiet_NaN(),
                                       std::numeric_limits<double>::quiet_NaN());
            }
            const double A_sq = A.x * A.x + A.y * A.y;
            const double B_sq = B.x * B.x + B.y * B.y;
            const double C_sq = C.x * C.x + C.y * C.y;
            const double cx = (A_sq * (B.y - C.y) + B_sq * (C.y - A.y) + C_sq * (A.y - B.y)) / D;
            const double cy = (A_sq * (C.x - B.x) + B_sq * (A.x - C.x) + C_sq * (B.x - A.x)) / D;
            const double r_sq = (A.x - cx) * (A.x - cx) + (A.y - cy) * (A.y - cy);
            return std::make_tuple(cx, cy, r_sq);
        };

        for(const auto &f : mesh.faces){
            const auto &A = mesh.vertices[f[0]];
            const auto &B = mesh.vertices[f[1]];
            const auto &C = mesh.vertices[f[2]];
            const auto [cx, cy, r_sq] = compute_circumcircle(A, B, C);
            if(!std::isfinite(cx) || !std::isfinite(cy) || !std::isfinite(r_sq)){
                continue;
            }

            // Check that no other vertex is strictly inside this circumcircle.
            // Using consistent tolerance with the implementation.
            const double r_tol = r_sq * machine_eps;
            for(size_t i = 0; i < mesh.vertices.size(); ++i){
                // Skip the triangle's own vertices.
                if(i == f[0] || i == f[1] || i == f[2]) continue;

                const auto &P = mesh.vertices[i];
                const double dist_sq = (P.x - cx) * (P.x - cx) + (P.y - cy) * (P.y - cy);
                // Points should be outside or on the circle (not strictly inside).
                // This mirrors the implementation's check: dist_sq < (r_sq - r_tol) means "inside".
                // So we require the opposite: dist_sq >= (r_sq - r_tol).
                REQUIRE( dist_sq >= (r_sq - r_tol) );
            }
        }
    }

    SUBCASE("random vertices produce valid triangulation"){
        std::mt19937 re(12345);
        std::uniform_real_distribution<double> rd(0.0, 10.0);

        std::vector<vec2<double>> verts;
        for(size_t i = 0; i < 20; ++i){
            verts.emplace_back(vec2<double>(rd(re), rd(re)));
        }

        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        REQUIRE( mesh.vertices.size() == 20 );
        REQUIRE( mesh.faces.size() > 0 );

        // All faces should be triangles with valid indices.
        for(const auto &f : mesh.faces){
            REQUIRE( f.size() == 3 );
            for(const auto &idx : f){
                REQUIRE( idx < mesh.vertices.size() );
            }
        }
    }

    SUBCASE("collinear vertices are rejected"){
        // All vertices on a line -- no valid triangulation possible.
        std::vector<vec2<double>> verts {{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(2.0, 0.0),
            vec2<double>(3.0, 0.0) }};
        REQUIRE_THROWS( Delaunay_Triangulation_2<double, uint32_t>(verts) );
    }

    SUBCASE("all vertices at same location are rejected"){
        // All vertices at the same point -- degenerate case, no valid triangulation.
        std::vector<vec2<double>> verts {{
            vec2<double>(1.0, 1.0),
            vec2<double>(1.0, 1.0),
            vec2<double>(1.0, 1.0),
            vec2<double>(1.0, 1.0) }};
        REQUIRE_THROWS( Delaunay_Triangulation_2<double, uint32_t>(verts) );
    }

    SUBCASE("works with float type and uint64_t indices"){
        std::vector<vec2<float>> verts {{
            vec2<float>(0.0f, 0.0f),
            vec2<float>(1.0f, 0.0f),
            vec2<float>(0.5f, 1.0f) }};
        const auto mesh = Delaunay_Triangulation_2<float, uint64_t>(verts);
        REQUIRE( mesh.vertices.size() == 3 );
        REQUIRE( mesh.faces.size() == 1 );
    }

    SUBCASE("arbitrary precision values round-trip through double"){
        ArbPrecPrecisionGuard precision_guard(320);

        const ArbPrec value(0.125);
        CHECK(static_cast<double>(value) == doctest::Approx(0.125));
        CHECK(static_cast<float>(ArbPrec(0.5f, 192)) == doctest::Approx(0.5f));
    }

    SUBCASE("duplicate input vertices do not create degenerate output faces"){
        std::vector<vec2<double>> verts {{
            vec2<double>(0.0, 0.0),
            vec2<double>(1.0, 0.0),
            vec2<double>(1.0, 1.0),
            vec2<double>(0.0, 1.0),
            vec2<double>(0.0, 1.0) }};
        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        for(const auto &f : mesh.faces){
            const auto &A = mesh.vertices[f[0]];
            const auto &B = mesh.vertices[f[1]];
            const auto &C = mesh.vertices[f[2]];
            const auto cross = (B - A).Cross(C - A);
            const auto area = cross.length() / 2.0;
            REQUIRE( area > eps );
        }
    }

    SUBCASE("regular polygon remains triangulable even when very small and offset from the origin"){
        std::vector<vec2<double>> verts;
        constexpr size_t N = 20;
        constexpr double radius = 1.0e-9;
        constexpr double x_offset = 123.456;
        constexpr double y_offset = -987.654;
        const auto pi = std::acos(-1.0);
        for(size_t i = 0; i < N; ++i){
            const auto angle = (2.0 * pi * static_cast<double>(i)) / static_cast<double>(N);
            verts.emplace_back(x_offset + radius * std::cos(angle),
                               y_offset + radius * std::sin(angle));
        }

        const auto mesh = Delaunay_Triangulation_2<double, uint32_t>(verts);
        REQUIRE(mesh.vertices.size() == N);
        REQUIRE(mesh.faces.size() == N - 2);
        for(const auto &face : mesh.faces){
            REQUIRE(face.size() == 3);
        }
    }

    SUBCASE("arbitrary precision regular polygon remains triangulable when very small and offset from the origin"){
        ArbPrecPrecisionGuard precision_guard(384);

        std::vector<vec2<ArbPrec>> verts;
        constexpr size_t N = 20;
        constexpr double radius = 1.0e-12;
        constexpr double x_offset = 123.456;
        constexpr double y_offset = -987.654;
        const auto pi = std::acos(-1.0);
        for(size_t i = 0; i < N; ++i){
            const auto angle = (2.0 * pi * static_cast<double>(i)) / static_cast<double>(N);
            verts.emplace_back(ArbPrec(x_offset + radius * std::cos(angle), 384),
                               ArbPrec(y_offset + radius * std::sin(angle), 384));
        }

        const auto mesh = Delaunay_Triangulation_2<ArbPrec, uint32_t>(verts);
        REQUIRE(mesh.vertices.size() == N);
        REQUIRE(mesh.faces.size() == N - 2);
        for(const auto &face : mesh.faces){
            REQUIRE(face.size() == 3);
        }
    }
}
