

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include <YgorMath.h>
#include <YgorMeshesBSPTree.h>
#include <YgorMeshesBoolean4.h>
#include <YgorMeshesOrient.h>
#include <YgorMeshesVerification.h>

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
        if(face.size() != 3UL) continue;
        const auto &a = mesh.vertices.at(face.at(0));
        const auto &b = mesh.vertices.at(face.at(1));
        const auto &c = mesh.vertices.at(face.at(2));
        total += static_cast<long double>(a.Dot(b.Cross(c))) / 6.0L;
    }
    return static_cast<double>(total);
}


template <class T, class I>
static fv_surface_mesh<T, I>
make_tetrahedron(const vec3<T> &a,
                 const vec3<T> &b,
                 const vec3<T> &c,
                 const vec3<T> &d){
    fv_surface_mesh<T, I> mesh;
    mesh.vertices = { a, b, c, d };
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


TEST_CASE("YgorMeshesBoolean4 -- mesh to BSP conversion"){
    SUBCASE("Box mesh converts to non-empty BSP tree"){
        const auto box = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        const auto vt = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(box, 42);
        REQUIRE(!vt.empty());
        REQUIRE(vt.get_root() != nullptr);
    }

    SUBCASE("Tetrahedron converts to non-empty BSP tree"){
        const auto tet = make_tetrahedron<double, uint64_t>(
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(0.0, 0.0, 1.0));
        const auto vt = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(tet, 42);
        REQUIRE(!vt.empty());
        REQUIRE(vt.get_root() != nullptr);
    }

    SUBCASE("Empty mesh produces empty BSP tree"){
        fv_surface_mesh<double, uint64_t> empty;
        const auto vt = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(empty, 42);
        REQUIRE(vt.empty());
    }

    SUBCASE("BSP to mesh conversion produces output faces"){
        const auto box = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        const auto vt = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(box, 42);
        const auto mesh = vt.to_fv_surface_mesh();
        REQUIRE(!mesh.faces.empty());
        REQUIRE(IsTriangularMesh(mesh));
    }

    SUBCASE("Float specialization compiles"){
        const auto box = make_box_mesh<float, uint32_t>(vec3<float>(0.0f, 0.0f, 0.0f),
                                                         vec3<float>(1.0f, 1.0f, 1.0f));
        const auto vt = bsp_tree_volume<float, uint32_t>::from_fv_surface_mesh(box, 42);
        REQUIRE(!vt.empty());
    }

    SUBCASE("Uint32_t index type compiles"){
        const auto box = make_box_mesh<double, uint32_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        const auto vt = bsp_tree_volume<double, uint32_t>::from_fv_surface_mesh(box, 42);
        REQUIRE(!vt.empty());
    }

    SUBCASE("Open mesh is rejected by from_fv_surface_mesh"){
        fv_surface_mesh<double, uint64_t> open_mesh;
        open_mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0)
        };
        open_mesh.faces = {{ 0, 1, 2 }};
        REQUIRE_THROWS(bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(open_mesh, 42));
    }
}


TEST_CASE("YgorMeshesBoolean4 -- BSP boolean operations"){
    SUBCASE("Self-union is not empty"){
        const auto box = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        const auto bt = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(box, 42);
        const auto result = bt.boolean_union(bt);
        REQUIRE(!result.empty());
    }

    SUBCASE("Self-intersection is not empty"){
        const auto box = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        const auto bt = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(box, 42);
        const auto result = bt.boolean_intersection(bt);
        REQUIRE(!result.empty());
    }

    SUBCASE("Union of disjoint boxes is not empty"){
        const auto lhs = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh<double, uint64_t>(vec3<double>(3.0, 3.0, 3.0),
                                                          vec3<double>(4.0, 4.0, 4.0));
        const auto lt = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(lhs, 42);
        const auto rt = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(rhs, 42);
        const auto result = lt.boolean_union(rt);
        REQUIRE(!result.empty());
    }

    SUBCASE("Intersection of overlapping boxes produces valid non-empty mesh"){
        const auto lhs = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh<double, uint64_t>(vec3<double>(0.5, 0.0, 0.0),
                                                          vec3<double>(1.5, 1.0, 1.0));
        const auto lt = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(lhs, 42);
        const auto rt = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(rhs, 42);
        const auto result = lt.boolean_intersection(rt);
        REQUIRE(!result.empty());
        const auto mesh = result.to_fv_surface_mesh();
        REQUIRE(!mesh.faces.empty());
        REQUIRE(IsTriangularMesh(mesh));
        REQUIRE(IsClosedManifold(mesh));
        REQUIRE(HasConsistentOrientation(mesh));
        const auto vol = mesh_signed_volume(mesh);
        REQUIRE(vol > 0.0);
    }

    SUBCASE("Subtraction of overlapping boxes is not empty"){
        const auto lhs = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh<double, uint64_t>(vec3<double>(0.5, 0.0, 0.0),
                                                          vec3<double>(1.5, 1.0, 1.0));
        const auto lt = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(lhs, 42);
        const auto rt = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(rhs, 42);
        const auto result = lt.boolean_subtraction(rt);
        REQUIRE(!result.empty());
    }

    SUBCASE("Exclusion of overlapping boxes is not empty"){
        const auto lhs = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh<double, uint64_t>(vec3<double>(0.5, 0.0, 0.0),
                                                          vec3<double>(1.5, 1.0, 1.0));
        const auto lt = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(lhs, 42);
        const auto rt = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(rhs, 42);
        const auto result = lt.boolean_exclusion(rt);
        REQUIRE(!result.empty());
    }
}


