
#include <cmath>
#include <vector>
#include <chrono>
#include <limits>
#include <stdexcept>

#include <YgorOptimizeLM.h>

#include "doctest/doctest.h"


TEST_CASE( "lm_optimizer basic construction" ){

    SUBCASE("default-constructed optimizer has no cost function"){
        lm_optimizer opt;
        REQUIRE( !opt.f );
    }

    SUBCASE("default log interval is 1 second"){
        lm_optimizer opt;
        REQUIRE( opt.log_interval == std::chrono::seconds(1) );
    }

    SUBCASE("optional termination conditions are unset by default"){
        lm_optimizer opt;
        REQUIRE( !opt.max_iterations.has_value() );
        REQUIRE( !opt.abs_tol.has_value() );
        REQUIRE( !opt.rel_tol.has_value() );
        REQUIRE( !opt.max_time.has_value() );
    }

    SUBCASE("optional bounds are unset by default"){
        lm_optimizer opt;
        REQUIRE( !opt.lower_bounds.has_value() );
        REQUIRE( !opt.upper_bounds.has_value() );
    }

    SUBCASE("default fd_step is 1e-6"){
        lm_optimizer opt;
        REQUIRE( opt.fd_step == 1.0e-6 );
    }

    SUBCASE("default initial_lambda is 1e-3"){
        lm_optimizer opt;
        REQUIRE( opt.initial_lambda == 1.0e-3 );
    }

    SUBCASE("default lambda_increase_factor is 10.0"){
        lm_optimizer opt;
        REQUIRE( opt.lambda_increase_factor == 10.0 );
    }

    SUBCASE("default lambda_decrease_factor is 0.1"){
        lm_optimizer opt;
        REQUIRE( opt.lambda_decrease_factor == 0.1 );
    }
}


TEST_CASE( "lm_optimizer input validation" ){

    SUBCASE("throws when cost function is not set"){
        lm_optimizer opt;
        opt.initial_params = {0.0};
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when initial_params is empty"){
        lm_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0]; };
        opt.max_iterations = 10;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when all termination conditions are unset"){
        lm_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when lower_bounds size does not match initial_params"){
        lm_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0, 2.0};
        opt.max_iterations = 10;
        opt.lower_bounds = std::vector<double>{0.0};
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when upper_bounds size does not match initial_params"){
        lm_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0, 2.0};
        opt.max_iterations = 10;
        opt.upper_bounds = std::vector<double>{10.0, 10.0, 10.0};
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when abs_tol is negative"){
        lm_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        opt.abs_tol = -1.0;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when abs_tol is zero"){
        lm_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        opt.abs_tol = 0.0;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when rel_tol is negative"){
        lm_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        opt.rel_tol = -1.0e-6;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when rel_tol is zero"){
        lm_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        opt.rel_tol = 0.0;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when max_time is negative"){
        lm_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        opt.max_time = std::chrono::seconds(-1);
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when lower_bounds exceed upper_bounds"){
        lm_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0, 2.0};
        opt.max_iterations = 10;
        opt.lower_bounds = std::vector<double>{5.0, 0.0};
        opt.upper_bounds = std::vector<double>{3.0, 10.0};
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }
}


TEST_CASE( "lm_optimizer 1D quadratic" ){
    lm_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        return (p[0] - 5.0) * (p[0] - 5.0);
    };
    opt.initial_params = {0.0};
    opt.abs_tol = 1.0e-6;
    opt.max_iterations = 1000;
    opt.log_interval = std::chrono::hours(1);

    SUBCASE("converges to the correct minimum"){
        auto result = opt.optimize();
        REQUIRE( result.converged );
        REQUIRE( std::abs(result.params[0] - 5.0) < 0.05 );
        REQUIRE( result.cost < 1.0e-3 );
    }
}


TEST_CASE( "lm_optimizer 2D Rosenbrock" ){
    lm_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        const double x = p[0];
        const double y = p[1];
        return (1.0 - x) * (1.0 - x) + 100.0 * (y - x * x) * (y - x * x);
    };
    opt.initial_params = {-1.0, -1.0};
    opt.abs_tol = 1.0e-10;
    opt.max_iterations = 10000;
    opt.log_interval = std::chrono::hours(1);

    SUBCASE("converges to the correct minimum"){
        auto result = opt.optimize();
        REQUIRE( result.converged );
        REQUIRE( std::abs(result.params[0] - 1.0) < 0.1 );
        REQUIRE( std::abs(result.params[1] - 1.0) < 0.1 );
        REQUIRE( result.cost < 0.01 );
    }
}


