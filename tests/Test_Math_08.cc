
#include <iostream>
#include <cmath>
#include <list>
#include <functional>
#include <tuple>
#include <vector>
#include <random>
#include <sstream>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorStats.h"
#include "YgorPlot.h" //For visual comparison. Hard to test the output!
#include "YgorFilesDirs.h"

int main(int argc, char **argv){
  
    Plotter2 toplot;
    toplot.Set_Global_Title("Two-sided moving averages");

    samples_1D<double> data; 
    data.uncertainties_known_to_be_independent_and_random = true;

    //Fill with data until [ctrl]+[D] or EOF.
    double x_i, sigma_x_i, f_i, sigma_f_i;
    while(std::cin >> x_i >> sigma_x_i >> f_i >> sigma_f_i){
        data.push_back(x_i, sigma_x_i, f_i, sigma_f_i);
    }

    
    toplot.Insert_samples_1D(data, "Raw data", "linespoints");

    auto data_sma = data.Moving_Average_Two_Sided_Spencers_15_point();
                        //.Moving_Average_Two_Sided_Spencers_15_point()
                        //.Moving_Average_Two_Sided_Spencers_15_point();
    toplot.Insert_samples_1D(data_sma, "Spencer's 15pt weighted moving average thrice", "linespoints");

    auto data_ma = data.Moving_Average_Two_Sided_Equal_Weighting(10);
                        //.Moving_Average_Two_Sided_Equal_Weighting(10)
                        //.Moving_Average_Two_Sided_Equal_Weighting(10);
    toplot.Insert_samples_1D(data_ma, "21pt moving average thrice", "linespoints");

    auto data_hma = data.Moving_Average_Two_Sided_Hendersons_23_point();
                        //.Moving_Average_Two_Sided_Hendersons_23_point()
                        //.Moving_Average_Two_Sided_Hendersons_23_point();
    toplot.Insert_samples_1D(data_hma, "Henderson's 23pt weighted moving average", "linespoints");

    auto data_gma = data.Moving_Average_Two_Sided_Gaussian_Weighting(4.0);
                        //.Moving_Average_Two_Sided_Gaussian_Weighting(3.0)
                        //.Moving_Average_Two_Sided_Gaussian_Weighting(2.0);
    toplot.Insert_samples_1D(data_gma, "Gaussian 4pt sigma weighted moving average", "linespoints");

    auto data_mf = data.Moving_Median_Filter_Two_Sided_Equal_Weighting(5)
                       .Moving_Median_Filter_Two_Sided_Equal_Weighting(5)
                       .Moving_Median_Filter_Two_Sided_Equal_Weighting(5);
    toplot.Insert_samples_1D(data_mf, "11pt moving median filter", "linespoints");

    auto data_gmf = data.Moving_Median_Filter_Two_Sided_Gaussian_Weighting(4.0)
                        .Moving_Median_Filter_Two_Sided_Gaussian_Weighting(4.0)
                        .Moving_Median_Filter_Two_Sided_Gaussian_Weighting(4.0);
    toplot.Insert_samples_1D(data_gmf, "Gaussian 4pt sigma weighted median filter", "linespoints");

    auto data_tmf = data.Moving_Median_Filter_Two_Sided_Triangular_Weighting(10);
    toplot.Insert_samples_1D(data_tmf, "21pt Triangular weighted median filter", "linespoints");

    auto data_detrended = data.Subtract(data_gma);
    toplot.Insert_samples_1D(data_detrended, "De-trended data", "linespoints");

    toplot.Plot();
    //toplot.Plot_as_PDF("/tmp/out.pdf");
    //OverwriteStringToFile(toplot.Dump_as_String(), "/tmp/plotme.gnuplot");

 
    return 0;
}
