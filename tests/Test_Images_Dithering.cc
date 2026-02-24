//Test_Images_04.cc.

#include <cstdint>
#include <stdexcept>
#include <set>

#include "YgorImages.h"
#include "YgorImagesDithering.h"


int main(int, char**){
    {
        planar_image<uint8_t,double> img;
        img.init_buffer(2,2,1);

        img.reference(0,0,0) = 0;
        img.reference(0,1,0) = 128;
        img.reference(1,0,0) = 128;
        img.reference(1,1,0) = 255;

        Floyd_Steinberg_Dither(img);

        if(img.value(0,0,0) != 0){
            throw std::runtime_error("Unexpected dither result at (0,0)");
        }
        if(img.value(0,1,0) != 255){
            throw std::runtime_error("Unexpected dither result at (0,1)");
        }
        if(img.value(1,0,0) != 0){
            throw std::runtime_error("Unexpected dither result at (1,0)");
        }
        if(img.value(1,1,0) != 255){
            throw std::runtime_error("Unexpected dither result at (1,1)");
        }
    }

    {
        planar_image<uint8_t,double> img;
        img.init_buffer(2,2,2);

        img.reference(0,0,0) = 10;
        img.reference(0,1,0) = 20;
        img.reference(1,0,0) = 30;
        img.reference(1,1,0) = 40;

        img.reference(0,0,1) = 0;
        img.reference(0,1,1) = 128;
        img.reference(1,0,1) = 128;
        img.reference(1,1,1) = 255;

        Floyd_Steinberg_Dither(img, std::set<int64_t>{1});

        if(img.value(0,0,0) != 10 || img.value(0,1,0) != 20
        || img.value(1,0,0) != 30 || img.value(1,1,0) != 40){
            throw std::runtime_error("Unselected channel was modified");
        }

        if(img.value(0,0,1) != 0 || img.value(0,1,1) != 255
        || img.value(1,0,1) != 0 || img.value(1,1,1) != 255){
            throw std::runtime_error("Selected channel did not dither as expected");
        }
    }

    return 0;
}
