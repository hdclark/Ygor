
#include <limits>
#include <iostream>
#include <sstream>

#include <YgorMath.h>
#include <YgorMathIOPLY.h>

#include "doctest/doctest.h"

TEST_CASE( "YgorMathIOPLY ReadFVSMeshFromASCIIPLY" ){
    // Prepare a mesh that will be purged during the read.
    fv_surface_mesh<double,uint32_t> sm_d;
    sm_d.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
    sm_d.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
    sm_d.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
    sm_d.faces = {{ static_cast<uint32_t>(0),
                    static_cast<uint32_t>(1),
                    static_cast<uint32_t>(2) }};
    sm_d.metadata["key"] = "value";

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

        REQUIRE(ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
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

        REQUIRE(ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
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

        REQUIRE(ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertices.back().z == 5.0);
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

        REQUIRE(ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertices.back().z == 5.0);
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

        REQUIRE(ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertices.back().z == 5.0);
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

        REQUIRE(ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertices.back().z == 5.0);
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
           << "property list uchar double vertex_index" << std::endl
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

        REQUIRE(ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
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
           << "property list uchar double vertex_index" << std::endl
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

        REQUIRE(ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.faces.size() == 3);
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

        REQUIRE(!ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
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

        REQUIRE(!ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
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
           << "property list uchar double vertex_index" << std::endl
           << "end_header" << std::endl
           << "1.0 1.0 1.0" << std::endl
           << "2.0 2.0 2.0" << std::endl
           << "1 0" << std::endl
           << "3 0 1" << std::endl;

        REQUIRE(!ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
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
           << "comment this should probably be rejected" << std::endl
           << "2.0 2.0 2.0" << std::endl;

        REQUIRE(!ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
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
           << "property list uchar double vertex_index" << std::endl
           << "end_header" << std::endl
           << "1.0 1.0 1.0" << std::endl
           << "2.0 2.0 2.0" << std::endl
           << "1 0" << std::endl
           << "3 0 1 3" << std::endl;

        REQUIRE(!ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
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

        REQUIRE(!ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
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
           << "property list uchar double vertex_index" << std::endl
           << "end_header" << std::endl;

        REQUIRE(!ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
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

        REQUIRE(!ReadFVSMeshFromASCIIPLY(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }
}

TEST_CASE( "YgorMathIOPLY WriteFVSMeshToASCIIPLY" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    SUBCASE("supported: vertices only, doubles"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToASCIIPLY(sm_d, ss));
    }

    SUBCASE("supported: vertices only, floats"){
        fv_surface_mesh<float,uint32_t> sm_f;
        sm_f.vertices.emplace_back(vec3<float>(1.0f, 1.0f, 1.0f));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToASCIIPLY(sm_f, ss));
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
        REQUIRE(WriteFVSMeshToASCIIPLY(sm_d, ss));
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
        REQUIRE(WriteFVSMeshToASCIIPLY(sm_d, ss));
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
        REQUIRE(WriteFVSMeshToASCIIPLY(sm_d, ss));
    }

    SUBCASE("unsupported: no vertices or faces"){
        fv_surface_mesh<double,uint32_t> sm_d;

        std::stringstream ss;
        REQUIRE(!WriteFVSMeshToASCIIPLY(sm_d, ss));
        REQUIRE(ss.str().empty());
    }
}

TEST_CASE( "YgorMathIOPLY fv_surface_mesh round-trips" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    SUBCASE("supported: vertices only"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> ps_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToASCIIPLY(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromASCIIPLY(ps_read, ss));
        REQUIRE(sm_orig == ps_read);
    }

    SUBCASE("supported: vertices and faces"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> ps_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_orig.faces = {{ static_cast<uint32_t>(0),
                           static_cast<uint32_t>(1),
                           static_cast<uint32_t>(2) }};

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToASCIIPLY(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromASCIIPLY(ps_read, ss));
        REQUIRE(sm_orig == ps_read);
    }

    SUBCASE("supported: vertices with infs"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> ps_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, inf, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(inf, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0,-inf));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToASCIIPLY(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromASCIIPLY(ps_read, ss));
        REQUIRE(sm_orig == ps_read);
    }

    SUBCASE("supported: vertices with nans"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> ps_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 1.0, nan));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToASCIIPLY(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromASCIIPLY(ps_read, ss));
        REQUIRE(sm_orig.vertices.size() == ps_read.vertices.size());
        REQUIRE(sm_orig.vertices.at(0).x == ps_read.vertices.at(0).x);
        REQUIRE(sm_orig.vertices.at(0).y == ps_read.vertices.at(0).y);
        REQUIRE(std::isnan(ps_read.vertices.at(0).z));
    }

    SUBCASE("unsupported: metadata"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> ps_read;

        sm_orig.vertices.emplace_back(vec3<double>(1.0, 0.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 1.0, 0.0));
        sm_orig.vertices.emplace_back(vec3<double>(0.0, 0.0, 1.0));
        sm_orig.faces = {{ static_cast<uint32_t>(0),
                           static_cast<uint32_t>(1),
                           static_cast<uint32_t>(2) }};
        sm_orig.metadata["key"] = "value";

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToASCIIPLY(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromASCIIPLY(ps_read, ss));
        REQUIRE(sm_orig != ps_read);
        REQUIRE(ps_read.metadata.empty());
        REQUIRE(!sm_orig.metadata.empty());
    }
}

