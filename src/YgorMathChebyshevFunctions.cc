//YgorMathChebyshevFunctions.cc.

#include <cmath>       //Needed for fabs, signbit, sqrt, etc...
#include <limits>      //Needed for std::numeric_limits::max().
#include <stdexcept>
#include <string>      //Needed for stringification routines.
#include <vector>
//#include <experimental/optional>

#include <boost/math/special_functions/bessel.hpp>

#include "YgorMathChebyshev.h"
#include "YgorMathChebyshevFunctions.h"
#include "YgorMisc.h"    //For the FUNC* and PERCENT_ERR macro functions.

//#ifndef YGORMATHCHEBYSHEVFUNCTIONS_DISABLE_ALL_SPECIALIZATIONS
//    #define YGORMATHCHEBYSHEVFUNCTIONS_DISABLE_ALL_SPECIALIZATIONS
//#endif

template <class T>
cheby_approx<T>
Chebyshev_Basis_Exact_Linear( T xmin_in,
                              T xmax_in,
                              T A,
                              T B ){
    //This routine spits out two coefficients. Because the first two Chebyshev polynomials are 
    // $T_{n=0}(x) = 1$ and $T_{n=1}(x) = x$ the only work we have to do is transforming the 
    // line $f(x) = Ax+B$ into some $f^{\prime}(z) = A^{\prime}z+B^{\prime}$ such that 
    // $A^{\prime}$ and $B^{\prime}$ map/compress the section of $f(x)$ over the domain 
    // $x \in [xmin,xmax]$ to $f^{\prime}(z)$ over $z \in [-1,+1]$.
    //
    // In other words, $f(xmin) \equiv f^{\prime}(-1)$ and $f(xmax) \equiv f^{\prime}(+1)$
    // and the rest of the domain gets squashed in between.
    //
    // You can effect this procedure by making the simple substitution:
    //     $ x = \frac{xmax - xmin}{2} z + \frac{xmax + xmin}{2} $
    // where $x \in [xmin,xmax]$ and $z \in [-1,+1]$. Then,
    //     $ f^{\prime}(z) = f(x(z))$
    // and, inversely,
    //     $ f^{\prime}(z(x)) = f(x).$
    //
    T xmin = xmin_in;
    T xmax = xmax_in;
    if(xmin > xmax){ //User provided domain backward. Swap it.
        xmin = xmax_in;
        xmax = xmin_in;
    }

    if(false){
    }else if( !std::isfinite(xmin) || !std::isfinite(xmax)){
        throw std::invalid_argument("Cannot approximate line over infinite domain");
    }else if( xmax == xmin ){
        throw std::invalid_argument("Provided an invalid/illegal domain");
    }else if( !std::isfinite(A) || !std::isfinite(B) ){
        throw std::invalid_argument("Cannot approximate line with provided non-finite parameters");
    }

    std::vector<T> c(2,static_cast<T>(0));
    const auto half = static_cast<T>(0.5);
    const auto Aprime = A * (xmax - xmin) * half;
    const auto Bprime = A * (xmax + xmin) * half + B;
    c[0] = Bprime;
    c[0] *= static_cast<T>(2); //We always halve the first coefficient upon evaluation.
    c[1] = Aprime;

    cheby_approx<T> out;
    out.Prepare(c,xmin,xmax);
    return out;
}                                        
#ifndef YGORMATHCHEBYSHEVFUNCTIONS_DISABLE_ALL_SPECIALIZATIONS
    template cheby_approx<float > Chebyshev_Basis_Exact_Linear<float >(float , float , float , float );
    template cheby_approx<double> Chebyshev_Basis_Exact_Linear<double>(double, double, double, double);
#endif


