
#include <limits>
#include <cmath>
#include <random>

#include <YgorMisc.h>
#include <YgorLog.h>
#include <YgorMath.h>
#include <YgorMath_Samples.h>
#include <YgorMeshesRemeshing.h>

#include "doctest/doctest.h"


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


TEST_CASE( "mesh_remesher construction and parameters" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("basic construction and parameter access"){
        auto mesh = fv_surface_mesh_tetrahedron();
        const double target_len = 1.0;
        mesh_remesher<double, uint64_t> remesher(mesh, target_len);

        REQUIRE( std::abs(remesher.get_target_edge_length() - target_len) < eps );
        REQUIRE( std::abs(remesher.get_max_edge_length() - 4.0/3.0 * target_len) < eps );
        REQUIRE( std::abs(remesher.get_min_edge_length() - 4.0/5.0 * target_len) < eps );
    }

    SUBCASE("parameter modification"){
        auto mesh = fv_surface_mesh_tetrahedron();
        const double initial_target_len = 1.0;
        mesh_remesher<double, uint64_t> remesher(mesh, initial_target_len);

        const double target_len = 2.0;
        remesher.set_target_edge_length(target_len);

        REQUIRE( std::abs(remesher.get_target_edge_length() - target_len) < eps );
        REQUIRE( std::abs(remesher.get_max_edge_length() - target_len * 4.0/3.0) < eps );
        REQUIRE( std::abs(remesher.get_min_edge_length() - target_len * 4.0/5.0) < eps );

        REQUIRE_THROWS( remesher.set_target_edge_length(-1.0) );
    }
}

TEST_CASE("mesh_remesher quality metrics"){
    auto mesh = fv_surface_mesh_single_quad();
    const double target_len = 1.0;
    mesh_remesher<double, uint64_t> remesher(mesh, target_len);
    
    double mean_len = remesher.mean_edge_length();
    REQUIRE(mean_len > 0.0);

    double stddev = remesher.edge_length_stddev();
    REQUIRE(stddev > 0.0);
}


TEST_CASE( "mesh_remesher edge splitting" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("splitting increases faces and vertices"){
        auto mesh = fv_surface_mesh_single_quad();

        // Use a target edge length that will cause edges to be split.
        // The diagonal of the quad is sqrt(2), which is longer than 4/3 * 0.5 = 0.667
        const double target_len = 0.5;
        mesh_remesher<double, uint64_t> remesher(mesh, target_len);

        const size_t original_faces = mesh.faces.size();
        const size_t original_vertices = mesh.vertices.size();

        int64_t splits = remesher.split_long_edges();

        REQUIRE( splits > 0 );
        REQUIRE( mesh.faces.size() > original_faces );
        REQUIRE( mesh.vertices.size() > original_vertices );
        REQUIRE( verify_mesh_integrity(mesh) );

        // Verify all edges are now within bounds.
        double max_edge = remesher.get_max_edge_length();
        double max_found = 0.0;
        for(const auto &face : mesh.faces){
            if(face.size() < 3) continue;
            for(size_t i = 0; i < face.size(); ++i){
                size_t j = (i + 1) % face.size();
                double len = mesh.vertices[face[i]].distance(mesh.vertices[face[j]]);
                max_found = std::max(max_found, len);
            }
        }
        REQUIRE( max_found < max_edge );
    }
}


TEST_CASE( "mesh_remesher edge collapsing" ){

    SUBCASE("collapsing reduces mesh"){
        // Create a mesh with some short edges
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices.push_back(vec3<double>(0, 0, 0));
        mesh.vertices.push_back(vec3<double>(1, 0, 0));
        mesh.vertices.push_back(vec3<double>(0.5, 1, 0));
        mesh.vertices.push_back(vec3<double>(0.05, 0, 0)); // close to vertex 0.
        mesh.faces.push_back({0, 3, 2});
        mesh.faces.push_back({3, 1, 2});

        const double target_len = 1.0; // min_len = 0.8, and edge 0-3 is 0.05.
        mesh_remesher<double, uint64_t> remesher(mesh, target_len);

        int64_t collapses = remesher.collapse_short_edges();
        REQUIRE( collapses > 0 );
        REQUIRE( verify_mesh_integrity(mesh) );
    }
}


TEST_CASE( "mesh_remesher edge flipping" ){

    SUBCASE("flipping preserves a valid mesh and does not crash"){
        // Create a configuration where flipping would improve valence.
        // A "bowtie" configuration: two triangles sharing an edge where opposite vertices
        // have high valence.
        fv_surface_mesh<double, uint64_t> mesh;
        mesh.vertices.push_back(vec3<double>(0, 0, 0));  // 0
        mesh.vertices.push_back(vec3<double>(2, 0, 0));  // 1
        mesh.vertices.push_back(vec3<double>(1, 1, 0));  // 2
        mesh.vertices.push_back(vec3<double>(1, -1, 0)); // 3

        // Triangles share edge 0-1.
        mesh.faces.push_back({0, 1, 2});
        mesh.faces.push_back({0, 3, 1});

        mesh_remesher<double, uint64_t> remesher(mesh, 1.0);

        const double initial_deviation = remesher.valence_deviation();
        const int64_t flips = remesher.flip_edges_for_valence();
        const double final_deviation = remesher.valence_deviation();

        // Verify valence was not worsened.
        REQUIRE(final_deviation <= initial_deviation);

        // Ensure that edge flipping did not corrupt the mesh topology.
        REQUIRE( verify_mesh_integrity(mesh) );
    }
}


