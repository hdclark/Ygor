
#include <cmath>
#include <vector>
#include <chrono>
#include <limits>
#include <stdexcept>

#include <YgorOptimizeBFGS.h>

#include "doctest/doctest.h"


TEST_CASE( "bfgs_optimizer basic construction" ){

    SUBCASE("default-constructed optimizer has no cost function"){
        bfgs_optimizer opt;
        REQUIRE( !opt.f );
    }

    SUBCASE("default log interval is 1 second"){
        bfgs_optimizer opt;
        REQUIRE( opt.log_interval == std::chrono::seconds(1) );
    }

    SUBCASE("optional termination conditions are unset by default"){
        bfgs_optimizer opt;
        REQUIRE( !opt.max_iterations.has_value() );
        REQUIRE( !opt.abs_tol.has_value() );
        REQUIRE( !opt.rel_tol.has_value() );
        REQUIRE( !opt.max_time.has_value() );
    }

    SUBCASE("optional bounds are unset by default"){
        bfgs_optimizer opt;
        REQUIRE( !opt.lower_bounds.has_value() );
        REQUIRE( !opt.upper_bounds.has_value() );
    }
}


TEST_CASE( "bfgs_optimizer input validation" ){

    SUBCASE("throws when cost function is not set"){
        bfgs_optimizer opt;
        opt.initial_params = {0.0};
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when initial_params is empty"){
        bfgs_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0]; };
        opt.max_iterations = 10;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when all termination conditions are unset"){
        bfgs_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when lower_bounds size does not match initial_params"){
        bfgs_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0, 2.0};
        opt.max_iterations = 10;
        opt.lower_bounds = std::vector<double>{0.0};
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when upper_bounds size does not match initial_params"){
        bfgs_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0, 2.0};
        opt.max_iterations = 10;
        opt.upper_bounds = std::vector<double>{10.0, 10.0, 10.0};
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }
}


TEST_CASE( "bfgs_optimizer 1D quadratic" ){

    // Minimize f(x) = (x - 5)^2, minimum at x=5 with f=0.
    bfgs_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        return (p[0] - 5.0) * (p[0] - 5.0);
    };
    opt.initial_params = {0.0};
    opt.abs_tol = 1.0e-10;
    opt.max_iterations = 200;
    opt.log_interval = std::chrono::hours(1); // Suppress logging in tests.

    SUBCASE("converges to the correct minimum"){
        auto result = opt.optimize();
        REQUIRE( result.converged );
        REQUIRE( std::abs(result.params[0] - 5.0) < 1.0e-4 );
        REQUIRE( result.cost < 1.0e-8 );
    }
}


TEST_CASE( "bfgs_optimizer 2D Rosenbrock" ){

    // Minimize f(x,y) = (1-x)^2 + 100*(y - x^2)^2, minimum at (1,1) with f=0.
    bfgs_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        const double x = p[0];
        const double y = p[1];
        return (1.0 - x) * (1.0 - x) + 100.0 * (y - x * x) * (y - x * x);
    };
    opt.initial_params = {-1.0, -1.0};
    opt.abs_tol = 1.0e-12;
    opt.max_iterations = 5000;
    opt.log_interval = std::chrono::hours(1);

    SUBCASE("converges to the correct minimum"){
        auto result = opt.optimize();
        REQUIRE( result.converged );
        REQUIRE( std::abs(result.params[0] - 1.0) < 1.0e-3 );
        REQUIRE( std::abs(result.params[1] - 1.0) < 1.0e-3 );
        REQUIRE( result.cost < 1.0e-5 );
    }
}


TEST_CASE( "bfgs_optimizer multi-dimensional sum of squares" ){

    // Minimize f(p) = sum_i (p_i - i)^2.
    bfgs_optimizer opt;
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
    opt.max_iterations = 500;
    opt.log_interval = std::chrono::hours(1);

    SUBCASE("converges to the correct minimum"){
        auto result = opt.optimize();
        REQUIRE( result.converged );
        for(size_t i = 0UL; i < 4UL; ++i){
            REQUIRE( std::abs(result.params[i] - static_cast<double>(i)) < 1.0e-3 );
        }
        REQUIRE( result.cost < 1.0e-6 );
    }
}


TEST_CASE( "bfgs_optimizer termination conditions" ){

    bfgs_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        return (p[0] - 100.0) * (p[0] - 100.0);
    };
    opt.initial_params = {0.0};
    opt.log_interval = std::chrono::hours(1);

    SUBCASE("max_iterations terminates early"){
        opt.max_iterations = 3;
        auto result = opt.optimize();
        REQUIRE( result.iterations == 3 );
        REQUIRE( result.reason == "reached maximum iterations" );
    }

    SUBCASE("max_time terminates early"){
        opt.max_time = std::chrono::nanoseconds(1);
        auto result = opt.optimize();
        REQUIRE( result.reason == "reached maximum elapsed time" );
    }

    SUBCASE("relative tolerance terminates"){
        opt.rel_tol = 1.0e-8;
        opt.max_iterations = 5000;
        auto result = opt.optimize();
        REQUIRE( result.converged );
        REQUIRE( result.reason == "relative tolerance satisfied" );
    }

    SUBCASE("no tolerances provided results in no convergence check"){
        // With only max_iterations set to a small value and nothing else,
        // the optimizer should stop at the iteration limit.
        opt.max_iterations = 1;
        auto result = opt.optimize();
        REQUIRE( !result.converged );
    }
}


TEST_CASE( "bfgs_optimizer configurable log interval" ){

    bfgs_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        return p[0] * p[0];
    };
    opt.initial_params = {5.0};
    opt.abs_tol = 1.0e-10;
    opt.max_iterations = 100;

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


TEST_CASE( "bfgs_optimizer parameter bounds" ){

    // Minimize f(x) = (x - 5)^2, minimum at x=5.
    bfgs_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        return (p[0] - 5.0) * (p[0] - 5.0);
    };
    opt.initial_params = {0.0};
    opt.abs_tol = 1.0e-10;
    opt.max_iterations = 500;
    opt.log_interval = std::chrono::hours(1);

    SUBCASE("lower bound constrains the result"){
        // Minimum at x=5, but lower bound at 6 should clamp to 6.
        opt.lower_bounds = std::vector<double>{6.0};
        auto result = opt.optimize();
        REQUIRE( result.params[0] >= 6.0 - 1.0e-9 );
    }

    SUBCASE("upper bound constrains the result"){
        // Minimum at x=5, but upper bound at 3 should clamp to 3.
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
        REQUIRE( std::abs(result.params[0] - 5.0) < 1.0e-4 );
    }
}