template <class T>
cheby_approx<T> 
Chebyshev_Basis_Approx_Exp_Sampled( size_t numb_of_c_in,
                                     T xmin_in,
                                     T xmax_in,
                                     T A,
                                     T B,
                                     T C ){

    //Directly-sampled approach, in which an exponential function is directly sampled at the
    // roots of the $N$th Chebyshev polynomial.

    T xmin = xmin_in;
    T xmax = xmax_in;
    if(xmin > xmax){ //User provided domain backward. Swap it.
        xmin = xmax_in;
        xmax = xmin_in;
    }

    if(false){
    }else if( !std::isfinite(xmin) || !std::isfinite(xmax)){
        throw std::invalid_argument("Cannot approximate exponential over infinite domain");
    }else if( numb_of_c_in < 3 ){
        throw std::invalid_argument("Cannot approximate exponential with < 3 coefficients");
    }else if( xmax == xmin ){
        throw std::invalid_argument("Provided an invalid/illegal domain");
    }else if( !std::isfinite(A) ){
        throw std::invalid_argument("Cannot approximate exponential with provided non-finite parameter A: " + std::to_string(A));
    }else if( !std::isfinite(B) ){
        throw std::invalid_argument("Cannot approximate exponential with provided non-finite parameter B: " + std::to_string(B));
    }else if( !std::isfinite(C) ){
        throw std::invalid_argument("Cannot approximate exponential with provided non-finite parameter C: " + std::to_string(C));
    }

    const T nT = static_cast<T>(numb_of_c_in);
    const auto N = numb_of_c_in;
    std::vector<T> c(N,static_cast<T>(0));
    std::vector<T> f(N,static_cast<T>(0));

    for(size_t i = 0; i < numb_of_c_in; ++i){
        //The point we sample f(x) at (i.e., a root of a Chebyshev polynomial) on the domain [-1,+1].
        const T iT = static_cast<T>(i);
        const T x = std::cos(M_PI * (iT + 0.5)/nT);

        //The point x on domain [-1,+1] mapped onto the user-provided domain [xmin,xmax].
        const T z = ( xmax * (x + static_cast<T>(1)) 
                    - xmin * (x - static_cast<T>(1)) )/static_cast<T>(2);
        f[i] = ( C*std::exp(A*z + B) );
    }

    for(size_t i = 0; i < numb_of_c_in; ++i){
        const T iT = static_cast<T>(i);
        T sum = static_cast<T>(0);
        for(size_t j = 0; j < numb_of_c_in; ++j){
            const T jT = static_cast<T>(j);
            sum += f[j] * static_cast<T>(std::cos(M_PI * iT * (jT + 0.5)/nT));
        }
        c[i] = (sum * static_cast<T>(2) / nT);
    }

    cheby_approx<T> out;
    out.Prepare(c,xmin,xmax);

    return out;
}
#ifndef YGORMATHCHEBYSHEVFUNCTIONS_DISABLE_ALL_SPECIALIZATIONS
    template cheby_approx<float > Chebyshev_Basis_Approx_Exp_Sampled<float >(size_t, float , float , float , float , float );
    template cheby_approx<double> Chebyshev_Basis_Approx_Exp_Sampled<double>(size_t, double, double, double, double, double);
#endif



template <class T>
cheby_approx<T> 
Chebyshev_Basis_Approx_Exp_Recurrence( size_t numb_of_c_in,
                                       T xmin_in,
                                       T xmax_in,
                                       T A,
                                       T B,
                                       T C ){

//    FUNCERR("Do not use. Under construction");

    //Analytical approach, using the Clenshaw method and the ODE recurrence relation directly.
    // This routine should be fairly fast, but will propagate numerical losses from coefficient
    // to coefficient.

    T xmin = xmin_in;
    T xmax = xmax_in;
    if(xmin > xmax){ //User provided domain backward. Swap it.
        xmin = xmax_in;
        xmax = xmin_in;
    }

    if(false){
    }else if( !std::isfinite(xmin) || !std::isfinite(xmax)){
        throw std::invalid_argument("Cannot approximate exponential over infinite domain");
    }else if( numb_of_c_in < 3 ){
        throw std::invalid_argument("Cannot approximate exponential with < 3 coefficients");
    }else if( xmax == xmin ){
        throw std::invalid_argument("Provided an invalid/illegal domain");
    }else if( !std::isfinite(A) ){
        throw std::invalid_argument("Cannot approximate exponential with provided non-finite parameter A: " + std::to_string(A));
    }else if( !std::isfinite(B) ){
        throw std::invalid_argument("Cannot approximate exponential with provided non-finite parameter B: " + std::to_string(B));
    }else if( !std::isfinite(C) ){
        throw std::invalid_argument("Cannot approximate exponential with provided non-finite parameter C: " + std::to_string(C));
    }


    //To cover the user-provided domain, we have to make a change of variables.
    // Keep in mind that what we will be asked to use the coefficients for is $C\exp(Az+B)$
    // (where z's domain is [xmin:xmax]), but we need to work in the Chebyshev polynomial
    // basis' domain and compute coefficients for $C^{\prime}\exp(A^{\prime}x+B^{\prime})$
    // (where x's domain is [-1:+1]). Therefore, we need to map the parameters {A,B,C} to
    // {A',B',C'} and set the coefficient $C_{0}$ to the boundary condition in the 
    // Chebyshev domain.
    const T Aprime = A * (xmax - xmin) / static_cast<T>(2);
    const T Bprime = B + A * (xmax + xmin) / static_cast<T>(2);
    const T Cprime = C;

    const auto N = numb_of_c_in;
    std::vector<T> c(N,static_cast<T>(0));
    const T g = static_cast<T>(2)/Aprime;

    //We traverse the coefficients in reverse order. We set $C_{N}$ and higher terms to zero.
    // This amounts to fixing one of the free parameters in the recurrence relation. We
    // arbitrarily choose $C_{N-1} = 1$ until we can later rescale using a boundary condition.
    // This amounts to fixing the second and final free parameter. 
    //
    // NOTE: If over-/under-flow is encountered, it might be prudent to try again using a 
    //       different value for $C_{N-1}$. The choice is otherwise totally arbitrary 
    //       other than being real and non-zero.
    //
    c[N-1] = static_cast<T>(1);
    T sum = static_cast<T>(0);
    for(size_t j = (N-1); j >= 1; --j){
        c[j-1] = g*static_cast<T>(j)*c[j] + ( ((j+1) > (N-1) ) ? static_cast<T>(0) : c[j+1] );
        sum += c[j] * static_cast<T>(std::cos(0.5*M_PI*static_cast<double>(j)));
    }
    sum += c[0]/static_cast<T>(2);

    const T fac = Cprime * std::exp(Bprime) / sum; // Scale to: (C')*exp((A')*0 + (B')).
    for(size_t j = 0; j < N; ++j) c[j] *= fac;

    cheby_approx<T> out;
    out.Prepare(c,xmin,xmax);

    return out;
}
#ifndef YGORMATHCHEBYSHEVFUNCTIONS_DISABLE_ALL_SPECIALIZATIONS
    template cheby_approx<float > Chebyshev_Basis_Approx_Exp_Recurrence<float >(size_t, float , float , float , float , float );
    template cheby_approx<double> Chebyshev_Basis_Approx_Exp_Recurrence<double>(size_t, double, double, double, double, double);
