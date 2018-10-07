//YgorMath.cc.

#include <algorithm>   //Needed for std::reverse.
#include <cmath>       //Needed for fabs, signbit, sqrt, etc...
#include <complex>
#include <exception>
#include <experimental/optional>
#include <fstream>
#include <functional>  //Needed for passing kernel functions to integration schemes.
#include <iomanip>     //Needed for std::setw() for pretty-printing.
#include <iterator>
#include <limits>      //Needed for std::numeric_limits::max().
#include <list>
#include <map>
#include <numeric>
#include <stdexcept>
#include <string>      //Needed for stringification routines.
#include <tuple>       //Needed for Spearman's Rank Correlation Coeff, other statistical routines.
#include <unordered_map>
#include <utility>
#include <vector>

#include "YgorFilesDirs.h"  //Used in samples_1D<T>::Write_To_File(...).
#include "YgorMath.h"
#include "YgorMisc.h"    //For the FUNC* and PERCENT_ERR macro functions.
#include "YgorPlot.h"    //A wrapper used for producing plots of contours.
#include "YgorStats.h"
#include "YgorString.h"

//#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    #define YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//#endif


//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------- vec3: A three-dimensional vector -------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//Constructors.
template <class T>    vec3<T>::vec3(){   x=(T)(0);   y=(T)(0);   z=(T)(0);  }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec3<int>::vec3(void);
//    template vec3<long int>::vec3(void);
    template vec3<float>::vec3(void);
    template vec3<double>::vec3(void);
#endif

template <class T>    vec3<T>::vec3(T a, T b, T c) : x(a), y(b), z(c) { }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec3<int>::vec3(int, int, int);
//    template vec3<long int>::vec3(long int, long int, long int);
    template vec3<float>::vec3(float, float, float);
    template vec3<double>::vec3(double, double, double);
#endif
    
template <class T>    vec3<T>::vec3( const vec3<T> &in ) : x(in.x), y(in.y), z(in.z) { }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float>  vec3<float>::Cross(const vec3<float> &in) const ;
    template vec3<double> vec3<double>::Cross(const vec3<double> &in) const ;
#endif
   
