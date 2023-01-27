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

    {
        long int random_seed = 123456;
        std::mt19937 re( random_seed );

        //std::uniform_real_distribution<> rd(-1.0, 1.0); //Random distribution.
        std::uniform_real_distribution<> rd(0.0f, 1.0f); //Random distribution.

        const auto run_for = [&](size_t N) -> void {
            Stats::Running_Sum<double> rs;
            double n = 0.0;
            const auto int_scale = std::numeric_limits<double>::digits10 + 1;
            __int128_t t = 0LL;
            std::vector<double> d;
            d.reserve(N);

            for(size_t i = 0; i < N; ++i){
                double x = rd(re);
                n += x;
                t += static_cast<__int128_t>(x * std::pow(10.0,1.0 * int_scale));
                rs.Digest(x);
                d.push_back(x);
            }

            const auto cs = rs.Current_Sum();
            const auto ss = Stats::Sum(d);
            const auto tt = static_cast<double>(t) / std::pow(10.0,1.0 * int_scale);

            std::cout.precision(std::numeric_limits<double>::digits10 + 1);
            YLOGINFO("After " << N << " double-precision samples the naive sum is:              " << n  << " (" << 100.0*(n -tt)/tt << "\% from the integer-summed result)");
            YLOGINFO("After " << N << " double-precision samples the compensated sum is:        " << cs << " (" << 100.0*(cs-tt)/tt << "\% from the integer-summed result)");
            YLOGINFO("After " << N << " double-precision samples the sorted compensated sum is: " << ss << " (" << 100.0*(ss-tt)/tt << "\% from the integer-summed result)");
            YLOGINFO("After " << N << " double-precision samples the integer-based sum is:      " << tt);
            std::cout << std::endl;
        };

        run_for(50);
        run_for(500);
        run_for(5'000);
        run_for(50'000);
        run_for(500'000);
        run_for(5'000'000);
        run_for(50'000'000);
    }


    {
        long int random_seed = 123456;
        std::mt19937 re( random_seed );

        //std::uniform_real_distribution<> rd(-1.0f, 1.0f); //Random distribution.
        std::uniform_real_distribution<> rd(0.0f, 1.0f); //Random distribution.

        const auto run_for = [&](size_t N) -> void {
            Stats::Running_Sum<float> rs;
            float n = 0.0;
            const auto int_scale = std::numeric_limits<float>::digits10 + 1;
            __int128_t t = 0LL;
            std::vector<float> d;
            d.reserve(N);

            for(size_t i = 0; i < N; ++i){
                float x = rd(re);
                n += x;
                t += static_cast<__int128_t>(x * std::pow(10.0,1.0 * int_scale));
                rs.Digest(x);
                d.push_back(x);
            }

            const auto cs = rs.Current_Sum();
            const auto ss = Stats::Sum(d);
            const auto tt = static_cast<float>(t) / std::pow(10.0,1.0 * int_scale);

            std::cout.precision(std::numeric_limits<float>::digits10 + 1);
            YLOGINFO("After " << N << " single-precision samples the naive sum is:              " << n  << " (" << 100.0*(n -tt)/tt << "\% from the integer-summed result)");
            YLOGINFO("After " << N << " single-precision samples the compensated sum is:        " << cs << " (" << 100.0*(cs-tt)/tt << "\% from the integer-summed result)");
            YLOGINFO("After " << N << " single-precision samples the sorted compensated sum is: " << ss << " (" << 100.0*(ss-tt)/tt << "\% from the integer-summed result)");
            YLOGINFO("After " << N << " single-precision samples the integer-based sum is:      " << tt);
            std::cout << std::endl;
        };

        run_for(50);
        run_for(500);
        run_for(5'000);
        run_for(50'000);
        run_for(500'000);
        run_for(5'000'000);
        run_for(50'000'000);
    }
    return 0;
}