#endif


template <class T>
cheby_approx<T>
Chebyshev_Basis_Approx_Exp_Analytic1( size_t numb_of_c_in,
                                      T xmin_in,
                                      T xmax_in,
                                      T A,
                                      T B,
                                      T C ){

    //Coefficients are known exactly if an infinite number are computed. We have to fix the
    // scale with a second-pass rescaling to account for the (assumed non-zero) factor of B.
    //
    // The coefficients are simply:
    //     $ c_{n} = 2 I_{n}(A) $
    // where $A$ is the 'exponential slope' defined as $\exp(Ax)$, and $I_{n}(x)$ is the 
    // "modified Bessel function of the first kind" and is also called 'cyl_bessel_i' in many
    // computer algebra systems and math libraries.

    T xmin = xmin_in;
    T xmax = xmax_in;
    if(xmin > xmax){ //User provided domain backward. Swap it.
        xmin = xmax_in;
        xmax = xmin_in;
    }

    if(false){
    }else if( !std::isfinite(xmin) || !std::isfinite(xmax)){
        throw std::invalid_argument("Cannot approximate exponential over infinite domain");
    }else if( numb_of_c_in < 2 ){
        throw std::invalid_argument("Cannot approximate exponential with < 2 coefficients");
    }else if( xmax == xmin ){
        throw std::invalid_argument("Provided an invalid/illegal domain");
    }else if( !std::isfinite(A) ){
        throw std::invalid_argument("Cannot approximate exponential with provided non-finite parameter A: " + std::to_string(A));
    }else if( !std::isfinite(B) ){
        throw std::invalid_argument("Cannot approximate exponential with provided non-finite parameter B: " + std::to_string(B));
    }else if( !std::isfinite(C) ){
        throw std::invalid_argument("Cannot approximate exponential with provided non-finite parameter C: " + std::to_string(C));
    }


    //To cover the user-provided domain, we have to make a change of variables.
    // Keep in mind that what we will be asked to use the coefficients for is $C\exp(Ax+B)$
    // (where x's domain is [xmin:xmax]), but we need to work in the Chebyshev polynomial
    // basis' domain and compute coefficients for $C^{\prime}\exp(A^{\prime}z+B^{\prime})$
    // (where z's domain is [-1:+1]). Therefore, we need to map the parameters {A,B,C} to
    // {A',B',C'} and set the coefficient $C_{0}$ to the boundary condition in the 
    // Chebyshev domain. See the exact linear function above for more details.

    const T Aprime = A * (xmax - xmin) / static_cast<T>(2);
    const T Bprime = B + A * (xmax + xmin) / static_cast<T>(2);
    const T Cprime = C;

    const auto N = numb_of_c_in;
    std::vector<T> c(N,static_cast<T>(0));
    cheby_approx<T> out;

    T sum = static_cast<T>(0);
    for(size_t j = 0; j < N; ++j){
        try{
            c[j] = static_cast<T>(2) * boost::math::cyl_bessel_i(j,Aprime);
        }catch(const std::overflow_error &e){
            c[j] = std::numeric_limits<T>::infinity();

            //Abort the rest of the computation to save some time.
            out.Prepare(c,xmin,xmax);
            return out;
        }
        sum += c[j] * static_cast<T>(std::cos(0.5*M_PI*static_cast<double>(j)));
    }
    sum -= c[0]/static_cast<T>(2); //Evaluation of $c_{0} T_{n}(x)$ is special case. Correct for T-count.

    const T fac = Cprime * std::exp(Bprime) / sum; // Scale to: (C')*exp((A')*0 + (B')).
    for(size_t j = 0; j < N; ++j) c[j] *= fac;

    out.Prepare(c,xmin,xmax);

    return out;
}
#ifndef YGORMATHCHEBYSHEVFUNCTIONS_DISABLE_ALL_SPECIALIZATIONS
    template cheby_approx<float > Chebyshev_Basis_Approx_Exp_Analytic1<float >(size_t, float , float , float , float , float );
    template cheby_approx<double> Chebyshev_Basis_Approx_Exp_Analytic1<double>(size_t, double, double, double, double, double);
