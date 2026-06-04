#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <optional>
#include <sstream>

#include <YgorMath.h>
#include <YgorMathIOOBJ.h>
#include <YgorMeshesBoolean3.h>
#include <YgorMeshesVerification.h>
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

template <class T>
static vec3<T>
rotate_x3(const vec3<T> &v,
          T angle){
    return vec3<T>(v.x,
                   std::cos(angle) * v.y - std::sin(angle) * v.z,
                   std::sin(angle) * v.y + std::cos(angle) * v.z);
}

template <class T>
static vec3<T>
rotate_y3(const vec3<T> &v,
          T angle){
    return vec3<T>(std::cos(angle) * v.x + std::sin(angle) * v.z,
                   v.y,
                  -std::sin(angle) * v.x + std::cos(angle) * v.z);
}

template <class T>
static vec3<T>
rotate_z3(const vec3<T> &v,
          T angle){
    return vec3<T>(std::cos(angle) * v.x - std::sin(angle) * v.y,
                   std::sin(angle) * v.x + std::cos(angle) * v.y,
                   v.z);
}

template <class T, class I>
static fv_surface_mesh<T, I>
transform_mesh3(fv_surface_mesh<T, I> mesh,
                const vec3<T> &centre,
                const vec3<T> &translation,
                const vec3<T> &angles){
    for(auto &v : mesh.vertices){
        auto p = v - centre;
        p = rotate_x3(p, angles.x);
        p = rotate_y3(p, angles.y);
        p = rotate_z3(p, angles.z);
        v = p + centre + translation;
    }
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
static fv_surface_mesh<T, I>
read_obj_mesh3(const std::string &obj){
    fv_surface_mesh<T, I> mesh;
    std::stringstream ss(obj);
    REQUIRE(ReadFVSMeshFromOBJ(mesh, ss));
    REQUIRE(!mesh.vertices.empty());
    REQUIRE(!mesh.faces.empty());
    REQUIRE(OrientFaces(mesh));
    mesh.recreate_involved_face_index();
    return mesh;
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
        REQUIRE(IsClosedManifold(out));
        CHECK(std::abs(std::abs(mesh_signed_volume3(out)) - 1.5) < 1.0e-5);
    }

    SUBCASE("Intersection of overlapping boxes has exact expected volume"){
        const auto lhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                           vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.5, 0.0, 0.0),
                                                           vec3<double>(1.5, 1.0, 1.0));

        const auto out = BooleanIntersection3(lhs, rhs);
        REQUIRE(!out.faces.empty());
        REQUIRE(IsClosedManifold(out));
        CHECK(std::abs(std::abs(mesh_signed_volume3(out)) - 0.5) < 1.0e-5);
    }

    SUBCASE("Subtraction with embedded box leaves a cavity"){
        const auto lhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                           vec3<double>(2.0, 2.0, 2.0));
        const auto rhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.5, 0.5, 0.5),
                                                           vec3<double>(1.5, 1.5, 1.5));

        const auto out = BooleanSubtraction3(lhs, rhs);
        REQUIRE(!out.faces.empty());
        REQUIRE(IsClosedManifold(out));
        CHECK(std::abs(std::abs(mesh_signed_volume3(out)) - 7.0) < 1.0e-5);
    }

    SUBCASE("Exclusion removes the shared overlap volume"){
        const auto lhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                           vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.5, 0.0, 0.0),
                                                           vec3<double>(1.5, 1.0, 1.0));

        const auto out = BooleanExclusion3(lhs, rhs);
        REQUIRE(!out.faces.empty());
        REQUIRE(IsClosedManifold(out));
        CHECK(std::abs(std::abs(mesh_signed_volume3(out)) - 1.0) < 1.0e-5);
    }

    SUBCASE("Coplanar shared faces are resolved in a union"){
        const auto lhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                           vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh3<double, uint64_t>(vec3<double>(1.0, 0.0, 0.0),
                                                           vec3<double>(2.0, 1.0, 1.0));

        const auto out = BooleanUnion3(lhs, rhs);
        REQUIRE(!out.faces.empty());
        REQUIRE(IsClosedManifold(out));
        CHECK(std::abs(std::abs(mesh_signed_volume3(out)) - 2.0) < 1.0e-5);
    }

    SUBCASE("Non-box closed mesh routes away from box-only optimization"){
        const auto lhs = make_tetra_mesh3<double, uint64_t>();
        const auto rhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                           vec3<double>(1.0, 1.0, 1.0));

        const auto out = BooleanUnion3(lhs, rhs);
        REQUIRE(!out.faces.empty());
        REQUIRE(IsClosedManifold(out));
        CHECK(std::abs(std::abs(mesh_signed_volume3(out)) - 1.0) < 1.0e-5);
    }

    SUBCASE("Rotated box overlap stays closed through flood-filled classification"){
        const auto lhs = make_box_mesh3<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                           vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = transform_mesh3(
            make_box_mesh3<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                             vec3<double>(0.79056770333667536,
                                                          0.61638930571214234,
                                                          0.86877650931814832)),
            vec3<double>(0.5, 0.5, 0.5),
            vec3<double>(-0.71375438444731121,
                         0.09331071326240159,
                         0.22327482635719553),
            vec3<double>(0.85458411866745732,
                         0.63179573031704728,
                         0.16916079591411626));

        const auto union_out = BooleanUnion3(lhs, rhs);
        REQUIRE(!union_out.faces.empty());
        REQUIRE(IsClosedManifold(union_out));

        const auto subtraction_out = BooleanSubtraction3(lhs, rhs);
        REQUIRE(!subtraction_out.faces.empty());
        REQUIRE(IsClosedManifold(subtraction_out));
    }

    SUBCASE("Provided skewed box regression succeeds for all Boolean operations"){
        const auto lhs = read_obj_mesh3<double, uint64_t>(R"obj(# Wavefront OBJ file.
v -58.991596638655466 -64.159663865546236 -10
v 67.058823529411768 -65.924369747899163 -10
v -53.69747899159664 58.613445378151262 -10
v 62.521008403361336 69.70588235294116 -10
v -58.991596638655466 -64.159663865546236 10
v 67.058823529411768 -65.924369747899163 10
v -53.69747899159664 58.613445378151262 10
v 62.521008403361336 69.70588235294116 10
f 5 6 7
f 1 3 2
f 7 6 8
f 3 4 2
f 1 2 6
f 1 6 5
f 3 1 5
f 3 5 7
f 2 4 8
f 2 8 6
f 4 3 7
f 4 7 8
)obj");
        const auto rhs = read_obj_mesh3<double, uint64_t>(R"obj(# Wavefront OBJ file.
v -29.747899159663863 -22.310924369747902 -24.5
v 29.495798319327729 -21.30252100840336 -24.5
v -27.731092436974787 31.638655462184872 -24.5
v 25.966386554621849 33.655462184873947 -24.5
v -29.747899159663863 -22.310924369747902 17.5
v 29.495798319327729 -21.30252100840336 17.5
v -27.731092436974787 31.638655462184872 17.5
v 25.966386554621849 33.655462184873947 17.5
f 5 6 7
f 1 3 2
f 7 6 8
f 3 4 2
f 1 2 6
f 1 6 5
f 3 1 5
f 3 5 7
f 2 4 8
f 2 8 6
f 4 3 7
f 4 7 8
)obj");

        const auto union_out = BooleanUnion3(lhs, rhs);
        REQUIRE(!union_out.faces.empty());
        REQUIRE(IsClosedManifold(union_out));

        const auto intersection_out = BooleanIntersection3(lhs, rhs);
        REQUIRE(!intersection_out.faces.empty());
        REQUIRE(IsClosedManifold(intersection_out));

        const auto exclusion_out = BooleanExclusion3(lhs, rhs);
        REQUIRE(!exclusion_out.faces.empty());
        REQUIRE(IsClosedManifold(exclusion_out));

        const auto subtraction_out = BooleanSubtraction3(lhs, rhs);
        REQUIRE(!subtraction_out.faces.empty());
        REQUIRE(IsClosedManifold(subtraction_out));
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
