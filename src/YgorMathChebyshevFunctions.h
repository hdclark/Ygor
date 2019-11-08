//YgorMathChebyshevFunctions.h.

#ifndef YGOR_MATH_CHEBYSHEV_FUNCTIONS_H_
#define YGOR_MATH_CHEBYSHEV_FUNCTIONS_H_

#include <stddef.h>
#include <array>
#include <cmath>
#include <complex>
#include <experimental/optional>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "YgorMathChebyshev.h"



//Returns an approximation of $Ax+B$ in the Chebyshev polynomial basis over the given domain.
//
// This routine uses an exact analytical. There will be exactly two coefficients.
//
template <class T>
cheby_approx<T>
Chebyshev_Basis_Exact_Linear( T xmin_in,
                              T xmax_in,
                              T A = static_cast<T>(1),
                              T B = static_cast<T>(0) );

//Returns an approximation of $C\exp(Ax+B)$ in the Chebyshev polynomial basis over the given domain.
//
// This routine uses a direct sampling approach, and is essentially a reimplementation of the
// generic user-provided functor sampling cheby_approx<T>::Prepare() routine specialized for the
// exponential function.
//
// NOTE: Parameters 'B' and 'C' are redundant. They are both permitted for convenience.
//
template <class T>
cheby_approx<T>
Chebyshev_Basis_Approx_Exp_Sampled( size_t numb_of_c_in,
                                    T xmin_in,
                                    T xmax_in,
                                    T A,
                                    T B = static_cast<T>(0),
                                    T C = static_cast<T>(1) );



//Returns an approximation of $C\exp(Ax+B)$ in the Chebyshev polynomial basis over the given domain.
//
// This routine uses an iterative calculation approach via the Clenshaw method and an ODE-derived
// recurrence relation (directly). Expansion coefficients are computed starting at the highest 
// order ($C_{n-1}$ where $n$ = 'numb_of_c_in') and working back to $C_{0}$. 
//
// NOTE: Numerical losses are certain to occur if $n$ is too large. You'll need to test with the
//       given parameters {A,B,C} over the given domain to see if breakdown occurs. If breakdown
//       *does* occur, try reducing $n$ (i.e., reducing the approximation quality) or switching to
//       the accompanying analytic method (which is slower but not recursive).
//
// NOTE: Parameters 'B' and 'C' are redundant. They are both permitted for convenience.
//
template <class T>
cheby_approx<T> 
Chebyshev_Basis_Approx_Exp_Recurrence( size_t numb_of_c_in,
                                       T xmin_in,
                                       T xmax_in,
                                       T A,
                                       T B = static_cast<T>(0), 
                                       T C = static_cast<T>(1) );

#ifdef YGOR_USE_BOOST
//Returns an approximation of $C\exp(Ax+B)$ in the Chebyshev polynomial basis over the given domain.
//
// This routine uses an analytic expression for the coefficients based on the modified Bessel function
// of the first kind: $I_{\nu}(x)$. See: "Chebyshev Methods in Numerical Approximation" by Snyder 
// 1966 (page 45, equations 4.2-1 to 4.2-7) for details.
//
// NOTE: When {A,B,C} are all ~ 1.0, I've found 5 coefficients to give a reasonable approximation
//       of the decay side of the exponential ~near x = 0. 
//
// NOTE: This routine may be slower than the accompanying recurrence method (depending on how the
//       modified Bessel function is computed). Prefer the faster recurrence method if you can.
//
// NOTE: Numerical losses will probably occur at high $n$ because $c_{n}$ grow fairly quickly. However,
//       this routine should remain intact longer than the accompanying recurrence method (which is
//       faster) because errors will not propagate between coefficients. If this routine fails, try
//       reworking the internals to be more considerate of intermediate computations which are likely
//       to overflow. For example, the coefficients are composed of factorials which are eventually
//       scaled down; the scaling factor could be passed in to pre-scale the factorial components if
//       needed.
//
// NOTE: Parameters 'B' and 'C' are redundant. They are both permitted for convenience.
//
// NOTE: Because the exponential greatly magnifies numerical precision losses in the parameters,
//       it is only a good idea to use this routine if: $C \approx 1$; $\exp(B) \approx 1$; and
//       $\exp(A*x) \approx 1$ for all $x \in [xmin:xmax]$. In particular, if these conditions do
//       not hold, you should expect losses originating from the multiplication of exponentials.
//
//       In such cases, it might be best to abandon using the specialized exp() routines and using
//       the cheby_approx<T>::Prepare() routine that accepts an arbitrary user-provided functor.
//       It can sample your desired $C\exp(Ax+B)$ directly using a cosine transform, without 
//       needing to shift and exponentials around. Whatever you do, perform some comparison tests!
//
template <class T>
cheby_approx<T> 
Chebyshev_Basis_Approx_Exp_Analytic1( size_t numb_of_c_in,
                                      T xmin_in,
                                      T xmax_in,
                                      T A,
                                      T B = static_cast<T>(0),
                                      T C = static_cast<T>(1) );
#endif // YGOR_USE_BOOST


//Returns an approximation of $C\exp(Ax+B)$ in the Chebyshev polynomial basis over the given domain.
//
// This routine uses an analytic expression for the coefficients which are derived using the Clenshaw
// method and an ODE-derived recurrence relation. The recurrence relation was solved giving an analytic
// expression for the coefficients. 
//
// The core benefit of this routine is that the coefficients are given in terms of simple functions
// (i.e., factorials). In practice, it will be slower and numerically 'leakier' than the other methods
// here.
//
// NOTE: This routine is likely to be much slower than the accompanying recurrence method. Prefer
//       the faster recurrence method if you can.
//
// NOTE: Numerical losses will probably occur at high $n$ because they grow fairly quickly. However,
//       this routine should remain intact longer than the accompanying recurrence method (which is
//       faster) because errors will not propagate between coefficients. If this routine fails, try
//       reworking the internals to be more considerate of intermediate computations which are likely
//       to overflow. For example, the coefficients are composed of factorials which are eventually
//       scaled down; the scaling factor could be passed in to pre-scale the factorial components if
//       needed.
//
// NOTE: Parameters 'B' and 'C' are redundant. They are both permitted for convenience.
//
// NOTE: Because the exponential greatly magnifies numerical precision losses in the parameters,
//       it is only a good idea to use this routine if: $C \approx 1$; $\exp(B) \approx 1$; and
//       $\exp(A*x) \approx 1$ for all $x \in [xmin:xmax]$. In particular, if these conditions do
//       not hold, you should expect losses originating from the multiplication of exponentials.
//
//       In such cases, it might be best to abandon using the specialized exp() routines and using
//       the cheby_approx<T>::Prepare() routine that accepts an arbitrary user-provided functor.
//       It can sample your desired $C\exp(Ax+B)$ directly using a cosine transform, without 
//       needing to shift and exponentials around. Whatever you do, perform some comparison tests!
//
template <class T>
cheby_approx<T> 
Chebyshev_Basis_Approx_Exp_Analytic2( size_t numb_of_c_in,
                                      T xmin_in,
                                      T xmax_in,
                                      T A,
                                      T B = static_cast<T>(0),
                                      T C = static_cast<T>(1) );


#endif
