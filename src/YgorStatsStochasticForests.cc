//YgorStatsStochasticForests.cc - A part of Ygor, 2026. Written by hal clark.

#include <cmath>
#include <cstdint>
#include <vector>
#include <functional>
#include <optional>
#include <chrono>
#include <string>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <numeric>
#include <algorithm>

#include "YgorDefinitions.h"
#include "YgorStatsStochasticForests.h"
#include "YgorLog.h"


template <class T>
Stats::StochasticForests<T>::StochasticForests(int64_t n_trees,
                                     int64_t max_depth,
                                     int64_t min_samples_split,
                                     int64_t max_features,
                                     uint64_t random_seed)
    : n_trees(n_trees),
      max_depth(max_depth),
      min_samples_split(min_samples_split),
      max_features(max_features),
      n_features_trained(-1),
      random_seed(random_seed) {
    
    if(n_trees <= 0){
        throw std::invalid_argument("Number of trees must be positive");
    }
    if(max_depth <= 0){
        throw std::invalid_argument("Maximum depth must be positive");
    }
    if(min_samples_split < 2){
        throw std::invalid_argument("Minimum samples to split must be at least 2");
    }
}
#ifndef YGOR_STATS_STOCHASTIC_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template Stats::StochasticForests<double>::StochasticForests(int64_t, int64_t, int64_t, int64_t, uint64_t);
    template Stats::StochasticForests<float>::StochasticForests(int64_t, int64_t, int64_t, int64_t, uint64_t);
#endif


template <class T>
void Stats::StochasticForests<T>::fit(const num_array<T> &X, const num_array<T> &y) {
    const int64_t n_samples = X.num_rows();
    const int64_t n_features = X.num_cols();
    
    // Validate inputs.
    if(n_samples == 0 || n_features == 0){
        throw std::invalid_argument("Input matrix X cannot be empty");
    }
    if(y.num_rows() != n_samples || y.num_cols() != 1){
        throw std::invalid_argument("Output vector y must be Nx1 where N matches X.num_rows()");
    }
    
    // Store the number of features for validation in predict().
    this->n_features_trained = n_features;
    
    // Determine effective max_features for this fit (don't modify the member variable).
    int64_t effective_max_features = this->max_features;
    if(effective_max_features <= 0){
        effective_max_features = std::max<int64_t>(1, static_cast<int64_t>(std::sqrt(n_features)));
    }
    
    // Ensure max_features doesn't exceed n_features.
    effective_max_features = std::min(effective_max_features, n_features);
    
    // Clear any existing trees.
    this->trees.clear();
    this->trees.reserve(this->n_trees);
    
    // Build each tree with bootstrap sampling.
    std::mt19937_64 rng(this->random_seed);
    std::uniform_int_distribution<int64_t> sample_dist(0, n_samples - 1);
    
    for(int64_t t = 0; t < this->n_trees; ++t){
        // Bootstrap sampling: sample with replacement.
        std::vector<int64_t> bootstrap_indices;
        bootstrap_indices.reserve(n_samples);
        for(int64_t i = 0; i < n_samples; ++i){
            bootstrap_indices.push_back(sample_dist(rng));
        }
        
        // Build a tree using the bootstrap sample.
        auto tree = build_tree(X, y, bootstrap_indices, 0, effective_max_features, rng);
        this->trees.push_back(std::move(tree));
    }
}
#ifndef YGOR_STATS_STOCHASTIC_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template void Stats::StochasticForests<double>::fit(const num_array<double> &, const num_array<double> &);
    template void Stats::StochasticForests<float>::fit(const num_array<float> &, const num_array<float> &);
#endif


