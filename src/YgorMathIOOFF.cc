//YgorMathIOOFF.cc - Routines for writing ASCII OFF ("Object File Format") files.
//
#include <fstream>
#include <vector>

#include "YgorMath.h"
#include "YgorMathIOOFF.h"


// This routine writes a collection of points (or vertices) without any connectivity between them.
//
// NOTE: comments should not have newlines. This may be enforced in the future.
//
template <class T>
bool
WritePointsToOFF(std::vector<vec3<T>> points,
                 const std::string &filename,
                 const std::string &comment){
    
    //Check if the file exists. If it does, we will refuse to overwrite it.
    {
        std::ifstream FI(filename);
        if(FI.good()) return false;
    }

    std::ofstream FO(filename, std::ios::out);
    if(!FO) return false;

    FO << "OFF" << std::endl;
    FO << std::endl;
    FO << "#" << comment << std::endl;
    FO << std::endl;

    FO << points.size() << " 0 0" << std::endl; //Number of vertices, faces, edges.  

    //Vertices.
    for(const auto &p : points){
        FO << p.x << " " << p.y << " " << p.z << std::endl;
    }
    FO << std::endl;

    //Check if it was successful, clean up, and carry on.
    if(!FO.good()) return false;
    FO.close();
    return true;
}
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool WritePointsToOFF(std::vector<vec3<float >>, const std::string &, const std::string &);
    template bool WritePointsToOFF(std::vector<vec3<double>>, const std::string &, const std::string &);
#endif


// This routine writes a line_segment in OFF format.
//
// NOTE: This routine writes a line_segment as a triangle with zero area rather than a single edge connecting 
//       two vertices for compatibility purposes.
//
// NOTE: Comments should not have newlines. This may be enforced in the future.
//
template <class T>
bool
WriteLineSegmentToOFF(line_segment<T> ls,
                      const std::string &filename,
                      const std::string &comment){

    //Check if the file exists. If it does, we will refuse to overwrite it.
    {
        std::ifstream FI(filename);
        if(FI.good()) return false;
    }

    std::ofstream FO(filename, std::ios::out);
    if(!FO) return false;

    FO << "OFF" << std::endl;
    FO << std::endl;
    FO << "# Note: this file contains a zero-area triangle that represents a line segment." << std::endl;
    FO << "#" << comment << std::endl;
    FO << std::endl;

    FO << "3 1 0" << std::endl; //Number of vertices, faces, edges (edges are ignorable).

    //Vertices.
    const auto R0 = ls.Get_R0();
    const auto R1 = ls.Get_R1();
    const auto R2 = (R0 + R1)*static_cast<T>(0.5); //The midpoint, to complete the triangle.
    FO << R0.x << " " << R0.y << " " << R0.z << std::endl; // "v0".
    FO << R1.x << " " << R1.y << " " << R1.z << std::endl; // "v1".
    FO << R2.x << " " << R2.y << " " << R2.z << std::endl; // "v2".
    FO << std::endl;

    //Faces. (Only a single zero-area face in this case.)
    FO << "3 0 1 2" << std::endl; //Triangle with three vertices: (v0,v1,v2).
    FO << std::endl;

    //Check if it was successful, clean up, and carry on.
    if(!FO.good()) return false;
    FO.close();
    return true;
}
#ifndef YGOR_IMAGES_IO_INCLUDE_ALL_SPECIALIZATIONS
    template bool WriteLineSegmentToOFF(line_segment<float >, const std::string &, const std::string &);
    template bool WriteLineSegmentToOFF(line_segment<double>, const std::string &, const std::string &);
#endif
