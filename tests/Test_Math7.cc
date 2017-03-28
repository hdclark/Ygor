//Test_Math7.cc - This is a test file for statistical routines in samples_1D.
//                Also << and >> operations for writing/reading/

#include <iostream>
#include <cmath>
#include <functional>
#include <list>
#include <random>
#include <string>

#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorAlgorithms.h"
//#include "YgorMath_Samples.h"
#include "YgorPlot.h"


void Perform_Analysis(const samples_1D<double> &in, const std::string &Title){
    std::cout << "Performing analysis on '" << Title << "' now." << std::endl;
    in.Plot(Title);

    const auto lrstats = in.Linear_Least_Squares_Regression();
    std::cout << "Linear regression gives slope, intercept = " << std::get<0>(lrstats) << ", " << std::get<1>(lrstats) << std::endl;

    NPRLL::Attempt_Auto_Analysis(in); //This might create a little too much data...

    const auto stats = in.Spearmans_Rank_Correlation_Coefficient();
    std::cout << Title << " has Spearman's rank corr coeff, num of samples, z-value, t-value: ";
    std::cout << std::get<0>(stats) << ", " << std::get<1>(stats) << ", " << std::get<2>(stats) << ", " << std::get<3>(stats) << std::endl;
    std::cout << "  P-value = " << PValue_From_StudentT_Two_Tail(std::get<3>(stats), std::get<1>(stats)) << std::endl;

    std::cout << "=====================================================================================" << std::endl;
    return;
}


int main(int argc, char **argv){
    samples_1D<double> data;

    std::random_device rde;
    std::mt19937 re( rde() );
    std::uniform_real_distribution<double> rd_x(-10.0, 20.0);
    std::uniform_real_distribution<double> rd_y(0.0, 10.0);

    //Fill the samples.
    for(long int i = 0; i < 25; ++i){
        const auto x = rd_x(re);
        const auto y = rd_y(re);
        data.samples.push_back(vec2<double>(x,y));
    }
    data.Order_Data_Lowest_First();
    Perform_Analysis(data, "Initial Distribution");

    {
        //Rotate the data by a fixed angle (about the origin).
        samples_1D<double> rotdata = data;
        const double t = -2.0*M_PI/6.0;
        for(auto it = rotdata.samples.begin(); it != rotdata.samples.end(); ++it){
            const auto x = it->x, y = it->y;
            it->x = std::cos(t)*x - std::sin(t)*y;
            it->y = std::sin(t)*x + std::cos(t)*y;
        }
        rotdata.Order_Data_Lowest_First();
        Perform_Analysis(rotdata, "Rotated Distribution");
    }
    {
        //Gradually step down all samples.
        samples_1D<double> stepdata = data;
        for(auto it = stepdata.samples.begin(); it != stepdata.samples.end(); ++it){
            const auto x = it->x, y = it->y;
            it->y -= (15.0/20.0)*x;
        }
        stepdata.Order_Data_Lowest_First(); //This is a noop..
        Perform_Analysis(stepdata, "Stepped-down Distribution");
    }

    {
        samples_1D<double> smaller;
        for(double x = 0.0; x <= 10.0; x += 1.0){
            const auto y = rd_y(re);
            smaller.samples.push_back(vec2<double>(x,y));
        }

        std::cout << "About to write samples_1D:" << std::endl;
        std::cout << smaller << std::endl;
//        std::cout << "Now pass in a samples_1D:" << std::endl;
//        std::cin  >> smaller;
//        std::cout << "Passed in samples_1D:" << std::endl;
//        std::cout << smaller << std::endl;
    }


    return 0;
}

