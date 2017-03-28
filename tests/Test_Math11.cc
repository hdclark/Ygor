//Test_Math11.cc.

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

#include <boost/serialization/nvp.hpp>

//For plain-text archives.
//#include <boost/archive/text_iarchive.hpp>
//#include <boost/archive/text_oarchive.hpp>

//For XML archives.
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>

#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorMathIOBoostSerialization.h"


int main(int , char** ){
    //This program serializes and then deserializes instances of classes defined in YgorMath.h.

    {
        vec3<double> A(1.0, 2.0, 3.0);
        std::ofstream ofs("/tmp/serial_vec3", std::ios::trunc);
        boost::archive::xml_oarchive ar(ofs);
        ar & boost::serialization::make_nvp("vec3double", A);
    }
    {
        vec3<double> A(100.0, 200.0, 300.0);
        std::ifstream ifs("/tmp/serial_vec3");
        try{
            boost::archive::xml_iarchive ar(ifs);
            ar & boost::serialization::make_nvp("not used", A);
            FUNCINFO("Deserialized vec3 to " << A);
        }catch(const std::exception &){
            FUNCINFO("Unable to deserialize vec3 file. It is not valid");
        }
    }




    {
        line<double> A;
        std::ofstream ofs("/tmp/serial_line", std::ios::trunc);
        boost::archive::xml_oarchive ar(ofs);
        ar & boost::serialization::make_nvp("linedouble", A);
    }
    {
        line<double> A;
        std::ifstream ifs("/tmp/serial_line");
        try{
            boost::archive::xml_iarchive ar(ifs);
            ar & boost::serialization::make_nvp("not used", A);
            FUNCINFO("Deserialized line to " << A.R_0 << "  " << A.U_0);
        }catch(const std::exception &){
            FUNCINFO("Unable to deserialize line file. It is not valid");
        }

    }




    {
        line_segment<double> A;
        A.t_0 = 2.3;
        A.t_1 = 3.2;
        std::ofstream ofs("/tmp/serial_line_segment", std::ios::trunc);
        boost::archive::xml_oarchive ar(ofs);
        ar & boost::serialization::make_nvp("line_segmentdouble", A);
    }
    {
        line_segment<double> A;
        std::ifstream ifs("/tmp/serial_line_segment");
        try{
            boost::archive::xml_iarchive ar(ifs);
            ar & boost::serialization::make_nvp("not used", A);
            FUNCINFO("Deserialized line_segment to " << A.R_0 << "  " << A.U_0 << "   " << A.t_0 << "   " << A.t_1);
        }catch(const std::exception &){
            FUNCINFO("Unable to deserialize line_segment file. It is not valid");
        }

    }





    {
        plane<double> A;
        std::ofstream ofs("/tmp/serial_plane", std::ios::trunc);
        boost::archive::xml_oarchive ar(ofs);
        ar & boost::serialization::make_nvp("planedouble", A);
    }
    {
        plane<double> A;
        std::ifstream ifs("/tmp/serial_plane");
        try{
            boost::archive::xml_iarchive ar(ifs);
            ar & boost::serialization::make_nvp("not used", A);
            FUNCINFO("Deserialized plane to " << A.N_0 << "  " << A.R_0);
        }catch(const std::exception &){
            FUNCINFO("Unable to deserialize plane file. It is not valid");
        }

    }




    {
        contour_of_points<double> A;
        A.points.push_back( vec3<double>(1.0,2.0,3.0) );
        A.points.push_back( vec3<double>(4.0,5.0,6.0) );
        A.points.push_back( vec3<double>(7.0,8.0,9.0) );
        A.closed = true;
        A.metadata["keyA"] = "valueA";
        A.metadata["keyB"] = "valueB";

        std::ofstream ofs("/tmp/serial_contour_of_points", std::ios::trunc);
        boost::archive::xml_oarchive ar(ofs);
        ar & boost::serialization::make_nvp("contour_of_pointsdouble", A);
    }
    {
        contour_of_points<double> A;
        std::ifstream ifs("/tmp/serial_contour_of_points");
        try{
            boost::archive::xml_iarchive ar(ifs);
            ar & boost::serialization::make_nvp("not used", A);
            FUNCINFO("Deserialized contour_of_points to " << A.points.size() << " points and " << A.metadata.size() << " metadata");
        }catch(const std::exception &){
            FUNCINFO("Unable to deserialize contour_of_points file. It is not valid");
        }

    }





    {
        samples_1D<double> A;
        A.push_back(0.0,0.0,1.0,1.0);
        A.push_back(1.0,0.0,2.0,1.0);
        A.push_back(2.0,0.0,3.0,1.0);
        A.metadata["keyA"] = "valueA";
        A.metadata["keyB"] = "valueB";
        std::ofstream ofs("/tmp/serial_samples_1D", std::ios::trunc);
        boost::archive::xml_oarchive ar(ofs);
        ar & boost::serialization::make_nvp("samples_1Ddouble", A);
    }
    {
        samples_1D<double> A;
        std::ifstream ifs("/tmp/serial_samples_1D");
        try{
            boost::archive::xml_iarchive ar(ifs);
            ar & boost::serialization::make_nvp("not used", A);
            FUNCINFO("Deserialized samples_1D to " << A.samples.size() << " samples and " << A.metadata.size() << " metadata");
        }catch(const std::exception &){
            FUNCINFO("Unable to deserialize samples_1D file. It is not valid");
        }

    }

    return 0;
}
