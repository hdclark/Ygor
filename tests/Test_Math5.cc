//Test_Math5.cc - This is a test file for contour_collection's.

#include <iostream>
#include <cmath>
#include <functional>
#include <list>

#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorMath_Samples.h"
#include "YgorPlot.h"


int main(int argc, char **argv){

    //Push back some contours.
    contour_collection<double> cc;
    {
        contour_of_points<double> contour(contour_of_points_sample_airplane());
        contour.Reorient_Counter_Clockwise();
        contour_of_points<double> contour2 = contour.Resample_Evenly_Along_Perimeter(100L);

        for(auto p_it = contour2.points.begin(); p_it != contour2.points.end(); ++p_it){
            p_it->z += static_cast<double>(1.23);
        }
        cc.contours.push_back(contour2);
    }

    cc.Plot();

    const auto centroid = cc.Average_Point();
    FUNCINFO("The center of the contour collection is at " << centroid);
    const auto splits = cc.Split_Along_Plane(plane<double>(vec3<double>(1.0,1.0,0.0).unit(), centroid));

    for(auto it = splits.begin(); it != splits.end(); ++it){
        it->Plot();

    }

    return 0;
}

