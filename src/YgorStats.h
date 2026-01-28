//YgorStats.h - Routines which require mostly special function evaluations or perform pure statistical computation
//              on numbers. (*Not* distributions or YgorMath classes!)

#pragma once
#ifndef YGOR_STATS_H_HDR_GRD
#define YGOR_STATS_H_HDR_GRD

#include <array>
#include <cstdint>
#include <list>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorContainers.h"

namespace Stats {


//-----------------------------------------------------------------------------------------------------------
//----------------------------------- Computational Support Routines ----------------------------------------
//-----------------------------------------------------------------------------------------------------------
//Simple 'building block' routines. 
template <class C> typename C::value_type Min(C in);
template <class C> typename C::value_type Max(C in);
template <class C> typename C::value_type Sum(C in);
template <class C> typename C::value_type Sum_Squares(C in);
template <class C> typename C::value_type Percentile(C in, double frac);
template <class C> typename C::value_type Median(C in);
template <class C> typename C::value_type Mean(C in);
template <class C> typename C::value_type Unbiased_Var_Est(C in);

//-----------------------------------------------------------------------------------------------------------
//----------------------------------- Running Accumulators and Tallies --------------------------------------
//-----------------------------------------------------------------------------------------------------------
template <class C>
class Running_MinMax {
    private:
        C PresentMin;
        C PresentMax;

    public:
        Running_MinMax();

        void Digest(C in);

        C Current_Min(void) const;
        C Current_Max(void) const;
};


// Implements Kahan (i.e., compensated) summation. Note that the user should attempt to sum the smallest-magnitude
// inputs first otherwise serious loss of precision may occur.
template <class C>
class Running_Sum {
    private:
        C PresentSum;
        C PresentCompen;

    public:
        Running_Sum();

        void Digest(C in);

        C Current_Sum(void) const;
};


// Implements Welford's algorithm for running variance calculation. Uses compensated summation internally
// to minimize floating-point numerical issues.
template <class C>
class Running_Variance {
    private:
        uint64_t Count;
        Running_Sum<C> Mean;
        Running_Sum<C> M2;  // Sum of squared differences from the current mean.

    public:
        Running_Variance();

        void Digest(C in);

        C Current_Mean(void) const;
        C Current_Variance(void) const;  // Population variance (divide by N).
        C Current_Sample_Variance(void) const;  // Sample variance (divide by N-1).
};


//-----------------------------------------------------------------------------------------------------------
//------------------------------------ Statistical Support Routines -----------------------------------------
//-----------------------------------------------------------------------------------------------------------

//These routines are applicable to any Student's t-test.
double P_From_StudT_1Tail(double tval, double dof);
double P_From_StudT_2Tail(double tval, double dof);

//These routines compute a P-value from any z-score.
double P_From_Zscore_2Tail(double zscore);
double P_From_Zscore_Upper_Tail(double zscore);
double P_From_Zscore_Lower_Tail(double zscore);


//-----------------------------------------------------------------------------------------------------------
//--------------------------------------- Paired Difference Tests -------------------------------------------
//-----------------------------------------------------------------------------------------------------------
//From Wikipedia (http://en.wikipedia.org/wiki/Paired_difference_test):
//  "The most familiar example of a paired difference test occurs when subjects are measured before and 
//  after a treatment. Such a "repeated measures" test compares these measurements within subjects, rather
//  than across subjects, and will generally have greater power than an unpaired test."
//
// These tests are useful for comparing two distributions in which datum measure the same thing.

//Paired t-test for comparing the mean difference between paired datum.
double P_From_Paired_Ttest_2Tail(const std::vector<std::array<double,2>> &paired_datum,
                                 double dof_override = -1.0);

//Paired Wilcoxon signed-rank test for assessing whether population mean ranks differ between the two
// samples. At the moment, P-values are only possible if there are ten or more unpruned points!
//
// NOTE: This is not the same as the "Wilcoxon rank-sum test."
//
double P_From_Paired_Wilcoxon_Signed_Rank_Test_2Tail(const std::vector<std::array<double,2>> &paired_datum);



//"Two-sample unpooled t-test for unequal variances" or "Welch's t-test" for comparing the means
// of two populations where the variance is not necessarily the same.
//
// NOTE: This test examines each distribution separately, instead of the differences between datum.
double P_From_StudT_Diff_Means_From_Uneq_Vars(double M1, double V1, double N1, 
                                              double M2, double V2, double N2);

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------

//P-value for Pearson's r (aka linear correlation coefficient). This is an alternative to the t-test
// which is thought to be more precise.
double P_From_Pearsons_Linear_Correlation_Coeff_1Tail(double corr_coeff, double total_num_of_datum);
double P_From_Pearsons_Linear_Correlation_Coeff_2Tail(double corr_coeff, double total_num_of_datum);

//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------

double Q_From_ChiSq_Fit(double chi_square, double dof);


//-----------------------------------------------------------------------------------------------------------
//--------------------------------------- Z-test for observed mean ------------------------------------------
//-----------------------------------------------------------------------------------------------------------
double Z_From_Observed_Mean(double sample_mean, //Or observed mean.
                            double population_mean, //Or true mean.
                            double population_stddev, //Pop. std. dev. of the distribution (NOT of mean!)
                            double sample_size); //Number of datum used to compute sample_mean.

double Z_From_Observed_Mean(double sample_mean, //Or observed mean.
                            double population_mean, //Or true mean.
                            double stddev_of_pop_mean); //Std. dev. of population's mean == std. err..


} //namespace Stats.

#endif
