
#include <limits>
#include <cmath>
#include <vector>
#include <random>
#include <array>
#include <sstream>
#include <string>
#include <cstdint>

#include <YgorStats.h>
#include <YgorMath.h>

#include "doctest/doctest.h"


TEST_CASE( "Running_Variance" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("known data [2,4,4,4,5,5,7,9]"){
        // Dataset: [2, 4, 4, 4, 5, 5, 7, 9]
        // Mean = 5.0
        // Population variance = 32/8 = 4.0
        // Sample variance = 32/7 = 4.571428...

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

        if(std::numeric_limits<double>::has_quiet_NaN){
            const double sample_variance = rv.Current_Sample_Variance();
            REQUIRE( std::isnan(sample_variance) );
        }else{
            REQUIRE_THROWS( rv.Current_Sample_Variance() );
        }
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

// ---------------------------------------------------------------------------
// Migrated from tests/Test_Stats_01.cc
// Welch's t-test (unequal variances) and Pearson correlation p-values.
// ---------------------------------------------------------------------------

TEST_CASE( "P_From_StudT_Diff_Means_From_Uneq_Vars" ){
    // Welch's t-test for difference of means with unequal variances.

    SUBCASE("significantly different means"){
        // Two groups with n=102 each, moderately different means and similar variances.
        // Reference: computed p ≈ 0.000273.
        double meanA = 8.605186092, varA = 0.801379847, numA = 102.0;
        double meanB = 8.132293417, varB = 0.8604915825, numB = 102.0;
        double p = Stats::P_From_StudT_Diff_Means_From_Uneq_Vars(meanA, varA, numA,
                                                                   meanB, varB, numB);
        REQUIRE( p > 0.0 );
        REQUIRE( p < 0.01 );
    }

    SUBCASE("significantly different means, altered variance"){
        // Same groups but varA replaced with sqrt(original varA) ≈ 0.895.
        // Reference: computed p ≈ 0.000394.
        double meanA = 8.605186092, varA = std::sqrt(0.801379847), numA = 102.0;
        double meanB = 8.132293417, varB = 0.8604915825, numB = 102.0;
        double p = Stats::P_From_StudT_Diff_Means_From_Uneq_Vars(meanA, varA, numA,
                                                                   meanB, varB, numB);
        REQUIRE( p > 0.0 );
        REQUIRE( p < 0.01 );
    }

    SUBCASE("identical parameters yield p ≈ 1"){
        // When both groups have identical mean and variance, p should be 1.
        double mean = 8.132293417, var = 0.8604915825, num = 102.0;
        double p = Stats::P_From_StudT_Diff_Means_From_Uneq_Vars(mean, var, num,
                                                                   mean, var, num);
        REQUIRE( p > 0.99 );
    }

    SUBCASE("hugely different means yield p ≈ 0"){
        // meanA = meanB + 20, same variance → extremely significant.
        double meanB = 8.132293417, varB = 0.8604915825, num = 102.0;
        double meanA = meanB + 20.0;
        double p = Stats::P_From_StudT_Diff_Means_From_Uneq_Vars(meanA, varB, num,
                                                                   meanB, varB, num);
        REQUIRE( p < 1.0e-50 );
    }

    SUBCASE("hugely different means, doubled variance"){
        // meanA = meanB + 20, varA = 2*varB → still extremely significant.
        double meanB = 8.132293417, varB = 0.8604915825, num = 102.0;
        double meanA = meanB + 20.0, varA = 2.0 * varB;
        double p = Stats::P_From_StudT_Diff_Means_From_Uneq_Vars(meanA, varA, num,
                                                                   meanB, varB, num);
        REQUIRE( p < 1.0e-50 );
    }
}


TEST_CASE( "P_From_Pearsons_Linear_Correlation_Coeff_2Tail" ){
    // Two-tailed p-values for Pearson's linear correlation coefficient.
    // Reference values from standard statistical tables.

    SUBCASE("r=0.3, n=6: expected p ≈ 0.56"){
        double p = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.3, 6.0);
        REQUIRE( std::abs(p - 0.5635) < 0.05 );
    }

    SUBCASE("r=0.8, n=3: expected p ≈ 0.41"){
        double p = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.8, 3.0);
        REQUIRE( std::abs(p - 0.41) < 0.05 );
    }

    SUBCASE("r=0.1, n=12: expected p ≈ 0.76"){
        double p = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.1, 12.0);
        REQUIRE( std::abs(p - 0.757) < 0.05 );
    }

    SUBCASE("r=1.0, n=5: expected p = 0"){
        double p = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(1.0, 5.0);
        REQUIRE( p < 1.0e-10 );
    }

    SUBCASE("r=1.0, n=35: expected p = 0"){
        double p = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(1.0, 35.0);
        REQUIRE( p < 1.0e-10 );
    }

    SUBCASE("r=0.0, n=5: expected p ≈ 1.0"){
        double p = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.0, 5.0);
        REQUIRE( std::abs(p - 1.0) < 0.01 );
    }

    SUBCASE("r=0.0, n=35: expected p ≈ 1.0"){
        double p = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.0, 35.0);
        REQUIRE( std::abs(p - 1.0) < 0.01 );
    }

    SUBCASE("r=0.4, n=19: expected p ≈ 0.09"){
        double p = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.4, 19.0);
        REQUIRE( std::abs(p - 0.09) < 0.02 );
    }

    SUBCASE("r=0.6, n=25: expected p ≈ 0.002"){
        double p = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.6, 25.0);
        REQUIRE( std::abs(p - 0.00152) < 0.005 );
    }

    SUBCASE("r=0.5, n=40: expected p ≈ 0.001"){
        double p = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.5, 40.0);
        REQUIRE( std::abs(p - 0.00102) < 0.005 );
    }

    SUBCASE("r=0.7, n=12: expected p ≈ 0.011"){
        double p = Stats::P_From_Pearsons_Linear_Correlation_Coeff_2Tail(0.7, 12.0);
        REQUIRE( std::abs(p - 0.0113) < 0.01 );
    }
}


