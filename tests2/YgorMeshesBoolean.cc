
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <utility>

#include <YgorMath.h>
#include <YgorMeshesBoolean.h>
#include <YgorMeshesOrient.h>

#include "doctest/doctest.h"


template <class T, class I>
static fv_surface_mesh<T, I>
make_box_mesh(const vec3<T> &bb_min,
              const vec3<T> &bb_max){
    fv_surface_mesh<T, I> mesh;
    mesh.vertices = {
        vec3<T>(bb_min.x, bb_min.y, bb_min.z),
        vec3<T>(bb_max.x, bb_min.y, bb_min.z),
        vec3<T>(bb_max.x, bb_max.y, bb_min.z),
        vec3<T>(bb_min.x, bb_max.y, bb_min.z),
        vec3<T>(bb_min.x, bb_min.y, bb_max.z),
        vec3<T>(bb_max.x, bb_min.y, bb_max.z),
        vec3<T>(bb_max.x, bb_max.y, bb_max.z),
        vec3<T>(bb_min.x, bb_max.y, bb_max.z)
    };

    mesh.faces = {
        { 0, 2, 1 }, { 0, 3, 2 },
        { 4, 5, 6 }, { 4, 6, 7 },
        { 0, 1, 5 }, { 0, 5, 4 },
        { 2, 3, 7 }, { 2, 7, 6 },
        { 0, 4, 7 }, { 0, 7, 3 },
        { 1, 2, 6 }, { 1, 6, 5 }
    };

    REQUIRE(OrientFaces(mesh));
    mesh.recreate_involved_face_index();
    return mesh;
}

template <class T, class I>
static double
mesh_signed_volume(const fv_surface_mesh<T, I> &mesh){
    long double total = 0.0L;
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3UL);
        const auto &a = mesh.vertices.at(face.at(0));
        const auto &b = mesh.vertices.at(face.at(1));
        const auto &c = mesh.vertices.at(face.at(2));
        total += static_cast<long double>(a.Dot(b.Cross(c))) / 6.0L;
    }
    return static_cast<double>(total);
}

template <class T, class I>
static void
check_closed_triangles(const fv_surface_mesh<T, I> &mesh){
    std::map<std::pair<I, I>, int> edge_counts;
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3UL);
        for(const auto vi : face){
            REQUIRE(vi < mesh.vertices.size());
        }
        for(size_t i = 0; i < 3UL; ++i){
            const auto a = face.at(i);
            const auto b = face.at((i + 1UL) % 3UL);
            edge_counts[{ std::min(a, b), std::max(a, b) }] += 1;
        }
    }

    for(const auto &ep : edge_counts){
        REQUIRE(ep.second == 2);
    }
}


TEST_CASE("YgorMeshesBoolean"){
    SUBCASE("Intersection of disjoint boxes is empty"){
        const auto lhs = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                         vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh<double, uint64_t>(vec3<double>(3.0, 3.0, 3.0),
                                                         vec3<double>(4.0, 4.0, 4.0));

        const auto out = BooleanIntersection(lhs, rhs, 5, 0.0);
        REQUIRE(out.faces.empty());
        REQUIRE(out.vertices.empty());
    }

    SUBCASE("Union of overlapping boxes produces a closed mesh with larger volume"){
        const auto lhs = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                         vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh<double, uint64_t>(vec3<double>(0.5, 0.0, 0.0),
                                                         vec3<double>(1.5, 1.0, 1.0));

        const auto out = BooleanUnion(lhs, rhs, 5, 0.0);
        REQUIRE(!out.faces.empty());
        REQUIRE(out.involved_faces.size() == out.vertices.size());
        check_closed_triangles(out);

        const auto volume = std::abs(mesh_signed_volume(out));
        REQUIRE(volume > 1.1);
        REQUIRE(volume < 1.9);
    }

    SUBCASE("Intersection of overlapping boxes has positive bounded volume"){
        const auto lhs = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                         vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh<double, uint64_t>(vec3<double>(0.5, 0.0, 0.0),
                                                         vec3<double>(1.5, 1.0, 1.0));

        const auto out = BooleanIntersection(lhs, rhs, 5, 0.0);
        REQUIRE(!out.faces.empty());
        check_closed_triangles(out);

        const auto volume = std::abs(mesh_signed_volume(out));
        REQUIRE(volume > 0.30);
        REQUIRE(volume < 0.70);
    }

    SUBCASE("Subtraction preserves an internal cavity"){
        const auto lhs = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                         vec3<double>(2.0, 2.0, 2.0));
        const auto rhs = make_box_mesh<double, uint64_t>(vec3<double>(0.5, 0.5, 0.5),
                                                         vec3<double>(1.5, 1.5, 1.5));

        const auto out = BooleanSubtraction(lhs, rhs, 5, 0.0);
        REQUIRE(!out.faces.empty());
        check_closed_triangles(out);

        const auto volume = std::abs(mesh_signed_volume(out));
        REQUIRE(volume > 6.0);
        REQUIRE(volume < 7.8);
    }

    SUBCASE("Exclusion of overlapping boxes removes the shared region"){
        const auto lhs = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                         vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh<double, uint64_t>(vec3<double>(0.5, 0.0, 0.0),
                                                         vec3<double>(1.5, 1.0, 1.0));

        const auto out = BooleanExclusion(lhs, rhs, 5, 0.0);
        REQUIRE(!out.faces.empty());
        check_closed_triangles(out);

        const auto volume = std::abs(mesh_signed_volume(out));
        REQUIRE(volume > 0.7);
        REQUIRE(volume < 1.3);
    }

    SUBCASE("Open meshes are rejected"){
        fv_surface_mesh<double, uint64_t> open_mesh;
        open_mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0)
        };
        open_mesh.faces = {{ 0, 1, 2 }};

        const auto rhs = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                         vec3<double>(1.0, 1.0, 1.0));

        REQUIRE_THROWS(BooleanUnion(open_mesh, rhs, 4, 0.0));
    }

    SUBCASE("Float specialization compiles and returns an empty disjoint intersection"){
        const auto lhs = make_box_mesh<float, uint32_t>(vec3<float>(0.0f, 0.0f, 0.0f),
                                                        vec3<float>(1.0f, 1.0f, 1.0f));
        const auto rhs = make_box_mesh<float, uint32_t>(vec3<float>(2.0f, 2.0f, 2.0f),
                                                        vec3<float>(3.0f, 3.0f, 3.0f));

        const auto out = BooleanIntersection(lhs, rhs, 4, 0.0f);
        REQUIRE(out.faces.empty());
        REQUIRE(out.vertices.empty());
    }
}
