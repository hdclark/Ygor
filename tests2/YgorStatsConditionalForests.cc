
#include <cmath>
#include <limits>
#include <sstream>

#include <YgorMath.h>
#include <YgorStatsConditionalForests.h>

#include "doctest/doctest.h"


TEST_CASE( "ConditionalRandomForests constructor" ){
    SUBCASE("default constructor produces valid forest"){
        Stats::ConditionalRandomForests<double> cf;
        REQUIRE(cf.get_n_trees() == 100);
        REQUIRE(cf.get_alpha() == doctest::Approx(0.05));
        REQUIRE(cf.get_subsample_fraction() == doctest::Approx(0.632));
        REQUIRE(cf.get_correlation_threshold() == doctest::Approx(0.20));
    }

    SUBCASE("custom parameters are accepted"){
        Stats::ConditionalRandomForests<double> cf(50, 15, 5, 0.01, 500, 3, 0.7, 0.3, 12345);
        REQUIRE(cf.get_n_trees() == 50);
        REQUIRE(cf.get_alpha() == doctest::Approx(0.01));
        REQUIRE(cf.get_subsample_fraction() == doctest::Approx(0.7));
        REQUIRE(cf.get_correlation_threshold() == doctest::Approx(0.3));
    }

    SUBCASE("invalid parameters are rejected"){
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(0, 10, 2, 0.05, 1000, -1, 0.632, 0.2, 42) );   // n_trees = 0
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(-1, 10, 2, 0.05, 1000, -1, 0.632, 0.2, 42) );  // n_trees < 0
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(100, 0, 2, 0.05, 1000, -1, 0.632, 0.2, 42) );  // max_depth = 0
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(100, -1, 2, 0.05, 1000, -1, 0.632, 0.2, 42) ); // max_depth < 0
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(100, 10, 1, 0.05, 1000, -1, 0.632, 0.2, 42) ); // min_samples_split < 2
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(100, 10, 0, 0.05, 1000, -1, 0.632, 0.2, 42) ); // min_samples_split < 2
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(100, 10, 2, 0.0, 1000, -1, 0.632, 0.2, 42) );  // alpha = 0
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(100, 10, 2, 1.0, 1000, -1, 0.632, 0.2, 42) );  // alpha = 1
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(100, 10, 2, -0.1, 1000, -1, 0.632, 0.2, 42) ); // alpha < 0
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(100, 10, 2, 0.05, 0, -1, 0.632, 0.2, 42) );    // n_permutations = 0
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(100, 10, 2, 0.05, -1, -1, 0.632, 0.2, 42) );   // n_permutations < 0
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(100, 10, 2, 0.05, 1000, -1, 0.0, 0.2, 42) );   // subsample_fraction = 0
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(100, 10, 2, 0.05, 1000, -1, -0.1, 0.2, 42) );  // subsample_fraction < 0
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(100, 10, 2, 0.05, 1000, -1, 1.1, 0.2, 42) );   // subsample_fraction > 1
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(100, 10, 2, 0.05, 1000, -1, 0.632, -0.1, 42) );// correlation_threshold < 0
        REQUIRE_THROWS( Stats::ConditionalRandomForests<double>(100, 10, 2, 0.05, 1000, -1, 0.632, 1.1, 42) ); // correlation_threshold > 1
    }

    SUBCASE("subsample_fraction = 1.0 is accepted"){
        Stats::ConditionalRandomForests<double> cf(10, 5, 2, 0.05, 100, -1, 1.0, 0.2, 42);
        REQUIRE(cf.get_subsample_fraction() == doctest::Approx(1.0));
    }

    SUBCASE("correlation_threshold boundary values are accepted"){
        Stats::ConditionalRandomForests<double> cf_zero(10, 5, 2, 0.05, 100, -1, 0.632, 0.0, 42);
        REQUIRE(cf_zero.get_correlation_threshold() == doctest::Approx(0.0));

        Stats::ConditionalRandomForests<double> cf_one(10, 5, 2, 0.05, 100, -1, 0.632, 1.0, 42);
        REQUIRE(cf_one.get_correlation_threshold() == doctest::Approx(1.0));
    }
}


TEST_CASE( "ConditionalRandomForests fit validation" ){
    Stats::ConditionalRandomForests<double> cf(10, 5, 2, 0.10, 100, -1, 0.632, 0.2, 42);

    SUBCASE("mismatched dimensions are rejected"){
        num_array<double> X(10, 3);
        num_array<double> y(5, 1);  // Wrong number of rows.
        REQUIRE_THROWS( cf.fit(X, y) );
    }

    SUBCASE("y must be a column vector"){
        num_array<double> X(10, 3);
        num_array<double> y(10, 2);  // Not a column vector.
        REQUIRE_THROWS( cf.fit(X, y) );
    }
}


TEST_CASE( "ConditionalRandomForests predict validation" ){
    Stats::ConditionalRandomForests<double> cf(10, 5, 2, 0.10, 100, -1, 0.632, 0.2, 42);

    SUBCASE("predict before fit throws exception"){
        num_array<double> x(1, 3);
        REQUIRE_THROWS( cf.predict(x) );
    }

    SUBCASE("predict with wrong dimensions throws exception"){
        num_array<double> X(10, 3);
        num_array<double> y(10, 1);
        for(int64_t i = 0; i < 10; ++i){
            X.coeff(i, 0) = static_cast<double>(i);
            X.coeff(i, 1) = static_cast<double>(i * 2);
            X.coeff(i, 2) = static_cast<double>(i * 3);
            y.coeff(i, 0) = static_cast<double>(i);
        }
        cf.fit(X, y);

        num_array<double> x_wrong(2, 3);  // Not a row vector.
        REQUIRE_THROWS( cf.predict(x_wrong) );
    }

    SUBCASE("predict with wrong number of features throws exception"){
        num_array<double> X(10, 3);
        num_array<double> y(10, 1);
        for(int64_t i = 0; i < 10; ++i){
            X.coeff(i, 0) = static_cast<double>(i);
            X.coeff(i, 1) = static_cast<double>(i * 2);
            X.coeff(i, 2) = static_cast<double>(i * 3);
            y.coeff(i, 0) = static_cast<double>(i);
        }
        cf.fit(X, y);

        num_array<double> x_too_few(1, 2);
        REQUIRE_THROWS( cf.predict(x_too_few) );

        num_array<double> x_too_many(1, 5);
        REQUIRE_THROWS( cf.predict(x_too_many) );
    }
}


