//Test_Math2.cc - This is a test file for performing contour integration with a user-specified kernel.
//
//The contour is made of straight line segments between points.

#include <iostream>
#include <cmath>
#include <functional>
#include <list>

#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorMath_Samples.h"
#include "YgorPlot.h"


double Kernel_Scalar(const vec3<double> &r,   const vec3<double> &A, const vec3<double> &B, const vec3<double> &U){
    //For a kernel of 1, we get the length of the contour.
//    return 1.0;

    //Some other kernels.
//    return r.length();
    const double  x = r.x,  y = r.y,  z = r.z;
    return x*x*x*exp(-y*y);
}


vec3<double> Kernel_Vector(const vec3<double> &r,   const vec3<double> &A, const vec3<double> &B, const vec3<double> &U){
    //For a kernel of (vector)(1), we get the length of the contour.
//    return vec3<double>(1.0, 1.0, 1.0);

    //Some other kernels.
//    return r.length();
    const double  x = r.x,  y = r.y,  z = r.z;
    return vec3<double>(x*x*y*y, y*exp(-x*x), x*y*z);
}


int main(int argc, char **argv){

    {
    //Create a simple contour.
    class contour_of_points<double> contour({ vec3<double>( 5.0, 10.0, 0.0),
                                              vec3<double>( 0.0, 10.0, 0.0),
                                              vec3<double>( 0.0,  0.0, 0.0),
                                              vec3<double>( 5.0,  0.0, 0.0)  });
    contour.closed = true;
    std::cout << "The scalar contour integration gives: " << contour.Integrate_Simple_Scalar_Kernel(Kernel_Scalar) << std::endl;
    std::cout << "The vector contour integration gives: " << contour.Integrate_Simple_Vector_Kernel(Kernel_Vector) << std::endl;
    }

    {
    //Testing some textbook problems.
    class contour_of_points<double> contour({  vec3<double>( 0.0,  0.0,  0.0),
                                               vec3<double>( 1.0,  2.0,  3.0)  });
    contour.closed = false;
    auto f = [](const vec3<double> &r,   const vec3<double> &A, const vec3<double> &B, const vec3<double> &U) -> double {
        const double  x = r.x,  y = r.y,  z = r.z;
        return x*exp(y*z);
    };

    std::cout << "The vector contour integration (Q11, p1107) gives: " << contour.Integrate_Simple_Scalar_Kernel(f) << std::endl;
    std::cout << " (The solution is sqrt(14)*(exp(6)-1)/12 = " << sqrt(14.0)*(exp(6.0)-1.0)/12.0 << std::endl;
    }


    {
    //Create a simple contour.
    class contour_of_points<double> contour({ vec3<double>( 1.0/9.0, 10.0, 0.0),
                                              vec3<double>( 0.0, 10.0, 0.0),
                                              vec3<double>( 0.0,  -72.123, 0.0),
                                              vec3<double>( 5.0,  0.0, 0.0)  });
    contour.closed = true;

    class contour_of_points<double> contour2({ vec3<double>( 5.0, 10.0, 0.0),
                                               vec3<double>( 0.1, 10.0, 0.0),
                                               vec3<double>( 0.0,  0.0, 0.0),
                                               vec3<double>( 5.0,  0.0, 0.0)  });
    contour2.closed = true;

    std::cout << "Contour1 is equal to itself:   " << !!(contour == contour) << std::endl;
    std::cout << "Contour2 is equal to itself:   " << !!(contour2 == contour2) << std::endl;
    std::cout << "Contour1 is equal to contour2: " << !!(contour == contour2) << std::endl;

    auto stringified = contour.write_to_string();
    std::cout << "Contour1 stringified to: '" << stringified << "'" << std::endl;
    if(contour.load_from_string(stringified)){
        std::cout << "Was able to reload the data back in." << std::endl;
    }else{
        std::cout << "Unable to reload the data back in. This is an error." << std::endl;
    }

    }

    {
        contour_of_points<double> contour(contour_of_points_sample_gumby());
        contour.closed = true;

        auto contour2 = contour.Resample_Evenly_Along_Perimeter(5000);

        std::list<contour_of_points<double>> collection({ contour, contour2 });            
 
        Plot_Container_of_Contour_of_Points(collection.begin(), collection.end());

        contour2.Plot();
    }

    {
        const vec3<double> A(10.0, 0.0, 0.0);
        const vec3<double> B(11.0, 0.0, 0.0);

        const line_segment<double> line(A,B);

        double spacing = 0.3, offset = 0.05, remain = 0.0;
        auto somepoints = line.Sample_With_Spacing(spacing, offset, remain); //'remain' is adjusted each time.
        for(auto it = somepoints.begin(); it != somepoints.end(); ++it){
            FUNCINFO("Sampled a point at " << *it );
        }
        FUNCINFO("Spacing is " << spacing << ", offset is " << offset << " and remaining is now " << remain << "  . We got " << somepoints.size() << " points, and the dl between contour points was " << A.distance(B));

    }

    return 0;
}

