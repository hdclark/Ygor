
#include <limits>
#include <cmath>
#include <sstream>
#include <string>

#include <YgorMath.h>

#include "doctest/doctest.h"


TEST_CASE( "samples_1D metadata serialization" ){
    using namespace std::string_literals;

    samples_1D<double> buffa;
    for(double x = 0.0; x < 10.0; x += 0.15){
        const double f = std::sin(x) * std::exp(-x/2.0);
        const double df = f * 0.05;
        const double dx = x * 0.005;
        buffa.push_back(x, dx, f, df);
    }

    // Inject metadata.
    const auto key_A = "key_A";
    const auto val_A = "val_A";

    const auto key_B = "key\tB";
    const auto val_B = "val_B";

    const auto key_C = "key_C";
    const auto val_C = "val\tC";

    const auto key_D = " very long key D with nasty characters \t \n \0 "s;
    const auto val_D = " very long val D with nasty characters \t \n \0 "s;

    buffa.metadata[key_A] = val_A;
    buffa.metadata[key_B] = val_B;
    buffa.metadata[key_C] = val_C;
    buffa.metadata[key_D] = val_D;

    SUBCASE("operator<< and operator>> round-trip preserves metadata"){
        samples_1D<double> buffb;

        std::stringstream ss;
        ss << buffa;
        ss >> buffb;

        REQUIRE( buffb.metadata[key_A] == val_A );
        REQUIRE( buffb.metadata[key_B] == val_B );
        REQUIRE( buffb.metadata[key_C] == val_C );
        REQUIRE( buffb.metadata[key_D] == val_D );
    }

    SUBCASE("Write_To_Stream and Read_From_Stream round-trip preserves metadata"){
        samples_1D<double> buffb;

        std::stringstream ss;
        buffa.Write_To_Stream(ss);
        buffb.Read_From_Stream(ss);

        REQUIRE( buffb.metadata[key_A] == val_A );
        REQUIRE( buffb.metadata[key_B] == val_B );
        REQUIRE( buffb.metadata[key_C] == val_C );
        REQUIRE( buffb.metadata[key_D] == val_D );
    }
}


TEST_CASE( "samples_1D median" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("median of 6 elements"){
        samples_1D<double> test;
        test.push_back(  1.0, 0.0,  2.0, 0.0 );
        test.push_back(  2.0, 0.0, 14.0, 0.0 );
        test.push_back(  4.0, 0.0,  2.0, 0.0 );
        test.push_back(  7.0, 0.0,  5.0, 0.0 );
        test.push_back(  1.0, 0.0,  8.0, 0.0 );
        test.push_back(  6.0, 0.0,  1.0, 0.0 );

        // median of 1, 2, 4, 7, 1, 6  = 3
        // median of 2, 14, 2, 5, 8, 1 = 3.5
        REQUIRE( std::abs(test.Median_x()[0] - 3.0) < eps );
        REQUIRE( std::abs(test.Median_y()[0] - 3.5) < eps );
    }

    SUBCASE("median of 7 elements"){
        samples_1D<double> test;
        test.push_back(  1.0, 0.0,  2.0, 0.0 );
        test.push_back(  2.0, 0.0, 14.0, 0.0 );
        test.push_back(  4.0, 0.0,  2.0, 0.0 );
        test.push_back( 53.0, 0.0, 53.0, 0.0 );
        test.push_back(  7.0, 0.0,  5.0, 0.0 );
        test.push_back(  1.0, 0.0,  8.0, 0.0 );
        test.push_back(  6.0, 0.0,  1.0, 0.0 );

        // median of 1, 2, 4, 53, 7, 1, 6  = 4
        // median of 2, 14, 2, 53, 5, 8, 1 = 5
        REQUIRE( std::abs(test.Median_x()[0] - 4.0) < eps );
        REQUIRE( std::abs(test.Median_y()[0] - 5.0) < eps );
    }
}


