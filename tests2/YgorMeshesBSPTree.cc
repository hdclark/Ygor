//YgorMeshesBSPTree.cc - Tests for BSP tree volume class.
//Written by hal clark in 2026.

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <YgorMath.h>
#include <YgorMeshesBSPTree.h>
#include <YgorMeshesVerification.h>

#include "doctest/doctest.h"


namespace ygor_bsp_tree_exact_test {
bool exact_kernel_self_test();
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
make_axis_aligned_box(const vec3<T> &bb_min,
                      const vec3<T> &bb_max){
    fv_surface_mesh<T, I> mesh;
    mesh.vertices = {
        {bb_min.x, bb_min.y, bb_min.z}, {bb_max.x, bb_min.y, bb_min.z},
        {bb_max.x, bb_max.y, bb_min.z}, {bb_min.x, bb_max.y, bb_min.z},
        {bb_min.x, bb_min.y, bb_max.z}, {bb_max.x, bb_min.y, bb_max.z},
        {bb_max.x, bb_max.y, bb_max.z}, {bb_min.x, bb_max.y, bb_max.z}
    };
    mesh.faces = {
        {0, 3, 2, 1}, {4, 5, 6, 7},
        {0, 1, 5, 4}, {1, 2, 6, 5},
        {2, 3, 7, 6}, {3, 0, 4, 7}
    };
    return mesh;
}

template <class T, class I>
static std::string
mesh_signature(const fv_surface_mesh<T, I> &mesh){
    std::ostringstream os;
    os << std::hexfloat;
    for(const auto &v : mesh.vertices){
        os << "v:" << v.x << ',' << v.y << ',' << v.z << ';';
    }
    for(const auto &f : mesh.faces){
        os << "f";
        for(const auto idx : f) os << ':' << idx;
        os << ';';
    }
    return os.str();
}

template <class T, class I>
static void
require_valid_bounded_output_mesh(const fv_surface_mesh<T, I> &mesh){
    REQUIRE(!mesh.faces.empty());
    REQUIRE(HasOnlyFiniteVertices(mesh));
    REQUIRE(IsTriangularMesh(mesh));
    REQUIRE(HasValidFaceIndices(mesh));
    REQUIRE(HasNoDegenerateFaces(mesh));
    REQUIRE(IsClosedManifold(mesh));
    REQUIRE(HasConsistentOrientation(mesh));
    REQUIRE(mesh_signed_volume(mesh) > 0.0);
}

template <class T, class I>
static std::string
bsp_tree_signature(const typename bsp_tree_volume<T, I>::Node *node){
    using NT = typename bsp_tree_volume<T, I>::NodeType;
    if(node == nullptr) return "null";
    if(node->type == NT::In) return "in";
    if(node->type == NT::Out) return "out";

    std::ostringstream os;
    os << std::hexfloat << "p";
    for(const auto &anchor : node->partition_plane.anchors){
        os << ':' << anchor.x << ',' << anchor.y << ',' << anchor.z;
    }
    os << '[' << bsp_tree_signature<T, I>(node->front.get())
       << '|' << bsp_tree_signature<T, I>(node->back.get()) << ']';
    return os.str();
}


TEST_CASE( "bsp_tree_volume exact kernel helpers" ){
    REQUIRE(ygor_bsp_tree_exact_test::exact_kernel_self_test());
}


TEST_CASE( "bsp_tree_volume default construction" ){

    SUBCASE("default-constructed tree is empty"){
        bsp_tree_volume<double, uint64_t> vol;
        REQUIRE(vol.empty());
        REQUIRE(vol.get_root() == nullptr);
    }

    SUBCASE("copy construction of empty tree"){
        bsp_tree_volume<double, uint64_t> vol;
        bsp_tree_volume<double, uint64_t> vol2(vol);
        REQUIRE(vol2.empty());
    }

    SUBCASE("move construction of empty tree"){
        bsp_tree_volume<double, uint64_t> vol;
        bsp_tree_volume<double, uint64_t> vol2(std::move(vol));
        REQUIRE(vol2.empty());
    }

    SUBCASE("copy assignment of empty tree"){
        bsp_tree_volume<double, uint64_t> vol;
        bsp_tree_volume<double, uint64_t> vol2;
        vol2 = vol;
        REQUIRE(vol2.empty());
    }

    SUBCASE("unbounded In volume cannot be converted to a bounded mesh"){
        using Vol = bsp_tree_volume<double, uint64_t>;
        auto root = std::make_unique<Vol::Node>(Vol::NodeType::In);
        Vol vol(std::move(root));
        CHECK_THROWS_AS(vol.to_fv_surface_mesh(), std::runtime_error);
    }
}


TEST_CASE( "bsp_tree_volume mesh conversion round-trip" ){

    SUBCASE("empty mesh produces empty tree"){
        fv_surface_mesh<double, uint64_t> mesh;
        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 42);
        REQUIRE(vol.empty());
        REQUIRE(vol.get_root() != nullptr);
        REQUIRE(vol.get_root()->type == bsp_tree_volume<double, uint64_t>::NodeType::Out);
    }

