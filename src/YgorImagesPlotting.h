//YgorImagesPlotting.h
#pragma once
#ifndef YGOR_IMAGES_PLOTTING_HDR_GRD_H
#define YGOR_IMAGES_PLOTTING_HDR_GRD_H

#include <memory>
#include <string>
#include <sstream>

#include <cstdio>  //For popen, class FILE.

#include "YgorDefinitions.h"
#include "YgorImages.h"


//Plot an outline of the image. Useful for alignment testing.
template <class T, class R>
inline 
void Plot_Outline(const planar_image<T,R> &img){
    FILE *fp = popen("gnuplot --persist ", "w");
    if(fp == nullptr) throw std::runtime_error("Unable to open a pipe to gnuplot");

    fprintf(fp, "reset\n");
    fprintf(fp, "set title 'Image Outline'\n"); //Object with address %s\n', std::tostring(static_cast<intmax_t>((size_t)(std::addressof(*this)))).c_str());

    fprintf(fp, "set size 1.0,1.0\n"); //Width, Height, clamped to [0,1].
    fprintf(fp, "set autoscale fix\n");
    //fprintf(fp, "set palette defined (0 'black', 1 'white')\n");
    //fprintf(fp, "set palette grey\n");
    fprintf(fp, "set tics scale 0\n");
    //fprintf(fp, "unset cbtics\n");
    //fprintf(fp, "unset key\n");
    fprintf(fp, "set xlabel 'x' \n set ylabel 'y' \n set zlabel 'z' \n");


    fprintf(fp, "$DATA << EOD\n");
    vec3<R> r1 = img.position(         0,             0);
    vec3<R> r2 = img.position(img.rows-1,             0);
    vec3<R> r3 = img.position(img.rows-1, img.columns-1);
    vec3<R> r4 = img.position(         0, img.columns-1);

    fprintf(fp, "%lf %lf %lf\n", static_cast<double>(r1.x), static_cast<double>(r1.y), static_cast<double>(r1.z));
    fprintf(fp, "%lf %lf %lf\n", static_cast<double>(r2.x), static_cast<double>(r2.y), static_cast<double>(r2.z));
    fprintf(fp, "%lf %lf %lf\n", static_cast<double>(r3.x), static_cast<double>(r3.y), static_cast<double>(r3.z));
    fprintf(fp, "%lf %lf %lf\n", static_cast<double>(r4.x), static_cast<double>(r4.y), static_cast<double>(r4.z));
    fprintf(fp, "%lf %lf %lf\n", static_cast<double>(r1.x), static_cast<double>(r1.y), static_cast<double>(r1.z));
    fprintf(fp, "EOD\n");
    fprintf(fp, "splot '$DATA' with linespoints lt 1 title ''\n");

    pclose(fp);
    return;
}

//Plot the pixels, disregarding spatial information.
template <class T, class R> 
inline 
void Plot_Pixels(const planar_image<T,R> &img, long int chnl){
    if(!isininc(0,chnl,img.channels-1)){
        throw std::runtime_error("Invalid channel specified.");
    }

    FILE *fp = popen("gnuplot --persist ", "w");
    if(fp == nullptr) throw std::runtime_error("Unable to open a pipe to gnuplot");

    const auto HWAspectRatio = static_cast<double>(img.pxl_dx * img.rows)
                             / static_cast<double>(img.pxl_dy * img.columns);

    fprintf(fp, "reset\n");
    fprintf(fp, "set size 1.0,1.0\n"); //Width, Height, clamped to [0,1].
    if(std::isfinite(HWAspectRatio)) fprintf(fp, "set size ratio %lf\n", HWAspectRatio);
    fprintf(fp, "set autoscale fix\n");
    fprintf(fp, "set palette defined (0 'black', 1 'white')\n");
    //fprintf(fp, "set palette grey\n");
    fprintf(fp, "set tics scale 0\n");
    //fprintf(fp, "unset cbtics\n");
    fprintf(fp, "unset key\n");

    fprintf(fp, "plot '-' binary array=(%ld,%ld) format='%%float' with image\n", img.columns, img.rows);
    for(auto row = 0; row < img.rows; ++row){
        for(auto col = 0; col < img.columns; ++col){
            auto val = static_cast<float>(img.value((img.rows-1-row), col, chnl));
            const size_t res = fwrite(reinterpret_cast<void *>(&val), sizeof(float), 1, fp);
            if(res != 1) throw std::runtime_error("Unable to write to pipe");
        }
    }
    fprintf(fp, "\n");
    pclose(fp);
    return;
}

