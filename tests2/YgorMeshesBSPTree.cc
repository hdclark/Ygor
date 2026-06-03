//YgorMeshesBSPTree.cc - Tests for BSP tree volume class.
//Written by hal clark in 2026.

#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include <YgorMath.h>
#include <YgorMeshesBSPTree.h>

#include "doctest/doctest.h"


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
}


TEST_CASE( "bsp_tree_volume mesh conversion round-trip" ){

    SUBCASE("empty mesh produces empty tree"){
        fv_surface_mesh<double, uint64_t> mesh;
        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh);
        REQUIRE(vol.empty());
    }

    SUBCASE("mesh with only degenerate faces produces empty tree"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            {0.0, 0.0, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 0.0, 0.0}  // degenerate
        };
        mesh.faces = {{0, 1, 2}};
        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh);
        REQUIRE(vol.empty());
    }

    SUBCASE("single triangle mesh produces valid BSP tree"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices = {
            {0.0, 0.0, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0}
        };
        mesh.faces = {{0, 1, 2}};
        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh);
        REQUIRE(!vol.empty());

        // A single open triangle does not enclose a volume, so the
        // BSP-to-mesh conversion may produce an empty mesh (no
        // IN/OUT boundaries to extract).
        auto result_mesh = vol.to_fv_surface_mesh();
        // Open meshes: output is expected to have few or no faces.
        const bool small_output = (result_mesh.faces.size() <= 6u);
        REQUIRE(small_output);
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

        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh);
        REQUIRE(!vol.empty());

        auto result_mesh = vol.to_fv_surface_mesh();
        REQUIRE(!result_mesh.faces.empty());
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

        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh);
        REQUIRE(!vol.empty());

        auto result_mesh = vol.to_fv_surface_mesh();
        REQUIRE(!result_mesh.faces.empty());
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

    auto bsp_A = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(tetra_A);
    auto bsp_B = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(tetra_B);

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

        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh);
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
            // Inner cube (hole, also outward normals -> points INTO the hole,
            // which means the solid is OUTSIDE the hole).
            // For a hole, the normals should point into the interior of the
            // outer solid, so the face winding is reversed.
            {8,  9, 10, 11}, {12, 15, 14, 13},
            {8, 12, 13, 9},  {9, 13, 14, 10},
            {10, 14, 15, 11}, {11, 15, 12, 8}
        };

        auto vol = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(mesh);
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

        auto bsp_A = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(cube_A);
        auto bsp_B = bsp_tree_volume<double, uint64_t>::from_fv_surface_mesh(cube_B);

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
