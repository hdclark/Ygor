//Test_Math19.cc - This is a test file for samples_1D Otsu filtering.

#include <iostream>
#include <cmath>
#include <functional>
#include <list>
#include <random>
#include <string>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorAlgorithms.h"


int main(int argc, char **argv){
    samples_1D<double> data;

    data.push_back(-1.0, 1.23);
    data.push_back( 1.0, 2.34);
    {
        const auto thresh = data.Find_Otsu_Binarization_Threshold();
        FUNCINFO("Otsu's threshold: " << thresh);
    }

    data.push_back( 2.0, 3.45);
    {
        const auto thresh = data.Find_Otsu_Binarization_Threshold();
        FUNCINFO("Otsu's threshold: " << thresh);
    }

    data.push_back( 3.0, 4.56);
    {
        const auto thresh = data.Find_Otsu_Binarization_Threshold();
        FUNCINFO("Otsu's threshold: " << thresh);
    }

    data.push_back( 4.0, 5.67);
    {
        const auto thresh = data.Find_Otsu_Binarization_Threshold();
        FUNCINFO("Otsu's threshold: " << thresh);
    }

    data.push_back( 5.0, 6.78);
    {
        const auto thresh = data.Find_Otsu_Binarization_Threshold();
        FUNCINFO("Otsu's threshold: " << thresh);
    }

    return 0;
}

