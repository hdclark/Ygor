
#include <cmath>
#include <limits>

#include <YgorMath.h>
#include <YgorStatsCITrees.h>

#include "doctest/doctest.h"


TEST_CASE( "ConditionalInferenceTrees constructor" ){
    SUBCASE("default constructor produces valid tree"){
        Stats::ConditionalInferenceTrees<double> ct;
        REQUIRE(ct.get_alpha() == doctest::Approx(0.05));
        REQUIRE(ct.get_n_permutations() == 1000);
    }

    SUBCASE("custom parameters are accepted"){
        Stats::ConditionalInferenceTrees<double> ct(15, 5, 0.01, 500, 12345);
        REQUIRE(ct.get_alpha() == doctest::Approx(0.01));
        REQUIRE(ct.get_n_permutations() == 500);
    }

    SUBCASE("invalid parameters are rejected"){
        REQUIRE_THROWS( Stats::ConditionalInferenceTrees<double>(0, 2, 0.05, 1000, 42) );   // max_depth = 0
        REQUIRE_THROWS( Stats::ConditionalInferenceTrees<double>(-1, 2, 0.05, 1000, 42) );  // max_depth < 0
        REQUIRE_THROWS( Stats::ConditionalInferenceTrees<double>(10, 1, 0.05, 1000, 42) );  // min_samples_split < 2
        REQUIRE_THROWS( Stats::ConditionalInferenceTrees<double>(10, 0, 0.05, 1000, 42) );  // min_samples_split < 2
        REQUIRE_THROWS( Stats::ConditionalInferenceTrees<double>(10, 2, 0.0, 1000, 42) );   // alpha = 0
        REQUIRE_THROWS( Stats::ConditionalInferenceTrees<double>(10, 2, 1.0, 1000, 42) );   // alpha = 1
        REQUIRE_THROWS( Stats::ConditionalInferenceTrees<double>(10, 2, -0.1, 1000, 42) );  // alpha < 0
        REQUIRE_THROWS( Stats::ConditionalInferenceTrees<double>(10, 2, 0.05, 0, 42) );     // n_permutations = 0
        REQUIRE_THROWS( Stats::ConditionalInferenceTrees<double>(10, 2, 0.05, -1, 42) );    // n_permutations < 0
    }
}


TEST_CASE( "ConditionalInferenceTrees fit validation" ){
    Stats::ConditionalInferenceTrees<double> ct(10, 2, 0.05, 100, 42);

    SUBCASE("mismatched dimensions are rejected"){
        num_array<double> X(10, 3);
        num_array<double> y(5, 1);  // Wrong number of rows.
        REQUIRE_THROWS( ct.fit(X, y) );
    }

    SUBCASE("y must be a column vector"){
        num_array<double> X(10, 3);
        num_array<double> y(10, 2);  // Not a column vector.
        REQUIRE_THROWS( ct.fit(X, y) );
    }
}


TEST_CASE( "ConditionalInferenceTrees predict validation" ){
    Stats::ConditionalInferenceTrees<double> ct(10, 2, 0.05, 100, 42);

    SUBCASE("predict before fit throws exception"){
        num_array<double> x(1, 3);
        REQUIRE_THROWS( ct.predict(x) );
    }

    SUBCASE("predict with wrong dimensions throws exception"){
        // First fit a simple model.
        num_array<double> X(10, 3);
        num_array<double> y(10, 1);
        for(int64_t i = 0; i < 10; ++i){
            X.coeff(i, 0) = static_cast<double>(i);
            X.coeff(i, 1) = static_cast<double>(i * 2);
            X.coeff(i, 2) = static_cast<double>(i * 3);
            y.coeff(i, 0) = static_cast<double>(i);
        }
        ct.fit(X, y);

        // Now try to predict with wrong dimensions.
        num_array<double> x_wrong(2, 3);  // Not a row vector.
        REQUIRE_THROWS( ct.predict(x_wrong) );
    }

    SUBCASE("predict with wrong number of features throws exception"){
        // First fit a simple model with 3 features.
        num_array<double> X(10, 3);
        num_array<double> y(10, 1);
        for(int64_t i = 0; i < 10; ++i){
            X.coeff(i, 0) = static_cast<double>(i);
            X.coeff(i, 1) = static_cast<double>(i * 2);
            X.coeff(i, 2) = static_cast<double>(i * 3);
            y.coeff(i, 0) = static_cast<double>(i);
        }
        ct.fit(X, y);

        // Try to predict with different number of features.
        num_array<double> x_too_few(1, 2);
        REQUIRE_THROWS( ct.predict(x_too_few) );

        num_array<double> x_too_many(1, 5);
        REQUIRE_THROWS( ct.predict(x_too_many) );
    }
}


