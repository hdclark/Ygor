//Test_Math14.cc - Tests of projection onto a line and cylindrical geometry.

#include <iostream>
#include <cmath>
#include <functional>
#include <list>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"


int main(int argc, char **argv){


    {
        line<double> L( vec3<double>(0.0, 0.0, 0.0), 
                        vec3<double>(0.0, 0.0, 1.0) );

        vec3<double> P(1.23, 4.56, 2.0);

        std::cout << "Projection of point " << P << " onto line [ R=" 
                  << L.R_0 << ", U=" << L.U_0 
                  << "] yields point " << L.Project_Point_Orthogonally(P) << std::endl;
        std::cout << "Should be (0,0,2)." << std::endl;
    }

    {
        line_segment<double> L( vec3<double>(0.0, 0.0, 0.0), 
                                vec3<double>(0.0, 0.0, 1.0) );
        double radius = 1.0;

        vec3<double> P1(1.23, 4.56, 2.0);
        vec3<double> P2(0.15, 0.25, 0.5);
        vec3<double> P3(0.00, 0.00, 3.0);
        vec3<double> P4(0.00, 0.00,-0.5);

        std::cout << "P1 within cylinder? --> " << !!(L.Within_Cylindrical_Volume(P1, radius)) << std::endl;
        std::cout << "    (should be 0)" << std::endl;

        std::cout << "P2 within cylinder? --> " << !!(L.Within_Cylindrical_Volume(P2, radius)) << std::endl;
        std::cout << "    (should be 1)" << std::endl;

        std::cout << "P3 within cylinder? --> " << !!(L.Within_Cylindrical_Volume(P3, radius)) << std::endl;
        std::cout << "    (should be 0)" << std::endl;

        std::cout << "P4 within cylinder? --> " << !!(L.Within_Cylindrical_Volume(P4, radius)) << std::endl;
        std::cout << "    (should be 0)" << std::endl;
    }

    return 0;
}

