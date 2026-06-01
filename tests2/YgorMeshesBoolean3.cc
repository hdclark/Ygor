#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>

#include <YgorMath.h>
#include <YgorMeshesBoolean3.h>
#include <YgorMeshesOrient.h>

#include "doctest/doctest.h"


template <class T, class I>
static fv_surface_mesh<T, I>
make_box_mesh3(const vec3<T> &bb_min,
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
static fv_surface_mesh<T, I>
make_tetra_mesh3(){
    fv_surface_mesh<T, I> mesh;
    mesh.vertices = {
        vec3<T>(0.2, 0.2, 0.2),
        vec3<T>(0.8, 0.2, 0.2),
        vec3<T>(0.2, 0.8, 0.2),
        vec3<T>(0.2, 0.2, 0.8)
    };
    mesh.faces = {
        { 0, 1, 2 },
        { 0, 3, 1 },
        { 0, 2, 3 },
        { 1, 3, 2 }
    };

    REQUIRE(OrientFaces(mesh));
    mesh.recreate_involved_face_index();
    return mesh;
}

template <class T, class I>
static double
mesh_signed_volume3(const fv_surface_mesh<T, I> &mesh){
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
check_closed_triangles3(const fv_surface_mesh<T, I> &mesh){
    std::map<std::pair<I, I>, int> edge_counts;
    for(const auto &face : mesh.faces){
        REQUIRE(face.size() == 3UL);
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

TEST_CASE("YgorMeshesBoolean3 symbolic support"){
    const auto px = MakeBoolean3Plane(vec3<double>(0.0, 0.0, 0.0),
                                      vec3<double>(0.0, 1.0, 0.0),
                                      vec3<double>(0.0, 0.0, 1.0));
    const auto py = MakeBoolean3Plane(vec3<double>(0.0, 0.0, 0.0),
                                      vec3<double>(1.0, 0.0, 0.0),
                                      vec3<double>(0.0, 0.0, 1.0));
    const auto pz = MakeBoolean3Plane(vec3<double>(0.0, 0.0, 0.0),
                                      vec3<double>(1.0, 0.0, 0.0),
                                      vec3<double>(0.0, 1.0, 0.0));
    const auto shifted = MakeBoolean3Plane(vec3<double>(1.0, 0.0, 0.0),
                                           vec3<double>(1.0, 1.0, 0.0),
                                           vec3<double>(1.0, 0.0, 1.0));

    SymbolicVertex<double> sv{{ px, py, pz }, std::nullopt};
    const auto p = EvaluateSymbolicVertex(sv);
    REQUIRE(p.has_value());
    CHECK(p->distance(vec3<double>(0.0, 0.0, 0.0)) < 1.0e-12);
    CHECK(OrientSymbolicVertexAgainstPlane(sv, shifted) < 0);
}

TEST_CASE("YgorMeshesBoolean3"){
    SUBCASE("Intersection of disjoint boxes is empty"){
        const auto lhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                           vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh3<double, uint64_t>(vec3<double>(3.0, 3.0, 3.0),
                                                           vec3<double>(4.0, 4.0, 4.0));

        const auto out = BooleanIntersection3(lhs, rhs);
        REQUIRE(out.vertices.empty());
        REQUIRE(out.faces.empty());
    }

    SUBCASE("Union of overlapping boxes has exact expected volume"){
        const auto lhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                           vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.5, 0.0, 0.0),
                                                           vec3<double>(1.5, 1.0, 1.0));

        const auto out = BooleanUnion3(lhs, rhs);
        REQUIRE(!out.faces.empty());
        check_closed_triangles3(out);
        CHECK(std::abs(std::abs(mesh_signed_volume3(out)) - 1.5) < 1.0e-5);
    }

    SUBCASE("Intersection of overlapping boxes has exact expected volume"){
        const auto lhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                           vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.5, 0.0, 0.0),
                                                           vec3<double>(1.5, 1.0, 1.0));

        const auto out = BooleanIntersection3(lhs, rhs);
        REQUIRE(!out.faces.empty());
        check_closed_triangles3(out);
        CHECK(std::abs(std::abs(mesh_signed_volume3(out)) - 0.5) < 1.0e-5);
    }

    SUBCASE("Subtraction with embedded box leaves a cavity"){
        const auto lhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                           vec3<double>(2.0, 2.0, 2.0));
        const auto rhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.5, 0.5, 0.5),
                                                           vec3<double>(1.5, 1.5, 1.5));

        const auto out = BooleanSubtraction3(lhs, rhs);
        REQUIRE(!out.faces.empty());
        check_closed_triangles3(out);
        CHECK(std::abs(std::abs(mesh_signed_volume3(out)) - 7.0) < 1.0e-5);
    }

    SUBCASE("Exclusion removes the shared overlap volume"){
        const auto lhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                           vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.5, 0.0, 0.0),
                                                           vec3<double>(1.5, 1.0, 1.0));

        const auto out = BooleanExclusion3(lhs, rhs);
        REQUIRE(!out.faces.empty());
        check_closed_triangles3(out);
        CHECK(std::abs(std::abs(mesh_signed_volume3(out)) - 1.0) < 1.0e-5);
    }

    SUBCASE("Coplanar shared faces are resolved in a union"){
        const auto lhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                           vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh3<double, uint64_t>(vec3<double>(1.0, 0.0, 0.0),
                                                           vec3<double>(2.0, 1.0, 1.0));

        const auto out = BooleanUnion3(lhs, rhs);
        REQUIRE(!out.faces.empty());
        check_closed_triangles3(out);
        CHECK(std::abs(std::abs(mesh_signed_volume3(out)) - 2.0) < 1.0e-5);
    }

    SUBCASE("Non-box closed mesh routes away from box-only optimization"){
        const auto lhs = make_tetra_mesh3<double, uint64_t>();
        const auto rhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                           vec3<double>(1.0, 1.0, 1.0));

        const auto out = BooleanUnion3(lhs, rhs);
        REQUIRE(!out.faces.empty());
        check_closed_triangles3(out);
        CHECK(std::abs(std::abs(mesh_signed_volume3(out)) - 1.0) < 1.0e-5);
    }

    SUBCASE("Float specialization compiles for disjoint inputs"){
        const auto lhs = make_box_mesh3<float, uint32_t>(vec3<float>(0.0f, 0.0f, 0.0f),
                                                          vec3<float>(1.0f, 1.0f, 1.0f));
        const auto rhs = make_box_mesh3<float, uint32_t>(vec3<float>(2.0f, 2.0f, 2.0f),
                                                          vec3<float>(3.0f, 3.0f, 3.0f));

        const auto out = BooleanIntersection3(lhs, rhs);
        REQUIRE(out.vertices.empty());
        REQUIRE(out.faces.empty());
    }
}