// ---------------------------------------------------------------------------
// Migrated from tests/Test_Stats_02.cc
// Paired t-test, Wilcoxon signed-rank test, and z-score p-values.
// ---------------------------------------------------------------------------

TEST_CASE( "P_From_Zscore" ){
    // P-values computed from critical z-scores.
    // These z-scores correspond to well-known significance levels.
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("two-tailed p-values from critical z-scores"){
        // z=1.645 → 2-tail p ≈ 0.10
        REQUIRE( std::abs(Stats::P_From_Zscore_2Tail(1.645) - 0.10) < 0.01 );
        // z=1.960 → 2-tail p ≈ 0.05
        REQUIRE( std::abs(Stats::P_From_Zscore_2Tail(1.960) - 0.05) < 0.01 );
        // z=2.326 → 2-tail p ≈ 0.02
        REQUIRE( std::abs(Stats::P_From_Zscore_2Tail(2.326) - 0.02) < 0.01 );
        // z=2.576 → 2-tail p ≈ 0.01
        REQUIRE( std::abs(Stats::P_From_Zscore_2Tail(2.576) - 0.01) < 0.005 );
        // z=3.291 → 2-tail p ≈ 0.001
        REQUIRE( std::abs(Stats::P_From_Zscore_2Tail(3.291) - 0.001) < 0.001 );
    }

    SUBCASE("upper-tailed p-values from critical z-scores"){
        // z=1.645 → upper-tail p ≈ 0.05
        REQUIRE( std::abs(Stats::P_From_Zscore_Upper_Tail(1.645) - 0.05) < 0.005 );
        // z=1.960 → upper-tail p ≈ 0.025
        REQUIRE( std::abs(Stats::P_From_Zscore_Upper_Tail(1.960) - 0.025) < 0.005 );
        // z=2.326 → upper-tail p ≈ 0.01
        REQUIRE( std::abs(Stats::P_From_Zscore_Upper_Tail(2.326) - 0.01) < 0.005 );
        // z=2.576 → upper-tail p ≈ 0.005
        REQUIRE( std::abs(Stats::P_From_Zscore_Upper_Tail(2.576) - 0.005) < 0.002 );
        // z=3.291 → upper-tail p ≈ 0.0005
        REQUIRE( std::abs(Stats::P_From_Zscore_Upper_Tail(3.291) - 0.0005) < 0.0005 );
    }

    SUBCASE("lower-tailed p-values from critical z-scores"){
        // lower-tail p = 1 - upper-tail p for positive z
        // z=1.645 → lower-tail p ≈ 0.95
        REQUIRE( std::abs(Stats::P_From_Zscore_Lower_Tail(1.645) - 0.95) < 0.005 );
        // z=1.960 → lower-tail p ≈ 0.975
        REQUIRE( std::abs(Stats::P_From_Zscore_Lower_Tail(1.960) - 0.975) < 0.005 );
        // z=2.326 → lower-tail p ≈ 0.99
        REQUIRE( std::abs(Stats::P_From_Zscore_Lower_Tail(2.326) - 0.99) < 0.005 );
        // z=2.576 → lower-tail p ≈ 0.995
        REQUIRE( std::abs(Stats::P_From_Zscore_Lower_Tail(2.576) - 0.995) < 0.002 );
        // z=3.291 → lower-tail p ≈ 0.9995
        REQUIRE( std::abs(Stats::P_From_Zscore_Lower_Tail(3.291) - 0.9995) < 0.0005 );
    }

    SUBCASE("symmetry: 2-tail = 2 * upper-tail"){
        double z_vals[] = {0.5, 1.0, 1.96, 2.576, 3.0};
        for(double z : z_vals){
            double two = Stats::P_From_Zscore_2Tail(z);
            double upper = Stats::P_From_Zscore_Upper_Tail(z);
            REQUIRE( std::abs(two - 2.0 * upper) < eps );
        }
    }

    SUBCASE("complementarity: upper + lower = 1"){
        double z_vals[] = {0.5, 1.0, 1.96, 2.576, 3.0};
        for(double z : z_vals){
            double upper = Stats::P_From_Zscore_Upper_Tail(z);
            double lower = Stats::P_From_Zscore_Lower_Tail(z);
            REQUIRE( std::abs(upper + lower - 1.0) < eps );
        }
    }
}