TEST_CASE( "ConditionalInferenceTrees simple linear regression" ){
    // Test that conditional inference tree can learn a simple linear relationship: y = 2*x.
    // Use a relaxed alpha so the tree is more likely to split on significant features.
    Stats::ConditionalInferenceTrees<double> ct(10, 2, 0.10, 200, 42);

    // Create training data: y = 2*x for x in [0, 10).
    const int64_t n_samples = 20;
    num_array<double> X(n_samples, 1);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        const double x_val = static_cast<double>(i) * 0.5;
        X.coeff(i, 0) = x_val;
        y.coeff(i, 0) = 2.0 * x_val;
    }

    // Fit the model.
    ct.fit(X, y);

    // Test predictions on training data.
    SUBCASE("predictions on training data are reasonable"){
        double total_error = 0.0;
        for(int64_t i = 0; i < n_samples; ++i){
            num_array<double> x_test(1, 1);
            x_test.coeff(0, 0) = X.read_coeff(i, 0);
            const double pred = ct.predict(x_test);
            const double true_val = y.read_coeff(i, 0);
            total_error += std::abs(pred - true_val);
        }
        const double mean_error = total_error / static_cast<double>(n_samples);

        // Single tree won't be as precise as a forest, but should be reasonable.
        REQUIRE( mean_error < 3.0 );
    }
}


TEST_CASE( "ConditionalInferenceTrees multi-feature regression" ){
    // Test with multiple features: y = x1 + 2*x2 + 3*x3
    Stats::ConditionalInferenceTrees<double> ct(15, 2, 0.10, 200, 123);

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

    ct.fit(X, y);

    // Test on training data.
    double total_error = 0.0;
    for(int64_t i = 0; i < n_samples; ++i){
        num_array<double> x_test(1, n_features);
        x_test.coeff(0, 0) = X.read_coeff(i, 0);
        x_test.coeff(0, 1) = X.read_coeff(i, 1);
        x_test.coeff(0, 2) = X.read_coeff(i, 2);

        const double pred = ct.predict(x_test);
        const double true_val = y.read_coeff(i, 0);
        total_error += std::abs(pred - true_val);
    }
    const double mean_error = total_error / static_cast<double>(n_samples);

    // Multi-feature model should learn reasonably well.
    REQUIRE( mean_error < 3.0 );
}


TEST_CASE( "ConditionalInferenceTrees non-linear regression" ){
    // Test with a non-linear relationship: y = x^2
    Stats::ConditionalInferenceTrees<double> ct(15, 2, 0.10, 200, 456);

    const int64_t n_samples = 30;
    num_array<double> X(n_samples, 1);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        const double x_val = static_cast<double>(i) * 0.3 - 4.5;  // Range from -4.5 to 4.5
        X.coeff(i, 0) = x_val;
        y.coeff(i, 0) = x_val * x_val;
    }

    ct.fit(X, y);

    // Test on training data.
    double total_error = 0.0;
    for(int64_t i = 0; i < n_samples; ++i){
        num_array<double> x_test(1, 1);
        x_test.coeff(0, 0) = X.read_coeff(i, 0);
        const double pred = ct.predict(x_test);
        const double true_val = y.read_coeff(i, 0);
        total_error += std::abs(pred - true_val);
    }
    const double mean_error = total_error / static_cast<double>(n_samples);

    // Non-linear relationships should be captured by tree-based models.
    REQUIRE( mean_error < 6.0 );
}


