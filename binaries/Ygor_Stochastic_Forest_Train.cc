//Ygor_Stochastic_Forest_Train.cc -- A command-line utility to train a stochastic forest model.

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "YgorArguments.h"
#include "YgorMath.h"
#include "YgorStatsStochasticForests.h"

int main(int argc, char **argv){

    std::string input_file;
    std::string output_file;
    bool has_header = false;
    int64_t n_trees = 100;
    int64_t max_depth = 10;
    int64_t min_samples_split = 2;
    int64_t max_features = -1;
    uint64_t random_seed = 42;
    std::string importance_str = "none";

    ArgumentHandler arger;
    arger.description = "Train a stochastic forest model from tabular data (CSV/TSV).";

    arger.push_back(std::make_tuple(1, 'i', "input", true, "<file>",
        "Input CSV/TSV file. Last column is the response variable.",
        [&](const std::string &optarg) -> void {
            input_file = optarg;
        }));
    arger.push_back(std::make_tuple(1, 'o', "output", true, "<file>",
        "Output file for the trained model.",
        [&](const std::string &optarg) -> void {
            output_file = optarg;
        }));
    arger.push_back(std::make_tuple(1, 'H', "header", false, "",
        "Indicate that the first row is a header (will be skipped).",
        [&](const std::string &) -> void {
            has_header = true;
        }));
    arger.push_back(std::make_tuple(2, 't', "n-trees", true, "<int>",
        "Number of trees (default: 100).",
        [&](const std::string &optarg) -> void {
            n_trees = std::stoll(optarg);
        }));
    arger.push_back(std::make_tuple(2, 'd', "max-depth", true, "<int>",
        "Maximum tree depth (default: 10).",
        [&](const std::string &optarg) -> void {
            max_depth = std::stoll(optarg);
        }));
    arger.push_back(std::make_tuple(2, 's', "min-samples-split", true, "<int>",
        "Minimum samples to split a node (default: 2).",
        [&](const std::string &optarg) -> void {
            min_samples_split = std::stoll(optarg);
        }));
    arger.push_back(std::make_tuple(2, 'f', "max-features", true, "<int>",
        "Maximum features per split; -1 for all (default: -1).",
        [&](const std::string &optarg) -> void {
            max_features = std::stoll(optarg);
        }));
    arger.push_back(std::make_tuple(2, 'r', "random-seed", true, "<int>",
        "Random seed (default: 42).",
        [&](const std::string &optarg) -> void {
            random_seed = std::stoull(optarg);
        }));
    arger.push_back(std::make_tuple(2, 'I', "importance", true, "<method>",
        "Feature importance method: none, gini, or permutation (default: none).",
        [&](const std::string &optarg) -> void {
            importance_str = optarg;
        }));

    arger.Launch(argc, argv);

    if(input_file.empty()){
        throw std::runtime_error("An input file must be specified via -i or --input.");
    }
    if(output_file.empty()){
        throw std::runtime_error("An output file must be specified via -o or --output.");
    }

    // Parse the importance method.
    Stats::ImportanceMethod importance_method = Stats::ImportanceMethod::none;
    if(importance_str == "none"){
        importance_method = Stats::ImportanceMethod::none;
    }else if(importance_str == "gini"){
        importance_method = Stats::ImportanceMethod::gini;
    }else if(importance_str == "permutation"){
        importance_method = Stats::ImportanceMethod::permutation;
    }else{
        throw std::runtime_error("Unknown importance method '" + importance_str + "'. Use none, gini, or permutation.");
    }

    // Read the input file.
    std::ifstream fi(input_file);
    if(!fi.good()){
        throw std::runtime_error("Unable to open input file '" + input_file + "'.");
    }

    std::vector<std::vector<double>> rows;
    std::string line;
    bool first_data_line = true;
    char delimiter = ',';

    while(std::getline(fi, line)){
        if(line.empty()) continue;
        if(has_header && first_data_line){
            has_header = false;
            // Auto-detect delimiter from header line.
            if(line.find('\t') != std::string::npos){
                delimiter = '\t';
            }
            continue;
        }
        if(first_data_line){
            if(line.find('\t') != std::string::npos){
                delimiter = '\t';
            }
            first_data_line = false;
        }

        std::vector<double> vals;
        std::stringstream ss(line);
        std::string token;
        while(std::getline(ss, token, delimiter)){
            vals.push_back(std::stod(token));
        }
        if(!vals.empty()){
            rows.push_back(vals);
        }
    }

    if(rows.empty()){
        throw std::runtime_error("No data rows found in input file.");
    }

    const int64_t n_rows = static_cast<int64_t>(rows.size());
    const int64_t n_cols = static_cast<int64_t>(rows.front().size());
    if(n_cols < 2){
        throw std::runtime_error("Input must have at least two columns (features + response).");
    }
    const int64_t n_features = n_cols - 1;

    num_array<double> X(n_rows, n_features, 0.0);
    num_array<double> y(n_rows, 1, 0.0);
    for(int64_t r = 0; r < n_rows; ++r){
        if(static_cast<int64_t>(rows[r].size()) != n_cols){
            throw std::runtime_error("Row " + std::to_string(r) + " has inconsistent number of columns.");
        }
        for(int64_t c = 0; c < n_features; ++c){
            X.coeff(r, c) = rows[r][c];
        }
        y.coeff(r, 0) = rows[r][n_features];
    }

    std::cout << "Training stochastic forest with " << n_rows << " samples and "
              << n_features << " features." << std::endl;

    // Train the model.
    Stats::StochasticForests<double> model(n_trees, max_depth, min_samples_split, max_features, random_seed);
    model.set_importance_method(importance_method);
    model.fit(X, y);

    // Compute and display feature importances.
    if(importance_method == Stats::ImportanceMethod::permutation){
        model.compute_permutation_importance(X, y);
    }
    if(importance_method != Stats::ImportanceMethod::none){
        std::vector<double> importances = model.get_feature_importances();
        std::cout << "Feature importances:" << std::endl;
        for(int64_t c = 0; c < static_cast<int64_t>(importances.size()); ++c){
            std::cout << "  feature " << c << ": " << importances[c] << std::endl;
        }
    }

    // Save the model.
    std::ofstream fo(output_file);
    if(!fo.good()){
        throw std::runtime_error("Unable to open output file '" + output_file + "'.");
    }
    if(!model.write_to(fo)){
        throw std::runtime_error("Failed to write model to output file.");
    }
    std::cout << "Model saved to '" << output_file << "'." << std::endl;

    return 0;
}
