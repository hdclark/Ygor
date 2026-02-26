//YgorStatsStochasticForests.h - Written by hal clark in 2026.
//
// Implementation of Breiman's 2001 ensemble-of-trees learning algorithm. See https://doi.org/10.1023/A:1010933404324 .
//

#pragma once

#ifndef YGOR_STATS_STOCHASTIC_FORESTS_HDR_GRD_H
#define YGOR_STATS_STOCHASTIC_FORESTS_HDR_GRD_H

#include <array>
#include <cstdint>
#include <iostream>
#include <list>
#include <vector>
#include <random>
#include <memory>

#include "YgorDefinitions.h"
#include "YgorMath.h"

namespace Stats {

// Variable importance estimation method for StochasticForests.
//
// Three options are available:
//
//   none:        No importance estimation (default). No overhead during fitting.
//
//   gini:        Gini variable importance, also known as MDI ("Mean Decrease in Impurity").
//                During tree building, the weighted variance reduction (impurity decrease) at
//                each split is accumulated for each feature. The importances are normalized to
//                sum to 1.0. Higher values indicate more important features. This method tends
//                to favour features with many possible split points. Set the method before calling
//                fit(), then retrieve importances via get_feature_importances() after fitting.
//
//   permutation: Permutation variable importance using out-of-bag (OOB) samples.
//                After fitting, for each tree the OOB samples are identified and a baseline
//                sum of squared residuals (SSR) is computed. Then, for each feature, the feature
//                values are randomly permuted among the OOB samples and the SSR is recomputed.
//                The importance of a feature is the mean increase in SSR across all trees.
//                Larger values indicate more important features. Set the method before calling
//                fit() (to enable OOB index tracking), then call compute_permutation_importance()
//                with the training data, and finally retrieve importances via
//                get_feature_importances().
//
enum class ImportanceMethod : int { none = 0, gini = 1, permutation = 2 };

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
//  - Optional variable importance estimation (Gini or permutation)
//  - Model serialization to and from text streams
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

        ImportanceMethod importance_method; // Variable importance method.
        std::vector<T> feature_importances; // Computed feature importances.
        std::vector<std::vector<int64_t>> oob_indices_per_tree; // OOB sample indices per tree (for permutation).
        std::vector<T> gini_importances_raw; // Raw accumulated Gini impurity decreases per feature.
        
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
        // If the importance method is set to gini, impurity decreases are accumulated during
        // tree building and Gini importances are available via get_feature_importances() after
        // this call returns. If the importance method is set to permutation, out-of-bag sample
        // indices are stored for later use by compute_permutation_importance().
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

        // Set the variable importance estimation method.
        //
        // Must be called before fit(). If not called, defaults to ImportanceMethod::none.
        void set_importance_method(ImportanceMethod method);

        // Get the current variable importance estimation method.
        ImportanceMethod get_importance_method() const;

        // Get computed feature importances.
        //
        // For the gini method, importances are available after fit(). Values are normalized
        // to sum to 1.0 and represent the mean decrease in impurity (variance reduction)
        // attributable to each feature. Higher values indicate more important features.
        //
        // For the permutation method, compute_permutation_importance() must be called after
        // fit() before this method returns valid results. Values represent the mean increase
        // in sum of squared residuals when each feature is randomly permuted among out-of-bag
        // samples. Larger values indicate more important features.
        //
        // Returns an empty vector if the importance method is none or if importances have not
        // been computed yet.
        std::vector<T> get_feature_importances() const;

        // Compute permutation-based variable importance.
        //
        // Only valid when the importance method is set to ImportanceMethod::permutation and
        // the model has been fitted. Uses the mean increase in out-of-bag sum of squared
        // residuals after randomly permuting each feature.
        //
        // Parameters:
        //   X: NxM training data matrix (must match the data used in fit()).
        //   y: Nx1 training output vector (must match the data used in fit()).
        //
        // Throws:
        //   std::runtime_error if importance method is not permutation or model not fitted.
        //   std::invalid_argument if X or y dimensions are invalid.
        void compute_permutation_importance(const num_array<T> &X, const num_array<T> &y);

        // Write the model to a text stream.
        //
        // Serializes all data members, parameters, and tree structures to a human-readable
        // text format. The model can be restored exactly using read_from() without any loss
        // in function or accuracy. Floating point values are written with maximum precision.
        //
        // Parameters:
        //   os: Output stream to write to.
        //
        // Returns:
        //   true on success, false if the stream enters a fail state.
        bool write_to(std::ostream &os) const;

        // Read a model from a text stream.
        //
        // Restores a model previously written by write_to(). All parameters, tree structures,
        // and importance data are restored exactly.
        //
        // Parameters:
        //   is: Input stream to read from.
        //
        // Returns:
        //   true on success, false if the stream format is invalid or enters a fail state.
        bool read_from(std::istream &is);
};

} //namespace Stats.

#endif // YGOR_STATS_STOCHASTIC_FORESTS_HDR_GRD_H
