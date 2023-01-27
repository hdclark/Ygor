//Test_Math21.cc - This is a test file for fv_surface_mesh.

#include <iostream>
#include <sstream>
#include <cmath>
#include <string>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMathIOOFF.h"
#include "YgorMathIOOBJ.h"
#include "YgorMathIOSTL.h"


int main(int argc, char **argv){

    // Attempt to round-trip a mesh with limited vertex precision.
    {
        fv_surface_mesh<double, uint64_t> smesh;

        std::stringstream ss;
        ss << "OFF" << std::endl
           << "# Whole-line comment" << std::endl
           << "# A # Confusing # comment" << std::endl
           << "100 100 #0 # An invalid line that should be disregarded" << std::endl
           << "12 20 0" << std::endl
           << "0 1.618034 1 " << std::endl  // The vertices.
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
           << "3 1 0 4" << std::endl          // The faces.
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
           << "3 3 10 5 # Another comment." << std::endl
           << "3 3 7 11" << std::endl
           << "" << std::endl;

        // Round-trip #1.
        if(!ReadFVSMeshFromOFF(smesh, ss)){
            throw std::runtime_error("Unable to read mesh from OFF stream: 1.");
        }else{
            YLOGINFO("First read OK");
        }
        YLOGINFO("Mesh surface area: " << smesh.surface_area());
        std::stringstream ss2;
        if(!WriteFVSMeshToOFF(smesh, ss2)){
            throw std::runtime_error("Unable to write mesh to OFF stream: 1.");
        }else{
            YLOGINFO("First write OK");
        }
        const auto output_A = ss2.str();

        // Round-trip #2.
        if(!ReadFVSMeshFromOFF(smesh, ss2)){
            throw std::runtime_error("Unable to read mesh from OFF stream: 2.");
        }else{
            YLOGINFO("Second read OK");
        }
        YLOGINFO("Mesh surface area: " << smesh.surface_area());
        std::stringstream ss3;
        if(!WriteFVSMeshToOFF(smesh, ss3)){
            throw std::runtime_error("Unable to write mesh to OFF stream: 2.");
        }else{
            YLOGINFO("Second write OK");
        }
        const auto output_B = ss3.str();

        if(output_A != output_B){
            throw std::runtime_error("Round-tripping mesh failed.");
        }
        YLOGINFO("OFF found-trip tests passed");
    }

    std::cout << "--------------" << std::endl;

    // Attempt to round-trip a mesh with limited vertex precision.
    {
        fv_surface_mesh<double, uint64_t> smesh;

        std::stringstream ss;
        ss << "# Wavefront OBJ file." << std::endl
           << "# A # Confusing # comment" << std::endl
           << "v 0 1.618034 1 " << std::endl  // The vertices.
           << "v 0 1.618034 -1 " << std::endl
           << "v 0 -1.618034 1 " << std::endl
           << "v 0 -1.618034 -1 " << std::endl
           << "v 1.618034 1 0 " << std::endl
           << "v 1.618034 -1 0 " << std::endl
           << "v -1.618034 1 0 " << std::endl
           << "v -1.618034 -1 0 " << std::endl
           << "v 1 0 1.618034 " << std::endl
           << "v -1 0 1.618034 " << std::endl
           << "v 1 0 -1.618034 " << std::endl
           << "v -1 0 -1.618034 " << std::endl
           << "f 2 1 5" << std::endl          // The faces.
           << "f 1 2 7" << std::endl
           << "f 3 4 6" << std::endl
           << "f 4 3 8" << std::endl
           << "f 5 6 11" << std::endl
           << "f 6 5 9" << std::endl
           << "f 7 8 10" << std::endl
           << "f 8 7 12" << std::endl
           << "f 9 10 3" << std::endl
           << "f 10 9 1" << std::endl
           << "f 11 12 2" << std::endl
           << "f 12 11 4" << std::endl
           << "f 1 9 5" << std::endl
           << "f 1 7 10" << std::endl
           << "f 2 5 11" << std::endl
           << "f 2 12 7" << std::endl
           << "f 3 6 9" << std::endl
           << "f 3 10 8" << std::endl
           << "f 4 11 6 # Another comment." << std::endl
           << "f 4 8 12" << std::endl
           << "" << std::endl;

        // Round-trip #1.
        if(!ReadFVSMeshFromOBJ(smesh, ss)){
            throw std::runtime_error("Unable to read mesh from OBJ stream: 1.");
        }else{
            YLOGINFO("First read OK");
        }
        YLOGINFO("Mesh surface area: " << smesh.surface_area());
        std::stringstream ss2;
        if(!WriteFVSMeshToOBJ(smesh, ss2)){
            throw std::runtime_error("Unable to write mesh to OBJ stream: 1.");
        }else{
            YLOGINFO("First write OK");
        }
        const auto output_A = ss2.str();

        // Round-trip #2.
        if(!ReadFVSMeshFromOBJ(smesh, ss2)){
            throw std::runtime_error("Unable to read mesh from OBJ stream: 2.");
        }else{
            YLOGINFO("Second read OK");
        }
        YLOGINFO("Mesh surface area: " << smesh.surface_area());
        std::stringstream ss3;
        const bool use_relative = true;
        if(!WriteFVSMeshToOBJ(smesh, ss3, use_relative)){
            throw std::runtime_error("Unable to write mesh to OBJ stream: 2.");
        }else{
            YLOGINFO("Second write OK");
        }

        // Round-trip #3.
        if(!ReadFVSMeshFromOBJ(smesh, ss3)){
            throw std::runtime_error("Unable to read mesh from OBJ stream: 3.");
        }else{
            YLOGINFO("Third read OK");
        }
        YLOGINFO("Mesh surface area: " << smesh.surface_area());
        std::stringstream ss4;
        if(!WriteFVSMeshToOBJ(smesh, ss4)){
            throw std::runtime_error("Unable to write mesh to OBJ stream: 3.");
        }else{
            YLOGINFO("Third write OK");
        }
        const auto output_B = ss4.str();


        if(output_A != output_B){
            throw std::runtime_error("Round-tripping mesh failed.");
        }
        YLOGINFO("OBJ round-trip tests passed");
    }

    std::cout << "--------------" << std::endl;

    // Attempt to write and read as an ASCII STL file.
    {
        fv_surface_mesh<double, uint64_t> smesh;

        std::stringstream ss;
        ss << "OFF" << std::endl
           << "# Whole-line comment" << std::endl
           << "# A # Confusing # comment" << std::endl
           << "100 100 #0 # An invalid line that should be disregarded" << std::endl
           << "12 20 0" << std::endl
           << "0 1.618034 1 " << std::endl  // The vertices.
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
           << "3 1 0 4" << std::endl          // The faces.
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
           << "3 3 10 5 # Another comment." << std::endl
           << "3 3 7 11" << std::endl
           << "" << std::endl;

        // Round-trip #1.
        if(!ReadFVSMeshFromOFF(smesh, ss)){
            throw std::runtime_error("Unable to read mesh from OFF stream: 1.");
        }else{
            YLOGINFO("First read OK");
        }
        YLOGINFO("Mesh surface area: " << smesh.surface_area());
        YLOGINFO("Mesh vertex count: " << smesh.vertices.size());
        std::stringstream ss2;
        if(!WriteFVSMeshToASCIISTL(smesh, ss2)){
            throw std::runtime_error("Unable to write mesh to ASCII STL stream: 1.");
        }else{
            YLOGINFO("First write OK");
        }
        const auto output_A = ss2.str();

        // Round-trip #2.
        if(!ReadFVSMeshFromASCIISTL(smesh, ss2)){
            throw std::runtime_error("Unable to read mesh from ASCII STL stream: 2.");
        }else{
            YLOGINFO("Second read OK");
        }
        YLOGINFO("Mesh surface area: " << smesh.surface_area());
        YLOGINFO("Mesh vertex count: " << smesh.vertices.size());
        std::stringstream ss3;
        if(!WriteFVSMeshToASCIISTL(smesh, ss3)){
            throw std::runtime_error("Unable to write mesh to ASCII STL stream: 2.");
        }else{
            YLOGINFO("Second write OK");
        }

        // Round-trip #3.
        if(!ReadFVSMeshFromASCIISTL(smesh, ss3)){
            throw std::runtime_error("Unable to read mesh from ASCII STL stream: 3.");
        }else{
            YLOGINFO("Third read OK");
        }
        YLOGINFO("Mesh surface area: " << smesh.surface_area());
        YLOGINFO("Mesh vertex count: " << smesh.vertices.size());
        std::stringstream ss4;
        if(!WriteFVSMeshToASCIISTL(smesh, ss4)){
            throw std::runtime_error("Unable to write mesh to ASCII STL stream: 3.");
        }else{
            YLOGINFO("Third write OK");
        }
        const auto output_B = ss4.str();


        if(output_A != output_B){
            throw std::runtime_error("Round-tripping mesh failed.");
        }
        YLOGINFO("ASCII STL round-trip tests passed");
    }

    std::cout << "--------------" << std::endl;

    // Attempt to write and read as a binary STL file.
    {
        fv_surface_mesh<double, uint64_t> smesh;

        std::stringstream ss;
        ss << "OFF" << std::endl
           << "# Whole-line comment" << std::endl
           << "# A # Confusing # comment" << std::endl
           << "100 100 #0 # An invalid line that should be disregarded" << std::endl
           << "12 20 0" << std::endl
           << "0 1.618034 1 " << std::endl  // The vertices.
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
           << "3 1 0 4" << std::endl          // The faces.
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
           << "3 3 10 5 # Another comment." << std::endl
           << "3 3 7 11" << std::endl
           << "" << std::endl;

        // Round-trip #1.
        if(!ReadFVSMeshFromOFF(smesh, ss)){
            throw std::runtime_error("Unable to read mesh from OFF stream: 1.");
        }else{
            YLOGINFO("First read OK");
        }
        YLOGINFO("Mesh surface area: " << smesh.surface_area());
        YLOGINFO("Mesh vertex count: " << smesh.vertices.size());
        std::stringstream ss2;
        if(!WriteFVSMeshToBinarySTL(smesh, ss2)){
            throw std::runtime_error("Unable to write mesh to Binary STL stream: 1.");
        }else{
            YLOGINFO("First write OK");
        }
        const auto output_A = ss2.str();

        // Round-trip #2.
        if(!ReadFVSMeshFromBinarySTL(smesh, ss2)){
            throw std::runtime_error("Unable to read mesh from Binary STL stream: 2.");
        }else{
            YLOGINFO("Second read OK");
        }
        YLOGINFO("Mesh surface area: " << smesh.surface_area());
        YLOGINFO("Mesh vertex count: " << smesh.vertices.size());
        std::stringstream ss3;
        if(!WriteFVSMeshToBinarySTL(smesh, ss3)){
            throw std::runtime_error("Unable to write mesh to Binary STL stream: 2.");
        }else{
            YLOGINFO("Second write OK");
        }

        // Round-trip #3.
        if(!ReadFVSMeshFromBinarySTL(smesh, ss3)){
            throw std::runtime_error("Unable to read mesh from Binary STL stream: 3.");
        }else{
            YLOGINFO("Third read OK");
        }
        YLOGINFO("Mesh surface area: " << smesh.surface_area());
        YLOGINFO("Mesh vertex count: " << smesh.vertices.size());
        std::stringstream ss4;
        if(!WriteFVSMeshToBinarySTL(smesh, ss4)){
            throw std::runtime_error("Unable to write mesh to Binary STL stream: 3.");
        }else{
            YLOGINFO("Third write OK");
        }
        const auto output_B = ss4.str();


        if(output_A != output_B){
            throw std::runtime_error("Round-tripping mesh failed.");
        }
        YLOGINFO("Binary STL round-trip tests passed");
    }

    return 0;
}