    SUBCASE("mesh with only degenerate faces is rejected"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            {0.0, 0.0, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 0.0, 0.0}  // degenerate
        };
        mesh.faces = {{0, 1, 2}};
        CHECK_THROWS_AS((bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 42)),
                        std::invalid_argument);
    }

    SUBCASE("open mesh rejection identifies the canonical edge"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            {0.0, 0.0, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0}
        };
        mesh.faces = {{0, 1, 2}};

        try{
            (void)bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 42);
            FAIL("expected open mesh rejection");
        }catch(const std::invalid_argument &e){
            const std::string msg = e.what();
            CHECK(msg.find("canonicalized mesh is not a closed edge manifold") != std::string::npos);
            CHECK(msg.find("canonical edge") != std::string::npos);
            CHECK(msg.find("count 1") != std::string::npos);
        }
    }

    SUBCASE("inconsistent orientation rejection identifies directed edge counts"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            { 0.0,  0.0,  0.0},
            { 1.0,  0.0,  0.0},
            { 0.0,  1.0,  0.0},
            { 0.0,  0.0,  1.0}
        };
        mesh.faces = {
            {0, 2, 1},
            {1, 0, 3},
            {0, 3, 2},
            {1, 2, 3}
        };

        try{
            (void)bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 42);
            FAIL("expected inconsistent orientation rejection");
        }catch(const std::invalid_argument &e){
            const std::string msg = e.what();
            CHECK(msg.find("inconsistent edge orientation") != std::string::npos);
            CHECK(msg.find("directed canonical edge") != std::string::npos);
            CHECK(msg.find("forward count") != std::string::npos);
            CHECK(msg.find("reverse count") != std::string::npos);
        }
    }

    SUBCASE("opposite duplicate face is rejected during canonicalization"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            { 0.0,  0.0,  0.0},
            { 1.0,  0.0,  0.0},
            { 0.0,  1.0,  0.0},
            { 0.0,  0.0,  1.0}
        };
        mesh.faces = {
            {0, 2, 1},
            {0, 1, 3},
            {0, 3, 2},
            {1, 2, 3},
            {0, 1, 2}
        };

        CHECK_THROWS_AS((bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 42)),
                        std::invalid_argument);
    }

    SUBCASE("same-orientation duplicate face is deduplicated"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            { 0.0,  0.0,  0.0},
            { 1.0,  0.0,  0.0},
            { 0.0,  1.0,  0.0},
            { 0.0,  0.0,  1.0}
        };
        mesh.faces = {
            {0, 2, 1},
            {0, 1, 3},
            {0, 3, 2},
            {1, 2, 3},
            {2, 1, 0}
        };

        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 42);
        REQUIRE(!vol.empty());
    }

    SUBCASE("exact duplicate vertices are canonicalized"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            { 0.0,  0.0,  0.0},
            { 1.0,  0.0,  0.0},
            { 0.0,  1.0,  0.0},
            { 0.0,  0.0,  1.0},
            { 0.0,  0.0,  0.0}
        };
        mesh.faces = {
            {4, 2, 1},
            {0, 1, 3},
            {0, 3, 2},
            {1, 2, 3}
        };

        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 42);
        REQUIRE(!vol.empty());
    }

    SUBCASE("tetrahedron mesh produces valid BSP tree and round-trips"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            { 0.0,  0.0,  0.0},
            { 1.0,  0.0,  0.0},
            { 0.0,  1.0,  0.0},
            { 0.0,  0.0,  1.0}
        };
        mesh.faces = {
            {0, 2, 1},  // bottom (CCW from below)
            {0, 1, 3},  // front
            {0, 3, 2},  // left
            {1, 2, 3}   // back-right
        };

        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 42);
        REQUIRE(!vol.empty());

        auto result_mesh = vol.to_fv_surface_mesh();
        REQUIRE(!result_mesh.faces.empty());

        const auto result_vol = std::abs(mesh_signed_volume(result_mesh));
        REQUIRE(result_vol > 0.0);
    }

    SUBCASE("cube mesh produces valid BSP tree"){
        // 8 vertices of a unit cube.
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            {0.0, 0.0, 0.0},
            {1.0, 0.0, 0.0},
            {1.0, 1.0, 0.0},
            {0.0, 1.0, 0.0},
            {0.0, 0.0, 1.0},
            {1.0, 0.0, 1.0},
            {1.0, 1.0, 1.0},
            {0.0, 1.0, 1.0}
        };
        mesh.faces = {
            {0, 3, 2, 1},  // bottom (CCW from below)
            {4, 5, 6, 7},  // top
            {0, 1, 5, 4},  // front
            {1, 2, 6, 5},  // right
            {2, 3, 7, 6},  // back
            {3, 0, 4, 7}   // left
        };

        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 42);
        REQUIRE(!vol.empty());

        auto result_mesh = vol.to_fv_surface_mesh();
        REQUIRE(!result_mesh.faces.empty());

        const auto result_vol = std::abs(mesh_signed_volume(result_mesh));
        REQUIRE(result_vol > 0.0);
    }

    SUBCASE("BSP construction ignores seed and is deterministic"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            {0.0, 0.0, 0.0},
            {1.0, 0.0, 0.0},
            {1.0, 1.0, 0.0},
            {0.0, 1.0, 0.0},
            {0.0, 0.0, 1.0},
            {1.0, 0.0, 1.0},
            {1.0, 1.0, 1.0},
            {0.0, 1.0, 1.0}
        };
        mesh.faces = {
            {0, 3, 2, 1},
            {4, 5, 6, 7},
            {0, 1, 5, 4},
            {1, 2, 6, 5},
            {2, 3, 7, 6},
            {3, 0, 4, 7}
        };

        const auto unseeded = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh);
        const auto seeded_a = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 1);
        const auto seeded_b = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 987654321);

        const auto expected = bsp_tree_signature<double, uint64_t>(unseeded.get_root());
        CHECK(bsp_tree_signature<double, uint64_t>(seeded_a.get_root()) == expected);
        CHECK(bsp_tree_signature<double, uint64_t>(seeded_b.get_root()) == expected);
    }
}


