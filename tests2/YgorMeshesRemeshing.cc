
#include <limits>
#include <cmath>
#include <random>

#include <YgorMath.h>
#include <YgorMeshesRemeshing.h>

#include "doctest/doctest.h"


static fv_surface_mesh<double, uint32_t> create_tetrahedron(){
    fv_surface_mesh<double, uint32_t> mesh;
    mesh.vertices.push_back(vec3<double>(1, 1, 1));
    mesh.vertices.push_back(vec3<double>(1, -1, -1));
    mesh.vertices.push_back(vec3<double>(-1, 1, -1));
    mesh.vertices.push_back(vec3<double>(-1, -1, 1));
    mesh.faces.push_back({0, 1, 2});
    mesh.faces.push_back({0, 2, 3});
    mesh.faces.push_back({0, 3, 1});
    mesh.faces.push_back({1, 3, 2});
    return mesh;
}

static fv_surface_mesh<double, uint32_t> create_octahedron(){
    fv_surface_mesh<double, uint32_t> mesh;
    mesh.vertices.push_back(vec3<double>(1, 0, 0));
    mesh.vertices.push_back(vec3<double>(-1, 0, 0));
    mesh.vertices.push_back(vec3<double>(0, 1, 0));
    mesh.vertices.push_back(vec3<double>(0, -1, 0));
    mesh.vertices.push_back(vec3<double>(0, 0, 1));
    mesh.vertices.push_back(vec3<double>(0, 0, -1));
    mesh.faces.push_back({0, 2, 4});
    mesh.faces.push_back({2, 1, 4});
    mesh.faces.push_back({1, 3, 4});
    mesh.faces.push_back({3, 0, 4});
    mesh.faces.push_back({2, 0, 5});
    mesh.faces.push_back({1, 2, 5});
    mesh.faces.push_back({3, 1, 5});
    mesh.faces.push_back({0, 3, 5});
    return mesh;
}

static fv_surface_mesh<double, uint32_t> create_quad(){
    fv_surface_mesh<double, uint32_t> mesh;
    mesh.vertices.push_back(vec3<double>(0, 0, 0));
    mesh.vertices.push_back(vec3<double>(1, 0, 0));
    mesh.vertices.push_back(vec3<double>(1, 1, 0));
    mesh.vertices.push_back(vec3<double>(0, 1, 0));
    mesh.faces.push_back({0, 1, 2});
    mesh.faces.push_back({0, 2, 3});
    return mesh;
}

static fv_surface_mesh<double, uint32_t> create_icosahedron(){
    fv_surface_mesh<double, uint32_t> mesh;
    const double phi = (1.0 + std::sqrt(5.0)) / 2.0;
    mesh.vertices.push_back(vec3<double>(0, 1, phi));
    mesh.vertices.push_back(vec3<double>(0, -1, phi));
    mesh.vertices.push_back(vec3<double>(0, 1, -phi));
    mesh.vertices.push_back(vec3<double>(0, -1, -phi));
    mesh.vertices.push_back(vec3<double>(1, phi, 0));
    mesh.vertices.push_back(vec3<double>(-1, phi, 0));
    mesh.vertices.push_back(vec3<double>(1, -phi, 0));
    mesh.vertices.push_back(vec3<double>(-1, -phi, 0));
    mesh.vertices.push_back(vec3<double>(phi, 0, 1));
    mesh.vertices.push_back(vec3<double>(-phi, 0, 1));
    mesh.vertices.push_back(vec3<double>(phi, 0, -1));
    mesh.vertices.push_back(vec3<double>(-phi, 0, -1));
    mesh.faces.push_back({0, 1, 8});
    mesh.faces.push_back({0, 8, 4});
    mesh.faces.push_back({0, 4, 5});
    mesh.faces.push_back({0, 5, 9});
    mesh.faces.push_back({0, 9, 1});
    mesh.faces.push_back({1, 6, 8});
    mesh.faces.push_back({8, 6, 10});
    mesh.faces.push_back({8, 10, 4});
    mesh.faces.push_back({4, 10, 2});
    mesh.faces.push_back({4, 2, 5});
    mesh.faces.push_back({5, 2, 11});
    mesh.faces.push_back({5, 11, 9});
    mesh.faces.push_back({9, 11, 7});
    mesh.faces.push_back({9, 7, 1});
    mesh.faces.push_back({1, 7, 6});
    mesh.faces.push_back({6, 7, 3});
    mesh.faces.push_back({6, 3, 10});
    mesh.faces.push_back({10, 3, 2});
    mesh.faces.push_back({2, 3, 11});
    mesh.faces.push_back({11, 3, 7});
    return mesh;
}


