//YgorImages.cc - Routines to help manage buffers of 2D data.
//
#include <iostream>
#include <memory>
#include <string.h>   //For memcpy.
#include <cmath>      //For std::round(...)
#include <algorithm>
#include <list>

#include "YgorMisc.h"
#include "YgorMath.h"
#include "YgorPlot.h"
#include "YgorImages.h"

#ifndef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    #define YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
#endif
//class planar_image;

//----------------------------------------------------------------------------------------------------
//---------------------------------------- planar_image ----------------------------------------------
//----------------------------------------------------------------------------------------------------
//Constructor/Destructors.
template <class T, class R> planar_image<T,R>::planar_image(){
    rows = columns = channels = -1;
    pxl_dx = pxl_dy = pxl_dz = (R)(0);
    start_time = end_time = (R)(0);
    sort_key_A = sort_key_B = 0;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template planar_image<uint8_t ,double>::planar_image(void);
    template planar_image<uint16_t,double>::planar_image(void);
    template planar_image<uint32_t,double>::planar_image(void);
    template planar_image<uint64_t,double>::planar_image(void);
#endif

template <class T, class R> planar_image<T,R>::planar_image(const planar_image<T,R> &in){
    (*this) = in;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template planar_image<uint8_t ,double>::planar_image(const planar_image<uint8_t ,double> &in);
    template planar_image<uint16_t,double>::planar_image(const planar_image<uint16_t,double> &in);
    template planar_image<uint32_t,double>::planar_image(const planar_image<uint32_t,double> &in);
    template planar_image<uint64_t,double>::planar_image(const planar_image<uint64_t,double> &in);
#endif


template <class T, class R> planar_image<T,R>::~planar_image(){ }
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template planar_image<uint8_t ,double>::~planar_image(void);
    template planar_image<uint16_t,double>::~planar_image(void);
    template planar_image<uint32_t,double>::~planar_image(void);
    template planar_image<uint64_t,double>::~planar_image(void);
#endif


//Allocating space and initializing the purely-2D-image members.
template <class T, class R> void planar_image<T,R>::init_buffer(long int rows, long int cols, long int chnls){
    if((rows <= 0) || (cols <= 0) || (chnls <= 0)){
        FUNCERR("Requested to initialize an image with impossible dimensions");
    }
    this->data.reset( new T [rows*cols*chnls] );
    this->rows     = rows;
    this->columns  = cols;
    this->channels = chnls;
    return;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template void planar_image<uint8_t ,double>::init_buffer(long int rows, long int cols, long int chnls);
    template void planar_image<uint16_t,double>::init_buffer(long int rows, long int cols, long int chnls);
    template void planar_image<uint32_t,double>::init_buffer(long int rows, long int cols, long int chnls);
    template void planar_image<uint64_t,double>::init_buffer(long int rows, long int cols, long int chnls);
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
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template void planar_image<uint8_t ,double>::init_spatial(double pxldx, double pxldy, double pxldz, const vec3<double> &anchr, const vec3<double> &offst);
    template void planar_image<uint16_t,double>::init_spatial(double pxldx, double pxldy, double pxldz, const vec3<double> &anchr, const vec3<double> &offst);
    template void planar_image<uint32_t,double>::init_spatial(double pxldx, double pxldy, double pxldz, const vec3<double> &anchr, const vec3<double> &offst);
    template void planar_image<uint64_t,double>::init_spatial(double pxldx, double pxldy, double pxldz, const vec3<double> &anchr, const vec3<double> &offst);
#endif

template <class T, class R> void planar_image<T,R>::init_temporal(R start, R end){
    this->start_time = start;
    this->end_time = end;
    return;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template void planar_image<uint8_t ,double>::init_temporal(double start, double end);
    template void planar_image<uint16_t,double>::init_temporal(double start, double end);
    template void planar_image<uint32_t,double>::init_temporal(double start, double end);
    template void planar_image<uint64_t,double>::init_temporal(double start, double end);
#endif

template <class T, class R> void planar_image<T,R>::init_orientation(const vec3<R> &rowunit, const vec3<R> &colunit){
    this->row_unit = rowunit.unit();
    this->col_unit = colunit.unit();
    return;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template void planar_image<uint8_t ,double>::init_orientation(const vec3<double> &rowunit, const vec3<double> &colunit);
    template void planar_image<uint16_t,double>::init_orientation(const vec3<double> &rowunit, const vec3<double> &colunit);
    template void planar_image<uint32_t,double>::init_orientation(const vec3<double> &rowunit, const vec3<double> &colunit);
    template void planar_image<uint64_t,double>::init_orientation(const vec3<double> &rowunit, const vec3<double> &colunit);
#endif


template <class T,class R> planar_image<T,R> & planar_image<T,R>::operator=(const planar_image<T,R> &rhs){
    if(this == &rhs) return *this;
    //Copies everything except the array's pointer. It allocates a new buffer for this purpose and copies the data.
    this->init_buffer(rhs.rows, rhs.columns, rhs.channels);
    this->init_spatial(rhs.pxl_dx, rhs.pxl_dy, rhs.pxl_dz, rhs.anchor, rhs.offset);
    this->init_temporal(rhs.start_time, rhs.end_time);
    this->init_orientation(rhs.row_unit, rhs.col_unit);

    this->sort_key_A = rhs.sort_key_A;
    this->sort_key_B = rhs.sort_key_B;

    if((rhs.data != nullptr) && (this->data != nullptr)){
        memcpy((void*)(this->data.get()), (void*)(rhs.data.get()), sizeof(T)*rhs.rows*rhs.columns*rhs.channels);
    }
    return *this;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template planar_image<uint8_t ,double> & planar_image<uint8_t ,double>::operator=(const planar_image<uint8_t ,double> &rhs);
    template planar_image<uint16_t,double> & planar_image<uint16_t,double>::operator=(const planar_image<uint16_t,double> &rhs);
    template planar_image<uint32_t,double> & planar_image<uint32_t,double>::operator=(const planar_image<uint32_t,double> &rhs);
    template planar_image<uint64_t,double> & planar_image<uint64_t,double>::operator=(const planar_image<uint64_t,double> &rhs);
#endif

template <class T,class R> bool planar_image<T,R>::operator==(const planar_image<T,R> &rhs) const {
    if(this == &rhs) return true;
    if((this->data == nullptr) && (rhs.data != nullptr)) return false;
    if((this->data != nullptr) && (rhs.data == nullptr)) return false;

    if((this->rows != rhs.rows) || (this->columns != rhs.columns) || (this->channels != rhs.channels)) return false;
    if((this->pxl_dx != rhs.pxl_dx) || (this->pxl_dy != rhs.pxl_dy) || (this->pxl_dz != rhs.pxl_dz))   return false;
    if((this->anchor != rhs.anchor) || (this->offset != rhs.offset)) return false;
    if((this->row_unit != rhs.row_unit) || (this->col_unit != rhs.col_unit)) return false;

    if((this->start_time != rhs.start_time) || (this->end_time != rhs.end_time)) return false;

    if((this->data == nullptr) && (rhs.data == nullptr)) return true;

    //If we get here, we need to compare each pixel.
    for(long int i = 0; i < rhs.rows*rhs.columns*rhs.channels; ++i){
        if(this->data[i] != rhs.data[i]) return false;
    }
    return true; //Equal!
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::operator==(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::operator==(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::operator==(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::operator==(const planar_image<uint64_t,double> &rhs) const;
#endif

template <class T,class R> bool planar_image<T,R>::operator!=(const planar_image<T,R> &rhs) const {
    return !(*this == rhs);
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::operator!=(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::operator!=(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::operator!=(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::operator!=(const planar_image<uint64_t,double> &rhs) const;
#endif

template <class T,class R> bool planar_image<T,R>::operator<(const planar_image<T,R> &rhs) const {
    if(this == &rhs) return false;

    //If the sort keys are different, use them as the primary sorting key.
    if(this->sort_key_A != rhs.sort_key_A) return (this->sort_key_A < rhs.sort_key_A);
    if(this->sort_key_B != rhs.sort_key_B) return (this->sort_key_B < rhs.sort_key_B);

    //Try the start and end times next.
    if(this->start_time != rhs.start_time) return (this->start_time < rhs.start_time);
    if(this->end_time   != rhs.end_time)   return (this->end_time   < rhs.end_time);

    //Now, compare the size (in sizeof(R)*bytes). The overall memory usage is probably a good comparison.
    if(this->rows*this->columns*this->channels != rhs.rows*rhs.columns*rhs.channels){
        return (this->rows*this->columns*this->channels != rhs.rows*rhs.columns*rhs.channels);
    }

    //Handle the case of empty images.
    if((this->data == nullptr) && (rhs.data != nullptr)) return true;  //Arbitrary. Stick with convention, though.
    if((this->data != nullptr) && (rhs.data == nullptr)) return false; //Arbitrary. Stick with convention, though.
    if((this->data == nullptr) || (rhs.data == nullptr)) return false; //Considered equal.

    //Next, compare the components of the zeroth pixel. This is also an ill-defined operation!
    const auto l = this->position(0,0);
    const auto r = rhs.position(0,0);
    return (l < r);
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::operator<(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::operator<(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::operator<(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::operator<(const planar_image<uint64_t,double> &rhs) const;
#endif

template <class T,class R> bool planar_image<T,R>::Temporally_eq(const planar_image<T,R> &rhs) const {
    if(this == &rhs) return true;

    //Defined explicitly.
    //return (this->start_time == rhs.start_time) && (this->end_time == rhs.end_time);
 
    //Defined in terms of Temporally_lt().
    return (!this->Temporally_lt(rhs) && !rhs.Temporally_lt(*this));
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::Temporally_eq(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::Temporally_eq(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::Temporally_eq(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::Temporally_eq(const planar_image<uint64_t,double> &rhs) const;
#endif

template <class T,class R> bool planar_image<T,R>::Temporally_lt(const planar_image<T,R> &rhs) const {
    if(this == &rhs) return false;

    if(this->start_time != rhs.start_time) return this->start_time < rhs.start_time;
    return this->end_time < rhs.end_time;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::Temporally_lt(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::Temporally_lt(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::Temporally_lt(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::Temporally_lt(const planar_image<uint64_t,double> &rhs) const;
#endif

template <class T,class R> bool planar_image<T,R>::Temporally_lte(const planar_image<T,R> &rhs) const {
    //Without using Temporally_eq()..
    //if(this == &rhs) return true;
    //if(this->Temporally_lt(rhs)) return true;
    //return (this->Temporally_lt(rhs)) == (rhs.Temporally_lt(*this));

    //With using Temporally_eq().
    return this->Temporally_lt(rhs) || this->Temporally_eq(rhs);
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::Temporally_lte(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::Temporally_lte(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::Temporally_lte(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::Temporally_lte(const planar_image<uint64_t,double> &rhs) const;
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
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::Spatially_eq(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::Spatially_eq(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::Spatially_eq(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::Spatially_eq(const planar_image<uint64_t,double> &rhs) const;
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
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::Spatially_lt(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::Spatially_lt(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::Spatially_lt(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::Spatially_lt(const planar_image<uint64_t,double> &rhs) const;
#endif

template <class T,class R> bool planar_image<T,R>::Spatially_lte(const planar_image<T,R> &rhs) const {
    //Without using Spatially_eq()..
    if(this == &rhs) return true;
    if(this->Spatially_lt(rhs)) return true;
    return (this->Spatially_lt(rhs)) == (rhs.Spatially_lt(*this));

    //With using Spatially_eq().
    //return this->Spatially_eq(rhs) || (this->Spatially_lt(rhs);
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::Spatially_lte(const planar_image<uint8_t ,double> &rhs) const;
    template bool planar_image<uint16_t,double>::Spatially_lte(const planar_image<uint16_t,double> &rhs) const;
    template bool planar_image<uint32_t,double>::Spatially_lte(const planar_image<uint32_t,double> &rhs) const;
    template bool planar_image<uint64_t,double>::Spatially_lte(const planar_image<uint64_t,double> &rhs) const;
#endif


//Zero-based indexing (the default).
template <class T, class R> long int planar_image<T,R>::index(long int row, long int col) const {
    return this->channels*( this->columns * row + col );
} 
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template long int planar_image<uint8_t ,double>::index(long int r, long int c) const;
    template long int planar_image<uint16_t,double>::index(long int r, long int c) const;
    template long int planar_image<uint32_t,double>::index(long int r, long int c) const;
    template long int planar_image<uint64_t,double>::index(long int r, long int c) const;
#endif

template <class T, class R> long int planar_image<T,R>::index(long int row, long int col, long int chnl) const {
    return this->channels*( this->columns * row + col ) + chnl;
} 
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template long int planar_image<uint8_t ,double>::index(long int row, long int col, long int chnl) const;
    template long int planar_image<uint16_t,double>::index(long int row, long int col, long int chnl) const;
    template long int planar_image<uint32_t,double>::index(long int row, long int col, long int chnl) const;
    template long int planar_image<uint64_t,double>::index(long int row, long int col, long int chnl) const;
#endif

//Returns -1 on failure or out-of-bounds.
template <class T, class R> long int planar_image<T,R>::index(const vec3<R> &point, long int chnl) const {
    const vec3<R> P(point - this->anchor - this->offset);
    const auto Nr = this->row_unit.Dot(P)/this->pxl_dx;  // ~row.
    const auto Nc = this->col_unit.Dot(P)/this->pxl_dy;  // ~col.
    const auto Uz = this->row_unit.Cross(this->col_unit).unit(); //Unit along orthogonal direction.
    const auto Nz = Uz.Dot(P)/( static_cast<R>(2.0)*this->pxl_dz );

    //Check if it is too far out of the plane of the 2D image. Be inclusive in case the image thickness is 0.
    if(!isininc((R)(-1.0),Nz,(R)(1.0))) return -1;

    //Now, Nr and Nc should (ideally) be integers. They will be very close in value to ints, too, except
    // for floating point errors which may have blurred them slightly above or below the actual value.
    // Because we do not expect the blur to be significant, we will just round them to the nearest int.
    // If the blur is more than this, we have larger issues to deal with! 
    const auto row = static_cast<long int>( std::round(Nr) ); 
    const auto col = static_cast<long int>( std::round(Nc) );

    if(!isininc(0,row,this->rows-1) || !isininc(0,col,this->columns-1) || !isininc(0,chnl,this->channels)){
        return -1;
    }
    return this->index(row,col,chnl);
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template long int planar_image<uint8_t ,double>::index(const vec3<double> &point, long int chnl) const;
    template long int planar_image<uint16_t,double>::index(const vec3<double> &point, long int chnl) const;
    template long int planar_image<uint32_t,double>::index(const vec3<double> &point, long int chnl) const;
    template long int planar_image<uint64_t,double>::index(const vec3<double> &point, long int chnl) const;
#endif


//Channel-value getters.
template <class T, class R> T planar_image<T,R>::value(long int row, long int col, long int chnl) const {
    if(!isininc(0,row,this->rows-1) || !isininc(0,col,this->columns-1) || !isininc(0,chnl,this->channels)){
        FUNCERR("Attempted to access part of image which does not exist");
    }
    return this->data[this->index(row,col,chnl)];
} 
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template uint8_t  planar_image<uint8_t ,double>::value(long int row, long int col, long int chnl) const;
    template uint16_t planar_image<uint16_t,double>::value(long int row, long int col, long int chnl) const;
    template uint32_t planar_image<uint32_t,double>::value(long int row, long int col, long int chnl) const;
    template uint64_t planar_image<uint64_t,double>::value(long int row, long int col, long int chnl) const;
#endif

//Returns the value of the voxel which contains the point, or zero.
//
//NOTE: This function is very slow and not always very safe! Use it sparingly!
template <class T, class R> T planar_image<T,R>::value(const vec3<R> &point, long int chnl) const {
    const auto indx = this->index(point,chnl); 
    if(indx == -1) return static_cast<T>(0);
    return this->data[indx];
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template uint8_t  planar_image<uint8_t ,double>::value(const vec3<double> &point, long int chnl) const;
    template uint16_t planar_image<uint16_t,double>::value(const vec3<double> &point, long int chnl) const;
    template uint32_t planar_image<uint32_t,double>::value(const vec3<double> &point, long int chnl) const;
    template uint64_t planar_image<uint64_t,double>::value(const vec3<double> &point, long int chnl) const;
#endif


//Channel-value references. This can be used to set the values.
template <class T,class R> T& planar_image<T,R>::reference(long int row, long int col, long int chnl){
    if(!isininc(0,row,this->rows-1) || !isininc(0,col,this->columns-1) || !isininc(0,chnl,this->channels)){
        FUNCERR("Attempted to access part of image which does not exist");
    }
    return this->data[this->index(row,col,chnl)];
} 
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template uint8_t  & planar_image<uint8_t ,double>::reference(long int row, long int col, long int chnl);
    template uint16_t & planar_image<uint16_t,double>::reference(long int row, long int col, long int chnl);
    template uint32_t & planar_image<uint32_t,double>::reference(long int row, long int col, long int chnl);
    template uint64_t & planar_image<uint64_t,double>::reference(long int row, long int col, long int chnl);
#endif

//Get an R^3 position of the *center* of the pixel/voxel.
template <class T,class R> vec3<R> planar_image<T,R>::position(long int row, long int col) const {
    if(!isininc(0,row,this->rows-1) || !isininc(0,col,this->columns-1)){
        FUNCERR("Attempted to access part of image which does not exist");
    }
    vec3<R> out(this->anchor);
    out += this->offset;
    out += this->row_unit*(this->pxl_dx*static_cast<R>(row));
    out += this->col_unit*(this->pxl_dy*static_cast<R>(col));
    return out;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template vec3<double> planar_image<uint8_t ,double>::position(long int row, long int col) const;
    template vec3<double> planar_image<uint16_t,double>::position(long int row, long int col) const;
    template vec3<double> planar_image<uint32_t,double>::position(long int row, long int col) const;
    template vec3<double> planar_image<uint64_t,double>::position(long int row, long int col) const;
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
    const auto maxdistrow  = this->row_unit.Dot(r0 - center);//Extra half-pixel width accounted for below.

    const auto coldist     = this->col_unit.Dot(in - center);
    const auto maxdistcol  = this->col_unit.Dot(r0 - center);//Extra half-pixel width accounted for below.

    const auto perpdist    = (this->col_unit.Cross(this->row_unit)).Dot(in - center);

    if(YGORABS(perpdist) >= (dt*0.5)) return false;
    if(YGORABS(rowdist)  >= (YGORABS(maxdistrow)+this->pxl_dx*0.5)) return false;
    if(YGORABS(coldist)  >= (YGORABS(maxdistcol)+this->pxl_dy*0.5)) return false;
    return true;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::encompasses_point(const vec3<double> &in) const;
    template bool planar_image<uint16_t,double>::encompasses_point(const vec3<double> &in) const;
    template bool planar_image<uint32_t,double>::encompasses_point(const vec3<double> &in) const;
    template bool planar_image<uint64_t,double>::encompasses_point(const vec3<double> &in) const;
#endif

template <class T,class R> bool planar_image<T,R>::sandwiches_point_within_top_bottom_planes(const vec3<R> &in) const {
    const auto center   = this->center();
    const auto dt = this->pxl_dz;
    const auto N = this->col_unit.Cross(this->row_unit).unit();
    const auto perpdist = N.Dot(in - center);
    if(YGORABS(perpdist) >= (dt*0.5)) return false;
    return true;
    
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::sandwiches_point_within_top_bottom_planes(const vec3<double> &in) const;
    template bool planar_image<uint16_t,double>::sandwiches_point_within_top_bottom_planes(const vec3<double> &in) const;
    template bool planar_image<uint32_t,double>::sandwiches_point_within_top_bottom_planes(const vec3<double> &in) const;
    template bool planar_image<uint64_t,double>::sandwiches_point_within_top_bottom_planes(const vec3<double> &in) const;
#endif

//Determine if a given time is (inclusively) encompassed by the image's temporal extent.
template <class T,class R> bool planar_image<T,R>::encompasses_time(R t) const {
    return isininc(this->start_time, t, this->end_time);
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::encompasses_time(double t) const;
    template bool planar_image<uint16_t,double>::encompasses_time(double t) const;
    template bool planar_image<uint32_t,double>::encompasses_time(double t) const;
    template bool planar_image<uint64_t,double>::encompasses_time(double t) const;
#endif


//Returns the R^3 center of the image. Nothing fancy.
template <class T,class R> vec3<R> planar_image<T,R>::center(void) const {
    const auto r0 = this->position(           0,              0);
//    const auto r1 = this->position(this->rows-1,              0);
    const auto r2 = this->position(this->rows-1,this->columns-1);
//    const auto r3 = this->position(           0,this->columns-1);
//    return (r0+r1+r2+r3)/4.0;
    return (r0+r2)/2.0;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template vec3<double> planar_image<uint8_t ,double>::center(void) const;
    template vec3<double> planar_image<uint16_t,double>::center(void) const;
    template vec3<double> planar_image<uint32_t,double>::center(void) const;
    template vec3<double> planar_image<uint64_t,double>::center(void) const;
#endif

//The central time of the image's temporal extent.
template <class T,class R> R planar_image<T,R>::temporal_center(void) const {
    return (R)(0.5)*(this->start_time + this->end_time);
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template double planar_image<uint8_t ,double>::temporal_center(void) const;
    template double planar_image<uint16_t,double>::temporal_center(void) const;
    template double planar_image<uint32_t,double>::temporal_center(void) const;
    template double planar_image<uint64_t,double>::temporal_center(void) const;
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
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template std::list<vec3<double>> planar_image<uint8_t ,double>::corners2D(void) const;
    template std::list<vec3<double>> planar_image<uint16_t,double>::corners2D(void) const;
    template std::list<vec3<double>> planar_image<uint32_t,double>::corners2D(void) const;
    template std::list<vec3<double>> planar_image<uint64_t,double>::corners2D(void) const;
#endif

//Returns true if the 3D volume of this image encompasses the 2D image of the given planar image.
template <class T,class R> bool planar_image<T,R>::encloses_2D_planar_image(const planar_image<T,R> &in) const {
    //This routine is slightly tricky because the encompass routines do not (and should not) include points
    // exactly on the boundary as being "encompassed."
    //
    //Therefore, we do a bit of a 'fuzzy' check here.
    const auto inc = in.corners2D();
    const auto thisc = this->corners2D();
    const auto scale = 0.5*YGORMIN(this->pxl_dx + this->pxl_dy, in.pxl_dx + in.pxl_dy);

    for(auto p_it = inc.begin(); p_it != inc.end(); ++p_it){
        //If the point is truly inside, no problem. If not, investigate further.
        if(!this->encompasses_point(*p_it)){
            //Slightly adjust the corner point and test it.
            const auto unit  = (*p_it - this->center()).unit();
            const auto Rtest = *p_it - unit*scale*(R)(0.01); //100x smaller than the smallest voxel width.
            if(!this->encompasses_point(Rtest)) return false;
        }
    }
    return true;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::encloses_2D_planar_image(const planar_image<uint8_t ,double> &in) const;
    template bool planar_image<uint16_t,double>::encloses_2D_planar_image(const planar_image<uint16_t,double> &in) const;
    template bool planar_image<uint32_t,double>::encloses_2D_planar_image(const planar_image<uint32_t,double> &in) const;
    template bool planar_image<uint64_t,double>::encloses_2D_planar_image(const planar_image<uint64_t,double> &in) const;
#endif


//Temporal overlap of the (start --> end) ranges.
template <class T,class R> bool planar_image<T,R>::Temporally_Overlap(const planar_image<T,R> &in) const {
    //If there is any overlap (endpoints inclusive) of the (start_time --> end_time) ranges, then there is
    // temporal overlap.
    //
    // We can check this by comparing whether each endpoint is within the two endpoints of the other range.
    // We must check both ranges this way in case one range fully encompasses the other (we don't know which
    // is the overlapping range in this case).
    const auto A0 = this->start_time;
    const auto A1 = this->end_time;
    const auto B0 = in.start_time;
    const auto B1 = in.end_time;
    return isininc(A0,B0,A1) ||
           isininc(A0,B1,A1) ||
           isininc(B0,A0,B1) ||
           isininc(B0,A1,B1);
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image<uint8_t ,double>::Temporally_Overlap(const planar_image<uint8_t ,double> &in) const;
    template bool planar_image<uint16_t,double>::Temporally_Overlap(const planar_image<uint16_t,double> &in) const;
    template bool planar_image<uint32_t,double>::Temporally_Overlap(const planar_image<uint32_t,double> &in) const;
    template bool planar_image<uint64_t,double>::Temporally_Overlap(const planar_image<uint64_t,double> &in) const;
#endif



//Plot an outline of the image. Useful for alignment testing.
template <class T,class R> void planar_image<T,R>::Plot_Outline(void) const {
    Plotter3 a_plot;
    a_plot.Set_Global_Title("Image Outline");//: Object with address " + Xtostring<long int>((size_t)((void *)(this))));
    vec3<R> r;
    r = this->position(            0,               0);    a_plot.Insert(r.x,r.y,r.z);
    r = this->position( this->rows-1,               0);    a_plot.Insert(r.x,r.y,r.z);
    r = this->position( this->rows-1, this->columns-1);    a_plot.Insert(r.x,r.y,r.z);
    r = this->position(            0, this->columns-1);    a_plot.Insert(r.x,r.y,r.z);
    r = this->position(            0,               0);    a_plot.Insert(r.x,r.y,r.z); //Close the contour.
    a_plot.Next_Line_Same_Style();
    a_plot.Plot();
    return;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template void planar_image<uint8_t ,double>::Plot_Outline(void) const;
    template void planar_image<uint16_t,double>::Plot_Outline(void) const;
    template void planar_image<uint32_t,double>::Plot_Outline(void) const;
    template void planar_image<uint64_t,double>::Plot_Outline(void) const;
#endif


//---------------------------------------------------------------------------------------------------------------------------
//---------------------- planar_image_collection: a collection of logically-related planar_images  --------------------------
//---------------------------------------------------------------------------------------------------------------------------
template <class T,class R> planar_image_collection<T,R>::planar_image_collection() {}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template planar_image_collection<uint8_t ,double>::planar_image_collection();
    template planar_image_collection<uint16_t,double>::planar_image_collection();
    template planar_image_collection<uint32_t,double>::planar_image_collection();
    template planar_image_collection<uint64_t,double>::planar_image_collection();
#endif

template <class T,class R> planar_image_collection<T,R>::planar_image_collection(const planar_image_collection<T,R> &in) : images(in.images) {}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template planar_image_collection<uint8_t ,double>::planar_image_collection(const planar_image_collection<uint8_t ,double> &in);
    template planar_image_collection<uint16_t,double>::planar_image_collection(const planar_image_collection<uint16_t,double> &in);
    template planar_image_collection<uint32_t,double>::planar_image_collection(const planar_image_collection<uint32_t,double> &in);
    template planar_image_collection<uint64_t,double>::planar_image_collection(const planar_image_collection<uint64_t,double> &in);
#endif

template <class T,class R> planar_image_collection<T,R>::planar_image_collection(const std::list<planar_image<T,R>> &in) : images(in) {}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template planar_image_collection<uint8_t ,double>::planar_image_collection(const std::list<planar_image<uint8_t ,double>> &in);
    template planar_image_collection<uint16_t,double>::planar_image_collection(const std::list<planar_image<uint16_t,double>> &in);
    template planar_image_collection<uint32_t,double>::planar_image_collection(const std::list<planar_image<uint32_t,double>> &in);
    template planar_image_collection<uint64_t,double>::planar_image_collection(const std::list<planar_image<uint64_t,double>> &in);
#endif

//Member functions.
template <class T,class R> planar_image_collection<T,R> & planar_image_collection<T,R>::operator=(const planar_image_collection<T,R> &rhs){
    if(this == &rhs) return *this;
    this->images = rhs.images;
    return *this;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template planar_image_collection<uint8_t ,double> & planar_image_collection<uint8_t ,double>::operator=(const planar_image_collection<uint8_t ,double> &in);
    template planar_image_collection<uint16_t,double> & planar_image_collection<uint16_t,double>::operator=(const planar_image_collection<uint16_t,double> &in);
    template planar_image_collection<uint32_t,double> & planar_image_collection<uint32_t,double>::operator=(const planar_image_collection<uint32_t,double> &in);
    template planar_image_collection<uint64_t,double> & planar_image_collection<uint64_t,double>::operator=(const planar_image_collection<uint64_t,double> &in);
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
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::operator==(const planar_image_collection<uint8_t ,double> &in) const;
    template bool planar_image_collection<uint16_t,double>::operator==(const planar_image_collection<uint16_t,double> &in) const;
    template bool planar_image_collection<uint32_t,double>::operator==(const planar_image_collection<uint32_t,double> &in) const;
    template bool planar_image_collection<uint64_t,double>::operator==(const planar_image_collection<uint64_t,double> &in) const;
#endif

template <class T,class R> bool planar_image_collection<T,R>::operator!=(const planar_image_collection<T,R> &in) const {
    return !(*this == in);
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::operator!=(const planar_image_collection<uint8_t ,double> &in) const;
    template bool planar_image_collection<uint16_t,double>::operator!=(const planar_image_collection<uint16_t,double> &in) const;
    template bool planar_image_collection<uint32_t,double>::operator!=(const planar_image_collection<uint32_t,double> &in) const;
    template bool planar_image_collection<uint64_t,double>::operator!=(const planar_image_collection<uint64_t,double> &in) const;
#endif

template <class T,class R> bool planar_image_collection<T,R>::operator<(const planar_image_collection<T,R> &rhs) const {
    //Compares the number of images, and then compares ONLY the first image. This is not ideal, but then again sorting a collection
    // of things generally requires an obtuse approach. 
    if(this->images.size() != rhs.images.size()) return (this->images.size() < rhs.images.size());
    const auto itA = this->images.begin(), itB = rhs.images.begin();
    return ((*itA) < (*itB));
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::operator<(const planar_image_collection<uint8_t ,double> &in) const;
    template bool planar_image_collection<uint16_t,double>::operator<(const planar_image_collection<uint16_t,double> &in) const;
    template bool planar_image_collection<uint32_t,double>::operator<(const planar_image_collection<uint32_t,double> &in) const;
    template bool planar_image_collection<uint64_t,double>::operator<(const planar_image_collection<uint64_t,double> &in) const;
#endif


template <class T,class R> void planar_image_collection<T,R>::Stable_Sort_on_Sort_Keys_Temporal(void){
    auto comparison = [](const planar_image<T,R> &lhs, const planar_image<T,R> &rhs) -> bool {
        //Sort keys.
        if(lhs.sort_key_A != rhs.sort_key_A) return lhs.sort_key_A < rhs.sort_key_A;
        if(lhs.sort_key_B != rhs.sort_key_B) return lhs.sort_key_B < rhs.sort_key_B;

        //Tie-break with temporal ordering.
        if(!lhs.Temporally_eq(rhs)) return lhs.Temporally_lt(rhs);

        //Otherwise, defer ties to the planar_image lt operator.
        return lhs < rhs;
    };
    this->images.sort(comparison);
    return;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template void planar_image_collection<uint8_t ,double>::Stable_Sort_on_Sort_Keys_Temporal(void);
    template void planar_image_collection<uint16_t,double>::Stable_Sort_on_Sort_Keys_Temporal(void);
    template void planar_image_collection<uint32_t,double>::Stable_Sort_on_Sort_Keys_Temporal(void);
    template void planar_image_collection<uint64_t,double>::Stable_Sort_on_Sort_Keys_Temporal(void);
#endif

template <class T,class R> void planar_image_collection<T,R>::Stable_Sort_on_Temporal_Sort_Keys(void){
    auto comparison = [](const planar_image<T,R> &lhs, const planar_image<T,R> &rhs) -> bool {
        //Temporal ordering.
        if(!lhs.Temporally_eq(rhs)) return lhs.Temporally_lt(rhs);

        //Tie-break with sort keys.
        if(lhs.sort_key_A != rhs.sort_key_A) return lhs.sort_key_A < rhs.sort_key_A;
        if(lhs.sort_key_B != rhs.sort_key_B) return lhs.sort_key_B < rhs.sort_key_B;

        //Otherwise, defer ties to the planar_image lt operator.
        return lhs < rhs;
    };
    this->images.sort(comparison);
    return;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template void planar_image_collection<uint8_t ,double>::Stable_Sort_on_Temporal_Sort_Keys(void);
    template void planar_image_collection<uint16_t,double>::Stable_Sort_on_Temporal_Sort_Keys(void);
    template void planar_image_collection<uint32_t,double>::Stable_Sort_on_Temporal_Sort_Keys(void);
    template void planar_image_collection<uint64_t,double>::Stable_Sort_on_Temporal_Sort_Keys(void);
#endif

template <class T,class R> void planar_image_collection<T,R>::Stable_Sort_on_Temporal_Spatial(void){
    auto comparison = [](const planar_image<T,R> &lhs, const planar_image<T,R> &rhs) -> bool {
        //Temporal ordering.
        if(!lhs.Temporally_eq(rhs)) return lhs.Temporally_lt(rhs);
        
        //Tie-break with spatial ordering.
        if(!lhs.Spatially_eq(rhs)) return lhs.Spatially_lt(rhs); 

        //Otherwise, defer ties to the planar_image lt operator.
        return lhs < rhs;
    };
    this->images.sort(comparison);
    return;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template void planar_image_collection<uint8_t ,double>::Stable_Sort_on_Temporal_Spatial(void);
    template void planar_image_collection<uint16_t,double>::Stable_Sort_on_Temporal_Spatial(void);
    template void planar_image_collection<uint32_t,double>::Stable_Sort_on_Temporal_Spatial(void);
    template void planar_image_collection<uint64_t,double>::Stable_Sort_on_Temporal_Spatial(void);
#endif

template <class T,class R> void planar_image_collection<T,R>::Stable_Sort_on_Spatial_Temporal(void){
    auto comparison = [](const planar_image<T,R> &lhs, const planar_image<T,R> &rhs) -> bool {

return lhs.Spatially_lt(rhs);

        //Spatial ordering.
        if(!lhs.Spatially_eq(rhs)) return lhs.Spatially_lt(rhs);

        //Tie-break with temporal ordering.
        if(!lhs.Temporally_eq(rhs)) return lhs.Temporally_lt(rhs);

        //Otherwise, defer ties to the planar_image lt operator.
        return lhs < rhs;
    };
    this->images.sort(comparison);
    return;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template void planar_image_collection<uint8_t ,double>::Stable_Sort_on_Spatial_Temporal(void);
    template void planar_image_collection<uint16_t,double>::Stable_Sort_on_Spatial_Temporal(void);
    template void planar_image_collection<uint32_t,double>::Stable_Sort_on_Spatial_Temporal(void);
    template void planar_image_collection<uint64_t,double>::Stable_Sort_on_Spatial_Temporal(void);
#endif

template <class T,class R> void planar_image_collection<T,R>::Stable_Sort_on_Sort_Keys_Spatial(void){
    auto comparison = [](const planar_image<T,R> &lhs, const planar_image<T,R> &rhs) -> bool {
        //Sort keys.
        if(lhs.sort_key_A != rhs.sort_key_A) return lhs.sort_key_A < rhs.sort_key_A;
        if(lhs.sort_key_B != rhs.sort_key_B) return lhs.sort_key_B < rhs.sort_key_B;

        //Tie-break with spatial ordering.
        if(!lhs.Spatially_eq(rhs)) return lhs.Spatially_lt(rhs);

        //Otherwise, defer ties to the planar_image lt operator.
        return lhs < rhs;
    };
    this->images.sort(comparison);
    return;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template void planar_image_collection<uint8_t ,double>::Stable_Sort_on_Sort_Keys_Spatial(void);
    template void planar_image_collection<uint16_t,double>::Stable_Sort_on_Sort_Keys_Spatial(void);
    template void planar_image_collection<uint32_t,double>::Stable_Sort_on_Sort_Keys_Spatial(void);
    template void planar_image_collection<uint64_t,double>::Stable_Sort_on_Sort_Keys_Spatial(void);
#endif

template <class T,class R> void planar_image_collection<T,R>::Stable_Sort_on_Spatial_Sort_Keys(void){
    auto comparison = [](const planar_image<T,R> &lhs, const planar_image<T,R> &rhs) -> bool {
        //Spatial ordering.
        if(!lhs.Spatially_eq(rhs)) return lhs.Spatially_lt(rhs);

        //Tie-break with sort keys.
        if(lhs.sort_key_A != rhs.sort_key_A) return lhs.sort_key_A < rhs.sort_key_A;
        if(lhs.sort_key_B != rhs.sort_key_B) return lhs.sort_key_B < rhs.sort_key_B;

        //Otherwise, defer ties to the planar_image lt operator.
        return lhs < rhs;
    };
    this->images.sort(comparison);
    return;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template void planar_image_collection<uint8_t ,double>::Stable_Sort_on_Spatial_Sort_Keys(void);
    template void planar_image_collection<uint16_t,double>::Stable_Sort_on_Spatial_Sort_Keys(void);
    template void planar_image_collection<uint32_t,double>::Stable_Sort_on_Spatial_Sort_Keys(void);
    template void planar_image_collection<uint64_t,double>::Stable_Sort_on_Spatial_Sort_Keys(void);
#endif




//Returns a list of pointers to images which encompass a given point.
// Be careful not to invalidate the data after calling this function.
template <class T,class R> std::list<typename std::list<planar_image<T,R>>::const_iterator> planar_image_collection<T,R>::get_images_which_encompass_point(const vec3<R> &in) const {
    std::list<typename std::list<planar_image<T,R>>::const_iterator> out;

    //Cycle through all of the images, maintaining the current order.
    for(auto i_it = this->images.begin(); i_it != this->images.end(); ++i_it){
        if(i_it->encompasses_point(in)) out.push_back(i_it);
    }
    return out;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template std::list<typename std::list<planar_image<uint8_t ,double>>::const_iterator>
        planar_image_collection<uint8_t ,double>::get_images_which_encompass_point(const vec3<double> &in) const;
    template std::list<typename std::list<planar_image<uint16_t,double>>::const_iterator>
        planar_image_collection<uint16_t,double>::get_images_which_encompass_point(const vec3<double> &in) const;
    template std::list<typename std::list<planar_image<uint32_t,double>>::const_iterator>
        planar_image_collection<uint32_t,double>::get_images_which_encompass_point(const vec3<double> &in) const;
    template std::list<typename std::list<planar_image<uint64_t,double>>::const_iterator> 
        planar_image_collection<uint64_t,double>::get_images_which_encompass_point(const vec3<double> &in) const;
#endif

//Returns a list of pointers to images which are sandwiched between the infinite planes lying along the top and bottom of the planar image (of finite thickness). 
// Be careful not to invalidate the data after calling this function.
template <class T,class R>
std::list<typename std::list<planar_image<T,R>>::const_iterator> 
planar_image_collection<T,R>::get_images_which_sandwich_point_within_top_bottom_planes(const vec3<R> &in) const {
    std::list<typename std::list<planar_image<T,R>>::const_iterator> out;

    //Cycle through all of the images, maintaining the current order.
    for(auto i_it = this->images.begin(); i_it != this->images.end(); ++i_it){
        if(i_it->sandwiches_point_within_top_bottom_planes(in)) out.push_back(i_it);
    }
    return out;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template std::list<typename std::list<planar_image<uint8_t ,double>>::const_iterator> 
        planar_image_collection<uint8_t ,double>::get_images_which_sandwich_point_within_top_bottom_planes(const vec3<double> &in) const;
    template std::list<typename std::list<planar_image<uint16_t,double>>::const_iterator> 
        planar_image_collection<uint16_t,double>::get_images_which_sandwich_point_within_top_bottom_planes(const vec3<double> &in) const;
    template std::list<typename std::list<planar_image<uint32_t,double>>::const_iterator> 
        planar_image_collection<uint32_t,double>::get_images_which_sandwich_point_within_top_bottom_planes(const vec3<double> &in) const;
    template std::list<typename std::list<planar_image<uint64_t,double>>::const_iterator> 
        planar_image_collection<uint64_t,double>::get_images_which_sandwich_point_within_top_bottom_planes(const vec3<double> &in) const;
#endif


//Returns a list of list::const_iterators to images which temporally overlap a given timepoint.
// Be careful not to invalidate the data after calling this!
template <class T,class R>
std::list<typename std::list<planar_image<T,R>>::const_iterator> 
planar_image_collection<T,R>::get_images_which_encompasses_time(R t) const {
    std::list<typename std::list<planar_image<T,R>>::const_iterator> out;

    //Cycle through all of the images, maintaining the current order.
    for(auto i_it = this->images.begin(); i_it != this->images.end(); ++i_it){
        if(i_it->encompasses_time(t)) out.push_back(i_it);
    }
    return out;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template std::list<typename std::list<planar_image<uint8_t ,double>>::const_iterator> 
        planar_image_collection<uint8_t ,double>::get_images_which_encompasses_time(double t) const;
    template std::list<typename std::list<planar_image<uint16_t,double>>::const_iterator> 
        planar_image_collection<uint16_t,double>::get_images_which_encompasses_time(double t) const;
    template std::list<typename std::list<planar_image<uint32_t,double>>::const_iterator> 
        planar_image_collection<uint32_t,double>::get_images_which_encompasses_time(double t) const;
    template std::list<typename std::list<planar_image<uint64_t,double>>::const_iterator> 
        planar_image_collection<uint64_t,double>::get_images_which_encompasses_time(double t) const;
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
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template vec3<double> planar_image_collection<uint8_t ,double>::center(void) const;
    template vec3<double> planar_image_collection<uint16_t,double>::center(void) const;
    template vec3<double> planar_image_collection<uint32_t,double>::center(void) const;
    template vec3<double> planar_image_collection<uint64_t,double>::center(void) const;
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
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::Spatially_eq(const planar_image_collection<uint8_t ,double> &in) const;
    template bool planar_image_collection<uint16_t,double>::Spatially_eq(const planar_image_collection<uint16_t,double> &in) const;
    template bool planar_image_collection<uint32_t,double>::Spatially_eq(const planar_image_collection<uint32_t,double> &in) const;
    template bool planar_image_collection<uint64_t,double>::Spatially_eq(const planar_image_collection<uint64_t,double> &in) const;
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
template <class T,class R> bool planar_image_collection<T,R>::Collate_Images(planar_image_collection<T,R> &in, 
                                                                             bool SortKeyAOverlapOK,        
                                                                             bool SortKeyBOverlapOK,        
                                                                             bool GeometricalOverlapOK,        
                                                                             bool TemporalOverlapOK){
    //Loop over the input images, bailing at the first merge difficulty.
    while(!in.images.empty()){
        auto img_it = in.images.begin();
   
        //If needed, check for sort key overlap.
        if(!SortKeyAOverlapOK){
            for(const auto &thisimg : this->images) if(img_it->sort_key_A == thisimg.sort_key_A) return false;
        }
        if(!SortKeyBOverlapOK){
            for(const auto &thisimg : this->images) if(img_it->sort_key_B == thisimg.sort_key_B) return false;
        }

        //If needed, check for geometrical overlap.
        if(!GeometricalOverlapOK){
            //We do a less rigorous approach instead of a true volume overlap procedure.
            // Ideally, something like Dice coefficient with a suitable threshold score would be better.
            for(const auto &thisimg : this->images) if(img_it->encloses_2D_planar_image(thisimg)) return false;
        }

        //If needed, check for temporal overlap.
        if(!TemporalOverlapOK){
            //This is inclusive of the endpoints. So same-time in start or end temporally overlap.
            for(const auto &thisimg : this->images) if(img_it->Temporally_Overlap(thisimg)) return false;
        }

        //If we get here, perform the merge by simply appending the image to this' collection.
        this->images.splice(this->images.end(),in.images,img_it);
    }
    return true;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template bool planar_image_collection<uint8_t ,double>::Collate_Images(planar_image_collection<uint8_t ,double> &in,
                                                                               bool SortKeyAOverlapOK,
                                                                               bool SortKeyBOverlapOK,
                                                                               bool GeometricalOverlapOK,
                                                                               bool TemporalOverlapOK);
    template bool planar_image_collection<uint16_t,double>::Collate_Images(planar_image_collection<uint16_t,double> &in,
                                                                               bool SortKeyAOverlapOK,
                                                                               bool SortKeyBOverlapOK,
                                                                               bool GeometricalOverlapOK,
                                                                               bool TemporalOverlapOK);
    template bool planar_image_collection<uint32_t,double>::Collate_Images(planar_image_collection<uint32_t,double> &in,
                                                                               bool SortKeyAOverlapOK,
                                                                               bool SortKeyBOverlapOK,
                                                                               bool GeometricalOverlapOK,
                                                                               bool TemporalOverlapOK);
    template bool planar_image_collection<uint64_t,double>::Collate_Images(planar_image_collection<uint64_t,double> &in,
                                                                               bool SortKeyAOverlapOK,
                                                                               bool SortKeyBOverlapOK,
                                                                               bool GeometricalOverlapOK,
                                                                               bool TemporalOverlapOK);
#endif


template <class T,class R> void planar_image_collection<T,R>::Plot_Outlines(const std::string &title) const {
    Plotter3 a_plot;
    a_plot.Set_Global_Title(title);
    for(auto it = this->images.begin(); it != this->images.end(); ++it){
        vec3<R> r;
        r = it->position(          0,             0);    a_plot.Insert(r.x,r.y,r.z);
        r = it->position( it->rows-1,             0);    a_plot.Insert(r.x,r.y,r.z);
        r = it->position( it->rows-1, it->columns-1);    a_plot.Insert(r.x,r.y,r.z);
        r = it->position(          0, it->columns-1);    a_plot.Insert(r.x,r.y,r.z);
        r = it->position(          0,             0);    a_plot.Insert(r.x,r.y,r.z); //Close the contour.
        a_plot.Next_Line_Same_Style();
    }
    a_plot.Plot();
    return;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template void planar_image_collection<uint8_t ,double>::Plot_Outlines(const std::string &title) const;
    template void planar_image_collection<uint16_t,double>::Plot_Outlines(const std::string &title) const;
    template void planar_image_collection<uint32_t,double>::Plot_Outlines(const std::string &title) const;
    template void planar_image_collection<uint64_t,double>::Plot_Outlines(const std::string &title) const;
#endif

template <class T,class R> void planar_image_collection<T,R>::Plot_Outlines(void) const {
    this->Plot_Outlines(""); //No title.
    return;
}
#ifdef YGOR_IMAGES_INCLUDE_ALL_SPECIALIZATIONS
    template void planar_image_collection<uint8_t ,double>::Plot_Outlines(void) const;
    template void planar_image_collection<uint16_t,double>::Plot_Outlines(void) const;
    template void planar_image_collection<uint32_t,double>::Plot_Outlines(void) const;
    template void planar_image_collection<uint64_t,double>::Plot_Outlines(void) const;
#endif


//---------------------------------------------------------------------------------------------------------------------------
//--------------------------- postscriptinator: a thin class for generating Postscript images -------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class generates Postscript files from simple data. It was developed to convert the workflow 
// R^3 -> OpenGL Bitmap -> R^3 (using potrace or similar) into just R^3.

template <class R> 
postscriptinator<R>::postscriptinator(){
    this->xmin = this->ymin = (R)( 1E99);
    this->xmax = this->ymax = (R)(-1E99);

    this->PageW = 21.0; //[cm]
    this->PageW = 24.0; //[cm]
    this->Enable_Auto_Sizing = true;

    //Set the page dimensions and coordinates.
    // Bottom left corner: (0,0). Top right: (21,24). Units are [cm]. Page size is 8.5"x11".
    this->Header += "%!PS\n";

    //Some definitions which are used to ~compress the resulting file.
    this->Definitions += "\n";
    this->Definitions += "/m {newpath moveto} bind def\n";
    this->Definitions += "/l {lineto} bind def\n";
    this->Definitions += "/cp {closepath} bind def\n";
    this->Definitions += "/s {stroke} bind def\n";
    this->Definitions += "/sg {setgray} bind def\n";
    this->Definitions += "\n";

    //Closing things.
    this->Footer += "showpage\n";
}


template <class R> void postscriptinator<R>::Import_Contour(const contour_of_points<R> &C, const vec3<R> &Proj2){
    //Performs two passes. First is to adjust the min/max values.


FUNCERR("This routine has not yet been written!");

}


template <class R> std::string postscriptinator<R>::Generate_Page_Geom(void) const {
    std::string out;
    //Compute the page dimension/density information.
    //Default is: 72 dpi (ie. 28.3465 dots/cm).
    out += "matrix currentmatrix /originmat exch def\n";
    out += "/umatrix {originmat matrix concatmatrix setmatrix} def\n";
    out += "[28.3465 0 0 28.3465 10.5 100.0] umatrix\n";

    return std::move(out);
}

template <class R> std::string postscriptinator<R>::Assemble(void) const {
    std::string out;
    out += this->Header;
    out += this->Definitions;
    out += this->Generate_Page_Geom();    
    for(auto it = this->Stack.begin(); it != this->Stack.end(); ++it){
        out += *it;
    }
    out += this->Footer;
    return std::move(out);
}




