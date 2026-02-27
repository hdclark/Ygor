//YgorStatsConditionalForests.h - Written by hal clark in 2026.
//
// Implementation of Strobl et al.'s Conditional Random Forest with Conditional Importance.
//
// This ensemble method builds on the conditional inference tree framework described in:
//   Hothorn T, Hornik K, Zeileis A. Unbiased recursive partitioning: A conditional inference
//   framework. Journal of Computational and Graphical Statistics. 2006 Sep 1;15(3):651-74.
//
// Ensemble construction uses subsampling without replacement as described in:
//   Strobl C, Boulesteix AL, Zeileis A, Hothorn T. Bias in random forest variable importance
//   measures: Illustrations, sources and a solution. BMC Bioinformatics. 2007;8(1):25.
//
// Conditional permutation importance is computed as described in:
//   Strobl C, Boulesteix AL, Kneib T, Augustin T, Zeileis A. Conditional variable importance
//   for random forests. BMC Bioinformatics. 2008;9(1):307.
//

#pragma once

#ifndef YGOR_STATS_CONDITIONAL_FORESTS_HDR_GRD_H
#define YGOR_STATS_CONDITIONAL_FORESTS_HDR_GRD_H

#include <cstdint>
#include <vector>
#include <random>
#include <memory>
#include <iostream>

#include "YgorDefinitions.h"
#include "YgorMath.h"

namespace Stats {

// Variable importance estimation method for ConditionalRandomForests.
//
// Three options are available:
//
//   none:        No importance estimation (default). No overhead during fitting.
//
//   permutation: Standard (marginal) permutation variable importance using out-of-bag (OOB) samples.
//                After fitting, for each tree the OOB samples are identified and a baseline
//                sum of squared residuals (SSR) is computed. Then, for each feature, the feature
//                values are randomly permuted among the OOB samples and the SSR is recomputed.
//                The importance of a feature is the mean increase in SSR across all trees.
//                Larger values indicate more important features. Set the method before calling
//                fit() (to enable OOB index tracking), then call compute_importance()
//                with the training data, and finally retrieve importances via
//                get_feature_importances().
//
//   conditional: Conditional permutation importance (Strobl et al. 2008).
//                Instead of globally permuting each feature, the feature is only permuted
//                within groups (grid cells) defined by its correlated predictor variables.
//                This corrects for the bias that marginal permutation importance exhibits
//                when predictor variables are correlated.
//                The algorithm:
//                  1. For each feature Xj, identify conditioning variables Z that have a
//                     Pearson correlation |r| above the correlation_threshold with Xj.
//                  2. Partition the data based on quantile bins of the conditioning variables.
//                  3. Within each cell of the partition, permute Xj values independently.
//                  4. Compute the increase in OOB SSR.
//                Set the method before calling fit(), then call compute_importance().
//
enum class ConditionalImportanceMethod : int { none = 0, permutation = 1, conditional = 2 };

//-----------------------------------------------------------------------------------------------------------
//---------------------------------- Conditional Random Forest Regressor ------------------------------------
//-----------------------------------------------------------------------------------------------------------
// A conditional random forest implementation for regression that combines:
//
//  1. Conditional inference trees (Hothorn et al. 2006) as base learners, using
//     permutation-based statistical testing for unbiased variable selection.
//
//  2. Subsampling without replacement (Strobl et al. 2007) with a default fraction of
//     0.632 * N, which avoids the selection bias that bootstrapping (sampling with
//     replacement) re-introduces toward variables with many categories.
//
//  3. Conditional permutation importance (Strobl et al. 2008) for unbiased variable
//     importance estimation in the presence of correlated predictors.
//
template <class T>
class ConditionalRandomForests {
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
        T alpha;                      // Significance threshold for the permutation test.
        int64_t n_permutations;       // Number of permutations for testing.
        int64_t max_features;         // Number of features to consider for each split (-1 for all).
        T subsample_fraction;         // Fraction of data to subsample (default: 0.632).
        int64_t n_features_trained;   // Number of features the model was trained on.
        uint64_t random_seed;         // Random seed for reproducibility.
        T correlation_threshold;      // Threshold for identifying conditioning variables.

        ConditionalImportanceMethod importance_method; // Variable importance method.
        std::vector<T> feature_importances; // Computed feature importances.
        std::vector<std::vector<int64_t>> oob_indices_per_tree; // OOB sample indices per tree.

        // Build a single conditional inference tree on a subsample.
        std::unique_ptr<TreeNode> build_tree(
            const num_array<T> &X,
            const num_array<T> &y,
            const std::vector<int64_t> &sample_indices,
            int64_t depth,
            std::mt19937 &rng
        );

