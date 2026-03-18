
#include <sstream>
#include <string>

#include <YgorMath.h>
#include <YgorMathIOOFF.h>
#include <YgorMathIOSTL.h>

#include "doctest/doctest.h"


static fv_surface_mesh<double, uint64_t> create_icosahedron_mesh(){
    fv_surface_mesh<double, uint64_t> smesh;

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
       << "3 3 7 11" << std::endl
       << "" << std::endl;

    if(!ReadFVSMeshFromOFF(smesh, ss)){
        throw std::runtime_error("Unable to read icosahedron from OFF stream.");
    }
    return smesh;
}


TEST_CASE( "ASCII STL round-trip" ){
    auto smesh = create_icosahedron_mesh();

    std::stringstream ss2;
    REQUIRE( WriteFVSMeshToASCIISTL(smesh, ss2) );
    const auto output_A = ss2.str();

    REQUIRE( ReadFVSMeshFromASCIISTL(smesh, ss2) );
    std::stringstream ss3;
    REQUIRE( WriteFVSMeshToASCIISTL(smesh, ss3) );
    const auto output_B = ss3.str();

    REQUIRE( output_A == output_B );
}


TEST_CASE( "Binary STL round-trip" ){
    auto smesh = create_icosahedron_mesh();

    std::stringstream ss2;
    REQUIRE( WriteFVSMeshToBinarySTL(smesh, ss2) );
    const auto output_A = ss2.str();

    REQUIRE( ReadFVSMeshFromBinarySTL(smesh, ss2) );
    std::stringstream ss3;
    REQUIRE( WriteFVSMeshToBinarySTL(smesh, ss3) );
    const auto output_B = ss3.str();

    REQUIRE( output_A == output_B );
}
