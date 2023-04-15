//Test_Math16.cc - Tests of planar orientation and grid alignment.

#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <functional>
#include <list>
#include <cstdint>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorStats.h"

struct spacings {
    double min = std::numeric_limits<double>::quiet_NaN();
    double max = std::numeric_limits<double>::quiet_NaN();
    double mean = std::numeric_limits<double>::quiet_NaN();
    double median = std::numeric_limits<double>::quiet_NaN();
};

struct spacings ComputeSpacing(double angle_rad){
    //This routine constructs a plane with the given orientation (from (1,0,0)) and a planar cartesian grid, computes the
    // distance between the plane and grid points, sorts them, and computes some statistics on the distributions. 
    //
    // Note: because the grid is finite, the types of statistics that can be computed are limited. Mean and median are
    //       sketchy without limiting the extent of the grid to something representative of an infinite grid.
    const vec3<double> zero(0.0, 0.0, 0.0);
    plane<double> aplane( vec3<double>( std::cos(angle_rad), std::sin(angle_rad), 0.0 ),
                          zero );

    std::vector<double> distances;
    const double dx = 1.0;
    const double dy = 1.0;
    const int64_t boxradius = 5;
    for(int64_t i = -boxradius; i <= boxradius; ++i){
        for(int64_t j = -boxradius; j <= boxradius; ++j){
            //if( (i==0) && (j==0) ) continue; // Disregard centre point.
            //if( (std::abs(i) == boxradius) && (std::abs(j) == boxradius) ) continue; //Disregard points on the corners.
            const vec3<double> p(dx*i, dy*j, 0.0);
            if( p.distance(zero) > 500.0 ) continue; //Disregard points outside of a large circle (circular region selector).

            const auto dist = std::abs( aplane.Get_Signed_Distance_To_Point(p) );
            if((dist == 0.0) || aplane.Is_Point_Above_Plane(p)) distances.push_back(dist);
        }
    }

    std::sort(distances.begin(), distances.end());
    std::vector<double> adjacent_diffs;

    auto itA = distances.begin();
    auto itB = std::next(itA);
    while(itB != distances.end()){
        adjacent_diffs.push_back( *itB - *itA );
        std::advance(itA,1);
        std::advance(itB,1);
    }
    
    spacings out;
    //std::sort(adjacent_diffs.begin(), adjacent_diffs.end());
    out.min = *std::min_element(adjacent_diffs.begin(), adjacent_diffs.end());
    out.max = *std::max_element(adjacent_diffs.begin(), adjacent_diffs.end());
    out.mean = Stats::Mean(adjacent_diffs);
    out.median = Stats::Median(adjacent_diffs);
    return out;
}


int main(int argc, char **argv){

    const int64_t N = 5000; // ~Number of angles to consider.
    for(int64_t i=0; i <= N; ++i){
        const double t = 0.0 + (M_PI/4.0)*(1.0*i/N);
    //for(double t = 18.4348; t <= 18.4350; t += 0.000001){
        const double theta_rad = t; //*M_PI/180.0;
        const double theta_deg = t*180.0/M_PI;
        const auto s = ComputeSpacing(theta_rad);
        std::cout << std::setprecision(std::numeric_limits<long double>::digits10 + 1)
                  << theta_deg << " " << s.min 
                               << " " << s.max 
                               << " " << s.mean 
                               << " " << s.median 
                               << std::endl;
    }

    return 0;
}

