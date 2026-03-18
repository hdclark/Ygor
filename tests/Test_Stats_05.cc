#include <iostream>
#include <cmath>
#include <vector>
#include <functional>
#include <random>
#include <iomanip>
#include <limits>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorStats.h"


int main(int argc, char **argv){

    // Test: Float precision
    {
        YLOGINFO("---- Test 5: Float precision test ----");
        int64_t random_seed = 654321;
        std::mt19937 re(random_seed);
        std::uniform_real_distribution<> rd(0.0f, 1.0f);
        
        const size_t N = 100000;
        Stats::Running_Variance<float> rv;
        
        for(size_t i = 0; i < N; ++i){
            float x = rd(re);
            rv.Digest(x);
        }
        
        const float mean = rv.Current_Mean();
        const float variance = rv.Current_Variance();
        
        std::cout.precision(std::numeric_limits<float>::digits10 + 1);
        YLOGINFO("After " << N << " single-precision samples:");
        YLOGINFO("Mean: " << mean << " (expected: ~0.5)");
        YLOGINFO("Variance: " << variance << " (expected: ~0.0833 for uniform [0,1])");
        std::cout << std::endl;
    }

    return 0;
}