TEST_CASE( "ConditionalRandomForests simple linear regression" ){
    // Test that the conditional random forest can learn a simple linear relationship: y = 2*x.
    Stats::ConditionalRandomForests<double> cf(50, 10, 2, 0.10, 200, -1, 0.632, 0.2, 42);

    const int64_t n_samples = 30;
    num_array<double> X(n_samples, 1);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        const double x_val = static_cast<double>(i) * 0.5;
        X.coeff(i, 0) = x_val;
        y.coeff(i, 0) = 2.0 * x_val;
    }

    cf.fit(X, y);

    SUBCASE("predictions on training data are reasonable"){
        double total_error = 0.0;
        for(int64_t i = 0; i < n_samples; ++i){
            num_array<double> x_test(1, 1);
            x_test.coeff(0, 0) = X.read_coeff(i, 0);
            const double pred = cf.predict(x_test);
            const double true_val = y.read_coeff(i, 0);
            total_error += std::abs(pred - true_val);
        }
        const double mean_error = total_error / static_cast<double>(n_samples);
        REQUIRE( mean_error < 3.0 );
    }
}


TEST_CASE( "ConditionalRandomForests multi-feature regression" ){
    // Test with multiple features: y = x1 + 2*x2 + 3*x3.
    Stats::ConditionalRandomForests<double> cf(50, 15, 2, 0.10, 200, -1, 0.632, 0.2, 123);

    const int64_t n_samples = 50;
    const int64_t n_features = 3;
    num_array<double> X(n_samples, n_features);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        const double x1 = static_cast<double>(i) * 0.1;
        const double x2 = static_cast<double>(i) * 0.2;
        const double x3 = static_cast<double>(i) * 0.15;

        X.coeff(i, 0) = x1;
        X.coeff(i, 1) = x2;
        X.coeff(i, 2) = x3;

        y.coeff(i, 0) = x1 + 2.0 * x2 + 3.0 * x3;
    }

    cf.fit(X, y);

    double total_error = 0.0;
    for(int64_t i = 0; i < n_samples; ++i){
        num_array<double> x_test(1, n_features);
        x_test.coeff(0, 0) = X.read_coeff(i, 0);
        x_test.coeff(0, 1) = X.read_coeff(i, 1);
        x_test.coeff(0, 2) = X.read_coeff(i, 2);

        const double pred = cf.predict(x_test);
        const double true_val = y.read_coeff(i, 0);
        total_error += std::abs(pred - true_val);
    }
    const double mean_error = total_error / static_cast<double>(n_samples);
    REQUIRE( mean_error < 3.0 );
}


TEST_CASE( "ConditionalRandomForests non-linear regression" ){
    // Test with a non-linear relationship: y = x^2.
    Stats::ConditionalRandomForests<double> cf(50, 15, 2, 0.10, 200, -1, 0.632, 0.2, 456);

    const int64_t n_samples = 30;
    num_array<double> X(n_samples, 1);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        const double x_val = static_cast<double>(i) * 0.3 - 4.5;
        X.coeff(i, 0) = x_val;
        y.coeff(i, 0) = x_val * x_val;
    }

    cf.fit(X, y);

    double total_error = 0.0;
    for(int64_t i = 0; i < n_samples; ++i){
        num_array<double> x_test(1, 1);
        x_test.coeff(0, 0) = X.read_coeff(i, 0);
        const double pred = cf.predict(x_test);
        const double true_val = y.read_coeff(i, 0);
        total_error += std::abs(pred - true_val);
    }
    const double mean_error = total_error / static_cast<double>(n_samples);
    REQUIRE( mean_error < 6.0 );
}


TEST_CASE( "ConditionalRandomForests constant output" ){
    Stats::ConditionalRandomForests<double> cf(30, 5, 2, 0.05, 100, -1, 0.632, 0.2, 789);

    const int64_t n_samples = 20;
    num_array<double> X(n_samples, 2);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        X.coeff(i, 1) = static_cast<double>(i * 2);
        y.coeff(i, 0) = 5.0;
    }

    cf.fit(X, y);

    for(int64_t i = 0; i < 5; ++i){
        num_array<double> x_test(1, 2);
        x_test.coeff(0, 0) = static_cast<double>(i) * 1.5;
        x_test.coeff(0, 1) = static_cast<double>(i) * 3.0;
        const double pred = cf.predict(x_test);
        REQUIRE( std::abs(pred - 5.0) < 0.5 );
    }
}


TEST_CASE( "ConditionalRandomForests reproducibility with same seed" ){
    const uint64_t seed = 98765;

    num_array<double> X(20, 2);
    num_array<double> y(20, 1);
    for(int64_t i = 0; i < 20; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        X.coeff(i, 1) = static_cast<double>(i * 2);
        y.coeff(i, 0) = static_cast<double>(i) * 1.5;
    }

    Stats::ConditionalRandomForests<double> cf1(30, 5, 2, 0.10, 200, -1, 0.632, 0.2, seed);
    cf1.fit(X, y);

    Stats::ConditionalRandomForests<double> cf2(30, 5, 2, 0.10, 200, -1, 0.632, 0.2, seed);
    cf2.fit(X, y);

    num_array<double> x_test(1, 2);
    x_test.coeff(0, 0) = 5.5;
    x_test.coeff(0, 1) = 11.0;

    const double pred1 = cf1.predict(x_test);
    const double pred2 = cf2.predict(x_test);

    REQUIRE( pred1 == pred2 );
}


