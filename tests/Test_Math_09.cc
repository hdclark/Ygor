
#include <iostream>
#include <cmath>
#include <list>
#include <functional>
#include <tuple>
#include <vector>
#include <random>
#include <sstream>
#include <limits>
#include <cstdint>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorStats.h"
#include "YgorPlot.h"
#include "YgorFilesDirs.h"
#include "YgorImages.h"
#include "YgorImagesIO.h"
#include "YgorImagesPlotting.h"

int main(int, char **){
  
    //A simple square.
    contour_of_points<double> cont;
    cont.points.emplace_back( 0.0, 0.0, 1.0 );
    cont.points.emplace_back( 1.0, 0.0, 1.0 );
    cont.points.emplace_back( 1.0, 1.0, 1.0 );
    cont.points.emplace_back( 0.0, 1.0, 1.0 );
    cont.closed = true;
 
    const vec3<double>   x_norm(1.0, 0.0, 0.0);
    const vec3<double>   y_norm(0.0, 1.0, 0.0);
    const vec3<double>   z_norm(0.0, 0.0, 1.0);

    const vec3<double>  xy_norm(1.0, 1.0, 0.0);
    const vec3<double>  yz_norm(0.0, 1.0, 1.0);
    const vec3<double> xyz_norm(1.0, 1.0, 1.0);

    YLOGINFO("Least-squares best-fit plane along " <<   x_norm << " has R_0 = " << cont.Least_Squares_Best_Fit_Plane(  x_norm).R_0);
    YLOGINFO("Least-squares best-fit plane along " <<   y_norm << " has R_0 = " << cont.Least_Squares_Best_Fit_Plane(  y_norm).R_0);
    YLOGINFO("Least-squares best-fit plane along " <<   z_norm << " has R_0 = " << cont.Least_Squares_Best_Fit_Plane(  z_norm).R_0);

    YLOGINFO("Least-squares best-fit plane along " <<  xy_norm << " has R_0 = " << cont.Least_Squares_Best_Fit_Plane( xy_norm).R_0);
    YLOGINFO("Least-squares best-fit plane along " <<  yz_norm << " has R_0 = " << cont.Least_Squares_Best_Fit_Plane( yz_norm).R_0);
    YLOGINFO("Least-squares best-fit plane along " << xyz_norm << " has R_0 = " << cont.Least_Squares_Best_Fit_Plane(xyz_norm).R_0);


    YLOGINFO("Proceeding with least-squares best-fit plane along " << z_norm);
    const auto ls_plane = cont.Least_Squares_Best_Fit_Plane(z_norm);

    vec3<double> p;

    p = vec3<double>(0.0, 0.0, -99.0);
    YLOGINFO("Is point " << p << " in the contour?  " << !!cont.Is_Point_In_Polygon_Projected_Orthogonally(ls_plane,p));
    p = vec3<double>(1.0, 0.0, -99.0);
    YLOGINFO("Is point " << p << " in the contour?  " << !!cont.Is_Point_In_Polygon_Projected_Orthogonally(ls_plane,p));
    p = vec3<double>(1.0, 1.0, -99.0);
    YLOGINFO("Is point " << p << " in the contour?  " << !!cont.Is_Point_In_Polygon_Projected_Orthogonally(ls_plane,p));
    p = vec3<double>(0.0, 1.0, -99.0);
    YLOGINFO("Is point " << p << " in the contour?  " << !!cont.Is_Point_In_Polygon_Projected_Orthogonally(ls_plane,p));


    p = vec3<double>(0.0, 0.5, -99.0);
    YLOGINFO("Is point " << p << " in the contour?  " << !!cont.Is_Point_In_Polygon_Projected_Orthogonally(ls_plane,p));
    p = vec3<double>(0.5, 1.0, -99.0);
    YLOGINFO("Is point " << p << " in the contour?  " << !!cont.Is_Point_In_Polygon_Projected_Orthogonally(ls_plane,p));
    p = vec3<double>(0.5, 0.0, -99.0);
    YLOGINFO("Is point " << p << " in the contour?  " << !!cont.Is_Point_In_Polygon_Projected_Orthogonally(ls_plane,p));
    p = vec3<double>(1.0, 0.5, -99.0);
    YLOGINFO("Is point " << p << " in the contour?  " << !!cont.Is_Point_In_Polygon_Projected_Orthogonally(ls_plane,p));


    p = vec3<double>(0.5, 0.5, -99.0);
    YLOGINFO("Is point " << p << " in the contour?  " << !!cont.Is_Point_In_Polygon_Projected_Orthogonally(ls_plane,p));
    p = vec3<double>(0.5, 1.5, -99.0);
    YLOGINFO("Is point " << p << " in the contour?  " << !!cont.Is_Point_In_Polygon_Projected_Orthogonally(ls_plane,p));
    p = vec3<double>(0.5, 1.0, -99.0);
    YLOGINFO("Is point " << p << " in the contour?  " << !!cont.Is_Point_In_Polygon_Projected_Orthogonally(ls_plane,p));


    //Generate an image using the Is_Point_In_Polygon_Projected_Orthogonally() routine.
    {
        const int64_t rows = 1000; //ymax, height
        const int64_t cols = 2000; //xmax, width
        const int64_t cnls = 1;
        planar_image<uint32_t,double> img;
    
        img.init_buffer(rows,cols,cnls);
        img.init_spatial(1.0, 1.0, 0.5, vec3<double>(0.0,0.0,0.0), vec3<double>(0.0,0.0,0.0));
        img.init_orientation(vec3<double>(0.0,1.0,0.0), vec3<double>(1.0,0.0,0.0));
        img.fill_pixels( std::numeric_limits<uint32_t>::max()/2 );

        contour_of_points<double> cont;
        cont.points.emplace_back(  100.0,  100.0, 1.0 );
        cont.points.emplace_back( 1900.0,  100.0, 1.0 );
        cont.points.emplace_back( 1900.0,  900.0, 1.0 );
        cont.points.emplace_back( 1000.0,  500.0, 1.0 );
        cont.points.emplace_back(  100.0,  900.0, 1.0 );
        cont.closed = true;
        //const auto ls_plane = cont.Least_Squares_Best_Fit_Plane(z_norm);
        const auto ls_plane = cont.Least_Squares_Best_Fit_Plane(yz_norm);

        for(int64_t row = 0; row < rows; ++row){
            for(int64_t col = 0; col < cols; ++col){
                for(int64_t chn = 0; chn < cnls; ++chn){
                    const auto pos = img.position(row,col);
                    if(cont.Is_Point_In_Polygon_Projected_Orthogonally(ls_plane,pos)){
                        img.reference(row,col,chn) = (uint32_t)(1000*row*col);
                    }
                }
            }
        }

        Plot_Pixels(img,0);

        const bool AutoScalePixels = false;
        if(!Dump_Casted_Scaled_Pixels<uint32_t,double,uint16_t>(img, "/tmp/outline.gray", YgorImageIOPixelScaling::TypeMinMax)){
             YLOGERR("Couldn't dump pixels to file");
        }else{
             YLOGINFO("Image written to '/tmp/outline.gray'. Convert like: \n  "
                      "convert -size 2000x1000 -depth 16 -define quantum:format=unsigned -type grayscale outline.gray -depth 16  out.png\n");
        }
    }


    return 0;
}
