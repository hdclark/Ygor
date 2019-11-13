//YgorImagesIOBoostSerialization.h - Written by hal clark in 2016.
//
// This file defines routines for using YgorImage classes with Boost.Serialization.
//
// This file is optional. If you choose to use it, ensure you link with libboost_serialization. 
//
// The objects in this file have been 'wrapped' into a boost.serialization name-value pair so that they can be
// serialized to archives requiring entities to be named (i.e., XML) in addition to the 'easier' formats (i.e.,
// plain text, binary).
//

#pragma once

#include <boost/serialization/nvp.hpp> 

#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>

#include "YgorDefinitions.h"
#include "YgorImages.h"

#include "YgorMathIOBoostSerialization.h"

namespace boost {
namespace serialization {


//Class: planar_image<T,R>.
template<typename Archive, class T, class R>
void serialize(Archive &a, planar_image<T,R> &p, const unsigned int /*version*/){
    a & boost::serialization::make_nvp("data",p.data)

      & boost::serialization::make_nvp("rows",p.rows)
      & boost::serialization::make_nvp("columns",p.columns)
      & boost::serialization::make_nvp("channels",p.channels)

      & boost::serialization::make_nvp("pxl_dx",p.pxl_dx)
      & boost::serialization::make_nvp("pxl_dy",p.pxl_dy)
      & boost::serialization::make_nvp("pxl_dz",p.pxl_dz)

      & boost::serialization::make_nvp("anchor",p.anchor)
      & boost::serialization::make_nvp("offset",p.offset)
      & boost::serialization::make_nvp("row_unit",p.row_unit)
      & boost::serialization::make_nvp("col_unit",p.col_unit)
      & boost::serialization::make_nvp("metadata",p.metadata);
    return;
}


//Class: planar_image_collection<T,R>.
template<typename Archive, class T, class R>
void serialize(Archive &a, planar_image_collection<T,R> &p, const unsigned int /*version*/){
    a & boost::serialization::make_nvp("images",p.images);
    return;
}


} // namespace serialization
} // namespace boost
