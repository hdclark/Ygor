
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

template <class T, class I>
std::vector<vec2<T>> materialize_piece(const std::vector<std::vector<vec2<T>>> &verts,
                                       const monotone_t<I> &piece){
    std::vector<vec2<T>> out;
    out.reserve(piece.vertices.size());
    for(const auto &[poly_idx, vert_idx] : piece.vertices){
        out.push_back(verts.at(static_cast<size_t>(poly_idx)).at(static_cast<size_t>(vert_idx)));
    }
    return out;
}

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
        const auto poly = materialize_piece(verts, piece);
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
        const auto poly = materialize_piece(verts, piece);
        if(point_in_polygon_or_on_boundary(poly, p)){
            score += piece.interior ? 1 : -1;
        }
    }
    return score;
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
        REQUIRE(pieces.size() >= 3);
        REQUIRE(std::any_of(pieces.begin(), pieces.end(), [](const auto &piece){ return piece.interior; }));
        REQUIRE(std::any_of(pieces.begin(), pieces.end(), [](const auto &piece){ return !piece.interior; }));
        require_valid_monotone_output(verts, pieces);
        REQUIRE(monotone_score(verts, pieces, vec2<double>(1.0, 1.0)) > 0);
        REQUIRE(monotone_score(verts, pieces, vec2<double>(2.5, 2.5)) == 0);
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
        REQUIRE(monotone_score(verts, pieces, vec2<double>(4.0, 2.0)) == 0);
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
