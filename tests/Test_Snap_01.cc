// Test_Snap_01.cc - Unit tests for YgorSnap.h fixed-precision snap type.
//
// This test validates the snap_fp class, which provides a drop-in replacement for
// floating point types with fixed-precision snapping behavior.

#include <iostream>
#include <cmath>
#include <sstream>
#include <limits>
#include <stdexcept>
#include <cstdlib>

#include "YgorSnap.h"
#include "YgorMisc.h"
#include "YgorLog.h"

// Simple test macro.
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAILED: " << message << " (line " << __LINE__ << ")" << std::endl; \
            return 1; \
        } \
    } while(0)

// Helper to check floating point equality with tolerance.
template<typename T>
bool approx_eq(T a, T b, T tolerance = static_cast<T>(1e-9)) {
    return std::abs(a - b) <= tolerance;
}

int main(int, char**) {
    std::cout << "Testing snap_fp fixed-precision type..." << std::endl;

    //-----------------------------------------------------------
    // Test construction and basic properties
    //-----------------------------------------------------------
    std::cout << "  Testing construction..." << std::endl;
    {
        // Default construction.
        snap_fp<double, 1000000> a;
        TEST_ASSERT(a.raw() == 0, "Default construction should be zero");
        TEST_ASSERT(a.to_fp() == 0.0, "Default construction to_fp should be 0.0");

        // Construction from floating point.
        snap_fp<double, 1000000> b(1.234567);
        TEST_ASSERT(b.raw() == 1234567, "Construction from 1.234567 should give raw 1234567");
        TEST_ASSERT(approx_eq(b.to_fp(), 1.234567), "to_fp should return approximately 1.234567");

        // Copy construction.
        snap_fp<double, 1000000> c(b);
        TEST_ASSERT(c.raw() == b.raw(), "Copy construction should copy raw value");
        TEST_ASSERT(c == b, "Copy should be equal to original");

        // Move construction.
        snap_fp<double, 1000000> d(std::move(snap_fp<double, 1000000>(2.5)));
        TEST_ASSERT(d.raw() == 2500000, "Move construction should work");
    }

    //-----------------------------------------------------------
    // Test snapping/rounding behavior
    //-----------------------------------------------------------
    std::cout << "  Testing snapping behavior..." << std::endl;
    {
        // Test that values are rounded to nearest representable value.
        snap_fp<double, 1000> a(1.2345);  // Should round to 1.235
        TEST_ASSERT(a.raw() == 1235, "1.2345 with scale 1000 should round to raw 1235");
        TEST_ASSERT(approx_eq(a.to_fp(), 1.235, 1e-6), "1.2345 should snap to 1.235");

        snap_fp<double, 1000> b(1.2344);  // Should round to 1.234
        TEST_ASSERT(b.raw() == 1234, "1.2344 with scale 1000 should round to raw 1234");
        TEST_ASSERT(approx_eq(b.to_fp(), 1.234, 1e-6), "1.2344 should snap to 1.234");

        // Test precision loss is bounded by scale.
        snap_fp<double, 100> c(3.14159);  // Should round to 3.14
        TEST_ASSERT(c.raw() == 314, "3.14159 with scale 100 should round to raw 314");
        TEST_ASSERT(approx_eq(c.to_fp(), 3.14, 1e-6), "3.14159 should snap to 3.14");
    }

    //-----------------------------------------------------------
    // Test assignment operators
    //-----------------------------------------------------------
    std::cout << "  Testing assignment operators..." << std::endl;
    {
        snap_fp<double, 1000000> a;
        
        // Assignment from floating point.
        a = 5.5;
        TEST_ASSERT(a.raw() == 5500000, "Assignment from 5.5 should give raw 5500000");
        
        // Copy assignment.
        snap_fp<double, 1000000> b;
        b = a;
        TEST_ASSERT(b == a, "Copy assignment should make values equal");

        // Self-assignment.
        a = a;
        TEST_ASSERT(a.raw() == 5500000, "Self-assignment should preserve value");
    }

    //-----------------------------------------------------------
    // Test arithmetic operators
    //-----------------------------------------------------------
    std::cout << "  Testing arithmetic operators..." << std::endl;
    {
        snap_fp<double, 1000000> a(2.5);
        snap_fp<double, 1000000> b(1.5);

        // Addition.
        auto sum = a + b;
        TEST_ASSERT(approx_eq(sum.to_fp(), 4.0), "2.5 + 1.5 should equal 4.0");
        TEST_ASSERT(sum.raw() == 4000000, "Sum raw should be 4000000");

        // Subtraction.
        auto diff = a - b;
        TEST_ASSERT(approx_eq(diff.to_fp(), 1.0), "2.5 - 1.5 should equal 1.0");
        TEST_ASSERT(diff.raw() == 1000000, "Diff raw should be 1000000");

        // Multiplication.
        auto prod = a * b;
        TEST_ASSERT(approx_eq(prod.to_fp(), 3.75), "2.5 * 1.5 should equal 3.75");

        // Division.
        snap_fp<double, 1000000> c(6.0);
        snap_fp<double, 1000000> d(2.0);
        auto quot = c / d;
        TEST_ASSERT(approx_eq(quot.to_fp(), 3.0), "6.0 / 2.0 should equal 3.0");

        // Unary minus.
        auto neg = -a;
        TEST_ASSERT(approx_eq(neg.to_fp(), -2.5), "Negation of 2.5 should be -2.5");
        TEST_ASSERT(neg.raw() == -2500000, "Negation raw should be -2500000");

        // Unary plus.
        auto pos = +a;
        TEST_ASSERT(pos == a, "Unary plus should return same value");
    }

    //-----------------------------------------------------------
    // Test compound assignment operators
    //-----------------------------------------------------------
    std::cout << "  Testing compound assignment operators..." << std::endl;
    {
        snap_fp<double, 1000000> a(10.0);
        snap_fp<double, 1000000> b(3.0);

        a += b;
        TEST_ASSERT(approx_eq(a.to_fp(), 13.0), "10 += 3 should equal 13");

        a -= b;
        TEST_ASSERT(approx_eq(a.to_fp(), 10.0), "13 -= 3 should equal 10");

        a *= b;
        TEST_ASSERT(approx_eq(a.to_fp(), 30.0), "10 *= 3 should equal 30");

        a /= b;
        TEST_ASSERT(approx_eq(a.to_fp(), 10.0), "30 /= 3 should equal 10");
    }

    //-----------------------------------------------------------
    // Test operations with floating point values
    //-----------------------------------------------------------
    std::cout << "  Testing operations with floating point values..." << std::endl;
    {
        snap_fp<double, 1000000> a(5.0);

        // snap_fp + double.
        auto sum = a + 2.5;
        TEST_ASSERT(approx_eq(sum.to_fp(), 7.5), "5.0 + 2.5 should equal 7.5");

        // double + snap_fp.
        auto sum2 = 2.5 + a;
        TEST_ASSERT(approx_eq(sum2.to_fp(), 7.5), "2.5 + 5.0 should equal 7.5");

        // snap_fp - double.
        auto diff = a - 2.5;
        TEST_ASSERT(approx_eq(diff.to_fp(), 2.5), "5.0 - 2.5 should equal 2.5");

        // double - snap_fp.
        auto diff2 = 10.0 - a;
        TEST_ASSERT(approx_eq(diff2.to_fp(), 5.0), "10.0 - 5.0 should equal 5.0");

        // snap_fp * double.
        auto prod = a * 2.0;
        TEST_ASSERT(approx_eq(prod.to_fp(), 10.0), "5.0 * 2.0 should equal 10.0");

        // double * snap_fp.
        auto prod2 = 2.0 * a;
        TEST_ASSERT(approx_eq(prod2.to_fp(), 10.0), "2.0 * 5.0 should equal 10.0");

        // snap_fp / double.
        auto quot = a / 2.0;
        TEST_ASSERT(approx_eq(quot.to_fp(), 2.5), "5.0 / 2.0 should equal 2.5");

        // double / snap_fp.
        auto quot2 = 10.0 / a;
        TEST_ASSERT(approx_eq(quot2.to_fp(), 2.0), "10.0 / 5.0 should equal 2.0");

        // Compound assignment with double.
        snap_fp<double, 1000000> b(10.0);
        b += 5.0;
        TEST_ASSERT(approx_eq(b.to_fp(), 15.0), "10 += 5.0 should equal 15");
        b -= 3.0;
        TEST_ASSERT(approx_eq(b.to_fp(), 12.0), "15 -= 3.0 should equal 12");
        b *= 2.0;
        TEST_ASSERT(approx_eq(b.to_fp(), 24.0), "12 *= 2.0 should equal 24");
        b /= 4.0;
        TEST_ASSERT(approx_eq(b.to_fp(), 6.0), "24 /= 4.0 should equal 6");
    }

    //-----------------------------------------------------------
    // Test comparison operators
    //-----------------------------------------------------------
    std::cout << "  Testing comparison operators..." << std::endl;
    {
        snap_fp<double, 1000000> a(5.0);
        snap_fp<double, 1000000> b(5.0);
        snap_fp<double, 1000000> c(3.0);
        snap_fp<double, 1000000> d(7.0);

        // Equality.
        TEST_ASSERT(a == b, "5.0 should equal 5.0");
        TEST_ASSERT(!(a == c), "5.0 should not equal 3.0");

        // Inequality.
        TEST_ASSERT(a != c, "5.0 should not equal 3.0");
        TEST_ASSERT(!(a != b), "5.0 should equal 5.0");

        // Less than.
        TEST_ASSERT(c < a, "3.0 should be less than 5.0");
        TEST_ASSERT(!(a < b), "5.0 should not be less than 5.0");
        TEST_ASSERT(!(d < a), "7.0 should not be less than 5.0");

        // Less than or equal.
        TEST_ASSERT(c <= a, "3.0 should be less than or equal to 5.0");
        TEST_ASSERT(a <= b, "5.0 should be less than or equal to 5.0");
        TEST_ASSERT(!(d <= a), "7.0 should not be less than or equal to 5.0");

        // Greater than.
        TEST_ASSERT(d > a, "7.0 should be greater than 5.0");
        TEST_ASSERT(!(a > b), "5.0 should not be greater than 5.0");
        TEST_ASSERT(!(c > a), "3.0 should not be greater than 5.0");

        // Greater than or equal.
        TEST_ASSERT(d >= a, "7.0 should be greater than or equal to 5.0");
        TEST_ASSERT(a >= b, "5.0 should be greater than or equal to 5.0");
        TEST_ASSERT(!(c >= a), "3.0 should not be greater than or equal to 5.0");

        // Comparison with floating point.
        TEST_ASSERT(a == 5.0, "5.0 should equal 5.0 (double)");
        TEST_ASSERT(5.0 == a, "5.0 (double) should equal 5.0");
        TEST_ASSERT(a < 6.0, "5.0 should be less than 6.0 (double)");
        TEST_ASSERT(4.0 < a, "4.0 (double) should be less than 5.0");
    }

    //-----------------------------------------------------------
    // Test mathematical functions
    //-----------------------------------------------------------
    std::cout << "  Testing mathematical functions..." << std::endl;
    {
        // abs.
        snap_fp<double, 1000000> neg(-5.0);
        snap_fp<double, 1000000> pos(5.0);
        TEST_ASSERT(abs(neg) == pos, "abs(-5) should equal 5");
        TEST_ASSERT(abs(pos) == pos, "abs(5) should equal 5");

        // sqrt.
        snap_fp<double, 1000000> four(4.0);
        auto sq = sqrt(four);
        TEST_ASSERT(approx_eq(sq.to_fp(), 2.0), "sqrt(4) should equal 2.0");

        snap_fp<double, 1000000> nine(9.0);
        auto sq3 = sqrt(nine);
        TEST_ASSERT(approx_eq(sq3.to_fp(), 3.0), "sqrt(9) should equal 3.0");

        // floor.
        snap_fp<double, 1000000> f1(3.7);
        TEST_ASSERT(approx_eq(floor(f1).to_fp(), 3.0), "floor(3.7) should equal 3.0");

        snap_fp<double, 1000000> f2(-3.7);
        TEST_ASSERT(approx_eq(floor(f2).to_fp(), -4.0), "floor(-3.7) should equal -4.0");

        // ceil.
        snap_fp<double, 1000000> c1(3.2);
        TEST_ASSERT(approx_eq(ceil(c1).to_fp(), 4.0), "ceil(3.2) should equal 4.0");

        snap_fp<double, 1000000> c2(-3.2);
        TEST_ASSERT(approx_eq(ceil(c2).to_fp(), -3.0), "ceil(-3.2) should equal -3.0");

        // round.
        snap_fp<double, 1000000> r1(3.4);
        TEST_ASSERT(approx_eq(round(r1).to_fp(), 3.0), "round(3.4) should equal 3.0");

        snap_fp<double, 1000000> r2(3.6);
        TEST_ASSERT(approx_eq(round(r2).to_fp(), 4.0), "round(3.6) should equal 4.0");

        // fmod.
        snap_fp<double, 1000000> m1(10.0);
        snap_fp<double, 1000000> m2(3.0);
        auto mod = fmod(m1, m2);
        TEST_ASSERT(approx_eq(mod.to_fp(), 1.0), "fmod(10, 3) should equal 1.0");

        // min/max.
        snap_fp<double, 1000000> a(5.0);
        snap_fp<double, 1000000> b(3.0);
        TEST_ASSERT(min(a, b) == b, "min(5, 3) should equal 3");
        TEST_ASSERT(max(a, b) == a, "max(5, 3) should equal 5");

        // pow.
        snap_fp<double, 1000000> base(2.0);
        snap_fp<double, 1000000> exp3(3.0);
        auto p1 = pow(base, exp3);
        TEST_ASSERT(approx_eq(p1.to_fp(), 8.0), "pow(2, 3) should equal 8");

        auto p2 = pow(base, 4);
        TEST_ASSERT(approx_eq(p2.to_fp(), 16.0), "pow(2, 4) should equal 16");

        // Trigonometric functions.
        snap_fp<double, 1000000> zero(0.0);
        snap_fp<double, 1000000> pi(3.14159265358979);
        snap_fp<double, 1000000> half_pi(1.5707963267949);

        TEST_ASSERT(approx_eq(sin(zero).to_fp(), 0.0, 1e-6), "sin(0) should equal 0");
        TEST_ASSERT(approx_eq(cos(zero).to_fp(), 1.0, 1e-6), "cos(0) should equal 1");
        TEST_ASSERT(approx_eq(sin(half_pi).to_fp(), 1.0, 1e-5), "sin(pi/2) should equal ~1");
        TEST_ASSERT(approx_eq(cos(pi).to_fp(), -1.0, 1e-5), "cos(pi) should equal ~-1");

        // Exponential and log.
        snap_fp<double, 1000000> one(1.0);
        snap_fp<double, 1000000> e_val(2.71828182845905);
        TEST_ASSERT(approx_eq(exp(zero).to_fp(), 1.0, 1e-6), "exp(0) should equal 1");
        TEST_ASSERT(approx_eq(log(e_val).to_fp(), 1.0, 1e-5), "log(e) should equal ~1");
        TEST_ASSERT(approx_eq(log10(snap_fp<double, 1000000>(100.0)).to_fp(), 2.0, 1e-6), "log10(100) should equal 2");
    }

    //-----------------------------------------------------------
    // Test utility functions
    //-----------------------------------------------------------
    std::cout << "  Testing utility functions..." << std::endl;
    {
        snap_fp<double, 1000000> zero;
        snap_fp<double, 1000000> pos(5.0);
        snap_fp<double, 1000000> neg(-5.0);

        TEST_ASSERT(zero.is_zero(), "Default constructed should be zero");
        TEST_ASSERT(!pos.is_zero(), "5.0 should not be zero");
        TEST_ASSERT(!neg.is_zero(), "-5.0 should not be zero");

        TEST_ASSERT(pos.is_positive(), "5.0 should be positive");
        TEST_ASSERT(!neg.is_positive(), "-5.0 should not be positive");
        TEST_ASSERT(!zero.is_positive(), "0 should not be positive");

        TEST_ASSERT(neg.is_negative(), "-5.0 should be negative");
        TEST_ASSERT(!pos.is_negative(), "5.0 should not be negative");
        TEST_ASSERT(!zero.is_negative(), "0 should not be negative");

        TEST_ASSERT(pos.sign() == 1, "sign of 5.0 should be 1");
        TEST_ASSERT(neg.sign() == -1, "sign of -5.0 should be -1");
        TEST_ASSERT(zero.sign() == 0, "sign of 0 should be 0");
    }

    //-----------------------------------------------------------
    // Test static utility functions
    //-----------------------------------------------------------
    std::cout << "  Testing static utility functions..." << std::endl;
    {
        using snap_t = snap_fp<double, 1000000>;

        auto eps = snap_t::epsilon();
        TEST_ASSERT(eps.raw() == 1, "epsilon raw should be 1");
        TEST_ASSERT(approx_eq(eps.to_fp(), 1e-6), "epsilon to_fp should be 1e-6");

        auto one = snap_t::one();
        TEST_ASSERT(one.raw() == 1000000, "one raw should equal scale factor");
        TEST_ASSERT(approx_eq(one.to_fp(), 1.0), "one to_fp should be 1.0");

        auto zero = snap_t::zero();
        TEST_ASSERT(zero.raw() == 0, "zero raw should be 0");
    }

    //-----------------------------------------------------------
    // Test stream operators
    //-----------------------------------------------------------
    std::cout << "  Testing stream operators..." << std::endl;
    {
        snap_fp<double, 1000000> a(3.14159);

        // Output.
        std::ostringstream oss;
        oss << a;
        // The output should be the floating point representation.
        // May have some rounding, but should be close to 3.14159.
        
        // Input.
        std::istringstream iss("2.71828");
        snap_fp<double, 1000000> b;
        iss >> b;
        TEST_ASSERT(approx_eq(b.to_fp(), 2.71828, 1e-5), "Stream input should read 2.71828");
    }

    //-----------------------------------------------------------
    // Test explicit conversion
    //-----------------------------------------------------------
    std::cout << "  Testing explicit conversion..." << std::endl;
    {
        snap_fp<double, 1000000> a(7.5);
        double d = static_cast<double>(a);
        TEST_ASSERT(approx_eq(d, 7.5), "Explicit conversion to double should give 7.5");
    }

    //-----------------------------------------------------------
    // Test from_raw static function
    //-----------------------------------------------------------
    std::cout << "  Testing from_raw..." << std::endl;
    {
        using snap_t = snap_fp<double, 1000000>;
        auto a = snap_t::from_raw(12345678);
        TEST_ASSERT(a.raw() == 12345678, "from_raw should set raw value directly");
        TEST_ASSERT(approx_eq(a.to_fp(), 12.345678), "from_raw(12345678) to_fp should be 12.345678");
    }

    //-----------------------------------------------------------
    // Test division by zero handling
    //-----------------------------------------------------------
    std::cout << "  Testing division by zero handling..." << std::endl;
    {
        snap_fp<double, 1000000> a(5.0);
        snap_fp<double, 1000000> zero;

        bool caught_exception = false;
        try {
            auto result = a / zero;
            (void)result;  // Avoid unused warning.
        } catch (const std::domain_error&) {
            caught_exception = true;
        }
        TEST_ASSERT(caught_exception, "Division by zero should throw domain_error");

        caught_exception = false;
        try {
            auto result = a / 0.0;
            (void)result;
        } catch (const std::domain_error&) {
            caught_exception = true;
        }
        TEST_ASSERT(caught_exception, "Division by 0.0 should throw domain_error");
    }

    //-----------------------------------------------------------
    // Test sqrt of negative number
    //-----------------------------------------------------------
    std::cout << "  Testing sqrt of negative number..." << std::endl;
    {
        snap_fp<double, 1000000> neg(-1.0);
        bool caught_exception = false;
        try {
            auto result = sqrt(neg);
            (void)result;
        } catch (const std::domain_error&) {
            caught_exception = true;
        }
        TEST_ASSERT(caught_exception, "sqrt of negative should throw domain_error");
    }

    //-----------------------------------------------------------
    // Test log of non-positive number
    //-----------------------------------------------------------
    std::cout << "  Testing log of non-positive number..." << std::endl;
    {
        snap_fp<double, 1000000> zero;
        snap_fp<double, 1000000> neg(-1.0);

        bool caught_exception = false;
        try {
            auto result = log(zero);
            (void)result;
        } catch (const std::domain_error&) {
            caught_exception = true;
        }
        TEST_ASSERT(caught_exception, "log of zero should throw domain_error");

        caught_exception = false;
        try {
            auto result = log(neg);
            (void)result;
        } catch (const std::domain_error&) {
            caught_exception = true;
        }
        TEST_ASSERT(caught_exception, "log of negative should throw domain_error");
    }

    //-----------------------------------------------------------
    // Test fmod by zero
    //-----------------------------------------------------------
    std::cout << "  Testing fmod by zero..." << std::endl;
    {
        snap_fp<double, 1000000> a(5.0);
        snap_fp<double, 1000000> zero;

        bool caught_exception = false;
        try {
            auto result = fmod(a, zero);
            (void)result;
        } catch (const std::domain_error&) {
            caught_exception = true;
        }
        TEST_ASSERT(caught_exception, "fmod by zero should throw domain_error");
    }

    //-----------------------------------------------------------
    // Test precision preservation in binary operations
    //-----------------------------------------------------------
    std::cout << "  Testing precision preservation..." << std::endl;
    {
        // Addition and subtraction should preserve precision exactly.
        snap_fp<double, 1000000> a(1.000001);
        snap_fp<double, 1000000> b(0.000001);
        
        auto sum = a + b;
        TEST_ASSERT(sum.raw() == 1000002, "1.000001 + 0.000001 raw should be 1000002");

        auto diff = a - b;
        TEST_ASSERT(diff.raw() == 1000000, "1.000001 - 0.000001 raw should be 1000000");

        // Test that operations use internal representation directly.
        snap_fp<double, 1000000> c = snap_fp<double, 1000000>::from_raw(1);
        snap_fp<double, 1000000> d = snap_fp<double, 1000000>::from_raw(1);
        auto sum2 = c + d;
        TEST_ASSERT(sum2.raw() == 2, "from_raw(1) + from_raw(1) should give raw 2");
    }

    //-----------------------------------------------------------
    // Test negative value handling
    //-----------------------------------------------------------
    std::cout << "  Testing negative value handling..." << std::endl;
    {
        snap_fp<double, 1000000> a(-3.5);
        snap_fp<double, 1000000> b(-2.5);

        // Addition of negatives.
        auto sum = a + b;
        TEST_ASSERT(approx_eq(sum.to_fp(), -6.0), "-3.5 + -2.5 should equal -6.0");

        // Subtraction.
        auto diff = a - b;
        TEST_ASSERT(approx_eq(diff.to_fp(), -1.0), "-3.5 - -2.5 should equal -1.0");

        // Multiplication.
        auto prod = a * b;
        TEST_ASSERT(approx_eq(prod.to_fp(), 8.75), "-3.5 * -2.5 should equal 8.75");

        // Division.
        snap_fp<double, 1000000> c(-10.0);
        snap_fp<double, 1000000> d(2.0);
        auto quot = c / d;
        TEST_ASSERT(approx_eq(quot.to_fp(), -5.0), "-10.0 / 2.0 should equal -5.0");
    }

    //-----------------------------------------------------------
    // Test with different scale factors
    //-----------------------------------------------------------
    std::cout << "  Testing different scale factors..." << std::endl;
    {
        // High precision.
        snap_fp<double, 1000000000> high_prec(1.123456789);
        TEST_ASSERT(high_prec.raw() == 1123456789, "High precision should capture 9 digits");

        // Low precision.
        snap_fp<double, 100> low_prec(3.14159);
        TEST_ASSERT(low_prec.raw() == 314, "Low precision (scale 100) should round to 3.14");
        TEST_ASSERT(approx_eq(low_prec.to_fp(), 3.14, 1e-6), "Low precision to_fp should be 3.14");

        // Integer precision.
        snap_fp<double, 1> int_prec(7.8);
        TEST_ASSERT(int_prec.raw() == 8, "Integer precision should round to 8");
        TEST_ASSERT(approx_eq(int_prec.to_fp(), 8.0), "Integer precision to_fp should be 8.0");
    }

    //-----------------------------------------------------------
    // Test type aliases
    //-----------------------------------------------------------
    std::cout << "  Testing type aliases..." << std::endl;
    {
        snap_double_micro a(1.234567);
        TEST_ASSERT(approx_eq(a.to_fp(), 1.234567), "snap_double_micro should work");

        snap_double_milli b(1.2345);
        TEST_ASSERT(approx_eq(b.to_fp(), 1.235, 1e-6), "snap_double_milli should round to 1.235");

        snap_float_micro c(2.5f);
        TEST_ASSERT(approx_eq(c.to_fp(), 2.5f, 1e-6f), "snap_float_micro should work");
    }

    //-----------------------------------------------------------
    // Test edge cases near limits
    //-----------------------------------------------------------
    std::cout << "  Testing edge cases..." << std::endl;
    {
        // Test operations near zero.
        snap_fp<double, 1000000> tiny(0.000001);
        snap_fp<double, 1000000> another_tiny(0.000001);
        
        auto sum = tiny + another_tiny;
        TEST_ASSERT(sum.raw() == 2, "0.000001 + 0.000001 raw should be 2");
        TEST_ASSERT(approx_eq(sum.to_fp(), 0.000002), "Sum of tiny values should be 0.000002");

        // Test that rounding to zero works correctly.
        snap_fp<double, 1000000> below_epsilon(0.0000001);  // Below epsilon
        TEST_ASSERT(below_epsilon.raw() == 0, "Value below epsilon should round to 0");
    }

    //-----------------------------------------------------------
    // Test consistency with repeated operations
    //-----------------------------------------------------------
    std::cout << "  Testing consistency with repeated operations..." << std::endl;
    {
        snap_fp<double, 1000000> a(0.1);
        snap_fp<double, 1000000> sum;

        // Add 0.1 ten times - should be exactly 1.0 due to snap behavior.
        for (int i = 0; i < 10; ++i) {
            sum += a;
        }
        // With floating point, 0.1*10 might not be exactly 1.0.
        // With snap_fp, it should be deterministic.
        TEST_ASSERT(sum.raw() == 1000000, "0.1 added 10 times should give raw 1000000");
        TEST_ASSERT(approx_eq(sum.to_fp(), 1.0), "0.1 added 10 times should equal 1.0");
    }

    //-----------------------------------------------------------
    // Test to_string
    //-----------------------------------------------------------
    std::cout << "  Testing to_string..." << std::endl;
    {
        snap_fp<double, 1000000> a(3.14159);
        std::string s = a.to_string();
        // Just verify it doesn't crash and returns something reasonable.
        TEST_ASSERT(!s.empty(), "to_string should return non-empty string");
    }

    std::cout << "All tests PASSED!" << std::endl;
    return 0;
}
