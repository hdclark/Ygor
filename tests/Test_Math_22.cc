//Test_Math22.cc - This is a test file for vec3 rotations.

#include <iostream>
#include <sstream>
#include <cmath>
#include <string>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMathIOOFF.h"
#include "YgorMathIOOBJ.h"


int main(int argc, char **argv){
  
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    {
        vec3<double> A( 1.0, 0.0, 0.0 );  // Test vector.
        vec3<double> B( 0.0, 0.0, 1.0 );  // Rotation axis.
        vec3<double> E(-1.0, 0.0, 0.0 );  // Expected result.
        const auto C = A.rotate_around_unit(B, M_PI);

        if(eps < C.distance(E)){
            std::stringstream ss;
            ss << "Rotation (1) around " << B << " failed";
            throw std::runtime_error(ss.str());
        }else{
            FUNCINFO("Rotation (1) around " << B << " was OK");
        }
    }

    {
        vec3<double> A( 2.0, 0.0, 0.0 );  // Test vector.
        vec3<double> B( 0.0, 0.0, 1.0 );  // Rotation axis.
        vec3<double> E(-2.0, 0.0, 0.0 );  // Expected result.
        const auto C = A.rotate_around_unit(B, M_PI);

        if(eps < C.distance(E)){
            std::stringstream ss;
            ss << "Rotation (2) around " << B << " failed";
            throw std::runtime_error(ss.str());
        }else{
            FUNCINFO("Rotation (2) around " << B << " was OK");
        }
    }

    {
        vec3<double> A( 2.0, 0.0, 0.0 );  // Test vector.
        vec3<double> B( 0.0, 1.0, 0.0 );  // Rotation axis.
        vec3<double> E(-2.0, 0.0, 0.0 );  // Expected result.
        const auto C = A.rotate_around_unit(B, M_PI);

        if(eps < C.distance(E)){
            std::stringstream ss;
            ss << "Rotation (3) around " << B << " failed";
            throw std::runtime_error(ss.str());
        }else{
            FUNCINFO("Rotation (3) around " << B << " was OK");
        }
    }

    {
        vec3<double> A( 2.0, 0.0, 0.0 );  // Test vector.
        vec3<double> B( 0.0, 1.0, 0.0 );  // Rotation axis.
        vec3<double> E(-2.0, 0.0, 0.0 );  // Expected result.
        const auto C = A.rotate_around_unit(B, -M_PI);

        if(eps < C.distance(E)){
            std::stringstream ss;
            ss << "Rotation (4) around " << B << " failed";
            throw std::runtime_error(ss.str());
        }else{
            FUNCINFO("Rotation (4) around " << B << " was OK");
        }
    }

    {
        vec3<double> A( 0.0, 2.0, 0.0 );  // Test vector.
        vec3<double> B( 1.0, 0.0, 0.0 );  // Rotation axis.
        vec3<double> E( 0.0,-2.0, 0.0 );  // Expected result.
        const auto C = A.rotate_around_unit(B, M_PI);

        if(eps < C.distance(E)){
            std::stringstream ss;
            ss << "Rotation (5) around " << B << " failed";
            throw std::runtime_error(ss.str());
        }else{
            FUNCINFO("Rotation (5) around " << B << " was OK");
        }
    }

    {
        vec3<double> A( 0.0, 2.0, 0.0 );  // Test vector.
        vec3<double> B( 0.0, 0.0, 1.0 );  // Rotation axis.
        vec3<double> E( 0.0,-2.0, 0.0 );  // Expected result.
        const auto C = A.rotate_around_unit(B, M_PI);

        if(eps < C.distance(E)){
            std::stringstream ss;
            ss << "Rotation (6) around " << B << " failed";
            throw std::runtime_error(ss.str());
        }else{
            FUNCINFO("Rotation (6) around " << B << " was OK");
        }
    }

    {
        vec3<double> A( 0.0, 2.0, 0.0 );  // Test vector.
        vec3<double> B( 0.0, 0.0, 1.0 );  // Rotation axis.
        vec3<double> E( 0.0, 2.0, 0.0 );  // Expected result.
        const auto C = A.rotate_around_unit(B, 2.0 * M_PI);

        if(eps < C.distance(E)){
            std::stringstream ss;
            ss << "Rotation (7) around " << B << " failed";
            throw std::runtime_error(ss.str());
        }else{
            FUNCINFO("Rotation (7) around " << B << " was OK");
        }
    }

    {
        vec3<double> A( 0.0, 2.0, 0.0 );  // Test vector.
        vec3<double> B( 0.0, 1.0, 0.0 );  // Rotation axis.
        vec3<double> E( 0.0, 2.0, 0.0 );  // Expected result.
        const auto C = A.rotate_around_unit(B, 2.0 * M_PI);

        if(eps < C.distance(E)){
            std::stringstream ss;
            ss << "Rotation (8) around " << B << " failed";
            throw std::runtime_error(ss.str());
        }else{
            FUNCINFO("Rotation (8) around " << B << " was OK");
        }
    }

    {
        vec3<double> A( 0.0, 2.0, 0.0 );  // Test vector.
        vec3<double> B( 1.0, 0.0, 0.0 );  // Rotation axis.
        vec3<double> E( 0.0, 2.0, 0.0 );  // Expected result.
        const auto C = A.rotate_around_unit(B, 2.0 * M_PI);

        if(eps < C.distance(E)){
            std::stringstream ss;
            ss << "Rotation (9) around " << B << " failed";
            throw std::runtime_error(ss.str());
        }else{
            FUNCINFO("Rotation (9) around " << B << " was OK");
        }
    }

    {
        vec3<double> A( 1.0, 2.0, 3.0 );  // Test vector.
        vec3<double> B( 1.0, 0.0, 0.0 );  // Rotation axis.
        vec3<double> E( 1.0, 2.0, 3.0 );  // Expected result.
        const auto C = A.rotate_around_unit(B, 2.0 * M_PI);

        if(eps < C.distance(E)){
            std::stringstream ss;
            ss << "Rotation (10) around " << B << " failed";
            throw std::runtime_error(ss.str());
        }else{
            FUNCINFO("Rotation (10) around " << B << " was OK");
        }
    }

    return 0;
}