TEST_CASE( "P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail" ){
    // Wilcoxon signed-rank test for paired samples.

    SUBCASE("Wikipedia example (10 pairs, small sample)"){
        // Standard textbook example. With only 10 pairs, may not have enough
        // data for a z-score approximation. Reference: p ≈ 0.594.
        std::vector<std::array<double,2>> data = {
            {125.0, 110.0}, {115.0, 122.0}, {130.0, 125.0},
            {140.0, 120.0}, {140.0, 140.0}, {115.0, 124.0},
            {140.0, 123.0}, {125.0, 137.0}, {140.0, 135.0},
            {135.0, 145.0}
        };
        double p = Stats::P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(data);
        REQUIRE( p >= 0.0 );
        REQUIRE( p <= 1.0 );
        REQUIRE( std::abs(p - 0.594) < 0.05 );
    }

    SUBCASE("Vassar stats example (16 pairs)"){
        // Example from Vassar statistics. Has enough data for z-score approximation.
        // Reference: p ≈ 0.035.
        std::vector<std::array<double,2>> data = {
            {78.0, 78.0}, {24.0, 24.0}, {64.0, 62.0}, {45.0, 48.0},
            {64.0, 68.0}, {52.0, 56.0}, {30.0, 25.0}, {50.0, 44.0},
            {64.0, 56.0}, {50.0, 40.0}, {78.0, 68.0}, {22.0, 36.0},
            {84.0, 68.0}, {40.0, 20.0}, {90.0, 58.0}, {72.0, 32.0}
        };
        double p = Stats::P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(data);
        REQUIRE( p > 0.0 );
        REQUIRE( p < 0.10 );
        REQUIRE( std::abs(p - 0.035) < 0.02 );
    }

    SUBCASE("Rosie Shier's Mathematics Learning Support Centre example (12 pairs)"){
        // Most pairs show an increase. Reference: p ≈ 0.012.
        std::vector<std::array<double,2>> data = {
            { 2.0,  3.5}, { 3.6,  5.7}, { 2.6,  2.9}, { 2.6,  2.4},
            { 7.3,  9.9}, { 3.4,  3.3}, {14.9, 16.7}, { 6.6,  6.0},
            { 2.3,  3.8}, { 2.0,  4.0}, { 6.8,  9.1}, { 8.5, 20.9}
        };
        double p = Stats::P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(data);
        REQUIRE( p > 0.0 );
        REQUIRE( p < 0.05 );
        REQUIRE( std::abs(p - 0.012) < 0.01 );
    }
}


