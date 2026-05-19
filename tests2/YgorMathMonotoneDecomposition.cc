
#include <algorithm>
#include <array>
#include <limits>
#include <set>
#include <utility>
#include <vector>

#include <YgorMath.h>
#include <YgorMathMonotoneDecomposition.h>

#include "doctest/doctest.h"

namespace {

template <class T>
long double polygon_area(const std::vector<vec2<T>> &poly){
    long double area = 0.0L;
    for(size_t i = 0; i < poly.size(); ++i){
        const auto &a = poly.at(i);
        const auto &b = poly.at((i + 1) % poly.size());
        area += (static_cast<long double>(a.x) * static_cast<long double>(b.y))
              - (static_cast<long double>(b.x) * static_cast<long double>(a.y));
    }
    return area / 2.0L;
}

template <class T>
bool event_higher(const vec2<T> &a, const vec2<T> &b){
    return (a.y > b.y) || ((a.y == b.y) && (a.x < b.x));
}

template <class T>
bool is_y_monotone(const std::vector<vec2<T>> &poly){
    if(poly.size() < 3){
        return false;
    }

    size_t top = 0;
    size_t bottom = 0;
    for(size_t i = 1; i < poly.size(); ++i){
        if(event_higher(poly.at(i), poly.at(top))){
            top = i;
        }
        if(event_higher(poly.at(bottom), poly.at(i))){
            bottom = i;
        }
    }

    auto chain_ok = [&](size_t step) -> bool {
        size_t cur = top;
        while(cur != bottom){
            const auto next = (cur + step) % poly.size();
            if(event_higher(poly.at(next), poly.at(cur))){
                return false;
            }
            cur = next;
        }
        return true;
    };

    return chain_ok(1) && chain_ok(poly.size() - 1);
}

template <class T, class I>
void require_valid_monotone_output(const std::vector<std::vector<vec2<T>>> &verts,
                                   const std::vector<monotone_t<I>> &pieces){
    REQUIRE(!pieces.empty());
    for(const auto &piece : pieces){
        REQUIRE(piece.vertices.size() >= 3);
        for(const auto &[poly_idx, vert_idx] : piece.vertices){
            REQUIRE(static_cast<size_t>(poly_idx) < verts.size());
            REQUIRE(static_cast<size_t>(vert_idx) < verts.at(static_cast<size_t>(poly_idx)).size());
        }
        const auto poly = Materialize_Polygon(verts, piece);
        REQUIRE(polygon_area(poly) > 0.0L);
        REQUIRE(is_y_monotone(poly));
    }
}

template <class T, class I>
int monotone_score(const std::vector<std::vector<vec2<T>>> &verts,
                   const std::vector<monotone_t<I>> &pieces,
                   const vec2<T> &p){
    int score = 0;
    for(const auto &piece : pieces){
        const auto poly = Materialize_Polygon(verts, piece);
        if(point_in_polygon_or_on_boundary(poly, p)){
            score += piece.interior ? 1 : -1;
        }
    }
    return score;
}

template <class T, class I>
long double monotone_area_sum(const std::vector<std::vector<vec2<T>>> &verts,
                              const std::vector<monotone_t<I>> &pieces,
                              bool include_exterior){
    long double area = 0.0L;
    for(const auto &piece : pieces){
        if(piece.interior || include_exterior){
            area += polygon_area(Materialize_Polygon(verts, piece));
        }
    }
    return area;
}

template <class T, class I>
long double mesh_projected_area(const fv_surface_mesh<T, I> &mesh){
    long double area = 0.0L;
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3);
        const auto &a = mesh.vertices.at(face.at(0));
        const auto &b = mesh.vertices.at(face.at(1));
        const auto &c = mesh.vertices.at(face.at(2));
        area += std::abs(((static_cast<long double>(a.x) * static_cast<long double>(b.y))
                        - (static_cast<long double>(b.x) * static_cast<long double>(a.y))
                        + (static_cast<long double>(b.x) * static_cast<long double>(c.y))
                        - (static_cast<long double>(c.x) * static_cast<long double>(b.y))
                        + (static_cast<long double>(c.x) * static_cast<long double>(a.y))
                        - (static_cast<long double>(a.x) * static_cast<long double>(c.y))) / 2.0L);
    }
    return area;
}

template <class T, class I>
void require_valid_triangulation_mesh(const fv_surface_mesh<T, I> &mesh){
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3);
        for(const auto idx : face){
            REQUIRE(idx < mesh.vertices.size());
            REQUIRE(mesh.vertices.at(idx).z == static_cast<T>(0));
        }

        const vec2<T> a(mesh.vertices.at(face.at(0)).x, mesh.vertices.at(face.at(0)).y);
        const vec2<T> b(mesh.vertices.at(face.at(1)).x, mesh.vertices.at(face.at(1)).y);
        const vec2<T> c(mesh.vertices.at(face.at(2)).x, mesh.vertices.at(face.at(2)).y);
        REQUIRE(orient_sign(a, b, c) > 0);
    }
}

} // namespace

