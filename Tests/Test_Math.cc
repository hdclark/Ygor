#include <iostream>
#include <list>
#include <cmath>

#include "YgorMath.h"
#include "YgorPlot.h"
#include "YgorString.h" //Needed for Xtostring(..)

int main(int argc, char **argv){


    //Test contour_of_points by computing the center of area for a simple contour.
    contour_of_points<double> C;
    C.closed = true;
  
    //Simple right triangle.
//    C.points.push_back( vec3<double>( 0.0, 0.0, 0.0 ) );
//    C.points.push_back( vec3<double>( 1.0, 0.0, 0.0 ) );
//    C.points.push_back( vec3<double>( 1.0, 1.0, 0.0 ) );

    //Simple right triangle - opposite orientation.
//    C.points.push_back( vec3<double>( 0.0, 0.0, 0.0 ) );
//    C.points.push_back( vec3<double>( 1.0, 0.0, 0.0 ) );
//    C.points.push_back( vec3<double>( 0.0, 1.0, 0.0 ) );

    //Simple isosoles triangle.
//    C.points.push_back( vec3<double>( 0.0, 0.0, 0.0 ) );
//    C.points.push_back( vec3<double>( 1.0, 0.0, 0.0 ) );
//    C.points.push_back( vec3<double>( 0.5, 0.25*std::sqrt(3.0), 0.0 ) );

    //Simple square.
//    C.points.push_back( vec3<double>( 0.0, 0.0, 0.0 ) );
//    C.points.push_back( vec3<double>( 1.0, 0.0, 0.0 ) );
//    C.points.push_back( vec3<double>( 1.0, 1.0, 0.0 ) );
//    C.points.push_back( vec3<double>( 0.0, 1.0, 0.0 ) );

    //Simple trapezoid-like thing.
//    C.points.push_back( vec3<double>( -1.0,  2.0,  0.0) ); 
//    C.points.push_back( vec3<double>( -1.0, -2.0,  0.0) );
//    C.points.push_back( vec3<double>(  4.0, -3.0,  0.0) );
//    C.points.push_back( vec3<double>(  4.0,  3.0,  0.0) );

    //A more complex beast (will result in multiple contour splits.)
    C.points.push_back( vec3<double>( -1, 0, 0) );
    C.points.push_back( vec3<double>( -1, 2, 0) );
    C.points.push_back( vec3<double>( -2, 2, 0) );
    C.points.push_back( vec3<double>( -2, -2, 0) );
    C.points.push_back( vec3<double>(  2, -2, 0) );
    C.points.push_back( vec3<double>( 2, 2, 0) );
    C.points.push_back( vec3<double>( 1, 2, 0) );
    C.points.push_back( vec3<double>( 1, 0, 0) );

   
    //Similar to previous, but no abcesses. 
//    C.points.push_back( vec3<double>( -0.5, 3.0, 0.0) );
//    C.points.push_back( vec3<double>( -1.0, 2.0, 0.0) );
//    C.points.push_back( vec3<double>( -2.0, 2.0, 0.0) );
//    C.points.push_back( vec3<double>( -2.0, -2.0, 0.0) );
//    C.points.push_back( vec3<double>(  2.0, -2.0, 0.0) );
//    C.points.push_back( vec3<double>( 2.0, 2.0, 0.0) );
//    C.points.push_back( vec3<double>( 1.0, 2.0, 0.0) );
//    C.points.push_back( vec3<double>( 0.5, 2.5, 0.0) );


    std::cout << "The area of the original contour is " << C.Get_Signed_Area() << std::endl;

    //Orient the points in the counter clockwise direction. This is useful if they are entered by hand in the negative orientation.
    C.Reorient_Counter_Clockwise();

    const vec3<double> R = C.Centroid();
    std::cout << "The centroid of the contour is " << R ;
    std::cout << " [the average point is " << C.Average_Point()  << " ] ";
//    C.Plot();

    const vec3<double> which_R( R );


    //Now we would like to split the contour with a line which intersects R. For illustration, we compute a large number of splits
    // with an arbitrary orientation. 
    double total_moment_abs_error = 0.0;
    for(double x = 0.01; x < 3.14159265; x += 0.1){
        const vec3<double> normal( vec3<double>(std::cos(x),std::sin(x),0.0).unit() );

        std::cout << "Splitting contour along plane defined with normal " << normal << " which intersects point " << which_R << std::endl;
        std::list<contour_of_points<double> > C4 = C.Split_Along_Plane( plane<double>(normal, which_R) );

        vec3<double> total_moment(0.0, 0.0, 0.0);
        for(auto i = C4.begin(); i != C4.end(); i++ ){
            //Reorient the contours clockwise. This is almost certainly not needed (unless the routine has been fed improperly-oriented points.)
            // This is here to demonstrate an appropriate use of the function. 
            i->Reorient_Counter_Clockwise();

            //Spit out the area of the split and some info about it.
            const double area = i->Get_Signed_Area();
            std::cout << "  Split contour # " << 1+std::distance(C4.begin(), i) << " of " << C4.size() << " has area " << area << " and is composed of the points ";
            for(auto j = i->points.begin(); j != i->points.end(); j++) std::cout << " " << *j ;
            std::cout << std::endl;
       
            //Sum the moment from each split about the centroid R (we assume the moment is produced by the area acting in some arbitrary
            // but uniform direction over the entire body.) An example of this might be a homogeneous mass density acting under classical gravity.
            const vec3<double> cent   = i->Centroid();
            // $\vec{M} = (\vec{R} - \vec{R}_{0}) \cross \vec{F}$. We consider the 'force' to act at the centroid (center of mass) of the contours.
            const vec3<double> moment = (cent - R).Cross( (vec3<double>(1.0, 2.0, 0.0)).unit() ) * area;
            total_moment += moment;
        }
        total_moment_abs_error += total_moment.length();
        std::cout << "  The total moment about the centroid R (along some arbitrary direction) is " << total_moment << std::endl;

        //Place the point R on the plot so we can examine whether or not the split occured where we wanted it.
        contour_of_points<double> temp;
        temp.closed = true;
        temp.points.push_back( which_R );
        C4.push_back( temp );

        //Plot the contours. We plot all the splits on one plot - they will be in different colours.
        //Plot_Container_of_Contour_of_Points( C4.begin(), C4.end() );
        Plot_Container_of_Contour_of_Points_to_File( C4.begin(), C4.end(), std::string("/tmp/triangles_") + Xtostring<double>(x) + ".ps");
    }

    std::cout << "The total (absolute value of the) moment's error is " << total_moment_abs_error << std::endl;
    return 0;
}
