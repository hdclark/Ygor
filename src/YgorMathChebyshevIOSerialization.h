//YgorMathChebyshevIOSerialization.h - Serialization routines for Chebyshev classes.

#pragma once

#include "YgorDefinitions.h"
#include "YgorIOXMLSerialization.h"
#include "YgorMathChebyshev.h"

namespace ygor {
namespace serialization {

template<class T>
void serialize(xml_oarchive &a, cheby_approx<T> &v){
    auto coeffs = v.Get_Coefficients();
    auto domain = v.Get_Domain();
    a << make_nvp("coeffs", coeffs)
      << make_nvp("xmin", domain.first)
      << make_nvp("xmax", domain.second);
}

template<class T>
void serialize(xml_iarchive &a, cheby_approx<T> &v){
    decltype(v.Get_Coefficients()) coeffs;
    decltype(v.Get_Domain()) domain;
    a >> make_nvp("coeffs", coeffs)
      >> make_nvp("xmin", domain.first)
      >> make_nvp("xmax", domain.second);
    v.Prepare(coeffs, domain.first, domain.second);
}

} // namespace serialization
} // namespace ygor