TEST_CASE( "Paired_tests_with_samples_1D_data" ){
    // Paired t-test and Wilcoxon signed-rank test on normalized dose–volume
    // histogram data from three anatomical structures (parotid, masseter, pharynx).
    // The data is parsed via samples_1D and normalized with Normalize_wrt_Self_Overlap().

    samples_1D<double> parotid, masseter, pharynx;

    {
        std::stringstream ss;
        ss << "(samples_1D. normalityassumed= 0 num_samples= 160"
           " 0 0 1.01562 0 3.839 0 1.06381 0 7.678 0 1.05854 0 11.517 0 1.06484 0"
           " 15.356 0 1.08508 0 19.195 0 1.12542 0 23.034 0 1.14002 0 26.873 0 1.18045 0"
           " 30.712 0 1.22493 0 34.552 0 1.3279 0 38.391 0 1.45237 0 42.23 0 1.5204 0"
           " 46.069 0 1.59349 0 49.908 0 1.62751 0 53.747 0 1.64555 0 57.586 0 1.63464 0"
           " 61.425 0 1.58377 0 65.264 0 1.54239 0 69.103 0 1.47915 0 72.943 0 1.42672 0"
           " 76.782 0 1.34445 0 80.621 0 1.24747 0 84.46 0 1.14485 0 88.299 0 1.09533 0"
           " 92.138 0 0.995995 0 95.977 0 0.887188 0 99.816 0 0.906273 0 103.655 0 0.900058 0"
           " 107.494 0 0.910562 0 111.334 0 0.939718 0 115.173 0 0.954063 0 119.012 0 0.943601 0"
           " 122.851 0 0.922706 0 126.69 0 0.904143 0 130.529 0 0.904529 0 134.368 0 0.907703 0"
           " 138.207 0 0.897426 0 142.046 0 0.875949 0 145.885 0 0.841873 0 149.725 0 0.837786 0"
           " 153.564 0 0.880344 0 157.403 0 0.884444 0 161.242 0 0.88573 0 165.081 0 0.913688 0"
           " 168.92 0 0.951541 0 172.759 0 0.962066 0 176.598 0 1.0138 0 180.437 0 1.04106 0"
           " 184.276 0 1.05357 0 188.115 0 1.08916 0 191.955 0 1.10095 0 195.794 0 1.10983 0"
           " 199.633 0 1.11512 0 203.472 0 1.12667 0 207.311 0 1.13829 0 211.15 0 1.1238 0"
           " 214.989 0 1.11381 0 218.828 0 1.09584 0 222.667 0 1.08974 0 226.506 0 1.06554 0"
           " 230.346 0 1.07161 0 234.185 0 1.03179 0 238.024 0 1.02063 0 241.863 0 1.01264 0"
           " 245.702 0 0.988766 0 249.541 0 0.963231 0 253.38 0 0.952252 0 257.219 0 0.955826 0"
           " 261.058 0 0.919561 0 264.897 0 0.875694 0 268.737 0 0.885181 0 272.576 0 1.06365 0"
           " 276.415 0 1.07132 0 280.254 0 1.08476 0 284.093 0 1.09875 0 287.932 0 1.13463 0"
           " 291.771 0 1.13676 0 295.61 0 1.16989 0 299.449 0 1.16298 0 303.288 0 1.17537 0"
           " 307.128 0 1.1725 0 310.967 0 1.16526 0 314.806 0 1.17836 0 318.645 0 1.16088 0"
           " 322.484 0 1.18485 0 326.323 0 1.23181 0 330.162 0 1.06401 0 334.001 0 1.0744 0"
           " 337.84 0 1.07711 0 341.679 0 1.077 0 345.518 0 1.05201 0 349.358 0 1.05633 0"
           " 353.197 0 1.02654 0 357.036 0 1.0366 0 360.875 0 1.03006 0 364.714 0 1.03278 0"
           " 368.553 0 1.03018 0 372.392 0 1.01966 0 376.231 0 1.07118 0 380.07 0 1.05291 0"
           " 383.909 0 1.04731 0 387.749 0 1.09475 0 391.588 0 1.11594 0 395.427 0 1.11108 0"
           " 399.266 0 1.12197 0 403.105 0 1.14387 0 406.944 0 1.14102 0 410.783 0 1.16803 0"
           " 414.622 0 1.23655 0 418.461 0 1.24317 0 422.3 0 1.2705 0 426.14 0 1.27493 0"
           " 429.979 0 1.29292 0 433.818 0 1.27613 0 437.657 0 1.2733 0 441.496 0 1.25583 0"
           " 445.335 0 1.32791 0 449.174 0 1.32334 0 453.013 0 1.32701 0 456.852 0 1.33017 0"
           " 460.691 0 1.31874 0 464.531 0 1.41039 0 468.37 0 1.43935 0 472.209 0 1.41345 0"
           " 476.048 0 1.43912 0 479.887 0 1.41073 0 483.726 0 1.42082 0 487.565 0 1.42486 0"
           " 491.404 0 1.44946 0 495.243 0 1.43009 0 499.082 0 1.40927 0 502.921 0 1.34882 0"
           " 506.761 0 1.31893 0 510.6 0 1.31651 0 514.439 0 1.30697 0 518.278 0 1.29097 0"
           " 522.117 0 1.22789 0 525.956 0 1.18106 0 529.795 0 1.15615 0 533.634 0 1.12064 0"
           " 537.473 0 1.13669 0 541.312 0 1.09394 0 545.152 0 1.12306 0 548.991 0 1.06674 0"
           " 552.83 0 1.0828 0 556.669 0 1.09425 0 560.508 0 1.08987 0 564.347 0 1.09982 0"
           " 568.186 0 1.09752 0 572.025 0 1.09647 0 575.864 0 1.15553 0 579.703 0 1.15264 0"
           " 583.543 0 1.16331 0 587.382 0 1.5941 0 591.221 0 1.64549 0 595.06 0 1.69685 0"
           " 598.899 0 1.75094 0 602.738 0 1.75746 0 606.577 0 1.82595 0 610.416 0 1.85449 0"
           " num_metadata= 0 )";
        ss >> parotid;
    }

    {
        std::stringstream ss;
        ss << "(samples_1D. normalityassumed= 0 num_samples= 160"
           " 0 0 0.124245 0 3.839 0 0.113352 0 7.678 0 0.1052 0 11.517 0 0.100014 0"
           " 15.356 0 0.0951938 0 19.195 0 0.0905557 0 23.034 0 0.0866533 0 26.873 0 0.0867034 0"
           " 30.712 0 0.134324 0 34.552 0 0.25156 0 38.391 0 0.40273 0 42.23 0 0.534563 0"
           " 46.069 0 0.656232 0 49.908 0 0.749597 0 53.747 0 0.769374 0 57.586 0 0.810791 0"
           " 61.425 0 0.823305 0 65.264 0 0.784526 0 69.103 0 0.717455 0 72.943 0 0.623244 0"
           " 76.782 0 0.484925 0 80.621 0 0.33441 0 84.46 0 0.186045 0 88.299 0 0.109697 0"
           " 92.138 0 0.0841506 0 95.977 0 0.0726764 0 99.816 0 0.0637969 0 103.655 0 0.0624207 0"
           " 107.494 0 0.0651357 0 111.334 0 0.0627178 0 115.173 0 0.0634492 0 119.012 0 0.0651144 0"
           " 122.851 0 0.0650617 0 126.69 0 0.0636952 0 130.529 0 0.0620742 0 134.368 0 0.0555182 0"
           " 138.207 0 0.0508865 0 142.046 0 0.046201 0 145.885 0 0.0464595 0 149.725 0 0.046104 0"
           " 153.564 0 0.0447876 0 157.403 0 0.042816 0 161.242 0 0.043249 0 165.081 0 0.0426918 0"
           " 168.92 0 0.0431102 0 172.759 0 0.0440611 0 176.598 0 0.0465236 0 180.437 0 0.0466961 0"
           " 184.276 0 0.0473576 0 188.115 0 0.0482582 0 191.955 0 0.0491528 0 195.794 0 0.0483368 0"
           " 199.633 0 0.0501933 0 203.472 0 0.0515781 0 207.311 0 0.0560583 0 211.15 0 0.0559256 0"
           " 214.989 0 0.0544701 0 218.828 0 0.0546636 0 222.667 0 0.0540459 0 226.506 0 0.0540523 0"
           " 230.346 0 0.0567152 0 234.185 0 0.0583739 0 238.024 0 0.0589267 0 241.863 0 0.058371 0"
           " 245.702 0 0.0597479 0 249.541 0 0.0615032 0 253.38 0 0.0597276 0 257.219 0 0.0585958 0"
           " 261.058 0 0.0583066 0 264.897 0 0.0555681 0 268.737 0 0.0566288 0 272.576 0 0.0600921 0"
           " 276.415 0 0.0612357 0 280.254 0 0.0645958 0 284.093 0 0.0669342 0 287.932 0 0.0649984 0"
           " 291.771 0 0.0651269 0 295.61 0 0.0656374 0 299.449 0 0.0676706 0 303.288 0 0.0661735 0"
           " 307.128 0 0.0694914 0 310.967 0 0.0689067 0 314.806 0 0.0661032 0 318.645 0 0.0635616 0"
           " 322.484 0 0.0613225 0 326.323 0 0.0580835 0 330.162 0 0.0558384 0 334.001 0 0.0555782 0"
           " 337.84 0 0.0554843 0 341.679 0 0.0555765 0 345.518 0 0.0553311 0 349.358 0 0.0557345 0"
           " 353.197 0 0.0537993 0 357.036 0 0.0550078 0 360.875 0 0.056308 0 364.714 0 0.050991 0"
           " 368.553 0 0.0511456 0 372.392 0 0.0513588 0 376.231 0 0.0523539 0 380.07 0 0.0500169 0"
           " 383.909 0 0.0499977 0 387.749 0 0.0497265 0 391.588 0 0.0503976 0 395.427 0 0.0498487 0"
           " 399.266 0 0.0513101 0 403.105 0 0.0503778 0 406.944 0 0.0477146 0 410.783 0 0.0495988 0"
           " 414.622 0 0.0485852 0 418.461 0 0.0490688 0 422.3 0 0.0510948 0 426.14 0 0.05199 0"
           " 429.979 0 0.0527995 0 433.818 0 0.0541391 0 437.657 0 0.0552376 0 441.496 0 0.0562477 0"
           " 445.335 0 0.0559649 0 449.174 0 0.0576771 0 453.013 0 0.0600685 0 456.852 0 0.0620805 0"
           " 460.691 0 0.0800851 0 464.531 0 0.0960101 0 468.37 0 0.103421 0 472.209 0 0.110371 0"
           " 476.048 0 0.110435 0 479.887 0 0.110142 0 483.726 0 0.109888 0 487.565 0 0.109268 0"
           " 491.404 0 0.1065 0 495.243 0 0.106719 0 499.082 0 0.106335 0 502.921 0 0.105394 0"
           " 506.761 0 0.1041 0 510.6 0 0.103863 0 514.439 0 0.102285 0 518.278 0 0.0981924 0"
           " 522.117 0 0.0937689 0 525.956 0 0.0921229 0 529.795 0 0.0947466 0 533.634 0 0.0941222 0"
           " 537.473 0 0.0944024 0 541.312 0 0.100495 0 545.152 0 0.0944395 0 548.991 0 0.0940312 0"
           " 552.83 0 0.0953654 0 556.669 0 0.0955088 0 560.508 0 0.0958314 0 564.347 0 0.0950385 0"
           " 568.186 0 0.0950293 0 572.025 0 0.0949703 0 575.864 0 0.0953085 0 579.703 0 0.0915556 0"
           " 583.543 0 0.0873823 0 587.382 0 0.387012 0 591.221 0 0.398983 0 595.06 0 0.417137 0"
           " 598.899 0 0.440002 0 602.738 0 0.475907 0 606.577 0 0.516201 0 610.416 0 0.565675 0"
           " num_metadata= 0 )";
        ss >> masseter;
    }

    {
        std::stringstream ss;
        ss << "(samples_1D. normalityassumed= 0 num_samples= 160"
           " 0 0 0.0377121 0 3.839 0 0.0458358 0 7.678 0 0.047502 0 11.517 0 0.0458828 0"
           " 15.356 0 0.0484886 0 19.195 0 0.0492785 0 23.034 0 0.0504158 0 26.873 0 0.0588414 0"
           " 30.712 0 0.0988049 0 34.552 0 0.211086 0 38.391 0 0.368419 0 42.23 0 0.550288 0"
           " 46.069 0 0.682907 0 49.908 0 0.775461 0 53.747 0 0.838908 0 57.586 0 0.874994 0"
           " 61.425 0 0.86833 0 65.264 0 0.826871 0 69.103 0 0.758103 0 72.943 0 0.656548 0"
           " 76.782 0 0.52773 0 80.621 0 0.368445 0 84.46 0 0.212967 0 88.299 0 0.118276 0"
           " 92.138 0 0.0859998 0 95.977 0 0.0782172 0 99.816 0 0.0736424 0 103.655 0 0.0741623 0"
           " 107.494 0 0.0711135 0 111.334 0 0.0702698 0 115.173 0 0.0697297 0 119.012 0 0.0695625 0"
           " 122.851 0 0.0688184 0 126.69 0 0.0666013 0 130.529 0 0.0635936 0 134.368 0 0.0607 0"
           " 138.207 0 0.0603108 0 142.046 0 0.0610561 0 145.885 0 0.0636915 0 149.725 0 0.0641999 0"
           " 153.564 0 0.0649581 0 157.403 0 0.0655202 0 161.242 0 0.0630764 0 165.081 0 0.064131 0"
           " 168.92 0 0.0639395 0 172.759 0 0.0665879 0 176.598 0 0.0670355 0 180.437 0 0.0654468 0"
           " 184.276 0 0.0674919 0 188.115 0 0.069213 0 191.955 0 0.0710492 0 195.794 0 0.0736232 0"
           " 199.633 0 0.0774034 0 203.472 0 0.0823709 0 207.311 0 0.0908069 0 211.15 0 0.0909089 0"
           " 214.989 0 0.0921903 0 218.828 0 0.0904022 0 222.667 0 0.0899652 0 226.506 0 0.0897819 0"
           " 230.346 0 0.0880833 0 234.185 0 0.0874559 0 238.024 0 0.0894015 0 241.863 0 0.0910594 0"
           " 245.702 0 0.0889377 0 249.541 0 0.0850975 0 253.38 0 0.0788352 0 257.219 0 0.0774264 0"
           " 261.058 0 0.0744838 0 264.897 0 0.0724689 0 268.737 0 0.0736443 0 272.576 0 0.0798839 0"
           " 276.415 0 0.0840428 0 280.254 0 0.085373 0 284.093 0 0.089174 0 287.932 0 0.0935498 0"
           " 291.771 0 0.0957526 0 295.61 0 0.0954592 0 299.449 0 0.0939907 0 303.288 0 0.0954262 0"
           " 307.128 0 0.0943993 0 310.967 0 0.0906123 0 314.806 0 0.088046 0 318.645 0 0.0865937 0"
           " 322.484 0 0.0832755 0 326.323 0 0.0815563 0 330.162 0 0.080707 0 334.001 0 0.0822692 0"
           " 337.84 0 0.0827854 0 341.679 0 0.0810827 0 345.518 0 0.0792156 0 349.358 0 0.0790914 0"
           " 353.197 0 0.078288 0 357.036 0 0.0760742 0 360.875 0 0.0759455 0 364.714 0 0.0723538 0"
           " 368.553 0 0.0720689 0 372.392 0 0.0709292 0 376.231 0 0.0714941 0 380.07 0 0.070142 0"
           " 383.909 0 0.072446 0 387.749 0 0.0715759 0 391.588 0 0.0741206 0 395.427 0 0.0723848 0"
           " 399.266 0 0.0715992 0 403.105 0 0.0695425 0 406.944 0 0.0692299 0 410.783 0 0.0701988 0"
           " 414.622 0 0.0687831 0 418.461 0 0.0666017 0 422.3 0 0.0667665 0 426.14 0 0.0683621 0"
           " 429.979 0 0.0684988 0 433.818 0 0.0694798 0 437.657 0 0.0707701 0 441.496 0 0.0687432 0"
           " 445.335 0 0.070432 0 449.174 0 0.0690008 0 453.013 0 0.0687072 0 456.852 0 0.0671452 0"
           " 460.691 0 0.0706095 0 464.531 0 0.0740976 0 468.37 0 0.0743658 0 472.209 0 0.0774453 0"
           " 476.048 0 0.0791396 0 479.887 0 0.0845228 0 483.726 0 0.0912632 0 487.565 0 0.0929489 0"
           " 491.404 0 0.0900391 0 495.243 0 0.0875683 0 499.082 0 0.0881047 0 502.921 0 0.0869976 0"
           " 506.761 0 0.0796258 0 510.6 0 0.07343 0 514.439 0 0.068705 0 518.278 0 0.0639894 0"
           " 522.117 0 0.065644 0 525.956 0 0.0679271 0 529.795 0 0.0688172 0 533.634 0 0.0714504 0"
           " 537.473 0 0.0714593 0 541.312 0 0.0687999 0 545.152 0 0.0684519 0 548.991 0 0.0681467 0"
           " 552.83 0 0.0681153 0 556.669 0 0.0706996 0 560.508 0 0.0736164 0 564.347 0 0.0724479 0"
           " 568.186 0 0.0739431 0 572.025 0 0.0742564 0 575.864 0 0.0746973 0 579.703 0 0.0744944 0"
           " 583.543 0 0.0738472 0 587.382 0 0.452857 0 591.221 0 0.479425 0 595.06 0 0.51137 0"
           " 598.899 0 0.547894 0 602.738 0 0.588812 0 606.577 0 0.645241 0 610.416 0 0.706846 0"
           " num_metadata= 0 )";
        ss >> pharynx;
    }

    parotid.Normalize_wrt_Self_Overlap();
    masseter.Normalize_wrt_Self_Overlap();
    pharynx.Normalize_wrt_Self_Overlap();

    // Build paired arrays from the normalized y-values (index [2]).
    auto make_pairs = [](const samples_1D<double> &L, const samples_1D<double> &R)
        -> std::vector<std::array<double,2>> {
        REQUIRE( L.size() == R.size() );
        REQUIRE( L.size() > 0 );
        std::vector<std::array<double,2>> pairs;
        for(size_t i = 0; i < static_cast<size_t>(L.size()); ++i){
            pairs.push_back({ L.samples[i][2], R.samples[i][2] });
        }
        return pairs;
    };

    const auto pm = make_pairs(parotid, masseter);
    const auto pp = make_pairs(parotid, pharynx);
    const auto mp = make_pairs(masseter, pharynx);

    SUBCASE("paired t-test: parotid vs masseter (significant)"){
        // Reference: p ≈ 1.27e-10 (highly significant).
        // DOF override (n-3) matches the original legacy test's explicit adjustment.
        const double dof = static_cast<double>(pm.size() - 1 - 2);
        double p = Stats::P_From_Paired_Ttest_2Tail(pm, dof);
        REQUIRE( p < 1.0e-5 );
    }

    SUBCASE("paired t-test: parotid vs pharynx (significant)"){
        // Reference: p ≈ 1.78e-10 (highly significant).
        // DOF override (n-3) matches the original legacy test's explicit adjustment.
        const double dof = static_cast<double>(pp.size() - 1 - 2);
        double p = Stats::P_From_Paired_Ttest_2Tail(pp, dof);
        REQUIRE( p < 1.0e-5 );
    }

    SUBCASE("paired t-test: masseter vs pharynx (not significant)"){
        // Reference: p ≈ 0.925 (not significant — distributions are similar).
        // DOF override (n-3) matches the original legacy test's explicit adjustment.
        const double dof = static_cast<double>(mp.size() - 1 - 2);
        double p = Stats::P_From_Paired_Ttest_2Tail(mp, dof);
        REQUIRE( p > 0.5 );
    }

    SUBCASE("Wilcoxon signed-rank: parotid vs masseter (significant)"){
        // Reference: p ≈ 8.56e-13.
        double p = Stats::P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(pm);
        REQUIRE( p < 1.0e-5 );
    }

    SUBCASE("Wilcoxon signed-rank: parotid vs pharynx (significant)"){
        // Reference: p ≈ 1.61e-11.
        double p = Stats::P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(pp);
        REQUIRE( p < 1.0e-5 );
    }

    SUBCASE("Wilcoxon signed-rank: masseter vs pharynx (not significant)"){
        // Reference: p ≈ 0.995.
        double p = Stats::P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(mp);
        REQUIRE( p > 0.5 );
    }
}


