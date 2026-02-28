//YgorStatsConditionalForests.cc - A part of Ygor, 2026. Written by hal clark.

#include <cmath>
#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <numeric>
#include <algorithm>

#include "YgorDefinitions.h"
#include "YgorStatsConditionalForests.h"
#include "YgorLog.h"


template <class T>
Stats::ConditionalRandomForests<T>::ConditionalRandomForests(int64_t n_trees,
                                                              int64_t max_depth,
                                                              int64_t min_samples_split,
                                                              T alpha,
                                                              int64_t n_permutations,
                                                              int64_t max_features,
                                                              T subsample_fraction,
                                                              T correlation_threshold,
                                                              uint64_t random_seed)
    : n_trees(n_trees),
      max_depth(max_depth),
      min_samples_split(min_samples_split),
      alpha(alpha),
      n_permutations(n_permutations),
      max_features(max_features),
      subsample_fraction(subsample_fraction),
      n_features_trained(-1),
      random_seed(random_seed),
      correlation_threshold(correlation_threshold),
      importance_method(ConditionalImportanceMethod::none) {

    if(n_trees <= 0){
        throw std::invalid_argument("Number of trees must be positive");
    }
    if(max_depth <= 0){
        throw std::invalid_argument("Maximum depth must be positive");
    }
    if(min_samples_split < 2){
        throw std::invalid_argument("Minimum samples to split must be at least 2");
    }
    if(alpha <= static_cast<T>(0) || static_cast<T>(1) <= alpha){
        throw std::invalid_argument("Alpha must be in the range (0, 1)");
    }
    if(n_permutations <= 0){
        throw std::invalid_argument("Number of permutations must be positive");
    }
    if(subsample_fraction <= static_cast<T>(0) || static_cast<T>(1) < subsample_fraction){
        throw std::invalid_argument("Subsample fraction must be in the range (0, 1]");
    }
    if(correlation_threshold < static_cast<T>(0) || static_cast<T>(1) < correlation_threshold){
        throw std::invalid_argument("Correlation threshold must be in the range [0, 1]");
    }
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template Stats::ConditionalRandomForests<double>::ConditionalRandomForests(int64_t, int64_t, int64_t, double, int64_t, int64_t, double, double, uint64_t);
    template Stats::ConditionalRandomForests<float>::ConditionalRandomForests(int64_t, int64_t, int64_t, float, int64_t, int64_t, float, float, uint64_t);
#endif


template <class T>
void Stats::ConditionalRandomForests<T>::fit(const num_array<T> &X, const num_array<T> &y) {
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

    // Clear any existing trees and importance data.
    this->trees.clear();
    this->trees.reserve(this->n_trees);
    this->feature_importances.clear();
    this->oob_indices_per_tree.clear();

    if(this->importance_method != ConditionalImportanceMethod::none){
        this->oob_indices_per_tree.reserve(this->n_trees);
    }

    // Compute the subsample size per Strobl et al. (2007):
    // Use subsampling without replacement at subsample_fraction * N.
    const int64_t subsample_size = std::max<int64_t>(1,
        static_cast<int64_t>(std::round(this->subsample_fraction * static_cast<T>(n_samples))));

    // Create the pool of all sample indices.
    std::vector<int64_t> all_indices(n_samples);
    std::iota(all_indices.begin(), all_indices.end(), 0);

    // Build each tree with subsampling without replacement.
    std::mt19937 rng(this->random_seed);

    for(int64_t t = 0; t < this->n_trees; ++t){
        // Subsampling without replacement (Strobl et al. 2007).
        // Shuffle the full index pool and take the first subsample_size indices.
        std::shuffle(all_indices.begin(), all_indices.end(), rng);

        std::vector<int64_t> subsample_indices(all_indices.begin(),
                                                all_indices.begin() + subsample_size);

        // Track OOB indices for importance computation.
        if(this->importance_method != ConditionalImportanceMethod::none){
            std::vector<bool> in_sample(n_samples, false);
            for(const auto idx : subsample_indices){
                in_sample[idx] = true;
            }
            std::vector<int64_t> oob;
            for(int64_t i = 0; i < n_samples; ++i){
                if(!in_sample[i]){
                    oob.push_back(i);
                }
            }
            this->oob_indices_per_tree.push_back(std::move(oob));
        }

        // Build a conditional inference tree on the subsample.
        auto tree = build_tree(X, y, subsample_indices, 0, rng);
        this->trees.push_back(std::move(tree));
    }
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template void Stats::ConditionalRandomForests<double>::fit(const num_array<double> &, const num_array<double> &);
    template void Stats::ConditionalRandomForests<float>::fit(const num_array<float> &, const num_array<float> &);
#endif


template <class T>
std::unique_ptr<typename Stats::ConditionalRandomForests<T>::TreeNode>
Stats::ConditionalRandomForests<T>::build_tree(
    const num_array<T> &X,
    const num_array<T> &y,
    const std::vector<int64_t> &sample_indices,
    int64_t depth,
    std::mt19937 &rng) {

    auto node = std::make_unique<TreeNode>();

    const int64_t n_samples = static_cast<int64_t>(sample_indices.size());
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

    // Create leaf if stopping criteria met (depth or min_samples).
    if(depth >= this->max_depth || n_samples < this->min_samples_split){
        node->is_leaf = true;
        node->value = mean;
        return node;
    }

    // Variable selection via permutation testing (Hothorn et al. 2006).
    int64_t best_feature = -1;
    T best_pvalue = static_cast<T>(1);

    if(!select_variable(X, y, sample_indices, best_feature, best_pvalue, rng)){
        node->is_leaf = true;
        node->value = mean;
        return node;
    }

    // If the minimum p-value is not below alpha, create a leaf.
    if(best_pvalue >= this->alpha){
        node->is_leaf = true;
        node->value = mean;
        return node;
    }

    // Find the best split point on the selected variable.
    T best_threshold = static_cast<T>(0);
    std::vector<int64_t> left_indices, right_indices;

    if(!find_best_split_point(X, y, sample_indices, best_feature,
                              best_threshold, left_indices, right_indices)){
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
    node->left = build_tree(X, y, left_indices, depth + 1, rng);
    node->right = build_tree(X, y, right_indices, depth + 1, rng);

    return node;
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template std::unique_ptr<typename Stats::ConditionalRandomForests<double>::TreeNode>
        Stats::ConditionalRandomForests<double>::build_tree(const num_array<double> &, const num_array<double> &,
                                                             const std::vector<int64_t> &, int64_t, std::mt19937 &);
    template std::unique_ptr<typename Stats::ConditionalRandomForests<float>::TreeNode>
        Stats::ConditionalRandomForests<float>::build_tree(const num_array<float> &, const num_array<float> &,
                                                            const std::vector<int64_t> &, int64_t, std::mt19937 &);
#endif


template <class T>
bool Stats::ConditionalRandomForests<T>::select_variable(
    const num_array<T> &X,
    const num_array<T> &y,
    const std::vector<int64_t> &sample_indices,
    int64_t &best_feature,
    T &best_pvalue,
    std::mt19937 &rng) {
    //
    // Variable selection via a permutation-based max-type global test, following
    // the conditional inference framework described in:
    //   Hothorn T, Hornik K, Zeileis A. Unbiased recursive partitioning: A conditional
    //   inference framework. Journal of Computational and Graphical Statistics.
    //   2006 Sep 1;15(3):651-74.
    //
    // This is the same method used in ConditionalInferenceTrees. When max_features > 0,
    // only a random subset of features is evaluated (for computational efficiency in
    // large forests), but the permutation-based testing is still used for variable selection.
    //

    const int64_t n_features = X.num_cols();
    const int64_t n_samples = static_cast<int64_t>(sample_indices.size());

    if(n_samples < 2){
        return false;
    }

    best_pvalue = static_cast<T>(1);
    best_feature = -1;

    // Determine which features to evaluate.
    std::vector<int64_t> candidate_features;
    if(this->max_features > 0 && this->max_features < n_features){
        // Random feature subset.
        std::vector<int64_t> all_features(n_features);
        std::iota(all_features.begin(), all_features.end(), 0);
        std::shuffle(all_features.begin(), all_features.end(), rng);
        candidate_features.assign(all_features.begin(),
                                   all_features.begin() + this->max_features);
    }else{
        // Use all features.
        candidate_features.resize(n_features);
        std::iota(candidate_features.begin(), candidate_features.end(), 0);
    }

    const int64_t n_candidates = static_cast<int64_t>(candidate_features.size());

    // Pre-extract response values for efficient shuffling.
    std::vector<T> y_values(n_samples);
    for(int64_t i = 0; i < n_samples; ++i){
        y_values[i] = y.read_coeff(sample_indices[i], 0);
    }

    // Precompute y mean.
    T y_sum = static_cast<T>(0);
    for(int64_t i = 0; i < n_samples; ++i){
        y_sum += y_values[i];
    }
    const T y_mean = y_sum / static_cast<T>(n_samples);

    // Precompute y deviations from mean and sum of squared deviations.
    std::vector<T> y_dev(n_samples);
    T y_ss = static_cast<T>(0);
    for(int64_t i = 0; i < n_samples; ++i){
        y_dev[i] = y_values[i] - y_mean;
        y_ss += y_dev[i] * y_dev[i];
    }

    // If response has zero variance, no variable can be significant.
    if(y_ss <= static_cast<T>(0)){
        return false;
    }

    // Pre-extract feature values and precompute standardized statistics.
    std::vector<std::vector<T>> x_dev(n_candidates);
    std::vector<T> x_ss(n_candidates, static_cast<T>(0));
    std::vector<T> inv_sqrt_x_ss(n_candidates, static_cast<T>(0));
    std::vector<T> observed_stat(n_candidates, static_cast<T>(0));
    std::vector<bool> feature_valid(n_candidates, false);

    T obs_max = static_cast<T>(0);
    int64_t best_candidate_idx = -1;

    for(int64_t c = 0; c < n_candidates; ++c){
        const int64_t j = candidate_features[c];
        x_dev[c].resize(n_samples);

        // Compute feature mean.
        T x_sum = static_cast<T>(0);
        for(int64_t i = 0; i < n_samples; ++i){
            x_sum += X.read_coeff(sample_indices[i], j);
        }
        const T x_mean = x_sum / static_cast<T>(n_samples);

        // Compute feature deviations and sum of squared deviations.
        T xy_sum = static_cast<T>(0);
        for(int64_t i = 0; i < n_samples; ++i){
            x_dev[c][i] = X.read_coeff(sample_indices[i], j) - x_mean;
            x_ss[c] += x_dev[c][i] * x_dev[c][i];
            xy_sum += x_dev[c][i] * y_dev[i];
        }

        // Skip features with zero variance.
        if(x_ss[c] <= static_cast<T>(0)){
            continue;
        }

        feature_valid[c] = true;
        inv_sqrt_x_ss[c] = static_cast<T>(1) / std::sqrt(x_ss[c]);

        // Standardized observed statistic.
        observed_stat[c] = std::abs(xy_sum) * inv_sqrt_x_ss[c];

        if(observed_stat[c] > obs_max){
            obs_max = observed_stat[c];
            best_candidate_idx = c;
        }
    }

    // If no valid feature was found, return false.
    if(best_candidate_idx < 0){
        return false;
    }

    best_feature = candidate_features[best_candidate_idx];

    // Permutation test using shared permutations for all candidate features.
    std::vector<T> y_dev_perm(y_dev);
    int64_t count_ge = 0;

    const int64_t early_check = std::min(static_cast<int64_t>(100), this->n_permutations);

    for(int64_t k = 0; k < this->n_permutations; ++k){
        std::shuffle(y_dev_perm.begin(), y_dev_perm.end(), rng);

        T perm_max = static_cast<T>(0);
        for(int64_t c = 0; c < n_candidates; ++c){
            if(!feature_valid[c]) continue;

            T perm_stat = static_cast<T>(0);
            for(int64_t i = 0; i < n_samples; ++i){
                perm_stat += x_dev[c][i] * y_dev_perm[i];
            }
            perm_stat = std::abs(perm_stat) * inv_sqrt_x_ss[c];

            if(perm_stat > perm_max){
                perm_max = perm_stat;
            }
        }

        if(perm_max >= obs_max){
            ++count_ge;
        }

        // Early exit for clearly non-significant tests.
        if(k + 1 == early_check && early_check < this->n_permutations){
            const T early_pvalue = static_cast<T>(count_ge + 1) / static_cast<T>(early_check + 1);
            if(early_pvalue > static_cast<T>(2) * this->alpha){
                best_pvalue = early_pvalue;
                return true;
            }
        }
    }

    // Global p-value: (b+1)/(B+1) per Davison & Hinkley (1997).
    best_pvalue = static_cast<T>(count_ge + 1) / static_cast<T>(this->n_permutations + 1);

    return true;
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template bool Stats::ConditionalRandomForests<double>::select_variable(
        const num_array<double> &, const num_array<double> &,
        const std::vector<int64_t> &, int64_t &, double &, std::mt19937 &);
    template bool Stats::ConditionalRandomForests<float>::select_variable(
        const num_array<float> &, const num_array<float> &,
        const std::vector<int64_t> &, int64_t &, float &, std::mt19937 &);
#endif


template <class T>
bool Stats::ConditionalRandomForests<T>::find_best_split_point(
    const num_array<T> &X,
    const num_array<T> &y,
    const std::vector<int64_t> &sample_indices,
    int64_t feature,
    T &best_threshold,
    std::vector<int64_t> &left_indices,
    std::vector<int64_t> &right_indices) {

    // Collect feature values and sort.
    std::vector<T> feature_values;
    feature_values.reserve(sample_indices.size());
    for(const auto idx : sample_indices){
        feature_values.push_back(X.read_coeff(idx, feature));
    }
    std::sort(feature_values.begin(), feature_values.end());

    T best_variance = std::numeric_limits<T>::infinity();
    bool found_split = false;

    left_indices.clear();
    right_indices.clear();

    // Try midpoints between consecutive unique values as thresholds.
    for(size_t i = 0; i + 1 < feature_values.size(); ++i){
        if(feature_values[i] == feature_values[i + 1]){
            continue;
        }

        const T threshold = (feature_values[i] + feature_values[i + 1]) / static_cast<T>(2);

        std::vector<int64_t> temp_left, temp_right;
        for(const auto idx : sample_indices){
            if(X.read_coeff(idx, feature) <= threshold){
                temp_left.push_back(idx);
            }else{
                temp_right.push_back(idx);
            }
        }

        if(temp_left.empty() || temp_right.empty()){
            continue;
        }

        const T variance = compute_weighted_child_variance(y, temp_left, temp_right);

        if(variance < best_variance){
            best_variance = variance;
            best_threshold = threshold;
            left_indices = temp_left;
            right_indices = temp_right;
            found_split = true;
        }
    }

    return found_split;
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template bool Stats::ConditionalRandomForests<double>::find_best_split_point(
        const num_array<double> &, const num_array<double> &,
        const std::vector<int64_t> &, int64_t, double &,
        std::vector<int64_t> &, std::vector<int64_t> &);
    template bool Stats::ConditionalRandomForests<float>::find_best_split_point(
        const num_array<float> &, const num_array<float> &,
        const std::vector<int64_t> &, int64_t, float &,
        std::vector<int64_t> &, std::vector<int64_t> &);
#endif


template <class T>
T Stats::ConditionalRandomForests<T>::compute_weighted_child_variance(
    const num_array<T> &y,
    const std::vector<int64_t> &left_indices,
    const std::vector<int64_t> &right_indices) {

    const int64_t n_left = static_cast<int64_t>(left_indices.size());
    const int64_t n_right = static_cast<int64_t>(right_indices.size());
    const int64_t n_total = n_left + n_right;

    if(n_total == 0) return static_cast<T>(0);

    // Compute variance for left child.
    T left_sum = static_cast<T>(0);
    for(const auto idx : left_indices){
        left_sum += y.read_coeff(idx, 0);
    }
    const T left_mean = left_sum / static_cast<T>(n_left);

    T left_sum_sq_dev = static_cast<T>(0);
    for(const auto idx : left_indices){
        const T dev = y.read_coeff(idx, 0) - left_mean;
        left_sum_sq_dev += dev * dev;
    }
    const T left_var = left_sum_sq_dev / static_cast<T>(n_left);

    // Compute variance for right child.
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

    return (static_cast<T>(n_left) * left_var +
            static_cast<T>(n_right) * right_var) / static_cast<T>(n_total);
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template double Stats::ConditionalRandomForests<double>::compute_weighted_child_variance(
        const num_array<double> &, const std::vector<int64_t> &, const std::vector<int64_t> &);
    template float Stats::ConditionalRandomForests<float>::compute_weighted_child_variance(
        const num_array<float> &, const std::vector<int64_t> &, const std::vector<int64_t> &);
#endif


template <class T>
T Stats::ConditionalRandomForests<T>::predict(const num_array<T> &x) const {
    if(x.num_rows() != 1){
        throw std::invalid_argument("Input x must be a 1xM matrix (row vector)");
    }
    if(this->trees.empty()){
        throw std::runtime_error("Model has not been fitted yet");
    }
    if(x.num_cols() != this->n_features_trained){
        throw std::invalid_argument("Input features must match training data features");
    }

    T sum = static_cast<T>(0);
    for(const auto &tree : this->trees){
        sum += predict_tree(tree.get(), x);
    }

    return sum / static_cast<T>(this->trees.size());
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template double Stats::ConditionalRandomForests<double>::predict(const num_array<double> &) const;
    template float Stats::ConditionalRandomForests<float>::predict(const num_array<float> &) const;
#endif


template <class T>
T Stats::ConditionalRandomForests<T>::predict_tree(const TreeNode *node, const num_array<T> &x) const {
    if(node->is_leaf){
        return node->value;
    }

    const T feature_val = x.read_coeff(0, node->split_feature);
    if(feature_val <= node->split_threshold){
        return predict_tree(node->left.get(), x);
    }else{
        return predict_tree(node->right.get(), x);
    }
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template double Stats::ConditionalRandomForests<double>::predict_tree(const TreeNode *, const num_array<double> &) const;
    template float Stats::ConditionalRandomForests<float>::predict_tree(const TreeNode *, const num_array<float> &) const;
#endif


template <class T>
int64_t Stats::ConditionalRandomForests<T>::get_n_trees() const {
    return this->n_trees;
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template int64_t Stats::ConditionalRandomForests<double>::get_n_trees() const;
    template int64_t Stats::ConditionalRandomForests<float>::get_n_trees() const;
#endif


template <class T>
T Stats::ConditionalRandomForests<T>::get_alpha() const {
    return this->alpha;
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template double Stats::ConditionalRandomForests<double>::get_alpha() const;
    template float Stats::ConditionalRandomForests<float>::get_alpha() const;
#endif


template <class T>
T Stats::ConditionalRandomForests<T>::get_subsample_fraction() const {
    return this->subsample_fraction;
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template double Stats::ConditionalRandomForests<double>::get_subsample_fraction() const;
    template float Stats::ConditionalRandomForests<float>::get_subsample_fraction() const;
#endif


template <class T>
T Stats::ConditionalRandomForests<T>::get_correlation_threshold() const {
    return this->correlation_threshold;
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template double Stats::ConditionalRandomForests<double>::get_correlation_threshold() const;
    template float Stats::ConditionalRandomForests<float>::get_correlation_threshold() const;
#endif


template <class T>
void Stats::ConditionalRandomForests<T>::set_importance_method(Stats::ConditionalImportanceMethod method) {
    if(this->n_features_trained != -1){
        throw std::logic_error("set_importance_method() must be called before fit(); "
                               "changing importance_method after training may lead to inconsistent state.");
    }
    this->importance_method = method;
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template void Stats::ConditionalRandomForests<double>::set_importance_method(Stats::ConditionalImportanceMethod);
    template void Stats::ConditionalRandomForests<float>::set_importance_method(Stats::ConditionalImportanceMethod);
#endif


template <class T>
Stats::ConditionalImportanceMethod Stats::ConditionalRandomForests<T>::get_importance_method() const {
    return this->importance_method;
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template Stats::ConditionalImportanceMethod Stats::ConditionalRandomForests<double>::get_importance_method() const;
    template Stats::ConditionalImportanceMethod Stats::ConditionalRandomForests<float>::get_importance_method() const;
#endif


template <class T>
std::vector<T> Stats::ConditionalRandomForests<T>::get_feature_importances() const {
    return this->feature_importances;
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template std::vector<double> Stats::ConditionalRandomForests<double>::get_feature_importances() const;
    template std::vector<float> Stats::ConditionalRandomForests<float>::get_feature_importances() const;
#endif


template <class T>
void Stats::ConditionalRandomForests<T>::compute_importance(
    const num_array<T> &X, const num_array<T> &y) {

    if(this->importance_method == ConditionalImportanceMethod::none){
        throw std::runtime_error("Importance method must be set to permutation or conditional before calling compute_importance");
    }
    if(this->trees.empty()){
        throw std::runtime_error("Model has not been fitted yet");
    }

    const int64_t n_samples = X.num_rows();
    const int64_t n_features = X.num_cols();

    if(n_samples == 0 || n_features == 0){
        throw std::invalid_argument("Input matrix X cannot be empty");
    }
    if(y.num_rows() != n_samples || y.num_cols() != 1){
        throw std::invalid_argument("Output vector y must be Nx1 where N matches X.num_rows()");
    }
    if(n_features != this->n_features_trained){
        throw std::invalid_argument("Input features must match training data features");
    }
    if(this->oob_indices_per_tree.size() != static_cast<size_t>(this->n_trees)){
        throw std::runtime_error("OOB indices not available; model may not have been fitted with importance enabled");
    }

    this->feature_importances.assign(n_features, static_cast<T>(0));

    std::mt19937 rng(this->random_seed + 1);
    int64_t n_trees_with_oob = 0;

    // Pre-extract feature columns for correlation computation (conditional method).
    // Also precompute the feature correlation matrix for the conditional method.
    // Per Strobl et al. (2008), the conditioning set Z for feature Xj consists
    // of all other features whose absolute Pearson correlation with Xj exceeds
    // the correlation_threshold.
    std::vector<std::vector<int64_t>> conditioning_sets;
    if(this->importance_method == ConditionalImportanceMethod::conditional){
        // Precompute feature means and standard deviations.
        std::vector<T> feat_mean(n_features, static_cast<T>(0));
        std::vector<T> feat_sd(n_features, static_cast<T>(0));
        for(int64_t f = 0; f < n_features; ++f){
            T fsum = static_cast<T>(0);
            for(int64_t i = 0; i < n_samples; ++i){
                fsum += X.read_coeff(i, f);
            }
            feat_mean[f] = fsum / static_cast<T>(n_samples);

            T sq_dev_sum = static_cast<T>(0);
            for(int64_t i = 0; i < n_samples; ++i){
                const T dev = X.read_coeff(i, f) - feat_mean[f];
                sq_dev_sum += dev * dev;
            }
            feat_sd[f] = std::sqrt(sq_dev_sum / static_cast<T>(n_samples));
        }

        // Compute pairwise absolute Pearson correlations and build conditioning sets.
        conditioning_sets.resize(n_features);
        for(int64_t j = 0; j < n_features; ++j){
            for(int64_t k = 0; k < n_features; ++k){
                if(k == j) continue;
                if(feat_sd[j] <= static_cast<T>(0) || feat_sd[k] <= static_cast<T>(0)) continue;

                // Compute Pearson correlation between features j and k.
                T cov_sum = static_cast<T>(0);
                for(int64_t i = 0; i < n_samples; ++i){
                    cov_sum += (X.read_coeff(i, j) - feat_mean[j]) *
                               (X.read_coeff(i, k) - feat_mean[k]);
                }
                const T corr = cov_sum / (static_cast<T>(n_samples) * feat_sd[j] * feat_sd[k]);

                if(std::abs(corr) > this->correlation_threshold){
                    conditioning_sets[j].push_back(k);
                }
            }
        }
    }

    // Preallocate a single row buffer for predictions.
    num_array<T> x_row(1, n_features);

    for(int64_t t = 0; t < this->n_trees; ++t){
        const auto &oob = this->oob_indices_per_tree[t];
        const int64_t n_oob = static_cast<int64_t>(oob.size());
        if(n_oob == 0) continue;
        ++n_trees_with_oob;

        // Compute baseline OOB SSR for this tree.
        T baseline_ssr = static_cast<T>(0);
        for(const auto idx : oob){
            for(int64_t f = 0; f < n_features; ++f){
                x_row.coeff(0, f) = X.read_coeff(idx, f);
            }
            const T pred = predict_tree(this->trees[t].get(), x_row);
            const T residual = y.read_coeff(idx, 0) - pred;
            baseline_ssr += residual * residual;
        }

        // For each feature, compute importance.
        for(int64_t feat = 0; feat < n_features; ++feat){
            if(this->importance_method == ConditionalImportanceMethod::permutation){
                // Standard (marginal) permutation importance.
                std::vector<T> permuted_vals;
                permuted_vals.reserve(n_oob);
                for(const auto idx : oob){
                    permuted_vals.push_back(X.read_coeff(idx, feat));
                }
                std::shuffle(permuted_vals.begin(), permuted_vals.end(), rng);

                T permuted_ssr = static_cast<T>(0);
                for(int64_t i = 0; i < n_oob; ++i){
                    for(int64_t ff = 0; ff < n_features; ++ff){
                        x_row.coeff(0, ff) = X.read_coeff(oob[i], ff);
                    }
                    x_row.coeff(0, feat) = permuted_vals[i];
                    const T pred = predict_tree(this->trees[t].get(), x_row);
                    const T residual = y.read_coeff(oob[i], 0) - pred;
                    permuted_ssr += residual * residual;
                }

                this->feature_importances[feat] += (permuted_ssr - baseline_ssr) / static_cast<T>(n_oob);

            }else{
                // Conditional permutation importance (Strobl et al. 2008).
                //
                // Algorithm:
                //   1. Identify the conditioning set Z for feature Xj (already precomputed).
                //   2. For each OOB sample, determine its grid cell based on the quantile
                //      bins of the conditioning variables.
                //   3. Within each grid cell, permute Xj values independently.
                //   4. Compute the increase in OOB SSR.

                const auto &cond_set = conditioning_sets[feat];

                if(cond_set.empty()){
                    // No conditioning variables: fall back to standard permutation.
                    std::vector<T> permuted_vals;
                    permuted_vals.reserve(n_oob);
                    for(const auto idx : oob){
                        permuted_vals.push_back(X.read_coeff(idx, feat));
                    }
                    std::shuffle(permuted_vals.begin(), permuted_vals.end(), rng);

                    T permuted_ssr = static_cast<T>(0);
                    for(int64_t i = 0; i < n_oob; ++i){
                        for(int64_t ff = 0; ff < n_features; ++ff){
                            x_row.coeff(0, ff) = X.read_coeff(oob[i], ff);
                        }
                        x_row.coeff(0, feat) = permuted_vals[i];
                        const T pred = predict_tree(this->trees[t].get(), x_row);
                        const T residual = y.read_coeff(oob[i], 0) - pred;
                        permuted_ssr += residual * residual;
                    }

                    this->feature_importances[feat] += (permuted_ssr - baseline_ssr) / static_cast<T>(n_oob);

                }else{
                    // Conditional permutation per Strobl et al. (2008):
                    // Partition OOB samples into grid cells based on bisection of each
                    // conditioning variable at its median. Per Strobl et al. (2008, Section
                    // "Conditional permutation importance"), the conditioning variables are
                    // "cut at their median" and Xj is permuted within the resulting cells.
                    //
                    // Each OOB sample is assigned a cell index as a binary encoding over
                    // conditioning variables: for each conditioning variable, 0 if below or
                    // equal to the median, 1 if above.

                    // Compute medians for conditioning variables using the full training data.
                    std::vector<T> cond_medians(cond_set.size());
                    for(size_t c = 0; c < cond_set.size(); ++c){
                        const int64_t cv = cond_set[c];
                        std::vector<T> vals(n_samples);
                        for(int64_t i = 0; i < n_samples; ++i){
                            vals[i] = X.read_coeff(i, cv);
                        }
                        std::sort(vals.begin(), vals.end());
                        if(n_samples % 2 == 0){
                            cond_medians[c] = (vals[n_samples / 2 - 1] + vals[n_samples / 2]) / static_cast<T>(2);
                        }else{
                            cond_medians[c] = vals[n_samples / 2];
                        }
                    }

                    // Limit the number of conditioning variables used to form the grid
                    // to avoid an excessive number of cells (2^n_cond cells). With many
                    // conditioning variables, cells become too sparse for meaningful
                    // within-cell permutation.
                    const size_t max_cond_vars = std::min<size_t>(cond_set.size(), 10);

                    // Assign each OOB sample to a grid cell.
                    std::map<int64_t, std::vector<int64_t>> cell_to_oob_local_indices;
                    for(int64_t i = 0; i < n_oob; ++i){
                        int64_t cell_id = 0;
                        for(size_t c = 0; c < max_cond_vars; ++c){
                            const int64_t cv = cond_set[c];
                            if(X.read_coeff(oob[i], cv) > cond_medians[c]){
                                cell_id |= (static_cast<int64_t>(1) << c);
                            }
                        }
                        cell_to_oob_local_indices[cell_id].push_back(i);
                    }

                    // Within each cell, permute the feature values independently.
                    std::vector<T> permuted_feat_vals(n_oob);
                    for(int64_t i = 0; i < n_oob; ++i){
                        permuted_feat_vals[i] = X.read_coeff(oob[i], feat);
                    }

                    for(auto &[cell_id, local_indices] : cell_to_oob_local_indices){
                        (void)cell_id;
                        if(local_indices.size() <= 1) continue;

                        // Extract feature values for this cell.
                        std::vector<T> cell_vals;
                        cell_vals.reserve(local_indices.size());
                        for(const auto li : local_indices){
                            cell_vals.push_back(X.read_coeff(oob[li], feat));
                        }
                        std::shuffle(cell_vals.begin(), cell_vals.end(), rng);

                        // Write back permuted values.
                        for(size_t v = 0; v < local_indices.size(); ++v){
                            permuted_feat_vals[local_indices[v]] = cell_vals[v];
                        }
                    }

                    // Compute permuted SSR.
                    T permuted_ssr = static_cast<T>(0);
                    for(int64_t i = 0; i < n_oob; ++i){
                        for(int64_t ff = 0; ff < n_features; ++ff){
                            x_row.coeff(0, ff) = X.read_coeff(oob[i], ff);
                        }
                        x_row.coeff(0, feat) = permuted_feat_vals[i];
                        const T pred = predict_tree(this->trees[t].get(), x_row);
                        const T residual = y.read_coeff(oob[i], 0) - pred;
                        permuted_ssr += residual * residual;
                    }

                    this->feature_importances[feat] += (permuted_ssr - baseline_ssr) / static_cast<T>(n_oob);
                }
            }
        }
    }

    // Average across trees.
    if(n_trees_with_oob > 0){
        for(int64_t f = 0; f < n_features; ++f){
            this->feature_importances[f] /= static_cast<T>(n_trees_with_oob);
        }
    }
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template void Stats::ConditionalRandomForests<double>::compute_importance(const num_array<double> &, const num_array<double> &);
    template void Stats::ConditionalRandomForests<float>::compute_importance(const num_array<float> &, const num_array<float> &);
#endif


template <class T>
bool Stats::ConditionalRandomForests<T>::write_tree_node(std::ostream &os, const TreeNode *node) const {
    if(node == nullptr){
        return false;
    }
    if(node->is_leaf){
        os << "L " << node->value << "\n";
    }else{
        os << "I " << node->split_feature << " " << node->split_threshold << "\n";
        if(!write_tree_node(os, node->left.get())) return false;
        if(!write_tree_node(os, node->right.get())) return false;
    }
    return (!os.fail());
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template bool Stats::ConditionalRandomForests<double>::write_tree_node(std::ostream &, const TreeNode *) const;
    template bool Stats::ConditionalRandomForests<float>::write_tree_node(std::ostream &, const TreeNode *) const;
#endif


template <class T>
std::unique_ptr<typename Stats::ConditionalRandomForests<T>::TreeNode>
Stats::ConditionalRandomForests<T>::read_tree_node(std::istream &is) {
    std::string node_type;
    is >> node_type;
    if(is.fail()) return nullptr;

    auto node = std::make_unique<TreeNode>();
    try{
        if(node_type == "L"){
            node->is_leaf = true;
            std::string val_str;
            is >> val_str;
            if(is.fail()) return nullptr;
            node->value = static_cast<T>(std::stold(val_str));
        }else if(node_type == "I"){
            node->is_leaf = false;
            is >> node->split_feature;
            std::string thresh_str;
            is >> thresh_str;
            if(is.fail()) return nullptr;
            node->split_threshold = static_cast<T>(std::stold(thresh_str));
            node->left = read_tree_node(is);
            node->right = read_tree_node(is);
            if(!node->left || !node->right) return nullptr;
        }else{
            return nullptr;
        }
    }catch(const std::invalid_argument &){
        return nullptr;
    }catch(const std::out_of_range &){
        return nullptr;
    }
    return node;
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template std::unique_ptr<typename Stats::ConditionalRandomForests<double>::TreeNode>
        Stats::ConditionalRandomForests<double>::read_tree_node(std::istream &);
    template std::unique_ptr<typename Stats::ConditionalRandomForests<float>::TreeNode>
        Stats::ConditionalRandomForests<float>::read_tree_node(std::istream &);
#endif


template <class T>
bool Stats::ConditionalRandomForests<T>::write_to(std::ostream &os) const {
    const auto original_precision = os.precision();
    os.precision( std::numeric_limits<T>::max_digits10 );

    struct precision_guard {
        std::ostream &s;
        std::streamsize p;
        ~precision_guard(){ s.precision(p); }
    } guard{os, original_precision};

    os << "ConditionalRandomForests_v1" << "\n";
    os << "n_trees " << this->n_trees << "\n";
    os << "max_depth " << this->max_depth << "\n";
    os << "min_samples_split " << this->min_samples_split << "\n";
    os << "alpha " << this->alpha << "\n";
    os << "n_permutations " << this->n_permutations << "\n";
    os << "max_features " << this->max_features << "\n";
    os << "subsample_fraction " << this->subsample_fraction << "\n";
    os << "correlation_threshold " << this->correlation_threshold << "\n";
    os << "n_features_trained " << this->n_features_trained << "\n";
    os << "random_seed " << this->random_seed << "\n";
    os << "importance_method " << static_cast<int>(this->importance_method) << "\n";

    // Write feature importances.
    const int64_t n_importances = static_cast<int64_t>(this->feature_importances.size());
    os << "feature_importances " << n_importances << "\n";
    for(int64_t i = 0; i < n_importances; ++i){
        os << this->feature_importances[i];
        if(i + 1 < n_importances) os << " ";
    }
    if(n_importances > 0) os << "\n";

    // Write OOB indices.
    const int64_t n_oob_sets = static_cast<int64_t>(this->oob_indices_per_tree.size());
    os << "oob_sets " << n_oob_sets << "\n";
    for(int64_t t_idx = 0; t_idx < n_oob_sets; ++t_idx){
        const auto &oob = this->oob_indices_per_tree[t_idx];
        os << oob.size();
        for(const auto idx : oob){
            os << " " << idx;
        }
        os << "\n";
    }

    // Write trees.
    const int64_t n_actual_trees = static_cast<int64_t>(this->trees.size());
    os << "trees " << n_actual_trees << "\n";
    for(int64_t t_idx = 0; t_idx < n_actual_trees; ++t_idx){
        os << "begin_tree " << t_idx << "\n";
        if(!write_tree_node(os, this->trees[t_idx].get())) return false;
        os << "end_tree\n";
    }

    os.flush();
    return (!os.fail());
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template bool Stats::ConditionalRandomForests<double>::write_to(std::ostream &) const;
    template bool Stats::ConditionalRandomForests<float>::write_to(std::ostream &) const;
#endif


template <class T>
bool Stats::ConditionalRandomForests<T>::read_from(std::istream &is) {
    try{
    std::string label;

    is >> label;
    if(is.fail() || label != "ConditionalRandomForests_v1") return false;

    is >> label >> this->n_trees;
    if(is.fail() || label != "n_trees") return false;

    is >> label >> this->max_depth;
    if(is.fail() || label != "max_depth") return false;

    is >> label >> this->min_samples_split;
    if(is.fail() || label != "min_samples_split") return false;

    {
        std::string val_str;
        is >> label >> val_str;
        if(is.fail() || label != "alpha") return false;
        this->alpha = static_cast<T>(std::stold(val_str));
    }

    is >> label >> this->n_permutations;
    if(is.fail() || label != "n_permutations") return false;

    is >> label >> this->max_features;
    if(is.fail() || label != "max_features") return false;

    {
        std::string val_str;
        is >> label >> val_str;
        if(is.fail() || label != "subsample_fraction") return false;
        this->subsample_fraction = static_cast<T>(std::stold(val_str));
    }

    {
        std::string val_str;
        is >> label >> val_str;
        if(is.fail() || label != "correlation_threshold") return false;
        this->correlation_threshold = static_cast<T>(std::stold(val_str));
    }

    is >> label >> this->n_features_trained;
    if(is.fail() || label != "n_features_trained") return false;

    is >> label >> this->random_seed;
    if(is.fail() || label != "random_seed") return false;

    int imp_method_int;
    is >> label >> imp_method_int;
    if(is.fail() || label != "importance_method") return false;
    if(imp_method_int < 0 || imp_method_int > 2){
        this->importance_method = ConditionalImportanceMethod::none;
        return false;
    }
    this->importance_method = static_cast<ConditionalImportanceMethod>(imp_method_int);

    // Read feature importances.
    int64_t n_importances;
    is >> label >> n_importances;
    if(is.fail() || label != "feature_importances") return false;
    if(n_importances < 0 || n_importances > 1'000'000) return false;
    this->feature_importances.resize(n_importances);
    for(int64_t i = 0; i < n_importances; ++i){
        std::string val_str;
        is >> val_str;
        if(is.fail()) return false;
        this->feature_importances[i] = static_cast<T>(std::stold(val_str));
    }

    // Read OOB indices.
    int64_t n_oob_sets;
    is >> label >> n_oob_sets;
    if(is.fail() || label != "oob_sets") return false;
    if(n_oob_sets < 0 || n_oob_sets > 1'000'000) return false;
    this->oob_indices_per_tree.resize(n_oob_sets);
    for(int64_t t_idx = 0; t_idx < n_oob_sets; ++t_idx){
        int64_t n_oob;
        is >> n_oob;
        if(is.fail()) return false;
        if(n_oob < 0 || n_oob > 1'000'000'000) return false;
        this->oob_indices_per_tree[t_idx].resize(n_oob);
        for(int64_t i = 0; i < n_oob; ++i){
            is >> this->oob_indices_per_tree[t_idx][i];
            if(is.fail()) return false;
        }
    }

    // Read trees.
    int64_t n_actual_trees;
    is >> label >> n_actual_trees;
    if(is.fail() || label != "trees") return false;
    if(n_actual_trees < 0 || n_actual_trees > 1'000'000) return false;

    this->trees.clear();
    this->trees.reserve(n_actual_trees);
    for(int64_t t_idx = 0; t_idx < n_actual_trees; ++t_idx){
        int64_t tree_idx;
        is >> label >> tree_idx;
        if(is.fail() || label != "begin_tree") return false;

        auto node = read_tree_node(is);
        if(!node) return false;
        this->trees.push_back(std::move(node));

        is >> label;
        if(is.fail() || label != "end_tree") return false;
    }

    // Validate invariants.
    if(n_actual_trees != this->n_trees){
        return false;
    }
    if(this->importance_method != ConditionalImportanceMethod::none){
        if(static_cast<int64_t>(this->oob_indices_per_tree.size()) != n_actual_trees){
            return false;
        }
    }

    return (!is.fail());
    }catch(const std::invalid_argument &){
        return false;
    }catch(const std::out_of_range &){
        return false;
    }
}
#ifndef YGOR_STATS_CONDITIONAL_FORESTS_DISABLE_ALL_SPECIALIZATIONS
    template bool Stats::ConditionalRandomForests<double>::read_from(std::istream &);
    template bool Stats::ConditionalRandomForests<float>::read_from(std::istream &);
#endif
