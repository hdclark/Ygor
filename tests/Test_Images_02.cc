//Test_Images2.cc.

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
#include "YgorImagesIOSerialization.h"


int main(int , char** ){
    //This program serializes and then deserializes instances of classes defined in YgorImages.h.
    namespace ys = ygor::serialization;

    {
        planar_image<float,double> A;
        A.init_buffer(20,40,5);
        A.init_spatial(0.5,1.5,2.5, vec3<double>(0.0,0.0,0.0), vec3<double>(5.0,5.0,5.0));
        A.init_orientation(vec3<double>(1.0,0.0,0.0), vec3<double>(0.0,1.0,0.0));
        A.fill_pixels(1.23f);

        std::ofstream ofs("/tmp/serial_planar_image", std::ios::trunc);
        ys::xml_oarchive ar(ofs);
        ar & ys::make_nvp("planar_imagefloatdouble", A);
    }
    {
        planar_image<float,double> A;
        std::ifstream ifs("/tmp/serial_planar_image");
        try{
            ys::xml_iarchive ar(ifs);
            ar & ys::make_nvp("not used", A);
            YLOGINFO("Deserialized planar_image to " << A.rows << "x" << A.columns << " image with first pixel = " << A.value(0,0,0));
        }catch(const std::exception &){
            YLOGINFO("Unable to deserialize planar_image file. It is not valid");
        }

    }



    {
        planar_image_collection<float,double> A;

        planar_image<float,double> B;
        B.init_buffer(20,40,5);
        B.init_spatial(0.5,1.5,2.5, vec3<double>(0.0,0.0,0.0), vec3<double>(5.0,5.0,5.0));
        B.init_orientation(vec3<double>(1.0,0.0,0.0), vec3<double>(0.0,1.0,0.0));
        B.fill_pixels(1.23f);

        planar_image<float,double> C = B;

        A.images.push_back(B);
        A.images.push_back(C);

        std::ofstream ofs("/tmp/serial_planar_image_collection", std::ios::trunc);
        ys::xml_oarchive ar(ofs);
        ar & ys::make_nvp("planar_image_collectionfloatdouble", A);
    }
    {
        planar_image_collection<float,double> A;
        std::ifstream ifs("/tmp/serial_planar_image_collection");
        try{
            ys::xml_iarchive ar(ifs);
            ar & ys::make_nvp("not used", A);
            YLOGINFO("Deserialized planar_image_collection to " << A.images.size() << " images, the first of which"
                   << " has " << A.images.front().rows << "x" << A.images.front().columns << " RxC with first pixel = "
                   << A.images.front().value(0,0,0));
        }catch(const std::exception &){
            YLOGINFO("Unable to deserialize planar_image_collection file. It is not valid");
        }

    }


    return 0;
}