TEST_CASE( "ConditionalInferenceTrees constant output" ){
    // Test with constant output values.
    Stats::ConditionalInferenceTrees<double> ct(5, 2, 0.05, 100, 789);

    const int64_t n_samples = 20;
    num_array<double> X(n_samples, 2);
    num_array<double> y(n_samples, 1);

    // All outputs are the same.
    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        X.coeff(i, 1) = static_cast<double>(i * 2);
        y.coeff(i, 0) = 5.0;  // Constant value.
    }

    ct.fit(X, y);

    // Predictions should be close to the constant value.
    // With constant y, no variable should be significant and the tree should be a single leaf.
    for(int64_t i = 0; i < 5; ++i){
        num_array<double> x_test(1, 2);
        x_test.coeff(0, 0) = static_cast<double>(i) * 1.5;
        x_test.coeff(0, 1) = static_cast<double>(i) * 3.0;
        const double pred = ct.predict(x_test);

        // Should predict close to 5.0.
        REQUIRE( std::abs(pred - 5.0) < 0.5 );
    }
}


TEST_CASE( "ConditionalInferenceTrees reproducibility with same seed" ){
    // Test that same seed produces same results.
    const uint64_t seed = 98765;

    num_array<double> X(20, 2);
    num_array<double> y(20, 1);
    for(int64_t i = 0; i < 20; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        X.coeff(i, 1) = static_cast<double>(i * 2);
        y.coeff(i, 0) = static_cast<double>(i) * 1.5;
    }

    Stats::ConditionalInferenceTrees<double> ct1(10, 2, 0.10, 200, seed);
    ct1.fit(X, y);

    Stats::ConditionalInferenceTrees<double> ct2(10, 2, 0.10, 200, seed);
    ct2.fit(X, y);

    // Predictions should be identical with same seed.
    num_array<double> x_test(1, 2);
    x_test.coeff(0, 0) = 5.5;
    x_test.coeff(0, 1) = 11.0;

    const double pred1 = ct1.predict(x_test);
    const double pred2 = ct2.predict(x_test);

    REQUIRE( pred1 == pred2 );
}


TEST_CASE( "ConditionalInferenceTrees with few samples" ){
    // Test with minimal number of samples.
    Stats::ConditionalInferenceTrees<double> ct(3, 2, 0.10, 100, 111);

    num_array<double> X(3, 1);
    num_array<double> y(3, 1);

    X.coeff(0, 0) = 1.0;
    X.coeff(1, 0) = 2.0;
    X.coeff(2, 0) = 3.0;

    y.coeff(0, 0) = 2.0;
    y.coeff(1, 0) = 4.0;
    y.coeff(2, 0) = 6.0;

    // Should not throw.
    ct.fit(X, y);

    num_array<double> x_test(1, 1);
    x_test.coeff(0, 0) = 2.5;
    const double pred = ct.predict(x_test);

    // Prediction should be reasonable (within range of training values).
    REQUIRE( pred >= 1.0 );
    REQUIRE( pred <= 7.0 );
}


TEST_CASE( "ConditionalInferenceTrees multiple fit calls" ){
    // Test that calling fit() multiple times works correctly.
    Stats::ConditionalInferenceTrees<double> ct(5, 2, 0.10, 100, 555);

    // First fit with 4 features.
    num_array<double> X1(20, 4);
    num_array<double> y1(20, 1);
    for(int64_t i = 0; i < 20; ++i){
        for(int64_t j = 0; j < 4; ++j){
            X1.coeff(i, j) = static_cast<double>(i * j) * 0.1;
        }
        y1.coeff(i, 0) = static_cast<double>(i);
    }
    ct.fit(X1, y1);

    // Verify first model works.
    num_array<double> x_test1(1, 4);
    for(int64_t j = 0; j < 4; ++j){
        x_test1.coeff(0, j) = static_cast<double>(j) * 0.2;
    }
    const double pred1 = ct.predict(x_test1);
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
    ct.fit(X2, y2);

    // Verify second model works.
    num_array<double> x_test2(1, 2);
    for(int64_t j = 0; j < 2; ++j){
        x_test2.coeff(0, j) = static_cast<double>(j) * 0.3;
    }
    const double pred2 = ct.predict(x_test2);
    REQUIRE( std::isfinite(pred2) );

    // Old test input should now be rejected since model expects 2 features.
    REQUIRE_THROWS( ct.predict(x_test1) );
}


