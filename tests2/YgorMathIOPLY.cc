
#include <limits>
#include <iostream>
#include <sstream>

#include <YgorMath.h>
#include <YgorMathIOPLY.h>

#include "doctest/doctest.h"

TEST_CASE( "YgorMathIOPLY ReadFVSMeshFromPLY (ASCII-only)" ){

    fv_surface_mesh<double,uint32_t> sm_d;
    sm_d.vertices.emplace_back(vec3<double>(1.0, 2.0, 3.0));
    sm_d.vertices.emplace_back(vec3<double>(4.0, 5.0, 6.0));
    sm_d.vertices.emplace_back(vec3<double>(7.0, 8.0, 9.0));
    sm_d.vertex_colours.emplace_back(static_cast<uint32_t>(1234567890));
    sm_d.vertex_colours.emplace_back(static_cast<uint32_t>(2345678901));
    sm_d.vertex_colours.emplace_back(static_cast<uint32_t>(3456789012));
    sm_d.vertex_normals.emplace_back(vec3<double>(1.0,-1.0,2.0).unit());
    sm_d.vertex_normals.emplace_back(vec3<double>(-2.0,0.0,1.0).unit());
    sm_d.vertex_normals.emplace_back(vec3<double>(0.1,3.0,-1.0).unit());
    sm_d.faces = {{ static_cast<uint32_t>(0),
                    static_cast<uint32_t>(1),
                    static_cast<uint32_t>(2) }};
    sm_d.metadata["key"] = "value";


    fv_surface_mesh<float,uint64_t> sm_f;
    sm_f.vertices.emplace_back(vec3<float>(1.0f, 2.0f, 3.0f));
    sm_f.vertices.emplace_back(vec3<float>(4.0f, 5.0f, 6.0f));
    sm_f.vertices.emplace_back(vec3<float>(7.0f, 8.0f, 9.0f));
    sm_f.vertex_colours.emplace_back(static_cast<uint64_t>(1234567890));
    sm_f.vertex_colours.emplace_back(static_cast<uint64_t>(2345678901));
    sm_f.vertex_colours.emplace_back(static_cast<uint64_t>(3456789012));
    sm_f.vertex_normals.emplace_back(vec3<float>(1.0f,-1.0f,2.0f).unit());
    sm_f.vertex_normals.emplace_back(vec3<float>(-2.0f,0.0f,1.0f).unit());
    sm_f.vertex_normals.emplace_back(vec3<float>(0.1f,3.0f,-1.0f).unit());
    sm_f.faces = {{ static_cast<uint64_t>(0),
                    static_cast<uint64_t>(1),
                    static_cast<uint64_t>(2) }};
    sm_f.metadata["key"] = "value";


    SUBCASE("supported: vertices only"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "comment this is a comment" << std::endl
           << " comment this is another comment  " << std::endl
           << "element vertex 5" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "end_header" << std::endl
           << "1 1.0 1.0E1" << std::endl
           << " " << std::endl
           << " 2.0 2.0\t2.0" << std::endl
           << "3.0E-4 nan inf  " << std::endl
           << "-4.0 -4.0 -4.0" << std::endl
           << "\t" << std::endl
           << "5.0 +5.0" << std::endl
           << "\t" << std::endl
           << "5.0" << std::endl
           << "" << std::endl;

        REQUIRE(ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices only, explicit newlines"){
        std::stringstream ss;
        ss << "ply\n"
           << "format ascii 1.0\n"
           << "comment this is a comment\n"
           << " comment this is another comment  \n"
           << "element vertex 5\n"
           << "property float x\n"
           << "property float y\n"
           << "property float z\n"
           << "end_header\n"
           << "1 1.0 1.0E1\n"
           << " \n"
           << " 2.0 2.0\t2.0\n"
           << "3.0E-4 nan inf  \n"
           << "-4.0 -4.0 -4.0\n"
           << "\t\n"
           << "5.0 +5.0\n"
           << "\t\n"
           << "5.0"; // <-- (n.b. missing final newline.)

        REQUIRE(ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices and normals"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "comment this is a comment" << std::endl
           << " comment this is another comment  " << std::endl
           << "element vertex 3" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "property float nx" << std::endl
           << "property float ny" << std::endl
           << "property float nz" << std::endl
           << "end_header" << std::endl
           << "1 2 3 1 0 0" << std::endl
           << "4 5 6 0 1 0" << std::endl
           << "7 8 9 0 0 1" << std::endl
           << "" << std::endl;

        REQUIRE(ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 3);
        REQUIRE(sm_d.vertex_normals.size() == 3);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices only, extraneous numbers in stream"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "comment this is a comment" << std::endl
           << " comment this is another comment  " << std::endl
           << "element vertex 5" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "end_header" << std::endl
           << "1 1.0 1.0E1" << std::endl
           << " " << std::endl
           << " 2.0 2.0\t2.0" << std::endl
           << "3.0E-4 nan inf  " << std::endl
           << "-4.0 -4.0 -4.0" << std::endl
           << "\t" << std::endl
           << "5.0 +5.0" << std::endl
           << "\t" << std::endl
           << "5.0" << std::endl
           << "10.0 10.0 10.0" << std::endl
           << "10.0 10.0 10.0" << std::endl
           << "10.0 10.0 10.0" << std::endl;

        REQUIRE(ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertices.back().z == 5.0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices only, extraneous non-list vertex properties"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "comment this is a comment" << std::endl
           << " comment this is another comment  " << std::endl
           << "element vertex 5" << std::endl
           << "property float k" << std::endl // Irrelevant.
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "property float w" << std::endl // Irrelevant.
           << "end_header" << std::endl
           << "1.0 1 1.0 1.0E1 1.0" << std::endl
           << " " << std::endl
           << "1.0 2.0 2.0\t2.0 1.0" << std::endl
           << "1.0 3.0E-4 nan inf 1.0" << std::endl
           << "1.0 -4.0 -4.0 -4.0 1.0" << std::endl
           << "\t" << std::endl
           << "1.0 5.0 +5.0" << std::endl
           << "\t" << std::endl
           << "5.0 1.0" << std::endl
           << "" << std::endl;

        REQUIRE(ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertices.back().z == 5.0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices only, extraneous list vertex properties"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "comment this is a comment" << std::endl
           << " comment this is another comment  " << std::endl
           << "element vertex 5" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "property list uchar float w" << std::endl // Irrelevant.
           << "end_header" << std::endl
           << "1 1.0 1.0E1" << std::endl
           << "1 1.0" << std::endl // list
           << "2.0 2.0\t2.0" << std::endl
           << "2 1.0 1.0" << std::endl // list
           << "3.0E-4 nan inf" << std::endl
           << "2 1.0 1.0" << std::endl // list
           << "-4.0 -4.0 -4.0" << std::endl
           << "1 1.0" << std::endl // list
           << "\t" << std::endl
           << "5.0 +5.0" << std::endl
           << "\t" << std::endl
           << "5.0" << std::endl
           << "1 1.0" << std::endl // list
           << "" << std::endl;

        REQUIRE(ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertices.back().z == 5.0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices only, extraneous elements"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "comment this is a comment" << std::endl
           << " comment this is another comment  " << std::endl
           << "element vertex 5" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "element irrelevant_element 2" << std::endl
           << "property int i" << std::endl
           << "end_header" << std::endl
           << "1 1.0 1.0E1" << std::endl
           << " " << std::endl
           << " 2.0 2.0\t2.0" << std::endl
           << "3.0E-4 nan inf  " << std::endl
           << "-4.0 -4.0 -4.0" << std::endl
           << "\t" << std::endl
           << "5.0 +5.0" << std::endl
           << "\t" << std::endl
           << "5.0" << std::endl
           << "10 10" << std::endl;

        REQUIRE(ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertices.back().z == 5.0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices and faces"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "comment this is a comment" << std::endl
           << " comment this is another comment  " << std::endl
           << "element vertex 5" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "element face 3" << std::endl
           << "property list uchar uchar vertex_index" << std::endl
           << "end_header" << std::endl

           // Vertices.
           << "1 1.0 1.0E1" << std::endl
           << " " << std::endl
           << " 2.0 2.0\t2.0" << std::endl
           << "3.0E-4 nan inf  " << std::endl
           << "-4.0 -4.0 -4.0" << std::endl
           << "\t" << std::endl
           << "5.0 +5.0" << std::endl
           << "\t" << std::endl
           << "5.0" << std::endl

           // Faces.
           << "3 0 1 2" << std::endl
           << "2 0 3" << std::endl
           << "4 4 3 2 1" << std::endl;

        REQUIRE(ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 3);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices and faces in reverse order"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "comment this is a comment" << std::endl
           << " comment this is another comment  " << std::endl
           << "element face 3" << std::endl
           << "property list uchar int32 vertex_index" << std::endl
           << "element vertex 5" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "end_header" << std::endl

           // Faces.
           << "3 0 1 2" << std::endl
           << "2 0 3" << std::endl
           << "4 4 3 2 1" << std::endl

           // Vertices.
           << "1 1.0 1.0E1" << std::endl
           << " " << std::endl
           << " 2.0 2.0\t2.0" << std::endl
           << "3.0E-4 nan inf  " << std::endl
           << "-4.0 -4.0 -4.0" << std::endl
           << "\t" << std::endl
           << "5.0 +5.0" << std::endl
           << "\t" << std::endl
           << "5.0" << std::endl;

        REQUIRE(ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 3);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: metadata"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "comment metadata: test_key_1 = test_value_1" << std::endl
           << "element vertex 2" << std::endl
           << "property float x" << std::endl
           << "comment metadata: test_key_2 = test_value_2" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "comment metadata: test_key_3 = test_value_3" << std::endl
           << "end_header" << std::endl
           << "1.0 1.0 1.0" << std::endl
           << "2.0 2.0 2.0" << std::endl;

        REQUIRE(ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 2);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.size() == 3);
        REQUIRE(sm_d.metadata.at("test_key_1") == "test_value_1");
        REQUIRE(sm_d.metadata.at("test_key_2") == "test_value_2");
        REQUIRE(sm_d.metadata.at("test_key_3") == "test_value_3");
    }

    SUBCASE("unsupported: magic numbers missing"){
        std::stringstream ss;
        ss << "format ascii 1.0" << std::endl
           << "element vertex 1" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "end_header" << std::endl
           << "1.0 1.0 1.0" << std::endl;

        REQUIRE(!ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: PLY format versions beyond 1.0"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 2.0" << std::endl
           << "element vertex 1" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "end_header" << std::endl
           << "1.0 1.0 1.0" << std::endl;

        REQUIRE(!ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: insufficient numbers"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "element vertex 2" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "end_header" << std::endl
           << "1.0 1.0 1.0" << std::endl
           << "2.0 2.0" << std::endl;

        REQUIRE(!ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: insufficient numbers in property list"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "element vertex 2" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "element face 2" << std::endl
           << "property list uchar short vertex_index" << std::endl
           << "end_header" << std::endl
           << "1.0 1.0 1.0" << std::endl
           << "2.0 2.0 2.0" << std::endl
           << "1 0" << std::endl
           << "3 0 1" << std::endl;

        REQUIRE(!ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: different number of vertices and normals"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "comment this is a comment" << std::endl
           << " comment this is another comment  " << std::endl
           << "element vertex 3" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "property float nx" << std::endl
           << "property float ny" << std::endl
           << "property float nz" << std::endl
           << "end_header" << std::endl
           << "1 2 3 1 0 0" << std::endl
           << "4 5 6 0 1 0" << std::endl
           << "7 8 9" << std::endl
           << "" << std::endl;

        REQUIRE(!ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: incomplete vertex elements"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "comment this is a comment" << std::endl
           << " comment this is another comment  " << std::endl
           << "element vertex 3" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "end_header" << std::endl
           << "1 2" << std::endl
           << "4 5" << std::endl
           << "7 8" << std::endl
           << "" << std::endl;

        REQUIRE(!ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: incomplete normal elements"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "comment this is a comment" << std::endl
           << " comment this is another comment  " << std::endl
           << "element vertex 3" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "property float nx" << std::endl
           << "end_header" << std::endl
           << "1 2 3 1" << std::endl
           << "4 5 6 0" << std::endl
           << "7 8 9 1" << std::endl
           << "" << std::endl;

        REQUIRE(!ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: comments in the numbers section"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "element vertex 2" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "end_header" << std::endl
           << "1.0 1.0 1.0" << std::endl
           << "comment this placement should be rejected" << std::endl
           << "2.0 2.0 2.0" << std::endl;

        REQUIRE(!ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: faces reference non-existent vertices"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "element vertex 2" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "element face 2" << std::endl
           << "property list uchar ushort16 vertex_index" << std::endl
           << "end_header" << std::endl
           << "1.0 1.0 1.0" << std::endl
           << "2.0 2.0 2.0" << std::endl
           << "1 0" << std::endl
           << "3 0 1 3" << std::endl;

        REQUIRE(!ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: no vertex element"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "element irrelevant 2" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "end_header" << std::endl
           << "1.0 1.0" << std::endl
           << "2.0 2.0" << std::endl;

        REQUIRE(!ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: no vertices present"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "element vertex 0" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "element face 0" << std::endl
           << "property list uchar int vertex_index" << std::endl
           << "end_header" << std::endl;

        REQUIRE(!ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: faces use a non-floating-point property list"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "element vertex 2" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "element face 2" << std::endl
           << "property list uchar int vertex_index" << std::endl
           << "end_header" << std::endl
           << "1.0 1.0 1.0" << std::endl
           << "2.0 2.0 2.0" << std::endl
           << "1 0" << std::endl
           << "3 0 1 3" << std::endl;

        REQUIRE(!ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: referenced/custom materials"){
        std::stringstream ss;
        ss << "ply" << std::endl
           << "format ascii 1.0" << std::endl
           << "element material 1" << std::endl
           << "property red int" << std::endl
           << "property green int" << std::endl
           << "property blue int" << std::endl
           << "element material 2" << std::endl
           << "property weight float" << std::endl
           << "element vertex 2" << std::endl
           << "property float x" << std::endl
           << "property float y" << std::endl
           << "property float z" << std::endl
           << "property material_index 0" << std::endl
           << "property material_index 1" << std::endl
           << "end_header" << std::endl
           << "1.0 1.0 1.0" << std::endl
           << "0.0 0.0 1.0" << std::endl
           << "1.0" << std::endl
           << "2.0 2.0 2.0" << std::endl
           << "0.0 0.0 1.0" << std::endl
           << "1.0" << std::endl;

        REQUIRE(!ReadFVSMeshFromPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }
}

TEST_CASE( "YgorMathIOPLY WriteFVSMeshToPLY (binary-only)" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();
    const bool as_binary = true;

    SUBCASE("supported: vertices only, doubles"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_d, ss, as_binary));
    }

    SUBCASE("supported: vertices only, floats"){
        fv_surface_mesh<float,uint32_t> sm_f;
        sm_f.vertices.emplace_back(vec3<float>(1.0f, 1.0f, 1.0f));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_f, ss));
    }

    SUBCASE("supported: vertices and normals"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_d.vertex_normals.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_d.vertex_normals.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_d.vertex_normals.emplace_back(vec3<double>(0.0, 0.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_d, ss, as_binary));
    }

    SUBCASE("supported: vertices and faces"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_d.faces = {{ static_cast<uint32_t>(0),
                        static_cast<uint32_t>(1),
                        static_cast<uint32_t>(2) }};

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_d, ss, as_binary));
    }

    SUBCASE("supported: vertices with infs"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, inf, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 1.0,-inf));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_d.faces = {{ static_cast<uint32_t>(0),
                        static_cast<uint32_t>(1),
                        static_cast<uint32_t>(2) }};

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_d, ss, as_binary));
    }

    SUBCASE("supported: vertices with nans"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, nan, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(nan, 1.0, 1.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 0.0, nan));
        sm_d.faces = {{ static_cast<uint32_t>(0),
                        static_cast<uint32_t>(1),
                        static_cast<uint32_t>(2) }};

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_d, ss, as_binary));
    }

    SUBCASE("supported: metadata"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_d.faces = {{ static_cast<uint32_t>(0),
                        static_cast<uint32_t>(1),
                        static_cast<uint32_t>(2) }};
        sm_d.metadata["test_key_A"] = "test_value_A";
        sm_d.metadata["test_key_B"] = "test_value_B";

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_d, ss, as_binary));
    }

    SUBCASE("unsupported: no vertices or faces"){
        fv_surface_mesh<double,uint32_t> sm_d;

        std::stringstream ss;
        REQUIRE(!WriteFVSMeshToPLY(sm_d, ss, as_binary));
        REQUIRE(ss.str().empty());
    }
}

TEST_CASE( "YgorMathIOPLY WriteFVSMeshToPLY (ASCII-only)" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    SUBCASE("supported: vertices only, doubles"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_d, ss));
    }

    SUBCASE("supported: vertices only, floats"){
        fv_surface_mesh<float,uint32_t> sm_f;
        sm_f.vertices.emplace_back(vec3<float>(1.0f, 1.0f, 1.0f));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_f, ss));
    }

    SUBCASE("supported: vertices and normals"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_d.vertex_normals.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_d.vertex_normals.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_d.vertex_normals.emplace_back(vec3<double>(0.0, 0.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_d, ss));
    }

    SUBCASE("supported: vertices and faces"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_d.faces = {{ static_cast<uint32_t>(0),
                        static_cast<uint32_t>(1),
                        static_cast<uint32_t>(2) }};

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_d, ss));
    }

    SUBCASE("supported: vertices with infs"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, inf, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 1.0,-inf));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_d.faces = {{ static_cast<uint32_t>(0),
                        static_cast<uint32_t>(1),
                        static_cast<uint32_t>(2) }};

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_d, ss));
    }

    SUBCASE("supported: vertices with nans"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, nan, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(nan, 1.0, 1.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 0.0, nan));
        sm_d.faces = {{ static_cast<uint32_t>(0),
                        static_cast<uint32_t>(1),
                        static_cast<uint32_t>(2) }};

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_d, ss));
    }

    SUBCASE("supported: metadata"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_d.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_d.faces = {{ static_cast<uint32_t>(0),
                        static_cast<uint32_t>(1),
                        static_cast<uint32_t>(2) }};
        sm_d.metadata["test_key_A"] = "test_value_A";
        sm_d.metadata["test_key_B"] = "test_value_B";

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_d, ss));
    }

    SUBCASE("unsupported: no vertices or faces"){
        fv_surface_mesh<double,uint32_t> sm_d;

        std::stringstream ss;
        REQUIRE(!WriteFVSMeshToPLY(sm_d, ss));
        REQUIRE(ss.str().empty());
    }
}