TEST_CASE( "mesh_remesher tangential relaxation" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("relaxation moves vertices"){
        auto mesh = fv_surface_mesh_octahedron();

        std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(-0.1, 0.1);
        for(auto &v : mesh.vertices){
            v.x += dist(rng);
            v.y += dist(rng);
            v.z += dist(rng);
        }

        mesh_remesher<double, uint64_t> remesher(mesh, 1.0);
        auto original_verts = mesh.vertices;

        int64_t moved = remesher.tangential_relaxation(0.5);

        int changed = 0;
        for(size_t i = 0; i < mesh.vertices.size(); ++i){
            if(mesh.vertices[i].distance(original_verts[i]) > eps){
                ++changed;
            }
        }
        REQUIRE( changed > 0 );
        REQUIRE( moved == changed );
        REQUIRE( verify_mesh_integrity(mesh) );
    }
}


TEST_CASE( "mesh_remesher full remeshing iteration" ){

    SUBCASE("full iteration produces valid mesh"){
        auto mesh = fv_surface_mesh_icosahedron();
        // Scale the entire mesh.
        for(auto &v : mesh.vertices){
            v *= 3.0;
        }

        mesh_remesher<double, uint64_t> remesher(mesh, 1.5);

        for(int iter = 0; iter < 3; ++iter){
            remesher.remesh_iteration();
        }

        for(const auto &face : mesh.faces){
            for(const auto &v : face){
                REQUIRE( v < mesh.vertices.size() );
            }
        }
        REQUIRE( verify_mesh_integrity(mesh) );
    }
}


TEST_CASE( "mesh_remesher aspect ratio" ){

    SUBCASE("aspect ratios are positive"){
        auto mesh = fv_surface_mesh_tetrahedron();
        mesh_remesher<double, uint64_t> remesher(mesh, 1.0);

        auto [min_ar, max_ar] = remesher.aspect_ratio_range();
        REQUIRE( min_ar > 0 );
        REQUIRE( max_ar > 0 );
        REQUIRE( verify_mesh_integrity(mesh) );
    }
}


TEST_CASE( "mesh_remesher validity after multiple operations" ){

    SUBCASE("mesh remains valid after many iterations"){
        auto mesh = fv_surface_mesh_octahedron();
        for(auto &v : mesh.vertices){
            v *= 5.0;
        }

        mesh_remesher<double, uint64_t> remesher(mesh, 1.0);

        for(int i = 0; i < 5; ++i){
            remesher.remesh_iteration();

            for(const auto &face : mesh.faces){
                REQUIRE( face.size() >= 3 );
                for(const auto &v : face){
                    REQUIRE( v < mesh.vertices.size() );
                }
            }
        }
        REQUIRE( verify_mesh_integrity(mesh) );
    }
}

TEST_CASE("mesh_remesher vertex attributes (normals and colours) honoured during edge splitting"){
    auto mesh = fv_surface_mesh_single_quad();

    // Add vertex normals.
    mesh.vertex_normals.resize(mesh.vertices.size());
    for(size_t i = 0; i < mesh.vertices.size(); ++i) {
        mesh.vertex_normals[i] = vec3<double>(0, 0, 1);  // All pointing up.
    }

    // Add vertex colours (using pack_RGBA32_colour).
    std::array<uint8_t, 4> colour = {255, 138, 13, 255};
    const auto packed_colour = mesh.pack_RGBA32_colour(colour);

    mesh.vertex_colours.resize(mesh.vertices.size());
    for(size_t i = 0; i < mesh.vertices.size(); ++i){
        mesh.vertex_colours[i] = packed_colour;
    }

    // Use a small target edge length to trigger splitting.
    mesh_remesher<double, uint64_t> remesher(mesh, 0.5);
    int64_t splits = remesher.split_long_edges();

    REQUIRE(splits > 0); // At least one edge should be split.

    // Verify new normals have valid values (should still point roughly up).
    for(const auto &n : mesh.vertex_normals) {
        REQUIRE(n.z > 0.9); // Normals should still be mostly pointing up.
    }

    // Verify colours remain consistent.
    for(size_t i = 0; i < mesh.vertices.size(); ++i){
        const auto c = mesh.vertex_colours[i];
        REQUIRE(c == packed_colour);
    }
}