template <class T>
std::unique_ptr<typename Stats::StochasticForests<T>::TreeNode>
Stats::StochasticForests<T>::build_tree(
    const num_array<T> &X,
    const num_array<T> &y,
    const std::vector<int64_t> &sample_indices,
    int64_t depth,
    int64_t effective_max_features,
    std::mt19937_64 &rng) {
    
    auto node = std::make_unique<TreeNode>();
    
    // Check stopping criteria.
    const int64_t n_samples = sample_indices.size();
    if(n_samples == 0){
        node->is_leaf = true;
        node->value = static_cast<T>(0);
        return node;
    }
    
    // Compute mean for potential leaf node.
    T sum = static_cast<T>(0);
    for(const auto idx : sample_indices){
        sum += y.read_coeff(idx, 0);
    }
    const T mean = sum / static_cast<T>(n_samples);
    
    // Create leaf if stopping criteria met.
    if(depth >= this->max_depth || n_samples < this->min_samples_split){
        node->is_leaf = true;
        node->value = mean;
        return node;
    }
    
    // Find best split using random feature selection.
    int64_t best_feature;
    T best_threshold;
    T best_score;
    std::vector<int64_t> left_indices, right_indices;
    
    if(!find_best_split(X, y, sample_indices, effective_max_features, 
                       best_feature, best_threshold, best_score,
                       left_indices, right_indices, rng)){
        // Could not find a valid split, create leaf.
        node->is_leaf = true;
        node->value = mean;
        return node;
    }
    
    // If split doesn't actually separate samples, create leaf.
    if(left_indices.empty() || right_indices.empty()){
        node->is_leaf = true;
        node->value = mean;
        return node;
    }
    
    // Create internal node and recursively build children.
    node->is_leaf = false;
    node->split_feature = best_feature;
    node->split_threshold = best_threshold;
    node->left = build_tree(X, y, left_indices, depth + 1, effective_max_features, rng);
    node->right = build_tree(X, y, right_indices, depth + 1, effective_max_features, rng);
    
    return node;
}
#ifndef YGOR_STATS_STOCHASTIC_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template std::unique_ptr<typename Stats::StochasticForests<double>::TreeNode>
        Stats::StochasticForests<double>::build_tree(const num_array<double> &, const num_array<double> &,
                                               const std::vector<int64_t> &, int64_t, int64_t, std::mt19937_64 &);
    template std::unique_ptr<typename Stats::StochasticForests<float>::TreeNode>
        Stats::StochasticForests<float>::build_tree(const num_array<float> &, const num_array<float> &,
                                              const std::vector<int64_t> &, int64_t, int64_t, std::mt19937_64 &);
#endif


template <class T>
bool Stats::StochasticForests<T>::find_best_split(
    const num_array<T> &X,
    const num_array<T> &y,
    const std::vector<int64_t> &sample_indices,
    int64_t effective_max_features,
    int64_t &best_feature,
    T &best_threshold,
    T &best_score,
    std::vector<int64_t> &left_indices,
    std::vector<int64_t> &right_indices,
    std::mt19937_64 &rng) {
    
    const int64_t n_features = X.num_cols();
    
    // Randomly select features to consider (feature randomness).
    std::vector<int64_t> feature_indices;
    for(int64_t f = 0; f < n_features; ++f){
        feature_indices.push_back(f);
    }
    std::shuffle(feature_indices.begin(), feature_indices.end(), rng);
    
    // Only consider effective_max_features random features.
    const int64_t n_features_to_try = std::min(effective_max_features, n_features);
    
    best_score = -std::numeric_limits<T>::infinity();
    bool found_split = false;
    
    // Clear output vectors.
    left_indices.clear();
    right_indices.clear();
    
    // Try each randomly selected feature.
    for(int64_t f_idx = 0; f_idx < n_features_to_try; ++f_idx){
        const int64_t feature = feature_indices[f_idx];
        
        // Collect unique feature values from samples to use as potential thresholds.
        std::vector<T> feature_values;
        feature_values.reserve(sample_indices.size());
        for(const auto idx : sample_indices){
            feature_values.push_back(X.read_coeff(idx, feature));
        }
        
        // Sort to find potential split points.
        std::sort(feature_values.begin(), feature_values.end());
        
        // Try midpoints between consecutive unique values as thresholds.
        for(size_t i = 0; i + 1 < feature_values.size(); ++i){
            // Skip if values are identical (no split point).
            if(feature_values[i] == feature_values[i + 1]){
                continue;
            }
            
            const T threshold = (feature_values[i] + feature_values[i + 1]) / static_cast<T>(2);
            
            // Split samples.
            std::vector<int64_t> temp_left, temp_right;
            for(const auto idx : sample_indices){
                if(X.read_coeff(idx, feature) <= threshold){
                    temp_left.push_back(idx);
                }else{
                    temp_right.push_back(idx);
                }
            }
            
            // Skip if split doesn't separate samples.
            if(temp_left.empty() || temp_right.empty()){
                continue;
            }
            
            // Compute variance reduction.
            const T score = compute_variance_reduction(y, temp_left, temp_right);
            
            if(score > best_score){
                best_score = score;
                best_feature = feature;
                best_threshold = threshold;
                left_indices = temp_left;
                right_indices = temp_right;
                found_split = true;
            }
        }
    }
    
    return found_split;
}
#ifndef YGOR_STATS_STOCHASTIC_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template bool Stats::StochasticForests<double>::find_best_split(const num_array<double> &, const num_array<double> &,
                                                               const std::vector<int64_t> &, int64_t, int64_t &, double &, double &,
                                                               std::vector<int64_t> &, std::vector<int64_t> &,
                                                               std::mt19937_64 &);
    template bool Stats::StochasticForests<float>::find_best_split(const num_array<float> &, const num_array<float> &,
                                                              const std::vector<int64_t> &, int64_t, int64_t &, float &, float &,
                                                              std::vector<int64_t> &, std::vector<int64_t> &,
                                                              std::mt19937_64 &);
