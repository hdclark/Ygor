//YgorOptimizeBFGS.cc - A part of Ygor, 2026. Written by hal clark.

#include <cmath>
#include <cstdint>
#include <vector>
#include <functional>
#include <optional>
#include <chrono>
#include <string>
#include <sstream>
#include <stdexcept>
#include <limits>
#include <numeric>
#include <algorithm>

#include "YgorDefinitions.h"
#include "YgorOptimizeBFGS.h"
#include "YgorLog.h"


std::vector<double>
bfgs_optimizer::approx_gradient(const std::vector<double> &params) const {
    const auto N = params.size();
    std::vector<double> grad(N, 0.0);
    for(size_t i = 0UL; i < N; ++i){
        auto p_fwd = params;
        auto p_bck = params;
        p_fwd[i] += this->fd_step;
        p_bck[i] -= this->fd_step;
        grad[i] = (this->f(p_fwd) - this->f(p_bck)) / (2.0 * this->fd_step);
    }
    return grad;
}


double
bfgs_optimizer::line_search(const std::vector<double> &params,
                            const std::vector<double> &direction,
                            double current_cost,
                            const std::vector<double> &grad) const {
    const double c1 = 1.0e-4;  // Armijo sufficient decrease parameter.
    const double rho = 0.5;    // Step contraction factor.
    double alpha = this->line_search_step;

    // Compute directional derivative: grad . direction.
    double dg = 0.0;
    for(size_t i = 0UL; i < grad.size(); ++i){
        dg += grad[i] * direction[i];
    }

    const auto N = params.size();
    std::vector<double> trial(N);
    for(int64_t k = 0; k < 60; ++k){
        for(size_t i = 0UL; i < N; ++i){
            trial[i] = params[i] + alpha * direction[i];
        }
        const double trial_cost = this->f(trial);
        if(trial_cost <= (current_cost + c1 * alpha * dg)){
            return alpha;
        }
        alpha *= rho;
    }
    return alpha;
}


bfgs_result
bfgs_optimizer::optimize() const {
    if(!this->f){
        throw std::invalid_argument("bfgs_optimizer: cost function 'f' is not set");
    }
    const auto N = this->initial_params.size();
    if(N == 0UL){
        throw std::invalid_argument("bfgs_optimizer: 'initial_params' is empty");
    }

    // Initialize the inverse Hessian approximation to identity.
    std::vector<std::vector<double>> H(N, std::vector<double>(N, 0.0));
    for(size_t i = 0UL; i < N; ++i){
        H[i][i] = 1.0;
    }

    auto params = this->initial_params;
    double cost = this->f(params);
    auto grad = this->approx_gradient(params);

    bfgs_result result;
    result.iterations = 0;
    result.converged = false;

    const auto t_start = std::chrono::steady_clock::now();
    auto t_last_log = t_start;

    for(;;){
        const auto t_now = std::chrono::steady_clock::now();

        // Periodic status logging.
        if((t_now - t_last_log) >= this->log_interval){
            const auto elapsed = std::chrono::duration<double>(t_now - t_start);
            YLOGINFO("BFGS: iteration " << result.iterations
                     << ", cost = " << cost
                     << ", elapsed = " << elapsed.count() << " s");
            t_last_log = t_now;
        }

        // Check termination conditions.
        if(this->max_iterations.has_value()
        && (result.iterations >= this->max_iterations.value())){
            result.reason = "reached maximum iterations";
            break;
        }
        if(this->max_time.has_value()
        && ((t_now - t_start) >= this->max_time.value())){
            result.reason = "reached maximum elapsed time";
            break;
        }

        // Compute search direction: d = -H * grad.
        std::vector<double> direction(N, 0.0);
        for(size_t i = 0UL; i < N; ++i){
            for(size_t j = 0UL; j < N; ++j){
                direction[i] -= H[i][j] * grad[j];
            }
        }

        // Line search.
        const double alpha = this->line_search(params, direction, cost, grad);

        // Update parameters.
        std::vector<double> s(N);
        for(size_t i = 0UL; i < N; ++i){
            s[i] = alpha * direction[i];
            params[i] += s[i];
        }

        const double new_cost = this->f(params);
        auto new_grad = this->approx_gradient(params);

        // Compute gradient difference y = new_grad - grad.
        std::vector<double> y(N);
        for(size_t i = 0UL; i < N; ++i){
            y[i] = new_grad[i] - grad[i];
        }

        // Check convergence using absolute tolerance (on the cost change).
        if(this->abs_tol.has_value()){
            const double delta = std::abs(new_cost - cost);
            if(delta < this->abs_tol.value()){
                cost = new_cost;
                grad = new_grad;
                ++(result.iterations);
                result.converged = true;
                result.reason = "absolute tolerance satisfied";
                break;
            }
        }

        // Check convergence using relative tolerance (on the cost change relative to the current cost).
        if(this->rel_tol.has_value()){
            const double delta = std::abs(new_cost - cost);
            const double scale = std::max(std::abs(cost), 1.0);
            if((delta / scale) < this->rel_tol.value()){
                cost = new_cost;
                grad = new_grad;
                ++(result.iterations);
                result.converged = true;
                result.reason = "relative tolerance satisfied";
                break;
            }
        }

        // BFGS inverse Hessian update.
        double ys = 0.0;
        for(size_t i = 0UL; i < N; ++i){
            ys += y[i] * s[i];
        }

        if(std::abs(ys) > 1.0e-30){
            // Compute H*y.
            std::vector<double> Hy(N, 0.0);
            for(size_t i = 0UL; i < N; ++i){
                for(size_t j = 0UL; j < N; ++j){
                    Hy[i] += H[i][j] * y[j];
                }
            }

            double yHy = 0.0;
            for(size_t i = 0UL; i < N; ++i){
                yHy += y[i] * Hy[i];
            }

            // H_new = H + ((s^T y + y^T H y) / (s^T y)^2) * s s^T - (1/(s^T y)) * (H y s^T + s y^T H)
            const double ys2 = ys * ys;
            for(size_t i = 0UL; i < N; ++i){
                for(size_t j = 0UL; j < N; ++j){
                    H[i][j] += ((ys + yHy) / ys2) * s[i] * s[j]
                              - (Hy[i] * s[j] + s[i] * Hy[j]) / ys;
                }
            }
        }

        cost = new_cost;
        grad = new_grad;
        ++(result.iterations);
    }

    result.params = params;
    result.cost = cost;
    return result;
}
