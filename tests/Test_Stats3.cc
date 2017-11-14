#include <iostream>
#include <cmath>
#include <vector>
#include <functional>

#include "YgorMisc.h"
#include "YgorStats.h"


int main(int argc, char **argv){

    auto print = [](std::vector<double> l) -> void {
        std::cout << " List: ";
        for(auto &e : l) std::cout << e << ", ";
        std::cout << std::endl;

        std::cout << "Percentile 0.00 = " << Stats::Percentile(l, 0.00) << std::endl;
        std::cout << "Percentile 0.20 = " << Stats::Percentile(l, 0.20) << std::endl;
        std::cout << "Percentile 0.50 = " << Stats::Percentile(l, 0.50) << std::endl;
        std::cout << "Median          = " << Stats::Median(l) << std::endl;
        std::cout << "Percentile 0.80 = " << Stats::Percentile(l, 0.80) << std::endl;
        std::cout << "Percentile 1.00 = " << Stats::Percentile(l, 1.00) << std::endl;

        std::cout << std::endl;

    };

    {
        print( { 0.0, 1.0, 2.0 } );

    }

    return 0;
}
