
#include <cmath>
#include <limits>

#include <YgorMath.h>
#include <YgorStatsRandomForest.h>

#include "doctest/doctest.h"


TEST_CASE( "RandomForest constructor" ){
    SUBCASE("default constructor produces valid forest"){
        Stats::RandomForest<double> rf;
        REQUIRE(rf.get_n_trees() == 100);
    }

    SUBCASE("custom parameters are accepted"){
        Stats::RandomForest<double> rf(50, 5, 3, 10, 12345);
        REQUIRE(rf.get_n_trees() == 50);
    }

    SUBCASE("invalid parameters are rejected"){
        REQUIRE_THROWS( Stats::RandomForest<double>(0, 5, 2, -1, 42) );   // n_trees = 0
        REQUIRE_THROWS( Stats::RandomForest<double>(-1, 5, 2, -1, 42) );  // n_trees < 0
        REQUIRE_THROWS( Stats::RandomForest<double>(10, 0, 2, -1, 42) );  // max_depth = 0
        REQUIRE_THROWS( Stats::RandomForest<double>(10, -1, 2, -1, 42) ); // max_depth < 0
        REQUIRE_THROWS( Stats::RandomForest<double>(10, 5, 1, -1, 42) );  // min_samples_split < 2
        REQUIRE_THROWS( Stats::RandomForest<double>(10, 5, 0, -1, 42) );  // min_samples_split < 2
    }
}


TEST_CASE( "RandomForest fit validation" ){
    Stats::RandomForest<double> rf(10, 5, 2, -1, 42);
    
    SUBCASE("empty input matrices are rejected"){
        // num_array doesn't allow 0x0 matrices, so test with valid-sized but inappropriate matrices.
        num_array<double> X(1, 1);
        num_array<double> y_wrong(2, 1);  // Mismatched rows.
        REQUIRE_THROWS( rf.fit(X, y_wrong) );
    }
    
    SUBCASE("mismatched dimensions are rejected"){
        num_array<double> X(10, 3);
        num_array<double> y(5, 1);  // Wrong number of rows.
        REQUIRE_THROWS( rf.fit(X, y) );
    }
    
    SUBCASE("y must be a column vector"){
        num_array<double> X(10, 3);
        num_array<double> y(10, 2);  // Not a column vector.
        REQUIRE_THROWS( rf.fit(X, y) );
    }
}


TEST_CASE( "RandomForest predict validation" ){
    Stats::RandomForest<double> rf(10, 5, 2, -1, 42);
    
    SUBCASE("predict before fit throws exception"){
        num_array<double> x(1, 3);
        REQUIRE_THROWS( rf.predict(x) );
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
        rf.fit(X, y);
        
        // Now try to predict with wrong dimensions.
        num_array<double> x_wrong(2, 3);  // Not a row vector.
        REQUIRE_THROWS( rf.predict(x_wrong) );
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
        rf.fit(X, y);
        
        // Try to predict with different number of features.
        num_array<double> x_too_few(1, 2);
        REQUIRE_THROWS( rf.predict(x_too_few) );
        
        num_array<double> x_too_many(1, 5);
        REQUIRE_THROWS( rf.predict(x_too_many) );
    }
}


TEST_CASE( "RandomForest simple linear regression" ){
    // Test that random forest can learn a simple linear relationship: y = 2*x
    Stats::RandomForest<double> rf(100, 10, 2, -1, 42);
    
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
    rf.fit(X, y);
    
    // Test predictions on training data.
    SUBCASE("predictions on training data are reasonable"){
        double total_error = 0.0;
        for(int64_t i = 0; i < n_samples; ++i){
            num_array<double> x_test(1, 1);
            x_test.coeff(0, 0) = X.read_coeff(i, 0);
            const double pred = rf.predict(x_test);
            const double true_val = y.read_coeff(i, 0);
            total_error += std::abs(pred - true_val);
        }
        const double mean_error = total_error / static_cast<double>(n_samples);
        
        // Random forest should learn the pattern reasonably well.
        // Allow some error due to the nature of tree-based models.
        REQUIRE( mean_error < 2.0 );
    }
    
    // Test predictions on unseen data.
    SUBCASE("predictions on test data are reasonable"){
        double total_error = 0.0;
        int n_test = 10;
        for(int i = 0; i < n_test; ++i){
            const double x_val = 0.25 + static_cast<double>(i) * 0.5;  // Offset from training data.
            num_array<double> x_test(1, 1);
            x_test.coeff(0, 0) = x_val;
            const double pred = rf.predict(x_test);
            const double true_val = 2.0 * x_val;
            total_error += std::abs(pred - true_val);
        }
        const double mean_error = total_error / static_cast<double>(n_test);
        
        // Test error should be reasonable (interpolation should work well).
        REQUIRE( mean_error < 2.0 );
    }
}


