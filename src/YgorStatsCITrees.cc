//YgorStatsCITrees.cc - A part of Ygor, 2026. Written by hal clark.

#include <cmath>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <limits>
#include <numeric>
#include <algorithm>

#include "YgorDefinitions.h"
#include "YgorStatsCITrees.h"
#include "YgorLog.h"


template <class T>
Stats::ConditionalInferenceTrees<T>::ConditionalInferenceTrees(int64_t max_depth,
                                                               int64_t min_samples_split,
                                                               T alpha,
                                                               int64_t n_permutations,
                                                               uint64_t random_seed)
    : max_depth(max_depth),
      min_samples_split(min_samples_split),
      alpha(alpha),
      n_permutations(n_permutations),
      n_features_trained(-1),
      random_seed(random_seed) {

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
}
#ifndef YGOR_STATS_CI_TREES_DISABLE_ALL_SPECIALIZATIONS
    template Stats::ConditionalInferenceTrees<double>::ConditionalInferenceTrees(int64_t, int64_t, double, int64_t, uint64_t);
    template Stats::ConditionalInferenceTrees<float>::ConditionalInferenceTrees(int64_t, int64_t, float, int64_t, uint64_t);
#endif


template <class T>
void Stats::ConditionalInferenceTrees<T>::fit(const num_array<T> &X, const num_array<T> &y) {
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

    // Create sample indices for the full dataset.
    std::vector<int64_t> sample_indices(n_samples);
    std::iota(sample_indices.begin(), sample_indices.end(), 0);

    // Build the tree.
    std::mt19937 rng(this->random_seed);
    this->root = build_tree(X, y, sample_indices, 0, rng);
}
#ifndef YGOR_STATS_CI_TREES_DISABLE_ALL_SPECIALIZATIONS
    template void Stats::ConditionalInferenceTrees<double>::fit(const num_array<double> &, const num_array<double> &);
    template void Stats::ConditionalInferenceTrees<float>::fit(const num_array<float> &, const num_array<float> &);
#endif