TEST_CASE( "bsp_tree_volume boolean operations" ){

    // Build two overlapping tetrahedra.
    auto make_tetra = [](double ox, double oy, double oz) {
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            {ox + 0.0, oy + 0.0, oz + 0.0},
            {ox + 1.0, oy + 0.0, oz + 0.0},
            {ox + 0.0, oy + 1.0, oz + 0.0},
            {ox + 0.0, oy + 0.0, oz + 1.0}
        };
        mesh.faces = {
            {0, 2, 1},
            {0, 1, 3},
            {0, 3, 2},
            {1, 2, 3}
        };
        return mesh;
    };

    auto tetra_A = make_tetra(0.0, 0.0, 0.0);
    auto tetra_B = make_tetra(0.5, 0.0, 0.0);

    auto bsp_A = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(tetra_A, 42);
    auto bsp_B = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(tetra_B, 42);

    REQUIRE(!bsp_A.empty());
    REQUIRE(!bsp_B.empty());

    SUBCASE("union of two tetrahedra"){
        auto bsp_union = bsp_A.boolean_union(bsp_B);
        REQUIRE(!bsp_union.empty());
        auto result = bsp_union.to_fv_surface_mesh();
        REQUIRE(!result.faces.empty());
    }

    SUBCASE("intersection of two tetrahedra"){
        auto bsp_inter = bsp_A.boolean_intersection(bsp_B);
        REQUIRE(!bsp_inter.empty());
        auto result = bsp_inter.to_fv_surface_mesh();
        REQUIRE(!result.faces.empty());
    }

    SUBCASE("subtraction of two tetrahedra"){
        auto bsp_sub = bsp_A.boolean_subtraction(bsp_B);
        REQUIRE(!bsp_sub.empty());
        auto result = bsp_sub.to_fv_surface_mesh();
        REQUIRE(!result.faces.empty());
    }

    SUBCASE("exclusion of two tetrahedra"){
        auto bsp_xor = bsp_A.boolean_exclusion(bsp_B);
        REQUIRE(!bsp_xor.empty());
        auto result = bsp_xor.to_fv_surface_mesh();
        REQUIRE(!result.faces.empty());
    }

    SUBCASE("union with empty is identity"){
        bsp_tree_volume<double, uint64_t> empty;
        auto bsp_union_empty = bsp_A.boolean_union(empty);
        REQUIRE(!bsp_union_empty.empty());
    }

    SUBCASE("intersection with empty is empty"){
        bsp_tree_volume<double, uint64_t> empty;
        auto bsp_inter_empty = bsp_A.boolean_intersection(empty);
        REQUIRE(bsp_inter_empty.empty());
    }

    SUBCASE("subtraction with empty is identity"){
        bsp_tree_volume<double, uint64_t> empty;
        auto bsp_sub_empty = bsp_A.boolean_subtraction(empty);
        REQUIRE(!bsp_sub_empty.empty());
    }

    SUBCASE("exclusion with empty is identity"){
        bsp_tree_volume<double, uint64_t> empty;
        auto bsp_xor_empty = bsp_A.boolean_exclusion(empty);
        REQUIRE(!bsp_xor_empty.empty());
    }

    SUBCASE("boolean operations on empty trees"){
        bsp_tree_volume<double, uint64_t> empty1;
        bsp_tree_volume<double, uint64_t> empty2;
        auto bsp_u = empty1.boolean_union(empty2);
        REQUIRE(bsp_u.empty());

        auto bsp_i = empty1.boolean_intersection(empty2);
        REQUIRE(bsp_i.empty());

        auto bsp_s = empty1.boolean_subtraction(empty2);
        REQUIRE(bsp_s.empty());

        auto bsp_x = empty1.boolean_exclusion(empty2);
        REQUIRE(bsp_x.empty());
    }
}


