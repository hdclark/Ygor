//YgorMathIOBoostSerialization.h - Written by hal clark in 2016.
//
// This file defines routines for using YgorMath classes with Boost.Serialization.
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
#include <boost/serialization/array.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/map.hpp>


#include "YgorMath.h"

namespace boost {
namespace serialization {


//Class: vec3<T>.
template<typename Archive, class T>
void serialize(Archive &a, vec3<T> &v, const unsigned int /*version*/){
    a & boost::serialization::make_nvp("x",v.x)
      & boost::serialization::make_nvp("y",v.y)
      & boost::serialization::make_nvp("z",v.z);
    return;
}

//Class: vec2<T>.
template<typename Archive, class T>
void serialize(Archive &a, vec2<T> &v, const unsigned int /*version*/){
    a & boost::serialization::make_nvp("x",v.x)
      & boost::serialization::make_nvp("y",v.y);
    return;
}

//Class: line<T>.
template<typename Archive, class T>
void serialize(Archive &a, line<T> &l, const unsigned int /*version*/){
    a & boost::serialization::make_nvp("R_0",l.R_0)
      & boost::serialization::make_nvp("U_0",l.U_0);
    return;
}

//Class: line_segment<T>.
template<typename Archive, class T>
void serialize(Archive &a, line_segment<T> &l, const unsigned int /*version*/){
    a & boost::serialization::make_nvp("base_lineT", boost::serialization::base_object<line<T>>(l))
      & boost::serialization::make_nvp("t_0",l.t_0)
      & boost::serialization::make_nvp("t_1",l.t_1);
    return;
}

//Class: plane<T>.
template<typename Archive, class T>
void serialize(Archive &a, plane<T> &p, const unsigned int /*version*/){
    a & boost::serialization::make_nvp("N_0",p.N_0)
      & boost::serialization::make_nvp("R_0",p.R_0);
    return;
}

//Class: contour_of_points<T>.
template<typename Archive, class T>
void serialize(Archive &a, contour_of_points<T> &c, const unsigned int /*version*/){
    a & boost::serialization::make_nvp("points",c.points)
      & boost::serialization::make_nvp("closed",c.closed)
      & boost::serialization::make_nvp("metadata",c.metadata);
    return;
}

//Class: contour_collection<T>.
template<typename Archive, class T>
void serialize(Archive &a, contour_collection<T> &c, const unsigned int /*version*/){
    a & boost::serialization::make_nvp("contours",c.contours);
    return;
}

//Class: fv_surface_mesh<T,I>.
template<typename Archive, class T, class I>
void serialize(Archive &a, fv_surface_mesh<T,I> &m, const unsigned int /*version*/){
    a & boost::serialization::make_nvp("vertices", m.vertices)
      & boost::serialization::make_nvp("faces", m.faces)
      & boost::serialization::make_nvp("involved_faces", m.involved_faces)
      & boost::serialization::make_nvp("metadata", m.metadata);
    return;
}

//Class: point_set<T>.
template<typename Archive, class T>
void serialize(Archive &a, point_set<T> &m, const unsigned int /*version*/){
    a & boost::serialization::make_nvp("points", m.points)
      & boost::serialization::make_nvp("metadata", m.metadata);
    return;
}

//Class: lin_reg_results<T>.
template<typename Archive, class T>
void serialize(Archive &a, lin_reg_results<T> &l, const unsigned int /*version*/){
    a & boost::serialization::make_nvp("slope",l.slope)
      & boost::serialization::make_nvp("sigma_slope",l.sigma_slope)
      & boost::serialization::make_nvp("intercept",l.intercept)
      & boost::serialization::make_nvp("sigma_intercept",l.sigma_intercept)

      & boost::serialization::make_nvp("N",l.N)
      & boost::serialization::make_nvp("dof",l.dof)
      & boost::serialization::make_nvp("sigma_f",l.sigma_f)
      & boost::serialization::make_nvp("covariance",l.covariance)
      & boost::serialization::make_nvp("lin_corr",l.lin_corr)

      & boost::serialization::make_nvp("sum_sq_res",l.sum_sq_res)
      & boost::serialization::make_nvp("tvalue",l.tvalue)
      & boost::serialization::make_nvp("pvalue",l.pvalue)
      
      & boost::serialization::make_nvp("chi_square",l.chi_square)
      & boost::serialization::make_nvp("qvalue",l.qvalue)
      & boost::serialization::make_nvp("cov_params",l.cov_params)
      & boost::serialization::make_nvp("corr_params",l.corr_params)

      & boost::serialization::make_nvp("mean_x",l.mean_x)
      & boost::serialization::make_nvp("mean_f",l.mean_f)
      & boost::serialization::make_nvp("sum_x",l.sum_x)
      & boost::serialization::make_nvp("sum_f",l.sum_f)
      & boost::serialization::make_nvp("sum_xx",l.sum_xx)
      & boost::serialization::make_nvp("sum_xf",l.sum_xf)
      & boost::serialization::make_nvp("Sxf",l.Sxf)
      & boost::serialization::make_nvp("Sxx",l.Sxx)
      & boost::serialization::make_nvp("Sff",l.Sff)

      & boost::serialization::make_nvp("mean_wx",l.mean_wx)
      & boost::serialization::make_nvp("mean_wf",l.mean_wf)

      & boost::serialization::make_nvp("sum_w",l.sum_w)
      & boost::serialization::make_nvp("sum_wx",l.sum_wx)
      & boost::serialization::make_nvp("sum_wf",l.sum_wf)
      & boost::serialization::make_nvp("sum_wxx",l.sum_wxx)
      & boost::serialization::make_nvp("sum_wxf",l.sum_wxf);     
    return;
}

//Class: samples_1D<T>.
template<typename Archive, class T>
void serialize(Archive &a, samples_1D<T> &s, const unsigned int /*version*/){
    a & boost::serialization::make_nvp("samples",s.samples)
      & boost::serialization::make_nvp("uktbiar",s.uncertainties_known_to_be_independent_and_random)
      & boost::serialization::make_nvp("metadata",s.metadata);
    return;
}


} // namespace serialization
} // namespace boost
