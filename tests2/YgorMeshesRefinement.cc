
#include <cmath>
#include <sstream>
#include <string>
#include <set>
#include <stdexcept>

#include <YgorMath.h>
#include <YgorMath_Samples.h>
#include <YgorMisc.h>
#include <YgorLog.h>
#include <YgorMathIOOFF.h>
#include <YgorMeshesRefinement.h>

#include "doctest/doctest.h"


static size_t count_edges(const fv_surface_mesh<double, uint64_t> &mesh){
    std::set<std::pair<uint64_t, uint64_t>> edges;
    for(const auto &face : mesh.faces){
        for(size_t i = 0; i < face.size(); ++i){
            uint64_t v0 = face[i];
            uint64_t v1 = face[(i + 1) % face.size()];
            if(v0 > v1) std::swap(v0, v1);
            edges.insert({v0, v1});
        }
    }
    return edges.size();
}

// Verify that all faces in the mesh are valid triangles with valid vertex indices.
static bool verify_mesh_integrity(const fv_surface_mesh<double, uint64_t> &mesh) {
    const size_t N_verts = mesh.vertices.size();

    for(size_t f_idx = 0; f_idx < mesh.faces.size(); ++f_idx){
        const auto &face = mesh.faces[f_idx];
        if(face.size() != 3){
            YLOGWARN("Face " << f_idx << " has " << face.size() << " vertices (expected 3).");
            return false;
        }
        for(const auto &v : face){
            if(v >= N_verts){
                YLOGWARN("Face " << f_idx << " references invalid vertex index " << v
                         << " (mesh has " << N_verts << " vertices).");
                return false;
            }
        }
    }

    // Verify involved_faces index is consistent.
    if(!mesh.involved_faces.empty()){
        if(mesh.involved_faces.size() != mesh.vertices.size()){
            YLOGWARN("involved_faces size mismatch.");
            return false;
        }
        for(size_t v_idx = 0; v_idx < mesh.involved_faces.size(); ++v_idx){
            for(const auto &f_idx : mesh.involved_faces[v_idx]){
                if(f_idx >= mesh.faces.size()){
                    YLOGWARN("involved_faces[" << v_idx << "] references invalid face " << f_idx);
                    return false;
                }
                // Check that the face actually contains this vertex.
                const auto &face = mesh.faces[f_idx];
                bool found = false;
                for(const auto &v : face){
                    if(v == v_idx) found = true;
                }
                if(!found){
                    YLOGWARN("involved_faces[" << v_idx << "] references face " << f_idx
                             << " but that face doesn't contain vertex " << v_idx);
                    return false;
                }
            }
        }
    }

    return true;
}

