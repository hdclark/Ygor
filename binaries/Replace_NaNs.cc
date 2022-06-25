//Replace_NaNs.cc -- A little program to replace NaNs in FITS images for better interoperativity.

#include <iostream>
#include <stdexcept>
#include <string>

#include "YgorImages.h"
#include "YgorImagesIO.h"

int main(int argc, char **argv){
    if(argc != 3) throw std::runtime_error("Must provide input and output filenames.");
    std::string FI(argv[1]);
    std::string FO(argv[2]);

    auto theimgs = ReadFromFITS<float,double>(FI);
    for(auto& img : theimgs.images){
        img.replace_nonfinite_pixels_with(0.0);
    }
    if(!WriteToFITS(theimgs,FO)){
        throw std::runtime_error("Unable to write FITS file.");
    }
    std::cout << "Image successfully written. Please verify it renders as expected." << std::endl;

    return 0;
}