TEST_CASE("Monotone_Decomposition_2 function"){
    SUBCASE("empty input returns no monotone pieces"){
        const std::vector<std::vector<vec2<double>>> verts;
        const auto pieces = Monotone_Decomposition_2<double, uint32_t>(verts);
        REQUIRE(pieces.empty());
    }

    SUBCASE("convex polygon stays intact"){
        const std::vector<std::vector<vec2<double>>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(2.0, 0.0),
            vec2<double>(3.0, 1.0),
            vec2<double>(1.0, 3.0),
            vec2<double>(-1.0, 1.0)
        }};

        const auto pieces = Monotone_Decomposition_2<double, uint32_t>(verts);
        REQUIRE(pieces.size() == 1);
        REQUIRE(pieces.front().interior);
        require_valid_monotone_output(verts, pieces);
    }

    SUBCASE("non-monotone simple polygon is split into multiple monotone pieces"){
        const std::vector<std::vector<vec2<double>>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(4.0, 0.0),
            vec2<double>(4.0, 4.0),
            vec2<double>(2.0, 2.0),
            vec2<double>(0.0, 4.0)
        }};

        const auto pieces = Monotone_Decomposition_2<double, uint32_t>(verts);
        REQUIRE(pieces.size() == 2);
        for(const auto &piece : pieces){
            REQUIRE(piece.interior);
        }
        require_valid_monotone_output(verts, pieces);
    }

    SUBCASE("closing duplicate vertices and clockwise winding are normalized"){
        const std::vector<std::vector<vec2<double>>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(0.0, 4.0),
            vec2<double>(2.0, 2.0),
            vec2<double>(4.0, 4.0),
            vec2<double>(4.0, 0.0),
            vec2<double>(0.0, 0.0)
        }};

        const auto pieces = Monotone_Decomposition_2<double, uint64_t>(verts);
        REQUIRE(pieces.size() == 2);
        require_valid_monotone_output(verts, pieces);
    }

    SUBCASE("nested polygons alternate interior parity"){
        const std::vector<std::vector<vec2<double>>> verts{
            {
                vec2<double>(0.0, 0.0),
                vec2<double>(8.0, 0.0),
                vec2<double>(8.0, 8.0),
                vec2<double>(0.0, 8.0)
            },
            {
                vec2<double>(2.0, 2.0),
                vec2<double>(2.0, 6.0),
                vec2<double>(6.0, 6.0),
                vec2<double>(6.0, 2.0)
            },
            {
                vec2<double>(3.0, 3.0),
                vec2<double>(5.0, 3.0),
                vec2<double>(4.0, 5.0)
            }
        };

        const auto pieces = Monotone_Decomposition_2<double, uint32_t>(verts);
        // Bridging holes and islands can legitimately change the exact monotone count, so this case checks semantic
        // coverage/parity instead of pinning the partition to a single triangulation-equivalent count.
        REQUIRE(pieces.size() >= 3);
        REQUIRE(std::any_of(pieces.begin(), pieces.end(), [](const auto &piece){ return piece.interior; }));
        REQUIRE(std::any_of(pieces.begin(), pieces.end(), [](const auto &piece){ return !piece.interior; }));
        require_valid_monotone_output(verts, pieces);
        REQUIRE(monotone_score(verts, pieces, vec2<double>(1.0, 1.0)) > 0);
        REQUIRE(monotone_score(verts, pieces, vec2<double>(2.5, 2.5)) < 0);
        REQUIRE(monotone_score(verts, pieces, vec2<double>(4.0, 3.75)) > 0);
        REQUIRE(monotone_score(verts, pieces, vec2<double>(9.0, 9.0)) == 0);
    }

    SUBCASE("nested polygons result in joined, not overlapping monotones"){
        const std::vector<std::vector<vec2<double>>> verts{
            {
                // Exterior.
                vec2<double>(0.0, 0.0),
                vec2<double>(8.0, 0.0),
                vec2<double>(4.0, 8.0)
            },
            {
                // Interior hole.
                vec2<double>(1.0, 1.0),
                vec2<double>(7.0, 1.0),
                vec2<double>(4.0, 7.0)
            }
        };

        const auto pieces = Monotone_Decomposition_2<double, uint32_t>(verts);
        REQUIRE(pieces.size() > 2);
        require_valid_monotone_output(verts, pieces);
        REQUIRE(monotone_score(verts, pieces, vec2<double>(4.0, 0.5)) > 0);
        REQUIRE(monotone_score(verts, pieces, vec2<double>(4.0, 2.0)) < 0);
    }

    SUBCASE("touching nested polygons are rejected"){
        const std::vector<std::vector<vec2<double>>> verts{
            {
                vec2<double>(0.0, 0.0),
                vec2<double>(4.0, 0.0),
                vec2<double>(4.0, 4.0),
                vec2<double>(0.0, 4.0)
            },
            {
                vec2<double>(1.0, 1.0),
                vec2<double>(4.0, 1.0),
                vec2<double>(4.0, 2.0),
                vec2<double>(1.0, 2.0)
            }
        };

        REQUIRE_THROWS(Monotone_Decomposition_2<double, uint32_t>(verts));
    }

    SUBCASE("self-intersecting polygons are rejected"){
        const std::vector<std::vector<vec2<double>>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(4.0, 4.0),
            vec2<double>(0.0, 4.0),
            vec2<double>(4.0, 0.0)
        }};

        REQUIRE_THROWS(Monotone_Decomposition_2<double, uint32_t>(verts));
    }
}

