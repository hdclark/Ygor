
#include <iostream>
#include <cmath>
#include <list>
#include <functional>
#include <tuple>
#include <vector>
#include <random>
#include <sstream>
#include <limits>
#include <chrono>
#include <thread>


#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorMathPlotting.h"
#include "YgorStats.h"
#include "YgorPlot.h"
#include "YgorFilesDirs.h"
#include "YgorString.h"

int main(int, char **){

    //Fill a buffer with some samples.  
    const bool InhibitSort = true;
    samples_1D<double> buffa, buffb, buffc, buffd, buffe;
    buffa.uncertainties_known_to_be_independent_and_random = true;
    buffb.uncertainties_known_to_be_independent_and_random = true;
    buffc.uncertainties_known_to_be_independent_and_random = true;
    buffd.uncertainties_known_to_be_independent_and_random = true;
    buffe.uncertainties_known_to_be_independent_and_random = true;
  
    for(double x = 0.0; x < 10.0; x += 0.15){
        const double f = std::sin(x) * std::exp(-x/2.0);
        const double df = f * 0.05;
        const double dx = x * 0.005;
        buffa.push_back(x, dx, f, df, InhibitSort);
    }
    for(double x = 0.0; x < 10.0; x += 0.2){
        const double f = std::sin(x) * std::exp(-x/2.0);
        const double df = f * 0.05;
        const double dx = x * 0.005;
        buffb.push_back(x, dx, f, df, InhibitSort);
    }
    for(double x = 0.0; x < 10.0; x += 0.4){
        const double f = std::sin(x) * std::exp(-x/2.0);
        const double df = f * 0.05;
        const double dx = x * 0.005;
        buffc.push_back(x, dx, f, df, InhibitSort);
    }
    for(double x = 0.0; x < 10.0; x += 0.8){
        const double f = std::sin(x) * std::exp(-x/2.0);
        const double df = f * 0.05;
        const double dx = x * 0.005;
        buffd.push_back(x, dx, f, df, InhibitSort);
    }
    for(double x = 0.0; x < 10.0; x += 1.6){
        const double f = std::sin(x) * std::exp(-x/2.0);
        const double df = f * 0.05;
        const double dx = x * 0.005;
        buffe.push_back(x, dx, f, df, InhibitSort);
    }

    auto local_curva = buffa.Local_Signed_Curvature_Three_Datum();
    auto local_curvb = buffb.Local_Signed_Curvature_Three_Datum();
    auto local_curvc = buffc.Local_Signed_Curvature_Three_Datum();
    auto local_curvd = buffd.Local_Signed_Curvature_Three_Datum();
    auto local_curve = buffe.Local_Signed_Curvature_Three_Datum();

    {
        YgorMathPlotting::Shuttle<samples_1D<double>> shtlA(local_curva, "Buffer A");
        YgorMathPlotting::Shuttle<samples_1D<double>> shtlB(local_curvb, "Buffer B");
        YgorMathPlotting::Shuttle<samples_1D<double>> shtlC(local_curvc, "Buffer C");

        YgorMathPlotting::Plot<double>({shtlA, shtlB, shtlC}, 
                                       std::string("Three Buffers (A,B,C)"), 
                                       std::string("x-axis title"),
                                       std::string("y-axis title"));
    }
    {
        YgorMathPlotting::Shuttle<samples_1D<double>> shtlA(local_curvc, "Buffer C");
        YgorMathPlotting::Shuttle<samples_1D<double>> shtlB(local_curvd, "Buffer D");
        YgorMathPlotting::Shuttle<samples_1D<double>> shtlC(local_curve, "Buffer E");

        YgorMathPlotting::Plot<double>({shtlA, shtlB, shtlC},
                                       std::string("Three Buffers (C,D,E)"),
                                       std::string("x-axis title"),
                                       std::string("y-axis title"));
    }
    {
        YgorMathPlotting::Shuttle<samples_1D<double>> shtlA(local_curva, "Buffer A");
        YgorMathPlotting::Shuttle<samples_1D<double>> shtlB(local_curvc, "Buffer C");
        YgorMathPlotting::Shuttle<samples_1D<double>> shtlC(local_curve, "Buffer E");

        YgorMathPlotting::Plot<double>({shtlA, shtlB, shtlC},
                                       std::string("Three Buffers (A,C,E)"),
                                       std::string("x-axis title"),
                                       std::string("y-axis title"));
    }

    FUNCINFO("Main thread waiting now for 10 seconds");
    std::this_thread::sleep_for(std::chrono::seconds(10));
    FUNCINFO("Main thread exiting now");

return 0;

    Plotter2 plota;
    plota.Set_Global_Title("Effects of x_i spacing (dx) on local curvature computation: Samples");
    plota.Insert_samples_1D(buffa, "dx = 0.15");
    plota.Insert_samples_1D(buffb, "dx = 0.2");
    plota.Insert_samples_1D(buffc, "dx = 0.4");
    plota.Insert_samples_1D(buffd, "dx = 0.8");
    plota.Insert_samples_1D(buffe, "dx = 1.6");
    plota.Plot();

    Plotter2 plotb;
    plotb.Set_Global_Title("Effects of x_i spacing (dx) on local curvature computation: Curvature");
    plotb.Insert_samples_1D(local_curva, "dx = 0.15");
    plotb.Insert_samples_1D(local_curvb, "dx = 0.2");
    plotb.Insert_samples_1D(local_curvc, "dx = 0.4");
    plotb.Insert_samples_1D(local_curvd, "dx = 0.8");
    plotb.Insert_samples_1D(local_curve, "dx = 1.6");
    plotb.Plot();

    
    //Generate a plot showing the ANGLE of deviation from a straight line segment vs. the curvature.
    Plotter2 plotc;
    for(auto s = 0.1; s < 10.0; s += 0.5){
        samples_1D<double> corres;
        for(double y = 0.0; y < 100.0*s; y += 0.1*s){
            samples_1D<double> shtl;
            shtl.push_back(  0.0, 0.0, 0.0, 0.0);
            shtl.push_back(    s, 0.0, 0.0, 0.0);
            shtl.push_back(2.0*s, 0.0, y,   0.0);

            const auto curv = shtl.Local_Signed_Curvature_Three_Datum();
            const auto angle = std::atan2(y,2.0);

            corres.push_back( angle, curv.samples.front()[2] , true);
        }
        plotc.Insert_samples_1D(corres, "Curvature vs. Angle of deviation from straight line: s = "_s + Xtostring(s), "lines");
    }
    plotc.Plot();

    return 0;
}
