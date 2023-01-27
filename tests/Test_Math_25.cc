
#include <iostream>
#include <cmath>
#include <list>
#include <functional>
#include <tuple>
#include <vector>
#include <sstream>
#include <limits>


#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorMathIOOFF.h"
#include "YgorMathIOOBJ.h"
#include "YgorMathIOXYZ.h"

int main(int, char **){

    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    {
        point_set<double> ps_A;
        ps_A.points.emplace_back( -1.0E-6,  0.0,  0.0 );
        ps_A.points.emplace_back(  1.0E-3,  0.0,  0.0 );
        ps_A.points.emplace_back(  0.0, -1.0,  0.0 );
        ps_A.points.emplace_back(  0.0,  1.0,  0.0 );
        ps_A.points.emplace_back(  0.0,  0.0, -1.0000005 );
        ps_A.points.emplace_back(  0.0,  0.0,  1.0000005 );

        const auto c_A = ps_A.Centroid();

        std::stringstream ss;

        if(!WritePointSetToOFF(ps_A, ss)) YLOGERR("Failed to write OFF to stream");
        YLOGINFO("Generated OFF file:\n" << ss.str() << "\n");

        point_set<double> ps_B;
        if(!ReadPointSetFromOFF(ps_B, ss)) YLOGERR("Failed to read OFF from stream");

        const auto c_B = ps_B.Centroid();

        if( (c_A - c_B).length() >  eps ){
            YLOGERR("Centroid (" << c_B << " ) differed from expected (" << c_A << ") by more than eps (" << eps << ")");
        }else{
            YLOGINFO("Centroid (" << c_B << " ) within expected (" << c_A << ") by less than eps (" << eps << ")");
        }
    }

    {
        point_set<double> ps_A;
        ps_A.points.emplace_back( -1.0E-6,  0.0,  0.0 );
        ps_A.points.emplace_back(  1.0E-3,  0.0,  0.0 );
        ps_A.points.emplace_back(  0.0, -1.0,  0.0 );
        ps_A.points.emplace_back(  0.0,  1.0,  0.0 );
        ps_A.points.emplace_back(  0.0,  0.0, -1.0000005 );
        ps_A.points.emplace_back(  0.0,  0.0,  1.0000005 );

        const auto c_A = ps_A.Centroid();

        std::stringstream ss;

        if(!WritePointSetToOBJ(ps_A, ss)) YLOGERR("Failed to write OBJ to stream");
        YLOGINFO("Generated OBJ file:\n" << ss.str() << "\n");

        point_set<double> ps_B;
        if(!ReadPointSetFromOBJ(ps_B, ss)) YLOGERR("Failed to read OBJ from stream");

        const auto c_B = ps_B.Centroid();

        if( (c_A - c_B).length() >  eps ){
            YLOGERR("Centroid (" << c_B << " ) differed from expected (" << c_A << ") by more than eps (" << eps << ")");
        }else{
            YLOGINFO("Centroid (" << c_B << " ) within expected (" << c_A << ") by less than eps (" << eps << ")");
        }
    }

    {
        point_set<double> ps_A;
        ps_A.points.emplace_back( -1.0E-6,  0.0,  0.0 );
        ps_A.points.emplace_back(  1.0E-3,  0.0,  0.0 );
        ps_A.points.emplace_back(  0.0, -1.0,  0.0 );
        ps_A.points.emplace_back(  0.0,  1.0,  0.0 );
        ps_A.points.emplace_back(  0.0,  0.0, -1.0000005 );
        ps_A.points.emplace_back(  0.0,  0.0,  1.0000005 );

        const auto c_A = ps_A.Centroid();

        std::stringstream ss;

        if(!WritePointSetToXYZ(ps_A, ss)) YLOGERR("Failed to write XYZ to stream");
        YLOGINFO("Generated XYZ file:\n" << ss.str() << "\n");

        point_set<double> ps_B;
        if(!ReadPointSetFromXYZ(ps_B, ss)) YLOGERR("Failed to read XYZ from stream");

        const auto c_B = ps_B.Centroid();

        if( (c_A - c_B).length() >  eps ){
            YLOGERR("Centroid (" << c_B << " ) differed from expected (" << c_A << ") by more than eps (" << eps << ")");
        }else{
            YLOGINFO("Centroid (" << c_B << " ) within expected (" << c_A << ") by less than eps (" << eps << ")");
        }
    }

    return 0;
}
