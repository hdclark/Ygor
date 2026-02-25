//YgorOptimizeLM.cc - A part of Ygor, 2026. Written by hal clark.

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
#include "YgorOptimizeLM.h"
#include "YgorLog.h"


std::vector<double>
lm_optimizer::approx_gradient(const std::vector<double> &params) const {
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


lm_result
lm_optimizer::optimize() const {
    if(!this->f){
        throw std::invalid_argument("lm_optimizer: cost function 'f' is not set");
    }
    const auto N = this->initial_params.size();
    if(N == 0UL){
        throw std::invalid_argument("lm_optimizer: 'initial_params' is empty");
    }

    // Validate configuration parameters.
    if(!(this->fd_step > 0.0)){
        throw std::invalid_argument("lm_optimizer: 'fd_step' must be positive");
    }
    if(!this->max_iterations.has_value()
    && !this->abs_tol.has_value()
    && !this->rel_tol.has_value()
    && !this->max_time.has_value()){
        throw std::invalid_argument("lm_optimizer: at least one termination condition must be set");
    }
    if(this->abs_tol.has_value()
    && !(this->abs_tol.value() > 0.0)){
        throw std::invalid_argument("lm_optimizer: 'abs_tol' must be positive");
    }
    if(this->rel_tol.has_value()
    && !(this->rel_tol.value() > 0.0)){
        throw std::invalid_argument("lm_optimizer: 'rel_tol' must be positive");
    }
    if(this->max_time.has_value()
    && (this->max_time.value() < std::chrono::steady_clock::duration::zero())){
        throw std::invalid_argument("lm_optimizer: 'max_time' must be non-negative");
    }
    if(this->lower_bounds.has_value()
    && (this->lower_bounds.value().size() != N)){
        throw std::invalid_argument("lm_optimizer: 'lower_bounds' size must match 'initial_params' size");
    }
    if(this->upper_bounds.has_value()
    && (this->upper_bounds.value().size() != N)){
        throw std::invalid_argument("lm_optimizer: 'upper_bounds' size must match 'initial_params' size");
    }
    if(this->lower_bounds.has_value()
    && this->upper_bounds.has_value()){
        for(size_t i = 0UL; i < N; ++i){
            if(this->lower_bounds.value()[i] > this->upper_bounds.value()[i]){
                throw std::invalid_argument("lm_optimizer: 'lower_bounds' must not exceed 'upper_bounds'");
            }
        }
    }
    if( !(this->initial_lambda > 0.0) ){
        throw std::invalid_argument("lm_optimizer: 'initial_lambda' must be positive");
    }
    if( !(this->lambda_increase_factor > 1.0) ){
        throw std::invalid_argument("lm_optimizer: 'lambda_increase_factor' must be greater than 1.0");
    }
    if(!(this->lambda_decrease_factor > 0.0) || !(this->lambda_decrease_factor < 1.0)){
        throw std::invalid_argument("lm_optimizer: 'lambda_decrease_factor' must be within (0,1)");
    }

    // Helper to clamp parameters to optional bounds.
    const auto clamp_params = [&](std::vector<double> &p){
        if(this->lower_bounds.has_value()){
            for(size_t i = 0UL; i < N; ++i){
                p[i] = std::max(p[i], this->lower_bounds.value()[i]);
            }
        }
        if(this->upper_bounds.has_value()){
            for(size_t i = 0UL; i < N; ++i){
                p[i] = std::min(p[i], this->upper_bounds.value()[i]);
            }
        }
    };

    auto params = this->initial_params;
    clamp_params(params);
    double cost = this->f(params);
    auto grad = this->approx_gradient(params);

    double lambda = this->initial_lambda;

    lm_result result;
    result.iterations = 0;
    result.converged = false;

    const auto t_start = std::chrono::steady_clock::now();
    auto t_last_log = t_start;

    for(;;){
        const auto t_now = std::chrono::steady_clock::now();

        // Periodic status logging.
        if((t_now - t_last_log) >= this->log_interval){
            const auto elapsed = std::chrono::duration<double>(t_now - t_start);
            YLOGINFO("LM: iteration " << result.iterations
                     << ", cost = " << cost
                     << ", lambda = " << lambda
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

        // Build the approximate Hessian H = grad * grad^T + lambda * I.
        std::vector<std::vector<double>> H(N, std::vector<double>(N, 0.0));
        for(size_t i = 0UL; i < N; ++i){
            for(size_t j = 0UL; j < N; ++j){
                H[i][j] = grad[i] * grad[j];
            }
            H[i][i] += lambda;
        }

        // Solve (H) * delta = -grad using Cholesky decomposition.
        // Since H is symmetric positive-definite (due to lambda * I), Cholesky is appropriate.
        std::vector<std::vector<double>> L(N, std::vector<double>(N, 0.0));
        bool cholesky_ok = true;
        for(size_t i = 0UL; i < N; ++i){
            for(size_t j = 0UL; j <= i; ++j){
                double sum = H[i][j];
                for(size_t k = 0UL; k < j; ++k){
                    sum -= L[i][k] * L[j][k];
                }
                if(i == j){
                    if(sum <= 0.0){
                        cholesky_ok = false;
                        break;
                    }
                    L[i][j] = std::sqrt(sum);
                }else{
                    L[i][j] = sum / L[j][j];
                }
            }
            if(!cholesky_ok) break;
        }

        std::vector<double> delta(N, 0.0);
        if(cholesky_ok){
            // Forward substitution: L * y = -grad.
            std::vector<double> y(N, 0.0);
            for(size_t i = 0UL; i < N; ++i){
                double sum = -grad[i];
                for(size_t k = 0UL; k < i; ++k){
                    sum -= L[i][k] * y[k];
                }
                y[i] = sum / L[i][i];
            }
            // Back substitution: L^T * delta = y.
            for(size_t i_plus = N; i_plus > 0UL; --i_plus){
                const size_t i = i_plus - 1UL;
                double sum = y[i];
                for(size_t k = i + 1UL; k < N; ++k){
                    sum -= L[k][i] * delta[k];
                }
                delta[i] = sum / L[i][i];
            }
        }else{
            // Fallback to steepest descent if Cholesky fails.
            for(size_t i = 0UL; i < N; ++i){
                delta[i] = -grad[i] / lambda;
            }
        }

        // Compute trial parameters.
        std::vector<double> trial(N);
        for(size_t i = 0UL; i < N; ++i){
            trial[i] = params[i] + delta[i];
        }
        clamp_params(trial);

        const double trial_cost = this->f(trial);

        if(trial_cost < cost){
            // Accept the step and decrease lambda.
            const double prev_cost = cost;
            params = trial;
            cost = trial_cost;
            grad = this->approx_gradient(params);
            lambda *= this->lambda_decrease_factor;

            ++(result.iterations);

            // Check convergence using absolute tolerance (on the cost change).
            if(this->abs_tol.has_value()){
                const double dc = std::abs(prev_cost - cost);
                if(dc < this->abs_tol.value()){
                    result.converged = true;
                    result.reason = "absolute tolerance satisfied";
                    break;
                }
            }

            // Check convergence using relative tolerance (on the cost change relative to the current cost).
            if(this->rel_tol.has_value()){
                const double dc = std::abs(prev_cost - cost);
                const double scale = std::max(std::abs(prev_cost), 1.0);
                if((dc / scale) < this->rel_tol.value()){
                    result.converged = true;
                    result.reason = "relative tolerance satisfied";
                    break;
                }
            }
        }else{
            // Reject the step and increase lambda.
            lambda *= this->lambda_increase_factor;
            ++(result.iterations);
        }
    }

    result.params = params;
    result.cost = cost;
    return result;
}
