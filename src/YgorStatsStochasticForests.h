//YgorStatsStochasticForests.h - Written by hal clark in 2026.
//
// Implementation of Breiman's 2001 ensemble-of-trees learning algorithm. See https://doi.org/10.1023/A:1010933404324 .
//

#pragma once

#ifndef YGOR_STATS_STOCHASTIC_FORESTS_HDR_GRD_H
#define YGOR_STATS_STOCHASTIC_FORESTS_HDR_GRD_H

#include <array>
#include <cstdint>
#include <list>
#include <vector>
#include <random>
#include <memory>

#include "YgorDefinitions.h"
#include "YgorMath.h"

namespace Stats {

//-----------------------------------------------------------------------------------------------------------
//--------------------------------------- Stochastic Forest Regressor ---------------------------------------
//-----------------------------------------------------------------------------------------------------------
// A stochastic forests implementation for regression that uses bootstrap aggregation (bagging) and
// feature randomness. The model is trained on an Nx1 output matrix (column vector) and an NxM
// matrix of independent variables, and can predict scalar outputs from 1xM input vectors.
//
// Key features:
//  - Bootstrap sampling (bagging) for training each tree
//  - Random feature selection at each split
//  - Ensemble averaging for predictions
//
template <class T>
class StochasticForests {
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
        StochasticForests(int64_t n_trees = 100,
                    int64_t max_depth = 10,
                    int64_t min_samples_split = 2,
                    int64_t max_features = -1,
                    uint64_t random_seed = 42);

        // Fit the stochastic forest model.
        // 
        // Trains the stochastic forest using bootstrap aggregation (bagging) and random feature
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

#endif // YGOR_STATS_STOCHASTIC_FORESTS_HDR_GRD_H