template <class T> vec3<T> vec3<T>::Mask(const vec3<T> &in) const {
    const T thex = this->x * in.x;
    const T they = this->y * in.y;
    const T thez = this->z * in.z;
    return vec3<T>( thex, they, thez );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float>  vec3<float>::Mask(const vec3<float> &in) const ;
    template vec3<double> vec3<double>::Mask(const vec3<double> &in) const ;
#endif 
    
template <class T> T vec3<T>::Dot(const vec3<T> &in) const {
    return (*this).x * in.x + (*this).y * in.y + (*this).z * in.z;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float vec3<float>::Dot(const vec3<float> &in) const;
    template double vec3<double>::Dot(const vec3<double> &in) const;
#endif
    
    
template <class T> vec3<T> vec3<T>::unit(void) const {
    const T tot = sqrt(x*x + y*y + z*z);
    return vec3<T>(x/tot, y/tot, z/tot);
} 
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float> vec3<float>::unit(void) const;
    template vec3<double> vec3<double>::unit(void) const;
#endif
    
    
template <class T> T vec3<T>::length(void) const {
    const T tot = sqrt(x*x + y*y + z*z);
    return tot;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float vec3<float>::length(void) const;
    template double vec3<double>::length(void) const;
#endif
    
    
template <class T>  T vec3<T>::distance(const vec3<T> &rhs) const {
    const T dist = sqrt((x-rhs.x)*(x-rhs.x) + (y-rhs.y)*(y-rhs.y) + (z-rhs.z)*(z-rhs.z));
    return dist;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float  vec3<float >::angle(const vec3<float > &rhs, bool *OK) const;
    template double vec3<double>::angle(const vec3<double> &rhs, bool *OK) const;
#endif

template <class T>
vec3<T>
vec3<T>::zero(void) const {
    return vec3<T>( static_cast<T>(0),
                    static_cast<T>(0),
                    static_cast<T>(0) );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float > vec3<float >::zero(void) const;
    template vec3<double> vec3<double>::zero(void) const;
#endif
    
template <class T>
vec3<T>
vec3<T>::rotate_around_x(T angle_rad) const {
    return vec3<T>( this->x,
                    this->y * std::cos(angle_rad) - this->z * std::sin(angle_rad),
                    this->y * std::sin(angle_rad) + this->z * std::cos(angle_rad) );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float > vec3<float >::rotate_around_x(float ) const;
    template vec3<double> vec3<double>::rotate_around_x(double) const;
#endif

template <class T>
vec3<T>
vec3<T>::rotate_around_y(T angle_rad) const {
    return vec3<T>( this->x * std::cos(angle_rad) + this->z * std::sin(angle_rad),
                    this->y,
                  - this->x * std::sin(angle_rad) + this->z * std::cos(angle_rad) );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float > vec3<float >::rotate_around_y(float ) const;
    template vec3<double> vec3<double>::rotate_around_y(double) const;
#endif

template <class T>
vec3<T>
vec3<T>::rotate_around_z(T angle_rad) const {
    return vec3<T>( this->x * std::cos(angle_rad) - this->y * std::sin(angle_rad),
                    this->x * std::sin(angle_rad) + this->y * std::cos(angle_rad),
                    this->z );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float > vec3<float >::rotate_around_z(float ) const;
    template vec3<double> vec3<double>::rotate_around_z(double) const;
#endif

template <class T>
bool 
vec3<T>::GramSchmidt_orthogonalize(vec3<T> &b, vec3<T> &c) const {
    //Using *this as seed, orthogonalize the inputs.
    
    //The first vector is *this.
    const auto UA = *this;
    const auto UB = b - UA * (UA.Dot(b) / UA.Dot(UA));
    const auto UC = c - UA * (UA.Dot(c) / UA.Dot(UA))
                      - UB * (UB.Dot(c) / UB.Dot(UB));
    if(this->isfinite() && b.isfinite() && c.isfinite()){
        b = UB;
        c = UC;
        return true;
    }
    return false;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool vec3<float >::GramSchmidt_orthogonalize(vec3<float > &, vec3<float > &) const;
    template bool vec3<double>::GramSchmidt_orthogonalize(vec3<double> &, vec3<double> &) const;
#endif


template <class T>  std::string vec3<T>::to_string(void) const {
    std::stringstream out;
    out << *this;
    return out.str();
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::string vec3<float >::to_string(void) const;
    template std::string vec3<double>::to_string(void) const;
#endif


//Sets *this and returns a copy.
template <class T>  vec3<T> vec3<T>::from_string(const std::string &in){
    std::stringstream ss;
    ss << in;
    ss >> *this;
    return *this;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float > vec3<float >::from_string(const std::string &in);
    template vec3<double> vec3<double>::from_string(const std::string &in);
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec3<int> & vec3<int>::operator=(const vec3<int> &rhs);
//    template vec3<long int> & vec3<long int>::operator=(const vec3<long int> &rhs);
    template vec3<float> & vec3<float>::operator=(const vec3<float> &rhs);
    template vec3<double> & vec3<double>::operator=(const vec3<double> &rhs);
#endif
    
    
template <class T>    vec3<T> vec3<T>::operator+(const vec3<T> &rhs) const {
    return vec3<T>( (*this).x + rhs.x, (*this).y + rhs.y, (*this).z + rhs.z);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec3<int> vec3<int>::operator+(const vec3<int> &rhs) const;
//    template vec3<long int> vec3<long int>::operator+(const vec3<long int> &rhs) const;
    template vec3<float> vec3<float>::operator+(const vec3<float> &rhs) const;
    template vec3<double> vec3<double>::operator+(const vec3<double> &rhs) const;
#endif

    
template <class T>    vec3<T> & vec3<T>::operator+=(const vec3<T> &rhs) {
    (*this).x += rhs.x;    (*this).y += rhs.y;    (*this).z += rhs.z;
    return *this;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec3<int> & vec3<int>::operator+=(const vec3<int> &rhs);
//    template vec3<long int> & vec3<long int>::operator+=(const vec3<long int> &rhs);
    template vec3<float> & vec3<float>::operator+=(const vec3<float> &rhs);
    template vec3<double> & vec3<double>::operator+=(const vec3<double> &rhs);
#endif
    
    
template <class T> vec3<T> vec3<T>::operator-(const vec3<T> &rhs) const {
    return vec3<T>( (*this).x - rhs.x, (*this).y - rhs.y, (*this).z - rhs.z);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec3<int> vec3<int>::operator-(const vec3<int> &rhs) const;
//    template vec3<long int> vec3<long int>::operator-(const vec3<long int> &rhs) const;
    template vec3<float> vec3<float>::operator-(const vec3<float> &rhs) const;
    template vec3<double> vec3<double>::operator-(const vec3<double> &rhs) const;
#endif

    
template <class T>    vec3<T> & vec3<T>::operator-=(const vec3<T> &rhs) {
    (*this).x -= rhs.x;    (*this).y -= rhs.y;    (*this).z -= rhs.z;
    return *this;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec3<int> vec3<int>::operator*(const int &rhs) const;
//    template vec3<long int> vec3<long int>::operator*(const long int &rhs) const;
    template vec3<float> vec3<float>::operator*(const float &rhs) const;
    template vec3<double> vec3<double>::operator*(const double &rhs) const;
#endif
    
template <class T>    vec3<T> & vec3<T>::operator*=(const T &rhs) {
    (*this).x *= rhs;    (*this).y *= rhs;    (*this).z *= rhs;
    return *this;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec3<int> & vec3<int>::operator*=(const int &rhs);
//    template vec3<long int> & vec3<long int>::operator*=(const long int &rhs);
    template vec3<float> & vec3<float>::operator*=(const float &rhs);
    template vec3<double> & vec3<double>::operator*=(const double &rhs);
#endif
    
    
    
template <class T>    vec3<T> vec3<T>::operator/(const T &rhs) const {
    return vec3<T>(x/rhs,y/rhs,z/rhs);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec3<int> vec3<int>::operator/(const int &rhs) const;
//    template vec3<long int> vec3<long int>::operator/(const long int &rhs) const;
    template vec3<float> vec3<float>::operator/(const float &rhs) const;
    template vec3<double> vec3<double>::operator/(const double &rhs) const;
#endif
    
template <class T>    vec3<T> & vec3<T>::operator/=(const T &rhs) {
    (*this).x /= rhs;    (*this).y /= rhs;    (*this).z /= rhs;
    return *this;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template bool vec3<int>::operator==(const vec3<int> &rhs) const;
//    template bool vec3<long int>::operator==(const vec3<long int> &rhs) const;
    template bool vec3<float>::operator==(const vec3<float> &rhs) const;
    template bool vec3<double>::operator==(const vec3<double> &rhs) const;
#endif
   
template <class T>    bool vec3<T>::operator!=(const vec3<T> &rhs) const {
    return !( *this == rhs );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template bool vec3<int>::operator!=(const vec3<int> &rhs) const;
//    template bool vec3<long int>::operator!=(const vec3<long int> &rhs) const;
    template bool vec3<float>::operator!=(const vec3<float> &rhs) const;
    template bool vec3<double>::operator!=(const vec3<double> &rhs) const;
#endif
 
    
template <class T>    bool vec3<T>::operator<(const vec3<T> &rhs) const {
    //NOTE: Do *NOT* change this unless there is a weakness in this approach. If you need some
    //      special behaviour, implement for your particular needs elsewhere.
    //
    //NOTE: Do *NOT* rely on any particular implementation! This operator is generally only
    //      useful for in maps.
    //

    //Since we are using floating point numbers, we should check for equality before making a 
    // consensus of less-than.
    if(*this == rhs) return false;

    //Approach A: ordering based on the ordering of individual coordinates. We have no choice 
    // but to make preferred directions, which will not always satisfy the particular meaning of
    // the users' code. One issue is that there are multiple comparisons needed. Benefits include
    // naturally ordering into a logically space-filling order, recovering one-dimensional
    // ordering, no chance of overflow, no truncation errors, and no need for computing sqrt().
    if(this->z != rhs.z) return this->z < rhs.z;  
    if(this->y != rhs.y) return this->y < rhs.y;  
    return this->x < rhs.x;

    //Approach B: ordering based on the length of the vector. This is a means of generating a
    // single number that describes each vector. One issue is that unit vectors are all considered
    // equal, which might be considered illogical. Another is that a sqrt() is required and an
    // overflow can happen.
    //return this->length() < rhs.length();

    //NOTE: Although this is a fairly "unsatisfying" result, it appears to properly allow 
    // vec3's to be placed in std::maps, whereas more intuitive methods (x<rhs.x, etc..) do NOT. 
    // If an actual operator< is to be defined, please do NOT overwrite this one (so that we 
    // can continue to put vec3's into std::map and not have garbled output and weird bugs!) 
    //
    //return ( (y < rhs.y) ); //  <--- BAD! (See previous note above ^)
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template bool vec3<int>::operator<(const vec3<int> &rhs) const;
//    template bool vec3<long int>::operator<(const vec3<long int> &rhs) const;
    template bool vec3<float>::operator<(const vec3<float> &rhs) const;
    template bool vec3<double>::operator<(const vec3<double> &rhs) const;
#endif
   
template <class T>    bool vec3<T>::isfinite(void) const {
    return std::isfinite(this->x) && std::isfinite(this->y) && std::isfinite(this->z);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool vec3<float >::isfinite(void) const;
    template bool vec3<double>::isfinite(void) const;
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec2<int>::vec2(void);
//    template vec2<long int>::vec2(void);
    template vec2<float>::vec2(void);
    template vec2<double>::vec2(void);
#endif

template <class T>    vec2<T>::vec2(T a, T b) : x(a), y(b) { }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec2<int>::vec2(int, int);
//    template vec2<long int>::vec2(long int, long int);
    template vec2<float>::vec2(float, float);
    template vec2<double>::vec2(double, double);
#endif
    
template <class T>    vec2<T>::vec2( const vec2<T> &in ) : x(in.x), y(in.y) { }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template std::ostream & operator<<(std::ostream &out, const vec2<int> &L );
//    template std::ostream & operator<<(std::ostream &out, const vec2<long int> &L );
    template std::ostream & operator<<(std::ostream &out, const vec2<float> &L );
    template std::ostream & operator<<(std::ostream &out, const vec2<double> &L );
#endif
    
    
template <class T> T vec2<T>::Dot(const vec2<T> &in) const {
    return (*this).x * in.x + (*this).y * in.y;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float vec2<float>::Dot(const vec2<float> &in) const;
    template double vec2<double>::Dot(const vec2<double> &in) const;
#endif
   
template <class T> vec2<T> vec2<T>::Mask(const vec2<T> &in) const {
    const T thex = this->x * in.x; 
    const T they = this->y * in.y;
    return vec2<T>( thex, they );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec2<float > vec2<float >::Mask(const vec2<float > &in) const ;
    template vec2<double> vec2<double>::Mask(const vec2<double> &in) const ;
#endif
 
    
template <class T> vec2<T> vec2<T>::unit(void) const {
    const T tot = sqrt(x*x + y*y);
    return vec2<T>(x/tot, y/tot);
} 
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec2<float> vec2<float>::unit(void) const;
    template vec2<double> vec2<double>::unit(void) const;
#endif
    
    
template <class T> T vec2<T>::length(void) const {
    const T tot = sqrt(x*x + y*y);
    return tot;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float vec2<float>::length(void) const;
    template double vec2<double>::length(void) const;
#endif
    
    
template <class T>  T vec2<T>::distance(const vec2<T> &rhs) const {
    const T dist = sqrt((x-rhs.x)*(x-rhs.x) + (y-rhs.y)*(y-rhs.y));
    return dist;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float vec2<float>::distance(const vec2<float> &rhs) const;
    template double vec2<double>::distance(const vec2<double> &rhs) const;
#endif
   
template <class T>  T vec2<T>::sq_dist(const vec2<T> &rhs) const {
    return ((x-rhs.x)*(x-rhs.x) + (y-rhs.y)*(y-rhs.y));
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float  vec2<float >::sq_dist(const vec2<float > &rhs) const;
    template double vec2<double>::sq_dist(const vec2<double> &rhs) const;
#endif

template <class T>
vec2<T>
vec2<T>::zero(void) const {
    return vec2<T>( static_cast<T>(0),
                    static_cast<T>(0) );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec2<float > vec2<float >::zero(void) const;
    template vec2<double> vec2<double>::zero(void) const;
#endif
    

template <class T>
vec2<T>
vec2<T>::rotate_around_z(T angle_rad) const {
    return vec2<T>( this->x * std::cos(angle_rad) - this->y * std::sin(angle_rad),
                    this->x * std::sin(angle_rad) + this->y * std::cos(angle_rad) );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec2<float > vec2<float >::rotate_around_z(float ) const;
    template vec2<double> vec2<double>::rotate_around_z(double) const;
#endif

template <class T>  std::string vec2<T>::to_string(void) const {
    std::stringstream out;
    out << *this;
    return out.str();
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::string vec2<float >::to_string(void) const;
    template std::string vec2<double>::to_string(void) const;
#endif

//Sets *this and returns a copy.
template <class T>  vec2<T> vec2<T>::from_string(const std::string &in){
    std::stringstream ss;
    ss << in;
    ss >> *this;
    return *this;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec2<float > vec2<float >::from_string(const std::string &in);
    template vec2<double> vec2<double>::from_string(const std::string &in); 
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec2<int> & vec2<int>::operator=(const vec2<int> &rhs);
//    template vec2<long int> & vec2<long int>::operator=(const vec2<long int> &rhs);
    template vec2<float> & vec2<float>::operator=(const vec2<float> &rhs);
    template vec2<double> & vec2<double>::operator=(const vec2<double> &rhs);
#endif
    
    
template <class T>    vec2<T> vec2<T>::operator+(const vec2<T> &rhs) const {
    return vec2<T>( (*this).x + rhs.x, (*this).y + rhs.y );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec2<int> vec2<int>::operator+(const vec2<int> &rhs) const;
//    template vec2<long int> vec2<long int>::operator+(const vec2<long int> &rhs) const;
    template vec2<float> vec2<float>::operator+(const vec2<float> &rhs) const;
    template vec2<double> vec2<double>::operator+(const vec2<double> &rhs) const;
#endif

    
template <class T>    vec2<T> & vec2<T>::operator+=(const vec2<T> &rhs) {
    (*this).x += rhs.x;    (*this).y += rhs.y;
    return *this;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec2<int> & vec2<int>::operator+=(const vec2<int> &rhs);
//    template vec2<long int> & vec2<long int>::operator+=(const vec2<long int> &rhs);
    template vec2<float> & vec2<float>::operator+=(const vec2<float> &rhs);
    template vec2<double> & vec2<double>::operator+=(const vec2<double> &rhs);
#endif
    
    
template <class T> vec2<T> vec2<T>::operator-(const vec2<T> &rhs) const {
    return vec2<T>( (*this).x - rhs.x, (*this).y - rhs.y);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec2<int> vec2<int>::operator-(const vec2<int> &rhs) const;
//    template vec2<long int> vec2<long int>::operator-(const vec2<long int> &rhs) const;
    template vec2<float> vec2<float>::operator-(const vec2<float> &rhs) const;
    template vec2<double> vec2<double>::operator-(const vec2<double> &rhs) const;
#endif

    
template <class T>    vec2<T> & vec2<T>::operator-=(const vec2<T> &rhs) {
    (*this).x -= rhs.x;    (*this).y -= rhs.y;
    return *this;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec2<int> & vec2<int>::operator-=(const vec2<int> &rhs);
//    template vec2<long int> & vec2<long int>::operator-=(const vec2<long int> &rhs);
    template vec2<float> & vec2<float>::operator-=(const vec2<float> &rhs);
    template vec2<double> & vec2<double>::operator-=(const vec2<double> &rhs);
#endif
    
//------------------------------ overloaded native-types -----------------------------


template <class T>    vec2<T> vec2<T>::operator*(const T &rhs) const {
    return vec2<T>(x*rhs,y*rhs);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec2<int> vec2<int>::operator*(const int &rhs) const;
//    template vec2<long int> vec2<long int>::operator*(const long int &rhs) const;
    template vec2<float> vec2<float>::operator*(const float &rhs) const;
    template vec2<double> vec2<double>::operator*(const double &rhs) const;
#endif
    
template <class T>    vec2<T> & vec2<T>::operator*=(const T &rhs) {
    (*this).x *= rhs;    (*this).y *= rhs;
    return *this;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec2<int> & vec2<int>::operator*=(const int &rhs);
//    template vec2<long int> & vec2<long int>::operator*=(const long int &rhs);
    template vec2<float> & vec2<float>::operator*=(const float &rhs);
    template vec2<double> & vec2<double>::operator*=(const double &rhs);
#endif
    
template <class T>    vec2<T> vec2<T>::operator/(const T &rhs) const {
    return vec2<T>(x/rhs,y/rhs);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec2<int> vec2<int>::operator/(const int &rhs) const;
//    template vec2<long int> vec2<long int>::operator/(const long int &rhs) const;
    template vec2<float> vec2<float>::operator/(const float &rhs) const;
    template vec2<double> vec2<double>::operator/(const double &rhs) const;
#endif
    
template <class T>    vec2<T> & vec2<T>::operator/=(const T &rhs) {
    (*this).x /= rhs;    (*this).y /= rhs;
    return *this;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template vec2<int> & vec2<int>::operator/=(const int &rhs);
//    template vec2<long int> & vec2<long int>::operator/=(const long int &rhs);
    template vec2<float> & vec2<float>::operator/=(const float &rhs);
    template vec2<double> & vec2<double>::operator/=(const double &rhs);
#endif
    
    
    
template <class T>    bool vec2<T>::operator==(const vec2<T> &rhs) const {
    return ( (x == rhs.x) && (y == rhs.y) );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template bool vec2<int>::operator==(const vec2<int> &rhs) const;
//    template bool vec2<long int>::operator==(const vec2<long int> &rhs) const;
    template bool vec2<float>::operator==(const vec2<float> &rhs) const;
    template bool vec2<double>::operator==(const vec2<double> &rhs) const;
#endif
   
template <class T>    bool vec2<T>::operator!=(const vec2<T> &rhs) const {
    return !( *this == rhs );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template bool vec2<int>::operator!=(const vec2<int> &rhs) const;
//    template bool vec2<long int>::operator!=(const vec2<long int> &rhs) const;
    template bool vec2<float>::operator!=(const vec2<float> &rhs) const;
    template bool vec2<double>::operator!=(const vec2<double> &rhs) const;
#endif
 
    
template <class T>    bool vec2<T>::operator<(const vec2<T> &rhs) const {
    //NOTE: Do *NOT* change this unless there is a weakness in this approach. If you need some
    //      special behaviour, implement for your particular needs elsewhere.
    //
    //NOTE: Do *NOT* rely on any particular implementation! This operator is generally only
    //      useful for in maps.
    //

    //Since we are using floating point numbers, we should check for equality before making a 
    // consensus of less-than.
    if(*this == rhs) return false;
    
    //Approach A: ordering based on the ordering of individual coordinates. We have no choice 
    // but to make preferred directions, which will not always satisfy the particular meaning of
    // the users' code. One issue is that there are multiple comparisons needed. Benefits include
    // naturally ordering into a logically space-filling order, recovering one-dimensional
    // ordering, no chance of overflow, no truncation errors, and no need for computing sqrt().
    if(this->y != rhs.y) return this->y < rhs.y;
    return this->x < rhs.x;
    
    //Approach B: ordering based on the length of the vector. This is a means of generating a
    // single number that describes each vector. One issue is that unit vectors are all considered
    // equal, which might be considered illogical. Another is that a sqrt() is required and an
    // overflow can happen.
    //return this->length() < rhs.length();

    //NOTE: Although this is a fairly "unsatisfying" result, it appears to properly allow 
    // vec3's to be placed in std::maps, whereas more intuitive methods (x<rhs.x, etc..) do NOT. 
    // If an actual operator< is to be defined, please do NOT overwrite this one (so that we 
    // can continue to put vec3's into std::map and not have garbled output and weird bugs!) 
    //
    //return ( (y < rhs.y) ); //  <--- BAD! (See previous note above ^)
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template bool vec2<int>::operator<(const vec2<int> &rhs) const;
//    template bool vec2<long int>::operator<(const vec2<long int> &rhs) const;
    template bool vec2<float>::operator<(const vec2<float> &rhs) const;
    template bool vec2<double>::operator<(const vec2<double> &rhs) const;
#endif

    
template <class T>    bool vec2<T>::isfinite(void) const {
    return std::isfinite(this->x) && std::isfinite(this->y);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool vec2<float >::isfinite(void) const;
    template bool vec2<double>::isfinite(void) const;
#endif


//---------------------------------------------------------------------------------------------------------------------------
//----------------------------------------- line: (infinitely-long) lines in 3D space ---------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//Constructors.
template <class T>    line<T>::line(){ }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template line<float>::line(void);
    template line<double>::line(void);
#endif

template <class T>    line<T>::line(const vec3<T> &R_A, const vec3<T> &R_B) : R_0(R_A) {
    vec3<T> temp(R_B);
    temp -= R_A;
    U_0 = temp.unit();
} 
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool line<float >::Closest_Point_To_Line( const line<float > &, vec3<float > &) const;
    template bool line<double>::Closest_Point_To_Line( const line<double> &, vec3<double> &) const;
#endif

template <class T>
vec3<T>
line<T>::Project_Point_Orthogonally( const vec3<T> &R ) const {
    // Projects the given point onto the nearest point on the line.
    const auto RR = R - this->R_0; // RR is a vector with a tail on the line somewhere, same as the line's unit U_0.
    return (this->U_0 * this->U_0.Dot(RR)) + this->R_0;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float > line<float >::Project_Point_Orthogonally( const vec3<float > & ) const;
    template vec3<double> line<double>::Project_Point_Orthogonally( const vec3<double> & ) const;
#endif

//---------------------------------------------------------------------------------------------------------------------------
//------------------------------------ line_segment: (finite-length) lines in 3D space --------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//Constructors.
template <class T>    line_segment<T>::line_segment(){ }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template line_segment<float>::line_segment(void);
    template line_segment<double>::line_segment(void);
#endif

template <class T>    line_segment<T>::line_segment(const vec3<T> &R_A, const vec3<T> &R_B) : t_0(0) {
    this->R_0 = R_A;  
    vec3<T> temp(R_B - R_A);
    
    t_1 = temp.length();
    this->U_0 = temp.unit();
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::list<vec3<float >> line_segment<float >::Sample_With_Spacing(float  spacing, float  offset, float  &remaining) const;
    template std::list<vec3<double>> line_segment<double>::Sample_With_Spacing(double spacing, double offset, double &remaining) const;
#endif


// Checks if the point is within a cylinder centred on the line segment.
template <class T>
bool 
line_segment<T>::Within_Cylindrical_Volume(const vec3<T> &R, T radius) const { 

    //First, check if the point is within the radius of the cylinder while ignoring the line segment endpoints.
    if(this->Distance_To_Point(R) > radius){
        return false;
    }

    //Otherwise, check the cylindrical bounds.
    const auto proj = this->Project_Point_Orthogonally(R);

    //The point is now projected upon the (infinite) line. If the distance to either R0 or R1 is greater than the
    // distance between R0 and R1 then the point is outside the bounds of the line segment. Squared distance too.
    const auto R0 = this->Get_R0();
    const auto R1 = this->Get_R1();
    const auto sq_dist_R0_R1   = R0.sq_dist(R1);
    const auto sq_dist_R0_Proj = R0.sq_dist(proj);
    const auto sq_dist_R1_Proj = R1.sq_dist(proj);
    return (sq_dist_R0_R1 >= sq_dist_R0_Proj) && (sq_dist_R0_R1 >= sq_dist_R1_Proj);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool line_segment<float >::Within_Cylindrical_Volume(const vec3<float > &, float  ) const;
    template bool line_segment<double>::Within_Cylindrical_Volume(const vec3<double> &, double ) const;
#endif    

// Checks if the point is within a cylinder centred on the line segment, or within spheres centred on the vertices.
// (This bounding volume is the shape of a cylindrical pill.)
template <class T>
bool 
line_segment<T>::Within_Pill_Volume(const vec3<T> &R, T radius) const { 

    //First, check if the point is within the radius of the cylinder while ignoring the line segment endpoints.
    if(this->Distance_To_Point(R) > radius){
        return false;
    }

    //The point is now projected upon the (infinite) line. If the distance to either R0 or R1 is greater than the
    // distance between R0 and R1 then the point is outside the bounds of the line segment. Squared distance too.
    const auto proj = this->Project_Point_Orthogonally(R);
    const auto R0 = this->Get_R0();
    const auto R1 = this->Get_R1();
    const auto sq_dist_R0_R1   = R0.sq_dist(R1);
    const auto sq_dist_R0_Proj = R0.sq_dist(proj);
    const auto sq_dist_R1_Proj = R1.sq_dist(proj);
    const auto Within_Cylinder = (sq_dist_R0_R1 >= sq_dist_R0_Proj) && (sq_dist_R0_R1 >= sq_dist_R1_Proj);

    //Check if within the spherical end-caps.
    const auto sq_radius = std::pow(radius,2.0);
    const auto sq_dist_R_R0 = R.sq_dist(R0);
    const auto sq_dist_R_R1 = R.sq_dist(R1);
    const auto Within_Endcaps = (sq_dist_R_R0 <= sq_radius) || (sq_dist_R_R1 <= sq_radius);

    return Within_Cylinder || Within_Endcaps;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool line_segment<float >::Within_Pill_Volume(const vec3<float > &, float  ) const;
    template bool line_segment<double>::Within_Pill_Volume(const vec3<double> &, double ) const;
#endif    

template <class T>    vec3<T> line_segment<T>::Get_R0(void) const {
    //These are here in case I need/want to change the internal storage format later...
    return this->R_0;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float > line_segment<float >::Get_R0(void) const;
    template vec3<double> line_segment<double>::Get_R0(void) const;
#endif

template <class T>    vec3<T> line_segment<T>::Get_R1(void) const {
    return this->R_0 + this->U_0 * (this->t_1 - this->t_0);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float > line_segment<float >::Get_R1(void) const;
    template vec3<double> line_segment<double>::Get_R1(void) const;
#endif

//---------------------------------------------------------------------------------------------------------------------------
//------------------------------------------ sphere: a convex 2D surface in 3D space ----------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

//Constructors.
template <class T>
sphere<T>::sphere(){ }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template sphere<float>::sphere(void);
    template sphere<double>::sphere(void);
#endif

template <class T>
sphere<T>::sphere(const vec3<T> &C, T r) : C_0(C), r_0(r) { }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template sphere<float >::sphere(const vec3<float > &, float );
    template sphere<double>::sphere(const vec3<double> &, double);
#endif

//Member functions.

//This function computes the distance from the sphere surface to any point in 3D space.
//
// Note: If within the surface, a distance of zero is returned.
template <class T>  T sphere<T>::Distance_To_Point( const vec3<T> &R ) const {
    const vec3<T> dR = R - this->C_0;
    const auto l = dR.length();
    return ( (l > this->r_0) ? l : static_cast<T>(0) );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float  sphere<float >::Distance_To_Point(const vec3<float > &) const;
    template double sphere<double>::Distance_To_Point(const vec3<double> &) const;
#endif


// This routine returns the points on the sphere which are also intersected by the line.
// Possible results include 0, 1, and 2 points.
template <class T>
std::vector<vec3<T>>
sphere<T>::Line_Intersections( const line<T> &L ) const {
    std::vector<vec3<T>> out;

    //Find the nearest point along the line to the sphere centre.
    const auto P = L.Project_Point_Orthogonally(this->C_0);

    const auto d = (P - this->C_0).length();
    if(d > this->r_0) return out; // No intersections.
    if(d == this->r_0){ // Should be compared to within some epsilon...
        out.emplace_back(P);
        return out;
    }
    const auto l = std::sqrt( this->r_0 * this->r_0 - d * d);
    out.emplace_back(P + L.U_0 * l);
    out.emplace_back(P - L.U_0 * l);
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::vector<vec3<float >> sphere<float >::Line_Intersections( const line<float > & ) const;
    template std::vector<vec3<double>> sphere<double>::Line_Intersections( const line<double> & ) const;
#endif

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------- plane: 2D planes in 3D space -----------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//Constructors.
template <class T>    plane<T>::plane(){ }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template plane<float>::plane(void);
    template plane<double>::plane(void);
#endif

template <class T>    plane<T>::plane(const plane<T> &P) : N_0(P.N_0), R_0(P.R_0) { }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template plane<float >::plane(const plane<float > &);
    template plane<double>::plane(const plane<double> &);
#endif

template <class T>    plane<T>::plane(const vec3<T> &N_0_in, const vec3<T> &R_0_in) : N_0(N_0_in), R_0(R_0_in) { }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float  plane<float>::Get_Signed_Distance_To_Point(const vec3<float> &R) const;
    template double plane<double>::Get_Signed_Distance_To_Point(const vec3<double> &R) const;
#endif

template <class T>    bool plane<T>::Is_Point_Above_Plane(const vec3<T> &R) const {
    const auto dist = this->Get_Signed_Distance_To_Point(R);
    //Check if exactly on the plane and if on the proper side or not.
    return (dist != (T)(0)) && (std::signbit(dist) == 0);
//    return ( std::signbit( this->Get_Signed_Distance_To_Point( R ) ) == 0 );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool plane<float >::Intersects_With_Line_Once(const line<float > &L, vec3<float > &out) const;
    template bool plane<double>::Intersects_With_Line_Once(const line<double> &L, vec3<double> &out) const;
#endif


template <class T>   vec3<T> plane<T>::Project_Onto_Plane_Orthogonally(const vec3<T> &point) const {
    //Project point (along a normal to this plane) onto a point on this plane.
    //
    // NOTE: You might be able to make this more precise by calling it several times in a row. I would
    //       verify and/or test whether this is the case or not before assuming it will work.
    const T dtp = this->Get_Signed_Distance_To_Point(point);
    return point - this->N_0.unit() * dtp;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float > plane<float >::Project_Onto_Plane_Orthogonally(const vec3<float > &point) const;
    template vec3<double> plane<double>::Project_Onto_Plane_Orthogonally(const vec3<double> &point) const;
#endif


// Fit a plane to a container of vec3<T> using plane-orthogonal least-squares regression.
//
// Note: This type of regression considers residuals orthogonal to the plane rather than
//       aligned with the coordinate axes (as for, e.g., traditional linear regression).
//       This routine should be more robust compared to traditional linear regression, but
//       the results of this planar fit will NOT necessarily (generally) be coincident with
//       traditional linear regression!
//
// Note: This algorithm was described circa 20170924 at 
//       http://www.ilikebigbits.com/blog/2017/9/24/fitting-a-plane-to-noisy-points-in-3d .
//
template <class C> 
plane<typename C::value_type::value_type>
Plane_Orthogonal_Regression(C in){
    using T = typename C::value_type; // e.g., vec3<double>.
    using R = decltype(in.front().x); // e.g., double.

    const auto N = in.size();
    if(N < 3){
        throw std::invalid_argument("This routine requires 3 or more non co-linear points.");
    }

        
    std::vector<R> xs;
    std::vector<R> ys;
    std::vector<R> zs;

    xs.reserve(N);
    ys.reserve(N);
    zs.reserve(N);
    for(const auto &p : in){
        xs.push_back(p.x);
        ys.push_back(p.y);
        zs.push_back(p.z);
    }
    const T centroid( Stats::Sum(xs) / static_cast<R>(N),
                      Stats::Sum(ys) / static_cast<R>(N),
                      Stats::Sum(zs) / static_cast<R>(N) ); // The plane must pass through this point.
    xs.clear();
    ys.clear();
    zs.clear();


    //Elements of the 3x3 covariance matrix.
    std::vector<R> xx_l;
    std::vector<R> xy_l;
    std::vector<R> xz_l;
    std::vector<R> yy_l;
    std::vector<R> yz_l;
    std::vector<R> zz_l;

    xx_l.reserve(N);
    xy_l.reserve(N);
    xz_l.reserve(N);
    yy_l.reserve(N);
    yz_l.reserve(N);
    zz_l.reserve(N);

    for(const auto &p : in){
        const auto r = (p - centroid);
        xx_l.push_back(r.x * r.x);
        xy_l.push_back(r.x * r.y);
        xz_l.push_back(r.x * r.z);
        yy_l.push_back(r.y * r.y);
        yz_l.push_back(r.y * r.z);
        zz_l.push_back(r.z * r.z);
    }
    const R xx = Stats::Sum(xx_l) / static_cast<R>(N);
    const R xy = Stats::Sum(xy_l) / static_cast<R>(N);
    const R xz = Stats::Sum(xz_l) / static_cast<R>(N);
    const R yy = Stats::Sum(yy_l) / static_cast<R>(N);
    const R yz = Stats::Sum(yz_l) / static_cast<R>(N);
    const R zz = Stats::Sum(zz_l) / static_cast<R>(N);

    xx_l.clear();
    xy_l.clear();
    xz_l.clear();
    yy_l.clear();
    yz_l.clear();
    zz_l.clear();

    T weighted_dir( static_cast<R>(0),
                    static_cast<R>(0),
                    static_cast<R>(0) );

    {
        const auto det_x = (yy*zz - yz*yz);
        const T axis_dir( det_x, xz*yz - xy*zz, xy*yz - xz*yy );
        const R posneg = ( (weighted_dir.Dot(axis_dir) < 0 ) ? static_cast<R>(-1) 
                                                             : static_cast<R>(1) );
        weighted_dir += axis_dir * det_x * det_x * posneg;
    }

    {
        const auto det_y = (xx*zz - xz*xz);
        const T axis_dir( xz*yz - xy*zz, det_y, xy*xz - yz*xx );
        const R posneg = ( (weighted_dir.Dot(axis_dir) < 0 ) ? static_cast<R>(-1) 
                                                             : static_cast<R>(1) );
        weighted_dir += axis_dir * det_y * det_y * posneg;
    }

    {
        const auto det_z = (xx*yy - xy*xy);
        const T axis_dir( xy*yz - xz*yy, xy*xz - yz*xx, det_z );
        const R posneg = ( (weighted_dir.Dot(axis_dir) < 0 ) ? static_cast<R>(-1) 
                                                             : static_cast<R>(1) );
        weighted_dir += axis_dir * det_z * det_z * posneg;
    }

    const auto plane_N = weighted_dir.unit();
    if(!std::isfinite( plane_N.length() )){
        throw std::domain_error("Plane not uniquely determined: the datum may be co-linear.");
    }
    return plane<R>(plane_N, centroid);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template plane<double> Plane_Orthogonal_Regression(std::list<vec3<double>> in);
    template plane<float > Plane_Orthogonal_Regression(std::list<vec3<float >> in);

    template plane<double> Plane_Orthogonal_Regression(std::vector<vec3<double>> in);
    template plane<float > Plane_Orthogonal_Regression(std::vector<vec3<float >> in);
#endif 


//---------------------------------------------------------------------------------------------------------------------------
//------------------ contour_of_points: a polygon of line segments in the form of a collection of points --------------------
//---------------------------------------------------------------------------------------------------------------------------
//Constructors.
template <class T>    contour_of_points<T>::contour_of_points() : closed(false) { }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template contour_of_points<float>::contour_of_points(void);
    template contour_of_points<double>::contour_of_points(void);
#endif

template <class T>    contour_of_points<T>::contour_of_points(std::list<vec3<T>> in_points) : points(std::move(in_points)), closed(false) { }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template contour_of_points<float>::contour_of_points(std::list<vec3<float> > in_points);
    template contour_of_points<double>::contour_of_points(std::list<vec3<double> > in_points);
#endif

template <class T>    contour_of_points<T>::contour_of_points(const contour_of_points<T> &in){ // : points(in.points), closed(in.closed), metadata(in.metadata) { }
    *this = in;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template contour_of_points<float >::contour_of_points(const contour_of_points<float > &in);
    template contour_of_points<double>::contour_of_points(const contour_of_points<double> &in);
#endif

//Member functions.
template <class T> T contour_of_points<T>::Get_Signed_Area(bool  /*AssumePlanarContours*/) const {
    //NOTE: This routine computes the 'signed' area with respect to an implicit unit vector (in this case,
    //      (0,0,1) == the z unit). I think computing 'signedness' with respect to this unit vector actually
    //      comes from computing the signed area and observing the sign. The idea of clockwise or counter/
    //      anti-clockwise comes from thinking about a contour as a planar contour with an implicit z-coord.
    //
    //      If there is a definite, unambiguous way to compute the signedness for a fully 3D contour, you 
    //      should implement it here. Otherwise, you should provide a means for the user to choose what they
    //      believe to be the 'positive' direction, and default to (0,0,1)!  FIXME TODO FIXME TODO
    //
    //      (I think a generic routine for 3D will necessarily require projection onto plane, which can get
    //      messy conceptually, but is straightforward to compute. It would require a user-specified plane
    //      then!) Check out the following pages for some leads:
    //          http://en.wikipedia.org/wiki/Orientation_%28vector_space%29
    //          http://en.wikipedia.org/wiki/Sign_convention
    //          http://en.wikipedia.org/wiki/Chirality_%28mathematics%29
    //          http://mathworld.wolfram.com/VectorSpaceOrientation.html
    //          http://en.wikipedia.org/wiki/Rotation_formalisms_in_three_dimensions (maybe not directly useful)
    //
    //      In any case, I don't believe this to be a critical problem. Usually fully 3D applications don't 
    //      actually care about the signedness, or actually want (0,0,1) to denote the positive orientation
    //      direction (as a contour is spun around its x or y axes, one will observe the orientation 'flip'
    //      every pi radians; this is the (0,0,1)-positive menace at work :).
    //
    //      In the meantime, a workaround has been made. If you:
    //      - explicitly specify that each contour is planar (regardless of whether it is true!) then area is 
    //        computed by (slowly) treating the contour as a triangle fan centred at the average point and
    //        summing up the area of individual triangular faces. 
    //      - do NOT explicitly specify planarity, but the contour is actually planar and all points share
    //        a common z-coord, then a fast technique is used to compute signed area. (This is the most safe
    //        and fastest way.)
    //      - do NOT explicitly specify planarity, and the contour is not actually planar, then this 
    //        computation will fail.      
    //
    //NOTE: This method could be substantially cleaned up merging the perimeter-walking algorithm with the
    //      "safe but slow" way. Just figure out the planar contour's normal (using points within the contour)
    //      and then find some local coordinate system in the plane of the contour. Again, you could simply 
    //      use the contour's points to define this. (The first three points could be used for both of these
    //      computations ... OTOH its better to fall back and try other points if needed.)


    //If the polygon is not closed, we complain. This is the easiest way to do it...
    if(!this->closed){
        FUNCERR("Computing the surface area of an unconnected contour is not well-defined. "
                "If this is a mistake, just mark your (implicitly) closed contours as closed."
                "Otherwise, to make it more well-defined, you'll need to specify a surface mesh or "
                "something more topologically well-defined. At time of writing, this is not supported");
    } 

    //If the polygon does not have enough points to form a 2D surface, return a zero. (This is legitimate.)
    if(this->points.size() < 3) return (T)(0);

    //If the polygon has only three points, we can easily and robustly compute the signed area regardless of the
    // orientation. It is half the area of the parallelogram formed by the cross product of edge vectors.
    if(this->points.size() == 3){
        //The unsigned area.
        const auto A = this->points.front(); //*std::next(this->points.begin(),0);
        const auto B = *std::next(this->points.begin(),1);
        const auto C = this->points.back(); //*std::next(this->points.begin(),2);
        const auto area_vec = (B - A).Cross(C - A);
        const auto unsigned_area = (T)(0.5)*(area_vec.length());

        //Figure out the sign. We cannot rely on this->Is_Counter_Clockwise() because it calls this routine...
        const auto assumed_positive_direction = vec3<T>((T)(0),(T)(0),(T)(1));   
        const auto dotprod = assumed_positive_direction.Dot(area_vec);
        const auto signed_area = std::copysign(unsigned_area, dotprod);
        return signed_area;
    }

    //If the user has provided an override, attempt to fallback to the safest (but also slowest) way to compute area.
    //
    // NOTE: this special routine could be obviated in liey of the perimeter-walking method following. TODO FIXME.
    if(true){ //AssumePlanarContours){
#pragma message "Warning - UGLY HACK FOR SKEW CONTOURS! FIXME! TODO!"

        //const auto C = this->Average_Point();
        const auto C = this->Centroid();

        auto itA = std::prev(this->points.end());
        auto itB = this->points.begin();
        T signed_area = (T)(0);
        std::vector<T> dArea;
        while(itB != this->points.end()){
            //Compute the area by treating the contour as a triangle fan and summing the contribution from each
            // triangular patch. Since we are told the contour is planar, we can use any point on the plane as
            // a reference point which connects to all triangle patches (i.e., the "spoke" point).
            contour_of_points<T> shtl;
            shtl.closed = true;
            shtl.points = { *itA, *itB, C };
            dArea.push_back(shtl.Get_Signed_Area());
            itA = itB;
            std::advance(itB,1);
        }
        return Stats::Sum(dArea); //signed_area;
    }

    //Otherwise, attempt to use a fast boundary walking method. This ONLY works if the contour is planar. The current
    // implementation assumes the plane is normal to (0,0,1). 
    T Area = (T)(0);
    auto iter_1 = --(this->points.end());
    const T specific_height = iter_1->z; //Used to abandon the computation if the contour is fully 3D.
    for( auto iter_2 = this->points.begin(); iter_2 != this->points.end(); ++iter_2 ){
        const vec3<T> r_a(*iter_1);
        const vec3<T> r_b(*iter_2);
        if( std::fabs(PERCENT_ERR(r_a.z, specific_height)) > 0.1 ){  //Our criteria for same height is a |percent error| < 0.1%.
            FUNCERR("This routine is unable to compute generic (signed) areas for fully 3D contours: found a contour with height "
                    << r_a.z << " when the general contour height is " << specific_height << ". "
                    << "This routine assumes the area of interest lies in an XY plane and chokes if Z-coords are not very close. "
                    << "Fix me if you really need this!");
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float  contour_of_points<float>::Get_Signed_Area(bool AssumePlanarContours) const;
    template double contour_of_points<double>::Get_Signed_Area(bool AssumePlanarContours) const;
#endif


template <class T> bool contour_of_points<T>::Is_Counter_Clockwise(void) const {
    //First, we compute the (signed) area. If the sign is positive, then the contour is counter-clockwise oriented.
    // Otherwise, it is of zero area or is clockwise.
    const T area = this->Get_Signed_Area();
    if( area < (T)(0.0) ) return false;
    return true;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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

    //Carry on the parent's metadata.
    for(auto c_it = output.begin(); c_it != output.end(); ++c_it){
        c_it->metadata = this->metadata;
    }

    return std::move(output);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
template std::list< contour_of_points<double>> contour_of_points<double>::Split_Along_Plane( const plane<double> &theplane ) const;
template std::list< contour_of_points<float>> contour_of_points<float>::Split_Along_Plane( const plane<float> &theplane ) const;
#endif




template <class T> contour_of_points<T> contour_of_points<T>::Bounding_Box_Along( const vec3<T> &r_n, T margin/* = (T)(0.0)*/ ) const {
    contour_of_points<T> bounding_box;
    bounding_box.closed = true;
    bounding_box.metadata = this->metadata;

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

#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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

    //Carry on the parent's metadata.
    for(auto c_it = output.begin(); c_it != output.end(); ++c_it){
        c_it->metadata = this->metadata;
    }

    return std::move(output);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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

    //Carry on the parent's metadata.
    for(auto c_it = output.begin(); c_it != output.end(); ++c_it){
        c_it->metadata = this->metadata;
    }

    return std::move(output);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::list<contour_of_points<float >> contour_of_points<float >::Split_Into_Core_Peel_Spherical(const vec3<float > &, float  frac_dist) const;
    template std::list<contour_of_points<double>> contour_of_points<double>::Split_Into_Core_Peel_Spherical(const vec3<double> &, double frac_dist) const;
#endif


//This function sums all the contour points and divides by the number of points, giving the 'average' point. This could be used as a 
// center point, a rough indication of 'where' a contour is, or as a means of rotating the contour.
template <class T> vec3<T> contour_of_points<T>::Average_Point(void) const {
    if(this->points.empty()) return vec3<T>( std::numeric_limits<T>::quiet_NaN(),
                                             std::numeric_limits<T>::quiet_NaN(),
                                             std::numeric_limits<T>::quiet_NaN() );

    vec3<T> out((T)(0), (T)(0), (T)(0));
    for(auto iter = (*this).points.begin(); iter != (*this).points.end(); ++iter){
        out += (*iter);
    }
    out /= (*this).points.size();
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float  contour_of_points<float >::Perimeter(void) const;
    template double contour_of_points<double>::Perimeter(void) const;
#endif

//Get an arbitrary but deterministic point within the contour. This is useful for applications that require partitioning
// into 'inside' and 'outside' spaces. A construction is used so that points exactly on or near edges or vertices are
// avoided, but may still be reported by this routine.
//
// Note: this routine assumes contours are planar or nearly planar; points are checked for inclusivity assuming a
// plane that is best-fit using an estimate normal. So this routine may fail or return nonsense if contours are not
// sufficiently planar.
//
template <class T>  vec3<T> contour_of_points<T>::Get_Point_Within_Contour(void) const {
    const auto thenormal = this->Estimate_Planar_Normal();
    const auto theplane = this->Least_Squares_Best_Fit_Plane(thenormal);
    const auto proj_cont = this->Project_Onto_Plane_Orthogonally(theplane);
    const auto N = this->points.size();
    const bool AlreadyProjected = true;

    //A simplistic, incomprehensive approach.
    for(long int i = 3; i < N; ++i){
        const auto P = proj_cont.First_N_Point_Avg(i);    
        if(proj_cont.Is_Point_In_Polygon_Projected_Orthogonally(theplane, P, AlreadyProjected)){
            return P;
        }
    }

    //Fall-back approaches.

    // ...

    // ( idea #1: Check progressively finer divisions of a circle centered around the first vertex and aligned with the
    //   contour plane. )

    // ...

    throw std::runtime_error("Unable to locate any interior points. (A more robust scheme would help.)");
    return vec3<T>();
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float > contour_of_points<float >::Get_Point_Within_Contour(void) const;
    template vec3<double> contour_of_points<double>::Get_Point_Within_Contour(void) const;
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

#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float  contour_of_points<float >::Integrate_Simple_Vector_Kernel(std::function< vec3<float > (const vec3<float > &r,  const vec3<float > &A, const vec3<float > &B, const vec3<float > &U)> k) const;
    template double contour_of_points<double>::Integrate_Simple_Vector_Kernel(std::function< vec3<double> (const vec3<double> &r,  const vec3<double> &A, const vec3<double> &B, const vec3<double> &U)> k) const;
#endif

template <class T>    contour_of_points<T> contour_of_points<T>::Resample_Evenly_Along_Perimeter(const T dl) const {
    // This routine will attempt to resample along the perimeter every [dl] with as many new points as will fit within
    // the perimeter of the original contour.

    //First, do some preliminary checks.
    if(this->points.size() <= 1) FUNCERR("Attempted to perform resampling on a contour with " << this->points.size() << " points. Surely this was not intended!");
    
    decltype(this->points) newpoints;
    const T perimeter = this->Perimeter();
    const long int N = static_cast<long int>( std::floor(perimeter/dl) );
    const T spacing = dl;
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

        offset = (zero - remain);
        remain = zero;
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
    out.metadata = this->metadata;
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template contour_of_points<float > contour_of_points<float >::Resample_Evenly_Along_Perimeter(const float  dl) const;
    template contour_of_points<double> contour_of_points<double>::Resample_Evenly_Along_Perimeter(const double dl) const;
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
    out.metadata = this->metadata;
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template contour_of_points<float > contour_of_points<float >::Resample_Evenly_Along_Perimeter(const long int N) const;
    template contour_of_points<double> contour_of_points<double>::Resample_Evenly_Along_Perimeter(const long int N) const;
#endif

template <class T>
contour_of_points<T> 
contour_of_points<T>::Subdivide_Midway(void) const {
    //First, do some preliminary checks.
    if(this->points.size() <= 1) FUNCERR("Attempted to perform resampling on a contour with " << this->points.size() << " points. Surely this was not intended!");
   
    decltype(this->points) newpoints;

    auto itA = this->points.begin(), itB = ++(this->points.begin());
    while(true){
        if(!(itA != this->points.end())) break;
        if(!(itB != this->points.end())) itB = this->points.begin();

        newpoints.push_back(*itA);
        newpoints.push_back(((*itA)+(*itB))/static_cast<T>(2));
        ++itA; 
        ++itB;
    }

    contour_of_points<T> out(std::move(newpoints));
    out.closed = this->closed;
    out.metadata = this->metadata;
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template contour_of_points<float > contour_of_points<float >::Subdivide_Midway(void) const;
    template contour_of_points<double> contour_of_points<double>::Subdivide_Midway(void) const;
#endif

//Remove individual vertices until the cumulative change in area exceeds the threshold.
//
// Note: A greedy algorithm is used which can result in non-optimal results if a large threshold is provided.
//
template <class T>
contour_of_points<T> 
contour_of_points<T>:: Remove_Vertices(T area_threshold) const {
    if(this->points.size() <= 3){
        throw std::runtime_error("Cannot simplify, contour is already a triangle");
    }

    const bool AssumePlanar = true;
    const auto A_orig = std::abs( this->Get_Signed_Area(AssumePlanar) );
//    const auto A_targ = A_orig - area_threshold;

    auto trimmed_area = static_cast<T>(0);

    contour_of_points<T> out(*this);
    out.closed = this->closed;
    out.metadata = this->metadata;

    using list_iter_t = decltype(out.points.begin());

    auto get_prev = []( list_iter_t first, list_iter_t last, list_iter_t it ) -> list_iter_t {
        return (it == first) ? last : std::prev(it);
    };
    auto get_next = []( list_iter_t first, list_iter_t last, list_iter_t it ) -> list_iter_t {
        return (it == last) ? first : std::next(it);
    };

   
    while(true){
        if(out.points.size() == 3) break;

        std::list< std::pair< T, list_iter_t > > removal_penalties;
        auto first = out.points.begin();
        auto last = std::prev(out.points.end());
        for(auto it = first; it != out.points.end(); ++it){
            auto it_prev = get_prev(first, last, it);
            auto it_next = get_next(first, last, it);

            contour_of_points<T> dummy;
            dummy.closed = true;
            dummy.points.push_back(*it_prev);
            dummy.points.push_back(*it);
            dummy.points.push_back(*it_next);
            const auto dA = std::abs( dummy.Get_Signed_Area() );

            removal_penalties.emplace_back( std::make_pair(dA, it) );
        }

        removal_penalties.sort([]( const std::pair<T,list_iter_t> &L,
                                   const std::pair<T,list_iter_t> &R ) -> bool {
                                       return (L.first < R.first);
                               });

        //Bail before removing the vertex if it would put us over the threshold.
        trimmed_area += removal_penalties.front().first;
        if(trimmed_area > area_threshold) break;

        out.points.erase(removal_penalties.front().second);
    }

    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template contour_of_points<float > contour_of_points<float >::Remove_Vertices(float ) const;
    template contour_of_points<double> contour_of_points<double>::Remove_Vertices(double) const;
#endif


//Scales distance from each point to given point by factor (scale).
//
// NOTE: Be aware that if the contour is not planar, scaling positively will cause the contour to be even less planar. 
//       A consequence of this is that large scaling factors will boost errors from planarity. If this is important, 
//       consider following this routine with a planar projection.
template <class T>    contour_of_points<T> contour_of_points<T>::Scale_Dist_From_Point(const vec3<T> &point, T scale) const {
    if(scale < (T)(0.0)){
        FUNCWARN("Passed a negative scaling factor. We cannot compute anything logical with this value. Continuing");
        // NOTE: If you actually need this, you can probably safely delete this check and warning. (Double check.) It 
        //       doesn't seem terribly useful to scale negatively at the moment, so I figured this would indicate a
        //       logic or programming error. If you *do* allow this, remember that the orientation will flip!
        return contour_of_points<T>();
    }

    contour_of_points<T> out(this->points);
    out.closed = this->closed;
    out.metadata = this->metadata;
    for(auto p_it = out.points.begin(); p_it != out.points.end(); ++p_it){
        //Scales along the unit vector from the provided point and the original point. If scale = 1, the original point is
        // retained.
        *p_it = (*p_it)*scale + point*((T)(1.0) - scale);
    }
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template contour_of_points<float > contour_of_points<float >::Resample_LTE_Evenly_Along_Perimeter(const long int N) const;
    template contour_of_points<double> contour_of_points<double>::Resample_LTE_Evenly_Along_Perimeter(const long int N) const;
#endif

template <class T> 
plane<T> contour_of_points<T>::Least_Squares_Best_Fit_Plane(const vec3<T> &plane_normal) const {
    //Compute the least-squares best-fit plane through the contour points with the provided normal. Helpful for later projection.

    //Step 1: Construct a plane with the given normal which passes through the origin.
    const auto zero = static_cast<T>(0);
    const auto norm = plane_normal.unit(); //Never trust the user!
    const plane<T> working_plane(norm, vec3<T>(zero,zero,zero));

    //Step 2: Amass all the signed distances to the plane, and find the mean.
    std::vector<T> sdists;
    sdists.reserve(this->points.size());
    for(const auto &point : this->points){
        const auto sdist = working_plane.Get_Signed_Distance_To_Point(point);
        sdists.push_back(sdist);
    }
    const auto mean = Stats::Mean(std::move(sdists));

    //Step 3: Construct the plane with the given normal and passing through the optimal point.
    return plane<T>(norm, norm * mean);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template plane<float > contour_of_points<float >::Least_Squares_Best_Fit_Plane(const vec3<float > &plane_normal) const;
    template plane<double> contour_of_points<double>::Least_Squares_Best_Fit_Plane(const vec3<double> &plane_normal) const;
#endif


//Estimates the planar normal using a few vertices via cross-product.
//
// The estimate may not represent any actual normal on the surface defined by the contour if the contour is non-planar.
// 
// NOTE: A better approach would solve the least-squares (or something more robust) estimate for the normal using all
//       datum available. However, this method works fine if the contour is planar, and there isn't much rationale in
//       calling this routine if the contour is not planar. So this method won out owing to simplicity.
template <class T>
vec3<T> 
contour_of_points<T>::Estimate_Planar_Normal() const {
    if(this->points.size() < 3){
        throw std::runtime_error("Failed to estimate contour plane: too few datum available.");
    }
    const auto N = this->points.size();
    const vec3<T> p0 = this->Average_Point();
    vec3<T> dp1p0;
    vec3<T> dp2p0;
    size_t n = 0;

    //Look for a usable p1.
    do{
        if((n+1) == N){
            throw std::runtime_error("Exhausted available points: too few usable datum available.");
        }
        dp1p0 = *std::next(this->points.begin(),n++) - p0;
    }while(!std::isnormal(dp1p0.length()));
    do{
        if((n+1) == N){
            throw std::runtime_error("Exhausted available points: too few usable datum available.");
        }
        dp2p0 = *std::next(this->points.begin(),n++) - p0;
    }while(!std::isnormal(dp2p0.length()));
    return dp1p0.Cross(dp2p0).unit(); //Using the counterclockwise <--> positive convention.
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float > contour_of_points<float >::Estimate_Planar_Normal() const;
    template vec3<double> contour_of_points<double>::Estimate_Planar_Normal() const;
#endif    

//Maintain connectivity, but project each point onto the given plane along a normal to the plane. 
//
// Returns an empty contour if any point cannot be projected to a finite point on the plane.
//
// NOTE: If the plane is pathologically chosen, the contour might be highly malformed or overlapping.
//       Such a projection is not an error, so you'll need to handle this case if it matters to you.
template <class T> contour_of_points<T> contour_of_points<T>::Project_Onto_Plane_Orthogonally(const plane<T> &P) const {
    contour_of_points<T> out;
    out.closed = this->closed;
    out.metadata = this->metadata;

    for(const auto &point : this->points){
        const auto pjctd = P.Project_Onto_Plane_Orthogonally(point);
        if(!pjctd.isfinite()) return contour_of_points<T>();
        out.points.emplace_back(std::move(pjctd));
    }

    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template contour_of_points<float > contour_of_points<float >::Project_Onto_Plane_Orthogonally(const plane<float > &P) const;
    template contour_of_points<double> contour_of_points<double>::Project_Onto_Plane_Orthogonally(const plane<double> &P) const;
#endif


//Check if the given point lies in a 2D polygon defined by the orthogonal projection (of both contour and point) onto a given plane.
// This routine entirely ignores the direction orthogonal to the plane, so filter contours at the wrong 'height' beforehand for 3D!
//
// NOTE: You will often want to generate the plane using this->Least_Squares_Best_Fit_Plane() with the normal (0,0,1).
//
// NOTE: This routine tries to be boundary and endpoint-inclusive, but it is not always exactly possible. If this matters, try 
//       passing a few planes with slightly varying normals and use something like majority voting. 
//
// NOTE: This routine performs a projection of both the point and contour data onto the plane. If you are going to call this 
//       routine many times (such as walking over the voxels of an image) you will probably want to precompute the projected items
//       and use the AlreadyProjected flag. This will avoid at least one walk over the contour points, and should about halve the
//       time needed. The original plane (or numerically close copy) is needed to generate an orthogonal coordinate system in the
//       plane.
//
//
template <class T> 
bool contour_of_points<T>::Is_Point_In_Polygon_Projected_Orthogonally(const plane<T> &plane, 
                                                                      const vec3<T> &point,
                                                                      bool AlreadyProjected,
                                                                      T boundary_eps) const {
    if(!this->closed) FUNCERR("Cannot perform test if a point is 'inside' a polygon if the polygon is open. Cannot continue");

    //Project the contour data and user-provided point onto the plane if needed. The direction along the normal is ignored, making
    // this routine effectively 2D.
    vec3<T> local_proj_point;
    contour_of_points<T> local_proj_cont;
    const vec3<T> * proj_point = &point;
    const contour_of_points<T> * proj_cont = &(*this);

    if(!AlreadyProjected){
        local_proj_point = plane.Project_Onto_Plane_Orthogonally(point);
        local_proj_cont  = this->Project_Onto_Plane_Orthogonally(plane);
        proj_point = &local_proj_point;
        proj_cont  = &local_proj_cont;
    }

    //Now we need to find two orthogonal coordinate directions in the plane. Any two distinct points in the plane will work
    // for this purpose. We already have one in the plane's definition, so we walk the contours until we find another
    // which is suitable. We then generate the last one and normalize them both. 
    const vec3<T> plane_z_unit = plane.N_0.unit();
    vec3<T> plane_x_unit;
    vec3<T> plane_y_unit;
    bool found_usable_point = false;
    for(const auto &p : proj_cont->points){
        plane_x_unit = (p - plane.R_0);
        if(std::isnormal(plane_x_unit.length())){
            found_usable_point = true;
            break;
        }
    }
    if(!found_usable_point) FUNCERR("Cannot generate coordinate units on the plane. Are the points in pathological formation? Cannot continue");
    plane_x_unit = plane_x_unit.unit();
    plane_y_unit = plane_z_unit.Cross(plane_x_unit).unit();

    //Using the plane's X-Y coordinate system, figure out the user's point's coordinates.
    const auto pX = ((*proj_point) - plane.R_0).Dot(plane_x_unit);
    const auto pY = ((*proj_point) - plane.R_0).Dot(plane_y_unit);

    //Now step over the projected points, toggling a counter each time the user's point satisfies some criteria.
    //
    // The gist of this algorithm is that we walk over the polygon's line segments, consistently partitioning 
    // space along one of the coordinates (into a band along y or x). The line segment naturally partitions this
    // band into two parts: the space above and the space below. The key insight is that we merely have to keep
    // track of the number of times the user's point is spotted in either the upper or lower part of the band
    // (it doesn't matter which as long as we consistently choose one). In fact, all we need to know is whether
    // the point was spotted in the half-band an even or odd number of times. 
    bool is_in_the_polygon = false;  //We do not lose any generality by setting this to false initially.
    auto B_it = std::prev(proj_cont->points.end());
    for(auto A_it = proj_cont->points.begin(); A_it != proj_cont->points.end(); B_it = (A_it++)){
        //Generate the planar X-Y coordinates of the projected contour points.
        const auto qaX = ((*A_it) - plane.R_0).Dot(plane_x_unit);
        const auto qaY = ((*A_it) - plane.R_0).Dot(plane_y_unit);
        const auto qbX = ((*B_it) - plane.R_0).Dot(plane_x_unit);
        const auto qbY = ((*B_it) - plane.R_0).Dot(plane_y_unit);

        const vec3<T> a2D(qaX, qaY, static_cast<T>(0));
        const vec3<T> b2D(qbX, qbY, static_cast<T>(0));
        const vec3<T> p2D( pX,  pY, static_cast<T>(0));
        const line_segment<T> l2D(a2D, b2D);
        if(l2D.Within_Cylindrical_Volume(p2D,boundary_eps)){
            is_in_the_polygon = true;
            break;
        }

        //We divide all of space into two parts: 
        //  1. between the y-coords of A and B (regardless of the x-coords, inclusive to only one of the
        //     points so we don't double-count any points that lie on the boundary) which forms a band, and
        //  2. outside this band.
        //
        // We only concern ourselves with the point at this line segment if it is within the band.
        //
        // Check if the point is in the line segment's y-coord band. Now, there could be two orientations of
        // the band. They will be distinct due to the semi-inclusivity of the endpoints, so check both.
        if( ((qaY <= pY) && (pY < qbY))     //(Orientation a.)
        ||  ((qbY <= pY) && (pY < qaY)) ){  //(Orientation b.)

            const auto BB = (pY - qaY) * ((qbX - qaX)/(qbY - qaY));
            if(pX < (BB + qaX)) is_in_the_polygon = !is_in_the_polygon;
        }
    }

    return is_in_the_polygon;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool contour_of_points<float >::Is_Point_In_Polygon_Projected_Orthogonally(
        const plane<float > &plane, const vec3<float > &point, bool, float ) const;
    template bool contour_of_points<double>::Is_Point_In_Polygon_Projected_Orthogonally(
        const plane<double> &plane, const vec3<double> &point, bool, double) const;
#endif

template <class T> contour_of_points<T> &  contour_of_points<T>::operator=(const contour_of_points<T> &rhs) {
    if(this == &rhs) return *this;
    this->closed   = rhs.closed;
    this->points   = rhs.points;
    this->metadata = rhs.metadata;
    return *this;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template contour_of_points<float > & contour_of_points<float >::operator=(const contour_of_points<float > &rhs);
    template contour_of_points<double> & contour_of_points<double>::operator=(const contour_of_points<double> &rhs);
#endif


//This routine attempts to determine equality between contours, ignoring the starting position of the contour. This is a computationally
// expensive task, so some shortcuts might be taken!
//
// NOTE: Ignores metadata.
//
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool contour_of_points<float >::operator==(const contour_of_points<float > &in) const;
    template bool contour_of_points<double>::operator==(const contour_of_points<double> &in) const;
#endif

template <class T> bool contour_of_points<T>::operator!=(const contour_of_points<T> &in) const {
    return !(*this == in);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
        if(P.Is_Point_Above_Plane(*it)){
            above = true;
        }else if(PN.Is_Point_Above_Plane(*it)){
            below = true;
        }

        //Check to see if we have found at least one point above and one below. If so, break out early.
        if(above && below) break;
    }

    if(above && !below) return 1;   //Above.
    if(!above && below) return -1;  //Below.

    //if((above && below) || (!above && !below))  return 0;   //Crosses plane!
    return 0;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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

        //Find the point along the line between p1 and p2 that is closest to the straddled (middle) point.
        auto mdl_it = std::next(p1_it);
        if(!(mdl_it != this->points.end())) mdl_it = this->points.begin();

        const line<T> p1p2(*p1_it,*p2_it);
        const vec3<T> proj_mdl = p1p2.Project_Point_Orthogonally(*mdl_it);

        //Let the user compare if they think the separation is close enough that the points are considered identical.
        if( Feq(*mdl_it,proj_mdl) ){
            //Remove the extraneous point between p1 and p2. Keep p1 the same.
            this->points.erase( mdl_it );
        }else{
            //Keep the point - it is needed for the contour shape. Iterate p1.
            p1_it = mdl_it;
        }
    }

    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void contour_of_points<float >::Remove_Extraneous_Points(std::function<bool(const vec3<float > &,const vec3<float > &)> Feq);
    template void contour_of_points<double>::Remove_Extraneous_Points(std::function<bool(const vec3<double> &,const vec3<double> &)> Feq);
#endif

//Checks if the key is present without inspecting the value.
template <class T> bool contour_of_points<T>::MetadataKeyPresent(std::string key) const {
    return (this->metadata.find(key) != this->metadata.end());
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool contour_of_points<float >::MetadataKeyPresent(std::string key) const;
    template bool contour_of_points<double>::MetadataKeyPresent(std::string key) const;
#endif

//Attempts to cast the value if present. Optional is disengaged if key is missing or cast fails.
template <class T> template <class U> std::experimental::optional<U>
contour_of_points<T>::GetMetadataValueAs(std::string key) const {
    const auto metadata_cit = this->metadata.find(key);
    if( (metadata_cit == this->metadata.end())  || !Is_String_An_X<U>(metadata_cit->second) ){
        return std::experimental::optional<U>();
    }else{
        return std::experimental::make_optional(stringtoX<U>(metadata_cit->second));
    }
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::experimental::optional<uint32_t> contour_of_points<float >::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<uint32_t> contour_of_points<double>::GetMetadataValueAs(std::string key) const;

    template std::experimental::optional<long int> contour_of_points<float >::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<long int> contour_of_points<double>::GetMetadataValueAs(std::string key) const;

    template std::experimental::optional<double> contour_of_points<float >::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<double> contour_of_points<double>::GetMetadataValueAs(std::string key) const;

    template std::experimental::optional<std::string> contour_of_points<float >::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<std::string> contour_of_points<double>::GetMetadataValueAs(std::string key) const;
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void contour_of_points<float>::Plot(void) const;
    template void contour_of_points<double>::Plot(void) const;
#endif


//Writes contour data to string. 
//
// NOTE: Ignores metadata. Write to JSON if you want to retain metadata.
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::string contour_of_points<float >::write_to_string(void) const;
    template std::string contour_of_points<double>::write_to_string(void) const;
#endif

//Returns true if it worked/was loaded into the contour, false otherwise.
//
// NOTE: Metadata is not recorded in this form, only raw contour data. Use JSON if you want to retain metadata.
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool contour_of_points<float >::load_from_string(const std::string &in);
    template bool contour_of_points<double>::load_from_string(const std::string &in);
#endif


//---------------------------------------------------------------------------------------------------------------------------
//---------------------- contour_collection: a collection of logically-related contour_of_points  ---------------------------
//---------------------------------------------------------------------------------------------------------------------------
template <class T>    contour_collection<T>::contour_collection(){ }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template contour_collection<float >::contour_collection(void);
    template contour_collection<double>::contour_collection(void);
#endif

template <class T>    contour_collection<T>::contour_collection(const contour_collection<T> &in) : contours(in.contours) { }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template contour_collection<float >::contour_collection(const contour_collection<float > &in);
    template contour_collection<double>::contour_collection(const contour_collection<double> &in);
#endif        

template <class T>    contour_collection<T>::contour_collection(std::list<contour_of_points<T>> in) : contours(std::move(in)) { }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template contour_collection<float >::contour_collection(std::list<contour_of_points<float >> in);
    template contour_collection<double>::contour_collection(std::list<contour_of_points<double>> in);
#endif


//Member functions.
template <class T>
void 
contour_collection<T>::Consume_Contours(contour_collection<T> &in){
    if(this == &in) return; //If self, do nothing.
    this->contours.splice( this->contours.begin(), std::move(in.contours) ); //in.contours is now empty.
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void contour_collection<float >::Consume_Contours(contour_collection<float > &);
    template void contour_collection<double>::Consume_Contours(contour_collection<double> &);
#endif    

template <class T>  T contour_collection<T>::Get_Signed_Area(bool AssumePlanarContours) const {
    //NOTE: ALL contours must be closed. See the implementation in contour_of_points.
    T Area = (T)(0);
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        Area += c_it->Get_Signed_Area(AssumePlanarContours);
    }
    return Area; //NOTE: Do NOT take the absolute value. We want to keep the sign for adding/subtracting/etc.. contour areas!
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float  contour_collection<float>::Get_Signed_Area(bool AssumePlanarContours) const;
    template double contour_collection<double>::Get_Signed_Area(bool AssumePlanarContours) const;
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void contour_collection<float>::Reorient_Counter_Clockwise(void);
    template void contour_collection<double>::Reorient_Counter_Clockwise(void);
#endif





//This function sums all the points of all contours and divides by the total number of points, giving the 'average' point. 
// This value has limited use becaue it depends on the density of points in the contour. It is not a true center of volume
// for this reason.
template <class T> vec3<T> contour_collection<T>::Average_Point(void) const {
    vec3<T> out((T)(0), (T)(0), (T)(0));
    long int N = 0;
    for(const auto &c : this->contours){
        for(const auto &p : c.points){
            out += p;
            ++N;
        }
    }
    if(N == 0) return vec3<T>( std::numeric_limits<T>::quiet_NaN(),
                               std::numeric_limits<T>::quiet_NaN(),
                               std::numeric_limits<T>::quiet_NaN() );
    out /= static_cast<T>(N);
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
template <class T> vec3<T> contour_collection<T>::Centroid(bool AssumePlanarContours) const {
    T Area = (T)(0);   
    vec3<T> RA((T)(0), (T)(0), (T)(0));
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        const auto area = c_it->Get_Signed_Area(AssumePlanarContours);
        Area += area;
        RA   += (c_it->Centroid())*area;
    }
    return RA/Area;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float> contour_collection<float>::Centroid(bool) const;
    template vec3<double> contour_collection<double>::Centroid(bool) const;
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float  contour_collection<float >::Perimeter(void) const;
    template double contour_collection<double>::Perimeter(void) const;
#endif

//Simple average of the perimeters of individual contours.
template <class T>  T contour_collection<T>::Average_Perimeter(void) const {
    const auto N = this->contours.size();
    if(N == 0) return (T)(0);
    return (this->Perimeter()/static_cast<T>(N));
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float  contour_collection<float >::Longest_Perimeter(void) const;
    template double contour_collection<double>::Longest_Perimeter(void) const;
#endif

template <class T>  T contour_collection<T>::Slab_Volume(T thickness, bool IgnoreContourOrientation) const {
    //Computes volume assuming each contour is a slab with the given thickness. This is "volume" in the sense of
    // something like the Lebesque-integral but not infinitesimal. (It is exact iff the perimeter is exactly 
    // specified by the polygon vertices and the thickness is exactly specified uniform across the contour.)
    //
    // Often, in reality, the 3D object described by these contours will have a smooth boundary that cannot be
    // well-approximated by this routine. If the distinction is meaningful, you'll need to more adequately describe
    // the surface; probably via a surface mesh or more precisely with some parameterization like NURBs. 
    // In such cases, this routine should be considered a non-exact estimator of volume.
    //
    // Ignores any overlap (or gap) due to improper choice of thickness, so you'll need to carefully figure out the 
    // appropriate thickness parameter for your problem domain. This step cannot be automated because the spacing
    // can vary in many situations, such as when the 3D structure is not solid.
    //
    // NOTE: This routine optionally takes into account the orientation of contours so that it can handle, say, 
    //       holes in solid structures. If the result is negative, you have simply got the orientation backward.
    //       This behaviour can be toggled through the IgnoreContourOrientation parameter.
    //
    // NOTE: This routine is only suitable for situations in which all contours can be considered to have the same 
    //       thickness. If something more exotic is needed, the thickness would need to somehow be specified for
    //       each contour. Since this would be difficult to account for in many routines, it is recommended to 
    //       simply implement such a routine separately elsewhere. Alternatively, write a method that accepts a
    //       closure and go to town, assuming you can figure out how to reliably squeeze conotur labels through 
    //       a closure interface.
    //
    // NOTE: Based on comparisons with Varian's Eclipse (TM) computing volumes of organ contours,
    //       the mean percent difference was -0.4 pm 2.0 %  and the mean absolute percent difference was 3.4 pm 1.4 %
    //       with N = 7 (6 parotids, 1 submandibular). I'm not sure how Eclispe computes volumes, but I wouldn't 
    //       consider the slab volume to be identical to the true volume. It more like an estimator of the volume.
    //
    //       Be aware that this algorithm (likely) badly estimates volume if the volume is: small, not very many
    //       contours are present, the contours are not appreciably stacked on top of one another, or if the contours
    //       are high-frequency and fluctuate compared with adjacent contours. There may be other failure modes.
    //       If in doubt, you will need to construct a watertight surface and try something like Marching Cubes.
    //
    if(thickness < (T)(0)) FUNCERR("Thickness is intrinsically positive. Cannot make sense of a negative thickness. Bailing");
    T signed_volume = (T)(0);
    for(auto c_it = this->contours.begin(); c_it != this->contours.end(); ++c_it){
        const auto signed_area = (IgnoreContourOrientation ? std::abs(c_it->Get_Signed_Area()) : c_it->Get_Signed_Area());
        signed_volume += signed_area*thickness;
    }
    return signed_volume;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float  contour_collection<float >::Slab_Volume(float  thickness, bool IgnoreContourOrientation) const;
    template double contour_collection<double>::Slab_Volume(double thickness, bool IgnoreContourOrientation) const;
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
template std::list<contour_collection<float >> contour_collection<float >::Split_Into_Core_Peel_Spherical(float  frac_dist) const;
template std::list<contour_collection<double>> contour_collection<double>::Split_Into_Core_Peel_Spherical(double frac_dist) const;
#endif


template <class T> contour_collection<T> &  contour_collection<T>::operator=(const contour_collection<T> &rhs){
    if(this == &rhs) return *this;
    this->contours = rhs.contours;
    return *this;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template contour_collection<float > & contour_collection<float >::operator=(const contour_collection<float > &rhs);
    template contour_collection<double> & contour_collection<double>::operator=(const contour_collection<double> &rhs);
#endif

template <class T>
std::list<contour_collection<T>>
contour_collection<T>::Total_Area_Bisection_Along_Plane(const vec3<T> &planar_unit_normal,
                                                        T desired_total_area_fraction_above_plane,
                                                        T acceptable_frac_deviation,
                                                        size_t max_iters,
                                                        plane<T> *final_plane,
                                                        size_t *iters_taken,
                                                        T *final_area_frac,
                                                        vec3<T> lower_bound,
                                                        vec3<T> upper_bound) const {

    //Split the contour collection such that the split (discretely) comes closest to the desired total area above a plane.
    //
    // NOTE: This routine will work with transverse planes, but be aware that attempting to split contours with a plane
    //       that is parallel to the plane the contours lie in is risky. You will probably get a 'discrete' split such
    //       that a given contour will either be above or below, but not split into pieces. You *might* be able to let
    //       the algorithm run for a long time in such a case if you are OK with floating-point instabilities, but a
    //       better approach would be to slightly rotate your plane or contours to get a more well-defined split.
    //
    //       Note, however, that this routine will probably work even with a transverse plane if the tolerances and/or
    //       max_iter are set to reasonable values. Examples of 'reasonable' are ~1-5% tolerance and no more than 10-20
    //       iterations (otherwise the contours might begin to get split in their own plane).
    //
    // NOTE: This routine might get run for an excessive time if either the tolerance or max_iters are not set to
    //       appropriate values.
    //
    // Parameter description:
    //   - planar_unit_normal  <-- Defines 'up' and the planar orientation.
    //   - desired_total_area_fraction_above_plane  <-- Here 'above' means in the positive normal direction.
    //   - acceptable_deviation  <-- Deviation from desired_total_area_fraction_above_plane.
    //   - final_plane  <-- If non-nullptr, storage for the final plane. (The user might not care.)
    //   - max_iters  <-- If the tolerance cannot be reached after this many iters, report the current plane as-is.
    //   - iters_taken  <-- If non-nullptr, storage for the number of iterations performed. (The user might not care.)
    //   - lower_bound  <-.
    //   - upper_bound  <-'` These define the upper and lower bounds of the desired planar split. They are points through
    //                       which a plane with the given unit normal intersects. If they are not finite, they will be
    //                       automatically derived from the most extreme points from the specified plane. Relying on
    //                       automatic bounds derivation should work in all except degenerate cases, but can result in
    //                       slow bisection compared with the user more closely bounding the desired planar split.
    //
    if(this->contours.empty()){
        throw std::invalid_argument("No contours to sub-segment.");
    }
    if(!isininc(static_cast<T>(0), desired_total_area_fraction_above_plane, static_cast<T>(1))){
        throw std::invalid_argument("Desired area parameter must be within [0,1].");
    }
    const auto highest_acceptable = desired_total_area_fraction_above_plane + acceptable_frac_deviation;
    const auto lowest_acceptable  = desired_total_area_fraction_above_plane - acceptable_frac_deviation;

    const auto offset_eps = std::sqrt(std::numeric_limits<T>::epsilon());
    const auto qNaN = std::sqrt(std::numeric_limits<T>::quiet_NaN());

    //If the bounds are not finite, try derive them from the contour data.
    {
        //Find the most extreme vertices along the planar normal.

        vec3<T> lower_vert(qNaN, qNaN, qNaN);
        vec3<T> upper_vert(qNaN, qNaN, qNaN);
        T lower_dist = qNaN;
        T upper_dist = qNaN;

        for(const auto &c : this->contours){
            for(const auto &p : c.points){
                const auto L = p.Dot(planar_unit_normal);
                if(!std::isfinite(lower_dist) || ( L < lower_dist ) ){
                    lower_dist = L;
                    lower_vert = p;
                }
                if(!std::isfinite(upper_dist) || ( L > upper_dist ) ){
                    upper_dist = L;
                    upper_vert = p;
                }
            }
        }

        // Add a small additional gap to increase the initial step size and help if a single planar_contour was
        // provided..
        const auto sep = (upper_dist - lower_dist);
        const auto gap = sep * static_cast<T>(0.1) + offset_eps;
                
        if(!std::isfinite(lower_bound.length())){
            lower_bound = lower_vert - (planar_unit_normal * gap);
        }
        if(!std::isfinite(upper_bound.length())){
            upper_bound = upper_vert + (planar_unit_normal * gap);
        }
    }

    if( !std::isfinite(lower_bound.length()) || !std::isfinite(upper_bound.length()) ){
        throw std::runtime_error("Unable to derive lower or upper bound. Cannot begin bisection routine.");
    }

    //Compute the area of all planar slices.
    const auto total_area = this->Get_Signed_Area();

    size_t iter = 1;
    while(true){

       //Given the current upper and lower planar bounds, compute the mid-plane.
       const auto lower_plane = plane<T>(planar_unit_normal, lower_bound);
       const auto upper_plane = plane<T>(planar_unit_normal, upper_bound);

       const auto dist_between_planes = lower_plane.Get_Signed_Distance_To_Point( upper_bound );
       const auto mid_plane = plane<T>(planar_unit_normal, lower_bound + planar_unit_normal * dist_between_planes * static_cast<T>(0.5));


       //Compute the area split with the mid-plane.
       const auto splits = this->Split_Along_Plane(mid_plane);
       if(splits.size() != 2){
           throw std::logic_error("Expected exactly two groups, above and below plane.");
       }
       const auto area_above_mid_plane = splits.back().Get_Signed_Area();
       const auto area_frac = area_above_mid_plane / total_area;

       //Check if the mid-plane is within tolerance. If it is, we can stop.
       //
       //Also check if the max iteration count has been reached. It is up to the user whether this is considered a
       // success or not. (For example, if a transverse plane has been used, it may not be possible to achieve the
       // tolerance requested. Another example: the tolerance is set very small and it cannot be reached due to
       // truncation errors.)
       if( isininc(lowest_acceptable,area_frac,highest_acceptable) || (iter >= max_iters) ){
           if(final_plane != nullptr) *final_plane = mid_plane;
           if(iters_taken != nullptr) *iters_taken = iter;
           if(final_area_frac != nullptr) *final_area_frac = area_frac;
           return splits;

       //If the area above the midplane is too small, replace the upper plane bound with the midplane.
       }else if(area_frac < desired_total_area_fraction_above_plane){
           upper_bound = mid_plane.R_0;

       //If it is too big, replace the lower plane bound with the midplane.
       }else{
           lower_bound = mid_plane.R_0;
       }

       ++iter;
    }

    throw std::logic_error("Should not ever get to this point");
    return std::list<contour_collection<T>>();
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
template std::list<contour_collection<float >> 
    contour_collection<float >::Total_Area_Bisection_Along_Plane(const vec3<float >&,
                                                                 float , float , size_t,
                                                                 plane<float > *, size_t *, float  *,
                                                                 vec3<float >, vec3<float >) const;
template std::list<contour_collection<double>> 
    contour_collection<double>::Total_Area_Bisection_Along_Plane(const vec3<double>&,
                                                                 double, double, size_t,
                                                                 plane<double> *, size_t *, double *,
                                                                 vec3<double>, vec3<double>) const;
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool contour_collection<float >::operator==(const contour_collection<float > &in) const;
    template bool contour_collection<double>::operator==(const contour_collection<double> &in) const;
#endif

template <class T> bool contour_collection<T>::operator!=(const contour_collection<T> &in) const {
    return !(*this == in);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void contour_collection<float >::Merge_Adjoining_Contours(std::function<bool(const vec3<float > &,const vec3<float > &)> Feq);
    template void contour_collection<double>::Merge_Adjoining_Contours(std::function<bool(const vec3<double> &,const vec3<double> &)> Feq);
#endif


//For all contours. Overwrites if existing keys present.
template <class T>
void
contour_collection<T>::Insert_Metadata(const std::string &key, const std::string &val){
    for(auto &c : this->contours){
        c.metadata[key] = val;
    }
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void contour_collection<float >::Insert_Metadata(const std::string &, const std::string &);
    template void contour_collection<double>::Insert_Metadata(const std::string &, const std::string &);
#endif



//Returns the metadata key-values that are "common" (i.e., identical among all contours).
// For ordering purposes, contours in *this are considered to have priority over those in 'in'.
template <class T>
std::map<std::string,std::string> 
contour_collection<T>::get_common_metadata(const std::list<std::reference_wrapper<contour_collection<T>>> &ccl,
                                           const std::list<std::reference_wrapper<contour_of_points<T>>> &copl){

    //Collect all available metadata.
    std::list<std::reference_wrapper<contour_of_points<T>>> c_refs;

    for(auto &c : this->contours){
        c_refs.emplace_back( std::ref(c) );
    }
    for(const auto &cc : ccl){
        for(auto &c : cc.get().contours){
            c_refs.emplace_back( std::ref(c) );
        }
    }
    for(const auto &cr : copl) c_refs.emplace_back(cr);

    std::multimap<std::string,std::string> all_m;
    for(const auto &c_ref : c_refs){ 
        for(const auto &m : c_ref.get().metadata){
            //If the key is not present, insert it unconditionally.
            //If the key is present twice or more, it is already spoiled so move along.
            //If the key is present once, check if values differ; insert iff they do.
            const auto count = all_m.count(m.first);
            if( (count == 0)
            ||  ((count == 1) && (all_m.lower_bound(m.first)->second != m.second)) ){
                all_m.insert(m);
            }
        }
    }

    //Construct the outgoing metadata iff there was a single value corresponding to a given key.
    std::map<std::string,std::string> out;
    for(const auto &m : all_m){
        if(all_m.count(m.first) == 1) out.insert(m);
    }
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::map<std::string,std::string> 
        contour_collection<float >::get_common_metadata(
                const std::list<std::reference_wrapper<contour_collection<float >>> &,
                const std::list<std::reference_wrapper<contour_of_points<float >>> &);
    template std::map<std::string,std::string> 
        contour_collection<double>::get_common_metadata(
                const std::list<std::reference_wrapper<contour_collection<double>>> &,
                const std::list<std::reference_wrapper<contour_of_points<double>>> &);
#endif

//Returns a copy of all values that correspond to the given key. Order is maintained.
template <class T>
std::list<std::string> 
contour_collection<T>::get_all_values_for_key(const std::string &akey) const {
    std::list<std::string> out;
    for(const auto &c : this->contours){
        auto it = c.metadata.find(akey);
        if(it != c.metadata.end()){
            out.emplace_back(it->second);
        }
    }
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::list<std::string> 
        contour_collection<float >::get_all_values_for_key(const std::string &) const;
    template std::list<std::string>
        contour_collection<double>::get_all_values_for_key(const std::string &) const;
#endif

//Returns a copy of all unique values that correspond to the given key. Original order is maintained.
template <class T>
std::list<std::string> 
contour_collection<T>::get_unique_values_for_key(const std::string &akey) const {
    auto all_values = this->get_all_values_for_key(akey);
    std::map<std::string,size_t> counts;
    for(const auto &avalue : all_values) counts[avalue]++;
    
    std::list<std::string> out;
    for(const auto &avalue : all_values){
        if(counts[avalue] == 1) out.emplace_back(avalue);
    }
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::list<std::string>
        contour_collection<float >::get_unique_values_for_key(const std::string &) const;
    template std::list<std::string>
        contour_collection<double>::get_unique_values_for_key(const std::string &) const;
#endif


//Removes contours if they have < N points. Duplicate points not considered.
template <class T> void contour_collection<T>::Purge_Contours_Below_Point_Count_Threshold(size_t N){
    auto unary_pred = [N](const contour_of_points<T> &c) -> bool { return (c.points.size() < N); };
    //this->contours.erase( std::remove_if( this->contours.begin(), this->contours.end(), unary_pred ), this->contours.end() );
    this->contours.remove_if(unary_pred);
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void contour_collection<float >::Purge_Contours_Below_Point_Count_Threshold(size_t N);
    template void contour_collection<double>::Purge_Contours_Below_Point_Count_Threshold(size_t N);
#endif


//This routine produces a very simple, default plot of the data. If more customization is required, you'll have to look elsewhere!
template <class T> void contour_collection<T>::Plot(const std::string &title) const {
    Plot3_List_of_contour_of_points(this->contours.begin(), this->contours.end(), title);
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void contour_collection<float >::Plot(const std::string &) const;
    template void contour_collection<double>::Plot(const std::string &) const;
#endif

template <class T> void contour_collection<T>::Plot(void) const {
    this->Plot(""); //No title.
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void contour_collection<float >::Plot(void) const;
    template void contour_collection<double>::Plot(void) const;
#endif


//Write basic contour data to a string.
//
// NOTE: This routine drops metadata. If metadata is needed, use something like JSON instead.
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::string contour_collection<float >::write_to_string(void) const;
    template std::string contour_collection<double>::write_to_string(void) const;
#endif

//Read basic data from a stringified contour. Returns true if it worked/was loaded into the contour, false otherwise.
//
// NOTE: This routine drops metadata. If metadata is needed, use something like JSON instead.
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool contour_collection<float >::load_from_string(const std::string &in);
    template bool contour_collection<double>::load_from_string(const std::string &in);
#endif


//Averages estimated contour normals to provide an average value for all provided contours.
template <class T>
vec3<T>
Average_Contour_Normals(const std::list<std::reference_wrapper<contour_collection<T>>> &ccs){
    // This routine estimates contour normals by averaging the normal estimated for each individual contour.

    vec3<T> N_sum( static_cast<T>(0), static_cast<T>(0), static_cast<T>(0) );
    long int count = 0;

    for(auto &cc : ccs){
        for(auto &cop : cc.get().contours){
            if(cop.points.size() < 3) continue;
            try{
                const auto N = cop.Estimate_Planar_Normal();
                N_sum += N;
                ++count;
            }catch(const std::exception &){}
        }
    }
    if(count == 0) throw std::runtime_error("Not possible to estimate contour normal; not enough vertices.");
    return N_sum.unit();
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template vec3<float >
        Average_Contour_Normals(const std::list<std::reference_wrapper<contour_collection<float >>> &);
    template vec3<double>
        Average_Contour_Normals(const std::list<std::reference_wrapper<contour_collection<double>>> &);
#endif                        


//Assumes each contour is planar, fits a plane using the given normal, removes planes that are within eps of one
// another, and sorts planes using the given normal (lowest to highest along it).
template <class T>
std::list<plane<T>> 
Unique_Contour_Planes(const std::list<std::reference_wrapper<contour_collection<T>>> &ccs,
                      const vec3<T> &N, 
                      T distance_eps){

    //Add a plane for every contour.
    const auto N_unit = N.unit();
    std::list<plane<T>> dummy;
    for(const auto &cc_ref : ccs){
        for(const auto &c : cc_ref.get().contours) dummy.emplace_back(c.Least_Squares_Best_Fit_Plane(N_unit));
    }
    if(dummy.empty()) return dummy;

    //Sort planes using signed distance from one of the planes (arbitrarily chosen).
    const auto arb_plane = dummy.front();
    dummy.sort([arb_plane](const plane<T> &L, const plane<T> &R) -> bool {
            //Note: this sort order is REVERSE of the final sorted order.
            return ( L.Get_Signed_Distance_To_Point(arb_plane.R_0) > R.Get_Signed_Distance_To_Point(arb_plane.R_0) );
        });

    //Remove planes that are adjacent and separated by a distance less than the provided epsilon.
    std::list<plane<T>> out; 
    out.emplace_back(dummy.front());
    dummy.pop_front();
    for(const auto &dp : dummy){
        const auto sep = std::abs(dp.Get_Signed_Distance_To_Point( out.back().R_0 ));
        if( sep >= distance_eps ) out.emplace_back(dp);
    }
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::list<plane<float >> 
        Unique_Contour_Planes(const std::list<std::reference_wrapper<contour_collection<float >>> &,
                              const vec3<float > &, float );
    template std::list<plane<double>> 
        Unique_Contour_Planes(const std::list<std::reference_wrapper<contour_collection<double>>> &,
                              const vec3<double> &, double);
#endif

// This routine is used to estimate the minimum separation of a collection of contours above some threshold value
// an 'epsilon' value. It is not always possible to estimate contour separation, but there are many methods which
// could yield an acceptable solution. The most direct method is attempted here.
//
// Note: If no method is successful, a default separation based on typical CT slice thickness is returned.
template <class T>
T
Estimate_Contour_Separation(const std::list<std::reference_wrapper<contour_collection<T>>> &ccs,
                            const vec3<T> &N, // The contour normal vector to use.
                            T distance_eps){
    //Zero contours have no meaningful separation.
    if(ccs.empty()) return std::numeric_limits<T>::quiet_NaN();

    const auto fallback_sep = static_cast<T>(2.5); // in DICOM units (usually mm).

    // This method will be costly if there are many contours. It provides the most up-to-date estimate, but also
    // requires an estimation of the contour normal. It also assumes the contour normal is identical for all contours,
    // which may not be true in some cases. This method will also fail for single contours.
    T min_spacing = std::numeric_limits<T>::quiet_NaN();

    auto ucpl = Unique_Contour_Planes(ccs, N);
    if(ucpl.size() < 2) throw std::runtime_error("Not enough unique contour planes to estimate separation.");

    for(auto p1_it = ucpl.begin(); p1_it != --(ucpl.end()); ++p1_it){
        auto p2_it = p1_it;
        ++p2_it;

        const T height1 = p1_it->R_0.Dot(N);
        const T height2 = p2_it->R_0.Dot(N);
        const T spacing = std::abs(height2 - height1);

        if(!std::isfinite(min_spacing)
        && (spacing < min_spacing)
        && (spacing > distance_eps)) min_spacing = spacing;
    }
    if(!std::isfinite(min_spacing)) 
        throw std::runtime_error("Not enough vertices in the provided contours. Unable to estimate separation.");

    return min_spacing;
}                            

#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float 
        Estimate_Contour_Separation(const std::list<std::reference_wrapper<contour_collection<float >>> &,
                                const vec3<float > &,
                                float );
    template double
        Estimate_Contour_Separation(const std::list<std::reference_wrapper<contour_collection<double>>> &,
                                const vec3<double> &,
                                double);
#endif                        

//---------------------------------------------------------------------------------------------------------------------------
//-------------- lin_reg_results: a simple helper class for dealing with output from linear regression routines -------------
//---------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------ Constructors -------------------------------------------------------
template <class T> lin_reg_results<T>::lin_reg_results() { }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template lin_reg_results<float >::lin_reg_results(void);
    template lin_reg_results<double>::lin_reg_results(void);
#endif

//--------------------------------------------------------- Members ---------------------------------------------------------

template <class T>
T
lin_reg_results<T>::evaluate_simple(T x) const {
    return this->intercept + this->slope * x;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template float  lin_reg_results<float >::evaluate_simple(float ) const;
    template double lin_reg_results<double>::evaluate_simple(double) const;
#endif

template <class T>
samples_1D<T>
lin_reg_results<T>::sample_uniformly_over(T xmin, T xmax, size_t n) const {
    if(n < 2) throw std::invalid_argument("Not possible to provide reasonable output with so few sample points");
    samples_1D<T> out;
    const bool InhibitSort = true;
    const auto dx = (xmax - xmin) / static_cast<T>(n-1);
    const auto zero = static_cast<T>(0);
    for(size_t i = 0; i < n; ++i){
        const auto x = static_cast<T>(i) * dx + xmin;
        out.push_back( x, zero, this->evaluate_simple(x), zero );
    }
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > lin_reg_results<float >::sample_uniformly_over(float , float , size_t) const;
    template samples_1D<double> lin_reg_results<double>::sample_uniformly_over(double, double, size_t) const;
#endif



template <class T> std::string lin_reg_results<T>::display_table(void) const {
    //Compare all members which aren't NAN. Aligns into a simple table.
    std::stringstream head, item;

    //Use a macro to simplify walking the struct. This could be replaced by introspection.
    #define LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( NAME )                      \
    {                                                                           \
        if( !std::isnan(this->NAME) ){                                          \
            const std::string n(#NAME);                                         \
            const std::string trunc = (n.size() > 10) ? n.substr(0,10) : n;     \
            head << std::setw(12) << std::setprecision(4) << trunc;             \
            item << std::setw(12) << std::setprecision(4) << this->NAME;        \
        }                                                                       \
    }

    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( slope );
    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( sigma_slope );
    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( intercept );
    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( sigma_intercept );

    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( N );
    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( dof );
    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( sigma_f );
    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( covariance );
    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( lin_corr );

    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( sum_sq_res );
    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( tvalue );
    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( pvalue );

    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( chi_square );
    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( qvalue );
    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( cov_params );
    LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER( corr_params );

    //Drop the macro.
    #undef LIN_REG_RESULTS_DISPLAY_TABLE_EXPANDER

    head << std::endl;
    item << std::endl;
    return head.str() + item.str();
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::string lin_reg_results<float >::display_table(void) const;
    template std::string lin_reg_results<double>::display_table(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::string lin_reg_results<T>::comparison_table(const lin_reg_results<T> &in) const {
    //Compare all members which aren't NAN. Aligns into a simple table.
    std::stringstream head, top, btm, comp;

    //Use a macro to simplify walking the struct. This could be replaced by introspection.
    #define LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( NAME )                   \
    {                                                                           \
        if( !std::isnan(this->NAME) || !std::isnan(in.NAME) ){                  \
            const std::string n(#NAME);                                         \
            const std::string trunc = (n.size() > 10) ? n.substr(0,10) : n;     \
            head << std::setw(12) << std::setprecision(4) << trunc;             \
            top  << std::setw(12) << std::setprecision(4) << this->NAME;        \
            btm  << std::setw(12) << std::setprecision(4) << in.NAME;           \
            if( !std::isnan(this->NAME) && !std::isnan(in.NAME) ){              \
                comp << std::setw(11) << std::setprecision(4)                   \
                     << RELATIVE_DIFF(this->NAME,in.NAME) << "%";               \
            }else{                                                              \
                comp << std::setw(11) << std::setprecision(4) << "---" << "%";  \
            }                                                                   \
        }                                                                       \
    }

    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( slope );
    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( sigma_slope );
    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( intercept );
    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( sigma_intercept );

    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( N );
    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( dof );
    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( sigma_f );
    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( covariance );
    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( lin_corr );

    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( sum_sq_res );
    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( tvalue );
    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( pvalue );

    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( chi_square );
    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( qvalue );
    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( cov_params );
    LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER( corr_params );

    //Drop the macro.
    #undef LIN_REG_RESULTS_COMPARISON_TABLE_EXPANDER

    head << std::endl;
    top  << std::endl;
    btm  << std::endl;
    comp << std::endl;
    return head.str() + top.str() + btm.str() + comp.str();
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::string lin_reg_results<float >::comparison_table(const lin_reg_results<float > &in) const;
    template std::string lin_reg_results<double>::comparison_table(const lin_reg_results<double> &in) const;
#endif



//---------------------------------------------------------------------------------------------------------------------------
//--------------------- samples_1D: a convenient way to collect a sequentially-sampled array of data ------------------------
//---------------------------------------------------------------------------------------------------------------------------

//------------------------------------------------------ Constructors -------------------------------------------------------
template <class T> samples_1D<T>::samples_1D(){ }
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float >::samples_1D(void);
    template samples_1D<double>::samples_1D(void);
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T>::samples_1D(const samples_1D<T> &in) : samples(in.samples)  {
    this->uncertainties_known_to_be_independent_and_random = in.uncertainties_known_to_be_independent_and_random;
    this->stable_sort();
    this->metadata = in.metadata;
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float >::samples_1D(const samples_1D<float > &in);
    template samples_1D<double>::samples_1D(const samples_1D<double> &in);
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T>::samples_1D(const std::list<vec2<T>> &in_samps){ 
    //Providing [x_i, f_i] data. Assumes sigma_x_i and sigma_f_i uncertainties are (T)(0).
    for(auto elem : in_samps) this->push_back(elem.x, elem.y);
    this->stable_sort();
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float >::samples_1D(const std::list< vec2<float >> &in_points);
    template samples_1D<double>::samples_1D(const std::list< vec2<double>> &in_points);
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T>::samples_1D(std::vector<std::array<T,4>> in_samps) : samples(std::move(in_samps)) { 
    this->stable_sort();
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float >::samples_1D(std::vector<std::array<float ,4>> in_samps);
    template samples_1D<double>::samples_1D(std::vector<std::array<double,4>> in_samps);
#endif


//---------------------------------------------------- Member Functions -----------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::operator=(const samples_1D<T> &rhs){
    if(this == &rhs) return *this;
    this->uncertainties_known_to_be_independent_and_random = rhs.uncertainties_known_to_be_independent_and_random;
    this->samples = rhs.samples;
    this->metadata = rhs.metadata;
    return *this;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::operator=(const samples_1D<float > &rhs);
    template samples_1D<double> samples_1D<double>::operator=(const samples_1D<double> &rhs);
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  void samples_1D<T>::push_back(T x_i, T sigma_x_i, T f_i, T sigma_f_i, bool inhibit_sort){
    this->samples.push_back( {x_i, sigma_x_i, f_i, sigma_f_i} );
    if(!inhibit_sort) this->stable_sort();
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void samples_1D<float >::push_back(float  x_i, float  sigma_x_i, float  f_i, float  sigma_f_i, bool inhibit_sort);
    template void samples_1D<double>::push_back(double x_i, double sigma_x_i, double f_i, double sigma_f_i, bool inhibit_sort);
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  void samples_1D<T>::push_back(const std::array<T,2> &x_dx, const std::array<T,2> &y_dy, bool inhibit_sort){
    this->samples.push_back( { std::get<0>(x_dx), std::get<1>(x_dx), std::get<0>(y_dy), std::get<1>(y_dy) } );
    if(!inhibit_sort) this->stable_sort();
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void samples_1D<float >::push_back(const std::array<float ,2> &x_dx, const std::array<float ,2> &y_dy, bool inhibit_sort);
    template void samples_1D<double>::push_back(const std::array<double,2> &x_dx, const std::array<double,2> &y_dy, bool inhibit_sort);
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  void samples_1D<T>::push_back(const std::array<T,4> &samp, bool inhibit_sort){
    this->samples.push_back( samp );
    if(!inhibit_sort) this->stable_sort();
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void samples_1D<float >::push_back(const std::array<float ,4> &samp, bool inhibit_sort);
    template void samples_1D<double>::push_back(const std::array<double,4> &samp, bool inhibit_sort);
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  void samples_1D<T>::push_back(T x_i, T f_i, bool inhibit_sort){
    //We assume sigma_x_i and sigma_f_i uncertainties are (T)(0).        
    this->samples.push_back( {x_i, (T)(0), f_i, (T)(0)} );
    if(!inhibit_sort) this->stable_sort();
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void samples_1D<float >::push_back(float  x_i, float  f_i, bool inhibit_sort);
    template void samples_1D<double>::push_back(double x_i, double f_i, bool inhibit_sort);
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  void samples_1D<T>::push_back(const vec2<T> &x_i_and_f_i, bool inhibit_sort){
    //We assume sigma_x_i and sigma_f_i uncertainties are (T)(0).        
    this->samples.push_back( {x_i_and_f_i.x, (T)(0), x_i_and_f_i.y, (T)(0)} );
    if(!inhibit_sort) this->stable_sort();
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void samples_1D<float >::push_back(const vec2<float > &x_i_and_f_i, bool inhibit_sort);
    template void samples_1D<double>::push_back(const vec2<double> &x_i_and_f_i, bool inhibit_sort);
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  void samples_1D<T>::push_back(T x_i, T f_i, T sigma_f_i, bool inhibit_sort){
    //We assume sigma_x_i is (T)(0).
    this->samples.push_back( {x_i, (T)(0), f_i, sigma_f_i} );
    if(!inhibit_sort) this->stable_sort();
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void samples_1D<float >::push_back(float  x_i, float  f_i, float  sigma_f_i, bool inhibit_sort);
    template void samples_1D<double>::push_back(double x_i, double f_i, double sigma_f_i, bool inhibit_sort);
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  bool samples_1D<T>::empty() const {
    return this->samples.empty();
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool samples_1D<float >::empty() const;
    template bool samples_1D<double>::empty() const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  size_t samples_1D<T>::size() const {
    return this->samples.size();
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template size_t samples_1D<float >::size() const;
    template size_t samples_1D<double>::size() const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  void samples_1D<T>::stable_sort() {
    //Sorts on x-axis. Lowest-first.
    //
    //This routine is called automatically after this->push_back(...) unless it is surpressed by the user. It can also be 
    // called explicitly by the user, but this is not needed unless they are directly editing this->samples for some reason.
    const auto sort_on_x_i = [](const std::array<T,4> &L, const std::array<T,4> &R) -> bool {
        return L[0] < R[0];
    };
    std::stable_sort(this->samples.begin(), this->samples.end(), sort_on_x_i);
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void samples_1D<float >::stable_sort();
    template void samples_1D<double>::stable_sort();
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  std::pair<std::array<T,4>,std::array<T,4>> samples_1D<T>::Get_Extreme_Datum_x(void) const {
    //Get the datum with the minimum and maximum x_i. If duplicates are found, there is no rule specifying which.
    if(this->empty()){
        const auto nan = std::numeric_limits<T>::quiet_NaN();
        return std::make_pair<std::array<T,4>,std::array<T,4>>({nan,nan,nan,nan},{nan,nan,nan,nan});
    }

    samples_1D<T> working;
    working = *this;
    working.stable_sort();
    return std::pair<std::array<T,4>,std::array<T,4>>(working.samples.front(),working.samples.back()); 
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::pair<std::array<float ,4>,std::array<float ,4>> samples_1D<float >::Get_Extreme_Datum_x(void) const;
    template std::pair<std::array<double,4>,std::array<double,4>> samples_1D<double>::Get_Extreme_Datum_x(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  std::pair<std::array<T,4>,std::array<T,4>> samples_1D<T>::Get_Extreme_Datum_y(void) const {
    //Get the datum with the minimum and maximum f_i. If duplicates are found, there is no rule specifying which.
    if(this->empty()){
        const auto nan = std::numeric_limits<T>::quiet_NaN();
        return std::make_pair<std::array<T,4>,std::array<T,4>>({nan,nan,nan,nan},{nan,nan,nan,nan});
    }

    samples_1D<T> working;
    working = this->Swap_x_and_y();
    working.stable_sort();
    const auto flippedF = working.samples.front();
    const auto flippedB = working.samples.back();
    return std::pair<std::array<T,4>,std::array<T,4>>({flippedF[2],flippedF[3],flippedF[0],flippedF[1]},
                          {flippedB[2],flippedB[3],flippedB[0],flippedB[1]});
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::pair<std::array<float ,4>,std::array<float ,4>> samples_1D<float >::Get_Extreme_Datum_y(void) const;
    template std::pair<std::array<double,4>,std::array<double,4>> samples_1D<double>::Get_Extreme_Datum_y(void) const;
#endif
///---------------------------------------------------------------------------------------------------------------------------
template <class T>   void samples_1D<T>::Normalize_wrt_Self_Overlap(void){
    //Normalizes data so that \int_{-inf}^{inf} f(x) (times) f(x) dx = 1 multiplying by constant factor.
    //
    // This routine ultimately scales all f_i by a constant factor. The sigma_f_i are also scaled.
    const auto I = this->Integrate_Overlap( *this );
    const T AA = I[0]; //We ignore the uncertainty of the integral because it isn't relevant for a simple scaling.
    const T A  = sqrt(AA);
    for(auto &P : this->samples){
        P[2] /= A;
        P[3] /= A;
    }
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void samples_1D<float >::Normalize_wrt_Self_Overlap(void);
    template void samples_1D<double>::Normalize_wrt_Self_Overlap(void);
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Strip_Uncertainties_in_x(void) const {
    //Sets uncertainties to zero. Useful in certain situations, such as computing aggregate std dev's.
    samples_1D<T> out(*this);
    for(auto &P : out.samples) P[1] = (T)(0);
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Strip_Uncertainties_in_x(void) const;
    template samples_1D<double> samples_1D<double>::Strip_Uncertainties_in_x(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Strip_Uncertainties_in_y(void) const {
    //Sets uncertainties to zero. Useful in certain situations, such as computing aggregate std dev's.
    samples_1D<T> out(*this);
    for(auto &P : out.samples) P[3] = (T)(0);
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Strip_Uncertainties_in_y(void) const;
    template samples_1D<double> samples_1D<double>::Strip_Uncertainties_in_y(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
//Ensure there is a single datum with the given x_i, averaging coincident data if necessary.
//
// Note: This routine sorts data along x_i, lowest-first.
//
// Note: Parameter eps need not be small. All parameters must be within eps of every other member of the
//       group in order to be grouped together and averaged. In other words, this routine requires <eps 
//       from the furthest-left datum, NOT <eps from the nearest-left datum. The latter would allow long
//       chains with a coincidence length potentially infinitely long.
//
// Note: Because coincident x_i are averaged (because eps need not be small), under certain circumstances
//       the output may have points separated by <eps. This should only happen when significant information
//       is lost during averaging, so is likely to happen for denormals, very large and very small inputs,
//       and any other numerically unstable situations. It is best in any case not to rely on the output
//       being separated by >=eps. (If you want such control, explicitly resampling/interpolating would be
//       a better idea.)
//
// Note: For uncertainty propagation purposes, datum with the same x_i (within eps) are considered to be
//       estimates of the same quantity. If you do not want this behaviour and you have uncertainties
//       that are known to be independent and random, consider 'faking' non-independence or non-randomness
//       for this call. That way uncertainties will be a simple average of the incoming d_x_i. 
//
template <class T> void samples_1D<T>::Average_Coincident_Data(T eps) {
    this->stable_sort();
    if(this->samples.size() < 2) return;
    auto itA = std::begin(this->samples);
    while(itA != std::end(this->samples)){
        //Advance a second iterator past all coincident samples.
        auto itB = std::next(itA);
        while(itB != std::end(this->samples)){
            if(((*itB)[0]-(*itA)[0]) < eps){
                ++itB;
            }else{
                break;
            }
        }

        //If there were not coincident data, move on to the next datum.
        const auto num = std::distance(itA,itB);
        if(num == 1){
            ++itA;
            continue;
        }

        //Average the elements [itA,itB).
        std::array<T,4> averaged;
        averaged.fill(static_cast<T>(0));
        for(auto it = itA; it != itB; ++it){
            averaged[2] += (*it)[2];
            averaged[0] += (*it)[0]; //Though these are all within eps of A, eps may not be small. 
                                     // Averaging avoids bias at the cost of some loss of precision.

            if(this->uncertainties_known_to_be_independent_and_random){
                averaged[1] += std::pow((*it)[1],static_cast<T>(2.0));
                averaged[3] += std::pow((*it)[3],static_cast<T>(2.0));
            }else{
                averaged[1] += std::abs((*it)[1]);
                averaged[3] += std::abs((*it)[3]);
            }
        }
        if(this->uncertainties_known_to_be_independent_and_random){
            averaged[1] = std::sqrt(averaged[1]);
            averaged[3] = std::sqrt(averaged[3]);
        }
        averaged[0] /= static_cast<T>(num);
        averaged[1] /= static_cast<T>(num);
        averaged[2] /= static_cast<T>(num);
        averaged[3] /= static_cast<T>(num);

        std::swap(*itA,averaged);
        itA = this->samples.erase(std::next(itA),itB);
    } 
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template void samples_1D<float >::Average_Coincident_Data(float  eps);
    template void samples_1D<double>::Average_Coincident_Data(double eps);
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Rank_x(void) const {
    //Replaces x-values with (~integer) rank. {dup,N}-plicates get an averaged (maybe non-integer) rank. The y-values (f_i 
    // and sigma_f_i) are not affected. The order of the elements will not be altered.
    //
    //NOTE: sigma_x_i uncertainties are all set to zero herein. Having an uncertainty and a rank is non-sensical.
    //
    //NOTE: Rank is 0-based!
    //
    //Note: All neighbouring points which share a common x ("N-plicates") get an averaged rank. This is important for
    //      statistical purposes and could be filtered out afterward if desired.
    samples_1D<T> out(*this);

    //Step 0 - special cases.
    if(out.empty()) return std::move(out);
    if(out.size() == 1){
        out.samples.front()[0] = (T)(0);
        out.samples.front()[1] = (T)(0);
        return std::move(out);
    }

    //Step 1 - cycle through the sorted samples.
    for(auto p_it = out.samples.begin(); p_it != out.samples.end(); ){
        const T X = (*p_it)[0]; //The x-value of interest.
        auto p2_it = p_it;      //Point where N-plicates stop.
        size_t num(1);          //Number of N-plicates.
       
        //Iterate just past the N-plicates. 
        while(((++p2_it) != out.samples.end()) && ((*p2_it)[0] == X)) ++num;
        
        //This rank gives the average of N sequential natural numbers.
        auto rank = static_cast<T>(std::distance(out.samples.begin(),p_it));
        rank += (T)(0.5) * static_cast<T>(num - (size_t)(1)); 
        
        //Set the rank and iterate past all the N-plicates.
        for(auto pp_it = p_it; pp_it != p2_it; ++pp_it){
            (*p_it)[0] = rank;
            (*p_it)[1] = (T)(0);
            ++p_it;
        }
    }
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Rank_x(void) const;
    template samples_1D<double> samples_1D<double>::Rank_x(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Rank_y(void) const {
    //Replaces y-values with (~integer) rank. {dup,N}-plicates get an averaged (maybe non-integer) rank. The x-values (x_i 
    // and sigma_x_i) are not affected. The order of the elements *might* be altered due to the use of a computational trick
    // here requiring two stable_sorts. If this is a problem, go ahead and implement a more stable version! (Why was this not
    // done in the first place? This routine was originally needed for a double-ranking scheme which didn't care about
    // stability of the order.)
    //
    //NOTE: sigma_x_i uncertainties are all set to zero herein. Having an uncertainty and a rank is non-sensical.
    //
    //NOTE: Rank is 0-based!
    //
    //Note: All neighbouring points which share a common y ("N-plicates") get an averaged rank. This is important for
    //      statistical purposes and could be filtered out afterward if desired.
    samples_1D<T> out(this->Swap_x_and_y().Rank_x().Swap_x_and_y());
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Rank_y(void) const;
    template samples_1D<double> samples_1D<double>::Rank_y(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Swap_x_and_y(void) const {
    //Swaps x_i with f_i and sigma_x_i with sigma_f_i.
    //
    //NOTE: This routine is often used to avoid writing two routines. Just write it, say, for operating on the x-values and
    //      then for the y-values version do:  Swap_x_and_y().<operation on x-values here>.Swap_x_and_y();. It is more costly
    //      in terms of computation, creating extra copies all around, but it is quite convenient for maintenance and 
    //      testing purposes.
    samples_1D<T> out(*this);
    for(auto p_it = out.samples.begin(); p_it != out.samples.end(); ++p_it){
        const auto F = (*p_it);
        (*p_it)[0] = F[2];
        (*p_it)[1] = F[3];
        (*p_it)[2] = F[0];
        (*p_it)[3] = F[1];
    }
    out.stable_sort();
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Swap_x_and_y(void) const;
    template samples_1D<double> samples_1D<double>::Swap_x_and_y(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Sum_x(void) const {
    //Computes the sum with proper uncertainty propagation.
    // 
    // NOTE: If there is no data, a sum of zero with infinite uncertainty will be returned.
    //
    if(this->empty()){
        return { (T)(0), std::numeric_limits<T>::infinity() };
    }else if(this->size() == 1){
        return { this->samples.front()[0], this->samples.front()[1] };
    }

    T sum((T)(0));
    T sigma((T)(0));
    for(const auto &P : this->samples){
        sum += P[0];
        if(this->uncertainties_known_to_be_independent_and_random){
            sigma += P[1]*P[1]; 
        }else{
            sigma += std::abs(P[1]);
        }
    }
    if(this->uncertainties_known_to_be_independent_and_random) sigma = std::sqrt(sigma);
    return {sum, sigma};
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Sum_x(void) const;
    template std::array<double,2> samples_1D<double>::Sum_x(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Sum_y(void) const {
    //Computes the sum with proper uncertainty propagation.
    // 
    // NOTE: If there is no data, a sum of zero with infinite uncertainty will be returned.
    //
    if(this->empty()){
        return { (T)(0), std::numeric_limits<T>::infinity() };
    }else if(this->size() == 1){
        return { this->samples.front()[2], this->samples.front()[3] };
    }
    
    T sum((T)(0));
    T sigma((T)(0));
    for(const auto &P : this->samples){
        sum += P[2];
        if(this->uncertainties_known_to_be_independent_and_random){
            sigma += P[3]*P[3]; 
        }else{
            sigma += std::abs(P[3]);
        }
    }   
    if(this->uncertainties_known_to_be_independent_and_random) sigma = std::sqrt(sigma);
    return {sum, sigma};
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Sum_y(void) const;
    template std::array<double,2> samples_1D<double>::Sum_y(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Average_x(void) const {
    //Computes the average and std dev of the *data*. This std dev is not a measure of uncertainty in the mean. Please keep 
    // reading if you are confused.
    // 
    // The "std dev of the data" characterizes the width of a distribution. An infinite number of datum will *not* 
    // necessarily result in a zero std dev of the data. In this case you are essentially wanting an AVERAGE or want to 
    // characterize a distribution of numbers. Each datum could be taken to have uncertainty equal to the std dev of the 
    // data. (See John R. Taylor's "An Introduction to Error Analysis", 2nd Ed. for justification).
    // 
    // The "std dev of the mean" characterizes how certain we are of the mean, as computed from a collection of datum which
    // we *believe* to be independent measurements of the same quantity. In this case you are essentially wanting a MEAN or
    // want to produce a best estimate of some quantity -- but you don't want to characterize the distribution the mean came
    // from. An infinite number of measurements will result in a zero std dev of the mean.
    //
    // Consider two illustrative scenarios:
    // 
    // 1. You measure the height of all the students in a class. You want to know how tall the average student is, so you 
    //    take all the measurements and compute the AVERAGE. The std dev (of the data) describes how much the height varies,
    //    and says something about the distribution the AVERAGE was pulled from. ---> Use this function!
    //
    // 2. You want to precisely measure a single student's height, so you get all the other students to measure his height.
    //    To get the best estimate, you compute the MEAN. The std dev (of the mean) describes how confident you are in the 
    //    MEAN height. More measurements (maybe a million) will give you a precise estimate, so the std dev (of the mean) 
    //    will tend to zero as more data is collected. ---> Use the MEAN function!
    //
    // NOTE: This routine ignores uncertainties. You are looking for a 'weighted-average' if you need to take into account
    //       existing uncertainties in the data.
    //
    // NOTE: This routine uses an unbiased estimate of the std dev (by dividing by N-1 instead of N) and is thus suitable 
    //       for incomplete data sets. Exactly-complete sets (aka population) should use N instead of N-1, although the 
    //       difference is often negligible when ~ N > 10.
    //
    // NOTE: If there is no data, an average of zero with infinite uncertainty will be returned.
    //
    if(this->empty()){
        //FUNCWARN("Cannot compute the average of an empty set");
        return { (T)(0), std::numeric_limits<T>::infinity() };
    }else if(this->size() == 1){
        //FUNCWARN("Computing the average of a single element. The std dev will be infinite");
        return { this->samples.front()[0], std::numeric_limits<T>::infinity() };
    } 
    const T N = static_cast<T>(this->samples.size());
    const T invN = (T)(1)/N;
    const T invNminusone = (T)(1)/(N - (T)(1));

    //First pass: compute the average.
    T x_avg((T)(0));
    for(auto p_it = this->samples.begin(); p_it != this->samples.end(); ++p_it){
        const T x_i = (*p_it)[0];
        x_avg += x_i*invN;
    }

    //Second pass: compute the std dev.
    T stddevdata((T)(0));
    for(auto p_it = this->samples.begin(); p_it != this->samples.end(); ++p_it){
        const T x_i = (*p_it)[0];
        stddevdata += (x_i - x_avg)*(x_i - x_avg)*invNminusone;
    }
    stddevdata = std::sqrt(stddevdata);

    return {x_avg, stddevdata};
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Average_x(void) const;
    template std::array<double,2> samples_1D<double>::Average_x(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Average_y(void) const {
    //See this->Average_x() for info.
    if(this->empty()){ 
        //FUNCWARN("Cannot compute the average of an empty set");
        return { (T)(0), std::numeric_limits<T>::infinity() };
    }else if(this->size() == 1){
        //FUNCWARN("Computing the average of a single element. The std dev will be infinite");
        return { this->samples.front()[2], std::numeric_limits<T>::infinity() };
    }
    const T N = static_cast<T>(this->samples.size());
    const T invN = (T)(1)/N;
    const T invNminusone = (T)(1)/(N - (T)(1));
    
    //First pass: compute the average.
    T y_avg((T)(0));
    for(auto p_it = this->samples.begin(); p_it != this->samples.end(); ++p_it){
        const T y_i = (*p_it)[2];
        y_avg += y_i*invN;
    }

    //Second pass: compute the std dev.
    T stddevdata((T)(0));
    for(auto p_it = this->samples.begin(); p_it != this->samples.end(); ++p_it){
        const T y_i = (*p_it)[2];
        stddevdata += (y_i - y_avg)*(y_i - y_avg)*invNminusone;
    }
    stddevdata = std::sqrt(stddevdata);

    return {y_avg, stddevdata};
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Average_y(void) const;
    template std::array<double,2> samples_1D<double>::Average_y(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Mean_x(void) const {
    //Computes the statistical mean and std dev of the *mean*. This std dev measures the uncertainty of the mean. For the 
    // std dev of the mean to be sensical, the samples must be measurements of the *same quantity*. Please keep reading if 
    // you are confused.
    // 
    // The "std dev of the data" characterizes the width of a distribution. An infinite number of datum will *not* 
    // necessarily result in a zero std dev of the data. In this case you are essentially wanting an AVERAGE or want to 
    // characterize a distribution of numbers. Each datum could be taken to have uncertainty equal to the std dev of the 
    // data. (See John R. Taylor's "An Introduction to Error Analysis", 2nd Ed. for justification).
    // 
    // The "std dev of the mean" characterizes how certain we are of the mean, as computed from a collection of datum which
    // we *believe* to be independent measurements of the same quantity. In this case you are essentially wanting a MEAN or
    // want to produce a best estimate of some quantity -- but you don't want to characterize the distribution the mean came
    // from. An infinite number of measurements will result in a zero std dev of the mean.
    //
    // Consider two illustrative scenarios:
    // 
    // 1. You measure the height of all the students in a class. You want to know how tall the average student is, so you 
    //    take all the measurements and compute the AVERAGE. The std dev (of the data) describes how much the height varies,
    //    and says something about the distribution the AVERAGE was pulled from. ---> Use the AVERAGE function!
    //
    // 2. You want to precisely measure a single student's height, so you get all the other students to measure his height.
    //    To get the best estimate, you compute the MEAN. The std dev (of the mean) describes how confident you are in the 
    //    MEAN height. More measurements (maybe a million) will give you a precise estimate, so the std dev (of the mean) 
    //    will tend to zero as more data is collected. ---> Use this function!
    //
    // NOTE: This routine ignores uncertainties. You are looking for a 'weighted-mean' if you need to take into account
    //       existing uncertainties in the data. Be aware the weighted mean is a slightly different beast!
    //
    // NOTE: This routine uses an unbiased estimate of the std dev (by dividing by N-1 instead of N) and is thus suitable 
    //       for incomplete data sets. Exactly-complete sets (aka population) should use N instead of N-1, although the 
    //       difference is often negligible when ~ N > 10.
    //
    // NOTE: If there is no data, an average of zero with infinite uncertainty will be returned.
    //
    // NOTE: Do you want the MEAN or the MEDIAN? From ("Weighted Median Filters: A Tutorial" by Lin Yin, 1996):
    //                   "[...] the median is the maximum likelihood estimate of the signal level in
    //                    the presence of uncorrelated additive biexponentially distributed noise;
    //                    while, the arithmetic mean is that for Gaussian distributed noise."
    // 
    if(this->empty()) return { (T)(0), std::numeric_limits<T>::infinity() }; 
    const T N = static_cast<T>(this->samples.size());
    const auto avg = this->Average_x();
    const T mean = avg[0];
    const T stddevdata = avg[1];
    const T stddevmean = ( std::isfinite(stddevdata) ? stddevdata/std::sqrt(N) : std::numeric_limits<T>::infinity() ); 
    return { mean, stddevmean };
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Mean_x(void) const;
    template std::array<double,2> samples_1D<double>::Mean_x(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Mean_y(void) const {
    //See this->Mean_x() for info.
    if(this->empty()) return { (T)(0), std::numeric_limits<T>::infinity() };
    const T N = static_cast<T>(this->samples.size());
    const auto avg = this->Average_y();
    const T mean = avg[0];
    const T stddevdata = avg[1];
    const T stddevmean = ( std::isfinite(stddevdata) ? stddevdata/std::sqrt(N) : std::numeric_limits<T>::infinity() );
    return { mean, stddevmean };
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Mean_y(void) const;
    template std::array<double,2> samples_1D<double>::Mean_y(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Weighted_Mean_x(void) const {
    //Computes the weighted mean and std dev of the *weighted data*. For the differences between what the AVERAGE and MEAN
    // routines return, see either the Mean_x()/Mean_y() or Average_x()/Average_y() routines. Hint: this is a true mean,
    // not an average. (The uncertainty relation can be derived using the standard error propagation relation.)
    //
    // The weighting factors used in the calculation are the sigma_x_i uncertainties: w_i for x_i is 1/(sigma_x_i^2).
    //
    // When is this routine appropriate? Let's say you have ten papers that report slightly different estimates of the
    // same physical constant. A weighted mean is a good way to figure out the aggregate estimate in which high-certainty
    // estimates are given more weight.
    //
    // NOTE: This function ought to handle ANY sigma_x_i uncertainties passed in, including zero. Keep in mind that if 
    //       there is data with zero sigma_x_i and there is also data with non-zero sigma_x_i, the non-zero sigma_x_i data
    //       will not contribute to the weighted average. This is because the zero sigma_x_i data are infinitely strongly
    //       weighted. Be aware that zero uncertainties going in will (correctly) yield zero uncertainties going out!
    //
    // NOTE: All infinitely-strongly weighted data are treated as being identically weighted. Be aware that this might not
    //       reflect reality, especially since the weights go like 1/(sigma_x_i^2) and will thus blow up quickly with small
    //       uncertainties!
    // 
    // NOTE: A weighted mean (and std dev of the weighted mean) reduces to the statistical mean (and std dev of the mean) 
    //       iff the weights are unanimously (T)(1). If the weights are all equal but not (T)(1) then the weighted mean will
    //       reduce to the statistical mean, but the std dev of the weighted mean will not reduce to the std dev of the 
    //       mean. It will be skewed.
    //
    // NOTE: If there is no data, an average of zero with infinite uncertainty will be returned.
    //
    // NOTE: This routine is *not* immune to overflow. Keep the numbers nice, or compress them if you must.
    //
    // NOTE: In almost all situations you will want to use this routine, you should declare (and ensure) that the datum
    //       are normally-distributed. If the datum are strongly covariant then it is OK to make no assumptions, but the
    //       idea of binning such data loses much of its sense if the data is not normally distributed. Try avoid such a
    //       situation by carefully evaluating the normality of the bins before relying on this routine.
    //
    if(this->empty()){
        //FUNCWARN("Cannot compute the weighted mean of an empty set");
        return { std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::infinity() };
    }else if(this->size() == 1){
        //FUNCWARN("Computing the weighted mean of a single element. The std dev will be infinite");
        return { this->samples.front()[0], std::numeric_limits<T>::infinity() };
    }

    //Characterize the data. If there are any very low uncertainty datum (~zero) then can later ignore the others.
    // We will also proactively compute some things so that only a single loop is required.
    long int Nfinite = 0; //The number of datum with finite weights.
    long int Ninfinite = 0; //The number of datum with infinite weights.
    T sum_w_fin = (T)(0); //Only finite-weights summed.
    T sum_w_x_fin = (T)(0); //Only finite-weighted datum: sum of w_i*x_i.
    T sum_x_inf = (T)(0); //The sum of x_i for all infinite-weighted datum.
    T sum_sqrt_w_i_fin = (T)(0); //Only finite-weighted datum: sum of sqrt(w_i) for non-normal data.
    for(const auto &P : this->samples){
        const T x_i = P[0];
        const T sigma_x_i = P[1];
        if(!std::isfinite(sigma_x_i)) continue; //Won't count toward mean.

        const T w_i = (T)(1)/(sigma_x_i*sigma_x_i);
        if(!std::isfinite(w_i)){
            ++Ninfinite;
            sum_x_inf += x_i;
        }else{
            ++Nfinite;
            sum_w_fin += w_i;
            sum_w_x_fin += w_i*x_i; 
            sum_sqrt_w_i_fin += (T)(1)/std::abs(sigma_x_i);
        }
    }

    //Verify that at least some data was suitable.
    if((Nfinite + Ninfinite) == 0){
        //FUNCWARN("No data was suitable for weighted mean. All datum had inf uncertainty");
        return { std::numeric_limits<T>::quiet_NaN(), std::numeric_limits<T>::infinity() };
    }

    //If there were infinite-weighted datum, it is easy to proceed. There is legitimately no uncertainty due to the
    // infinitly-strong weighting. And the weights are all equal, so the mean is a regular mean.
    if(Ninfinite != 0){
        const T mean = sum_x_inf / static_cast<T>(Ninfinite);
        return { mean, (T)(0) };
    }

    //Otherwise, we need to calculate uncertainty.
    const T mean = sum_w_x_fin / sum_w_fin;
    T sigma = (T)(0);
    if(this->uncertainties_known_to_be_independent_and_random){
        sigma = (T)(1)/std::sqrt(sum_w_fin);
    }else{
        sigma = sum_sqrt_w_i_fin / sum_w_fin;
    }
    return { mean, sigma };
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Weighted_Mean_x(void) const;
    template std::array<double,2> samples_1D<double>::Weighted_Mean_x(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Weighted_Mean_y(void) const {
    //See this->Weighted_Mean_x() for info.
    const auto copy(this->Swap_x_and_y());
    return copy.Weighted_Mean_x();
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Weighted_Mean_y(void) const;
    template std::array<double,2> samples_1D<double>::Weighted_Mean_y(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Median_y(void) const {
    //Finds or computes the median datum with linear interpolation halfway between datum (i.e., un-weighted mean) if N is odd.
    std::array<T,2> out = { std::numeric_limits<T>::quiet_NaN(),
                            std::numeric_limits<T>::quiet_NaN() };

    if(this->samples.empty()) return out;
    if(this->samples.size() == 1){
        out[0] = this->samples.front()[2];
        out[1] = this->samples.front()[3];
        return out;
    }

    //Sort the y-data.
    samples_1D<T> working(this->Swap_x_and_y());
    working.stable_sort();

    size_t N = working.samples.size();
    size_t M = N/2;

    //Advance to just shy of the halfway datum, or to the left of the central data pair.
    auto it = std::next(working.samples.begin(),M-1);
    if(2*M == N){ //If there is a pair, take the mean.
        const auto L = *it;
        ++it;
        const auto R = *it;

        //Grab the left and right f_i and sigma_f_i. (Remember: we treat them as if they were
        // x_i and sigma_x_i becasue we swapped x and y earlier.)
        const auto L_f_i = L[0];
        const auto L_sigma_f_i = L[1];
        const auto R_f_i = R[0];
        const auto R_sigma_f_i = R[1];

        out[0] = (T)(0.5)*L_f_i + (T)(0.5)*R_f_i;

        //Simple half-way point (i.e., un-weighted mean) uncertainty propagation.
        if(this->uncertainties_known_to_be_independent_and_random){
            out[1] = std::sqrt( std::pow((T)(0.5)*L_sigma_f_i, 2) + std::pow((T)(0.5)*R_sigma_f_i, 2) );
        }else{
            out[1] = (T)(0.5)*std::abs(L_sigma_f_i) + (T)(0.5)*std::abs(R_sigma_f_i);
        }
        return out;
    }
    ++it;
    out[0] = (*it)[0];
    out[1] = (*it)[1];
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Median_y(void) const;
    template std::array<double,2> samples_1D<double>::Median_y(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Median_x(void) const {
    //See this->Median_y() for info.
    const auto copy(this->Swap_x_and_y());
    return copy.Median_y();
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Median_x(void) const;
    template std::array<double,2> samples_1D<double>::Median_x(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Select_Those_Within_Inc(T L, T H) const {
    //Selects those with a key within [L,H] (inclusive), leaving the order intact.
    //
    //We just scan from lowest to highest, copying the elements which are inclusively in the range. This obviates the
    // need to sort afterward.
    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
    out.metadata = this->metadata;
    const bool inhibit_sort = true;
    for(const auto &elem : this->samples){
        const T x_i = elem[0];
        if(isininc(L, x_i, H)) out.push_back(elem, inhibit_sort);
    }
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Select_Those_Within_Inc(float  L, float  H) const;
    template samples_1D<double> samples_1D<double>::Select_Those_Within_Inc(double L, double H) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  std::array<T,4> samples_1D<T>::Interpolate_Linearly(const T &at_x) const {
    //Interpolation assuming the datum are connected with line segments. Standard error propagation formulae are used to 
    // propagate uncertainties (i.e., the uncertainties are *not* themselves linearly interpolated).
    //
    // Returns [at_x, sigma_at_x (== 0), f, sigma_f] interpolated at 'at_x'. If at_x is not within the range of the samples, 
    // expect {at_x, (T)(0), (T)(0),(T)(0)}.
    //
    // If at_x exactly matches one of the samples, no interpolation is performed. Datum are scanned from lowest to highest, 
    // so interpolation at high values will be more costly than low values.
    //
    // NOTE: The returned sigma_at_x is always (T)(0). Due to propagation of uncertainties, under the assumption that datum
    //       are connected by line segment, the nearby sigma_x_i and sigma_f_i will be converted into an equivalent total
    //       sigma_f. So the sigma_at_x is truly zero. It is returned for consistency.
    //
    // NOTE: While uncertainties in x_i are honoured for uncertainty propagation purposes, they are disregarded when
    //       determining which two neighbouring datum the provided x lies between. If the uncertainties in x_i are larger
    //       than the separation of x_i, then a more sophisticated treatment will probably be necessary. For example, taking
    //       into account the excessive uncertainty of neighbouring x_i might be worthwhile. This condition is NOT inspected
    //       herein, so you should always be aware of how large your uncertainties are.
    //
    //NOTE: If at_x lies exactly on a datum, then we end up over-estimating the uncertainty due to the interpolation
    //      formula simplifying. We do not correct this because exactly on the datum it is difficult to assess what
    //      happens to the sigma_x_i. (Is it somehow converted to sigma_f_i? Is it reported as-is, even though all 
    //      other interpolated points have no sigma_x_i? etc..)
    //

    //Step 0 - Sanity checks.
    if(this->samples.size() < 2) FUNCERR("Unable to interpolate - there are less than 2 samples");

    const std::array<T,4> at_x_as_F { at_x, (T)(0), (T)(0), (T)(0) }; //For easier integration with std::algorithms.

    //Step 1 - Check if the point is within the endpoints. 
    const T lowest_x  = this->samples.front()[0];
    const T highest_x = this->samples.back()[0];
    if(!isininc(lowest_x, at_x, highest_x)){
        //Here will we 'interpolate' to zero. I *think* this is makes the most mathematical sense: the distribution is not 
        // defined outside its range, so it should either be zero or +-infinity. There is only one (mathematical) zero and
        // two infinities. Zero is the unique, sane choice.
        //
        // Note that this causes two problems. It possibly makes the distribution discontinuous, and also possibly makes the
        // uncertainty discontinuous. Be aware of this if continuity is important for your problem.
        return at_x_as_F;
    }

    //Step 2 - Check if the point is bounded by two samples (or if it is sitting exactly on a sample).
    //
    //The following are initialized to 0 to silence warnings.
    T  x0((T)(0)),  x1((T)(0)),  y0((T)(0)),  y1((T)(0));
    T sx0((T)(0)), sx1((T)(0)), sy0((T)(0)), sy1((T)(0)); 
    {
        //We use a binary search routine to find the bounds. First, we look for the right bound and then assume the point
        // just to the left is the left bound. (So this routine should honour the ordering of points with identical x_i.)
        const auto lt_xval = [](const std::array<T,4> &L, const std::array<T,4> &R) -> bool {
            return L[0] < R[0];
        };
        auto itR = std::upper_bound(this->samples.begin(), this->samples.end(), at_x_as_F, lt_xval);
        //Check if at_x is exactly on the highest_x point. If it is, we simplify the rest of the routine by including this
        // point in the preceeding range. Since there is one extra point that is not naturally captured when using ranges
        // like [x_i, x_{i+1}) or (x_i, x_{i+1}] this is a necessary step.
        if(itR == this->samples.end()) itR = std::prev(this->samples.end());
        const auto itL = std::prev(itR);
        x0  = (*itL)[0];  x1  = (*itR)[0];  //x_i to the left of at_x.
        sx0 = (*itL)[1];  sx1 = (*itR)[1];  //uncertainty in x_i to the left of at_x.
        y0  = (*itL)[2];  y1  = (*itR)[2];  //x_i to the right of at_x.
        sy0 = (*itL)[3];  sy1 = (*itR)[3];  //uncertainty in x_i to the right of at_x.
    }

    //Step 2.5 - Check if the points are (effectively, numerically) on top of one another. Bail, if so.
    const T dx = x1 - x0;
    const T invdx = (T)(1)/dx;
    if(!std::isnormal(dx) || !std::isnormal(invdx)){ //Catches dx=0, 0=1/dx, inf=1/dx cases.
        FUNCERR("Cannot interpolate between two (computationally-identical) points. Try removing them by "
                "averaging f_i and adding sigma_x_i and sigma_f_i in quadrature. Cannot continue");
        //In this case, trying to assume that (at_x-x0) is half that of (x1-x0) and other schemes to re-use the linear eqn
        // will fail. Why? We ultimately need a derivative, and the derivative is zero here (unless f_i are the same too).
        // The only reasonable solution is to deal with these problems PRIOR to calling this function; we cannot do anything
        // because this is a const member!
    }

    //Step 3 - Given a (distinct) lower and an upper point, we perform the linear interpolation.
    const T f_at_x = y0 + (y1 - y0)*(at_x - x0)*invdx;

    //Step 4 - Work out the uncertainty. These require partial derivatives of f_at_x (== F) thus we have dF/dy0, etc..
    const T dFdy0 = (x1 - at_x)*invdx;
    const T dFdy1 = (at_x - x0)*invdx;
    const T dFdx0 = ((at_x - x1)*(y1 - y0))*invdx*invdx;
    const T dFdx1 = ((x0 - at_x)*(y1 - y0))*invdx*invdx;

    T sigma_f_a_x = (T)(0);
    if(this->uncertainties_known_to_be_independent_and_random){
        sigma_f_a_x = std::sqrt( (dFdy0 * sy0)*(dFdy0 * sy0) + (dFdy1 * sy1)*(dFdy1 * sy1)
                               + (dFdx0 * sx0)*(dFdx0 * sx0) + (dFdx1 * sx1)*(dFdx1 * sx1) );
    }else{
        sigma_f_a_x = ( std::abs(dFdy0 * sy0) + std::abs(dFdy1 * sy1)
                      + std::abs(dFdx0 * sx0) + std::abs(dFdx1 * sx1) );
    }
    return { at_x, (T)(0), f_at_x, sigma_f_a_x };
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,4> samples_1D<float >::Interpolate_Linearly(const float  &at_x) const;
    template std::array<double,4> samples_1D<double>::Interpolate_Linearly(const double &at_x) const;
#endif

//---------------------------------------------------------------------------------------------------------------------------
template <class T> 
samples_1D<T> 
samples_1D<T>::Crossings(T val) const {
    //Returns linearly interpolated crossing-points. 
    //
    // Note: This routine splits the vertical dimension into exactly two parts: >= the value, and < the value.
    //       This slight bias eliminates the degenerate case of infinite intersections. It is possible to eliminate the
    //       bias, but then the degenerate case must be dealt with directly.
    //
    // Note: This routine requires input to sorted.
    //
    // Note: The crossings are returned with an estimate of the uncertainty and f_at_x (which should equal the specified
    //       value). Mostly you'll probably just want the x's though.
    //       
    const auto N = this->samples.size();
    if(N < 2) throw std::invalid_argument("Unable to detect crossing-points with < 2 datum");

    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
    const bool InhibitSort = true;

    bool LastAbove = (this->samples.front()[2] >= val);
    for(size_t i = 1; i < N; ++i){
        const auto f = this->samples[i][2];
        const bool Above = (f >= val);
        if(Above != LastAbove){
            const auto xA = this->samples[i-1][0];
            const auto xB = this->samples[i][0];
            const auto fA = this->samples[i-1][2];
            const auto fB = f;
            line<T> lf( vec3<T>(xA, fA, static_cast<T>(0)),
                        vec3<T>(xB, fB, static_cast<T>(0)) );
            line<T> lv( vec3<T>(xA, val, static_cast<T>(0)),
                        vec3<T>(xB, val, static_cast<T>(0)) );
            vec3<T> P;
            if(lf.Closest_Point_To_Line( lv, P )){
                out.push_back( this->Interpolate_Linearly( P.x ), InhibitSort );
            }else{
                //Getting here could happen if the implementations differ in their susceptibility to floating-point issues.
                throw std::logic_error("Unable to compute line-line intersection point.");
            }
        }
        LastAbove = Above;
    }
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Crossings(float ) const;
    template samples_1D<double> samples_1D<double>::Crossings(double) const;
#endif

//---------------------------------------------------------------------------------------------------------------------------
template <class T> 
samples_1D<T> 
samples_1D<T>::Peaks(void) const {
    //Returns the locations linearly-interpolated peaks.
    //
    // Note: This routine requires input to sorted.
    //
    // Note: The crossings are returned with an estimate of the uncertainty and f_at_x (i.e., the height of the peak).
    //       
    const auto N = this->samples.size();
    if(N < 3) throw std::invalid_argument("Unable to identify peaks with < 3 datum");

    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
    const bool InhibitSort = true;

    //Compute the first and second derivatives.
    auto deriv_1st = this->Derivative_Centered_Finite_Differences();
    auto deriv_2nd = deriv_1st.Derivative_Centered_Finite_Differences();

    //Locate zero-crossings of the first derivative.
    auto zero_crossings = deriv_1st.Crossings(static_cast<T>(0));

    //For each, evaluate the inflection. If negative, add the peak x and f_at_x to the outgoing collection.
    for(const auto &p : zero_crossings.samples){
        const auto x = p[0];
        const auto fpp = deriv_2nd.Interpolate_Linearly(x);
        if(fpp[2] <= static_cast<T>(0)){
            out.push_back( this->Interpolate_Linearly(x), InhibitSort );
        }
    }
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Peaks(void) const;
    template samples_1D<double> samples_1D<double>::Peaks(void) const;
#endif

//---------------------------------------------------------------------------------------------------------------------------
template <class T> 
samples_1D<T> 
samples_1D<T>::Resample_Equal_Spacing(size_t N) const {
    //Resamples the data into approximately equally-spaced samples using linear interpolation.
    if(N < 2) throw std::invalid_argument("Unable to resample < 2 datum with equal spacing");

    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
    const bool InhibitSort = true;

    //Get the endpoints.
    auto extrema = this->Get_Extreme_Datum_x();
    const T xmin = extrema.first[0];
    const T xmax = extrema.second[0];
    const T dx = (xmax - xmin) / static_cast<T>(N-1);
    for(size_t i = 0; i < N; ++i){
        out.push_back( this->Interpolate_Linearly( xmin + dx * static_cast<T>(i) ), InhibitSort );
    }
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Resample_Equal_Spacing(size_t) const;
    template samples_1D<double> samples_1D<double>::Resample_Equal_Spacing(size_t) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Multiply_With(T factor) const {
    //Multiply all sample f_i's by a given factor. Uncertainties are appropriately scaled too.
    samples_1D<T> out;
    out = *this;
    const T abs_factor = std::abs(factor);
    for(auto &elem : out.samples){
        elem[2] *= factor;
        elem[3] *= abs_factor; //Same for either type of uncertainty.
    }
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Multiply_With(float  factor) const;
    template samples_1D<double> samples_1D<double>::Multiply_With(double factor) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Sum_With(T factor) const {
    //Add the given factor to all sample f_is. Uncertainties are unchanged (since the given factor
    // implicitly has no uncertainty).
    samples_1D<T> out;
    out = *this;
    for(auto &elem : out.samples){
        elem[2] += factor;
    }
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Sum_With(float  factor) const;
    template samples_1D<double> samples_1D<double>::Sum_With(double factor) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Apply_Abs(void) const {
    //Apply an absolute value functor to all f_i.
    samples_1D<T> out;
    out = *this;
    for(auto &elem : out.samples) elem[2] = std::abs(elem[2]);
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Apply_Abs(void) const;
    template samples_1D<double> samples_1D<double>::Apply_Abs(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Sum_x_With(T dx) const {
    //Shift all x_i's by a factor. No change to uncertainties.
    samples_1D<T> out;
    out = *this;
    for(auto &sample : out.samples) sample[0] += dx;
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Sum_x_With(float  dx) const;
    template samples_1D<double> samples_1D<double>::Sum_x_With(double dx) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Multiply_x_With(T x) const {
    //Multiply all x_i's by a factor. No change to uncertainties.
    samples_1D<T> out;
    out = *this;
    for(auto &sample : out.samples) sample[0] *= x;
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Multiply_x_With(float  x) const;
    template samples_1D<double> samples_1D<double>::Multiply_x_With(double x) const;
#endif
///---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Sum_With(const samples_1D<T> &g) const {
    //This routine sums two samples_1D by assuming they are connected by line segments. Because linear interpolation is used,
    // the error propagation may deal with sigma_x_i uncertainty in an odd way; don't be surprised if the vertical 
    // uncertainties grow larger and the horizontal uncertainties grow smaller, or if both grow larger!
    //
    // NOTE: If the samples share exactly the same x_i in the same order, this routine should only require a single pass
    //       through the datum of each. If one has a subset of the other's x_i with no holes, it should also only require
    //       a single pass. I think all other combinations will require linear interpolation which could involve sweeping
    //       over all datum in either samples_1D (unless some sort of index has been built in after writing). Be aware of
    //       potentially high computation cost of combining large or numerous distinct-x_i samples_1D. 
    //
    // NOTE: Metadata from both inputs are injected into the new contour, but items from *this have priority in case there
    //       is a collision.
    //
    // NOTE: Uncertainty bars may abruptly grow large or shrink at the range perhiphery of one samples_1D. For example, if 
    //       summing two samples_1D with similar f_i and sigmas, the uncertainty will naturally jump at the edges.
    //
    //   F:                              |               |
    //                                   |               |
    //                               |   X               X  |                        
    //                               |   |               |  |                                                    
    //   ----------------------------X---|---------------|--X--------------------------------------------------> x
    //                               |                      |                                                    
    //                               |                      |                                                    
    //                           
    //                                                                  |                                       
    //   G:                     |            |           |              |            |                           
    //          |      |        |            |           |              X            |        |                   
    //      |   |      |        X            X           X              |            X        |       |          
    //      |   X      X        |            |           |              |            |        X       |           
    //   ---X---|------|--------|------------|-----------|---------------------------|--------|-------X--------> x
    //      |   |      |                                                                      |       |
    //      |                                                                                         |          
    //                           
    //                                 (region of reduced              _ ____
    //                       ___ _       uncertainties)            __-- |    ----____                           
    //  F+G:            __---   | -_    _ ___ ____   ____       _--     |            |                           
    //          |      |        |   -  / |   |    ---    |\  _--        X            |        |                   
    //      |   |      |        X    |/  X   X           X \|           |            X        |       |          
    //      |   X      X        |    |   |   |           |  |           |            |        X       |           
    //   ---X---|------|--------|----X----------------------X------------------------|--------|-------X--------> x
    //      |   |      |             |                      |                                 |       |
    //      |                                                                                         |          
    //                           
    //                             /|______________________/|___ Jump in uncertainties here.
    // 
    //   Because F's domain is much shorter than G's domain, the uncertainties will jump discontinuously at the boundary
    //   of F. This is natural, and is due to the discontinuity of F's uncertaintites over the domain of G. 
    //
    //   One way around this would be to SMOOTHLY TAPER the edges of F so the discontinuity is not so abrupt. How best to
    //   smooth requires interpretation of the data, but something as simple as a linear line should work. Use f_i = 0 and 
    //   sigma_f_i = 0 just to the right and just to the left of the domain.
    //

    //Step 0 - Special cases. If either *this or g are empty (i.e., zero everywhere), simply return the other.
    if(g.empty()) return *this;
    if(this->empty()) return g;

    //We can assume that the resultant samples_1D is normal, random, and independent only if both the input are too.
    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = ( this->uncertainties_known_to_be_independent_and_random
                                                          && g.uncertainties_known_to_be_independent_and_random ); 
    out.metadata = this->metadata;
    out.metadata.insert(g.metadata.begin(), g.metadata.end()); //Will not overwrite existing elements from this->metadata.

    //These are used to try avoid (costly) interpolations.
    const T f_lowest_x  = this->samples.front()[0];
    const T f_highest_x = this->samples.back()[0];
    const T g_lowest_x  = g.samples.front()[0];
    const T g_highest_x = g.samples.back()[0];

    //Get the part on the left where only one set of samples lies.
    auto f_it = this->samples.begin(); 
    auto g_it = g.samples.begin();
    const bool inhibit_sort = true; // We scan from lowest to highest and thus never need to sort.

    while( true ){
        const bool fvalid = (f_it != this->samples.end());  
        const bool gvalid = (g_it != g.samples.end());

        if(fvalid && !gvalid){
            out.push_back(*f_it, inhibit_sort);
            ++f_it;
            continue;
        }else if(gvalid && !fvalid){
            out.push_back(*g_it, inhibit_sort);
            ++g_it;
            continue;
        }else if(fvalid && gvalid){
            const T F_x_i       = (*f_it)[0];
            const T F_sigma_x_i = (*f_it)[1];
            const T F_f_i       = (*f_it)[2];
            const T F_sigma_f_i = (*f_it)[3];
            const T G_x_i       = (*g_it)[0];
            const T G_sigma_x_i = (*g_it)[1];
            const T G_f_i       = (*g_it)[2];
            const T G_sigma_f_i = (*g_it)[3];

            //The lowest x_i gets pushed into the summation.
            if( F_x_i == G_x_i ){
                const T x_i = F_x_i; // == G_x_i == 0.5*(F_x_i + G_x_i).
                const T f_i = F_f_i + G_f_i;
                T sigma_x_i, sigma_f_i;

                if(out.uncertainties_known_to_be_independent_and_random){
                    //I originally treated this as an average, but this doesn't make sense. If G_sigma_x_i is zero,
                    // then we end up with 0.5*F_sigma_i for the new point. Rather, I *believe* we should end up
                    // with 1.0*F_sigma_i. This seems more intuitively correct and is in any case safer, so I 
                    // decided to just drop the 0.5* factor.
                    //
                    // I'm not so sure that I even can treat it as an average. The x_i line up, but they are not
                    // considered measurements of the same thing. They just happen to have the same expected mean...
                    //
                    //sigma_x_i = 0.5*std::sqrt( F_sigma_x_i*F_sigma_x_i + G_sigma_x_i*G_sigma_x_i );
                    sigma_x_i = std::sqrt( F_sigma_x_i*F_sigma_x_i + G_sigma_x_i*G_sigma_x_i );
                    sigma_f_i = std::sqrt( F_sigma_f_i*F_sigma_f_i + G_sigma_f_i*G_sigma_f_i );
                }else{
                    //sigma_x_i = 0.5*(std::abs(F_sigma_x_i) + std::abs(G_sigma_x_i));
                    sigma_x_i = (std::abs(F_sigma_x_i) + std::abs(G_sigma_x_i));
                    sigma_f_i = std::abs(F_sigma_f_i) + std::abs(G_sigma_f_i);
                }
                out.push_back(x_i, sigma_x_i, f_i, sigma_f_i, inhibit_sort);
                ++f_it;
                ++g_it;
                continue;
            }else if( F_x_i < G_x_i ){
                //Check if we can avoid a costly interpolation.
                if(!isininc(g_lowest_x, F_x_i, g_highest_x)){
                    out.push_back(*f_it, inhibit_sort);
                }else{
                    const auto GG = g.Interpolate_Linearly(F_x_i);
                    const T GG_x_i       = GG[0];
                    //const T GG_sigma_x_i = GG[1]; //Should always be zero.
                    const T GG_f_i       = GG[2];
                    const T GG_sigma_f_i = GG[3];
                    
                    const T x_i = F_x_i;
                    const T f_i = F_f_i + GG_f_i;
                    const T sigma_x_i = F_sigma_x_i;
                    T sigma_f_i;

                    if(out.uncertainties_known_to_be_independent_and_random){
                        sigma_f_i = std::sqrt(F_sigma_f_i*F_sigma_f_i + GG_sigma_f_i*GG_sigma_f_i);
                    }else{
                        sigma_f_i = std::abs(F_sigma_f_i) + std::abs(GG_sigma_f_i);
                    }
                    out.push_back(x_i, sigma_x_i, f_i, sigma_f_i, inhibit_sort);
                }
                ++f_it;
                continue;
            }else{
                //Check if we can avoid a costly interpolation.
                if(!isininc(f_lowest_x, G_x_i, f_highest_x)){
                    out.push_back(*g_it, inhibit_sort);
                }else{
                    const auto FF = this->Interpolate_Linearly(G_x_i);
                    const T FF_x_i       = FF[0];
                    //const T FF_sigma_x_i = FF[1]; //Should always be zero.
                    const T FF_f_i       = FF[2];
                    const T FF_sigma_f_i = FF[3];
                    
                    const T x_i = G_x_i;
                    const T f_i = G_f_i + FF_f_i;
                    const T sigma_x_i = G_sigma_x_i;
                    T sigma_f_i;
                    
                    if(out.uncertainties_known_to_be_independent_and_random){
                        sigma_f_i = std::sqrt(G_sigma_f_i*G_sigma_f_i + FF_sigma_f_i*FF_sigma_f_i);
                    }else{
                        sigma_f_i = std::abs(G_sigma_f_i) + std::abs(FF_sigma_f_i);
                    }
                    out.push_back(x_i, sigma_x_i, f_i, sigma_f_i, inhibit_sort);
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
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Sum_With(const samples_1D<float > &in) const;
    template samples_1D<double> samples_1D<double>::Sum_With(const samples_1D<double> &in) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Subtract(const samples_1D<T> &B) const {
    return this->Sum_With( B.Multiply_With( (T)(-1) ) );
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Subtract(const samples_1D<float > &in) const;
    template samples_1D<double> samples_1D<double>::Subtract(const samples_1D<double> &in) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>  samples_1D<T> samples_1D<T>::Purge_Nonfinite_Samples(void) const {
    //Using the "erase and remove" idiom.
    auto out = *this;
    out.samples.erase( std::remove_if( out.samples.begin(), out.samples.end(), 
                                       [](const std::array<T,4> &s) -> bool { return !std::isfinite(s[2]); } ), 
                       out.samples.end() );
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Purge_Nonfinite_Samples(void) const;
    template samples_1D<double> samples_1D<double>::Purge_Nonfinite_Samples(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>
template <class Function> std::array<T,2> samples_1D<T>::Integrate_Generic(const samples_1D<T> &g, Function F) const {
    //Numerically computes the generic integral: [\int_{-inf}^{inf} F(f(x), g(x), x) dx] where f(x) (i.e., f_x) are the 
    // data in *this and g(x) (i.e., g_x) are the data in a passed-in samples_1D. Often, the functional F() will simply be 
    // f(x)*g(x) = f_x*g_x so that we compute the overlap of two numeric distributions [\int_{-inf}^{inf} f(x) * g(x) dx].
    //
    // NOTE: This routine, as it stands, involves linear interpolation. A more robust implementation may do something
    //       fancier (though I don't think it is unreasonable to assume datum are connected with line segments).
    //
    // NOTE: Experiments indicate that assuming independent, random, normal uncertainties produces integration uncertainties 
    //       which are more sensitive to the integration procedure. Using 'safer' no-assumptions is more stable. In either
    //       case, I'm not certain if the approach taken here is valid. (I would suggest researching it if you have time or
    //       questions.) I think it should at least be reasonable -- I've expressed the integration as a single equation and
    //       used the standard propagation of uncertainties trick of computing partial derivatives and summing either in 
    //       quadrature or in absolute value.
    //
    // NOTE: The reported uncertainty is a ROUGH estimate of the uncertainty in the resulting parameter due to sigma_x_i
    //       and sigma_f_i. It does NOT have anything to do with numerical uncertainties arising from the blunt treatment
    //       herein. If you need such an estimate, you could try sampling an analytic function and comparing numerical and
    //       exact integrals. 
    //
    // NOTE: This function also acts as a generic backend for other numerical integration routines. Be aware if you plan to 
    //       break the api.
    //
    // NOTE: The "Function F" template needs only to define [T operator(T f_x, T g_x, T x)]. A typical function would be the 
    //       simple overlap integration where F = f_x*g_x. More exotic integrals will involve a kernel of some sort, maybe an
    //       exponential so that [F = exp(-A*x)*f_x*g_x].
    //
    // NOTE: This routine performs a numerical approximation which might be poor if: 
    //         1. your dx's change size rapidly, 
    //         2. there is a lot of cancellation and a loss of precision will be significant,
    //         3. the f_x and g_x consistently over- or under-represent the true distributions (i.e., if they are totally 
    //            concave or convex and you're not being careful), etc.
    //
    //       In such case, it might be worthwhile to analytically derive the integrals assuming f_x and g_x are connected by
    //       line segments. This function cannot handle such an approach. See the other analytical Integrate_...() functions
    //       which might help. If in doubt, sub-sample at finer resolution (if you can) to see how much loss is happening.
    //
    // NOTE: Uncertainties are propagated using an imprecise numerical scheme. This involves computing numerical derivatives
    //       which are tricky because F is a generic function! The usual numerical stability caveats apply.
    //
    // History:  
    //  v. 0.0 - Simple, inefficient, imprecise midpoint rule w/ interpolation of f and g. Hard-coded interpolation points
    //           separated by an equal amount of distance. (This is basically the worst implementation I could muster.)
    //  V. 0.1 - Modified to compute uncertainties. Adaptive integration + differentiation schemes would sure be nice.
    //           Instead of hard-coding, I sample with 10x the number of datum. Not much better, but surely less insane.
    //           Also much slower. Maybe this will be an impetus to improve it?
    //
    const bool AssumeNormal = ( this->uncertainties_known_to_be_independent_and_random
                             && g.uncertainties_known_to_be_independent_and_random );

    //Step 0 - Sanity checks.
    if(this->size() < 2) FUNCERR("Unable to integrate f - there are less than 2 samples");
    if(g.size() < 2) FUNCERR("Unable to integrate g - there are less than 2 samples");

    //Step 1 - Establish actual endpoints.
    const T f_lowest_x  = this->samples.front()[0];
    const T f_highest_x = this->samples.back()[0];
    const T g_lowest_x  = g.samples.front()[0];
    const T g_highest_x = g.samples.back()[0]; 

    const T lowest_x = (f_lowest_x < g_lowest_x) ? f_lowest_x : g_lowest_x;
    const T highest_x = (f_highest_x > g_highest_x) ? f_highest_x : g_highest_x;

    //Step 2 - Given the bounds, we integrate by interpolation.
    const T N = static_cast<T>(this->size() + g.size());
    const T dx = (highest_x-lowest_x)/((T)(10)*N);
    T res = (T)(0);
    T sigma = (T)(0);
    for(auto x = lowest_x;  x < (highest_x-(T)(0.5)*dx); x += dx){
        const T x_low = x;
        const T x_mid = x + (T)(0.5)*dx;
        const T x_top = x + dx;

        const auto f_at_mid = this->Interpolate_Linearly(x_mid);
        const auto g_at_mid = g.Interpolate_Linearly(x_mid);
        if((f_at_mid[1] != (T)(0)) || (g_at_mid[1] != (T)(0))){
            FUNCERR("This routine assumes all sigma_x_i uncertainty has been converted to "
                    "sigma_f_i uncertainty during interpolation. Cannot continue");
        }

        //The small area element for the integral (midpoint rule).
        const T F_at_mid = F(f_at_mid[2], g_at_mid[2], x_mid);
        res += dx*F_at_mid;

        //The contribution to the uncertainty from the small area element dA. The sigma_x_i uncertainty will be converted
        // to sigma_f_i uncertainty during interpolation. If it is not, we will need to re-write this routine.
        //
        // This is where we need to compute a numerical derivative. It is with respect to the function F and not wrt to 
        // the data. Therefore, we are probably safe to go small (assuming the function F is analytic and smooth).
        const T df = (T)(1E-5); // A "small" element. This is an arbitrary choice and should be scaled to the scale of f_i.
        const T dg = (T)(1E-5); // A "small" element. This is an arbitrary choice and should be scaled to the scale of g_i.
        const T dFdf = (F(f_at_mid[2] + df, g_at_mid[2], x_mid) - F_at_mid)/df;
        const T dFdg = (F(f_at_mid[2], g_at_mid[2] + dg, x_mid) - F_at_mid)/dg;

        if(AssumeNormal){
            sigma += (dFdf*f_at_mid[3])*(dFdf*f_at_mid[3]);  //Uncertainty from f.
            sigma += (dFdg*g_at_mid[3])*(dFdg*g_at_mid[3]);  //Uncertainty from g.
        }else{
            sigma += std::abs(dFdf*f_at_mid[3]);  //Uncertainty from f.
            sigma += std::abs(dFdg*g_at_mid[3]);  //Uncertainty from g.
        }
    }
    if(AssumeNormal){
        sigma = dx*std::sqrt(sigma);
    }else{
        sigma = dx*std::abs(sigma);
    }
    return { res, sigma };
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Integrate_Generic(const samples_1D<float > &g, std::function< float (float , float , float )> F) const;
    template std::array<double,2> samples_1D<double>::Integrate_Generic(const samples_1D<double> &g, std::function< double(double, double, double)> F) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>   std::array<T,2> samples_1D<T>::Integrate_Overlap(const samples_1D<T> &g) const {
    auto F = [](T f_x, T g_x, T) -> T { return f_x * g_x; };
    //return Integrate_Generic<T>< std::function< T (T, T, T)> >(g, F); //Does not compile.
    //return this->template Integrate_Generic<decltype(F)>(g, F);  //Works, but weird syntax!
    return this->Integrate_Generic<decltype(F)>(g, F); //Works.
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Integrate_Overlap(const samples_1D<float > &g) const;
    template std::array<double,2> samples_1D<double>::Integrate_Overlap(const samples_1D<double> &g) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>   std::array<T,2> samples_1D<T>::Integrate_Over_Kernel_unit() const {
    //Computes the integral: [\int_{-inf}^{+inf} f(x) dx] where f_x are the data in *this assuming f_x are
    // connected by straight line segments. A numerical evaluation is performed using a symbolically-derived 
    // pre-computed scheme. There will be no losses in precision due to use of, say, the midpoint scheme. However, 
    // the assumption that f_x are connected by line segments probably will introduce some errors. This error has 
    // the benefit of being easily quantifiable if you know the true distribution. One bonus of this routine is 
    // that it does not need interpolation between f_x points (except possibly at the endpoints) and may thus be 
    // faster than a totally-numerical evaluation.
    //
    // NOTE: This routine, as it stands, involves linear interpolation. A more robust implementation may do something
    //       fancier (though I don't think it is unreasonable to assume datum are connected with line segments).
    //

    //Step 0 - Sanity checks.
    if(this->empty()){ 
        //If the distribution is everywhere zero, then we know the integral is zero, but we don't know anything about
        //  how good our estimate is so the sigma_f_i uncertainty is infinite.
        return { (T)(0), std::numeric_limits<T>::infinity() };
    }else if(this->size() == 1){
        //FUNCWARN("Unable to interpolate f - there is only a single sample.

        //Since there is a single point, with no effective width and no way to determine the true strength of an infinite
        // pulse numerically, the integral is zero. We know basically nothing about the certainty of this result so the
        // sigma_f_i uncertainty is infinite. (This is basically an infinitely-narrow, finite-amplitude delta function.)
        return { (T)(0), std::numeric_limits<T>::infinity() };
    }

    //Step over adjacent data pairs, accumulating the contribution from each range. 
    T res = (T)(0);
    T sigma = (T)(0);
    auto itL = this->samples.begin();
    auto itR = std::next(itL);
    for( ; (itR != this->samples.end()); ++itL, ++itR){

        //Try simplify some notation.
        const T XL  = (*itL)[0];
        const T dXL = (*itL)[1];
        const T YL  = (*itL)[2];
        const T dYL = (*itL)[3];
        
        const T XR  = (*itR)[0];
        const T dXR = (*itR)[1];
        const T YR  = (*itR)[2];
        const T dYR = (*itR)[3];

        const T dX  = XR - XL;
        const T dY  = YR - YL;
        const T m   = dY / dX; //Used as a guard here.

        //Ensure the dx, dy between points is not going to be problematic.
        if(std::abs(dX) == (T)(0)){
            //No contribution -- no width. And it doesn't matter if dy is non-zero here because we are piecewise
            // integrating. We never 'encounter' the discontinuity!
            continue;
        }else if(!std::isfinite(m)){
            FUNCWARN("Encountered difficulty computing slope of line segment. Integral contains infinite contributions");
            return { std::numeric_limits<T>::infinity(),  std::numeric_limits<T>::infinity() };
        }

        //Now compute the integral over the current line segment. Ensure it isn't borked due to numerical issues.
        const T F = ((T)(0.5)*YR + (T)(0.5)*YL)*dX;

        if(!std::isfinite(F)){
            //Ideally we would FUNCERR() or report the issue with an exception and let the user deal with it.
            // The latter would be preferred because this routine may be used in situations where INF's are 
            // acceptable, such as function fitting with unbounded parameters. For now we'll be sloppy.
            FUNCWARN("Integral contains infinite contributions. Are the datum finite?");
            return { std::numeric_limits<T>::infinity(),  std::numeric_limits<T>::infinity() };
        }
        res += F;

        //Work out the pieces needed for uncertainty propagation. Just need to find partial derivatives.
        const T dFdYR = (T)(0.5)*dX;
        const T dFdYL = (T)(0.5)*dX;
        const T dFdXR = (T)(0.5)*(YR + YL);
        const T dFdXL = -(T)(0.5)*(YR + YL);

        if(this->uncertainties_known_to_be_independent_and_random){
            sigma += (dFdYR * dYR)*(dFdYR * dYR);
            sigma += (dFdYL * dYL)*(dFdYL * dYL);
            sigma += (dFdXR * dXR)*(dFdXR * dXR);
            sigma += (dFdXL * dXL)*(dFdXL * dXL);
        }else{
            sigma += std::abs(dFdYR * dYR);
            sigma += std::abs(dFdYL * dYL);
            sigma += std::abs(dFdXR * dXR);
            sigma += std::abs(dFdXL * dXL);
        }
    }

    if(this->uncertainties_known_to_be_independent_and_random){
        sigma = std::sqrt(sigma);
    }
    return { res, sigma };
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Integrate_Over_Kernel_unit() const;
    template std::array<double,2> samples_1D<double>::Integrate_Over_Kernel_unit() const;
#endif
//--------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
template <class T>   std::array<T,2> samples_1D<T>::Integrate_Over_Kernel_unit(T xmin, T xmax) const {
    //Computes the integral: [\int_{xmin}^{xmax} f(x) dx] where f_x are the data in *this assuming f_x are
    // connected by straight line segments. A numerical evaluation is performed using a symbolically-derived 
    // pre-computed scheme. There will be no losses in precision due to use of, say, the midpoint scheme. However, 
    // the assumption that f_x are connected by line segments probably will introduce some errors. This error has 
    // the benefit of being easily quantifiable if you know the true distribution. One bonus of this routine is 
    // that it does not need interpolation between f_x points (except possibly at the endpoints) and may thus be 
    // faster than a totally-numerical evaluation.
    //
    // NOTE: This routine, as it stands, involves linear interpolation. A more robust implementation may do something
    //       fancier (though I don't think it is unreasonable to assume datum are connected with line segments).
    //
    // NOTE: Feel free to add more routines with other kernels. Remember that integration is linear and that you 
    //       may be able to combine results of various kernels to get what you need. Consider the totally numeric 
    //       approach if your kernel is nasty (but be aware of the inherent losses of the numerical approach).
    //
    // NOTE: xmin and xmax are integration bounds. If they extend outside the domain of f_x the contribution of 
    //       the out-of-domain parts will be zero. In other words, the distribution f_x is assumed to be zero 
    //       outside of the explicitly stated domain. This may be inconvenient in specific situations (e.g., an 
    //       f_x that remains non-zero over the entire range of real numbers), but is the only sane default.
    //

    //Step 0 - Sanity checks.
    if(this->empty()){ 
        //If the distribution is everywhere zero, then we know the integral is zero, but we don't know anything about
        //  how good our estimate is so the sigma_f_i uncertainty is infinite.
        return { (T)(0), std::numeric_limits<T>::infinity() };
    }else if(this->size() == 1){
        //FUNCWARN("Unable to interpolate f - there is only a single sample.

        //Since there is a single point, with no effective width and no way to determine the true strength of an infinite
        // pulse numerically, the integral is zero. We know basically nothing about the certainty of this result so the
        // sigma_f_i uncertainty is infinite. (This is basically an infinitely-narrow, finite-amplitude delta function.)
        return { (T)(0), std::numeric_limits<T>::infinity() };
        
    }

    // The user supplied the integration bounds backward.
    if((xmax - xmin) < (T)(0)){
        // The user supplied the integration bounds backwards. Swap them, invert the result, and return.
        auto ret = this->Integrate_Over_Kernel_unit(xmax, xmin);
        ret[0] *= (T)(-1);
        return ret;
    }
    if(std::abs(xmax - xmin) == (T)(0)) return { (T)(0), (T)(0) };

    //Step 1 - Establish actual data (f_x) xmin and xmax.
    const T f_low_x  = this->samples.front()[0];
    const T f_high_x = this->samples.back()[0];

    const T non_trivial_left_bound  = (xmin < f_low_x)  ? f_low_x  : xmin;
    const T non_trivial_right_bound = (xmax > f_high_x) ? f_high_x : xmax;

    //Step 2 - Step over adjacent data pairs, accumulating the contribution from each range. If the integration bounds 
    // are within the range, perform linear interpolation at the boundar(y|ies).
    //
    // We will go from left (low x) to right (high x) and STOP when the right boundary is reached. In normal use this 
    // will be OK, but it will fail if the distribution is a path that winds back and forth.
    T res = (T)(0);
    T sigma = (T)(0);
    auto it1 = this->samples.begin();
    auto it2 = std::next(it1);
    bool reached_right_boundary = false;
    std::array<T,4> actual_L, actual_R;
    for( ; (it2 != this->samples.end()) && !reached_right_boundary; ++it1, ++it2){
        //Ensure we are in the region which is to be integrated.
        if( !isininc(non_trivial_left_bound, (*it1)[0], non_trivial_right_bound)
        &&  !isininc(non_trivial_left_bound, (*it2)[0], non_trivial_right_bound) ){
            continue;
        }

        //Get the actual bounds for this range. They might be shortened due to the user-specified
        // integration range. Also set the early exit flag if needed.
        if((*it1)[0] < non_trivial_left_bound){
            actual_L = this->Interpolate_Linearly(non_trivial_left_bound);
        }else{
            actual_L = (*it1); //Will probably have non-zero sigma_x_i !
        }
        if((*it2)[0] > non_trivial_right_bound){
            actual_R = this->Interpolate_Linearly(non_trivial_right_bound);
            reached_right_boundary = true;
        }else{
            actual_R = (*it2); //Will probably have non-zero sigma_x_i !
        }

        //Try simplify some notation.
        const T XL  = actual_L[0];
        const T dXL = actual_L[1];
        const T YL  = actual_L[2];
        const T dYL = actual_L[3];
        
        const T XR  = actual_R[0];
        const T dXR = actual_R[1];
        const T YR  = actual_R[2];
        const T dYR = actual_R[3];

        const T dX  = XR - XL;
        const T dY  = YR - YL;
        const T m   = dY / dX; //Used as a guard here.

        //Ensure the dx, dy between points is not going to be problematic.
        if(std::abs(dX) == (T)(0)){
            //No contribution -- no width. And it doesn't matter if dy is non-zero here because we are piecewise
            // integrating. We never 'encounter' the discontinuity!
            continue;
        }else if(!std::isfinite(m)){
            FUNCWARN("Encountered difficulty computing slope of line segment. Integral contains infinite contributions");
            return { std::numeric_limits<T>::infinity(),  std::numeric_limits<T>::infinity() };
        }

        //Now compute the integral over the current line segment. Ensure it isn't borked due to numerical issues.
        const T F = ((T)(0.5)*YR + (T)(0.5)*YL)*dX;

        if(!std::isfinite(F)){
            //Ideally we would FUNCERR() or report the issue with an exception and let the user deal with it.
            // The latter would be preferred because this routine may be used in situations where INF's are 
            // acceptable, such as function fitting with unbounded parameters. For now we'll be sloppy.
            FUNCWARN("Integral contains infinite contributions. Are the datum finite?");
            return { std::numeric_limits<T>::infinity(),  std::numeric_limits<T>::infinity() };
        }
        res += F;

        //Work out the pieces needed for uncertainty propagation. Just need to find partial derivatives.
        const T dFdYR = (T)(0.5)*dX;
        const T dFdYL = (T)(0.5)*dX;
        const T dFdXR = (T)(0.5)*(YR + YL);
        const T dFdXL = -(T)(0.5)*(YR + YL);

        if(this->uncertainties_known_to_be_independent_and_random){
            sigma += (dFdYR * dYR)*(dFdYR * dYR);
            sigma += (dFdYL * dYL)*(dFdYL * dYL);
            sigma += (dFdXR * dXR)*(dFdXR * dXR);
            sigma += (dFdXL * dXL)*(dFdXL * dXL);
        }else{
            sigma += std::abs(dFdYR * dYR);
            sigma += std::abs(dFdYL * dYL);
            sigma += std::abs(dFdXR * dXR);
            sigma += std::abs(dFdXL * dXL);
        }
    }

    if(this->uncertainties_known_to_be_independent_and_random){
        sigma = std::sqrt(sigma);
    }
    return { res, sigma };
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Integrate_Over_Kernel_unit(float  xmin, float  xmax) const;
    template std::array<double,2> samples_1D<double>::Integrate_Over_Kernel_unit(double xmin, double xmax) const;
#endif
//--------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,2> samples_1D<T>::Integrate_Over_Kernel_exp(T xmin, T xmax, std::array<T,2> inA, std::array<T,2> inx0) const {
    //Compute the integral: [\int_{xmin}^{xmax} exp(A*(x+x0)) * f_x dx] where f_x are the data in *this assuming f_x 
    // are connected by straight line segments. A numerical evaluation is performed using a symbolically-derived 
    // pre-computed scheme. There will be no losses in precision due to use of, say, the midpoint scheme. However, 
    // the assumption that f_x are connected by line segments probably will introduce some errors. This error has 
    // the benefit of being easily quantifiable if you know the true distribution. One bonus of this routine is 
    // that it does not need interpolation between f_x points (except possibly at the endpoints) and may thus be 
    // faster than a totally-numerical evaluation.
    //
    // NOTE: This routine, as it stands, involves linear interpolation. A more robust implementation may do something
    //       fancier (though I don't think it is unreasonable to assume datum are connected with line segments).
    //
    // NOTE: Feel free to add more routines with other kernels. Remember that integration is linear and that you 
    //       may be able to combine results of various kernels to get what you need. Consider the totally numeric 
    //       approach if your kernel is nasty (but be aware of the inherent losses of the numerical approach).
    //
    // NOTE: xmin and xmax are integration bounds. If they extend outside the domain of f_x the contribution of 
    //       the out-of-domain parts will be zero. In other words, the distribution f_x is assumed to be zero 
    //       outside of the explicitly stated domain. This may be inconvenient in specific situations (e.g., an 
    //       f_x that remains non-zero over the entire range of real numbers), but is the only sane default.
    //

    //Step 0 - Sanity checks.
    if(this->empty()){ 
        //If the distribution is everywhere zero, then we know the integral is zero, but we don't know anything about
        //  how good our estimate is so the sigma_f_i uncertainty is infinite.
        return { (T)(0), std::numeric_limits<T>::infinity() };
    }else if(this->size() == 1){
        //FUNCWARN("Unable to interpolate f - there is only a single sample.

        //Since there is a single point, with no effective width and no way to determine the true strength of an infinite
        // pulse numerically, the integral is zero. We know basically nothing about the certainty of this result so the
        // sigma_f_i uncertainty is infinite.
        return { (T)(0), std::numeric_limits<T>::infinity() };
        
    }
    //if(this->size() < 2) FUNCERR("Unable to interpolate f - there are less than 2 samples");

    // The user supplied the integration bounds backward.
    if((xmax - xmin) < (T)(0)){
        // The user supplied the integration bounds backwards. Swap them, invert the result, and return.
        auto ret = this->Integrate_Over_Kernel_exp(xmax, xmin, inA, inx0);
        ret[0] *= (T)(-1);
        return ret;
    }
    if(std::abs(xmax - xmin) == (T)(0)) return { (T)(0), (T)(0) };

    //Step 1 - Establish actual data (f_x) xmin and xmax.
    const T f_low_x  = this->samples.front()[0];
    const T f_high_x = this->samples.back()[0];

    const T non_trivial_left_bound  = (xmin < f_low_x)  ? f_low_x  : xmin;
    const T non_trivial_right_bound = (xmax > f_high_x) ? f_high_x : xmax;

    //Step 2 - Step over adjacent data pairs, accumulating the contribution from each range. If the integration bounds 
    // are within the range, perform linear interpolation at the boundar(y|ies).
    //
    // We will go from left (low x) to right (high x) and STOP when the right boundary is reached. In normal use this 
    // will be OK, but it will fail if the distribution is a path that winds back and forth.
    T res = (T)(0);
    T sigma = (T)(0);
    auto it1 = this->samples.begin();
    auto it2 = std::next(it1);
    bool reached_right_boundary = false;
    std::array<T,4> actual_L, actual_R;
    for( ; (it2 != this->samples.end()) && !reached_right_boundary; ++it1, ++it2){
        //Ensure we are in the region which is to be integrated.
        if( !isininc(non_trivial_left_bound, (*it1)[0], non_trivial_right_bound)
        &&  !isininc(non_trivial_left_bound, (*it2)[0], non_trivial_right_bound) ){
            continue;
        }

        //Get the actual bounds for this range. They might be shortened due to the user-specified
        // integration range. Also set the early exit flag if needed.
        if((*it1)[0] < non_trivial_left_bound){
            actual_L = this->Interpolate_Linearly(non_trivial_left_bound);
        }else{
            actual_L = (*it1); //Will probably have non-zero sigma_x_i !
        }
        if((*it2)[0] > non_trivial_right_bound){
            actual_R = this->Interpolate_Linearly(non_trivial_right_bound);
            reached_right_boundary = true;
        }else{
            actual_R = (*it2); //Will probably have non-zero sigma_x_i !
        }

        //Try simplify some notation.
        const T XL  = actual_L[0];
        const T dXL = actual_L[1];
        const T YL  = actual_L[2];
        const T dYL = actual_L[3];
        
        const T XR  = actual_R[0];
        const T dXR = actual_R[1];
        const T YR  = actual_R[2];
        const T dYR = actual_R[3];

        const T A   = inA[0];
        const T dA  = inA[1];
        const T X0  = inx0[0];
        const T dX0 = inx0[1];

        const T dY  = YR - YL;
        const T dX  = XR - XL;
        const T m   = dY / dX; //Slope.

        //Ensure the dx, dy between points is not going to be problematic.
        if(std::abs(dX) == (T)(0)){
            //No contribution -- no width. And it doesn't matter if dy is non-zero here because we are piecewise
            // integrating. We never 'encounter' the discontinuity!
            continue;
        }else if(!std::isfinite(m)){
            FUNCWARN("Encountered difficulty computing slope of line segment. Integral contains infinite contributions");
            return { std::numeric_limits<T>::infinity(),  std::numeric_limits<T>::infinity() };
        }

        //Now use the CAS-derived result over the current line segment. Ensure it isn't borked due to numerical issues.
        const T F = ((YR*A - m)*std::exp(A*(XR + X0)) - (YL*A - m)*std::exp(A*(XL + X0)))/(A*A); 

        if(!std::isfinite(F)){
            //Ideally we would FUNCERR() or report the issue with an exception and let the user deal with it.
            // The latter would be preferred because this routine may be used in situations where INF's are 
            // acceptable, such as function fitting with unbounded parameters. For now we'll be sloppy.
            FUNCWARN("Integral contains infinite contributions. Perhaps the 'A' parameter is overflowing std::exp()?");
            return { std::numeric_limits<T>::infinity(),  std::numeric_limits<T>::infinity() };
        }
        res += F;

        //Work out the pieces needed for uncertainty propagation. Just find partial derivatives using a CAS.
        // E.g., using Maxima:
        //    
        //    (%i1) m:(YR-YL)/(XR-XL);
        //    (%i2) F: ((YR*A-m)*exp(A*(XR+X0)) - (YL*A-m)*exp(A*(XL+X0)))/(A*A);
        //    (%i16) dFdYR: diff(F,YR);
        //    (%i19) dFdYL: diff(F,YL);
        //    (%i20) dFdXR: diff(F,XR);
        //    (%i21) dFdXL: diff(F,XL);
        //    (%i26) dFdA:  diff(F,A);
        //    (%i27) dFdX0: diff(F,X0);
        //    
        const T dFdYR = ((A-(T)(1)/(XR-XL))*std::exp(A*(XR+X0)) 
                      + std::exp(A*(XL+X0))/(XR-XL))/(A*A);
        const T dFdYL = (std::exp(A*(XR+X0))/(XR-XL)
                      - std::exp(A*(XL+X0))*((T)(1)/(XR-XL)+A))/(A*A);
        const T dFdXR = (A*std::exp(A*(XR+X0))*(A*YR-m) 
                      + std::exp(A*(XR+X0))*(YR-YL)/std::pow(XR-XL,2)
                      - std::exp(A*(XL+X0))*(YR-YL)/std::pow(XR-XL,2))/(A*A);
        const T dFdXL = (-A*std::exp(A*(XL+X0))*(A*YL-m)
                      - std::exp(A*(XR+X0))*(YR-YL)/std::pow(XR-XL,2)
                      + std::exp(A*(XL+X0))*(YR-YL)/std::pow(XR-XL,2))/(A*A);
        const T dFdA  = ((XR+X0)*std::exp(A*(XR+X0))*(A*YR-m)
                      - (XL+X0)*std::exp(A*(XL+X0))*(A*YL-m)
                      + std::exp(A*(XR+X0))*YR 
                      - std::exp(A*(XL+X0))*YL)/(A*A)
                      - (T)(2)*(std::exp(A*(XR+X0))*(A*YR-m)
                      - std::exp(A*(XL+X0))*(A*YL-m))/(A*A*A);
        const T dFdX0 = (A*std::exp(A*(XR+X0))*(A*YR-m)
                      -  A*std::exp(A*(XL+X0))*(A*YL-m))/(A*A);
        if(this->uncertainties_known_to_be_independent_and_random){
            sigma += (dFdYR * dYR)*(dFdYR * dYR);
            sigma += (dFdYL * dYL)*(dFdYL * dYL);
            sigma += (dFdXR * dXR)*(dFdXR * dXR);
            sigma += (dFdXL * dXL)*(dFdXL * dXL);
            sigma += (dFdA  * dA )*(dFdA  * dA );
            sigma += (dFdX0 * dX0)*(dFdX0 * dX0);
        }else{
            sigma += std::abs(dFdYR * dYR);
            sigma += std::abs(dFdYL * dYL);
            sigma += std::abs(dFdXR * dXR);
            sigma += std::abs(dFdXL * dXL);
            sigma += std::abs(dFdA  * dA );
            sigma += std::abs(dFdX0 * dX0);
        }
    }

    if(this->uncertainties_known_to_be_independent_and_random){
        sigma = std::sqrt(sigma);
    }
    return { res, sigma };
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,2> samples_1D<float >::Integrate_Over_Kernel_exp(float  xmin, float  xmax, std::array<float ,2> inA, std::array<float ,2> inx0) const;
    template std::array<double,2> samples_1D<double>::Integrate_Over_Kernel_exp(double xmin, double xmax, std::array<double,2> inA, std::array<double,2> inx0) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Aggregate_Equal_Sized_Bins_Weighted_Mean(long int N, bool explicitbins) const {
    //This routine groups a samples_1D into equal sized bins, finding the WEIGHTED MEAN of their f_i to produce a single 
    // datum with: x_i at the bin's mid-x-point, sigma_x_i=0, f_i= WEIGHTED MEAN of bin data, and sigma_f_i= std dev of
    // the weighted mean. This routine might be useful as a preparatory step for linear regressing noisy data, but this is
    // perhaps not statistically sound (the sigma_x_i are completely ignored). This routine is suitable for eliminating
    // datum which share an x_i.
    //
    // The datum in the output are WEIGHTED MEANS of the original data f_i and sigma_f_i destined for that bin. All data in
    // a bin are weighted the same along the x-coordinate, regardless of sigma_x_i or x_i. (Why? Because the bins have a 
    // definite x-position and width, so there is no way to reasonably propagate sigma_x_i or x_i).
    //
    // To recap: f_i and sigma_f_i are used to compute a WEIGHTED MEAN, but x_i and sigma_x_i are essentially IGNORED.
    //
    // NOTE: If you do not want to take into account sigma_f_i uncertainties, either strip them or set them all equal.
    //       (Say, to (T)(1) so that they have the same weight). If some datum have zero sigma_f_i uncertainty and others
    //       have non-zero sigma_f_i, the latter will be disregarded due to the infinitely-strong weighting of zero-
    //       sigma_f_i uncertainty.
    //
    // NOTE: If a bin contains no data, the bin f_i and sigma_f_i will be { (T)(0), (T)(0) }.
    //
    // NOTE: Take care to ensure your data is appropriate for binning. Generally: the smaller the bin, the more statistical
    //       sense binning makes.
    //
    // NOTE: In almost all situations you will want to use this routine, you should declare (and ensure) that the datum
    //       are normally-distributed. If the datum are strongly covariant then it is OK to make no assumptions, but the
    //       idea of binning such data loses much of its sense if the data is not normally distributed. Try avoid such a
    //       situation by carefully evaluating the normality of the bins before relying on this routine.
    //
    // NOTE: To ensure the bins are always sorted in the proper order (which is tough to guarantee, even with a stable
    //       sort, because of floating point errors in determining the bin widths), explicit bins are not exactly the 
    //       true bin width. They are slightly narrower at the base (98% of the bin width) and taper further at the top 
    //       (96% of the bin width). This is cosmetic, and is common for histograms. So if you are computing, say, overlap
    //       between histograms (which you shouldn't be doing with explicit bins...) expect a small discrepancy.
    //       
 
    //Step 0 - Sanity checks.
    if(!isininc((long int)(1),N,(long int)(this->size()))){
         FUNCERR("Asking to aggregate data into a nonsensical number of bins");
    }

    //Get the min/max x-value and bin spacing.
    const auto xmin  = this->samples.front()[0];
    const auto xmax  = this->samples.back()[0];
    const auto spanx = xmax - xmin; //Span of lowest to highest.
    const auto dx    = spanx/static_cast<T>(N);
    if(!std::isnormal(dx)){
        FUNCERR("Aggregating could not proceed: encountered problem with bin width");
    }

    //Cycle through the data, collecting the data for each bin.
    std::vector<std::vector<std::array<T,4>>> binnedraw(N);
    for(const auto &P : this->samples){
        //Determine which bin this datum should go in. We scale the datum's x-coordinate to the span of all coordinates.
        // Then, we multiply by the bin number to get the 'block' of x-coords it belongs to. Then we floor to an int and
        // handle the special upper point.
        //
        // The upper bin is the only both-endpoint-inclusive bin. (This is not a problem, because one of the bins *must*
        // be doubly-inclusive. Why? Because for N bins there are N+1 bin delimiters, and N+1 isn't divisible by N for
        // a general N.)
        const T x_i = P[0];
        const T distfl = std::abs(x_i - xmin); //Distance from lowest.
        const T sdistfl = distfl/spanx; //Scaled from [0,1].
        const T TbinN = std::floor(static_cast<T>(N)*sdistfl); //Bin number as type T.
        const size_t binN = static_cast<size_t>(TbinN);
        const size_t truebinN = (binN >= (size_t)(N)) ? (size_t)(N)-(size_t)(1) : binN;
        binnedraw[truebinN].push_back(P);
    }

    //Now the raw data is ready for aggregating. Cycle through each bin, performing the reduction.
    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
    out.metadata = this->metadata;
    const bool inhibit_sort = true;
    for(long int i = 0; i < N; ++i){
        const T binhw  = dx*(T)(0.48); //Bin half-width -- used to keep bins ordered upon sorting.
        const T basedx = dx*(T)(0.01); //dx from true bin L and R -- used to keep bins ordered upon sorting.
        const T xmid   = xmin + dx*(static_cast<T>(i) + (T)(0.5)); //Middle of the bin.
        const T xbtm   = xmid - binhw;
        const T xtop   = xmid + binhw;

        //Compute the weighted mean of the data. (This routine will handle any sigma_f_i uncertainties properly.)
        // If no data are present, just report the Weight_Mean_y() fallback (probably a mean of {(T)(0),inf}.)
        samples_1D<T> abin(binnedraw[i]);
        abin.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
        abin.metadata = this->metadata; //Needed? Most likely not, but better to be safe here.
        const std::array<T,2> binwmy = abin.Weighted_Mean_y();      

        if(!explicitbins){
            //If the user wants a pure resample, omit the bins.
            out.push_back({xmid, (T)(0), binwmy[0], binwmy[1] }, inhibit_sort);

        }else{ 
            //Otherwise, if displaying on screen (or similar) we explicitly show the bin edges.
            const T xbtm_base = xbtm - basedx;  //Just a tad wider than the top of the bin.
            const T xtop_base = xtop + basedx;
            out.push_back({xbtm_base, (T)(0),    (T)(0),    (T)(0)}, inhibit_sort);
            out.push_back({xbtm,      (T)(0), binwmy[0],    (T)(0)}, inhibit_sort);
            out.push_back({xmid,      (T)(0), binwmy[0], binwmy[1]}, inhibit_sort); //Center of bin, w/ sigma_f_i intact.
            out.push_back({xtop,      (T)(0), binwmy[0],    (T)(0)}, inhibit_sort);
            out.push_back({xtop_base, (T)(0),    (T)(0),    (T)(0)}, inhibit_sort);
        }
    }
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Aggregate_Equal_Sized_Bins_Weighted_Mean(long int N, bool showbins) const;
    template samples_1D<double> samples_1D<double>::Aggregate_Equal_Sized_Bins_Weighted_Mean(long int N, bool showbins) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Aggregate_Equal_Datum_Bins_Weighted_Mean(long int N) const {
    //This routine groups a samples_1D into bins of an equal number (N) of datum. The bin x_i and f_i are generated from 
    // the WEIGHTED MEANS of the bin data. This routine propagates sigma_x_i and sigma_f_i. They are propagated separately,
    // meaning that sigma_x_i and sigma_f_i have no influence on one another (just as x_i and f_i have no influence on each
    // other for the purposes of computing the WEIGHTED MEAN).
    //
    // I believe this routine to be more statistically sound than the ...Equal_Sized_Bins() variant because the x_i and
    // sigma_x_i are taken into account herein. Prefer to use this routine, especially on data with equal x_i separation
    // where this routine essentially has no downside compared to the ...Equal_Sized_Bins() variant.
    //
    // To recap: f_i and sigma_f_i are used to compute a WEIGHTED MEAN, and x_i and sigma_x_i are used to compute a
    // WEIGHTED MEAN, but the two have no influence on one another.
    //
    // NOTE: If you do not want to take into account sigma_f_i or sigma_x_i uncertainties, either strip them or set them
    //       all equal. (Say, to (T)(1) so that they have the same weight). If some datum have zero uncertainty while 
    //       others have non-zero uncertainty, the latter will be disregarded due to the infinitely-strong weighting of
    //       zero-uncertainty datum.
    //
    // NOTE: Take care to ensure your data is appropriate for binning. Generally: the smaller the bin, the more statistical
    //       sense binning makes.
    //
    // NOTE: All bins will contain at least a single point, but the bins obviously cannot always all be composed of the
    //       same number of datum. Therefore the bin with the largest x may have less datum than the rest. If you are 
    //       worried about this, try find a divisor before calling this routine.
    //
    // NOTE: If your data has lots of datum with identical x_i (i.e., at least 2*N of them) then it is possible that two
    //       outgoing bins will also exactly overlap. Be aware of this. This routine will run fine, but it can cause issues
    //       in other routines. The solution is to remove all datum with identical x_i prior to calling this routine.
    //
    // NOTE: In almost all situations you will want to use this routine, you should declare (and ensure) that the datum
    //       are normally-distributed. If the datum are strongly covariant then it is OK to make no assumptions, but the
    //       idea of binning such data loses much of its sense if the data is not normally distributed. Try avoid such a
    //       situation by carefully evaluating the normality of the bins before relying on this routine.
    //
 
    //Step 0 - Sanity checks.
    if(N < 1){
         FUNCERR("Asking to aggregate data with a nonsensical number of datum in each bin");
    }

    //Cycle through the data, collecting the data for each bin.
    std::vector<std::vector<std::array<T,4>>> binnedraw;
    std::vector<std::array<T,4>> shtl;
    for(const auto &P : this->samples){
        //Add element to the shuttle.
        shtl.push_back(P);

        //Check if the requisite number of datum has been collected for a complete bin.
        if(N == (long int)(shtl.size())){
            //If so, then a bin has been harvested. Dump the shuttle into the raw data bins and prep for next iteration.
            binnedraw.push_back(std::move(shtl));
            shtl = std::vector<std::array<T,4>>();
        }
    }
    if(!shtl.empty()) binnedraw.push_back(std::move(shtl)); //Final, possibly incomplete bin.

    //Now the raw data is ready for aggregating. Cycle through each bin, performing the reduction.
    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
    out.metadata = this->metadata;
    const bool inhibit_sort = true;
    for(auto const &rawbin : binnedraw){
        samples_1D<T> abin(rawbin);
        abin.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
        abin.metadata = this->metadata; //Probably not needed, but better to be safe here.
 
        //Compute the weighted mean of the data. (This routine will handle any uncertainties properly.)
        // If no data are present, just report the Weight_Mean_y() fallback (probably a mean of {(T)(0),inf}.)
        const std::array<T,2> binwmx = abin.Weighted_Mean_x();      
        const std::array<T,2> binwmy = abin.Weighted_Mean_y();

        //If the user wants a pure resample, omit the bins.
        out.push_back({ binwmx[0], binwmx[1], binwmy[0], binwmy[1] }, inhibit_sort);
    }
    out.stable_sort(); //In case the weighted mean somehow screws up the order.
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Aggregate_Equal_Datum_Bins_Weighted_Mean(long int N) const;
    template samples_1D<double> samples_1D<double>::Aggregate_Equal_Datum_Bins_Weighted_Mean(long int N) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Histogram_Equal_Sized_Bins(long int N, bool explicitbins) const {
    //This routine groups a samples_1D into equal sized bins, finding the SUM of their f_i to produce a single datum with
    // x_i at the bin's mid-x-point, sigma_x_i=0, f_i= SUM of bin data, and sigma_f_i= proper propagated uncertainty for 
    // the data sigma_f_i. This routine is useful for plotting a distribution of numbers, possibly with a sigma_f_i
    // weighting. 
    //
    // Often the sigma_f_i will be set to (T)(1) and then the height of all bins will be divided by the total number of
    // datum. This will generate a 'proper' histogram; each bin's height will be the fraction (# of bin datum)/(total # of
    // datum). Such a histogram can be used, for example, to inspect the normality of a distribution of numbers. (In this 
    // case the bin sigma_f_i have no meaning. But if each datum had a proper uncertainty then the bins would correctly
    // propagate the uncertainty due to summation over sigma_f_i in each bin.)
    //
    // NOTE: If a bin contains no data, the bin f_i and sigma_f_i will be { (T)(0), (T)(0) }.
    //
    // NOTE: To ensure the bins are always sorted in the proper order (which is tough to guarantee, even with a stable
    //       sort, because of floating point errors in determining the bin widths), explicit bins are not exactly the 
    //       true bin width. They are slightly narrower at the base (98% of the bin width) and taper further at the top 
    //       (96% of the bin width). This is cosmetic, and is common for histograms. So if you are computing, say, overlap
    //       between histograms (which you shouldn't be doing with explicit bins...) expect a small discrepancy.
    //       
 
    //Step 0 - Sanity checks.
    if(!isininc((long int)(1),N,(long int)(this->size()))){
         FUNCERR("Asking to histogram data into a nonsensical number of bins");
    }

    //Get the min/max x-value and bin spacing.
    const auto xmin  = this->samples.front()[0];
    const auto xmax  = this->samples.back()[0];
    const auto spanx = xmax - xmin; //Span of lowest to highest.
    const auto dx    = spanx/static_cast<T>(N);
    if(!std::isnormal(dx)){
        FUNCERR("Histogram generation could not proceed: encountered problem with bin width");
    }

    //Step 1 - Cycle through the data, collecting the data for each bin.
    std::vector<std::vector<std::array<T,4>>> binnedraw(N);
    for(const auto &P : this->samples){
        //Determine which bin this datum should go in. We scale the datum's x-coordinate to the span of all coordinates.
        // Then, we multiply by the bin number to get the 'block' of x-coords it belongs to. Then we floor to an int and
        // handle the special upper point.
        //
        // The upper bin is the only both-endpoint-inclusive bin. (This is not a problem, because one of the bins *must*
        // be doubly-inclusive. Why? Because for N bins there are N+1 bin delimiters, and N+1 isn't divisible by N for
        // a general N.)
        const T x_i = P[0];
        const T distfl = std::abs(x_i - xmin); //Distance from lowest.
        const T sdistfl = distfl/spanx; //Scaled from [0,1].
        const T TbinN = std::floor(static_cast<T>(N)*sdistfl); //Bin number as type T.
        const size_t binN = static_cast<size_t>(TbinN);
        const size_t truebinN = (binN >= (size_t)(N)) ? (size_t)(N)-(size_t)(1) : binN;
        binnedraw[truebinN].push_back(P);
    }

    //Step 2 - Now the raw data is ready to histogram. Cycle through each bin, performing the reduction.
    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
    out.metadata = this->metadata;
    const bool inhibit_sort = true;
    for(long int i = 0; i < N; ++i){
        const T binhw  = dx*(T)(0.48); //Bin half-width -- used to keep bins ordered upon sorting.
        const T basedx = dx*(T)(0.01); //dx from true bin L and R -- used to keep bins ordered upon sorting.
        const T xmid   = xmin + dx*(static_cast<T>(i) + (T)(0.5)); //Middle of the bin.
        const T xbtm   = xmid - binhw;
        const T xtop   = xmid + binhw;

        //Compute the sum of the data. (This routine will handle any sigma_f_i uncertainties properly.)
        // If no data are present, just report the Sum_y() fallback (probably a mean of {(T)(0),inf}.)
        samples_1D<T> abin(binnedraw[i]);
        abin.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
        abin.metadata = this->metadata; //Probably not needed, but better to be safe here.
        const std::array<T,2> binsumy = abin.Sum_y();

        if(!explicitbins){
            //If the user wants a pure resample, omit the bins.
            out.push_back({xmid, (T)(0), binsumy[0],  binsumy[1] }, inhibit_sort);

        }else{ 
            //Otherwise, if displaying on screen (or similar) we explicitly show the bin edges.
            const T xbtm_base = xbtm - basedx;  //Just a tad wider than the top of the bin.
            const T xtop_base = xtop + basedx;
            out.push_back({xbtm_base, (T)(0),     (T)(0),     (T)(0)}, inhibit_sort);
            out.push_back({xbtm,      (T)(0), binsumy[0],     (T)(0)}, inhibit_sort);
            out.push_back({xmid,      (T)(0), binsumy[0], binsumy[1]}, inhibit_sort); //Center of bin, w/ sigma_f_i intact.
            out.push_back({xtop,      (T)(0), binsumy[0],     (T)(0)}, inhibit_sort);
            out.push_back({xtop_base, (T)(0),     (T)(0),     (T)(0)}, inhibit_sort);
        }
    }
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Histogram_Equal_Sized_Bins(long int N, bool showbins) const;
    template samples_1D<double> samples_1D<double>::Histogram_Equal_Sized_Bins(long int N, bool showbins) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,4> samples_1D<T>::Spearmans_Rank_Correlation_Coefficient(bool *OK) const {
    //Computes a correlation coefficient suitable for judging correlation (without any underlying model!) when the data is 
    // monotonically increasing or decreasing. See:
    //   http://en.wikipedia.org/wiki/Spearman's_rank_correlation_coefficient#Definition_and_calculation
    // for more info. This routine (and Spearman's Rank Correlation Coefficient in general) IGNORES all uncertainties.
    //
    // Indeed, it would be difficult to ascribe an uncertainty to the result because ranking is used. I suspect the most
    // reliable approach would use Monte Carlo for a bootstrap-like estimation.
    //
    // Four numbers are returned:
    //     1. The coefficient itself (rho),
    //     2. The number of samples used (as a type T, not an integer),
    //     3. The z-value,
    //     4. The t-value.
    //
    // NOTE: To compute the P-value from the t-value, use DOF = N-2 where N is given as the second return value.
    //
    // NOTE: Don't rely on the z- or t-values computed here if N < 10. 
    //       From http://vassarstats.net/corr_rank.html:
    //       "Note that t is not a good approximation of the sampling distribution of rho when n is less than 10.
    //       For values of n less than 10, you should use the following table of critical values of rs, which shows 
    //       the values of + or —rs required for significance at the .05 level, for both a directional and a 
    //       non-directional test."
    //
    if(OK != nullptr) *OK = false;
    const auto ret_on_err = std::array<T,4>({(T)(-1),(T)(-1),(T)(-1),(T)(-1)}); 

    const samples_1D<T> ranked(this->Rank_x().Rank_y());
    const auto avgx = ranked.Mean_x()[0]; //Don't care about the uncertainty.
    const auto avgy = ranked.Mean_y()[0];

    T numer((T)(0)), denomA((T)(0)), denomB((T)(0));
    for(const auto &P : ranked.samples){
        numer  += (P[0] - avgx)*(P[2] - avgy);
        denomA += (P[0] - avgx)*(P[0] - avgx);
        denomB += (P[2] - avgy)*(P[2] - avgy);
    }
    const auto rho = numer/std::sqrt(denomA*denomB);
    if(!std::isfinite(rho) || (YGORABS(rho) > (T)(1))){
        if(OK == nullptr) FUNCERR("Found coefficient which was impossible or nan. Bailing");
        FUNCWARN("Found coefficient which was impossible or nan. Bailing");
        return ret_on_err;
    }
    const auto num = static_cast<T>(ranked.samples.size());

    if(ranked.size() < 3){
        if(OK == nullptr) FUNCERR("Unable to compute z-value - too little data");
        FUNCWARN("Unable to compute z-value - too little data");
        return ret_on_err;
    }
    const T zval = std::sqrt((num - 3.0)/1.060)*std::atanh(rho);

    if(ranked.size() < 2){
        if(OK == nullptr) FUNCERR("Unable to compute t-value - too little data");
        FUNCWARN("Unable to compute t-value - too little data");
        return ret_on_err;
    }
    const T tval_n = rho*std::sqrt(num - (T)(2));
    const T tval_d = std::sqrt((T)(1) - rho*rho);
    const T tval   = std::isfinite((T)(1)/tval_d) ? tval_n/tval_d : std::numeric_limits<T>::infinity();

    if(OK != nullptr) *OK = true;
    return {rho, num, zval, tval};
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,4> samples_1D<float >::Spearmans_Rank_Correlation_Coefficient(bool *OK) const;
    template std::array<double,4> samples_1D<double>::Spearmans_Rank_Correlation_Coefficient(bool *OK) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> std::array<T,3> samples_1D<T>::Compute_Sxy_Sxx_Syy(bool *OK) const {
    //Computes {Sxy, Sxx, Syy} which are used for linear regression and other procedures.
    if(OK != nullptr) *OK = false;
    const auto ret_on_err = std::array<T,3>({(T)(-1),(T)(-1),(T)(-1)});

    //Ensure the data is suitable.
    if(this->size() < 1){
        if(OK == nullptr) FUNCERR("Unable to calculate Sxy,Sxx,Syy with no data");
        FUNCWARN("Unable to calculate Sxy,Sxx,Syy with no data. Bailing");
        return ret_on_err;
    }

    const auto mean_x = this->Mean_x()[0]; //Disregard uncertainties.
    const auto mean_y = this->Mean_y()[0]; 
    T Sxx((T)(0)), Sxy((T)(0)), Syy((T)(0));
    for(const auto &P : this->samples){
        Sxy += (P[0] - mean_x)*(P[2] - mean_y);
        Sxx += (P[0] - mean_x)*(P[0] - mean_x);
        Syy += (P[2] - mean_y)*(P[2] - mean_y);
    }
    if(OK != nullptr) *OK = true;
    return {Sxy,Sxx,Syy};
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::array<float ,3> samples_1D<float >::Compute_Sxy_Sxx_Syy(bool *OK) const;
    template std::array<double,3> samples_1D<double>::Compute_Sxy_Sxx_Syy(bool *OK) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Moving_Average_Two_Sided_Driver(const std::vector<T> &weights) const {
    //Computes a generic two-sided weighted moving average or mean of the f_i using a local window and weights as provided. 
    // Ignores x_i spacing. This is a window convolution that effectively acts like a low-pass filter and is generally 
    // used to find the 'trend' of data. 
    //
    // NOTE: This routine is variously called "weighted, two-sided moving average", "centered weighted average", "sliding 
    //       window average", "windowed convolution", "linear moving averages", "seasonal trend remover", etc...
    //
    // NOTE: The weights MUST be normalized such that their sum is 1.0. This is not verified! Weights are defined as:
    //              weight[0] == a_o  
    //           and otherwise:
    //              weight[i] == a_i == a_{-i}.
    //       So the weights are assumed to be symmetric about the central weighting term. Weights CAN be negative, but must
    //       still sum to 1.0. Providing N weights implies a two-sided moving average in which each point is averaged with 
    //       the N + (N - 1) = 2*N-1 nearest points.
    //
    // NOTE: This routine completely ignores x_i spacing. It assumes a constant or nearly constant x_i spacing is present,
    //       so be aware. If you do NOT have constant x_i spacing, you should probably just consider convoluting with a
    //       normalized Gaussian or rectangular window or something. (Note: see YgorImage's pixel-wise Gaussian blur routine
    //       for a 2D implementation of a Gaussian convolution of this sort which is effectively a low-pass filter.)
    //
    // NOTE: This routine ignores uncertainties in sigma_x_i, and simply passes them through. The user's assumption about
    //       whether uncertainties are thought to be independent and random is passed on as-is too. If you're cautious then
    //       do not assume independent + random errors to begin with.
    //
    // NOTE: It is safe to apply this routine consecutively. One guy claims on his website (which made it to Wikipedia...)
    //       that applying three times might be best. But note that this claim was for a uniformly-weighted (non-Spencer)
    //       two-sided moving average.
    //
    // NOTE: There are MANY variants of moving averages -- this is just one I've seen around. See the book by Kendall
    //       called "Advanced statistics" Vol. 3, which covers many. The 15-point two-sided Spencer moving average has the
    //       nice property that it allows polynomials of order <=3 to pass through essentially as-is (up to numerics).
    //
    // NOTE: Because this routine requires points on either side of each datum, and we have to deal with two endpoints, a
    //       pragmatic choice is made to insert "phantom" points to the left and right of the data. Their f_i is assumed to
    //       be the nearest non-virtual f_i (i.e., the endpoints). Thus, the SEVEN endpoints should be treated as somewhat
    //       suspect. However, the computed uncertainty follows uncertainty propagation rules correctly (handling an annoying
    //       underestimation gotcha due to virtual endpoint extension) and the result should at least be consistent (even if
    //       the uncertainties flare up at the endpoints).
    //
    //       In the literature (Weighted Median Filters: A Tutorial Lin Yin):
    //
    //                      "The appending of the input signal is commonly performed by
    //                       replicating the outmost input samples as many times as needed. 
    //                       This appending strategy is referred to as the first and last 
    //                       values carry-on appending strategy, in the literature [29] ..."
    //
    // NOTE: Be aware if you plan to mix moving averages with harmonic or spectral analysis: a moving average can cause an
    //       "irregular oscillation" to mysteriously appear in the data. This is called the Slutzky-Yule effect. (This 
    //       should be obvious -- of course you're modifying the spectrum by filtering the data!)
    //
    samples_1D<T> out;
    out.metadata = this->metadata;

    //Not sure whether we can make assumptions about the nature of uncertainties after calling this routine. At the moment
    // sigma_f_i uncertainties are just passed through. So I'm gonna just roll with it and assume any assumptions remain
    // unmodified. If you're cautious then do not assume independent + random errors to begin with!
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;

    const auto sample_count = static_cast<long int>(this->size());
    if(weights.empty()) FUNCERR("No weights provided. Cannot continue");
    const auto weights_size = static_cast<long int>(weights.size());

    for(long int i = 0; i < sample_count; ++i){
        std::array<T,4> newdatum(this->samples[i]);
        newdatum[2] = newdatum[3] = (T)(0);
        T Luncert = (T)(0); //Used to catch faux uncertainties do to the "virtual" extension of the endpoints 
        T Runcert = (T)(0); // outward as needed. If normality is present, points near the ends will have 
                            // underestimated uncertainties unless we catch it!

        for(long int j = (1-weights_size); j < weights_size; ++j){
            const auto weight = weights[ YGORABS(j) ];
            const auto pm_indx = i + j;
            const auto indx = YGORABS(pm_indx);

            const auto realpoint = isininc(0,pm_indx,sample_count-1); //Is a real point (not virtual).
            const auto Lendpoint = (pm_indx <= 0);                    //inclusive to the L endpoint.
            const auto Rendpoint = (pm_indx >= (sample_count-1));     //inclusive to the R endpoint.

            long int bnd_index;
            if(realpoint){
                bnd_index = indx;
            }else if(Lendpoint){
                bnd_index = 0;
            }else if(Rendpoint){
                bnd_index = (sample_count-1);
            }

            //Compute the weighted contribution from this point, virtual or real.
            newdatum[2] += (this->samples[bnd_index][2]) * weight;

            //The uncertainties are where this gets hairy.
            if(this->uncertainties_known_to_be_independent_and_random){
                //If this is a virtual point, we have to handle the underestimation of the naive approach (of 
                // endpoint-virtual extension) by summing the total weighted contribution from all the virtual 
                // points before squaring and adding to the running total.
                //
                // This issue comes about because we sum weighted uncertainties in quadrature, and 
                //    x^2  !=  (0.5*x)^2  +  (0.5*x)^2
                // like the nicely additive case is:
                //    x    ==  x^2  +  x^2.
                if(Lendpoint){
                    Luncert += (this->samples[bnd_index][3]) * weight;
                }else if(Rendpoint){
                    Runcert += (this->samples[bnd_index][3]) * weight;
 
                //If this is a real point, then there is little issue. Just do the normal thing.
                }else if(realpoint){
                    newdatum[3] += std::pow((this->samples[bnd_index][3]) * weight, 2);
                }
            }else{
                //This is the nicely additive case; no worries or complications here.
                newdatum[3] += std::abs((this->samples[bnd_index][3]) * weight);
            }
        }
        if(this->uncertainties_known_to_be_independent_and_random){
            newdatum[3] = std::sqrt(newdatum[3] + std::pow(Luncert,2) + std::pow(Runcert,2));
        }
        out.push_back(newdatum);
    }
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Moving_Average_Two_Sided_Driver(const std::vector<float > &weights) const;
    template samples_1D<double> samples_1D<double>::Moving_Average_Two_Sided_Driver(const std::vector<double> &weights) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Moving_Average_Two_Sided_Spencers_15_point(void) const {
    //Computes a "Spencer's 15-point, two-sided, weighted moving average" or mean of the f_i. Ignores x_i spacing. This is 
    // a convolution that effectively acts like a low-pass filter and is generally used to find a the 'trend' of data. The 
    // "15-point" part means 7 points to the left, 7 to the right, and the central point are used for each average.
    //
    // NOTE: See this->Moving_Average_Two_Sided_Driver() function for notes and caveats.
    //
    // NOTE: There are MANY variants of moving averages -- this is just one I've seen around. See the book by Kendall
    //       called "Advanced statistics" Vol. 3, which covers many. The 15-point two-sided Spencer moving average has the
    //       nice property that it allows polynomials of order <=3 to pass through essentially as-is (up to numerics).
    //
    const std::vector<T> weights = { (T)(74)/(T)(320), (T)(67)/(T)(320), (T)(46)/(T)(320), (T)(21)/(T)(320),
                                     (T)( 3)/(T)(320), (T)(-5)/(T)(320), (T)(-6)/(T)(320), (T)(-3)/(T)(320) };
    return this->Moving_Average_Two_Sided_Driver(weights);
};
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Moving_Average_Two_Sided_Spencers_15_point(void) const;
    template samples_1D<double> samples_1D<double>::Moving_Average_Two_Sided_Spencers_15_point(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Moving_Average_Two_Sided_Hendersons_23_point(void) const {
    //Computes a "Henderson's 23-point, two-sided, weighted moving average" or mean of the f_i. Ignores x_i spacing. This is 
    // a convolution that effectively acts like a low-pass filter and is generally used to find a the 'trend' of data. The 
    // "23-point" part means 11 points to the left, 11 to the right, and the central point are used for each average.
    //
    // NOTE: See this->Moving_Average_Two_Sided_Driver() function for notes and caveats.
    //
    // NOTE: There are MANY variants of moving averages -- this is just one I've seen around. See the book by Kendall
    //       called "Advanced statistics" Vol. 3, which covers many.
    //
    const std::vector<T> weights = { (T)(0.148), (T)(0.138), (T)(0.122), (T)(0.097), (T)(0.068), (T)(0.039), 
                                     (T)(0.013), (T)(-0.005), (T)(-0.015), (T)(-0.016), (T)(-0.011), (T)(-0.004) };
    return this->Moving_Average_Two_Sided_Driver(weights);

/* Other Henderson's weightings:
Filter Length   Symmetric Weighting Pattern for Henderson Moving Average
5 Term  (-0.073, 0.294, 0.558, 0.294, -0.073)
7 Term  (-0.059, 0.059, 0.294, 0.412, 0.294, 0.059, -0.059)
9 Term  (-0.041, -0.010, 0.119, 0.267, 0.330, 0.267, 0.119, -0.010, -0.041)
13 Term (-0.019, -0.028, 0.0, 0.066, 0.147, 0.214, 0.240, 0.214, 0.147, 0.066, 0.0, -0.028, -0.019)
From http://www.abs.gov.au/websitedbs/d3310114.nsf/51c9a3d36edfd0dfca256acb00118404/5fc845406def2c3dca256ce100188f8e!OpenDocument 
*/
};
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Moving_Average_Two_Sided_Hendersons_23_point(void) const;
    template samples_1D<double> samples_1D<double>::Moving_Average_Two_Sided_Hendersons_23_point(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Moving_Average_Two_Sided_Equal_Weighting(long int N) const {
    //Computes a (2N+1)-point, all-points-in-window-are-equally-weighted, two-sided moving average or mean of the f_i. Ignores
    // x_i spacing. This is a convolution that effectively acts like a low-pass filter and is generally used to find a the 
    // 'trend' of data.
    //
    // NOTE: See this->Moving_Average_Two_Sided_Driver() function for notes and caveats.
    //
    // NOTE: Passing in N=3 will use 3 datum to the left + 3 to the right + the centre point (=7 in total) for the average
    //       and each point. The choice of N will depend on how 'smooth' you want the output, and the scale of the data.
    //       Be aware that fishing until something appears/disappears is quite dangerous!
    //
    // NOTE: It is safe to apply this routine consecutively. One guy claims on his website (which made it to Wikipedia...)
    //       that applying three times might be best. 
    //
    if(N <  0) FUNCERR("Nonsensical parameter passed in. Cannot average less than <0 points");
    if(N == 0) return *this;

    std::vector<T> weights(N+1, (T)(1.0)/static_cast<T>(2*N+1)); //Fill constructor. Uniform weights for all datum in window.
    return this->Moving_Average_Two_Sided_Driver(weights);
};
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Moving_Average_Two_Sided_Equal_Weighting(long int N) const;
    template samples_1D<double> samples_1D<double>::Moving_Average_Two_Sided_Equal_Weighting(long int N) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Moving_Average_Two_Sided_Gaussian_Weighting(T datum_sigma) const {
    //Computes a (2N+1)-point, Gaussian-weighted, two-sided moving average or mean of the f_i. Ignores x_i spacing. 
    // This is a convolution that effectively acts like a low-pass filter and is generally used to find the 'trend' of data.
    //
    // This routine is identical to a Gaussian blur in 1D, except that the spacing between datum is ignored and 
    // assumed to be constant. 
    //
    // NOTE: See this->Moving_Average_Two_Sided_Driver() function for notes and caveats.
    //
    // NOTE: Passing in datum_sigma=2 will use 3*datum_sigma datum to the left, and to the right, and the centre point
    //       for each point. The choice of datum_sigma depends on how 'smooth' you want the output, and the scale of the data.
    //       Be aware that fishing until something appears/disappears is quite dangerous!
    //
    if(datum_sigma <= (T)(0)) FUNCERR("Nonsensical datum_sigma parameter passed in. Try increasing it");

    const T window_size_f      = (T)(3.0)*datum_sigma; //How far away to stop computing. 3sigma ~> 0.01. 5sigma ~> 1E-5 or so.
    const long int window_size = static_cast<long int>(std::ceil(window_size_f));
    const T w_denom            = static_cast<T>(std::sqrt(2.0*M_PI)*datum_sigma);

    std::vector<T> weights;
    for(long int i = 0; i < window_size; ++i){
        weights.push_back(std::exp(-(T)(0.5)*std::pow(i,2)/(datum_sigma*datum_sigma)) / w_denom);
    }

    //Ensure the weighting is as close to 1.0 as we can muster. Needed because we cut-off at some few-sigma level instead
    // of extend out to infinity. We sum the tail twice as per the definition/symmetry of the weighting coefficients.
    T summed_weight = std::accumulate(weights.begin(), weights.end(), (T)(0));
    if(weights.size() > 1) summed_weight += std::accumulate(std::next(weights.begin()), weights.end(), (T)(0));
    for(auto &weight : weights) weight /= summed_weight;
    return this->Moving_Average_Two_Sided_Driver(weights);
};
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Moving_Average_Two_Sided_Gaussian_Weighting(float  datum_sigma) const;
    template samples_1D<double> samples_1D<double>::Moving_Average_Two_Sided_Gaussian_Weighting(double datum_sigma) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Moving_Median_Filter_Two_Sided_Driver(const std::vector<uint64_t> &weights) const {
    //Applies a generic two-sided weighted moving median filter to the f_i using a local window and weights as provided. 
    // Ignores x_i, sigma_x_i. This is a robust low-pass filter and is generally used to find the 'trend' of data. 
    //
    // NOTE: The weights MUST be positive integers, and must sum to at least one. (They need NOT be normalized.)
    //       Weights are assumed to be symmetric about the central weighting term. Providing N weights implies a two-sided
    //       median filter in which each point is combined with the N + (N - 1) = 2*N-1 nearest points.
    //
    //       Integer weights represent the number of times a datum is included in the list of elements to find the median
    //       of. Keep this in mind if you plan to use heavy weighting!
    //
    // NOTE: This routine completely ignores x_i spacing. It assumes a constant or nearly constant x_i spacing is present,
    //       so be aware.
    //
    // NOTE: This routine ignores uncertainties in sigma_x_i, and simply passes them through. The user's assumption about
    //       whether uncertainties are thought to be independent and random is passed on as-is too. If you're cautious then
    //       do not assume independent + random errors to begin with.
    //
    // NOTE: Because this routine requires points on either side of each datum, and we have to deal with two endpoints, a
    //       pragmatic choice is made to insert "phantom" points to the left and right of the data. Their f_i is assumed to
    //       be the nearest non-virtual f_i (i.e., the endpoints). Thus, the SEVEN endpoints should be treated as somewhat
    //       suspect. However, the computed uncertainty follows uncertainty propagation rules correctly (handling an annoying
    //       underestimation gotcha due to virtual endpoint extension) and the result should at least be consistent (even if
    //       the uncertainties flare up at the endpoints).
    //
    // NOTE: See the excellent report "Weighted Median Filters: A Tutorial" by Yin et al., 1996 on the subject of weighted 
    //       median filters and their interesting properties. Spoiler: they are robust and a little hard to characterize.
    //       There is also a statement about the upper limit on the number of times a consecutive median filter can be applied
    //       and continue to have any effect.
    //
    // NOTE: Be aware if you plan to mix moving filters with harmonic or spectral analysis: a moving average can cause an
    //       "irregular oscillation" to mysteriously appear in the data. This is called the Slutzky-Yule effect.
    //
    samples_1D<T> out;
    out.metadata = this->metadata;

    //Not sure whether we can make assumptions about the nature of uncertainties after calling this routine. At the moment
    // sigma_f_i uncertainties are just passed through. So I'm gonna just roll with it and assume any assumptions remain
    // unmodified. If you're cautious then do not assume independent + random errors to begin with!
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;

    const auto sample_count = static_cast<long int>(this->size());
    if(weights.empty()) FUNCERR("No weights provided. Cannot continue");
    const auto weights_size = static_cast<long int>(weights.size());

    for(long int i = 0; i < sample_count; ++i){
        std::array<T,4> newdatum(this->samples[i]);

        //Push back all the weighted points (real or virtual) needed for a median calculation.
        samples_1D<T> shtl;
        shtl.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;
        for(long int j = (1-weights_size); j < weights_size; ++j){
            const auto weight = weights[ YGORABS(j) ];
            const auto pm_indx = i + j;
            const auto indx = YGORABS(pm_indx);

            const auto realpoint = isininc(0,pm_indx,sample_count-1); //Is a real point (not virtual).
            const auto Lendpoint = (pm_indx <= 0);                    //inclusive to the L endpoint.
            const auto Rendpoint = (pm_indx >= (sample_count-1));     //inclusive to the R endpoint.

            long int bnd_index;
            if(realpoint){
                bnd_index = indx;
            }else if(Lendpoint){
                bnd_index = 0;
            }else if(Rendpoint){
                bnd_index = (sample_count-1);
            }

            //Push the datum into the shuttle as many times as the weighting requires.
            std::array<T,4> windowdatum( this->samples[bnd_index] );
            for(uint64_t cnt = 0; cnt < weight; ++cnt) shtl.push_back(windowdatum);
        }

        std::array<T,2> themedian = shtl.Median_y();
        newdatum[2] = themedian[0];
        newdatum[3] = themedian[1];
        out.push_back(newdatum);
    }
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Moving_Median_Filter_Two_Sided_Driver(const std::vector<uint64_t> &weights) const;
    template samples_1D<double> samples_1D<double>::Moving_Median_Filter_Two_Sided_Driver(const std::vector<uint64_t> &weights) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Moving_Median_Filter_Two_Sided_Equal_Weighting(long int N) const {
    //Computes a (2N+1)-point, all-points-in-window-are-equally-weighted, two-sided median filter of the f_i. Ignores
    // x_i, sigma_x_i. This is a robust low-pass filter and is generally used to find a the 'trend' of data.
    //
    // NOTE: See this->Moving_Median_Filter_Two_Sided_Driver() function for notes and caveats.
    //
    // NOTE: Passing in N=3 will use 3 datum to the left + 3 to the right + the centre point (=7 in total) for the average
    //       and each point. The choice of N will depend on how 'smooth' you want the output, and the scale of the data.
    //       Be aware that fishing until something appears/disappears is quite dangerous!
    //
    if(N <  0) FUNCERR("Nonsensical parameter passed in. Cannot average less than <0 points");
    if(N == 0) return *this;
    std::vector<uint64_t> weights(N+1, static_cast<uint64_t>(1)); //Fill constructor. Uniform weights for all datum in window.
    return this->Moving_Median_Filter_Two_Sided_Driver(weights);
};
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Moving_Median_Filter_Two_Sided_Equal_Weighting(long int N) const;
    template samples_1D<double> samples_1D<double>::Moving_Median_Filter_Two_Sided_Equal_Weighting(long int N) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Moving_Median_Filter_Two_Sided_Gaussian_Weighting(T datum_sigma) const {
    //Applies a (2N+1)-point, Gaussian-weighted, two-sided median filter to the f_i. Ignores x_i, sigma_x_i.
    //
    // NOTE: See this->Moving_Median_Filter_Two_Sided_Driver() function for notes and caveats.
    //
    // NOTE: Passing in datum_sigma=2 will use 3*datum_sigma datum to the left, and to the right, and the centre point
    //       for each point. The choice of datum_sigma depends on how 'smooth' you want the output, and the scale of the data.
    //       Be aware that fishing until something appears/disappears is quite dangerous!
    //
    // NOTE: Because the weighted median requires integer weightings, the actual ~convolution kernel applied to the data
    //       could be quite different from a true Gaussian if the window is small enough. There is nothing that can be done
    //       about this short of implementing a median filter capable of taking real weights instead.
    // 
    if(datum_sigma <= (T)(0)) FUNCERR("Nonsensical datum_sigma parameter passed in. Try increasing it");

    const T window_size_f      = (T)(3.0)*datum_sigma; //How far away to stop computing. 3sigma ~> 0.01. 5sigma ~> 1E-5 or so.
    const long int window_size = static_cast<long int>(std::ceil(window_size_f));
    const T w_denom            = static_cast<T>(std::sqrt(2.0*M_PI)*datum_sigma);

    //Number of discrete, distinct weightings to use. Higher means more precise, but also dramatically more expensive 
    // computations. Consider 1/resolution to be approx. equal to the weighting fidelity. So 1/20 = 0.05 which is probably
    // appropriate for a 3*sigma cutoff. You can make this a specifiable parameter if it is too high (which it will be for
    // moderate-to-large data sets).
    const uint64_t resolution  = 20;

    std::vector<uint64_t> weights;
    for(long int i = 0; i < window_size; ++i){
        const T gaussian_weight = std::exp(-(T)(0.5)*std::pow(i,2)/(datum_sigma*datum_sigma)) / w_denom;
        const uint64_t discrete_weight = static_cast<uint64_t>(gaussian_weight * resolution);
        weights.push_back(discrete_weight);
    }

    //There is no need (or means) to normalize the weights. (Contrast to the weighted moving average case.)
    return this->Moving_Median_Filter_Two_Sided_Driver(weights);
};
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Moving_Median_Filter_Two_Sided_Gaussian_Weighting(float  datum_sigma) const;
    template samples_1D<double> samples_1D<double>::Moving_Median_Filter_Two_Sided_Gaussian_Weighting(double datum_sigma) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Moving_Median_Filter_Two_Sided_Triangular_Weighting(long int N) const {
    //Computes a (2N+1)-point, triangular-weighted, two-sided median filter of the f_i. Ignores x_i, sigma_x_i. This is 
    // a robust low-pass filter and is generally used to find a the 'trend' of data.
    //
    // NOTE: See this->Moving_Median_Filter_Two_Sided_Driver() function for notes and caveats.
    //
    // NOTE: Passing in N=3 will use 3 datum to the left + 3 to the right + the centre point (=7 in total) for the average
    //       and each point. The choice of N will depend on how 'smooth' you want the output, and the scale of the data.
    //       Be aware that fishing until something appears/disappears is quite dangerous!
    //
    // NOTE: What is "triangular weighting"? This:
    //
    //                                             *                                       
    //                                        *    *    *                                  
    //                                  *     *    *    *     *                            
    //                            *     *     *    *    *     *     *                      
    //       Points:   ... x-4   x-3   x-2   x-1   x   x+1   x+2   x+3   x+4 ...
    //       Weights:  ...  0     1     2     3    4    3     2     1     0  ...
    //
    if(N <  0) FUNCERR("Nonsensical parameter passed in. Cannot average less than <0 points");
    if(N == 0) return *this;

    std::vector<uint64_t> weights;
    for(uint64_t i = static_cast<uint64_t>(N); i > 0; --i) weights.push_back(i);

    return this->Moving_Median_Filter_Two_Sided_Driver(weights);
};
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Moving_Median_Filter_Two_Sided_Triangular_Weighting(long int N) const;
    template samples_1D<double> samples_1D<double>::Moving_Median_Filter_Two_Sided_Triangular_Weighting(long int N) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Moving_Variance_Two_Sided(long int N) const {
    //Calculates an unbiased estimate of a population's variance over a window of (2N+1) points. Endpoints use fewer points 
    // (min = N) and have higher variance.
    //
    // NOTE: For meaningful results, one should use at least 5 samples to estimate the variance. This means set N >= 5 (if
    //       the endpoints are of interest to you) or N >= 2 (if you are OK with bad estimates near the endpoints).
    //
    // NOTE: Ignores x_i spacing, sigma_x_i, *and* sigma_f_i! Outgoing f_i are the variances. Outgoing sigma_f_i are zero,
    //       and so no assumption is made about the totally zero uncertainties.
    //
    // NOTE: This routine SHOULD be updated to take sigma_f_i into account. Also, the outgoing sigma_f_i should be set to the
    //       variance of the variance if it can be calculated.
    //
    // NOTE: If treatment of endpoints bothers you, consider doing a discrete binning instead of windowing. Ignores x_i, sigma_x_i,
    //       *and* sigma_f_i!
    //
    // NOTE: Be aware if you plan to mix moving filters with harmonic or spectral analysis: a moving average can cause an
    //       "irregular oscillation" to mysteriously appear in the data. This is called the Slutzky-Yule effect.
    //
    if(N <  0) FUNCERR("Nonsensical parameter passed in. Cannot compute variance from less than 0 points");
    if(N == 0) FUNCERR("Nonsensical parameter passed in. Cannot compute variance from single point");

    const bool InhibitSort = true;
    samples_1D<T> out;
    out.metadata = this->metadata;

    const auto samps_size = static_cast<long int>(this->samples.size());
    for(long int i = 0; i < samps_size; ++i){
        std::list<double> shtl;
        for(long int j = (i-N); j <= (i+N); ++j){
            if(isininc(0,j,samps_size-1)) shtl.push_back(this->samples[j][2]);
        }
        const auto var = static_cast<T>(Stats::Unbiased_Var_Est(shtl));        
        out.push_back({ this->samples[i][0], this->samples[i][1], var, (T)(0) }, InhibitSort);
    } 
    return std::move(out);
};
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Moving_Variance_Two_Sided(long int N) const;
    template samples_1D<double> samples_1D<double>::Moving_Variance_Two_Sided(long int N) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Derivative_Forward_Finite_Differences(void) const {
    //Calculates the discrete derivative using forward finite differences. (The right-side endpoint uses backward 
    // finite differences to handle the boundary.) Data should be reasonably smooth -- no interpolation is used.
    //
    // NOTE: The data needs to be relatively smooth for this routine to be useful. No interpolation or smoothing 
    //       is performed herein. If such is needed, you should do it before calling this routine.
    //
    // NOTE: If the spacing between datum is not too large, the centered finite difference will probably be a
    //       better estimate of the derivative because it is not forward or backward biased.
    //
    // NOTE: At the moment, this routine cannot handle when adjacents points' dx is not finite. In the future, 
    //       these points might be averaged -- though the intolerance behaviour is not unreasonable. Do not
    //       count on either behaviour. (At the time of writing, the derivative at such point will simply be 
    //       infinite or NaN, depending on the implementation.)
    //
    // NOTE: AT THE MOMENT, UNCERTAINTIES sigma_x_i and sigma_f_i ARE IGNORED! This should be rectified!
    //       Ideally, they should propagate standard operations errors AND the inherent finite-difference
    //       errors (of order O(dx)) PLUS any estimated local sampling noise (if possible).

    #pragma message "Warning - finite difference routine are lacking uncertainty-handling!"

    const auto samps_size = static_cast<long int>(this->samples.size());
    if(samps_size < 2) FUNCERR("Cannot compute derivative with so few points. Cannot continue");    
    const bool InhibitSort = true;
    samples_1D<T> out = *this;

    for(long int i = 0; i < samps_size; ++i){
        //Use backward finite differences at the far boundary.
        if(i == (samps_size - 1)){
            const auto L = this->samples[i-1];
            const auto C = this->samples[i];
            //const auto R = this->samples[i+1];
            const auto new_x_i = C[0];
            const auto new_sigma_x_i = C[1]; // <---- need something better here!
            const auto new_f_i = (C[2] - L[2])/(C[0] - L[0]);
            const auto new_sigma_f_i = C[3]; // <---- need something better here!

            out.samples[i] = { new_x_i, new_sigma_x_i, new_f_i, new_sigma_f_i };

        //Else, use forward finite differences.
        }else{
            //const auto L = this->samples[i-1];
            const auto C = this->samples[i];
            const auto R = this->samples[i+1];
            const auto new_x_i = C[0];
            const auto new_sigma_x_i = C[1]; // <---- need something better here!
            const auto new_f_i = (R[2] - C[2])/(R[0] - C[0]);
            const auto new_sigma_f_i = C[3]; // <---- need something better here!

            out.samples[i] = { new_x_i, new_sigma_x_i, new_f_i, new_sigma_f_i };

        }
    }
    return std::move(out);
};
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Derivative_Forward_Finite_Differences(void) const;
    template samples_1D<double> samples_1D<double>::Derivative_Forward_Finite_Differences(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Derivative_Backward_Finite_Differences(void) const {
    //Calculates the discrete derivative using backward finite differences. (The right-side endpoint uses forward 
    // finite differences to handle the boundary.) Data should be reasonably smooth -- no interpolation is used.
    //
    // NOTE: The data needs to be relatively smooth for this routine to be useful. No interpolation or smoothing 
    //       is performed herein. If such is needed, you should do it before calling this routine.
    //
    // NOTE: If the spacing between datum is not too large, the centered finite difference will probably be a
    //       better estimate of the derivative because it is not forward or backward biased.
    //
    // NOTE: At the moment, this routine cannot handle when adjacents points' dx is not finite. In the future, 
    //       these points might be averaged -- though the intolerance behaviour is not unreasonable. Do not
    //       count on either behaviour. (At the time of writing, the derivative at such point will simply be 
    //       infinite or NaN, depending on the implementation.)
    //
    // NOTE: AT THE MOMENT, UNCERTAINTIES sigma_x_i and sigma_f_i ARE IGNORED! This should be rectified!
    //       Ideally, they should propagate standard operations errors AND the inherent finite-difference
    //       errors (of order O(dx)) PLUS any estimated local sampling noise (if possible).

    #pragma message "Warning - finite difference routine are lacking uncertainty-handling!"

    const auto samps_size = static_cast<long int>(this->samples.size());
    if(samps_size < 2) FUNCERR("Cannot compute derivative with so few points. Cannot continue");    
    const bool InhibitSort = true;
    samples_1D<T> out = *this;

    for(long int i = 0; i < samps_size; ++i){
        //Handle left boundary: use forward finite differences.
        if(i == 0){
            //const auto L = this->samples[i-1];
            const auto C = this->samples[i];
            const auto R = this->samples[i+1];
            const auto new_x_i = C[0];
            const auto new_sigma_x_i = C[1]; // <---- need something better here!
            const auto new_f_i = (R[2] - C[2])/(R[0] - C[0]);
            const auto new_sigma_f_i = C[3]; // <---- need something better here!

            out.samples[i] = { new_x_i, new_sigma_x_i, new_f_i, new_sigma_f_i };

        //Else, use backward finite differences.
        }else{
            const auto L = this->samples[i-1];
            const auto C = this->samples[i];
            //const auto R = this->samples[i+1];
            const auto new_x_i = C[0];
            const auto new_sigma_x_i = C[1]; // <---- need something better here!
            const auto new_f_i = (C[2] - L[2])/(C[0] - L[0]);
            const auto new_sigma_f_i = C[3]; // <---- need something better here!

            out.samples[i] = { new_x_i, new_sigma_x_i, new_f_i, new_sigma_f_i };

        }
    }
    return std::move(out);
};
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Derivative_Backward_Finite_Differences(void) const;
    template samples_1D<double> samples_1D<double>::Derivative_Backward_Finite_Differences(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> samples_1D<T> samples_1D<T>::Derivative_Centered_Finite_Differences(void) const {
    //Calculates the discrete derivative using centered finite differences. (The endpoints use forward and backward 
    // finite differences to handle boundaries.) Data should be reasonably smooth -- no interpolation is used.
    //
    // NOTE: The data needs to be relatively smooth for this routine to be useful. No interpolation or smoothing 
    //       is performed herein. If such is needed, you should do it before calling this routine.
    //
    // NOTE: If the spacing between datum is not too large, the centered finite difference will probably be a
    //       better estimate of the derivative because it is not forward or backward biased.
    //
    // NOTE: At the moment, this routine cannot handle when adjacents points' dx is not finite. In the future, 
    //       these points might be averaged -- though the intolerance behaviour is not unreasonable. Do not
    //       count on either behaviour. (At the time of writing, the derivative at such point will simply be 
    //       infinite or NaN, depending on the implementation.)
    //
    // NOTE: AT THE MOMENT, UNCERTAINTIES sigma_x_i and sigma_f_i ARE IGNORED! This should be rectified!
    //       Ideally, they should propagate standard operations errors AND the inherent finite-difference
    //       errors (of order O(dx)) PLUS any estimated local sampling noise (if possible).

    #pragma message "Warning - finite difference routine are lacking uncertainty-handling!"

    const auto samps_size = static_cast<long int>(this->samples.size());
    if(samps_size < 2) FUNCERR("Cannot compute derivative with so few points. Cannot continue");    
    const bool InhibitSort = true;
    samples_1D<T> out = *this;

    for(long int i = 0; i < samps_size; ++i){
        //Handle left boundary: use forward finite differences.
        if(i == 0){
            //const auto L = this->samples[i-1];
            const auto C = this->samples[i];
            const auto R = this->samples[i+1];
            const auto new_x_i = C[0];
            const auto new_sigma_x_i = C[1]; // <---- need something better here!
            const auto new_f_i = (R[2] - C[2])/(R[0] - C[0]);
            const auto new_sigma_f_i = C[3]; // <---- need something better here!

            out.samples[i] = { new_x_i, new_sigma_x_i, new_f_i, new_sigma_f_i };

        //Handle right boundary: use backward finite differences.
        }else if(i == (samps_size - 1)){
            const auto L = this->samples[i-1];
            const auto C = this->samples[i];
            //const auto R = this->samples[i+1];
            const auto new_x_i = C[0];
            const auto new_sigma_x_i = C[1]; // <---- need something better here!
            const auto new_f_i = (C[2] - L[2])/(C[0] - L[0]);
            const auto new_sigma_f_i = C[3]; // <---- need something better here!

            out.samples[i] = { new_x_i, new_sigma_x_i, new_f_i, new_sigma_f_i };

        //Else, use centered finite differences.
        }else{
            const auto L = this->samples[i-1];
            const auto C = this->samples[i];
            const auto R = this->samples[i+1];
            const auto new_x_i = C[0];
            const auto new_sigma_x_i = C[1]; // <---- need something better here!
            const auto new_f_i = (R[2] - L[2])/(R[0] - L[0]);
            const auto new_sigma_f_i = C[3]; // <---- need something better here!

            out.samples[i] = { new_x_i, new_sigma_x_i, new_f_i, new_sigma_f_i };

        }
    }
    return std::move(out);
};
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Derivative_Centered_Finite_Differences(void) const;
    template samples_1D<double> samples_1D<double>::Derivative_Centered_Finite_Differences(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> 
samples_1D<T> samples_1D<T>::Local_Signed_Curvature_Three_Datum(void) const {
    //Calculates the local signed curvature at x_i using adjacent nearest-neighbour datum. Endpoints are dropped. 
    // Both sigma_x_i and sigma_f_i are propagated. Curvature taken to be (1/r)*sign where r is the radius of the 
    // circle tangent to three consecutive samples and sign is +1 if curving upward (+y-direction) or -1 if 
    // curving downward.
    //
    // NOTE: Which curvature is this? It is apparently called the "first curvature" in Riemannian geometry. It is
    //       kappa*sign = (1/r)*sign which means r is called the "radius of curvature." The circle which defines r 
    //       is called the "osculating circle" (though we are only approximating it because the osculating circle 
    //       requires continuous tangents to compute exactly). The osculating circle can be thought of as the
    //       best-fit circle of the curve at the specified point.
    //
    // NOTE: Although a circle is 'fit' through three points, this routine is completely deterministic.
    //
    // NOTE: This routine will fail if x_i = x_j for any i and j (it may report infinite curvature). The datum must 
    //       be sorted.
    //
    // NOTE: It is unclear what affect the x-spacing (dx_i) will have on the curvature estimate. This routine is NOT
    //       robust to noise. In fact, it is highly sensitive to noise and even small loses of numerical precision 
    //       due to truncation and roundoff. It might be best to try smooth or average the data if possible before
    //       this computation. Note that the smoothing will almost certainly alter the curvature estimates. 
    //
    //       Conceptually, the closer adjacent points are (the higher the sampling), the less stable the tangent
    //       circle calculation is going to be. Thus, the more frequently you sample, the more certain you need to
    //       be about the measurements in order to maintain the same uncertainty estimates. However, the less 
    //       frequently you sample, the less precise your local curvature estimate becomes!
    //
    //       Since this obviously puts you, the user, in a fairly tough situation, consider using the uncertainty
    //       estimates to help form your opinion. Binning would be worthwhile as both sigma_x_i and sigma_f_i are
    //       handled by this routine. The choice of bin width or count is something you'll need to figure out, but
    //       one thing is certain: you'll need bins to be at least 2-3x narrower than the minimum feature width 
    //       you want to see in the output. (So if you want to see observe a Gaussian 'blip' with width ~10 units,
    //       your bin width should be no more than ~3-4 units.)
    //
    //       If nothing else, consider binning the result to estimate the (true) variance and see if there is a
    //       natural bin width (i.e., bins must be at least X units wide for reasonable variance) that pops out.
    //
    // NOTE: This routine propagates uncertainty. It does not estimate actual variance like binning can.
    //
    // NOTE: The uncertainties can get quite large due to the nature of the defintion of the local curvature and the
    //       fact that the radius can be substantially changed if the points are moved slightly. Nonetheless, I think
    //       the uncertainty estimate here is a slight overestimate. The circle centre found is equidistant from all 
    //       three datum, but we only use the central datum to calculate the radius. The two adjacent data could 
    //       probably be used, which should logically reduce the total resulting uncertainty. For example, you could
    //       explicitly chose to use the datum with the smallest total uncertainty instead of defaulting to the
    //       central datum. This is not done here. On the other hand, the uncertainty of all three datum are already
    //       incorporated during the calculation (all three are necessary to find a solution) and so the reported
    //       uncertainty might already be a proper amalgamation of all available uncertainty.
    //
    //       Anyways, I would suggest not relying on the uncertainties. If you do, I suggest you bin afterward. If
    //       you cannot bin, try averaging if possible. If you can do neither, and you think the uncertainty seems 
    //       too large, scaling them by 1/sqrt(3) might be defensible. I wouldn't scale them by any more. 
    //
    samples_1D<T> out;
    out.uncertainties_known_to_be_independent_and_random = this->uncertainties_known_to_be_independent_and_random;   
    out.metadata = this->metadata;

    if(this->size() < 3) FUNCERR("Unable to compute local curvature with fewer than 3 datum. Cannot continue"); 
    const auto InhibitSort = true;

    auto itL = this->samples.begin();
    auto itC = std::next(itL,1);
    auto itR = std::next(itL,2);
    while(itR != this->samples.end()){
        //This technique comes from writing out the equations of a circle, linearizing by subtracting off the squared
        // unknowns, writing as a linear system, inverting the coefficient matrix, solving for the circle's center 
        // coordinates, and then figuring out the distance (radius) from either L, C, or R to the centre.
        //

        const T  Ax((*itC)[0]),  Bx((*itL)[0]),  Cx((*itR)[0]);
        const T dAx((*itC)[1]), dBx((*itL)[1]), dCx((*itR)[1]);
        const T  Ay((*itC)[2]),  By((*itL)[2]),  Cy((*itR)[2]);
        const T dAy((*itC)[3]), dBy((*itL)[3]), dCy((*itR)[3]);

        const T AA(std::pow(Ay,2) + std::pow(Ax,2));
        const T BB(std::pow(By,2) + std::pow(Bx,2));
        const T CC(std::pow(Cy,2) + std::pow(Cx,2));
        
        const T AAmCC(AA-CC),  AAmBB(AA-BB), BBmCC(BB-CC);
        const T CCmAA(-AAmCC), BBmAA(-AAmBB), CCmBB(-BBmCC);
        
        const T AymBy(Ay-By), AymCy(Ay-Cy), BymCy(By-Cy);
        const T AxmBx(Ax-Bx), AxmCx(Ax-Cx), BxmCx(Bx-Cx);
        const T BymAy(-AymBy), CymAy(-AymCy), CymBy(-BymCy);
        const T BxmAx(-AxmBx), CxmAx(-AxmCx), CxmBx(-BxmCx);
        
        const T det = AymBy*AxmCx-AxmBx*AymCy;
        const T invdet = (T)(1)/det;
        const T invdetsq = invdet * invdet;
        const T half = (T)(0.5);
        const T x0 = half*invdet*(AymBy*AAmCC - AymCy*AAmBB); //The circle's centre's coordinates.
        const T y0 = half*invdet*(AxmCx*AAmBB - AxmBx*AAmCC);

        const T y0mAy = y0-Ay;       
        const T x0mAx = x0-Ax;       
        const T r = std::hypot(x0-Ax, y0-Ay);  //Radius of the tangent circle, computed using the central point's coords.
        const T rinv = (T)(1)/r;  //This is enough for x_i, sigma_x_i, and f_i. The rest is for sigma_f_i.
        const T drinv_common_inv_den = std::pow(rinv,3);
       
        //Figure out if the curvature is positive (upward) or negative (downward).
        const vec3<T> Avec(Ax, Ay, (T)(0));
        const vec3<T> Bvec(Bx, By, (T)(0));
        const vec3<T> Cvec(Cx, Cy, (T)(0));
        const vec3<T> BCplaneN(vec3<T>((T)(0),(T)(0),(T)(1)).Cross(Cvec-Bvec).unit());
        const plane<T> BCplane(BCplaneN, Cvec);
        const T posorneg = BCplane.Is_Point_Above_Plane(Avec) ? (T)(-1) : (T)(1);

        //Figure out the uncertainty propagated by computing rinv.
        const T drinvdAxnum = -(((half*CCmBB+Ax*BxmCx)*invdet-(AAmBB*AxmCx-AAmCC*AxmBx)*CymBy*half*invdetsq)*y0mAy 
                              + (Ax*CymBy*invdet-(AAmCC*AymBy-AAmBB*AymCy)*CymBy*half*invdetsq-(T)(1))*x0mAx);
        const T drinvdAx = drinvdAxnum * drinv_common_inv_den;
        
        const T drinvdAynum = -((BxmCx*Ay*invdet-(AAmBB*AxmCx-AAmCC*AxmBx)*BxmCx*half*invdetsq-(T)(1))*y0mAy
                              + ((BBmCC+Ay*CymBy)*invdet-(AAmCC*AymBy-AAmBB*AymCy)*BxmCx*half*invdetsq)*x0mAx);
        const T drinvdAy = drinvdAynum * drinv_common_inv_den;
        
        const T drinvdBxnum = -(((half*AAmCC-AxmCx*Bx)*invdet-(AAmBB*AxmCx-AAmCC*AxmBx)*AymCy*half*invdetsq)*y0mAy
                              + (AymCy*Bx*invdet-AymCy*(AAmCC*AymBy-AAmBB*AymCy)*half*invdetsq)*x0mAx);
        const T drinvdBx = drinvdBxnum * drinv_common_inv_den;
        
        const T drinvdBynum = -((-AxmCx*By*invdet-(AAmBB*AxmCx-AAmCC*AxmBx)*CxmAx*half*invdetsq)*y0mAy
                              + ((half*CCmAA+AymCy*By)*invdet-(AAmCC*AymBy-AAmBB*AymCy)*CxmAx*half*invdetsq)*x0mAx);
        const T drinvdBy = drinvdBynum * drinv_common_inv_den;
        
        const T drinvdCxnum = -(((AxmBx*Cx+half*BBmAA)*invdet-(AAmBB*AxmCx-AAmCC*AxmBx)*BymAy*half*invdetsq)*y0mAy
                              + (-AymBy*Cx*invdet-(AAmCC*AymBy-AAmBB*AymCy)*BymAy*half*invdetsq)*x0mAx);
        const T drinvdCx = drinvdCxnum * drinv_common_inv_den;
        
        const T drinvdCynum = -((AxmBx*Cy*invdet-AxmBx*(AAmBB*AxmCx-AAmCC*AxmBx)*half*invdetsq)*y0mAy
                              + ((half*AAmBB-AymBy*Cy)*invdet-AxmBx*(AAmCC*AymBy-AAmBB*AymCy)*half*invdetsq)*x0mAx);
        const T drinvdCy = drinvdCynum * drinv_common_inv_den;

        out.push_back((*itC)[0],  (T)(0),  rinv * posorneg,  (T)(0),  InhibitSort);

        if(this->uncertainties_known_to_be_independent_and_random){
            out.samples.back()[3] = std::sqrt( std::pow(drinvdAx*dAx,2) + std::pow(drinvdAy*dAy,2) 
                                             + std::pow(drinvdBx*dBx,2) + std::pow(drinvdBy*dBy,2)
                                             + std::pow(drinvdCx*dCx,2) + std::pow(drinvdCy*dCy,2) );
        }else{
            out.samples.back()[3] = ( std::abs(drinvdAx*dAx) + std::abs(drinvdAy*dAy)
                                    + std::abs(drinvdBx*dBx) + std::abs(drinvdBy*dBy)
                                    + std::abs(drinvdCx*dCx) + std::abs(drinvdCy*dCy) );
        }

        std::advance(itL,1);
        std::advance(itC,1);
        std::advance(itR,1);
    }

    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > samples_1D<float >::Local_Signed_Curvature_Three_Datum(void) const;
    template samples_1D<double> samples_1D<double>::Local_Signed_Curvature_Three_Datum(void) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> lin_reg_results<T> samples_1D<T>::Linear_Least_Squares_Regression(bool *OK, bool SkipExtras) const {
    //Performs a standard linear (y=m*x+b) regression. This routine ignores all provided uncertainties, and instead reports
    // the calculated (perceived?) uncertainties from the data. Thus, this routine ASSUMES all sigma_y_i are the same, and
    // ASSUMES all sigma_x_i are ZERO. Obviously these are not robust assumptions, so it is preferrable to use WEIGHTED
    // REGRESSION if uncertainties are known. 
    //
    // An example where this routine would NOT be appropriate: you take some measurements and then 'linearize' your data by
    // taking the logarithm of all y_i. Unless your data all have very similar y_i, you will very certainly break the 
    // assumption that all sigma_f_i are equal. (Weighted regression *is* suitable for such a situation.)
    //
    // Returns: {[0]slope, [1]sigma_slope, [2]intercept, [3]sigma_intercept, [4]the # of datum used, [5]std dev of 
    // the data ("sigma_f"), [6]sum-of-squared residuals, [7]covariance, [8]linear corr. coeff. (r, aka "Pearson's),
    // [9]t-value for r, [10]two-tailed P-value for r}. 
    //
    // If you only want the slope and intercept, pass in SkipExtras = true. This should speed up the computation. But
    // conversely, do not rely on ANYTHING being present other than slope and intercept if you set SkipExtras = true.
    //
    // Some important comments on the output parameters:
    //
    // - The slope and intercept are defined as [y = slope*x + intercept]. 
    //
    // - The std dev of the data is reported -- this is the sigma_f_i for all datum under the assumption that all 
    //   sigma_f_i are equal. 
    //
    // - The covariance is useful for computing uncertainties while performing interpolation using the resultant parameters.
    //   Neglecting the covariance can lead to substantial under- or over-prediction of uncertainty! (If in doubt, check 
    //   the standard uncertainty propagation formulae: it includes a covariance term I otherwise neglect. Performing the 
    //   uncertainty propagation is straightforward so you have no excuse to forgo it.)
    //
    // - The sum-of-squared residuals is the metric for fitting. It should be at a minimum for the optimal fit and is often
    //   reported for non-linear fits. It can be used to compare various types of fits (e.g., linear vs. nonlinear).
    //
    // - The linear correlation coefficient (aka "Pearson's", aka "r") describes how well the data fits a linear functional
    //   form. Numerical Recipes suggests using Spearman's Rank Correlation Coefficient (Rs) or Kendell's Tau coefficient 
    //   instead, as they take into account characteristics of the x_i and f_i distribution that Pearson's r does not.
    //
    // - The t-value and number of datum used can be converted to a probability (P-value) as either a one- or two-tailed 
    //   distribution. The Degrees of Freedom (DOF) to use is [# of datum used - 2] to account for the number of free 
    //   parameters. (It might be better to simply use the returned P-value, I don't know at the moment.)
    //
    // - The two-tailed P-value describes the probability that the data (N measurements of two uncorrelated variables; the 
    //   ordinate and abscissa) would give a *larger* |r| than what we reported. So if N=20 and |r|=0.5 then P=0.025 and the
    //   linear correlation is significant (assuming your threshold is P=0.05). Thus this value can be used to inspect 
    //   whether the fit line is a 'good fit'. Numerical recipes advocates a Student's t-test while John R. Taylor gives an
    //   (more likely to be) exact integral for computing p-values which will necessitate a numerical integration or 
    //   evaluation of a hypergeometric function. (See John R. Taylor's "An Introduction to Error Analysis", 2nd Ed., 
    //   Appendix C.) We have gone the Student's t-test route for speed and simplicity, but a routine to compute Taylor's
    //   way is provided in YgorStats. A cursory comparison showed strong agreement in reasonable circumstances. It would
    //   be prudent to ensure the computations agree if relying on the p-values.
    //
    // NOTE: This routine is often used to get first-order guess of fit parameters for more sophisticated routines (i.e.,
    //       weighted linear regression). This is an established (possibly even statistically legitimate) strategy.
    //
    // NOTE: We could go on to compute lots of additional things with what this routine spits out:
    //       - P-values corresponding to the t-value and DOF=#datum_used-2 (either one- or two-tailed),
    //       - Coefficient of determination = r*r = r^2,
    //       - Standard error of r = sqrt((1-r*r)/DOF), (<--- Note this is not the same as the std dev!)
    //       - (I think) a z-value,
    //       - If all sigma_x_i are equal, you can transform it into a sigma_f_i uncertainty by rolling it into the
    //         reported std dev of the data ("sigma_f"). This gives you an equivalent total sigma_f using either:
    //         1. [std::hypot(sigma_f, slope*sigma_x);] (if normal, random, independent uncertainties), or
    //         2. [std::abs(sigma_f) + std::abs(slope*sigma_x);] otherwise.
    //       - The uncertainty in sigma_y as the estimate of the true width of the distribution. Double check this, 
    //         but I believe it is merely 1/sqrt(2*(N-2)). (Please double check! Search "fractional uncertainty in sigma".)
    //       And on and on. 
    //

    if(OK != nullptr) *OK = false;
    const lin_reg_results<T> ret_on_err;
    lin_reg_results<T> res;

    //Ensure the data is suitable.
    if(this->size() < 3){
        //If we have two points, there are as many parameters as datum. We cannot even compute the stats.
        // While it is possible to do it for 2 data points, the closed form solution in that case is 
        // easy enough to do in a couple lines, and probably should just be implemented as needed.
        if(OK == nullptr) FUNCERR("Unable to perform meaningful linear regression with so few points");
        FUNCWARN("Unable to perform meaningful linear regression with so few points. Bailing");
        return ret_on_err;
    }
    res.N = static_cast<T>(this->size());
    res.dof = res.N - (T)(2);

    //Cycle through the data, accumulating the basic ingredients for later.
/*
    res.sum_x  = (T)(0);
    res.sum_f  = (T)(0);
    res.sum_xx = (T)(0);
    res.sum_xf = (T)(0);
    for(const auto &P : this->samples){
        res.sum_x  += P[0];
        res.sum_xx += P[0]*P[0];
        res.sum_f  += P[2];
        res.sum_xf += P[0]*P[2];
    }
*/
    {   //Accumulate the data before summing, so we can be more careful about summing the numbers.
        std::vector<T> data_x, data_f, data_xx, data_xf;
        for(const auto &P : this->samples){
            data_x.push_back(  P[0]      );
            data_f.push_back(  P[2]      );
            data_xx.push_back( P[0]*P[0] );
            data_xf.push_back( P[0]*P[2] );
        }
        res.sum_x  = Stats::Sum(data_x);
        res.sum_f  = Stats::Sum(data_f);
        res.sum_xx = Stats::Sum(data_xx);
        res.sum_xf = Stats::Sum(data_xf);
    }


    //Compute the fit parameters.
    const T common_denom = std::abs(res.N*res.sum_xx - res.sum_x*res.sum_x);
    if(!std::isnormal(common_denom)){
        //This cannot be zero, inf, or nan. Proceeding with a sub-normal is also a bad idea.
        if(OK == nullptr) FUNCERR("Encountered difficulties with data. Is the data pathological?");
        FUNCWARN("Encountered difficulties with data. Is the data pathological? Bailing");
        return ret_on_err;
    }

    res.slope = (res.N*res.sum_xf - res.sum_x*res.sum_f)/common_denom;
    res.intercept = (res.sum_xx*res.sum_f - res.sum_x*res.sum_xf)/common_denom;
    if(!std::isfinite(res.slope) || !std::isfinite(res.intercept)){
        //While we *could* proceed, something must be off. Maybe an overflow (and inf or nan) during accumulation?
        if(OK == nullptr) FUNCERR("Encountered difficulties computing m and b. Is the data pathological?");
        FUNCWARN("Encountered difficulties computing m and b. Is the data pathological? Bailing");
        return ret_on_err;
    }

    if(SkipExtras){
        if(OK != nullptr) *OK = true;
        return std::move(res);
    }

    //Now compute the statistical stuff.
    res.mean_x = res.sum_x/res.N;
    res.mean_f = res.sum_f/res.N;
    res.sum_sq_res = (T)(0);
    res.Sxf = (T)(0);
    res.Sxx = (T)(0);
    res.Sff = (T)(0);
    for(const auto &P : this->samples){
        res.sum_sq_res += std::pow(P[2] - (res.intercept + res.slope*P[0]), 2);
        res.Sxf += (P[0] - res.mean_x)*(P[2] - res.mean_f);
        res.Sxx += std::pow(P[0] - res.mean_x, 2.0);
        res.Sff += std::pow(P[2] - res.mean_f, 2.0);
    }
    res.sigma_f = std::sqrt(res.sum_sq_res/res.dof);
    res.sigma_slope = res.sigma_f*std::sqrt(res.N/common_denom);
    res.sigma_intercept = res.sigma_f*std::sqrt(res.sum_x/common_denom);
    res.lin_corr = res.Sxf/std::sqrt(res.Sxx*res.Sff);
    res.covariance = res.Sxf/res.N;
    res.tvalue = res.lin_corr*std::sqrt(res.dof/((T)(1)-std::pow(res.lin_corr,2.0)));

    //Reminder: this p-value is based on t-values and is thus only approximately correct!
    res.pvalue = Stats::P_From_StudT_2Tail(res.tvalue, res.dof);

    if(OK != nullptr) *OK = true;
    return std::move(res);
}       
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template lin_reg_results<float > samples_1D<float >::Linear_Least_Squares_Regression(bool *OK, bool SkipExtras) const;
    template lin_reg_results<double> samples_1D<double>::Linear_Least_Squares_Regression(bool *OK, bool SkipExtras) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> lin_reg_results<T> samples_1D<T>::Weighted_Linear_Least_Squares_Regression(bool *OK, T *slope_guess) const {
    // This routine is not an exact drop-in replacement for the un-weighted case!
    //
    // It is a more sophisticated linear regression. Takes into account both sigma_x_i and sigma_f_i uncertainties to 
    // compute a weighted linear regression using likelihood-maximizing weighting. The sigma_x_i uncertainties are 
    // converted into equivalent sigma_f_i uncertainties based on a trial (ordinary, non-weighted) linear regression. So 
    // the underlying assumption is that the uncertainties are of sufficient uniformity that they will not substantially 
    // affect the regression. This technique is covered in chapter 8 of John R. Taylor's "An Introduction to Error 
    // Analysis" 2nd Ed.. See eqn. 8.21, its footnote, and problem 8.9 for detail. A better solution would be to 
    // iterate through the regression with successively-computed slopes, only stopping when (if?) no change is detected
    // between iterations. (How to achieve this is discussed below.) Some quantities computed were inspired by Numerical
    // Recipes.
    //
    // From what I can ascertain, this routine IS suitable for properly fitting 'linearized' data, so long as the 
    // uncertainties are transformed according to the standard error propagation formula.
    //
    // Returns: {[0]slope, [1]sigma_slope, [2]intercept, [3]sigma_intercept, [4]the # of datum used, [5]std dev of 
    // the data ("sigma_f"), [6]Chi-square, [7]covariance, [8]linear corr. coeff. (aka "Pearson's), [9]Q-value of fit,
    // [10]covariance of the slope and intercept, [11]corr. coeff. for sigma_slope and sigma_intercept}.
    //
    // Many of these parameters are also reported in the un-weighted linear regression case. The differences:
    //
    // - There is no t-value because a Chi-square statistic is now used. There is nothing like the t-value for the Chi-
    //   square, so the Chi-square itself is reported. (This is essentially a weighted sum-of-square residuals.)
    //
    // - There is no p-value. Instead a q-value is reported. This is the Chi-square equivalent to a p-value, but is less
    //   precise. (It does not involve the linear correlation coefficient like the un-weighted case.) 
    //
    // - The correlation between uncertainties ("r_{a,b}" in Numerical Recipes) is reported (c.f., linear correlation 
    //   coefficient -- aka Pearson's -- which computes the correlation between abscissa and ordinate). If r_{a,b} is 
    //   positive, errors in the slope and intercept are likely to have the same sign. If negative, they'll be likely to
    //   have opposite signs. Do not confuse r_{a,b} and Pearson's r. They are totaly different quantities!
    //
    //   Also note that if all the datum have ~zero uncertainty, and are thus infinitely weighted, r_{a,b} is not useful
    //   for anything because sigma_slope and sigma_intercept are zero.
    //
    // - The 'covariance of the slope and intercept' ["cov(a,b)"] is different from the 'covariance of the data.' 
    //   It is introduced in Numerical Recipes, but not discussed in any depth.
    //
    // NOTE: You can determine the best slope_guess to use by starting with a guess and iterating the regression.
    //       Simple tests indicate it seems stable and can be jump-started with just about anything reasonable.
    //       For example, using data:     { { 1.00, 0.10, 2.0000, 134.221 },
    //                                    { 1.05, 0.20, 3.0500, 134.221 },
    //                                    { 1.10, 0.20, 10.600, 134.221 },
    //                                    { 1.40, 0.90, 1000.0, 134.221 } }
    //       with the first and last points to guess the slope initially gives successive slopes: 
    //                                1523, 1675, 1643, 1649, 1648, 1648 ... 
    //       with a corresponding shrink of sigma_slope. It is recommended to do this if your uncertainties are highly
    //       variable or if a small number of points appear to be determining the fit.
    //
    //       Leaving slope_guess as nullptr will default to an un-weighted regression slope.
    //
    // NOTE: This routine can be made to report nearly identical parameters as standard (unweighted) linear regression by
    //       making all sigma_x_i zero and making all sigma_f_i equal to the std dev of the data ("sigma_y") reported by 
    //       standard linear regression. Alternatively, increasing sigma_x_i while decreasing sigma_f_i can have the
    //       same effect (the exact amount depends on the assumptions made about uncertainties).
    //
    // *****************************************************************************************************************
    // ****** Therefore, setting the uncertainties to (T)(1) WILL NOT give you standard linear regression! If you ******
    // ****** don't have uncertainties, I strongly recommend you to use standard non-weighted linear regression!  ******
    // *****************************************************************************************************************
    //
    // NOTE: The deferral of determining the slope with non-weight linear regression is often used to get a first-order 
    //       guess of fit parameters for more sophisticated routines (i.e., weighted linear regression). This is an 
    //       established (possibly even statistically legitimate) strategy. Still, it could be more robust. If in doubt,
    //       use a Monte Carlo or bootstrap approach instead.
    //
    // NOTE: From what I can tell, the conversion of sigma_x_i to an equivalent sigma_f_i based on the slope should always
    //       combine uncertainties in quadrature -- even if the data are not assumed to be normal, random, and independent
    //       themselves. This seems to be what John R. Taylor advocates in his book, anyways. I'm not thrilled about this
    //       and so have decided to play it safe and use the assumption-less propagation formula if no assumptions are 
    //       specified. This may result in an over-estimate of the uncertainty.
    //
    // NOTE: Like the weighted mean, if there are any datum with effectively zero sigma_f_i mixed with non-zero sigma_f_i
    //       datum, the latter will simply not contribute. And because we cannot differentiate various strengths of inf
    //       the strong certainty datum will all be treated as if they have identical weighting. This will be surprising
    //       if you're not prepared, but is the best we can do short of total failure. (And it still makes sense 
    //       logically, so it seems OK to do it this way.)
    //

    bool l_OK(false);
    if(OK != nullptr) *OK = false;
    const lin_reg_results<T> ret_on_err;
    lin_reg_results<T> res;

    //Approximating slope, used for converting sigma_x_i to equiv. sigma_f_i.
    T approx_slope;
    if(slope_guess != nullptr){
        approx_slope = *slope_guess;
    }else{
        //Attempt to compute the non-weighted linear regression slope. 
        const auto nwlr = this->Linear_Least_Squares_Regression(&l_OK);
        if(!l_OK){
            if(OK == nullptr) FUNCERR("Standard linear regression failed. Cannot properly propagate unertainties");
            FUNCWARN("Standard linear regression failed. Cannot properly propagate unertainties. Bailing");
            return ret_on_err;
        }
        approx_slope = nwlr.slope;
    }

    //Ensure the data is suitable.
    if(this->size() < 3){
        //If we have two points, there are as many parameters as datum. We cannot even compute the stats...
        if(OK == nullptr) FUNCERR("Unable to perform meaningful linear regression with so few points");
        FUNCWARN("Unable to perform meaningful linear regression with so few points. Bailing");
        return ret_on_err;
    }
    res.N = static_cast<T>(this->size());
    res.dof = res.N - (T)(2);

    //Segregate datum into two groups based on whether the weighting is finite.
    samples_1D<T> fin_data, inf_data;

    //Pass over the datum, preparing them and computing some simple quantities.
    res.sum_x  = (T)(0);
    res.sum_f  = (T)(0);
    res.sum_xx = (T)(0);
    res.sum_xf = (T)(0);

    res.sum_w    = (T)(0);
    res.sum_wx   = (T)(0);
    res.sum_wf   = (T)(0);
    res.sum_wxx  = (T)(0);
    res.sum_wxf  = (T)(0);

    const bool inhibit_sort = true;
    for(const auto &P : this->samples){
        auto CP = P; //"CP" == "copy of p".

        //Using the standard linear regression slope, transform sigma_x_i into equivalent sigma_f_i.
        if(this->uncertainties_known_to_be_independent_and_random){
            CP[3] = std::hypot(CP[3], approx_slope*CP[1]);
        }else{
            CP[3] = std::abs(CP[3]) + std::abs(approx_slope*CP[1]);
        }
        CP[1] = (T)(0);

        //Take care of the un-weighted quantities.
        res.sum_x  += CP[0];
        res.sum_xx += CP[0]*CP[0];
        res.sum_f  += CP[2];
        res.sum_xf += CP[0]*CP[2];

        //Will be infinite if P[3] is zero. If any are finite, all these sums will also be finite.
        const T w_i = (std::isfinite(CP[3])) ? (T)(1)/(CP[3]*CP[3]) : (T)(0);
        if(std::isfinite(w_i)){
            fin_data.push_back(CP,inhibit_sort);
        }else{
            inf_data.push_back(CP,inhibit_sort);
            res.sum_w     = std::numeric_limits<T>::infinity();
            res.sum_wx    = std::numeric_limits<T>::infinity();
            res.sum_wf    = std::numeric_limits<T>::infinity();
            res.sum_wxx   = std::numeric_limits<T>::infinity();
            res.sum_wxf   = std::numeric_limits<T>::infinity();
            continue;
        }
        res.sum_w    += w_i;
        res.sum_wx   += w_i*CP[0];
        res.sum_wxx  += w_i*CP[0]*CP[0];
        res.sum_wf   += w_i*CP[2];
        res.sum_wxf  += w_i*CP[0]*CP[2];
    }
    const bool tainted_by_inf = !inf_data.empty();
    res.mean_x = res.sum_x/res.N;
    res.mean_f = res.sum_f/res.N;
    if(tainted_by_inf){
        res.mean_wx = std::numeric_limits<T>::infinity();
        res.mean_wf = std::numeric_limits<T>::infinity();
    }else{
        res.mean_wx = res.sum_wx/res.N;
        res.mean_wf = res.sum_wf/res.N;
    }


    //Case 1 - all datum have finite weight, and all datum will have been pushed into fin_data.
    if(!tainted_by_inf){
        const T common_denom = res.sum_w*res.sum_wxx - res.sum_wx*res.sum_wx;
        if(!std::isnormal(common_denom)){
            //This cannot be zero, inf, or nan. Proceeding with a sub-normal is also a bad idea.
            if(OK == nullptr) FUNCERR("Encountered difficulties with data. Is the data pathological?");
            FUNCWARN("Encountered difficulties with data. Is the data pathological? Bailing");
            return ret_on_err;
        }

        res.slope     = (res.sum_w*res.sum_wxf - res.sum_wx*res.sum_wf)/common_denom;
        res.intercept = (res.sum_wxx*res.sum_wf - res.sum_wx*res.sum_wxf)/common_denom;
        if(!std::isfinite(res.slope) || !std::isfinite(res.intercept)){
            //While we *could* proceed, something must be off. Maybe an overflow (and inf or nan) during accumulation?
            if(OK == nullptr) FUNCERR("Encountered difficulties computing m and b. Is the data pathological?");
            FUNCWARN("Encountered difficulties computing m and b. Is the data pathological? Bailing");
            return ret_on_err;
        }

        //Now compute the statistical stuff.
        res.chi_square = (T)(0);
        res.Sxf        = (T)(0);
        res.Sxx        = (T)(0);
        res.Sff        = (T)(0);
        for(const auto &P : fin_data.samples){
            res.chi_square += std::pow((P[2] - (res.intercept + res.slope*P[0]))/P[3], 2.0);
            res.Sxf  += (P[0] - res.mean_x)*(P[2] - res.mean_f);
            res.Sxx  += (P[0] - res.mean_x)*(P[0] - res.mean_x);
            res.Sff  += (P[2] - res.mean_f)*(P[2] - res.mean_f);
        }
        res.sigma_slope     = std::sqrt(res.sum_w/common_denom);
        res.sigma_intercept = std::sqrt(res.sum_wxx/common_denom);
        res.covariance      = res.Sxf/res.N;
        res.lin_corr        = res.Sxf/std::sqrt(res.Sxx*res.Sff); //Pearson's linear correlation coefficient.

//TODO : check if this is the correct sigma_f or if I should be using corr_params or cov_params here.

        res.sigma_f         = std::sqrt(res.chi_square/(((T)(1)-std::pow(res.lin_corr,2.0))*res.Sff)); //(Of interest? sigma_f_i were provided.)
        res.qvalue          = Stats::Q_From_ChiSq_Fit(res.chi_square, res.dof);
        res.cov_params      = -res.sum_wx/common_denom;
        res.corr_params     = -res.sum_wx/std::sqrt(res.sum_w*res.sum_wxx);

        if(OK != nullptr) *OK = true;
        return std::move(res);

    //Case 2 - some data had infinite weighting.
    }else{

        if(!fin_data.empty()){
            //Handling this case will require juggling infs and non-infs for all calculations. I have not done this.
            // To get around this, just give very small uncertainties to the problematic data. If this functionality
            // is needed (even though it is a truly stange situation), try doing something like an epsilon argument
            // where a portion of the data has small uncertainty epsilon and the rest has much larger values. Expand
            // all expressions for small epsilon with something like a Taylor expansion, and see if there is a finite
            // expansion.
            if(OK == nullptr) FUNCERR("Data has a mix of zero (or denormal) and non-zero uncertainties. Not (yet?) supported");
            FUNCWARN("Data has a mix of zero (or denormal) and non-zero uncertainties. Not (yet?) supported");
            return ret_on_err;
        }

        //Note: For the rest of the routine, we assume the data all have the same (infinite) weighting, so sums will all
        //      cancel nicely. I'm not sure we could possibly do anything else.

        const T common_denom = res.N*res.sum_xx - res.sum_x*res.sum_x; //Omitting factor [1/sigma_f_i^2].
        if(!std::isnormal(common_denom)){
            //This cannot be zero, inf, or nan. Proceeding with a sub-normal is also a bad idea.
            if(OK == nullptr) FUNCERR("Encountered difficulties with data. Is the data pathological?");
            FUNCWARN("Encountered difficulties with data. Is the data pathological? Bailing");
            return ret_on_err;
        }
        res.slope     = (res.N*res.sum_xf - res.sum_x*res.sum_f)/common_denom;
        res.intercept = (res.sum_xx*res.sum_f - res.sum_x*res.sum_xf)/common_denom;

        res.sum_sq_res = (T)(0);
        res.Sxf = (T)(0);
        res.Sxx = (T)(0);
        res.Sff = (T)(0);
        for(const auto &P : inf_data.samples){

// TODO : check if I need to use WEIGHTED means for these quantities.
            res.sum_sq_res += std::pow(P[2] - (res.intercept + res.slope*P[0]), 2.0);
            res.Sxf += (P[0] - res.mean_x)*(P[2] - res.mean_f);
            res.Sxx += (P[0] - res.mean_x)*(P[0] - res.mean_x);
            res.Sff += (P[2] - res.mean_f)*(P[2] - res.mean_f);
        }
        res.sigma_f         = std::sqrt(res.sum_sq_res/res.dof);
        res.sigma_slope     = (T)(0); //std::sqrt(infSumW/common_denom);
        res.sigma_intercept = (T)(0); //std::sqrt(res.sum_xx/common_denom);
        res.covariance      = res.Sxf/res.N;
        res.lin_corr        = res.Sxf/std::sqrt(res.Sxx*res.Sff); //Pearson's linear correlation coefficient.
        res.chi_square      = std::numeric_limits<T>::infinity(); //Due to sigma_x_i being ~zero.
        res.qvalue          = (T)(0); // Limit as ChiSq ---> inf.

        res.cov_params      = (T)(0); //Infinitely weighted down to zero.
        res.corr_params     = -res.sum_x/std::sqrt(res.N*res.sum_xx);

        if(OK != nullptr) *OK = true;
        return std::move(res);
    }
  
    FUNCERR("Programming error. Should not have got to this point"); 
    return ret_on_err;
}       
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template lin_reg_results<float > samples_1D<float >::Weighted_Linear_Least_Squares_Regression(bool *OK, float  *slope_guess) const;
    template lin_reg_results<double> samples_1D<double>::Weighted_Linear_Least_Squares_Regression(bool *OK, double *slope_guess) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>   bool samples_1D<T>::MetadataKeyPresent(std::string key) const {
    //Checks if the key is present without inspecting the value.
    return (this->metadata.find(key) != this->metadata.end());
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool samples_1D<float >::MetadataKeyPresent(std::string key) const;
    template bool samples_1D<double>::MetadataKeyPresent(std::string key) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> template <class U> std::experimental::optional<U>
samples_1D<T>::GetMetadataValueAs(std::string key) const {
    //Attempts to cast the value if present. Optional is disengaged if key is missing or cast fails.
    const auto metadata_cit = this->metadata.find(key);
    if( (metadata_cit == this->metadata.end())  || !Is_String_An_X<U>(metadata_cit->second) ){
        return std::experimental::optional<U>();
    }else{
        return std::experimental::make_optional(stringtoX<U>(metadata_cit->second));
    }
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::experimental::optional<uint32_t> samples_1D<float >::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<uint32_t> samples_1D<double>::GetMetadataValueAs(std::string key) const;

    template std::experimental::optional<long int> samples_1D<float >::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<long int> samples_1D<double>::GetMetadataValueAs(std::string key) const;

    template std::experimental::optional<double> samples_1D<float >::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<double> samples_1D<double>::GetMetadataValueAs(std::string key) const;

    template std::experimental::optional<std::string> samples_1D<float >::GetMetadataValueAs(std::string key) const;
    template std::experimental::optional<std::string> samples_1D<double>::GetMetadataValueAs(std::string key) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>   bool samples_1D<T>::Write_To_File(const std::string &filename) const {
    //This routine writes the numerical data to file in a 4-column format. It can be directly plotted or otherwise 
    // manipulated by, say, Gnuplot.
    //
    //NOTE: This routine will not overwrite or append an existing file. It will return 'false' on any error or if 
    //      encountering an existing file.
    if(Does_File_Exist_And_Can_Be_Read(filename)) return false;
    return WriteStringToFile(this->Write_To_String(), filename);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool samples_1D<float >::Write_To_File(const std::string &filename) const;
    template bool samples_1D<double>::Write_To_File(const std::string &filename) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>   bool samples_1D<T>::Read_From_File(const std::string &filename){
    //This routine reads numerical data from a file in 4-column format. No metadata is retained or recovered. 
    //
    // NOTE: This routine will return 'false' on any error.
    //
    // NOTE: This routine shouldn't overwrite existing data if a failure is encountered.
    //

    std::ifstream FI(filename, std::ifstream::in);
    if(!FI.good()) return false;

    const bool InhibitSort = false;
    samples_1D<T> indata;

    //Read in the numbers until EOF is encountered.
    std::string line;
    while(FI.good()){
        line.clear();
        std::getline(FI, line);
        if(line.empty()) continue;

//    while(!FI.eof()){
//        std::getline(FI, line);
//        if(FI.eof()) break;
        
        std::size_t nonspace = line.find_first_not_of(" \t");
        if(nonspace == std::string::npos) continue; //Only whitespace.
        if(line[nonspace] == '#') continue; //Comment.

        std::stringstream ss(line);
        T a, b, c, d;
        ss >> a >> b;
        const auto ab_ok = (!ss.fail());
        ss >> c >> d;
        const auto cd_ok = (!ss.fail());
        if(ab_ok && !cd_ok){
            indata.push_back(a, static_cast<T>(0),
                             b, static_cast<T>(0), InhibitSort);
        }else if(ab_ok && cd_ok){
            indata.push_back(a, b, c, d, InhibitSort);
        }
    }

    *this = indata;
    this->metadata.clear();
    return true;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template bool samples_1D<float >::Read_From_File(const std::string &filename);
    template bool samples_1D<double>::Read_From_File(const std::string &filename);
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>   std::string samples_1D<T>::Write_To_String() const {
    std::stringstream out;
    for(const auto &P : this->samples) out << P[0] << " " << P[1] << " " << P[2] << " " << P[3] << std::endl;
    return out.str();
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::string samples_1D<float >::Write_To_String() const;
    template std::string samples_1D<double>::Write_To_String() const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> void samples_1D<T>::Plot(const std::string &Title) const {
    //This routine produces a very simple, default plot of the data.
    //
    Plotter2 plot_coll;
    if(!Title.empty()) plot_coll.Set_Global_Title(Title);
    plot_coll.Insert_samples_1D(*this);//,"", const std::string &linetype = "");
    plot_coll.Plot();
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template void samples_1D<float >::Plot(const std::string &) const;
    template void samples_1D<double>::Plot(const std::string &) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T> void samples_1D<T>::Plot_as_PDF(const std::string &Title, const std::string &Filename_In) const {
    //This routine produces a very simple, default plot of the data.
    Plotter2 plot_coll;
    if(!Title.empty()) plot_coll.Set_Global_Title(Title);
    plot_coll.Insert_samples_1D(*this);//,"", const std::string &linetype = "");
    plot_coll.Plot_as_PDF(Filename_In);
    return;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
//    template void samples_1D<float >::Plot_as_PDF(const std::string &, const std::string &) const;
    template void samples_1D<double>::Plot_as_PDF(const std::string &, const std::string &) const;
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>    std::ostream & operator<<( std::ostream &out, const samples_1D<T> &L ) {
    //Note: This friend is templated (Y) within the templated class (T). We only
    // care about friend template when T==Y.
    out << "(samples_1D.";
    out << " normalityassumed= " << (L.uncertainties_known_to_be_independent_and_random ? 1 : 0);
    out << " num_samples= " << L.size() << " ";
    for(auto s_it = L.samples.begin(); s_it != L.samples.end(); ++s_it){
        out << (*s_it)[0] << " " << (*s_it)[1] << " " << (*s_it)[2] << " " << (*s_it)[3] << " ";
    }
    out << ")";
    return out;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::ostream & operator<<(std::ostream &out, const samples_1D<float > &L );
    template std::ostream & operator<<(std::ostream &out, const samples_1D<double> &L );
#endif
//---------------------------------------------------------------------------------------------------------------------------
template <class T>    std::istream &operator>>( std::istream &in, samples_1D<T> &L ) {
    //Note: This friend is templated (Y) within the templated class (T). We only
    //      care about friend template when T==Y.
    //
    //NOTE: Best to use like: ` std::stringstream("...text...") >> samps; `.
    //
    //NOTE: If you're getting weird crashes and not sure why, try using a fresh stringstream.
    //      Naiively reusing a stringstream will probably cause issues. 
    const bool skip_sort = true;
    L.samples.clear(); 
    std::string grbg;
    long int N;
    long int U;
    in >> grbg; //'(samples_1D.'
    in >> grbg; //'normalityassumed='
    in >> U;    //'1' or '0'
    in >> grbg; //'num_samples='
    in >> N;    //'13'   ...or something...
    //FUNCINFO("N is " << N);
    for(long int i=0; i<N; ++i){
        T x_i, sigma_x_i, f_i, sigma_f_i;
        in >> x_i >> sigma_x_i >> f_i >> sigma_f_i;
        L.push_back({x_i, sigma_x_i, f_i, sigma_f_i}, skip_sort);
    }
    in >> grbg; //')'
    if(U == 1){     L.uncertainties_known_to_be_independent_and_random = true;
    }else{          L.uncertainties_known_to_be_independent_and_random = false;
    }
    L.stable_sort();
    return in;
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template std::istream & operator>>(std::istream &out, samples_1D<float > &L );
    template std::istream & operator>>(std::istream &out, samples_1D<double> &L );
#endif

//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------

template <class C> 
samples_1D<typename C::value_type> 
Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const C &nums, long int N, bool explicitbins){
    //This function takes a list of (possibly unordered) numbers and returns a histogram with N bars of equal width
    // suitable for plotting or further computation. The histogram will be normalized such that each bar will be 
    // (number_of_points_represented_by_bar/total_number_of_points). In other words, the occurence rate.
    //
    using T = typename C::value_type; // float or double.

    const T NDasT = static_cast<T>(nums.size());

    //Put the data into a samples_1D with equal sigma_f_i and no uncertainty.
    const bool inhibit_sort = true; //Temporarily inhibit the sort. We just defer it until all the data is ready.
    samples_1D<T> shtl; //Used to shuttle the data to the histogramming routine.
    for(const auto &P : nums){
        shtl.push_back( { P, (T)(0), (T)(1), (T)(0) }, inhibit_sort );
    }
    shtl.stable_sort();

    //Get a non-normalized histogram out of the data. 
    samples_1D<T> out = shtl.Histogram_Equal_Sized_Bins(N, explicitbins);

    //Normalize the bins with the total number of datum. Ensure the uncertainty is zero, because we have ignored it thusfar.
    for(auto &P : out.samples){
        P[2] /= NDasT;
        P[3] = (T)(0);
    }
    return std::move(out);
}
#ifndef YGORMATH_DISABLE_ALL_SPECIALIZATIONS
    template samples_1D<float > Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const std::list<float > &nums, long int N, bool explicitbins);
    template samples_1D<double> Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const std::list<double> &nums, long int N, bool explicitbins);

    template samples_1D<float > Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const std::vector<float > &nums, long int N, bool explicitbins);
    template samples_1D<double> Bag_of_numbers_to_N_equal_bin_samples_1D_histogram(const std::vector<double> &nums, long int N, bool explicitbins);
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
