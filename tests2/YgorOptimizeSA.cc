
#include <cmath>
#include <vector>
#include <chrono>
#include <limits>
#include <stdexcept>

#include <YgorOptimizeSA.h>

#include "doctest/doctest.h"


TEST_CASE( "sa_optimizer basic construction" ){

    SUBCASE("default-constructed optimizer has no cost function"){
        sa_optimizer opt;
        REQUIRE( !opt.f );
    }

    SUBCASE("default log interval is 1 second"){
        sa_optimizer opt;
        REQUIRE( opt.log_interval == std::chrono::seconds(1) );
    }

    SUBCASE("optional termination conditions are unset by default"){
        sa_optimizer opt;
        REQUIRE( !opt.max_iterations.has_value() );
        REQUIRE( !opt.abs_tol.has_value() );
        REQUIRE( !opt.rel_tol.has_value() );
        REQUIRE( !opt.max_time.has_value() );
    }

    SUBCASE("optional bounds are unset by default"){
        sa_optimizer opt;
        REQUIRE( !opt.lower_bounds.has_value() );
        REQUIRE( !opt.upper_bounds.has_value() );
    }

    SUBCASE("default SA-specific parameters"){
        sa_optimizer opt;
        REQUIRE( opt.initial_temperature == 1.0 );
        REQUIRE( opt.cooling_rate == 0.995 );
        REQUIRE( opt.step_scale == 1.0 );
        REQUIRE( !opt.seed.has_value() );
    }
}


TEST_CASE( "sa_optimizer input validation" ){

    SUBCASE("throws when cost function is not set"){
        sa_optimizer opt;
        opt.initial_params = {0.0};
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when initial_params is empty"){
        sa_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0]; };
        opt.max_iterations = 10;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when all termination conditions are unset"){
        sa_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when lower_bounds size does not match initial_params"){
        sa_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0, 2.0};
        opt.max_iterations = 10;
        opt.lower_bounds = std::vector<double>{0.0};
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when upper_bounds size does not match initial_params"){
        sa_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0, 2.0};
        opt.max_iterations = 10;
        opt.upper_bounds = std::vector<double>{10.0, 10.0, 10.0};
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when abs_tol is negative"){
        sa_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        opt.abs_tol = -1.0;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when abs_tol is zero"){
        sa_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        opt.abs_tol = 0.0;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when rel_tol is negative"){
        sa_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        opt.rel_tol = -1.0e-6;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when rel_tol is zero"){
        sa_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        opt.rel_tol = 0.0;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when max_time is negative"){
        sa_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        opt.max_time = std::chrono::seconds(-1);
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when lower_bounds exceed upper_bounds"){
        sa_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0, 2.0};
        opt.max_iterations = 10;
        opt.lower_bounds = std::vector<double>{5.0, 0.0};
        opt.upper_bounds = std::vector<double>{3.0, 10.0};
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when initial_temperature is not positive"){
        sa_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        opt.max_iterations = 10;
        opt.initial_temperature = 0.0;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when cooling_rate is not in (0, 1)"){
        sa_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        opt.max_iterations = 10;

        opt.cooling_rate = 0.0;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );

        opt.cooling_rate = 1.0;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }

    SUBCASE("throws when step_scale is not positive"){
        sa_optimizer opt;
        opt.f = [](const std::vector<double> &p) -> double { return p[0] * p[0]; };
        opt.initial_params = {1.0};
        opt.max_iterations = 10;
        opt.step_scale = 0.0;
        REQUIRE_THROWS_AS( opt.optimize(), std::invalid_argument );
    }
}


TEST_CASE( "sa_optimizer 1D quadratic" ){

    // Minimize f(x) = (x - 5)^2, minimum at x=5 with f=0.
    sa_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        return (p[0] - 5.0) * (p[0] - 5.0);
    };
    opt.initial_params = {0.0};
    opt.abs_tol = 1.0e-6;
    opt.max_iterations = 50000;
    opt.initial_temperature = 100.0;
    opt.step_scale = 1.0;
    opt.seed = 42;
    opt.log_interval = std::chrono::hours(1); // Suppress logging in tests.

    SUBCASE("converges near the correct minimum"){
        auto result = opt.optimize();
        REQUIRE( (result.converged || (std::abs(result.params[0] - 5.0) < 1.0 && result.cost < 1.0)) );
    }
}


TEST_CASE( "sa_optimizer 2D Rosenbrock" ){

    // Minimize f(x,y) = (1-x)^2 + 100*(y - x^2)^2, minimum at (1,1) with f=0.
    sa_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        const double x = p[0];
        const double y = p[1];
        return (1.0 - x) * (1.0 - x) + 100.0 * (y - x * x) * (y - x * x);
    };
    opt.initial_params = {-1.0, -1.0};
    opt.max_iterations = 500000;
    opt.initial_temperature = 100.0;
    opt.step_scale = 0.1;
    opt.seed = 42;
    opt.log_interval = std::chrono::hours(1);

    SUBCASE("approaches the correct minimum"){
        auto result = opt.optimize();
        // SA is a stochastic method; verify cost decreased significantly from initial value of 404.
        REQUIRE( result.cost < 100.0 );
    }
}


