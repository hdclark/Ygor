
#include <limits>
#include <iostream>
#include <sstream>

#include <YgorMath.h>
#include <YgorMathIOOFF.h>

#include "doctest/doctest.h"

TEST_CASE( "YgorMathIOOFF ReadPointSetFromOFF" ){

    SUBCASE("supported: OFF magic present"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "OFF" << std::endl
           << "1 0 \t0 " << std::endl
           << "1.0 1.0 1.0" << std::endl;

        REQUIRE(ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 1);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: OFF magic not present"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "1 0 \t0 " << std::endl
           << "1.0 1.0 1.0" << std::endl;

        REQUIRE(ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 1);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: vertices only"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

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
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: vertices only, explicit newlines"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

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
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: vertices and normals"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

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
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: vertices followed by a comment"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0" << std::endl
           << "1.0\t1.0\t1.0 # some\tharmless\ttext" << std::endl;

        REQUIRE(ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 2);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("supported: normals followed by a comment"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0\t0.0\t0.0\t1.0" << std::endl
           << "1.0\t1.0\t1.0\t0.0\t1.0\t0.0 # some\tharmless\ttext" << std::endl;

        REQUIRE(ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 2);
        REQUIRE(ps_d.normals.size() == 2);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices with 2 coordinates"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "1.0\t1.0\t1.0" << std::endl
           << "2.0\t2.0\t" << std::endl;

        REQUIRE(!ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices with 4 coordinates"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "1.0\t1.0\t1.0" << std::endl
           << "2.0\t2.0\t2.0\t1.0" << std::endl;

        REQUIRE(!ReadPointSetFromOFF(ps_d, ss));
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
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "1.0\t1.0\t1.0\t0.0\t0.0\t1.0" << std::endl
           << "2.0\t2.0\t2.0\t0.0\t0.0\t1.0\t0.0" << std::endl;

        REQUIRE(!ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: vertices followed by text"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0" << std::endl
           << "1.0\t1.0\t1.0\tsome\tdevious\ttext" << std::endl;

        REQUIRE(!ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: normals followed by text"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0\t0.0\t0.0\t1.0" << std::endl
           << "1.0\t1.0\t1.0\t0.0\t1.0\t0.0\tsome\tdevious\ttext" << std::endl;

        REQUIRE(!ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
        REQUIRE(ps_d.metadata.empty());
    }

    SUBCASE("unsupported: no vertices (empty point cloud)"){
        point_set<double> ps_d;
        ps_d.points.emplace_back(vec3<double>(1.0, 1.0, 1.0));
        ps_d.normals.emplace_back(vec3<double>(1.0, 1.0, 1.0).unit());
        ps_d.metadata["key"] = "value";

        std::stringstream ss;
        ss << "OFF" << std::endl
           << "0 0 0" << std::endl;

        REQUIRE(!ReadPointSetFromOFF(ps_d, ss));
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
        ss << "OFF" << std::endl
           << "2 0 0" << std::endl
           << "2.0\t2.0\t2.0" << std::endl
           << "1.0\t1.0\t1.0\t0.0\t1.0\t0.0" << std::endl;

        REQUIRE(!ReadPointSetFromOFF(ps_d, ss));
        REQUIRE(ps_d.points.size() == 0);
        REQUIRE(ps_d.normals.size() == 0);
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