// ---------------------------------------------------------------------------
// Migrated from tests/Test_Stats_03.cc
// Percentile and Median calculations.
// ---------------------------------------------------------------------------

TEST_CASE( "Percentile_and_Median" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("sorted input [0, 1, 2]"){
        std::vector<double> v = {0.0, 1.0, 2.0};

        // Percentile(0.0) = minimum = 0
        REQUIRE( std::abs(Stats::Percentile(v, 0.00) - 0.0) < eps );
        // Percentile(0.2) = 0.4 (linear interpolation)
        REQUIRE( std::abs(Stats::Percentile(v, 0.20) - 0.4) < eps );
        // Percentile(0.5) = Median = 1.0
        REQUIRE( std::abs(Stats::Percentile(v, 0.50) - 1.0) < eps );
        REQUIRE( std::abs(Stats::Median(v)           - 1.0) < eps );
        // Percentile(0.8) = 1.6
        REQUIRE( std::abs(Stats::Percentile(v, 0.80) - 1.6) < eps );
        // Percentile(1.0) = maximum = 2
        REQUIRE( std::abs(Stats::Percentile(v, 1.00) - 2.0) < eps );
    }

    SUBCASE("unsorted input [1, 0, 2] gives same results as sorted"){
        std::vector<double> v = {1.0, 0.0, 2.0};

        REQUIRE( std::abs(Stats::Percentile(v, 0.00) - 0.0) < eps );
        REQUIRE( std::abs(Stats::Percentile(v, 0.20) - 0.4) < eps );
        REQUIRE( std::abs(Stats::Percentile(v, 0.50) - 1.0) < eps );
        REQUIRE( std::abs(Stats::Median(v)           - 1.0) < eps );
        REQUIRE( std::abs(Stats::Percentile(v, 0.80) - 1.6) < eps );
        REQUIRE( std::abs(Stats::Percentile(v, 1.00) - 2.0) < eps );
    }

    SUBCASE("single element"){
        std::vector<double> v = {42.0};
        REQUIRE( std::abs(Stats::Percentile(v, 0.00) - 42.0) < eps );
        REQUIRE( std::abs(Stats::Percentile(v, 0.50) - 42.0) < eps );
        REQUIRE( std::abs(Stats::Percentile(v, 1.00) - 42.0) < eps );
        REQUIRE( std::abs(Stats::Median(v)           - 42.0) < eps );
    }

    SUBCASE("even number of elements"){
        std::vector<double> v = {1.0, 2.0, 3.0, 4.0};
        // Median of [1,2,3,4] = 2.5 (average of middle two)
        REQUIRE( std::abs(Stats::Median(v) - 2.5) < eps );
        REQUIRE( std::abs(Stats::Percentile(v, 0.00) - 1.0) < eps );
        REQUIRE( std::abs(Stats::Percentile(v, 1.00) - 4.0) < eps );
    }
}