TEST_CASE( "samples_1D linear interpolation" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("lowest-x first orientation"){
        samples_1D<double> testarray( { vec2<double>(0.0, 0.0),
                                        vec2<double>(1.0, 1.0),
                                        vec2<double>(2.0, 0.0),
                                        vec2<double>(3.0, 2.0) } );

        REQUIRE( std::abs(testarray.Interpolate_Linearly(-1E9)[2] - 0.0) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly( 0.0)[2] - 0.0) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly( 0.5)[2] - 0.5) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly( 1.0)[2] - 1.0) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly( 1.5)[2] - 0.5) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly( 2.0)[2] - 0.0) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly(2.25)[2] - 0.5) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly( 3.0)[2] - 2.0) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly( 1E9)[2] - 0.0) < eps );
    }

    SUBCASE("lowest-x last orientation (auto-sorted)"){
        samples_1D<double> testarray( { vec2<double>(3.0, 2.0),
                                        vec2<double>(2.0, 0.0),
                                        vec2<double>(1.0, 1.0),
                                        vec2<double>(0.0, 0.0) } );

        REQUIRE( std::abs(testarray.Interpolate_Linearly(-1E9)[2] - 0.0) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly( 0.0)[2] - 0.0) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly( 0.5)[2] - 0.5) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly( 1.0)[2] - 1.0) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly( 1.5)[2] - 0.5) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly( 2.0)[2] - 0.0) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly(2.25)[2] - 0.5) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly( 3.0)[2] - 2.0) < eps );
        REQUIRE( std::abs(testarray.Interpolate_Linearly( 1E9)[2] - 0.0) < eps );
    }
}


TEST_CASE( "samples_1D overlap integration" ){
    const double tol = 0.01;
    const auto pi = std::acos(-1.0);

    SUBCASE("rectangle overlap is 1.0"){
        samples_1D<double> f( { vec2<double>(0.0, 1.0),
                                vec2<double>(1.0, 1.0) } );
        samples_1D<double> g( { vec2<double>(0.0, 1.0),
                                vec2<double>(1.0, 1.0) } );

        REQUIRE( std::abs(f.Integrate_Overlap(g)[0] - 1.0) < tol );
        REQUIRE( std::abs(g.Integrate_Overlap(f)[0] - 1.0) < tol );
    }

    SUBCASE("rectangle overlap backwards-defined g is 1.0"){
        samples_1D<double> f( { vec2<double>(0.0, 1.0),
                                vec2<double>(1.0, 1.0) } );
        samples_1D<double> g( { vec2<double>(1.0, 1.0),
                                vec2<double>(0.0, 1.0) } );

        REQUIRE( std::abs(f.Integrate_Overlap(g)[0] - 1.0) < tol );
        REQUIRE( std::abs(g.Integrate_Overlap(f)[0] - 1.0) < tol );
    }

    SUBCASE("wider f with partial overlap g is 1.0"){
        samples_1D<double> f( { vec2<double>(0.0, 1.0),
                                vec2<double>(1.0, 1.0),
                                vec2<double>(2.0, 1.0),
                                vec2<double>(3.0, 1.0) } );
        samples_1D<double> g( { vec2<double>(0.0, 1.0),
                                vec2<double>(1.0, 1.0) } );

        REQUIRE( std::abs(f.Integrate_Overlap(g)[0] - 1.0) < tol );
        REQUIRE( std::abs(g.Integrate_Overlap(f)[0] - 1.0) < tol );
    }

    SUBCASE("sin*cos over [0,2pi] is ~0"){
        samples_1D<double> f, g;
        for(auto x = 0.0; x < 2.0 * pi; x += 0.005){
            f.push_back( vec2<double>( x, std::sin(x) ) );
            g.push_back( vec2<double>( x, std::cos(x) ) );
        }

        REQUIRE( std::abs(f.Integrate_Overlap(g)[0]) < tol );
        REQUIRE( std::abs(g.Integrate_Overlap(f)[0]) < tol );
    }

    SUBCASE("complex trig overlap ~3.2866"){
        samples_1D<double> f, g;
        for(auto x = 0.0; x < 2.0 * pi; x += 0.005){
            f.push_back( vec2<double>( x, std::sin(x)*std::sin(x) + std::cos(x)*std::sin(x+0.1) ) );
            g.push_back( vec2<double>( x, std::sin(x+0.3) + std::cos(x/10.0) ) );
        }

        REQUIRE( std::abs(f.Integrate_Overlap(g)[0] - 3.2866) < tol );
        REQUIRE( std::abs(g.Integrate_Overlap(f)[0] - 3.2866) < tol );
    }
}


