//YgorMathChebyshev.cc.

#include <algorithm>   //Needed for std::reverse.
#include <cmath>       //Needed for fabs, signbit, sqrt, etc...
#include <functional>  //Needed for passing kernel functions to integration schemes.
#include <limits>      //Needed for std::numeric_limits::max().
#include <stdexcept>
#include <utility>     //Needed for std::pair.
#include <vector>
//#include <experimental/optional>

#include "YgorMath.h"
#include "YgorMathChebyshev.h"

//#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
//    #define YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
//#endif

//---------------------------------------------------------------------------------------------------------------------------
//------------------------------------- cheby_approx: A function approximator -----------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

template <class T> 
cheby_approx<T>::cheby_approx(){};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template cheby_approx<float >::cheby_approx(void);
    template cheby_approx<double>::cheby_approx(void);
#endif

template <class T>
cheby_approx<T>::cheby_approx( const cheby_approx<T> &in ){
    *this = in;
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template cheby_approx<float >::cheby_approx( const cheby_approx<float > &);
    template cheby_approx<double>::cheby_approx( const cheby_approx<double> &);
#endif

template <class T>
cheby_approx<T> & 
cheby_approx<T>::operator=(const cheby_approx<T> &rhs){
    //Check if it is itself.
    if(this == &rhs) return *this;

    this->c = rhs.c;
    this->xmin = rhs.xmin;
    this->xmax = rhs.xmax;
    return *this;
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template cheby_approx<float > & cheby_approx<float >::operator=( const cheby_approx<float > &);
    template cheby_approx<double> & cheby_approx<double>::operator=( const cheby_approx<double> &);
#endif


template <class T>
cheby_approx<T> 
cheby_approx<T>::operator*(const cheby_approx<T> &rhs) const {
    //This routine multiplies two cheby_approx so that the resulting expression is an approximation
    // of the approximated functions.
    //
    // NOTE: This routine can suffer from serious loss of numerical precision if applied too many
    //       times. Why? Because many numbers (which can range from large to small) are being
    //       summed. If this is a problem, I suggest writing an alternative, slower but more careful
    //       product routine using proper numerically-safer accumulators. See YgorStats.h for ideas.
    //
    //       Nonetheless, it may help ameliorate the problem if you can simply increase the number
    //       of coefficients being used to approximate the input functions.
    //
    // NOTE: This routine will produce $N+M-1$ coefficients where $N,M$ are the number of 
    //       coefficients in the LHS and RHS 'cheby_approx'. Clearly this is wasteful considering
    //       that an equivalent or better approximation can be had with only $N$ or $M$ 
    //       coefficients if the product of the approximants is directly approximated.
    //       
    //       You may find that fewer than $N+M-1$ coefficients can be used to evaluate the product.
    //       A reasonable rule of thumb is to only use the first $\max(N,M)$ coefficients, but 
    //       you'll need to evaluate the impact of your choice of cut-off in the actual dataset.
    //
    //       If fewer coefficients are to be used, it may not be necessary to actually purge them.
    //       Just pass the cut-off number to the evaluation routine.
    //
    // NOTE: This routine makes use of the general formula for products of Chebyshev polynomials:
    //             $ T_{n}(x) \cdot T_{m}(x) = \frac{1}{2} ( T_{n+m}(x) + T_{|n+m|} ) $
    //       which can be easily derived using the trigonometric representation and making use of
    //       the trigonometric identity:
    //             $ 2 \cos(a)\cos(b) = \cos(a+b) + \cos(a-b) $.
    //

    if(false){
    }else if( this->c.empty() || !std::isfinite(this->xmin) || !std::isfinite(this->xmax) ){
        throw std::invalid_argument("Cannot perform multiplication; LHS has not been prepared");
    }else if( rhs.c.empty() || !std::isfinite(rhs.xmin) || !std::isfinite(rhs.xmax) ){
        throw std::invalid_argument("Cannot perform multiplication; RHS has not been prepared");
    }else if( this->c.empty() || rhs.c.empty() ){
        throw std::invalid_argument("Cannot perform multiplication; too few coefficients were used to prepare the approximations"); 
    }else if( (this->xmin != rhs.xmin) || (this->xmax != rhs.xmax) ){
        throw std::invalid_argument("Cannot perform multiplication; domains are not exactly equal");
    }

    const auto N = this->c.size(); 
    const auto M = rhs.c.size(); 
    std::vector<T> c(N+M-1,static_cast<T>(0));

    const T one = static_cast<T>(1);
    const T half = static_cast<T>(0.5);

    cheby_approx<T> out;

    //The following factors are used to account for the subtraction of $C_{0}/2$ in the general
    // Chebyshev polynomial expansion. 
    T ifac, jfac;

    // NOTE: In the special case where N==M, we can exploit the symmetry to slightly reduce the
    //       dimensionality of the following double loop. This optimization is not implemented
    //       here for simplicity purposes.
    //
    //       This implementation effectively computes matrix elements for an $N-1$ by $M-1$ 
    //       matrix and takes the sums of diagonals as coefficients. 
    for(long int i = 0; i < static_cast<long int>(N); ++i){
        ifac = (i==0) ? half : one ;
        for(long int j = 0; j < static_cast<long int>(M); ++j){
            jfac = (j==0) ? half : one;
            c[i+j] += half * ifac * jfac * this->c[i] * rhs.c[j];
            const size_t diff = (i > j) ? (i-j) : (j-i);
            c[diff] += half * ifac * jfac * this->c[i] * rhs.c[j];
        }
    }
    c[0] *= static_cast<T>(2);
    out.Prepare(c,xmin,xmax);
    return out;
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template cheby_approx<float > cheby_approx<float >::operator*( const cheby_approx<float > &) const;
    template cheby_approx<double> cheby_approx<double>::operator*( const cheby_approx<double> &) const;
#endif



template <class T> 
void
cheby_approx<T>::Prepare( const samples_1D<T> &s1D,
                          size_t numb_of_c_in,
                          T xmin_in,
                          T xmax_in ){

    //Basic input sanity checking.
    if(false){
    }else if( s1D.empty() ){
        throw std::invalid_argument("Cannot approximate an empty samples_1D");
    }else if( numb_of_c_in == 0 ){
        throw std::invalid_argument("Cannot approximate with zero coefficients");
    }

    //Preparing data to be in a uniform state.
    samples_1D<T> s1D_sorted(s1D);
    s1D_sorted.stable_sort();
    if( std::isfinite(xmin_in) && std::isfinite(xmax_in) ){
        if( xmin_in < xmax_in ){
            this->xmin = xmin_in;
            this->xmax = xmax_in;
        }else{ // Domain empty or user provided domain backward. Catch the error later.
            this->xmin = xmax_in;
            this->xmax = xmin_in;
        }
    }else{
        this->xmin = s1D_sorted.Get_Extreme_Datum_x().first[0];
        this->xmax = s1D_sorted.Get_Extreme_Datum_x().second[0];
    }
    if( this->xmin == this->xmax ){
        throw std::invalid_argument("Domain consists of a single point. Approximation not useful");
    }
    const T nT = static_cast<T>(numb_of_c_in);

    //Calculate the Chebyshev coefficients (up to user-specified n; not all must be used later).    
    this->c.empty();
    this->c.reserve(numb_of_c_in);

    std::vector<T> f;
    f.reserve(numb_of_c_in);

    for(size_t i = 0; i < numb_of_c_in; ++i){
        //The point we sample f(x) at (i.e., a root of a Chebyshev polynomial) on the domain [-1,+1].
        const T iT = static_cast<T>(i);
        const T x = std::cos(M_PI * (iT + 0.5)/nT);
        
        //The point x on domain [-1,+1] mapped onto the user-provided domain.
        const T z = ( this->xmax * (x + 1.0) - this->xmin * (x - 1.0) )/2.0;
        f.push_back( s1D_sorted.Interpolate_Linearly(z)[2] );
    }

    for(size_t i = 0; i < numb_of_c_in; ++i){
        const T iT = static_cast<T>(i);
        T sum = static_cast<T>(0);
        for(size_t j = 0; j < numb_of_c_in; ++j){
            const T jT = static_cast<T>(j);
            sum += f[j] * std::cos(M_PI * iT * (jT + 0.5)/nT);
        }
        this->c.push_back(sum * 2.0 / nT);
    }

    return;
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template void cheby_approx<float >::Prepare(const samples_1D<float > &, size_t, float , float );
    template void cheby_approx<double>::Prepare(const samples_1D<double> &, size_t, double, double);
#endif


template <class T> 
void
cheby_approx<T>::Prepare( const std::function<T(T)> &func,
                          size_t numb_of_c_in,
                          T xmin_in,
                          T xmax_in ){

    //Basic input sanity checking.
    if(false){
    }else if( !func ){
        throw std::invalid_argument("Cannot evaluate functor; it is not valid");
    }else if( numb_of_c_in == 0 ){
        throw std::invalid_argument("Cannot approximate with zero coefficients");
    }else if( !std::isfinite(xmin_in) || !std::isfinite(xmax_in) ){
        throw std::invalid_argument("Cannot approximate functor with infinite domain");
    }

    //Preparing data to be in a uniform state.
    if( xmin_in < xmax_in ){
        this->xmin = xmin_in;
        this->xmax = xmax_in;
    }else{ // Domain empty or user provided domain backward. Catch the error later.
        this->xmin = xmax_in;
        this->xmax = xmin_in;
    }
    if( this->xmin == this->xmax ){
        throw std::invalid_argument("Domain consists of a single point. Approximation not useful");
    }
    const T nT = static_cast<T>(numb_of_c_in);

    //Calculate the Chebyshev coefficients (up to user-specified n; not all must be used later).    
    this->c.empty();
    this->c.reserve(numb_of_c_in);

    std::vector<T> f;
    f.reserve(numb_of_c_in);

    for(size_t i = 0; i < numb_of_c_in; ++i){
        //The point we sample f(x) at (i.e., a root of a Chebyshev polynomial) on the domain [-1,+1].
        const T iT = static_cast<T>(i);
        const T x = std::cos(M_PI * (iT + 0.5)/nT);
        
        //The point x on domain [-1,+1] mapped onto the user-provided domain.
        const T z = ( this->xmax * (x + 1.0) - this->xmin * (x - 1.0) )/2.0;
        f.push_back( func(z) );
    }

    for(size_t i = 0; i < numb_of_c_in; ++i){
        const T iT = static_cast<T>(i);
        T sum = static_cast<T>(0);
        for(size_t j = 0; j < numb_of_c_in; ++j){
            const T jT = static_cast<T>(j);
            sum += f[j] * std::cos(M_PI * iT * (jT + 0.5)/nT);
        }
        this->c.push_back(sum * 2.0 / nT);
    }

    //for(size_t j = 0; j < numb_of_c_in; ++j) std::cout << "userfunc: c[" << j << "] = " << c[j] << std::endl;

    return;
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template void cheby_approx<float >::Prepare(const std::function<float (float )> &, size_t, float , float );
    template void cheby_approx<double>::Prepare(const std::function<double(double)> &, size_t, double, double);
#endif

template <class T> 
void
cheby_approx<T>::Prepare( const std::vector<T> &c_in,
                          T xmin_in,
                          T xmax_in ){

    //Basic input sanity checking.
    if(false){
    }else if( c_in.size() < 2 ){
        throw std::invalid_argument("Refusing to construct approximation with so coefficients");
        // Zero coefficients is not well-defined. One coefficient is a static line parallel to y=0.
        // You could enable the latter if needed, but should probably just use a constant such as
        // a mean or median or something...
    }else if( !std::isfinite(xmin_in) || !std::isfinite(xmax_in) ){
        throw std::invalid_argument("Cannot approximate functor with infinite domain");
    }

    //Preparing data to be in a uniform state.
    if( xmin_in < xmax_in ){
        this->xmin = xmin_in;
        this->xmax = xmax_in;
    }else{ // Domain empty or user provided domain backward. Catch the error later.
        this->xmin = xmax_in;
        this->xmax = xmin_in;
    }
    if( this->xmin == this->xmax ){
        throw std::invalid_argument("Domain consists of a single point. Approximation not useful");
    }

    this->c.empty();
    this->c = c_in;
    return;
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template void cheby_approx<float >::Prepare(const std::vector<float > &c_in, float , float );
    template void cheby_approx<double>::Prepare(const std::vector<double> &c_in, double, double);
#endif


template <class T>
T
cheby_approx<T>::Sample( T z,
                         size_t numb_of_c_to_use ) const {
    if(false){
    }else if( this->c.empty() || !std::isfinite(this->xmin) || !std::isfinite(this->xmax) ){
        throw std::invalid_argument("Cannot evalute approximation; approximation not yet prepared");
    }else if( (z < this->xmin) || (z > this->xmax) ){
        return std::numeric_limits<T>::quiet_NaN();
    }else if( numb_of_c_to_use > this->c.size() ){
        throw std::invalid_argument("Unable to use requested number of coefficients; not enough pre-computed");
        //If you want more, simply re-prepare requesting a sufficiently high number of coefficients.
    }
    if(numb_of_c_to_use == 0) numb_of_c_to_use = this->c.size();

    T d  = static_cast<T>(0);
    T dd = static_cast<T>(0);
    T x = ( (z - this->xmax) + (z - this->xmin) )/( this->xmax - this->xmin );
    for(size_t j = (numb_of_c_to_use - 1); j >= 1; --j){
        const T prev_d = d;
        d = 2.0 * x * d - dd + this->c[j];
        dd = prev_d;
    }
    const T approx_f = x*d - dd + 0.5*c[0];
    return approx_f;
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template float  cheby_approx<float >::Sample(float , size_t) const;
    template double cheby_approx<double>::Sample(double, size_t) const;
#endif


template <class T>
samples_1D<T>
cheby_approx<T>::Sample_Uniformly( size_t numb_of_samples,
                                   size_t numb_of_c_to_use,
                                   T xmin_in,
                                   T xmax_in ) const {

    T xmin = xmin_in;
    T xmax = xmax_in;
    if(xmin > xmax){ //User provided domain backward. Swap it.
        xmin = xmax_in;
        xmax = xmin_in;
    }
    if(!std::isfinite(xmin) || !std::isfinite(xmax)){
        xmin = this->xmin;
        xmax = this->xmax;
    }

    if(false){
    }else if( this->c.empty() || !std::isfinite(this->xmin) || !std::isfinite(this->xmax) ){
        throw std::invalid_argument("Cannot evalute approximation; approximation not yet prepared");
    }else if( xmax == xmin ){
        throw std::invalid_argument("Provided an invalid/illegal domain");
    }else if( (xmin < this->xmin) || (xmax > this->xmax) ){
        throw std::domain_error("Requested domain is not enclosed within the approximation's domain of validity");
    }else if( numb_of_samples == 0 ){
        throw std::invalid_argument("Unable to use requested number of coefficients; not enough pre-computed");
    }else if( numb_of_c_to_use > this->c.size() ){
        throw std::invalid_argument("Unable to use requested number of coefficients; not enough pre-computed");
        //If you want more, simply re-prepare requesting a sufficiently high number of coefficients.
    }

    samples_1D<T> out;
    const bool InhibitSort = true;
    if( numb_of_samples == 1 ){
        const T x = 0.5*(xmax - xmin);
        const T f = this->Sample(x,numb_of_c_to_use);
        out.push_back( x, static_cast<T>(0), f, static_cast<T>(0), InhibitSort );
        return out;
    }

    const T dx = (xmax - xmin) / static_cast<T>( numb_of_samples - 1 );
    for(size_t i = 0; i < numb_of_samples; ++i){
        const T x = ((i+1) == numb_of_samples) ? xmax    // Try avoid loss of precision on positive-most extrema.
                                               : xmin + dx * static_cast<T>(i);
//if(!std::isfinite(x)){
//     FUNCERR("Bad x! x = " << x << " at i = " << i << " and xmin / xmax = " << xmin << " / " << xmax);
//}else{
//     FUNCWARN("Good x! x = " << x << " at i = " << i << " and xmin / xmax = " << xmin << " / " << xmax);
//}
        const T f = this->Sample(x,numb_of_c_to_use);
//if(!std::isfinite(f)) FUNCERR("Bad f! f = " << f);
        out.push_back( x, static_cast<T>(0), f, static_cast<T>(0), InhibitSort );
    }
    return out;       
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > cheby_approx<float >::Sample_Uniformly(size_t, size_t, float , float ) const;
    template samples_1D<double> cheby_approx<double>::Sample_Uniformly(size_t, size_t, double, double) const;
#endif


template <class T>
samples_1D<T>
cheby_approx<T>::Sample_On( const samples_1D<T> &s1D,
                            size_t numb_of_c_to_use ) const {


    samples_1D<T> s1D_sorted(s1D);
    s1D_sorted.stable_sort();
  
    const T xmin = s1D_sorted.Get_Extreme_Datum_x().first[0];
    const T xmax = s1D_sorted.Get_Extreme_Datum_x().second[0];

    if(false){
    }else if( this->c.empty() || !std::isfinite(this->xmin) || !std::isfinite(this->xmax) ){
        throw std::invalid_argument("Cannot evalute approximation; approximation not yet prepared");
    }else if( xmax == xmin ){
        throw std::invalid_argument("Provided an invalid/illegal domain");
    }else if( (xmin < this->xmin) || (xmax > this->xmax) ){
        throw std::domain_error("Requested domain is not enclosed within the approximation's domain of validity");
    }else if( numb_of_c_to_use > this->c.size() ){
        throw std::invalid_argument("Unable to use requested number of coefficients; not enough pre-computed");
        //If you want more, simply re-prepare requesting a sufficiently high number of coefficients.
    }

    const bool InhibitSort = true;
    for(auto &asample : s1D_sorted.samples){
        const T x = asample[0];
        const T f = this->Sample(x,numb_of_c_to_use);
        asample[2] = f;
    }
    return s1D_sorted;
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > cheby_approx<float >::Sample_On(const samples_1D<float > &, size_t) const;
    template samples_1D<double> cheby_approx<double>::Sample_On(const samples_1D<double> &, size_t) const;
#endif


template <class T>
cheby_approx<T>
cheby_approx<T>::Chebyshev_Derivative(void) const {
    if(false){
    }else if( this->c.empty() || !std::isfinite(this->xmin) || !std::isfinite(this->xmax) ){
        throw std::invalid_argument("Cannot evalute integral; approximation not yet prepared");
    }else if( this->c.size() <= 2 ){
        throw std::domain_error("Cannot evalute derivative; an insufficient number of coefficients were originally computed");
        // I believe it is still posisble to compute the derivative, but it will require special cases that are
        // of suspect relevance.
    }
    cheby_approx<T> out(*this);

    const auto n = this->c.size();
    const T con = static_cast<T>(2) / (this->xmax - this->xmin);

    out.c[n-1] = static_cast<T>(0);
    out.c[n-2] = this->c[n-1] * static_cast<T>(2*(n-1));
    if(n >= 3){
        for(size_t j = (n - 3); ; --j){
            out.c[j] = out.c[j+2] + this->c[j+1] * static_cast<T>(2*(j+1));
            if(j == 0) break;
        }
    }
    for(size_t j = 0; j < n; ++j) out.c[j] *= con;
    return out;
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template cheby_approx<float > cheby_approx<float >::Chebyshev_Derivative(void) const;
    template cheby_approx<double> cheby_approx<double>::Chebyshev_Derivative(void) const;
#endif

template <class T>
cheby_approx<T>
cheby_approx<T>::Chebyshev_Integral(void) const {

    if(false){
    }else if( this->c.empty() || !std::isfinite(this->xmin) || !std::isfinite(this->xmax) ){
        throw std::invalid_argument("Cannot evalute integral; approximation not yet prepared");
    }else if( this->c.size() <= 2 ){
        throw std::domain_error("Cannot evalute integral; an insufficient number of coefficients were originally computed");
        // I believe it is still posisble to compute the integral, but it will require special cases that are
        // of suspect relevance.
    }
    cheby_approx<T> out(*this);

    const auto n = this->c.size();
    T sum = static_cast<T>(0);
    T fac = static_cast<T>(1);
    const T con = (this->xmax - this->xmin)/static_cast<T>(4);

    for(size_t j = 1; (j + 2) <= n; ++j){
        out.c[j] = con * (this->c[j-1] - this->c[j+1]) / static_cast<T>(j);
        sum += fac * out.c[j];
        fac = -fac; // Toggle the sign.
    }
    out.c[n-1] = con * this->c[n-2] / static_cast<T>(n-1);
    sum += fac * out.c[n-1];
    out.c[0] = sum * static_cast<T>(2);
    return out;
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template cheby_approx<float > cheby_approx<float >::Chebyshev_Integral(void) const;
    template cheby_approx<double> cheby_approx<double>::Chebyshev_Integral(void) const;
#endif

template <class T>
std::vector<T>
cheby_approx<T>::Get_Coefficients(void) const {
    return this->c;
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template std::vector<float > cheby_approx<float >::Get_Coefficients(void) const;
    template std::vector<double> cheby_approx<double>::Get_Coefficients(void) const;
#endif

template <class T>
std::pair<T,T>
cheby_approx<T>::Get_Domain(void) const {
    return std::make_pair(this->xmin,this->xmax);
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template std::pair<float , float > cheby_approx<float >::Get_Domain(void) const;
    template std::pair<double, double> cheby_approx<double>::Get_Domain(void) const;
#endif


template <class T>
cheby_approx<T>
cheby_approx<T>::Fast_Approx_Multiply(const cheby_approx<T> &rhs, 
                                      size_t numb_of_c_to_use) const {

    // NOTE: This routine makes use of the general formula for products of Chebyshev polynomials:
    //             $ T_{n}(x) \cdot T_{m}(x) = \frac{1}{2} ( T_{n+m}(x) + T_{|n+m|} ) $
    //       which can be easily derived using the trigonometric representation and making use of
    //       the trigonometric identity:
    //             $ 2 \cos(a)\cos(b) = \cos(a+b) + \cos(a-b) $.
    //

    if(false){
    }else if( this->c.empty() || !std::isfinite(this->xmin) || !std::isfinite(this->xmax) ){
        throw std::invalid_argument("Cannot perform multiplication; LHS has not been prepared");
    }else if( rhs.c.empty() || !std::isfinite(rhs.xmin) || !std::isfinite(rhs.xmax) ){
        throw std::invalid_argument("Cannot perform multiplication; RHS has not been prepared");
    }else if( this->c.empty() || rhs.c.empty() ){
        throw std::invalid_argument("Cannot perform multiplication; too few coefficients were used to prepare the approximations"); 
    }else if( (this->xmin != rhs.xmin) || (this->xmax != rhs.xmax) ){
        throw std::invalid_argument("Cannot perform multiplication; domains are not exactly equal");
    }

    const auto N = static_cast<long int>(this->c.size()); 
    const auto M = static_cast<long int>(rhs.c.size()); 
    const auto P = static_cast<long int>( (numb_of_c_to_use == 0) ? std::max(N,M) : numb_of_c_to_use );

    //If too many coefficients are being requested, use the non-truncated version to compute the
    // full multiplication.
    if( P >= (N+M-1) ) return (*this) * rhs;
    if( P == 0 ) throw std::invalid_argument("Cannot construct approximation with zero coefficients.");

    std::vector<T> c(P,static_cast<T>(0));

    const T one = static_cast<T>(1);
    const T half = static_cast<T>(0.5);

    cheby_approx<T> out;

    //The following factors are used to account for the subtraction of $C_{0}/2$ in the general
    // Chebyshev polynomial expansion. 
    T ifac, jfac;

    //First, the 'sum' part.
    for(long int k = 0; k < static_cast<long int>(P); ++k){
        long int i_start = (k > (N-1)) ? (N-1)     : k;
        long int i_end   = (k > (M-1)) ? (k-(M-1)) : 0;
        long int j_start = (k > (N-1)) ? (k-(N-1)) : 0;
        long int i = i_start;
        for(long int j = j_start ; i >= i_end ; --i, ++j){
            ifac = (i==0) ? half : one;
            jfac = (j==0) ? half : one;
            c[k] += half * ifac * jfac * this->c[i] * rhs.c[j];
        }
    }

    //Second, the main diagonal of the 'difference' part.
    c[0] += half * half * half * this->c[0] * rhs.c[0];
    long int diag_max_k = std::min(N,M);
    for(long int k = 1; k < diag_max_k; ++k){
        c[0] += half * this->c[k] * rhs.c[k];
    }

    //Third, the upper diagonal 'difference' part.
    long int diag_max_P_M = std::min(P,M);
    for(long int k = 1; k < diag_max_P_M; ++k){
        long int i_start = 0;
        long int i_end   = std::min(N,M-k)-1;
        long int j_start = k;

        long int j = j_start;
        for(long int i = i_start ; i <= i_end; ++i, ++j){
            ifac = (i==0) ? half : one;
            jfac = (j==0) ? half : one;
            c[k] += half * ifac * jfac * this->c[i] * rhs.c[j];
        }
    }
    //Third, the lower diagonal 'difference' part.
    long int diag_max_P_N = std::min(P,N);
    for(long int k = 1; k < diag_max_P_N; ++k){
        long int i_start = k;
        long int j_end   = std::min(M,N-k)-1;
        long int j_start = 0;

        long int i = i_start;
        for(long int j = j_start ; j <= j_end; ++i, ++j){
            ifac = (i==0) ? half : one;
            jfac = (j==0) ? half : one;
            c[k] += half * ifac * jfac * this->c[i] * rhs.c[j];
        }
    }

    c[0] *= static_cast<T>(2);
    out.Prepare(c,xmin,xmax);
    return out;
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template cheby_approx<float > 
        cheby_approx<float >::Fast_Approx_Multiply(const cheby_approx<float > &, size_t) const;
    template cheby_approx<double> 
        cheby_approx<double>::Fast_Approx_Multiply(const cheby_approx<double> &, size_t) const;
#endif


template <class T>
cheby_approx<T>
cheby_approx<T>::Fast_Approx_Multiply(const cheby_approx<T> &rhs, 
                                      double fraction_of_max_c_to_use) const {
    if(fraction_of_max_c_to_use < 0.0){
        throw std::invalid_argument("Provided fraction was negative. Cannot be converted to size_t");
    }

    const auto N = this->c.size(); 
    const auto M = rhs.c.size(); 
    const auto P = static_cast<size_t>( std::round( fraction_of_max_c_to_use * std::max(N,M) ) );

    return this->Fast_Approx_Multiply(rhs, P);
};
#ifndef YGORMATHCHEBYSHEV_DISABLE_ALL_SPECIALIZATIONS
    template cheby_approx<float > 
        cheby_approx<float >::Fast_Approx_Multiply(const cheby_approx<float > &, double) const;
    template cheby_approx<double> 
        cheby_approx<double>::Fast_Approx_Multiply(const cheby_approx<double> &, double) const;
#endif