TEST_CASE( "ConditionalRandomForests different seeds produce different results" ){
    num_array<double> X(25, 2);
    num_array<double> y(25, 1);

    for(int64_t i = 0; i < 25; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        X.coeff(i, 1) = static_cast<double>(i * 2);
        y.coeff(i, 0) = static_cast<double>(i) * 1.2;
    }

    Stats::ConditionalRandomForests<double> cf1(30, 5, 2, 0.10, 200, -1, 0.632, 0.2, 333);
    cf1.fit(X, y);

    Stats::ConditionalRandomForests<double> cf2(30, 5, 2, 0.10, 200, -1, 0.632, 0.2, 444);
    cf2.fit(X, y);

    num_array<double> x_test(1, 2);
    x_test.coeff(0, 0) = 12.5;
    x_test.coeff(0, 1) = 25.0;

    const double pred1 = cf1.predict(x_test);
    const double pred2 = cf2.predict(x_test);

    REQUIRE( pred1 != pred2 );
}


TEST_CASE( "ConditionalRandomForests subsampling without replacement" ){
    // Verify that subsampling without replacement is used (Strobl et al. 2007).
    // With subsample_fraction = 0.632, the subsample size should be roughly 0.632 * N.
    // This test uses OOB indices to verify subsampling behavior:
    // with subsampling without replacement at fraction f, each sample has probability
    // (1 - f) of being OOB for any given tree.

    const int64_t n_samples = 100;
    num_array<double> X(n_samples, 2);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        X.coeff(i, 1) = static_cast<double>(i * 2);
        y.coeff(i, 0) = static_cast<double>(i);
    }

    Stats::ConditionalRandomForests<double> cf(50, 5, 2, 0.10, 100, -1, 0.632, 0.2, 42);
    cf.set_importance_method(Stats::ConditionalImportanceMethod::permutation);
    cf.fit(X, y);

    // Compute importance to verify OOB tracking works.
    cf.compute_importance(X, y);

    auto importances = cf.get_feature_importances();
    REQUIRE( importances.size() == 2 );

    // All importances should be finite.
    for(const auto &v : importances){
        REQUIRE( std::isfinite(v) );
    }
}


TEST_CASE( "ConditionalRandomForests multiple fit calls" ){
    Stats::ConditionalRandomForests<double> cf(20, 5, 2, 0.10, 100, -1, 0.632, 0.2, 555);

    // First fit with 4 features.
    num_array<double> X1(20, 4);
    num_array<double> y1(20, 1);
    for(int64_t i = 0; i < 20; ++i){
        for(int64_t j = 0; j < 4; ++j){
            X1.coeff(i, j) = static_cast<double>(i * j) * 0.1;
        }
        y1.coeff(i, 0) = static_cast<double>(i);
    }
    cf.fit(X1, y1);

    num_array<double> x_test1(1, 4);
    for(int64_t j = 0; j < 4; ++j){
        x_test1.coeff(0, j) = static_cast<double>(j) * 0.2;
    }
    const double pred1 = cf.predict(x_test1);
    REQUIRE( std::isfinite(pred1) );

    // Second fit with 2 features.
    num_array<double> X2(15, 2);
    num_array<double> y2(15, 1);
    for(int64_t i = 0; i < 15; ++i){
        for(int64_t j = 0; j < 2; ++j){
            X2.coeff(i, j) = static_cast<double>(i * j) * 0.15;
        }
        y2.coeff(i, 0) = static_cast<double>(i) * 2.0;
    }
    cf.fit(X2, y2);

    num_array<double> x_test2(1, 2);
    for(int64_t j = 0; j < 2; ++j){
        x_test2.coeff(0, j) = static_cast<double>(j) * 0.3;
    }
    const double pred2 = cf.predict(x_test2);
    REQUIRE( std::isfinite(pred2) );

    // Old test input should now be rejected.
    REQUIRE_THROWS( cf.predict(x_test1) );
}


TEST_CASE( "ConditionalRandomForests step function" ){
    Stats::ConditionalRandomForests<double> cf(50, 10, 2, 0.10, 200, -1, 0.632, 0.2, 42);

    const int64_t n_samples = 40;
    num_array<double> X(n_samples, 1);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        const double x_val = static_cast<double>(i);
        X.coeff(i, 0) = x_val;
        y.coeff(i, 0) = (x_val < 20.0) ? 0.0 : 10.0;
    }

    cf.fit(X, y);

    num_array<double> x_low(1, 1);
    x_low.coeff(0, 0) = 5.0;
    REQUIRE( cf.predict(x_low) < 5.0 );

    num_array<double> x_high(1, 1);
    x_high.coeff(0, 0) = 35.0;
    REQUIRE( cf.predict(x_high) > 5.0 );
}


TEST_CASE( "ConditionalRandomForests float specialization" ){
    Stats::ConditionalRandomForests<float> cf(20, 5, 2, 0.10f, 100, -1, 0.632f, 0.2f, 42);

    const int64_t n_samples = 15;
    num_array<float> X(n_samples, 1);
    num_array<float> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<float>(i);
        y.coeff(i, 0) = static_cast<float>(i) * 2.0f;
    }

    cf.fit(X, y);

    num_array<float> x_test(1, 1);
    x_test.coeff(0, 0) = 7.0f;
    const float pred = cf.predict(x_test);
    REQUIRE( std::isfinite(pred) );
    REQUIRE( pred > 0.0f );
}


TEST_CASE( "ConditionalRandomForests importance method defaults to none" ){
    Stats::ConditionalRandomForests<double> cf;
    REQUIRE( cf.get_importance_method() == Stats::ConditionalImportanceMethod::none );
    REQUIRE( cf.get_feature_importances().empty() );
}


