//Test_Math17.cc - Tests of planar regression.

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
        // Fit a plane to some co-planar data.
        std::vector< vec3<double> > thepoints{
                     vec3<double>( 1.0, 1.0, 5.0 ),
                     vec3<double>( 1.0, 2.0, 5.0 ),
                     vec3<double>( 2.0, 1.0, 5.0 ) };

        const auto theplane = Plane_Orthogonal_Regression(thepoints);
        std::cout << "The fitted plane should have R = "
                  << "(1.33333, 1.33333, 5.0)"
                  << " and N = (0, 0, 1) or (0, 0, -1)" << std::endl;
        std::cout << " It actually has R = " << theplane.R_0
                  << " and N = " << theplane.N_0 << std::endl;
    }

    std::cout << std::endl;
    {
        // Fit a plane to some co-planar data.
        std::vector< vec3<double> > thepoints{
                     vec3<double>( 5.0, 1.0, 1.0 ),
                     vec3<double>( 5.0, 2.0, 1.0 ),
                     vec3<double>( 5.0, 1.0, 2.0 ) };

        const auto theplane = Plane_Orthogonal_Regression(thepoints);
        std::cout << "The fitted plane should have R = "
                  << "(5.0, 1.33333, 1.33333)"
                  << " and N = (1, 0, 0) or (-1, 0, 0)" << std::endl;
        std::cout << " It actually has R = " << theplane.R_0
                  << " and N = " << theplane.N_0 << std::endl;
    }

    std::cout << std::endl;
    {
        // Fit a plane to some co-planar data.
        std::vector< vec3<double> > thepoints{
                     vec3<double>( 1.0, 5.0, 1.0 ),
                     vec3<double>( 2.0, 5.0, 1.0 ),
                     vec3<double>( 1.0, 5.0, 2.0 ) };

        const auto theplane = Plane_Orthogonal_Regression(thepoints);
        std::cout << "The fitted plane should have R = "
                  << "(1.33333, 5, 1.33333)"
                  << " and N = (0, 1, 0) or (0, -1, 0)" << std::endl;
        std::cout << " It actually has R = " << theplane.R_0
                  << " and N = " << theplane.N_0 << std::endl;
    }

    std::cout << std::endl;
    try{
        // Fit a plane to some ambiguously non-planar data (co-linear data).
        std::vector< vec3<double> > thepoints{
                     vec3<double>( 0.0, 0.0, 0.0 ),
                     vec3<double>( 1.0, 1.0, 1.0 ),
                     vec3<double>( 2.0, 2.0, 2.0 ) };

        Plane_Orthogonal_Regression(thepoints);
        FUNCERR("Should not get here -- routine should throw due to co-linear data!");
    }catch(const std::exception &e){
        std::cout << "Successfully failed to continue in presence of co-linear data:"
                  << std::endl
                  << "  " << e.what()
                  << std::endl;
    }

    std::cout << std::endl;
    try{
        // Fit a plane to some ambiguously non-planar data (a cube).
        std::vector< vec3<double> > thepoints{
                     vec3<double>( 0.0, 0.0, 0.0 ),
                     vec3<double>( 5.0, 0.0, 0.0 ),
                     vec3<double>( 0.0, 5.0, 0.0 ),
                     vec3<double>( 5.0, 5.0, 0.0 ),

                     vec3<double>( 0.0, 0.0, 5.0 ),
                     vec3<double>( 5.0, 0.0, 5.0 ),
                     vec3<double>( 0.0, 5.0, 5.0 ),
                     vec3<double>( 5.0, 5.0, 5.0 ) };

        const auto theplane = Plane_Orthogonal_Regression(thepoints);
        std::cout << "Plane fitted to ambiguously non-planar data has R = " << theplane.R_0
                  << " and N = " << theplane.N_0 << std::endl;

        FUNCWARN("Possible to get here due to numerical instabilities -- routine will ideally throw due to ambiguously non-planar data!");
    }catch(const std::exception &e){
        std::cout << "Successfully failed to continue in presence of ambiguously non-planar data:"
                  << std::endl
                  << "  " << e.what()
                  << std::endl;
    }


    return 0;
}