// ---------------------------------------------------------------------------
// Migrated from tests/Test_Stats_04.cc
// Running_Sum compensated summation precision vs naive summation.
// Uses an integer-scaled reference for ground truth.
// ---------------------------------------------------------------------------

TEST_CASE( "Running_Sum" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );
#if defined(__SIZEOF_INT128__)

    SUBCASE("compensated sum is at least as accurate as naive for double"){
        // Sum many random doubles in [0,1]. Compare Running_Sum (Kahan-style
        // compensated sum) and Stats::Sum (sorted compensated) against a
        // high-precision integer-scaled reference.
        int64_t random_seed = 123456;
        std::mt19937 re(random_seed);
        std::uniform_real_distribution<> rd(0.0, 1.0);

        const size_t N = 50000;
        Stats::Running_Sum<double> rs;
        double naive = 0.0;
        const auto int_scale = std::numeric_limits<double>::digits10 + 1;
        const auto scale = std::pow(10.0, static_cast<double>(int_scale));
        __int128_t t = 0;
        std::vector<double> d;
        d.reserve(N);

        for(size_t i = 0; i < N; ++i){
            double x = rd(re);
            naive += x;
            t += static_cast<__int128_t>(x * scale);
            rs.Digest(x);
            d.push_back(x);
        }

        const double cs = rs.Current_Sum();
        const double ss = Stats::Sum(d);
        const double tt = static_cast<double>(t) / scale;

        // All sums should be close to the reference.
        double cs_err = std::abs(cs - tt) / tt;
        double ss_err = std::abs(ss - tt) / tt;
        double naive_err = std::abs(naive - tt) / tt;

        // Compensated sums should be very accurate (relative error < 1e-12).
        REQUIRE( cs_err < 1.0e-12 );
        REQUIRE( ss_err < 1.0e-12 );

        // Compensated sum should be at least as accurate as naive.
        REQUIRE( cs_err <= naive_err + eps );
    }

    SUBCASE("compensated sum is at least as accurate as naive for float"){
        int64_t random_seed = 123456;
        std::mt19937 re(random_seed);
        std::uniform_real_distribution<> rd(0.0, 1.0);

        const size_t N = 50000;
        Stats::Running_Sum<float> rs;
        float naive = 0.0f;
        const auto int_scale = std::numeric_limits<float>::digits10 + 1;
        const auto scale = std::pow(10.0, static_cast<double>(int_scale));
        __int128_t t = 0;
        std::vector<float> d;
        d.reserve(N);

        for(size_t i = 0; i < N; ++i){
            float x = static_cast<float>(rd(re));
            naive += x;
            t += static_cast<__int128_t>(x * scale);
            rs.Digest(x);
            d.push_back(x);
        }

        const float cs = rs.Current_Sum();
        const float ss = Stats::Sum(d);
        const float tt = static_cast<float>(t) / static_cast<float>(scale);

        double cs_err = std::abs(static_cast<double>(cs) - static_cast<double>(tt))
                        / static_cast<double>(tt);
        double ss_err = std::abs(static_cast<double>(ss) - static_cast<double>(tt))
                        / static_cast<double>(tt);
        double naive_err = std::abs(static_cast<double>(naive) - static_cast<double>(tt))
                           / static_cast<double>(tt);

        // For float, compensated sum should still be reasonably accurate.
        REQUIRE( cs_err < 1.0e-4 );
        REQUIRE( ss_err < 1.0e-4 );

        // Compensated should beat or match naive.
        REQUIRE( cs_err <= naive_err + 1.0e-7 );
    }

    SUBCASE("small known sum"){
        Stats::Running_Sum<double> rs;
        std::vector<double> vals = {0.1, 0.2, 0.3, 0.4};
        for(auto v : vals) rs.Digest(v);
        REQUIRE( std::abs(rs.Current_Sum() - 1.0) < eps );
    }