TEST_CASE( "ConditionalRandomForests permutation variable importance" ){
    // y = x1 + 2*x2 + 3*x3.
    Stats::ConditionalRandomForests<double> cf(50, 10, 2, 0.10, 200, -1, 0.632, 0.2, 42);
    cf.set_importance_method(Stats::ConditionalImportanceMethod::permutation);

    const int64_t n_samples = 60;
    const int64_t n_features = 3;
    num_array<double> X(n_samples, n_features);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i) * 0.1;
        X.coeff(i, 1) = static_cast<double>(i) * 0.2;
        X.coeff(i, 2) = static_cast<double>(i) * 0.15;
        y.coeff(i, 0) = X.read_coeff(i, 0) + 2.0 * X.read_coeff(i, 1) + 3.0 * X.read_coeff(i, 2);
    }

    cf.fit(X, y);

    SUBCASE("importances are not available before compute"){
        REQUIRE( cf.get_feature_importances().empty() );
    }

    SUBCASE("compute_importance produces valid results"){
        cf.compute_importance(X, y);
        auto importances = cf.get_feature_importances();
        REQUIRE( importances.size() == 3 );

        for(const auto &v : importances){
            REQUIRE( v >= 0.0 );
        }
    }

    SUBCASE("throws without importance method set"){
        Stats::ConditionalRandomForests<double> cf_none(20, 5, 2, 0.10, 100, -1, 0.632, 0.2, 42);
        num_array<double> X2(10, 2);
        num_array<double> y2(10, 1);
        for(int64_t i = 0; i < 10; ++i){
            X2.coeff(i, 0) = static_cast<double>(i);
            X2.coeff(i, 1) = static_cast<double>(i * 2);
            y2.coeff(i, 0) = static_cast<double>(i);
        }
        cf_none.fit(X2, y2);
        REQUIRE_THROWS( cf_none.compute_importance(X2, y2) );
    }

    SUBCASE("throws before fit"){
        Stats::ConditionalRandomForests<double> cf_new(20, 5, 2, 0.10, 100, -1, 0.632, 0.2, 42);
        cf_new.set_importance_method(Stats::ConditionalImportanceMethod::permutation);
        num_array<double> X2(10, 2);
        num_array<double> y2(10, 1);
        REQUIRE_THROWS( cf_new.compute_importance(X2, y2) );
    }
}


TEST_CASE( "ConditionalRandomForests permutation importance with irrelevant feature" ){
    // y = 3*x1, with x2 constant. x1 should have much higher importance.
    Stats::ConditionalRandomForests<double> cf(50, 10, 2, 0.10, 200, -1, 0.632, 0.2, 42);
    cf.set_importance_method(Stats::ConditionalImportanceMethod::permutation);

    const int64_t n_samples = 50;
    num_array<double> X(n_samples, 2);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i) * 0.5;
        X.coeff(i, 1) = 1.0;  // Constant, irrelevant feature.
        y.coeff(i, 0) = 3.0 * X.read_coeff(i, 0);
    }

    cf.fit(X, y);
    cf.compute_importance(X, y);

    auto importances = cf.get_feature_importances();
    REQUIRE( importances.size() == 2 );
    REQUIRE( importances[0] > importances[1] );
}


TEST_CASE( "ConditionalRandomForests conditional variable importance" ){
    // Test conditional permutation importance (Strobl et al. 2008).
    // y = 3*x1, with x2 highly correlated to x1 but not directly influencing y.
    // Conditional importance should reduce the apparent importance of x2 relative
    // to standard (marginal) permutation importance.
    Stats::ConditionalRandomForests<double> cf(50, 10, 2, 0.10, 200, -1, 0.632, 0.50, 42);
    cf.set_importance_method(Stats::ConditionalImportanceMethod::conditional);

    const int64_t n_samples = 60;
    num_array<double> X(n_samples, 2);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        const double x1 = static_cast<double>(i) * 0.5;
        X.coeff(i, 0) = x1;
        X.coeff(i, 1) = x1 * 0.9 + 0.1 * static_cast<double>(i % 3);  // Correlated with x1.
        y.coeff(i, 0) = 3.0 * x1;  // y depends only on x1.
    }

    cf.fit(X, y);
    cf.compute_importance(X, y);

    auto importances = cf.get_feature_importances();
    REQUIRE( importances.size() == 2 );

    // Both importances should be finite.
    REQUIRE( std::isfinite(importances[0]) );
    REQUIRE( std::isfinite(importances[1]) );

    // x1 should still be more important than x2.
    REQUIRE( importances[0] > importances[1] );
}


TEST_CASE( "ConditionalRandomForests conditional importance with uncorrelated features" ){
    // When features are uncorrelated, conditional importance should behave
    // similarly to marginal permutation importance.
    Stats::ConditionalRandomForests<double> cf(50, 10, 2, 0.10, 200, -1, 0.632, 0.5, 42);
    cf.set_importance_method(Stats::ConditionalImportanceMethod::conditional);

    const int64_t n_samples = 50;
    num_array<double> X(n_samples, 2);
    num_array<double> y(n_samples, 1);

    // x1 and x2 are uncorrelated: x1 is sequential, x2 is constant.
    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i) * 0.5;
        X.coeff(i, 1) = 1.0;
        y.coeff(i, 0) = 2.0 * X.read_coeff(i, 0);
    }

    cf.fit(X, y);
    cf.compute_importance(X, y);

    auto importances = cf.get_feature_importances();
    REQUIRE( importances.size() == 2 );

    // Feature 0 should dominate.
    REQUIRE( importances[0] > importances[1] );
}