TEST_CASE("Triangulate_Monotone_Decomposition function"){
    SUBCASE("convex monotone polygon triangulates into n-2 positive triangles"){
        const std::vector<std::vector<vec2<double>>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(2.0, 0.0),
            vec2<double>(3.0, 1.0),
            vec2<double>(1.0, 3.0),
            vec2<double>(-1.0, 1.0)
        }};

        const auto pieces = Monotone_Decomposition_2<double, uint32_t>(verts);
        const auto mesh = Triangulate_Monotone_Decomposition<double, uint32_t>(verts, pieces, false);
        REQUIRE(mesh.vertices.size() == 5);
        REQUIRE(mesh.faces.size() == 3);
        require_valid_triangulation_mesh(mesh);
        REQUIRE(mesh_projected_area(mesh) == doctest::Approx(monotone_area_sum(verts, pieces, false)));
    }

    SUBCASE("non-monotone polygon triangulates through its monotone partition"){
        const std::vector<std::vector<vec2<double>>> verts{{
            vec2<double>(0.0, 0.0),
            vec2<double>(4.0, 0.0),
            vec2<double>(4.0, 4.0),
            vec2<double>(2.0, 2.0),
            vec2<double>(0.0, 4.0)
        }};

        const auto pieces = Monotone_Decomposition_2<double, uint32_t>(verts);
        const auto mesh = Triangulate_Monotone_Decomposition<double, uint32_t>(verts, pieces, false);
        REQUIRE(mesh.vertices.size() == 5);
        REQUIRE(mesh.faces.size() == 2);
        require_valid_triangulation_mesh(mesh);
        REQUIRE(mesh_projected_area(mesh) == doctest::Approx(monotone_area_sum(verts, pieces, false)));
    }

    SUBCASE("selector excludes and includes exterior monotones for nested polygons"){
        const std::vector<std::vector<vec2<double>>> verts{
            {
                vec2<double>(0.0, 0.0),
                vec2<double>(8.0, 0.0),
                vec2<double>(8.0, 8.0),
                vec2<double>(0.0, 8.0)
            },
            {
                vec2<double>(2.0, 2.0),
                vec2<double>(2.0, 6.0),
                vec2<double>(6.0, 6.0),
                vec2<double>(6.0, 2.0)
            },
            {
                vec2<double>(3.0, 3.0),
                vec2<double>(5.0, 3.0),
                vec2<double>(4.0, 5.0)
            }
        };

        const auto pieces = Monotone_Decomposition_2<double, uint32_t>(verts);
        const auto mesh_interior = Triangulate_Monotone_Decomposition<double, uint32_t>(verts, pieces, false);
        const auto mesh_all = Triangulate_Monotone_Decomposition<double, uint32_t>(verts, pieces, true);

        REQUIRE(mesh_interior.vertices.size() == 11);
        REQUIRE(mesh_all.vertices.size() == 11);
        REQUIRE(mesh_all.faces.size() > mesh_interior.faces.size());
        require_valid_triangulation_mesh(mesh_interior);
        require_valid_triangulation_mesh(mesh_all);
        REQUIRE(mesh_projected_area(mesh_interior) == doctest::Approx(monotone_area_sum(verts, pieces, false)));
        REQUIRE(mesh_projected_area(mesh_all) == doctest::Approx(monotone_area_sum(verts, pieces, true)));
        REQUIRE(mesh_projected_area(mesh_all) > mesh_projected_area(mesh_interior));
    }

    SUBCASE("zero-area bridge preprocessing does not create degenerate output triangles"){
        const std::vector<std::vector<vec2<double>>> verts{
            {
                vec2<double>(0.0, 0.0),
                vec2<double>(8.0, 0.0),
                vec2<double>(4.0, 8.0)
            },
            {
                vec2<double>(1.0, 1.0),
                vec2<double>(7.0, 1.0),
                vec2<double>(4.0, 7.0)
            }
        };

        const auto pieces = Monotone_Decomposition_2<double, uint32_t>(verts);
        const auto mesh = Triangulate_Monotone_Decomposition<double, uint32_t>(verts, pieces, false);
        REQUIRE(mesh.vertices.size() == 6);
        REQUIRE(!mesh.faces.empty());
        require_valid_triangulation_mesh(mesh);
        REQUIRE(mesh_projected_area(mesh) == doctest::Approx(monotone_area_sum(verts, pieces, false)));
    }
}