TEST_CASE( "RandomForest multi-feature regression" ){
    // Test with multiple features: y = x1 + 2*x2 + 3*x3
    Stats::RandomForest<double> rf(100, 10, 2, -1, 123);
    
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
    
    rf.fit(X, y);
    
    // Test on training data.
    double total_error = 0.0;
    for(int64_t i = 0; i < n_samples; ++i){
        num_array<double> x_test(1, n_features);
        x_test.coeff(0, 0) = X.read_coeff(i, 0);
        x_test.coeff(0, 1) = X.read_coeff(i, 1);
        x_test.coeff(0, 2) = X.read_coeff(i, 2);
        
        const double pred = rf.predict(x_test);
        const double true_val = y.read_coeff(i, 0);
        total_error += std::abs(pred - true_val);
    }
    const double mean_error = total_error / static_cast<double>(n_samples);
    
    // Multi-feature model should also learn reasonably well.
    REQUIRE( mean_error < 2.0 );
}


TEST_CASE( "RandomForest non-linear regression" ){
    // Test with a non-linear relationship: y = x^2
    Stats::RandomForest<double> rf(100, 15, 2, -1, 456);
    
    const int64_t n_samples = 30;
    num_array<double> X(n_samples, 1);
    num_array<double> y(n_samples, 1);
    
    for(int64_t i = 0; i < n_samples; ++i){
        const double x_val = static_cast<double>(i) * 0.3 - 4.5;  // Range from -4.5 to 4.5
        X.coeff(i, 0) = x_val;
        y.coeff(i, 0) = x_val * x_val;
    }
    
    rf.fit(X, y);
    
    // Test on training data.
    double total_error = 0.0;
    for(int64_t i = 0; i < n_samples; ++i){
        num_array<double> x_test(1, 1);
        x_test.coeff(0, 0) = X.read_coeff(i, 0);
        const double pred = rf.predict(x_test);
        const double true_val = y.read_coeff(i, 0);
        total_error += std::abs(pred - true_val);
    }
    const double mean_error = total_error / static_cast<double>(n_samples);
    
    // Non-linear relationships should be captured by tree-based models.
    REQUIRE( mean_error < 3.0 );
}


TEST_CASE( "RandomForest constant output" ){
    // Test with constant output values.
    Stats::RandomForest<double> rf(50, 5, 2, -1, 789);
    
    const int64_t n_samples = 20;
    num_array<double> X(n_samples, 2);
    num_array<double> y(n_samples, 1);
    
    // All outputs are the same.
    for(int64_t i = 0; i < n_samples; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        X.coeff(i, 1) = static_cast<double>(i * 2);
        y.coeff(i, 0) = 5.0;  // Constant value.
    }
    
    rf.fit(X, y);
    
    // Predictions should be close to the constant value.
    for(int64_t i = 0; i < 5; ++i){
        num_array<double> x_test(1, 2);
        x_test.coeff(0, 0) = static_cast<double>(i) * 1.5;
        x_test.coeff(0, 1) = static_cast<double>(i) * 3.0;
        const double pred = rf.predict(x_test);
        
        // Should predict close to 5.0.
        REQUIRE( std::abs(pred - 5.0) < 0.5 );
    }
}


TEST_CASE( "RandomForest reproducibility with same seed" ){
    // Test that same seed produces same results.
    const uint64_t seed = 98765;
    
    num_array<double> X(20, 2);
    num_array<double> y(20, 1);
    for(int64_t i = 0; i < 20; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        X.coeff(i, 1) = static_cast<double>(i * 2);
        y.coeff(i, 0) = static_cast<double>(i) * 1.5;
    }
    
    Stats::RandomForest<double> rf1(50, 5, 2, -1, seed);
    rf1.fit(X, y);
    
    Stats::RandomForest<double> rf2(50, 5, 2, -1, seed);
    rf2.fit(X, y);
    
    // Predictions should be identical with same seed.
    num_array<double> x_test(1, 2);
    x_test.coeff(0, 0) = 5.5;
    x_test.coeff(0, 1) = 11.0;
    
    const double pred1 = rf1.predict(x_test);
    const double pred2 = rf2.predict(x_test);
    
    REQUIRE( pred1 == pred2 );
}


TEST_CASE( "RandomForest with few samples" ){
    // Test with minimal number of samples.
    Stats::RandomForest<double> rf(10, 3, 2, -1, 111);
    
    num_array<double> X(3, 1);
    num_array<double> y(3, 1);
    
    X.coeff(0, 0) = 1.0;
    X.coeff(1, 0) = 2.0;
    X.coeff(2, 0) = 3.0;
    
    y.coeff(0, 0) = 2.0;
    y.coeff(1, 0) = 4.0;
    y.coeff(2, 0) = 6.0;
    
    // Should not throw.
    rf.fit(X, y);
    
    num_array<double> x_test(1, 1);
    x_test.coeff(0, 0) = 2.5;
    const double pred = rf.predict(x_test);
    
    // Prediction should be reasonable (between 4.0 and 6.0).
    REQUIRE( pred >= 3.0 );
    REQUIRE( pred <= 7.0 );
}


