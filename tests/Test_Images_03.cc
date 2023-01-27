//Test_Images_03.cc.

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <string>    
#include <vector>
#include <set> 
#include <map>
#include <unordered_map>
#include <list>
#include <functional>
#include <thread>
#include <array>
#include <mutex>
#include <limits>
#include <cmath>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorImages.h"


int main(int , char** ){
    //This program tests the planar_image_adjacency class.

    {
        const vec3<double> zero3(0.0,0.0,0.0);
        const vec3<double> row_unit(1.0,0.0,0.0);
        const vec3<double> col_unit(0.0,1.0,0.0);
        const vec3<double> ort_unit(0.0,0.0,1.0);

        planar_image<float,double> A;
        A.init_buffer(20,40,1);
        A.init_spatial(0.5,1.5,2.5, zero3, vec3<double>(5.0,5.0,5.0));
        A.init_orientation(row_unit, col_unit);
        A.fill_pixels(1.0f);

        planar_image<float,double> B;
        B.init_buffer(20,40,1);
        B.init_spatial(0.5,1.5,2.5, zero3, vec3<double>(5.0,5.0,7.5));
        B.init_orientation(vec3<double>(1.0,0.0,0.0), vec3<double>(0.0,1.0,0.0));
        B.init_orientation(row_unit, col_unit);
        B.fill_pixels(2.0f);

        planar_image<float,double> C;
        C.init_buffer(20,40,1);
        C.init_spatial(0.5,1.5,2.5, zero3, vec3<double>(5.0,5.0,10.0));
        C.init_orientation(row_unit, col_unit);
        C.fill_pixels(3.0f);

        //planar_image_collection<float,double> D;
        //D.images.push_back(A);
        //D.images.push_back(B);
        //D.images.push_back(C);

        std::list<std::reference_wrapper<planar_image<float,double>>> img_refws;
        img_refws.push_back( std::ref(A) );
        img_refws.push_back( std::ref(B) );
        img_refws.push_back( std::ref(C) );

        if( !Images_Form_Rectilinear_Grid<float,double>(img_refws) ){
            throw std::runtime_error("Images do not form a rectilinear grid. Cannot continue.");
        }

        // Pre-compute adjacency information.
        planar_image_adjacency<float,double> adj( img_refws, {}, ort_unit );

        // Query for adjacency imformation. We will aim to select each of the three images once.
        const auto index_b = adj.image_to_index( std::ref(B) );
        auto img_refw_0    = adj.index_to_image( index_b );
        auto img_refw_up   = adj.index_to_image( index_b + 1 );
        auto img_refw_down = adj.index_to_image( index_b - 1 );

        const auto val_0    = img_refw_0.get().value(0,0,0);
        const auto val_up   = img_refw_up.get().value(0,0,0);
        const auto val_down = img_refw_down.get().value(0,0,0);

        if( (val_0 == val_up) || (val_0 == val_down) || (val_up == val_down) ){
            throw std::runtime_error("Adjacency calculation failed.");
        }
        YLOGINFO("Adjacency OK");


        // Interpolate between image slices.
        const auto interp_A = adj.trilinearly_interpolate( vec3<double>(6.0, 6.0, 6.25), 0);
        const auto interp_B = adj.trilinearly_interpolate( vec3<double>(6.0, 6.0, 5.0), 0);
        const auto interp_C = adj.trilinearly_interpolate( vec3<double>(6.0, 6.0, 8.75), 0);

        const auto eps = std::sqrt( 10.0 * std::numeric_limits<double>::epsilon() );

        if( eps < std::abs(interp_A - 1.5f)){
            YLOGERR("Trilinear interpolation failed for point A (" << interp_A << ")");
        }
        if( eps < std::abs(interp_B - 1.0f)){
            YLOGERR("Trilinear interpolation failed for point B (" << interp_B << ")");
        }
        if( eps < std::abs(interp_C - 2.5f)){
            YLOGERR("Trilinear interpolation failed for point C (" << interp_C << ")");
        }


    }

    return 0;
}
