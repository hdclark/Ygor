//YgorMathIOSerialization.h - Written by hal clark in 2026.
//
//Serialization routines for YgorMath classes.

#pragma once

#include "YgorDefinitions.h"
#include "YgorIOXMLSerialization.h"
#include "YgorMath.h"

namespace ygor {
namespace serialization {

template<typename Archive, class T>
void serialize(Archive &a, vec3<T> &v){
    a & make_nvp("x", v.x)
      & make_nvp("y", v.y)
      & make_nvp("z", v.z);
}

template<typename Archive, class T>
void serialize(Archive &a, vec2<T> &v){
    a & make_nvp("x", v.x)
      & make_nvp("y", v.y);
}

template<typename Archive, class T>
void serialize(Archive &a, line<T> &l){
    a & make_nvp("R_0", l.R_0)
      & make_nvp("U_0", l.U_0);
}

template<typename Archive, class T>
void serialize(Archive &a, line_segment<T> &l){
    auto &base_line = static_cast<line<T>&>(l);
    a & make_nvp("base_lineT", base_line)
      & make_nvp("t_0", l.t_0)
      & make_nvp("t_1", l.t_1);
}

template<typename Archive, class T>
void serialize(Archive &a, plane<T> &p){
    a & make_nvp("N_0", p.N_0)
      & make_nvp("R_0", p.R_0);
}

template<typename Archive, class T>
void serialize(Archive &a, contour_of_points<T> &c){
    a & make_nvp("points", c.points)
      & make_nvp("closed", c.closed)
      & make_nvp("metadata", c.metadata);
}

template<typename Archive, class T>
void serialize(Archive &a, contour_collection<T> &c){
    a & make_nvp("contours", c.contours);
}

template<typename Archive, class T, class I>
void serialize(Archive &a, fv_surface_mesh<T,I> &m){
    a & make_nvp("vertices", m.vertices)
      & make_nvp("vertex_normals", m.vertex_normals)
      & make_nvp("vertex_colours", m.vertex_colours)
      & make_nvp("faces", m.faces)
      & make_nvp("involved_faces", m.involved_faces)
      & make_nvp("metadata", m.metadata);
}

template<typename Archive, class T>
void serialize(Archive &a, point_set<T> &m){
    a & make_nvp("points", m.points)
      & make_nvp("normals", m.normals)
      & make_nvp("colours", m.colours)
      & make_nvp("metadata", m.metadata);
}

template<typename Archive, class T>
void serialize(Archive &a, lin_reg_results<T> &l){
    a & make_nvp("slope", l.slope)
      & make_nvp("sigma_slope", l.sigma_slope)
      & make_nvp("intercept", l.intercept)
      & make_nvp("sigma_intercept", l.sigma_intercept)

      & make_nvp("N", l.N)
      & make_nvp("dof", l.dof)
      & make_nvp("sigma_f", l.sigma_f)
      & make_nvp("covariance", l.covariance)
      & make_nvp("lin_corr", l.lin_corr)

      & make_nvp("sum_sq_res", l.sum_sq_res)
      & make_nvp("tvalue", l.tvalue)
      & make_nvp("pvalue", l.pvalue)

      & make_nvp("chi_square", l.chi_square)
      & make_nvp("qvalue", l.qvalue)
      & make_nvp("cov_params", l.cov_params)
      & make_nvp("corr_params", l.corr_params)

      & make_nvp("mean_x", l.mean_x)
      & make_nvp("mean_f", l.mean_f)
      & make_nvp("sum_x", l.sum_x)
      & make_nvp("sum_f", l.sum_f)
      & make_nvp("sum_xx", l.sum_xx)
      & make_nvp("sum_xf", l.sum_xf)
      & make_nvp("Sxf", l.Sxf)
      & make_nvp("Sxx", l.Sxx)
      & make_nvp("Sff", l.Sff)

      & make_nvp("mean_wx", l.mean_wx)
      & make_nvp("mean_wf", l.mean_wf)

      & make_nvp("sum_w", l.sum_w)
      & make_nvp("sum_wx", l.sum_wx)
      & make_nvp("sum_wf", l.sum_wf)
      & make_nvp("sum_wxx", l.sum_wxx)
      & make_nvp("sum_wxf", l.sum_wxf);
}

template<typename Archive, class T>
void serialize(Archive &a, samples_1D<T> &s){
    a & make_nvp("samples", s.samples)
      & make_nvp("uktbiar", s.uncertainties_known_to_be_independent_and_random)
      & make_nvp("metadata", s.metadata);
}

} // namespace serialization
} // namespace ygor