TEST_CASE( "YgorMathIOPLY fv_surface_mesh round-trips (ASCII-only)" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    SUBCASE("supported: vertices only"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices and faces"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_orig.faces = {{ static_cast<uint32_t>(0),
                           static_cast<uint32_t>(1),
                           static_cast<uint32_t>(2) }};

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices and normals"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_orig.vertex_normals.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertex_normals.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertex_normals.emplace_back(vec3<double>(0.0, 0.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices, vertex normals, and faces"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_orig.vertex_normals.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertex_normals.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertex_normals.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_orig.faces = {{ static_cast<uint32_t>(0),
                           static_cast<uint32_t>(1),
                           static_cast<uint32_t>(2) }};

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices with infs"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, inf, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(inf, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0,-inf));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices with nans"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 1.0, nan));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_orig.vertices.size() == sm_read.vertices.size());
        REQUIRE(sm_orig.vertices.at(0).x == sm_read.vertices.at(0).x);
        REQUIRE(sm_orig.vertices.at(0).y == sm_read.vertices.at(0).y);
        REQUIRE(std::isnan(sm_read.vertices.at(0).z));
    }

    SUBCASE("supported: metadata"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_orig.faces = {{ static_cast<uint32_t>(0),
                           static_cast<uint32_t>(1),
                           static_cast<uint32_t>(2) }};
        sm_orig.metadata["test_key_A"] = "test_value_A";
        sm_orig.metadata["test_key_B"] = "test_value_B";

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
        REQUIRE(sm_orig.metadata == sm_read.metadata);
        REQUIRE(sm_read.metadata.size() == 2);
    }

    SUBCASE("supported: metadata that needs to be base64 encoded/decoded"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_orig.faces = {{ static_cast<uint32_t>(0),
                           static_cast<uint32_t>(1),
                           static_cast<uint32_t>(2) }};
        sm_orig.metadata["test_key_A"] = "test_problematic_char_next_=";
        sm_orig.metadata["test_key_B"] = "test_value_B";
        sm_orig.metadata["test_key_C"] = "test_problematic_because_\r_is_unprintable";
        sm_orig.metadata["test_key_D"] = "test_problematic_because_\n_is_unprintable";
        sm_orig.metadata["test_key_E"] = "test_problematic_because_\t_is_unprintable";
        sm_orig.metadata["test_key\r\t\n_F"] = "test_value_F";

        // Note: text-mode line ending conversion should NOT be a concern here.
        // The probematic sequences will be encoded before being written.
        sm_orig.metadata["test_key\r\n_G"] = "test_value\r\n_G";
        sm_orig.metadata["test_key\n\r_H"] = "test_value\n\r_H";

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_read.metadata.size() == 8);
        REQUIRE(sm_orig.metadata == sm_read.metadata);
        REQUIRE(sm_orig == sm_read);
    }
}

