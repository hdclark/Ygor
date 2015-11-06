//YgorMath.cc.

#include <cmath>       //Needed for fabs, signbit, sqrt, etc...
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>   //Needed for std::reverse.
#include <map>
#include <unordered_map>
#include <list>
#include <iterator>
#include <functional>  //Needed for passing kernel functions to integration schemes.
#include <string>      //Needed for stringification routines.
#include <tuple>       //Needed for Spearman's Rank Correlation Coeff, other statistical routines.
#include <limits>      //Needed for std::numeric_limits::max().
#include <vector>

//#include <iomanip>   //REMOVE ME - used for testing!

//#include <gsl/gsl_math.h>
//#include <gsl/gsl_sf_gamma.h>

#include "YgorMath.h"
//#include "YgorStats.h"
#include "YgorMisc.h"    //For the FUNC* and PERCENT_ERR macro functions.

#include "YgorFilesDirs.h"  //Used in samples_1D<T>::Write_To_File(...).

#include "YgorPlot.h"    //A wrapper used for producing plots of contours.
class Plotter;

#ifndef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    #define YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
#endif

//Forward declaration. Needed due to mixing of classes (find point where line and plane intersect, etc..)
template <class T> class vec3;
template <class T> class vec2;
template <class T> class line;
template <class T> class line_segment;
template <class T> class plane;
template <class T> class contour_of_points;


/*
double Stats::P_From_StudT_1Tail(double tval, double dof){
    const double y = std::sqrt(tval*tval + dof);
    const double x = 0.5*(y - tval)/y;
    const double a = dof/2.0;
    const double b = dof/2.0;

    //Regularized beta ratio function (I_x(a,b) = B_x(a,b)/B(a,b)).
    const double reg_beta_incom = gsl_sf_beta_inc(a,b,x); 
    return reg_beta_incom;
}

double Stats::P_From_StudT_2Tail(double tval, double dof){
    if(tval >= 0.0) return 2.0*Stats::P_From_StudT_1Tail(tval,dof);
    return 2.0*Stats::P_From_StudT_1Tail(-tval,dof);
}

double Stats::P_From_StudT_Diff_Means_From_Uneq_Vars(double M1, double S1, double N1, double M2, double S2, double N2){
    //Given two means (M1 and M2) with associated (possibly non-equal) variances, compute the significance
    // that the means are different. In other words, given the means, what is the probability that they 
    // are drawn from the same population? Are the means significantly different?
    //
    //Input:
    //  - M1 and M2 - the means of either distribution. I don't believe this routine is applicable to 
    //      other quantities. See numerical recipes for a nice discussion.
    //  - S1 and S2 - the sigma (ie. one std.dev. or `variance') of either distribution.
    //  - N1 and N2 - the number of points in either distribution. *NOT* the DOF!
    //
    //This routine is often used to determine if some quantity has changed significantly. Examples could be
    // a quantity changing at two points in time, or comparison of non-linear fitting parameters obtained
    // with two different techniques (ie. least-sum of squares vs. least-median of squares).
    //
    //Logically, it is similar to the area of overlap of the two distributions. Obviously, if the 
    // distributions have wildly different means or variances, this routine may not appropriate. 
    //
    //NOTE: This tests significance in the difference of means. Do not conflate significance of means
    //      with significance in difference of the distributions themselves! 
    if((N1 < 2.0) || (N2 < 2.0)) FUNCERR("Not enough points available for computation");

    const double t_num = M1 - M2;
    const double t_den = std::sqrt((S1/N1)+(S2/N2));
    if(!std::isnormal(t_den)) FUNCERR("Encountered difficulty computing Student's t-value. Cannot continue");
 
    const double dof_num = std::pow((S1/N1)+(S2/N2),2.0);
    const double dof_den = ((S1*S1)/(N1*N1*(N1-1.0))) + ((S2*S2)/(N2*N2*(N2-1.0)));
    if(!std::isnormal(dof_den)) FUNCERR("Encountered difficulty computing dof. Cannot continue");

    return Stats::P_From_StudT_2Tail(t_num/t_den, dof_num/dof_den);
}



double Stats::Q_From_ChiSq_Fit(double chi_square, double dof){
    //See Numerical Recipes, C, Section 6.2 (pp 221) or section on nonlinear fitting.
    //
    //NOTE: This function should use the chi-square which includes uncertainties.
    //NOTE: This function should not use the reduced chi-square!
    //
    // A ~decent fit has Q > 0.001 (although a little low).
    // If Q is too near 1.0, the errors are probably overestimated.
    // If Q is a little lower then 0.001, errors are probably underestimated.
    // If Q is very low (<1E-6) the model is a terrible fit.
    return gsl_sf_gamma_inc_Q(dof/2.0, chi_square/2.0);
}


double Stats::Unbiased_Var_Est(std::list<double> in){
    //This is an unbiased estimate of a population's variance, as computed from a
    // finite sample size. If you have the *entire* population (ie. every entity)
    // then this will produce a slightly incorrect value. See 
    // http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance for more
    // info.
    if(in.empty()) FUNCERR("Cannot compute variance of empty set. Unable to continue");
    if(in.size() == 1) FUNCERR("Cannot estimate variance from single point. Unable to continue");
    if(in.size() <= 5) FUNCWARN("Very few points were used to estimate variance. Be weary of result");

    in.sort(); //Sort it now and avoid having to do so again (twice) from scratch.
    double n = static_cast<double>(in.size());
    return (Ygor_Sum_Squares(in) - pow(Ygor_Sum(in),2.0)/n)/(n-1);
}

*/

double Ygor_Sum(std::list<double> in){
    in.sort(); // <-- trying to reduce gross errors from occuring during summation.
    double res = 0.0;
    for(auto it = in.begin(); it != in.end(); ++it) res += *it;
    return res;
}

double Ygor_Sum_Squares(std::list<double> in){
    in.sort(); // <-- trying to reduce gross errors from occuring during summation.
    double res = 0.0;
    for(auto it = in.begin(); it != in.end(); ++it) res += (*it)*(*it);
    return res;
}

double Ygor_Median(std::list<double> in){
    if(in.empty()) FUNCERR("Cannot compute median of empty set. Unable to continue");
    in.sort();
    size_t N = in.size();
    size_t M = N/2;
    auto it = std::next(in.begin(),M-1);
    if(2*M == N){ //N % 2 == 0){
        const auto L = *it;
        ++it;
        const auto R = *it;
        return (L+R)/2.0;
    }else if((2*M+1) != N){
        FUNCERR("Programming error...late night...tired...writing...thesis...ugh....fixme");
    }
    ++it;
    return *it;
}

double Ygor_Mean(std::list<double> in){
    //Speed? Efficiency? Careful attention to potential signal degredation? Go nuts!
    if(in.empty()) FUNCERR("Cannot compute mean of empty set. Unable to continue");

    return Ygor_Sum(in) / static_cast<double>(in.size());
}


//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------- vec3: A three-dimensional vector -------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//Constructors.
template <class T>    vec3<T>::vec3(){   x=(T)(0);   y=(T)(0);   z=(T)(0);  }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec3<int>::vec3(void);
//    template vec3<long int>::vec3(void);
    template vec3<float>::vec3(void);
    template vec3<double>::vec3(void);
#endif

template <class T>    vec3<T>::vec3(T a, T b, T c) : x(a), y(b), z(c) { }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec3<int>::vec3(int, int, int);
//    template vec3<long int>::vec3(long int, long int, long int);
    template vec3<float>::vec3(float, float, float);
    template vec3<double>::vec3(double, double, double);
#endif
    
template <class T>    vec3<T>::vec3( const vec3<T> &in ) : x(in.x), y(in.y), z(in.z) { }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec3<int>::vec3( const vec3<int> & );
//    template vec3<long int>::vec3( const vec3<long int> & );
    template vec3<float>::vec3( const vec3<float> & );
    template vec3<double>::vec3( const vec3<double> & );
