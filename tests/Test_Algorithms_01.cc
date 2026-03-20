#include <iostream>
#include <cmath>

#include "YgorAlgorithms.h"

int main(int argc, char **argv){

    //This is the function we will minimize. The minimum is exactly -71361/2500  ==  -28.5444 at f(0,-2.5,-2.88,-4.5).
    auto func_to_min = [](double p[]) -> double { return p[0]*p[0]*3.0 + (p[1]+2.5)*(p[1]+2.5) + (p[2]+5.76)*p[2] + (p[3]+9.0)*p[3]; };

    int dimen = 4;
   
    //These are the parameters which will be passed in to the function.
    double params[dimen];
    double result;
    size_t i;

    //Fill the parameters with some best-guesses.
    for(i=0;i<dimen;++i) params[i] = 0.67;

    //NMSimplex<double> minimizer(dimension, characteristic length, max number of iters, ftol);
    NMSimplex<double> minimizer(4, 3.01,  5000, 1E-8);
    minimizer.init(params, func_to_min);

    std::cout.precision(10);

    while(minimizer.iter() == 0){
        std::cout << "Best function value is currently " << minimizer.func_vals[minimizer.curr_min];
        std::cout << " at iteration " << minimizer.iteration << std::endl;
    }

    if(minimizer.iter() == -1) std::cout << "Scheme exited due to error. Maybe the initial parameters were not set?" << std::endl;
    if(minimizer.iter() ==  1) std::cout << "Scheme finished due to max number of iterations being exceeded." << std::endl;
    if(minimizer.iter() ==  2) std::cout << "Scheme completed due to ftol < ftol_min." << std::endl;

    minimizer.get_all(func_to_min,params,result);
    std::cout << "The result of the double nelder-mead simplex method was " << result << std::endl;


    return 0;
}
