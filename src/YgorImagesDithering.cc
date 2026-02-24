//YgorImagesDithering.cc - Error-diffusion dithering routines.

#include <cmath>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "YgorImagesDithering.h"


template <class T, class R>
void
Floyd_Steinberg_Dither(planar_image<T,R> &img,
                       std::set<int64_t> chnls,
                       T low,
                       T high,
                       long double threshold){
    if(high < low){
        throw std::invalid_argument("Dithering bounds are inconsistent");
    }

    if(std::isnan(threshold)){
        threshold = (static_cast<long double>(low) + static_cast<long double>(high)) / 2.0L;
    }

    if constexpr (std::is_floating_point<T>::value){
        if( !std::isfinite(static_cast<long double>(low))
        ||  !std::isfinite(static_cast<long double>(high))
        ||  !std::isfinite(threshold) ){
            throw std::invalid_argument("Dithering bounds and threshold must be finite");
        }
    }

    if(chnls.empty()){
        for(int64_t chnl = 0; chnl < img.channels; ++chnl){
            chnls.insert(chnl);
        }
    }

    for(const auto chnl : chnls){
        if((chnl < 0) || (img.channels <= chnl)){
            throw std::invalid_argument("Requested channel is out-of-bounds");
        }

        std::vector<long double> channel_data(static_cast<size_t>(img.rows * img.columns));
        for(int64_t row = 0; row < img.rows; ++row){
            for(int64_t col = 0; col < img.columns; ++col){
                const auto i = static_cast<size_t>(row * img.columns + col);
                channel_data.at(i) = static_cast<long double>(img.value(row, col, chnl));
            }
        }

        for(int64_t row = 0; row < img.rows; ++row){
            for(int64_t col = 0; col < img.columns; ++col){
                const auto i = static_cast<size_t>(row * img.columns + col);

                const auto old_val = channel_data.at(i);
                const auto new_val = (threshold <= old_val) ? static_cast<long double>(high)
                                                             : static_cast<long double>(low);
                channel_data.at(i) = new_val;

                const auto quantization_error = old_val - new_val;
                if((col + 1) < img.columns){
                    channel_data.at(i + 1) += quantization_error * (7.0L / 16.0L);
                }
                if((row + 1) < img.rows){
                    if(0 < col){
                        channel_data.at(i + static_cast<size_t>(img.columns - 1)) += quantization_error * (3.0L / 16.0L);
                    }
                    channel_data.at(i + static_cast<size_t>(img.columns)) += quantization_error * (5.0L / 16.0L);
                    if((col + 1) < img.columns){
                        channel_data.at(i + static_cast<size_t>(img.columns + 1)) += quantization_error * (1.0L / 16.0L);
                    }
                }
            }
        }

        for(int64_t row = 0; row < img.rows; ++row){
            for(int64_t col = 0; col < img.columns; ++col){
                const auto i = static_cast<size_t>(row * img.columns + col);
                img.reference(row, col, chnl) = static_cast<T>(channel_data.at(i));
            }
        }
    }

    return;
}

#ifndef YGOR_IMAGES_DITHERING_DISABLE_ALL_SPECIALIZATIONS
    template void Floyd_Steinberg_Dither<uint8_t ,double>(planar_image<uint8_t ,double> &, std::set<int64_t>, uint8_t , uint8_t , long double);
    template void Floyd_Steinberg_Dither<uint16_t,double>(planar_image<uint16_t,double> &, std::set<int64_t>, uint16_t, uint16_t, long double);
    template void Floyd_Steinberg_Dither<uint32_t,double>(planar_image<uint32_t,double> &, std::set<int64_t>, uint32_t, uint32_t, long double);
    template void Floyd_Steinberg_Dither<uint64_t,double>(planar_image<uint64_t,double> &, std::set<int64_t>, uint64_t, uint64_t, long double);
    template void Floyd_Steinberg_Dither<float   ,double>(planar_image<float   ,double> &, std::set<int64_t>, float   , float   , long double);
    template void Floyd_Steinberg_Dither<double  ,double>(planar_image<double  ,double> &, std::set<int64_t>, double  , double  , long double);
#endif
