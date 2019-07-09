//Test_Math19.cc - This is a test file for line-line_segment interactions.

#include <iostream>
#include <cmath>
#include <functional>
#include <list>
#include <random>
#include <string>

#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorAlgorithms.h"


int main(int argc, char **argv){

    const
    line<double> l( vec3<double>(0.0, 0.0, 0.0), 
                    vec3<double>(0.0, 0.0, 1.0) );

    {
        line_segment<double> s( vec3<double>(-1.0, 0.0, 0.0), 
                                vec3<double>( 1.0, 0.0, 0.0) );

        vec3<double> p;
        if(!s.Closest_Point_To_Line(l,p)){
            throw std::logic_error("line-line_segment closest point routine failed.");
        }
        if(!s.Within_Pill_Volume(p, 1E-9)){
            throw std::logic_error("Closest point along line_segment is not coincident with the line_segment.");
        }
        FUNCINFO("Closest point between line and line_segment along the line_segment = " << p);
    }

    {
        line_segment<double> s( vec3<double>(-1.0,-1.0, 0.0), 
                                vec3<double>( 1.0, 1.0, 0.0) );

        vec3<double> p;
        if(!s.Closest_Point_To_Line(l,p)){
            throw std::logic_error("line-line_segment closest point routine failed.");
        }
        if(!s.Within_Pill_Volume(p, 1E-9)){
            throw std::logic_error("Closest point along line_segment is not coincident with the line_segment.");
        }
        FUNCINFO("Closest point between line and line_segment along the line_segment = " << p);
    }

    {
        line_segment<double> s( vec3<double>(-1.0, 0.0, 0.0), 
                                vec3<double>(10.0, 1.0, 0.0) );

        vec3<double> p;
        if(!s.Closest_Point_To_Line(l,p)){
            throw std::logic_error("line-line_segment closest point routine failed.");
        }
        if(!s.Within_Pill_Volume(p, 1E-9)){
            throw std::logic_error("Closest point along line_segment is not coincident with the line_segment.");
        }
        FUNCINFO("Closest point between line and line_segment along the line_segment = " << p);
    }

    {
        line_segment<double> s( vec3<double>(-1.0, 0.0, -1.0), 
                                vec3<double>(10.0, 1.0, 1.0) );

        vec3<double> p;
        if(!s.Closest_Point_To_Line(l,p)){
            throw std::logic_error("line-line_segment closest point routine failed.");
        }
        if(!s.Within_Pill_Volume(p, 1E-9)){
            throw std::logic_error("Closest point along line_segment is not coincident with the line_segment.");
        }
        FUNCINFO("Closest point between line and line_segment along the line_segment = " << p);
    }

    return 0;
}