#endif



//Static support functions for the following routine.
template <class T>
static 
inline
T l_a(size_t n, size_t m){
    const auto log_of_res =   static_cast<T>(2) * std::lgamma(n-m+1) 
                            - static_cast<T>(2) * std::lgamma(m+1) 
                            - std::lgamma(n-m-m+1);
    return static_cast<T>(std::exp(log_of_res));
}

template <class T>
static 
inline
T l_b(size_t n, size_t m){
    const auto log_of_res =   std::lgamma(n+1+1) + std::lgamma(n+1) 
                            - std::lgamma(m+1+1) - std::lgamma(m+1) 
                            - std::lgamma(n-m+1);
    return static_cast<T>(std::exp(log_of_res));
}

template <class T>
static 
inline
T Chebyshev_basis_exp_C_fn(T g, size_t n){
    if(n == 0) return static_cast<T>(1);
    if(n == 1) return static_cast<T>(0);

    T res = static_cast<T>(0);
    for(size_t j = 0; j <= (std::floor(n/2.0) - 1); ++j){
        const T gfac = static_cast<T>(std::pow(g, n-2-2*j));
        const T bfac = l_b<T>(n-2-j,j);
        res += gfac * bfac;
    }
    return res;
}

template <class T>
static 
inline
T Chebyshev_basis_exp_C_gn(T g, size_t n){
    if(n == 0) return static_cast<T>(0);
    if(n == 1) return static_cast<T>(1);

    T res = static_cast<T>(0);
    for(size_t j = 0; j <= (std::ceil(n/2.0) - 1); ++j){
        const double gfac = static_cast<T>(std::pow(g, n-1-2*j));
        const double afac = l_a<T>(n-1,j);
        res += gfac * afac;
    }
    return res;
}


