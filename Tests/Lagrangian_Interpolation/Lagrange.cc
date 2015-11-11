#include <iostream>
#include <random>

#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorMathPlottingGnuplot.h"
#include "YgorMathPlottingVTK.h"

int main(){

    std::vector<YgorMathPlottingVTK::Shuttle<samples_1D<double>>> shuttle;
    //std::vector<YgorMathPlottingGnuplot::Shuttle<samples_1D<double>>> shuttle;


    // Read the data in from stdin as 4 doubles: {x, dx, f, df}.
    // This is the simple samples_1D dump format (via samples_1D::Write_To_File(), NOT the serialization format).
    samples_1D<double> data;
    while(std::cin.good()){
        std::array<double,4> shtl;
        std::cin >> shtl[0] >> shtl[1] >> shtl[2] >> shtl[3];
        data.push_back(shtl);
    }
    shuttle.emplace_back(data, "Original data");

    /////////////////////////////////////////////////////////////////////////////////////////////

    std::mt19937 re;
    re.seed(123);
    samples_1D<double> data2;
    std::uniform_real_distribution<> x_noise(-2.0, 2.0);
    std::uniform_real_distribution<> f_noise(-2.0, 2.0);
    for(const auto &d : data.samples){
        for(size_t i = 0; i < 5; ++i){
            auto shtl = d;
            shtl[0] += x_noise(re); 
            shtl[1] = 1.0;
            shtl[2] += f_noise(re);
            shtl[3] = 1.0;
            data2.push_back(shtl);
        }
    }
    shuttle.emplace_back(data2, "Duplicated");

    //Step 0: Sort on x, lowest-first. Average any points that coincide exactly at a given x.
    samples_1D<double> data3(data2);
    data3.Average_Coincident_Data(2.0);

    shuttle.emplace_back(data3, "Coincident");
   

 
    //Step 1: Figure out how to sample the interpolation.
    //
    // There are theoretical arguments for specific sampling patterns. For example, Chebyshev point
    // sampling in which the samples have a density proportional to 1/sqrt(1+x*x) so that the points
    // at infinity have many points. 
    //
    // This approach begets 'compressed sensing' by potentially discarding needed information in 
    // regions of high energy (in real-world data).
    //
    // It is easier to sample uniformly, and easy to say that it seems reasonable for general data,
    // however there are theoretical arguments that show polynomial interpolation on equally-spaced
    // data is an ill-conditioned problem. See the papers on interpolation downloaded 20151109.
 


    //Simple uniform sampling.
    const auto extrema = data.Get_Extreme_Datum_x();
    const double xmin = extrema[0];
    const double xmax = extrema[2];
    const double dx = (xmax - xmin)/1000.0;
    if(dx < 0.0) throw std::runtime_error("Data cannot be interpolated, or sorting was disabled.");


    //


  

    FUNCINFO("Everything seems OK. Loading gnuplots..");
    YgorMathPlottingVTK::Plot<double>(shuttle, "Lagrange/Newton Interpolation", "time (s)", "Contrast Enhancement");
    //YgorMathPlottingGnuplot::Plot<double>(shuttle, "Lagrange/Newton Interpolation", "time (s)", "Contrast Enhancement");
    std::cout << "Waiting for input to terminate..." << std::endl;

    char a;
    std::cin >> a;
    return 0;
}