TEST_CASE( "sa_optimizer multi-dimensional sum of squares" ){

    // Minimize f(p) = sum_i (p_i - i)^2.
    sa_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        double sum = 0.0;
        for(size_t i = 0UL; i < p.size(); ++i){
            const double d = p[i] - static_cast<double>(i);
            sum += d * d;
        }
        return sum;
    };
    opt.initial_params = {10.0, 10.0, 10.0, 10.0};
    opt.abs_tol = 1.0e-6;
    opt.max_iterations = 100000;
    opt.initial_temperature = 100.0;
    opt.seed = 42;
    opt.log_interval = std::chrono::hours(1);

    SUBCASE("converges near the correct minimum"){
        auto result = opt.optimize();
        for(size_t i = 0UL; i < 4UL; ++i){
            REQUIRE( std::abs(result.params[i] - static_cast<double>(i)) < 1.0 );
        }
        REQUIRE( result.cost < 4.0 );
    }
}


TEST_CASE( "sa_optimizer termination conditions" ){

    sa_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        return (p[0] - 100.0) * (p[0] - 100.0);
    };
    opt.initial_params = {0.0};
    opt.seed = 42;
    opt.log_interval = std::chrono::hours(1);

    SUBCASE("max_iterations terminates early"){
        opt.max_iterations = 3;
        auto result = opt.optimize();
        REQUIRE( result.iterations == 3 );
    }

    SUBCASE("max_time terminates early"){
        opt.max_time = std::chrono::nanoseconds(1);
        auto result = opt.optimize();
        REQUIRE( result.reason == "reached maximum elapsed time" );
    }

    SUBCASE("relative tolerance terminates"){
        opt.rel_tol = 1.0e-4;
        opt.max_iterations = 200000;
        opt.initial_temperature = 100.0;
        opt.seed = 42;
        auto result = opt.optimize();
        REQUIRE( result.converged );
    }

    SUBCASE("no tolerances provided results in no convergence check"){
        opt.max_iterations = 1;
        auto result = opt.optimize();
        REQUIRE( !result.converged );
    }
}


TEST_CASE( "sa_optimizer configurable log interval" ){

    sa_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        return p[0] * p[0];
    };
    opt.initial_params = {5.0};
    opt.max_iterations = 100;
    opt.seed = 42;

    SUBCASE("log interval can be set to a custom duration"){
        opt.log_interval = std::chrono::milliseconds(500);
        auto result = opt.optimize();
        REQUIRE( result.iterations == 100 );
    }

    SUBCASE("log interval can be set to zero"){
        opt.log_interval = std::chrono::seconds(0);
        auto result = opt.optimize();
        REQUIRE( result.iterations == 100 );
    }
}


TEST_CASE( "sa_optimizer parameter bounds" ){

    // Minimize f(x) = (x - 5)^2, minimum at x=5.
    sa_optimizer opt;
    opt.f = [](const std::vector<double> &p) -> double {
        return (p[0] - 5.0) * (p[0] - 5.0);
    };
    opt.initial_params = {0.0};
    opt.abs_tol = 1.0e-8;
    opt.max_iterations = 50000;
    opt.initial_temperature = 100.0;
    opt.seed = 42;
    opt.log_interval = std::chrono::hours(1);

    SUBCASE("lower bound constrains the result"){
        // Minimum at x=5, but lower bound at 6 should clamp to 6.
        opt.lower_bounds = std::vector<double>{6.0};
        auto result = opt.optimize();
        REQUIRE( result.params[0] >= 6.0 - 0.1 );
    }

    SUBCASE("upper bound constrains the result"){
        // Minimum at x=5, but upper bound at 3 should clamp to 3.
        opt.upper_bounds = std::vector<double>{3.0};
        auto result = opt.optimize();
        REQUIRE( result.params[0] <= 3.0 + 0.1 );
    }

    SUBCASE("both bounds constrain the result"){
        opt.lower_bounds = std::vector<double>{2.0};
        opt.upper_bounds = std::vector<double>{3.0};
        auto result = opt.optimize();
        REQUIRE( result.params[0] >= 2.0 - 0.1 );
        REQUIRE( result.params[0] <= 3.0 + 0.1 );
    }

    SUBCASE("no bounds allows unconstrained optimization"){
        auto result = opt.optimize();
        REQUIRE( std::abs(result.params[0] - 5.0) < 1.0 );
    }
}