TEST_CASE("YgorMeshesBoolean4 -- all-IN / all-OUT edge cases"){
    SUBCASE("All-IN BSP tree converts to unit bounding box mesh"){
        auto root = std::make_unique<typename bsp_tree_volume<double, uint64_t>::Node>(
            bsp_tree_volume<double, uint64_t>::NodeType::In);
        bsp_tree_volume<double, uint64_t> vt(std::move(root));

        const auto mesh = vt.to_fv_surface_mesh();
        REQUIRE(!mesh.faces.empty());
        REQUIRE(IsTriangularMesh(mesh));
    }

    SUBCASE("All-OUT BSP tree converts to empty mesh"){
        auto root = std::make_unique<typename bsp_tree_volume<double, uint64_t>::Node>(
            bsp_tree_volume<double, uint64_t>::NodeType::Out);
        bsp_tree_volume<double, uint64_t> vt(std::move(root));

        const auto mesh = vt.to_fv_surface_mesh();
        REQUIRE(mesh.faces.empty());
    }

    SUBCASE("Empty BSP tree converts to empty mesh"){
        bsp_tree_volume<double, uint64_t> vt;
        REQUIRE(vt.empty());
        const auto mesh = vt.to_fv_surface_mesh();
        REQUIRE(mesh.faces.empty());
    }

    SUBCASE("Boolean operations with empty BSP tree"){
        bsp_tree_volume<double, uint64_t> empty;
        auto root = std::make_unique<typename bsp_tree_volume<double, uint64_t>::Node>(
            bsp_tree_volume<double, uint64_t>::NodeType::In);
        bsp_tree_volume<double, uint64_t> solid(std::move(root));

        REQUIRE(!empty.boolean_union(solid).empty());
        REQUIRE(!solid.boolean_union(empty).empty());
        REQUIRE(empty.boolean_intersection(solid).empty());
        REQUIRE(solid.boolean_intersection(empty).empty());
        REQUIRE(empty.boolean_subtraction(solid).empty());
        REQUIRE(!solid.boolean_subtraction(empty).empty());
    }
}