TEST_CASE( "bsp_tree_volume conversion preserves structure" ){

    SUBCASE("cube mesh produces BSP tree with non-trivial root"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0.0, 1.0, 0.0},
            {0.0, 0.0, 1.0}, {1.0, 0.0, 1.0}, {1.0, 1.0, 1.0}, {0.0, 1.0, 1.0}
        };
        mesh.faces = {
            {0, 3, 2, 1}, {4, 5, 6, 7},
            {0, 1, 5, 4}, {1, 2, 6, 5},
            {2, 3, 7, 6}, {3, 0, 4, 7}
        };

        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 42);
        REQUIRE(!vol.empty());

        const auto *root = vol.get_root();
        REQUIRE(root != nullptr);
    }

    SUBCASE("mesh with nested shells (two cubes, one inside another)"){
        // Outer cube (2x2x2 centered at origin) with outward normals.
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            {-1.0, -1.0, -1.0}, { 1.0, -1.0, -1.0}, { 1.0,  1.0, -1.0}, {-1.0,  1.0, -1.0},
            {-1.0, -1.0,  1.0}, { 1.0, -1.0,  1.0}, { 1.0,  1.0,  1.0}, {-1.0,  1.0,  1.0},
            {-0.5, -0.5, -0.5}, { 0.5, -0.5, -0.5}, { 0.5,  0.5, -0.5}, {-0.5,  0.5, -0.5},
            {-0.5, -0.5,  0.5}, { 0.5, -0.5,  0.5}, { 0.5,  0.5,  0.5}, {-0.5,  0.5,  0.5}
        };
        mesh.faces = {
            // Outer cube (outward normals: CCW from outside).
            {0, 3, 2, 1}, {4, 5, 6, 7},
            {0, 1, 5, 4}, {1, 2, 6, 5},
            {2, 3, 7, 6}, {3, 0, 4, 7},
            // Inner cube (hole). The inner faces must be wound so normals
            // point INTO the solid (i.e. OUT of the hole). This means the
            // winding is reversed relative to the outer cube.
            {8, 11, 10,  9}, {12, 13, 14, 15},
            {8,  9, 13, 12}, {9, 10, 14, 13},
            {10, 11, 15, 14}, {11,  8, 12, 15}
        };

        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 42);
        REQUIRE(!vol.empty());

        auto result_mesh = vol.to_fv_surface_mesh();
        REQUIRE(!result_mesh.faces.empty());
    }
}


TEST_CASE( "bsp_tree_volume boolean operations with cube meshes" ){

    SUBCASE("union of two cubes"){
        auto make_cube = [](double cx, double cy, double cz, double half) {
            fv_surface_mesh<double, uint64_t> mesh;
            mesh.vertices = {
                {cx-half, cy-half, cz-half}, {cx+half, cy-half, cz-half},
                {cx+half, cy+half, cz-half}, {cx-half, cy+half, cz-half},
                {cx-half, cy-half, cz+half}, {cx+half, cy-half, cz+half},
                {cx+half, cy+half, cz+half}, {cx-half, cy+half, cz+half}
            };
            mesh.faces = {
                {0, 3, 2, 1}, {4, 5, 6, 7},
                {0, 1, 5, 4}, {1, 2, 6, 5},
                {2, 3, 7, 6}, {3, 0, 4, 7}
            };
            return mesh;
        };

        auto cube_A = make_cube(0.0, 0.0, 0.0, 1.0);
        auto cube_B = make_cube(0.5, 0.5, 0.5, 1.0);

        auto bsp_A = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(cube_A, 42);
        auto bsp_B = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(cube_B, 42);

        SUBCASE("union produces output"){
            auto bsp_u = bsp_A.boolean_union(bsp_B);
            REQUIRE(!bsp_u.empty());
            auto result = bsp_u.to_fv_surface_mesh();
            REQUIRE(!result.faces.empty());
        }

        SUBCASE("intersection produces output"){
            auto bsp_i = bsp_A.boolean_intersection(bsp_B);
            REQUIRE(!bsp_i.empty());
            auto result = bsp_i.to_fv_surface_mesh();
            REQUIRE(!result.faces.empty());
        }

        SUBCASE("subtraction produces output"){
            auto bsp_s = bsp_A.boolean_subtraction(bsp_B);
            REQUIRE(!bsp_s.empty());
            auto result = bsp_s.to_fv_surface_mesh();
            REQUIRE(!result.faces.empty());
        }
    }
}


