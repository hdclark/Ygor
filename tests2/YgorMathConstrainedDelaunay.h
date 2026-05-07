#pragma once

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include <YgorMath.h>
#include <YgorMathConstrainedDelaunay.h>

#include "doctest/doctest.h"

namespace ygor_test_constrained_delaunay {

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

template <class T>
int orient2d(const vec2<T> &a, const vec2<T> &b, const vec2<T> &c){
    return orient2d_sign(a, b, c);
}

template <class T>
int incircle2d(const vec2<T> &a, const vec2<T> &b, const vec2<T> &c, const vec2<T> &d){
    return incircle2d_sign(a, b, c, d);
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

template <class T, class I>
void require_all_faces_are_triangles(const fv_surface_mesh<T, I> &mesh){
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3);
    }
}

template <class T, class I>
void require_constraints_are_triangle_edges(const fv_surface_mesh<T, I> &mesh,
                                            const std::vector<edge_type<I>> &constraints){
    const auto mesh_edges = collect_triangle_edges(mesh);
    for(const auto &edge : constraints){
        REQUIRE(mesh_edges.count(edge) == 1);
    }
}

template <class T, class I>
void require_non_constraint_edges_are_locally_delaunay(const fv_surface_mesh<T, I> &mesh,
                                                       const std::vector<edge_type<I>> &constraints){
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
        const auto a = as_vec2(mesh.vertices.at(edge.first));
        const auto b = as_vec2(mesh.vertices.at(edge.second));
        const auto c = as_vec2(mesh.vertices.at(w));
        const auto d = as_vec2(mesh.vertices.at(x));
        REQUIRE(incircle2d(a, b, c, d) <= static_cast<T>(0));
    }
}

template <class T, class I>
void require_triangle_centroids_within_polygon(const fv_surface_mesh<T, I> &mesh,
                                               const std::vector<vec2<T>> &polygon){
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3);
        const auto &a = mesh.vertices.at(face.at(0));
        const auto &b = mesh.vertices.at(face.at(1));
        const auto &c = mesh.vertices.at(face.at(2));
        const vec2<T> centroid((a.x + b.x + c.x) / static_cast<T>(3),
                               (a.y + b.y + c.y) / static_cast<T>(3));
        REQUIRE(::point_in_polygon_or_on_boundary(polygon, centroid));
    }
}

} // namespace ygor_test_constrained_delaunay