template <class T>
cheby_approx<T>
Chebyshev_Basis_Approx_Exp_Analytic2( size_t numb_of_c_in,
                                      T xmin_in,
                                      T xmax_in,
                                      T A,
                                      T B,
                                      T C ){

    FUNCERR("This routine is under construction. Do not use");
    // Outstanding issue:
    //    This routine seems to work if the domain is [-1,+1] but doesn't work for 
    //   arbitrary {A,B,C} for some reason. I'm guessing there is an issue with my
    //   derivation, though, because the Bessel function approach seems to be exact
    //   and so must ultimately match this routine. It might be best to simply drop 
    //   this approach in favour of the Bessel function approach...






    //Explicit formulae are worked out for the coefficients. There are two free parameters,
    // corresponding to the $C_{0}$ and $C_{1}$ coefficients. Each coefficient looks like:
    //       $ C_{n} = f_{n} \cdot C_{0} + g_{n} \cdot C_{1} $
    // where $n \ge 0$. The values of $C_{0}$ and $C_{1}$ are fixed in two ways:
    // 
    // 1. The relative scale of $C_{0}$ and $C_{1}$ is fixed by choosing some $C_{n}$
    //    to be zero. This is done to the coefficient above the user's choice of N (i.e., 
    //    the number of polynomials to keep). The result is that, for some N,
    //          $ C_{1} = - C_{0} * \sfrac{f_{n}}{g_{n}} $.
    //
    //    Note that this procedure is well-defined for all $N>1$ for these specific $f_{n}$ 
    //    and $g_{n}$, but it might be advantageous numerically to invert the result.
    // 
    // 2. The factor $C_{0}$ is chosen using a boundary condition. The boundary condition
    //    is needed because an ODE for which the exponential function was a solution was 
    //    created to find the recurrence relation (which in turn was needed to generate 
    //    the analytic expressions for the coefficients). Invoking the ODE created a
    //    'virtual' degree of freedom which we must therefore remove.
    //    
    //    The boundary condition we choose to use is the exponential function's value at
    //    $x=0$ (which is $e^{A*0})=1$) because it is easy to sum the Chebyshev polynomials
    //    at $x=0$. Any other point could alternatively be used.
    //
    // 
    // The utility of this routine over using the recurrence relation is small. It was 
    // hoped that the coefficients would be separable from the exponential scaling factor
    // $A$, but it turned out to not be the case. This routine requires multiple gamma
    // function evaluations, plus many context switches and a few extra calculations
    // compared with using the recurrence relation directly. Two benefits are: greater
    // numerical stability, because numerical imprecision is not propagated from 
    // coefficient to coefficient, and greater transparency into the procedure.
    //

    T xmin = xmin_in;
    T xmax = xmax_in;
    if(xmin > xmax){ //User provided domain backward. Swap it.
        xmin = xmax_in;
        xmax = xmin_in;
    }

    if(false){
    }else if( !std::isfinite(xmin) || !std::isfinite(xmax)){
        throw std::invalid_argument("Cannot approximate exponential over infinite domain");
    }else if( numb_of_c_in < 2 ){
        throw std::invalid_argument("Cannot approximate exponential with < 2 coefficients");
    }else if( xmax == xmin ){
        throw std::invalid_argument("Provided an invalid/illegal domain");
    }else if( !std::isfinite(A) ){
        throw std::invalid_argument("Cannot approximate exponential with provided non-finite parameter A: " + std::to_string(A));
    }else if( !std::isfinite(B) ){
        throw std::invalid_argument("Cannot approximate exponential with provided non-finite parameter B: " + std::to_string(B));
    }else if( !std::isfinite(C) ){
        throw std::invalid_argument("Cannot approximate exponential with provided non-finite parameter C: " + std::to_string(C));
    }

    //To cover the user-provided domain, we have to make a change of variables.
    // Keep in mind that what we will be asked to use the coefficients for is $C\exp(Az+B)$
    // (where z's domain is [xmin:xmax]), but we need to work in the Chebyshev polynomial
    // basis' domain and compute coefficients for $C^{\prime}\exp(A^{\prime}x+B^{\prime})$
    // (where x's domain is [-1:+1]). Therefore, we need to map the parameters {A,B,C} to
    // {A',B',C'} and set the coefficient $C_{0}$ to the boundary condition in the 
    // Chebyshev domain.

    const T Aprime = A * (xmax - xmin) / static_cast<T>(2);
    const T Bprime = B + A * (xmax + xmin) / static_cast<T>(2);
    const T Cprime = C;

    const auto N = numb_of_c_in;
    std::vector<T> c(N,static_cast<T>(0));

    //Choose the combining factor such that the next coefficient ($C_{N}$) is zero.
    const T g = -static_cast<T>(2)/Aprime;
    const T relative_fac = - Chebyshev_basis_exp_C_fn(g,N) / Chebyshev_basis_exp_C_gn(g,N);
    T sum = static_cast<T>(0);
    for(size_t j = 0; j < N; ++j){
        c[j] = Chebyshev_basis_exp_C_fn(g,j) + relative_fac * Chebyshev_basis_exp_C_gn(g,j);
        sum += c[j] * static_cast<T>(std::cos(0.5*M_PI*static_cast<double>(j)));
        //sum += c[j]; //Evaluate at x=+1 and z=zmax.
    }
    sum -= c[0]/static_cast<T>(2); //Evaluation of $c_{0} T_{n}(x)$ is special case. Correct for T-count.

    const T fac = Cprime * std::exp(Bprime) / sum; // Scale to: (C')*exp((A')*0 + (B')).
    for(size_t j = 0; j < N; ++j) c[j] *= fac;

    cheby_approx<T> out;
    out.Prepare(c,xmin,xmax);

    return out;
}
#ifndef YGORMATHCHEBYSHEVFUNCTIONS_DISABLE_ALL_SPECIALIZATIONS
    template cheby_approx<float > Chebyshev_Basis_Approx_Exp_Analytic2<float >(size_t, float , float , float , float , float );
    template cheby_approx<double> Chebyshev_Basis_Approx_Exp_Analytic2<double>(size_t, double, double, double, double, double);
#endif