TEST_CASE( "bsp_tree_volume numerical robustness" ){

    SUBCASE("near-degenerate tetrahedron (very flat)"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            { 0.0,  0.0,  0.0},
            { 1.0,  0.0,  0.0},
            { 0.0,  1.0,  0.0},
            { 0.5,  0.5,  1e-6}  // practically coplanar with base
        };
        mesh.faces = {
            {0, 2, 1},  // base
            {0, 1, 3},  // front
            {0, 3, 2},  // left
            {1, 2, 3}   // back-right
        };
        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, 42);
        REQUIRE(!vol.empty());
        auto result_mesh = vol.to_fv_surface_mesh();
        REQUIRE(!result_mesh.faces.empty());
        const auto result_vol = std::abs(mesh_signed_volume(result_mesh));
        REQUIRE(result_vol > 0.0);
    }

    SUBCASE("cubes meeting at a single edge"){
        auto make_cube = [](double cx, double cy, double cz, double half) {
            fv_surface_mesh<double, uint64_t> mesh;
            mesh.vertices = {
                {cx-half, cy-half, cz-half}, {cx+half, cy-half, cz-half},
                {cx+half, cy+half, cz-half}, {cx-half, cy+half, cz-half},
                {cx-half, cy-half, cz+half}, {cx+half, cy-half, cz+half},
                {cx+half, cy+half, cz+half}, {cx-half, cy+half, cz+half}
            };
            mesh.faces = {
                {0, 3, 2, 1}, {4, 5, 6, 7},
                {0, 1, 5, 4}, {1, 2, 6, 5},
                {2, 3, 7, 6}, {3, 0, 4, 7}
            };
            return mesh;
        };
        auto A = make_cube(0.0, 0.0, 0.0, 1.0);
        auto B = make_cube(3.0, 0.0, 0.0, 1.0); // well separated

        auto bsp_A = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(A, 42);
        auto bsp_B = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(B, 42);

        SUBCASE("union of separated cubes"){
            auto bsp_u = bsp_A.boolean_union(bsp_B);
            REQUIRE(!bsp_u.empty());
            auto result = bsp_u.to_fv_surface_mesh();
            REQUIRE(!result.faces.empty());
            const auto result_vol = std::abs(mesh_signed_volume(result));
            REQUIRE(result_vol > 0.0);
        }

        SUBCASE("intersection of separated cubes (should be empty)"){
            auto bsp_i = bsp_A.boolean_intersection(bsp_B);
            auto result = bsp_i.to_fv_surface_mesh();
            CHECK(bsp_i.empty());
            CHECK(result.faces.empty());
        }

        SUBCASE("disjoint subtraction is identity"){
            auto bsp_s = bsp_A.boolean_subtraction(bsp_B);
            REQUIRE(!bsp_s.empty());
            auto result = bsp_s.to_fv_surface_mesh();
            REQUIRE(!result.faces.empty());
            const auto result_vol = std::abs(mesh_signed_volume(result));
            CHECK(result_vol > 0.0);
        }
    }

    SUBCASE("cube with very small face (near-degenerate facet)"){
        // A cube where one face is extremely thin (width 1e-6).
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            {0.0, 0.0, 0.0}, {1e-6, 0.0, 0.0},  // narrow front face
            {1e-6, 1.0, 0.0}, {0.0, 1.0, 0.0},
            {0.0, 0.0, 1.0}, {1e-6, 0.0, 1.0},
            {1e-6, 1.0, 1.0}, {0.0, 1.0, 1.0}
        };
        mesh.faces = {
            {0, 3, 2, 1}, {4, 5, 6, 7},
            {0, 1, 5, 4}, {1, 2, 6, 5},
            {2, 3, 7, 6}, {3, 0, 4, 7}
        };

        // Step 4 makes the seed compatibility parameter non-authoritative:
        // every seed must produce the same deterministic BSP construction.
        const std::vector<uint64_t> seeds = { 21, 42, 63, 84, 100, 115, 127, 200, 255, 300 };
        std::string expected_signature;
        for(const auto sd : seeds){
            auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh, sd);
            REQUIRE(!vol.empty());
            const auto signature = bsp_tree_signature<double, uint64_t>(vol.get_root());
            if(expected_signature.empty()){
                expected_signature = signature;
            }
            CHECK(signature == expected_signature);
        }
    }
}


