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
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMathIOOFF.h"
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
            YLOGINFO("Deserialized vec3 to " << A);
        }catch(const std::exception &){
            YLOGINFO("Unable to deserialize vec3 file. It is not valid");
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
            YLOGINFO("Deserialized line to " << A.R_0 << "  " << A.U_0);
        }catch(const std::exception &){
            YLOGINFO("Unable to deserialize line file. It is not valid");
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
            YLOGINFO("Deserialized line_segment to " << A.R_0 << "  " << A.U_0 << "   " << A.t_0 << "   " << A.t_1);
        }catch(const std::exception &){
            YLOGINFO("Unable to deserialize line_segment file. It is not valid");
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
            YLOGINFO("Deserialized plane to " << A.N_0 << "  " << A.R_0);
        }catch(const std::exception &){
            YLOGINFO("Unable to deserialize plane file. It is not valid");
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
            YLOGINFO("Deserialized contour_of_points to " << A.points.size() << " points and " << A.metadata.size() << " metadata");
        }catch(const std::exception &){
            YLOGINFO("Unable to deserialize contour_of_points file. It is not valid");
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
            YLOGINFO("Deserialized samples_1D to " << A.samples.size() << " samples and " << A.metadata.size() << " metadata");
        }catch(const std::exception &){
            YLOGINFO("Unable to deserialize samples_1D file. It is not valid");
        }

    }



    {
        fv_surface_mesh<double, uint64_t> smesh;

        std::stringstream ss;
        ss << "OFF" << std::endl
           << "# Whole-line comment" << std::endl
           << "# A # Confusing # comment" << std::endl
           << "100 100 #0 # An invalid line that should be disregarded" << std::endl
           << "12 20 0" << std::endl
           << "0 1.618034 1 " << std::endl  // The vertices.
           << "0 1.618034 -1 " << std::endl
           << "0 -1.618034 1 " << std::endl
           << "0 -1.618034 -1 " << std::endl
           << "1.618034 1 0 " << std::endl
           << "1.618034 -1 0 " << std::endl
           << "-1.618034 1 0 " << std::endl
           << "-1.618034 -1 0 " << std::endl
           << "1 0 1.618034 " << std::endl
           << "-1 0 1.618034 " << std::endl
           << "1 0 -1.618034 " << std::endl
           << "-1 0 -1.618034 " << std::endl
           << "3 1 0 4" << std::endl          // The faces.
           << "3 0 1 6" << std::endl
           << "3 2 3 5" << std::endl
           << "3 3 2 7" << std::endl
           << "3 4 5 10" << std::endl
           << "3 5 4 8" << std::endl
           << "3 6 7 9" << std::endl
           << "3 7 6 11" << std::endl
           << "3 8 9 2" << std::endl
           << "3 9 8 0" << std::endl
           << "3 10 11 1" << std::endl
           << "3 11 10 3" << std::endl
           << "3 0 8 4" << std::endl
           << "3 0 6 9" << std::endl
           << "3 1 4 10" << std::endl
           << "3 1 11 6" << std::endl
           << "3 2 5 8" << std::endl
           << "3 2 9 7" << std::endl
           << "3 3 10 5 # Another comment." << std::endl
           << "3 3 7 11" << std::endl
           << "" << std::endl;

        if(!ReadFVSMeshFromOFF(smesh, ss)){
            throw std::runtime_error("Unable to read mesh from OFF stream.");
        }
        YLOGINFO("Mesh surface area: " << smesh.surface_area());
        smesh.metadata["keyA"] = "valueA";
        smesh.metadata["keyB"] = "valueB";

        std::ofstream ofs("/tmp/serial_fv_surface_mesh", std::ios::trunc);
        boost::archive::xml_oarchive ar(ofs);
        ar & boost::serialization::make_nvp("fv_surface_mesh", smesh);
    }
    {
        fv_surface_mesh<double, uint64_t> smesh;
        std::ifstream ifs("/tmp/serial_fv_surface_mesh");
        try{
            boost::archive::xml_iarchive ar(ifs);
            ar & boost::serialization::make_nvp("not used", smesh);
            YLOGINFO("Deserialized fv_surface_mesh has surface area " << smesh.surface_area());
        }catch(const std::exception &){
            YLOGINFO("Unable to deserialize fv_surface_mesh file. It is not valid");
        }

    }

    return 0;
}
