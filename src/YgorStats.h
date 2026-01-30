//YgorStats.h - Routines which require mostly special function evaluations or perform pure statistical computation
//              on numbers. (*Not* distributions or YgorMath classes!)

#pragma once
#ifndef YGOR_STATS_H_HDR_GRD
#define YGOR_STATS_H_HDR_GRD

#include <array>
#include <cstdint>
#include <list>
#include <vector>
#include <random>
#include <memory>

#include "YgorDefinitions.h"
#include "YgorContainers.h"
#include "YgorMath.h"

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


//-----------------------------------------------------------------------------------------------------------
//----------------------------------------- Random Forest Regressor -----------------------------------------
//-----------------------------------------------------------------------------------------------------------
// A random forest implementation for regression that uses bootstrap aggregation (bagging) and
// feature randomness. The model is trained on an Nx1 output matrix (column vector) and an NxM
// matrix of independent variables, and can predict scalar outputs from 1xM input vectors.
//
// Key features:
//  - Bootstrap sampling (bagging) for training each tree
//  - Random feature selection at each split
//  - Ensemble averaging for predictions
//
template <class T>
class RandomForest {
    private:
        // Internal decision tree node structure.
        struct TreeNode {
            bool is_leaf;
            T value;                    // Prediction value (for leaf nodes).
            int64_t split_feature;      // Feature index to split on (for internal nodes).
            T split_threshold;          // Threshold value for split (for internal nodes).
            std::unique_ptr<TreeNode> left;   // Left child (feature <= threshold).
            std::unique_ptr<TreeNode> right;  // Right child (feature > threshold).
            
            TreeNode() : is_leaf(false), value(static_cast<T>(0)), 
                        split_feature(-1), split_threshold(static_cast<T>(0)) {}
        };

        std::vector<std::unique_ptr<TreeNode>> trees;  // Ensemble of decision trees.
        int64_t n_trees;              // Number of trees in the forest.
        int64_t max_depth;            // Maximum depth of each tree.
        int64_t min_samples_split;    // Minimum samples required to split a node.
        int64_t max_features;         // Number of features to consider for each split.
        int64_t n_features_trained;   // Number of features the model was trained on (for validation).
        uint64_t random_seed;         // Random seed for reproducibility.
        
        // Build a single decision tree using bootstrap sampling.
        std::unique_ptr<TreeNode> build_tree(
            const num_array<T> &X,
            const num_array<T> &y,
            const std::vector<int64_t> &sample_indices,
            int64_t depth,
            int64_t effective_max_features,
            std::mt19937_64 &rng
        );
        
        // Find the best split for a node using random feature selection.
        // Returns the split indices along with the best split parameters.
        bool find_best_split(
            const num_array<T> &X,
            const num_array<T> &y,
            const std::vector<int64_t> &sample_indices,
            int64_t effective_max_features,
            int64_t &best_feature,
            T &best_threshold,
            T &best_score,
            std::vector<int64_t> &left_indices,
            std::vector<int64_t> &right_indices,
            std::mt19937_64 &rng
        );
        
        // Compute variance reduction (impurity) for regression.
        T compute_variance_reduction(
            const num_array<T> &y,
            const std::vector<int64_t> &left_indices,
            const std::vector<int64_t> &right_indices
        );
        
        // Predict using a single tree.
        T predict_tree(const TreeNode *node, const num_array<T> &x) const;

    public:
        // Constructor.
        // 
        // Parameters:
        //   n_trees: Number of decision trees in the forest (default: 100).
        //   max_depth: Maximum depth of each decision tree (default: 10).
        //   min_samples_split: Minimum number of samples required to split a node (default: 2).
        //   max_features: Number of features to consider when looking for the best split.
        //                 If <= 0, uses sqrt(n_features) automatically (default: -1).
        //                 If > n_features, will be capped at n_features during fit().
        //   random_seed: Seed for random number generator for reproducibility (default: 42).
        RandomForest(int64_t n_trees = 100,
                    int64_t max_depth = 10,
                    int64_t min_samples_split = 2,
                    int64_t max_features = -1,
                    uint64_t random_seed = 42);

        // Fit the random forest model.
        // 
        // Trains the random forest using bootstrap aggregation (bagging) and random feature
        // selection. Each tree is trained on a bootstrap sample (sampling with replacement)
        // of the training data, and at each split, only a random subset of features is
        // considered (feature randomness).
        //
        // Parameters:
        //   X: NxM matrix of independent variables (N samples, M features).
        //   y: Nx1 matrix (column vector) of scalar outputs.
        //
        // Throws:
        //   std::invalid_argument if X is empty or if y dimensions don't match X.
        void fit(const num_array<T> &X, const num_array<T> &y);

        // Predict a scalar output from input features.
        // 
        // Returns the ensemble prediction by averaging predictions from all trees in the forest.
        //
        // Parameters:
        //   x: 1xM matrix of input features (M must match the number of features used in fit()).
        //
        // Returns:
        //   Predicted scalar value.
        //
        // Throws:
        //   std::invalid_argument if x is not a row vector or has wrong number of features.
        //   std::runtime_error if model has not been fitted yet.
        T predict(const num_array<T> &x) const;
        
        // Get number of trees in the forest.
        int64_t get_n_trees() const;
};


} //namespace Stats.

#endif