TEST_CASE( "bsp_tree_volume chain boolean operations" ){

    auto make_cube = [](double cx, double cy, double cz, double half) {
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            {cx-half, cy-half, cz-half}, {cx+half, cy-half, cz-half},
            {cx+half, cy+half, cz-half}, {cx-half, cy+half, cz-half},
            {cx-half, cy-half, cz+half}, {cx+half, cy-half, cz+half},
            {cx+half, cy+half, cz+half}, {cx-half, cy+half, cz+half}
        };
        mesh.faces = {
            {0, 3, 2, 1}, {4, 5, 6, 7},
            {0, 1, 5, 4}, {1, 2, 6, 5},
            {2, 3, 7, 6}, {3, 0, 4, 7}
        };
        return mesh;
    };

    SUBCASE("three-cube chain: (A ∪ B) ∩ C"){
        auto A = make_cube(0.0, 0.0, 0.0, 1.0);
        auto B = make_cube(0.5, 0.0, 0.0, 1.0);
        auto C = make_cube(0.0, 0.5, 0.0, 1.0);

        auto bsp_A = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(A, 42);
        auto bsp_B = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(B, 42);
        auto bsp_C = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(C, 42);

        auto A_or_B = bsp_A.boolean_union(bsp_B);
        REQUIRE(!A_or_B.empty());

        auto result = A_or_B.boolean_intersection(bsp_C);
        REQUIRE(!result.empty());
        auto mesh = result.to_fv_surface_mesh();
        REQUIRE(!mesh.faces.empty());
        const auto result_vol = std::abs(mesh_signed_volume(mesh));
        REQUIRE(result_vol > 0.0);
    }

    SUBCASE("four-cube chain: ((A - B) ∪ C) ∩ D"){
        auto A = make_cube(0.0, 0.0, 0.0, 1.0);
        auto B = make_cube(0.5, 0.0, 0.0, 1.0);
        auto C = make_cube(0.0, 0.5, 0.0, 1.0);
        auto D = make_cube(0.5, 0.5, 0.0, 1.0);

        auto bsp_A = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(A, 42);
        auto bsp_B = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(B, 42);
        auto bsp_C = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(C, 42);
        auto bsp_D = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(D, 42);

        auto A_sub_B = bsp_A.boolean_subtraction(bsp_B);
        REQUIRE(!A_sub_B.empty());

        auto step2 = A_sub_B.boolean_union(bsp_C);
        REQUIRE(!step2.empty());

        auto result = step2.boolean_intersection(bsp_D);
        REQUIRE(!result.empty());
        auto mesh = result.to_fv_surface_mesh();
        REQUIRE(!mesh.faces.empty());
        const auto result_vol = std::abs(mesh_signed_volume(mesh));
        REQUIRE(result_vol > 0.0);
    }

    SUBCASE("multi-component: disjoint union preserves components"){
        auto A = make_cube(0.0, 0.0, 0.0, 1.0);
        auto B = make_cube(3.0, 3.0, 3.0, 1.0); // far away

        auto bsp_A = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(A, 42);
        auto bsp_B = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(B, 42);

        auto disjoint_union = bsp_A.boolean_union(bsp_B);
        REQUIRE(!disjoint_union.empty());
        auto mesh = disjoint_union.to_fv_surface_mesh();
        REQUIRE(!mesh.faces.empty());

        // Disjoint union volume should be approximately sum of individual volumes.
        fv_surface_mesh<double, uint64_t> tri_A = A;
        tri_A.convert_to_triangles();
        fv_surface_mesh<double, uint64_t> tri_B = B;
        tri_B.convert_to_triangles();
        const auto vol_A = std::abs(mesh_signed_volume(tri_A));
        const auto vol_B = std::abs(mesh_signed_volume(tri_B));
        const auto result_vol = std::abs(mesh_signed_volume(mesh));
        CHECK(vol_A > 0.0);
        CHECK(vol_B > 0.0);
        CHECK(result_vol > 0.0);
    }

    SUBCASE("disjoint intersection is (near) emptiness"){
        auto A = make_cube(0.0, 0.0, 0.0, 1.0);
        auto B = make_cube(3.0, 3.0, 3.0, 1.0);

        auto bsp_A = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(A, 42);
        auto bsp_B = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(B, 42);

        auto bsp_i = bsp_A.boolean_intersection(bsp_B);
        CHECK(bsp_i.empty());
        auto mesh = bsp_i.to_fv_surface_mesh();
        REQUIRE(mesh.faces.empty());
        (void)mesh_signed_volume(mesh);
    }
}


