//YgorImagesIOSerialization.h - Written by hal clark in 2026.
//
//Serialization routines for YgorImage classes.

#pragma once

#include "YgorDefinitions.h"
#include "YgorImages.h"
#include "YgorMathIOSerialization.h"

namespace ygor {
namespace serialization {

template<typename Archive, class T, class R>
void serialize(Archive &a, planar_image<T,R> &p){
    a & make_nvp("data", p.data)
      & make_nvp("rows", p.rows)
      & make_nvp("columns", p.columns)
      & make_nvp("channels", p.channels)
      & make_nvp("pxl_dx", p.pxl_dx)
      & make_nvp("pxl_dy", p.pxl_dy)
      & make_nvp("pxl_dz", p.pxl_dz)
      & make_nvp("anchor", p.anchor)
      & make_nvp("offset", p.offset)
      & make_nvp("row_unit", p.row_unit)
      & make_nvp("col_unit", p.col_unit)
      & make_nvp("metadata", p.metadata);
}

template<typename Archive, class T, class R>
void serialize(Archive &a, planar_image_collection<T,R> &p){
    a & make_nvp("images", p.images);
}

} // namespace serialization
} // namespace ygor