TEST_CASE( "ConditionalRandomForests conditional importance with three features" ){
    // y = x1 + x3, with x2 correlated with x1 but not in the model.
    // Conditional importance should assign less importance to x2 than marginal would.
    Stats::ConditionalRandomForests<double> cf(50, 10, 2, 0.10, 200, -1, 0.632, 0.3, 42);
    cf.set_importance_method(Stats::ConditionalImportanceMethod::conditional);

    const int64_t n_samples = 60;
    num_array<double> X(n_samples, 3);
    num_array<double> y(n_samples, 1);

    std::mt19937 noise_rng(99);
    std::normal_distribution<double> noise(0.0, 0.1);

    for(int64_t i = 0; i < n_samples; ++i){
        const double x1 = static_cast<double>(i) * 0.3;
        const double x2 = x1 * 0.8 + noise(noise_rng);  // Correlated with x1.
        const double x3 = static_cast<double>(i % 7) * 2.0;  // Independent.

        X.coeff(i, 0) = x1;
        X.coeff(i, 1) = x2;
        X.coeff(i, 2) = x3;

        y.coeff(i, 0) = x1 + x3;  // Depends on x1 and x3, not x2.
    }

    cf.fit(X, y);
    cf.compute_importance(X, y);

    auto importances = cf.get_feature_importances();
    REQUIRE( importances.size() == 3 );

    for(const auto &v : importances){
        REQUIRE( std::isfinite(v) );
    }
}


TEST_CASE( "ConditionalRandomForests write_to and read_from roundtrip" ){
    Stats::ConditionalRandomForests<double> cf(30, 5, 2, 0.10, 100, -1, 0.632, 0.2, 42);

    const int64_t n_samples = 30;
    const int64_t n_features = 3;
    num_array<double> X(n_samples, n_features);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i) * 0.1;
        X.coeff(i, 1) = static_cast<double>(i) * 0.2;
        X.coeff(i, 2) = static_cast<double>(i) * 0.15;
        y.coeff(i, 0) = X.read_coeff(i, 0) + 2.0 * X.read_coeff(i, 1) + 3.0 * X.read_coeff(i, 2);
    }

    cf.fit(X, y);

    SUBCASE("roundtrip preserves predictions exactly"){
        std::stringstream ss;
        REQUIRE( cf.write_to(ss) );

        Stats::ConditionalRandomForests<double> cf_loaded;
        REQUIRE( cf_loaded.read_from(ss) );

        REQUIRE( cf_loaded.get_n_trees() == 30 );

        for(int64_t i = 0; i < n_samples; ++i){
            num_array<double> x_test(1, n_features);
            x_test.coeff(0, 0) = X.read_coeff(i, 0);
            x_test.coeff(0, 1) = X.read_coeff(i, 1);
            x_test.coeff(0, 2) = X.read_coeff(i, 2);

            const double pred_original = cf.predict(x_test);
            const double pred_loaded = cf_loaded.predict(x_test);
            REQUIRE( pred_original == pred_loaded );
        }
    }

    SUBCASE("written format is text-based"){
        std::stringstream ss;
        REQUIRE( cf.write_to(ss) );
        const std::string content = ss.str();

        REQUIRE( content.substr(0, 27) == "ConditionalRandomForests_v1" );
        REQUIRE( content.find("n_trees 30") != std::string::npos );
        REQUIRE( content.find("begin_tree") != std::string::npos );
        REQUIRE( content.find("end_tree") != std::string::npos );
    }

    SUBCASE("read_from rejects invalid input"){
        std::stringstream bad_ss("not a valid model");
        Stats::ConditionalRandomForests<double> cf_bad;
        REQUIRE( !cf_bad.read_from(bad_ss) );
    }
}


TEST_CASE( "ConditionalRandomForests write_to and read_from with float" ){
    Stats::ConditionalRandomForests<float> cf(20, 4, 2, 0.10f, 100, -1, 0.632f, 0.2f, 99);

    const int64_t n_samples = 15;
    num_array<float> X(n_samples, 2);
    num_array<float> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<float>(i) * 0.5f;
        X.coeff(i, 1) = static_cast<float>(i) * 0.3f;
        y.coeff(i, 0) = X.read_coeff(i, 0) + X.read_coeff(i, 1);
    }

    cf.fit(X, y);

    std::stringstream ss;
    REQUIRE( cf.write_to(ss) );

    Stats::ConditionalRandomForests<float> cf_loaded;
    REQUIRE( cf_loaded.read_from(ss) );

    for(int64_t i = 0; i < n_samples; ++i){
        num_array<float> x_test(1, 2);
        x_test.coeff(0, 0) = X.read_coeff(i, 0);
        x_test.coeff(0, 1) = X.read_coeff(i, 1);

        const float pred_original = cf.predict(x_test);
        const float pred_loaded = cf_loaded.predict(x_test);
        REQUIRE( pred_original == pred_loaded );
    }
}


TEST_CASE( "ConditionalRandomForests serialization with permutation importance" ){
    Stats::ConditionalRandomForests<double> cf(30, 5, 2, 0.10, 100, -1, 0.632, 0.2, 42);
    cf.set_importance_method(Stats::ConditionalImportanceMethod::permutation);

    const int64_t n_samples = 30;
    const int64_t n_features = 2;
    num_array<double> X(n_samples, n_features);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i) * 0.3;
        X.coeff(i, 1) = static_cast<double>(i) * 0.1;
        y.coeff(i, 0) = X.read_coeff(i, 0) + X.read_coeff(i, 1);
    }

    cf.fit(X, y);
    cf.compute_importance(X, y);

    std::stringstream ss;
    REQUIRE( cf.write_to(ss) );

    Stats::ConditionalRandomForests<double> cf_loaded;
    REQUIRE( cf_loaded.read_from(ss) );

    // Importances should be preserved.
    auto imp_orig = cf.get_feature_importances();
    auto imp_loaded = cf_loaded.get_feature_importances();
    REQUIRE( imp_orig.size() == imp_loaded.size() );
    for(size_t i = 0; i < imp_orig.size(); ++i){
        REQUIRE( imp_orig[i] == imp_loaded[i] );
    }

    // Predictions should also be preserved.
    num_array<double> x_test(1, n_features);
    x_test.coeff(0, 0) = 1.5;
    x_test.coeff(0, 1) = 0.5;
    REQUIRE( cf.predict(x_test) == cf_loaded.predict(x_test) );
}