TEST_CASE( "YgorMathIOPLY fv_surface_mesh round-trips (binary-only)" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();
    bool as_binary = true;

    SUBCASE("supported: vertices only"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss, as_binary));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices and faces"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_orig.faces = {{ static_cast<uint32_t>(0),
                           static_cast<uint32_t>(1),
                           static_cast<uint32_t>(2) }};

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss, as_binary));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices and normals"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_orig.vertex_normals.emplace_back(vec3<double>(1.0, 0.0, 0.0).unit());
        sm_orig.vertex_normals.emplace_back(vec3<double>(0.0, 1.0, 0.0).unit());
        sm_orig.vertex_normals.emplace_back(vec3<double>(0.0, 0.0, 1.0).unit());

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss, as_binary));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices, vertex normals, and faces"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_orig.vertex_normals.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertex_normals.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertex_normals.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_orig.faces = {{ static_cast<uint32_t>(0),
                           static_cast<uint32_t>(1),
                           static_cast<uint32_t>(2) }};

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss, as_binary));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices with infs"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, inf, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(inf, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0,-inf));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss, as_binary));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices with nans"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 1.0, nan));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss, as_binary));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_orig.vertices.size() == sm_read.vertices.size());
        REQUIRE(sm_orig.vertices.at(0).x == sm_read.vertices.at(0).x);
        REQUIRE(sm_orig.vertices.at(0).y == sm_read.vertices.at(0).y);
        REQUIRE(std::isnan(sm_read.vertices.at(0).z));
    }

    SUBCASE("supported: metadata"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_orig.faces = {{ static_cast<uint32_t>(0),
                           static_cast<uint32_t>(1),
                           static_cast<uint32_t>(2) }};
        sm_orig.metadata["test_key_A"] = "test_value_A";
        sm_orig.metadata["test_key_B"] = "test_value_B";

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss, as_binary));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
        REQUIRE(sm_orig.metadata == sm_read.metadata);
        REQUIRE(sm_read.metadata.size() == 2);
    }

    SUBCASE("supported: metadata that needs to be base64 encoded/decoded"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_orig.faces = {{ static_cast<uint32_t>(0),
                           static_cast<uint32_t>(1),
                           static_cast<uint32_t>(2) }};
        sm_orig.metadata["test_key_A"] = "test_problematic_char_next_=";
        sm_orig.metadata["test_key_B"] = "test_value_B";
        sm_orig.metadata["test_key_C"] = "test_problematic_because_\r_is_unprintable";
        sm_orig.metadata["test_key_D"] = "test_problematic_because_\n_is_unprintable";
        sm_orig.metadata["test_key_E"] = "test_problematic_because_\t_is_unprintable";
        sm_orig.metadata["test_key\r\t\n_F"] = "test_value_F";

        // Note: text-mode line ending conversion should NOT be a concern here.
        // The probematic sequences will be encoded before being written.
        sm_orig.metadata["test_key\r\n_G"] = "test_value\r\n_G";
        sm_orig.metadata["test_key\n\r_H"] = "test_value\n\r_H";

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToPLY(sm_orig, ss, as_binary));
        REQUIRE(ReadFVSMeshFromPLY(sm_read, ss));
        REQUIRE(sm_read.metadata.size() == 8);
        REQUIRE(sm_orig.metadata == sm_read.metadata);
        REQUIRE(sm_orig == sm_read);
    }
}

