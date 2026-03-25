//Ygor_Conditional_Forest_Predict.cc -- A command-line utility to predict using a trained conditional random forest model.

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "YgorArguments.h"
#include "YgorMath.h"
#include "YgorMathIOCSV.h"
#include "YgorStatsConditionalForests.h"

int main(int argc, char **argv){

    std::string model_file;
    std::string input_file;
    bool has_header = false;

    ArgumentHandler arger;
    arger.description = "Predict using a trained conditional random forest model.";

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
    Stats::ConditionalRandomForests<double> model;
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

    auto csv_result = ReadNumArrayFromCSV<double>(fi, has_header);
    const auto &all_data = csv_result.data;
    const int64_t n_rows = all_data.num_rows();
    const int64_t n_cols = all_data.num_cols();

    for(int64_t r = 0; r < n_rows; ++r){
        num_array<double> x = all_data.subarray(r, r + 1, 0, n_cols);
        double prediction = model.predict(x);
        std::cout << prediction << std::endl;
    }

    return 0;
}