TEST_CASE( "loop_subdivide" ){
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

    SUBCASE("single triangle: V'=V+E, F'=4F"){
        auto mesh = fv_surface_mesh_single_triangle();
        const size_t V_before = mesh.vertices.size();
        const size_t F_before = mesh.faces.size();
        const size_t E_before = count_edges(mesh);

        loop_subdivide(mesh, 1);

        REQUIRE( mesh.vertices.size() == V_before + E_before );
        REQUIRE( mesh.faces.size() == 4 * F_before );
        REQUIRE( verify_mesh_integrity(mesh) );
    }

    SUBCASE("tetrahedron: V'=10, F'=16"){
        auto mesh = fv_surface_mesh_tetrahedron();
        loop_subdivide(mesh, 1);

        // Tetrahedron has 4 vertices, 4 faces, 6 edges.
        // After subdivision: V' = 4 + 6 = 10, F' = 4 * 4 = 16.
        REQUIRE( mesh.vertices.size() == 10 );
        REQUIRE( mesh.faces.size() == 16 );
        REQUIRE( verify_mesh_integrity(mesh) );
    }

    SUBCASE("icosahedron: V'=42, F'=80"){
        auto mesh = fv_surface_mesh_icosahedron();
        REQUIRE( mesh.vertices.size() == 12 );
        REQUIRE( mesh.faces.size() == 20 );
        REQUIRE( count_edges(mesh) == 30 );

        loop_subdivide(mesh, 1);

        // After subdivision: V' = 12 + 30 = 42, F' = 4 * 20 = 80.
        REQUIRE( mesh.vertices.size() == 42 );
        REQUIRE( mesh.faces.size() == 80 );
        REQUIRE( verify_mesh_integrity(mesh) );
    }

    SUBCASE("2 iterations of tetrahedron: V'=34, F'=64"){
        auto mesh = fv_surface_mesh_tetrahedron();
        loop_subdivide(mesh, 2);

        // After iteration 1: V_1 = V_0 + E_0 = 4 + 6 = 10, F_1 = 4 * F_0 = 16.
        // E_1 can be computed from Euler: E_1 = V_1 + F_1 - 2 = 10 + 16 - 2 = 24 (for closed surface).
        // After iteration 2: V_2 = V_1 + E_1 = 10 + 24 = 34, F_2 = 4 * F_1 = 64.
        REQUIRE( mesh.vertices.size() == 34 );
        REQUIRE( mesh.faces.size() == 64 );
        REQUIRE( verify_mesh_integrity(mesh) );
    }

    SUBCASE("0 iterations is a no-op"){
        auto mesh = fv_surface_mesh_tetrahedron();
        const size_t V_before = mesh.vertices.size();
        const size_t F_before = mesh.faces.size();

        loop_subdivide(mesh, 0);

        REQUIRE( mesh.vertices.size() == V_before );
        REQUIRE( mesh.faces.size() == F_before );
    }

    SUBCASE("negative iterations is a no-op"){
        auto mesh = fv_surface_mesh_tetrahedron();
        const size_t V_before = mesh.vertices.size();
        const size_t F_before = mesh.faces.size();

        loop_subdivide(mesh, -1);

        REQUIRE( mesh.vertices.size() == V_before );
        REQUIRE( mesh.faces.size() == F_before );
    }

    SUBCASE("non-triangle mesh should throw"){
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices.emplace_back(vec3<double>(0, 0, 0));
        mesh.vertices.emplace_back(vec3<double>(1, 0, 0));
        mesh.vertices.emplace_back(vec3<double>(1, 1, 0));
        mesh.vertices.emplace_back(vec3<double>(0, 1, 0));
        mesh.faces.push_back({0, 1, 2, 3}); // quad, not triangle
        mesh.recreate_involved_face_index();

        REQUIRE_THROWS( loop_subdivide(mesh, 1) );
    }

    SUBCASE("vertex normals preserved"){
        auto mesh = fv_surface_mesh_tetrahedron();
        for(const auto &v : mesh.vertices){
            mesh.vertex_normals.emplace_back(v.unit());
        }

        loop_subdivide(mesh, 1);

        REQUIRE( mesh.vertex_normals.size() == mesh.vertices.size() );
        for(const auto &n : mesh.vertex_normals){
            REQUIRE( n.length() > 0.9 );
            REQUIRE( n.length() < 1.1 );
        }
        REQUIRE( verify_mesh_integrity(mesh) );
    }

    SUBCASE("vertex colours preserved"){
        auto mesh = fv_surface_mesh_tetrahedron();
        mesh.vertex_colours.push_back(mesh.pack_RGBA32_colour({255, 0, 0, 255})); // Red
        mesh.vertex_colours.push_back(mesh.pack_RGBA32_colour({0, 255, 0, 255})); // Green 
        mesh.vertex_colours.push_back(mesh.pack_RGBA32_colour({0, 0, 255, 255})); // Blue
        mesh.vertex_colours.push_back(mesh.pack_RGBA32_colour({255, 255, 0, 255})); // Yellow

        loop_subdivide(mesh, 1);

        REQUIRE( mesh.vertex_colours.size() == mesh.vertices.size() );

        // Verify colours can be unpacked without error.
        for(const auto &c : mesh.vertex_colours){
            // Just verify it doesn't crash and alpha is reasonable.
            auto unpacked = mesh.unpack_RGBA32_colour(c);
            REQUIRE(unpacked.at(3) == 255);
        }

        REQUIRE( verify_mesh_integrity(mesh) );
    }

    SUBCASE("involved_faces index matches recreate after 1 iteration"){
        auto mesh = fv_surface_mesh_tetrahedron();
        loop_subdivide(mesh, 1);

        REQUIRE( !mesh.involved_faces.empty() );
        REQUIRE( mesh.involved_faces.size() == mesh.vertices.size() );

        // Compare with a full rebuild to confirm index correctness.
        require_involved_faces_matches_recreate(mesh);
    }

    SUBCASE("involved_faces index matches recreate after 2 iterations"){
        auto mesh = fv_surface_mesh_tetrahedron();
        loop_subdivide(mesh, 2);

        REQUIRE( !mesh.involved_faces.empty() );
        REQUIRE( mesh.involved_faces.size() == mesh.vertices.size() );

        require_involved_faces_matches_recreate(mesh);
    }

    SUBCASE("involved_faces index matches recreate for icosahedron"){
        auto mesh = fv_surface_mesh_icosahedron();
        loop_subdivide(mesh, 1);

        REQUIRE( !mesh.involved_faces.empty() );
        REQUIRE( mesh.involved_faces.size() == mesh.vertices.size() );

        require_involved_faces_matches_recreate(mesh);
    }
}
