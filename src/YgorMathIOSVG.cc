//YgorMathIOSVG.cc - Routines for reading and writing SVG planar contours.

#include <iostream>
#include <istream>
#include <ostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdint>

#include "YgorDefinitions.h"
#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorString.h"
#include "YgorBase64.h"   //Used for metadata serialization.

#include "YgorMathIOSVG.h"


// This routine writes a planar contours and metadata to an SVG format stream.
template <class T>
bool
WriteCCToSVG(const contour_collection<T> &cc,
             const plane<T> &P,
             std::ostream &os,
             T extra_space,     // The nearest separation between the contour and the edge of the viewbox.
             T font_size,       // The size of the text's font in the same units as the contours (i.e., mm).
             std::string perimeter_text){

    // Ensure the contour is planar by projecting it onto a plane derived from the contour itself.
    //const auto N = cop.Estimate_Planar_Normal();
    //const auto P = cop.Least_Squares_Best_Fit_Plane(N);
    const auto N = P.N_0;
    const auto P_zero = P.Project_Onto_Plane_Orthogonally(vec3<T>(0.0, 0.0, 0.0));

    // Used to determine when text must be base64 encoded.
    const auto needs_to_be_escaped = [](const std::string &in) -> bool {
        for(const auto &x : in){
            // Permit words/sentences but not characters that could potentially affect file interpretation.
            if( !std::isprint(x) 
                || (x == static_cast<unsigned char>('\''))
                || (x == static_cast<unsigned char>('<'))
                || (x == static_cast<unsigned char>('>'))
                || (x == static_cast<unsigned char>('&')) ) return true;
                //|| (x == static_cast<unsigned char>('"'))
                //|| (x == static_cast<unsigned char>('\'')) ) return true;
        }
        return false;
    };

    // Create a reproducible 2D coordinate system.
    const T pi = static_cast<T>(3.14159265358979323846264338328);
    vec3<T> P_X = N.rotate_around_z(pi * static_cast<T>(0.5)); // First try Z. Will often be idempotent.
    if(P_X.Dot(N) > static_cast<T>(0.25)){
        P_X = N.rotate_around_y(pi * static_cast<T>(0.5));  // Should always work since we now know N is ~parallel to Z.
    }
    vec3<T> P_Y = N.Cross(P_X);
    if(!N.GramSchmidt_orthogonalize(P_X, P_Y)){
        throw std::runtime_error("Unable to find 2D planar coordinate vectors.");
    }
    P_X = P_X.unit();
    P_Y = P_Y.unit();

    // Determine bounds for the contour.
    auto x_min = std::numeric_limits<double>::infinity();
    auto x_max = -(std::numeric_limits<double>::infinity());
    auto y_min = std::numeric_limits<double>::infinity();
    auto y_max = -(std::numeric_limits<double>::infinity());

    for(const auto &cop : cc.contours){
        const auto proj = cop.Project_Onto_Plane_Orthogonally(P);
        for(const auto &p : proj.points){

            const auto x_proj = P_X.Dot( p - P_zero );
            const auto y_proj = P_Y.Dot( p - P_zero );
            if(!std::isfinite(x_min) || ((x_proj - extra_space) < x_min)) x_min = (x_proj - extra_space);
            if(!std::isfinite(x_max) || ((x_proj + extra_space) > x_max)) x_max = (x_proj + extra_space);
            if(!std::isfinite(y_min) || ((y_proj - extra_space) < y_min)) y_min = (y_proj - extra_space);
            if(!std::isfinite(y_max) || ((y_proj + extra_space) > y_max)) y_max = (y_proj + extra_space);
        }
    }

    // Maximize precision prior to emitting the vertices.
    const auto original_precision = os.precision();
    os.precision( std::numeric_limits<T>::max_digits10 );

    // Emit the SVG header.
    os << R"***(<?xml version="1.0" standalone="no"?>)***" << std::endl;
    os << R"***(<!-- The dimensions of the contours are in mm, but have been projected onto a plane. -->)***" << std::endl;
    os << R"***(<!-- Beware that projection can result in skewed contour dimensions. -->)***" << std::endl;

    os << R"***(<svg)***"
       << R"***( width=")***" << (x_max - x_min) << R"***(mm")***" 
       << R"***( height=")***" << (y_max - y_min) << R"***(mm")***" 
       << R"***( viewBox=")***" << x_min << " " << -y_max << " " << std::abs(x_max - x_min) << " " << std::abs(y_max - y_min) << R"***(")***" 
       << R"***( xmlns="http://www.w3.org/2000/svg")***" 
       << R"***( version="1.1">)***" << std::endl;

    os << R"***(<desc>Projected contours.</desc>)***" << std::endl;

    // Emit metadata.
    {
        auto cm = cc.get_common_metadata({}, {});

        for(const auto &mp : cm){
            const auto key = mp.first;
            const auto value = mp.second;
            const bool must_encode = needs_to_be_escaped(key) || needs_to_be_escaped(value);
            if(must_encode){
                const auto encoded_key   = Base64::EncodeFromString(key);
                const auto encoded_value = Base64::EncodeFromString(value);
                os << "<!-- base64 metadata: '" << encoded_key << "' = '" << encoded_value << "' -->" << std::endl;
            }else{
                // If encoding is not needed then don't. It will make the data more accessible.
                os << "<!-- metadata: '" << key << "' = '" << value << "' -->" << std::endl;
            }
        }
    }

    // Emit a non-displayed path that we can later attach text to.
    if(!perimeter_text.empty()){
        os << R"***(<defs>)***" << std::endl;
        int64_t contour_number = 0;
        for(const auto &cop : cc.contours){
            const auto proj = cop.Project_Onto_Plane_Orthogonally(P);
            contour_of_points<T> reoriented;
            reoriented.closed = true;
            for(const auto &p : proj.points){
                const auto x_proj = P_X.Dot( p - P_zero );
                const auto y_proj = P_Y.Dot( p - P_zero );
                reoriented.points.emplace_back(x_proj, -y_proj, static_cast<T>(0));
            }
            reoriented.Reorient_Counter_Clockwise();

            os << R"***(<path id="contour)***" << contour_number++ << R"***(" fill="none" stroke="black")***";
            os << R"***( d=")***";

            bool first = true;
            for(const auto &p : reoriented.points){
                if(first){
                    first = false;
                    os << "M "; // Move to the first vertex without creating a line.
                }else{
                    os << " L "; // Line to the next vertex.
                }
                os << p.x << "," << p.y;
            }
            os << R"***( Z " />)***" << std::endl; // Close the polygon.
        }
        os << R"***(</defs>)***" << std::endl;
    }

    // Emit contours.
    for(const auto &cop : cc.contours){
        const auto proj = cop.Project_Onto_Plane_Orthogonally(P);
        os << R"***(<polygon points=")***";
        bool first = true;
        for(const auto &p : proj.points){
            const auto x_proj = P_X.Dot( p - P_zero );
            const auto y_proj = P_Y.Dot( p - P_zero );
            if(first){
                first = false;
            }else{
                os << " ";
            }
            os << x_proj << "," << -y_proj;
        }
        os << R"***(")***";
        //os << R"***( style="fill: silver; stroke: black; stroke-width: 0.1mm; fill-rule: nonzero;" />)***" << std::endl;
        os << R"***( style="fill: silver; fill-opacity: 75%; stroke: black; stroke-width: 0.1mm; " />)***" << std::endl;
    }

    // Place text along the paths.
    if(!perimeter_text.empty()){
        os << R"***(<text>)***" << std::endl;
        int64_t contour_number = 0;
        for(const auto &cop : cc.contours){
            os << R"***(<textPath href="#contour)***" << contour_number++ << R"***(")***" << std::endl;
            os << R"***( style="font-size: )***" << font_size << R"***(mm; ">)***" << std::endl;
            const auto expanded_perimeter_text = ExpandMacros(perimeter_text, cop.metadata, "$");
            if(needs_to_be_escaped(expanded_perimeter_text)){
                os << "base64: " << Base64::EncodeFromString(expanded_perimeter_text) << std::endl;
            }else{
                os << expanded_perimeter_text << std::endl;
            }
            os << R"***(</textPath>)***" << std::endl;
        }
        os << R"***(</text>)***" << std::endl;
    }

    os << R"***(</svg>)***" << std::endl;

    // Reset the precision on the stream.
    os.precision( original_precision );
    os.flush();

    return(!os.bad());
}
#ifndef YGORMATHIOSVG_DISABLE_ALL_SPECIALIZATIONS
    template bool WriteCCToSVG(const contour_collection<float > &, const plane<float > &, std::ostream &,
                               float , float , std::string);
    template bool WriteCCToSVG(const contour_collection<double> &, const plane<double> &, std::ostream &,
                               double, double, std::string);
#endif


