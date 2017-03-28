#include <memory>
#include <iostream>
#include <list>

#include "YgorMisc.h"
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
    class planar_image<float,double> img;
    const long int ROWS  = 512;
    const long int COLS  = 1024; //wide image.
    const long int CHNLS = 1;

    img.init_buffer(ROWS, COLS, CHNLS);

    for(size_t row = 0; row < ROWS; ++row){
        for(size_t col = 0; col < COLS; ++col){
            img.reference(row,col,0) = 1.1274671245*col;
            //if(row == col) img.reference(row,col,0) = 1.1274671245*row;
            //else           img.reference(row,col,0) = 0.0;
        }
    }

    if(!WriteToFITS(img, "/tmp/test_image_be.fits")) FUNCERR("Could not write test image to big endian FITS file");
    if(!WriteToFITS(img, "/tmp/test_image_le.fits", YgorImageIOEndianness::Little))  FUNCERR("Could not write test image to little endian FITS file");

  }

  if(true){
    auto img = ReadFromFITS<float,double>("/tmp/test_image_be.fits");
    Plot_Pixels(img,0);

    auto img2 = ReadFromFITS<uint8_t,double>("smiley.fit");
    img2.init_spatial( 1.0, 1.0, 1.0, vec3<double>(0.0,0.0,0.0), vec3<double>(0.0,0.0,0.0) );
    Plot_Pixels(img2,0);
    //Plot_Pixels_RGB(img2,0,0,0);

    auto img3 = ReadFromFITS<uint8_t,double>("wonky_test_image.fits");
    img3.init_spatial( 1.0, 1.0, 1.0, vec3<double>(0.0,0.0,0.0), vec3<double>(0.0,0.0,0.0) );
    Plot_Pixels(img3,0);

    auto img4 = ReadFromFITS<uint8_t,double>("wonky_test_image_rgb_colored.fits");
    img4.init_spatial( 1.0, 1.0, 1.0, vec3<double>(0.0,0.0,0.0), vec3<double>(0.0,0.0,0.0) );
    Plot_Pixels_RGB(img4,0,1,2);
    Plot_Pixels(img4,0);
    Plot_Pixels(img4,1);
    Plot_Pixels(img4,2);
  }

  if(true){
    class planar_image<float,double> imgA;
    const long int ROWS  = 512;
    const long int COLS  = 1024; //wide image.
    const long int CHNLS = 3;

    imgA.init_buffer(ROWS, COLS, CHNLS);
    imgA.init_spatial( 1.0, 1.0, 1.0, vec3<double>(0.0,0.0,0.0), vec3<double>(0.0,0.0,0.0) );
    imgA.init_orientation( vec3<double>(1.0,0.0,0.0), vec3<double>(0.0,1.0,0.0) );

    for(size_t row = 0; row < ROWS; ++row){
        for(size_t col = 0; col < COLS; ++col){
            imgA.reference(row,col,0) = 1.1274671245*col;
            imgA.reference(row,col,1) = -2.236*row;
            imgA.reference(row,col,2) = 0.04*row*col;
        }
    }
    //Plot_Pixels_RGB(imgA);
    //Plot_Pixels(imgA,0);
    //Plot_Pixels(imgA,1);
    //Plot_Pixels(imgA,2);

    if(!WriteToFITS(imgA, "/tmp/test_colour_imageA.fits")) FUNCERR("Could not write test colour image file A");

    auto imgB = ReadFromFITS<float,double>("/tmp/test_colour_imageA.fits");
    //Plot_Pixels_RGB(imgB);
    //Plot_Pixels(imgB,0);
    //Plot_Pixels(imgB,1);
    //Plot_Pixels(imgB,2);

    if(!WriteToFITS(imgB, "/tmp/test_colour_imageB.fits")) FUNCERR("Could not write test colour image file B");

    auto imgC = ReadFromFITS<float,double>("/tmp/test_colour_imageB.fits");
    //Plot_Pixels_RGB(imgC);
    //Plot_Pixels(imgC,0);
    //Plot_Pixels(imgC,1);
    //Plot_Pixels(imgC,2);

    if(!WriteToFITS(imgC, "/tmp/test_colour_imageC.fits")) FUNCERR("Could not write test colour image file C");

    //Now check files B and C. They should ideally be identical, except for maybe some floating-point loses.
  }

  if(true){
    auto img4 = ReadFromFITS<uint8_t,double>("colour_swatch.fits");
    img4.init_spatial( 1.0, 1.0, 1.0, vec3<double>(0.0,0.0,0.0), vec3<double>(0.0,0.0,0.0) );
    Plot_Pixels_RGB(img4,0,1,2);
  }

    return 0;
}




