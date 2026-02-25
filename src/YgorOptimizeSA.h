//YgorOptimizeSA.h - Written by hal clark in 2026.
//
// Routines for numerical optimization using the Simulated Annealing (SA) algorithm.
//

#pragma once

#ifndef YGOR_OPTIMIZE_SA_HDR_GRD_H
#define YGOR_OPTIMIZE_SA_HDR_GRD_H

#include <cstdint>
#include <vector>
#include <functional>
#include <optional>
#include <chrono>
#include <string>

#include "YgorDefinitions.h"


// SA optimization result.
struct sa_result {
    std::vector<double> params;       // Optimized parameter values.
    double cost;                      // Final cost function value.
    int64_t iterations;               // Number of iterations performed.
    bool converged;                   // Whether the optimizer converged.
    std::string reason;               // Description of why the optimization terminated.
};


// Simulated Annealing optimizer class.
//
// Minimizes a scalar-valued cost function f(params) using the Simulated Annealing method.
//
// Usage example:
//
//   sa_optimizer opt;
//   opt.f = [](const std::vector<double> &p) -> double {
//       return (p[0] - 3.0) * (p[0] - 3.0) + (p[1] + 1.0) * (p[1] + 1.0);
//   };
//   opt.initial_params = {0.0, 0.0};
//   opt.max_iterations = 10000;      // Set at least one termination condition.
//   opt.abs_tol = 1.0e-8;            // Optional: absolute tolerance.
//   auto result = opt.optimize();
//
class sa_optimizer {
    public:
        // The cost function to minimize. Required.
        std::function<double(const std::vector<double> &)> f;

        // Initial parameter values. Required.
        std::vector<double> initial_params;

        // Optional termination conditions.
        // At least one must be set; if all are unset, optimize() will throw.
        // If not set, the corresponding condition is not checked.
        std::optional<int64_t> max_iterations;
        std::optional<double>  abs_tol;
        std::optional<double>  rel_tol;
        std::optional<std::chrono::steady_clock::duration> max_time;

        // Optional lower and upper bounds on parameters.
        // If set, the vectors must have the same size as initial_params.
        std::optional<std::vector<double>> lower_bounds;
        std::optional<std::vector<double>> upper_bounds;

        // Initial temperature for the annealing schedule.
        double initial_temperature = 1.0;

        // Multiplicative cooling factor applied each iteration. Must be in (0, 1).
        double cooling_rate = 0.995;

        // Scale of random perturbations applied to generate candidate points.
        double step_scale = 1.0;

        // Optional random seed for reproducibility.
        std::optional<uint64_t> seed;

        // Periodic logging interval (default 1 second).
        std::chrono::steady_clock::duration log_interval = std::chrono::seconds(1);

        // Run the SA optimization. Returns an sa_result.
        sa_result optimize() const;
};

#endif // YGOR_OPTIMIZE_SA_HDR_GRD_H
