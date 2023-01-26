//Test_Math18.cc - Tests of spherical regression.

#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <functional>
#include <list>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorStats.h"

int main(int argc, char **argv){

    std::cout << std::endl;
    {
        // Fit a sphere to the corners of a cube.
        std::vector< vec3<double> > thepoints{
                     vec3<double>( -1.0, -1.0, -1.0 ),
                     vec3<double>( -1.0, -1.0,  1.0 ),
                     vec3<double>( -1.0,  1.0, -1.0 ),
                     vec3<double>( -1.0,  1.0,  1.0 ),
                     vec3<double>(  1.0, -1.0, -1.0 ),
                     vec3<double>(  1.0, -1.0,  1.0 ),
                     vec3<double>(  1.0,  1.0, -1.0 ),
                     vec3<double>(  1.0,  1.0,  1.0 ) };

        const auto thesphere = Sphere_Orthogonal_Regression(thepoints);
        std::cout << "The fitted sphere should have C = "
                  << "(0.0, 0.0, 0.0)"
                  << " and R = sqrt(3) = 1.732050808" << std::endl;
        std::cout << " It actually has C = " << thesphere.C_0
                  << " and R = " << thesphere.r_0 << std::endl;
    }

    std::cout << std::endl;
    {
        // Fit a sphere to the corners of an elongated.
        std::vector< vec3<double> > thepoints{
                     vec3<double>( -2.0, -2.0, -2.0 ),
                     vec3<double>( -1.0, -1.0,  1.0 ),
                     vec3<double>( -1.0,  1.0, -1.0 ),
                     vec3<double>( -1.0,  1.0,  1.0 ),
                     vec3<double>(  1.0, -1.0, -1.0 ),
                     vec3<double>(  1.0, -1.0,  1.0 ),
                     vec3<double>(  1.0,  1.0, -1.0 ),
                     vec3<double>(  2.0,  2.0,  2.0 ) };

        const auto thesphere = Sphere_Orthogonal_Regression(thepoints);
        std::cout << "The fitted sphere should have C = "
                  << "(0.0, 0.0, 0.0)"
                  << " and R = sqrt(3)*3/4 + sqrt(12)*1/4 = 2.165063509" << std::endl;
        std::cout << " It actually has C = " << thesphere.C_0
                  << " and R = " << thesphere.r_0 << std::endl;
    }

    std::cout << std::endl;
    {
        // Fit a sphere to a square.
        std::vector< vec3<double> > thepoints{
                     vec3<double>( -1.0, -1.0,  0.0 ),
                     vec3<double>( -1.0,  1.0,  0.0 ),
                     vec3<double>(  1.0, -1.0,  0.0 ),
                     vec3<double>(  1.0,  1.0,  0.0 ) };

        const auto thesphere = Sphere_Orthogonal_Regression(thepoints);
        std::cout << "The fitted sphere should have C = "
                  << "(0.0, 0.0, 0.0)"
                  << " and R = sqrt(2) = 1.414213562" << std::endl;
        std::cout << " It actually has C = " << thesphere.C_0
                  << " and R = " << thesphere.r_0 << std::endl;
    }

    std::cout << std::endl;
    {
        // Fit a sphere to some ambiguously non-planar data (two duplicate points).
        std::vector< vec3<double> > thepoints{
                     vec3<double>( -2.0, 0.0, 0.0 ),
                     vec3<double>( -2.0, 0.0, 0.0 ),
                     vec3<double>(  2.0, 0.0, 0.0 ),
                     vec3<double>(  2.0, 0.0, 0.0 ) };

        const auto thesphere = Sphere_Orthogonal_Regression(thepoints);
        std::cout << "The fitted sphere should have C = "
                  << "(0.0, 0.0, 0.0)"
                  << " and R = 2.0" << std::endl;
        std::cout << " It actually has C = " << thesphere.C_0
                  << " and R = " << thesphere.r_0 << std::endl;
    }

    std::cout << std::endl;
    try{
        // Fit a sphere to some degenerate data.
        std::vector< vec3<double> > thepoints{
                     vec3<double>( 1.0, 1.0, 1.0 ),
                     vec3<double>( 1.0, 1.0, 1.0 ),
                     vec3<double>( 1.0, 1.0, 1.0 ),
                     vec3<double>( 1.0, 1.0, 1.0 ) };

        const auto thesphere = Sphere_Orthogonal_Regression(thepoints);
        std::cout << "Sphere fitted to degenerate data has C = " << thesphere.C_0
                  << " and R = " << thesphere.r_0 << std::endl;

        FUNCWARN("Possible to get here due to numerical instabilities -- routine will ideally throw due to ambiguously degenerate data!");
    }catch(const std::exception &e){
        std::cout << "Successfully failed to continue in presence of ambiguously degenerate data:"
                  << std::endl
                  << "  " << e.what()
                  << std::endl;
    }

    return 0;
}