//Plot the pixels, disregarding spatial information.
template <class T, class R> 
inline
void Plot_Pixels_RGB(const planar_image<T,R> &img, long int Rchnl = 0, long int Gchnl = 1, long int Bchnl = 2){
    if(!isininc(0,Rchnl,img.channels-1)
    || !isininc(0,Bchnl,img.channels-1)
    || !isininc(0,Bchnl,img.channels-1) ){
        throw std::runtime_error("Invalid channel(s) specified.");
    }
    const std::vector<long int> channels = {Rchnl,Gchnl,Bchnl};

    FILE *fp = popen("gnuplot --persist ", "w");
    if(fp == nullptr) throw std::runtime_error("Unable to open a pipe to gnuplot");

    const auto HWAspectRatio = static_cast<double>(img.pxl_dx * img.rows)
                             / static_cast<double>(img.pxl_dy * img.columns);

    fprintf(fp, "reset\n");
    fprintf(fp, "set size 1.0,1.0\n"); //Width, Height, clamped to [0,1].
    if(std::isfinite(HWAspectRatio)) fprintf(fp, "set size ratio %lf\n", HWAspectRatio);
    fprintf(fp, "set autoscale fix\n");
    fprintf(fp, "set tics scale 0\n");
    fprintf(fp, "unset key\n");

    fprintf(fp, "plot '-' binary array=(%ld,%ld) format='%%float' with rgbimage\n", img.columns, img.rows);
    for(auto row = 0; row < img.rows; ++row){
        for(auto col = 0; col < img.columns; ++col){
            for(const auto & chnl : channels){
                auto val = static_cast<float>(img.value((img.rows-1-row), col, chnl));
                const size_t res = fwrite(reinterpret_cast<void *>(&val), sizeof(float), 1, fp);
                if(res != 1) throw std::runtime_error("Unable to write to pipe");
            }
        }
    }
    fprintf(fp, "\n");
    pclose(fp);
    return;
}



template <class T,class R>
inline
void Plot_Outlines(const planar_image_collection<T,R> &imgcoll, std::string title = ""){

    FILE *fp = popen("gnuplot --persist ", "w");
    if(fp == nullptr) throw std::runtime_error("Unable to open a pipe to gnuplot");

    fprintf(fp, "reset\n");
    if(title.empty()){
        fprintf(fp, "unset title\n");
    }else{
        fprintf(fp, "set title '%s'\n", title.c_str());
    }
    //fprintf(fp, "set size 1.0,1.0\n"); //Width, Height, clamped to [0,1].
    fprintf(fp, "set autoscale fix\n");
    //fprintf(fp, "set tics scale 0\n");
    //fprintf(fp, "set palette defined (0 'black', 1 'white')\n");
    //fprintf(fp, "set palette grey\n");
    //fprintf(fp, "unset cbtics\n");
    //fprintf(fp, "unset key\n");
    fprintf(fp, "set xlabel 'x' \n set ylabel 'y' \n set zlabel 'z' \n");

    const std::string base("$DATA");
    size_t i = 0;
    std::stringstream ss;
    for(const auto &img : imgcoll.images){
        const auto var_name = base + std::to_string(i);

        //Send the data as a gnuplot variable.
        fprintf(fp, "%s << EOD\n", var_name.c_str());
        vec3<R> r1 = img.position(         0,             0);
        vec3<R> r2 = img.position(img.rows-1,             0);
        vec3<R> r3 = img.position(img.rows-1, img.columns-1);
        vec3<R> r4 = img.position(         0, img.columns-1);
    
        fprintf(fp, "%lf %lf %lf\n", static_cast<double>(r1.x), static_cast<double>(r1.y), static_cast<double>(r1.z));
        fprintf(fp, "%lf %lf %lf\n", static_cast<double>(r2.x), static_cast<double>(r2.y), static_cast<double>(r2.z));
        fprintf(fp, "%lf %lf %lf\n", static_cast<double>(r3.x), static_cast<double>(r3.y), static_cast<double>(r3.z));
        fprintf(fp, "%lf %lf %lf\n", static_cast<double>(r4.x), static_cast<double>(r4.y), static_cast<double>(r4.z));
        fprintf(fp, "%lf %lf %lf\n", static_cast<double>(r1.x), static_cast<double>(r1.y), static_cast<double>(r1.z));
        fprintf(fp, "EOD\n");
 
        //Prepare the portion of the final plotting command that depends on this data.
        // Note:  [splot ] ... <previous portion> <this portion> <next portion> ...
        ss << " '" << var_name << "' with linespoints lt 1 title 'Image " << i << "', ";
        ++i;
    }    

    fprintf(fp, "splot %s\n", ss.str().c_str());
    pclose(fp);
    return;
}

#endif