template <class T>
std::unique_ptr<typename Stats::ConditionalInferenceTrees<T>::TreeNode>
Stats::ConditionalInferenceTrees<T>::build_tree(
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

    // Variable selection via permutation testing.
    int64_t best_feature = -1;
    T best_pvalue = static_cast<T>(1);

    if(!select_variable(X, y, sample_indices, best_feature, best_pvalue, rng)){
        // No significant variable found, create leaf.
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
        // Could not find a valid split point, create leaf.
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
#ifndef YGOR_STATS_CI_TREES_DISABLE_ALL_SPECIALIZATIONS
    template std::unique_ptr<typename Stats::ConditionalInferenceTrees<double>::TreeNode>
        Stats::ConditionalInferenceTrees<double>::build_tree(const num_array<double> &, const num_array<double> &,
                                                             const std::vector<int64_t> &, int64_t, std::mt19937 &);
    template std::unique_ptr<typename Stats::ConditionalInferenceTrees<float>::TreeNode>
        Stats::ConditionalInferenceTrees<float>::build_tree(const num_array<float> &, const num_array<float> &,
                                                            const std::vector<int64_t> &, int64_t, std::mt19937 &);
#endif


template <class T>
bool Stats::ConditionalInferenceTrees<T>::select_variable(
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
    // For each feature j, the test statistic is the standardized linear association:
    //   c_j = |sum_i (X_{ij} - Xbar_j)(Y_i - Ybar)| / sqrt(SS_Xj)
    // where SS_Xj = sum_i (X_{ij} - Xbar_j)^2.
    //
    // The global null hypothesis (all features independent of the response) is tested
    // using the max-type statistic: c_max = max_j c_j, with the p-value estimated from
    // the permutation distribution of c_max. This naturally controls for multiple
    // testing across features, avoiding selection bias.
    //
    // This permutation-based variant avoids the need to compute the incomplete gamma
    // function that would be required for the asymptotic chi-squared approximation.
    //

    const int64_t n_features = X.num_cols();
    const int64_t n_samples = static_cast<int64_t>(sample_indices.size());

    if(n_samples < 2){
        return false;
    }

    best_pvalue = static_cast<T>(1);
    best_feature = -1;

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

    // Pre-extract feature values and precompute feature deviations from mean and SS_Xj.
    // Also compute observed standardized statistic for each feature:
    //   c_j = |sum_i x_dev_j[i] * y_dev[i]| / sqrt(x_ss_j)
    std::vector<std::vector<T>> x_dev(n_features);
    std::vector<T> x_ss(n_features, static_cast<T>(0));
    std::vector<T> inv_sqrt_x_ss(n_features, static_cast<T>(0));
    std::vector<T> observed_stat(n_features, static_cast<T>(0));
    std::vector<bool> feature_valid(n_features, false);

    T obs_max = static_cast<T>(0);

    for(int64_t j = 0; j < n_features; ++j){
        x_dev[j].resize(n_samples);

        // Compute feature mean.
        T x_sum = static_cast<T>(0);
        for(int64_t i = 0; i < n_samples; ++i){
            x_sum += X.read_coeff(sample_indices[i], j);
        }
        const T x_mean = x_sum / static_cast<T>(n_samples);

        // Compute feature deviations and sum of squared deviations.
        T xy_sum = static_cast<T>(0);
        for(int64_t i = 0; i < n_samples; ++i){
            x_dev[j][i] = X.read_coeff(sample_indices[i], j) - x_mean;
            x_ss[j] += x_dev[j][i] * x_dev[j][i];
            xy_sum += x_dev[j][i] * y_dev[i];
        }

        // Skip features with zero variance.
        if(x_ss[j] <= static_cast<T>(0)){
            continue;
        }

        feature_valid[j] = true;
        inv_sqrt_x_ss[j] = static_cast<T>(1) / std::sqrt(x_ss[j]);

        // Standardized observed statistic: |T_j| / sqrt(SS_Xj).
        observed_stat[j] = std::abs(xy_sum) * inv_sqrt_x_ss[j];

        if(observed_stat[j] > obs_max){
            obs_max = observed_stat[j];
            best_feature = j;
        }
    }

    // If no valid feature was found, return false.
    if(best_feature < 0){
        return false;
    }

    // Permutation test using shared permutations for all features (global max-type test).
    // For each permutation, shuffle y_dev, compute the standardized statistic for all valid
    // features, take the maximum, and compare against the observed maximum.
    std::vector<T> y_dev_perm(y_dev);
    int64_t count_ge = 0;

    // Early-exit parameters: check after early_check permutations.
    const int64_t early_check = std::min(static_cast<int64_t>(100), this->n_permutations);

    for(int64_t k = 0; k < this->n_permutations; ++k){
        std::shuffle(y_dev_perm.begin(), y_dev_perm.end(), rng);

        // Compute the max standardized statistic across all valid features for this permutation.
        T perm_max = static_cast<T>(0);
        for(int64_t j = 0; j < n_features; ++j){
            if(!feature_valid[j]) continue;

            T perm_stat = static_cast<T>(0);
            for(int64_t i = 0; i < n_samples; ++i){
                perm_stat += x_dev[j][i] * y_dev_perm[i];
            }
            perm_stat = std::abs(perm_stat) * inv_sqrt_x_ss[j];

            if(perm_stat > perm_max){
                perm_max = perm_stat;
            }
        }

        if(perm_max >= obs_max){
            ++count_ge;
        }

        // Early exit: after early_check permutations, if the global test is clearly
        // non-significant, skip the remaining permutations.
        if(k + 1 == early_check && early_check < this->n_permutations){
            const T early_pvalue = static_cast<T>(count_ge + 1) / static_cast<T>(early_check + 1);
            // If the estimated p-value is much larger than alpha, exit early.
            // Use a conservative threshold: 2x alpha.
            if(early_pvalue > static_cast<T>(2) * this->alpha){
                best_pvalue = early_pvalue;
                return true;
            }
        }
    }

    // Global p-value from the permutation distribution of the max statistic.
    // Uses the formula (b+1)/(B+1) per Davison & Hinkley (1997) for valid permutation p-values.
    best_pvalue = static_cast<T>(count_ge + 1) / static_cast<T>(this->n_permutations + 1);

    return true;
}
#ifndef YGOR_STATS_CI_TREES_DISABLE_ALL_SPECIALIZATIONS
    template bool Stats::ConditionalInferenceTrees<double>::select_variable(
        const num_array<double> &, const num_array<double> &,
        const std::vector<int64_t> &, int64_t &, double &, std::mt19937 &);
    template bool Stats::ConditionalInferenceTrees<float>::select_variable(
        const num_array<float> &, const num_array<float> &,
        const std::vector<int64_t> &, int64_t &, float &, std::mt19937 &);
#endif


template <class T>
T Stats::ConditionalInferenceTrees<T>::compute_association(
    const num_array<T> &X,
    const num_array<T> &y,
    const std::vector<int64_t> &sample_indices,
    int64_t feature) {

    const int64_t n = static_cast<int64_t>(sample_indices.size());
    if(n < 2) return static_cast<T>(0);

    // Compute means.
    T x_sum = static_cast<T>(0);
    T y_sum = static_cast<T>(0);
    for(const auto idx : sample_indices){
        x_sum += X.read_coeff(idx, feature);
        y_sum += y.read_coeff(idx, 0);
    }
    const T x_mean = x_sum / static_cast<T>(n);
    const T y_mean = y_sum / static_cast<T>(n);

    // Compute absolute unnormalized covariance: |sum_i (X_ij - Xbar)(Y_i - Ybar)|.
    T xy_sum = static_cast<T>(0);
    for(const auto idx : sample_indices){
        xy_sum += (X.read_coeff(idx, feature) - x_mean) * (y.read_coeff(idx, 0) - y_mean);
    }

    return std::abs(xy_sum);
}
#ifndef YGOR_STATS_CI_TREES_DISABLE_ALL_SPECIALIZATIONS
    template double Stats::ConditionalInferenceTrees<double>::compute_association(
        const num_array<double> &, const num_array<double> &,
        const std::vector<int64_t> &, int64_t);
    template float Stats::ConditionalInferenceTrees<float>::compute_association(
        const num_array<float> &, const num_array<float> &,
        const std::vector<int64_t> &, int64_t);
#endif


template <class T>
bool Stats::ConditionalInferenceTrees<T>::find_best_split_point(
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

    // Sort to find potential split points.
    std::sort(feature_values.begin(), feature_values.end());

    T best_variance = std::numeric_limits<T>::infinity();
    bool found_split = false;

    // Clear output vectors.
    left_indices.clear();
    right_indices.clear();

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

        // Compute weighted child variance (lower is better).
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
#ifndef YGOR_STATS_CI_TREES_DISABLE_ALL_SPECIALIZATIONS
    template bool Stats::ConditionalInferenceTrees<double>::find_best_split_point(
        const num_array<double> &, const num_array<double> &,
        const std::vector<int64_t> &, int64_t, double &,
        std::vector<int64_t> &, std::vector<int64_t> &);
    template bool Stats::ConditionalInferenceTrees<float>::find_best_split_point(
        const num_array<float> &, const num_array<float> &,
        const std::vector<int64_t> &, int64_t, float &,
        std::vector<int64_t> &, std::vector<int64_t> &);
#endif


template <class T>
T Stats::ConditionalInferenceTrees<T>::compute_weighted_child_variance(
    const num_array<T> &y,
    const std::vector<int64_t> &left_indices,
    const std::vector<int64_t> &right_indices) {

    const int64_t n_left = static_cast<int64_t>(left_indices.size());
    const int64_t n_right = static_cast<int64_t>(right_indices.size());
    const int64_t n_total = n_left + n_right;

    if(n_total == 0) return static_cast<T>(0);

    // Compute variance for left child using two-pass algorithm.
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

    // Compute weighted variance of children.
    return (static_cast<T>(n_left) * left_var +
            static_cast<T>(n_right) * right_var) / static_cast<T>(n_total);
}
#ifndef YGOR_STATS_CI_TREES_DISABLE_ALL_SPECIALIZATIONS
    template double Stats::ConditionalInferenceTrees<double>::compute_weighted_child_variance(
        const num_array<double> &, const std::vector<int64_t> &, const std::vector<int64_t> &);
    template float Stats::ConditionalInferenceTrees<float>::compute_weighted_child_variance(
        const num_array<float> &, const std::vector<int64_t> &, const std::vector<int64_t> &);
#endif


template <class T>
T Stats::ConditionalInferenceTrees<T>::predict(const num_array<T> &x) const {
    // Validate input.
    if(x.num_rows() != 1){
        throw std::invalid_argument("Input x must be a 1xM matrix (row vector)");
    }
    if(!this->root){
        throw std::runtime_error("Model has not been fitted yet");
    }
    if(x.num_cols() != this->n_features_trained){
        throw std::invalid_argument("Input features must match training data features");
    }

    return predict_tree(this->root.get(), x);
}
#ifndef YGOR_STATS_CI_TREES_DISABLE_ALL_SPECIALIZATIONS
    template double Stats::ConditionalInferenceTrees<double>::predict(const num_array<double> &) const;
    template float Stats::ConditionalInferenceTrees<float>::predict(const num_array<float> &) const;
#endif


template <class T>
T Stats::ConditionalInferenceTrees<T>::predict_tree(const TreeNode *node, const num_array<T> &x) const {
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
#ifndef YGOR_STATS_CI_TREES_DISABLE_ALL_SPECIALIZATIONS
    template double Stats::ConditionalInferenceTrees<double>::predict_tree(const TreeNode *, const num_array<double> &) const;
    template float Stats::ConditionalInferenceTrees<float>::predict_tree(const TreeNode *, const num_array<float> &) const;
#endif


template <class T>
T Stats::ConditionalInferenceTrees<T>::get_alpha() const {
    return this->alpha;
}
#ifndef YGOR_STATS_CI_TREES_DISABLE_ALL_SPECIALIZATIONS
    template double Stats::ConditionalInferenceTrees<double>::get_alpha() const;
    template float Stats::ConditionalInferenceTrees<float>::get_alpha() const;
#endif


template <class T>
int64_t Stats::ConditionalInferenceTrees<T>::get_n_permutations() const {
    return this->n_permutations;
}
#ifndef YGOR_STATS_CI_TREES_DISABLE_ALL_SPECIALIZATIONS
    template int64_t Stats::ConditionalInferenceTrees<double>::get_n_permutations() const;
    template int64_t Stats::ConditionalInferenceTrees<float>::get_n_permutations() const;
#endif