TEST_CASE("YgorMeshesBoolean4 -- BooleanMeshOp4 integration"){
    SUBCASE("Union of overlapping boxes via BooleanMeshOp4 produces faces"){
        const auto lhs = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh<double, uint64_t>(vec3<double>(0.5, 0.0, 0.0),
                                                          vec3<double>(1.5, 1.0, 1.0));
        const auto out = BooleanMeshOp4(lhs, rhs, MeshBooleanOperation4::Union);
        REQUIRE(!out.faces.empty());
    }

    SUBCASE("Union, intersection, and subtraction compile and run"){
        const auto lhs = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh<double, uint64_t>(vec3<double>(0.5, 0.0, 0.0),
                                                          vec3<double>(1.5, 1.0, 1.0));
        REQUIRE(!BooleanUnion4(lhs, rhs).faces.empty());
        REQUIRE(!BooleanIntersection4(lhs, rhs).faces.empty());
        REQUIRE(!BooleanSubtraction4(lhs, rhs).faces.empty());
    }

    SUBCASE("Exclusion compiles and runs"){
        const auto lhs = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        const auto rhs = make_box_mesh<double, uint64_t>(vec3<double>(0.5, 0.0, 0.0),
                                                          vec3<double>(1.5, 1.0, 1.0));
        REQUIRE(!BooleanExclusion4(lhs, rhs).faces.empty());
    }

    SUBCASE("Empty mesh union returns other mesh"){
        const auto box = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        fv_surface_mesh<double, uint64_t> empty;
        const auto out = BooleanMeshOp4(box, empty, MeshBooleanOperation4::Union);
        REQUIRE(!out.faces.empty());
    }

    SUBCASE("Empty mesh intersection returns empty"){
        const auto box = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        fv_surface_mesh<double, uint64_t> empty;
        const auto out = BooleanMeshOp4(box, empty, MeshBooleanOperation4::Intersection);
        REQUIRE(out.faces.empty());
    }

    SUBCASE("Float specialization compiles and runs"){
        const auto lhs = make_box_mesh<float, uint32_t>(vec3<float>(0.0f, 0.0f, 0.0f),
                                                         vec3<float>(1.0f, 1.0f, 1.0f));
        const auto rhs = make_box_mesh<float, uint32_t>(vec3<float>(0.5f, 0.0f, 0.0f),
                                                         vec3<float>(1.5f, 1.0f, 1.0f));
        REQUIRE(!BooleanUnion4(lhs, rhs).faces.empty());
    }
}


TEST_CASE("YgorMeshesBoolean4 -- verification functions"){
    SUBCASE("Box mesh passes all verification checks"){
        const auto box = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        REQUIRE(HasOnlyFiniteVertices(box));
        REQUIRE(IsTriangularMesh(box));
        REQUIRE(HasValidFaceIndices(box));
        REQUIRE(HasNoDegenerateFaces(box));
        REQUIRE(IsClosedManifold(box));
        REQUIRE(HasConsistentOrientation(box));
        const auto info = ClassifyEdges<double, uint64_t>(box);
        REQUIRE(info.boundary_edges == 0UL);
        REQUIRE(info.nonmanifold_edges == 0UL);
    }

    SUBCASE("ValidateClosedTriangularMesh succeeds for valid mesh"){
        const auto box = make_box_mesh<double, uint64_t>(vec3<double>(0.0, 0.0, 0.0),
                                                          vec3<double>(1.0, 1.0, 1.0));
        REQUIRE(ValidateClosedTriangularMesh(box, "test", false));
    }

    SUBCASE("Open mesh fails closed manifold check"){
        fv_surface_mesh<double, uint64_t> open_mesh;
        open_mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0)
        };
        open_mesh.faces = {{ 0, 1, 2 }};
        REQUIRE(!IsClosedManifold(open_mesh));
    }

    SUBCASE("Non-manifold mesh detected via ClassifyEdges"){
        fv_surface_mesh<double, uint64_t> nm_mesh;
        nm_mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(0.0, 0.0, 1.0)
        };
        nm_mesh.faces = {
            { 0, 1, 2 }, { 0, 1, 2 },
            { 0, 2, 3 }, { 0, 3, 1 }, { 1, 3, 2 }
        };
        const auto info = ClassifyEdges<double, uint64_t>(nm_mesh);
        REQUIRE(info.nonmanifold_edges > 0UL);
    }

    SUBCASE("Inconsistent orientation detected"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0),
            vec3<double>(0.0, 0.0, 1.0)
        };
        mesh.faces = {
            { 0, 1, 2 }, { 0, 1, 3 },
            { 0, 2, 3 }, { 1, 2, 3 }
        };
        REQUIRE(!HasConsistentOrientation(mesh));
    }

    SUBCASE("Degenerate face detected"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(0.0, 0.0, 0.0)
        };
        mesh.faces = {{ 0, 1, 2 }};
        REQUIRE(!HasNoDegenerateFaces(mesh));
    }

    SUBCASE("Open mesh is rejected by from_fv_surface_mesh"){
        fv_surface_mesh<double, uint64_t> open_mesh;
        open_mesh.vertices = {
            vec3<double>(0.0, 0.0, 0.0),
            vec3<double>(1.0, 0.0, 0.0),
            vec3<double>(0.0, 1.0, 0.0)
        };
        open_mesh.faces = {{ 0, 1, 2 }};
        REQUIRE_THROWS(bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(open_mesh, 42));
    }

    SUBCASE("Mesh with non-finite vertices is rejected"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = { vec3<double>(std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0) };
        mesh.faces = {{ 0, 0, 0 }};
        REQUIRE_THROWS(bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 42));
    }
}
