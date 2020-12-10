//YgorMathChebyshevIOBoostSerialization.h - Written by hal clark in 2016.
//
// This file defines routines for using Chebyshev polynomial classes with Boost.Serialization.
//
// This file is optional. If you choose to use it, ensure you link with libboost_serialization.
//
// The objects in this file have been 'wrapped' into a boost.serialization name-value pair so that they can be
// serialized to archives requiring entities to be named (i.e., XML) in addition to the 'easier' formats (i.e.,
// plain text, binary).
//

#pragma once

#include <boost/serialization/version.hpp> 
#if __has_include(<boost/serialization/library_version_type.hpp>)
    // Required for Boost 1.74.
    #include <boost/serialization/library_version_type.hpp> 
#endif
#include <boost/serialization/nvp.hpp> 

#include <boost/serialization/string.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/split_free.hpp>

#include "YgorDefinitions.h"
#include "YgorMathChebyshev.h"

namespace boost {
namespace serialization {


//Class: cheby_approx<T>.
template<typename Archive, class T>
void save(Archive &a, const cheby_approx<T> &v, const unsigned int /*version*/){
    auto coeffs = v.Get_Coefficients();
    auto domain = v.Get_Domain();

    a << boost::serialization::make_nvp("coeffs",coeffs)
      << boost::serialization::make_nvp("xmin",domain.first)
      << boost::serialization::make_nvp("xmax",domain.second);

    return;
}

template<typename Archive, class T>
void load(Archive &a, cheby_approx<T> &v, const unsigned int /*version*/){
    decltype(v.Get_Coefficients()) coeffs;
    decltype(v.Get_Domain()) domain;

    a >> boost::serialization::make_nvp("coeffs",coeffs)
      >> boost::serialization::make_nvp("xmin",domain.first)
      >> boost::serialization::make_nvp("xmax",domain.second);

    v.Prepare(coeffs,domain.first,domain.second);

    return;
}

template<typename Archive, class T>
inline void serialize(Archive &a, cheby_approx<T> &v, const unsigned int version){
    boost::serialization::split_free(a, v, version);
    return;
}


} // namespace serialization
} // namespace boost
