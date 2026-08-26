
#include <limits>
#include <utility>
#include <iostream>
#include <random>
#include <cstdint>
#include <algorithm>

#include <YgorMath.h>
#include <YgorMeshesHoles.h>

#include "doctest/doctest.h"


TEST_CASE( "YgorMeshesHoles" ){
    const vec3<double> p1(1.0, 0.0, 0.0);
    const vec3<double> p2(0.0, 1.0, 0.0);
    const vec3<double> p3(0.0, 0.0, 1.0);
    const vec3<double> p4(0.0, 0.0, 0.0);
    const vec3<double> p5(1.0, 0.0, 1.0);
    const vec3<double> p6(1.0, 1.0, 0.0);
    const auto require_involved_faces_matches_recreate = [](const auto &mesh){
        auto reference = mesh;
        reference.recreate_involved_face_index();
        for(size_t v = 0; v < mesh.vertices.size(); ++v){
            auto got = mesh.involved_faces[v];
            auto expected = reference.involved_faces[v];
            std::sort(got.begin(), got.end());
            std::sort(expected.begin(), expected.end());
            REQUIRE(got == expected);
        }
    };

    SUBCASE("FindBoundaryChains and FillBoundaryChainsByZippering"){
        fv_surface_mesh<double, uint32_t> mesh2;
        mesh2.vertices = {{ p1, p2, p3, p4 }};
        mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) },
                       { static_cast<uint32_t>(0), static_cast<uint32_t>(3), static_cast<uint32_t>(1) },
                       { static_cast<uint32_t>(1), static_cast<uint32_t>(3), static_cast<uint32_t>(2) }};

        const auto holes = FindBoundaryChains(mesh2);
        REQUIRE(holes.has_nonmanifold_edges == false);
        REQUIRE(holes.chains.size() == 1UL);
        REQUIRE(holes.chains.front().is_closed == true);
        REQUIRE(holes.chains.front().vertices.size() == 3UL);

        REQUIRE(FillBoundaryChainsByZippering(mesh2, holes));
        REQUIRE(mesh2.faces.size() == 4UL);
    }

    SUBCASE("FillBoundaryChainsByZippering updates involved_faces via diff"){
        // Build a tetrahedron missing one face (open mesh with a hole).
        fv_surface_mesh<double, uint32_t> mesh2;
        mesh2.vertices = {{ p1, p2, p3, p4 }};
        mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) },
                       { static_cast<uint32_t>(0), static_cast<uint32_t>(3), static_cast<uint32_t>(1) },
                       { static_cast<uint32_t>(1), static_cast<uint32_t>(3), static_cast<uint32_t>(2) }};

        // Pre-build the index so the diff path is exercised.
        mesh2.recreate_involved_face_index();
        REQUIRE(mesh2.involved_faces.size() == mesh2.vertices.size());

        const auto holes = FindBoundaryChains(mesh2);
        REQUIRE(FillBoundaryChainsByZippering(mesh2, holes));
        REQUIRE(mesh2.faces.size() == 4UL);

        // The involved_faces index should have been updated by the diff.
        REQUIRE(mesh2.involved_faces.size() == mesh2.vertices.size());

        // Compare with a full rebuild to confirm correctness.
        require_involved_faces_matches_recreate(mesh2);
    }

    SUBCASE("FillBoundaryChainsByZippering rebuilds when index absent"){
        fv_surface_mesh<double, uint32_t> mesh2;
        mesh2.vertices = {{ p1, p2, p3, p4 }};
        mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) },
                       { static_cast<uint32_t>(0), static_cast<uint32_t>(3), static_cast<uint32_t>(1) },
                       { static_cast<uint32_t>(1), static_cast<uint32_t>(3), static_cast<uint32_t>(2) }};

        // Do NOT pre-build the index; the fallback rebuild path should be used.
        REQUIRE(mesh2.involved_faces.empty());

        const auto holes = FindBoundaryChains(mesh2);
        REQUIRE(FillBoundaryChainsByZippering(mesh2, holes));
        REQUIRE(mesh2.faces.size() == 4UL);
        REQUIRE(mesh2.involved_faces.size() == mesh2.vertices.size());

        // Verify the rebuilt index matches a fresh recreate.
        require_involved_faces_matches_recreate(mesh2);
    }

    SUBCASE("EnsureConsistentFaceOrientation"){
        fv_surface_mesh<double, uint32_t> mesh2;
        mesh2.vertices = {{ p1, p2, p3, p4 }};
        mesh2.faces = {{ static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2) },
                       { static_cast<uint32_t>(0), static_cast<uint32_t>(3), static_cast<uint32_t>(1) },
                       { static_cast<uint32_t>(1), static_cast<uint32_t>(3), static_cast<uint32_t>(2) },
                       { static_cast<uint32_t>(0), static_cast<uint32_t>(2), static_cast<uint32_t>(3) }}; // Flipped face.

        int64_t genus = -1;
        REQUIRE(EnsureConsistentFaceOrientation(mesh2, 1.0E-6, &genus));
        REQUIRE(genus == 0);

        std::map<std::pair<uint32_t,uint32_t>, std::vector<std::pair<uint32_t,uint32_t>>> edges;
        for(uint32_t f = 0UL; f < mesh2.faces.size(); ++f){
            const auto &face = mesh2.faces[f];
            for(uint32_t i = 0UL; i < face.size(); ++i){
                const auto v_a = face[i];
                const auto v_b = face[(i + 1UL) % face.size()];
                edges[{ std::min(v_a, v_b), std::max(v_a, v_b) }].push_back({ f, (v_a < v_b) ? 1U : 0U });
            }
        }
        for(const auto &ep : edges){
            if(ep.second.size() != 2UL) continue;
            REQUIRE(ep.second[0].second != ep.second[1].second);
        }
    }

    SUBCASE("EnsureConsistentFaceOrientation edge cases"){
        // Non-manifold edge: three faces sharing the same undirected edge (0,1).
        fv_surface_mesh<double, uint32_t> non_manifold_mesh;
        non_manifold_mesh.vertices = {{ p1, p2, p3, p4, p5 }};
        non_manifold_mesh.faces = {{
            static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2)
        }, {
            static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(3)
        }, {
            static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(4)
        }};

        int64_t genus_non_manifold = -1;
        REQUIRE(EnsureConsistentFaceOrientation(non_manifold_mesh, 1.0E-6, &genus_non_manifold) == false);
        REQUIRE(genus_non_manifold == -1);

        // Multiple disconnected components: two separate triangles sharing no vertices.
        fv_surface_mesh<double, uint32_t> multi_component_mesh;
        multi_component_mesh.vertices = {{ p1, p2, p3, p4, p5, p6 }};
        multi_component_mesh.faces = {{
            static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2)
        }, {
            static_cast<uint32_t>(3), static_cast<uint32_t>(4), static_cast<uint32_t>(5)
        }};

        int64_t genus_multi_component = -1;
        REQUIRE(EnsureConsistentFaceOrientation(multi_component_mesh, 1.0E-6, &genus_multi_component));
        REQUIRE(genus_multi_component == 0);

        // Excessively large duplicate-vertex tolerance can collapse topology and
        // make genus estimation invalid; the routine should throw with guidance.
        fv_surface_mesh<double, uint32_t> invalid_genus_mesh;
        invalid_genus_mesh.vertices = {{ p1, p2, p3, p4 }};
        invalid_genus_mesh.faces = {{
            static_cast<uint32_t>(0), static_cast<uint32_t>(1), static_cast<uint32_t>(2)
        }, {
            static_cast<uint32_t>(0), static_cast<uint32_t>(3), static_cast<uint32_t>(1)
        }, {
            static_cast<uint32_t>(1), static_cast<uint32_t>(3), static_cast<uint32_t>(2)
        }, {
            static_cast<uint32_t>(0), static_cast<uint32_t>(2), static_cast<uint32_t>(3)
        }};
        int64_t invalid_genus = -1;
        REQUIRE_THROWS(EnsureConsistentFaceOrientation(invalid_genus_mesh, 10.0, &invalid_genus));
    }
}