#endif


template <class T>
T Stats::StochasticForests<T>::compute_variance_reduction(
    const num_array<T> &y,
    const std::vector<int64_t> &left_indices,
    const std::vector<int64_t> &right_indices) {
    
    const int64_t n_left = left_indices.size();
    const int64_t n_right = right_indices.size();
    const int64_t n_total = n_left + n_right;
    
    if(n_total == 0) return static_cast<T>(0);
    
    // Compute variance for left child using numerically stable two-pass algorithm.
    // First pass: compute mean.
    T left_sum = static_cast<T>(0);
    for(const auto idx : left_indices){
        left_sum += y.read_coeff(idx, 0);
    }
    const T left_mean = left_sum / static_cast<T>(n_left);
    
    // Second pass: compute variance using deviations from mean.
    T left_sum_sq_dev = static_cast<T>(0);
    for(const auto idx : left_indices){
        const T dev = y.read_coeff(idx, 0) - left_mean;
        left_sum_sq_dev += dev * dev;
    }
    const T left_var = left_sum_sq_dev / static_cast<T>(n_left);
    
    // Compute variance for right child using same two-pass algorithm.
    T right_sum = static_cast<T>(0);
    for(const auto idx : right_indices){
        right_sum += y.read_coeff(idx, 0);
    }
    const T right_mean = right_sum / static_cast<T>(n_right);
    
    T right_sum_sq_dev = static_cast<T>(0);
    for(const auto idx : right_indices){
        const T dev = y.read_coeff(idx, 0) - right_mean;
        right_sum_sq_dev += dev * dev;
    }
    const T right_var = right_sum_sq_dev / static_cast<T>(n_right);
    
    // Compute weighted variance of children (lower is better for splits).
    const T weighted_child_var = (static_cast<T>(n_left) * left_var + 
                                  static_cast<T>(n_right) * right_var) / static_cast<T>(n_total);
    
    // Return negative weighted variance so higher scores are better.
    return -weighted_child_var;
}
#ifndef YGOR_STATS_STOCHASTIC_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template double Stats::StochasticForests<double>::compute_variance_reduction(const num_array<double> &,
                                                                           const std::vector<int64_t> &,
                                                                           const std::vector<int64_t> &);
    template float Stats::StochasticForests<float>::compute_variance_reduction(const num_array<float> &,
                                                                         const std::vector<int64_t> &,
                                                                         const std::vector<int64_t> &);
#endif


template <class T>
T Stats::StochasticForests<T>::predict(const num_array<T> &x) const {
    // Validate input.
    if(x.num_rows() != 1){
        throw std::invalid_argument("Input x must be a 1xM matrix (row vector)");
    }
    if(this->trees.empty()){
        throw std::runtime_error("Model has not been fitted yet");
    }
    if(x.num_cols() != this->n_features_trained){
        throw std::invalid_argument("Input features must match training data features");
    }
    
    // Average predictions from all trees (ensemble).
    T sum = static_cast<T>(0);
    for(const auto &tree : this->trees){
        sum += predict_tree(tree.get(), x);
    }
    
    return sum / static_cast<T>(this->trees.size());
}
#ifndef YGOR_STATS_STOCHASTIC_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template double Stats::StochasticForests<double>::predict(const num_array<double> &) const;
    template float Stats::StochasticForests<float>::predict(const num_array<float> &) const;
#endif


template <class T>
T Stats::StochasticForests<T>::predict_tree(const TreeNode *node, const num_array<T> &x) const {
    if(node->is_leaf){
        return node->value;
    }
    
    // Traverse tree based on feature value.
    const T feature_val = x.read_coeff(0, node->split_feature);
    if(feature_val <= node->split_threshold){
        return predict_tree(node->left.get(), x);
    }else{
        return predict_tree(node->right.get(), x);
    }
}
#ifndef YGOR_STATS_STOCHASTIC_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template double Stats::StochasticForests<double>::predict_tree(const TreeNode *, const num_array<double> &) const;
    template float Stats::StochasticForests<float>::predict_tree(const TreeNode *, const num_array<float> &) const;
#endif


template <class T>
int64_t Stats::StochasticForests<T>::get_n_trees() const {
    return this->n_trees;
}
#ifndef YGOR_STATS_STOCHASTIC_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template int64_t Stats::StochasticForests<double>::get_n_trees() const;
    template int64_t Stats::StochasticForests<float>::get_n_trees() const;
#endif


