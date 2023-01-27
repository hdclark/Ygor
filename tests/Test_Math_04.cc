//Test_Math4.cc - This is a test file for contour_collection's.

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

    //Test the angle-between-vectors code.
    {
      vec3<double> A(1.0,0.0,0.0);
      vec3<double> B(0.0,1.0,0.0);
      std::cout << A.angle(B,nullptr) << std::endl; //1.5708 == pi/2.

      A = vec3<double>(1.0,0.0,0.0);
      B = vec3<double>(1.0,0.0,0.0);
      std::cout << A.angle(B,nullptr) << std::endl; //0.0.

      A = vec3<double>(0.8,0.0,0.0);
      B = vec3<double>(1.0,0.0,0.0);
      std::cout << A.angle(B,nullptr) << std::endl; //0.0.

      A = vec3<double>(1.0,0.1,0.0);
      B = vec3<double>(1.0,0.0,0.0);
      std::cout << A.angle(B,nullptr) << std::endl; //0.0996687.

      A = vec3<double>(-1.0,0.0,0.0);
      B = vec3<double>(1.0,0.0,0.0);
      std::cout << A.angle(B,nullptr) << std::endl;  //3.14159.
    }

    //Push back some contours.
    contour_collection<double> cc;
    for(int i=0; i<=10; ++i){
        contour_of_points<double> contour(contour_of_points_sample_gumby());
        contour.Reorient_Counter_Clockwise();
        contour_of_points<double> contour2 = contour.Resample_Evenly_Along_Perimeter(250L);

        //Shift the z-coordinate up by a small amount to show some 3D action!
        for(auto p_it = contour2.points.begin(); p_it != contour2.points.end(); ++p_it){
            p_it->z += static_cast<double>(i);
        }

        cc.contours.push_back(contour2);
    }

    //Spit out some info about the contour_collection.
    YLOGINFO("Contour collection has signed area:         " << cc.Get_Signed_Area());
    YLOGINFO("Contour collection is counter-clockwise:    " << !!cc.Is_Counter_Clockwise());
    YLOGINFO("Contour collection has average point:       " << cc.Average_Point());
    YLOGINFO("Contour collection has centroid:            " << cc.Centroid());
    YLOGINFO("Contour collection has perimeter:           " << cc.Perimeter());
    YLOGINFO("Contour collection has average perimeter:   " << cc.Average_Perimeter());
    const std::string crystalizedcc = cc.write_to_string();
    if(!cc.load_from_string(crystalizedcc)){
        YLOGERR("Failed to load contour collection from stringified contour collection");
    }else{
        YLOGINFO("Contour collection was successfully loaded from stringified contour collection");
    }

    //Spit out some info about one of the contours.
    auto c = *cc.contours.begin();

    YLOGINFO("Contour has signed area:         " << c.Get_Signed_Area());
    YLOGINFO("Contour is counter-clockwise:    " << !!c.Is_Counter_Clockwise());
    YLOGINFO("Contour has average point:       " << c.Average_Point());
    YLOGINFO("Contour has centroid:            " << c.Centroid());
    YLOGINFO("Contour has perimeter:           " << c.Perimeter());
    const std::string crystalizedc = c.write_to_string();
    if(!c.load_from_string(crystalizedc)){
        YLOGERR("Failed to load contour from stringified contour");
    }else{
        YLOGINFO("Contour was successfully loaded from stringified contour");
    }


    //Plot the contour_collection.
    cc.Plot();

    return 0;
}

