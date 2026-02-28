//YgorOptimizeLM.h - Written by hal clark in 2026.
//
// Routines for numerical optimization using the Levenberg-Marquardt (LM) algorithm.
//

#pragma once

#ifndef YGOR_OPTIMIZE_LM_HDR_GRD_H
#define YGOR_OPTIMIZE_LM_HDR_GRD_H

#include <cstdint>
#include <vector>
#include <functional>
#include <optional>
#include <chrono>
#include <string>

#include "YgorDefinitions.h"


// Levenberg-Marquardt optimization result.
struct lm_result {
    std::vector<double> params;       // Optimized parameter values.
    double cost;                      // Final cost function value.
    int64_t iterations;               // Number of iterations performed.
    bool converged;                   // Whether the optimizer converged.
    std::string reason;               // Description of why the optimization terminated.
};


// Levenberg-Marquardt optimizer class.
//
// Minimizes a scalar-valued cost function f(params) using the Levenberg-Marquardt method.
// The algorithm interpolates between gradient descent and Gauss-Newton by maintaining a
// damping parameter lambda. The approximate Hessian is formed using central finite differences,
// and parameter updates are computed by solving (H + lambda * I) * delta = -gradient.
//
// References:
//   - Levenberg K. A method for the solution of certain non-linear problems in least squares.
//     Q Appl Math. 1944;2(2):164-168.
//   - Marquardt DW. An algorithm for least-squares estimation of nonlinear parameters.
//     SIAM J Appl Math. 1963;11(2):431-441.
//   - Moré JJ. The Levenberg-Marquardt algorithm: implementation and theory.
//     In: Numerical Analysis, Dundee 1977. Springer; 1978:105-116.
//
// Usage example:
//
//   lm_optimizer opt;
//   opt.f = [](const std::vector<double> &p) -> double {
//       return (p[0] - 3.0) * (p[0] - 3.0) + (p[1] + 1.0) * (p[1] + 1.0);
//   };
//   opt.initial_params = {0.0, 0.0};
//   opt.max_iterations = 100;       // Set at least one termination condition.
//   opt.abs_tol = 1.0e-8;           // Optional: absolute tolerance.
//   auto result = opt.optimize();
//
class lm_optimizer {
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

        // Initial damping parameter.
        double initial_lambda = 1.0e-3;

        // Factor by which lambda is increased on a rejected step.
        double lambda_increase_factor = 10.0;

        // Factor by which lambda is multiplied on an accepted step (should be < 1).
        double lambda_decrease_factor = 0.1;

        // Periodic logging interval (default 1 second).
        std::chrono::steady_clock::duration log_interval = std::chrono::seconds(1);

        // Run the Levenberg-Marquardt optimization. Returns an lm_result.
        lm_result optimize() const;

    private:
        // Clamp parameter vector to configured bounds, if present.
        void clamp_to_bounds(std::vector<double> &params) const;

        // Approximate the gradient of the cost function at params using central finite differences.
        std::vector<double> approx_gradient(const std::vector<double> &params) const;

        // Approximate the Hessian of the cost function at params using central finite differences.
        // The caller provides the current cost f(params) to avoid a redundant function evaluation,
        // since the optimizer already tracks this value and f may be expensive to compute.
        std::vector<std::vector<double>> approx_hessian(const std::vector<double> &params,
                                                        double f0) const;
};

#endif // YGOR_OPTIMIZE_LM_HDR_GRD_H
