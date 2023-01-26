//Test_Math13.cc - This is a test file for contour_collection's.

#include <iostream>
#include <cmath>
#include <functional>
#include <list>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMath_Samples.h"
#include "YgorPlot.h"


int main(int argc, char **argv){

    //Create a contour collection by stacking some contours.
    contour_collection<double> cc;
    {
        contour_of_points<double> contour(contour_of_points_sample_airplane());
        contour.Reorient_Counter_Clockwise();

       
        for(double dz = 1.0 ; dz <= 10.0 ; dz += 1.0){
            contour_of_points<double> contour2 = contour.Resample_Evenly_Along_Perimeter(100L);

            for(auto p_it = contour2.points.begin(); p_it != contour2.points.end(); ++p_it){
                p_it->z += dz;
            }
            cc.contours.push_back(contour2);
        }
    }
    cc.Plot();


    //Split the contour collection such that the split comes closest to the desired total area above a plane.
    {
        //const vec3<double> planar_unit_normal = vec3<double>(0.03, 0.0, 1.0).unit(); //Defines 'up' and the planar orientation.
        //const double desired_total_area_fraction_above_plane = 0.36; //Here 'above' means in the positive normal direction.
        //const double acceptable_deviation = 0.01; //Deviation from desired_total_area_fraction_above_plane.
        //const size_t max_iters = 100; //If the tolerance cannot be reached after this many iters, report the current plane as-is.

        const vec3<double> planar_unit_normal = vec3<double>(0.03, 0.0, 1.0).unit(); //Defines 'up' and the planar orientation.
        const double desired_total_area_fraction_above_plane = 0.361234; //Here 'above' means in the positive normal direction.
        const double acceptable_deviation = 0.00001; //Deviation from desired_total_area_fraction_above_plane.
        const size_t max_iters = 100; //If the tolerance cannot be reached after this many iters, report the current plane as-is.

        plane<double> final_plane;
        size_t iters_taken = 0;
        double final_area_frac = std::numeric_limits<double>::quiet_NaN();

        const auto splits = cc.Total_Area_Bisection_Along_Plane(planar_unit_normal,
                                                                desired_total_area_fraction_above_plane,
                                                                acceptable_deviation,
                                                                max_iters,
                                                                &final_plane,
                                                                &iters_taken,
                                                                &final_area_frac);

        FUNCINFO("Using bisection, the fraction of planar area above the final plane was " << final_area_frac);
        FUNCINFO(iters_taken << " iterations were taken");
                                                                
        for(auto it = splits.begin(); it != splits.end(); ++it){
            it->Plot();
        }
    }

    return 0;
}

