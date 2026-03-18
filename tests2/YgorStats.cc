
#include <limits>
#include <cmath>
#include <vector>
#include <random>

#include <YgorStats.h>

#include "doctest/doctest.h"


TEST_CASE( "Running_Variance" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("known data [2,4,4,4,5,5,7,9]"){
        Stats::Running_Variance<double> rv;
        std::vector<double> data = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};

        for(const auto &x : data){
            rv.Digest(x);
        }

        REQUIRE( std::abs(rv.Current_Mean() - 5.0) < eps );
        REQUIRE( std::abs(rv.Current_Variance() - 4.0) < eps );
        REQUIRE( std::abs(rv.Current_Sample_Variance() - 32.0/7.0) < eps );
    }

    SUBCASE("single value"){
        Stats::Running_Variance<double> rv;
        rv.Digest(42.0);

        REQUIRE( std::abs(rv.Current_Mean() - 42.0) < eps );
        REQUIRE( std::abs(rv.Current_Variance() - 0.0) < eps );
    }

    SUBCASE("identical values"){
        Stats::Running_Variance<double> rv;
        for(int i = 0; i < 100; ++i){
            rv.Digest(7.0);
        }

        REQUIRE( std::abs(rv.Current_Mean() - 7.0) < eps );
        REQUIRE( std::abs(rv.Current_Variance() - 0.0) < eps );
        REQUIRE( std::abs(rv.Current_Sample_Variance() - 0.0) < eps );
    }

    SUBCASE("random data: running vs batch"){
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
        const double running_sample_variance = rv.Current_Sample_Variance();

        const double batch_mean = Stats::Mean(data);
        const double batch_sample_variance = Stats::Unbiased_Var_Est(data);

        REQUIRE( std::abs(running_mean - batch_mean) < eps );
        REQUIRE( std::abs(running_sample_variance - batch_sample_variance) < eps );
    }

    SUBCASE("numerical stability with large offset"){
        Stats::Running_Variance<double> rv;

        const double offset = 1e10;
        std::vector<double> data = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};

        for(const auto &x : data){
            rv.Digest(x + offset);
        }

        REQUIRE( std::abs(rv.Current_Mean() - (5.0 + offset)) < 1.0 );
        REQUIRE( std::abs(rv.Current_Variance() - 4.0) < 1.0e-6 );
    }
}