        // Select the best variable via a permutation-based max-type global test
        // (Hothorn et al., 2006), as in ConditionalInferenceTrees.
        bool select_variable(
            const num_array<T> &X,
            const num_array<T> &y,
            const std::vector<int64_t> &sample_indices,
            int64_t &best_feature,
            T &best_pvalue,
            std::mt19937 &rng
        );

        // Find the best split point for a given feature that minimizes weighted child variance.
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

        // Predict using a single tree.
        T predict_tree(const TreeNode *node, const num_array<T> &x) const;

        // Serialization helpers.
        bool write_tree_node(std::ostream &os, const TreeNode *node) const;
        std::unique_ptr<TreeNode> read_tree_node(std::istream &is);

    public:
        // Constructor.
        //
        // Parameters:
        //   n_trees: Number of decision trees in the forest (default: 100).
        //   max_depth: Maximum depth of each decision tree (default: 10).
        //   min_samples_split: Minimum number of samples required to split a node (default: 2).
        //   alpha: Significance threshold for the permutation test stopping criterion (default: 0.05).
        //   n_permutations: Number of permutations for the statistical test (default: 1000).
        //   max_features: Number of features to consider for each split.
        //                 If <= 0, all features are considered at each split (default: -1).
        //   subsample_fraction: Fraction of training samples to use for each tree (default: 0.632).
        //                       Per Strobl et al. (2007), subsampling without replacement at this
        //                       fraction avoids selection bias toward variables with many categories.
        //   correlation_threshold: Absolute Pearson correlation threshold for identifying
        //                          conditioning variables in conditional importance (default: 0.20).
        //   random_seed: Seed for random number generator for reproducibility (default: 42).
        ConditionalRandomForests(int64_t n_trees = 100,
                                 int64_t max_depth = 10,
                                 int64_t min_samples_split = 2,
                                 T alpha = static_cast<T>(0.05),
                                 int64_t n_permutations = 1000,
                                 int64_t max_features = -1,
                                 T subsample_fraction = static_cast<T>(0.632),
                                 T correlation_threshold = static_cast<T>(0.20),
                                 uint64_t random_seed = 42);

        // Fit the conditional random forest model.
        //
        // Trains the conditional random forest using subsampling without replacement
        // (Strobl et al. 2007) and conditional inference trees (Hothorn et al. 2006).
        // Each tree is trained on a random subsample of size subsample_fraction * N
        // drawn without replacement. At each node, a global max-type permutation test
        // determines the splitting variable.
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
        // Returns the ensemble prediction by averaging predictions from all trees.
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

        // Get the significance threshold (alpha).
        T get_alpha() const;

        // Get the subsample fraction.
        T get_subsample_fraction() const;

        // Get the correlation threshold for conditional importance.
        T get_correlation_threshold() const;

        // Set the variable importance estimation method.
        //
        // Must be called before fit(). If not called, defaults to ConditionalImportanceMethod::none.
        void set_importance_method(ConditionalImportanceMethod method);

        // Get the current variable importance estimation method.
        ConditionalImportanceMethod get_importance_method() const;

        // Get computed feature importances.
        //
        // Returns an empty vector if the importance method is none or if importances have not
        // been computed yet. Call compute_importance() after fit() to populate importances.
        std::vector<T> get_feature_importances() const;

        // Compute variable importance using the configured method.
        //
        // For the permutation method, standard (marginal) OOB permutation importance is computed.
        // For the conditional method, the Strobl et al. (2008) conditional permutation importance
        // algorithm is used, which permutes each feature only within groups defined by correlated
        // conditioning variables.
        //
        // Must be called after fit() with the same training data.
        //
        // Parameters:
        //   X: NxM training data matrix (must match the data used in fit()).
        //   y: Nx1 training output vector (must match the data used in fit()).
        //
        // Throws:
        //   std::runtime_error if importance method is none or model not fitted.
        //   std::invalid_argument if X or y dimensions are invalid.
        void compute_importance(const num_array<T> &X, const num_array<T> &y);

        // Write the model to a text stream.
        //
        // Serializes all data members, parameters, and tree structures to a human-readable
        // text format. The model can be restored exactly using read_from().
        //
        // Parameters:
        //   os: Output stream to write to.
        //
        // Returns:
        //   true on success, false if the stream enters a fail state.
        bool write_to(std::ostream &os) const;

        // Read a model from a text stream.
        //
        // Restores a model previously written by write_to().
        //
        // Parameters:
        //   is: Input stream to read from.
        //
        // Returns:
        //   true on success, false if the stream format is invalid or enters a fail state.
        bool read_from(std::istream &is);
};

} //namespace Stats.

#endif // YGOR_STATS_CONDITIONAL_FORESTS_HDR_GRD_H
