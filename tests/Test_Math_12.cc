//Test_Math12.cc - Chebyshev polynomial approximation fast multiplication tests.

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

#include <unistd.h>           //fork().
#include <stdlib.h>           //quick_exit(), EXIT_SUCCESS.
#include <getopt.h>           //Needed for 'getopts' argument parsing.
#include <cstdlib>            //Needed for exit() calls.
#include <utility>            //Needed for std::pair.
#include <algorithm>
#include <experimental/optional>

#include "YgorMisc.h"         //Needed for FUNCINFO, FUNCWARN, FUNCERR macros.
#include "YgorLog.h"
#include "YgorMath.h"         //Needed for vec3 class.
#include "YgorMathPlottingGnuplot.h" //Needed for YgorMathPlottingGnuplot::*.
#include "YgorMathChebyshev.h" //Needed for cheby_approx class.
#include "YgorMathChebyshevFunctions.h"
#include "YgorStats.h"        //Needed for Stats:: namespace.
#include "YgorFilesDirs.h"    //Needed for Does_File_Exist_And_Can_Be_Read(...), etc..
#include "YgorContainers.h"   //Needed for bimap class.
#include "YgorPerformance.h"  //Needed for YgorPerformance_dt_from_last().
#include "YgorAlgorithms.h"   //Needed for For_Each_In_Parallel<..>(...)
#include "YgorArguments.h"    //Needed for ArgumentHandler class.
#include "YgorString.h"       //Needed for GetFirstRegex(...)
#include "YgorImages.h"
#include "YgorImagesIO.h"
#include "YgorImagesPlotting.h"



static
void 
PlotTimeCourses(std::string title,
                         const std::map<std::string, samples_1D<double>> &s1D_time_courses,
                         const std::map<std::string, cheby_approx<double>> &cheby_time_courses,
                         std::string xlabel,
                         std::string ylabel,
                         int64_t cheby_samples){
    // NOTE: This routine is spotty. It doesn't always work, and seems to have a hard time opening a display window when a
    //       large data set is loaded. Files therefore get written for backup access.
    //
    // NOTE: This routine does not persist after the parent terminates. It could be made to by dealing with signalling.
    //       A better approach would be sending data to a dedicated server over the net. Better for headless operations,
    //       better for managing the plots and data, better for archiving, etc..

    auto pid = fork();
    if(pid == 0){ //Child process.

        //Package the data into a shuttle and write the to file.
        std::vector<YgorMathPlottingGnuplot::Shuttle<samples_1D<double>>> shuttle;
        for(auto & tcs : s1D_time_courses){
            const auto lROIname = tcs.first;
            const auto lTimeCourse = tcs.second;
            shuttle.emplace_back(lTimeCourse, lROIname);
            const auto lFileName = Get_Unique_Sequential_Filename("/tmp/samples1D_time_course_",6,".txt");
            lTimeCourse.Write_To_File(lFileName);
            AppendStringToFile("# Time course for ROI '"_s + lROIname + "'.\n", lFileName);
            YLOGINFO("Time course for ROI '" << lROIname << "' written to '" << lFileName << "'");
        }
        for(auto & tcs : cheby_time_courses){
            const auto lROIname = tcs.first;
            const auto lTimeCourse = tcs.second;
            const auto domain = lTimeCourse.Get_Domain();
            const double dx = (domain.second - domain.first)/static_cast<double>(cheby_samples);

            samples_1D<double> lTimeCourseSamples1D;
            const bool inhibitsort = true;
            for(int64_t i = 0; i < cheby_samples; ++i){
                double t = domain.first + dx * static_cast<double>(i);
                lTimeCourseSamples1D.push_back(t, 0.0, lTimeCourse.Sample(t), 0.0, inhibitsort);
            }

            shuttle.emplace_back(lTimeCourseSamples1D, lROIname);
            const auto lFileName = Get_Unique_Sequential_Filename("/tmp/cheby_approx_time_course_",6,".txt");
            lTimeCourseSamples1D.Write_To_File(lFileName);
            AppendStringToFile("# Time course for ROI '"_s + lROIname + "'.\n", lFileName);
            YLOGINFO("Time course for ROI '" << lROIname << "' written to '" << lFileName << "'");
        }

        //Plot the data.
        int64_t max_attempts = 20;
        for(int64_t attempt = 1; attempt <= max_attempts; ++attempt){
            try{
                YgorMathPlottingGnuplot::Plot<double>(shuttle, title, xlabel, ylabel);
                break;
            }catch(const std::exception &e){
                YLOGWARN("Unable to plot time courses: '" << e.what() 
                         << "'. Attempt " << attempt << " of " << max_attempts << " ...");
            }
        }

        quick_exit(EXIT_SUCCESS);
    }
    return;
}

int main(int , char** ){
    //This program compares the results of fast, approximate Chebyshev multiplication.

    
    std::map<std::string, samples_1D<double>> stc;
    std::map<std::string, cheby_approx<double>> ctc;

    const double xmin = -0.05;
    const double xmax = 0.30;

    const cheby_approx<double> curveA = Chebyshev_Basis_Approx_Exp_Analytic1(9,xmin,xmax,-2.0,3.0,4.0);;
    const cheby_approx<double> curveB = Chebyshev_Basis_Approx_Exp_Analytic1(11,xmin,xmax,-2.0,3.0,4.0);;

    //cheby_approx<double> curveC = curveA.Fast_Approx_Multiply( curveB, (size_t)(15) );

    cheby_approx<double> curveD = curveA.Fast_Approx_Multiply( curveA, (size_t)(0) );
    ctc["Default Approx"] = curveD;

    cheby_approx<double> curveE = curveA.Fast_Approx_Multiply( curveA, (size_t)(30) );
    ctc["Approx: 30"] = curveE;

    cheby_approx<double> curveF = curveA.Fast_Approx_Multiply( curveA, (size_t)(15) );
    ctc["Approx: 15"] = curveF;

    cheby_approx<double> curveG = curveA.Fast_Approx_Multiply( curveA, (size_t)(3) );
    ctc["Approx: 3"] = curveG;


    PlotTimeCourses("Comparison of multiplication methods", stc, ctc, "X", "Y", 500);

    return 0;
}
