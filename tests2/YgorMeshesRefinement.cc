
#include <cmath>
#include <sstream>
#include <string>
#include <set>
#include <stdexcept>

#include <YgorMath.h>
#include <YgorMathIOOFF.h>
#include <YgorMeshesRefinement.h>

#include "doctest/doctest.h"


static fv_surface_mesh<double, uint64_t> create_tetrahedron(){
    fv_surface_mesh<double, uint64_t> mesh;
    mesh.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));
    mesh.vertices.emplace_back(vec3<double>(1.0, -1.0, -1.0));
    mesh.vertices.emplace_back(vec3<double>(-1.0, 1.0, -1.0));
    mesh.vertices.emplace_back(vec3<double>(-1.0, -1.0, 1.0));
    mesh.faces.push_back({0, 1, 2});
    mesh.faces.push_back({0, 3, 1});
    mesh.faces.push_back({0, 2, 3});
    mesh.faces.push_back({1, 3, 2});
    mesh.recreate_involved_face_index();
    return mesh;
}

static fv_surface_mesh<double, uint64_t> create_single_triangle(){
    fv_surface_mesh<double, uint64_t> mesh;
    mesh.vertices.emplace_back(vec3<double>(0.0, 0.0, 0.0));
    mesh.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
    mesh.vertices.emplace_back(vec3<double>(0.5, std::sqrt(3.0)/2.0, 0.0));
    mesh.faces.push_back({0, 1, 2});
    mesh.recreate_involved_face_index();
    return mesh;
}

static fv_surface_mesh<double, uint64_t> create_icosahedron(){
    fv_surface_mesh<double, uint64_t> mesh;
    std::stringstream ss;
    ss << "OFF" << std::endl
       << "12 20 0" << std::endl
       << "0 1.618034 1 " << std::endl
       << "0 1.618034 -1 " << std::endl
       << "0 -1.618034 1 " << std::endl
       << "0 -1.618034 -1 " << std::endl
       << "1.618034 1 0 " << std::endl
       << "1.618034 -1 0 " << std::endl
       << "-1.618034 1 0 " << std::endl
       << "-1.618034 -1 0 " << std::endl
       << "1 0 1.618034 " << std::endl
       << "-1 0 1.618034 " << std::endl
       << "1 0 -1.618034 " << std::endl
       << "-1 0 -1.618034 " << std::endl
       << "3 1 0 4" << std::endl
       << "3 0 1 6" << std::endl
       << "3 2 3 5" << std::endl
       << "3 3 2 7" << std::endl
       << "3 4 5 10" << std::endl
       << "3 5 4 8" << std::endl
       << "3 6 7 9" << std::endl
       << "3 7 6 11" << std::endl
       << "3 8 9 2" << std::endl
       << "3 9 8 0" << std::endl
       << "3 10 11 1" << std::endl
       << "3 11 10 3" << std::endl
       << "3 0 8 4" << std::endl
       << "3 0 6 9" << std::endl
       << "3 1 4 10" << std::endl
       << "3 1 11 6" << std::endl
       << "3 2 5 8" << std::endl
       << "3 2 9 7" << std::endl
       << "3 3 10 5" << std::endl
       << "3 3 7 11" << std::endl;
    if(!ReadFVSMeshFromOFF(mesh, ss)){
        throw std::runtime_error("Failed to create icosahedron.");
    }
    return mesh;
}

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

static bool verify_mesh_integrity(const fv_surface_mesh<double, uint64_t> &mesh){
    const size_t N_verts = mesh.vertices.size();
    for(size_t f_idx = 0; f_idx < mesh.faces.size(); ++f_idx){
        const auto &face = mesh.faces[f_idx];
        if(face.size() != 3) return false;
        for(const auto &v : face){
            if(v >= N_verts) return false;
        }
    }
    return true;
}


TEST_CASE( "loop_subdivide" ){

    SUBCASE("single triangle: V'=V+E, F'=4F"){
        auto mesh = create_single_triangle();
        const size_t V_before = mesh.vertices.size();
        const size_t F_before = mesh.faces.size();
        const size_t E_before = count_edges(mesh);

        loop_subdivide(mesh, 1);

        REQUIRE( mesh.vertices.size() == V_before + E_before );
        REQUIRE( mesh.faces.size() == 4 * F_before );
        REQUIRE( verify_mesh_integrity(mesh) );
    }

    SUBCASE("tetrahedron: V'=10, F'=16"){
        auto mesh = create_tetrahedron();
        loop_subdivide(mesh, 1);

        REQUIRE( mesh.vertices.size() == 10 );
        REQUIRE( mesh.faces.size() == 16 );
        REQUIRE( verify_mesh_integrity(mesh) );
    }

    SUBCASE("icosahedron: V'=42, F'=80"){
        auto mesh = create_icosahedron();
        REQUIRE( mesh.vertices.size() == 12 );
        REQUIRE( mesh.faces.size() == 20 );
        REQUIRE( count_edges(mesh) == 30 );

        loop_subdivide(mesh, 1);

        REQUIRE( mesh.vertices.size() == 42 );
        REQUIRE( mesh.faces.size() == 80 );
        REQUIRE( verify_mesh_integrity(mesh) );
    }

    SUBCASE("2 iterations of tetrahedron: V'=34, F'=64"){
        auto mesh = create_tetrahedron();
        loop_subdivide(mesh, 2);

        REQUIRE( mesh.vertices.size() == 34 );
        REQUIRE( mesh.faces.size() == 64 );
        REQUIRE( verify_mesh_integrity(mesh) );
    }

    SUBCASE("0 iterations is a no-op"){
        auto mesh = create_tetrahedron();
        const size_t V_before = mesh.vertices.size();
        const size_t F_before = mesh.faces.size();

        loop_subdivide(mesh, 0);

        REQUIRE( mesh.vertices.size() == V_before );
        REQUIRE( mesh.faces.size() == F_before );
    }

    SUBCASE("negative iterations is a no-op"){
        auto mesh = create_tetrahedron();
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
        mesh.faces.push_back({0, 1, 2, 3});
        mesh.recreate_involved_face_index();

        REQUIRE_THROWS( loop_subdivide(mesh, 1) );
    }

    SUBCASE("vertex normals preserved"){
        auto mesh = create_tetrahedron();
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
        auto mesh = create_tetrahedron();
        mesh.vertex_colours.push_back(mesh.pack_RGBA32_colour({255, 0, 0, 255}));
        mesh.vertex_colours.push_back(mesh.pack_RGBA32_colour({0, 255, 0, 255}));
        mesh.vertex_colours.push_back(mesh.pack_RGBA32_colour({0, 0, 255, 255}));
        mesh.vertex_colours.push_back(mesh.pack_RGBA32_colour({255, 255, 0, 255}));

        loop_subdivide(mesh, 1);

        REQUIRE( mesh.vertex_colours.size() == mesh.vertices.size() );
        REQUIRE( verify_mesh_integrity(mesh) );
    }
}
