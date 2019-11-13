//YgorMathIOXYZ.cc - Routines for reading and writing ASCII XYZ point cloud files.
//
#include <iostream>
#include <istream>
#include <ostream>
#include <fstream>
#include <sstream>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorString.h"

#include "YgorMathIOXYZ.h"


// This routine reads an point_set from an XYZ format stream.
template <class T>
bool
ReadPointSetFromXYZ(point_set<T> &ps,
                    std::istream &is ){

    // The following file describes the core format and will be correctly read by this routine:
    //  _________________________________________________________________________
    //  |# This is a comment. It should be ignored.                             |
    //  |# The next line is intentionally blank. It should be ignored too.      |
    //  |                                                                       |
    //  |1.0 1.0 1.0                                                            |
    //  | 2.0 2.0 2.0                                                           |
    //  |3,3,3                                                                  |
    //  |                                                                       |
    //  |4;4 4                                                                  |
    //  |5.0E-4 nan inf                                                         |
    //  |                                                                       |
    //  |6.0,6.0,6.0 # This is also a comment and should be ignored.            |
    //  |_______________________________________________________________________|
    //
    // Only ASCII format is accepted. Multiple separators are accepted, and whitespace is generally not significant
    // (except if used as a separator between numbers). Only lines with 3 scalars are accepted as valid points.
    // Reading metadata encoded into comments is not currently supported.
    //
    // The accepted format is variable, and it is hard to decide whether a given file is definitively in XYZ format. The
    // threshold to decide is whether any single line contains a point that can be successfully read. If this happens,
    // the file is considered to be in XYZ format. Therefore, it is best to attempt loading other, more strucured
    // formats if uncertain about the file type ahead of time.
    //

    if(!is.good()){
        throw std::runtime_error("Unable to read file.");
    }

    ps.points.clear();

    std::string aline;
    while(!is.eof()){
        std::getline(is, aline);
        if(is.eof()) break;

        // Strip away comments.
        const auto com_pos = aline.find_first_of("#");
        if(com_pos != std::string::npos){
            // Handle metadata packed into a comment line here...
            // ... TODO ...

            // Strip away the comment, passing through anything before the comment tokens.
            aline = aline.substr(0,com_pos);
        }

        // Handle empty lines.
        aline = Canonicalize_String2(aline, CANONICALIZE::TRIM);
        if(aline.empty()) continue;

        // Split the line assuming the the separator could be anything common.
        std::vector<std::string> xyz = { aline };
        xyz = SplitVector(xyz, ' ', 'd');
        xyz = SplitVector(xyz, ',', 'd');
        xyz = SplitVector(xyz, ';', 'd');
        xyz = SplitVector(xyz, '\t', 'd');

        std::vector<T> shtl;
        for(const auto &t : xyz){
            if(shtl.size() > 3) break; // Terminate early if the line is already invalid.
            try{
                const auto num = std::stod(t);
                shtl.emplace_back(num);
            }catch(const std::exception &e){ }
        }

        if(shtl.empty()){
            continue; // Line contained no numbers -- was probably all whitespace so ignore.
        }else if(shtl.size() == 3){
            ps.points.emplace_back( shtl.at(0), shtl.at(1), shtl.at(2) );
        }else{
            FUNCWARN("Encountered line with " << shtl.size() << " numerical coordinates. Refusing to continue");
            return false;
        }
    }

    // Reject the file if no points were successfully read from it.
    if(ps.points.empty()){
        return false;
    }

    FUNCINFO("Loaded XYZ file with " << ps.points.size() << " points");
    return true;
}
#ifndef YGORMATHIOXYZ_DISABLE_ALL_SPECIALIZATIONS
    template bool ReadPointSetFromXYZ(point_set<float > &, std::istream &);
    template bool ReadPointSetFromXYZ(point_set<double> &, std::istream &);
#endif


// This routine writes a point_set to an XYZ format stream.
//
// Note that metadata is currently not written.
template <class T>
bool
WritePointSetToXYZ(const point_set<T> &ps,
                   std::ostream &os ){

    os << "# XYZ point cloud file" << std::endl;

    // Maximize precision prior to emitting the vertices.
    const auto original_precision = os.precision();
    os.precision( std::numeric_limits<T>::digits10 + 1 );
    for(const auto &p : ps.points){
        os << p.x << " "
           << p.y << " "
           << p.z << '\n';
    }
    // Reset the precision on the stream.
    os.precision( original_precision );
    os.flush();

    return(!os.fail());
}
#ifndef YGORMATHIOXYZ_DISABLE_ALL_SPECIALIZATIONS
    template bool WritePointSetToXYZ(const point_set<float > &, std::ostream &);
    template bool WritePointSetToXYZ(const point_set<double> &, std::ostream &);
#endif

