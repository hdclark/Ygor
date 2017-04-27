//YgorImages.cc - Routines to help manage buffers of 2D data.
//
#include <iostream>
#include <memory>
#include <string.h>   //For memcpy.
#include <cmath>      //For std::round(...)
#include <algorithm>
#include <list>
#include <vector>
#include <functional>
#include <stdexcept>
#include <fstream>
#include <experimental/optional>
#include <experimental/any>
#include <type_traits>
#include <stdio.h>  //For popen.
#include <future>
#include <thread>

#ifndef YGOR_IMAGES_DISABLE_EIGEN
    #include <eigen3/Eigen/Dense>
#endif

#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorStats.h"    //For Stats::Mean().
//#include "YgorPlot.h"
#include "YgorString.h"   //For Is_String_An_X<>().
#include "YgorImages.h"


//#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
//    #define YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
//#endif

//class planar_image;

//----------------------------------------------------------------------------------------------------
//---------------------------------------- planar_image ----------------------------------------------
//----------------------------------------------------------------------------------------------------
//Constructor/Destructors.
template <class T, class R> planar_image<T,R>::planar_image(){
    rows = columns = channels = -1;
    pxl_dx = pxl_dy = pxl_dz = (R)(0);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template planar_image<uint8_t ,double>::planar_image(void);
    template planar_image<uint16_t,double>::planar_image(void);
    template planar_image<uint32_t,double>::planar_image(void);
    template planar_image<uint64_t,double>::planar_image(void);
    template planar_image<float   ,double>::planar_image(void);
#endif

template <class T, class R> planar_image<T,R>::planar_image(const planar_image<T,R> &in){
    (*this) = in; //Deep-copies the data.
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template planar_image<uint8_t ,double>::planar_image(const planar_image<uint8_t ,double> &in);
    template planar_image<uint16_t,double>::planar_image(const planar_image<uint16_t,double> &in);
    template planar_image<uint32_t,double>::planar_image(const planar_image<uint32_t,double> &in);
    template planar_image<uint64_t,double>::planar_image(const planar_image<uint64_t,double> &in);
    template planar_image<float   ,double>::planar_image(const planar_image<float   ,double> &in);
#endif


template <class T, class R> planar_image<T,R>::~planar_image(){ }
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template planar_image<uint8_t ,double>::~planar_image(void);
    template planar_image<uint16_t,double>::~planar_image(void);
    template planar_image<uint32_t,double>::~planar_image(void);
    template planar_image<uint64_t,double>::~planar_image(void);
    template planar_image<float   ,double>::~planar_image(void);
#endif


//Allocating space and initializing the purely-2D-image members.
template <class T, class R> void planar_image<T,R>::init_buffer(long int rows, long int cols, long int chnls){
    if((rows <= 0) || (cols <= 0) || (chnls <= 0)){
        FUNCERR("Requested to initialize an image with impossible dimensions");
    }
    this->rows     = rows;
    this->columns  = cols;
    this->channels = chnls;
    this->data.resize(rows*cols*chnls);
    return;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template void planar_image<uint8_t ,double>::init_buffer(long int rows, long int cols, long int chnls);
    template void planar_image<uint16_t,double>::init_buffer(long int rows, long int cols, long int chnls);
    template void planar_image<uint32_t,double>::init_buffer(long int rows, long int cols, long int chnls);
    template void planar_image<uint64_t,double>::init_buffer(long int rows, long int cols, long int chnls);
    template void planar_image<float   ,double>::init_buffer(long int rows, long int cols, long int chnls);
#endif


//Initializing the R^3 members. These are less important because they won't cause a segfault.
template <class T, class R> void planar_image<T,R>::init_spatial(R pxldx, R pxldy, R pxldz, const vec3<R> &anchr, const vec3<R> &offst){
    this->pxl_dx = pxldx;
    this->pxl_dy = pxldy;
    this->pxl_dz = pxldz;
    this->anchor = anchr;
    this->offset = offst;
    return;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template void planar_image<uint8_t ,double>::init_spatial(double pxldx, double pxldy, double pxldz, const vec3<double> &anchr, const vec3<double> &offst);
    template void planar_image<uint16_t,double>::init_spatial(double pxldx, double pxldy, double pxldz, const vec3<double> &anchr, const vec3<double> &offst);
    template void planar_image<uint32_t,double>::init_spatial(double pxldx, double pxldy, double pxldz, const vec3<double> &anchr, const vec3<double> &offst);
    template void planar_image<uint64_t,double>::init_spatial(double pxldx, double pxldy, double pxldz, const vec3<double> &anchr, const vec3<double> &offst);
    template void planar_image<float   ,double>::init_spatial(double pxldx, double pxldy, double pxldz, const vec3<double> &anchr, const vec3<double> &offst);
#endif

template <class T, class R> void planar_image<T,R>::init_orientation(const vec3<R> &rowunit, const vec3<R> &colunit){
    this->row_unit = rowunit.unit();
    this->col_unit = colunit.unit();
    return;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template void planar_image<uint8_t ,double>::init_orientation(const vec3<double> &rowunit, const vec3<double> &colunit);
    template void planar_image<uint16_t,double>::init_orientation(const vec3<double> &rowunit, const vec3<double> &colunit);
    template void planar_image<uint32_t,double>::init_orientation(const vec3<double> &rowunit, const vec3<double> &colunit);
    template void planar_image<uint64_t,double>::init_orientation(const vec3<double> &rowunit, const vec3<double> &colunit);
    template void planar_image<float   ,double>::init_orientation(const vec3<double> &rowunit, const vec3<double> &colunit);
#endif


template <class T,class R> planar_image<T,R> & planar_image<T,R>::operator=(const planar_image<T,R> &rhs){
    if(this == &rhs) return *this;
    //Deep-copy: copies everything except the array's pointer, for which a new buffer is allocated and raw pixel data copied.
    this->init_buffer(rhs.rows, rhs.columns, rhs.channels);
    this->init_spatial(rhs.pxl_dx, rhs.pxl_dy, rhs.pxl_dz, rhs.anchor, rhs.offset);
    this->init_orientation(rhs.row_unit, rhs.col_unit);
    this->metadata = rhs.metadata;
    this->data = rhs.data;
    return *this;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template planar_image<uint8_t ,double> & planar_image<uint8_t ,double>::operator=(const planar_image<uint8_t ,double> &rhs);
    template planar_image<uint16_t,double> & planar_image<uint16_t,double>::operator=(const planar_image<uint16_t,double> &rhs);
    template planar_image<uint32_t,double> & planar_image<uint32_t,double>::operator=(const planar_image<uint32_t,double> &rhs);
    template planar_image<uint64_t,double> & planar_image<uint64_t,double>::operator=(const planar_image<uint64_t,double> &rhs);
    template planar_image<float   ,double> & planar_image<float   ,double>::operator=(const planar_image<float   ,double> &rhs);
#endif

template <class T,class R> bool planar_image<T,R>::operator==(const planar_image<T,R> &rhs) const {
    if(this == &rhs) return true;
    if((*this < rhs) || (rhs < *this)) return false;
    return true;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::operator==(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::operator==(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::operator==(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::operator==(const planar_image<uint64_t,double> &rhs) const;
    template bool planar_image<float   ,double>::operator==(const planar_image<float   ,double> &rhs) const;
#endif

template <class T,class R> bool planar_image<T,R>::operator!=(const planar_image<T,R> &rhs) const {
    return !(*this == rhs);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::operator!=(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::operator!=(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::operator!=(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::operator!=(const planar_image<uint64_t,double> &rhs) const;
    template bool planar_image<float   ,double>::operator!=(const planar_image<float   ,double> &rhs) const;
#endif

template <class T,class R> bool planar_image<T,R>::operator<(const planar_image<T,R> &rhs) const {
    if(this == &rhs) return false; //Equal!
    if(this->rows != rhs.rows) return (this->rows < rhs.rows);
    if(this->columns != rhs.columns) return (this->columns < rhs.columns);
    if(this->channels != rhs.channels) return (this->channels < rhs.channels);

    if(this->pxl_dx != rhs.pxl_dx) return (this->pxl_dx < rhs.pxl_dx);
    if(this->pxl_dy != rhs.pxl_dy) return (this->pxl_dy < rhs.pxl_dy);
    if(this->pxl_dz != rhs.pxl_dz) return (this->pxl_dz < rhs.pxl_dz);

    if((this->anchor + this->offset) != (rhs.anchor + rhs.offset)) return ((this->anchor + this->offset) < (rhs.anchor + rhs.offset));

    if(this->row_unit != rhs.row_unit) return (this->row_unit < rhs.row_unit);
    if(this->col_unit != rhs.col_unit) return (this->col_unit < rhs.col_unit);

    //Potentially costly metadata check.
    if(this->metadata != rhs.metadata) return (this->metadata < rhs.metadata);

    //Potentially (very) costly per-pixel check.
    return (this->data < rhs.data);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::operator<(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::operator<(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::operator<(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::operator<(const planar_image<uint64_t,double> &rhs) const;
    template bool planar_image<float   ,double>::operator<(const planar_image<float   ,double> &rhs) const;
#endif

template <class T,class R> bool planar_image<T,R>::Spatially_eq(const planar_image<T,R> &rhs) const {
    if(this == &rhs) return true;
 
    //Defined explicitly.
    //
    //NOTE: Returns false if the images do *not* have similar geometrical properties. There might be some 
    // ambiguity in what is meant by "similar", but it is necessary since we are dealing with floating-point
    // values.
    //return (   (this->rows     == rhs.rows)
    //        || (this->columns  == rhs.columns)
    //        || (this->channels == rhs.channels)
    //        || (this->pxl_dx   == rhs.pxl_dx)
    //        || (this->pxl_dy   == rhs.pxl_dy)
    //        || (this->pxl_dz   == rhs.pxl_dz)
    //        || ((this->anchor + this->offset) == (rhs.anchor + rhs.offset))
    //        || (this->row_unit == rhs.row_unit)
    //        || (this->col_unit == rhs.col_unit)    );

    //Defined in terms of Spatially_lt().
    return (!this->Spatially_lt(rhs) && !rhs.Spatially_lt(*this));
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::Spatially_eq(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::Spatially_eq(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::Spatially_eq(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::Spatially_eq(const planar_image<uint64_t,double> &rhs) const;
    template bool planar_image<float   ,double>::Spatially_eq(const planar_image<float   ,double> &rhs) const;
#endif

template <class T,class R> bool planar_image<T,R>::Spatially_lt(const planar_image<T,R> &rhs) const {
    if(this == &rhs) return false;

    //We are stuck with choosing a preferred ordering and treating all comparisons the same way.
    // It might be convenient to adapt the comparison to make more logical sense (for example when
    // images share a common orientation with a normal not along z-axis), but this breaks the 
    // requirement of associativity: if (a<b) and (b<c) then we require (a<c). Including a special
    // logic which might compare b and c differently than a and b could ruin associativity!
    //
    // So we are stuck sorting spatially on a logically identical part of the image (e.g., the 
    // row=0, column=0 pixel position) using a preferred ordering of z-axis, y-axis, and then 
    // x-axis separately.
    //
    // To tie-break, we look at other characteristics of the image. Though the ordering might not
    // be how you think it ought to be, it was chosen to:
    //  1. Be discriminative enough to catch minor differences between images, and
    //  2. Be associative.
    //
    const auto pos_l = (this->anchor + this->offset);
    const auto pos_r = (rhs.anchor + rhs.offset);
    if(pos_l.z       != pos_r.z)     return (pos_l.z       < pos_r.z);
    if(pos_l.y       != pos_r.y)     return (pos_l.y       < pos_r.y);
    if(pos_l.x       != pos_r.x)     return (pos_l.x       < pos_r.x);

    //Tie break on spatial extent.
    const auto row_extent_l = (this->rows*this->pxl_dx);
    const auto row_extent_r = (rhs.rows*rhs.pxl_dx);
    if(row_extent_l != row_extent_r) return (row_extent_l < row_extent_r);

    const auto col_extent_l = (this->columns*this->pxl_dy);
    const auto col_extent_r = (rhs.columns*rhs.pxl_dy);
    if(col_extent_l != col_extent_r) return (col_extent_l < col_extent_r);

    if(this->pxl_dz  != rhs.pxl_dz)  return (this->pxl_dz  < rhs.pxl_dz);

    //Tie break using the default vec3 operator< on the unit vectors.
    if(this->row_unit != rhs.row_unit) return (this->row_unit < rhs.row_unit);
    return (this->col_unit < rhs.col_unit);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::Spatially_lt(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::Spatially_lt(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::Spatially_lt(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::Spatially_lt(const planar_image<uint64_t,double> &rhs) const;
    template bool planar_image<float   ,double>::Spatially_lt(const planar_image<float   ,double> &rhs) const;
#endif

template <class T,class R> bool planar_image<T,R>::Spatially_lte(const planar_image<T,R> &rhs) const {
    //Without using Spatially_eq()..
    if(this == &rhs) return true;
    if(this->Spatially_lt(rhs)) return true;
    return (this->Spatially_lt(rhs)) == (rhs.Spatially_lt(*this));

    //With using Spatially_eq().
    //return this->Spatially_eq(rhs) || (this->Spatially_lt(rhs);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::Spatially_lte(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::Spatially_lte(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::Spatially_lte(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::Spatially_lte(const planar_image<uint64_t,double> &rhs) const;
    template bool planar_image<float   ,double>::Spatially_lte(const planar_image<float   ,double> &rhs) const;
#endif


//Zero-based indexing. 
template <class T, class R> long int planar_image<T,R>::index(long int row, long int col) const {
    if( !isininc(0,row,this->rows-1) 
    ||  !isininc(0,col,this->columns-1) ){
        return -1;
    }
    return this->channels*( this->columns * row + col );
} 
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template long int planar_image<uint8_t ,double>::index(long int r, long int c) const;
    template long int planar_image<uint16_t,double>::index(long int r, long int c) const;
    template long int planar_image<uint32_t,double>::index(long int r, long int c) const;
    template long int planar_image<uint64_t,double>::index(long int r, long int c) const;
    template long int planar_image<float   ,double>::index(long int r, long int c) const;
#endif

template <class T, class R> long int planar_image<T,R>::index(long int row, long int col, long int chnl) const {
    if( !isininc(0,row,this->rows-1) 
    ||  !isininc(0,col,this->columns-1)
    ||  !isininc(0,chnl,this->channels-1)){
        return -1;
    }
    return this->channels*( this->columns * row + col ) + chnl;
} 
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template long int planar_image<uint8_t ,double>::index(long int row, long int col, long int chnl) const;
    template long int planar_image<uint16_t,double>::index(long int row, long int col, long int chnl) const;
    template long int planar_image<uint32_t,double>::index(long int row, long int col, long int chnl) const;
    template long int planar_image<uint64_t,double>::index(long int row, long int col, long int chnl) const;
    template long int planar_image<float   ,double>::index(long int row, long int col, long int chnl) const;
#endif

template <class T, class R> long int planar_image<T,R>::index(const vec3<R> &point, long int chnl) const {
    const vec3<R> P(point - this->anchor - this->offset); // Get a vector in the image's plane.
    const auto Nr = this->row_unit.Dot(P)/this->pxl_dx;   // Appriximate row number.
    const auto Nc = this->col_unit.Dot(P)/this->pxl_dy;   // Approximate col number.
    const auto Uz = this->row_unit.Cross(this->col_unit).unit(); // Orthogonal unit vector.

    //Check if it is too far out of the plane of the 2D image. Be inclusive in case the image thickness is 0.
    const auto Nz = Uz.Dot(P)/( static_cast<R>(0.5)*this->pxl_dz );
    if(!isininc((R)(-1.0),Nz,(R)(1.0))) return -1;

    //Now, Nr and Nc should (ideally) be integers. They will be very close in value to ints, too, except
    // for floating point errors which may have blurred them slightly above or below the actual value.
    // Because we do not expect the blur to be significant, we will just round them to the nearest int.
    // If the blur is more than this, we have larger issues to deal with! 
    const auto row = static_cast<long int>( std::round(Nr) ); 
    const auto col = static_cast<long int>( std::round(Nc) );

    if( !isininc(0,row,this->rows-1) 
    ||  !isininc(0,col,this->columns-1) 
    ||  !isininc(0,chnl,this->channels-1) ){
        return -1;
    }
    return this->index(row,col,chnl);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template long int planar_image<uint8_t ,double>::index(const vec3<double> &point, long int chnl) const;
    template long int planar_image<uint16_t,double>::index(const vec3<double> &point, long int chnl) const;
    template long int planar_image<uint32_t,double>::index(const vec3<double> &point, long int chnl) const;
    template long int planar_image<uint64_t,double>::index(const vec3<double> &point, long int chnl) const;
    template long int planar_image<float   ,double>::index(const vec3<double> &point, long int chnl) const;
#endif

//Work backward from the index to get row, column, or channel.
template <class T, class R> std::tuple<long int,long int,long int> planar_image<T,R>::row_column_channel_from_index(long int userindex) const {
    const auto fail_res = std::make_tuple<long int,long int,long int>(-1,-1,-1);
    if(userindex < 0) return fail_res;
    if(userindex > this->index(this->rows-1,this->columns-1,this->channels-1)) return fail_res;

    //We just 'peel off' the factors until we get what is needed. 
    //
    // NOTE: Assumes the index is (still) computed like this->channels*(this->columns*row+col)+chnl.
    const ldiv_t y = ::ldiv(userindex,this->channels);
    const auto chnl = y.rem;
    const ldiv_t z = ::ldiv(y.quot,this->columns);
    const auto col = z.rem;
    const auto row = z.quot;
    if(this->index(row,col,chnl) != userindex) FUNCERR("Programming error. Has the index scheme been changed?");
    return std::make_tuple(row,col,chnl);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template std::tuple<long int,long int,long int> planar_image<uint8_t ,double>::row_column_channel_from_index(long int index) const;
    template std::tuple<long int,long int,long int> planar_image<uint16_t,double>::row_column_channel_from_index(long int index) const;
    template std::tuple<long int,long int,long int> planar_image<uint32_t,double>::row_column_channel_from_index(long int index) const;
    template std::tuple<long int,long int,long int> planar_image<uint64_t,double>::row_column_channel_from_index(long int index) const;
    template std::tuple<long int,long int,long int> planar_image<float   ,double>::row_column_channel_from_index(long int index) const;
#endif

//Compute fractional row and column numbers when a point in R^3 is known. Throws if out of bounds. This routine is
// useful for converting between pixel space and spatial dimension, especially for routines that are easier to implement
// in one or the other spaces (e.g., interpolation).
//
// Note: returns fractional (row,column) values.
//
template <class T, class R> std::pair<R, R> planar_image<T,R>::fractional_row_column(const vec3<R> &point) const {
    const vec3<R> P(point - this->anchor - this->offset); // Transform the vector to share coord system with image.
    const auto Nr = this->row_unit.Dot(P)/this->pxl_dx;   // Approximate row number.
    const auto Nc = this->col_unit.Dot(P)/this->pxl_dy;   // Approximate col number.
    const auto Uz = this->row_unit.Cross(this->col_unit).unit(); // Orthogonal unit vector.

    //Check if it is too far out of the plane of the 2D image. Be inclusive in case the image thickness is 0.
    const auto Nz = Uz.Dot(P)/( static_cast<R>(0.5)*this->pxl_dz );
    if(!isininc((R)(-1.0),Nz,(R)(1.0))){
         throw std::invalid_argument("Point was out-of-bounds (outside the image thickness).");
    }

    //Check if the (integer) coordinates are within the bounds of the image.
    const auto row = static_cast<long int>( std::round(Nr) ); 
    const auto col = static_cast<long int>( std::round(Nc) );
    if( !isininc(0,row,this->rows-1) 
    ||  !isininc(0,col,this->columns-1) ){
        throw std::invalid_argument("Point was out-of-bounds (within the image thickness, but outside row/col bounds.");
    }
    return std::make_pair(Nr,Nc);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template std::pair<double,double> planar_image<uint8_t ,double>::fractional_row_column(const vec3<double> &) const;
    template std::pair<double,double> planar_image<uint16_t,double>::fractional_row_column(const vec3<double> &) const;
    template std::pair<double,double> planar_image<uint32_t,double>::fractional_row_column(const vec3<double> &) const;
    template std::pair<double,double> planar_image<uint64_t,double>::fractional_row_column(const vec3<double> &) const;
    template std::pair<double,double> planar_image<float   ,double>::fractional_row_column(const vec3<double> &) const;
#endif

//Channel-value getters.
template <class T, class R> T planar_image<T,R>::value(long int row, long int col, long int chnl) const {
    if( !isininc(0,row,this->rows-1) 
    ||  !isininc(0,col,this->columns-1) 
    ||  !isininc(0,chnl,this->channels) ){
        throw std::domain_error("Attempted to access part of image which does not exist");
    }
    return this->data[this->index(row,col,chnl)];
} 
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template uint8_t  planar_image<uint8_t ,double>::value(long int row, long int col, long int chnl) const;
    template uint16_t planar_image<uint16_t,double>::value(long int row, long int col, long int chnl) const;
    template uint32_t planar_image<uint32_t,double>::value(long int row, long int col, long int chnl) const;
    template uint64_t planar_image<uint64_t,double>::value(long int row, long int col, long int chnl) const;
    template float    planar_image<float   ,double>::value(long int row, long int col, long int chnl) const;
#endif

//Returns the value of the voxel which contains the point. If the voxel does not exist an exception is thrown.
//
//NOTE: This function is very slow and not always very safe! Use it sparingly!
template <class T, class R> T planar_image<T,R>::value(const vec3<R> &point, long int chnl) const {
    const auto indx = this->index(point,chnl); 
    if(indx == -1){
        throw std::domain_error("Attempted to access part of image which does not exist");
    }
    return this->data[indx];
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template uint8_t  planar_image<uint8_t ,double>::value(const vec3<double> &point, long int chnl) const;
    template uint16_t planar_image<uint16_t,double>::value(const vec3<double> &point, long int chnl) const;
    template uint32_t planar_image<uint32_t,double>::value(const vec3<double> &point, long int chnl) const;
    template uint64_t planar_image<uint64_t,double>::value(const vec3<double> &point, long int chnl) const;
    template float    planar_image<float   ,double>::value(const vec3<double> &point, long int chnl) const;
#endif

//Returns the value of the voxel which contains the point. If the voxel does not exist an exception is thrown.
//
//NOTE: This function is very slow and not always very safe! Use it sparingly!
template <class T, class R> T planar_image<T,R>::value(long int userindex) const {
    if( (userindex < 0)
    ||  (userindex > this->index(this->rows-1,this->columns-1,this->channels-1)) ){
        throw std::domain_error("Attempted to access part of image which does not exist");
    }
    return this->data[userindex];
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template uint8_t  planar_image<uint8_t ,double>::value(long int indx) const;
    template uint16_t planar_image<uint16_t,double>::value(long int indx) const;
    template uint32_t planar_image<uint32_t,double>::value(long int indx) const;
    template uint64_t planar_image<uint64_t,double>::value(long int indx) const;
    template float    planar_image<float   ,double>::value(long int indx) const;
#endif



//Channel-value references. This can be used to set the values.
template <class T,class R> T& planar_image<T,R>::reference(long int row, long int col, long int chnl){
    if( !isininc(0,row,this->rows-1) 
    ||  !isininc(0,col,this->columns-1)
    ||  !isininc(0,chnl,this->channels-1) ){
        throw std::domain_error("Attempted to access part of image which does not exist");
    }
    return this->data[this->index(row,col,chnl)];
} 
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template uint8_t  & planar_image<uint8_t ,double>::reference(long int row, long int col, long int chnl);
    template uint16_t & planar_image<uint16_t,double>::reference(long int row, long int col, long int chnl);
    template uint32_t & planar_image<uint32_t,double>::reference(long int row, long int col, long int chnl);
    template uint64_t & planar_image<uint64_t,double>::reference(long int row, long int col, long int chnl);
    template float    & planar_image<float   ,double>::reference(long int row, long int col, long int chnl);
#endif

template <class T,class R> T& planar_image<T,R>::reference(const vec3<R> &point, long int chnl){
    const auto indx = this->index(point,chnl);
    if(indx == -1){
        throw std::domain_error("Attempted to access part of image which does not exist");
    }
    return this->data[indx];
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template uint8_t  & planar_image<uint8_t ,double>::reference(const vec3<double> &point, long int chnl);
    template uint16_t & planar_image<uint16_t,double>::reference(const vec3<double> &point, long int chnl);
    template uint32_t & planar_image<uint32_t,double>::reference(const vec3<double> &point, long int chnl);
    template uint64_t & planar_image<uint64_t,double>::reference(const vec3<double> &point, long int chnl);
    template float    & planar_image<float   ,double>::reference(const vec3<double> &point, long int chnl);
#endif

template <class T,class R> T& planar_image<T,R>::reference(long int userindex){
    if( (userindex < 0)
    ||  (userindex > this->index(this->rows-1,this->columns-1,this->channels-1)) ){
        throw std::domain_error("Attempted to access part of image which does not exist");
    }
    return this->data[userindex];
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template uint8_t  & planar_image<uint8_t ,double>::reference(long int indx);
    template uint16_t & planar_image<uint16_t,double>::reference(long int indx);
    template uint32_t & planar_image<uint32_t,double>::reference(long int indx);
    template uint64_t & planar_image<uint64_t,double>::reference(long int indx);
    template float    & planar_image<float   ,double>::reference(long int indx);
#endif



//Interpolate within the plane of the image, in pixel number coordinates (e.g, permitting fractional pixel row and numbers).
// This routine can be tricky for the user so look at the diagram and see the notes below. 
//
// Diagram showing what's happening and how the input should be provided:
//   - Notation: Points are like: (row,col).
//   - Pixels are not square to emphasize that we discard pixel shape information (i.e., pxl_dx, pxl_dy, pxl_dz).
//
// (-1/2,-1/2)                                        (cols-1/2,-1/2)
//      \                                                 /
//       \_______________________________________________/
//(-1/2, |       |       |       |       |       |       |
//    0) |       |       |       |       |       |       |
//     \ |       |       |       |       |       | (0,   |
//      \| (0,0) | (0,1) | (0,2) | (0,3) |       |cols-1)|
//       |   x   |   x   |   x   |   x   |   x   |   x   |
//(-1/2, |       |       |       |       |       |       |
//  1/2) |       |       |       |       |       |       |
//    \  |       |       |       |       |       |       |
//     \ |       |       |       |       |       |       |
//      \|_______|_______|_______|_______|_______|_______|
//       |       |       |       |       |       |       |
//       |       |       |       |       |       |       |
//       |       |       |       |       |       |       |
//       | (1,0) |       |       |       |       |       |
//(-1/2, |   x   |   x---|---x   |   x   |   x   |   x   |
//  3/2) |       |       | +     |       |       |       |
//   \   |       |       | +     |       |       |       |
//    \  |       |       | 0 <----Interp.|       |       |
//     \ |       |       | +     | Point |       |       |
//      \|_______|_______|_+_____|_______|_______|_______|
//       |       |       | +     |       |       |       |
//       |       |       | +     |       |       |      _|__ (rows-1, cols-1)
//       |(rows  |       | +     |       |       |     / |
//       |  -1,0)|       | +     |       |       |    /  |
//       |   x   |   x---|---x   |   x   |   x   |   x   |
//(-1/2, |       |       |       |       |       |       |
// rows- |       |       |       |       |       |       | (rows-1/2, cols-1/2)
//  1/2) |       |       |       |       |       |       |  /
//     \ |       |       |       |       |       |       | /
//      \|_______|_______|_______|_______|_______|_______|/
//
//
// NOTE: If the image has N rows (columns) then the input can range from [-1/2, N-1/2]. This is done because the pixel's 
//       value is treated as residing at the CENTRE of the pixel's spatial extent. So when you want to interpolate at the
//       point (0.0, 0.0) you should get the exact first pixel value. (If the input ranged from [0, N] then you would have
//       to specify (0.5, 0.5) to get the exact first pixel. That isn't very intuitive!)
//
//       For example, if you are scaling the image dimensions 2x (so the edge length is twice what is was before), and you
//       are populating a new image with interpolated values, then your loop will look like:
//
//                   for(int row = 0; row < new_rows; ++row){
//                       for(int col = 0; col < new_cols; ++col){
//                           auto interpolate_old_row_at = ((row*1.0 + 0.5)/2) - 0.5;
//                           ...
//                       }
//                   }
//
//       If instead of scaling by a factor of 2 you scale by a factor of N (a whole number), you will need to use:
//       (2*row - N + 1)/(2N). If you are reducing by a factor of 2 you can just use (row/2). (Using this routine to reduce
//       images is not recommended; pixels are discarded instead of properly condensed!)
//
// NOTE: This routine IGNORES pixel dimensions. Specifically, it treats them as equal. A separate interpolation scheme will
//       be needed to take this info into account.
//
// NOTE: This routine operates entirely in the 2D plane of the image, with no regard for the pxl_dz 'thickness'.
//
// NOTE: This routine uses MIRROR boundary conditions, meaning a virtual border of the edge points is placed around the
//       image such that they equal the nearest real pixel value. This is done to avoid handling overflow (as could happen
//       if extrapolation was used) and abrupt or sharp edges (as would happen if toroidal b.c.'s were used). 
//
// NOTE: This routine interpolates nearest neighbour (i.e., directly adjacent and touching) and the single closest diagonal
//       pixel's values.
//
// NOTE: This routine effectively produces line segments that interpolate along the row- and column-axes. It does not 
//       produce a smooth output, and the output will not be suitable for computing derivatives (unless you remain in the
//       purely linear region between pixel centres). If you need this, consider spline interpolation or something else.
//
// NOTE: This routine could be combined with a finite-difference scheme to get bilinearly-interpolated derivatives. You
//       would need to first replace pixels with the pixel value's (Nth) derivative, and then you can run this interpolation
//       scheme to interpolate (Nth) derivatives smoothly. The (N+1)th derivative will, however, not be smooth!
//
template <class T,class R> T planar_image<T,R>::bilinearly_interpolate_in_pixel_number_space(R row, R col, long int chnl) const { 
    //Get pixel space coordinates for the pixel that the user's input specifies.
    const auto r_e_p = static_cast<long int>(std::floor(row + static_cast<R>(0.5))); //"Enclosing pixel row"
    const auto c_e_p = static_cast<long int>(std::floor(col + static_cast<R>(0.5))); //"Enclosing pixel column"

    //Note: fails on out-of-bounds input.
    if(!isininc(0,r_e_p,this->rows-1)
    || !isininc(0,c_e_p,this->columns-1)
    || !isininc(0,chnl,this->channels-1)){
        FUNCERR("Attempted to access part of image which does not exist");
    }

    //Figure out which quandrant of the enclosing cell the point is. It changes which adjacent cells we will use to interpolate.
    const auto r_adj_is_plus = (row > r_e_p); //Adjacent cell should be taken to be r_e_p + 1. Else r_e_p - 1.
    const auto c_adj_is_plus = (col > c_e_p); //Adjacent cell should be taken to be c_e_p + 1. Else c_e_p - 1.

    //Figure out the 'virtual' minimum and maximum row and column numbers of the four nearest (surrounding) pixel centres.
    const auto r_min_virt = r_e_p + (r_adj_is_plus ? 0 : -1);
    const auto r_max_virt = r_e_p + (r_adj_is_plus ? 1 :  0);
    const auto c_min_virt = c_e_p + (c_adj_is_plus ? 0 : -1);
    const auto c_max_virt = c_e_p + (c_adj_is_plus ? 1 :  0);

    //Figure out the 'real' minimum and maximum row and column numbers of the four nearest (surrounding) pixel centres.
    // (This is where our mirror boundary conditions first come into play.
    const auto r_min = std::max(static_cast<long int>(0),r_min_virt);
    const auto r_max = std::min(this->rows-1, r_max_virt);
    const auto c_min = std::max(static_cast<long int>(0),c_min_virt);
    const auto c_max = std::min(this->columns-1, c_max_virt);

    //Get the fractional [0,1) indicator of row/col position between the min and max pixel coordinates.
    const auto drow = (row - static_cast<double>(r_min_virt)); // pxl_dx <-- if pixel shape were considered.
    const auto dcol = (col - static_cast<double>(c_min_virt)); // pxl_dy <-- if pixel shape were considered.

    //Get the pixel values at each of the four pixel centres.
    const auto y_r_min_c_min = static_cast<double>(this->data[this->index(r_min,c_min,chnl)]);
    const auto y_r_min_c_max = static_cast<double>(this->data[this->index(r_min,c_max,chnl)]);
    const auto y_r_max_c_min = static_cast<double>(this->data[this->index(r_max,c_min,chnl)]);
    const auto y_r_max_c_max = static_cast<double>(this->data[this->index(r_max,c_max,chnl)]);

    //Interpolate along the row axis between the min and max elements.
    const auto y_r_interp_c_min = y_r_min_c_min + (y_r_max_c_min - y_r_min_c_min) * drow;
    const auto y_r_interp_c_max = y_r_min_c_max + (y_r_max_c_max - y_r_min_c_max) * drow;

    //Interpolate along the column axis between the row axis interpolated values.
    const auto y_r_interp_c_interp = y_r_interp_c_min + (y_r_interp_c_max - y_r_interp_c_min) * dcol;
    return static_cast<T>( y_r_interp_c_interp );
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template uint8_t  planar_image<uint8_t ,double>::bilinearly_interpolate_in_pixel_number_space(double row, double col, long int chnl) const;
    template uint16_t planar_image<uint16_t,double>::bilinearly_interpolate_in_pixel_number_space(double row, double col, long int chnl) const;
    template uint32_t planar_image<uint32_t,double>::bilinearly_interpolate_in_pixel_number_space(double row, double col, long int chnl) const;
    template uint64_t planar_image<uint64_t,double>::bilinearly_interpolate_in_pixel_number_space(double row, double col, long int chnl) const;
    template float    planar_image<float   ,double>::bilinearly_interpolate_in_pixel_number_space(double row, double col, long int chnl) const;
#endif


//Compute centered finite-difference approximations of derivatives (in pixel coordinate space) along the row and column axes.
// First derivatives. Only nearest neighbour pixels are used, and mirror boundary conditions are assumed. Pixel shape is ignored.
// The following routines fail with out-of-bounds input.
//
// NOTE: The return type was chosen to be R (the user-specified 'real' type) because it is assumed to be float, double, or something that
//       identifies a 'suitable level of precision' for the user. Though the spatial shape of the pixels is not taken into account, and so
//       type T might seem more suitable, we do not know if type T can hold negatives (which are certain to show up in most cases).
//       
//       An alternative would be to template this routine separately, but it would still likely not fix the problem because derivatives 
//       necessarily combine value (type T) with location (type R). In certain cases you might wish to have this routine spit out 
//       (long double) or even arbitrary precision types in special cases. These are special cases, though.
//
// NOTE: Coordinate system: The positive row-axis (column-axis) direction is in the direction of increasing row (column).
//
template <class T,class R> R planar_image<T,R>::row_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const {
    // NOTE: This routine computes $\partial_{r} P(row,col)$.
    if(!isininc(0,row,this->rows-1)
    || !isininc(0,col,this->columns-1)
    || !isininc(0,chnl,this->channels-1)){
        FUNCERR("Attempted to access part of image which does not exist");
    }
    long int row_m_1 = std::max(static_cast<long int>(0),row-1);
    long int row_p_1 = std::max(static_cast<long int>(0),row+1);

    return (  static_cast<R>(this->data[this->index(row_p_1,col,chnl)])
            - static_cast<R>(this->data[this->index(row_m_1,col,chnl)]) )
           / static_cast<R>(2); // <--- All NN pixels are separated by 1.0 in pixel coords!
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template double planar_image<uint8_t ,double>::row_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint16_t,double>::row_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint32_t,double>::row_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint64_t,double>::row_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<float   ,double>::row_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
#endif
template <class T,class R> R planar_image<T,R>::column_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const {
    // NOTE: This routine computes $\partial_{c} P(row,col)$.
    if(!isininc(0,row,this->rows-1)
    || !isininc(0,col,this->columns-1)
    || !isininc(0,chnl,this->channels-1)){
        FUNCERR("Attempted to access part of image which does not exist");
    }
    long int col_m_1 = std::max(static_cast<long int>(0),col-1);
    long int col_p_1 = std::max(static_cast<long int>(0),col+1);
    
    return (  static_cast<R>(this->data[this->index(row,col_p_1,chnl)])
            - static_cast<R>(this->data[this->index(row,col_m_1,chnl)]) )
           / static_cast<R>(2); // <--- All NN pixels are separated by 1.0 in pixel coords!
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template double planar_image<uint8_t ,double>::column_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint16_t,double>::column_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint32_t,double>::column_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint64_t,double>::column_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<float   ,double>::column_aligned_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
#endif
//Compute centered finite-difference approximations of derivatives (in pixel coordinate space) along the row and column axes.
// Second derivatives. Only nearest neighbour pixels are used, and mirror boundary conditions are assumed. Pixel shape is ignored.
// The following routines fail with out-of-bounds input.
//
// NOTE: The return type was chosen to be R (the user-specified 'real' type) because it is assumed to be float, double, or something that
//       identifies a 'suitable level of precision' for the user. Though the spatial shape of the pixels is not taken into account, and so
//       type T might seem more suitable, we do not know if type T can hold negatives (which are certain to show up in most cases).
//       
//       An alternative would be to template this routine separately, but it would still likely not fix the problem because derivatives 
//       necessarily combine value (type T) with location (type R). In certain cases you might wish to have this routine spit out 
//       (long double) or even arbitrary precision types in special cases. These are special cases, though.
//
// NOTE: Coordinate system: The positive row-axis (column-axis) direction is in the direction of increasing row (column).
//
template <class T,class R> R planar_image<T,R>::row_aligned_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const {
    // NOTE: This routine computes $\partial_{c}^{2} P(row,col)$.
    if(!isininc(0,row,this->rows-1)
    || !isininc(0,col,this->columns-1)
    || !isininc(0,chnl,this->channels-1)){
        FUNCERR("Attempted to access part of image which does not exist");
    }
    long int row_m_1 = std::max(static_cast<long int>(0),row-1);
    long int row_p_1 = std::max(static_cast<long int>(0),row+1);

    return (  this->row_aligned_derivative_centered_finite_difference(row_p_1, col, chnl)
            - this->row_aligned_derivative_centered_finite_difference(row_m_1, col, chnl) )
           / static_cast<R>(2); // <--- All NN pixels are separated by 1.0 in pixel coords!
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template double planar_image<uint8_t ,double>::row_aligned_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint16_t,double>::row_aligned_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint32_t,double>::row_aligned_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint64_t,double>::row_aligned_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<float   ,double>::row_aligned_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
#endif
template <class T,class R> R planar_image<T,R>::column_aligned_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const {
    // NOTE: This routine computes $\partial_{r}^{2} P(row,col)$.
    if(!isininc(0,row,this->rows-1)
    || !isininc(0,col,this->columns-1)
    || !isininc(0,chnl,this->channels-1)){
        FUNCERR("Attempted to access part of image which does not exist");
    }
    long int col_m_1 = std::max(static_cast<long int>(0),col-1);
    long int col_p_1 = std::max(static_cast<long int>(0),col+1);

    return (  this->column_aligned_derivative_centered_finite_difference(row, col_p_1, chnl)
            - this->column_aligned_derivative_centered_finite_difference(row, col_m_1, chnl) )
           / static_cast<R>(2); // <--- All NN pixels are separated by 1.0 in pixel coords!
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template double planar_image<uint8_t ,double>::column_aligned_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint16_t,double>::column_aligned_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint32_t,double>::column_aligned_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint64_t,double>::column_aligned_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<float   ,double>::column_aligned_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
#endif
template <class T,class R> R planar_image<T,R>::cross_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const {
    // NOTE: This routine computes $\partial_{r,c} P(row,col)$.
    if(!isininc(0,row,this->rows-1)
    || !isininc(0,col,this->columns-1)
    || !isininc(0,chnl,this->channels-1)){
        FUNCERR("Attempted to access part of image which does not exist");
    }
    long int row_m_1 = std::max(static_cast<long int>(0),row-1);
    long int row_p_1 = std::max(static_cast<long int>(0),row+1);
    long int col_m_1 = std::max(static_cast<long int>(0),col-1);
    long int col_p_1 = std::max(static_cast<long int>(0),col+1);

    return (  static_cast<R>(this->data[this->index(row_p_1,col_p_1,chnl)])
            - static_cast<R>(this->data[this->index(row_m_1,col_p_1,chnl)])
            - static_cast<R>(this->data[this->index(row_p_1,col_m_1,chnl)])
            + static_cast<R>(this->data[this->index(row_m_1,col_m_1,chnl)]) )
           / ( static_cast<R>(2) * static_cast<R>(2) ); // <--- All NN pixels are separated by 1.0 in pixel coords!
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template double planar_image<uint8_t ,double>::cross_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint16_t,double>::cross_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint32_t,double>::cross_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<uint64_t,double>::cross_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
    template double planar_image<float   ,double>::cross_second_derivative_centered_finite_difference(long int row, long int col, long int chnl) const;
#endif


//Interpolate within the plane of the image, in pixel number coordinates (e.g, permitting fractional pixel row and numbers).
//
// This routine produces smoother interpolation than bilinear interpolation, but the smoothness is somewhat arbitrarily defined
// in terms of the finite-difference derivative of the original data. (See below.) If the true derivatives are not known, and
// you need them, and you are going to approximate them using finite differences anyways, then this routine is probably a good 
// match for you.
//
// If you are going to use this to later compute derivatives, it would be best to use a dedicated routine. Derivatives
// computed using the output from this routine will only correspond to the implementation. It would probably be better to 
// explicitly compute finite differences and then use bilinear interpolation on the finite differences. OTOH, bicubic
// interpolation *can* directly produce derivatives. You may want to modify this routine to do so.
//
// Diagram showing what's happening and how the input should be provided:
//   - Notation: Points are like: (row,col). Pixel centres marked 'U' or 'u' are used, those marked 'I' are ignored.
//   - Pixels are not square to emphasize that we discard pixel shape information (i.e., pxl_dx, pxl_dy, pxl_dz).
//   - The inner ring of 'U' pixels are used directly. The outer ring of 'u' pixels are used only for computing 
//     finite-difference derivatives for 'U' pixels.
//
// (-1/2,-1/2)                                        (cols-1/2,-1/2)
//      \                                                 /
//       \_______________________________________________/
//(-1/2, |       |       |       |       |       |       |
//    0) |       |       |       |       |       |       |
//     \ |       |       |       |       |       | (0,   |
//      \| (0,0) | (0,1) | (0,2) | (0,3) |       |cols-1)|
//       |   I   |   u   |   u   |   I   |   I   |   I   |
//(-1/2, |       |       |       |       |       |       |
//  1/2) |       |       |       |       |       |       |
//    \  |       |       |       |       |       |       |
//     \ |       |       |       |       |       |       |
//      \|_______|_______|_______|_______|_______|_______|
//       |       |       |       |       |       |       |
//       |       |       |       |       |       |       |
//       |       |       |       |       |       |       |
//       | (1,0) |       |       |       |       |       |
//(-1/2, |   u   |   U+++|+++U   |   u   |   I   |   I   |
//  3/2) |       |   +   |   +   |       |       |       |
//   \   |       |   +   |   +   |       |       |       |
//    \  |       |   +   | 0 <----Interp.|       |       |
//     \ |       |   +   |   +   | Point |       |       |
//      \|_______|_______|_______|_______|_______|_______|
//       |       |   +   |   +   |       |       |       |
//       |       |   +   |   +   |       |       |      _|__ (rows-1, cols-1)
//       |(rows  |   +   |   +   |       |       |     / |
//       |  -1,0)|   +   |   +   |       |       |    /  |
//       |   u   |   U+++|+++U   |   u   |   I   |   I   |
//(-1/2, |       |       |       |       |       |       |
// rows- |       |       |       |       |       |       | (rows-1/2, cols-1/2)
//  1/2) |       |       |       |       |       |       |  /
//     \ |       |       |       |       |       |       | /
//      \|_______|_______|_______|_______|_______|_______|/
//
//
// NOTE: If the image has N rows (columns) then the input can range from [-1/2, N-1/2]. This is done because the pixel's 
//       value is treated as residing at the CENTRE of the pixel's spatial extent. So when you want to interpolate at the
//       point (0.0, 0.0) you should get the exact first pixel value. (If the input ranged from [0, N] then you would have
//       to specify (0.5, 0.5) to get the exact first pixel. That isn't very intuitive!)
//
//       For example, if you are scaling the image dimensions 2x (so the edge length is twice what is was before), and you
//       are populating a new image with interpolated values, then your loop will look like:
//
//                   for(int row = 0; row < new_rows; ++row){
//                       for(int col = 0; col < new_cols; ++col){
//                           auto interpolate_old_row_at = ((row*1.0 + 0.5)/2) - 0.5;
//                           ...
//                       }
//                   }
//
//       If instead of scaling by a factor of 2 you scale by a factor of N (a whole number), you will need to use:
//       (2*row - N + 1)/(2N). If you are reducing by a factor of 2 you can just use (row/2). (Using this routine to reduce
//       images is not recommended; pixels are discarded instead of properly condensed!)
//
// NOTE: This routine IGNORES pixel dimensions. Specifically, it treats them as equal. A separate interpolation scheme will
//       be needed to take this info into account.
//
// NOTE: This routine operates entirely in the 2D plane of the image, with no regard for the pxl_dz 'thickness'.
//
// NOTE: This routine uses MIRROR boundary conditions, meaning a virtual border of the edge points is placed around the
//       image such that they equal the nearest real pixel value. This is done to avoid handling overflow (as could happen
//       if extrapolation was used) and abrupt or sharp edges (as would happen if toroidal b.c.'s were used). 
//
//       Because the footprint of the window used to compute derivatives extends so far, you will want to ensure there are 
//       at least a few extra 'throwaway' pixels around the perimeter of the image. (This is common for, e.g., medical images.)
//
// NOTE: This routine interpolates nearest neighbour (i.e., directly adjacent and touching) and the single closest diagonal
//       pixel's values. The derivatives of these four points use their own nearest neighbours. So this routine will end up
//       using 12 distinct pixels for a single interpolation. (They will be evaluated several times each.)
//       
//       Be aware of the computational cost! This routine is *much* slower than bilinear interpolation. If possible, crop the 
//       image to some region of interest before running this routine!
//
// NOTE: This routine produces smooth output, but the derivatives are ficticious (i.e., they come from finite-difference 
//       calculations on the original pixel values).
//
//       This routine differs from bilinear interpolation in that the first derivative of the interpolation (in row- or column-axis 
//       directions) will be continuous (up to numerical precision). Also, the first derivative is ASSUMED to be equal to the finite
//       difference computed using neighbouring pixels. The derivatives will therefore, in general, not be meaningful. They are only
//       useful for bicubic interpolation purposes and will not match the true derivatives (if they are known). If the true 
//       derivatives are not known, and you are going to approximate them using finite differences anyways, then this routine is 
//       probably a good match for you.
//
template <class T,class R> T planar_image<T,R>::bicubically_interpolate_in_pixel_number_space(R row, R col, long int chnl) const { 
    //Get pixel space coordinates for the pixel that the user's input specifies.
    const auto r_e_p = static_cast<long int>(std::floor(row + static_cast<R>(0.5))); //"Enclosing pixel row"
    const auto c_e_p = static_cast<long int>(std::floor(col + static_cast<R>(0.5))); //"Enclosing pixel column"

    //Note: fails on out-of-bounds input.
    if(!isininc(0,r_e_p,this->rows-1) 
    || !isininc(0,c_e_p,this->columns-1) 
    || !isininc(0,chnl,this->channels-1)){
        FUNCERR("Attempted to access part of image which does not exist");
    }

    //Figure out which quandrant of the enclosing cell the point is. It changes which adjacent cells we will use to interpolate.
    const auto r_adj_is_plus = (row > r_e_p); //Adjacent cell should be taken to be r_e_p + 1. Else r_e_p - 1.
    const auto c_adj_is_plus = (col > c_e_p); //Adjacent cell should be taken to be c_e_p + 1. Else c_e_p - 1.

    //Figure out the 'virtual' minimum and maximum row and column numbers of the four nearest (surrounding) pixel centres.
    const auto r_min_virt = r_e_p + (r_adj_is_plus ? 0 : -1);
    const auto r_max_virt = r_e_p + (r_adj_is_plus ? 1 :  0);
    const auto c_min_virt = c_e_p + (c_adj_is_plus ? 0 : -1);
    const auto c_max_virt = c_e_p + (c_adj_is_plus ? 1 :  0);

    //Figure out the 'real' minimum and maximum row and column numbers of the four nearest (surrounding) pixel centres.
    // (This is where our mirror boundary conditions first come into play.
    const auto r_min = std::max(static_cast<long int>(0),r_min_virt);
    const auto r_max = std::min(this->rows-1, r_max_virt);
    const auto c_min = std::max(static_cast<long int>(0),c_min_virt);
    const auto c_max = std::min(this->columns-1, c_max_virt);

    //Get the fractional [0,1) indicator of row/col position between the min and max pixel coordinates.
    const auto drow = (row - static_cast<double>(r_min_virt)); // pxl_dx <-- if pixel shape were considered.
    const auto dcol = (col - static_cast<double>(c_min_virt)); // pxl_dy <-- if pixel shape were considered.

    //Get the pixel values at each of the four pixel centres.
    const auto y_r_min_c_min = static_cast<double>(this->data[this->index(r_min,c_min,chnl)]);
    const auto y_r_min_c_max = static_cast<double>(this->data[this->index(r_min,c_max,chnl)]);
    const auto y_r_max_c_min = static_cast<double>(this->data[this->index(r_max,c_min,chnl)]);
    const auto y_r_max_c_max = static_cast<double>(this->data[this->index(r_max,c_max,chnl)]);

    //Get the centered finite difference first derivatives along row and column axis for each pixel centre.
    const auto dydr_r_min_c_min = static_cast<double>(this->row_aligned_derivative_centered_finite_difference(r_min,c_min,chnl));
    const auto dydr_r_min_c_max = static_cast<double>(this->row_aligned_derivative_centered_finite_difference(r_min,c_max,chnl));
    const auto dydr_r_max_c_min = static_cast<double>(this->row_aligned_derivative_centered_finite_difference(r_max,c_min,chnl));
    const auto dydr_r_max_c_max = static_cast<double>(this->row_aligned_derivative_centered_finite_difference(r_max,c_max,chnl));

    const auto dydc_r_min_c_min = static_cast<double>(this->column_aligned_derivative_centered_finite_difference(r_min,c_min,chnl));
    const auto dydc_r_min_c_max = static_cast<double>(this->column_aligned_derivative_centered_finite_difference(r_min,c_max,chnl));
    const auto dydc_r_max_c_min = static_cast<double>(this->column_aligned_derivative_centered_finite_difference(r_max,c_min,chnl));
    const auto dydc_r_max_c_max = static_cast<double>(this->column_aligned_derivative_centered_finite_difference(r_max,c_max,chnl));

    //Get the centered finite difference second 'cross' derivative along row and column axis for each pixel centre.
    const auto d2ydrdc_r_min_c_min = static_cast<double>(this->cross_second_derivative_centered_finite_difference(r_min,c_min,chnl));
    const auto d2ydrdc_r_min_c_max = static_cast<double>(this->cross_second_derivative_centered_finite_difference(r_min,c_max,chnl));
    const auto d2ydrdc_r_max_c_min = static_cast<double>(this->cross_second_derivative_centered_finite_difference(r_max,c_min,chnl));
    const auto d2ydrdc_r_max_c_max = static_cast<double>(this->cross_second_derivative_centered_finite_difference(r_max,c_max,chnl));

#ifdef YGOR_IMAGES_DISABLE_EIGEN
    FUNCERR("This routine depends on Eigen and this functionality was disabled. Cannot continue");
    //NOTE: This particular application can be easily worked out and expressed in non-matrix form.
    //      Fire up your CAS of choice iff needed.
    return static_cast<T>(0);
#else
    Eigen::Matrix4d A;
    A <<  1.0,  0.0,  0.0,  0.0,
          0.0,  0.0,  1.0,  0.0,
         -3.0,  3.0, -2.0, -1.0,
          2.0, -2.0,  1.0,  1.0;
    auto AT = A.transpose();

    Eigen::Matrix4d F;
    F <<     y_r_min_c_min,     y_r_max_c_min,   -dydr_r_min_c_min,   -dydr_r_max_c_min,
             y_r_min_c_max,     y_r_max_c_max,   -dydr_r_min_c_max,   -dydr_r_max_c_max,
         -dydc_r_min_c_min, -dydc_r_max_c_min, d2ydrdc_r_min_c_min, d2ydrdc_r_max_c_min,
         -dydc_r_min_c_max, -dydc_r_max_c_max, d2ydrdc_r_min_c_max, d2ydrdc_r_max_c_max;

    auto C = A * F * AT; // Coefficients.

    double res = 0.0;
    for(int i = 0; i < 4; ++i){
        for(int j = 0; j < 4; ++j){
            res += C(i,j)*std::pow(dcol,i)*std::pow(drow,j);
        }
    } 
    return static_cast<T>( res );
#endif

}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template uint8_t  planar_image<uint8_t ,double>::bicubically_interpolate_in_pixel_number_space(double row, double col, long int chnl) const;
    template uint16_t planar_image<uint16_t,double>::bicubically_interpolate_in_pixel_number_space(double row, double col, long int chnl) const;
    template uint32_t planar_image<uint32_t,double>::bicubically_interpolate_in_pixel_number_space(double row, double col, long int chnl) const;
    template uint64_t planar_image<uint64_t,double>::bicubically_interpolate_in_pixel_number_space(double row, double col, long int chnl) const;
    template float    planar_image<float   ,double>::bicubically_interpolate_in_pixel_number_space(double row, double col, long int chnl) const;
#endif

//Average a block of pixels. Boundaries are inclusive. Out-of-bounds parts are ignored. Negatives OK (they are just ignored).
template <class T,class R> 
T
planar_image<T,R>::block_average(long int row_min, long int row_max, long int col_min, long int col_max, long int chnl) const {
    //On every failure, we return NaN.
    const auto failval = std::numeric_limits<T>::quiet_NaN();

    //Check if the channel exists.
    if(!isininc(0,chnl,this->channels-1)) return failval;

    //Check for backward parameter specification. 
    if(row_max < row_min){
        return this->block_average(row_max, row_min, col_min, col_max, chnl);
    }else if(col_max < col_min){
        return this->block_average(row_min, row_max, col_max, col_min, chnl);
    }

    //Check for a completely out-of-bounds rectangle.
    if((row_max < 0) || (col_max < 0) || (row_min > (this->rows-1)) || (col_min > (this->columns-1))) return failval;

    //Ignore invalid portions of the coordinates.
    const long int r_min = (row_min < 0) ? 0 : row_min;
    const long int c_min = (col_min < 0) ? 0 : col_min;
    const long int r_max = (row_max > (this->rows-1))    ? (this->rows-1)    : row_max;
    const long int c_max = (col_max > (this->columns-1)) ? (this->columns-1) : col_max;

    std::vector<T> vals;
    vals.reserve((r_max - r_min + 1) * (c_max - c_min + 1));
    for(long int r = r_min; r <= r_max; ++r){
        for(long int c = c_min; c <= c_max; ++c){
            vals.push_back(this->value(r,c,chnl));
        }
    }
    return Stats::Mean(vals);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template uint8_t  planar_image<uint8_t ,double>::block_average(long int, long int, long int, long int, long int) const;
    template uint16_t planar_image<uint16_t,double>::block_average(long int, long int, long int, long int, long int) const;
    template uint32_t planar_image<uint32_t,double>::block_average(long int, long int, long int, long int, long int) const;
    template uint64_t planar_image<uint64_t,double>::block_average(long int, long int, long int, long int, long int) const;
    template float    planar_image<float   ,double>::block_average(long int, long int, long int, long int, long int) const;
#endif

template <class T,class R> 
T
planar_image<T,R>::block_median(long int row_min, long int row_max, long int col_min, long int col_max, long int chnl) const {
    //On every failure, we return NaN.
    const auto failval = std::numeric_limits<T>::quiet_NaN();

    //Check if the channel exists.
    if(!isininc(0,chnl,this->channels-1)) return failval;

    //Check for backward parameter specification. 
    if(row_max < row_min){
        return this->block_median(row_max, row_min, col_min, col_max, chnl);
    }else if(col_max < col_min){
        return this->block_median(row_min, row_max, col_max, col_min, chnl);
    }

    //Check for a completely out-of-bounds rectangle.
    if((row_max < 0) || (col_max < 0) || (row_min > (this->rows-1)) || (col_min > (this->columns-1))) return failval;

    //Ignore invalid portions of the coordinates.
    const long int r_min = (row_min < 0) ? 0 : row_min;
    const long int c_min = (col_min < 0) ? 0 : col_min;
    const long int r_max = (row_max > (this->rows-1))    ? (this->rows-1)    : row_max;
    const long int c_max = (col_max > (this->columns-1)) ? (this->columns-1) : col_max;

    std::vector<T> vals;
    vals.reserve((r_max - r_min + 1) * (c_max - c_min + 1));
    for(long int r = r_min; r <= r_max; ++r){
        for(long int c = c_min; c <= c_max; ++c){
            vals.push_back(this->value(r,c,chnl));
        }
    }
    return Stats::Median(vals);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template uint8_t  planar_image<uint8_t ,double>::block_median(long int, long int, long int, long int, long int) const;
    template uint16_t planar_image<uint16_t,double>::block_median(long int, long int, long int, long int, long int) const;
    template uint32_t planar_image<uint32_t,double>::block_median(long int, long int, long int, long int, long int) const;
    template uint64_t planar_image<uint64_t,double>::block_median(long int, long int, long int, long int, long int) const;
    template float    planar_image<float   ,double>::block_median(long int, long int, long int, long int, long int) const;
#endif



//The min/maximum pixel values of all channels.
template <class T,class R> std::pair<T,T> planar_image<T,R>::minmax(void) const {
    if(this->rows*this->columns*this->channels <= 0){
        throw std::runtime_error("Cannot compute min/max of zero pixels. This is undefined!");
    }
    T min = std::numeric_limits<T>::max();
    T max = std::numeric_limits<T>::min();
    for(long int i = 0; i < this->rows*this->columns*this->channels; ++i){
        if(min > this->data[i]) min = this->data[i];
        if(max < this->data[i]) max = this->data[i];
    }
    return std::make_pair(min,max);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template std::pair<uint8_t ,uint8_t > planar_image<uint8_t ,double>::minmax(void) const;
    template std::pair<uint16_t,uint16_t> planar_image<uint16_t,double>::minmax(void) const;
    template std::pair<uint32_t,uint32_t> planar_image<uint32_t,double>::minmax(void) const;
    template std::pair<uint64_t,uint64_t> planar_image<uint64_t,double>::minmax(void) const;
    template std::pair<float   ,float   > planar_image<float   ,double>::minmax(void) const;
#endif


//Set all pixel data of specific channel to the given value. No-op if channel is non-existent.
template <class T,class R> void planar_image<T,R>::fill_pixels(long int chnl, T val){
    if(!isininc(0,chnl,this->channels-1)) return;

    //Feel free to speed this up using knowledge of the layout or contiguity or whatever. I needed it yesterday
    // when writing, so I wrote the first thing that popped into mind.
    for(auto row = 0; row < this->rows; ++row){
        for(auto col = 0; col < this->columns; ++col){ 
            this->reference(row, col, chnl) = val;
        }
    }                
    return;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template void planar_image<uint8_t ,double>::fill_pixels(long int chnl, uint8_t  val);
    template void planar_image<uint16_t,double>::fill_pixels(long int chnl, uint16_t val);
    template void planar_image<uint32_t,double>::fill_pixels(long int chnl, uint32_t val);
    template void planar_image<uint64_t,double>::fill_pixels(long int chnl, uint64_t val);
    template void planar_image<float   ,double>::fill_pixels(long int chnl, float    val);
#endif

//Set all pixel data (all channels) to the given value.
template <class T,class R> void planar_image<T,R>::fill_pixels(T val){
    //Feel free to speed this up using knowledge of the layout or contiguity or whatever. I needed it yesterday
    // when writing, so I wrote the first thing that popped into mind.
    //
    // This routine especially would benefit from something like memset_to_zero(this->data.data(), 0, rows*cols*chnls*sizeof(T))...
    for(auto row = 0; row < this->rows; ++row){
        for(auto col = 0; col < this->columns; ++col){
            for(auto chnl = 0; chnl < this->channels; ++chnl){
                this->reference(row, col, chnl) = val;
            }
        }                       
    }       
    return;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template void planar_image<uint8_t ,double>::fill_pixels(uint8_t  val);
    template void planar_image<uint16_t,double>::fill_pixels(uint16_t val);
    template void planar_image<uint32_t,double>::fill_pixels(uint32_t val);
    template void planar_image<uint64_t,double>::fill_pixels(uint64_t val);
    template void planar_image<float   ,double>::fill_pixels(float    val);
#endif

//Fill pixels above a given plane. Returns the number of affected pixels.
template<class T, class R>
long int
planar_image<T,R>::set_voxels_above_plane(const plane<R> &aplane, T val, std::set<long int> chnls){
    long int N = 0;
    for(auto row = 0; row < this->rows; ++row){
        for(auto col = 0; col < this->columns; ++col){ 
            const auto p = this->position(row, col);
            if(aplane.Is_Point_Above_Plane(p)){
                ++N;
                if(chnls.empty()){
                    for(auto c = 0; c < this->channels; ++c) this->reference(row, col, c) = val;
                }else{
                    for(const auto &c : chnls) this->reference(row, col, c) = val;
                }
            }
        }
    }
    return N;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template long int planar_image<uint8_t ,double>::set_voxels_above_plane(const plane<double> &aplane, uint8_t  val, std::set<long int>);
    template long int planar_image<uint16_t,double>::set_voxels_above_plane(const plane<double> &aplane, uint16_t val, std::set<long int>);
    template long int planar_image<uint32_t,double>::set_voxels_above_plane(const plane<double> &aplane, uint32_t val, std::set<long int>);
    template long int planar_image<uint64_t,double>::set_voxels_above_plane(const plane<double> &aplane, uint64_t val, std::set<long int>);
    template long int planar_image<float   ,double>::set_voxels_above_plane(const plane<double> &aplane, float    val, std::set<long int>);
#endif

//Apply a functor to individual pixels.
template<class T, class R>
void
planar_image<T,R>::apply_to_pixels(std::function<void(long int row, long int col, long int chnl, T &val)> func){
    for(auto row = 0; row < this->rows; ++row){
        for(auto col = 0; col < this->columns; ++col){
            for(auto chn = 0; chn < this->channels; ++chn){
                func(row, col, chn, this->reference(row, col, chn));
            }
        }
    }
    return;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template void planar_image<uint8_t ,double>::apply_to_pixels(std::function<void(long int row, long int col, long int chnl, uint8_t  &val)>);
    template void planar_image<uint16_t,double>::apply_to_pixels(std::function<void(long int row, long int col, long int chnl, uint16_t &val)>);
    template void planar_image<uint32_t,double>::apply_to_pixels(std::function<void(long int row, long int col, long int chnl, uint32_t &val)>);
    template void planar_image<uint64_t,double>::apply_to_pixels(std::function<void(long int row, long int col, long int chnl, uint64_t &val)>);
    template void planar_image<float   ,double>::apply_to_pixels(std::function<void(long int row, long int col, long int chnl, float    &val)>);
#endif


//Replace non-finite numbers.
template <class T,class R>
void 
planar_image<T,R>::replace_nonfinite_pixels_with(long int chnl, T val){
    if(!isininc(0,chnl,this->channels-1)) return;

    for(auto row = 0; row < this->rows; ++row){
        for(auto col = 0; col < this->columns; ++col){ 
            if(!std::isfinite(this->value(row, col, chnl))) this->reference(row, col, chnl) = val;
        }
    }                
    return;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template void planar_image<uint8_t ,double>::replace_nonfinite_pixels_with(long int chnl, uint8_t  val);
    template void planar_image<uint16_t,double>::replace_nonfinite_pixels_with(long int chnl, uint16_t val);
    template void planar_image<uint32_t,double>::replace_nonfinite_pixels_with(long int chnl, uint32_t val);
    template void planar_image<uint64_t,double>::replace_nonfinite_pixels_with(long int chnl, uint64_t val);
    template void planar_image<float   ,double>::replace_nonfinite_pixels_with(long int chnl, float    val);
#endif

template <class T,class R>
void 
planar_image<T,R>::replace_nonfinite_pixels_with(T val){
    for(auto row = 0; row < this->rows; ++row){
        for(auto col = 0; col < this->columns; ++col){ 
            for(auto chnl = 0; chnl < this->channels; ++chnl){
                if(!std::isfinite(this->value(row, col, chnl))) this->reference(row, col, chnl) = val;
            }
        }
    }                
    return;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template void planar_image<uint8_t ,double>::replace_nonfinite_pixels_with(uint8_t  val);
    template void planar_image<uint16_t,double>::replace_nonfinite_pixels_with(uint16_t val);
    template void planar_image<uint32_t,double>::replace_nonfinite_pixels_with(uint32_t val);
    template void planar_image<uint64_t,double>::replace_nonfinite_pixels_with(uint64_t val);
    template void planar_image<float   ,double>::replace_nonfinite_pixels_with(float    val);
#endif


//Get an R^3 position of the *center* of the pixel/voxel.
template <class T,class R> vec3<R> planar_image<T,R>::position(long int row, long int col) const {
    if( !isininc(0,row,this->rows-1) 
    ||  !isininc(0,col,this->columns-1)){
        FUNCERR("Attempted to access part of image which does not exist");
    }
    vec3<R> out(this->anchor);
    out += this->offset;
    out += this->row_unit*(this->pxl_dx*static_cast<R>(row));
    out += this->col_unit*(this->pxl_dy*static_cast<R>(col));
    return out;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template vec3<double> planar_image<uint8_t ,double>::position(long int row, long int col) const;
    template vec3<double> planar_image<uint16_t,double>::position(long int row, long int col) const;
    template vec3<double> planar_image<uint32_t,double>::position(long int row, long int col) const;
    template vec3<double> planar_image<uint64_t,double>::position(long int row, long int col) const;
    template vec3<double> planar_image<float   ,double>::position(long int row, long int col) const;
#endif


//Determine if a given point in R^3 is encompassed by this image (of 'thickness' pxl_dz).
template <class T,class R> bool planar_image<T,R>::encompasses_point(const vec3<R> &in) const {
    //This routine is suitable only for the rectangular slab images in this class.
    // For a generic 3D bounding box routine, see the YgorMath.cc SANDBOX section.
    //
    //Anyways: here we project the components of a given vector (minus the center of the image) 
    // onto the natural coordinate system given by the row and column unit vecs. If the components
    // have too large a projection, we know they must be outside the box.
    
    const auto r0 = this->position(           0,              0);
//    const auto r2 = this->position(this->rows-1,              0);
//    const auto r3 = this->position(this->rows-1,this->columns-1);   
//    const auto r4 = this->position(           0,this->columns-1);
    const auto dt = this->pxl_dz;

    const auto center      = this->center();
    const auto rowdist     = this->row_unit.Dot(in - center);
    const auto maxdistrow  = this->row_unit.Dot(r0 - center); //Extra half-pixel width accounted for below.

    const auto coldist     = this->col_unit.Dot(in - center);
    const auto maxdistcol  = this->col_unit.Dot(r0 - center); //Extra half-pixel width accounted for below.

    const auto perpdist    = (this->col_unit.Cross(this->row_unit)).Dot(in - center);

    if(YGORABS(perpdist) >= (dt*0.5)) return false;
    if(YGORABS(rowdist)  >= (YGORABS(maxdistrow)+this->pxl_dx*0.5)) return false;
    if(YGORABS(coldist)  >= (YGORABS(maxdistcol)+this->pxl_dy*0.5)) return false;
    return true;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::encompasses_point(const vec3<double> &in) const;
    template bool planar_image<uint16_t,double>::encompasses_point(const vec3<double> &in) const;
    template bool planar_image<uint32_t,double>::encompasses_point(const vec3<double> &in) const;
    template bool planar_image<uint64_t,double>::encompasses_point(const vec3<double> &in) const;
    template bool planar_image<float   ,double>::encompasses_point(const vec3<double> &in) const;
#endif

template <class T,class R> bool planar_image<T,R>::sandwiches_point_within_top_bottom_planes(const vec3<R> &in) const {
    const auto center   = this->center();
    const auto dt = this->pxl_dz;
    const auto N = this->col_unit.Cross(this->row_unit).unit();
    const auto perpdist = N.Dot(in - center);
    if(YGORABS(perpdist) >= (dt*0.5)) return false;
    return true;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::sandwiches_point_within_top_bottom_planes(const vec3<double> &in) const;
    template bool planar_image<uint16_t,double>::sandwiches_point_within_top_bottom_planes(const vec3<double> &in) const;
    template bool planar_image<uint32_t,double>::sandwiches_point_within_top_bottom_planes(const vec3<double> &in) const;
    template bool planar_image<uint64_t,double>::sandwiches_point_within_top_bottom_planes(const vec3<double> &in) const;
    template bool planar_image<float   ,double>::sandwiches_point_within_top_bottom_planes(const vec3<double> &in) const;
#endif

//Determine if a contour is contained within the 3D volume of the image.
template <class T,class R> bool planar_image<T,R>::encompasses_contour_of_points(const contour_of_points<R> &in) const {
    //Simply walk over all points in the contour, inspecting whether they are encompassed by the image.
    for(const auto & point : in.points){
        if(!this->encompasses_point(point)) return false;
    }
    return true;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::encompasses_contour_of_points(const contour_of_points<double> &in) const;
    template bool planar_image<uint16_t,double>::encompasses_contour_of_points(const contour_of_points<double> &in) const;
    template bool planar_image<uint32_t,double>::encompasses_contour_of_points(const contour_of_points<double> &in) const;
    template bool planar_image<uint64_t,double>::encompasses_contour_of_points(const contour_of_points<double> &in) const;
    template bool planar_image<float   ,double>::encompasses_contour_of_points(const contour_of_points<double> &in) const;
#endif

//Determine if at least one contour is fully contained within the 3D volume of the image.
template <class T,class R> bool planar_image<T,R>::encompasses_any_contour_in_collection(const contour_collection<R> &in) const {
    //Simply walk over all points in the contour, inspecting whether they are encompassed by the image.
    for(const auto & contour : in.contours){
        if(this->encompasses_contour_of_points(contour)) return true;
    }
    return false;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::encompasses_any_contour_in_collection(const contour_collection<double> &in) const;
    template bool planar_image<uint16_t,double>::encompasses_any_contour_in_collection(const contour_collection<double> &in) const;
    template bool planar_image<uint32_t,double>::encompasses_any_contour_in_collection(const contour_collection<double> &in) const;
    template bool planar_image<uint64_t,double>::encompasses_any_contour_in_collection(const contour_collection<double> &in) const;
    template bool planar_image<float   ,double>::encompasses_any_contour_in_collection(const contour_collection<double> &in) const;
#endif

//Determine if any vertex from a contour is contained within the 3D volume of the image.
template <class T,class R> bool planar_image<T,R>::encompasses_any_of_contour_of_points(const contour_of_points<R> &in) const {
    //Simply walk over all points in the contour, inspecting whether they are encompassed by the image.
    for(const auto & point : in.points){
        if(this->encompasses_point(point)) return true;
    }
    return false;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::encompasses_any_of_contour_of_points(const contour_of_points<double> &in) const;
    template bool planar_image<uint16_t,double>::encompasses_any_of_contour_of_points(const contour_of_points<double> &in) const;
    template bool planar_image<uint32_t,double>::encompasses_any_of_contour_of_points(const contour_of_points<double> &in) const;
    template bool planar_image<uint64_t,double>::encompasses_any_of_contour_of_points(const contour_of_points<double> &in) const;
    template bool planar_image<float   ,double>::encompasses_any_of_contour_of_points(const contour_of_points<double> &in) const;
#endif

//Determine if at least one contour is partially contained within the 3D volume of the image.
template <class T,class R> bool planar_image<T,R>::encompasses_any_part_of_contour_in_collection(const contour_collection<R> &in) const {
    for(const auto & contour : in.contours){
        if(this->encompasses_any_of_contour_of_points(contour)) return true;
    }
    return false;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::encompasses_any_part_of_contour_in_collection(const contour_collection<double> &in) const;
    template bool planar_image<uint16_t,double>::encompasses_any_part_of_contour_in_collection(const contour_collection<double> &in) const;
    template bool planar_image<uint32_t,double>::encompasses_any_part_of_contour_in_collection(const contour_collection<double> &in) const;
    template bool planar_image<uint64_t,double>::encompasses_any_part_of_contour_in_collection(const contour_collection<double> &in) const;
    template bool planar_image<float   ,double>::encompasses_any_part_of_contour_in_collection(const contour_collection<double> &in) const;
#endif


//Returns the R^3 center of the image. Nothing fancy.
template <class T,class R> vec3<R> planar_image<T,R>::center(void) const {
    const auto r0 = this->position(           0,              0);
//    const auto r1 = this->position(this->rows-1,              0);
    const auto r2 = this->position(this->rows-1,this->columns-1);
//    const auto r3 = this->position(           0,this->columns-1);
//    return (r0+r1+r2+r3)/4.0;
    return (r0+r2)/static_cast<R>(2);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template vec3<double> planar_image<uint8_t ,double>::center(void) const;
    template vec3<double> planar_image<uint16_t,double>::center(void) const;
    template vec3<double> planar_image<uint32_t,double>::center(void) const;
    template vec3<double> planar_image<uint64_t,double>::center(void) const;
    template vec3<double> planar_image<float   ,double>::center(void) const;
#endif

//Returns an ordered list of the corners of the 2D image. Does NOT use thickness!
template <class T,class R> std::list<vec3<R>> planar_image<T,R>::corners2D(void) const {
    std::list<vec3<R>> out;
    const auto Rrow(this->row_unit*this->pxl_dx*(R)(0.5)); //Vectors along the row/col vecs. Used for 
    const auto Rcol(this->row_unit*this->pxl_dx*(R)(0.5)); // translating the center to the corner.

    // Guaranteed point ordering:
    //
    // point(0,0)    point(#rows-1,0)
    //     \             /
    //       1---------2          positive
    //       |         |    _____\ row_unit
    //       |         |   |     /
    //       |         |   | 
    //       |         |   |   positive
    //       4---------3  \|/  col_unit
    //
    //Do *NOT* change the order of these points!
    out.push_back(this->position(           0,              0) - Rrow - Rcol);
    out.push_back(this->position(this->rows-1,              0) + Rrow - Rcol);
    out.push_back(this->position(this->rows-1,this->columns-1) + Rrow + Rcol);
    out.push_back(this->position(           0,this->columns-1) - Rrow + Rcol);
    return std::move(out);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template std::list<vec3<double>> planar_image<uint8_t ,double>::corners2D(void) const;
    template std::list<vec3<double>> planar_image<uint16_t,double>::corners2D(void) const;
    template std::list<vec3<double>> planar_image<uint32_t,double>::corners2D(void) const;
    template std::list<vec3<double>> planar_image<uint64_t,double>::corners2D(void) const;
    template std::list<vec3<double>> planar_image<float   ,double>::corners2D(void) const;
#endif

//Returns the plane that the image resides in. Useful for is_point_in_poly routines.
// The image lies wholy in this plane, though the plane is infinite and there will be numerical 
// precision loss the further from the centre of the image you look.
template <class T,class R> plane<R> planar_image<T,R>::image_plane(void) const {
    const auto orthog_unit = this->row_unit.Cross( this->col_unit );
    const auto centre_point = this->center();
    return plane<R>(orthog_unit, centre_point);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template plane<double> planar_image<uint8_t ,double>::image_plane(void) const;
    template plane<double> planar_image<uint16_t,double>::image_plane(void) const;
    template plane<double> planar_image<uint32_t,double>::image_plane(void) const;
    template plane<double> planar_image<uint64_t,double>::image_plane(void) const;
    template plane<double> planar_image<float   ,double>::image_plane(void) const;
#endif


//Returns true if the 3D volume of this image encompasses the 2D image of the given planar image.
template <class T,class R> bool planar_image<T,R>::encloses_2D_planar_image(const planar_image<T,R> &in) const {
    //This routine is slightly tricky because the encompass routines do not (and should not) include points
    // exactly on the boundary as being "encompassed."
    //
    //Therefore, we do a bit of a 'fuzzy' check here.
    const auto inc = in.corners2D();
    const auto thisc = this->corners2D();
    const auto scale = static_cast<R>(0.5)*YGORMIN(this->pxl_dx + this->pxl_dy, in.pxl_dx + in.pxl_dy);

    for(auto p_it = inc.begin(); p_it != inc.end(); ++p_it){
        //If the point is truly inside, no problem. If not, investigate further.
        if(!this->encompasses_point(*p_it)){
            //Slightly adjust the corner point and test it.
            const auto unit  = (*p_it - this->center()).unit();
            const auto Rtest = *p_it - unit*scale*static_cast<R>(0.01); //100x smaller than the smallest voxel width.
            if(!this->encompasses_point(Rtest)) return false;
        }
    }
    return true;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::encloses_2D_planar_image(const planar_image<uint8_t ,double> &in) const;
    template bool planar_image<uint16_t,double>::encloses_2D_planar_image(const planar_image<uint16_t,double> &in) const;
    template bool planar_image<uint32_t,double>::encloses_2D_planar_image(const planar_image<uint32_t,double> &in) const;
    template bool planar_image<uint64_t,double>::encloses_2D_planar_image(const planar_image<uint64_t,double> &in) const;
    template bool planar_image<float   ,double>::encloses_2D_planar_image(const planar_image<float   ,double> &in) const;
#endif

//Returns (R)(1.0) for perfect spatial overlap, (R)(0.0) for no spatial overlap.
template <class T,class R> R planar_image<T,R>::Spatial_Overlap_Dice_Sorensen_Coefficient(const planar_image<T,R> &in) const {
    //This function returns the Dice-Sørensen index applied to spatial (volumetric) overlap of two planar images.

    // Because the images not be aligned, the computation is difficult to work out in generality. Give it a shot if you can.
    // If you figure it out, you might want to extract the important parts into a set of volumetric boolean tools.
    // Might be better to just rely on an external library (CGAL?) for something of this sort...

    FUNCERR("Not yet implemented!");
    return static_cast<R>(0);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template double planar_image<uint8_t ,double>::Spatial_Overlap_Dice_Sorensen_Coefficient(const planar_image<uint8_t ,double> &in) const;
    template double planar_image<uint16_t,double>::Spatial_Overlap_Dice_Sorensen_Coefficient(const planar_image<uint16_t,double> &in) const;
    template double planar_image<uint32_t,double>::Spatial_Overlap_Dice_Sorensen_Coefficient(const planar_image<uint32_t,double> &in) const;
    template double planar_image<uint64_t,double>::Spatial_Overlap_Dice_Sorensen_Coefficient(const planar_image<uint64_t,double> &in) const;
    template double planar_image<float   ,double>::Spatial_Overlap_Dice_Sorensen_Coefficient(const planar_image<float   ,double> &in) const;
#endif


//Blur pixels isotropically, completely ignoring pixel shape and real-space coordinates.
//
// NOTE: This routine ignores the real-space coordinates of pixels, treats pixel dimensions isotropically,
//       and does not take any real-space parameters. The width 'sigma' is in units of pixels!
//
// NOTE: This routine uses a rectangular window which is wide enough so that the weighting of pixels ignored 
//       in the blur is 3*sigma OR LESS. Three sigma ~> 0.01 whereas five sigma ~> 1E-5 or so, but would involve
//       a lot more computation.
//
// NOTE: To handle boundaries and cutoff uniformly, the sum of weights is tracked and not merely assumed to 
//       be normalized to one. In particular, pixels near boundaries and corners will be more strongly weighted
//       by the original pixel than would pixels far from boundaries. The weighting cutoff (at N*sigma) is also
//       handled this way.  What this all means is that the summed pixel intensity *should* be maintained after
//       blurring (though at the time of writing this has not been verified).
//
// NOTE: This routine assumes that pixels are localized to a single point at the center of the pixel, not 
//       smeared out over the ranges [-pxl_dx,x,pxl_dx] and [-pxl_dy,y,pxl_dy]. Thus the weight of a pixel is
//       unambiguous. However, this 'discrete' treatment will skew pixel weighting. Especially if the sigma is 
//       very narrow and thus the weighting varies substantially over the width of a single pixel's dimensions. 
//
//       Fix this if you get a chance. Also don't count on either behaviour.
//
// NOTE: If you want to make this faster, start by pre-computing the matrix elements of the weighting kernel.
//       This will save a huge amount of computation. It is simply not of concern at the time of writing.
//       Additionally, the Gaussian kernel can be slipt and applied separately in X and Y directions. This could
//       speed it up because the implementation could become highly parallelized, if you are so inclined.
//
template <class T,class R> bool planar_image<T,R>::Gaussian_Pixel_Blur(std::set<long int> chnls, double sigma_in_units_of_pixels){
    //Verify that all indicated channels are actually present in the image. If not, fail before doing anything.
    for(const auto &achnl : chnls) if(!isininc(0,achnl,this->channels-1)) return false;

    //If chnls is empty, blur on all channels.
    for(long int chnl = 0; chnl < this->channels; ++chnl) chnls.insert(chnl);

    //Make a copy of the image so we can modify the pixels without destroying the computation.
    planar_image<T,R> ref_img(*this);

    //Gaussian blur over PIXELS via a rectangular grid of width and height 2*cutoff*sigma.
    const double pixel_sigma      = sigma_in_units_of_pixels;  //Width of Gaussian (for pixel intensity weighting).
    const double pixel_box_radius = 3.0*pixel_sigma; //How far away to stop computing. 3sigma ~> 0.01. 5sigma ~> 1E-5 or so.
    const long int pixel_R        = std::ceil(pixel_box_radius);
    const double w_denom          = 2.0*M_PI*pixel_sigma*pixel_sigma; //Helps prevent overflow.

    //Loop over the rows, columns, and channels.
    for(long int row = 0; row < this->rows; ++row){
        for(long int col = 0; col < this->columns; ++col){
            for(const auto &achnl : chnls){
                double sum_weights = 0.0;
                double sum_weighted_vals = 0.0;

                //Walk over the 'box' of nearby pixels and matrix elements (composed of the weighting kernel).
                for(long int l_row = (row-pixel_R); l_row < (row+pixel_R); ++l_row){
                    for(long int l_col = (col-pixel_R); l_col < (col+pixel_R); ++l_col){
                        if(isininc(0,l_row,this->rows-1) && isininc(0,l_col,this->columns-1)){
                            //Get the original (box) pixel's value.
                            const auto pixel_val = ref_img.value(l_row, l_col, achnl);
                            
                            //Get the weighting for this (box) pixel in terms of the distance to the present pixel.
                            //
                            // NOTE: Since these only depend on the (pixel-)distance between pixels, they could easily
                            //       be pre-computed. This would save a lot of computational time, and could be used as
                            //       a generic discrete convolution kernel routine for other types of blurring, etc..
                            const double sq_pixel_dist = std::pow(l_row-row,2) + std::pow(l_col-col,2);
                            const double weight        = std::exp(-0.5*sq_pixel_dist/(pixel_sigma*pixel_sigma))/w_denom;
                            sum_weights       += weight;
                            sum_weighted_vals += weight * static_cast<double>(pixel_val);
                        }
                    }
                }
    
                //Update the working image pixel values.
                if(sum_weights > 0.0){
                    this->reference(row, col, achnl) = static_cast<T>(sum_weighted_vals/sum_weights);
                }
            }
        }
    }

    this->metadata["Operations Performed"] += "Gaussian blurred;";
    this->metadata["Description"] += " Gaussian Blurred";

    //Reference image is now dropped.
    return true;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::Gaussian_Pixel_Blur(std::set<long int>, double);
    template bool planar_image<uint16_t,double>::Gaussian_Pixel_Blur(std::set<long int>, double);
    template bool planar_image<uint32_t,double>::Gaussian_Pixel_Blur(std::set<long int>, double);
    template bool planar_image<uint64_t,double>::Gaussian_Pixel_Blur(std::set<long int>, double);
    template bool planar_image<float   ,double>::Gaussian_Pixel_Blur(std::set<long int>, double);
#endif


//Checks if the key is present without inspecting the value.
template <class T,class R> bool planar_image<T,R>::MetadataKeyPresent(std::string key) const {
    return (this->metadata.find(key) != this->metadata.end());
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::MetadataKeyPresent(std::string key) const;
    template bool planar_image<uint16_t,double>::MetadataKeyPresent(std::string key) const;
    template bool planar_image<uint32_t,double>::MetadataKeyPresent(std::string key) const;
    template bool planar_image<uint64_t,double>::MetadataKeyPresent(std::string key) const;
    template bool planar_image<float   ,double>::MetadataKeyPresent(std::string key) const;
#endif

//Attempts to cast the value if present. Optional is disengaged if key is missing or cast fails.
template <class T,class R> template <class U> std::experimental::optional<U> 
planar_image<T,R>::GetMetadataValueAs(std::string key) const {
    const auto metadata_cit = this->metadata.find(key);
    if( (metadata_cit == this->metadata.end())  || !Is_String_An_X<U>(metadata_cit->second) ){
        return std::experimental::optional<U>();
    }else{
        return std::experimental::make_optional(stringtoX<U>(metadata_cit->second));
    }
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template std::experimental::optional<uint32_t> planar_image<uint8_t ,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<uint32_t> planar_image<uint16_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<uint32_t> planar_image<uint32_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<uint32_t> planar_image<uint64_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<uint32_t> planar_image<float   ,double>::GetMetadataValueAs(std::string key) const;

    template std::experimental::optional<long int> planar_image<uint8_t ,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<long int> planar_image<uint16_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<long int> planar_image<uint32_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<long int> planar_image<uint64_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<long int> planar_image<float   ,double>::GetMetadataValueAs(std::string key) const;

    template std::experimental::optional<float> planar_image<uint8_t ,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<float> planar_image<uint16_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<float> planar_image<uint32_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<float> planar_image<uint64_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<float> planar_image<float   ,double>::GetMetadataValueAs(std::string key) const;

    template std::experimental::optional<double> planar_image<uint8_t ,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<double> planar_image<uint16_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<double> planar_image<uint32_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<double> planar_image<uint64_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<double> planar_image<float   ,double>::GetMetadataValueAs(std::string key) const;

    template std::experimental::optional<std::string> planar_image<uint8_t ,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<std::string> planar_image<uint16_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<std::string> planar_image<uint32_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<std::string> planar_image<uint64_t,double>::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<std::string> planar_image<float   ,double>::GetMetadataValueAs(std::string key) const;
#endif

//---------------------------------------------------------------------------------------------------------------------------
//---------------------- planar_image_collection: a collection of logically-related planar_images  --------------------------
//---------------------------------------------------------------------------------------------------------------------------
template <class T,class R> planar_image_collection<T,R>::planar_image_collection() {}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template planar_image_collection<uint8_t ,double>::planar_image_collection();
    template planar_image_collection<uint16_t,double>::planar_image_collection();
    template planar_image_collection<uint32_t,double>::planar_image_collection();
    template planar_image_collection<uint64_t,double>::planar_image_collection();
    template planar_image_collection<float   ,double>::planar_image_collection();
#endif

template <class T,class R> planar_image_collection<T,R>::planar_image_collection(const planar_image_collection<T,R> &in) : images(in.images) {}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template planar_image_collection<uint8_t ,double>::planar_image_collection(const planar_image_collection<uint8_t ,double> &in);
    template planar_image_collection<uint16_t,double>::planar_image_collection(const planar_image_collection<uint16_t,double> &in);
    template planar_image_collection<uint32_t,double>::planar_image_collection(const planar_image_collection<uint32_t,double> &in);
    template planar_image_collection<uint64_t,double>::planar_image_collection(const planar_image_collection<uint64_t,double> &in);
    template planar_image_collection<float   ,double>::planar_image_collection(const planar_image_collection<float   ,double> &in);
#endif

template <class T,class R> planar_image_collection<T,R>::planar_image_collection(const std::list<planar_image<T,R>> &in) : images(in) {}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template planar_image_collection<uint8_t ,double>::planar_image_collection(const std::list<planar_image<uint8_t ,double>> &in);
    template planar_image_collection<uint16_t,double>::planar_image_collection(const std::list<planar_image<uint16_t,double>> &in);
    template planar_image_collection<uint32_t,double>::planar_image_collection(const std::list<planar_image<uint32_t,double>> &in);
    template planar_image_collection<uint64_t,double>::planar_image_collection(const std::list<planar_image<uint64_t,double>> &in);
    template planar_image_collection<float   ,double>::planar_image_collection(const std::list<planar_image<float   ,double>> &in);
#endif

//Member functions.
template <class T,class R> planar_image_collection<T,R> & planar_image_collection<T,R>::operator=(const planar_image_collection<T,R> &rhs){
    if(this == &rhs) return *this;
    this->images = rhs.images;
    return *this;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template planar_image_collection<uint8_t ,double> & planar_image_collection<uint8_t ,double>::operator=(const planar_image_collection<uint8_t ,double> &in);
    template planar_image_collection<uint16_t,double> & planar_image_collection<uint16_t,double>::operator=(const planar_image_collection<uint16_t,double> &in);
    template planar_image_collection<uint32_t,double> & planar_image_collection<uint32_t,double>::operator=(const planar_image_collection<uint32_t,double> &in);
    template planar_image_collection<uint64_t,double> & planar_image_collection<uint64_t,double>::operator=(const planar_image_collection<uint64_t,double> &in);
    template planar_image_collection<float   ,double> & planar_image_collection<float   ,double>::operator=(const planar_image_collection<float   ,double> &in);
#endif

template <class T,class R> bool planar_image_collection<T,R>::operator==(const planar_image_collection<T,R> &in) const {
    //First, check the obvious things.
    if(this->images.size() != in.images.size()) return false;

    //Now walk through the data and compare piecewise.
    for(auto itA = this->images.begin(), itB = in.images.begin(); (itA != this->images.end()) && (itB != in.images.end()); ++itA, ++itB){
        if((*itA) != (*itB)) return false;
    }
    return true;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::operator==(const planar_image_collection<uint8_t ,double> &in) const;
    template bool planar_image_collection<uint16_t,double>::operator==(const planar_image_collection<uint16_t,double> &in) const;
    template bool planar_image_collection<uint32_t,double>::operator==(const planar_image_collection<uint32_t,double> &in) const;
    template bool planar_image_collection<uint64_t,double>::operator==(const planar_image_collection<uint64_t,double> &in) const;
    template bool planar_image_collection<float   ,double>::operator==(const planar_image_collection<float   ,double> &in) const;
#endif

template <class T,class R> bool planar_image_collection<T,R>::operator!=(const planar_image_collection<T,R> &in) const {
    return !(*this == in);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::operator!=(const planar_image_collection<uint8_t ,double> &in) const;
    template bool planar_image_collection<uint16_t,double>::operator!=(const planar_image_collection<uint16_t,double> &in) const;
    template bool planar_image_collection<uint32_t,double>::operator!=(const planar_image_collection<uint32_t,double> &in) const;
    template bool planar_image_collection<uint64_t,double>::operator!=(const planar_image_collection<uint64_t,double> &in) const;
    template bool planar_image_collection<float   ,double>::operator!=(const planar_image_collection<float   ,double> &in) const;
#endif

template <class T,class R> bool planar_image_collection<T,R>::operator<(const planar_image_collection<T,R> &rhs) const {
    //Compares the number of images, and then compares ONLY the first image. This is not ideal, but then again sorting a collection
    // of things generally requires an obtuse approach. 
    if(this->images.size() != rhs.images.size()) return (this->images.size() < rhs.images.size());
    const auto itA = this->images.begin(), itB = rhs.images.begin();
    return ((*itA) < (*itB));
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::operator<(const planar_image_collection<uint8_t ,double> &in) const;
    template bool planar_image_collection<uint16_t,double>::operator<(const planar_image_collection<uint16_t,double> &in) const;
    template bool planar_image_collection<uint32_t,double>::operator<(const planar_image_collection<uint32_t,double> &in) const;
    template bool planar_image_collection<uint64_t,double>::operator<(const planar_image_collection<uint64_t,double> &in) const;
    template bool planar_image_collection<float   ,double>::operator<(const planar_image_collection<float   ,double> &in) const;
#endif

template <class T,class R> void planar_image_collection<T,R>::Swap(planar_image_collection<T,R> &rhs){
    if(this == &rhs) return; //Nothing to do.
    this->images.swap(rhs.images);
    return;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template void planar_image_collection<uint8_t ,double>::Swap(planar_image_collection<uint8_t ,double> &in);
    template void planar_image_collection<uint16_t,double>::Swap(planar_image_collection<uint16_t,double> &in);
    template void planar_image_collection<uint32_t,double>::Swap(planar_image_collection<uint32_t,double> &in);
    template void planar_image_collection<uint64_t,double>::Swap(planar_image_collection<uint64_t,double> &in);
    template void planar_image_collection<float   ,double>::Swap(planar_image_collection<float   ,double> &in);
#endif


template <class T,class R> void planar_image_collection<T,R>::Stable_Sort(std::function<bool(const planar_image<T,R> &lhs,const planar_image<T,R> &rhs)> lt_func){
    //If no operator< provided, use a sane default.
    auto default_lt_func = [](const planar_image<T,R> &lhs, const planar_image<T,R> &rhs) -> bool {
        return lhs < rhs;
    };
    if(lt_func){
        this->images.sort(lt_func);
    }else{
        this->images.sort(default_lt_func);
    }
    return;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template void planar_image_collection<uint8_t ,double>::Stable_Sort(
        std::function<bool(const planar_image<uint8_t ,double> &lhs, const planar_image<uint8_t ,double> &rhs)>);
    template void planar_image_collection<uint16_t,double>::Stable_Sort(
        std::function<bool(const planar_image<uint16_t,double> &lhs, const planar_image<uint16_t,double> &rhs)>);
    template void planar_image_collection<uint32_t,double>::Stable_Sort(
        std::function<bool(const planar_image<uint32_t,double> &lhs, const planar_image<uint32_t,double> &rhs)>);
    template void planar_image_collection<uint64_t,double>::Stable_Sort(
        std::function<bool(const planar_image<uint64_t,double> &lhs, const planar_image<uint64_t,double> &rhs)>);
    template void planar_image_collection<float   ,double>::Stable_Sort(
        std::function<bool(const planar_image<float   ,double> &lhs, const planar_image<float   ,double> &rhs)>);
#endif


template <class T,class R> 
template <class P>
void planar_image_collection<T,R>::Stable_Sort_on_Metadata_Keys_Value_Numeric(const std::string &thekey){
    auto Sort_Predicate_Numeric = [=](const planar_image<T,R> &lhs, 
                                      const planar_image<T,R> &rhs) -> bool {

        //This is an operator< routine which does a numeric sort on a single piece of metadata.
        // A missing key is treated as being > the maximum element of the numeric type.
        const auto lhs_metadata  = lhs.metadata.find(thekey);
        const bool lhs_exists    = (lhs_metadata != lhs.metadata.end());
        const auto rhs_metadata  = rhs.metadata.find(thekey);
        const bool rhs_exists    = (rhs_metadata != rhs.metadata.end());

        const bool lhs_converts  = (lhs_exists && Is_String_An_X<P>(lhs_metadata->second));
        const bool rhs_converts  = (rhs_exists && Is_String_An_X<P>(rhs_metadata->second));

        if( lhs_converts &&  rhs_converts) return stringtoX<P>(lhs_metadata->second) < stringtoX<P>(rhs_metadata->second);
        if( lhs_converts && !rhs_converts) return true;
        if(!lhs_converts &&  rhs_converts) return false;
        if(!lhs_converts && !rhs_converts) return false;  //This case is effectively equality, so not <.
    };

    this->Stable_Sort(Sort_Predicate_Numeric);
    return;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template void planar_image_collection<uint8_t ,double>::Stable_Sort_on_Metadata_Keys_Value_Numeric<long int>(const std::string &);
    template void planar_image_collection<uint16_t,double>::Stable_Sort_on_Metadata_Keys_Value_Numeric<long int>(const std::string &);
    template void planar_image_collection<uint32_t,double>::Stable_Sort_on_Metadata_Keys_Value_Numeric<long int>(const std::string &);
    template void planar_image_collection<uint64_t,double>::Stable_Sort_on_Metadata_Keys_Value_Numeric<long int>(const std::string &);
    template void planar_image_collection<float   ,double>::Stable_Sort_on_Metadata_Keys_Value_Numeric<long int>(const std::string &);

    template void planar_image_collection<uint8_t ,double>::Stable_Sort_on_Metadata_Keys_Value_Numeric<double>(const std::string &);
    template void planar_image_collection<uint16_t,double>::Stable_Sort_on_Metadata_Keys_Value_Numeric<double>(const std::string &);
    template void planar_image_collection<uint32_t,double>::Stable_Sort_on_Metadata_Keys_Value_Numeric<double>(const std::string &);
    template void planar_image_collection<uint64_t,double>::Stable_Sort_on_Metadata_Keys_Value_Numeric<double>(const std::string &);
    template void planar_image_collection<float   ,double>::Stable_Sort_on_Metadata_Keys_Value_Numeric<double>(const std::string &);
#endif

template <class T,class R> void planar_image_collection<T,R>::Stable_Sort_on_Metadata_Keys_Value_Lexicographic(const std::string &thekey){
    const auto Sort_Predicate_Lexicographic = [=](const planar_image<T,R> &lhs, 
                                                  const planar_image<T,R> &rhs) -> bool {

        //This is an operator< routine which does a lexicographic sort on a single piece of metadata.
        // A missing key is treated as being > than all possible strings and == all non-existent strings.
        const auto lhs_metadata  = lhs.metadata.find(thekey);
        const bool lhs_exists    = (lhs_metadata != lhs.metadata.end());
        const auto rhs_metadata  = rhs.metadata.find(thekey);
        const bool rhs_exists    = (rhs_metadata != rhs.metadata.end());
        
        if( lhs_exists &&  rhs_exists) return lhs_metadata->second < rhs_metadata->second;
        if( lhs_exists && !rhs_exists) return true;
        if(!lhs_exists &&  rhs_exists) return false;  
        if(!lhs_exists && !rhs_exists) return false;  //This case is effectively equality, so not <.
    };

    this->Stable_Sort(Sort_Predicate_Lexicographic);
    return;

    //auto Bounded_Sort_Predicate = std::bind(Sort_Predicate_Lexicographic, thekey, std::placeholders::_1, std::placeholders::_2);
    //this->Stable_Sort(Bounded_Sort_Predicate);
    //return;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template void planar_image_collection<uint8_t ,double>::Stable_Sort_on_Metadata_Keys_Value_Lexicographic(const std::string &);
    template void planar_image_collection<uint16_t,double>::Stable_Sort_on_Metadata_Keys_Value_Lexicographic(const std::string &);
    template void planar_image_collection<uint32_t,double>::Stable_Sort_on_Metadata_Keys_Value_Lexicographic(const std::string &);
    template void planar_image_collection<uint64_t,double>::Stable_Sort_on_Metadata_Keys_Value_Lexicographic(const std::string &);
    template void planar_image_collection<float   ,double>::Stable_Sort_on_Metadata_Keys_Value_Lexicographic(const std::string &);
#endif

//Generate a stable-ordered list of iterators to images. Be careful not to invalidate the data after calling these.
template <class T,class R> std::list<typename std::list<planar_image<T,R>>::iterator> planar_image_collection<T,R>::get_all_images(void){
    std::list<typename std::list<planar_image<T,R>>::iterator> out;

    //Cycle through all of the images, maintaining the current order.
    for(auto i_it = this->images.begin(); i_it != this->images.end(); ++i_it){
        out.push_back(i_it);
    }
    return out;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template std::list<typename std::list<planar_image<uint8_t ,double>>::iterator>  planar_image_collection<uint8_t ,double>::get_all_images(void);
    template std::list<typename std::list<planar_image<uint16_t,double>>::iterator>  planar_image_collection<uint16_t,double>::get_all_images(void);
    template std::list<typename std::list<planar_image<uint32_t,double>>::iterator>  planar_image_collection<uint32_t,double>::get_all_images(void);
    template std::list<typename std::list<planar_image<uint64_t,double>>::iterator>  planar_image_collection<uint64_t,double>::get_all_images(void);
    template std::list<typename std::list<planar_image<float   ,double>>::iterator>  planar_image_collection<float   ,double>::get_all_images(void);
#endif

//Generate a stable-ordered list of iterators to images. Be careful not to invalidate the data after calling these.
// If the selection criteria is true for an image, an iterator to it is added to the output.
template <class T,class R> std::list<typename std::list<planar_image<T,R>>::iterator> 
planar_image_collection<T,R>::get_images_satisfying(std::function<bool(const planar_image<T,R> &animg)> select_pred){
    std::list<typename std::list<planar_image<T,R>>::iterator> out;

    if(select_pred){
        for(auto i_it = this->images.begin(); i_it != this->images.end(); ++i_it){
            if(select_pred(*i_it)) out.push_back(i_it);
        }
    }
    return out;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template std::list<typename std::list<planar_image<uint8_t ,double>>::iterator>  
                               planar_image_collection<uint8_t ,double>::get_images_satisfying(std::function<bool(const planar_image<uint8_t ,double> &)> pred);
    template std::list<typename std::list<planar_image<uint16_t,double>>::iterator>  
                               planar_image_collection<uint16_t,double>::get_images_satisfying(std::function<bool(const planar_image<uint16_t,double> &)> pred);
    template std::list<typename std::list<planar_image<uint32_t,double>>::iterator>  
                               planar_image_collection<uint32_t,double>::get_images_satisfying(std::function<bool(const planar_image<uint32_t,double> &)> pred);
    template std::list<typename std::list<planar_image<uint64_t,double>>::iterator>  
                               planar_image_collection<uint64_t,double>::get_images_satisfying(std::function<bool(const planar_image<uint64_t,double> &)> pred);
    template std::list<typename std::list<planar_image<float   ,double>>::iterator>  
                               planar_image_collection<float   ,double>::get_images_satisfying(std::function<bool(const planar_image<float   ,double> &)> pred);
#endif


//Generate a stable-ordered list of iterators to images. Be careful not to invalidate the data after calling these.
template <class T,class R> 
std::list<typename std::list<planar_image<T,R>>::iterator> 
planar_image_collection<T,R>::get_images_which_encompass_point(const vec3<R> &in){
    auto select_predicate = [=](const planar_image<T,R> &animg) -> bool {
        return animg.encompasses_point(in);
    };
    return this->get_images_satisfying(select_predicate);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template std::list<typename std::list<planar_image<uint8_t ,double>>::iterator>
        planar_image_collection<uint8_t ,double>::get_images_which_encompass_point(const vec3<double> &in);
    template std::list<typename std::list<planar_image<uint16_t,double>>::iterator>
        planar_image_collection<uint16_t,double>::get_images_which_encompass_point(const vec3<double> &in);
    template std::list<typename std::list<planar_image<uint32_t,double>>::iterator>
        planar_image_collection<uint32_t,double>::get_images_which_encompass_point(const vec3<double> &in);
    template std::list<typename std::list<planar_image<uint64_t,double>>::iterator> 
        planar_image_collection<uint64_t,double>::get_images_which_encompass_point(const vec3<double> &in);
    template std::list<typename std::list<planar_image<float   ,double>>::iterator> 
        planar_image_collection<float   ,double>::get_images_which_encompass_point(const vec3<double> &in);
#endif

//Generate a stable-ordered list of iterators to images. Be careful not to invalidate the data after calling these.
template <class T,class R> 
std::list<typename std::list<planar_image<T,R>>::iterator>  
planar_image_collection<T,R>::get_images_which_encompass_all_points(const std::list<vec3<R>> &in){
    auto select_predicate = [=](const planar_image<T,R> &animg) -> bool {
        for(const auto &apoint : in){
            if(!animg.encompasses_point(apoint)) return false;
        }
        return true;
    };
    return this->get_images_satisfying(select_predicate);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template std::list<typename std::list<planar_image<uint8_t ,double>>::iterator>
        planar_image_collection<uint8_t ,double>::get_images_which_encompass_all_points(const std::list<vec3<double>> &in);
    template std::list<typename std::list<planar_image<uint16_t,double>>::iterator>
        planar_image_collection<uint16_t,double>::get_images_which_encompass_all_points(const std::list<vec3<double>> &in);
    template std::list<typename std::list<planar_image<uint32_t,double>>::iterator>
        planar_image_collection<uint32_t,double>::get_images_which_encompass_all_points(const std::list<vec3<double>> &in);
    template std::list<typename std::list<planar_image<uint64_t,double>>::iterator>
        planar_image_collection<uint64_t,double>::get_images_which_encompass_all_points(const std::list<vec3<double>> &in);
    template std::list<typename std::list<planar_image<float   ,double>>::iterator>
        planar_image_collection<float   ,double>::get_images_which_encompass_all_points(const std::list<vec3<double>> &in);
#endif


//Returns a list of pointers to images which are sandwiched between the infinite planes lying along the top and bottom of the planar image (of finite thickness). 
// Be careful not to invalidate the data after calling this function.
template <class T,class R>
std::list<typename std::list<planar_image<T,R>>::iterator> 
planar_image_collection<T,R>::get_images_which_sandwich_point_within_top_bottom_planes(const vec3<R> &in){
    auto select_predicate = [=](const planar_image<T,R> &animg) -> bool {
        return animg.sandwiches_point_within_top_bottom_planes(in);
    };
    return this->get_images_satisfying(select_predicate);
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template std::list<typename std::list<planar_image<uint8_t ,double>>::iterator> 
        planar_image_collection<uint8_t ,double>::get_images_which_sandwich_point_within_top_bottom_planes(const vec3<double> &in);
    template std::list<typename std::list<planar_image<uint16_t,double>>::iterator> 
        planar_image_collection<uint16_t,double>::get_images_which_sandwich_point_within_top_bottom_planes(const vec3<double> &in);
    template std::list<typename std::list<planar_image<uint32_t,double>>::iterator> 
        planar_image_collection<uint32_t,double>::get_images_which_sandwich_point_within_top_bottom_planes(const vec3<double> &in);
    template std::list<typename std::list<planar_image<uint64_t,double>>::iterator> 
        planar_image_collection<uint64_t,double>::get_images_which_sandwich_point_within_top_bottom_planes(const vec3<double> &in);
    template std::list<typename std::list<planar_image<float   ,double>>::iterator> 
        planar_image_collection<float   ,double>::get_images_which_sandwich_point_within_top_bottom_planes(const vec3<double> &in);
#endif

//Returns two lists of pointers to images which are nearest and either above (pair.first) or below (pair.second) the
// plane of the image, but not encompassing it. Images are sorted by their distance to the specified image (nearest in
// the front). If there are no images above or below, the respective list is empty. The distance to the images is not
// considered, so there could be a gap or overlap between images. Images are assumed to be parallel and the centers are
// used for distance computation.
//
// This routine is used for finding nearest-neighbours when the image slices are known (or expected) to contiguously
// cover some volume (no gap, no overlap).
//
template <class T,class R>
std::pair< std::list< typename std::list<planar_image<T,R>>::iterator >,
           std::list< typename std::list<planar_image<T,R>>::iterator > >
planar_image_collection<T,R>::get_nearest_images_above_below_not_encompassing_image(const planar_image<T,R> &animg){
    const auto animg_plane = animg.image_plane();
    auto all_imgs = this->get_all_images();

    //Remove images if they encompass the specified image.
    all_imgs.remove_if( [animg](images_list_it_t it) -> bool {
            if(animg.encompasses_point( it->center() )) return true;
            return false;
        });

    //Sort based on distance to the plane of the specified image.
    all_imgs.sort([animg_plane]( const images_list_it_t &A, const images_list_it_t &B ) -> bool {
            return (   std::abs(animg_plane.Get_Signed_Distance_To_Point( A->center() ))
                     < std::abs(animg_plane.Get_Signed_Distance_To_Point( B->center() )) );
        });

    //Parition the images into above and below groups.
    auto above = all_imgs;
    auto below = all_imgs;

    above.remove_if([animg_plane](images_list_it_t it) -> bool {
            if(animg_plane.Is_Point_Above_Plane( it->center() )) return false;
            return true;
        });

    below.remove_if([animg_plane](images_list_it_t it) -> bool {
            if(animg_plane.Is_Point_Above_Plane( it->center() )) return true;
            return false;
        });

    return { above, below };
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template std::pair< std::list<typename std::list<planar_image<uint8_t ,double>>::iterator>,
                        std::list<typename std::list<planar_image<uint8_t ,double>>::iterator> >
        planar_image_collection<uint8_t ,double>::get_nearest_images_above_below_not_encompassing_image(
            const planar_image<uint8_t ,double> & );
    template std::pair< std::list<typename std::list<planar_image<uint16_t,double>>::iterator>,
                        std::list<typename std::list<planar_image<uint16_t,double>>::iterator> >
        planar_image_collection<uint16_t,double>::get_nearest_images_above_below_not_encompassing_image(
            const planar_image<uint16_t,double> & );
    template std::pair< std::list<typename std::list<planar_image<uint32_t,double>>::iterator>,
                        std::list<typename std::list<planar_image<uint32_t,double>>::iterator> >
        planar_image_collection<uint32_t,double>::get_nearest_images_above_below_not_encompassing_image(
            const planar_image<uint32_t,double> & );
    template std::pair< std::list<typename std::list<planar_image<uint64_t,double>>::iterator>,
                        std::list<typename std::list<planar_image<uint64_t,double>>::iterator> >
        planar_image_collection<uint64_t,double>::get_nearest_images_above_below_not_encompassing_image(
            const planar_image<uint64_t,double> & );
    template std::pair< std::list<typename std::list<planar_image<float   ,double>>::iterator>,
                        std::list<typename std::list<planar_image<float   ,double>>::iterator> >
        planar_image_collection<float   ,double>::get_nearest_images_above_below_not_encompassing_image(
            const planar_image<float   ,double> & );
#endif

//Returns the metadata key-values that are "common" (i.e., identical among all images).
// For ordering purposes, images in *this are considered to have priority over those in 'in'.
template <class T,class R>
std::map<std::string,std::string> 
planar_image_collection<T,R>::get_common_metadata(const std::list<images_list_it_t> &in) const {
    //Collect all available metadata.

    std::list<typename std::list<planar_image<T,R>>::const_iterator> img_its;

    for(auto it = this->images.cbegin(); it != this->images.cend(); ++it){
        img_its.emplace_back(it);
    }
    for(const auto &it : in) img_its.emplace_back(it);

    std::multimap<std::string,std::string> all_m;
    for(const auto &img_it : img_its){ 
        for(const auto &m : img_it->metadata){
            //If the key is not present, insert it unconditionally.
            //If the key is present twice or more, it is already spoiled so move along.
            //If the key is present once, check if values differ; insert iff they do.
            const auto count = all_m.count(m.first);
            if( (count == 0)
            ||  ((count == 1) && (all_m.lower_bound(m.first)->second != m.second)) ){
                all_m.insert(m);
            }
        }
    }

    //Construct the outgoing metadata iff there was a single value corresponding to a given key.
    std::map<std::string,std::string> out;
    for(const auto &m : all_m){
        if(all_m.count(m.first) == 1) out.insert(m);
    }
    return out;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template std::map<std::string,std::string> 
        planar_image_collection<uint8_t ,double>::get_common_metadata(const std::list<images_list_it_t> &) const;
    template std::map<std::string,std::string> 
        planar_image_collection<uint16_t,double>::get_common_metadata(const std::list<images_list_it_t> &) const;
    template std::map<std::string,std::string> 
        planar_image_collection<uint32_t,double>::get_common_metadata(const std::list<images_list_it_t> &) const;
    template std::map<std::string,std::string> 
        planar_image_collection<uint64_t,double>::get_common_metadata(const std::list<images_list_it_t> &) const;
    template std::map<std::string,std::string> 
        planar_image_collection<float   ,double>::get_common_metadata(const std::list<images_list_it_t> &) const;
#endif

//Returns a copy of all values that correspond to the given key. Order is maintained.
template <class T,class R>
std::list<std::string> 
planar_image_collection<T,R>::get_all_values_for_key(const std::string &akey) const {
    std::list<std::string> out;
    for(const auto &animg : this->images){
        auto it = animg.metadata.find(akey);
        if(it != animg.metadata.end()){
            out.emplace_back(it->second);
        }
    }
    return out;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template std::list<std::string> 
        planar_image_collection<uint8_t ,double>::get_all_values_for_key(const std::string &) const;
    template std::list<std::string>
        planar_image_collection<uint16_t,double>::get_all_values_for_key(const std::string &) const;
    template std::list<std::string>
        planar_image_collection<uint32_t,double>::get_all_values_for_key(const std::string &) const;
    template std::list<std::string>
        planar_image_collection<uint64_t,double>::get_all_values_for_key(const std::string &) const;
    template std::list<std::string>
        planar_image_collection<float   ,double>::get_all_values_for_key(const std::string &) const;
#endif

//Returns a copy of all unique values that correspond to the given key. Original order is maintained.
template <class T,class R>
std::list<std::string> 
planar_image_collection<T,R>::get_unique_values_for_key(const std::string &akey) const {
    auto all_values = this->get_all_values_for_key(akey);
    std::map<std::string,size_t> counts;
    for(const auto &avalue : all_values) counts[avalue]++;
    
    std::list<std::string> out;
    for(const auto &avalue : all_values){
        if(counts[avalue] == 1) out.emplace_back(avalue);
    }
    return out;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template std::list<std::string>
        planar_image_collection<uint8_t ,double>::get_unique_values_for_key(const std::string &) const;
    template std::list<std::string>
        planar_image_collection<uint16_t,double>::get_unique_values_for_key(const std::string &) const;
    template std::list<std::string>
        planar_image_collection<uint32_t,double>::get_unique_values_for_key(const std::string &) const;
    template std::list<std::string>
        planar_image_collection<uint64_t,double>::get_unique_values_for_key(const std::string &) const;
    template std::list<std::string>
        planar_image_collection<float   ,double>::get_unique_values_for_key(const std::string &) const;
#endif

//Image pruning/partitioning routine. Returns 'pruned' images; retains the rest. If pruning predicate is true, image is pruned.
// Image order is stable for both pruned and retained images.
//
// NOTE: Original images are transferred to the new planar_image_collection using std::list:splice or equivalent.
//
template <class T,class R>
planar_image_collection<T,R> 
planar_image_collection<T,R>::Prune_Images_Satisfying(std::function<bool(const planar_image<T,R> &animg)> prune_pred){
    planar_image_collection<T,R> out;

    if(prune_pred){
        for(auto i_it = this->images.begin(); i_it != this->images.end();  ){
            if(prune_pred(*i_it)){
                auto i_it_next = std::next(i_it); 
                out.images.splice( out.images.end(), this->images, i_it );
                i_it = i_it_next;
            }else{
                ++i_it;
            }
        }
    }
    return out;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template planar_image_collection<uint8_t ,double>
             planar_image_collection<uint8_t ,double>::Prune_Images_Satisfying(std::function<bool(const planar_image<uint8_t ,double> &animg)> pred);
    template planar_image_collection<uint16_t,double>
             planar_image_collection<uint16_t,double>::Prune_Images_Satisfying(std::function<bool(const planar_image<uint16_t,double> &animg)> pred);
    template planar_image_collection<uint32_t,double>
             planar_image_collection<uint32_t,double>::Prune_Images_Satisfying(std::function<bool(const planar_image<uint32_t,double> &animg)> pred);
    template planar_image_collection<uint64_t,double>
             planar_image_collection<uint64_t,double>::Prune_Images_Satisfying(std::function<bool(const planar_image<uint64_t,double> &animg)> pred);
    template planar_image_collection<float   ,double>
             planar_image_collection<float   ,double>::Prune_Images_Satisfying(std::function<bool(const planar_image<float   ,double> &animg)> pred);
#endif

//Image pruning/partitioning routine. Returns 'retained' images; prunes the rest. If retaining predicate evaluates to true, image is retained.
// Image order is stable for both pruned and retained images.
//
// NOTE: Original images are transferred to the new planar_image_collection using std::list:splice or equivalent.
//
template <class T,class R>
planar_image_collection<T,R> 
planar_image_collection<T,R>::Retain_Images_Satisfying(std::function<bool(const planar_image<T,R> &animg)> retain_pred){
    planar_image_collection<T,R> out;
    if(!retain_pred) return out;

    for(auto i_it = this->images.begin(); i_it != this->images.end();  ){
        if(retain_pred(*i_it)){
            ++i_it;
        }else{
            auto i_it_next = std::next(i_it); 
            out.images.splice( out.images.end(), this->images, i_it );
            i_it = i_it_next;
        }
    }
    return out;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template planar_image_collection<uint8_t ,double>
             planar_image_collection<uint8_t ,double>::Retain_Images_Satisfying(std::function<bool(const planar_image<uint8_t ,double> &animg)> pred);
    template planar_image_collection<uint16_t,double>
             planar_image_collection<uint16_t,double>::Retain_Images_Satisfying(std::function<bool(const planar_image<uint16_t,double> &animg)> pred);
    template planar_image_collection<uint32_t,double>
             planar_image_collection<uint32_t,double>::Retain_Images_Satisfying(std::function<bool(const planar_image<uint32_t,double> &animg)> pred);
    template planar_image_collection<uint64_t,double>
             planar_image_collection<uint64_t,double>::Retain_Images_Satisfying(std::function<bool(const planar_image<uint64_t,double> &animg)> pred);
    template planar_image_collection<float   ,double>
             planar_image_collection<float   ,double>::Retain_Images_Satisfying(std::function<bool(const planar_image<float   ,double> &animg)> pred);
#endif

//Generic routine for processing/combining groups of images into single images. Useful for spatial averaging, blurring, etc..
// Modified *this, so deep-copy beforehand if needed. See in-source default functors for descriptions/examples.
template <class T,class R>
bool planar_image_collection<T,R>::Process_Images(
                             std::function<typename std::list<images_list_it_t> (images_list_it_t,
                                      std::reference_wrapper<planar_image_collection<T,R>> )>         image_grouper,
                             std::function<bool (images_list_it_t, 
                                      std::list<images_list_it_t>,
                                      std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                                      std::list<std::reference_wrapper<contour_collection<R>>>,
                                      std::experimental::any )>                                       operation_functor,      
                             std::list<std::reference_wrapper<planar_image_collection<T,R>>>          external_images,
                             std::list<std::reference_wrapper<contour_collection<R>>>                 contour_collections,
                             std::experimental::any                                                   user_data ){


    //Generate a comprehensive list of iterators to all remaining images in the deep-copied array. This list will be
    // pruned after images have been successfully averaged or otherwise manipulated.
    auto to_be_avgd = this->get_all_images();

    //Loop over the iterators in the comprehensive list. (The loop will have elements erased from it as you go.)
    while(!to_be_avgd.empty()){
        //Take the first iterator and find all other images which should be averaged with it.
        // Generally you will simply want spatial overlap, but you can do whatever you want with the metadata too.
        // Be selective at this stage because later we simply walk over images on a pixel-by-pixel basis.
        auto curr_img_it = to_be_avgd.front(); //A copy of the iterator, not a reference to it, is needed here.

        //Functor for grouping images to be processed together as a single batches. 
        //
        // Given an arbitrary image and access to all images in the array, this functor should find all images that
        // should be operated on concurrently with the provided image (including the image itself).
        //
        // It's important to be both selective and consistent here. All images must eventually be selected, though
        // you can opt to simply discard the uninteresting images after processing.
        //
        // NOTE: Since at least one image must be present in the output (by definition first_img_it must be present
        //       in it's own group), returning an empty group, or a group without first_img_it in it, is considered
        //       a failure.
        auto default_image_grouper = [](typename planar_image_collection<T,R>::images_list_it_t first_img_it, 
                                        typename std::reference_wrapper<planar_image_collection<T,R>> pic) -> auto {

            //Default: groups with only one member: the first image.
            return std::list< planar_image_collection<T,R>::images_list_it_t >({first_img_it});

            //Alternative: group all images together into one big group.
            //return pic.get_all_images();

            //Alternative: group only images which spatially overlap.
            //const auto img_cntr = first_img_it->center();
            //const auto ortho = first_img_it->row_unit.Cross( first_img_it->col_unit ).unit();
            //const std::list<vec3<R>> points = { img_cntr, img_cntr + ortho * first_img_it->pxl_dz * (R)(0.25),
            //                                              img_cntr - ortho * first_img_it->pxl_dz * (R)(0.25) };
            //return pic.get_images_which_encompass_all_points(points);
        };

        auto selected_imgs = (image_grouper) ? image_grouper(curr_img_it, *this) 
                                             : default_image_grouper(curr_img_it, *this);

        //Consistency check of user's grouping functor: verify curr_img_it is present in the selected images.
        {
          bool present = false;
          for(const auto &an_img_it : selected_imgs){
              if(an_img_it == curr_img_it){
                  present = true;
                  break;
              }
          }
          if(!present){
              FUNCWARN("Image grouping functor failed to select self image and thus failed basic consistency. "
                       "(Each image's group must be composed of at least the image.)");
              return false;
          }
        }

        //Verify that there are an identical number of rows, columns, and channels in each remaining image. Die if
        // this is not the case. Ignore differences in spatial layout at this stage; this detail should have been 
        // taken care of when selecting the spatially-overlapping images.
        {
          auto rows     = curr_img_it->rows;
          auto columns  = curr_img_it->columns;
          auto channels = curr_img_it->channels;

          for(const auto &an_img_it : selected_imgs){
              if( (rows     != an_img_it->rows)
              ||  (columns  != an_img_it->columns)
              ||  (channels != an_img_it->channels) ){
                  FUNCWARN("Images have differing number of rows, columns, or channels. Are you sure you've got the correct data?");
                  return false;
                  //This might not be an error if you know what you're doing. However, you will still potentially attempt to 
                  // access an image's pixels out-of-bounds without altering the following code. (Are you sure you want this?
                  // Wouldn't it be easier and more logically sound to try something like voxel resampling, or explicit zeroing
                  // of the jutting image portions beforehand instead?)
                  //
                  // If the answers to these questions are "no" then go ahead and remove the 'return false' but leave the 
                  // notice.
                  //

              }
          }
        }

        //Functor for the main operation, where we will process a batch of images and produce a single output.
        //
        // For example, we might want to average the image group. The image pointed to by `first_img_it` will be
        // the only image retained -- the rest will be pruned right after this routine is called.
        //
        // NOTE: The image pointed to by first_img_it is also pointed to by one of the iterators in selected_img_its,
        //       so you'll need to be VERY careful about how you update the image data. To get around this, you should
        //       (1) create a DEEP COPY of first_img_it, (2) modify the copy as needed, and (3) DEEP COPY back to the 
        //       original first_img_it before returning.
        //
        // NOTE: Why not just always provide a deep copy, or working space for one? Because sometimes you can get around 
        //       the issue without unnecessary copying.
        //
        // NOTE: This routine should only return true if everything was OK.
        //
        auto default_op_func = [](typename planar_image_collection<T,R>::images_list_it_t first_img_it,
                                  typename std::list<planar_image_collection<T,R>::images_list_it_t> selected_img_its,
                                  typename std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                                  typename std::list<std::reference_wrapper<contour_collection<R>>>,
                                  typename std::experimental::any ) -> bool {

            //Default: do absolutely nothing.
            return true;
 
            //Alternative: average pixels.
            //for(rows) for(cols) for(chnls) { 
            //    for(auto &an_img_it : selected_img_its){
            //        pixel_val = an_img_it->value(row, col, chan);
            //        pixel_vals.push_back( static_cast<double>(pixel_val) );
            //    }
            //    pixel_mean = Stats::Mean(pixel_vals);
            //    first_img_it->reference(row, col, chan) = static_cast<T>(pixel_mean);
            //}
        
            //Alternative: tweak/insert/erase the metadata.
            //auto orig_md = first_img_it->metadata;
            //first_img_it->metadata["Operations performed"] += "something, "; 
        };




        if(operation_functor){
            operation_functor(curr_img_it, selected_imgs, external_images, contour_collections, user_data);
        }else{
            default_op_func(curr_img_it, selected_imgs, external_images, contour_collections, user_data);
        }

        //Remove *all* of the overlapping image iterators from the to_be_avgd list, including the image you 
        // just altered. At the same time, remove all but the first overlapping image from the image array.
        // Be careful to purge in the correct order -- removing the actual image will invalidate any dangling
        // iterators! (Prune from the pre-pruned list first, because that won't invalidate the actual iters.)
        for(auto &an_img_it : selected_imgs){
             to_be_avgd.remove(an_img_it); //std::list::remove() erases all elements equal to input value.

             if(an_img_it != curr_img_it){
                 this->images.erase(an_img_it);
             }
        }   

        //Print a simple progress bar to the screen. Helpful for long-running processing.
        FUNCINFO("Images still to be processed: " << to_be_avgd.size());
    }

    return true;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::Process_Images( 
         std::function<typename std::list<images_list_it_t> (images_list_it_t, 
                  std::reference_wrapper<planar_image_collection<uint8_t ,double>> )>,
         std::function<bool (images_list_it_t, 
                  std::list<images_list_it_t>, 
                  std::list<std::reference_wrapper<planar_image_collection<uint8_t ,double>>>,
                  std::list<std::reference_wrapper<contour_collection<double>>>,
                  std::experimental::any )>,
         std::list<std::reference_wrapper<planar_image_collection<uint8_t ,double>>>,
         std::list<std::reference_wrapper<contour_collection<double>>>,
         std::experimental::any );

    template bool planar_image_collection<uint16_t,double>::Process_Images(
         std::function<typename std::list<images_list_it_t> (images_list_it_t, 
                  std::reference_wrapper<planar_image_collection<uint16_t,double>> )>,
         std::function<bool (images_list_it_t, 
                  std::list<images_list_it_t>, 
                  std::list<std::reference_wrapper<planar_image_collection<uint16_t,double>>>,
                  std::list<std::reference_wrapper<contour_collection<double>>>, 
                  std::experimental::any )>, 
         std::list<std::reference_wrapper<planar_image_collection<uint16_t,double>>>,
         std::list<std::reference_wrapper<contour_collection<double>>>,
         std::experimental::any );

    template bool planar_image_collection<uint32_t,double>::Process_Images(
         std::function<typename std::list<images_list_it_t> (images_list_it_t, 
                  std::reference_wrapper<planar_image_collection<uint32_t,double>> )>,
         std::function<bool (images_list_it_t, 
                  std::list<images_list_it_t>, 
                  std::list<std::reference_wrapper<planar_image_collection<uint32_t,double>>>,
                  std::list<std::reference_wrapper<contour_collection<double>>>, 
                  std::experimental::any )>, 
         std::list<std::reference_wrapper<planar_image_collection<uint32_t,double>>>,
         std::list<std::reference_wrapper<contour_collection<double>>>,
         std::experimental::any );

    template bool planar_image_collection<uint64_t,double>::Process_Images(
         std::function<typename std::list<images_list_it_t> (images_list_it_t, 
                  std::reference_wrapper<planar_image_collection<uint64_t,double>> )>,
         std::function<bool (images_list_it_t, 
                  std::list<images_list_it_t>, 
                  std::list<std::reference_wrapper<planar_image_collection<uint64_t,double>>>,
                  std::list<std::reference_wrapper<contour_collection<double>>>, 
                  std::experimental::any )>,
         std::list<std::reference_wrapper<planar_image_collection<uint64_t,double>>>,
         std::list<std::reference_wrapper<contour_collection<double>>>,
         std::experimental::any );

    template bool planar_image_collection<float   ,double>::Process_Images(
         std::function<typename std::list<images_list_it_t> (images_list_it_t,
                  std::reference_wrapper<planar_image_collection<float   ,double>> )>,
         std::function<bool (images_list_it_t,
                  std::list<images_list_it_t>,
                  std::list<std::reference_wrapper<planar_image_collection<float   ,double>>>,
                  std::list<std::reference_wrapper<contour_collection<double>>>,
                  std::experimental::any )>,
         std::list<std::reference_wrapper<planar_image_collection<float   ,double>>>,
         std::list<std::reference_wrapper<contour_collection<double>>>,
         std::experimental::any );

#endif


//Generic routine for processing/combining groups of images into single images. Useful for spatial averaging, blurring, etc..
// Modified *this, so deep-copy beforehand if needed. See in-source default functors for descriptions/examples.
//
// This is a parallel version. The user needs to ensure there are no race conditions.
//
template <class T,class R>
bool planar_image_collection<T,R>::Process_Images_Parallel(
                             std::function<typename std::list<images_list_it_t> (images_list_it_t,
                                      std::reference_wrapper<planar_image_collection<T,R>> )>         image_grouper,
                             std::function<bool (images_list_it_t, 
                                      std::list<images_list_it_t>,
                                      std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                                      std::list<std::reference_wrapper<contour_collection<R>>>,
                                      std::experimental::any )>                                       operation_functor,      
                             std::list<std::reference_wrapper<planar_image_collection<T,R>>>          external_images,
                             std::list<std::reference_wrapper<contour_collection<R>>>                 contour_collections,
                             std::experimental::any                                                   user_data ){

    //It is much more tedious to provide a 'sane' default, so we require users to provide a valid image grouping
    // functor.
    if(!image_grouper) return false;

    //Generate a comprehensive list of iterators to all remaining images in the deep-copied array. This list will be
    // pruned after images have been successfully averaged or otherwise manipulated.
    auto to_be_avgd = this->get_all_images();

    std::list< std::pair< images_list_it_t, std::list<images_list_it_t> > > groupings;
    std::list< images_list_it_t > taken;

    const auto total_image_count = to_be_avgd.size();

    //Loop over the iterators in the comprehensive list. (The loop will have elements erased from it as you go.)
    while(!to_be_avgd.empty()){
        //Take the first iterator and find all other images which should be averaged with it.
        // Generally you will simply want spatial overlap, but you can do whatever you want with the metadata too.
        // Be selective at this stage because later we simply walk over images on a pixel-by-pixel basis.
        auto curr_img_it = to_be_avgd.front(); //A copy of the iterator, not a reference to it, is needed here.
        auto selected_imgs = image_grouper(curr_img_it, *this);

        //Remove any which are already claimed by an earlier grouping.
        for(const auto &ataken : taken) selected_imgs.remove(ataken);

        //Consistency check of user's grouping functor: verify curr_img_it is present in the selected images.
        {
          bool present = false;
          for(const auto &an_img_it : selected_imgs){
              if(an_img_it == curr_img_it){
                  present = true;
                  break;
              }
          }
          if(!present){
              FUNCWARN("Image grouping functor failed to select self image and thus failed basic consistency. "
                       "(Each image's group must be composed of at least the image.)");
              return false;
          }
        }

        //Verify that there are an identical number of rows, columns, and channels in each remaining image. Die if
        // this is not the case. Ignore differences in spatial layout at this stage; this detail should have been 
        // taken care of when selecting the spatially-overlapping images.
        {
          auto rows     = curr_img_it->rows;
          auto columns  = curr_img_it->columns;
          auto channels = curr_img_it->channels;

          for(const auto &an_img_it : selected_imgs){
              if( (rows     != an_img_it->rows)
              ||  (columns  != an_img_it->columns)
              ||  (channels != an_img_it->channels) ){
                  FUNCWARN("Images have differing number of rows, columns, or channels. Are you sure you've got the correct data?");
                  return false;
                  //This might not be an error if you know what you're doing. However, you will still potentially attempt to 
                  // access an image's pixels out-of-bounds without altering the following code. (Are you sure you want this?
                  // Wouldn't it be easier and more logically sound to try something like voxel resampling, or explicit zeroing
                  // of the jutting image portions beforehand instead?)
                  //
                  // If the answers to these questions are "no" then go ahead and remove the 'return false' but leave the 
                  // notice.
                  //

              }
          }
        }

        groupings.emplace_back( std::make_pair(curr_img_it, selected_imgs) );

        //Add the remaining image iterators to the taken pile.
        for(const auto &an_img_it : selected_imgs) taken.emplace_back(an_img_it);

        //Remove *all* of the overlapping image iterators from the to_be_avgd list.
        for(auto &an_img_it : selected_imgs) to_be_avgd.remove(an_img_it);
    }


    // --- Preparations now complete. Now iterate over the groupings, applying the user's operation to each group. ---


    //Functor for the main operation, where we will process a batch of images and produce a single output.
    //
    // For example, we might want to average the image group. The image pointed to by `first_img_it` will be
    // the only image retained -- the rest will be pruned right after this routine is called.
    //
    // NOTE: The image pointed to by first_img_it is also pointed to by one of the iterators in selected_img_its,
    //       so you'll need to be VERY careful about how you update the image data. To get around this, you should
    //       (1) create a DEEP COPY of first_img_it, (2) modify the copy as needed, and (3) DEEP COPY back to the 
    //       original first_img_it before returning.
    //
    // NOTE: Why not just always provide a deep copy, or working space for one? Because sometimes you can get around 
    //       the issue without unnecessary copying.
    //
    // NOTE: This routine should only return true if everything was OK.
    //
    auto default_op_func = [](typename planar_image_collection<T,R>::images_list_it_t first_img_it,
                              typename std::list<planar_image_collection<T,R>::images_list_it_t> selected_img_its,
                              typename std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                              typename std::list<std::reference_wrapper<contour_collection<R>>>,
                              typename std::experimental::any ) -> bool {

        //Default: do absolutely nothing.
        return true;

        //Alternative: average pixels.
        //for(rows) for(cols) for(chnls) { 
        //    for(auto &an_img_it : selected_img_its){
        //        pixel_val = an_img_it->value(row, col, chan);
        //        pixel_vals.push_back( static_cast<double>(pixel_val) );
        //    }
        //    pixel_mean = Stats::Mean(pixel_vals);
        //    first_img_it->reference(row, col, chan) = static_cast<T>(pixel_mean);
        //}
    
        //Alternative: tweak/insert/erase the metadata.
        //auto orig_md = first_img_it->metadata;
        //first_img_it->metadata["Operations performed"] += "something, "; 
    };
    auto the_op_func = (operation_functor) ? operation_functor : default_op_func;

    //Launch and wait on the tasks.
    size_t images_processed = 0;
    std::list<std::future<bool>> futures;
    bool eject = false;
    auto at_a_time = std::thread::hardware_concurrency();
    if(at_a_time == 0) at_a_time = 2;
    for(const auto &agroup : groupings){
        images_processed += agroup.second.size();
        futures.emplace_back( std::async( std::launch::async, the_op_func, agroup.first, agroup.second, external_images, contour_collections, user_data ) );
        if(futures.size() > at_a_time){
            for(auto &afuture : futures){
                auto res = afuture.get();
                if(!res) eject = true;
            }
            futures.clear();
            if(eject) return false;
        }
        FUNCINFO("Images still to be processed: " << (total_image_count - images_processed));
    }
    for(auto &afuture : futures){
        auto res = afuture.get();
        if(!res) eject = true;
    }
    futures.clear();
    if(eject) return false;


    //Now remove all images except the one designated the 'curr_img_it' from each grouping.
    for(const auto &agroup : groupings){
        const auto group_leader = agroup.first;
        for(const auto &group_member : agroup.second){
            if(group_member != group_leader){
                this->images.erase(group_member);
            }
        }
    }

    return true;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::Process_Images_Parallel( 
         std::function<typename std::list<images_list_it_t> (images_list_it_t, 
                  std::reference_wrapper<planar_image_collection<uint8_t ,double>> )>,
         std::function<bool (images_list_it_t, 
                  std::list<images_list_it_t>, 
                  std::list<std::reference_wrapper<planar_image_collection<uint8_t ,double>>>,
                  std::list<std::reference_wrapper<contour_collection<double>>>,
                  std::experimental::any )>,
         std::list<std::reference_wrapper<planar_image_collection<uint8_t ,double>>>,
         std::list<std::reference_wrapper<contour_collection<double>>>,
         std::experimental::any );

    template bool planar_image_collection<uint16_t,double>::Process_Images_Parallel(
         std::function<typename std::list<images_list_it_t> (images_list_it_t, 
                  std::reference_wrapper<planar_image_collection<uint16_t,double>> )>,
         std::function<bool (images_list_it_t, 
                  std::list<images_list_it_t>, 
                  std::list<std::reference_wrapper<planar_image_collection<uint16_t,double>>>,
                  std::list<std::reference_wrapper<contour_collection<double>>>, 
                  std::experimental::any )>, 
         std::list<std::reference_wrapper<planar_image_collection<uint16_t,double>>>,
         std::list<std::reference_wrapper<contour_collection<double>>>,
         std::experimental::any );

    template bool planar_image_collection<uint32_t,double>::Process_Images_Parallel(
         std::function<typename std::list<images_list_it_t> (images_list_it_t, 
                  std::reference_wrapper<planar_image_collection<uint32_t,double>> )>,
         std::function<bool (images_list_it_t, 
                  std::list<images_list_it_t>, 
                  std::list<std::reference_wrapper<planar_image_collection<uint32_t,double>>>,
                  std::list<std::reference_wrapper<contour_collection<double>>>, 
                  std::experimental::any )>, 
         std::list<std::reference_wrapper<planar_image_collection<uint32_t,double>>>,
         std::list<std::reference_wrapper<contour_collection<double>>>,
         std::experimental::any );

    template bool planar_image_collection<uint64_t,double>::Process_Images_Parallel(
         std::function<typename std::list<images_list_it_t> (images_list_it_t, 
                  std::reference_wrapper<planar_image_collection<uint64_t,double>> )>,
         std::function<bool (images_list_it_t, 
                  std::list<images_list_it_t>, 
                  std::list<std::reference_wrapper<planar_image_collection<uint64_t,double>>>,
                  std::list<std::reference_wrapper<contour_collection<double>>>, 
                  std::experimental::any )>,
         std::list<std::reference_wrapper<planar_image_collection<uint64_t,double>>>,
         std::list<std::reference_wrapper<contour_collection<double>>>,
         std::experimental::any );

    template bool planar_image_collection<float   ,double>::Process_Images_Parallel(
         std::function<typename std::list<images_list_it_t> (images_list_it_t,
                  std::reference_wrapper<planar_image_collection<float   ,double>> )>,
         std::function<bool (images_list_it_t,
                  std::list<images_list_it_t>,
                  std::list<std::reference_wrapper<planar_image_collection<float   ,double>>>,
                  std::list<std::reference_wrapper<contour_collection<double>>>,
                  std::experimental::any )>,
         std::list<std::reference_wrapper<planar_image_collection<float   ,double>>>,
         std::list<std::reference_wrapper<contour_collection<double>>>,
         std::experimental::any );

#endif



//Generic routine for performing an operation on images which may depend on external images (such as pixel maps).
// No images are deleted, though the user has individual access to all images. This routine is complementary
// to the Process_Images() routine, and has different use-cases and approaches.
template <class T,class R>
bool planar_image_collection<T,R>::Transform_Images( 
            std::function<bool (images_list_it_t,                              
                     std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                     std::list<std::reference_wrapper<contour_collection<R>>>,
                     std::experimental::any )>                                          op_func,
            std::list<std::reference_wrapper<planar_image_collection<T,R>>>             external_imgs,
            std::list<std::reference_wrapper<contour_collection<R>>>                    contour_collections,
            std::experimental::any                                                      user_data ){

    if(!op_func) return false;
    for(auto img_it = this->images.begin(); img_it != this->images.end(); ++img_it){
        if(!op_func(img_it, external_imgs, contour_collections, user_data)) return false;
    }
    return true;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::Transform_Images(
            std::function<bool (images_list_it_t,
                     std::list<std::reference_wrapper<planar_image_collection<uint8_t ,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>,
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<uint8_t ,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections,
            std::experimental::any                                                        user_data );

    template bool planar_image_collection<uint16_t,double>::Transform_Images(
            std::function<bool (images_list_it_t,
                     std::list<std::reference_wrapper<planar_image_collection<uint16_t,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>, 
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<uint16_t,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections, 
            std::experimental::any                                                        user_data );

    template bool planar_image_collection<uint32_t,double>::Transform_Images(
            std::function<bool (images_list_it_t,
                     std::list<std::reference_wrapper<planar_image_collection<uint32_t,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>, 
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<uint32_t,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections, 
            std::experimental::any                                                        user_data );

    template bool planar_image_collection<uint64_t,double>::Transform_Images(
            std::function<bool (images_list_it_t,
                     std::list<std::reference_wrapper<planar_image_collection<uint64_t,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>, 
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<uint64_t,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections, 
            std::experimental::any                                                        user_data );

    template bool planar_image_collection<float   ,double>::Transform_Images(
            std::function<bool (images_list_it_t,
                     std::list<std::reference_wrapper<planar_image_collection<float   ,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>,
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<float   ,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections,
            std::experimental::any                                                        user_data );

#endif

//Generic routine for performing an operation on images which may depend on external images (such as pixel maps).
// No images are deleted, though the user has individual access to all images. This routine is complementary
// to the Process_Images() routine, and has different use-cases and approaches.
template <class T,class R>
bool planar_image_collection<T,R>::Transform_Images_Parallel( 
            std::function<bool (images_list_it_t,                              
                     std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                     std::list<std::reference_wrapper<contour_collection<R>>>,
                     std::experimental::any )>                                          op_func,
            std::list<std::reference_wrapper<planar_image_collection<T,R>>>             external_imgs,
            std::list<std::reference_wrapper<contour_collection<R>>>                    contour_collections,
            std::experimental::any                                                      user_data ){

    if(!op_func) return false;

    //Launch and wait on the tasks.
    std::list<std::future<bool>> futures;
    bool eject = false;
    auto at_a_time = std::thread::hardware_concurrency();
    if(at_a_time == 0) at_a_time = 2;

    for(auto img_it = this->images.begin(); img_it != this->images.end(); ++img_it){
        futures.emplace_back( std::async( std::launch::async, op_func, img_it, external_imgs, contour_collections, user_data ) );
        if(futures.size() > at_a_time){

            for(auto &afuture : futures){
                auto res = afuture.get();
                if(!res) eject = true;
            }
            futures.clear();
            if(eject) return false;
        }
    }

    for(auto &afuture : futures){
        auto res = afuture.get();
        if(!res) eject = true;
    }
    futures.clear();
    if(eject) return false;

    return true;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::Transform_Images_Parallel(
            std::function<bool (images_list_it_t,
                     std::list<std::reference_wrapper<planar_image_collection<uint8_t ,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>,
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<uint8_t ,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections,
            std::experimental::any                                                        user_data );

    template bool planar_image_collection<uint16_t,double>::Transform_Images_Parallel(
            std::function<bool (images_list_it_t,
                     std::list<std::reference_wrapper<planar_image_collection<uint16_t,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>, 
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<uint16_t,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections, 
            std::experimental::any                                                        user_data );

    template bool planar_image_collection<uint32_t,double>::Transform_Images_Parallel(
            std::function<bool (images_list_it_t,
                     std::list<std::reference_wrapper<planar_image_collection<uint32_t,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>, 
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<uint32_t,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections, 
            std::experimental::any                                                        user_data );

    template bool planar_image_collection<uint64_t,double>::Transform_Images_Parallel(
            std::function<bool (images_list_it_t,
                     std::list<std::reference_wrapper<planar_image_collection<uint64_t,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>, 
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<uint64_t,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections, 
            std::experimental::any                                                        user_data );

    template bool planar_image_collection<float   ,double>::Transform_Images_Parallel(
            std::function<bool (images_list_it_t,
                     std::list<std::reference_wrapper<planar_image_collection<float   ,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>,
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<float   ,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections,
            std::experimental::any                                                        user_data );

#endif


//Generic routine for altering images as a whole, or computing histograms, time courses, or any sort of distribution 
// the entire set of images. No images are deleted, though the user has full read-write access to all images.
// This routine is very simple, and merely provides a consistent interface for doing operations on the image collection.
//
// Note: The functor gets called only once by this routine. Any attempt at parallelization should probably happen
//       within the user's functor.
//
template <class T,class R>
bool planar_image_collection<T,R>::Compute_Images( 
            std::function<bool ( planar_image_collection<T,R> &, 
                                 std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                                 std::list<std::reference_wrapper<contour_collection<R>>>,
                                 std::experimental::any )>                                    op_func,
            std::list<std::reference_wrapper<planar_image_collection<T,R>>>                   external_imgs,
            std::list<std::reference_wrapper<contour_collection<R>>>                          contour_collections,
            std::experimental::any                                                            user_data ){

    if(!op_func) return false;
    return op_func(std::ref(*this), external_imgs, contour_collections, user_data);
}

#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::Compute_Images(
            std::function<bool (planar_image_collection<uint8_t,double> &,
                     std::list<std::reference_wrapper<planar_image_collection<uint8_t ,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>,
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<uint8_t ,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections,
            std::experimental::any                                                        user_data );

    template bool planar_image_collection<uint16_t,double>::Compute_Images(
            std::function<bool (planar_image_collection<uint16_t,double> &,
                     std::list<std::reference_wrapper<planar_image_collection<uint16_t,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>, 
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<uint16_t,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections, 
            std::experimental::any                                                        user_data );

    template bool planar_image_collection<uint32_t,double>::Compute_Images(
            std::function<bool (planar_image_collection<uint32_t,double> &,
                     std::list<std::reference_wrapper<planar_image_collection<uint32_t,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>, 
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<uint32_t,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections, 
            std::experimental::any                                                        user_data );

    template bool planar_image_collection<uint64_t,double>::Compute_Images(
            std::function<bool (planar_image_collection<uint64_t,double> &,
                     std::list<std::reference_wrapper<planar_image_collection<uint64_t,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>, 
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<uint64_t,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections, 
            std::experimental::any                                                        user_data );

    template bool planar_image_collection<float   ,double>::Compute_Images(
            std::function<bool (planar_image_collection<float,double> &,
                     std::list<std::reference_wrapper<planar_image_collection<float   ,double>>>,
                     std::list<std::reference_wrapper<contour_collection<double>>>,
                     std::experimental::any )>                                            op_func,
            std::list<std::reference_wrapper<planar_image_collection<float   ,double>>>   external_imgs,
            std::list<std::reference_wrapper<contour_collection<double>>>                 contour_collections,
            std::experimental::any                                                        user_data );

#endif




//Condense groups of images to a single image by averaging at the pixel level, channel by channel. Only the final,
// averaged image remains. 
//
// NOTE: The default image_grouper functor groups ALL images together. Specify something more specific if needed.
//
// NOTE: Returns false is an error was encountered. The most common errors are likely to arise from differing numbers
//       of rows, columns, and channels in images.
//
template <class T,class R>
bool planar_image_collection<T,R>::Condense_Average_Images(
              std::function<typename std::list<images_list_it_t>
                 (images_list_it_t, std::reference_wrapper<planar_image_collection<T,R>>)> image_grouper ){

    //Default in case nothing was provided.
    auto default_image_grouper = [](planar_image_collection<T,R>::images_list_it_t first_img_it, 
                                    std::reference_wrapper<planar_image_collection<T,R>> pic) -> auto {
        //Default: group all images together.
        return pic.get().get_all_images();

        //Alternative: groups with only one member: the first image.
        //return std::list< planar_image_collection<T,R>::images_list_it_t >({first_img_it});

        //Alternative: groups of images which spatially overlap.
        //const auto img_cntr = first_img_it->center();
        //const auto ortho = first_img_it->row_unit.Cross( first_img_it->col_unit ).unit();
        //const std::list<vec3<R>> points = { img_cntr, img_cntr + ortho * first_img_it->pxl_dz * (R)(0.25),
        //                                              img_cntr - ortho * first_img_it->pxl_dz * (R)(0.25) };
        //return pic.get().get_images_which_encompass_all_points(points);
    };

    //Functor for pixel-level, channel-separate averaging.
    auto op_func = [](typename planar_image_collection<T,R>::images_list_it_t first_img_it,
                      typename std::list<planar_image_collection<T,R>::images_list_it_t> selected_img_its,
                      typename std::list<std::reference_wrapper<planar_image_collection<T,R>>>,
                      typename std::list<std::reference_wrapper<contour_collection<R>>>,
                      typename std::experimental::any ) -> bool {
        //Loop over the rows, columns, and channels.
        for(auto row = 0; row < first_img_it->rows; ++row){
            for(auto col = 0; col < first_img_it->columns; ++col){ 
                for(auto chan = 0; chan < first_img_it->channels; ++chan){

                    //Loop over the list of images, collecting the pixel values for each image.
                    // This is also where you could grab nearest-neighbour pixels for averaging, or whatever you want.
                    std::vector<double> pixel_vals;
                    pixel_vals.reserve(selected_img_its.size());
                    for(auto &an_img_it : selected_img_its){
                        const auto pixel_val = an_img_it->value(row, col, chan);
                        pixel_vals.push_back( static_cast<double>(pixel_val) );
                    }

                    //Compute the pixel average, or whatever you want to compute with the pixels.
                    const double pixel_mean = Stats::Mean(pixel_vals);

                    //Update the first image (so as to maintain order as easily as possible) pixel values.
                    // REMEMBER that the first_img_it image is also one of the images in selected_img_its!
                    // BE CAREFUL that altering it will not alter future loops!
                    first_img_it->reference(row, col, chan) = static_cast<T>(pixel_mean);
                }
            }
        }
     
        //Alter the first image's metadata to reflect that averaging has occurred. You might want to consider
        // a selective whitelist approach so that unique IDs are not duplicated accidentally.
        //first_img_it->metadata.clear();
        first_img_it->metadata["Operations Performed"] += "Averaged with "_s + Xtostring(selected_img_its.size()) + " images;";
        first_img_it->metadata["Description"] += " Averaged";
        return true;
    };

    if(image_grouper) return this->Process_Images_Parallel(image_grouper, op_func, {}, {});
    return this->Process_Images_Parallel(default_image_grouper, op_func, {}, {});
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::Condense_Average_Images( 
         std::function<typename std::list<images_list_it_t> (images_list_it_t, std::reference_wrapper<planar_image_collection<uint8_t ,double>>)>);

    template bool planar_image_collection<uint16_t,double>::Condense_Average_Images( 
         std::function<typename std::list<images_list_it_t> (images_list_it_t, std::reference_wrapper<planar_image_collection<uint16_t,double>>)>);

    template bool planar_image_collection<uint32_t,double>::Condense_Average_Images( 
         std::function<typename std::list<images_list_it_t> (images_list_it_t, std::reference_wrapper<planar_image_collection<uint32_t,double>>)>);

    template bool planar_image_collection<uint64_t,double>::Condense_Average_Images( 
         std::function<typename std::list<images_list_it_t> (images_list_it_t, std::reference_wrapper<planar_image_collection<uint64_t,double>>)>);

    template bool planar_image_collection<float   ,double>::Condense_Average_Images( 
         std::function<typename std::list<images_list_it_t> (images_list_it_t, std::reference_wrapper<planar_image_collection<float   ,double>>)>);
#endif


//Blur pixels isotropically, independently in their image plane, completely ignoring pixel shape and real-space coordinates.
// Leave chnls empty for all channels. For more selectivity, run on each image separately.
//
// NOTE: If a failure is encountered and false is returned, some images may be blurred while others are not. There is no way
//       to report which at the moment. If this happens, you should consider running the planar_image blur function directly
//       on images.
//
template <class T,class R> bool planar_image_collection<T,R>::Gaussian_Pixel_Blur(std::set<long int> chnls, double sigma_in_units_of_pixels){
    for(auto &animg : this->images){
        if(!animg.Gaussian_Pixel_Blur(chnls,sigma_in_units_of_pixels)) return false;
    }
    return true;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::Gaussian_Pixel_Blur(std::set<long int> chnls, double sigma_in_units_of_pixels);
    template bool planar_image_collection<uint16_t,double>::Gaussian_Pixel_Blur(std::set<long int> chnls, double sigma_in_units_of_pixels);
    template bool planar_image_collection<uint32_t,double>::Gaussian_Pixel_Blur(std::set<long int> chnls, double sigma_in_units_of_pixels);
    template bool planar_image_collection<uint64_t,double>::Gaussian_Pixel_Blur(std::set<long int> chnls, double sigma_in_units_of_pixels);
    template bool planar_image_collection<float   ,double>::Gaussian_Pixel_Blur(std::set<long int> chnls, double sigma_in_units_of_pixels);
#endif

//Fill pixels above a given plane. Returns the number of affected pixels.
template <class T,class R> 
long int
planar_image_collection<T,R>::set_voxels_above_plane(const plane<R> &aplane, T val, std::set<long int> chnls){
    long int N = 0;
    for(auto &animg : this->images){
        N += animg.set_voxels_above_plane(aplane, val, chnls);
    }
    return N;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template long int planar_image_collection<uint8_t ,double>::set_voxels_above_plane(const plane<double> &aplane, uint8_t  val, std::set<long int>);
    template long int planar_image_collection<uint16_t,double>::set_voxels_above_plane(const plane<double> &aplane, uint16_t val, std::set<long int>);
    template long int planar_image_collection<uint32_t,double>::set_voxels_above_plane(const plane<double> &aplane, uint32_t val, std::set<long int>);
    template long int planar_image_collection<uint64_t,double>::set_voxels_above_plane(const plane<double> &aplane, uint64_t val, std::set<long int>);
    template long int planar_image_collection<float   ,double>::set_voxels_above_plane(const plane<double> &aplane, float    val, std::set<long int>);
#endif

//Apply a functor to individual pixels.
template<class T, class R>
void
planar_image_collection<T,R>::apply_to_pixels( std::function<void(long int row, long int col, long int chnl, T &val)> func){
    for(auto &animg : this->images){
        animg.apply_to_pixels(func);
    }
    return;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template void planar_image_collection<uint8_t ,double>::apply_to_pixels(std::function<void(long int row, long int col, long int chnl, uint8_t  &val)>);
    template void planar_image_collection<uint16_t,double>::apply_to_pixels(std::function<void(long int row, long int col, long int chnl, uint16_t &val)>);
    template void planar_image_collection<uint32_t,double>::apply_to_pixels(std::function<void(long int row, long int col, long int chnl, uint32_t &val)>);
    template void planar_image_collection<uint64_t,double>::apply_to_pixels(std::function<void(long int row, long int col, long int chnl, uint64_t &val)>);
    template void planar_image_collection<float   ,double>::apply_to_pixels(std::function<void(long int row, long int col, long int chnl, float    &val)>);
#endif

//Returns the R^3 center of the image. Nothing fancy.
template <class T,class R> vec3<R> planar_image_collection<T,R>::center(void) const {
    if(this->images.empty()) FUNCERR("Unable to compute center-point. This collection contains no images");
    vec3<R> out;
    for(auto i_it = this->images.begin(); i_it != this->images.end(); ++i_it){
        out += i_it->center();
    }
    out /= static_cast<double>(this->images.size());
    return out;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template vec3<double> planar_image_collection<uint8_t ,double>::center(void) const;
    template vec3<double> planar_image_collection<uint16_t,double>::center(void) const;
    template vec3<double> planar_image_collection<uint32_t,double>::center(void) const;
    template vec3<double> planar_image_collection<uint64_t,double>::center(void) const;
    template vec3<double> planar_image_collection<float   ,double>::center(void) const;
#endif


//Compare the geometrical (non-pixel/voxel) aspects of the images to one another.
//
//Returns a false if the image collections appear geometrically different. This is basically an operator==
// but does *not* examine the pixel/voxel values.
template <class T,class R> bool planar_image_collection<T,R>::Spatially_eq(const planar_image_collection<T,R> &in) const {
    if(this->images.size() != in.images.size()) return false;

    //Generate a unique pointer to each image in either collection.
    std::list<decltype(in.images.begin())> rhs; 
    for(auto it = in.images.begin(); it != in.images.end(); ++it) rhs.push_back(it);

    //Walk through the elements, eliminating those in rhs which are found to spatially match one in lhs.
    for(const auto &img1 : this->images){
        auto img_it2 = rhs.begin();
        while((img_it2 != rhs.end()) && !rhs.empty()){
            if(img1.Spatially_eq(**img_it2)){
                rhs.erase(img_it2);
                break;
            }
        }
    }
  
    //Iff all elements in rhs have been accounted for, then the match is OK.
    return rhs.empty();
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::Spatially_eq(const planar_image_collection<uint8_t ,double> &in) const;
    template bool planar_image_collection<uint16_t,double>::Spatially_eq(const planar_image_collection<uint16_t,double> &in) const;
    template bool planar_image_collection<uint32_t,double>::Spatially_eq(const planar_image_collection<uint32_t,double> &in) const;
    template bool planar_image_collection<uint64_t,double>::Spatially_eq(const planar_image_collection<uint64_t,double> &in) const;
    template bool planar_image_collection<float   ,double>::Spatially_eq(const planar_image_collection<float   ,double> &in) const;
#endif

//Collates images together, taking ownership of input image data on success. Behaviour can be specified.
//
//Note: This routine does not *combine* image data -- just collections of images. The pixels are not combined.
//      The new image is simply appended to the end of the list of planar_images.
//
//Note: This routine 'eats' the input image collection, taking ownership of the image data without making 
//      copies, even if collation ultimately fails. If you need copies, make them manually and explicitly in 
//      the calling code.
//
//Note: If this routine cannot collate the input, both (*this) and (in) could be in an inconsistent state.
//      Some images may have transferred while others did not. If this is not suitable, make copies to do
//      a manual ("poor-man's") transactional rollback in the client side.
//
//Note: Should always work if either image collection is empty, because there is no conflict possible.
//
template <class T,class R> bool planar_image_collection<T,R>::Collate_Images(planar_image_collection<T,R> &in, bool GeometricalOverlapOK){
    //Loop over the input images, bailing at the first merge difficulty.
    while(!in.images.empty()){
        auto img_it = in.images.begin();
   
        //If needed, check for geometrical overlap.
        if(!GeometricalOverlapOK){
            //We do a less rigorous approach instead of a true volume overlap procedure.
            // Ideally, something like Dice coefficient with a suitable threshold score would be better.
            for(const auto &thisimg : this->images) if(img_it->encloses_2D_planar_image(thisimg)) return false;
        }

        //If we get here, perform the merge by simply appending the image to this' collection.
        this->images.splice(this->images.end(),in.images,img_it);
    }
    return true;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::Collate_Images(planar_image_collection<uint8_t ,double> &in, bool GeometricalOverlapOK);
    template bool planar_image_collection<uint16_t,double>::Collate_Images(planar_image_collection<uint16_t,double> &in, bool GeometricalOverlapOK);
    template bool planar_image_collection<uint32_t,double>::Collate_Images(planar_image_collection<uint32_t,double> &in, bool GeometricalOverlapOK);
    template bool planar_image_collection<uint64_t,double>::Collate_Images(planar_image_collection<uint64_t,double> &in, bool GeometricalOverlapOK);
    template bool planar_image_collection<float   ,double>::Collate_Images(planar_image_collection<float   ,double> &in, bool GeometricalOverlapOK);
#endif


//Interpolate linearly in R^3. The nearest two images (above and below) are interpolated between. Specifically, the
// position of interest is projected orthographically onto the image and the relative distance to each plane is used to
// determine weighting. If the projected point is not within the bounds of (either of) the nearest two images, a special
// value is returned. 
//
// This routine can deal with gaps and images with differing numbers of rows and/or columns. It can also deal with
// non-coplanar images, but beware that the interpolation can get sort of 'wonky' if adjacent images are highly oblique.
//
// Note: this routine is fairly slow. It is designed for random access, not uniformly (re-)interpolating a whole
// image; in the latter case a dedicated routine should be able to cache or precompute some things to reduce redundant
// computations.
//
// Note: this routine requires all images to have a uniformly (or at least coherently) specified normal. In other words
// the planes must have a consistent notion of which side is 'positive' space and which is 'negative.'
//
// Note: this routine will get confused by images that (exactly) spatially overlap. In this case there is 'no room' for
// the interpolation to occur, and numerical difficulties may ensue.
//
// Note: this routine will round-trip pixel values (type T) to the spatial type (type R), scaling them, and then back
// (to type T). This is necessary since the interpolation uses distance as the weighting factor. For best results,
// ensure that the spatial type (R) is wider than the pixel type (T), or at least ensure that the types are wide enough
// that the loss of precision is irrelevant. (If you cannot deal with this, spatial interpolation is probably not what
// you want!)
//
template <class T,class R> T planar_image_collection<T,R>::trilinearly_interpolate(const vec3<R> &pos, long int chnl, R out_of_bounds){
    if(this->images.empty()) throw std::runtime_error("Cannot interpolate in R^3; there are no images.");

    //First, identify the nearest planes above and below the point.
    std::list< std::pair< R, std::reference_wrapper<planar_image<T,R>> > > planes_above;
    std::list< std::pair< R, std::reference_wrapper<planar_image<T,R>> > > planes_below;
    for(auto & animg : this->images){
        const auto theplane = animg.image_plane();
        const auto signed_dist = theplane.Get_Signed_Distance_To_Point(pos);
        const auto is_above = (signed_dist >= static_cast<R>(0));

        if(is_above){
            planes_above.push_back(std::make_pair<R,std::reference_wrapper<planar_image<T,R>>>(std::abs(signed_dist),
            std::ref(animg)));
        }else{
            planes_below.push_back(std::make_pair<R,std::reference_wrapper<planar_image<T,R>>>(std::abs(signed_dist),
            std::ref(animg)));
        }
    }

    //Sort both lists using the distance magnitude.
    const auto sort_less = [](const std::pair<R,std::reference_wrapper<planar_image<T,R>>> &A, 
                              const std::pair<R,std::reference_wrapper<planar_image<T,R>>> &B) -> bool {
        return A.first < B.first;
    };
    planes_above.sort(sort_less);
    planes_below.sort(sort_less);

    T out = out_of_bounds;

    if(false){
    }else if( !planes_above.empty() && !planes_below.empty()){
        //Interpolate each image in-plane and then interpolate the in-plane values.
        // This path does not need to explictly check if the point is within either image.
        const auto A_dist = planes_above.front().first;
        const auto B_dist = planes_below.front().first;
        const auto tot_dist = A_dist + B_dist;

        const auto A_plane = planes_above.front().second.get().image_plane();
        const auto B_plane = planes_below.front().second.get().image_plane();

        const auto A_P = A_plane.Project_Onto_Plane_Orthogonally(pos);
        const auto B_P = B_plane.Project_Onto_Plane_Orthogonally(pos);

        try{
            const auto A_frc = planes_above.front().second.get().fractional_row_column(A_P);
            const auto B_frc = planes_below.front().second.get().fractional_row_column(B_P);

            const auto A_out = planes_above.front().second.get().bilinearly_interpolate_in_pixel_number_space(A_frc.first, A_frc.second, chnl);
            const auto B_out = planes_below.front().second.get().bilinearly_interpolate_in_pixel_number_space(B_frc.first, B_frc.second, chnl);

            out = static_cast<T>( (B_dist/tot_dist)*static_cast<R>(A_out)
                                + (A_dist/tot_dist)*static_cast<R>(B_out) );
        }catch(const std::exception &){
            out = out_of_bounds;
        }

    }else if( !planes_above.empty() &&  planes_below.empty()){
        //No interpolation necessary, but the point may be outside the image collection. 
        try{
            out = planes_above.front().second.get().value(pos,chnl); //Will throw if outside image bounds.
        }catch(const std::exception &){
            out = out_of_bounds;
        }

    }else if(  planes_above.empty() && !planes_below.empty()){
        //No interpolation necessary, but the point may be outside the image collection. 
        try{
            out = planes_below.front().second.get().value(pos,chnl); //Will throw if outside image bounds.
        }catch(const std::exception &){
            out = out_of_bounds;
        }

    }else{
        throw std::logic_error("No planes to interpolate; precondition violated.");
    }

    return out; 
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template uint8_t  planar_image_collection<uint8_t ,double>::trilinearly_interpolate(const vec3<double> &pos, long int chnl, double oob);
    template uint16_t planar_image_collection<uint16_t,double>::trilinearly_interpolate(const vec3<double> &pos, long int chnl, double oob);
    template uint32_t planar_image_collection<uint32_t,double>::trilinearly_interpolate(const vec3<double> &pos, long int chnl, double oob);
    template uint64_t planar_image_collection<uint64_t,double>::trilinearly_interpolate(const vec3<double> &pos, long int chnl, double oob);
    template float    planar_image_collection<float   ,double>::trilinearly_interpolate(const vec3<double> &pos, long int chnl, double oob);
#endif


//------------------------------------------------------------------------------------------------------------


//Produce an image by cutting through the image collection and copying intersecting pixels.
// No interpolation is performed -- pixels that spatially overlap are simply copied.
//
// NOTE: The return value is the number of pixels that were successfully copied. Zero means here was no 
//       (pixel-wise) overlap.
//
// NOTE: If there are several images in the collection that spatially overlap a pixel in the input image,
//       only one of the pixels will be copied. No averaging or interpolation is performed. Furthermore,
//       which pixel is chosen is left unspecified.
//
// NOTE: Due to the numerical nature of the way we specify images, you may get banding or missed pixels
//       if the input image orientation or offset is skewed compared to the image collection images.
//       This could be ameliorated using, e.g., nearest-pixel sampling or interpolation schemes, but 
//       is not done in this routine.
//
// NOTE: Pixels in the input image which do not overlap any pixels in the image collection are not
//       altered. Therefore you will probably want to fill the pixels with zeros or NaNs.
//
// NOTE: This routine works on a per-channel basis. The input image must have an equal or lesser number
//       of channels than all images in the collection.
//
template <class T,class R> 
long int Intersection_Copy(planar_image<T,R> &in, 
                           const std::list<typename planar_image_collection<T,R>::images_list_it_t> &imgs){
    long int count = 0;
    for(auto row = 0; row < in.rows; ++row){
        for(auto col = 0; col < in.columns; ++col){
            //Find the first image that coincides with this pixel (if any exists).
            const auto pos = in.position(row,col);
            for(const auto &img_it : imgs){
                if(img_it->index(pos,0) > 0){
                    //Cycle through the channels, copying the coinciding pixel.
                    for(auto chnl = 0; chnl < in.channels; ++chnl){
                        const auto indx = img_it->index(pos,chnl);
                        in.reference(row, col, chnl) = img_it->value(indx);
                        ++count;
                    }
                    break;
                }
            }
        }
    }
  
    if(count != 0){
        planar_image_collection<T,R> dummy;
        in.metadata = dummy.get_common_metadata(imgs);
    }
    return count;
}
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template long int Intersection_Copy(planar_image<uint8_t ,double> &,
             const std::list<typename planar_image_collection<uint8_t ,double>::images_list_it_t> &);
    template long int Intersection_Copy(planar_image<uint16_t,double> &, 
             const std::list<typename planar_image_collection<uint16_t,double>::images_list_it_t> &);
    template long int Intersection_Copy(planar_image<uint32_t,double> &, 
             const std::list<typename planar_image_collection<uint32_t,double>::images_list_it_t> &);
    template long int Intersection_Copy(planar_image<uint64_t,double> &, 
             const std::list<typename planar_image_collection<uint64_t,double>::images_list_it_t> &);
    template long int Intersection_Copy(planar_image<float   ,double> &, 
             const std::list<typename planar_image_collection<float   ,double>::images_list_it_t> &);
#endif


//Produce an image collection that contiguously covers the volume containing the provided contours.
//
// NOTE: Grid will be aligned with provided vectors. A Gram-Schmidt orthogonalization routine is used to ensure they are
//       orthogonal, but at least partially orthogonal seed vectors are needed -- z is not modified.
//
// NOTE: This routine automatically works out voxel dimensions to accomodate the provided numbers. In order to satisfy
//       the parameters, voxel dimensions may become elongated along one or more dimensions.
//
// NOTE: This routine bounds on the vertices, but does NOT make any attempt to compute a margin so that the contour will
//       sit on the plane of any image slice. You will have to work out the separation needed for the top and bottom
//       contours/slices and incorporate it into your z_margin!
//
//       (Why is this not done automatically? First, it is possible that the original images that define the contours
//       are not evenly spaced. This routine would have to be able to figure that out, and it isn't always possible or
//       easy. Second, this routine can be used for many purposes. Sometimes you want to generate a grid over some
//       ROI(s) without any margin at all. Trying to be clever and automatically sticking in margins would be annoying
//       and surprising.)
//
// NOTE: The margins get added to both sides along GridX, GridY, and GridZ. So the TOTAL margin along each vector will
//       be 2*margin (1*margin on each side).
// 
// NOTE: This routine cannot be used to produce images that are centred along a line; there is a complementary routine
//       that provides this below.
//
template <class T,class R>
planar_image_collection<T,R> 
Contiguously_Grid_Volume(const std::list<std::reference_wrapper<contour_collection<R>>> &ccs,
                         const R x_margin,    // Extra space (gap) length to leave on the perhiphery of the contours.
                         const R y_margin,    // Extra space (gap) length to leave on the perhiphery of the contours.
                         const R z_margin,    // Extra space (gap) length to leave on the perhiphery of the contours.
                         const long int number_of_rows,
                         const long int number_of_columns,
                         const long int number_of_channels,
                         const long int number_of_images,
                         const vec3<R> &x_orientation,
                         const vec3<R> &y_orientation,
                         const vec3<R> &z_orientation,
                         const R pixel_fill,
                         bool only_top_and_bottom){ //Only create the top and bottom (i.e., extremal) images.

    if(ccs.empty()){
        throw std::invalid_argument("No contours provided. Cannot continue.");
    }

    //The z-vector is designated as the primary alignment vector, so we orthogonalize the other vectors around it.
    vec3<R> GridX = x_orientation;
    vec3<R> GridY = y_orientation;
    vec3<R> GridZ = z_orientation;
 
    if(!GridZ.GramSchmidt_orthogonalize(GridX, GridY)){
        throw std::runtime_error("Unable to find grid orientation vectors.");
    }
    GridX = GridX.unit();
    GridY = GridY.unit();
    GridZ = GridZ.unit();

    //Find an appropriately aligned bounding box encompassing the ROI surface.
    R grid_x_min = std::numeric_limits<R>::quiet_NaN();
    R grid_x_max = std::numeric_limits<R>::quiet_NaN();
    R grid_y_min = std::numeric_limits<R>::quiet_NaN();
    R grid_y_max = std::numeric_limits<R>::quiet_NaN();
    R grid_z_min = std::numeric_limits<R>::quiet_NaN();
    R grid_z_max = std::numeric_limits<R>::quiet_NaN();

    //Make three planes defined by the orientation normals. (They intersect the origin to simplify computing offsets.)
    const vec3<R> zero(0.0, 0.0, 0.0);
    const plane<R> GridXZeroPlane(GridX, zero);
    const plane<R> GridYZeroPlane(GridY, zero);
    const plane<R> GridZZeroPlane(GridZ, zero);

    //Bound the vertices on the ROI.
    for(const auto &cc_ref : ccs){
        for(const auto &cop : cc_ref.get().contours){
            for(const auto &v : cop.points){
                //Compute the distance to each plane.
                const auto distX = GridXZeroPlane.Get_Signed_Distance_To_Point(v);
                const auto distY = GridYZeroPlane.Get_Signed_Distance_To_Point(v);
                const auto distZ = GridZZeroPlane.Get_Signed_Distance_To_Point(v);

                //Score the minimum and maximum distances.
                if(false){
                }else if(!std::isfinite(grid_x_min) || (distX < grid_x_min)){  grid_x_min = distX;
                }else if(!std::isfinite(grid_x_max) || (distX > grid_x_max)){  grid_x_max = distX;
                }else if(!std::isfinite(grid_y_min) || (distY < grid_y_min)){  grid_y_min = distY;
                }else if(!std::isfinite(grid_y_max) || (distY > grid_y_max)){  grid_y_max = distY;
                }else if(!std::isfinite(grid_z_min) || (distZ < grid_z_min)){  grid_z_min = distZ;
                }else if(!std::isfinite(grid_z_max) || (distZ > grid_z_max)){  grid_z_max = distZ;
                }
            }
        }
    }

    //Add margins.
    grid_x_min -= x_margin;
    grid_x_max += x_margin;
    grid_y_min -= y_margin;
    grid_y_max += y_margin;
    grid_z_min -= z_margin;
    grid_z_max += z_margin;

    //Create images that live on each Z-plane.
    const R xwidth = grid_x_max - grid_x_min;
    const R ywidth = grid_y_max - grid_y_min;
    const R zwidth = grid_z_max - grid_z_min;
    const auto voxel_dx = xwidth / static_cast<R>(number_of_columns);
    const auto voxel_dy = ywidth / static_cast<R>(number_of_rows);
    const auto voxel_dz = zwidth / static_cast<R>(number_of_images);

    //Find a 'corner' point which defines the location of the center of the (0,0)th voxel. (This point
    // ignores z-direction and will be projected onto an appropriate z-plane later.)
    const vec3<R> near_corner_zero = zero + (GridX * grid_x_min) + (GridY * grid_y_min)
                                          + (GridX * voxel_dx * static_cast<R>(0.5)) 
                                          + (GridY * voxel_dy * static_cast<R>(0.5));

    //Generate the images.
    planar_image_collection<T,R> out;
    for(long int i = 0; i < number_of_images; ++i){
        if(only_top_and_bottom && (i > 0) && (i < (number_of_images-1))) continue;

        //Construct a plane where the image will sit.
        const plane<R> img_plane( GridZ, zero + (GridZ * grid_z_min)                        //Move to the boundary of the ROI(s) volume.        
                                              + (GridZ * voxel_dz * static_cast<R>(i))      //Move to the volume reserved for this image.
                                              + (GridZ * voxel_dz * static_cast<R>(0.5)) ); //Centre of the volume.

        //Project the generic corner point coordinates onto the image's plane.
        const auto img_offset = img_plane.Project_Onto_Plane_Orthogonally(near_corner_zero);

        //Create and initialize the image.
        out.images.emplace_back();
        out.images.back().init_buffer(number_of_rows, number_of_columns, number_of_channels);
        out.images.back().init_spatial(voxel_dx, voxel_dy, voxel_dz, zero, img_offset);
        out.images.back().init_orientation(GridX, GridY);
        out.images.back().fill_pixels(pixel_fill);
    }

    return out;
}                         
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template planar_image_collection<uint8_t ,double> Contiguously_Grid_Volume(
             const std::list<std::reference_wrapper<contour_collection<double>>> &,
             const double, const double, const double, const long int, const long int, const long int, const long int,
             const vec3<double> &, const vec3<double> &, const vec3<double> &, const double, bool);
    template planar_image_collection<uint16_t,double> Contiguously_Grid_Volume(
             const std::list<std::reference_wrapper<contour_collection<double>>> &,
             const double, const double, const double, const long int, const long int, const long int, const long int,
             const vec3<double> &, const vec3<double> &, const vec3<double> &, const double, bool);
    template planar_image_collection<uint32_t,double> Contiguously_Grid_Volume(
             const std::list<std::reference_wrapper<contour_collection<double>>> &,
             const double, const double, const double, const long int, const long int, const long int, const long int,
             const vec3<double> &, const vec3<double> &, const vec3<double> &, const double, bool);
    template planar_image_collection<uint64_t,double> Contiguously_Grid_Volume(
             const std::list<std::reference_wrapper<contour_collection<double>>> &,
             const double, const double, const double, const long int, const long int, const long int, const long int,
             const vec3<double> &, const vec3<double> &, const vec3<double> &, const double, bool);
    template planar_image_collection<float   ,double> Contiguously_Grid_Volume(
             const std::list<std::reference_wrapper<contour_collection<double>>> &,
             const double, const double, const double, const long int, const long int, const long int, const long int,
             const vec3<double> &, const vec3<double> &, const vec3<double> &, const double, bool);
#endif


//This routine is a modification of Contiguously_Grid_Volume() that produces images that are centred on the provided
// line. See the comments for Contiguously_Grid_Volume() for more info. This routine should be preferred over
// Contiguously_Grid_Volume() when image centres have to coincide with a given line. 
//
// NOTE: Both Contiguously_Grid_Volume() and this routine align images along some unit vectors (or a unit vector along a
//       provided line). This routine *also* makes image centres coincide with a line. Note that this can cause large
//       margins (when the specified line is near the edge or outside the geometry of interest).
// 
template <class T,class R>
planar_image_collection<T,R> 
Symmetrically_Contiguously_Grid_Volume(const std::list<std::reference_wrapper<contour_collection<R>>> &ccs,
                         const R x_margin,    // Extra space (gap) length to leave on the perhiphery of the contours.
                         const R y_margin,    // Extra space (gap) length to leave on the perhiphery of the contours.
                         const R z_margin,    // Extra space (gap) length to leave on the perhiphery of the contours.
                         const long int number_of_rows,
                         const long int number_of_columns,
                         const long int number_of_channels,
                         const long int number_of_images,
                         const line<R> &symm_line, // Note: ~ the z_orientation with a pinning intersection somewhere.
                         const vec3<R> &x_orientation,
                         const vec3<R> &y_orientation,
                         const R pixel_fill,
                         bool only_top_and_bottom){ //Only create the top and bottom (i.e., extremal) images.

    if(ccs.empty()){
        throw std::invalid_argument("No contours provided. Cannot continue.");
    }

    //The symmetry line ("z-vector") is designated as the primary alignment vector, so we orthogonalize the other vectors around it.
    vec3<R> GridX = x_orientation;
    vec3<R> GridY = y_orientation;
    vec3<R> GridZ = symm_line.U_0;
 
    if(!GridZ.GramSchmidt_orthogonalize(GridX, GridY)){
        throw std::runtime_error("Unable to find grid orientation vectors.");
    }
    GridX = GridX.unit();
    GridY = GridY.unit();
    GridZ = GridZ.unit();

    //Find an appropriately aligned bounding box encompassing the ROI surface.
    R grid_x_ext = std::numeric_limits<R>::quiet_NaN(); //Extreme = sup( |min|, |max| ).
    R grid_y_ext = std::numeric_limits<R>::quiet_NaN(); //Extreme = sup( |min|, |max| ).
    R grid_z_min = std::numeric_limits<R>::quiet_NaN();
    R grid_z_max = std::numeric_limits<R>::quiet_NaN();

    //Make three planes defined by the orientation normals. (They intersect the origin to simplify computing offsets.)
    const vec3<R> zero(0.0, 0.0, 0.0);
    const plane<R> GridZZeroPlane(GridZ, zero);

    //Bound the vertices on the ROI.
    for(const auto &cc_ref : ccs){
        for(const auto &cop : cc_ref.get().contours){
            for(const auto &v : cop.points){
                //Compute the distance to each plane.
                const auto v_on_line = symm_line.Project_Point_Orthogonally(v);
                const auto distX = std::abs( (v - v_on_line).Dot(GridX) );
                const auto distY = std::abs( (v - v_on_line).Dot(GridY) );
                const auto distZ = GridZZeroPlane.Get_Signed_Distance_To_Point(v);

                //Score the minimum and maximum distances.
                if(false){
                }else if(!std::isfinite(grid_x_ext) || (distX > grid_x_ext)){  grid_x_ext = distX;
                }else if(!std::isfinite(grid_y_ext) || (distY > grid_y_ext)){  grid_y_ext = distY;
                }else if(!std::isfinite(grid_z_min) || (distZ < grid_z_min)){  grid_z_min = distZ;
                }else if(!std::isfinite(grid_z_max) || (distZ > grid_z_max)){  grid_z_max = distZ;
                }
            }
        }
    }

    //Add margins.
    grid_x_ext += x_margin;
    grid_y_ext += y_margin;
    grid_z_min -= z_margin;
    grid_z_max += z_margin;

    //Create images that live on each Z-plane.
    const R xwidth = grid_x_ext * static_cast<R>(2);
    const R ywidth = grid_y_ext * static_cast<R>(2);
    const R zwidth = grid_z_max - grid_z_min;
    const auto voxel_dx = xwidth / static_cast<R>(number_of_columns);
    const auto voxel_dy = ywidth / static_cast<R>(number_of_rows);
    const auto voxel_dz = zwidth / static_cast<R>(number_of_images);

    //Find a 'corner' point which defines the location of the center of the (0,0)th voxel. (This point
    // ignores z-direction and will be projected onto an appropriate z-plane later.)
    const vec3<R> near_corner_zero = symm_line.R_0 - (GridX * grid_x_ext) - (GridY * grid_y_ext)
                                          + (GridX * voxel_dx * static_cast<R>(0.5)) 
                                          + (GridY * voxel_dy * static_cast<R>(0.5));

    //Generate the images.
    planar_image_collection<T,R> out;
    for(long int i = 0; i < number_of_images; ++i){
        if(only_top_and_bottom && (i > 0) && (i < (number_of_images-1))) continue;

        //Construct a plane where the image will sit.
        const plane<R> img_plane( GridZ, zero + (GridZ * grid_z_min)                        //Move to the boundary of the ROI(s) volume.        
                                              + (GridZ * voxel_dz * static_cast<R>(i))      //Move to the volume reserved for this image.
                                              + (GridZ * voxel_dz * static_cast<R>(0.5)) ); //Centre of the volume.

        //Project the generic corner point coordinates onto the image's plane.
        const auto img_offset = img_plane.Project_Onto_Plane_Orthogonally(near_corner_zero);

        //Create and initialize the image.
        out.images.emplace_back();
        out.images.back().init_buffer(number_of_rows, number_of_columns, number_of_channels);
        out.images.back().init_spatial(voxel_dx, voxel_dy, voxel_dz, zero, img_offset);
        out.images.back().init_orientation(GridX, GridY);
        out.images.back().fill_pixels(pixel_fill);
    }

    return out;
}                         
#ifndef YGOR_IMAGES_DISABLE_ALL_SPECIALIZATIONS
    template planar_image_collection<uint8_t ,double> Symmetrically_Contiguously_Grid_Volume(
             const std::list<std::reference_wrapper<contour_collection<double>>> &,
             const double, const double, const double, const long int, const long int, const long int, const long int,
             const line<double> &, const vec3<double> &, const vec3<double> &, const double, bool);
    template planar_image_collection<uint16_t,double> Symmetrically_Contiguously_Grid_Volume(
             const std::list<std::reference_wrapper<contour_collection<double>>> &,
             const double, const double, const double, const long int, const long int, const long int, const long int,
             const line<double> &, const vec3<double> &, const vec3<double> &, const double, bool);
    template planar_image_collection<uint32_t,double> Symmetrically_Contiguously_Grid_Volume(
             const std::list<std::reference_wrapper<contour_collection<double>>> &,
             const double, const double, const double, const long int, const long int, const long int, const long int,
             const line<double> &, const vec3<double> &, const vec3<double> &, const double, bool);
    template planar_image_collection<uint64_t,double> Symmetrically_Contiguously_Grid_Volume(
             const std::list<std::reference_wrapper<contour_collection<double>>> &,
             const double, const double, const double, const long int, const long int, const long int, const long int,
             const line<double> &, const vec3<double> &, const vec3<double> &, const double, bool);
    template planar_image_collection<float   ,double> Symmetrically_Contiguously_Grid_Volume(
             const std::list<std::reference_wrapper<contour_collection<double>>> &,
             const double, const double, const double, const long int, const long int, const long int, const long int,
             const line<double> &, const vec3<double> &, const vec3<double> &, const double, bool);
#endif
