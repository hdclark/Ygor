
#include <limits>
#include <iostream>
#include <sstream>

#include <YgorMath.h>
#include <YgorMathIOOBJ.h>

#include "doctest/doctest.h"

// ------------------------------------------------------------------------------------------
// fv_surface_mesh
// ------------------------------------------------------------------------------------------

TEST_CASE( "YgorMathIOOBJ ReadFVSMeshFromOBJ" ){

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
        ss << "# This is a comment. It should be ignored." << std::endl
           << "# The next line is intentionally blank. It should be ignored too." << std::endl
           << "" << std::endl
           << "v 1.0 1.0 1.0" << std::endl
           << " v 2.0 2.0 2.0" << std::endl
           << "v 3\t3\t3" << std::endl
           << " " << std::endl
           << "v  4.0E-4 nan inf  " << std::endl
           << "\t" << std::endl
           << "v 5.0 5.0\t5.0 # This is also a comment and should be ignored." << std::endl
           << "" << std::endl;

        REQUIRE(ReadFVSMeshFromOBJ(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices only, explicit newlines"){
        std::stringstream ss;
        ss << "# This is a comment. It should be ignored.\n"
              "# The next line is intentionally blank. It should be ignored too.\n"
              "\n"
              "v 1.0 1.0   -1.0\n"
              " v 2.0 2.0 2.0\n"
              "v 3\t3\t3\n"
              " \n"
              "v  4.0E-4\tnan inf  \n"
              "\t\n"
              "v 5.0 5.0 5.0 # This is also a comment and should be ignored.\n"
              "\n";

        REQUIRE(ReadFVSMeshFromOBJ(sm_f, ss));
        REQUIRE(sm_f.vertices.size() == 5);
        REQUIRE(sm_f.vertex_normals.size() == 0);
        REQUIRE(sm_f.vertex_colours.size() == 0);
        REQUIRE(sm_f.faces.size() == 0);
        REQUIRE(sm_f.metadata.empty());
    }

    SUBCASE("supported: vertices and faces"){
        std::stringstream ss;
        ss << "v 1.0\t1.0\t1.0" << std::endl
           << "v 2.0\t2.0\t2.0" << std::endl
           << "v 3.0\t3.0\t3.0" << std::endl
           << "f 1\t2\t3" << std::endl;

        REQUIRE(ReadFVSMeshFromOBJ(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 3);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.faces.size() == 1);
        REQUIRE(sm_d.faces.at(0).at(0) == 0);
        REQUIRE(sm_d.faces.at(0).at(1) == 1);
        REQUIRE(sm_d.faces.at(0).at(2) == 2);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices and normals"){
        std::stringstream ss;
        ss << "v 1.0\t1.0\t1.0" << std::endl
           << "v 2.0\t2.0\t2.0" << std::endl
           << "vn 0\t0\t1" << std::endl
           << "vn 0\t1\t0" << std::endl;

        REQUIRE(ReadFVSMeshFromOBJ(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 2);
        REQUIRE(sm_d.vertex_normals.size() == 2);
        REQUIRE(sm_d.vertices.at(0) == vec3<double>(1.0, 1.0, 1.0));
        REQUIRE(sm_d.vertices.at(1) == vec3<double>(2.0, 2.0, 2.0));
        REQUIRE(sm_d.vertex_normals.at(0) == vec3<double>(0.0, 0.0, 1.0));
        REQUIRE(sm_d.vertex_normals.at(1) == vec3<double>(0.0, 1.0, 0.0));
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices with extra weight parameter"){
        std::stringstream ss("v 1.0\t2.0\t3.0\t0.5");

        REQUIRE(ReadFVSMeshFromOBJ(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 1);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.vertices.at(0) == vec3<double>(1.0, 2.0, 3.0));
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices followed by a comment"){
        std::stringstream ss("v 1.0\t1.0\t1.0 # some harmless comment");

        REQUIRE(ReadFVSMeshFromOBJ(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 1);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices and normals followed by a comment"){
        std::stringstream ss;
        ss << "v 1.0\t1.0\t1.0" << std::endl
           << "v 2.0\t2.0\t2.0 # some harmless comment" << std::endl
           << "v 3.0\t3.0\t3.0" << std::endl
           << "vn 0\t0\t1 # some harmless comment" << std::endl
           << "vn 0\t1\t0" << std::endl
           << "vn 1\t0\t0" << std::endl;

        REQUIRE(ReadFVSMeshFromOBJ(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 3);
        REQUIRE(sm_d.vertex_normals.size() == 3);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: unspecified primitive type"){
        std::stringstream ss("1.0 1.0 1.0"); // Note: no 'v' prefix.

        REQUIRE(!ReadFVSMeshFromOBJ(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices with 2 coordinates"){
        std::stringstream ss("v 1.0\t1.0\t");

        REQUIRE(!ReadFVSMeshFromOBJ(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices with 5 coordinates"){
        std::stringstream ss("v 1.0\t1.0\t1.0\t1.0\t1.0");

        REQUIRE(!ReadFVSMeshFromOBJ(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: normals with 2 coordinates"){
        std::stringstream ss;
        ss << "v 1.0\t1.0\t1.0" << std::endl
           << "vn 0\t1" << std::endl;

        REQUIRE(!ReadFVSMeshFromOBJ(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: normals with 4 coordinates"){
        std::stringstream ss;
        ss << "v 1.0\t1.0\t1.0" << std::endl
           << "vn 0\t1\t0\t0" << std::endl;

        REQUIRE(!ReadFVSMeshFromOBJ(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices followed by text"){
        std::stringstream ss("v 1.0\t1.0\t1.0\ttext");

        REQUIRE(!ReadFVSMeshFromOBJ(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: no vertices (completely empty mesh)"){
        std::stringstream ss("");

        REQUIRE(!ReadFVSMeshFromOBJ(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: different number of vertices and normals"){
        std::stringstream ss;
        ss << "v 1.0\t1.0\t1.0" << std::endl
           << "v 2.0\t2.0\t2.0" << std::endl
           << "v 3.0\t3.0\t3.0" << std::endl
           << "vn 0\t1\t0" << std::endl
           << "vn 1\t0\t0" << std::endl;

        REQUIRE(!ReadFVSMeshFromOBJ(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }
}

TEST_CASE( "YgorMathIOOBJ WriteFVSMeshToOBJ" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    SUBCASE("supported: vertices only, doubles"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOBJ(sm_d, ss));
    }

    SUBCASE("supported: vertices only, floats"){
        fv_surface_mesh<float,uint64_t> sm_f;
        sm_f.vertices.emplace_back(vec3<float>(1.0f, 1.0f, 1.0f));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOBJ(sm_f, ss));
    }

    SUBCASE("supported: vertices and normals"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        sm_d.vertex_normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOBJ(sm_d, ss));
    }

    SUBCASE("supported: vertices and faces"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        sm_d.vertices.emplace_back(vec3<double>(2.0, 2.0, 2.0));
        sm_d.vertices.emplace_back(vec3<double>(3.0, 3.0, 3.0));
        sm_d.faces = {{ static_cast<uint32_t>(0),
                        static_cast<uint32_t>(1),
                        static_cast<uint32_t>(2) }};

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOBJ(sm_d, ss));
    }

    SUBCASE("supported: vertices with infs"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        sm_orig.vertices.emplace_back(vec3<double>(1.0, inf, -inf));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOBJ(sm_orig, ss));
    }

    SUBCASE("supported: vertices with nans"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        sm_orig.vertices.emplace_back(vec3<double>(1.0, 1.0, nan));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOBJ(sm_orig, ss));
    }

    SUBCASE("unsupported: no vertices (empty mesh)"){
        fv_surface_mesh<double,uint32_t> sm_d;

        std::stringstream ss;
        REQUIRE(!WriteFVSMeshToOBJ(sm_d, ss));
        REQUIRE(ss.str().empty());
    }

    SUBCASE("unsupported: differing number of vertices and normals"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        sm_d.vertices.emplace_back(vec3<double>(2.0, 2.0, 2.0));
        sm_d.vertex_normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());

        std::stringstream ss;
        REQUIRE(!WriteFVSMeshToOBJ(sm_d, ss));
        REQUIRE(ss.str().empty());
    }
}

TEST_CASE( "YgorMathIOOBJ fv_surface_mesh round-trips" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    fv_surface_mesh<double,uint32_t> sm_orig;
    fv_surface_mesh<double,uint32_t> sm_read;

    SUBCASE("supported: vertices only"){
        sm_orig.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOBJ(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromOBJ(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices and normals"){
        sm_orig.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        sm_orig.vertex_normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOBJ(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromOBJ(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices, faces, and normals"){
        sm_orig.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        sm_orig.vertices.emplace_back(vec3<double>(2.0, 2.0, 2.0));
        sm_orig.vertices.emplace_back(vec3<double>(3.0, 3.0, 3.0));
        sm_orig.faces = {{ static_cast<uint32_t>(0),
                           static_cast<uint32_t>(1),
                           static_cast<uint32_t>(2) }};
        sm_orig.vertex_normals.emplace_back(vec3<double>( 1.0,  1.0, 1.0).unit());
        sm_orig.vertex_normals.emplace_back(vec3<double>(-1.0,  1.0, 1.0).unit());
        sm_orig.vertex_normals.emplace_back(vec3<double>( 1.0, -1.0, 1.0).unit());

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOBJ(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromOBJ(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices with infs"){
        sm_orig.vertices.emplace_back(vec3<double>(1.0, inf, -inf));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOBJ(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromOBJ(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices with nans"){
        sm_orig.vertices.emplace_back(vec3<double>(1.0, 1.0, nan));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOBJ(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromOBJ(sm_read, ss));
        REQUIRE(sm_orig.vertices.size() == sm_read.vertices.size());
        REQUIRE(sm_orig.vertices.at(0).x == sm_read.vertices.at(0).x);
        REQUIRE(sm_orig.vertices.at(0).y == sm_read.vertices.at(0).y);
        REQUIRE(std::isnan(sm_read.vertices.at(0).z));
    }

    SUBCASE("unsupported: metadata"){
        sm_orig.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        sm_orig.metadata["key"] = "value";

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOBJ(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromOBJ(sm_read, ss));
        REQUIRE(sm_orig != sm_read);
        REQUIRE(sm_read.metadata.empty());
        REQUIRE(!sm_orig.metadata.empty());
    }
}

// ------------------------------------------------------------------------------------------
// point_set
// ------------------------------------------------------------------------------------------

TEST_CASE( "YgorMathIOOBJ ReadPointSetFromOBJ" ){

    SUBCASE("supported: vertices only"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "# This is a comment. It should be ignored." << std::endl
           << "# The next line is intentionally blank. It should be ignored too." << std::endl
           << "" << std::endl
           << "v 1.0 1.0 1.0" << std::endl
           << " v 2.0 2.0 2.0" << std::endl
           << "v 3\t3\t3" << std::endl
           << " " << std::endl
           << "v  4.0E-4 nan inf  " << std::endl
           << "\t" << std::endl
           << "v 5.0 5.0\t5.0 # This is also a comment and should be ignored." << std::endl
           << "" << std::endl;

        REQUIRE(ReadPointSetFromOBJ(ps_d, ss));
        REQUIRE(ps_d.points.size() == 5);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: vertices only, explicit newlines"){
        point_set<float> ps_f;
        ps_f.points.emplace_back(vec3<float>(1.0f, 1.0f, 1.0f));
        ps_f.normals.emplace_back(vec3<float>(1.0f, 1.0f, 1.0f).unit());
        ps_f.metadata["key"] = "value";

        std::stringstream ss;
        ss << "# This is a comment. It should be ignored.\n"
              "# The next line is intentionally blank. It should be ignored too.\n"
              "\n"
              "v 1.0 1.0   -1.0\n"
              " v 2.0 2.0 2.0\n"
              "v 3\t3\t3\n"
              " \n"
              "v  4.0E-4\tnan inf  \n"
              "\t\n"
              "v 5.0 5.0 5.0 # This is also a comment and should be ignored.\n"
              "\n";

        REQUIRE(ReadPointSetFromOBJ(ps_f, ss));
        REQUIRE(ps_f.points.size() == 5);
        REQUIRE(ps_f.normals.size() == 0);
        REQUIRE(ps_f.metadata.empty());
    }

    SUBCASE("supported: vertices and normals"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "v 1.0\t1.0\t1.0" << std::endl
           << "v 2.0\t2.0\t2.0" << std::endl
           << "vn 0\t0\t1" << std::endl
           << "vn 0\t1\t0" << std::endl;

        REQUIRE(ReadPointSetFromOBJ(ps_d, ss));
        REQUIRE(ps_d.points.size() == 2);
        REQUIRE(ps_d.normals.size() == 2);
        REQUIRE(ps_d.points.at(0) == vec3<double>(1.0, 1.0, 1.0));
        REQUIRE(ps_d.points.at(1) == vec3<double>(2.0, 2.0, 2.0));
        REQUIRE(ps_d.normals.at(0) == vec3<double>(0.0, 0.0, 1.0));
        REQUIRE(ps_d.normals.at(1) == vec3<double>(0.0, 1.0, 0.0));
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: vertices with extra weight parameter"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss("v 1.0\t2.0\t3.0\t0.5");

        REQUIRE(ReadPointSetFromOBJ(ps_d, ss));
        REQUIRE(ps_d.points.size() == 1);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.points.at(0) == vec3<double>(1.0, 2.0, 3.0));
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: vertices followed by a comment"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss("v 1.0\t1.0\t1.0 # some harmless comment");

        REQUIRE(ReadPointSetFromOBJ(ps_d, ss));
        REQUIRE(ps_d.points.size() == 1);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: vertices and normals followed by a comment"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "v 1.0\t1.0\t1.0" << std::endl
           << "v 2.0\t2.0\t2.0 # some harmless comment" << std::endl
           << "v 3.0\t3.0\t3.0" << std::endl
           << "vn 0\t0\t1 # some harmless comment" << std::endl
           << "vn 0\t1\t0" << std::endl
           << "vn 1\t0\t0" << std::endl;

        REQUIRE(ReadPointSetFromOBJ(ps_d, ss));
        REQUIRE(ps_d.points.size() == 3);
        REQUIRE(ps_d.normals.size() == 3);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: unspecified primitive type"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss("1.0 1.0 1.0"); // Note: no 'v' prefix.

        REQUIRE(!ReadPointSetFromOBJ(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices with 2 coordinates"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss("v 1.0\t1.0\t");

        REQUIRE(!ReadPointSetFromOBJ(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices with 5 coordinates"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss("v 1.0\t1.0\t1.0\t1.0\t1.0");

        REQUIRE(!ReadPointSetFromOBJ(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: normals with 2 coordinates"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "v 1.0\t1.0\t1.0" << std::endl
           << "vn 0\t1" << std::endl;

        REQUIRE(!ReadPointSetFromOBJ(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: normals with 4 coordinates"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "v 1.0\t1.0\t1.0" << std::endl
           << "vn 0\t1\t0\t0" << std::endl;

        REQUIRE(!ReadPointSetFromOBJ(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices followed by text"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss("v 1.0\t1.0\t1.0\ttext");

        REQUIRE(!ReadPointSetFromOBJ(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: no vertices (empty point cloud)"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss("");

        REQUIRE(!ReadPointSetFromOBJ(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: different number of vertices and normals"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "v 1.0\t1.0\t1.0" << std::endl
           << "v 2.0\t2.0\t2.0" << std::endl
           << "v 3.0\t3.0\t3.0" << std::endl
           << "vn 0\t1\t0" << std::endl
           << "vn 1\t0\t0" << std::endl;

        REQUIRE(!ReadPointSetFromOBJ(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }
}

TEST_CASE( "YgorMathIOOBJ WritePointSetToOBJ" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    SUBCASE("supported: vertices only, doubles"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));

        std::stringstream ss;
        REQUIRE(WritePointSetToOBJ(ps_d, ss));
    }

    SUBCASE("supported: vertices only, floats"){
        point_set<float> ps_f;
        ps_f.points.emplace_back(vec3<float>(1.0f, 1.0f, 1.0f));

        std::stringstream ss;
        REQUIRE(WritePointSetToOBJ(ps_f, ss));
    }

    SUBCASE("supported: vertices and normals"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());

        std::stringstream ss;
        REQUIRE(WritePointSetToOBJ(ps_d, ss));
    }

    SUBCASE("supported: vertices with infs"){
        point_set<double> ps_orig;
        ps_orig.points.emplace_back(vec3<double>(1.0, inf, -inf));

        std::stringstream ss;
        REQUIRE(WritePointSetToOBJ(ps_orig, ss));
    }

    SUBCASE("supported: vertices with nans"){
        point_set<double> ps_orig;
        ps_orig.points.emplace_back(vec3<double>(1.0, 1.0, nan));

        std::stringstream ss;
        REQUIRE(WritePointSetToOBJ(ps_orig, ss));
    }

    SUBCASE("unsupported: no vertices (empty point cloud)"){
        point_set<double> ps_d;

        std::stringstream ss;
        REQUIRE(!WritePointSetToOBJ(ps_d, ss));
        REQUIRE(ss.str().empty());
    }

    SUBCASE("unsupported: differing number of vertices and normals"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.points.emplace_back(vec3<double>(2.0, 2.0, 2.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());

        std::stringstream ss;
        REQUIRE(!WritePointSetToOBJ(ps_d, ss));
        REQUIRE(ss.str().empty());
    }

}

TEST_CASE( "YgorMathIOOBJ point_set round-trips" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    SUBCASE("supported: vertices only"){
        point_set<double> ps_orig;
        point_set<double> ps_read;
        ps_orig.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));

        std::stringstream ss;
        REQUIRE(WritePointSetToOBJ(ps_orig, ss));
        REQUIRE(ReadPointSetFromOBJ(ps_read, ss));
        REQUIRE(ps_orig == ps_read);
    }

    SUBCASE("supported: vertices and normals"){
        point_set<double> ps_orig;
        point_set<double> ps_read;
        ps_orig.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_orig.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());

        std::stringstream ss;
        REQUIRE(WritePointSetToOBJ(ps_orig, ss));
        REQUIRE(ReadPointSetFromOBJ(ps_read, ss));
        REQUIRE(ps_orig == ps_read);
    }

    SUBCASE("supported: vertices with infs"){
        point_set<double> ps_orig;
        point_set<double> ps_read;
        ps_orig.points.emplace_back(vec3<double>(1.0, inf, -inf));

        std::stringstream ss;
        REQUIRE(WritePointSetToOBJ(ps_orig, ss));
        REQUIRE(ReadPointSetFromOBJ(ps_read, ss));
        REQUIRE(ps_orig == ps_read);
    }

    SUBCASE("supported: vertices with nans"){
        point_set<double> ps_orig;
        point_set<double> ps_read;
        ps_orig.points.emplace_back(vec3<double>(1.0, 1.0, nan));

        std::stringstream ss;
        REQUIRE(WritePointSetToOBJ(ps_orig, ss));
        REQUIRE(ReadPointSetFromOBJ(ps_read, ss));
        REQUIRE(ps_orig.points.size() == ps_read.points.size());
        REQUIRE(ps_orig.points.at(0).x == ps_read.points.at(0).x);
        REQUIRE(ps_orig.points.at(0).y == ps_read.points.at(0).y);
        REQUIRE(std::isnan(ps_read.points.at(0).z));
    }

    SUBCASE("unsupported: metadata"){
        point_set<double> ps_orig;
        point_set<double> ps_read;
        ps_orig.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_orig.metadata["key"] = "value";

        std::stringstream ss;
        REQUIRE(WritePointSetToOBJ(ps_orig, ss));
        REQUIRE(ReadPointSetFromOBJ(ps_read, ss));
        REQUIRE(ps_orig != ps_read);
        REQUIRE(ps_read.metadata.empty());
        REQUIRE(!ps_orig.metadata.empty());
    }
}

