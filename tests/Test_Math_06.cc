//Test_Math6.cc - This is a test file for lines and planes.

#include <iostream>
#include <cmath>
#include <functional>
#include <list>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
//#include "YgorMath_Samples.h"
#include "YgorPlot.h"


int main(int argc, char **argv){

    {
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
    }

    {
        line<double> intersection;

        plane<double> PA( vec3<double>(0.0, 0.0, 1.0), vec3<double>(0.0, 0.0, 0.0) );
        plane<double> PB( vec3<double>(0.0, 1.0, 0.0), vec3<double>(0.0, 0.0, 0.0) );

        if(PA.Intersects_With_Plane_Along_Line(PB, intersection)){
            FUNCINFO("Intersection between PA and PB is " << intersection);
        }else{
            FUNCERR("Failed to compute intersection between planes");
        }

        FUNCINFO("Distance between the intersection line and PA:" << 
            PA.Get_Signed_Distance_To_Point(intersection.R_0));
        FUNCINFO("Distance between the intersection line and PA:" << 
            PA.Get_Signed_Distance_To_Point(intersection.R_0 + intersection.U_0 * 10.0));
        FUNCINFO("Distance between the intersection line and PB:" << 
            PB.Get_Signed_Distance_To_Point(intersection.R_0));
        FUNCINFO("Distance between the intersection line and PB:" << 
            PB.Get_Signed_Distance_To_Point(intersection.R_0 + intersection.U_0 * 10.0));
    }

    {
        line<double> intersection;

        plane<double> PA( vec3<double>(0.0, 0.0, 1.0), vec3<double>(1.2, 2.3, 4.5) );
        plane<double> PB( vec3<double>(0.0, 0.1, 0.9).unit(), vec3<double>(5.6, 6.7, 7.8) );

        if(PA.Intersects_With_Plane_Along_Line(PB, intersection)){
            FUNCINFO("Intersection between PA and PB is " << intersection);
        }else{
            FUNCERR("Failed to compute intersection between planes");
        }

        FUNCINFO("Distance between the intersection line and PA:" << 
            PA.Get_Signed_Distance_To_Point(intersection.R_0));
        FUNCINFO("Distance between the intersection line and PA:" << 
            PA.Get_Signed_Distance_To_Point(intersection.R_0 + intersection.U_0 * 10.0));
        FUNCINFO("Distance between the intersection line and PB:" << 
            PB.Get_Signed_Distance_To_Point(intersection.R_0));
        FUNCINFO("Distance between the intersection line and PB:" << 
            PB.Get_Signed_Distance_To_Point(intersection.R_0 + intersection.U_0 * 10.0));
    }

    {
        line<double> intersection;

        plane<double> PA( vec3<double>(0.0, 0.5,  0.5).unit(), vec3<double>(1.2, 2.3, 4.5) );
        plane<double> PB( vec3<double>(0.0, 0.1, -1.0).unit(), vec3<double>(5.6, 6.7, 7.8) );

        if(PA.Intersects_With_Plane_Along_Line(PB, intersection)){
            FUNCINFO("Intersection between PA and PB is " << intersection);
        }else{
            FUNCERR("Failed to compute intersection between planes");
        }

        FUNCINFO("Distance between the intersection line and PA:" << 
            PA.Get_Signed_Distance_To_Point(intersection.R_0));
        FUNCINFO("Distance between the intersection line and PA:" << 
            PA.Get_Signed_Distance_To_Point(intersection.R_0 + intersection.U_0 * 10.0));
        FUNCINFO("Distance between the intersection line and PB:" << 
            PB.Get_Signed_Distance_To_Point(intersection.R_0));
        FUNCINFO("Distance between the intersection line and PB:" << 
            PB.Get_Signed_Distance_To_Point(intersection.R_0 + intersection.U_0 * 10.0));
    }

    {
        line<double> intersection;

        plane<double> PA( vec3<double>(0.0, 0.0, 1.0), vec3<double>(1.2, 2.3, 4.5) );
        plane<double> PB( vec3<double>(0.0, 0.0, 1.0), vec3<double>(5.6, 6.7, 7.8) );

        if(PA.Intersects_With_Plane_Along_Line(PB, intersection)){
            FUNCERR("Erroneously found an intersection between non-intersecting planes");
        }else{
            FUNCINFO("Correctly rejected degenerate case");
        }
    }

    {
        line_segment<double> L( vec3<double>(0.0, 0.0, -1.0), vec3<double>(0.0, 0.0, 1.0) );

        plane<double> PA( vec3<double>(0.0, 0.0, 1.0), vec3<double>(0.0, 0.0, 0.0) );
        plane<double> PB( vec3<double>(2.0, 2.0, 2.0), vec3<double>(0.0, 0.0, 1.1) );

        vec3<double> P;

        if( PA.Intersects_With_Line_Segment_Once(L, P)
        &&  (P.distance(vec3<double>(0.0, 0.0, 0.0)) < 1E-5) ){
            FUNCINFO("Correctly located line_segment-plane intersection");
        }else{
            FUNCERR("Failed to locate line_segment-plane intersection");
        }

        if( !PB.Intersects_With_Line_Segment_Once(L, P) ){
            FUNCINFO("Correctly rejected line_segment-plane intersection");
        }else{
            FUNCERR("Erroneously located an invalid line_segment-plane intersection");
        }
    }

    {
        line_segment<double> L( vec3<double>(0.0, 0.0, -100.0), vec3<double>(0.0, 0.0, 0.1) );

        plane<double> PA( vec3<double>(0.0, 1.0, 1.0), vec3<double>(0.0, 0.0, 0.0) );
        plane<double> PB( vec3<double>(2.0, 2.0, -2.0), vec3<double>(0.0, 0.0, 0.2) );

        vec3<double> P;

        if( PA.Intersects_With_Line_Segment_Once(L, P)
        &&  (P.distance(vec3<double>(0.0, 0.0, 0.0)) < 1E-5) ){
            FUNCINFO("Correctly located line_segment-plane intersection");
        }else{
            FUNCERR("Failed to locate line_segment-plane intersection");
        }

        if( !PB.Intersects_With_Line_Segment_Once(L, P) ){
            FUNCINFO("Correctly rejected line_segment-plane intersection");
        }else{
            FUNCERR("Erroneously located an invalid line_segment-plane intersection");
        }
    }

    return 0;
}

