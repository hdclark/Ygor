
#include <limits>
#include <iostream>
#include <sstream>

#include <YgorMath.h>
#include <YgorMathIOOFF.h>

#include "doctest/doctest.h"

// ------------------------------------------------------------------------------------------
// fv_surface_mesh
// ------------------------------------------------------------------------------------------

TEST_CASE( "YgorMathIOOFF ReadFVSMeshFromOFF" ){

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


    SUBCASE("supported: OFF magic present"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "1 0 \t0 " << std::endl
           << "1.0 1.0 1.0" << std::endl;

        REQUIRE(ReadFVSMeshFromOFF(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 1);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: OFF magic not present"){
        std::stringstream ss;
        ss << "1 0 \t0 " << std::endl
           << "1.0 1.0 1.0" << std::endl;

        REQUIRE(ReadFVSMeshFromOFF(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 1);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices only"){
        std::stringstream ss;
        ss << "# This is a comment. It should be ignored." << std::endl
           << "# The next line is intentionally blank. It should be ignored too." << std::endl
           << "OFF" << std::endl
           << "5 0 \t0 " << std::endl
           << "1.0 1.0 1.0" << std::endl
           << " 2.0 2.0 2.0" << std::endl
           << " 3\t3\t3" << std::endl
           << " " << std::endl
           << "4.0E-4 nan inf  " << std::endl
           << "\t" << std::endl
           << "5.0 5.0\t5.0 # This is also a comment and should be ignored." << std::endl
           << "" << std::endl;

        REQUIRE(ReadFVSMeshFromOFF(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices and faces"){
        std::stringstream ss;
        ss << "# This is a comment. It should be ignored." << std::endl
           << "# The next line is intentionally blank. It should be ignored too." << std::endl
           << "OFF" << std::endl
           << "3 2 0" << std::endl
           << "1.0 2.0 3.0" << std::endl
           << "4.0 5.0 6.0" << std::endl
           << "7.0 8.0 9.0" << std::endl
           << "3 1 0 2" << std::endl
           << "2 2 1" << std::endl;

        REQUIRE(ReadFVSMeshFromOFF(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 3);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 2);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices only, explicit newlines"){
        std::stringstream ss;
        ss << "# This is a comment. It should be ignored.\n"
           << "# The next line is intentionally blank. It should be ignored too.\n" 
           << "OFF\n" 
           << "5 0 \t0 \n" 
           << "1.0 1.0 1.0\n" 
           << " 2.0 2.0 2.0\n" 
           << " 3\t3\t3\n" 
           << " \n" 
           << "4.0E-4 nan inf  \n" 
           << "\t\n" 
           << "5.0 5.0\t5.0 # This is also a comment and should be ignored.\n" 
           << "\n"; 

        REQUIRE(ReadFVSMeshFromOFF(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 5);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: vertices and normals"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "1.0\t1.0\t1.0\t0.0\t0.0\t1.0" << std::endl
           << "2.0\t2.0\t2.0\t0.0\t1.0\t0.0" << std::endl;

        REQUIRE(ReadFVSMeshFromOFF(sm_d, ss));
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

    SUBCASE("supported: vertices followed by a comment"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0" << std::endl
           << "1.0\t1.0\t1.0 # some\tharmless\ttext" << std::endl;

        REQUIRE(ReadFVSMeshFromOFF(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 2);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("supported: normals followed by a comment"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0\t0.0\t0.0\t1.0" << std::endl
           << "1.0\t1.0\t1.0\t0.0\t1.0\t0.0 # some\tharmless\ttext" << std::endl;

        REQUIRE(ReadFVSMeshFromOFF(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 2);
        REQUIRE(sm_d.vertex_normals.size() == 2);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices with 2 coordinates"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "1.0\t1.0\t1.0" << std::endl
           << "2.0\t2.0\t" << std::endl;

        REQUIRE(!ReadFVSMeshFromOFF(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices with 4 coordinates"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "1.0\t1.0\t1.0" << std::endl
           << "2.0\t2.0\t2.0\t1.0" << std::endl;

        REQUIRE(!ReadFVSMeshFromOFF(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: normals with 4 coordinates"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "1.0\t1.0\t1.0\t0.0\t0.0\t1.0" << std::endl
           << "2.0\t2.0\t2.0\t0.0\t0.0\t1.0\t0.0" << std::endl;

        REQUIRE(!ReadFVSMeshFromOFF(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices followed by text"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0" << std::endl
           << "1.0\t1.0\t1.0\tsome\tdevious\ttext" << std::endl;

        REQUIRE(!ReadFVSMeshFromOFF(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: normals followed by text"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0\t0.0\t0.0\t1.0" << std::endl
           << "1.0\t1.0\t1.0\t0.0\t1.0\t0.0\tsome\tdevious\ttext" << std::endl;

        REQUIRE(!ReadFVSMeshFromOFF(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: no vertices (empty surface mesh)"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "0 0 0" << std::endl;

        REQUIRE(!ReadFVSMeshFromOFF(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }

    SUBCASE("unsupported: different number of vertices and normals"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0" << std::endl
           << "1.0\t1.0\t1.0\t0.0\t1.0\t0.0" << std::endl;

        REQUIRE(!ReadFVSMeshFromOFF(sm_d, ss));
        REQUIRE(sm_d.vertices.size() == 0);
        REQUIRE(sm_d.vertex_normals.size() == 0);
        REQUIRE(sm_d.vertex_colours.size() == 0);
        REQUIRE(sm_d.faces.size() == 0);
        REQUIRE(sm_d.metadata.empty());
    }
}

TEST_CASE( "YgorMathIOOFF WriteFVSMeshToOFF" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    SUBCASE("supported: vertices only, doubles"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOFF(sm_d, ss));
    }

    SUBCASE("supported: vertices only, floats"){
        fv_surface_mesh<float,uint64_t> sm_f;
        sm_f.vertices.emplace_back(vec3<float>(1.0f, 1.0f, 1.0f));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOFF(sm_f, ss));
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
        REQUIRE(WriteFVSMeshToOFF(sm_d, ss));
    }

    SUBCASE("supported: vertices and normals"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        sm_d.vertex_normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOFF(sm_d, ss));
    }

    SUBCASE("supported: vertices with infs"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        sm_orig.vertices.emplace_back(vec3<double>(1.0, inf, -inf));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOFF(sm_orig, ss));
    }

    SUBCASE("supported: vertices with nans"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        sm_orig.vertices.emplace_back(vec3<double>(1.0, 1.0, nan));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOFF(sm_orig, ss));
    }

    SUBCASE("unsupported: no vertices (empty surface mesh)"){
        fv_surface_mesh<double,uint32_t> sm_d;

        std::stringstream ss;
        REQUIRE(!WriteFVSMeshToOFF(sm_d, ss));
        REQUIRE(ss.str().empty());
    }

    SUBCASE("unsupported: differing number of vertices and normals"){
        fv_surface_mesh<double,uint32_t> sm_d;
        sm_d.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        sm_d.vertices.emplace_back(vec3<double>(2.0, 2.0, 2.0));
        sm_d.vertex_normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());

        std::stringstream ss;
        REQUIRE(!WriteFVSMeshToOFF(sm_d, ss));
        REQUIRE(ss.str().empty());
    }

}

TEST_CASE( "YgorMathIOOFF fv_surface_mesh round-trips" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    SUBCASE("supported: vertices only"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;
        sm_orig.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOFF(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromOFF(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices and normals"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;
        sm_orig.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        sm_orig.vertex_normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOFF(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromOFF(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices and faces"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;
        sm_orig.vertices.emplace_back(vec3<double>(1.0, 2.0, 3.0));
        sm_orig.vertices.emplace_back(vec3<double>(4.0, 5.0, 6.0));
        sm_orig.vertices.emplace_back(vec3<double>(7.0, 8.0, 9.0));
        sm_orig.faces = {{ static_cast<uint32_t>(1),
                           static_cast<uint32_t>(2),
                           static_cast<uint32_t>(0) }};

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOFF(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromOFF(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices, normals, and faces"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;
        sm_orig.vertices.emplace_back(vec3<double>(1.0, 2.0, 3.0));
        sm_orig.vertices.emplace_back(vec3<double>(4.0, 5.0, 6.0));
        sm_orig.vertices.emplace_back(vec3<double>(7.0, 8.0, 9.0));
        sm_orig.vertex_normals.emplace_back(vec3<double>( 1.0,  2.0, 3.0).unit());
        sm_orig.vertex_normals.emplace_back(vec3<double>( 3.0, -2.0, 1.0).unit());
        sm_orig.vertex_normals.emplace_back(vec3<double>( 2.0, -1.0, 1.0).unit());
        sm_orig.faces = {{ static_cast<uint32_t>(1),
                           static_cast<uint32_t>(2),
                           static_cast<uint32_t>(0) }};

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOFF(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromOFF(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices with infs"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;
        sm_orig.vertices.emplace_back(vec3<double>(1.0, inf, -inf));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOFF(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromOFF(sm_read, ss));
        REQUIRE(sm_orig == sm_read);
    }

    SUBCASE("supported: vertices with nans"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;
        sm_orig.vertices.emplace_back(vec3<double>(1.0, 1.0, nan));

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOFF(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromOFF(sm_read, ss));
        REQUIRE(sm_orig.vertices.size() == sm_read.vertices.size());
        REQUIRE(sm_orig.vertices.at(0).x == sm_read.vertices.at(0).x);
        REQUIRE(sm_orig.vertices.at(0).y == sm_read.vertices.at(0).y);
        REQUIRE(std::isnan(sm_read.vertices.at(0).z));
    }

    SUBCASE("unsupported: metadata"){
        fv_surface_mesh<double,uint32_t> sm_orig;
        fv_surface_mesh<double,uint32_t> sm_read;
        sm_orig.vertices.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        sm_orig.metadata["key"] = "value";

        std::stringstream ss;
        REQUIRE(WriteFVSMeshToOFF(sm_orig, ss));
        REQUIRE(ReadFVSMeshFromOFF(sm_read, ss));
        REQUIRE(sm_orig != sm_read);
        REQUIRE(sm_read.metadata.empty());
        REQUIRE(!sm_orig.metadata.empty());
    }
}


// ------------------------------------------------------------------------------------------
// point_set
// ------------------------------------------------------------------------------------------

TEST_CASE( "YgorMathIOOFF ReadPointSetFromOFF" ){

    point_set<double> ps_d;
    ps_d.points.emplace_back(vec3<double>(1.0, 2.0, 3.0));
    ps_d.points.emplace_back(vec3<double>(4.0, 5.0, 6.0));
    ps_d.points.emplace_back(vec3<double>(7.0, 8.0, 9.0));
    ps_d.normals.emplace_back(vec3<double>(1.0,-1.0,2.0).unit());
    ps_d.normals.emplace_back(vec3<double>(-1.0,0.0,3.0).unit());
    ps_d.normals.emplace_back(vec3<double>(0.1,3.0,-1.0).unit());
    ps_d.colours.emplace_back( static_cast<uint32_t>(1234567890) );
    ps_d.colours.emplace_back( static_cast<uint32_t>(2345678901) );
    ps_d.colours.emplace_back( static_cast<uint32_t>(3456789012) );
    ps_d.metadata["key"] = "value";

    point_set<float> ps_f;
    ps_f.points.emplace_back(vec3<float>(1.0f, 2.0f, 3.0f));
    ps_f.points.emplace_back(vec3<float>(4.0f, 5.0f, 6.0f));
    ps_f.points.emplace_back(vec3<float>(7.0f, 8.0f, 9.0f));
    ps_f.normals.emplace_back(vec3<float>(1.0f,-1.0f,2.0f).unit());
    ps_f.normals.emplace_back(vec3<float>(-1.0f,0.0f,3.0f).unit());
    ps_f.normals.emplace_back(vec3<float>(0.1f,3.0f,-1.0f).unit());
    ps_f.colours.emplace_back( static_cast<uint32_t>(1234567890) );
    ps_f.colours.emplace_back( static_cast<uint32_t>(2345678901) );
    ps_f.colours.emplace_back( static_cast<uint32_t>(3456789012) );
    ps_f.metadata["key"] = "value";


    SUBCASE("supported: OFF magic present"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "1 0 \t0 " << std::endl
           << "1.0 1.0 1.0" << std::endl;

        REQUIRE(ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 1);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.colours.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: OFF magic not present"){
        std::stringstream ss;
        ss << "1 0 \t0 " << std::endl
           << "1.0 1.0 1.0" << std::endl;

        REQUIRE(ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 1);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.colours.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: vertices only"){
        std::stringstream ss;
        ss << "# This is a comment. It should be ignored." << std::endl
           << "# The next line is intentionally blank. It should be ignored too." << std::endl
           << "OFF" << std::endl
           << "5 0 \t0 " << std::endl
           << "1.0 1.0 1.0" << std::endl
           << " 2.0 2.0 2.0" << std::endl
           << " 3\t3\t3" << std::endl
           << " " << std::endl
           << "4.0E-4 nan inf  " << std::endl
           << "\t" << std::endl
           << "5.0 5.0\t5.0 # This is also a comment and should be ignored." << std::endl
           << "" << std::endl;

        REQUIRE(ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 5);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.colours.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: vertices only, explicit newlines"){
        std::stringstream ss;
        ss << "# This is a comment. It should be ignored.\n"
           << "# The next line is intentionally blank. It should be ignored too.\n" 
           << "OFF\n" 
           << "5 0 \t0 \n" 
           << "1.0 1.0 1.0\n" 
           << " 2.0 2.0 2.0\n" 
           << " 3\t3\t3\n" 
           << " \n" 
           << "4.0E-4 nan inf  \n" 
           << "\t\n" 
           << "5.0 5.0\t5.0 # This is also a comment and should be ignored.\n" 
           << "\n"; 

        REQUIRE(ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 5);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.colours.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: vertices and normals"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "1.0\t1.0\t1.0\t0.0\t0.0\t1.0" << std::endl
           << "2.0\t2.0\t2.0\t0.0\t1.0\t0.0" << std::endl;

        REQUIRE(ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 2);
        REQUIRE(ps_d.normals.size() == 2);
        REQUIRE(ps_d.points.at(0) == vec3<double>(1.0, 1.0, 1.0));
        REQUIRE(ps_d.points.at(1) == vec3<double>(2.0, 2.0, 2.0));
        REQUIRE(ps_d.normals.at(0) == vec3<double>(0.0, 0.0, 1.0));
        REQUIRE(ps_d.normals.at(1) == vec3<double>(0.0, 1.0, 0.0));
        REQUIRE(ps_d.colours.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: vertices followed by a comment"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0" << std::endl
           << "1.0\t1.0\t1.0 # some\tharmless\ttext" << std::endl;

        REQUIRE(ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 2);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.colours.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: normals followed by a comment"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0\t0.0\t0.0\t1.0" << std::endl
           << "1.0\t1.0\t1.0\t0.0\t1.0\t0.0 # some\tharmless\ttext" << std::endl;

        REQUIRE(ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 2);
        REQUIRE(ps_d.normals.size() == 2);
        REQUIRE(ps_d.colours.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices with 2 coordinates"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "1.0\t1.0\t1.0" << std::endl
           << "2.0\t2.0\t" << std::endl;

        REQUIRE(!ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.colours.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices with 4 coordinates"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "1.0\t1.0\t1.0" << std::endl
           << "2.0\t2.0\t2.0\t1.0" << std::endl;

        REQUIRE(!ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.colours.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: normals with 4 coordinates"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "1.0\t1.0\t1.0\t0.0\t0.0\t1.0" << std::endl
           << "2.0\t2.0\t2.0\t0.0\t0.0\t1.0\t0.0" << std::endl;

        REQUIRE(!ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.colours.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices followed by text"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0" << std::endl
           << "1.0\t1.0\t1.0\tsome\tdevious\ttext" << std::endl;

        REQUIRE(!ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.colours.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: normals followed by text"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0\t0.0\t0.0\t1.0" << std::endl
           << "1.0\t1.0\t1.0\t0.0\t1.0\t0.0\tsome\tdevious\ttext" << std::endl;

        REQUIRE(!ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.colours.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: no vertices (empty point cloud)"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "0 0 0" << std::endl;

        REQUIRE(!ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.colours.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: different number of vertices and normals"){
        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0" << std::endl
           << "1.0\t1.0\t1.0\t0.0\t1.0\t0.0" << std::endl;

        REQUIRE(!ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.colours.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }
}

TEST_CASE( "YgorMathIOOFF WritePointSetToOFF" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    SUBCASE("supported: vertices only, doubles"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));

        std::stringstream ss;
        REQUIRE(WritePointSetToOFF(ps_d, ss));
    }

    SUBCASE("supported: vertices only, floats"){
        point_set<float> ps_f;
        ps_f.points.emplace_back(vec3<float>(1.0f, 1.0f, 1.0f));

        std::stringstream ss;
        REQUIRE(WritePointSetToOFF(ps_f, ss));
    }

    SUBCASE("supported: vertices and normals"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());

        std::stringstream ss;
        REQUIRE(WritePointSetToOFF(ps_d, ss));
    }

    SUBCASE("supported: vertices with infs"){
        point_set<double> ps_orig;
        ps_orig.points.emplace_back(vec3<double>(1.0, inf, -inf));

        std::stringstream ss;
        REQUIRE(WritePointSetToOFF(ps_orig, ss));
    }

    SUBCASE("supported: vertices with nans"){
        point_set<double> ps_orig;
        ps_orig.points.emplace_back(vec3<double>(1.0, 1.0, nan));

        std::stringstream ss;
        REQUIRE(WritePointSetToOFF(ps_orig, ss));
    }

    SUBCASE("unsupported: no vertices (empty point cloud)"){
        point_set<double> ps_d;

        std::stringstream ss;
        REQUIRE(!WritePointSetToOFF(ps_d, ss));
        REQUIRE(ss.str().empty());
    }

    SUBCASE("unsupported: differing number of vertices and normals"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.points.emplace_back(vec3<double>(2.0, 2.0, 2.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());

        std::stringstream ss;
        REQUIRE(!WritePointSetToOFF(ps_d, ss));
        REQUIRE(ss.str().empty());
    }

}

TEST_CASE( "YgorMathIOOFF point_set round-trips" ){
    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    SUBCASE("supported: vertices only"){
        point_set<double> ps_orig;
        point_set<double> ps_read;
        ps_orig.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));

        std::stringstream ss;
        REQUIRE(WritePointSetToOFF(ps_orig, ss));
        REQUIRE(ReadPointSetFromOFF(ps_read, ss));
        REQUIRE(ps_orig == ps_read);
    }

    SUBCASE("supported: vertices and normals"){
        point_set<double> ps_orig;
        point_set<double> ps_read;
        ps_orig.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_orig.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());

        std::stringstream ss;
        REQUIRE(WritePointSetToOFF(ps_orig, ss));
        REQUIRE(ReadPointSetFromOFF(ps_read, ss));
        REQUIRE(ps_orig == ps_read);
    }

    SUBCASE("supported: vertices with infs"){
        point_set<double> ps_orig;
        point_set<double> ps_read;
        ps_orig.points.emplace_back(vec3<double>(1.0, inf, -inf));

        std::stringstream ss;
        REQUIRE(WritePointSetToOFF(ps_orig, ss));
        REQUIRE(ReadPointSetFromOFF(ps_read, ss));
        REQUIRE(ps_orig == ps_read);
    }

    SUBCASE("supported: vertices with nans"){
        point_set<double> ps_orig;
        point_set<double> ps_read;
        ps_orig.points.emplace_back(vec3<double>(1.0, 1.0, nan));

        std::stringstream ss;
        REQUIRE(WritePointSetToOFF(ps_orig, ss));
        REQUIRE(ReadPointSetFromOFF(ps_read, ss));
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
        REQUIRE(WritePointSetToOFF(ps_orig, ss));
        REQUIRE(ReadPointSetFromOFF(ps_read, ss));
        REQUIRE(ps_orig != ps_read);
        REQUIRE(ps_read.metadata.empty());
        REQUIRE(!ps_orig.metadata.empty());
    }
}

