//YgorOptimizeBFGS.h - Written by hal clark in 2026.
//
// Routines for numerical optimization using the Broyden-Fletcher-Goldfarb-Shanno (BFGS) algorithm.
//

#pragma once

#ifndef YGOR_OPTIMIZE_BFGS_HDR_GRD_H
#define YGOR_OPTIMIZE_BFGS_HDR_GRD_H

#include <cstdint>
#include <vector>
#include <functional>
#include <optional>
#include <chrono>
#include <string>

#include "YgorDefinitions.h"


// BFGS optimization result.
struct bfgs_result {
    std::vector<double> params;       // Optimized parameter values.
    double cost;                      // Final cost function value.
    int64_t iterations;               // Number of iterations performed.
    bool converged;                   // Whether the optimizer converged.
    std::string reason;               // Description of why the optimization terminated.
};


// BFGS optimizer class.
//
// Minimizes a scalar-valued cost function f(params) using the BFGS quasi-Newton method.
//
// Usage example:
//
//   bfgs_optimizer opt;
//   opt.f = [](const std::vector<double> &p) -> double {
//       return (p[0] - 3.0) * (p[0] - 3.0) + (p[1] + 1.0) * (p[1] + 1.0);
//   };
//   opt.initial_params = {0.0, 0.0};
//   auto result = opt.optimize();
//
class bfgs_optimizer {
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

        // Step size used for finite-difference gradient approximation.
        double fd_step = 1.0e-6;

        // Initial step size for line search.
        double line_search_step = 1.0;

        // Maximum number of steepest-descent fallbacks before resetting the inverse Hessian to identity.
        int64_t max_descent_fallbacks = 50;

        // Periodic logging interval (default 1 second).
        std::chrono::steady_clock::duration log_interval = std::chrono::seconds(1);

        // Run the BFGS optimization. Returns a bfgs_result.
        bfgs_result optimize() const;

    private:
        // Approximate the gradient of the cost function at params using central finite differences.
        std::vector<double> approx_gradient(const std::vector<double> &params) const;

        // Backtracking line search satisfying the Armijo condition.
        double line_search(const std::vector<double> &params,
                           const std::vector<double> &direction,
                           double current_cost,
                           const std::vector<double> &grad) const;
};

#endif // YGOR_OPTIMIZE_BFGS_HDR_GRD_H