TEST_CASE( "lm_optimizer multi-dimensional sum of squares" ){
    lm_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        double sum = 0.0;
        for(size_t i = 0UL; i < p.size(); ++i){
            const double d = p[i] - static_cast<double>(i);
            sum += d * d;
        }
        return sum;
    };
    opt.initial_params = {10.0, 10.0, 10.0, 10.0};
    opt.abs_tol = 1.0e-10;
    opt.max_iterations = 2000;
    opt.log_interval = std::chrono::hours(1);

    SUBCASE("approaches the correct minimum"){
        auto result = opt.optimize();
        for(size_t i = 0UL; i < 4UL; ++i){
            REQUIRE( std::abs(result.params[i] - static_cast<double>(i)) < 0.01 );
        }
        REQUIRE( result.cost < 1.0e-3 );
    }
}


TEST_CASE( "lm_optimizer termination conditions" ){
    lm_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        return (p[0] - 100.0) * (p[0] - 100.0);
    };
    opt.initial_params = {0.0};
    opt.log_interval = std::chrono::hours(1);

    SUBCASE("max_iterations terminates early"){
        opt.max_iterations = 3;
        auto result = opt.optimize();
        REQUIRE( result.iterations <= 3 );
        REQUIRE( result.reason == "reached maximum iterations" );
    }

    SUBCASE("max_time terminates early"){
        opt.max_time = std::chrono::nanoseconds(1);
        auto result = opt.optimize();
        REQUIRE( result.reason == "reached maximum elapsed time" );
    }

    SUBCASE("relative tolerance terminates"){
        opt.rel_tol = 1.0e-4;
        opt.max_iterations = 10000;
        auto result = opt.optimize();
        REQUIRE( result.converged );
        REQUIRE( result.reason == "relative tolerance satisfied" );
    }

    SUBCASE("no tolerances provided results in no convergence check"){
        opt.max_iterations = 1;
        auto result = opt.optimize();
        REQUIRE( !result.converged );
    }
}


TEST_CASE( "lm_optimizer configurable log interval" ){
    lm_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        return p[0] * p[0];
    };
    opt.initial_params = {5.0};
    opt.abs_tol = 1.0e-6;
    opt.max_iterations = 1000;

    SUBCASE("log interval can be set to a custom duration"){
        opt.log_interval = std::chrono::milliseconds(500);
        auto result = opt.optimize();
        REQUIRE( result.converged );
    }

    SUBCASE("log interval can be set to zero"){
        opt.log_interval = std::chrono::seconds(0);
        auto result = opt.optimize();
        REQUIRE( result.converged );
    }
}


TEST_CASE( "lm_optimizer parameter bounds" ){
    lm_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        return (p[0] - 5.0) * (p[0] - 5.0);
    };
    opt.initial_params = {0.0};
    opt.abs_tol = 1.0e-6;
    opt.max_iterations = 1000;
    opt.log_interval = std::chrono::hours(1);

    SUBCASE("lower bound constrains the result"){
        opt.lower_bounds = std::vector<double>{6.0};
        auto result = opt.optimize();
        REQUIRE( result.params[0] >= 6.0 - 1.0e-9 );
    }

    SUBCASE("upper bound constrains the result"){
        opt.upper_bounds = std::vector<double>{3.0};
        auto result = opt.optimize();
        REQUIRE( result.params[0] <= 3.0 + 1.0e-9 );
    }

    SUBCASE("both bounds constrain the result"){
        opt.lower_bounds = std::vector<double>{2.0};
        opt.upper_bounds = std::vector<double>{3.0};
        auto result = opt.optimize();
        REQUIRE( result.params[0] >= 2.0 - 1.0e-9 );
        REQUIRE( result.params[0] <= 3.0 + 1.0e-9 );
    }

    SUBCASE("no bounds allows unconstrained optimization"){
        auto result = opt.optimize();
        REQUIRE( result.converged );
        REQUIRE( std::abs(result.params[0] - 5.0) < 0.05 );
    }

    SUBCASE("finite-difference steps respect bounds at active constraints"){
        lm_optimizer bnd_opt;
        bnd_opt.f = [](const std::vector<double> &p) -> double {
            const double x = p[0];
            if(x < 0.0) return std::numeric_limits<double>::infinity();
            const double d = std::sqrt(x) - 2.0;
            return d * d;
        };
        bnd_opt.initial_params = {0.0};
        bnd_opt.lower_bounds = std::vector<double>{0.0};
        bnd_opt.abs_tol = 1.0e-10;
        bnd_opt.max_iterations = 1000;
        bnd_opt.log_interval = std::chrono::hours(1);

        auto result = bnd_opt.optimize();
        REQUIRE( result.converged );
        REQUIRE( std::abs(result.params[0] - 4.0) < 0.05 );
    }
}
