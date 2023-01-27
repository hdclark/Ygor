#include <memory>
#include <iostream>
#include <list>

#include "YgorMisc.h"
#include "YgorLog.h"
#include "YgorMath.h"
#include "YgorImages.h"
#include "YgorImagesIO.h"
#include "YgorImagesPlotting.h"


int main(int argc, char **argv){
    std::list<planar_image<unsigned int,double>> collection;

  if(true){
    class planar_image<unsigned int,double> animg;
    const long int ROWS  = 10;
    const long int COLS  = 50;
    const long int CHNLS = 4;
    animg.init_buffer(ROWS, COLS, CHNLS);
    animg.reference(8,49,3) = 4200;
    std::cout << "After setting the value to 4200, we get the value: " << animg.value(8,49,3) << std::endl;
  }

  if(true){
    class planar_image<unsigned int,double> another;
    const long int ROWS  = 10;
    const long int COLS  = 50;
    const long int CHNLS = 4;
    another.init_buffer(ROWS, COLS, CHNLS);
    another.reference(8,49,3) = 4200;
    std::cout << "After setting the value to 4200, we get the value: " << another.value(8,49,3) << std::endl;
  }

  if(true){
    class planar_image<unsigned int,double> another;
    const long int ROWS  = 50;
    const long int COLS  = 50;
    const long int CHNLS = 100; //Lots of channels!
    double pxl_dx = 2.0;
    double pxl_dy = 2.0;
    double pxl_dz = 5000.0; //"Thickness" - arbitrary!

    vec3<double> anchor(-100.0,-200.0,-400.0); //Arbitrary common origin from which to offset from.
    vec3<double> offset( 100.0, 200.0, 400.0);

    vec3<double> row_unit(1.0,0.0,0.0);
    vec3<double> col_unit(0.0,1.0,0.0); 

    another.init_buffer(ROWS, COLS, CHNLS);
    another.init_spatial(pxl_dx,pxl_dy,pxl_dz, anchor,offset);
    another.init_orientation(row_unit,col_unit);

    //Spit out the (center of the) pixels in the corner of the image.
    std::cout << "The location of pixel( 0, 0) is " << another.position( 0, 0) << std::endl;
    std::cout << "The location of pixel(49, 0) is " << another.position(49, 0) << std::endl;
    std::cout << "The location of pixel( 0,49) is " << another.position( 0,49) << std::endl;
    std::cout << "The location of pixel(49,49) is " << another.position(49,49) << std::endl;

    Plot_Outline(another);
    collection.push_back(std::move(another));
  }

  if(true){
    class planar_image<unsigned int,double> another;
    const long int ROWS  = 50;
    const long int COLS  = 50;
    const long int CHNLS = 100; //Lots of channels!
    double pxl_dx = 2.0;
    double pxl_dy = 3.0;
    double pxl_dz = 5000.0; //"Thickness" - arbitrary!

    vec3<double> anchor(-100.0,-200.0,-400.0); //Arbitrary common origin from which to offset from.
    vec3<double> offset( 100.0, 200.0, 410.0);

    vec3<double> row_unit(0.0,-1.0,0.0);
    vec3<double> col_unit(1.0,0.0,0.0);

    another.init_buffer(ROWS, COLS, CHNLS);
    another.init_spatial(pxl_dx,pxl_dy,pxl_dz, anchor,offset);
    another.init_orientation(row_unit,col_unit);

    //Spit out the (center of the) pixels in the corner of the image.
    std::cout << "The location of pixel( 0, 0) is " << another.position( 0, 0) << std::endl;
    std::cout << "The location of pixel(49, 0) is " << another.position(49, 0) << std::endl;
    std::cout << "The location of pixel( 0,49) is " << another.position( 0,49) << std::endl;
    std::cout << "The location of pixel(49,49) is " << another.position(49,49) << std::endl;

    collection.push_back(std::move(another));
  }

  if(true){
    class planar_image_collection<unsigned int,double> someimgs(collection);
    Plot_Outlines(someimgs);


  }

  if(true){
    class planar_image_collection<float,double> imgs;
    imgs.images.emplace_back();
    const long int ROWS  = 512;
    const long int COLS  = 1024; //wide image.
    const long int CHNLS = 1;

    imgs.images.back().init_buffer(ROWS, COLS, CHNLS);

    for(size_t row = 0; row < ROWS; ++row){
        for(size_t col = 0; col < COLS; ++col){
            imgs.images.back().reference(row,col,0) = 1.1274671245*col;
            //if(row == col) img.reference(row,col,0) = 1.1274671245*row;
            //else           img.reference(row,col,0) = 0.0;
        }
    }

    if(!WriteToFITS(imgs, "/tmp/test_image_be.fits")) YLOGERR("Could not write test image to big endian FITS file");
    if(!WriteToFITS(imgs, "/tmp/test_image_le.fits", YgorEndianness::Little))  YLOGERR("Could not write test image to little endian FITS file");

  }

  if(true){
    auto imgs = ReadFromFITS<float,double>("/tmp/test_image_be.fits");
    for(const auto& img : imgs.images) Plot_Pixels(img,0);
  }

  if(true){
    class planar_image_collection<float,double> imgsA;
    const long int ROWS  = 512;
    const long int COLS  = 1024; //wide image.
    const long int CHNLS = 3;

    imgsA.images.emplace_back();
    imgsA.images.back().init_buffer(ROWS, COLS, CHNLS);
    imgsA.images.back().init_spatial( 1.0, 1.0, 1.0, vec3<double>(0.0,0.0,0.0), vec3<double>(0.0,0.0,0.0) );
    imgsA.images.back().init_orientation( vec3<double>(1.0,0.0,0.0), vec3<double>(0.0,1.0,0.0) );

    for(size_t row = 0; row < ROWS; ++row){
        for(size_t col = 0; col < COLS; ++col){
            imgsA.images.back().reference(row,col,0) = 1.1274671245*col;
            imgsA.images.back().reference(row,col,1) = -2.236*row;
            imgsA.images.back().reference(row,col,2) = 0.04*row*col;
        }
    }
    //Plot_Pixels_RGB(imgA);
    //Plot_Pixels(imgA,0);
    //Plot_Pixels(imgA,1);
    //Plot_Pixels(imgA,2);

    if(!WriteToFITS(imgsA, "/tmp/test_colour_imageA.fits")) YLOGERR("Could not write test colour image file A");

    auto imgsB = ReadFromFITS<float,double>("/tmp/test_colour_imageA.fits");
    //Plot_Pixels_RGB(imgB);
    //Plot_Pixels(imgB,0);
    //Plot_Pixels(imgB,1);
    //Plot_Pixels(imgB,2);

    if(!WriteToFITS(imgsB, "/tmp/test_colour_imageB.fits")) YLOGERR("Could not write test colour image file B");

    auto imgsC = ReadFromFITS<float,double>("/tmp/test_colour_imageB.fits");
    //Plot_Pixels_RGB(imgC);
    //Plot_Pixels(imgC,0);
    //Plot_Pixels(imgC,1);
    //Plot_Pixels(imgC,2);

    if(!WriteToFITS(imgsC, "/tmp/test_colour_imageC.fits")) YLOGERR("Could not write test colour image file C");

    //Now check files B and C. They should ideally be identical, except for maybe some floating-point loses.
  }

    return 0;
}




