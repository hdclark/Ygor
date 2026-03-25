//Ygor_Stochastic_Forest_Predict.cc -- A command-line utility to predict using a trained stochastic forest model.

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

    std::string model_file;
    std::string input_file;
    bool has_header = false;

    ArgumentHandler arger;
    arger.description = "Predict using a trained stochastic forest model.";

    arger.push_back(std::make_tuple(1, 'm', "model", true, "<file>",
        "Trained model file to load.",
        [&](const std::string &optarg) -> void {
            model_file = optarg;
        }));
    arger.push_back(std::make_tuple(1, 'i', "input", true, "<file>",
        "Input CSV/TSV file with feature values.",
        [&](const std::string &optarg) -> void {
            input_file = optarg;
        }));
    arger.push_back(std::make_tuple(1, 'H', "header", false, "",
        "Indicate that the first row is a header (will be skipped).",
        [&](const std::string &) -> void {
            has_header = true;
        }));

    arger.Launch(argc, argv);

    if(model_file.empty()){
        throw std::runtime_error("A model file must be specified via -m or --model.");
    }
    if(input_file.empty()){
        throw std::runtime_error("An input file must be specified via -i or --input.");
    }

    // Load the model.
    Stats::StochasticForests<double> model;
    {
        std::ifstream fm(model_file);
        if(!fm.good()){
            throw std::runtime_error("Unable to open model file '" + model_file + "'.");
        }
        if(!model.read_from(fm)){
            throw std::runtime_error("Failed to read model from '" + model_file + "'.");
        }
    }

    // Read the input file.
    std::ifstream fi(input_file);
    if(!fi.good()){
        throw std::runtime_error("Unable to open input file '" + input_file + "'.");
    }

    std::string line;
    bool first_data_line = true;
    char delimiter = ',';

    while(std::getline(fi, line)){
        if(line.empty()) continue;
        if(has_header && first_data_line){
            has_header = false;
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
        if(vals.empty()) continue;

        const int64_t n_features = static_cast<int64_t>(vals.size());
        num_array<double> x(1, n_features, 0.0);
        for(int64_t c = 0; c < n_features; ++c){
            x.coeff(0, c) = vals[c];
        }

        double prediction = model.predict(x);
        std::cout << prediction << std::endl;
    }

    return 0;
}