#endif
    
    
//More general: (but is it needed?)
//template<class Ch,class Tr,class T>     std::basic_ostream<Ch,Tr> & operator<<( std::basic_ostream<Ch,Tr> &&out, const vec3<T> &L ){
//    out << "(" << L.x << ", " << L.y << ", " << L.z << ")";
//    return out;
//}
template <class T>    std::ostream & operator<<( std::ostream &out, const vec3<T> &L ){
    //Note: This friend is templated (Y) within the templated class (T). We only
    // care about friend template when T==Y.
    //
    //There is significant whitespace here!
    out << "(" << L.x << ", " << L.y << ", " << L.z << ")";
    return out;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template std::ostream & operator<<(std::ostream &out, const vec3<int> &L );
//    template std::ostream & operator<<(std::ostream &out, const vec3<long int> &L );
    template std::ostream & operator<<(std::ostream &out, const vec3<float> &L );
    template std::ostream & operator<<(std::ostream &out, const vec3<double> &L );
#endif
    
    
template <class T> vec3<T> vec3<T>::Cross(const vec3<T> &in) const {
    const T thex = (*this).y * in.z - (*this).z * in.y;
    const T they = (*this).z * in.x - (*this).x * in.z;
    const T thez = (*this).x * in.y - (*this).y * in.x;
    return vec3<T>( thex, they, thez );
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template vec3<float>  vec3<float>::Cross(const vec3<float> &in) const ;
    template vec3<double> vec3<double>::Cross(const vec3<double> &in) const ;
#endif
   
template <class T> vec3<T> vec3<T>::Mask(const vec3<T> &in) const {
    const T thex = this->x * in.x;
    const T they = this->y * in.y;
    const T thez = this->z * in.z;
    return vec3<T>( thex, they, thez );
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template vec3<float>  vec3<float>::Mask(const vec3<float> &in) const ;
    template vec3<double> vec3<double>::Mask(const vec3<double> &in) const ;
#endif 
    
template <class T> T vec3<T>::Dot(const vec3<T> &in) const {
    return (*this).x * in.x + (*this).y * in.y + (*this).z * in.z;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float vec3<float>::Dot(const vec3<float> &in) const;
    template double vec3<double>::Dot(const vec3<double> &in) const;
#endif
    
    
template <class T> vec3<T> vec3<T>::unit(void) const {
    const T tot = sqrt(x*x + y*y + z*z);
    return vec3<T>(x/tot, y/tot, z/tot);
} 
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template vec3<float> vec3<float>::unit(void) const;
    template vec3<double> vec3<double>::unit(void) const;
#endif
    
    
template <class T> T vec3<T>::length(void) const {
    const T tot = sqrt(x*x + y*y + z*z);
    return tot;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float vec3<float>::length(void) const;
    template double vec3<double>::length(void) const;
#endif
    
    
template <class T>  T vec3<T>::distance(const vec3<T> &rhs) const {
    const T dist = sqrt((x-rhs.x)*(x-rhs.x) + (y-rhs.y)*(y-rhs.y) + (z-rhs.z)*(z-rhs.z));
    return dist;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float vec3<float>::distance(const vec3<float> &rhs) const;
    template double vec3<double>::distance(const vec3<double> &rhs) const;
#endif
    
/*
template <class T>  T vec3<T>::distance(vec3 rhs){
    const T dist = sqrt((x-rhs.x)*(x-rhs.x) + (y-rhs.y)*(y-rhs.y) + (z-rhs.z)*(z-rhs.z));
    return dist;
}
    
template float vec3<float>::distance(vec3<float> rhs);
template double vec3<double>::distance(vec3<double> rhs);
*/
template <class T>  T vec3<T>::sq_dist(const vec3<T> &rhs) const {
    return ((x-rhs.x)*(x-rhs.x) + (y-rhs.y)*(y-rhs.y) + (z-rhs.z)*(z-rhs.z));
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float vec3<float>::sq_dist(const vec3<float> &rhs) const;
    template double vec3<double>::sq_dist(const vec3<double> &rhs) const;
#endif


template <class T>  T vec3<T>::angle(const vec3<T> &rhs, bool *OK) const {
    const bool useOK = (OK != nullptr);
    if(useOK) *OK = false;

    const auto Alen = this->length();
    const auto Blen = rhs.length();

    if( !std::isfinite((T)(1)/(Alen))
    ||  !std::isfinite((T)(1)/(Blen))
    ||  !std::isfinite((T)(1)/(Alen*Blen)) ){
        if(useOK) return (T)(-1); //If the user is handling errors.
        FUNCERR("Not possible to compute angle - one of the vectors is too short");
    }

    const auto A = this->unit();
    const auto B = rhs.unit();
    const auto AdotB = A.Dot(B); //Are they pointing along the same direction?

    const auto C = A.Cross(B);
    const auto Clen = C.length(); // == sin(angle);
    const auto principleangle = asin(Clen); // is in range [-pi/2, pi/2].
    const auto absprinangle = YGORABS(principleangle); // is in range [0, pi/2].

    T theangle;
    if(AdotB > (T)(0)){ //Pointing in same direction: angle < 90 degrees.
        theangle = absprinangle;
    }else{
        theangle = (T)(M_PI) - absprinangle;
    }
    if(useOK) *OK = true;
    return theangle;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float  vec3<float >::angle(const vec3<float > &rhs, bool *OK) const;
    template double vec3<double>::angle(const vec3<double> &rhs, bool *OK) const;
#endif
      
template <class T>    std::istream &operator>>(std::istream &in, vec3<T> &L){
    //Note: This friend is templated (Y) within the templated class (T). We only
    // care about friend template when T==Y.
    //
    //... << "("  << L.x << ", " << L.y << ", " <<  L.z  <<  ")";
    //We have at least TWO options here. We can use a method which is compatible
    // with the ( , , ) notation, or we can ask for straight-up numbers. 
    //We will discriminate here based on what 'in' is.
//    if(&in != &std::cin){                                            //Neat trick, but makes it hard to build on...
        char grbg;
        //... << "("  << L.x << ", " << L.y << ", " <<  L.z  <<  ")";
        in    >> grbg >> L.x >> grbg >> L.y >> grbg >>  L.z  >> grbg;
//    }else  in >> L.x >> L.y >> L.z;
    return in;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template std::istream & operator>>(std::istream &out, vec3<int> &L );
//    template std::istream & operator>>(std::istream &out, vec3<long int> &L );
    template std::istream & operator>>(std::istream &out, vec3<float> &L );
    template std::istream & operator>>(std::istream &out, vec3<double> &L );
#endif
     
    
template <class T>    vec3<T> & vec3<T>::operator=(const vec3<T> &rhs) {
    //Check if it is itself.
    if(this == &rhs) return *this; 
    (*this).x = rhs.x;    (*this).y = rhs.y;    (*this).z = rhs.z;
    return *this;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec3<int> & vec3<int>::operator=(const vec3<int> &rhs);
//    template vec3<long int> & vec3<long int>::operator=(const vec3<long int> &rhs);
    template vec3<float> & vec3<float>::operator=(const vec3<float> &rhs);
    template vec3<double> & vec3<double>::operator=(const vec3<double> &rhs);
#endif
    
    
template <class T>    vec3<T> vec3<T>::operator+(const vec3<T> &rhs) const {
    return vec3<T>( (*this).x + rhs.x, (*this).y + rhs.y, (*this).z + rhs.z);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec3<int> vec3<int>::operator+(const vec3<int> &rhs) const;
//    template vec3<long int> vec3<long int>::operator+(const vec3<long int> &rhs) const;
    template vec3<float> vec3<float>::operator+(const vec3<float> &rhs) const;
    template vec3<double> vec3<double>::operator+(const vec3<double> &rhs) const;
#endif

    
template <class T>    vec3<T> & vec3<T>::operator+=(const vec3<T> &rhs) {
    (*this).x += rhs.x;    (*this).y += rhs.y;    (*this).z += rhs.z;
    return *this;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec3<int> & vec3<int>::operator+=(const vec3<int> &rhs);
//    template vec3<long int> & vec3<long int>::operator+=(const vec3<long int> &rhs);
    template vec3<float> & vec3<float>::operator+=(const vec3<float> &rhs);
    template vec3<double> & vec3<double>::operator+=(const vec3<double> &rhs);
#endif
    
    
template <class T> vec3<T> vec3<T>::operator-(const vec3<T> &rhs) const {
    return vec3<T>( (*this).x - rhs.x, (*this).y - rhs.y, (*this).z - rhs.z);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec3<int> vec3<int>::operator-(const vec3<int> &rhs) const;
//    template vec3<long int> vec3<long int>::operator-(const vec3<long int> &rhs) const;
    template vec3<float> vec3<float>::operator-(const vec3<float> &rhs) const;
    template vec3<double> vec3<double>::operator-(const vec3<double> &rhs) const;
#endif

    
template <class T>    vec3<T> & vec3<T>::operator-=(const vec3<T> &rhs) {
    (*this).x -= rhs.x;    (*this).y -= rhs.y;    (*this).z -= rhs.z;
    return *this;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec3<int> & vec3<int>::operator-=(const vec3<int> &rhs);
//    template vec3<long int> & vec3<long int>::operator-=(const vec3<long int> &rhs);
    template vec3<float> & vec3<float>::operator-=(const vec3<float> &rhs);
    template vec3<double> & vec3<double>::operator-=(const vec3<double> &rhs);
#endif
    
//------------------------------ overloaded native-types -----------------------------

/*
template <class T>    vec3<T> vec3<T>::operator*(const T rhs) {
    return vec3<T>(x*rhs,y*rhs,z*rhs);
}
template <class T>    vec3<T> & vec3<T>::operator*=(const T rhs) {
    (*this).x *= rhs;    (*this).y *= rhs;    (*this).z *= rhs;
     return *this;
}
template vec3<int> & vec3<int>::operator*=(const int rhs);
template vec3<float> & vec3<float>::operator*=(const float rhs);
template vec3<double> & vec3<double>::operator*=(const double rhs);



template <class T>    vec3<T> vec3<T>::operator/(const T rhs) {
    return vec3<T>(x/rhs,y/rhs,z/rhs);
}
template <class T>    vec3<T> & vec3<T>::operator/=(const T rhs) {
    (*this).x /= rhs;    (*this).y /= rhs;    (*this).z /= rhs;
     return *this;
}
template vec3<int> & vec3<int>::operator/=(const int rhs);
template vec3<float> & vec3<float>::operator/=(const float rhs);
template vec3<double> & vec3<double>::operator/=(const double rhs);
*/

//--------

template <class T>    vec3<T> vec3<T>::operator*(const T &rhs) const {
    return vec3<T>(x*rhs,y*rhs,z*rhs);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec3<int> vec3<int>::operator*(const int &rhs) const;
//    template vec3<long int> vec3<long int>::operator*(const long int &rhs) const;
    template vec3<float> vec3<float>::operator*(const float &rhs) const;
    template vec3<double> vec3<double>::operator*(const double &rhs) const;
#endif
    
template <class T>    vec3<T> & vec3<T>::operator*=(const T &rhs) {
    (*this).x *= rhs;    (*this).y *= rhs;    (*this).z *= rhs;
    return *this;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec3<int> & vec3<int>::operator*=(const int &rhs);
//    template vec3<long int> & vec3<long int>::operator*=(const long int &rhs);
    template vec3<float> & vec3<float>::operator*=(const float &rhs);
    template vec3<double> & vec3<double>::operator*=(const double &rhs);
#endif
    
    
    
template <class T>    vec3<T> vec3<T>::operator/(const T &rhs) const {
    return vec3<T>(x/rhs,y/rhs,z/rhs);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec3<int> vec3<int>::operator/(const int &rhs) const;
//    template vec3<long int> vec3<long int>::operator/(const long int &rhs) const;
    template vec3<float> vec3<float>::operator/(const float &rhs) const;
    template vec3<double> vec3<double>::operator/(const double &rhs) const;
#endif
    
template <class T>    vec3<T> & vec3<T>::operator/=(const T &rhs) {
    (*this).x /= rhs;    (*this).y /= rhs;    (*this).z /= rhs;
    return *this;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec3<int> & vec3<int>::operator/=(const int &rhs);
//    template vec3<long int> & vec3<long int>::operator/=(const long int &rhs);
    template vec3<float> & vec3<float>::operator/=(const float &rhs);
    template vec3<double> & vec3<double>::operator/=(const double &rhs);
#endif
    
    
    
template <class T>    bool vec3<T>::operator==(const vec3<T> &rhs) const {
    //There are, of course, varying degrees of equality for floating-point values.
    //
    //Typically the safest approach is an exact bit-wise equality using ==. This is
    // the least flexible but most reliable. 
    return ( (x == rhs.x) && (y == rhs.y) && (z == rhs.z) );


    //Define a maximum relative difference threshold. If above this, the numbers
    // are different. If below, they are 'equal' (to the threshold).
    //
    //NOTE: This will be unsuitable for some situations. If you've come here to
    // change this value: DON'T! Instead, define an auxiliary class or routine to
    // handle your special case!
    static const T MAX_REL_DIFF(1E-6);

    //Perfect, bit-wise match. Sometimes even copying will render this useless.
    if((x == rhs.x) && (y == rhs.y) && (z == rhs.z)) return true;
    
    //Relative difference match. Should handle zeros and low-near-zeros (I think). 
    // Macro RELATIVE_DIFF is currently defined in YgorMisc.h (Feb. 2013).
    if((RELATIVE_DIFF(x, rhs.x) < MAX_REL_DIFF) 
       && (RELATIVE_DIFF(y, rhs.y) < MAX_REL_DIFF)  
       && (RELATIVE_DIFF(z, rhs.z) < MAX_REL_DIFF)) return true;

    return false;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template bool vec3<int>::operator==(const vec3<int> &rhs) const;
//    template bool vec3<long int>::operator==(const vec3<long int> &rhs) const;
    template bool vec3<float>::operator==(const vec3<float> &rhs) const;
    template bool vec3<double>::operator==(const vec3<double> &rhs) const;
#endif
   
template <class T>    bool vec3<T>::operator!=(const vec3<T> &rhs) const {
    return !( *this == rhs );
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template bool vec3<int>::operator!=(const vec3<int> &rhs) const;
//    template bool vec3<long int>::operator!=(const vec3<long int> &rhs) const;
    template bool vec3<float>::operator!=(const vec3<float> &rhs) const;
    template bool vec3<double>::operator!=(const vec3<double> &rhs) const;
#endif
 
    
template <class T>    bool vec3<T>::operator<(const vec3<T> &rhs) const {
    //This is the default, fairly-logical solution. If equality is NOT bit-wise
    // equality, this should be changed to something which can account for the 
    // range of equality. See operator== for more info.
    return this->length() < rhs.length();

    //Since we are using floating point numbers, we should check for equality before making a 
    // consensus of less-than.
    if(*this == rhs) return false;
    return this->length() < rhs.length();

    //NOTE: Although this is a fairly "unsatisfying" result, it appears to properly allow 
    // vec3's to be placed in std::maps, whereas more intuitive methods (x<rhs.x, etc..) do NOT. 
    // If an actual operator< is to be defined, please do NOT overwrite this one (so that we 
    // can continue to put vec3's into std::map and not have garbled output and weird bugs!) 
    //
    //return ( (y < rhs.y) ); //  <--- BAD! (See previous note above ^)
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template bool vec3<int>::operator<(const vec3<int> &rhs) const;
//    template bool vec3<long int>::operator<(const vec3<long int> &rhs) const;
    template bool vec3<float>::operator<(const vec3<float> &rhs) const;
    template bool vec3<double>::operator<(const vec3<double> &rhs) const;
#endif
    
//-------------------------------------------------------------------------------------------------------------------------------------
//This is a function for rotation unit vectors in some plane. It requires angles to describe the plane of rotation, angle of rotation. 
// It alo requires a unit vector with which to rotate the plane about.
vec3<double> rotate_unit_vector_in_plane(const vec3<double> &A, const double &theta, const double &R){
    // A    --- The unit vector which defines the central axis of the plane. We rotate about this vector to make a plane.
    // theta -- The angle of rotation for a unit vector within the plane.
    // R    --- An angle from [0:2*pi] which specifies the second axis of the plane (along with A.)

    //###########################################################################################
    //## This method is NOT ideal. It is expensive AND brittle. Fix it with a better solution. ##    ---> it came from "Project - Transport"
    //###########################################################################################

    std::complex<double> a1 = (fabs(A.x) > 1E-11) ? ( (fabs(A.x) < (1.0-1E-10)) ? A.x :  A.x - 1E-10 ) : 1E-11 + A.x;    // ~~ A.x
    std::complex<double> a2 = (fabs(A.y) > 1E-11) ? ( (fabs(A.y) < (1.0-1E-10)) ? A.y :  A.y - 1E-10 ) : 1E-11 + A.y;    // ~~ A.y
    std::complex<double> a3 = (fabs(A.z) > 1E-11) ? ( (fabs(A.z) < (1.0-1E-10)) ? A.z :  A.z - 1E-10 ) : 1E-11 + A.z;    // ~~ A.z

    static const std::complex<double> i(0.0,1.0);

    std::complex<double> p, t; //Angles.

    if(fabs(A.z) < 0.75){ //Handles special cases. Doesn't do so safely, though!

    //Now, given the rotation angle and the unit vector coordinates of A, we generate a unit vector in the plane orthogonal to A.   
    p  = (R > 1E-11) ? R : 1E-11 + R;  // ~~ R

    //Two solutions for t when fixing p. Pick one (I think they correspond to the plus/minus orientation, which should be irrelevant here.)
    t  = -i*log(-1.0*pow(a2*pow(M_E,2.0*i*p)+i*a1*pow(M_E,2.0*i*p)-1.0*a2-1.0*i*a1,-1.0)*pow(pow(a3,2.0)*pow(M_E,4.0*i*p)+pow(a2,2.0)*pow(M_E,4.0*i*p)+pow(a1,2.0)*pow(M_E,4.0*i*p)+2.0*pow(a3,2.0)*pow(M_E,2.0*i*p)-2.0*pow(a2,2.0)*pow(M_E,2.0*i*p)-2.0*pow(a1,2.0)*pow(M_E,2.0*i*p)+pow(a3,2.0)+pow(a2,2.0)+pow(a1,2.0),0.5)+a3*pow(M_E,2.0*i*p)*pow(a2*pow(M_E,2.0*i*p)+i*a1*pow(M_E,2.0*i*p)-1.0*a2-1.0*i*a1,-1.0)+a3*pow(a2*pow(M_E,2.0*i*p)+i*a1*pow(M_E,2.0*i*p)-1.0*a2-1.0*i*a1,-1.0));

    //t  = -i*log(pow(a2*pow(M_E,2.0*i*p)+i*a1*pow(M_E,2.0*i*p)-1.0*a2-1.0*i*a1,-1.0)*pow(pow(a3,2.0)*pow(M_E,4.0*i*p)+pow(a2,2.0)*pow(M_E,4.0*i*p)+pow(a1,2.0)*pow(M_E,4.0*i*p)+2.0*pow(a3,2.0)*pow(M_E,2.0*i*p)-2.0*pow(a2,2.0)*pow(M_E,2.0*i*p)-2.0*pow(a1,2.0)*pow(M_E,2.0*i*p)+pow(a3,2.0)+pow(a2,2.0)+pow(a1,2.0),0.5)+a3*pow(M_E,2.0*i*p)*pow(a2*pow(M_E,2.0*i*p)+i*a1*pow(M_E,2.0*i*p)-1.0*a2-1.0*i*a1,-1.0)+a3*pow(a2*pow(M_E,2.0*i*p)+i*a1*pow(M_E,2.0*i*p)-1.0*a2-1.0*i*a1,-1.0));


    }else{
    
        t = R;

        //We are going to use least-significant bit selection here. Ugh... Of COURSE this is not portable. Endianness breaks it, if not something else.
        union {
            double the_number;
            char   asChars[sizeof(double)];
        };
        the_number = R;

        //This time we cannot simply ignore one of the solutions, lest we have a one-sided universe...
     
        // http://stackoverflow.com/questions/4743115/how-do-i-use-bitwise-operators-on-a-double-on-c
 
        //if(asChars[sizeof(double) - 1] & 0x1){
        if(asChars[0] & 0x1){  //Least significant bit selection. Sorry, if you are reading this, for such a shit solution :(
            p = -i*log(-1.0*pow(a2*pow(M_E,2.0*i*t)*pow(a2*pow(M_E,2.0*i*t)+i*a1*pow(M_E,2.0*i*t)-2.0*a3*pow(M_E,i*t)-1.0*a2+i*a1,-1.0)+i*a1*pow(M_E,2.0*i*t)*pow(a2*pow(M_E,2.0*i*t)+i*a1*pow(M_E,2.0*i*t)-2.0*a3*pow(M_E,i*t)-1.0*a2+i*a1,-1.0)+2.0*a3*pow(M_E,i*t)*pow(a2*pow(M_E,2.0*i*t)+i*a1*pow(M_E,2.0*i*t)-2.0*a3*pow(M_E,i*t)-1.0*a2+i*a1,-1.0)-1.0*a2*pow(a2*pow(M_E,2.0*i*t)+i*a1*pow(M_E,2.0*i*t)-2.0*a3*pow(M_E,i*t)-1.0*a2+i*a1,-1.0)+i*a1*pow(a2*pow(M_E,2.0*i*t)+i*a1*pow(M_E,2.0*i*t)-2.0*a3*pow(M_E,i*t)-1.0*a2+i*a1,-1.0),0.5));
        }else{
            p = -0.5*i*log(a2*pow(M_E,2.0*i*t)*pow(a2*pow(M_E,2.0*i*t)+i*a1*pow(M_E,2.0*i*t)-2.0*a3*pow(M_E,i*t)-1.0*a2+i*a1,-1.0)+i*a1*pow(M_E,2.0*i*t)*pow(a2*pow(M_E,2.0*i*t)+i*a1*pow(M_E,2.0*i*t)-2.0*a3*pow(M_E,i*t)-1.0*a2+i*a1,-1.0)+2.0*a3*pow(M_E,i*t)*pow(a2*pow(M_E,2.0*i*t)+i*a1*pow(M_E,2.0*i*t)-2.0*a3*pow(M_E,i*t)-1.0*a2+i*a1,-1.0)-1.0*a2*pow(a2*pow(M_E,2.0*i*t)+i*a1*pow(M_E,2.0*i*t)-2.0*a3*pow(M_E,i*t)-1.0*a2+i*a1,-1.0)+i*a1*pow(a2*pow(M_E,2.0*i*t)+i*a1*pow(M_E,2.0*i*t)-2.0*a3*pow(M_E,i*t)-1.0*a2+i*a1,-1.0));
        }
    }

    double u1;
    double u2;
    double u3;

    u1 = ( cos(t)*sin(p) ).real();
    u2 = ( sin(t)*sin(p) ).real();
    u3 = ( cos(p) ).real();

    //Note that taking the real part messes up the normalization, leading to some fairly funky 'strands' in the rotated vector.
    // If you sample theta but leave A and R static, you should see these strands if you remove the following renormalization.
    const double utot = sqrt( u1*u1 + u2*u2 + u3*u3 );
    u1 /= utot;
    u2 /= utot;
    u3 /= utot;

    //Now we rotate by the angle provided.
    const double out_x = cos(theta)*A.x + sin(theta)*u1;
    const double out_y = cos(theta)*A.y + sin(theta)*u2;
    const double out_z = cos(theta)*A.z + sin(theta)*u3;

    //Now we ship out the vector.
    return vec3<double>( out_x, out_y, out_z );

}


//This function evolves a pair of position and velocity (x(t=0),v(t=0)) to a pair (x(t=T),v(t=T)) using the
// classical expression for a time- and position-dependent force F(x;t). It is highly unstable, so the number
// of iterations must be specified. If this is going to be used for anything important, make sure that the
// number of iterations is chosen sufficiently high so as to produce negligible errors.
std::tuple<vec3<double>,vec3<double>> Evolve_x_v_over_T_via_F(const std::tuple<vec3<double>,vec3<double>> &x_and_v, 
                                                              std::function<vec3<double>(vec3<double> x, double T)> F,  
                                                              double T, long int steps){
    std::tuple<vec3<double>,vec3<double>> out(x_and_v), last(x_and_v);
    const double m = 1.0;

    if(steps <= 0) FUNCERR("Unable to evolve x and v - the number of steps specified is impossible");
    //if(T <= 0.0) ...   This is OK!
    if(!F) FUNCERR("Given function F is not valid. Unable to do anything");

    const double dt = T/static_cast<double>(steps);

    for(long int i=0; i<steps; ++i){
        const auto curr_t = static_cast<double>(i)*dt;
        const auto F_at_last_x_curr_t = F( std::get<0>(last), curr_t );

        //Update V.
        std::get<1>(out) = F_at_last_x_curr_t*(dt/m) + std::get<1>(last);
 
        //Use current V to update X.
        std::get<0>(out) = std::get<1>(out)*dt + std::get<0>(last);

        //Store the old values.
        std::get<0>(last) = std::get<0>(out);
        std::get<1>(last) = std::get<1>(out);
    }

    return std::move(out);
}


//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------- vec2: A three-dimensional vector -------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//Constructors.
template <class T>    vec2<T>::vec2(){   x=(T)(0);   y=(T)(0); }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec2<int>::vec2(void);
//    template vec2<long int>::vec2(void);
    template vec2<float>::vec2(void);
    template vec2<double>::vec2(void);
#endif

template <class T>    vec2<T>::vec2(T a, T b) : x(a), y(b) { }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec2<int>::vec2(int, int);
//    template vec2<long int>::vec2(long int, long int);
    template vec2<float>::vec2(float, float);
    template vec2<double>::vec2(double, double);
#endif
    
template <class T>    vec2<T>::vec2( const vec2<T> &in ) : x(in.x), y(in.y) { }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec2<int>::vec2( const vec2<int> & );
//    template vec2<long int>::vec2( const vec2<long int> & );
    template vec2<float>::vec2( const vec2<float> & );
    template vec2<double>::vec2( const vec2<double> & );
#endif
    
    
//More general: (but is it needed?)
//template<class Ch,class Tr,class T>     std::basic_ostream<Ch,Tr> & operator<<( std::basic_ostream<Ch,Tr> &&out, const vec2<T> &L ){
//    out << "(" << L.x << ", " << L.y << ", " << L.z << ")";
//    return out;
//}
template <class T>    std::ostream & operator<<( std::ostream &out, const vec2<T> &L ) {
    //Note: This friend is templated (Y) within the templated class (T). We only
    // care about friend template when T==Y.
    //
    //There is significant whitespace here!
    out << "(" << L.x << ", " << L.y << ")";
    return out;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template std::ostream & operator<<(std::ostream &out, const vec2<int> &L );
//    template std::ostream & operator<<(std::ostream &out, const vec2<long int> &L );
    template std::ostream & operator<<(std::ostream &out, const vec2<float> &L );
    template std::ostream & operator<<(std::ostream &out, const vec2<double> &L );
#endif
    
    
template <class T> T vec2<T>::Dot(const vec2<T> &in) const {
    return (*this).x * in.x + (*this).y * in.y;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float vec2<float>::Dot(const vec2<float> &in) const;
    template double vec2<double>::Dot(const vec2<double> &in) const;
#endif
   
template <class T> vec2<T> vec2<T>::Mask(const vec2<T> &in) const {
    const T thex = this->x * in.x; 
    const T they = this->y * in.y;
    return vec2<T>( thex, they );
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template vec2<float > vec2<float >::Mask(const vec2<float > &in) const ;
    template vec2<double> vec2<double>::Mask(const vec2<double> &in) const ;
#endif
 
    
template <class T> vec2<T> vec2<T>::unit(void) const {
    const T tot = sqrt(x*x + y*y);
    return vec2<T>(x/tot, y/tot);
} 
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template vec2<float> vec2<float>::unit(void) const;
    template vec2<double> vec2<double>::unit(void) const;
#endif
    
    
template <class T> T vec2<T>::length(void) const {
    const T tot = sqrt(x*x + y*y);
    return tot;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float vec2<float>::length(void) const;
    template double vec2<double>::length(void) const;
#endif
    
    
template <class T>  T vec2<T>::distance(const vec2<T> &rhs) const {
    const T dist = sqrt((x-rhs.x)*(x-rhs.x) + (y-rhs.y)*(y-rhs.y));
    return dist;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float vec2<float>::distance(const vec2<float> &rhs) const;
    template double vec2<double>::distance(const vec2<double> &rhs) const;
#endif
   
template <class T>  T vec2<T>::sq_dist(const vec2<T> &rhs) const {
    return ((x-rhs.x)*(x-rhs.x) + (y-rhs.y)*(y-rhs.y));
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float  vec2<float >::sq_dist(const vec2<float > &rhs) const;
    template double vec2<double>::sq_dist(const vec2<double> &rhs) const;
#endif
 
template <class T>    std::istream &operator>>( std::istream &in, vec2<T> &L ) {
    //Note: This friend is templated (Y) within the templated class (T). We only
    // care about friend template when T==Y.
    //
    //... << "("  << L.x << ", " << L.y << ", " <<  L.z  <<  ")";
    //We have at least TWO options here. We can use a method which is compatible
    // with the ( , , ) notation, or we can ask for straight-up numbers. 
    //We will discriminate here based on what 'in' is.
//    if(&in != &std::cin){                                   //Neat trick, but makes it hard to build on..
        char grbg;
        //... << "("  << L.x << ", " << L.y <<  ")";
        in    >> grbg >> L.x >> grbg >> L.y >> grbg;
//    }else  in >> L.x >> L.y;
    return in;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template std::istream & operator>>(std::istream &out, vec2<int> &L );
//    template std::istream & operator>>(std::istream &out, vec2<long int> &L );
    template std::istream & operator>>(std::istream &out, vec2<float> &L );
    template std::istream & operator>>(std::istream &out, vec2<double> &L );
#endif
    
    
template <class T>    vec2<T> & vec2<T>::operator=(const vec2<T> &rhs) {
    //Check if it is itself.
    if (this == &rhs) return *this; 
    (*this).x = rhs.x;    (*this).y = rhs.y;
    return *this;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec2<int> & vec2<int>::operator=(const vec2<int> &rhs);
//    template vec2<long int> & vec2<long int>::operator=(const vec2<long int> &rhs);
    template vec2<float> & vec2<float>::operator=(const vec2<float> &rhs);
    template vec2<double> & vec2<double>::operator=(const vec2<double> &rhs);
#endif
    
    
template <class T>    vec2<T> vec2<T>::operator+(const vec2<T> &rhs) const {
    return vec2<T>( (*this).x + rhs.x, (*this).y + rhs.y );
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec2<int> vec2<int>::operator+(const vec2<int> &rhs) const;
//    template vec2<long int> vec2<long int>::operator+(const vec2<long int> &rhs) const;
    template vec2<float> vec2<float>::operator+(const vec2<float> &rhs) const;
    template vec2<double> vec2<double>::operator+(const vec2<double> &rhs) const;
#endif

    
template <class T>    vec2<T> & vec2<T>::operator+=(const vec2<T> &rhs) {
    (*this).x += rhs.x;    (*this).y += rhs.y;
    return *this;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec2<int> & vec2<int>::operator+=(const vec2<int> &rhs);
//    template vec2<long int> & vec2<long int>::operator+=(const vec2<long int> &rhs);
    template vec2<float> & vec2<float>::operator+=(const vec2<float> &rhs);
    template vec2<double> & vec2<double>::operator+=(const vec2<double> &rhs);
#endif
    
    
template <class T> vec2<T> vec2<T>::operator-(const vec2<T> &rhs) const {
    return vec2<T>( (*this).x - rhs.x, (*this).y - rhs.y);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec2<int> vec2<int>::operator-(const vec2<int> &rhs) const;
//    template vec2<long int> vec2<long int>::operator-(const vec2<long int> &rhs) const;
    template vec2<float> vec2<float>::operator-(const vec2<float> &rhs) const;
    template vec2<double> vec2<double>::operator-(const vec2<double> &rhs) const;
#endif

    
template <class T>    vec2<T> & vec2<T>::operator-=(const vec2<T> &rhs) {
    (*this).x -= rhs.x;    (*this).y -= rhs.y;
    return *this;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec2<int> & vec2<int>::operator-=(const vec2<int> &rhs);
//    template vec2<long int> & vec2<long int>::operator-=(const vec2<long int> &rhs);
    template vec2<float> & vec2<float>::operator-=(const vec2<float> &rhs);
    template vec2<double> & vec2<double>::operator-=(const vec2<double> &rhs);
#endif
    
//------------------------------ overloaded native-types -----------------------------


template <class T>    vec2<T> vec2<T>::operator*(const T &rhs) const {
    return vec2<T>(x*rhs,y*rhs);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec2<int> vec2<int>::operator*(const int &rhs) const;
//    template vec2<long int> vec2<long int>::operator*(const long int &rhs) const;
    template vec2<float> vec2<float>::operator*(const float &rhs) const;
    template vec2<double> vec2<double>::operator*(const double &rhs) const;
#endif
    
template <class T>    vec2<T> & vec2<T>::operator*=(const T &rhs) {
    (*this).x *= rhs;    (*this).y *= rhs;
    return *this;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec2<int> & vec2<int>::operator*=(const int &rhs);
//    template vec2<long int> & vec2<long int>::operator*=(const long int &rhs);
    template vec2<float> & vec2<float>::operator*=(const float &rhs);
    template vec2<double> & vec2<double>::operator*=(const double &rhs);
#endif
    
template <class T>    vec2<T> vec2<T>::operator/(const T &rhs) const {
    return vec2<T>(x/rhs,y/rhs);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec2<int> vec2<int>::operator/(const int &rhs) const;
//    template vec2<long int> vec2<long int>::operator/(const long int &rhs) const;
    template vec2<float> vec2<float>::operator/(const float &rhs) const;
    template vec2<double> vec2<double>::operator/(const double &rhs) const;
#endif
    
template <class T>    vec2<T> & vec2<T>::operator/=(const T &rhs) {
    (*this).x /= rhs;    (*this).y /= rhs;
    return *this;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template vec2<int> & vec2<int>::operator/=(const int &rhs);
//    template vec2<long int> & vec2<long int>::operator/=(const long int &rhs);
    template vec2<float> & vec2<float>::operator/=(const float &rhs);
    template vec2<double> & vec2<double>::operator/=(const double &rhs);
#endif
    
    
    
template <class T>    bool vec2<T>::operator==(const vec2<T> &rhs) const {
    return ( (x == rhs.x) && (y == rhs.y) );
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template bool vec2<int>::operator==(const vec2<int> &rhs) const;
//    template bool vec2<long int>::operator==(const vec2<long int> &rhs) const;
    template bool vec2<float>::operator==(const vec2<float> &rhs) const;
    template bool vec2<double>::operator==(const vec2<double> &rhs) const;
#endif
   
template <class T>    bool vec2<T>::operator!=(const vec2<T> &rhs) const {
    return !( *this == rhs );
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template bool vec2<int>::operator!=(const vec2<int> &rhs) const;
//    template bool vec2<long int>::operator!=(const vec2<long int> &rhs) const;
    template bool vec2<float>::operator!=(const vec2<float> &rhs) const;
    template bool vec2<double>::operator!=(const vec2<double> &rhs) const;
#endif
 
    
template <class T>    bool vec2<T>::operator<(const vec2<T> &rhs) const {
    return (*this).length() < rhs.length();  // NOTE: Although this is a fairly "unsatisfying" result, it appears to properly allow vec2's to be placed in std::maps, whereas more intuitive methods (x<rhs.x, etc..) do NOT. 
                                             // If an actual operator< is to be defined, please do NOT overwrite this one (so that we can continue to put vec2's into std::map and not have garbled output and weird bugs!) 
//    return ( (y < rhs.y) ); //  <--- BAD! (See previous note above ^)
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template bool vec2<int>::operator<(const vec2<int> &rhs) const;
//    template bool vec2<long int>::operator<(const vec2<long int> &rhs) const;
    template bool vec2<float>::operator<(const vec2<float> &rhs) const;
    template bool vec2<double>::operator<(const vec2<double> &rhs) const;
#endif
    


//---------------------------------------------------------------------------------------------------------------------------
//----------------------------------------- line: (infinitely-long) lines in 3D space ---------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//Constructors.
template <class T>    line<T>::line(){ }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template line<float>::line(void);
    template line<double>::line(void);
#endif

template <class T>    line<T>::line(const vec3<T> &R_A, const vec3<T> &R_B) : R_0(R_A) {
    vec3<T> temp(R_B);
    temp -= R_A;
    U_0 = temp.unit();
} 
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template line<float>::line(const vec3<float> &R_A, const vec3<float> &R_B);
    template line<double>::line(const vec3<double> &R_A, const vec3<double> &R_B);
#endif


//Member functions.

/*
//This function takes a point and a unit vector along each line and computes the point at which they intersect. If they diverge, the function returns false. Otherwise,
// it returns true and places the point of intersection in "output."
//
// If the function returns false, it does not imply that the lines diverge - it implies only that the solution computed with this method was unstable!
//
//This function accepts 3D vectors, but only uses the x and y parts. The z-component is entirely ignored.
template <class T>  bool line<T>::Intersects_With_Line_Once( const line<T> &in, vec3<T> &out) const {

    //------------
    // Speculation on how to extend this result to a fully-3D result:
    //   step 1 - find the plane which intersects with line 1 infinitely-many places and which only intersects in one place for line 2.
    //   step 2 - determine the point in this plane where line 2 intersects.
    //   step 3 - determine if line 1 and this point coincide.
    //                   If they coincide, then the lines coincide. If any difficulties arise or line 1 and the point do not coincide, then the lines do not coincide.
    //------------


    //If the pivot point R_0 of each line is identical, then we say an intersection occurs there.
    if( (*this).R_0 == in.R_0 ){
        if( !((*this).U_0 == in.U_0) ){
            out = in.R_0;
            return true;
        }
        //The lines overlap! We have an infinite number of solutions, so we pretend it is unsolveable.
        FUNCWARN("Attempting to determine intersection point of two identical lines. Pretending they do not intersect!");
        return false;
    }

    //If the two lines are not in the same z-plane, then the following routine is insufficient!
    if( ((*this).R_0.z == in.R_0.z) || ((*this).U_0.z != (T)(0)) || (in.U_0.z != (T)(0)) ){
        FUNCWARN("This function can not handle fully-3D lines. Lines which do not have a constant z-component are not handled. Continuing and indicating that we could not determine the point of intersection");
        return false;
    }

    //We parametrize each line like (R(t) = point + unit*t) and attempt to determine t for each line.
    // From Maxima:
    //   solve([u1x*t1 - u2x*t2 = Cx, u1y*t1 - u2y*t2 = Cy], [t1,t2]);
    //   --->  [[t1=(Cy*u2x-Cx*u2y)/(u1y*u2x-u1x*u2y) , t2=(Cy*u1x-Cx*u1y)/(u1y*u2x-u1x*u2y)]]
    const T denom = ((*this).U_0.y*in.U_0.x - (*this).U_0.x*in.U_0.y);
    if(fabs(denom) < (T)(1E-99)){
        FUNCWARN("Unable to compute the intersection of two lines. Either the lines do not converge, or the tolerances are set too high. Continuing and indicating that we could not determine the point of intersection");
        return false;
    }

    const T Cx       = (in.R_0.x - (*this).R_0.x);
    const T Cy       = (in.R_0.y - (*this).R_0.y);
    const T numer_t1 = (Cy*in.U_0.x      - Cx*in.U_0.y     );
    const T numer_t2 = (Cy*(*this).U_0.x - Cx*(*this).U_0.y);
    const T t1       = numer_t1 / denom;
    const T t2       = numer_t2 / denom;

    //Now we could (should) check if the two t's lead to consistent results. This is not done at the moment because I will surely have nicely-orthogonal lines that will definately intersect nicely.
    //out = vec3<double>( (*this).R_0.x + (*this).U_0.x*t1, (*this).R_0.y + (*this).U_0.y*t1, (*this).R_0.z  );
    out.x = (*this).R_0.x + (*this).U_0.x*t1;
    out.y = (*this).R_0.y + (*this).U_0.y*t1;
    out.z = (*this).R_0.z;
    return true;
}
*/

//This function computes the distance from any line to any point in 3D space.
template <class T>  T line<T>::Distance_To_Point( const vec3<T> &R ) const {
    //This is a fairly simple result. Check out http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html for a slightly
    // overtly-difficult description of the derivation.
    const vec3<T> dR = R - (*this).R_0;
    return  ( dR.Cross( dR - (*this).U_0 ) ).length();
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float line<float>::Distance_To_Point( const vec3<float> &R ) const;
    template double line<double>::Distance_To_Point( const vec3<double> &R ) const;
#endif

/*
//This function accepts any line embedded in 3D space.
template <class T>  bool line<T>::Intersects_With_Line_Once( const line<T> &in, vec3<T> &out) const {
    //First, we construct a plane which houses the unit vectors of the two lines.
    // This will give us two planes: $\vec{N} \cdot ( \vec{R} - \vec{R}_{a,0} )$ and $\vec{N} \cdot ( \vec{R} - \vec{R}_{b,0} )$
    // where $\vec{N} = \vec{U}_{a} \otimes \vec{U}_{b}.$ Since the planes are parallel, we just compute the distance between planes. 
    const vec3<T> N( this->U_0.Cross( in.U_0 ) );
    //FUNCINFO("The cross product of the unit vectors " << (*this).U_0 << " and " << in.U_0  << " of the lines is " << N);

FUNCWARN("This functions requires a code review!");

    if(N.length() < (T)(1E-9) ){
        //I might be wrong (very tired right now) but I think this means there are either infinite solutions or none. Either way, we cannot
        // compute them, so we just return a big, fat false.
        return false;
    }

    //The distance between planes can be computed as the distance from a single point on one plane to the other plane. (We know R_0 is on the plane.)
    const plane<T> plane_b( N, in.R_0 );
    const T separation = std::fabs( plane_b.Get_Signed_Distance_To_Point( (*this).R_0 ) ); 
    //FUNCINFO("The separation between planes is " << separation);

    // Explicitly, the signed distance is   dist = (u_a x u_b) dot (R_a - R_b), which removes the need to compute the plane...

    if( separation < (T)(1E-9) ){
        //Determine the point of intersection here.
        // First, we set the two line equations equal to each other in order to determine the parametrization ta and tb where they intersect.
        // Then we dot each (vector) equation with alternatively U_0,a and U_0,b to give us two equations for two unknowns (instead of three
        // equations and two unknowns with a ghost parameter.) The extra piece of information was used during the calculation of plane 
        // separation: ie. we have not lost any info by dotting both sides of the identity to reduce the dimensionality.
        const vec3<T> dR( (*this).R_0 - in.R_0 ); //dR = R_0_a - R_0_b
        const T udotu = (*this).U_0.Dot( in.U_0 );
        const T denom = (T)(1.0) - udotu*udotu;
        if(denom < (T)(1E-9)) return false; //Is this line required, given that we know ua x ub to be nearly zero ?

        //For a line, we only need to compute one of these. For a line segment, we'll need both so we can range-check.
        const T ta = -( ( (*this).U_0 - ( in.U_0 * (udotu) ) ).Dot( dR ) ) / denom;
        //const T tb =  ( ( in.U_0 - ( (*this).U_0 * (udotu) ) ).Dot( dR ) ) / denom; 

        out = (*this).R_0 + ( ( (*this).U_0 ) * ta );
        return true;
    }
    return false;
}
*/

//This function accepts any line embedded in 3D space.
//
//NOTE: If 'false' is returned, then the vec3 passed in will be undefined.
template <class T>  bool line<T>::Intersects_With_Line_Once( const line<T> &in, vec3<T> &out) const {
    //This function assumes the the lines are well-formed and that U's are normalized to unit length.
    if(this->U_0 == in.U_0){
        //This means there are either an infinite number of intersections or none. Neither of these applies.
        return false;
    }
   
    //Construct a token plane out of the two unit vectors. We will duplicate and shift the plane to encompass 
    // each line's arbitrary point. This will give us two planes
    //     $\vec{N} \cdot ( \vec{R} - \vec{R}_{a,0} )$   and   $\vec{N} \cdot ( \vec{R} - \vec{R}_{b,0} )$
    // where $\vec{N} = \vec{U}_{a} \otimes \vec{U}_{b}.$ 
    // Since the planes are parallel, we just compute the distance between planes. 
    const vec3<T> N(this->U_0.Cross(in.U_0)); //NOT of unit length!

/*
    if(N.length() < (T)(1E-9) ){  //Too arbitrary - any better way??
        //I might be wrong (very tired right now) but I think this means there are either infinite solutions or none. Either way, we cannot
        // compute them, so we just return a big, fat false.
        return false;
    }
*/

    //The distance between planes can be computed as the distance from a single point on one plane to the other plane.
    // We know R_0 is within one of the planes, so we can actually avoid computing it.
    const plane<T> plane_b(N.unit(), in.R_0);
    const T separation = std::fabs(plane_b.Get_Signed_Distance_To_Point(this->R_0)); 

    //Check if the planes are separated by a reasonable distance or not. If the separation is too great, we
    // believe the lines do not intersect. 
    //
    //To judge if the lines intersect, we examine whether the separation is a normal floating point number.
    // A normal is NOT a nan, zero, inf, or subnormal. This is quite arbitrary, but is a reasonable place
    // to partition because it hugs machine precision (but not too closely). In other words, we are VERY
    // selective about whether the lines intersect or not!
    if(std::isnormal(separation)) return false;

    //Determine the point of intersection here.
    // First, we set the two line equations equal to each other in order to determine the parametrization ta and tb where they intersect.
    // Then we dot each (vector) equation with alternatively U_0,a and U_0,b to give us two equations for two unknowns (instead of three
    // equations and two unknowns with a ghost parameter.) The extra piece of information was used during the calculation of plane 
    // separation: ie. we have not lost any info by dotting both sides of the identity to reduce the dimensionality.
    const vec3<T> dR(this->R_0 - in.R_0); //dR = R_0_a - R_0_b
    const T udotu = this->U_0.Dot(in.U_0);
    const T denom = (T)(1.0) - udotu*udotu;

    if(!std::isnormal(denom)) return false;

    //For a line, we only need to compute one of these. For a line segment, we'll need both so we can range-check.
    const T ta = -((this->U_0 - (in.U_0 * udotu)).Dot(dR))/denom;
    //const T tb =  ( ( in.U_0 - ( (*this).U_0 * (udotu) ) ).Dot( dR ) ) / denom; 

    out = this->R_0 + (this->U_0 * ta);
    return true;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool line<float>::Intersects_With_Line_Once( const line<float> &in, vec3<float> &out) const;
    template bool line<double>::Intersects_With_Line_Once( const line<double> &in, vec3<double> &out) const;
#endif

template <class T>  bool line<T>::Closest_Point_To_Line(const line<T> &in, vec3<T> &out) const {
    //Returns the point on (*this) which is closest to the given line. Can only fail if the lines
    // are parallel (or due to fp uncertainties).
//    const auto LA = *this;
    const auto UA = this->U_0;
    const auto RA = this->R_0;
//    const auto LB = in;
    const auto UB = in.U_0;
    const auto RB = in.R_0;

    const auto UAB  = UA.Dot(UB);
    const auto dRAB = RA - RB;

    if(((T)(1) == UAB) || ((T)(1) == UAB*UAB)) return false; //Parallel lines - no possible single point.

    const auto tA_numer = (UB*UAB - UA).Dot(dRAB);
    const auto tA_denom = ((T)(1) - UAB*UAB);
//    const auto tB_numer = (UB - UA*UAB).Dot(dRAB);
//    const auto tB_denom = tA_denom;

    if(!std::isnormal(tA_denom)) return false;
    if(!std::isnormal((T)(1)/tA_denom)) return false;
//    if(!std::isnormal(tB_denom)) return false;
//    if(!std::isnormal((T)(1)/tB_denom)) return false;

    const auto tA = tA_numer/tA_denom;
//    const auto tB = tB_numer/tB_denom;

    const auto PA = UA*tA + RA;
    out = PA;
    return true;

    //If PB or the line PA-PB are required:
    //const auto PB = UB*tB + RB;
    //The line intersecting both.
    //const auto UC = (PB - PA).Unit();
    //const auto PC = PA;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool line<float >::Closest_Point_To_Line( const line<float > &, vec3<float > &) const;
    template bool line<double>::Closest_Point_To_Line( const line<double> &, vec3<double> &) const;
#endif

//---------------------------------------------------------------------------------------------------------------------------
//------------------------------------ line_segment: (finite-length) lines in 3D space --------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//Constructors.
template <class T>    line_segment<T>::line_segment(){ }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template line_segment<float>::line_segment(void);
    template line_segment<double>::line_segment(void);
#endif

template <class T>    line_segment<T>::line_segment(const vec3<T> &R_A, const vec3<T> &R_B) : t_0(0) {
    this->R_0 = R_A;  
    vec3<T> temp(R_B - R_A);
    
    t_1 = temp.length();
    this->U_0 = temp.unit();
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template line_segment<float>::line_segment(const vec3<float> &R_A, const vec3<float> &R_B);
    template line_segment<double>::line_segment(const vec3<double> &R_A, const vec3<double> &R_B);
#endif


//Samples every <spacing>, beginning at offset. Returns sampled points and remaining space along segment.
//
//NOTE: Parameter 'remaining' is CLEARED prior to adjustment.
//NOTE: Parameter 'offset' can be negative, but no check is done to 
template <class T>    std::list<vec3<T>> line_segment<T>::Sample_With_Spacing(T spacing, T offset, T & remaining) const {
    std::list<vec3<T>> points;
    if(this->t_1 <= this->t_0) FUNCERR("Our line segment is backward. We should reverse the normal and flip the sign on both t_0, t_1");
    const T L = (this->t_1 - this->t_0); //dR.length();
//    const T L = (this->R_0 + this->U_0*this->t_0).distance(this->R_0 + this->U_0*this->t_1);
    if(offset > L){ //No points can be sampled - the points are too close together.
        remaining = (L - offset);
        return points;
    }

    while(offset <= L){
        if(offset >= 0.0){
            vec3<T> R(this->U_0);
            R *= (this->t_0 + offset);
            R += this->R_0;
            points.push_back( R );
        }
        remaining = L - offset;
        offset += spacing;
    }
    return points;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template std::list<vec3<float >> line_segment<float >::Sample_With_Spacing(float  spacing, float  offset, float  &remaining) const;
    template std::list<vec3<double>> line_segment<double>::Sample_With_Spacing(double spacing, double offset, double &remaining) const;
#endif


template <class T>    vec3<T> line_segment<T>::Get_R0(void) const {
    //These are here in case I need/want to change the internal storage format later...
    return this->R_0;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template vec3<float > line_segment<float >::Get_R0(void) const;
    template vec3<double> line_segment<double>::Get_R0(void) const;
#endif

template <class T>    vec3<T> line_segment<T>::Get_R1(void) const {
    return this->R_0 + this->U_0 * (this->t_1 - this->t_0);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template vec3<float > line_segment<float >::Get_R1(void) const;
    template vec3<double> line_segment<double>::Get_R1(void) const;
#endif




//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------- plane: 2D planes in 3D space -----------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//Constructors.
template <class T>    plane<T>::plane(){ }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template plane<float>::plane(void);
    template plane<double>::plane(void);
#endif

template <class T>    plane<T>::plane(const vec3<T> &N_0_in, const vec3<T> &R_0_in) : N_0(N_0_in), R_0(R_0_in) { }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template plane<float>::plane(const vec3<float> &N_0_in, const vec3<float> &R_0_in);
    template plane<double>::plane(const vec3<double> &N_0_in, const vec3<double> &R_0_in);
#endif

//Member functions.
template <class T>    T plane<T>::Get_Signed_Distance_To_Point(const vec3<T> &R) const {
    //It is really as simple as $Dist_{signed} = \vec{N}_{0} \cdot ( \vec{R} - \vec{R}_{0} ) .$
    // N should already be a unit. We divide out the length for S&Gs.
    return this->N_0.Dot( R - this->R_0 ) / this->N_0.length();
//    return (this->N_0.x*(R.x-this->R_0.x) + this->N_0.y*(R.y-this->R_0.y) + this->N_0.z*(R.z-this->R_0.z)) / this->N_0.length();  
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float  plane<float>::Get_Signed_Distance_To_Point(const vec3<float> &R) const;
    template double plane<double>::Get_Signed_Distance_To_Point(const vec3<double> &R) const;
#endif

template <class T>    bool plane<T>::Is_Point_Above_Plane(const vec3<T> &R) const {
    const auto dist = this->Get_Signed_Distance_To_Point(R);
    //Check if exactly on the plane and if on the proper side or not.
    return (dist != (T)(0)) && (std::signbit(dist) == 0);
//    return ( std::signbit( this->Get_Signed_Distance_To_Point( R ) ) == 0 );
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool plane<float>::Is_Point_Above_Plane(const vec3<float> &R) const;
    template bool plane<double>::Is_Point_Above_Plane(const vec3<double> &R) const;
#endif


template <class T>    bool plane<T>::Intersects_With_Line_Once(const line<T> &L, vec3<T> &out) const {
   //This is a fairly simple, robust routine. Set the distance to a point along the line to zero and find the parameter t
   // that corresponds. Edge cases are infinite and zero intersection points.
   const T denom = this->N_0.Dot( L.U_0 );

   //If the plane's normal and the line's direction are orthogonal, they either intersect nowhere or everywhere.
   if(!std::isfinite((T)(1)/denom)) return false; 
//   if(fabs(denom) < 1E-9) return false; //Contains both the infinite and zero intersection cases.

   const T numer = this->N_0.Dot( L.R_0 - this->R_0 );
   out = L.R_0 - L.U_0*(numer/denom);
   return true;
}


//---------------------------------------------------------------------------------------------------------------------------
//------------------ contour_of_points: a polygon of line segments in the form of a collection of points --------------------
//---------------------------------------------------------------------------------------------------------------------------
//Constructors.
template <class T>    contour_of_points<T>::contour_of_points() : closed(false) { }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template contour_of_points<float>::contour_of_points(void);
    template contour_of_points<double>::contour_of_points(void);
#endif

template <class T>    contour_of_points<T>::contour_of_points(const std::list< vec3<T> > &in_points) : points(in_points), closed(false) { }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template contour_of_points<float>::contour_of_points(const std::list< vec3<float> > &in_points);
    template contour_of_points<double>::contour_of_points(const std::list< vec3<double> > &in_points);
#endif


//Member functions.
template <class T> T contour_of_points<T>::Get_Signed_Area(void) const {
    //If the polygon is not closed, we complain. This is the easiest way to do it...
    if(!this->closed){
        FUNCERR("Computing the surface area of an unconnected contour is not well-defined. Make sure to mark your (implicitly) closed contours as closed.");
    } 

    //If the polygon does not have enough points to form a 2D surface, return a zero. (This is legitimate.)
    if(this->points.size() < 3) return (T)(0);
    T Area = (T)(0);

    auto iter_1 = --(this->points.end());
    const T specific_height = iter_1->z; //Used to abandon the computation if the contour is fully 3D.
    for( auto iter_2 = this->points.begin(); iter_2 != this->points.end(); ++iter_2 ){
        const vec3<T> r_a(*iter_1);
        const vec3<T> r_b(*iter_2);
        if( std::fabs(PERCENT_ERR(r_a.z, specific_height)) > 0.1 ){  //Our criteria for same height is a |percent error| < 0.1%.
            FUNCERR("This routine is unable to compute generic (signed) areas for fully 3D contours: found a contour with height " << r_a.z << " when the general contour height is " << specific_height << ". This routine assumes the area of interest lies in an XY plane. Fix me if you really need this!");
            //NOTE: A more general 3D routine could probably be accomplished by using the cross product. I've avoided it because
            // it requires one to locate a point somewhere on the contour which will allow a linear interpolation of area between
            // all contour points. This is probably not possible except in special cases. For instance, a spherical shell has
            // curvature which could not be accounted for with said technique. If you need to implement this, consider the overall
            // utility of describing your surface with a contour around the surface boundary...
            // 
            //NOTE: It would be better to implement your particular solution to this problem as a totally separate routine. Maybe
            // something like "Get_Signed_Area_for_spherical_shell()".
        }
        const T n = r_b.y - r_a.y; //numerator.
        const T d = r_b.x - r_a.x; //denominator.

        //Determine if we consider the points to be equal (and thus have no area between them).
        if( ((n == (T)(0)) && (d == (T)(0))) || (r_a == r_b) ){
            //This is not an error - there is no area between overlapping points.
            FUNCWARN("Found two equal adjacent points in a contour. Check the input and/or consider decreasing the range of equality for vec3's. This is not an error");

        //If we are required to use a specific parametrization then do so.
        }else if(!std::isfinite(d/n)){
            const T m_1 = n/d;
            const T b_1 = r_a.y - m_1*r_a.x;
            Area += -0.5*b_1*(r_b.x - r_a.x); //The negative comes from Green's method!
        }else if(!std::isfinite(n/d)){
            const T m_2 = d/n;
            const T b_2 = r_a.x - m_2*r_a.y;
            Area +=  0.5*b_2*(r_b.y - r_a.y); //The negative is intentionally missing here!

        //Otherwise, try to determine which is more stable. Cursory feeling: slope closest to zero seems most stable. A proper analysis should be done, though.
        }else if(fabs(n/d) < fabs(d/n)){
            const T m_1 = n/d;
            const T b_1 = r_a.y - m_1*r_a.x;
            Area += -0.5*b_1*(r_b.x - r_a.x); //The negative comes from Green's method!
        }else{
            const T m_2 = d/n;
            const T b_2 = r_a.x - m_2*r_a.y;
            Area +=  0.5*b_2*(r_b.y - r_a.y); //The negative is intentionally missing here!


//        }else{
//            FUNCERR("Unable to properly parametrize two contour points (x,y,z) = " << r_a << " and " << r_b << ". This is not a recoverable error");
        }
        iter_1 = iter_2;
    }
    return Area; //NOTE: Do NOT take the absolute value. We want to keep the sign for adding/subtracting/etc.. contour areas!
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float  contour_of_points<float>::Get_Signed_Area(void) const;
    template double contour_of_points<double>::Get_Signed_Area(void) const;
#endif


template <class T> bool contour_of_points<T>::Is_Counter_Clockwise(void) const {
    //First, we compute the (signed) area. If the sign is positive, then the contour is counter-clockwise oriented.
    // Otherwise, it is of zero area or is clockwise.
    const T area = this->Get_Signed_Area();
    if( area < (T)(0.0) ) return false;
    return true;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool contour_of_points<float>::Is_Counter_Clockwise(void) const;
    template bool contour_of_points<double>::Is_Counter_Clockwise(void) const;
#endif


template <class T> void contour_of_points<T>::Reorient_Counter_Clockwise(void){
    //This routine is safe to call on contours which are already counter-clockwise!
    //
    //NOTE: It is not reasonable for us to determine if the contour is ordered properly 
    // (ie. the points are in a nice, non-overlapping order). The best we can do is 
    // flip the vec3 chain forward or backward!
    if(this->Is_Counter_Clockwise() == true) return;
    std::reverse(this->points.begin(), this->points.end());
    return;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template void contour_of_points<float>::Reorient_Counter_Clockwise(void);
    template void contour_of_points<double>::Reorient_Counter_Clockwise(void);
#endif


/*
template <class T> std::list<contour_of_points<T>> contour_of_points<T>::Split_Along_Plane(const plane<T> &theplane) const {
    //I cannot tell if this routine will be good for non-planar contours or not. Ygor feeling is 'no', but I wonder about
    // the overall ability to describe non-planar contours with a series of points. Maybe, if the curvature of the contour
    // is small near the plane, and it is understood that the points are linearlly interpolated between to form the contour,
    // then we will not actually observe any accidental loss in curvature from this routine.  I'll have to test it..
    std::list<contour_of_points<T>> output;


////////////// Needed?
//    //Search for adjacent, duplicate points. If any are found die immediately.
//    {
//      auto p1_it = --(this->points.end());
//      for(auto p2_it = this->points.begin(); (p2_it != this->points.end()) && (p1_it != this->points.end()); ){
//          if((*p1_it == *p2_it) && (p1_it != p2_it)){
//              FUNCERR("Found an adjacent duplicated point in input contour");
//          }else{
//              p1_it = p2_it;
//              ++p2_it;
//          }
//      }
//    }
/////////////////////

    if(this->points.size() < 3){
        FUNCERR("Not enough contour points to properly split contour. Pretending this contour has been split.");
    }
    long int number_of_crossings = 0;

    auto copy_of_points = this->points; //This is super inefficient, but it lets us shift the data around later...
    auto offset = copy_of_points.end();
    for(auto iter_2 = copy_of_points.begin(), iter_1 = --(copy_of_points.end());  iter_2 != copy_of_points.end();  ++iter_2){
        //iter_1 is the first point in the contour, iter_2 is one step ahead.      
        //We check if the line segment that connects the two points crosses the plane.
        vec3<T> garbage;
        if( (theplane.Is_Point_Above_Plane(*iter_1) != theplane.Is_Point_Above_Plane(*iter_2)) 
              && theplane.Intersects_With_Line_Once(line<T>(*iter_1,*iter_2),garbage) ){
            ++number_of_crossings;
            if(offset == copy_of_points.end()) offset = iter_1; //Look for the first point of the first pair which cross the plane.
        }
        iter_1 = iter_2;
    }

    if(number_of_crossings == 0){
        output.push_back(*this);
        return output;
    }
   
    std::rotate(copy_of_points.begin(), offset, copy_of_points.end()); //The first point of the first pair .. is now the first point.

    //Duplicate the first point. This will allow us to cycle through the list and more easily determine the intersection points.
    copy_of_points.push_back(copy_of_points.front());
    auto iter_of_second = ++(copy_of_points.begin());
    copy_of_points.push_back(*iter_of_second);

    //Step 5 - For each such pair, determine the point on the plane where they intersect.
    //
    //Step 6 - For each continuous stream of points, create a new contour. Insert the upstream plane-intersection point at the 
    // beginning and the downstream one at the end.
    std::vector<std::vector<vec3<T>>> linesegments;
    std::vector<vec3<T>> linesegment_shuttle; //Catches points thrown at it which are eventually thrown into the linesegments vector vector.
    std::vector<vec3<T>> endpoints;           //This is a simple list of the plane-intersection points.
    
    {
      auto iter_1 = copy_of_points.begin();  
      auto iter_2 = ++(copy_of_points.begin());
      for( ;  iter_2 != copy_of_points.end();  ++iter_2, ++iter_1){
          linesegment_shuttle.push_back(*iter_1);
          vec3<T> intersection;

          if((theplane.Is_Point_Above_Plane(*iter_1) != theplane.Is_Point_Above_Plane(*iter_2)) && theplane.Intersects_With_Line_Once(line<T>(*iter_1,*iter_2), intersection)){
              linesegment_shuttle.push_back( intersection );
              if(linesegment_shuttle.size() > 2){ //Each complete contour will have two endpoints and at least one other point! (Our entry into this routine gives us a short contour.)
                  linesegments.push_back(linesegment_shuttle);
                  endpoints.push_back( intersection );
              }else{
                  //We do not push the shuttle back - there are too few points because we are on the first iteration.
              }
              linesegment_shuttle.clear();
              //We append the "frontpoint" to the front of the next line segment, too.
              linesegment_shuttle.push_back( intersection );
          }
      }
    }

    if( number_of_crossings != static_cast<long int>(linesegments.size()) ){
        FUNCERR("We somehow produced " << number_of_crossings << " crossings and " << linesegments.size() << " line segments. This is an error. Check the (input) contour data for repeating points and then check the algorithm.");
    }
   
std::cout << "'linesegments' contains: " << std::endl;
for(auto sh_it = linesegments.begin(); sh_it != linesegments.end(); ++sh_it){
    for(auto shit = sh_it->begin(); shit != sh_it->end(); ++shit){
        std::cout << *shit << "    ";
    }
    std::cout << std::endl;
}
std::cout << std::endl;


    //Now, we determine which endpoint is furthest from the center point. This is done so we can consistently sequentially 'walk' over 
    // the endpoints, and the choice of the most distant is otherwise arbitrary. This assumes the contour is not in the plane
    // with the plane we are splitting on. This means the split will occur along the line defined by the crossing of the plane 
    // we are splitting on and the plane the contour lies on.
    const vec3<T> r_0 = this->Average_Point();
    auto lambda_distance_to_r_0 = [r_0](const vec3<T> &a, const vec3<T> &b) -> bool { return r_0.distance(a) < r_0.distance(b); };
    vec3<T> furthest_point = *(std::max_element(endpoints.begin(), endpoints.end(), lambda_distance_to_r_0));

    
    //Because each point is on a plane, we can determine which endpoints are connected across a contour by examining the order walking from the furthest point in the direction of the central point.
    //
    //In practice, this means we can simply sort endpoints (ie. the points which lie on the plane - or rather a line along the plane!) via their distance from the most distant point. After sorting,
    // the points will be ordered like A, B, C, D, E, F, ...  where the line segment A and B will cross a contour, the line segment B and C do NOT cross a segment, the line segment C and D will cross
    // a contour, D and E will not, etc.. Thus, we will be left with *pairs* of points which logically connect the contours along the plane. We can then walk through the endpointed-line segments 
    // and jump around whenever one of the pairs instructs us to. 
    //
    //Using the actual point coordinates is a decent way to match points, because no two points should be exactly identical. If there are some, they are both easy and inconsequential to remove prior to 
    // using this algorithm.
    auto lambda_distance_to_furthest_point = [furthest_point](const vec3<T> &a, const vec3<T> &b) -> bool { return (furthest_point.distance(a) < furthest_point.distance(b)); };
    std::sort(endpoints.begin(), endpoints.end(), lambda_distance_to_furthest_point); 
    std::map<vec3<T>,vec3<T>> paired_endpoints;
    for(long int ii = 0; (2*ii) < static_cast<long int>(endpoints.size()); ++ii){
//Commented because this doesn't seem to work...
       paired_endpoints[ endpoints[2*ii+0] ] = endpoints[2*ii+1];
       paired_endpoints[ endpoints[2*ii+1] ] = endpoints[2*ii+0];  //Is this needed if we are always traversing the (original and linesegmented) contour data in the same direction? (apparently yes!)

//       const auto A = endpoints[2*ii+0];
//       const auto B = endpoints[2*ii+1];
//FUNCINFO("OKAY");
//       if(paired_endpoints.find(A) != paired_endpoints.end()){
//           FUNCERR("Attempted to push back a paired endpoint A which we already have");
//       }
//       paired_endpoints.insert(std::pair<vec3<T>,vec3<T>>(A,B));
//
//       if(paired_endpoints.find(B) != paired_endpoints.end()){
//           FUNCERR("Attempted to push back a paired endpoint B which we already have");
//       }
//       paired_endpoints.insert(std::pair<vec3<T>,vec3<T>>(B,A));


    }
   
//if(paired_endpoints.size() != endpoints.size()){
//    FUNCERR("We pushed back too few paired endpoints " << paired_endpoints.size() << " compared with total number of endpoints " << endpoints.size() << ". Are two close enough to be floating-point-equal?");
//}
 
    //Now we cycle through the line segments until they are all used. 
    std::vector<bool> is_this_linesegment_used;
    for(long int ii=0; ii < static_cast<long int>(linesegments.size()); ++ii){
        is_this_linesegment_used.push_back(false);
    }

    std::vector<std::vector<vec3<T>>> newcontours;
    std::vector<vec3<T>> newcontour_shuttle;
    long int next_linesegment = -1;
    bool finished_shuttling = false;
    do{
    
//FUNCINFO("1 - Entering loop now. next_linesegment = " << next_linesegment << ", shuttle size = " << newcontour_shuttle.size() );
        {
          //Find the index of the next unused line segment if none is present.
          //
          //At no extra cost, check the exiting condition.
          // (If all segments are used AND the shuttle is empty, then we can exit the loop.)
          bool all_used = true;
          for(long int j = 0; j < static_cast<long int>(linesegments.size()); ++j){
              if(is_this_linesegment_used[j] == false){
                  all_used = false;
                  if(next_linesegment == -1) next_linesegment = j;
                  break;
              }
          }
    
          bool shuttle_empty = newcontour_shuttle.empty();
          finished_shuttling = (all_used && shuttle_empty);
          if(finished_shuttling) break; //Done.
        }
    
//FUNCINFO("2 - Just past first verification. next_linesegment = " << next_linesegment << ", shuttle size = " << newcontour_shuttle.size() << ", and is_this_linesegment_used[next_linesegment] = " << (is_this_linesegment_used[next_linesegment] ? 1 : 0) );
    
    
        //If we have a valid next_linesegment, and we have a non-empty shuttle, and next_linesegment points to a used segment, we have a complete contour in the shuttle.
        if( (next_linesegment != -1) && (!newcontour_shuttle.empty()) && (is_this_linesegment_used[next_linesegment] == true) ){
            newcontours.push_back(newcontour_shuttle);
            newcontour_shuttle.clear();
            next_linesegment = -1;
//                    continue;
//FUNCINFO("3A - Entered stream A - pushing completed shuttle onto the stack.");
    
        //If we have a valid next_linesegment, and it points to an unused segment, push in onto the shuttle, mark the segment as used, and set next_linesegment to the appropriate value.
        }else if((next_linesegment != -1) && (is_this_linesegment_used[next_linesegment] == false) ){
            //Mark the line segment "used."
            is_this_linesegment_used[next_linesegment] = true;
    
//FUNCINFO("3B - Just past first verification. next_linesegment = " << next_linesegment << ", shuttle size = " << newcontour_shuttle.size() << ", and is_this_linesegment_used[next_linesegment] = " << (is_this_linesegment_used[next_linesegment] ? 1 : 0) );
            //Append the line segment's points to the shuttle.
            newcontour_shuttle.insert(newcontour_shuttle.end(), linesegments[next_linesegment].begin(), linesegments[next_linesegment].end());
    
            //Find the "frontpoint" which corresponds to the endpoint of the current line segment.
            vec3<T> terminator = newcontour_shuttle[newcontour_shuttle.size() - 1]; 

            //Works fine on 64bit machine. Hardly ever works on 32bit (???)
            if(!(paired_endpoints.find(terminator) != paired_endpoints.end())){
                FUNCERR("Unable to find initiating endpoint " << terminator << ". This might be due to roundoff error");
            }
            vec3<T> theinitiator = paired_endpoints[ terminator ];



//            T minsqdist(1E30);
//            vec3<T> theinitiator;
//            for(auto m_it = paired_endpoints.begin(); m_it != paired_endpoints.end(); ++m_it){
//                const T thissqdist = m_it->first.sq_dist(terminator);
//                if(thissqdist < minsqdist){
//                    minsqdist = thissqdist;
//                    theinitiator = m_it->second;
//                }
//            }
//
//if(minsqdist != 0.0){ 
//    FUNCINFO("Chose theinitiator to be " << theinitiator << " whilst looking for point " << terminator << " because the minsqdist was " << std::setprecision(100) << minsqdist);
//}    


            bool could_find_it = false;
            for(long int j = 0; j < static_cast<long int>(linesegments.size()); ++j){
//FUNCINFO("Trying vector " << linesegments[j][0]);
                if(linesegments[j][0] == theinitiator){
                    next_linesegment = j;
                    could_find_it = true;
                    break;
                }
            }
            if(could_find_it == false) FUNCERR("Was unable to find the next line segment in this contour. Is it there?");
//FUNCINFO("4B - Entered stream B - pushing line segment onto the shuttle.");
        }
    
    
        {
          //If all segments are used AND the shuttle is empty, then we can exit the loop.
          bool all_used = true;
          for(long int j = 0; j < static_cast<long int>(linesegments.size()); ++j){
              if(is_this_linesegment_used[j] == false){
                  all_used = false;
                  break;
              }
          }
     
          bool shuttle_empty = newcontour_shuttle.empty();
          finished_shuttling = (all_used && shuttle_empty);
          if(finished_shuttling) break; //Done.
        }
    }while(finished_shuttling == false);

//FUNCINFO("Exited contour-generation routine. Now dumping contours");
    
    //We check for the number of output contours versus the number of plan crossings.
    //  2 crossings -> 2 contours.
    //  4 crossings -> 3 contours.
    //  6 crossings -> 4 contours.
    // so N crossings -> (N/2) + 1 contours.

    if( (2*(static_cast<long int>(newcontours.size()) - 1)) != number_of_crossings ){
        FUNCERR("This contour originally had " << number_of_crossings << " plane crossings and has been exploded into " << newcontours.size() << " contours. This is not the amount we should have!");
    }else{
        //If all looks swell, we push the split contours onto the output.
        for(long int j=0; j < static_cast<long int>(newcontours.size()); ++j){
            contour_of_points<T> new_contour;
            new_contour.closed = true;

            for(long int jj=0; jj < static_cast<long int>(newcontours[j].size()); ++jj){
                new_contour.points.push_back( newcontours[j][jj] );
            }

            output.push_back(new_contour);
        }
    }


//    //Search for and remove any adjacent, duplicate points. Also look for impossibly small contours.
//    for(auto c_it = output.begin(); c_it != output.end(); ++c_it){
//        if(c_it->points.size() < 3) FUNCERR("Produced a contour with too few points. This should not happen");
//
//        auto p1_it = --(c_it->points.end());
//        for(auto p2_it = c_it->points.begin(); (p2_it != c_it->points.end()) && (p1_it != c_it->points.end()); ){
//            if((*p1_it == *p2_it) && (p1_it != p2_it)){
//                //Walk the second iter along the chain.
//                p2_it = c_it->points.erase(p2_it);
//                FUNCWARN("Removed a neighbouring duplicate point. This may indicate errors in the splitting routine!");
//            }else{
//                p1_it = p2_it;
//                ++p2_it;
//            }
//        }
//
//        if(c_it->points.size() < 3) FUNCERR("Produced a contour with too many duplicates. Removing the dupes produced a malformed contour");
//    }

    return output;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
template std::list< contour_of_points<double>> contour_of_points<double>::Split_Along_Plane( const plane<double> &theplane ) const;
template std::list< contour_of_points<float>> contour_of_points<float>::Split_Along_Plane( const plane<float> &theplane ) const;
#endif
*/

template <class T> std::list<contour_of_points<T>> contour_of_points<T>::Split_Along_Plane(const plane<T> &theplane) const {
    std::list<contour_of_points<T>> output;

    //Re-written for the Nth time because behaviour was not identical between 32bit and 64bit machines. After some minor
    // tweaking, the 64bit and 32bit versions matched, but produced erroneous, brittle behaviour. 
    //
    //I believe the result was due to the use of equality on the vec3's. Although it was done logically (comparing only 
    // copies), I think some processors discarded extra bits (up to 80 bits are used on Intel machines). My hypothesis
    // is that when we had lots of contours we would blow the cache, forcing the cpu to discard extra bits. When reloading
    // the values, comparing copies was somehow no longer valid. Just a theory, but it matched behaviour fairly well.
    //
    //I cannot tell if this routine will be good for non-planar contours or not. My feeling is 'no', but I wonder about
    // the overall ability to describe non-planar contours with a series of points. Maybe, if the curvature of the contour
    // is small near the plane, and it is understood that the points are linearlly interpolated between to form the contour,
    // then we will not actually observe any accidental loss in curvature from this routine.  I'll have to test it..
    const auto Norig = this->points.size();
    if(Norig < 3) FUNCERR("Contour contains too few points to split");

    //Search for adjacent, duplicate points. If any are found die immediately.
    {
      auto p1_it = --(this->points.end()), p2_it = this->points.begin();
      while( (p2_it != this->points.end()) && (p1_it != this->points.end()) ){
          if((*p1_it == *p2_it) && (p1_it != p2_it)){
//              FUNCERR("Found an adjacent duplicated point in input contour");
              FUNCWARN("Found an adjacent duplicated point in input contour - attempting removal");
              //Duplicate the data.
              contour_of_points<T> dup(*this);

              //Remove one of the points.
              auto p3_it = dup.points.begin();
              std::advance(p3_it,  std::distance(this->points.begin(), p2_it));
              dup.points.erase(p3_it);

              //Pass off the computation as our own...
              return std::move(dup.Split_Along_Plane(theplane));

          }else{
              p1_it = p2_it;
              ++p2_it;
          }
      }
    }

    //Generate a center point for the entire contour. It shouldn't matter how precise it is,
    // but is safest to use the centroid.
    const auto Rcentre = this->Centroid();

    //Produce a dual of the contour points: a list of *pointers* to each element.
    // Insert a bool element so we can differentiate intersection points (later).
    std::list<std::pair<decltype(this->points.begin()), bool>> point_pointers;
    for(auto p_it = this->points.begin(); p_it != this->points.end(); ++p_it){
        point_pointers.push_back( std::pair<decltype(this->points.begin()), bool>(p_it,false) );
    }

    //Scan the points. We do three things here:
    // 1) determine the total number of plane crossings,
    // 2) identify the (arbitrary) first intersection point, and
    // 3) insert intersection points as needed. These are the only newly generated points!
    long int number_of_crossings(0);
    auto first_intersection = point_pointers.end();
    std::list<vec3<T>> intersections;
    {
      vec3<T> intersection;
      auto p1_it = --(point_pointers.end()), p2_it = point_pointers.begin();
      while((p2_it != point_pointers.end()) && (p1_it != point_pointers.end())){
          //This if statement could be replaced with a function Intersects_With_Line_Once***_Segment***(...).
          if( theplane.Is_Point_Above_Plane(*(p1_it->first)) != theplane.Is_Point_Above_Plane(*(p2_it->first)) ){
              if(!theplane.Intersects_With_Line_Once(line<T>(*(p1_it->first),*(p2_it->first)),intersection)){
                  FUNCERR("Unable to determine where plane intersects line (we know they cross). This is probably a floating-point booboo");
              }

              //Increment the counter.
              ++number_of_crossings;

              //Insert the intersection point in between p1_it and p2_it.
              const auto i_it = intersections.insert(intersections.end(),intersection);
              const auto p12_it = point_pointers.insert(p2_it,std::pair<decltype(this->points.begin()), bool>(i_it, true));

              //Mark the first intersection, if appropriate.
              if(first_intersection == point_pointers.end()) first_intersection = p12_it;
          }
          p1_it = p2_it;
          ++p2_it;
      }
    }

    //Check for special cases an errors.
    if(number_of_crossings == 0){
        output.push_back(*this);
        return output;
    }else if(number_of_crossings % 2 != 0){
        FUNCERR("Generated an odd number of plane crossings. This is impossible for a closed, non-overlapping contour");
    }

    //Rotate the list so the first intersection is the first point.
    std::rotate(point_pointers.begin(), first_intersection, point_pointers.end());

    //Duplicate the first intersection so we can cycle through more easily.
    point_pointers.push_back(point_pointers.front());

    //Sort the intersection points twice. First to find the most distant point from the centroid
    // and then in increasing distance from that point. Afterward, the first will have distance 0.0.
    //
    auto lambda_dist_from_Rcentre = [Rcentre](const vec3<T> &A, const vec3<T> &B) -> bool {
        return Rcentre.distance(A) < Rcentre.distance(B);
    }; 
    const auto Redge = *std::max_element(intersections.begin(), intersections.end(), lambda_dist_from_Rcentre);
    auto lambda_dist_from_Redge = [Redge](const vec3<T> &A, const vec3<T> &B) -> bool {
        return Redge.distance(A) < Redge.distance(B);
    };
    intersections.sort(lambda_dist_from_Redge);

    //The intersections are now ordered in such a way that for each N, intersection # 2*N and 2*N+1 are joined 
    // by crossing contour and NOT empty space along the plane. Furthermore, all other intersections cannot be
    // joined in such a way.
    //
    //Explicitly group them into pairs. We treat the iterator's dereferenced object's address (cast to size_t) 
    // as the map key. This means two iterators pointing to same point are considered equal.
    //
    //NOTE: Ensure no ...end()'s get pushed into the map. They do not point to anything (I think) which will 
    // cause undefined behaviour (or worse - maybe silent error or seg faults).
    std::unordered_map<decltype(this->points.begin()), decltype(this->points.begin()), std::function<size_t (const decltype(this->points.begin()) &x)>>
    intersection_pairs(24, [](const decltype(this->points.begin()) &x){
            return (size_t)(&(*x)); //Get address of object pointed to casted to size_t.
        });

    for(auto v2_it = intersections.begin(); v2_it != intersections.end(); ){
        const auto v1_it = v2_it;
        ++v2_it;
        intersection_pairs[v1_it] = v2_it;
        intersection_pairs[v2_it] = v1_it;
        ++v2_it;
    }

    //Prepare a new contour buffer.
    auto nc_it = output.end();
    output.push_back(contour_of_points<T>());
    nc_it = --(output.end());
    nc_it->closed = true;
    
    //Go until we have inserted all non-intersection points into appropriate contours. 
    while(true){ //point_pointers.size() != (Norig + intersections.size() + 1)){
        //Start at the first available normal point.
        auto p_it_it = point_pointers.begin();
        while(p_it_it->second == true){ 
            ++p_it_it;
            if(!(p_it_it != point_pointers.end())) FUNCERR("Ran out of non-intersection points before completing contour");
        }
        const auto beginning = --p_it_it; //First intersection point beginning line segment.
        ++p_it_it;
        
        while(true){
            //------------------------ Error Catching ---------------------------
            //Catch 1 - Run out of points.
            if(!(p_it_it != point_pointers.end())){
                FUNCERR("Ran out of points during contour generation. This shouldn't happen");
            
            //Catch 2 - We have looped around and didn't properly catch it.
            }else if(p_it_it == beginning){
                FUNCERR("We have looped around and did not properly exit contour generation loop");
            }
            //---------------------- Point Accumulation -------------------------
            //Case 1 - This is a normal point.
            if(p_it_it->second == false){
                //Push it back by value, remove it from pointer list, and continue along this
                // line segment sequentially.
                nc_it->points.push_back( *(p_it_it->first) );
                p_it_it = point_pointers.erase(p_it_it);
                continue;

            //Case 2 - This is an intersection point.
            }else{
                //Push it back.
                nc_it->points.push_back( *(p_it_it->first) );

                //Find out which intersection we should jump to. We have to work backward because
                // we do not (atm) have a mapping from point iterators to map iterators (aka point
                // iterator iterators).
                const auto jump_p_it = intersection_pairs[p_it_it->first];
                decltype(p_it_it) jump_it_it = point_pointers.begin();
                while(jump_it_it->first != jump_p_it){
                    ++jump_it_it;
                    if(!(jump_it_it != point_pointers.end())){
                        FUNCERR("Unable to find intersection point to jump to. Has it been removed?");
//Might also be a mistake in iteration, removing points, or anything...
                    }
                }

                //If the jump intersection is the one we started on, add it and break out.
                if(jump_it_it == beginning){
                    nc_it->points.push_back( *(beginning->first) );
                    break;

                //Otherwise, add the jump intersection and iterate past it (to avoid confusing the loop).
                }else{
                    nc_it->points.push_back( *(jump_it_it->first) );
                    p_it_it = jump_it_it;
                    ++p_it_it;
                    continue;
                }
            }
        }

        //When we get here, we should have a complete contour sitting in the output's last slot.

        //If we have any non-intersection points remaining, we will loop again. Iterate the output
        // contour's slot and continue;
        if(point_pointers.size() != (/*Norig + */ intersections.size() + 1)){
            output.push_back(contour_of_points<T>());
            nc_it = --(output.end());
            nc_it->closed = true;
            continue;

        //If we make it here, we have used all points and filled the output with all necessary
        // contours. Simply break out and continue.
        }else{
            break;
        }
    }

    //------------------------------ Cursory verification ----------------------------------
    //Search for adjacent, duplicate points. If any are found then warn, remove them, and try to recover.
    for(auto c_it = output.begin(); c_it != output.end(); ++c_it){
        auto p1_it = --(c_it->points.end()), p2_it = c_it->points.begin();
        while( (p2_it != c_it->points.end()) && (p1_it != c_it->points.end()) ){
            if((*p1_it == *p2_it) && (p1_it != p2_it)){
                const auto posA = std::distance(c_it->points.begin(), p1_it);
                const auto posB = std::distance(c_it->points.begin(), p2_it);
                FUNCWARN("Found adjacent duplicated points in split contour (points #" << posA << " and " << posB << ". Removing one and continuing");
                //Notes: 
                // Keep an eye on when these pop up. I'm not sure where they originate from. I have seen them
                // pop up due to mixing coronal/sagittal and per-volume/per-contour splitting after about 5
                // splits. This gave #'s 1 and 2. Splitting again gave #'s 2 and 3.
                //
                // It is possible these are spit out from some other routine (line intersection is a point
                // is exactly on a plane - we do not consider it a plane crossing if the point is exactly
                // on the plane!)
                //
                // Overall, it seems like these occurences are rarely fatal. It is most likely OK to deal with
                // them specially by-hand like this.

                //We should remove the first one so that we will notice if the next one is a duplicate too.
                p1_it = c_it->points.erase(p1_it);
                if(c_it->points.size() < 3) FUNCERR("After removing duplicate point, contour contains < 3 points. Unable to continue");

            }else{
                p1_it = p2_it;
                ++p2_it;
            }
        }
    }

  

    //Ensure the contours are oriented uniformly.
    for(auto c_it = output.begin(); c_it != output.end(); ++c_it){
        c_it->Reorient_Counter_Clockwise();
    }

    return std::move(output);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
template std::list< contour_of_points<double>> contour_of_points<double>::Split_Along_Plane( const plane<double> &theplane ) const;
template std::list< contour_of_points<float>> contour_of_points<float>::Split_Along_Plane( const plane<float> &theplane ) const;
#endif




template <class T> contour_of_points<T> contour_of_points<T>::Bounding_Box_Along( const vec3<T> &r_n, T margin/* = (T)(0.0)*/ ) const {
    contour_of_points<T> bounding_box;
    bounding_box.closed = true;

    if(r_n.z != 0.0){
        FUNCWARN("This routine is unable to sensibly handle non-zero z-components in the direction unit vector. Please use another algorithm!");
        return bounding_box;
    }

    if( this->points.size() < 3 ){
        FUNCWARN("Too few points in this contour to adaquetly compute a bounding box. Ignoring it and continuing..");
        return bounding_box;
    }

    //Now we determine the (2D) bounding box (which is oriented with the unit vector!) by projecting each point onto 
    // the r_1 and r_2 coordinate system.
    const vec3<T> r_1( r_n.x, r_n.y, r_n.z);
    const vec3<T> r_2(-r_n.y, r_n.x, r_n.z); //(= r_1 rotated by +pi/2).

    vec3<T> r_1_most  = this->points.front();
    vec3<T> r_2_most  = this->points.front();
    vec3<T> r_1_least = this->points.front();
    vec3<T> r_2_least = this->points.front();
    for(auto it = this->points.begin(); it != this->points.end(); ++it){
        //Length of the vector on the units which define the bounding box coordinates.
        const T r_1_dot_centered  = r_1.Dot( (*it) );
        const T r_2_dot_centered  = r_2.Dot( (*it) );
        
        const T r_1_dot_r_1_most  = r_1.Dot( r_1_most  );  //It is silly to compute these each time. ..."Clean up crew to aisle $HERE."
        const T r_1_dot_r_1_least = r_1.Dot( r_1_least );
        const T r_2_dot_r_2_most  = r_2.Dot( r_2_most  );
        const T r_2_dot_r_2_least = r_2.Dot( r_2_least );

        if( r_1_dot_centered > r_1_dot_r_1_most  ) r_1_most  = (*it);
        if( r_1_dot_centered < r_1_dot_r_1_least ) r_1_least = (*it);

        if( r_2_dot_centered > r_2_dot_r_2_most  ) r_2_most  = (*it);
        if( r_2_dot_centered < r_2_dot_r_2_least ) r_2_least = (*it);
    }
    //FUNCINFO(" Four extremity points: " << r_1_most << "  --  " << r_2_most << "  --  " << r_1_least << "  --  " << r_2_least);

    //We have the extremity points from the contour now. We make four lines describing the bounding box, 
    // and each line bounding box lies on top of a(t least one) point in the contour.
    //
    // Counter clockwise orientation:  (clockwise would just negate each unit vector!)
    //
    // Line 1:  point = r_1_most  ,  unit vector = -r_2
    // Line 2:  point = r_2_least ,  unit vector = -r_1
    // Line 3:  point = r_1_least ,  unit vector =  r_2
    // Line 4:  point = r_2_most  ,  unit vector =  r_1

    const vec3<T> u_l1( -r_2.x, -r_2.y, -r_2.z ),    p_l1( r_1_most  );
    const vec3<T> u_l2( -r_1.x, -r_1.y, -r_1.z ),    p_l2( r_2_least );
    const vec3<T> u_l3(  r_2.x,  r_2.y,  r_2.z ),    p_l3( r_1_least );
    const vec3<T> u_l4(  r_1.x,  r_1.y,  r_1.z ),    p_l4( r_2_most  );

    const line<T> L1( p_l1, p_l1 + u_l1 );
    const line<T> L2( p_l2, p_l2 + u_l2 );
    const line<T> L3( p_l3, p_l3 + u_l3 );
    const line<T> L4( p_l4, p_l4 + u_l4 );

    //Now determine the four points where the bounding box intersects.
    vec3<T> intersection;

/*
    //Negative (clockwise) orientation.
    if( L4.Intersects_With_Line_Once(L1, intersection) ){
        bounding_box.points.push_back( intersection + (r_1 + r_2)*margin);
    }else{
        FUNCERR("Unable to determine the point of intersection. Unable to continue");
    }

    if( L1.Intersects_With_Line_Once(L2, intersection) ){
        bounding_box.points.push_back( intersection + (r_1 - r_2)*margin);
    }else{
        FUNCERR("Unable to determine the point of intersection. Unable to continue");
    }

    if( L2.Intersects_With_Line_Once(L3, intersection) ){
        bounding_box.points.push_back( intersection - (r_1 + r_2)*margin);
    }else{
        FUNCERR("Unable to determine the point of intersection. Unable to continue");
    }

    if( L3.Intersects_With_Line_Once(L4, intersection) ){
        bounding_box.points.push_back( intersection + (r_2 - r_1)*margin);
    }else{
        FUNCERR("Unable to determine the point of intersection. Unable to continue");
    }
*/

    //Positive (counter-clockwise) orientation.
    if( L4.Intersects_With_Line_Once(L1, intersection) ){
        bounding_box.points.push_back( intersection + (r_1 + r_2)*margin);
    }else{
//        FUNCERR("Unable to determine the point of intersection 1. Unable to continue");
        FUNCWARN("Could not determine exact point of intersection (1). Computing closest-point instead");
        if( L4.Closest_Point_To_Line(L1, intersection) ){
            bounding_box.points.push_back( intersection + (r_1 + r_2)*margin);
        }else{
            FUNCERR("Unable to determine closest point on L4 to L1. Cannot proceed");
        }
    }

    if( L3.Intersects_With_Line_Once(L4, intersection) ){
        bounding_box.points.push_back( intersection + (r_2 - r_1)*margin);
    }else{
//        FUNCERR("Unable to determine the point of intersection 2. Unable to continue");
        FUNCWARN("Could not determine exact point of intersection (2). Computing closest-point instead");
        if( L3.Closest_Point_To_Line(L4, intersection) ){
            bounding_box.points.push_back( intersection + (r_2 - r_1)*margin);
        }else{
            FUNCERR("Unable to determine closest point on L3 to L4. Cannot proceed");
        }
    }

    if( L2.Intersects_With_Line_Once(L3, intersection) ){
        bounding_box.points.push_back( intersection - (r_1 + r_2)*margin);
    }else{
//        FUNCERR("Unable to determine the point of intersection 3. Unable to continue");
        FUNCWARN("Could not determine exact point of intersection (3). Computing closest-point instead");
        if( L2.Closest_Point_To_Line(L3, intersection) ){
            bounding_box.points.push_back( intersection - (r_1 + r_2)*margin);
        }else{
            FUNCERR("Unable to determine closest point on L2 to L3. Cannot proceed");
        }
    }

    if( L1.Intersects_With_Line_Once(L2, intersection) ){
        bounding_box.points.push_back( intersection + (r_1 - r_2)*margin);
    }else{
//        FUNCERR("Unable to determine the point of intersection 4. Unable to continue");
        FUNCWARN("Could not determine exact point of intersection (4). Computing closest-point instead");
        if( L1.Closest_Point_To_Line(L2, intersection) ){
            bounding_box.points.push_back( intersection + (r_1 - r_2)*margin);
        }else{
            FUNCERR("Unable to determine closest point on L1 to L2. Cannot proceed");
        }
    }


    //Now we dump the bounding box contour.
    return bounding_box;
}

#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
template contour_of_points<float> contour_of_points<float>::Bounding_Box_Along( const vec3<float> & , float) const ;
template contour_of_points<double> contour_of_points<double>::Bounding_Box_Along( const vec3<double> & , double) const ;
#endif


//This routine is hacky and doesn't produce the most reliable results. In particular, we wrap each contour in a bounding box
// and ray-cast within the edges of the box. This means that mis-shapen contours will be troublesome.
//
//The number of generated points needs to be quite high to avoid unsightly edges and corners. A proper routine would
// address these points (or maybe at least smooth the unsightly edges).
//
//Works somewhat alright, but we haven't had a need to use it yet. Fix it if you need to!
template <class T> std::list< contour_of_points<T> > contour_of_points<T>::Split_Against_Ray( const vec3<T> &r_n ) const {
    std::list<contour_of_points<T>> output;

    FUNCWARN("This routine may or may not work OK.");

    //Grab the bounding box.
    const contour_of_points<T> the_bounding_box = this->Bounding_Box_Along(r_n, 1.0);
    if(the_bounding_box.points.size() != 4){
        FUNCWARN("Unable to compute a bounding box. Ignoring and continuing.");
        return output;
    }

    //Given the bounding box and the contour data, we now crawl along the edge and ray cast through the contour. 
    const auto number_of_contour_points = static_cast<long int>(this->points.size());
    //const long int number_of_new_points = number_of_contour_points;  //Too many!
    long int number_of_new_points = number_of_contour_points / 3;
    const long int min_number_of_new_points = 10;    //NOTE: We are NOT guaranteed to get this many. This is an upper bound!
                                                     // The bounding box may cover a large area which is void of contour!
    if( number_of_new_points < min_number_of_new_points ) number_of_new_points = min_number_of_new_points;

    auto BB_point_iter = the_bounding_box.points.begin();
    //These points are considered to be specified in a counter clockwise orientation. 

    //const vec3<T> r_0 = (*BB_point_iter); //  <--- This one is not used here. Maybe I should use it?
    ++BB_point_iter;
    const vec3<T> r_1 = (*BB_point_iter); 
    ++BB_point_iter;
    const vec3<T> r_2 = (*BB_point_iter); 
    ++BB_point_iter;
    const vec3<T> r_3 = (*BB_point_iter); 

    const vec3<T> dr_32(r_3 - r_2); //WAS r2 - r1. 
    const T width_of_bb = dr_32.length();
    const T dL = width_of_bb / static_cast<T>(number_of_new_points); //dL is the length of separation between ray casts along the unit vector orthogonal to r_n in the XY plane.
    
    //We work our way along the bounding box's "lower" edge and ray cast upward through the contour in the direction of the unit vector provided.
    std::vector<vec3<T>> halfway_points;
    vec3<T> u_ray(r_1 - r_2);   //WAS r_3 - r_2 !!!
//    u_ray -= r_2;
    u_ray.z = 0.0;
    u_ray = u_ray.unit();

    long int TIMES_STUCK_IN_KEY = 0; //Hack to ensure we don't loop endlessly...
    for(T L = 0.05*dL; L < width_of_bb; L += dL){
        //First, we precisely specify the ray cast line. u_ray is the unit vector and p_ray is the point defining the line of the ray. It is NOT constrained to the bounding box!
        const vec3<T> p_ray = r_2 + r_n*L;

        //Now we find which pairs of points are joined by a line which intersects the ray cast line. We determine the precise point where the pair of point's line intersects the ray cast line.
        std::vector<vec3<T>> intersection_points;

        //Get all intersection points.
        const plane<T> theplane(r_n, p_ray); 
        {
            auto iter_1 = this->closed ? --(this->points.end()) :    this->points.begin();
            auto iter_2 = this->closed ?   this->points.begin() : ++(this->points.begin());

//            auto iter_1 = this->points.begin();
//            auto iter_2 = ++(this->points.begin());
            for( ;  iter_2 != this->points.end();  ++iter_2){
                vec3<T> intersection;
                if(theplane.Is_Point_Above_Plane(*iter_1) != theplane.Is_Point_Above_Plane(*iter_2)){
                    if(theplane.Intersects_With_Line_Once(line<T>(*iter_1,*iter_2), intersection)){
                        intersection_points.push_back( intersection );
                    }else{
                        FUNCINFO("Unable to find intersection (maybe it intersects many times?). Assuming center point is intersection!");
                        intersection_points.push_back( (*iter_1 + *iter_2)*0.5 );
                    }
                }
                iter_1 = iter_2;
            }
        }

        //Now we sort the intersections based on their distance from the edge of the bounding box. This is required because the contour can snake around any number of ways, and we might
        // not start circling the contour at the point nearest the first ("lowest") intersection.
        auto lambda_distance_along_u_ray = [u_ray](const vec3<T> &a, const vec3<T> &b) -> bool { return (u_ray.Dot(a) < u_ray.Dot(b)); };

        std::sort( intersection_points.begin(), intersection_points.end(), lambda_distance_along_u_ray );
 
        //Now, given the total number of intersections, we can compute the total length of the ray cast which is within the contour.
        if( (intersection_points.size() % 2) != 0 ){
            ++TIMES_STUCK_IN_KEY;
            if(TIMES_STUCK_IN_KEY > 25){
                FUNCWARN("We have crossed an odd number of contour lines, but we should have crossed an even number. Unable to nudge, so giving up!");
                continue;
            }

//            FUNCERR("We have crossed an odd number of contour lines, but we should have crossed an even number.");
//            FUNCWARN("We have crossed an odd number of contour lines, but we should have crossed an even number. Nudging ray slightly...");
            L += 0.01*dL;
            L -= dL;  //To counter the loop's L += dL.
            continue;
        }
        if( intersection_points.size() == 0 ){
            ++TIMES_STUCK_IN_KEY;
            if(TIMES_STUCK_IN_KEY > 25){
                FUNCWARN("We failed to find any points of intersection. Unable to nudge, so giving up!");
                continue;
            }
//            FUNCERR("We failed to find any points of intersection. This is likely an issue with the bounding box or the data being wonky. This could be due to multi-contour input - if so, fix me please!");
//            FUNCWARN("We failed to find any points of intersection - was the contour oddly shaped, excessively small, or very point-sparse? Nudging ray slightly...");
            L -= 0.007*dL;
            L -= dL;  //To counter the loop's L += dL.
            continue;
//return output;
        }

        TIMES_STUCK_IN_KEY = 0;

        T contour_length_at_this_L = 0.0;
        for(size_t i=0; (2*i)<intersection_points.size(); ++i){
            contour_length_at_this_L += intersection_points[2*i + 0].distance( intersection_points[2*i + 1] );
        }

        //Knowing this total length, we can walk through the intersection points until we find the point which is halfway through the contour (but which is somewhere inside the contour!)
        vec3<T> the_center_point;
        T remaining_contour_halfway_length = 0.5*contour_length_at_this_L;
        bool found_a_center_point = false;
        for(size_t i=0; (2*i)<intersection_points.size(); ++i){
            const T separation = intersection_points[2*i + 0].distance( intersection_points[2*i + 1] );

            //If we do not terminate the search for the halfway point within this pair of intersection points.
            if( remaining_contour_halfway_length > separation ){
                remaining_contour_halfway_length -= separation;

            //If we DO terminate between this pair.
            }else{
                found_a_center_point = true;
                the_center_point = intersection_points[2*i + 0] + u_ray * remaining_contour_halfway_length;
                break;
            }
        }

        //Knowing this point, we push it back into the list of halfway points. Since we have walked the bounding box in a specific way, we will be able to place the point with the correct contour given
        // its location in the halfway point vector.
        if(found_a_center_point == true){
            halfway_points.push_back( the_center_point );
        }else{
            FUNCERR("Was unable to find a center-point. This is an algorithmic error.");
        }

    }

    if( halfway_points.size() == 0 ){
        FUNCWARN("No mid-contour points were generated. Not exactly sure why.. Maybe insufficient number of points were requested?");
        return output;
    }

    //We should now have an ordered list of center points which split the contour. We just have to figure out where best to attach them to the contour endpoints.
    // We take the two (newly-generated) endpoints and determine which contour point is nearest to them. We then split the contours on these points.

    vec3<T> nearest_first( (*this).points.front() );
    vec3<T> nearest_last( (*this).points.front() );
    vec3<T> halfway_first = halfway_points.front();
    vec3<T> halfway_last  = halfway_points.back();

    long int offset_first = 0;
    long int offset_last  = 0;
    for(auto it = (*this).points.begin(); it != (*this).points.end(); ++it ){
        if( halfway_first.distance( (*it) ) < halfway_first.distance( nearest_first ) ){
            nearest_first = (*it);
            offset_first = std::distance( (*this).points.begin(), it );
        }
        if( halfway_last.distance( (*it) )  < halfway_last.distance(  nearest_last )  ){
            nearest_last  = (*it);
            offset_last = std::distance( (*this).points.begin(), it );
        }
    }

    //At this point, the contour is split into two pieces. We enter the contour loop by inserting the halfway points and then looping around as needed.
    std::vector< vec3<T> > contour_A;  //Goes in direction from first to last along the halfway points.
    std::vector< vec3<T> > contour_B;  //Goes in direction from last to first along the halfway points.

    for(long int i=0; i < static_cast<long int>(halfway_points.size()); ++i){
        contour_A.push_back( halfway_points[i] );
        contour_B.push_back( halfway_points[(halfway_points.size() - 1) - i] );
    }

    { //Contour A.
      bool passed_first = false;
      bool passed_last  = false;
      long int i=0;
      while(!(passed_first && passed_last)){
          const long int ii = i%(*this).points.size();
          if( ii == offset_last ){
              passed_last = true;  //Start pushing points and watching for the first point during this iteration.
          }
          if( (ii == offset_first) && (passed_last == true) ){
              passed_first = true; //Exit the loop after this iteration (we need both first and last endpoints on the contours.)
          }
          if(passed_last == true){
              auto it = (*this).points.begin();
              std::advance(it, ii);
              contour_A.push_back( (*it) );
          }
          ++i;
      }
    }

    { //Contour B.
      bool passed_first = false;
      bool passed_last  = false;
      long int i=0;
      while(!(passed_first && passed_last)){
          const long int ii = i%(*this).points.size();
          if( (ii == offset_last) && (passed_first == true) ){
              passed_last = true;  //Exit the loop after this iteration (we need both first and last endpoints on the contours.)
          }
          if( ii == offset_first ){
              passed_first = true; //Start pushing points and watching for the first point during this iteration.
          }
          if(passed_first == true){
              auto it = (*this).points.begin();
              std::advance(it, ii);
              contour_B.push_back( (*it) );
          }
          ++i;
      }
    }


   {
    contour_of_points<T> temp;
    for(size_t i=0; i<contour_A.size(); ++i){
        temp.points.push_back( contour_A[i] );
    }
    temp.closed = true;
    temp.Remove_Sequential_Duplicate_Points(nullptr);    //NOTE: May fail if <= 3 points remain...  Maybe resample for small point sizes?
    output.push_back( temp );
   }

   {
    contour_of_points<T> temp;
    for(size_t i=0; i<contour_B.size(); ++i){
        temp.points.push_back( contour_B[i] );
    }
    temp.closed = true;
    temp.Remove_Sequential_Duplicate_Points(nullptr);    //NOTE: May fail if <= 3 points remain...  Maybe resample for small point sizes?
    output.push_back( temp );
   }

    return std::move(output);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
template std::list<contour_of_points<double>> contour_of_points<double>::Split_Against_Ray(const vec3<double> &) const;
template std::list<contour_of_points<float>> contour_of_points<float>::Split_Against_Ray(const vec3<float> &) const;
#endif

//Splits contour into an inner (core) and outer (peel). Projects from 3D point onto contour plane. The 2D ~analog is Scale_Dist_From_Point().
//
//NOTE: Only use this routine on simple, ~spherical/round contours. No ray-casting is performed, so it will not gracefully handle
// large dips or folds (unless a very small core is desired).
//
//NOTE: It is recommended to use a contour collection's centroid (but go nuts, if desired).
//
//NOTE: This routine always creates TWO output contours. The core is always first. One or more may be empty, so make sure to check!
template <class T> std::list<contour_of_points<T>> contour_of_points<T>::Split_Into_Core_Peel_Spherical(const vec3<T> &point, T frac_dist) const {
    std::list<contour_of_points<T>> output;
    if(frac_dist < (T)(0.0)){
        FUNCWARN("Passed a negative scale factor. This is nonsensical and nothing reasonable can be computed. Continuing");
        return std::move(output);
    }else if(frac_dist > (T)(1.0)){
        FUNCWARN("Passed a scale factor > 1.0. This case is not handled and would produce a core larger than peel! Continuing");
        return std::move(output);
    }
    if(this->points.empty()){
        FUNCWARN("Attempted to perform core and peel splitting with no contour points! Continuing");
        return std::move(output);
    }
    if(this->closed != true){ 
        FUNCERR("The core and peel technique is not able to reasonably handle open contours. Unable to continue");
        //The peel is easy enough to deal with, but how do we treat the core? Should we always assume it
        // is closed, no matter what? This is quite ambiguous. It seems ill-defined to consider a core and
        // peel for contours which are open!
    }

    //Prepare space for the inner and outer data.
    output.push_back(contour_of_points<T>()); //inner (core).
    output.push_back(contour_of_points<T>(this->points)); //outer (peel).
    output.front().closed = this->closed;
    output.back().closed  = this->closed;

    //Loop through the points, computing the boundary between core and peel via projection
    // (ie. the contour of the core/inner contour of peel). 
    const auto C = this->Centroid();
    const auto P_C = point - C;
    const auto P_C_len = P_C.length();
    const auto P_C_sq_len = P_C_len * P_C_len;
    for(auto p_it = this->points.begin(); p_it != this->points.end(); ++p_it){
        const auto Pi_P = (*p_it) - point;
        const auto Pi_P_len = Pi_P.length();
        const auto Pi_P_sq_len = Pi_P_len * Pi_P_len;

        const auto C_Pi = C - (*p_it);
        const auto C_Pi_len = C_Pi.length();
        const auto C_Pi_sq_len = C_Pi_len * C_Pi_len;

        const auto numer = (T)(2.0)*((T)(1.0)-frac_dist)*C_Pi_len*Pi_P_sq_len;
        const auto denom = C_Pi_sq_len + Pi_P_sq_len - P_C_sq_len;
        const auto D = std::isfinite((T)(1.0)/denom) ? (numer/denom) : std::numeric_limits<T>::max();
        //Only push back points which remain on the present side (and within the bounds) of the centroid.
        if(isininc((T)(0.0),D,C_Pi_len)){
            const auto PiPrime = C_Pi.unit()*D + (*p_it);
            output.front().points.push_back(PiPrime);
        }
    }

    //The inner (core, first) is ready to go, but we need to boolean subtract it from the outer (peel).
    if(output.front().points.empty()) return std::move(output);
    const auto peel_final_point = output.back().points.back();

    decltype(output.front().points) inner_copy(output.front().points);
    inner_copy.push_back( inner_copy.front() );
    inner_copy.reverse();

    output.back().points.splice(output.back().points.end(), std::move(inner_copy));
    output.back().points.push_back(peel_final_point);

    //Cleanup to contours.
    output.front().Remove_Sequential_Duplicate_Points(nullptr);
    output.back().Remove_Sequential_Duplicate_Points(nullptr);
    return std::move(output);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template std::list<contour_of_points<float >> contour_of_points<float >::Split_Into_Core_Peel_Spherical(const vec3<float > &, float  frac_dist) const;
    template std::list<contour_of_points<double>> contour_of_points<double>::Split_Into_Core_Peel_Spherical(const vec3<double> &, double frac_dist) const;
#endif


//This function sums all the contour points and divides by the number of points, giving the 'average' point. This could be used as a 
// center point, a rough indication of 'where' a contour is, or as a means of rotating the contour.
template <class T> vec3<T> contour_of_points<T>::Average_Point(void) const {
    vec3<T> out((T)(0), (T)(0), (T)(0));
    for(auto iter = (*this).points.begin(); iter != (*this).points.end(); ++iter){
        out += (*iter);
    }
    out /= (*this).points.size();
    return out;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template vec3<float> contour_of_points<float>::Average_Point(void) const;
    template vec3<double> contour_of_points<double>::Average_Point(void) const;
#endif


//This function returns the 'average pointed weighted by the area enclosed by the contour.' In other words, this returns the 
// center of mass of contour if the contour were filled with a homogeneous medium with a planar mass density.
//
//Splitting a contour on this point (along a straight line in the plane of the contour which intersects this point) will 
// result in two or more contours. 
//
// NOTE: This routine will *NOT* provide a point about which to generically split a contour into parts of equal area, despite
// what you may have read online about 'centroids.' If you do not believe this to be true, compute the centroid for a simple
// right triangle and then split it into two parts using a straight line of non-round-number slope. Compute the area of each
// piece and examine that it is not what you might expect. In particular, if the result is "off by a bit," observe that the
// difference IS off by a bit! There is no such generic "splits it into two" point even for a simple polygon!
//
template <class T> vec3<T> contour_of_points<T>::Centroid(void) const {
    //If there are pathological cases, we can deal with them fairly easily.
    if(this->points.size() == 0) FUNCERR("Attempted to compute Center of area for a contour with no points");
    if(this->points.size() < 3){
         vec3<T> R((T)(0), (T)(0), (T)(0));
         for(auto i = this->points.begin(); i != this->points.end(); i++){
             R += *i;
         }
         R /= static_cast<T>( this->points.size() );
         return R;
    }

    //First, we determine the average contour point. We hope that this is within the contour. 
    // More precisely, we hope the contour is planar (or nearly planar!) Any point interior to 
    // the contour will work, though, so maybe this should be fixed.                 FIXME
    const vec3<T> C( this->Average_Point() );
    vec3<T> RA((T)(0), (T)(0), (T)(0));
    T Area = (T)(0);

    //Cycle through the points in the contour sequentially, accumulating the area and area*center of the 
    // triangle made by two neighbouring points (A and B) and the center point (C).
    contour_of_points<T> temp_contour;
    temp_contour.closed = true;
    auto j = --( this->points.end() );
    for(auto i = this->points.begin(); i != this->points.end(); j = i++){
        const vec3<T> A = *j;
        const vec3<T> B = *i;
        temp_contour.points.clear();
        temp_contour.points.push_back(C);
        temp_contour.points.push_back(A);
        temp_contour.points.push_back(B);
        const T area = temp_contour.Get_Signed_Area(); //Sign will cancel if it is uniformly oriented.
        const vec3<T> r_a( temp_contour.Average_Point() );

//The problem with these is that the result we get is not a signed distance... (I think.)
//..it is a shame, too, because creating a contour_of_points/populating a list each time is hard on the stack.
//        const T area = ((A-C).Cross(B-C)).length() * (T)(0.5);  //The area of the triangle ABC.
//        const T area = std::sqrt(((A-C).Cross(B-C)).Dot( (A-C).Cross(B-C) )) * (T)(0.5);  //The area of the triangle ABC.

//        const T area = std::sqrt(((A).Cross(B)).Dot( (A).Cross(B) )) * (T)(0.5);  //The area of the triangle ABC.
//        const vec3<T> r_a( (A + B) / (T)(3.0) );               //The center of the triangle.

//FUNCINFO("The current area is " << area << " and the current RA is " << r_a);
        Area += area;
        RA += r_a * area;
    }
    return RA/Area;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template vec3<float> contour_of_points<float>::Centroid(void) const;
    template vec3<double> contour_of_points<double>::Centroid(void) const;
#endif


//This routine will produce a point which lies somewhere within the region of a contour. It may be outside of the contour
// for U-shaped contours, and may not lie in the local plane of the contour. It is best used for quick and dirty location
// of a contour or for checking which side of a plane a spit contour lies on.
template <class T> vec3<T> contour_of_points<T>::First_N_Point_Avg(const long int N) const {
    if((N <= 0) || (N > static_cast<long int>(this->points.size()))){
        FUNCERR("Attempted to average N=" << N << " points. This is not possible. The contour has " << this->points.size() << " points");
    }
    vec3<T> out;
    auto it = this->points.begin();
    long int i = 0;
    while(i < N){
        out += *it;
        ++it;
        ++i;
    }
    return out/static_cast<T>(N);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template vec3<float> contour_of_points<float>::First_N_Point_Avg(const long int N) const;
    template vec3<double> contour_of_points<double>::First_N_Point_Avg(const long int N) const;
#endif

//This routine returns the (positive-or-zero) perimeter of a contour. No attempt it made to ensure the points are in any
// specific order (or orientation.)
//
//This routine can handle empty contours and single points (zero perimeter.)
template <class T>  T contour_of_points<T>::Perimeter(void) const {
    T out = (T)(0);
    if(this->points.size() <= 1) return out;
    for(auto it = this->points.begin(), it2 = ++(this->points.begin()); (it != this->points.end()) && (it2 != this->points.end()); ++it, ++it2){
        out += it->distance(*it2);
    }

    //Handle the last edge, if it is closed.
    if(this->closed){ out += (this->points.begin())->distance( *(this->points.rbegin()) ); }
    return out;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float  contour_of_points<float >::Perimeter(void) const;
    template double contour_of_points<double>::Perimeter(void) const;
#endif

//This routine performs (numerical) contour integration over the contour with the provided (scalar-valued) kernel.
//
// More precisely, this function computes:  $ \Chi = \sum_{i=1}^{N} \Chi_{i} $ where the summation is over the 
// straight line segments between contour points, and 
//  $ \Chi_{i} = \int_{\vec{A_{i}}}^{\vec{B_{i}}}  k( \vec{r}; \vec{A_{i}}, \vec{B_{i}} ) ds $
// or, equivalently,
//  $ \Chi_{i} = \int_{0}^{1} k( \vec{r(t)}; \vec{A_{i}}, \vec{B_{i}} ) q_{i} dt $
// where $ q_{i} \equiv | \vec{B_{i}} - \vec{A_{i}} | \equiv | \vec{U_{i}} |.$
//
//Most importantly, the kernel cannot depend on any additional parameters over the duration of the integration.
// The factors passed to the kernel may or may not be used: U is passed in as a convenience, but it is merely B-A.
//
//NOTE: This routine is robust with regard to whether or not the contour is closed, and can handle fully-3D 
// contours.
//
//NOTE: This is a numerical scheme. The actual integration is the weakest aspect: it should be upgraded ASAP. FIXME.
template <class T>    T contour_of_points<T>::Integrate_Simple_Scalar_Kernel(std::function<    T    (const vec3<T> &r,  const vec3<T> &A, const vec3<T> &B, const vec3<T> &U)> k) const {
    T out = 0.0;

    if(!k){
        FUNCERR("This routine requires a scalar kernel. If no kernel is required, provide a function which always gives 1.0");
    }

    for(auto p1_it = this->points.begin(); p1_it != this->points.end(); ++p1_it){
        //Set up the points. Loop point B (point 2) around to the first point if the contour is closed.
        auto p2_it = p1_it;
        ++p2_it;
        if(p2_it == this->points.end()){
            if(this->closed == true){
                p2_it = this->points.begin();
            }else{
                break;
            }
        }

        const vec3<T> A = *p1_it;
        const vec3<T> B = *p2_it;
        const vec3<T> U = B - A;
        //const vec3<T> U_unit = U.unit();
        const T U_length = U.length();

        //Perform a simple numerical integration.
        T tot = 0.0;
        const T dt = 0.0001;
        for(T t = 0.0; t <= (1.0-dt); t += dt){
            const T t0 = t;
            const T t1 = t + dt;
            const T p0 = k( A + U*t0,   A, B, U);
            const T p1 = k( A + U*t1,   A, B, U);
            tot += 0.5*(p0 + p1)*dt;
        }
        out += tot * U_length;
    }
    return out;
}

#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float  contour_of_points<float >::Integrate_Simple_Scalar_Kernel(std::function< float  (const vec3<float > &r,  const vec3<float > &A, const vec3<float > &B, const vec3<float > &U)> k) const;
    template double contour_of_points<double>::Integrate_Simple_Scalar_Kernel(std::function< double (const vec3<double> &r,  const vec3<double> &A, const vec3<double> &B, const vec3<double> &U)> k) const;
#endif


//This routine performs (numerical) contour integration over the contour with the provided (vector-valued) kernel.
//
// More precisely, this function computes:  $ \Chi = \sum_{i=1}^{N} \Chi_{i} $ where the summation is over the 
// straight line segments between contour points, and 
//  $ \Chi_{i} = \int_{\vec{A_{i}}}^{\vec{B_{i}}}  k( \vec{r}; \vec{A_{i}}, \vec{B_{i}} ) d\vec{r} $
// or, equivalently,
//  $ \Chi_{i} = \int_{0}^{1} k( \vec{r(t)}; \vec{A_{i}}, \vec{B_{i}} ) \vec{U}_{i} dt $
// where $ \vec{U_{i}} \equiv \vec{B_{i}} - \vec{A_{i}} .$
//
//Most importantly, the kernel cannot depend on any additional parameters over the duration of the integration.
// The factors passed to the kernel may or may not be used: U is passed in as a convenience, but it is merely B-A.
//
//NOTE: This routine is robust with regard to whether or not the contour is closed, and can handle fully-3D 
// contours.
//
//NOTE: This is a numerical scheme. The actual integration is the weakest aspect: it should be upgraded ASAP. FIXME.
template <class T>    T contour_of_points<T>::Integrate_Simple_Vector_Kernel(std::function< vec3<T> (const vec3<T> &r,  const vec3<T> &A, const vec3<T> &B, const vec3<T> &U)> k) const{
    T out = 0.0;
    if(!k){
        FUNCERR("This routine requires a vector kernel. If no kernel is required, figure out how to provide a static kernel to do what you want.");
    }

    for(auto p1_it = this->points.begin(); p1_it != this->points.end(); ++p1_it){
        //Set up the points. Loop point B (point 2) around to the first point if the contour is closed.
        auto p2_it = p1_it;
        ++p2_it;
        if(p2_it == this->points.end()){
            if(this->closed == true){
                p2_it = this->points.begin();
            }else{
                break;
            }
        }
        const vec3<T> A = *p1_it;
        const vec3<T> B = *p2_it;
        const vec3<T> U = B - A;
        //const vec3<T> U_unit = U.unit();
        const T U_length = U.length();

        //Perform a simple numerical integration.
        T tot = 0.0;
        const T dt = 0.0001;
        for(T t = 0.0; t <= (1.0-dt); t += dt){
            const T t0 = t;
            const T t1 = t + dt;
            const T p0 = U.Dot( k( A + U*t0,   A, B, U) );
            const T p1 = U.Dot( k( A + U*t1,   A, B, U) );
            tot += 0.5*(p0 + p1)*dt;
        }
        out += tot * U_length;
    }
    return out;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float  contour_of_points<float >::Integrate_Simple_Vector_Kernel(std::function< vec3<float > (const vec3<float > &r,  const vec3<float > &A, const vec3<float > &B, const vec3<float > &U)> k) const;
    template double contour_of_points<double>::Integrate_Simple_Vector_Kernel(std::function< vec3<double> (const vec3<double> &r,  const vec3<double> &A, const vec3<double> &B, const vec3<double> &U)> k) const;
#endif

template <class T>    contour_of_points<T> contour_of_points<T>::Resample_Evenly_Along_Perimeter(const long int N) const {
    //First, do some preliminary checks.
    if(this->points.size() <= 1) FUNCERR("Attempted to perform resampling on a contour with " << this->points.size() << " points. Surely this was not intended!");
    if(N <= 0) FUNCERR("Attempted to perform resampling into " << N << " points. Surely this was not intended!");
    
    //Issue some reasonable warnings. These could be safely removed - we are not losing any info because we are making a copy. Leave it here for testing, though!
//    if(static_cast<float>(N) < 0.2*static_cast<float>(this->points.size())){
//        FUNCWARN("Resampling into less than 1/5th of the original contour point density. Watch out for contour mangling!");
//    }

    decltype(this->points) newpoints;
    const T spacing = this->Perimeter() / static_cast<T>(N);
    const T zero = (T)(0);
    T offset = (T)(0);
    T remain = (T)(0);

    auto itA = this->points.begin(), itB = ++(this->points.begin());
    while(true){
        if(!(itA != this->points.end())){ //Wrap around will only be required if the contour is closed (not always, though.) itB will be the one to wrap!
            FUNCERR("The first iterator wrapped around. This should not happen, and is likely due to round off. There are " << newpoints.size() << "/" << N << " points.");
        }
        if(!(itB != this->points.end())) itB = this->points.begin();

        const line_segment<T> line(*itA, *itB);  //From A to B.
        auto somepoints = line.Sample_With_Spacing(spacing, offset, remain); //'remain' is adjusted each time.
        //FUNCINFO("Spacing is " << spacing << ", offset is " << offset << " and remaining is now " << remain << "  . We got " << somepoints.size() << " points, and the dl between contour points was " << itA->distance(*itB));

        offset = (zero - remain);
        remain = zero;
//        newpoints.splice(newpoints.end(), somepoints);
        while(!somepoints.empty() && (static_cast<long int>(newpoints.size()) < N)){
            auto it = somepoints.begin();
            newpoints.push_back(*it);
            somepoints.erase(it);
        }
        if(static_cast<long int>(newpoints.size()) == N) break;
        ++itA; 
        ++itB;
    }

    contour_of_points<T> out(std::move(newpoints));
    out.closed = this->closed;
    return std::move(out);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template contour_of_points<float > contour_of_points<float >::Resample_Evenly_Along_Perimeter(const long int N) const;
    template contour_of_points<double> contour_of_points<double>::Resample_Evenly_Along_Perimeter(const long int N) const;
#endif

//Scales distance from each point to given point by factor (scale).
template <class T>    contour_of_points<T> contour_of_points<T>::Scale_Dist_From_Point(const vec3<T> &point, T scale) const {
    if(scale < (T)(0.0)){
        FUNCWARN("Passed a negative scaling factor. We cannot compute anything logical with this value. Continuing");
        return contour_of_points<T>();
    }

    contour_of_points<T> out(this->points);
    out.closed = this->closed;
    for(auto p_it = out.points.begin(); p_it != out.points.end(); ++p_it){
        *p_it = (*p_it)*scale + point*((T)(1.0) - scale);
    }
    return std::move(out);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template contour_of_points<float > contour_of_points<float >::Scale_Dist_From_Point(const vec3<float > &, float  scale) const;
    template contour_of_points<double> contour_of_points<double>::Scale_Dist_From_Point(const vec3<double> &, double scale) const;
#endif


//Performs a resampling only if the contour is longer than the resample size.
template <class T>    contour_of_points<T> contour_of_points<T>::Resample_LTE_Evenly_Along_Perimeter(const long int N) const {
    if(static_cast<long int>(this->points.size()) > N){
        return this->Resample_Evenly_Along_Perimeter(N);
    }else{
        return (*this);
    }
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template contour_of_points<float > contour_of_points<float >::Resample_LTE_Evenly_Along_Perimeter(const long int N) const;
    template contour_of_points<double> contour_of_points<double>::Resample_LTE_Evenly_Along_Perimeter(const long int N) const;
#endif


template <class T> contour_of_points<T> &  contour_of_points<T>::operator=(const contour_of_points<T> &rhs) {
    if(this == &rhs) return *this;
    this->closed = rhs.closed;
    this->points = rhs.points;
    return *this;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template contour_of_points<float > & contour_of_points<float >::operator=(const contour_of_points<float > &rhs);
    template contour_of_points<double> & contour_of_points<double>::operator=(const contour_of_points<double> &rhs);
#endif


//This routine attempts to determine equality between contours, ignoring the starting position of the contour. This is a computationally
// expensive task, so some shortcuts might be taken!
template <class T> bool contour_of_points<T>::operator==(const contour_of_points<T> &in) const {
    //Currently, this routine assumes that contours are either ordered or anti-ordered, meaning that this equality
    // *does not* discriminate between contours which are jumbled. Namely, with contours of points A,B,C,D,E,F like
    //    C1 = A B C D E,    C2 = A C E D B,   C3 = A B C D E F    , we consider C1 == C2, but C3 != C2 or C1.
    // This is done by sorting the points into a jumble in some (poorly-defined) way. To 
    // retire this 'feature,' one will need to cycle through each contour until a common point is found, and then
    // cycle through the rest of the points (in order!) to see if a true match exists.
    //
    // The current technique was chosen for it's simplicity and because it is believed it will satisfactorially
    // work in most situations.                                                                                                           FIXME
    
    //Check for basic, quick things to help filter as much as possible.
    if(this->points.size()  != in.points.size() ) return false;
    if(this->points.empty() && in.points.empty()) return true;
    if(this->closed != in.closed) return false;
    if(this == &in) return true;

    //Now copy and sort the points in a systematic way.
    auto Apoints = this->points;
    auto Bpoints = in.points;

//    auto lambda_lt_sort = [](const vec3<T> &a, const vec3<T> &b) -> bool { return (a < b); };
    Apoints.sort(); //lambda_lt_sort);  //Uses a LAME vec3::operator< to sort on vector magnitude!
    Bpoints.sort(); //lambda_lt_sort);  // This is most likely very bad

    for(auto itA = Apoints.begin(), itB = Bpoints.begin(); (itA != Apoints.end()) && (itB != Bpoints.end()); ++itA, ++itB){
        if((*itA) != (*itB)) return false;
    }
    return true;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool contour_of_points<float >::operator==(const contour_of_points<float > &in) const;
    template bool contour_of_points<double>::operator==(const contour_of_points<double> &in) const;
#endif

template <class T> bool contour_of_points<T>::operator!=(const contour_of_points<T> &in) const {
    return !(*this == in);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool contour_of_points<float >::operator!=(const contour_of_points<float > &in) const;
    template bool contour_of_points<double>::operator!=(const contour_of_points<double> &in) const;
#endif

template <class T> bool contour_of_points<T>::operator<(const contour_of_points<T> &rhs) const {
    //Compares the *SIZE* of the contours. Given an equality, the closed curve is always the largest.
    //Compares: [number of points]  [whether one is open / other is closed]  [Perimeter] [Centroid (vec3) <]

    //First, compare the number of points.
    if(this->points.size() != rhs.points.size()) return (this->points.size() < rhs.points.size());

    //Open curves are less than closed curves.
    if(!this->closed &&  rhs.closed) return true;
    if( this->closed && !rhs.closed) return false;

    //Finally, a more expensive technique: perimeter.
    const auto perimeterA = this->Perimeter();
    const auto perimeterB = rhs.Perimeter();
    if(perimeterA != perimeterB) return (perimeterA < perimeterB);

    //Finally, delegate the task to elsewhere: rely on the vec3 operator<.
    return (this->Average_Point() < rhs.Average_Point());
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool contour_of_points<float >::operator<(const contour_of_points<float > &rhs) const;
    template bool contour_of_points<double>::operator<(const contour_of_points<double> &rhs) const;
#endif

//This is a contour-to-plane comparison which is performed on a per-point basis.
// The return value is -1 if contour is below P, 0 if it intersects/crosses it, 1 if it is above.
template <class T> long int contour_of_points<T>::Avoids_Plane(const plane<T> &P) const {
    if(this->points.empty()) FUNCERR("Unable to determine if contour avoids plane or not - there are no points to compare!");
    bool above(false), below(false);
    const vec3<T>  NN(vec3<T>((T)(0), (T)(0), (T)(0)) - P.N_0); //Sign reversed normal.
    const plane<T> PN(NN, P.R_0); //Plane with opposite orientation.

    for(auto it = this->points.begin(); it != this->points.end(); ++it){
        //Update the 'above' and 'below' bools as necessary.
        const auto normal = P.Is_Point_Above_Plane(*it);
        if(normal){
            above = true;
        }else{
            const auto mirror = PN.Is_Point_Above_Plane(*it);
            if(mirror){
                below = true;
            }
        }

        //Check to see if we have found at least one point above and one below. If so, break out early.
        if(above && below) break;
    }

    if(above && !below) return 1;   //Above.
    if(!above && below) return -1;  //Below.

    //if((above && below) || (!above && !below))  return 0;   //Crosses plane!
    return 0;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template long int contour_of_points<float >::Avoids_Plane(const plane<float > &P) const;
    template long int contour_of_points<double>::Avoids_Plane(const plane<double> &P) const;
#endif


template <class T> void contour_of_points<T>::Remove_Sequential_Duplicate_Points(std::function<bool(const vec3<T> &,const vec3<T> &)> Feq){
    if(!Feq) Feq = [](const vec3<T> &A, const vec3<T> &B) -> bool { return A == B; };

    if(this->points.size() <= 3) return; //Not willing to possibly produce a 2-point contour!
    auto p1_it = this->closed ? --(this->points.end())  :    this->points.begin();
    auto p2_it = this->closed ?    this->points.begin() : ++(this->points.begin());
    while(p2_it != this->points.end()){
        if( Feq(*p1_it,*p2_it) ){
            //Remove the duplicate point.
            p2_it = this->points.erase(p2_it);
            if(this->points.size() <= 3) return; //Not willing to possibly produce a 2-point contour!
        }else{
            p1_it = p2_it;
            ++p2_it;
        }
    }
    return;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template void contour_of_points<float >::Remove_Sequential_Duplicate_Points(std::function<bool(const vec3<float > &,const vec3<float > &)> Feq);
    template void contour_of_points<double>::Remove_Sequential_Duplicate_Points(std::function<bool(const vec3<double> &,const vec3<double> &)> Feq);
#endif



//Performs (recursive, if needed) needle removal. In other words, turns
//
//                            B        C
//                   |--<-->--o--<-->--o
//                   |
//     ...-->--o-->--o-->--o-->--...         into       ...-->--o-->--o-->--o-->--...
//             X     A     Y                                    X     A     Y
//                 
// by eliminating the spurious, zero-area A--B--C--B--A path. These can occur when doing 
// boolean operations, merging adjoining contours, etc..
template <class T> void contour_of_points<T>::Remove_Needles(std::function<bool(const vec3<T> &,const vec3<T> &)> Feq){
    if(!Feq) Feq = [](const vec3<T> &A, const vec3<T> &B) -> bool { return A == B; };

    if(this->points.size() <= 3) return; //Not willing to possibly produce a 1-point contour!
    auto p1_it = this->closed ? --(--(this->points.end())) : this->points.begin();
    auto p2_it = this->closed ? this->points.begin()       : ++(++(this->points.begin()));

    while(p2_it != this->points.end()){
        //Find the point in between p1 and p2.
        auto mdl_it = std::next(p1_it);
        if(!(mdl_it != this->points.end())) mdl_it = this->points.begin();

        if( Feq(*p1_it,*p2_it) ){
            //This is a needle, according to the user's criteria.
            // Remove the point in between p1 and p2 and also p2, iterating p2.
            this->points.erase( mdl_it );
            p2_it = this->points.erase(p2_it);
    
            if(this->points.size() <= 3) return; //Not willing to possibly produce a 1-point contour!

            //At this point, p1 and p2 are sequential (ie. there is no seperating point).
            // Move p1 one step back. This helps us deal with long needles more efficiently.
            if(!(p1_it != this->points.begin())){
                p1_it = --(this->points.end());
            }else{
                --p1_it;
            }

            //break;
        }else{
            ++p2_it;
            p1_it = mdl_it;
        }
    }

    return;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template void contour_of_points<float >::Remove_Needles(std::function<bool(const vec3<float > &,const vec3<float > &)> Feq);
    template void contour_of_points<double>::Remove_Needles(std::function<bool(const vec3<double> &,const vec3<double> &)> Feq);
#endif

//Removes points which can be safely removed such that the shape of the contour is not altered.
// These can occur when resampling long lines, merging contours from cartesian grids, or 
// directly from input (which often is aligned with some regularity. 
template <class T> void contour_of_points<T>::Remove_Extraneous_Points(std::function<bool(const vec3<T> &,const vec3<T> &)> Feq){
    if(this->points.size() <= 3) return; //Not willing to possibly produce a 1-point contour!
    if(!Feq) Feq = [](const vec3<T> &A, const vec3<T> &B) -> bool { return A == B; };

    auto p1_it = this->closed ? --(--(this->points.end())) : this->points.begin();
    auto p2_it = this->closed ? this->points.begin()       : ++(++(this->points.begin()));
    for(  ; p2_it != this->points.end(); ++p2_it){
        if(this->points.size() <= 3) return; //Not willing to possibly produce a 1-point contour!

        //Find the point in between p1 and p2.
        auto mdl_it = std::next(p1_it);
        if(!(mdl_it != this->points.end())) mdl_it = this->points.begin();

        const line<T> p1p2(*p1_it,*p2_it);
        const auto dist = p1p2.Distance_To_Point(*mdl_it);
        if(YGORABS(dist) == (T)(0.0)){
            //Remove the extraneous point between p1 and p2. Keep p1 the same.
            this->points.erase( mdl_it );
        }else{
            //Keep the point - it is needed for the contour shape. Iterate p1.
            p1_it = mdl_it;
        }
    }

    return;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template void contour_of_points<float >::Remove_Extraneous_Points(std::function<bool(const vec3<float > &,const vec3<float > &)> Feq);
    template void contour_of_points<double>::Remove_Extraneous_Points(std::function<bool(const vec3<double> &,const vec3<double> &)> Feq);
#endif

//This routine produces a very simple, default plot of the data. If more customization is required, you'll have to look elsewhere!
template <class T> void contour_of_points<T>::Plot(void) const {
    Plotter a_plot;
    a_plot.ss << "# Default, simple plot for Contour of points: " << std::endl;
    for(auto iter = (*this).points.begin(); iter != (*this).points.end(); ++iter){
        a_plot.ss << (*iter).x << " ";
        a_plot.ss << (*iter).y << " ";
        //a_plot.ss << (*iter).z << " ";
        a_plot.ss << std::endl;
    }
    a_plot.Plot();
    return;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template void contour_of_points<float>::Plot(void) const;
    template void contour_of_points<double>::Plot(void) const;
#endif


template <class T> std::string contour_of_points<T>::write_to_string(void) const {
    std::stringstream out;
    //There IS significant spaces in this representation.
    out << "{ contour ";
    out << (this->closed ? "closed " : "open ");
    out << this->points.size() << " "; 
    for(auto it = this->points.begin(); it != this->points.end(); ++it){
        out << *it << " "; 
    }
    out << " }";
    return out.str();
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template std::string contour_of_points<float >::write_to_string(void) const;
    template std::string contour_of_points<double>::write_to_string(void) const;
#endif

//Returns true if it worked/was loaded into the contour, false otherwise.
template <class T> bool contour_of_points<T>::load_from_string(const std::string &in){
    if(in.size() < 19) return false; //Not enough characters to possibly hold a contour.
    if(in.find(' ') == std::string::npos) return false; //All whitespace has been stripped. We cannot parse it now.

    std::stringstream ins(in);
    std::string grbg;
    ins >> grbg >> grbg; // "{" and "contour"
    if(grbg != "contour") return false;
    ins >> grbg; // "closed" or "open"
    if((grbg != "closed") && (grbg != "open")) return false;

    //Here we can be reasonably certain it (looks) like a contour.
    this->points.clear();
    this->closed = (grbg == "closed");
    long int N;
    ins >> N;
    if(N < 0) return false;

    vec3<T> p;
    for(long int i = 0; i < N; ++i){
        ins >> p;
        this->points.push_back(p);
    }
    ins >> grbg; // final "}"
    if(grbg != "}") return false;
    return true;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool contour_of_points<float >::load_from_string(const std::string &in);
    template bool contour_of_points<double>::load_from_string(const std::string &in);
#endif


//---------------------------------------------------------------------------------------------------------------------------
//---------------------- contour_collection: a collection of logically-related contour_of_points  ---------------------------
//---------------------------------------------------------------------------------------------------------------------------
template <class T>    contour_collection<T>::contour_collection(){ }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template contour_collection<float >::contour_collection(void);
    template contour_collection<double>::contour_collection(void);
#endif

template <class T>    contour_collection<T>::contour_collection(const contour_collection<T> &in) : contours(in.contours) { }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template contour_collection<float >::contour_collection(const contour_collection<float > &in);
    template contour_collection<double>::contour_collection(const contour_collection<double> &in);
#endif        

template <class T>    contour_collection<T>::contour_collection(const std::list<contour_of_points<T>> &in) : contours(in) { }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template contour_collection<float >::contour_collection(const std::list<contour_of_points<float >> &in);
    template contour_collection<double>::contour_collection(const std::list<contour_of_points<double>> &in);
#endif


//Member functions.
template <class T>  T contour_collection<T>::Get_Signed_Area(void) const {
    //NOTE: ALL contours must be closed. See the implementation in contour_of_points.
    T Area = (T)(0);
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        Area += c_it->Get_Signed_Area();
    }
    return Area; //NOTE: Do NOT take the absolute value. We want to keep the sign for adding/subtracting/etc.. contour areas!
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float  contour_collection<float>::Get_Signed_Area(void) const;
    template double contour_collection<double>::Get_Signed_Area(void) const;
#endif

//This routine verifies that all contours are counter clockwise. 
//
//NOTE: No effort is made to ensure the ordering of points or contours is sane (how could we?)
template <class T> bool contour_collection<T>::Is_Counter_Clockwise(void) const {
    //First, we compute the (signed) area. If the sign is positive, then the contour is counter-clockwise oriented.
    // Otherwise, it is of zero area or is clockwise.
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        if(!c_it->Is_Counter_Clockwise()) return false;
    }
    return true;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool contour_collection<float>::Is_Counter_Clockwise(void) const;
    template bool contour_collection<double>::Is_Counter_Clockwise(void) const;
#endif

//This routine will cycle through each contour and reorient to be CCW (if required.)
template <class T> void contour_collection<T>::Reorient_Counter_Clockwise(void){
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        if(!c_it->Is_Counter_Clockwise()) c_it->Reorient_Counter_Clockwise();
    }
    return;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template void contour_collection<float>::Reorient_Counter_Clockwise(void);
    template void contour_collection<double>::Reorient_Counter_Clockwise(void);
#endif





//This function sums all the points of all contours and divides by the total number of points, giving the 'average' point. 
// This value has limited use becaue it depends on the density of points in the contour. It is not a true center of volume
// for this reason.
template <class T> vec3<T> contour_collection<T>::Average_Point(void) const {
    vec3<T> out((T)(0), (T)(0), (T)(0));
    long int N = 0;
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        for(auto p_it = c_it->points.begin(); p_it != c_it->points.end(); ++p_it){
            out += (*p_it);
            ++N;
        }
    }
    out /= static_cast<T>(N);
    return out;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template vec3<float> contour_collection<float>::Average_Point(void) const;
    template vec3<double> contour_collection<double>::Average_Point(void) const;
#endif


//This function returns the 'average pointed weighted by the area enclosed by the contours.' In other words, this returns the 
// center of mass of the contours as if they were filled with a homogeneous medium with a planar mass density. See the 
// contour_of_points implementation to learn more.
//
//NOTE: Each contour in the collection is assumed to be oriented as the user desires. Mixing orientation is not advised,
// as it will (probably) shift the computed centroid!
//NOTE: This is NOT the average of the contour centroids, but attempts to legitimately weight each centroid by the area in the
// given contour. This may or may not be a validcentroid, but it is certainly NOT the straight averaging of centroids! 
template <class T> vec3<T> contour_collection<T>::Centroid(void) const {
    T Area = (T)(0);   
    vec3<T> RA((T)(0), (T)(0), (T)(0));
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        const auto area = c_it->Get_Signed_Area();
        Area += area;
        RA   += (c_it->Centroid())*area;
    }
    return RA/Area;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template vec3<float> contour_collection<float>::Centroid(void) const;
    template vec3<double> contour_collection<double>::Centroid(void) const;
#endif


//This routine returns a point which should be contained within the boundary of one of the contours in the collection.
// The point can be OUTSIDE the contour if it is U-shaped. This point is NOT an overall average point or centroid.
// It is useful when dealing with planes for determining if above or below. No split-on-a-plane contours should have
// a U-shape on the edge of the boundary.
//
//NOTE: Uses first N contours with first M points from each. Decent defaults are: N = 1 and M = 3. 
template <class T> vec3<T> contour_collection<T>::Generic_Avg_Boundary_Point(const long int N, const long int M) const {
    if((N <= 0) || (N > static_cast<long int>(this->contours.size()))){
        FUNCERR("Attempted to average N=" << N << " contours. This is not possible. The collection has " << this->contours.size() << " contours");
    }
    vec3<T> out;
    auto it = this->contours.begin();
    long int i = 0;
    while(i < N){
        out += it->First_N_Point_Avg(M);
        ++it;
        ++i;
    }
    return out/static_cast<T>(N);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template vec3<float> contour_collection<float>::Generic_Avg_Boundary_Point(const long int N, const long int M) const;
    template vec3<double> contour_collection<double>::Generic_Avg_Boundary_Point(const long int N, const long int M) const;
#endif

//This routine returns the (positive-or-zero) total perimeter of the contours. No attempt it made to ensure the 
// points are in any specific order (or orientation.) Overall orientation doesn't matter, because we always sum
// the (positive) distance from point to point, regardless of direction/phase.
template <class T>  T contour_collection<T>::Perimeter(void) const {
    T out = (T)(0);
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        out += c_it->Perimeter();
    }
    return out;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float  contour_collection<float >::Perimeter(void) const;
    template double contour_collection<double>::Perimeter(void) const;
#endif

//Simple average of the perimeters of individual contours.
template <class T>  T contour_collection<T>::Average_Perimeter(void) const {
    const auto N = this->contours.size();
    if(N == 0) return (T)(0);
    return (this->Perimeter()/static_cast<T>(N));
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float  contour_collection<float >::Average_Perimeter(void) const;
    template double contour_collection<double>::Average_Perimeter(void) const;
#endif


template <class T>  T contour_collection<T>::Longest_Perimeter(void) const {
    T longest = (T)(0);
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        const auto perim = c_it->Perimeter();
        if(perim > longest) longest = perim;
    }
    return longest;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float  contour_collection<float >::Longest_Perimeter(void) const;
    template double contour_collection<double>::Longest_Perimeter(void) const;
#endif

template <class T> std::list<contour_collection<T>> contour_collection<T>::Split_Along_Plane(const plane<T> &theplane) const {
    //There will be two (or one) collections here. Which piece goes where is tricky to do in general.
    // Those contours with an average point below the plane (ie. have negative signed distance) are first. 
    // Those above (ie. have positive signed distance) are second.
    std::list<contour_collection<T>> out;
    out.push_back(contour_collection<T>()); //Below.
    out.push_back(contour_collection<T>()); //Above.

    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        auto split_cs = c_it->Split_Along_Plane(theplane);
        if(split_cs.empty()) FUNCERR("No contours returned from splitting. We should have one or more");

        for(auto sc_it = split_cs.begin(); sc_it != split_cs.end(); ){
            const auto Rave = sc_it->Average_Point();
            if(!theplane.Is_Point_Above_Plane(Rave)){
                out.front().contours.push_back(*sc_it);
            }else{
                out.back().contours.push_back(*sc_it);
            }
            sc_it = split_cs.erase(sc_it);
        }
    }

    return out;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
template std::list<contour_collection<double>> contour_collection<double>::Split_Along_Plane(const plane<double> &theplane) const;
template std::list<contour_collection<float>> contour_collection<float>::Split_Along_Plane(const plane<float> &theplane) const;
#endif

template <class T> std::list<contour_collection<T>> contour_collection<T>::Split_Against_Ray(const vec3<T> &thenormal) const {
    //There will be two (or one) collections here. Which piece goes where is tricky to do in general, but should be fairly easy to 
    // figure out in most problem domains. Therefore, let the user figure out which part they want. 
    //
    //NOTE: We make no guarantee about the order of the returned contour_collections.
    std::list<contour_collection<T>> out;
    out.push_back(contour_collection<T>());
    out.push_back(contour_collection<T>());
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        auto split_cs = c_it->Split_Against_Ray(thenormal);
        if(split_cs.empty()) FUNCERR("No contours returned from splitting. We should have either one or two");
        out.front().contours.push_back(split_cs.front());    
        split_cs.erase(split_cs.begin());      

        if(split_cs.empty()) continue; //This is OK - it means the splitting plane missed this contour.
        out.back().contours.push_back(split_cs.front());
        split_cs.erase(split_cs.begin());

        if(!split_cs.empty()) FUNCERR("Contour was split into more than 2 parts on a plane. This is not possible");
    }

    return out;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
template std::list<contour_collection<double>> contour_collection<double>::Split_Against_Ray(const vec3<double> &thenormal) const;
template std::list<contour_collection<float>> contour_collection<float>::Split_Against_Ray(const vec3<float> &thenormal) const;
#endif


//Splits contour into an inner (core) and outer (peel) using the cc centroid.
//
//NOTE: Only use this routine on simple, ~spherical/round contours. No ray-casting is performed, so it will not gracefully handle
// large dips or folds (unless a very small core is desired).
//
//NOTE: This routine always creates TWO output ccs. The core is always first. One or more may be empty, so make sure to check!
template <class T> std::list<contour_collection<T>> contour_collection<T>::Split_Into_Core_Peel_Spherical(T frac_dist) const {
    std::list<contour_collection> output;
    if(frac_dist < (T)(0.0)){
        FUNCWARN("Passed a negative scale factor. This is nonsensical and nothing reasonable can be computed. Continuing");
        return std::move(output);
    }else if(frac_dist > (T)(1.0)){
        FUNCWARN("Passed a scale factor > 1.0. This case is not handled and would produce a core larger than peel! Continuing");
        return std::move(output);
    }

    //Prepare space for the inner and outer data.
    output.push_back(contour_collection<T>()); //inner (core).
    output.push_back(contour_collection<T>()); //outer (peel).

    const auto C = this->Centroid();
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        auto c_list = c_it->Split_Into_Core_Peel_Spherical(C, frac_dist);
        if(!(c_list.back().points.empty()))  output.back().contours.push_back(std::move(c_list.back()));
        if(!(c_list.front().points.empty())) output.front().contours.push_back(std::move(c_list.front()));
    }
    return output;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
template std::list<contour_collection<float >> contour_collection<float >::Split_Into_Core_Peel_Spherical(float  frac_dist) const;
template std::list<contour_collection<double>> contour_collection<double>::Split_Into_Core_Peel_Spherical(double frac_dist) const;
#endif


template <class T> contour_collection<T> &  contour_collection<T>::operator=(const contour_collection<T> &rhs){
    if(this == &rhs) return *this;
    this->contours = rhs.contours;
    return *this;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template contour_collection<float > & contour_collection<float >::operator=(const contour_collection<float > &rhs);
    template contour_collection<double> & contour_collection<double>::operator=(const contour_collection<double> &rhs);
#endif


//This routine attempts to determine equality between a collection of contours, ignoring the starting position of each contour.
// This is a computationally expensive task, so some shortcuts might be taken! See the contour_of_points operator== for more
// info.
//
//NOTE: This routine will NOT ignore unsorted contours. This means if the nth contour of the input does not match the nth
// input of the local contours, then they are NOT considered equal!
template <class T> bool contour_collection<T>::operator==(const contour_collection<T> &in) const {
    //First, check the obvious things.
    if(this->contours.size() != in.contours.size()) return false;

    //Now walk through the data and compare piecewise.
    for(auto itA = this->contours.begin(), itB = in.contours.begin(); (itA != this->contours.end()) && (itB != in.contours.end()); ++itA, ++itB){
        if((*itA) != (*itB)) return false;
    }
    return true;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool contour_collection<float >::operator==(const contour_collection<float > &in) const;
    template bool contour_collection<double>::operator==(const contour_collection<double> &in) const;
#endif

template <class T> bool contour_collection<T>::operator!=(const contour_collection<T> &in) const {
    return !(*this == in);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool contour_collection<float >::operator!=(const contour_collection<float > &in) const;
    template bool contour_collection<double>::operator!=(const contour_collection<double> &in) const;
#endif

template <class T> bool contour_collection<T>::operator<(const contour_collection<T> &rhs) const {
    //Compares the number of the contours, and then compares ONLY the first contour. 
    //                                                           NOTE TO SELF: Would it be at all advantageous to sort the 
    //                                                           contours prior to comparing the first two? Will it ever matter??
    if(this->contours.size() != rhs.contours.size()) return (this->contours.size() < rhs.contours.size());
    const auto itA = this->contours.begin(), itB = rhs.contours.begin();
    return ((*itA) < (*itB));
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool contour_collection<float >::operator<(const contour_collection<float > &rhs) const;
    template bool contour_collection<double>::operator<(const contour_collection<double> &rhs) const;
#endif


template <class T>    contour_collection<T> contour_collection<T>::Resample_Evenly_Along_Perimeter(const long int N) const {
    contour_collection<T> out;
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        out.contours.push_back( c_it->Resample_Evenly_Along_Perimeter(N) );
    }
    return out;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template contour_collection<float > contour_collection<float >::Resample_Evenly_Along_Perimeter(const long int N) const;
    template contour_collection<double> contour_collection<double>::Resample_Evenly_Along_Perimeter(const long int N) const;
#endif

//Performs a resampling only if the contour is longer than the resample size.
template <class T>    contour_collection<T> contour_collection<T>::Resample_LTE_Evenly_Along_Perimeter(const long int N) const {
    contour_collection<T> out;
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        out.contours.push_back( c_it->Resample_LTE_Evenly_Along_Perimeter(N) );
    }
    return out;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template contour_collection<float > contour_collection<float >::Resample_LTE_Evenly_Along_Perimeter(const long int N) const;
    template contour_collection<double> contour_collection<double>::Resample_LTE_Evenly_Along_Perimeter(const long int N) const;
#endif


//This is a contour-to-plane comparison which is performed on a per-point basis.
// The return value is -1 if contour is below P, 0 if it intersects/crosses it, 1 if it is above.
template <class T> long int contour_collection<T>::Avoids_Plane(const plane<T> &P) const {
    if(this->contours.empty()) FUNCERR("Unable to determine if contour collection avoids plane or not - there are no points to compare!");

    bool above(false), below(false);
    for(auto it = this->contours.begin(); it != this->contours.end(); ++it){
        const auto avoids = it->Avoids_Plane(P);
        if(avoids == -1) below = true;
        if(avoids ==  1) above = true;

        if(above && below) break;  //Crosses plane. Break early.
    }

    if(above && !below) return  1; //Above.
    if(!above && below) return -1; //Below.
    //if((above && below) || (!above && !below))  return 0;   //Crosses plane!
    return 0;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template long int contour_collection<float >::Avoids_Plane(const plane<float > &P) const;
    template long int contour_collection<double>::Avoids_Plane(const plane<double> &P) const;
#endif


//Given a notion of 'closeness' between points (ie. probably just A==B, but maybe something more exotic)
// we try to merge contours which have (at least) two sequential points in opposing orientations.
//    o-->---o        o-->---o         o------>------o        o------>------o        o------>------o
//    |      |        |      |         |             |        |             |        |             |
//    o->--<-o  ===>  o      o   AND   o-<-->--o-<->-o  ===>  o       o-<->-o  ===>  o             o
//    |      |        |      |         |             |        |             |        |             |
//    o--<---o        o--<---o         o------<------o        o------<------o        o------<------o
// Contours which adjoin themselves are *NOT* merged by this routine. A separate routine should be written
// if/when this is needed (but be careful because it is easy to end up with total islands in this case!).
//
// Note: In the diagram above on the right, you can see how multiple adjoins ("needles") are handled.
// Sometimes unnecessary points will remain in the contours. The user is suggested to use the appropriate
// routine (unnecessary_point_removal, resampling, etc..) AFTER all merging has been completed. It is 
// important to do so AFTER because this routine depends on points on separate contours exactly overlapping.
// Removing some seemingly unnecessary points could inhibit further merging!
template <class T> void contour_collection<T>::Merge_Adjoining_Contours(std::function<bool(const vec3<T> &,const vec3<T> &)> Feq){
    if(this->contours.size() < 2) return; //Nothing to do.
    if(!Feq) Feq = [](const vec3<T> &A, const vec3<T> &B) -> bool { return A == B; };

    bool Altered = true;
    while(Altered == true){ //Keep iterating until nothing happens. We can probably get rid of this with a smarter looping strategy...
        Altered = false;
    
        auto c1_it = this->contours.begin();
        while(c1_it != this->contours.end()){
            if(c1_it->points.size() <= 2){
                ++c1_it;
                continue;
            }
            auto c2_it = std::next(c1_it);
    
            continue_cycling_through:         //  <-------------------- NOTE!
            while(c2_it != this->contours.end()){
                if(c2_it->points.size() <= 2){
                    ++c2_it;
                    continue;
                }
    
                //Cycle through the points of c1 forward and that of c2 backward looking for matches.
                auto p11_it = c1_it->closed ? --(c1_it->points.end()) :    c1_it->points.begin();
                auto p12_it = c1_it->closed ?   c1_it->points.begin() : ++(c1_it->points.begin());
                while(p12_it != c1_it->points.end()){
                    auto p21_it = c2_it->closed ? --(c2_it->points.end()) :    c2_it->points.begin();
                    auto p22_it = c2_it->closed ?   c2_it->points.begin() : ++(c2_it->points.begin());
                    while(p22_it != c2_it->points.end()){
                        if( Feq(*p11_it,*p22_it) && Feq(*p12_it,*p21_it) ){
                            //The contours adjoin, acording to the user's criteria.
                            Altered = true;
    
                            //Rotate c2 so that p22_it is the first point in the list.
                            if(!c2_it->closed) FUNCERR("This routine cannot merge when c2 is not closed. You'll need to figure out how to if this is needed!");
                            //     NOTE: Probably the easiest way is to split the contour into two pieces and perform the merge on each separately. 
                            //     In the general case, this merging could produce some nasty resulting contours! Good luck.
                            std::rotate(c2_it->points.begin(), p22_it, c2_it->points.end());
    
                            //Remove the duplicate points from c2.
                            c2_it->points.erase( c2_it->points.begin() );
                            c2_it->points.erase( --(c2_it->points.end()) );
    
                            //Insert the points from c2 into c1 after p11_it/before p12_it.
                            c1_it->points.splice( p12_it, std::move(c2_it->points) );

                            //NOTE: It may be OK to uncomment the following line. I'm worried that it may eliminate
                            // necessary points for further merging. The benefit would be (I think) increased speed.
                            // Brief testing on cartesian 2D-gridded contours indicated that it seemed to work.
                            // Probably better to leave it the safe way. Speed-up was not drastic...
                            //c1_it->Remove_Needles(Feq);

                            //Erase c2. c1 will remain valid because it is != c2.
                            c2_it = this->contours.erase(c2_it);
    
                            //Jump out of the point loops because the iterators for c2 are now invalid. 
                            goto continue_cycling_through;  //  <-------------------- NOTE!
                        }
                        p21_it = p22_it;
                        ++p22_it;
                    }
                    p11_it = p12_it;
                    ++p12_it;
                }
                ++c2_it;
            }
            ++c1_it;
        }
    }

    //Remove this if enabling the above needle-removal.
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        c_it->Remove_Needles(Feq);
    }
    return;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template void contour_collection<float >::Merge_Adjoining_Contours(std::function<bool(const vec3<float > &,const vec3<float > &)> Feq);
    template void contour_collection<double>::Merge_Adjoining_Contours(std::function<bool(const vec3<double> &,const vec3<double> &)> Feq);
#endif

//Tries to find a rudimentary surface from the contour data. If points are sep. by > min_dist, they are considered not coupled.
// There is zero guarantee that this procedure will use all available points (in fact, it most likely will not). 
// The data is stored as: <three vec3's holding triangle vertices> <one vec3 normal>
//Note: Do NOT use this routine on data with wildly varying sampling rates.
//Note: Do NOT use this routine on data with wildly varying contour separation.
//Note: This routine only "works" for contours in xy plane.
//Note: Do NOT use this routine on disjoint contour sets (ie. has islands).
//Note: This routine assumes that all contours are closed.
template <class T> std::vector<vec3<T>> contour_collection<T>::Generate_Basic_Surface(T min_dist) const {
    std::vector<vec3<T>> out;
    if(this->contours.size() <= 2){
        FUNCWARN("Attempted to perform basic surface reconstruction on data with too few contours (<=2). Continuing");
        return std::move(out);
    }

    //First, sort the data into distinct heights along z.
    typedef typename std::list<contour_of_points<T>>::const_iterator  height_to_c_it_t;
    std::map<T,height_to_c_it_t> slices;
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        const auto z = (c_it->First_N_Point_Avg(3)).Dot( vec3<T>((T)(0),(T)(0),(T)(1)) );
        if(slices.find(z) != slices.end()){
            FUNCWARN("Attempting to perform basic surface reconstruction on data which contains disjoint islands. Ignoring smallest contour at this height and continuing.");
            const auto A1 = c_it->Get_Signed_Area();
            const auto A2 = slices[z]->Get_Signed_Area();
            if(YGORABS(A1) > YGORABS(A2)){
                slices[z] = c_it;
            }
        }else{
            slices[z] = c_it;
        }
    }

    //Now, stepping over the contours from the bottom to the top (or vice versa) we step over the points
    // one by one and find nearest neighbours.
    auto s1_it = slices.begin();
    auto s2_it = ++(slices.begin());
    vec3<T> Last_N2, Last_N1;

    while(s2_it != slices.end()){
        //p0_it is point on slice 1.
        //p1_it is point immediately after p0_it on slice 1.
        auto p0_it = --(s1_it->second->points.end());  
        for(auto p1_it = s1_it->second->points.begin(); p1_it != s1_it->second->points.end(); ++p1_it){

            //Find the two (distinct) nearest points [in the second contour].
            auto N1 = s2_it->second->points.begin(); //Nearest.
            auto N2 = s2_it->second->points.begin(); //Second-nearest. (penultimate nearest).
            T dist_N1(1E50), dist_N2(1E60);
            for(auto p2_it = s2_it->second->points.begin(); p2_it != s2_it->second->points.end(); ++p2_it){
                const auto dist = ((*p0_it + *p1_it)/2.0).distance(*p2_it);
                if(dist < dist_N1){
                    //Bump the previous nearest down the line.
                    dist_N2 = dist_N1;
                    N2 = N1;
       
                    //Update the nearest.
                    dist_N1 = dist;
                    N1 = p2_it;
                    continue;
                }else if(dist < dist_N2){
                    dist_N2 = dist;
                    N2 = p2_it;
                }
            }

            //Now, we have several neighbouring points. Construct surface triangles and continue.
            if( (dist_N1 != 1E50) && (dist_N2 != 1E60) && (dist_N1 != dist_N2) && (N1 != N2) ){
                //Keep orientation consistent!
                out.push_back(*p0_it); out.push_back(*p1_it); out.push_back(*N2); out.push_back((*p1_it - *p0_it).Cross(*N2 - *p0_it)); 
                out.push_back(*p0_it); out.push_back(*N2);    out.push_back(*N1); out.push_back((*N2 - *p0_it).Cross(*N1 - *p0_it)); 

            }else{
                //FUNCWARN(" .... encountered insufficient data for terrible surface reconstruction? :) ...");
                //NOTE: News flash! World's Worst Algorithm Sucks! What did you expect?!
            }
            p0_it = p1_it;
        }
        s1_it = s2_it;
        ++s2_it;

    }
    return std::move(out);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template std::vector<vec3<float >> contour_collection<float >::Generate_Basic_Surface(float  min_dist) const;
    template std::vector<vec3<double>> contour_collection<double>::Generate_Basic_Surface(double min_dist) const;
#endif


template <class T> std::vector<vec3<T>> contour_collection<T>::Generate_Reconstructed_Surface(T qual) const {
    std::vector<vec3<T>> out;
    if(this->contours.size() <= 2){
        FUNCWARN("Attempted to perform basic surface reconstruction on data with too few contours (<=2). Continuing");
        return std::move(out);
    }
   
    const vec3<T> Contour_Separation_Unit((T)(0),(T)(0),(T)(1));

    //First, sort the data into distinct heights along z.
    typedef typename std::list<contour_of_points<T>>::const_iterator  height_to_c_it_t;
    std::map<T,height_to_c_it_t> slices;
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        const auto z = (c_it->First_N_Point_Avg(3)).Dot( Contour_Separation_Unit );
        if(slices.find(z) != slices.end()){
            FUNCWARN("Attempting to perform basic surface reconstruction on data which contains disjoint islands. Ignoring smallest contour at this height and continuing.");
            const auto A1 = c_it->Get_Signed_Area();
            const auto A2 = slices[z]->Get_Signed_Area();
            if(YGORABS(A1) > YGORABS(A2)){
                slices[z] = c_it;
            }
        }else{
            slices[z] = c_it;
        }
    }

    //Now, stepping over the contours from the bottom to the top (or vice versa) we step over the points
    // one by one and find nearest neighbours.
    auto s1_it = slices.begin();
    auto s2_it = ++(slices.begin());
    vec3<T> Last_N2, Last_N1;

    while(s2_it != slices.end()){
        if(!s1_it->second->closed || !s2_it->second->closed){
            FUNCWARN("Performing surface reconstruction on a contour which isn't closed. Treating as closed and continuing!");
        }

//        const auto height_sep = YGORABS(s2_it->first - s1_it-first);

        //Create copies of the contours (so we can later rotate them).
        contour_of_points<T> copy1(*(s1_it->second));
        contour_of_points<T> copy2(*(s2_it->second));

        //Ensure the orientation is the same.
        copy1.Reorient_Counter_Clockwise();
        copy2.Reorient_Counter_Clockwise();

        //Find the two closest points from s1 and s2. Rotate the contours so they are first in each.
        {
          auto p0_s1_it = copy1.points.begin();
          auto p0_s2_it = copy2.points.begin();
          T min_sq_dist = p0_s1_it->sq_dist(*p0_s2_it);
          for(auto p_s1_it = copy1.points.begin(); p_s1_it != copy1.points.end(); ++p_s1_it){
              for(auto p_s2_it = copy2.points.begin(); p_s2_it != copy2.points.end(); ++p_s2_it){
                  const auto sq_dist = p_s1_it->sq_dist(*p_s2_it);
                  if(sq_dist < min_sq_dist){
                      min_sq_dist = sq_dist;
                      p0_s1_it = p_s1_it;
                      p0_s2_it = p_s2_it;
                  }
              }
          }
          std::rotate(copy1.points.begin(), p0_s1_it, copy1.points.end());
          std::rotate(copy2.points.begin(), p0_s2_it, copy2.points.end());
        }

        //Resample the contours evenly.
        const auto num_points1 = static_cast<long int>(copy1.points.size());
        const auto num_points2 = static_cast<long int>(copy2.points.size());
        long int num_points = YGORMAX(num_points1, num_points2);
        num_points = static_cast<long int>( std::round( static_cast<T>(num_points)*qual ) );

        copy1.Resample_Evenly_Along_Perimeter(num_points);
        copy2.Resample_Evenly_Along_Perimeter(num_points);

//        const auto lperim1 = copy1.Average_Perimeter();
//        const auto lperim2 = copy2.Average_Perimeter();

        //Cycle through the (now ordered) points, reconstructing triangle faces.
        // NOTE: These are very naturally GL_TRIANGLE_FANS. Probably a good idea to exploit it!
        //
        //                       p0_s2_it             p1_s2_it
        //  ...--->----o--->-------o--------------------o-------->-----o---->----...     <---Contour s2 (above)
        //                         |              ___---|
        //                         |       ___----      |
        //                         |___----             |
        //  ...--->----o--->-------o--------------------o-------->-----o---->----...     <---Contour s1 (below)
        //                       p0_s1_it             p1_s1_it
        //
        //
        auto p0_s1_it = copy1.points.begin();
        auto p0_s2_it = copy2.points.begin();
        auto p1_s1_it = std::next(p0_s1_it);
        auto p1_s2_it = std::next(p0_s2_it);

        while( (p1_s1_it != copy1.points.end()) && (p1_s2_it != copy2.points.end()) ){
/*
            //If we have become separated by a significant distance due to rapid contour changes, then attempt
            // to jog it a little.
            const auto p_s1_avg = (*p0_s1_it + *p1_s1_it)*0.5;
            const auto p_s2_avg = (*p0_s2_it + *p1_s2_it)*0.5;
            const auto reasonable_dist = 2.0*std::sqrt(height_sep*height_sep + (*p1_s1_it - *p0_s1_it).sq_dist());
//            const auto reasonable_dist1 = 2.0*std::sqrt(height_sep*height_sep + (*p1_s1_it - *p0_s1_it).sq_dist());
            const auto dist = (p_s1_avg - p_s2_avg).distance();
            if(dist > reasonable_dist){ //Jogged too far over...
 
            p0_s1_it = p1_s1_it;
            ++p1_s1_it;

                continue;
            }
*/
            out.push_back(*p0_s1_it); out.push_back(*p1_s1_it); out.push_back(*p1_s2_it); //Lower triangle. 
            out.push_back((*p1_s2_it - *p1_s1_it).Cross(*p0_s1_it - *p1_s1_it).unit()); //Normal

            out.push_back(*p0_s1_it); out.push_back(*p1_s2_it); out.push_back(*p0_s2_it); //Upper triangle. 
            out.push_back((*p0_s1_it - *p0_s2_it).Cross(*p1_s2_it - *p0_s2_it).unit()); //Normal

            p0_s1_it = p1_s1_it;
            ++p1_s1_it;
            p0_s2_it = p1_s2_it;
            ++p1_s2_it;
        }

        s1_it = s2_it;
        ++s2_it;
    }
    return std::move(out);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template std::vector<vec3<float >> contour_collection<float >::Generate_Reconstructed_Surface(float  qual) const;
    template std::vector<vec3<double>> contour_collection<double>::Generate_Reconstructed_Surface(double qual) const;
#endif





//This routine produces a very simple, default plot of the data. If more customization is required, you'll have to look elsewhere!
template <class T> void contour_collection<T>::Plot(const std::string &title) const {
    Plot3_List_of_contour_of_points(this->contours.begin(), this->contours.end(), title);
    return;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template void contour_collection<float >::Plot(const std::string &) const;
    template void contour_collection<double>::Plot(const std::string &) const;
#endif

template <class T> void contour_collection<T>::Plot(void) const {
    this->Plot(""); //No title.
    return;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template void contour_collection<float >::Plot(void) const;
    template void contour_collection<double>::Plot(void) const;
#endif



template <class T> std::string contour_collection<T>::write_to_string(void) const {
    std::stringstream out;
    //There IS significant spaces in this representation.
    out << "{ contour_collection ";
    out << this->contours.size() << " ";
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        out << c_it->write_to_string() << " ENDOFCONTOUR ";
    }
    out << " }";
    return out.str();
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template std::string contour_collection<float >::write_to_string(void) const;
    template std::string contour_collection<double>::write_to_string(void) const;
#endif

//Returns true if it worked/was loaded into the contour, false otherwise.
template <class T> bool contour_collection<T>::load_from_string(const std::string &in){
    if(in.size() < 25) return false; //Not enough characters to possibly hold a contour.
    if(in.find(' ') == std::string::npos) return false; //All whitespace has been stripped. We cannot parse it now.
//    if(in.find("ENDOFCONTOUR") == std::string::npos) return false;  //There are no contours, or formatting is bad.

    std::stringstream ins(in);
    std::string grbg;
    ins >> grbg >> grbg; // "{" and "contour_collection"
    if(grbg != "contour_collection") return false;

    //Here we can be reasonably certain it (looks) like a contour collection.
    this->contours.clear();
    long int N;
    ins >> N;
    if(N < 0) return false;

    for(long int i = 0; i < N; ++i){
        //Loop until the next end of the contour is found.
        std::string raw, contour_string;
        do{
            ins >> raw;
            if(raw != "ENDOFCONTOUR") contour_string += raw + " ";
        }while(raw != "ENDOFCONTOUR");

        //Attempt to parse the accumulated contour.
        contour_of_points<T> contour;
        if(!contour.load_from_string(contour_string)) return false; //Failure to parse!
        this->contours.push_back(contour);
    }
    ins >> grbg; // final "}"
    if(grbg != "}") return false;
    return true;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool contour_collection<float >::load_from_string(const std::string &in);
    template bool contour_collection<double>::load_from_string(const std::string &in);
#endif

//---------------------------------------------------------------------------------------------------------------------------
//--------------------- samples_1D: a convenient way to collect a sequentially-sampled array of data ------------------------
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T>::samples_1D(){ }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template samples_1D<float>::samples_1D(void);
    template samples_1D<double>::samples_1D(void);
#endif

template <class T> samples_1D<T>::samples_1D(const std::list< vec2<T> > &in_samps) : samples(in_samps)  { }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template samples_1D<float>::samples_1D(const std::list< vec2<float> > &in_points);
    template samples_1D<double>::samples_1D(const std::list< vec2<double> > &in_points);
#endif

template <class T> samples_1D<T>::samples_1D(const samples_1D<T> &in) : samples(in.samples)  { }
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template samples_1D<float >::samples_1D(const samples_1D<float > &in);
    template samples_1D<double>::samples_1D(const samples_1D<double> &in);
#endif

template <class T>  void samples_1D<T>::push_back(T x, T y){
    this->samples.push_back( vec2<T>(x,y) );
    return;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template void samples_1D<float >::push_back(float  x, float  y);
    template void samples_1D<double>::push_back(double x, double y);
#endif

template <class T>  bool samples_1D<T>::empty() const {
    return this->samples.empty();
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool samples_1D<float >::empty() const;
    template bool samples_1D<double>::empty() const;
#endif

template <class T>  size_t samples_1D<T>::size() const {
    return this->samples.size();
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template size_t samples_1D<float >::size() const;
    template size_t samples_1D<double>::size() const;
#endif


//Selects those with a key within [L,H] (inclusive), leaving the order intact.
template <class T>  samples_1D<T> samples_1D<T>::Select_Those_Within_Inc(T L, T H) const {
    //Easy, inefficient way: copy everything, prune out those which do not satisfy the criteria.
    samples_1D<T> out(*this);
    auto it = out.samples.begin();
    while(it != out.samples.end()){
        if(!isininc(L,it->x,H)){
            it = out.samples.erase(it);
        }else{
            ++it;
        }
    }
    return std::move(out);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Select_Those_Within_Inc(float  L, float  H) const;
    template samples_1D<double> samples_1D<double>::Select_Those_Within_Inc(double L, double H) const;
#endif

template <class T>   T samples_1D<T>::Interpolate_Linearly(const T &x) const {
    //Step 0 - Sanity checks.
    if(this->samples.size() < 2) FUNCERR("Unable to interpolate - there are less than 2 samples");

    //Step 1 - Check if the point is within the endpoints. 
    const T first_x = (*(this->samples.begin())).x;
    const T last_x  = (*(this->samples.rbegin())).x;

    const bool Ordered_Low_First = (first_x < last_x) ? true : false;
    const T lowest_x  = (first_x < last_x) ? first_x : last_x;
    const T highest_x = (first_x < last_x) ? last_x  : first_x;

    if(!isininc(lowest_x, x, highest_x)){
        //Here will we 'interpolate' to zero. I *think* this is makes the most mathematical sense:
        // the distribution is not defined so it should either be zero or +-infinity. There is only one
        // (mathematical) zero and two infinities. Zero is the unique choice.
        return (T)(0.0);
    }

    //Step 2 - Check if the point is bounded by two samples (or if it is sitting exactly on a sample.)
//    bool bounded = false;
    T x0((T)(0.0)), x1((T)(0.0)), y0((T)(0.0)), y1((T)(0.0)); //Initialized to 0 to silence warnings.
    for(auto it0 = this->samples.begin(); it0 != this->samples.end(); ++it0){
        auto it1 = it0;
        ++it1;

        if(Ordered_Low_First){
            if( isininc( it0->x, x, it1->x ) ){
                //Check if the point is *exactly* on a sample. Important for integer ("bin") samples.
                if( x == it0->x ) return it0->y;
                if( x == it1->x ) return it1->y;

//                bounded = true;
                x0 = it0->x;   x1 = it1->x;
                y0 = it0->y;   y1 = it1->y;
                break;
            }
        }else{
            if( isininc( it1->x, x, it0->x ) ){
                //Check if the point is *exactly* on a sample. Important for integer ("bin") samples.
                if( x == it0->x ) return it0->y;
                if( x == it1->x ) return it1->y;

//                bounded = true;
                x0 = it1->x;   x1 = it0->x;
                y0 = it1->y;   y1 = it0->y;
                break;
            }
        }
    }

    //Step 3 - Given a (distinct) lower and an upper point, we perform the linear interpolation.
    return y0 + ( y1 - y0 ) * (x - x0)/( x1 - x0 );
}

#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float samples_1D<float>::Interpolate_Linearly(const float &x) const;
    template double samples_1D<double>::Interpolate_Linearly(const double &x) const;
#endif

template <class T> void samples_1D<T>::Order_Data_Lowest_First(void){
    this->samples.sort( []( vec2<T> A, vec2<T> B ) -> bool { return A.x < B.x; } );
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template void samples_1D<float>::Order_Data_Lowest_First(void);
    template void samples_1D<double>::Order_Data_Lowest_First(void);
#endif

template <class T> void samples_1D<T>::Order_Data_Lowest_Last(void){
    this->samples.sort( []( vec2<T> A, vec2<T> B ) -> bool { return A.x > B.x; } );
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template void samples_1D<float>::Order_Data_Lowest_Last(void);
    template void samples_1D<double>::Order_Data_Lowest_Last(void);
#endif



//Numerically computes the generic integral: $$ \int_{-inf}^{inf} F(f(x), g(x), x) dx $$ 
// where f(x) (i.e., f_x) are the data in *this and g(x) (i.e., g_x) are the data in a 
// passed-in samples_1D. Often, the functional F() will simply be f(x)*g(x) = f_x*g_x so 
// that we compute the overlap of two numeric distributions 
// $$ \int_{-inf}^{inf} f(x) * g(x) dx $$.
//
// NOTE: Assumes data are sorted on x with lowest first. Will not sort on your behalf and
//       will certainly not compute the correct integral if the data are out of order.
//       This routine ASSUMES the first, last elements are the min, max x elements and
//       should thus not be used for any exotic jumbled-order integrations.
//
// NOTE: This function should also act as a generic backend for other numerical 
//       integration routines. Be aware if you plan to break the api.
//
// NOTE: The "Function F" template needs only to define:  [ T operator(T f_x, T g_x, T x) ]. 
//       A typical function would be the simple overlap integration where F = f_x*g_x. More
//       exotic integrals will involve a kernel of some sort, maybe an exponential so that
//       F = exp(-A*x)*f_x*g_x
//
// NOTE: This routine performs a numerical approximation which might be poor if: 
//         1. your dx's change size rapidly, 
//         2. there is a lot of cancellation and a loss of precision will be significant,
//         3. the f_x and g_x consistently over- or under-represent the true distributions
//            (i.e., if they are totally concave or convex and you're not being careful), etc.
//       In such case, it might be worthwhile to analytically derive the integrals assuming
//       f_x and g_x are connected by line segments. This function cannot handle such an
//       approach. See the other analytical Integrate_...() functions which might help.
//
//History:  
//  - v. 0.0 - simple, inefficient, imprecise midpoint rule w/ interpolation of f and g.
//
template <class T>
template <class Function> T samples_1D<T>::Integrate_Generic(const samples_1D<T> &g, Function F) const {
    //Step 0 - Sanity checks.
    if(this->samples.size() < 2) FUNCERR("Unable to interpolate f - there are less than 2 samples");
    if(g.samples.size() < 2) FUNCERR("Unable to interpolate g - there are less than 2 samples");

    //Step 1 - Establish actual endpoints.
    const T f_first_x = (*(this->samples.begin())).x;
    const T f_last_x  = (*(this->samples.rbegin())).x;
    const T g_first_x = (*(g.samples.begin())).x;
    const T g_last_x  = (*(g.samples.rbegin())).x;

    const T f_lowest_x  = (f_first_x < f_last_x) ? f_first_x : f_last_x;
    const T f_highest_x = (f_first_x < f_last_x) ? f_last_x  : f_first_x;
    const T g_lowest_x  = (g_first_x < g_last_x) ? g_first_x : g_last_x;
    const T g_highest_x = (g_first_x < g_last_x) ? g_last_x  : g_first_x;

    const T lowest_x = (f_lowest_x < g_lowest_x) ? f_lowest_x : g_lowest_x;
    const T highest_x = (f_highest_x > g_highest_x) ? f_highest_x : g_highest_x;

    //Step 2 - Given the bounds, we integrate by interpolation.
    const T dx = (highest_x - lowest_x) / (T)(100.0);
    T res = (T)(0.0);
    for(auto x = lowest_x;  x < (highest_x-0.5*dx); x += dx){
//      res += dx * this->Interpolate_Linearly(x+0.5*dx) * g.Interpolate_Linearly(x+0.5*dx);
        res += dx * F( this->Interpolate_Linearly(x+0.5*dx), g.Interpolate_Linearly(x+0.5*dx), x+0.5*dx );
    }
    return res;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float samples_1D<float>::Integrate_Generic(const samples_1D<float> &g, std::function< float(float, float, float)> F) const;
    template double samples_1D<double>::Integrate_Generic(const samples_1D<double> &g, std::function< double(double, double, double)> F) const;
#endif


template <class T>   T samples_1D<T>::Integrate_Overlap(const samples_1D<T> &g) const {
    auto F = [](T f_x, T g_x, T x) -> T {
        return f_x * g_x;
    };
    //return Integrate_Generic<T>< std::function< T (T, T, T)> >(g, F); //Does not compile.
    //return this->template Integrate_Generic<decltype(F)>(g, F);  //Works, but weird syntax!
    return this->Integrate_Generic<decltype(F)>(g, F); //Works.
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float samples_1D<float>::Integrate_Overlap(const samples_1D<float> &g) const;
    template double samples_1D<double>::Integrate_Overlap(const samples_1D<double> &g) const;
#endif




//Compute the integral: $$ \int_{xmin}^{xmax} exp(A*(x+x0)) * f_x dx $$ where f_x are the data 
// in *this assuming f_x are connected by straight line segments. A numerical evaluation is
// performed using a symbolically-derived pre-computed scheme. There will be no losses in 
// precision due to use of, say, the midpoint scheme. However, the assumption that f_x are
// connected by line segments probably will introduce some errors. This error has the benefit
// of being easily quantifiable if you know the true distribution. One bonus of this routine
// is that it does not need interpolation between f_x points (except possibly at the 
// endpoints) and may thus be faster than a totally-numerical evaluation.
//
// NOTE: Assumes data are sorted on x with lowest first. Will not sort on your behalf and
//       will certainly not compute the correct integral if the data are out of order.
//       This routine ASSUMES the first, last elements are the min, max x elements and
//       should thus not be used for any exotic jumbled-order integrations.
//
// NOTE: Feel free to add more routines with other kernels. Remember that integration is 
//       linear and that you may be able to combine results of various kernels to get what
//       you need. Consider the totally numeric approach if your kernel is nasty.
//
// NOTE: xmin and xmax are integration bounds. If they extend outside the domain of f_x 
//       the contribution of the out-of-domain parts will be zero. In other words, the 
//       distribution f_x is assumed to be zero outside of the explicitly stated domain.
//       This may be inconvenient in specific situations (e.g., an f_x that remains non-zero
//       over the entire range of real numbers), but is the only sane default.
//
template <class T> T samples_1D<T>::Integrate_Over_Kernel_exp(T xmin, T xmax, T A, T x0) const {
    //Step 0 - Sanity checks.
    if(this->samples.size() < 2) FUNCERR("Unable to interpolate f - there are less than 2 samples");

    // The user supplied the integration bounds backward.
    if((xmax - xmin) < (T)(0.0)){
        // The user supplied the integration bounds backwards. Swap them and return.
        return (T)(-1.0)*this->Integrate_Over_Kernel_exp(xmax, xmin, A, x0);
    }else if((xmax - xmin) == (T)(0.0)){
        return (T)(0.0);
    }

    //Step 1 - Establish actual data (f_x) xmin and xmax.
    const T f_low_x  = this->samples.begin()->x;
    const T f_high_x = this->samples.rbegin()->x;

    const T non_trivial_left_bound  = (xmin < f_low_x)  ? f_low_x  : xmin;
    const T non_trivial_right_bound = (xmax > f_high_x) ? f_high_x : xmax;

    //Step 2 - Step over adjacent data pairs, accumulating the contribution from each range.
    //         If the integration bounds are within the range, perform linear interpolation at
    //         the boundar(y|ies).
    //
    //         We will go from left (low x) to right (high x) and STOP when the right boundary
    //         is reached. In normal use this will be OK, but it will fail if the distribution 
    //         is a path that winds back and forth.
    T res = (T)(0.0);
    auto it1 = this->samples.begin();
    auto it2 = std::next(it1);
    bool reached_right_boundary = false;
    T actual_x1, actual_x2;
    T actual_y1, actual_y2;
    for( ; (it2 != this->samples.end()) && !reached_right_boundary; ++it1, ++it2){
        //Get the actual bounds for this range. They might be shortened due to the user-specified
        // integration range. Also set the early exit flag if needed.
        if(it1->x < non_trivial_left_bound){
            actual_x1 = non_trivial_left_bound;
            actual_y1 = this->Interpolate_Linearly(actual_x1);
        }else{
            actual_x1 = it1->x;
            actual_y1 = it1->y;
        }
        if(it2->x > non_trivial_right_bound){
            actual_x2 = non_trivial_right_bound;
            actual_y2 = this->Interpolate_Linearly(actual_x2);
            reached_right_boundary = true;
        }else{
            actual_x2 = it2->x;
            actual_y2 = it2->y;
        }

        //Ensure the dx, dy between points is not going to be problematic.
        const T actual_dy = actual_y2 - actual_y1;
        const T actual_dx = actual_x2 - actual_x1;
        if(actual_dx == (T)(0.0)){
            //No contribution -- no width!
            continue;
        }

        const T slope = actual_dy / actual_dx;
        if(!std::isfinite(slope)){
            FUNCWARN("Encountered difficulty computing slope of line segment. Integral contains infinite contributions");
            return std::numeric_limits<T>::infinity();
        }

        //Now use the CAS-derived result over the range. Ensure it isn't borked due to numerical issues.
        const T den = A*A;
        const T num = (actual_y2*A - slope)*std::exp(A*(actual_x2 + x0)) 
                     -(actual_y1*A - slope)*std::exp(A*(actual_x1 + x0)); 
        const T contrib = num / den;
        if(!std::isfinite(contrib)){
            //Ideally we would FUNCERR() or report the issue with an exception and let the user deal with it.
            // The latter would be preferred because this routine may be used in situations where INF's are 
            // acceptable, such as function fitting with unbounded parameters. For now we'll be sloppy.
            FUNCWARN("Integral contains infinite contributions. Perhaps the 'A' parameter is overflowing std::exp()?");
            return std::numeric_limits<T>::infinity();
        }
        res += contrib;
    }

    return res;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float  samples_1D<float >::Integrate_Over_Kernel_exp(float  xmin, float  xmax, float  A, float  x0) const;
    template double samples_1D<double>::Integrate_Over_Kernel_exp(double xmin, double xmax, double A, double x0) const;
#endif













template <class T>  samples_1D<T> samples_1D<T>::operator=(const samples_1D<T> &rhs){
    if(this == &rhs) return *this;
    this->samples = rhs.samples;
    return *this;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::operator=(const samples_1D<float > &rhs);
    template samples_1D<double> samples_1D<double>::operator=(const samples_1D<double> &rhs);
#endif

template <class T>  samples_1D<T> samples_1D<T>::Sum_With(const samples_1D<T> &g) const {
    //Step 0 - Sanity checks.
    if(g.samples.empty()) return samples_1D<T>(this->samples);
    if(this->samples.empty()) return samples_1D<T>();

    samples_1D<T> out;
    samples_1D<T> gcopy = samples_1D<T>( g.samples );
    samples_1D<T> fcopy = samples_1D<T>( this->samples );

    gcopy.Order_Data_Lowest_First();
    fcopy.Order_Data_Lowest_First();

    const T f_first_x = (*(fcopy.samples.begin())).x;
    const T f_last_x  = (*(fcopy.samples.rbegin())).x;
    const T g_first_x = (*(gcopy.samples.begin())).x;
    const T g_last_x  = (*(gcopy.samples.rbegin())).x;

    const T f_lowest_x  = (f_first_x < f_last_x) ? f_first_x : f_last_x;
    const T f_highest_x = (f_first_x < f_last_x) ? f_last_x  : f_first_x;
    const T g_lowest_x  = (g_first_x < g_last_x) ? g_first_x : g_last_x;
    const T g_highest_x = (g_first_x < g_last_x) ? g_last_x  : g_first_x;

    //Get the part on the left where only one set of samples lies.
    auto f_it = fcopy.samples.begin(); 
    auto g_it = gcopy.samples.begin();

    while( true ){
        const bool fvalid = (f_it != fcopy.samples.end());  
        const bool gvalid = (g_it != gcopy.samples.end());

        if(fvalid && !gvalid){
            out.samples.push_back( *f_it );
            ++f_it;
            continue;
        }else if(gvalid && !fvalid){
            out.samples.push_back( *g_it );
            ++g_it;
            continue;
        }else if(fvalid && gvalid){
            //The point which is left-most gets pushed into the summation.
            if( f_it->x == g_it->x ){
                out.samples.push_back( vec2<T>(f_it->x, f_it->y + g_it->y) );
                ++f_it;
                ++g_it;
                continue;
            }else if( f_it->x < g_it->x ){
                if(isininc(g_lowest_x, f_it->x, g_highest_x)){ //If we can interpolate, then do so. This saves errors on single-point samples.
                    out.samples.push_back( vec2<T>(f_it->x, f_it->y + gcopy.Interpolate_Linearly(f_it->x))  );
                }else{
                    out.samples.push_back( *f_it );
                }
                ++f_it;
                continue;
            }else{
                if(isininc(f_lowest_x, g_it->x, f_highest_x)){ //If we can interpolate, then do so. This saves errors on single-point samples.
                    out.samples.push_back( vec2<T>(g_it->x, g_it->y + fcopy.Interpolate_Linearly(g_it->x))  );
                }else{
                    out.samples.push_back( *g_it );
                }
                ++g_it;
                continue;
            }
        }else if(!fvalid && !gvalid){
            break; //Done.
        }
    }
    return out;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template samples_1D<float> samples_1D<float>::Sum_With(const samples_1D<float> &in) const;
    template samples_1D<double> samples_1D<double>::Sum_With(const samples_1D<double> &in) const;
#endif

template <class T>  samples_1D<T> samples_1D<T>::Subtract(const samples_1D<T> &B) const {
    samples_1D neg(B);
    for(auto it = neg.samples.begin(); it != neg.samples.end(); ++it) it->y = -it->y;
    return this->Sum_With(neg);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Subtract(const samples_1D<float > &in) const;
    template samples_1D<double> samples_1D<double>::Subtract(const samples_1D<double> &in) const;
#endif


//This routine takes a string of number pairs, groups them into equal sized bins, and averages their
// values to produce a single pair at the bin's mid-x-point with height = average of datum within 
// the bin.  Useful for linear regressing noisy data -- but not very statistically sound!
template <class T> samples_1D<T> samples_1D<T>::Resample_Average_Into_N_Equal_Sized_Bins(long int N, bool explicitbins) const {
    //Ensure we are sorted from lowest to highest.
    samples_1D<T> out( this->samples );
    out.Order_Data_Lowest_First();

    //Get the min/max x-value and bin spacing.
    const auto xmin = out.samples.front().x;
    const auto xmax = out.samples.back().x;
    const auto dx   = (xmax - xmin)/static_cast<T>(N);
    out.samples.clear();

    //Cycle through the original data. We make N passes because we cannot use the copied/sorted data.
    // This should be fixed, but the solution is not obvious. Maybe a more functionally pure approach
    // with sorting?
    for(long int i = 0; i < N; ++i){
        T val((T)(0)), num((T)(0));
        const auto btm = xmin + static_cast<T>(i)*dx; //"Bottom" as in left-most. Bottom of the x-ordering.
        const auto top = xmin + static_cast<T>(i+1)*dx;
        for(auto p_it = this->samples.begin(); p_it != this->samples.end(); ++p_it){
            if(isininc(btm, p_it->x, top)){
                val += p_it->y;
                num += (T)(1);
            }
        }
        //We make a dummy variable to handle the case where there were no datum within the bin.
        // There are two possible options in this case:
        //   - Make the height of the bin zero (possibly creating issues for later programs), or
        //   - Ignoring the bin. As in, just not outputting it. This makes bin-width interp. hard.
        // We go with the first option.
        const T height = (num == (T)(0)) ? (T)(0) : val/num;

        //If the user wants a pure resample, omit the bins.
        if(!explicitbins){
            out.samples.push_back(vec2<T>((T)(0.5)*(btm+top), height));

        //Otherwise, if displaying on screen (or similar) we explicitly show the bin edges.
        }else{ // if(num != (T)(0)){
            out.samples.push_back(vec2<T>(btm, (T)(0)) );
            if(height != (T)(0)) out.samples.push_back(vec2<T>(btm, height));
            if(height != (T)(0)) out.samples.push_back(vec2<T>(top, height));
            out.samples.push_back(vec2<T>(top, (T)(0)) );
        }
    }
    return std::move(out);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Resample_Average_Into_N_Equal_Sized_Bins(long int N, bool showbins) const;
    template samples_1D<double> samples_1D<double>::Resample_Average_Into_N_Equal_Sized_Bins(long int N, bool showbins) const;
#endif

//This routine takes a string of number pairs, groups them into equal-datum bins, and averages their
// values to produce a single pair at the bin's mid-x-point with height = average of datum within 
// the bin.  Useful for linear regressing noisy data -- but not very statistically sound!
//
//NOTE: The bins obviously cannot always all be composed of the same number of datum. Therefore
// the bin with the largest x may be incomplete. If you are worried about this, try find a divisor
// before calling this routine.
template <class T> samples_1D<T> samples_1D<T>::Resample_Average_Into_N_Equal_Datum_Bins(long int N, bool explicitbins) const {
    //Ensure we are sorted from lowest to highest.
    samples_1D<T> in( this->samples );
    in.Order_Data_Lowest_First();

    samples_1D<T> out;
    T xmaxprev = in.samples.front().x; //The rightmost edge of the previous bin.
    T val = (T)(0);  //Holds sum of datum y-values.
    T pos = (T)(0);  //Holds sum of datum x-values.
    long int i = 0;  //The number of points currently 'munged' into val and pos.
    for(auto p_it = in.samples.begin(); p_it != in.samples.end(); ++p_it){
        val += p_it->y;
        pos += p_it->x;
        ++i;
        
        if((i == N) || (std::next(p_it) == in.samples.end())){
            if(i == 0) break; //No bin needed (or possible).
            const T biny = val/static_cast<T>(i);
            const T binx = pos/static_cast<T>(i);
            const T binhalfwidth = (binx - xmaxprev);
            
            //If the user wants a pure resample, omit the bins.
            if(!explicitbins){
                out.samples.push_back(vec2<T>(binx, biny));

            //Otherwise, if displaying on screen (or similar) we explicitly show the bin edges.
            }else{ // if(num != (T)(0)){
                out.samples.push_back(vec2<T>(xmaxprev, (T)(0)) );
                if(biny != (T)(0)) out.samples.push_back(vec2<T>(xmaxprev, biny));
                if(biny != (T)(0)) out.samples.push_back(vec2<T>(binx + binhalfwidth, biny));
                out.samples.push_back(vec2<T>(binx + binhalfwidth, (T)(0)) );
            }

            xmaxprev = binx + binhalfwidth;
            val = (T)(0);
            pos = (T)(0);
            i = 0; 
        }
    }
    return std::move(out);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Resample_Average_Into_N_Equal_Datum_Bins(long int N, bool showbins) const;
    template samples_1D<double> samples_1D<double>::Resample_Average_Into_N_Equal_Datum_Bins(long int N, bool showbins) const;
#endif

//Replaces x-values with (integer) rank. N-plicates get an averaged (maybe non-integer) rank.
//
//NOTE: Rank is 0-based!
template <class T> samples_1D<T> samples_1D<T>::Rank_x(void) const {
    samples_1D<T> out(*this);

    //First, sort x's lowest-first and cycle through, replacing the x-value with the rank (1-based integer). 
    // All neighbouring points which share a common x (N-plicates!) get an averaged rank. This is important for
    // statistical purposes and could be filtered out afterward if desired.
    out.Order_Data_Lowest_First(); //Sort on x.
    if(out.samples.size() < 2) return std::move(out);

    for(auto p_it = out.samples.begin(); p_it != out.samples.end(); ){
        const auto X = p_it->x; //The x-value of interest.
        auto p2_it = p_it;      //Point where N-plicates stop.
        size_t num(1);          //Number of N-plicates x.
       
        //Iterate just past the N-plicates. 
        while(((++p2_it) != out.samples.end()) && (p2_it->x == X)) ++num;
        
        //This rank gives the average of N sequential natural numbers.
        auto rank = static_cast<T>(std::distance(out.samples.begin(),p_it));
        rank += (T)(0.5) * static_cast<T>(num - 1); 
        
        //Set the rank and iterate past all the N-plicates.
        for(auto pp_it = p_it; pp_it != p2_it; ++pp_it){
            //FUNCINFO("Assigning x-value " << p_it->x << " a rank of " << rank);
            p_it->x = rank;
            ++p_it;
        }
    }
    return std::move(out);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Rank_x(void) const;
    template samples_1D<double> samples_1D<double>::Rank_x(void) const;
#endif

//Replaces y-values with (integer) rank. N-plicates get an averaged (maybe non-integer) rank.
//
//NOTE: Rank is 0-based!
template <class T> samples_1D<T> samples_1D<T>::Rank_y(void) const {
    samples_1D<T> out(this->Swap_x_and_y().Rank_x().Swap_x_and_y());
    out.Order_Data_Lowest_First();
    return std::move(out);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Rank_y(void) const;
    template samples_1D<double> samples_1D<double>::Rank_y(void) const;
#endif


template <class T> samples_1D<T> samples_1D<T>::Swap_x_and_y(void) const {
    samples_1D<T> out(*this);
    for(auto p_it = out.samples.begin(); p_it != out.samples.end(); ++p_it){
        const auto copyx = p_it->x;
        p_it->x = p_it->y;
        p_it->y = copyx;
    }
    return std::move(out);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Swap_x_and_y(void) const;
    template samples_1D<double> samples_1D<double>::Swap_x_and_y(void) const;
#endif

template <class T> T samples_1D<T>::Average_x(void) const {
    T out((T)(0));
    for(auto p_it = this->samples.begin(); p_it != this->samples.end(); ++p_it){
        out += p_it->x;
    }
    return out / static_cast<T>(this->samples.size());
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float  samples_1D<float >::Average_x(void) const;
    template double samples_1D<double>::Average_x(void) const;
#endif

template <class T> T samples_1D<T>::Average_y(void) const {
    T out((T)(0));
    for(auto p_it = this->samples.begin(); p_it != this->samples.end(); ++p_it){
        out += p_it->y;
    }
    return out / static_cast<T>(this->samples.size());
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template float  samples_1D<float >::Average_y(void) const;
    template double samples_1D<double>::Average_y(void) const;
#endif


//------------------------------- Statistical quantities ---------------------------------
template <class T> std::tuple<T,T,T,T> samples_1D<T>::Spearmans_Rank_Correlation_Coefficient(bool *OK/*=nullptr*/) const {
    //Computes a correlation coefficient suitable for judging correlation (without any underlying model!) when the data is 
    // monotonically increasing or decreasing.
    //
    //See http://en.wikipedia.org/wiki/Spearman's_rank_correlation_coefficient#Definition_and_calculation for more info.
    //
    //Returns three numbers:
    //  1) The coefficient itself (rho),
    //  2) The number of samples used (type T - NOT integer),
    //  3) The z-value,
    //  4) The t-value.
    //
    //NOTE: To compute the P-value from the t-value, use DOF = N-2 where N is given as the second return value.
    //
    if(OK != nullptr) *OK = false;
    const auto ret_on_err = std::make_tuple((T)(-1),(T)(-1),(T)(-1),(T)(-1)); 

    const samples_1D<T> ranked(this->Rank_x().Rank_y());
    const auto avgx = ranked.Average_x();
    const auto avgy = ranked.Average_y();

    T numer((T)(0)), denomA((T)(0)), denomB((T)(0));
    for(auto p_it = ranked.samples.begin(); p_it != ranked.samples.end(); ++p_it){
        numer  += (p_it->x - avgx)*(p_it->y - avgy);
        denomA += (p_it->x - avgx)*(p_it->x - avgx);
        denomB += (p_it->y - avgy)*(p_it->y - avgy);
    }
    const auto rho = numer/std::sqrt(denomA*denomB);
    if(!std::isfinite(rho) || (YGORABS(rho) > (T)(1))){
        if(OK == nullptr) FUNCERR("Found coefficient which was impossible or nan. Bailing");
        FUNCWARN("Found coefficient which was impossible or nan. Bailing");
        return ret_on_err;
    }
    const auto num = static_cast<T>(ranked.samples.size());

    if(ranked.samples.size() < 3){
        if(OK == nullptr) FUNCERR("Unable to compute z-value - too little data");
        FUNCWARN("Unable to compute z-value - too little data");
        return ret_on_err;
    }
    const T zval = std::sqrt((num - 3.0)/1.060)*std::atanh(rho);

    if(ranked.samples.size() < 2){
        if(OK == nullptr) FUNCERR("Unable to compute t-value - too little data");
        FUNCWARN("Unable to compute t-value - too little data");
        return ret_on_err;
    }
    const T tval_n = rho*std::sqrt(num - (T)(2.0));
    const T tval_d = std::sqrt((T)(1.0) - rho*rho);
    const T tval   = std::isfinite((T)(1.0)/tval_d) ? tval_n/tval_d : std::numeric_limits<T>::max(); //Is actually infinite!

    if(OK != nullptr) *OK = true;
    return std::make_tuple(rho, num, zval, tval);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template std::tuple<float ,float ,float ,float > samples_1D<float >::Spearmans_Rank_Correlation_Coefficient(bool *OK) const;
    template std::tuple<double,double,double,double> samples_1D<double>::Spearmans_Rank_Correlation_Coefficient(bool *OK) const;
#endif

//Computes Sxy, Sxx, Syy which are used for linear regression.
template <class T> std::tuple<T,T,T> samples_1D<T>::Compute_Sxy_Sxx_Syy(bool *OK/*=nullptr*/) const {
    if(OK != nullptr) *OK = false;
    const auto ret_on_err = std::make_tuple((T)(-1),(T)(-1),(T)(-1));

    //Ensure the data is suitable.
    if(this->samples.size() < 1){
        if(OK == nullptr) FUNCERR("Unable to calculate Sxy,Sxx,Syy with so few points. Bailing");
        FUNCWARN("Unable to calculate Sxy,Sxx,Syy with so few points. Bailing");
        return ret_on_err;
    }

    const auto mean_x = this->Average_x();
    const auto mean_y = this->Average_y();
    T Sxx((T)(0)), Sxy((T)(0)), Syy((T)(0));
    for(auto v_it = this->samples.begin(); v_it != this->samples.end(); ++v_it){
        Sxy += (v_it->x - mean_x)*(v_it->y - mean_y);
        Sxx += (v_it->x - mean_x)*(v_it->x - mean_x);
        Syy += (v_it->y - mean_y)*(v_it->y - mean_y);
    }
    if(OK != nullptr) *OK = true;
    return std::make_tuple(Sxy,Sxx,Syy);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template std::tuple<float ,float ,float > samples_1D<float >::Compute_Sxy_Sxx_Syy(bool *OK) const;
    template std::tuple<double,double,double> samples_1D<double>::Compute_Sxy_Sxx_Syy(bool *OK) const;
#endif

//Returns the pieces of a linear line: slope (A), intercept (B):  y = x*A + B.
template <class T> std::tuple<T,T> samples_1D<T>::Linear_Least_Squares_Regression(bool *OK/*=nullptr*/) const {
    bool l_OK(false);
    if(OK != nullptr) *OK = false;
    const auto ret_on_err = std::make_tuple((T)(-1),(T)(-1));

    //Ensure the data is suitable.
    if(this->samples.size() < 2){
        if(OK == nullptr) FUNCERR("Unable to perform linear regression with so few points. Bailing");
        FUNCWARN("Unable to perform linear regression with so few points. Bailing");
        return ret_on_err;
    }

    const auto mean_x = this->Average_x();
    const auto mean_y = this->Average_y();
    const auto Sxyxxyy = this->Compute_Sxy_Sxx_Syy(&l_OK);
    if(!l_OK){
        if(OK == nullptr) FUNCERR("Encountered problem computing Sxy,Sxx,Syy, which is needed for linear regression. Bailing");
        FUNCWARN("Encountered problem computing Sxy,Sxx,Syy, which is needed for linear regression. Bailing");
        return ret_on_err;
    }
    const auto Sxy = std::get<0>(Sxyxxyy);
    const auto Sxx = std::get<1>(Sxyxxyy);
    //const auto Syy = std::get<2>(Sxyxxyy); //Not needed.

    const auto slope = Sxy/Sxx;
    if(!std::isfinite(slope)){ //Will catch nan too.
        if(OK == nullptr) FUNCERR("Linear regression computation failed. Slope was nan or infinite. Bailing");
        FUNCWARN("Linear regression computation failed. Slope was nan or infinite. Bailing");
        return ret_on_err;
    }
    const auto intercept = mean_y - slope*mean_x;

    if(OK != nullptr) *OK = true;
    return std::make_tuple(slope,intercept);
}       
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template std::tuple<float ,float > samples_1D<float >::Linear_Least_Squares_Regression(bool *OK) const;
    template std::tuple<double,double> samples_1D<double>::Linear_Least_Squares_Regression(bool *OK) const;
#endif



//Normalizes data so that \int_{-inf}^{inf} f(x) (times) f(x) dx ~= 1.
template <class T>   void samples_1D<T>::Normalize_wrt_Self_Overlap(void){
    const T AA = this->Integrate_Overlap( *this );
    const T A  = sqrt(AA);
    for(auto it = this->samples.begin(); it != this->samples.end(); ++it){
        it->y /= A;
    }
    return;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template void samples_1D<float>::Normalize_wrt_Self_Overlap(void);
    template void samples_1D<double>::Normalize_wrt_Self_Overlap(void);
#endif


//This routine writes the numerical data to file in a 2-column format. It can be directly plotted or
// otherwise manipulated by, say, GNUplot.
//
//NOTE: This routine will not overwrite or append an existing file. It will return 'false' on any error
// or if encountering an existing file.
template <class T>   bool samples_1D<T>::Write_To_File(const std::string &filename) const {
    //Check if the file already exists. 
    if(Does_File_Exist_And_Can_Be_Read(filename)) return false;

    return WriteStringToFile(this->Write_To_String(), filename);
//    std::stringstream out;
//    for(auto it = this->samples.begin(); it != this->samples.end(); ++it){
//        out << it->x << " " << it->y << std::endl;
//    }
//    return WriteStringToFile(out.str(),filename);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template bool samples_1D<float >::Write_To_File(const std::string &filename) const;
    template bool samples_1D<double>::Write_To_File(const std::string &filename) const;
#endif

template <class T>   std::string samples_1D<T>::Write_To_String() const {
    std::stringstream out;
    for(auto it = this->samples.begin(); it != this->samples.end(); ++it){
        out << it->x << " " << it->y << std::endl;
    }
    return out.str();
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template std::string samples_1D<float >::Write_To_String() const;
    template std::string samples_1D<double>::Write_To_String() const;
#endif


//This routine produces a very simple, default plot of the data.
//
//NOTE: No attempt is made to ensure the data is ordered - this may or may not be desirable for the user.
template <class T> void samples_1D<T>::Plot(const std::string &Title) const {
/*
    Plotter a_plot;
    a_plot.ss << "# Default, simple plot for 1D sequential samples: " << std::endl;

    for(auto s_it = this->samples.begin(); s_it != this->samples.end(); ++s_it){
        a_plot.ss << s_it->x << " ";
        a_plot.ss << s_it->y << " ";
        a_plot.ss << std::endl;
    }
    a_plot.Plot();
    return;
*/
/*
    Plotter2 a_plot;
    if(!(Title.empty())) a_plot.Set_Global_Title(Title);
    //a_plot.Set_Current_Line_Title(it->first);
    for(auto s_it = this->samples.begin(); s_it != this->samples.end(); ++s_it){
        a_plot.Insert(s_it->x, s_it->y);
    }
    //a_plot.Next_Line();
    a_plot.Plot();
    return;
*/
    Plotter2 plot_coll;
    if(!Title.empty()) plot_coll.Set_Global_Title(Title);
    plot_coll.Insert_samples_1D(*this);//,"", const std::string &linetype = "");
    plot_coll.Plot();
    return;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template void samples_1D<float >::Plot(const std::string &) const;
    template void samples_1D<double>::Plot(const std::string &) const;
#endif

//This routine produces a very simple, default plot of the data.
//
//NOTE: No attempt is made to ensure the data is ordered - this may or may not be desirable for the user.
template <class T> void samples_1D<T>::Plot_as_PDF(const std::string &Title, const std::string &Filename_In) const {
    Plotter2 plot_coll;
    if(!Title.empty()) plot_coll.Set_Global_Title(Title);
    plot_coll.Insert_samples_1D(*this);//,"", const std::string &linetype = "");
    plot_coll.Plot_as_PDF(Filename_In);
    return;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
//    template void samples_1D<float >::Plot_as_PDF(const std::string &, const std::string &) const;
    template void samples_1D<double>::Plot_as_PDF(const std::string &, const std::string &) const;
#endif



template <class T>    std::ostream & operator<<( std::ostream &out, const samples_1D<T> &L ) {
    //Note: This friend is templated (Y) within the templated class (T). We only
    // care about friend template when T==Y.
    out << "(samples_1D. num_samples= " << L.samples.size() << " ";
    for(auto s_it = L.samples.begin(); s_it != L.samples.end(); ++s_it){
        out << *s_it;
        out << " ";
        /*if(std::next(s_it) != L.samples.end()) out << ", "; */
    }
    out << ")";
    return out;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template std::ostream & operator<<(std::ostream &out, const samples_1D<float > &L );
    template std::ostream & operator<<(std::ostream &out, const samples_1D<double> &L );
#endif

template <class T>    std::istream &operator>>( std::istream &in, samples_1D<T> &L ) {
    //Note: This friend is templated (Y) within the templated class (T). We only
    // care about friend template when T==Y.
    L.samples.clear(); 
    std::string grbg;
    long int N;
    in >> grbg; //'(samples_1D.'
    in >> grbg; //'num_samples='
    in >> N;    //'13'   ...or something...
    for(long int i=0; i<N; ++i){
        vec2<T> shtl;
        in >> shtl;
        L.samples.push_back(shtl);
    }
    in >> grbg; //')'
    return in;
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template std::istream & operator>>(std::istream &out, samples_1D<float > &L );
    template std::istream & operator>>(std::istream &out, samples_1D<double> &L );
#endif



//This function takes a list of (possibly unordered) numbers and returns a histogram with N bars of equal width
// suitable for plotting or further computation. The histogram will be normalized such that each bar will be 
// (number_of_points_represented_by_bar/total_number_of_points). In other words, the occurence rate.
//
//It is very inefficient and should be fixed. I was in a hurry when I wrote it.
template <class T> samples_1D<T> Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const std::list<T> &nums, long int N, bool explicitbins){
    std::list<T> in(nums);
    in.sort();
    samples_1D<T> out;

    //Get the min/max x-value and bin spacing.
    const auto xmin = in.front();
    const auto xmax = in.back();
    const auto dx   = (xmax - xmin)/static_cast<T>(N);
    const auto tot  = static_cast<T>(in.size());

    for(long int i = 0; i < N; ++i){
        T num((T)(0));
        const auto btm = xmin + static_cast<T>(i)*dx;
        const auto top = xmin + static_cast<T>(i+1)*dx;
        for(auto it = in.begin(); it != in.end(); ++it){
            if(isininc(btm, *it, top)){
                num += (T)(1);
            }
        }
        //If the user wants a pure resample, omit the bins.
        if(!explicitbins){
            out.samples.push_back(vec2<T>((T)(0.5)*(btm+top), num/tot));

        //Otherwise, if displaying on screen (or similar) we explicitly show the bin edges.
        }else if(num != (T)(0)){
            out.samples.push_back(vec2<T>(btm, (T)(0)) );
            out.samples.push_back(vec2<T>(btm, num/tot));
            out.samples.push_back(vec2<T>(top, num/tot));
            out.samples.push_back(vec2<T>(top, (T)(0)) );
        }
    }
    return std::move(out);
}
#ifdef YGORMATH_INCLUDE_ALL_SPECIALIZATIONS
    template samples_1D<float > Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const std::list<float > &nums, long int N, bool explicitbins);
    template samples_1D<double> Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const std::list<double> &nums, long int N, bool explicitbins);
#endif




//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////              SANDBOX STUFF              ///////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
    I didn't need this at the moment, but I feel it is something which might be useful for many things.
    Cannot get it to work so I abandoned it for a special case which happened to be applicable (planar_image).

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //This is a generic 3D bounding box routine. It is written in such a way as to 
    // hopefully move it into a more general spot somewhere in YgorMath.cc.

    // Diagram:
    //              _______------D_                               
    //            A __             --__                      
    //           /    --__             ---_                  
    //           |        --__             -_                
    //          /             --__     ___-- C                   
    //          |                  B --     /                
    //       r1  --__             /         |                    
    //         /     --__         |        /            
    //         |         --__    /    ____ |            
    //        /              --_ | ---      r3                   
    //        |                   r2      /             
    //       E __               /         |                  
    //           --__           |        /                  
    //               --__      /       __G           z         
    //                   --__  |  ___--              |  y         
    //                        F --                   | /    
    //                                               |/____x
    // Requirements:
    //   1) The points (A,B,C,D) must be ordered (in either orientation). 
    //   2) The points (E,F,G,H) must be ordered in the SAME orientation as (A,B,C,D).
    //   3) Point A must be next to all of (B,D,E).
    //   4) Point B must be next to all of (A,C,F).
    //   5) Point C must be next to all of (C,D,G).
    //   6) Point D must be next to all of (A,C,H).  (etc.)
    //
    // Notes:
    //   1) This routine does *not* require any of the points to lie on any planes, just
    //      that they are ordered uniformly.
    //   2) We may be given (r1,r2,r3,r4) and a thickness (A + E)*0.5 instead. This limits
    //      the geometry, but is equivalent (ie. we can reconstruct (A,B,C,D,E,F,G,H)).
    //

    const auto r1 = this->position(           0,              0);  // = (A + E)*0.5;
    const auto r2 = this->position(this->rows-1,              0);  // = (B + F)*0.5;
    const auto r3 = this->position(this->rows-1,this->columns-1);  // = (C + G)*0.5;  
    const auto r4 = this->position(           0,this->columns-1);  // = (D + H)*0.5;

    const auto dt = this->pxl_dz; // = |A-E| == |B-F| == etc..

    const auto A  = r1 + ((r2-r1).Cross(r4-r1)).unit()*(dt*0.5);
    const auto B  = A  + (r2-r1);
    const auto C  = A  + (r3-r1);
    const auto D  = A  + (r4-r1);

    const auto E  = r1 - ((r2-r1).Cross(r4-r1)).unit()*(dt*0.5);
    const auto F  = A  + (r2-r1);
    const auto G  = A  + (r3-r1);
    const auto H  = A  + (r4-r1);

    //Now create planes describing the edges. (Arbitraily) choose outward as the 
    // positive orientation. Once we choose the orientation, we must stick with it.
    std::list<plane<double>> planes;
    planes.push_back(plane<double>( ((B-A).Cross(D-A)).unit(), A ));
    planes.push_back(plane<double>( ((E-A).Cross(B-A)).unit(), A ));
    planes.push_back(plane<double>( ((D-A).Cross(E-A)).unit(), A ));

    planes.push_back(plane<double>( ((C-G).Cross(F-G)).unit(), G ));
    planes.push_back(plane<double>( ((F-G).Cross(H-G)).unit(), G ));
    planes.push_back(plane<double>( ((H-G).Cross(C-G)).unit(), G ));

    //Cycle through planes to see if the point is *above* (as per our out-positive
    // orientation).
    for(auto it = planes.begin(); it != planes.end(); ++it){
        if(it->Is_Point_Above_Plane(in)) return false;
    }

    return true;

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*/
