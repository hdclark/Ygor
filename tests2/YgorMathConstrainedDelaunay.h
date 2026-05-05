#pragma once

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "../src/YgorMathConstrainedDelaunay.h"
#include <YgorMath.h>
#include <YgorMeshesConvexHull.h>

#include "doctest/doctest.h"

namespace ygor_test_constrained_delaunay {

template <class I>
using edge_type = std::pair<I, I>;

template <class I>
edge_type<I> make_edge(I a, I b) {
    if(b < a){
        std::swap(a, b);
    }
    return std::make_pair(a, b);
}

template <class T>
T orient2d(const vec3<T> &a, const vec3<T> &b, const vec3<T> &c) {
    const std::array<T, 3> pa{{ a.x, a.y, static_cast<T>(0) }};
    const std::array<T, 3> pb{{ b.x, b.y, static_cast<T>(0) }};
    const std::array<T, 3> pc{{ c.x, c.y, static_cast<T>(0) }};
    const std::array<T, 3> pd{{ static_cast<T>(0), static_cast<T>(0), static_cast<T>(1) }};
    return -adaptive_predicate::orient3d(pa.data(), pb.data(), pc.data(), pd.data());
}

template <class T>
T incircle2d(const vec3<T> &a, const vec3<T> &b, const vec3<T> &c, const vec3<T> &d) {
    const std::array<T, 3> pa{{ a.x, a.y, a.x * a.x + a.y * a.y }};
    const std::array<T, 3> pb{{ b.x, b.y, b.x * b.x + b.y * b.y }};
    const std::array<T, 3> pc{{ c.x, c.y, c.x * c.x + c.y * c.y }};
    const std::array<T, 3> pd{{ d.x, d.y, d.x * d.x + d.y * d.y }};

    auto det = adaptive_predicate::orient3d(pa.data(), pb.data(), pc.data(), pd.data());
    if(orient2d(a, b, c) < static_cast<T>(0)){
        det = -det;
    }
    return det;
}

template <class T, class I>
void require_triangle_edges_are_listed(const fv_surface_mesh<T, I> &mesh) {
    std::set<edge_type<I>> listed_edges;
    for(const auto &face : mesh.faces){
        if(face.size() == 2){
            listed_edges.insert(make_edge(face.at(0), face.at(1)));
        }
    }

    for(const auto &face : mesh.faces){
        if(face.size() != 3){
            continue;
        }
        REQUIRE(listed_edges.count(make_edge(face.at(0), face.at(1))) == 1);
        REQUIRE(listed_edges.count(make_edge(face.at(1), face.at(2))) == 1);
        REQUIRE(listed_edges.count(make_edge(face.at(2), face.at(0))) == 1);
    }
}

template <class T, class I>
void require_non_constraint_edges_are_locally_delaunay(const fv_surface_mesh<T, I> &mesh,
                                                       const std::vector<edge_type<I>> &constraints) {
    std::set<edge_type<I>> constraint_edges(constraints.begin(), constraints.end());
    std::map<edge_type<I>, std::vector<std::array<I, 3>>> incident;

    for(const auto &face : mesh.faces){
        if(face.size() != 3){
            continue;
        }
        const std::array<I, 3> tri{{ face.at(0), face.at(1), face.at(2) }};
        incident[make_edge(tri[0], tri[1])].push_back(tri);
        incident[make_edge(tri[1], tri[2])].push_back(tri);
        incident[make_edge(tri[2], tri[0])].push_back(tri);
    }

    for(const auto &[edge, tris] : incident){
        if((tris.size() != 2) || (constraint_edges.count(edge) != 0)){
            continue;
        }

        const auto &t0 = tris.at(0);
        const auto &t1 = tris.at(1);
        const auto find_opposite_vertex = [](const std::array<I, 3> &tri, edge_type<I> edge_key) -> I {
            for(const auto idx : tri){
                if((idx != edge_key.first) && (idx != edge_key.second)){
                    return idx;
                }
            }
            return edge_key.first;
        };

        const auto w = find_opposite_vertex(t0, edge);
        const auto x = find_opposite_vertex(t1, edge);
        const auto &a = mesh.vertices.at(edge.first);
        const auto &b = mesh.vertices.at(edge.second);
        const auto &c = mesh.vertices.at(w);
        const auto &d = mesh.vertices.at(x);
        REQUIRE(incircle2d(a, b, c, d) <= static_cast<T>(0));
    }
}

} // namespace ygor_test_constrained_delaunay