TEST_CASE( "samples_1D normalization" ){
    const double tol = 0.01;

    SUBCASE("self-overlap after normalization is 1.0"){
        samples_1D<double> f( { vec2<double>(0.0, 1.0),
                                vec2<double>(1.0, 1.0),
                                vec2<double>(2.0, 1.0),
                                vec2<double>(3.0, 1.0) } );

        f.Normalize_wrt_Self_Overlap();
        REQUIRE( std::abs(f.Integrate_Overlap(f)[0] - 1.0) < tol );
    }

    SUBCASE("normalization with uncertainties"){
        samples_1D<double> f( { { -1.0, 1.0, 0.0, 1.0 },
                                {  0.0, 0.5, 1.0, 0.3 },
                                {  1.0, 2.5, 2.0, 1.3 },
                                {  2.0, 0.1, 4.0, 0.1 } } );

        f.Normalize_wrt_Self_Overlap();
        REQUIRE( std::abs(f.Integrate_Overlap(f)[0] - 1.0) < tol );
    }
}


TEST_CASE( "samples_1D summation" ){
    const auto eps = std::sqrt( std::numeric_limits<double>::epsilon() );

    SUBCASE("sum of two identical constant functions produces doubled values"){
        samples_1D<double> f( { vec2<double>(0.0, 1.0),
                                vec2<double>(1.0, 1.0),
                                vec2<double>(2.0, 1.0),
                                vec2<double>(3.0, 1.0) } );
        samples_1D<double> g( { vec2<double>(0.0, 1.0),
                                vec2<double>(1.0, 1.0),
                                vec2<double>(2.0, 1.0),
                                vec2<double>(3.0, 1.0) } );

        auto h = f.Sum_With(g);
        REQUIRE( h.samples.size() == 4 );
        for(const auto &s : h.samples){
            REQUIRE( std::abs(s[2] - 2.0) < eps );
        }
    }
}


TEST_CASE( "samples_1D weighted linear regression" ){
    SUBCASE("known regression values"){
        samples_1D<double> dat({ { 1.00, 0.00, 2.0000, 1.00 },
                                 { 1.05, 0.00, 3.0500, 1.00 },
                                 { 1.10, 0.00, 10.600, 1.00 },
                                 { 1.40, 0.00, 1000.0, 1.00 } });
        auto res = dat.Weighted_Linear_Least_Squares_Regression();

        REQUIRE( res.N == 4 );
        REQUIRE( res.dof == 2 );
        REQUIRE( std::abs(res.slope - 2699.98) < 1.0 );
    }
}


TEST_CASE( "samples_1D Integrate_Over_Kernel_exp" ){
    const double tol = 0.01;
    const auto pi = std::acos(-1.0);

    SUBCASE("expected value ~3.9356"){
        const bool inhibit_sort = true;
        samples_1D<double> f;
        double dx = 0.01;
        for(auto x = 0.0; x < 2.0 * pi; x += dx){
            const auto f_at_x = std::sin(x)*std::sin(x) + std::cos(x)*std::sin(x+0.1)
                              + std::sin(x+0.3) + std::cos(x/10.0);
            f.push_back( { x, 0.5*dx, f_at_x, f_at_x*0.1 }, inhibit_sort );
        }
        f.uncertainties_known_to_be_independent_and_random = true;

        const auto F = f.Integrate_Over_Kernel_exp(0.0, 2.0*pi, { -0.5, 0.1 }, { 0.0, 0.01 });
        REQUIRE( std::abs(F[0] - 3.9356) < tol );
    }
}


TEST_CASE( "samples_1D Mean and Average" ){
    const double tol = 0.01;

    SUBCASE("Mean_x, Mean_y, Average_x, Average_y"){
        samples_1D<double> f( { { 0.5, 0.0, 1.1, 0.0 },
                                { 1.0, 0.0, 1.5, 0.0 },
                                { 1.1, 0.0, 1.2, 0.0 },
                                { 3.5, 0.0, 1.3, 0.0 } } );
        const auto meanx = f.Mean_x();
        const auto avgx  = f.Average_x();
        const auto meany = f.Mean_y();
        const auto avgy  = f.Average_y();

        // Mean_x = (0.5+1.0+1.1+3.5)/4 = 1.525
        REQUIRE( std::abs(meanx[0] - 1.525) < tol );
        // Mean_y = (1.1+1.5+1.2+1.3)/4 = 1.275
        REQUIRE( std::abs(meany[0] - 1.275) < tol );
        // Average_x should also be defined
        REQUIRE( std::abs(avgx[0] - 1.525) < tol );
        // Average_y
        REQUIRE( std::abs(avgy[0] - 1.275) < tol );
    }
}