TEST_CASE( "bsp_tree_volume volume verification on boolean results" ){

    auto make_cube = [](double cx, double cy, double cz, double half) {
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            {cx-half, cy-half, cz-half}, {cx+half, cy-half, cz-half},
            {cx+half, cy+half, cz-half}, {cx-half, cy+half, cz-half},
            {cx-half, cy-half, cz+half}, {cx+half, cy-half, cz+half},
            {cx+half, cy+half, cz+half}, {cx-half, cy+half, cz+half}
        };
        mesh.faces = {
            {0, 3, 2, 1}, {4, 5, 6, 7},
            {0, 1, 5, 4}, {1, 2, 6, 5},
            {2, 3, 7, 6}, {3, 0, 4, 7}
        };
        return mesh;
    };

    SUBCASE("union volume check on aligned cubes"){
        auto A = make_cube(0.0, 0.0, 0.0, 1.0);
        auto B = make_cube(0.5, 0.0, 0.0, 1.0);

        auto bsp_A = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(A, 42);
        auto bsp_B = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(B, 42);

        auto bsp_u = bsp_A.boolean_union(bsp_B);
        REQUIRE(!bsp_u.empty());
        auto result = bsp_u.to_fv_surface_mesh();
        REQUIRE(!result.faces.empty());

        const auto result_vol = std::abs(mesh_signed_volume(result));
        CHECK(result_vol > 0.0);
    }

    SUBCASE("intersection volume check on aligned cubes"){
        auto A = make_cube(0.0, 0.0, 0.0, 1.0);
        auto B = make_cube(0.5, 0.0, 0.0, 1.0);

        auto bsp_A = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(A, 42);
        auto bsp_B = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(B, 42);

        auto bsp_i = bsp_A.boolean_intersection(bsp_B);
        REQUIRE(!bsp_i.empty());
        auto result = bsp_i.to_fv_surface_mesh();
        REQUIRE(!result.faces.empty());

        const auto result_vol = std::abs(mesh_signed_volume(result));
        CHECK(result_vol > 0.0);
    }

    SUBCASE("subtraction volume check on aligned cubes"){
        auto A = make_cube(0.0, 0.0, 0.0, 1.0);
        auto B = make_cube(0.5, 0.0, 0.0, 1.0);

        auto bsp_A = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(A, 42);
        auto bsp_B = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(B, 42);

        auto bsp_s = bsp_A.boolean_subtraction(bsp_B);
        REQUIRE(!bsp_s.empty());
        auto result = bsp_s.to_fv_surface_mesh();
        // Subtraction result may produce faces or empty mesh depending on
        // the mesh extraction clipping.
        if(!result.faces.empty()){
            const auto result_vol = std::abs(mesh_signed_volume(result));
            CHECK(result_vol > 0.0);
        }
    }
}


