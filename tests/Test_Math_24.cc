
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

int main(int, char **){

    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    {
        point_set<double> ps;
        ps.points.emplace_back( -1.0,  0.0,  0.0 );
        ps.points.emplace_back(  1.0,  0.0,  0.0 );

        ps.points.emplace_back(  0.0, -1.0,  0.0 );
        ps.points.emplace_back(  0.0,  1.0,  0.0 );

        ps.points.emplace_back(  0.0,  0.0, -1.0 );
        ps.points.emplace_back(  0.0,  0.0,  1.0 );

        const auto c = ps.Centroid();
        const auto e = vec3<double>(0.0,0.0,0.0);
        if( (c - e).length() >  eps ){
            YLOGERR("Centroid (" << c << " ) differed from expected (" << e << ") by more than eps (" << eps << ")");
        }else{
            YLOGINFO("Centroid (" << c << " ) within expected (" << e << ") by less than eps (" << eps << ")");
        }
    }

    {
        point_set<double> ps;
        ps.points.emplace_back( -2.0,  0.0,  0.0 );
        ps.points.emplace_back(  0.0,  0.0,  0.0 );

        ps.points.emplace_back( -1.0, -1.0,  0.0 );
        ps.points.emplace_back( -1.0,  1.0,  0.0 );

        ps.points.emplace_back( -1.0,  0.0, -1.0 );
        ps.points.emplace_back( -1.0,  0.0,  1.0 );

        const auto c = ps.Centroid();
        const auto e = vec3<double>(-1.0,0.0,0.0);
        if( (c - e).length() >  eps ){
            YLOGERR("Centroid (" << c << " ) differed from expected (" << e << ") by more than eps (" << eps << ")");
        }else{
            YLOGINFO("Centroid (" << c << " ) within expected (" << e << ") by less than eps (" << eps << ")");
        }
    }

    {
        point_set<double> ps;
        ps.points.emplace_back(  4.0, -5.0,  3.0 );
        ps.points.emplace_back(  6.0, -5.0,  3.0 );

        ps.points.emplace_back(  5.0, -6.0,  3.0 );
        ps.points.emplace_back(  5.0, -4.0,  3.0 );

        ps.points.emplace_back(  5.0, -5.0,  2.0 );
        ps.points.emplace_back(  5.0, -5.0,  4.0 );

        const auto c = ps.Centroid();
        const auto e = vec3<double>(5.0,-5.0,3.0);
        if( (c - e).length() >  eps ){
            YLOGERR("Centroid (" << c << " ) differed from expected (" << e << ") by more than eps (" << eps << ")");
        }else{
            YLOGINFO("Centroid (" << c << " ) within expected (" << e << ") by less than eps (" << eps << ")");
        }
    }


    return 0;
}
