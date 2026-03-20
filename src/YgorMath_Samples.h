//YgorMath_Samples.h - A few sample things which can be used for testing.

#ifndef YGOR_MATH_SAMPLES_H_
#define YGOR_MATH_SAMPLES_H_

#include <list>

#include "YgorDefinitions.h"
#include "YgorMath.h"

//-------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------- contour_of_points ----------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------
contour_of_points<double> contour_of_points_sample_blob(void);
contour_of_points<double> contour_of_points_sample_gumby(void);
contour_of_points<double> contour_of_points_sample_airplane(void);

//-------------------------------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------- contour_collection ---------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------
contour_collection<double> contour_collection_sample_ICCR2013(void);

//-------------------------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------ samples_1D -------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------
samples_1D<double> samples_1D_sample_CMB(void);
samples_1D<double> samples_1D_sample_CMB_first_half(void);

samples_1D<double> samples_1D_sample_LIDAR(void);

samples_1D<double> samples_1D_sample_Calories(void);

//-------------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------- fv_surface_mesh ----------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------------------------------------
fv_surface_mesh<double, uint64_t> fv_surface_mesh_icosahedron();
fv_surface_mesh<double, uint64_t> fv_surface_mesh_tetrahedron();
fv_surface_mesh<double, uint64_t> fv_surface_mesh_octahedron();
fv_surface_mesh<double, uint64_t> fv_surface_mesh_single_triangle();
fv_surface_mesh<double, uint64_t> fv_surface_mesh_single_quad();


#endif
