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

    // Test 1: Simple known variance case
    {
        YLOGINFO("---- Test 1: Simple known variance case ----");
        Stats::Running_Variance<double> rv;
        
        // Dataset: [2, 4, 4, 4, 5, 5, 7, 9]
        // Mean = 5.0
        // Population variance = 4.0
        // Sample variance = 4.571428...
        std::vector<double> data = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
        
        for(const auto &x : data){
            rv.Digest(x);
        }
        
        const double mean = rv.Current_Mean();
        const double variance = rv.Current_Variance();
        const double sample_variance = rv.Current_Sample_Variance();
        
        std::cout.precision(std::numeric_limits<double>::digits10 + 1);
        YLOGINFO("Mean: " << mean << " (expected: 5.0)");
        YLOGINFO("Population variance: " << variance << " (expected: 4.0)");
        YLOGINFO("Sample variance: " << sample_variance << " (expected: ~4.571428)");
        std::cout << std::endl;
    }

    // Test 2: Single value
    {
        YLOGINFO("---- Test 2: Single value ----");
        Stats::Running_Variance<double> rv;
        rv.Digest(42.0);
        
        const double mean = rv.Current_Mean();
        const double variance = rv.Current_Variance();
        
        YLOGINFO("Mean: " << mean << " (expected: 42.0)");
        YLOGINFO("Population variance: " << variance << " (expected: 0.0)");
        
        // Sample variance with N=1 should return NaN
        const double sample_variance = rv.Current_Sample_Variance();
        YLOGINFO("Sample variance: " << sample_variance << " (expected: NaN)");
        std::cout << std::endl;
    }

    // Test 3: Identical values (zero variance)
    {
        YLOGINFO("---- Test 3: Identical values (zero variance) ----");
        Stats::Running_Variance<double> rv;
        
        for(int i = 0; i < 100; ++i){
            rv.Digest(7.0);
        }
        
        const double mean = rv.Current_Mean();
        const double variance = rv.Current_Variance();
        const double sample_variance = rv.Current_Sample_Variance();
        
        YLOGINFO("Mean: " << mean << " (expected: 7.0)");
        YLOGINFO("Population variance: " << variance << " (expected: 0.0)");
        YLOGINFO("Sample variance: " << sample_variance << " (expected: 0.0)");
        std::cout << std::endl;
    }

    // Test 4: Random values (compare with Stats::Unbiased_Var_Est)
    {
        YLOGINFO("---- Test 4: Random values (compare with batch calculation) ----");
        int64_t random_seed = 123456;
        std::mt19937 re(random_seed);
        std::uniform_real_distribution<> rd(0.0, 100.0);
        
        const size_t N = 10000;
        Stats::Running_Variance<double> rv;
        std::vector<double> data;
        data.reserve(N);
        
        for(size_t i = 0; i < N; ++i){
            double x = rd(re);
            rv.Digest(x);
            data.push_back(x);
        }
        
        const double running_mean = rv.Current_Mean();
        const double running_variance = rv.Current_Variance();
        const double running_sample_variance = rv.Current_Sample_Variance();
        
        const double batch_mean = Stats::Mean(data);
        const double batch_sample_variance = Stats::Unbiased_Var_Est(data);
        
        YLOGINFO("Running mean: " << running_mean);
        YLOGINFO("Batch mean:   " << batch_mean);
        YLOGINFO("Difference:   " << std::abs(running_mean - batch_mean));
        std::cout << std::endl;
        
        YLOGINFO("Running sample variance: " << running_sample_variance);
        YLOGINFO("Batch sample variance:   " << batch_sample_variance);
        YLOGINFO("Difference:              " << std::abs(running_sample_variance - batch_sample_variance));
        std::cout << std::endl;
    }

    // Test 5: Float precision
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

    // Test 6: Numerical stability with very large values
    {
        YLOGINFO("---- Test 6: Numerical stability with large values ----");
        Stats::Running_Variance<double> rv;
        
        // Add a large offset to test numerical stability
        const double offset = 1e10;
        std::vector<double> data = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
        
        for(const auto &x : data){
            rv.Digest(x + offset);
        }
        
        const double mean = rv.Current_Mean();
        const double variance = rv.Current_Variance();
        
        YLOGINFO("Mean: " << mean << " (expected: " << (5.0 + offset) << ")");
        YLOGINFO("Population variance: " << variance << " (expected: 4.0)");
        YLOGINFO("Variance error from expected: " << std::abs(variance - 4.0));
        std::cout << std::endl;
    }

    return 0;
}