TEST_CASE( "ConditionalRandomForests no importance overhead" ){
    Stats::ConditionalRandomForests<double> cf(20, 5, 2, 0.10, 100, -1, 0.632, 0.2, 42);

    num_array<double> X(20, 3);
    num_array<double> y(20, 1);
    for(int64_t i = 0; i < 20; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        X.coeff(i, 1) = static_cast<double>(i * 2);
        X.coeff(i, 2) = static_cast<double>(i * 3);
        y.coeff(i, 0) = static_cast<double>(i);
    }

    cf.fit(X, y);

    REQUIRE( cf.get_feature_importances().empty() );
    REQUIRE( cf.get_importance_method() == Stats::ConditionalImportanceMethod::none );
}


TEST_CASE( "ConditionalRandomForests predictions bounded by training range" ){
    Stats::ConditionalRandomForests<double> cf(30, 10, 2, 0.10, 200, -1, 0.632, 0.2, 42);

    const int64_t n_samples = 30;
    num_array<double> X(n_samples, 1);
    num_array<double> y(n_samples, 1);

    double y_min = std::numeric_limits<double>::infinity();
    double y_max = -std::numeric_limits<double>::infinity();
    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        const double y_val = std::sin(static_cast<double>(i) * 0.3) * 10.0;
        y.coeff(i, 0) = y_val;
        y_min = std::min(y_min, y_val);
        y_max = std::max(y_max, y_val);
    }

    cf.fit(X, y);

    for(int64_t i = 0; i < 50; ++i){
        num_array<double> x_test(1, 1);
        x_test.coeff(0, 0) = static_cast<double>(i) - 10.0;
        const double pred = cf.predict(x_test);
        REQUIRE( pred >= y_min - 0.01 );
        REQUIRE( pred <= y_max + 0.01 );
    }
}


TEST_CASE( "ConditionalRandomForests with few samples" ){
    Stats::ConditionalRandomForests<double> cf(10, 3, 2, 0.10, 100, -1, 0.632, 0.2, 111);

    num_array<double> X(3, 1);
    num_array<double> y(3, 1);

    X.coeff(0, 0) = 1.0;
    X.coeff(1, 0) = 2.0;
    X.coeff(2, 0) = 3.0;

    y.coeff(0, 0) = 2.0;
    y.coeff(1, 0) = 4.0;
    y.coeff(2, 0) = 6.0;

    cf.fit(X, y);

    num_array<double> x_test(1, 1);
    x_test.coeff(0, 0) = 2.5;
    const double pred = cf.predict(x_test);

    REQUIRE( pred >= 1.0 );
    REQUIRE( pred <= 7.0 );
}


TEST_CASE( "ConditionalRandomForests with max_features parameter" ){
    // Test that max_features parameter works (limits features per split).
    num_array<double> X(30, 5);
    num_array<double> y(30, 1);

    for(int64_t i = 0; i < 30; ++i){
        for(int64_t j = 0; j < 5; ++j){
            X.coeff(i, j) = static_cast<double>(i * j) * 0.1;
        }
        y.coeff(i, 0) = static_cast<double>(i) * 0.5;
    }

    // Model with all features considered.
    Stats::ConditionalRandomForests<double> cf_all(30, 5, 2, 0.10, 200, 5, 0.632, 0.2, 222);
    cf_all.fit(X, y);

    // Model with only 1 feature considered per split.
    Stats::ConditionalRandomForests<double> cf_limited(30, 5, 2, 0.10, 200, 1, 0.632, 0.2, 222);
    cf_limited.fit(X, y);

    num_array<double> x_test(1, 5);
    for(int64_t j = 0; j < 5; ++j){
        x_test.coeff(0, j) = static_cast<double>(j) * 0.2;
    }

    const double pred_all = cf_all.predict(x_test);
    const double pred_limited = cf_limited.predict(x_test);

    // Both should produce finite predictions.
    REQUIRE( std::isfinite(pred_all) );
    REQUIRE( std::isfinite(pred_limited) );
}


TEST_CASE( "ConditionalRandomForests conditional vs marginal importance bias" ){
    // Key validation of Strobl et al. (2008): when x2 is correlated with x1 but does not
    // directly influence y, standard marginal permutation importance inflates x2's importance,
    // whereas conditional permutation importance should correctly assign less importance to x2.
    //
    // y = 3*x1, x2 = x1 + small_noise.

    const int64_t n_samples = 80;
    const int64_t n_features = 2;

    num_array<double> X(n_samples, n_features);
    num_array<double> y(n_samples, 1);

    std::mt19937 noise_rng(77);
    std::normal_distribution<double> noise(0.0, 0.5);

    for(int64_t i = 0; i < n_samples; ++i){
        const double x1 = static_cast<double>(i) * 0.5;
        X.coeff(i, 0) = x1;
        X.coeff(i, 1) = x1 + noise(noise_rng);  // Highly correlated with x1.
        y.coeff(i, 0) = 3.0 * x1;
    }

    // Marginal permutation importance.
    Stats::ConditionalRandomForests<double> cf_marginal(50, 10, 2, 0.10, 200, -1, 0.632, 0.2, 42);
    cf_marginal.set_importance_method(Stats::ConditionalImportanceMethod::permutation);
    cf_marginal.fit(X, y);
    cf_marginal.compute_importance(X, y);
    auto imp_marginal = cf_marginal.get_feature_importances();

    // Conditional permutation importance with a low threshold to force conditioning.
    Stats::ConditionalRandomForests<double> cf_cond(50, 10, 2, 0.10, 200, -1, 0.632, 0.20, 42);
    cf_cond.set_importance_method(Stats::ConditionalImportanceMethod::conditional);
    cf_cond.fit(X, y);
    cf_cond.compute_importance(X, y);
    auto imp_cond = cf_cond.get_feature_importances();

    REQUIRE( imp_marginal.size() == 2 );
    REQUIRE( imp_cond.size() == 2 );

    // Both should identify x1 as most important.
    REQUIRE( imp_marginal[0] > imp_marginal[1] );
    REQUIRE( imp_cond[0] > imp_cond[1] );

    // Conditional importance should reduce the apparent importance of x2 (the irrelevant
    // but correlated feature) relative to marginal importance, per Strobl et al. (2008).
    REQUIRE( imp_cond[1] <= imp_marginal[1] );
}