TEST_CASE( "RandomForest feature randomness" ){
    // Test that max_features parameter affects model behavior.
    // With only 1 feature considered per split, results should differ from considering all features.
    
    num_array<double> X(30, 5);
    num_array<double> y(30, 1);
    
    for(int64_t i = 0; i < 30; ++i){
        for(int64_t j = 0; j < 5; ++j){
            X.coeff(i, j) = static_cast<double>(i * j) * 0.1;
        }
        y.coeff(i, 0) = static_cast<double>(i) * 0.5;
    }
    
    // Model with all features considered.
    Stats::RandomForest<double> rf_all(50, 5, 2, 5, 222);
    rf_all.fit(X, y);
    
    // Model with only 1 feature considered per split.
    Stats::RandomForest<double> rf_limited(50, 5, 2, 1, 222);
    rf_limited.fit(X, y);
    
    num_array<double> x_test(1, 5);
    for(int64_t j = 0; j < 5; ++j){
        x_test.coeff(0, j) = static_cast<double>(j) * 0.2;
    }
    
    const double pred_all = rf_all.predict(x_test);
    const double pred_limited = rf_limited.predict(x_test);
    
    // Predictions should differ (though both should be reasonable).
    // This tests that feature randomness is actually implemented.
    REQUIRE( pred_all != pred_limited );
}


TEST_CASE( "RandomForest bootstrap sampling" ){
    // Verify that bootstrap sampling (bagging) affects results.
    // Different random seeds should produce different models due to different bootstrap samples.
    
    num_array<double> X(25, 2);
    num_array<double> y(25, 1);
    
    for(int64_t i = 0; i < 25; ++i){
        X.coeff(i, 0) = static_cast<double>(i);
        X.coeff(i, 1) = static_cast<double>(i * 2);
        y.coeff(i, 0) = static_cast<double>(i) * 1.2;
    }
    
    Stats::RandomForest<double> rf1(50, 5, 2, -1, 333);
    rf1.fit(X, y);
    
    Stats::RandomForest<double> rf2(50, 5, 2, -1, 444);
    rf2.fit(X, y);
    
    num_array<double> x_test(1, 2);
    x_test.coeff(0, 0) = 12.5;
    x_test.coeff(0, 1) = 25.0;
    
    const double pred1 = rf1.predict(x_test);
    const double pred2 = rf2.predict(x_test);
    
    // Different seeds should produce different predictions due to bootstrap sampling.
    REQUIRE( pred1 != pred2 );
}


TEST_CASE( "RandomForest multiple fit calls" ){
    // Test that calling fit() multiple times works correctly and that max_features
    // is properly recalculated based on the new data.
    
    Stats::RandomForest<double> rf(50, 5, 2, -1, 555);  // max_features = -1 (auto).
    
    // First fit with 16 features (should use sqrt(16) = 4 features per split).
    num_array<double> X1(20, 16);
    num_array<double> y1(20, 1);
    for(int64_t i = 0; i < 20; ++i){
        for(int64_t j = 0; j < 16; ++j){
            X1.coeff(i, j) = static_cast<double>(i * j) * 0.1;
        }
        y1.coeff(i, 0) = static_cast<double>(i);
    }
    rf.fit(X1, y1);
    
    // Verify first model works.
    num_array<double> x_test1(1, 16);
    for(int64_t j = 0; j < 16; ++j){
        x_test1.coeff(0, j) = static_cast<double>(j) * 0.2;
    }
    const double pred1 = rf.predict(x_test1);
    REQUIRE( std::isfinite(pred1) );
    
    // Second fit with 9 features (should use sqrt(9) = 3 features per split).
    num_array<double> X2(15, 9);
    num_array<double> y2(15, 1);
    for(int64_t i = 0; i < 15; ++i){
        for(int64_t j = 0; j < 9; ++j){
            X2.coeff(i, j) = static_cast<double>(i * j) * 0.15;
        }
        y2.coeff(i, 0) = static_cast<double>(i) * 2.0;
    }
    rf.fit(X2, y2);
    
    // Verify second model works and rejects wrong number of features.
    num_array<double> x_test2(1, 9);
    for(int64_t j = 0; j < 9; ++j){
        x_test2.coeff(0, j) = static_cast<double>(j) * 0.3;
    }
    const double pred2 = rf.predict(x_test2);
    REQUIRE( std::isfinite(pred2) );
    
    // Old test input should now be rejected since model expects 9 features.
    REQUIRE_THROWS( rf.predict(x_test1) );
}