#endif
}


// ---------------------------------------------------------------------------
// Migrated from tests/Test_Stats_05.cc
// Running_Variance with float (single-precision).
// The existing Running_Variance tests use double; this covers float precision.
// ---------------------------------------------------------------------------

TEST_CASE( "Running_Variance_float" ){

    SUBCASE("float precision: 100k uniform samples"){
        // Verify Running_Variance<float> with many uniform [0,1] samples.
        // Expected mean ≈ 0.5, expected population variance ≈ 1/12 ≈ 0.0833.
        int64_t random_seed = 654321;
        std::mt19937 re(random_seed);
        std::uniform_real_distribution<> rd(0.0, 1.0);

        const size_t N = 100000;
        Stats::Running_Variance<float> rv;

        for(size_t i = 0; i < N; ++i){
            float x = static_cast<float>(rd(re));
            rv.Digest(x);
        }

        const float mean = rv.Current_Mean();
        const float variance = rv.Current_Variance();

        // Mean of uniform [0,1] ≈ 0.5 (within 0.01).
        REQUIRE( std::abs(mean - 0.5f) < 0.01f );
        // Population variance of uniform [0,1] = 1/12 ≈ 0.0833 (within 0.005).
        REQUIRE( std::abs(variance - 1.0f/12.0f) < 0.005f );
    }

    SUBCASE("float known data [2,4,4,4,5,5,7,9]"){
        // Same test as double variant but with float.
        Stats::Running_Variance<float> rv;
        std::vector<float> data = {2.0f, 4.0f, 4.0f, 4.0f, 5.0f, 5.0f, 7.0f, 9.0f};
        for(const auto &x : data) rv.Digest(x);

        const float feps = std::sqrt(std::numeric_limits<float>::epsilon());
        REQUIRE( std::abs(rv.Current_Mean() - 5.0f) < feps );
        REQUIRE( std::abs(rv.Current_Variance() - 4.0f) < feps );
        REQUIRE( std::abs(rv.Current_Sample_Variance() - 32.0f/7.0f) < feps );
    }
}