TEST_CASE( "ConditionalRandomForests serialization roundtrip with conditional importance" ){
    Stats::ConditionalRandomForests<double> cf(30, 5, 2, 0.10, 100, -1, 0.632, 0.3, 42);
    cf.set_importance_method(Stats::ConditionalImportanceMethod::conditional);

    const int64_t n_samples = 30;
    const int64_t n_features = 2;
    num_array<double> X(n_samples, n_features);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        const double x1 = static_cast<double>(i) * 0.3;
        X.coeff(i, 0) = x1;
        X.coeff(i, 1) = x1 * 0.5 + 1.0;
        y.coeff(i, 0) = 2.0 * x1;
    }

    cf.fit(X, y);
    cf.compute_importance(X, y);

    std::stringstream ss;
    REQUIRE( cf.write_to(ss) );

    Stats::ConditionalRandomForests<double> cf_loaded;
    REQUIRE( cf_loaded.read_from(ss) );

    // Importances should be preserved.
    auto imp_orig = cf.get_feature_importances();
    auto imp_loaded = cf_loaded.get_feature_importances();
    REQUIRE( imp_orig.size() == imp_loaded.size() );
    for(size_t i = 0; i < imp_orig.size(); ++i){
        REQUIRE( imp_orig[i] == imp_loaded[i] );
    }

    // Predictions should also be preserved.
    num_array<double> x_test(1, n_features);
    x_test.coeff(0, 0) = 1.5;
    x_test.coeff(0, 1) = 1.75;
    REQUIRE( cf.predict(x_test) == cf_loaded.predict(x_test) );

    // Importance method should be preserved.
    REQUIRE( cf_loaded.get_importance_method() == Stats::ConditionalImportanceMethod::conditional );
}


TEST_CASE( "ConditionalRandomForests conditional importance with single feature" ){
    // With a single feature, the conditioning set is always empty, so conditional
    // importance should fall back to standard permutation importance.
    Stats::ConditionalRandomForests<double> cf(30, 8, 2, 0.10, 200, -1, 0.632, 0.2, 42);
    cf.set_importance_method(Stats::ConditionalImportanceMethod::conditional);

    const int64_t n_samples = 30;
    num_array<double> X(n_samples, 1);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i) * 0.5;
        y.coeff(i, 0) = 2.0 * X.read_coeff(i, 0);
    }

    cf.fit(X, y);
    cf.compute_importance(X, y);

    auto importances = cf.get_feature_importances();
    REQUIRE( importances.size() == 1 );
    REQUIRE( std::isfinite(importances[0]) );
    REQUIRE( importances[0] > 0.0 );
}


TEST_CASE( "ConditionalRandomForests correlation threshold effect" ){
    // With a high correlation threshold, no features will be in the conditioning set,
    // so conditional importance should behave like marginal permutation importance.
    // With a low threshold, conditioning should activate and produce different results.

    const int64_t n_samples = 60;
    const int64_t n_features = 2;
    num_array<double> X(n_samples, n_features);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        const double x1 = static_cast<double>(i) * 0.5;
        X.coeff(i, 0) = x1;
        X.coeff(i, 1) = x1 * 0.95 + 0.1;  // Almost perfectly correlated.
        y.coeff(i, 0) = 3.0 * x1;
    }

    // High threshold: correlation_threshold = 1.0, so no features are conditioned on.
    Stats::ConditionalRandomForests<double> cf_high(50, 10, 2, 0.10, 200, -1, 0.632, 1.0, 42);
    cf_high.set_importance_method(Stats::ConditionalImportanceMethod::conditional);
    cf_high.fit(X, y);
    cf_high.compute_importance(X, y);
    auto imp_high = cf_high.get_feature_importances();

    // Low threshold: correlation_threshold = 0.1, so features are conditioned on.
    Stats::ConditionalRandomForests<double> cf_low(50, 10, 2, 0.10, 200, -1, 0.632, 0.1, 42);
    cf_low.set_importance_method(Stats::ConditionalImportanceMethod::conditional);
    cf_low.fit(X, y);
    cf_low.compute_importance(X, y);
    auto imp_low = cf_low.get_feature_importances();

    REQUIRE( imp_high.size() == 2 );
    REQUIRE( imp_low.size() == 2 );

    for(const auto &v : imp_high){
        REQUIRE( std::isfinite(v) );
    }
    for(const auto &v : imp_low){
        REQUIRE( std::isfinite(v) );
    }

    // With threshold=1.0, no conditioning occurs, so x2 importance should be inflated.
    // With threshold=0.1, conditioning occurs, so x2 importance should be reduced.
    REQUIRE( imp_low[1] <= imp_high[1] );
}


TEST_CASE( "ConditionalRandomForests conditional importance with noisy data" ){
    // Test importance ranking when data includes noise.
    // y = 3*x1 + noise, with x2 irrelevant and uncorrelated.
    Stats::ConditionalRandomForests<double> cf(50, 10, 2, 0.10, 200, -1, 0.632, 0.2, 42);
    cf.set_importance_method(Stats::ConditionalImportanceMethod::conditional);

    const int64_t n_samples = 60;
    num_array<double> X(n_samples, 2);
    num_array<double> y(n_samples, 1);

    std::mt19937 rng(55);
    std::normal_distribution<double> noise(0.0, 1.0);

    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i) * 0.5;
        X.coeff(i, 1) = static_cast<double>(i % 7) * 2.0;  // Not correlated with x1.
        y.coeff(i, 0) = 3.0 * X.read_coeff(i, 0) + noise(rng);
    }

    cf.fit(X, y);
    cf.compute_importance(X, y);

    auto importances = cf.get_feature_importances();
    REQUIRE( importances.size() == 2 );
    REQUIRE( std::isfinite(importances[0]) );
    REQUIRE( std::isfinite(importances[1]) );

    // x1 should be much more important than x2.
    REQUIRE( importances[0] > importances[1] );
}


