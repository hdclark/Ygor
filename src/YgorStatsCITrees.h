//YgorStatsCITrees.h - Written by hal clark in 2026.
//
// Implementation of conditional inference trees based on the algorithm described in:
// Hothorn T, Hornik K, Zeileis A. Unbiased recursive partitioning: A conditional inference
// framework. Journal of Computational and Graphical Statistics. 2006 Sep 1;15(3):651-74.
//
// Uses permutation-based testing for variable selection and variance-minimizing splits.
//

#pragma once

#ifndef YGOR_STATS_CI_TREES_HDR_GRD_H
#define YGOR_STATS_CI_TREES_HDR_GRD_H

#include <cstdint>
#include <vector>
#include <random>
#include <memory>

#include "YgorDefinitions.h"
#include "YgorMath.h"

namespace Stats {

//-----------------------------------------------------------------------------------------------------------
//------------------------------------ Conditional Inference Tree Regressor ---------------------------------
//-----------------------------------------------------------------------------------------------------------
// A conditional inference tree implementation for regression that uses permutation-based
// statistical testing for unbiased variable selection and variance-minimizing splits.
//
// Key features:
//  - Permutation-based testing for variable selection (avoids selection bias)
//  - Statistical significance threshold (alpha) as stopping criterion
//  - Early-exit optimization for clearly non-significant variables
//
template <class T>
class ConditionalInferenceTrees {
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

        std::unique_ptr<TreeNode> root;       // Root of the decision tree.
        int64_t max_depth;                    // Maximum depth of the tree.
        int64_t min_samples_split;            // Minimum samples required to split a node.
        T alpha;                              // Significance threshold for stopping.
        int64_t n_permutations;               // Number of permutations for testing.
        int64_t n_features_trained;           // Number of features the model was trained on.
        uint64_t random_seed;                 // Random seed for reproducibility.

        // Build the conditional inference tree recursively.
        std::unique_ptr<TreeNode> build_tree(
            const num_array<T> &X,
            const num_array<T> &y,
            const std::vector<int64_t> &sample_indices,
            int64_t depth,
            std::mt19937 &rng
        );

        // Select the best variable via permutation testing.
        // Returns true if a significant variable was found, and sets best_feature and best_pvalue.
        bool select_variable(
            const num_array<T> &X,
            const num_array<T> &y,
            const std::vector<int64_t> &sample_indices,
            int64_t &best_feature,
            T &best_pvalue,
            std::mt19937 &rng
        );

        // Compute the association statistic (absolute correlation) between a feature and the response.
        T compute_association(
            const num_array<T> &X,
            const num_array<T> &y,
            const std::vector<int64_t> &sample_indices,
            int64_t feature
        );

        // Find the best split point for a given feature that maximizes discrepancy.
        bool find_best_split_point(
            const num_array<T> &X,
            const num_array<T> &y,
            const std::vector<int64_t> &sample_indices,
            int64_t feature,
            T &best_threshold,
            std::vector<int64_t> &left_indices,
            std::vector<int64_t> &right_indices
        );

        // Compute weighted child variance for a candidate split.
        T compute_weighted_child_variance(
            const num_array<T> &y,
            const std::vector<int64_t> &left_indices,
            const std::vector<int64_t> &right_indices
        );

        // Predict using the tree from a given node.
        T predict_tree(const TreeNode *node, const num_array<T> &x) const;

    public:
        // Constructor.
        //
        // Parameters:
        //   max_depth: Maximum depth of the decision tree (default: 10).
        //   min_samples_split: Minimum number of samples required to split a node (default: 2).
        //   alpha: Significance threshold for the permutation test stopping criterion (default: 0.05).
        //   n_permutations: Number of permutations for the statistical test (default: 1000).
        //   random_seed: Seed for random number generator for reproducibility (default: 42).
        ConditionalInferenceTrees(int64_t max_depth = 10,
                                  int64_t min_samples_split = 2,
                                  T alpha = static_cast<T>(0.05),
                                  int64_t n_permutations = 1000,
                                  uint64_t random_seed = 42);

        // Fit the conditional inference tree model.
        //
        // Trains a single conditional inference tree using permutation-based testing
        // for variable selection. At each node, the variable with the strongest
        // association to the response (lowest p-value) is selected, and the split
        // point is chosen to maximize the discrepancy between child nodes.
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
        // Returns the prediction by traversing the conditional inference tree.
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

        // Get the significance threshold (alpha).
        T get_alpha() const;

        // Get the number of permutations.
        int64_t get_n_permutations() const;
};

} //namespace Stats.

#endif // YGOR_STATS_CI_TREES_HDR_GRD_H
