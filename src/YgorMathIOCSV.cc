//YgorMathIOCSV.cc - Routines for reading tabular CSV/TSV data into num_array.
//

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <istream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cctype>

#include "YgorDefinitions.h"
#include "YgorMath.h"
#include "YgorMathIOCSV.h"


template <class T>
csv_load_result<T>
ReadNumArrayFromCSV(std::istream &is,
                    bool has_header,
                    csv_non_numeric_callback_t non_numeric_cb){

    csv_load_result<T> result;
    std::map<std::string, int64_t> &string_to_int = result.string_to_int;
    std::map<int64_t, std::string> &int_to_string = result.int_to_string;
    std::map<int64_t, std::vector<std::pair<int64_t, int64_t>>> &int_to_locations = result.int_to_locations;
    int64_t next_mapped_int = 1; // Start with non-zero value in case zero confuses/weights classifier.

    // Default callback for non-numeric tokens.
    const auto default_cb = [&](const std::string &token, int64_t row, int64_t col) -> double {
        // Case-insensitive comparison for NaN.
        std::string lower;
        lower.reserve(token.size());
        for(const auto &ch : token){
            lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }

        if(lower.empty() || lower == "nan"){
            return static_cast<double>(std::numeric_limits<T>::quiet_NaN());
        }
        if(lower == "inf" || lower == "+inf"){
            return static_cast<double>(std::numeric_limits<T>::infinity());
        }
        if(lower == "-inf"){
            return static_cast<double>(-std::numeric_limits<T>::infinity());
        }

        // Map distinct strings to distinct integers (case sensitive on the original token).
        auto it = string_to_int.find(token);
        if(it == string_to_int.end()){
            string_to_int[token] = next_mapped_int;
            int_to_string[next_mapped_int] = token;
            it = string_to_int.find(token);
            ++next_mapped_int;
        }
        int_to_locations[it->second].emplace_back(row, col);
        return static_cast<double>(it->second);
    };

    const auto &cb = non_numeric_cb ? non_numeric_cb : default_cb;

    // Helper to trim leading/trailing whitespace from a token.
    const auto trim = [](const std::string &s) -> std::string {
        const auto begin = s.find_first_not_of(" \t\r\n");
        if(begin == std::string::npos) return "";
        const auto end = s.find_last_not_of(" \t\r\n");
        return s.substr(begin, end - begin + 1);
    };

    std::vector<std::vector<T>> rows;
    std::string line;
    bool first_data_line = true;
    char delimiter = ',';

    while(std::getline(is, line)){
        if( trim(line).empty() ) continue;
        if(first_data_line){
            // Auto-detect delimiter from first non-empty line.
            if(line.find('\t') != std::string::npos){
                delimiter = '\t';
            }
            if(has_header){
                first_data_line = false;
                continue;
            }
            first_data_line = false;
        }

        const int64_t row_idx = static_cast<int64_t>(rows.size());
        std::vector<T> vals;
        std::stringstream ss(line);
        std::string token;
        int64_t col_idx = 0;
        while(std::getline(ss, token, delimiter)){
            const std::string trimmed = trim(token);
            // First try to parse as a number.
            bool parsed = false;
            if(!trimmed.empty()){
                try{
                    size_t pos = 0;
                    const double val = std::stod(trimmed, &pos);
                    if(pos == trimmed.size()){
                        vals.push_back(static_cast<T>(val));
                        parsed = true;
                    }
                }catch(const std::invalid_argument &){
                }catch(const std::out_of_range &){
                }
            }
            if(!parsed){
                // Use the callback for non-numeric tokens.
                const double mapped_val = cb(trimmed, row_idx, col_idx);
                vals.push_back(static_cast<T>(mapped_val));
            }
            ++col_idx;
        }
        if(!vals.empty()){
            rows.push_back(vals);
        }
    }

    if(rows.empty()){
        throw std::runtime_error("No data rows found in CSV/TSV input.");
    }

    const int64_t n_rows = static_cast<int64_t>(rows.size());
    const int64_t n_cols = static_cast<int64_t>(rows.front().size());

    for(int64_t r = 0; r < n_rows; ++r){
        if(static_cast<int64_t>(rows[r].size()) != n_cols){
            throw std::runtime_error("Row " + std::to_string(r) + " has "
                + std::to_string(rows[r].size()) + " columns, expected " + std::to_string(n_cols) + ".");
        }
    }

    result.data = num_array<T>(n_rows, n_cols, static_cast<T>(0));
    for(int64_t r = 0; r < n_rows; ++r){
        for(int64_t c = 0; c < n_cols; ++c){
            result.data.coeff(r, c) = rows[r][c];
        }
    }

    return result;
}
#ifndef YGORMATHIOCSV_DISABLE_ALL_SPECIALIZATIONS
    template csv_load_result<float > ReadNumArrayFromCSV(std::istream &, bool, csv_non_numeric_callback_t);
    template csv_load_result<double> ReadNumArrayFromCSV(std::istream &, bool, csv_non_numeric_callback_t);
#endif