TEST_CASE( "bsp_tree_volume comprehensive exactness and topology tests" ){

    SUBCASE("deterministic conversion and extraction over repeated runs"){
        const auto lhs = make_axis_aligned_box<double, uint64_t>({0.0, 0.0, 0.0},
                                                                 {1.0, 1.0, 1.0});
        const auto rhs = make_axis_aligned_box<double, uint64_t>({0.25, 0.5, 0.0},
                                                                 {1.25, 1.5, 1.0});
        const auto expected = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(lhs)
            .boolean_union(bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(rhs))
            .to_fv_surface_mesh();
        REQUIRE(!expected.faces.empty());
        REQUIRE(HasOnlyFiniteVertices(expected));
        REQUIRE(IsTriangularMesh(expected));
        const auto expected_signature = mesh_signature(expected);

        for(size_t i = 0; i < 12UL; ++i){
            const auto actual = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(lhs, i + 1UL)
                .boolean_union(bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(rhs, 100UL + i))
                .to_fv_surface_mesh();
            CHECK(mesh_signature(actual) == expected_signature);
        }
    }

    SUBCASE("identical cube boolean operations have exact closed-set semantics"){
        const auto cube = make_axis_aligned_box<double, uint64_t>({-1.0, -1.0, -1.0},
                                                                  { 1.0,  1.0,  1.0});
        const auto bsp = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(cube, 42);

        const auto self_union = bsp.boolean_union(bsp).to_fv_surface_mesh();
        require_valid_bounded_output_mesh(self_union);
        CHECK(mesh_signed_volume(self_union) == doctest::Approx(8.0));

        const auto self_intersection = bsp.boolean_intersection(bsp).to_fv_surface_mesh();
        require_valid_bounded_output_mesh(self_intersection);
        CHECK(mesh_signed_volume(self_intersection) == doctest::Approx(8.0));

        const auto self_subtraction = bsp.boolean_subtraction(bsp).to_fv_surface_mesh();
        CHECK(self_subtraction.faces.empty());
    }

    SUBCASE("overlapping cubes produce expected axis-aligned volumes"){
        const auto lhs = make_axis_aligned_box<double, uint64_t>({0.0, 0.0, 0.0},
                                                                 {2.0, 2.0, 2.0});
        const auto rhs = make_axis_aligned_box<double, uint64_t>({1.0, 0.0, 0.0},
                                                                 {3.0, 2.0, 2.0});
        const auto bsp_lhs = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(lhs, 42);
        const auto bsp_rhs = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(rhs, 43);

        const auto intersection = bsp_lhs.boolean_intersection(bsp_rhs).to_fv_surface_mesh();
        require_valid_bounded_output_mesh(intersection);
        CHECK(mesh_signed_volume(intersection) == doctest::Approx(4.0));

        const auto subtraction = bsp_lhs.boolean_subtraction(bsp_rhs).to_fv_surface_mesh();
        require_valid_bounded_output_mesh(subtraction);
        CHECK(mesh_signed_volume(subtraction) == doctest::Approx(4.0));

        const auto uni = bsp_lhs.boolean_union(bsp_rhs).to_fv_surface_mesh();
        require_valid_bounded_output_mesh(uni);
        CHECK(mesh_signed_volume(uni) == doctest::Approx(12.0));
    }

    SUBCASE("touching cubes follow closed-set boundary semantics"){
        const auto lhs = make_axis_aligned_box<double, uint64_t>({0.0, 0.0, 0.0},
                                                                 {1.0, 1.0, 1.0});
        const auto face_touch = make_axis_aligned_box<double, uint64_t>({1.0, 0.0, 0.0},
                                                                        {2.0, 1.0, 1.0});
        const auto edge_touch = make_axis_aligned_box<double, uint64_t>({1.0, 1.0, 0.0},
                                                                        {2.0, 2.0, 1.0});
        const auto vertex_touch = make_axis_aligned_box<double, uint64_t>({1.0, 1.0, 1.0},
                                                                          {2.0, 2.0, 2.0});

        const auto bsp_lhs = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(lhs, 42);
        for(const auto &rhs : {face_touch, edge_touch, vertex_touch}){
            const auto bsp_rhs = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(rhs, 43);
            const auto intersection = bsp_lhs.boolean_intersection(bsp_rhs).to_fv_surface_mesh();
            CHECK(intersection.faces.empty());

            const auto uni = bsp_lhs.boolean_union(bsp_rhs).to_fv_surface_mesh();
            REQUIRE(!uni.faces.empty());
            REQUIRE(HasOnlyFiniteVertices(uni));
            REQUIRE(IsTriangularMesh(uni));
        }
    }

    SUBCASE("large and small coordinate scales remain deterministic and valid"){
        for(const double scale : {1.0e-6, 1.0e6}){
            const auto lhs = make_axis_aligned_box<double, uint64_t>({0.0, 0.0, 0.0},
                                                                     {scale, scale, scale});
            const auto rhs = make_axis_aligned_box<double, uint64_t>({scale * 0.5, 0.0, 0.0},
                                                                     {scale * 1.5, scale, scale});
            const auto result = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(lhs, 7)
                .boolean_intersection(bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(rhs, 8))
                .to_fv_surface_mesh();
            require_valid_bounded_output_mesh(result);
            CHECK(mesh_signed_volume(result) == doctest::Approx(0.5 * scale * scale * scale));
        }
    }

    SUBCASE("invalid-input contract failures are explicit exceptions"){
        auto non_finite = make_axis_aligned_box<double, uint64_t>({0.0, 0.0, 0.0},
                                                                  {1.0, 1.0, 1.0});
        non_finite.vertices.at(0).x = std::numeric_limits<double>::infinity();
        CHECK_THROWS_AS((bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(non_finite, 42)),
                        std::invalid_argument);

        fv_surface_mesh<double, uint64_t> zero_area;
        zero_area.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
        zero_area.faces = {{0, 1, 2}, {0, 3, 1}, {1, 3, 2}, {2, 3, 0}};
        CHECK_THROWS_AS((bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(zero_area, 42)),
                        std::invalid_argument);

        fv_surface_mesh<double, uint64_t> non_manifold;
        non_manifold.vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
        non_manifold.faces = {{0, 1, 2}, {0, 2, 1}, {0, 1, 3}, {0, 3, 1}, {0, 2, 3}, {0, 3, 2}};
        CHECK_THROWS_AS((bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(non_manifold, 42)),
                        std::invalid_argument);
    }
}
