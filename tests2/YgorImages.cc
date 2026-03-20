
#include <limits>
#include <cmath>
#include <cstdint>
#include <functional>
#include <list>
#include <set>

#include <YgorMath.h>
#include <YgorImages.h>
#include <YgorImagesDithering.h>

#include "doctest/doctest.h"


TEST_CASE( "planar_image_adjacency" ){
    const auto eps = std::sqrt( 10.0 * std::numeric_limits<double>::epsilon() );

    const vec3<double> zero3(0.0, 0.0, 0.0);
    const vec3<double> row_unit(1.0, 0.0, 0.0);
    const vec3<double> col_unit(0.0, 1.0, 0.0);
    const vec3<double> ort_unit(0.0, 0.0, 1.0);

    planar_image<float,double> A;
    A.init_buffer(20, 40, 1);
    A.init_spatial(0.5, 1.5, 2.5, zero3, vec3<double>(5.0, 5.0, 5.0));
    A.init_orientation(row_unit, col_unit);
    A.fill_pixels(1.0f);

    planar_image<float,double> B;
    B.init_buffer(20, 40, 1);
    B.init_spatial(0.5, 1.5, 2.5, zero3, vec3<double>(5.0, 5.0, 7.5));
    B.init_orientation(row_unit, col_unit);
    B.fill_pixels(2.0f);

    planar_image<float,double> C;
    C.init_buffer(20, 40, 1);
    C.init_spatial(0.5, 1.5, 2.5, zero3, vec3<double>(5.0, 5.0, 10.0));
    C.init_orientation(row_unit, col_unit);
    C.fill_pixels(3.0f);

    std::list<std::reference_wrapper<planar_image<float,double>>> img_refws;
    img_refws.push_back( std::ref(A) );
    img_refws.push_back( std::ref(B) );
    img_refws.push_back( std::ref(C) );

    REQUIRE( Images_Form_Rectilinear_Grid<float,double>(img_refws) );

    // Compute adjacency information.
    planar_image_adjacency<float,double> adj( img_refws, {}, ort_unit );

    SUBCASE("adjacency index lookups return distinct images"){
        const auto index_b = adj.image_to_index( std::ref(B) );
        auto img_refw_0    = adj.index_to_image( index_b );
        auto img_refw_up   = adj.index_to_image( index_b + 1 );
        auto img_refw_down = adj.index_to_image( index_b - 1 );

        const auto val_0    = img_refw_0.get().value(0, 0, 0);
        const auto val_up   = img_refw_up.get().value(0, 0, 0);
        const auto val_down = img_refw_down.get().value(0, 0, 0);

        REQUIRE( val_0 != val_up );
        REQUIRE( val_0 != val_down );
        REQUIRE( val_up != val_down );
    }

    SUBCASE("trilinear interpolation"){
        const auto interp_A = adj.trilinearly_interpolate( vec3<double>(6.0, 6.0, 6.25), 0);
        const auto interp_B = adj.trilinearly_interpolate( vec3<double>(6.0, 6.0, 5.0), 0);
        const auto interp_C = adj.trilinearly_interpolate( vec3<double>(6.0, 6.0, 8.75), 0);

        REQUIRE( std::abs(interp_A - 1.5f) < eps );
        REQUIRE( std::abs(interp_B - 1.0f) < eps );
        REQUIRE( std::abs(interp_C - 2.5f) < eps );
    }
}


TEST_CASE( "Floyd-Steinberg dithering" ){

    SUBCASE("single channel 2x2 image"){
        planar_image<uint8_t,double> img;
        int64_t rows = 2;
        int64_t cols = 2;
        int64_t chns = 1;
        img.init_buffer(rows, cols, chns);

        img.reference(0, 0, 0) = 0;
        img.reference(0, 1, 0) = 128;
        img.reference(1, 0, 0) = 128;
        img.reference(1, 1, 0) = 255;

        Floyd_Steinberg_Dither(img);

        REQUIRE( img.value(0, 0, 0) == 0 );
        REQUIRE( img.value(0, 1, 0) == 255 );
        REQUIRE( img.value(1, 0, 0) == 0 );
        REQUIRE( img.value(1, 1, 0) == 255 );
    }

    SUBCASE("multi-channel with selected channel"){
        planar_image<uint8_t,double> img;
        int64_t rows = 2;
        int64_t cols = 2;
        int64_t chns = 2;
        img.init_buffer(rows, cols, chns);

        img.reference(0, 0, 0) = 10;
        img.reference(0, 1, 0) = 20;
        img.reference(1, 0, 0) = 30;
        img.reference(1, 1, 0) = 40;

        img.reference(0, 0, 1) = 0;
        img.reference(0, 1, 1) = 128;
        img.reference(1, 0, 1) = 128;
        img.reference(1, 1, 1) = 255;

        Floyd_Steinberg_Dither(img, std::set<int64_t>{1});

        // Unselected channel should not be modified.
        REQUIRE( img.value(0, 0, 0) == 10 );
        REQUIRE( img.value(0, 1, 0) == 20 );
        REQUIRE( img.value(1, 0, 0) == 30 );
        REQUIRE( img.value(1, 1, 0) == 40 );

        // Selected channel should be modified.
        REQUIRE( img.value(0, 0, 1) == 0 );
        REQUIRE( img.value(0, 1, 1) == 255 );
        REQUIRE( img.value(1, 0, 1) == 0 );
        REQUIRE( img.value(1, 1, 1) == 255 );
    }
}
