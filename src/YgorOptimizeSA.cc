//YgorOptimizeSA.cc - A part of Ygor, 2026. Written by hal clark.

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
#include <random>

#include "YgorDefinitions.h"
#include "YgorOptimizeSA.h"
#include "YgorLog.h"


sa_result
sa_optimizer::optimize() const {
    if(!this->f){
        throw std::invalid_argument("sa_optimizer: cost function 'f' is not set");
    }
    const auto N = this->initial_params.size();
    if(N == 0UL){
        throw std::invalid_argument("sa_optimizer: 'initial_params' is empty");
    }

    // Validate configuration parameters.
    if(!(this->initial_temperature > 0.0)){
        throw std::invalid_argument("sa_optimizer: 'initial_temperature' must be positive");
    }
    if(!(this->cooling_rate > 0.0) || !(this->cooling_rate < 1.0)){
        throw std::invalid_argument("sa_optimizer: 'cooling_rate' must be in (0, 1)");
    }
    if(!(this->step_scale > 0.0)){
        throw std::invalid_argument("sa_optimizer: 'step_scale' must be positive");
    }
    if(!this->max_iterations.has_value()
    && !this->abs_tol.has_value()
    && !this->rel_tol.has_value()
    && !this->max_time.has_value()){
        throw std::invalid_argument("sa_optimizer: at least one termination condition must be set");
    }
    if(this->abs_tol.has_value()
    && !(this->abs_tol.value() > 0.0)){
        throw std::invalid_argument("sa_optimizer: 'abs_tol' must be positive");
    }
    if(this->rel_tol.has_value()
    && !(this->rel_tol.value() > 0.0)){
        throw std::invalid_argument("sa_optimizer: 'rel_tol' must be positive");
    }
    if(this->max_time.has_value()
    && (this->max_time.value() < std::chrono::steady_clock::duration::zero())){
        throw std::invalid_argument("sa_optimizer: 'max_time' must be non-negative");
    }
    if(this->lower_bounds.has_value()
    && (this->lower_bounds.value().size() != N)){
        throw std::invalid_argument("sa_optimizer: 'lower_bounds' size must match 'initial_params' size");
    }
    if(this->upper_bounds.has_value()
    && (this->upper_bounds.value().size() != N)){
        throw std::invalid_argument("sa_optimizer: 'upper_bounds' size must match 'initial_params' size");
    }
    if(this->lower_bounds.has_value()
    && this->upper_bounds.has_value()){
        for(size_t i = 0UL; i < N; ++i){
            if(this->lower_bounds.value()[i] > this->upper_bounds.value()[i]){
                throw std::invalid_argument("sa_optimizer: 'lower_bounds' must not exceed 'upper_bounds'");
            }
        }
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

    // Initialize the random number generator.
    std::mt19937_64 rng;
    if(this->seed.has_value()){
        rng.seed(this->seed.value());
    }else{
        rng.seed(std::random_device{}());
    }
    std::normal_distribution<double> normal_dist(0.0, 1.0);
    std::uniform_real_distribution<double> uniform_dist(0.0, 1.0);

    auto current_params = this->initial_params;
    clamp_params(current_params);
    double current_cost = this->f(current_params);

    // Track the best solution found overall.
    auto best_params = current_params;
    double best_cost = current_cost;
    double prev_best_cost = best_cost;

    // Track best cost periodically for stagnation-based convergence.
    double prev_check_cost = best_cost;
    bool any_improvement_found = false;
    const int64_t convergence_check_interval = 1000;

    double temperature = this->initial_temperature;

    sa_result result;
    result.iterations = 0;
    result.converged = false;

    const auto t_start = std::chrono::steady_clock::now();
    auto t_last_log = t_start;

    for(;;){
        const auto t_now = std::chrono::steady_clock::now();

        // Periodic status logging.
        if((t_now - t_last_log) >= this->log_interval){
            const auto elapsed = std::chrono::duration<double>(t_now - t_start);
            YLOGINFO("SA: iteration " << result.iterations
                     << ", best cost = " << best_cost
                     << ", temperature = " << temperature
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

        // Generate a candidate by perturbing the current parameters.
        std::vector<double> candidate(N);
        for(size_t i = 0UL; i < N; ++i){
            candidate[i] = current_params[i] + this->step_scale * temperature * normal_dist(rng);
        }
        clamp_params(candidate);

        const double candidate_cost = this->f(candidate);
        const double delta_cost = candidate_cost - current_cost;

        // Metropolis acceptance criterion.
        if(delta_cost < 0.0 || uniform_dist(rng) < std::exp(-delta_cost / temperature)){
            current_params = candidate;
            current_cost = candidate_cost;
        }

        // Update the best solution found overall.
        if(current_cost < best_cost){
            prev_best_cost = best_cost;
            best_params = current_params;
            best_cost = current_cost;
            any_improvement_found = true;

            // Check convergence using absolute tolerance (on the best cost improvement).
            if(this->abs_tol.has_value()){
                const double delta = std::abs(best_cost - prev_best_cost);
                if(delta < this->abs_tol.value()){
                    ++(result.iterations);
                    result.converged = true;
                    result.reason = "absolute tolerance satisfied";
                    break;
                }
            }

            // Check convergence using relative tolerance (on the best cost improvement).
            if(this->rel_tol.has_value()){
                const double delta = std::abs(best_cost - prev_best_cost);
                const double scale = std::max(std::abs(prev_best_cost), 1.0);
                if((delta / scale) < this->rel_tol.value()){
                    ++(result.iterations);
                    result.converged = true;
                    result.reason = "relative tolerance satisfied";
                    break;
                }
            }
        }

        // Periodic stagnation-based convergence check.
        // If the best cost has not improved significantly over a window of iterations,
        // declare convergence. This handles the case where the temperature has cooled
        // to the point that no new improvements are found.
        if(any_improvement_found
        && result.iterations > 0
        && (result.iterations % convergence_check_interval) == 0){
            if(this->abs_tol.has_value()){
                const double delta = std::abs(best_cost - prev_check_cost);
                if(delta < this->abs_tol.value()){
                    ++(result.iterations);
                    result.converged = true;
                    result.reason = "absolute tolerance satisfied";
                    break;
                }
            }
            if(this->rel_tol.has_value()){
                const double delta = std::abs(best_cost - prev_check_cost);
                const double scale = std::max(std::abs(prev_check_cost), 1.0);
                if((delta / scale) < this->rel_tol.value()){
                    ++(result.iterations);
                    result.converged = true;
                    result.reason = "relative tolerance satisfied";
                    break;
                }
            }
            prev_check_cost = best_cost;
        }

        // Cool the temperature.
        temperature *= this->cooling_rate;

        ++(result.iterations);
    }

    result.params = best_params;
    result.cost = best_cost;
    return result;
}