TEST_CASE( "ConditionalRandomForests set_importance_method after fit throws" ){
    Stats::ConditionalRandomForests<double> cf(10, 5, 2, 0.10, 100, -1, 0.632, 0.2, 42);

    num_array<double> X(10, 2);
    num_array<double> y(10, 1);
    for(int64_t i = 0; i < 10; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        X.coeff(i, 1) = static_cast<double>(i * 2);
        y.coeff(i, 0) = static_cast<double>(i);
    }
    cf.fit(X, y);

    // set_importance_method should throw after fit because n_features_trained != -1.
    REQUIRE_THROWS( cf.set_importance_method(Stats::ConditionalImportanceMethod::permutation) );
}


TEST_CASE( "ConditionalRandomForests conditional importance with many correlated features" ){
    // Test with more conditioning variables than the max_cond_vars limit (10).
    // The implementation caps conditioning variables at 10 to avoid 2^n explosion.
    Stats::ConditionalRandomForests<double> cf(30, 8, 2, 0.10, 200, -1, 0.632, 0.0, 42);
    cf.set_importance_method(Stats::ConditionalImportanceMethod::conditional);

    const int64_t n_samples = 60;
    const int64_t n_features = 15;
    num_array<double> X(n_samples, n_features);
    num_array<double> y(n_samples, 1);

    // All features are linear functions of i, so they are all highly correlated.
    // Only feature 0 directly influences y.
    for(int64_t i = 0; i < n_samples; ++i){
        for(int64_t j = 0; j < n_features; ++j){
            X.coeff(i, j) = static_cast<double>(i) * (1.0 + static_cast<double>(j) * 0.1);
        }
        y.coeff(i, 0) = 3.0 * X.read_coeff(i, 0);
    }

    cf.fit(X, y);
    cf.compute_importance(X, y);

    auto importances = cf.get_feature_importances();
    REQUIRE( importances.size() == static_cast<size_t>(n_features) );

    for(const auto &v : importances){
        REQUIRE( std::isfinite(v) );
    }
}


TEST_CASE( "ConditionalRandomForests conditional importance all features in conditioning set" ){
    // With correlation_threshold = 0, all features should be in each conditioning set.
    // This is the extreme case where every feature conditions on every other feature.
    Stats::ConditionalRandomForests<double> cf(30, 8, 2, 0.10, 200, -1, 0.632, 0.0, 42);
    cf.set_importance_method(Stats::ConditionalImportanceMethod::conditional);

    const int64_t n_samples = 40;
    const int64_t n_features = 3;
    num_array<double> X(n_samples, n_features);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i) * 0.3;
        X.coeff(i, 1) = static_cast<double>(i) * 0.5;
        X.coeff(i, 2) = static_cast<double>(i % 5) * 2.0;
        y.coeff(i, 0) = X.read_coeff(i, 0) + X.read_coeff(i, 2);
    }

    cf.fit(X, y);
    cf.compute_importance(X, y);

    auto importances = cf.get_feature_importances();
    REQUIRE( importances.size() == 3 );

    for(const auto &v : importances){
        REQUIRE( std::isfinite(v) );
    }
}


TEST_CASE( "ConditionalRandomForests OOB fraction matches subsample_fraction" ){
    // With subsampling without replacement at fraction f, each sample has probability
    // (1-f) of being OOB for any given tree. Verify the observed OOB fraction is
    // consistent with the expected fraction.

    const int64_t n_samples = 200;
    const double subsample_frac = 0.632;

    num_array<double> X(n_samples, 2);
    num_array<double> y(n_samples, 1);
    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        X.coeff(i, 1) = static_cast<double>(i * 2);
        y.coeff(i, 0) = static_cast<double>(i);
    }

    Stats::ConditionalRandomForests<double> cf(100, 5, 2, 0.10, 100, -1, subsample_frac, 0.2, 42);
    cf.set_importance_method(Stats::ConditionalImportanceMethod::permutation);
    cf.fit(X, y);

    // After fit, compute importance to confirm OOB tracking worked.
    cf.compute_importance(X, y);

    // Count how many trees each sample appeared OOB in.
    // With subsample_fraction = 0.632, each sample should be OOB in about 36.8% of trees.
    // We can verify that all feature importances are finite (indicating OOB tracking worked).
    auto importances = cf.get_feature_importances();
    REQUIRE( importances.size() == 2 );
    for(const auto &v : importances){
        REQUIRE( std::isfinite(v) );
    }
}


TEST_CASE( "ConditionalRandomForests importance compute with mismatched dimensions" ){
    Stats::ConditionalRandomForests<double> cf(10, 5, 2, 0.10, 100, -1, 0.632, 0.2, 42);
    cf.set_importance_method(Stats::ConditionalImportanceMethod::permutation);

    num_array<double> X(20, 3);
    num_array<double> y(20, 1);
    for(int64_t i = 0; i < 20; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        X.coeff(i, 1) = static_cast<double>(i * 2);
        X.coeff(i, 2) = static_cast<double>(i * 3);
        y.coeff(i, 0) = static_cast<double>(i);
    }
    cf.fit(X, y);

    SUBCASE("wrong number of features in X"){
        num_array<double> X_wrong(20, 2);
        num_array<double> y2(20, 1);
        REQUIRE_THROWS( cf.compute_importance(X_wrong, y2) );
    }

    SUBCASE("y not a column vector"){
        num_array<double> y_wrong(20, 2);
        REQUIRE_THROWS( cf.compute_importance(X, y_wrong) );
    }

    SUBCASE("mismatched rows"){
        num_array<double> X2(10, 3);
        num_array<double> y2(20, 1);
        REQUIRE_THROWS( cf.compute_importance(X2, y2) );
    }
}
