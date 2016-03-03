//YgorMathChebyshev.h

#ifndef YGOR_MATH_CHEBYSHEV_H_
#define YGOR_MATH_CHEBYSHEV_H_

#include <cmath>
#include <iostream>
#include <fstream>
#include <complex>
#include <list>
#include <map>
#include <string>
#include <functional>
#include <tuple>
#include <vector>
#include <array>
#include <limits>
#include <utility>
#include <experimental/optional>


//---------------------------------------------------------------------------------------------------------------------------
//------------------------------------- cheby_approx: A function approximator -----------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//This class is a 1D function approximator based on Chebyshev polynomials (of the "first kind" --> T_{n}).
// It operates on scalar functions of one dimension (i.e., f(x), g(t), etc.). The function f can, in principle,
// be any scalar distribution (nb. it can include discontinuities and discontinuities in derivatives), but 
// highly discontinous/nasty input will require a lot of coefficients to capture such detail.
//
// Currently, two types of inputs are supported: (1) samples_1D, which are treated as straight line segments 
// and are thus piecewise smooth with discontinuous derivatives, and (2) user-provided std::function's that
// must be evaluable arbitrarily within the specified domain.
//
// NOTE: The samples_1D input is a 'simplistic' approach wherein linear interpolation is used to evaluate the 
//       scalar ordinate at each given abscissa. In principle, you might be able to get away with directly
//       using the samples (i.e., no interpolation necessary) under suitable conditions. These conditions are
//       harsh, though, and require both being clever and lucky. For the scheme to work: (1) the samples need
//       to be spaced according to the Chebyshev sampling strategy (see below implementation); (2) you must
//       be OK using only as many sample points as you have (maybe less, but no more), which can run afoul of
//       the Nyquist theorem in pathological cases; and (3) the data must be relatively noise-free, otherwise
//       the approximation will probably be garbage.
//
// NOTE: This technique would be exact (up to issues such as discontinuities) if an infinite number of 
//       coefficients could be calculated. You'll need to test to determine a suitable cutoff point.
//       As of writing this, you'll need to perform such testing manually. To improve ergonomics, you could
//       implement several helpful schemes, such as letting the user specify a 'maximum deviation' threshold
//       which is tested after each additional datum is added. At the moment, the only specifyable parameter
//       is the number of coefficients to keep, which is extremely opaque with regards to the approximating
//       precision.
//
// NOTE: Infinite approximation ranges are not supported. You'll need to specify finite abscissa extrema.
//
// NOTE: This technique is inspired by that covered in "Numerical Recipes in C" by Press et al. and as is
//       described in the "NIST Handbook of Mathematical Functions" by Olver et al. 2010. 
//
//       In particular, a given (finite) function is mapped onto the domain of the Chebyshev polynomials 
//       [-1,+1]. This can incur loss of precision, but such losses should be fairly small.
//       
//       Additional information about the technique can be found at:
//         - Olver et al. 2010, section 3.11 "Approximation Techniques", subsection 3.11(ii) 
//           "Chebyshev-Series Expansions" and references therein; chapter 18 "Orthogonal Polynomials."
//         - "Chebyshev Polynomials" by Mason et al. 2003.
//         - https://en.wikipedia.org/wiki/Chebyshev_polynomials (20160127)
//         - https://en.wikipedia.org/wiki/Clenshaw%E2%80%93Curtis_quadrature (20160127)
//         - https://en.wikipedia.org/wiki/Clenshaw_algorithm (20160127)
//
template <class T> class cheby_approx {
    private:

        // The expansion of f(x) into the basis composed of Chebyshev polynomials of the first kind:
        //
        //   $f(x) \approx \left[ \sum_{i}^{N-1} c_{i} T_{i}(x) \right] - \frac{c_{0}}{2}$.
        //
        std::vector<T> c;
    
        // The lower and upper bounds that the approximation is valid for. Specified when generating
        // the expansion coefficients.
        T xmin = std::numeric_limits<T>::quiet_NaN();
        T xmax = std::numeric_limits<T>::quiet_NaN();

    public:
        // Constructors.
        cheby_approx();
        cheby_approx(const cheby_approx<T> &);
        //cheby_approx(T xmin_in, T xmax_in, T numb_of_c_in);
   
        // Operators.
        cheby_approx<T> & operator=(const cheby_approx<T> &);

        cheby_approx<T> operator*(const cheby_approx<T> &) const;

 
        // Prepare an approximation using a samples_1D. A simple approximation scheme, which depends
        // on linear interpolation, is used. 
        //
        // NOTE: General recommendations cannot be safely made for all possible distributions, but
        //       at the very least you should probably specify 'numb_of_c_in' to be 2x that of the
        //       number of samples in 's1D'. You can often get away with fewer, though, so you'll
        //       need to experiment.
        //
        // NOTE: 'numb_of_c_in' must be >= 1. Zero is reserved for possible future automated 
        //       coefficient generation.
        //
        // NOTE: If both xmin_in and xmax_in are not finite, the min and max will be derived directly 
        //       from the provided samples_1D.
        //
        // NOTE: xmin_in and xmax_in may legitimately be beyond the provided samples_1D's domain. In
        //       such cases, the value is taken to be whatever the interpolation scheme provides.
        //       Reasonable possibilities include NaN's, static extrapolation where the endpoints are
        //       implicitly extended to +- infinity, and extrapolation using some user-suggested
        //       behaviour.
        //
        void Prepare( const samples_1D<T> &s1D, 
                      size_t numb_of_c_in, 
                      T xmin_in = std::numeric_limits<T>::quiet_NaN(),
                      T xmax_in = std::numeric_limits<T>::quiet_NaN() );


        // Prepare an approximation using an arbitrary, user-provided functor.
        //
        // NOTE: 'numb_of_c_in' must be >= 1. Zero is reserved for possible future automated 
        //       coefficient generation.
        //
        // NOTE: Both xmin_in and xmax_in must be finite. 
        //
        // NOTE: Both xmin_in and xmax_in must be within the domain of the provided functor. 
        //
        void Prepare( const std::function<T(T)> &f,
                      size_t numb_of_c_in,
                      T xmin_in,
                      T xmax_in);


        // Prepare an approximation using explicitly-calculated coefficients.
        //
        // NOTE: Both xmin_in and xmax_in must be finite. 
        //
        void Prepare( const std::vector<T> &c_in,
                      T xmin_in,
                      T xmax_in);


        // Sample the approximation of f(x) at a given point, using the specified number of coefficients.
        //
        // NOTE: 'numb_of_c_to_use' must be <= 'numb_of_c'. You can sometimes get away with using 
        //       << 'numb_of_c' which is computationally advantageous, but you'll need to experiment
        //       'numb_of_c_to_use' can be 0, which is simply shorthand that implies using all available 
        //       coefficients.
        //       
        // NOTE: This routine throws if there are not enough coefficients computed to satisfy the
        //       desired 'numb_of_c_to_use'.
        //       
        // NOTE: This routine returns NaN if the requested abscissa is outside the previously-
        //       specified abscissa extrema [xmin, xmax].
        //       
        T Sample( T x,
                  size_t numb_of_c_to_use = 0 ) const;


        // Sample the approximation of f(x) at N equally-spaced points over the provided domain.
        //
        // NOTE: Internally, this routine repeatedly calls the simple cheby_approx::Sample(x) routine.
        //       All caveats therefore also apply to this routine.
        //
        // NOTE: 'numb_of_samples' must be >0.
        //
        // NOTE: If both xmin_in and xmax_in are not finite, the min and max will be derived directly 
        //       from the provided samples_1D.
        //
        // NOTE: Both xmin_in and xmax_in must be within the domain of the provided functor. 
        //
        // NOTE: Both abscissa extrema will house points. The only exception is when 'numb_of_samples' 
        //       is zero. In this case there will be a single output datum at the midpoint of the extrema.
        // 
        samples_1D<T> 
        Sample_Uniformly( size_t numb_of_samples,
                          size_t numb_of_c_in_to_use = 0,
                          T xmin_in = std::numeric_limits<T>::quiet_NaN(),
                          T xmax_in = std::numeric_limits<T>::quiet_NaN() ) const;


        // Sample the approximation of f(x) at the existing abcissae of the provided samples_1D, over-
        // writing the existing ordinates.
        //
        // NOTE: Internally, this routine repeatedly calls the simple cheby_approx::Sample(x) routine.
        //       All caveats therefore also apply to this routine.
        //
        // NOTE: 'numb_of_c_to_use' must be <= 'numb_of_c'. You can sometimes get away with using 
        //       << 'numb_of_c' which is computationally advantageous, but you'll need to experiment
        //       'numb_of_c_to_use' can be 0, which is simply shorthand that implies using all available 
        //       coefficients.
        //       
        // NOTE: 's1d' can have any number of samples, including zero, which essentially creates a no-op.
        //
        // NOTE: The domain of 's1d' must be fully contained within the domain of the provided functor.
        //
        samples_1D<T>
        Sample_On( const samples_1D<T> &s1d,
                   size_t numb_of_c_to_use = 0 ) const;


        // Compute coefficients for the first derivative along the abcissa. The returned approximation
        // is as if the derivative was taken, and then the derivative was expanded into the Chebyshev
        // basis.
        //
        // NOTE: This is superior to a simple finite-difference numerical derivative. It requires a bit
        //       more computation, but only involves some basic manipulation of the expansion
        //       coefficients.
        //
        // NOTE: Be aware of the usual caveats regarding endpoints. Do *not* rely on the derivatives 
        //       near abscissa extrema to be well-behaved! In particular, oscillations can be very high
        //       in amplitude.
        //
        // NOTE: You *should* be able to round-trip the derivative of the integral many times without
        //       any major losses or instabilities. This is because the manipulations on the coefficients
        //       are well-defined, and do not depend on the actual value of the functor (at least not
        //       directly). If there are losses, they should be strictly due to numerical losses. Also,
        //       still be weary of the abscissa extrema, because the derivative cannot be known exactly
        //       there -- even if the method can round-trip without issue!
        //       
        cheby_approx<T>
        Chebyshev_Derivative(void) const;


        // Compute coefficients for the indefinite integral along the abcissa. The returned approximation
        // is as if the integral was taken, and then the result was expanded into the Chebyshev basis.
        //
        // NOTE: Remember that this is an *indefinite integral*. There is an implicit constant of
        //       integration that you'll need to fix via boundary condition or other constraints!
        //       It's the job of *you* (the user) to carry this factor around and apply as necessary!
        //
        //       However, the reported coefficients explicitly choose the integration constant such that
        //       the integral evaluated at 'xmin' is zero. This is useful in case you want to integrate
        //       over the entire domain so that only a single evaluation of the integral is needed.
        //
        // NOTE: This is superior to a simple numerical integration. Furthermore, it probably requires
        //       less computation unless you have a fancy adaptive integration algorithm and your 
        //       approximand was well-behaved.
        //
        cheby_approx<T>
        Chebyshev_Integral(void) const;

        // Get a copy of the coefficients. Useful for direct manipulation, but you'll need to construct
        // a new cheby_approx<T> in order to use them via the class mechanisms.
        std::vector<T>
        Get_Coefficients(void) const;

        // Get the bounds.
        std::pair<T,T>
        Get_Domain(void) const;

        //Friends.
//        template<class Y> friend std::ostream & operator << (std::ostream &, const vec3<Y> &); // ---> Overloaded stream operators.
//        template<class Y> friend std::istream & operator >> (std::istream &, vec3<Y> &);      
};

#endif
