//YgorMathIOCSV.h - Written by hal clark in 2026.
//
// Routines for reading tabular CSV/TSV data into num_array.
//

#pragma once

#ifndef YGOR_MATH_IO_CSV_HDR_GRD_H
#define YGOR_MATH_IO_CSV_HDR_GRD_H

#include <cstdint>
#include <functional>
#include <istream>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include "YgorDefinitions.h"
#include "YgorMath.h"


// Result of loading a CSV/TSV file into a num_array.
//
// In addition to the numeric matrix, this structure provides metadata about
// non-numeric values that were mapped to integers by the default callback.
//
template <class T>
struct csv_load_result {
    num_array<T> data;

    // Mapping from each distinct non-numeric string to its assigned integer value.
    // Only populated by the default non-numeric callback.
    std::map<std::string, int64_t> string_to_int;

    // Reverse mapping: for each assigned integer, a list of (row, col) locations
    // in the matrix where that mapped value appears.
    std::map<int64_t, std::vector<std::pair<int64_t, int64_t>>> int_to_locations;
};


// Callback type for converting a non-numeric token to a numeric value.
//
// Parameters:
//   token: the raw (whitespace-trimmed) cell text that could not be parsed as a number.
//   row: the 0-based row index of the cell.
//   col: the 0-based column index of the cell.
//
// Returns: the numeric value to store in the matrix.
//
using csv_non_numeric_callback_t = std::function<double(const std::string &token, int64_t row, int64_t col)>;


// Read a CSV or TSV stream into a num_array.
//
// The delimiter is auto-detected: if the first non-empty line (header or data) contains
// a tab character the delimiter is tab; otherwise comma.
//
// Parameters:
//   is: Input stream to read from.
//   has_header: If true, the first non-empty line is treated as a header and skipped.
//   non_numeric_cb: Optional callback for non-numeric tokens. If not provided, the
//       default callback maps:
//         - "nan" / "NaN" / "NAN" (case insensitive) and empty cells to quiet_NaN.
//         - "inf" / "+inf" / "-inf" (case insensitive) to C++ positive/negative infinity.
//         - Any other non-numeric token (including punctuation/quoted strings, case
//           sensitive) to a distinct integer, with the mapping stored in the returned
//           csv_load_result.
// Returns:
//   A csv_load_result<T> containing the loaded num_array and any string-to-int mappings.
//
// Throws:
//   std::runtime_error if the stream contains no data rows, or if rows have inconsistent
//   column counts.
//
template <class T>
csv_load_result<T>
ReadNumArrayFromCSV(std::istream &is,
                    bool has_header = false,
                    csv_non_numeric_callback_t non_numeric_cb = {});


#endif // YGOR_MATH_IO_CSV_HDR_GRD_H
