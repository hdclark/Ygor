
#include <sstream>
#include <string>
#include <stdexcept>

#include <YgorMath.h>
#include <YgorMathIOOFF.h>
#include <YgorMathIOSTL.h>
#include <YgorMath_Samples.h>

#include "doctest/doctest.h"

TEST_CASE( "ASCII STL round-trip" ){
    const auto eps = 100.0 * std::sqrt( std::numeric_limits<double>::epsilon() );

    auto smesh = fv_surface_mesh_icosahedron();
    const auto mesh1_sa = smesh.surface_area();
    const auto mesh1_vc = smesh.vertices.size();

    // Round-trip #1.
    std::stringstream ss;
    REQUIRE( WriteFVSMeshToASCIISTL(smesh, ss) );
    const auto output_A = ss.str();
    REQUIRE( ReadFVSMeshFromASCIISTL(smesh, ss) );

    // Round-trip #2.
    std::stringstream ss2;
    REQUIRE( WriteFVSMeshToASCIISTL(smesh, ss2) );
    REQUIRE( ReadFVSMeshFromASCIISTL(smesh, ss2) );

    // Round-trip #3.
    std::stringstream ss3;
    REQUIRE( WriteFVSMeshToASCIISTL(smesh, ss3) );
    const auto output_B = ss3.str();
    REQUIRE( ReadFVSMeshFromASCIISTL(smesh, ss3) );

    const auto mesh2_sa = smesh.surface_area();
    const auto mesh2_vc = smesh.vertices.size();

    // Validate.
    REQUIRE( output_A == output_B );
    REQUIRE( mesh1_vc == mesh2_vc );
    REQUIRE( std::abs(mesh2_sa - mesh1_sa) < eps );
}


TEST_CASE( "Binary STL round-trip" ){
    const auto eps = 100.0 * std::sqrt( std::numeric_limits<double>::epsilon() );

    auto smesh = fv_surface_mesh_icosahedron();
    const auto mesh1_sa = smesh.surface_area();
    const auto mesh1_vc = smesh.vertices.size();

    // Round-trip #1.
    std::stringstream ss;
    REQUIRE( WriteFVSMeshToBinarySTL(smesh, ss) );
    const auto output_A = ss.str();
    REQUIRE( ReadFVSMeshFromBinarySTL(smesh, ss) );

    // Round-trip #2.
    std::stringstream ss2;
    REQUIRE( WriteFVSMeshToBinarySTL(smesh, ss2) );
    REQUIRE( ReadFVSMeshFromBinarySTL(smesh, ss2) );

    // Round-trip #3.
    std::stringstream ss3;
    REQUIRE( WriteFVSMeshToBinarySTL(smesh, ss3) );
    const auto output_B = ss3.str();
    REQUIRE( ReadFVSMeshFromBinarySTL(smesh, ss3) );

    const auto mesh2_sa = smesh.surface_area();
    const auto mesh2_vc = smesh.vertices.size();

    // Validate.
    REQUIRE( output_A == output_B );
    REQUIRE( mesh1_vc == mesh2_vc );
    REQUIRE( std::abs(mesh2_sa - mesh1_sa) < eps );
}