TEST_CASE( "ConditionalInferenceTrees alpha controls splitting" ){
    // Test that a very low alpha prevents splitting (creates a leaf-only tree).
    num_array<double> X(20, 1);
    num_array<double> y(20, 1);
    for(int64_t i = 0; i < 20; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        y.coeff(i, 0) = static_cast<double>(i) * 2.0;
    }

    // Very low alpha should make it very hard to split.
    Stats::ConditionalInferenceTrees<double> ct_strict(10, 2, 0.001, 100, 42);
    ct_strict.fit(X, y);

    // With a more relaxed alpha, should split more.
    Stats::ConditionalInferenceTrees<double> ct_relaxed(10, 2, 0.50, 100, 42);
    ct_relaxed.fit(X, y);

    // Both should produce finite predictions.
    num_array<double> x_test(1, 1);
    x_test.coeff(0, 0) = 10.0;
    REQUIRE( std::isfinite(ct_strict.predict(x_test)) );
    REQUIRE( std::isfinite(ct_relaxed.predict(x_test)) );
}


TEST_CASE( "ConditionalInferenceTrees with noise feature" ){
    // Test that the conditional inference tree can identify the relevant feature
    // when there is one informative and one noise feature.
    Stats::ConditionalInferenceTrees<double> ct(10, 2, 0.10, 200, 42);

    const int64_t n_samples = 40;
    num_array<double> X(n_samples, 2);
    num_array<double> y(n_samples, 1);

    // Feature 0 is informative (y = 3*x0), feature 1 is constant (noise).
    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i) * 0.25;
        X.coeff(i, 1) = 5.0;  // Constant -- no information.
        y.coeff(i, 0) = 3.0 * static_cast<double>(i) * 0.25;
    }

    ct.fit(X, y);

    // Predictions should be reasonable -- the tree should use feature 0.
    double total_error = 0.0;
    for(int64_t i = 0; i < n_samples; ++i){
        num_array<double> x_test(1, 2);
        x_test.coeff(0, 0) = X.read_coeff(i, 0);
        x_test.coeff(0, 1) = X.read_coeff(i, 1);
        const double pred = ct.predict(x_test);
        const double true_val = y.read_coeff(i, 0);
        total_error += std::abs(pred - true_val);
    }
    const double mean_error = total_error / static_cast<double>(n_samples);
    REQUIRE( mean_error < 5.0 );
}


TEST_CASE( "ConditionalInferenceTrees step function" ){
    // Test with a step function that should be easy for a tree to learn.
    Stats::ConditionalInferenceTrees<double> ct(10, 2, 0.10, 200, 42);

    const int64_t n_samples = 30;
    num_array<double> X(n_samples, 1);
    num_array<double> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        const double x_val = static_cast<double>(i);
        X.coeff(i, 0) = x_val;
        y.coeff(i, 0) = (x_val < 15.0) ? 0.0 : 10.0;
    }

    ct.fit(X, y);

    // Test predictions: values well below and well above the step should be accurate.
    num_array<double> x_low(1, 1);
    x_low.coeff(0, 0) = 5.0;
    REQUIRE( ct.predict(x_low) < 5.0 );

    num_array<double> x_high(1, 1);
    x_high.coeff(0, 0) = 25.0;
    REQUIRE( ct.predict(x_high) > 5.0 );
}


TEST_CASE( "ConditionalInferenceTrees float specialization" ){
    // Test that the float specialization works.
    Stats::ConditionalInferenceTrees<float> ct(5, 2, 0.10f, 100, 42);

    const int64_t n_samples = 15;
    num_array<float> X(n_samples, 1);
    num_array<float> y(n_samples, 1);

    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<float>(i);
        y.coeff(i, 0) = static_cast<float>(i) * 2.0f;
    }

    ct.fit(X, y);

    num_array<float> x_test(1, 1);
    x_test.coeff(0, 0) = 7.0f;
    const float pred = ct.predict(x_test);
    REQUIRE( std::isfinite(pred) );
    REQUIRE( pred > 0.0f );
}
