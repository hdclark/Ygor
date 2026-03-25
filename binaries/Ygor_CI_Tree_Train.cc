//Ygor_CI_Tree_Train.cc -- A command-line utility to train a conditional inference tree model.

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
#include "YgorStatsCITrees.h"

int main(int argc, char **argv){

    std::string input_file;
    std::string output_file;
    bool has_header = false;
    int64_t max_depth = 10;
    int64_t min_samples_split = 2;
    double alpha = 0.05;
    int64_t n_permutations = 1000;
    uint64_t random_seed = 42;

    ArgumentHandler arger;
    arger.description = "Train a conditional inference tree model from tabular data (CSV/TSV).";

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
    arger.push_back(std::make_tuple(2, 'a', "alpha", true, "<float>",
        "Significance level for conditional inference tests (default: 0.05).",
        [&](const std::string &optarg) -> void {
            alpha = std::stod(optarg);
        }));
    arger.push_back(std::make_tuple(2, 'p', "n-permutations", true, "<int>",
        "Number of permutations for hypothesis tests (default: 1000).",
        [&](const std::string &optarg) -> void {
            n_permutations = std::stoll(optarg);
        }));
    arger.push_back(std::make_tuple(2, 'r', "random-seed", true, "<int>",
        "Random seed (default: 42).",
        [&](const std::string &optarg) -> void {
            random_seed = std::stoull(optarg);
        }));

    arger.Launch(argc, argv);

    if(input_file.empty()){
        throw std::runtime_error("An input file must be specified via -i or --input.");
    }
    if(output_file.empty()){
        throw std::runtime_error("An output file must be specified via -o or --output.");
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
        if(first_data_line){
            if(line.find('\t') != std::string::npos){
                delimiter = '\t';
            }
            if(has_header){
                first_data_line = false;
                continue;
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

    std::cout << "Training conditional inference tree with " << n_rows << " samples and "
              << n_features << " features." << std::endl;

    // Train the model.
    Stats::ConditionalInferenceTrees<double> model(max_depth, min_samples_split, alpha,
                                                   n_permutations, random_seed);
    model.fit(X, y);

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
