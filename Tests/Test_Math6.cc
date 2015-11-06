//Test_Math6.cc - This is a test file for line's.

#include <iostream>
#include <cmath>
#include <functional>
#include <list>

#include "YgorMisc.h"
#include "YgorMath.h"
//#include "YgorMath_Samples.h"
#include "YgorPlot.h"


int main(int argc, char **argv){

    line<double> LA(vec3<double>(0.0, 0.0, 0.0), vec3<double>(1.0, 0.0, 0.0));
    line<double> LB(vec3<double>(0.0, 0.0, 0.0), vec3<double>(0.0, 1.0, 0.0));
    line<double> LC(vec3<double>(1.0, 0.0, 0.0), vec3<double>(1.0, 0.0, 1.0));

    vec3<double> intersection;
  
    if(LA.Intersects_With_Line_Once(LB,intersection)){
        FUNCINFO("Intersection between LA and LB is " << intersection);
    }else{
        FUNCERR("Failed to find intersection between LA and LB");
    }

    if(LB.Intersects_With_Line_Once(LC,intersection)){
        FUNCERR("Found a ficticious intersection point between LB and LC at " << intersection);
    }else{
        FUNCINFO("Correctly determined that LB and LC do not intersect");
    }

    if(LB.Closest_Point_To_Line(LC,intersection)){
        FUNCINFO("Closest point on LB to (any point on) LC is " << intersection);
    }else{
        FUNCERR("Failed to find closest point on LB to LC");
    }

    if(LC.Closest_Point_To_Line(LB,intersection)){
        FUNCINFO("Closest point on LC to (any point on) LB is " << intersection);
    }else{
        FUNCERR("Failed to find closest point on LC to LB");
    }

    if(LC.Closest_Point_To_Line(LC,intersection)){
        FUNCERR("Erroneously found closest point on LC to itself at " << intersection);
    }else{
        FUNCINFO("Correctly determined that the closest point on LC to itself is ill-defined");
    }

    return 0;
}