TEST_CASE( "mesh_remesher construction and parameters" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("basic construction and parameter access"){
        auto mesh = create_tetrahedron();
        const double target_len = 1.0;
        mesh_remesher<double, uint32_t> remesher(mesh, target_len);

        REQUIRE( std::abs(remesher.get_target_edge_length() - target_len) < eps );
        REQUIRE( std::abs(remesher.get_max_edge_length() - 4.0/3.0 * target_len) < eps );
        REQUIRE( std::abs(remesher.get_min_edge_length() - 4.0/5.0 * target_len) < eps );
    }

    SUBCASE("parameter modification"){
        auto mesh = create_tetrahedron();
        mesh_remesher<double, uint32_t> remesher(mesh, 1.0);

        remesher.set_target_edge_length(2.0);

        REQUIRE( std::abs(remesher.get_target_edge_length() - 2.0) < eps );
        REQUIRE( std::abs(remesher.get_max_edge_length() - 4.0/3.0 * 2.0) < eps );
        REQUIRE( std::abs(remesher.get_min_edge_length() - 4.0/5.0 * 2.0) < eps );

        REQUIRE_THROWS( remesher.set_target_edge_length(-1.0) );
    }
}


TEST_CASE( "mesh_remesher edge splitting" ){

    SUBCASE("splitting increases faces and vertices"){
        auto mesh = create_quad();
        const double target_len = 0.5;
        mesh_remesher<double, uint32_t> remesher(mesh, target_len);

        const size_t original_faces = mesh.faces.size();
        const size_t original_vertices = mesh.vertices.size();

        int64_t splits = remesher.split_long_edges();

        REQUIRE( splits > 0 );
        REQUIRE( mesh.faces.size() > original_faces );
        REQUIRE( mesh.vertices.size() > original_vertices );
    }
}


TEST_CASE( "mesh_remesher edge collapsing" ){

    SUBCASE("collapsing reduces mesh"){
        fv_surface_mesh<double, uint32_t> mesh;
        mesh.vertices.push_back(vec3<double>(0, 0, 0));
        mesh.vertices.push_back(vec3<double>(1, 0, 0));
        mesh.vertices.push_back(vec3<double>(0.5, 1, 0));
        mesh.vertices.push_back(vec3<double>(0.05, 0, 0));
        mesh.faces.push_back({0, 3, 2});
        mesh.faces.push_back({3, 1, 2});

        const double target_len = 1.0;
        mesh_remesher<double, uint32_t> remesher(mesh, target_len);

        int64_t collapses = remesher.collapse_short_edges();
        REQUIRE( collapses > 0 );
    }
}


TEST_CASE( "mesh_remesher edge flipping" ){

    SUBCASE("flipping does not crash"){
        fv_surface_mesh<double, uint32_t> mesh;
        mesh.vertices.push_back(vec3<double>(0, 0, 0));
        mesh.vertices.push_back(vec3<double>(2, 0, 0));
        mesh.vertices.push_back(vec3<double>(1, 1, 0));
        mesh.vertices.push_back(vec3<double>(1, -1, 0));
        mesh.faces.push_back({0, 1, 2});
        mesh.faces.push_back({0, 3, 1});

        mesh_remesher<double, uint32_t> remesher(mesh, 1.0);
        remesher.flip_edges_for_valence();
    }
}


TEST_CASE( "mesh_remesher tangential relaxation" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("relaxation moves vertices"){
        auto mesh = create_octahedron();

        std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(-0.1, 0.1);
        for(auto &v : mesh.vertices){
            v.x += dist(rng);
            v.y += dist(rng);
            v.z += dist(rng);
        }

        mesh_remesher<double, uint32_t> remesher(mesh, 1.0);
        auto original_verts = mesh.vertices;

        remesher.tangential_relaxation(0.5);

        int changed = 0;
        for(size_t i = 0; i < mesh.vertices.size(); ++i){
            if(mesh.vertices[i].distance(original_verts[i]) > eps){
                ++changed;
            }
        }
        REQUIRE( changed > 0 );
    }
}


TEST_CASE( "mesh_remesher full remeshing iteration" ){

    SUBCASE("full iteration produces valid mesh"){
        auto mesh = create_icosahedron();
        for(auto &v : mesh.vertices){
            v *= 3.0;
        }

        mesh_remesher<double, uint32_t> remesher(mesh, 1.5);

        for(int iter = 0; iter < 3; ++iter){
            remesher.remesh_iteration();
        }

        for(const auto &face : mesh.faces){
            for(const auto &v : face){
                REQUIRE( v < mesh.vertices.size() );
            }
        }
    }
}


TEST_CASE( "mesh_remesher aspect ratio" ){

    SUBCASE("aspect ratios are positive"){
        auto mesh = create_tetrahedron();
        mesh_remesher<double, uint32_t> remesher(mesh, 1.0);

        auto [min_ar, max_ar] = remesher.aspect_ratio_range();
        REQUIRE( min_ar > 0 );
        REQUIRE( max_ar > 0 );
    }
}


TEST_CASE( "mesh_remesher validity after multiple operations" ){

    SUBCASE("mesh remains valid after many iterations"){
        auto mesh = create_octahedron();
        for(auto &v : mesh.vertices){
            v *= 5.0;
        }

        mesh_remesher<double, uint32_t> remesher(mesh, 1.0);

        for(int i = 0; i < 5; ++i){
            remesher.remesh_iteration();

            for(const auto &face : mesh.faces){
                REQUIRE( face.size() >= 3 );
                for(const auto &v : face){
                    REQUIRE( v < mesh.vertices.size() );
                }
            }
        }
    }
}
