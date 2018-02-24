//YgorMisc.h
#ifndef YGOR_MISC_HDR_GRD_H
#define YGOR_MISC_HDR_GRD_H


#include <iostream>
#include <sstream>
#include <cstdlib>  //Needed for popen, pclose.
#include <string>
//#include <ctime>    //Needed for gen_time_random().
#include <climits>  //Needed for CHAR_BIT.
#include <limits>
#include <type_traits>

#include <list>
//#include <set>
#include <algorithm>

#include <unistd.h>              //Needed for read.


//------------------------------------------------------------------------------------------------------
//----------------------------------- Error/Warning/Verbosity macros -----------------------------------
//------------------------------------------------------------------------------------------------------

//------- Executable name variants.
#ifndef YGOR_BASIC_ERRFUNC_
    #define YGOR_BASIC_ERRFUNC_
    #define ERR( x )  { std::cerr << "--(E) " << argv[0] << ": " << x << ". Terminating program." << std::endl; \
                        std::cerr.flush();  \
                        std::exit(-1); }
#endif

#ifndef YGOR_BASIC_WARNFUNC_
    #define YGOR_BASIC_WARNFUNC_
    #define WARN( x )  { std::cout << "--(W) " << argv[0] << ": " << x << "." << std::endl; \
                         std::cout.flush();  }
#endif

#ifndef YGOR_BASIC_INFOFUNC_
    #define YGOR_BASIC_INFOFUNC_
    #define INFO( x )  { std::cout << "--(I) " << argv[0] << ": " << x << "." << std::endl; \
                         std::cout.flush();  }
#endif


//------- Function name variants.
#ifdef __GNUC__ //If using gcc..
    #ifndef __PRETTY_FUNCTION__
        #define __PRETTY_FUNCTION__ __func__
    #endif
#endif //__GNUC__
#ifndef __PRETTY_FUNCTION__   //(this is a fallback!)
    #define __PRETTY_FUNCTION__ '(function name not available)'
#endif


#ifndef FUNCYGOR_BASIC_ERRFUNC_
    #define FUNCYGOR_BASIC_ERRFUNC_
    #define FUNCERR( x )  {std::cerr << "--(E) In function: " << __PRETTY_FUNCTION__; \
                           std::cerr <<  ": " << x << ". Terminating program." << std::endl; \
                           std::cerr.flush();  \
                           std::exit(-1); }
#endif

#ifndef FUNCYGOR_BASIC_WARNFUNC_
    #define FUNCYGOR_BASIC_WARNFUNC_
    #define FUNCWARN( x )  {std::cout << "--(W) In function: " << __PRETTY_FUNCTION__; \
                            std::cout <<  ": " << x << "." << std::endl; \
                            std::cout.flush();  }
#endif


#ifndef FUNCYGOR_BASIC_INFOFUNC_
    #define FUNCYGOR_BASIC_INFOFUNC_
    #define FUNCINFO( x )  {std::cout << "--(I) In function: " << __PRETTY_FUNCTION__; \
                            std::cout <<  ": " << x << "." << std::endl; \
                            std::cout.flush();  }
#endif


//------------------------------------------------------------------------------------------------------
//------------------------------------- Convenience (math) macros --------------------------------------
//------------------------------------------------------------------------------------------------------

#define YGORABS(X)    ((X) < 0 ? -(X) : (X))
#define YGORMAX(A, B) ((A) > (B) ? (A) : (B))
#define YGORMIN(A, B) ((A) < (B) ? (A) : (B))


#ifndef isininc
    //Inclusive_in_range()      isininc( 0, 10, 100) == true
    //                          isininc( 0, 100, 10) == false
    //                          isininc( 0, 10, 10)  == true
    //                          isininc( 10, 10, 10) == true
    #define isininc( A, x, B ) (((x) >= (A)) && ((x) <= (B)))
#endif

//For computing percent error relative to some known value. This should not be used for determining 
// equality of floating point values. See RELATIVE_DIFF for this purpose.
//
// NOTE: The 'known' or 'true' value should be supplied as parameter [B] here. Be aware
//       that the concept of percent difference doesn't make much sense when the true 
//       value is zero (or near machine precision of zero). There is a computational 
//       singularity there. It is better to use the relative-difference in such 
//       situations, if possible.
//
#ifndef PERCENT_ERR
    #define PERCENT_ERR( A, B ) (100.0 * ((B) - (A)) / (B))
#endif

//For computing the relative difference (i.e. the equality to a given threshold). This routine
// should be used like:  if(RELATIVE_DIFF(1.23456, 1.23654) < 1E-3){ //then equivalent within 1E-3.
//
// NOTE: Does this handle both positive and negative floating point zeros? I doubt it...
#ifndef RELATIVE_DIFF
    #define RELATIVE_DIFF( A, B ) \
        (YGORMAX(YGORABS(A), YGORABS(B)) == 0.0 ? 0.0 : \
             YGORABS((A)-(B)) / YGORMAX(YGORABS(A), YGORABS(B)))
#endif


//------------------------------------------------------------------------------------------------------
//----------------------------------- Convenience (Bitwise) macros -------------------------------------
//------------------------------------------------------------------------------------------------------
//
/*
// NOTE: These do not currently work consistently across architectures.
//
template <class T, class R>
inline T BITWISE_ROT_L(const T N, const R S){
    return (((S & sizeof(N)*CHAR_BIT) - 1) == 0) ? N : ((N << ((S & sizeof(N)*CHAR_BIT) - 1)) | (N >> (sizeof(N)*CHAR_BIT - ((S & sizeof(N)*CHAR_BIT) - 1))));
}
template <class T, class R>
inline T BITWISE_ROT_R(const T N, const R S){
    return (((S & sizeof(N)*CHAR_BIT) - 1) == 0) ? N : ((N >> ((S & sizeof(N)*CHAR_BIT) - 1)) | (N << (sizeof(N)*CHAR_BIT - ((S & sizeof(N)*CHAR_BIT) - 1))));
}
*/

template <class T, class R>  inline T PER_BYTE_BITWISE_ROT_L(const T N, const R S);
template <class T, class R>  inline T PER_BYTE_BITWISE_ROT_R(const T N, const R S);

template <class T, class R>
inline T PER_BYTE_BITWISE_ROT_L(const T N, const R S){
    //This routine will not handle 'negative'-leftward rotations. Pass them to sister function.
//    if(static_cast<long int>(S) < 0L) return PER_BYTE_BITWISE_ROT_R(N,-S);
    if(S < static_cast<R>(0)) return PER_BYTE_BITWISE_ROT_R(N,-S);
    const long int SS(static_cast<long int>(S % 8));

    union a_byte_array {
        T num; //The number (N).
        unsigned char dat[sizeof(T)]; //Access to the individual bytes.
    } C;
    C.num = N;

    for(long int i=0; i<static_cast<long int>(sizeof(T)); ++i){
        unsigned char out(C.dat[i]);
        for(long int j=0; j<SS; ++j){
            const unsigned char initially(out);
            out = static_cast<unsigned char>(out << 1);
            if((out >> 1) != initially) out |= 1; //Insert a low-bit.
        }
        C.dat[i] = out;
    }
    return C.num;
}
template <class T, class R>
inline T PER_BYTE_BITWISE_ROT_R(const T N, const R S){
    //This routine will not handle 'negative'-rightward rotations. Pass them to sister function.
//    if(static_cast<long int>(S) < 0L) return PER_BYTE_BITWISE_ROT_L(N,-S);
    if(S < static_cast<R>(0)) return PER_BYTE_BITWISE_ROT_L(N,-S);

    const long int SS(static_cast<long int>(S % 8));

    union a_byte_array {
        T num; //The number (N).
        unsigned char dat[sizeof(T)]; //Access to the individual bytes.
    } C;
    C.num = N;

    for(long int i=0; i<static_cast<long int>(sizeof(T)); ++i){
        unsigned char out(C.dat[i]);
        for(long int j=0; j<SS; ++j){
            const unsigned char initially(out);
            out = static_cast<unsigned char>(out >> 1);
            if((out << 1) != initially) out |= (1 << 7); //128; //Insert a high-bit.
        }
        C.dat[i] = out;
    }
    return C.num;
}


//Checks if a variable, bitwise AND'ed with a bitmask, equals the bitmask. Ensure the ordering is observed
// because the operation is non-commutative.
//
// For example: let A = 0110011
//        and BITMASK = 0010010
//   then A & BITMASK = 0010010 == BITMASK.
//
// For example: let A = 0100011
//        and BITMASK = 0010010
//   then A & BITMASK = 0000010 != BITMASK.
//
// NOTE: This operation is mostly useful for checking for embedded 'flags' within a variable. These flags 
// are set by bitwise ORing them into the variable.
//
#ifndef BITMASK_BITS_ARE_SET
    #define BITMASK_BITS_ARE_SET( A, BITMASK ) \
        ( (A & BITMASK) == BITMASK )
#endif


//------------------------------------------------------------------------------------------------------
//-------------------------------------- Homogeneous Sorting -------------------------------------------
//------------------------------------------------------------------------------------------------------
//These routines can be delegated to to call the correct sorting routine for std::lists and non-
// std::lists. The issue is that std::sort is not specialized to call std::list::sort() when it really
// should. 

//---------------------------------------- Plain Functions ---------------------------------------------
//Function defined when C is a std::list<>.
template <typename C,
          typename std::enable_if<std::is_same<C,typename std::list<typename C::value_type>
                                              >::value
                                 >::type* = nullptr>
void Ygor_Container_Sort(C &c){
    c.sort();
}
//Function defined when C is NOT a std::list<>.
template <typename C,
          typename std::enable_if<!std::is_same<C,typename std::list<typename C::value_type>
                                               >::value
                                 >::type* = nullptr>
void Ygor_Container_Sort(C &c){
    std::sort(c.begin(), c.end());
}

//-------------------------------------- Comparator Functions ------------------------------------------
//Function defined when C is a std::list<>.
template <typename C,
          typename Compare,
          typename std::enable_if<std::is_same<C,typename std::list<typename C::value_type>
                                              >::value
                                 >::type* = nullptr>
void Ygor_Container_Sort(C &c, Compare f){
    c.sort(f);
}
//Function defined when C is NOT a std::list<>.
template <typename C,
          typename Compare,
          typename std::enable_if<!std::is_same<C,typename std::list<typename C::value_type>
                                               >::value
                                 >::type* = nullptr>
void Ygor_Container_Sort(C &c, Compare f){
    std::sort(c.begin(),c.end(),f);
}

//------------------------------------------------------------------------------------------------------
//---------------------------- Alias Casting/Type Packing/Type Conversion ------------------------------
//------------------------------------------------------------------------------------------------------
//This routine 'packs' one data type into another using a union. Accessing an inactive union member is
// NOT officially well-defined behaviour. In fact, it is undefined behaviour. But it is generally 
// portable as many compilers (apparently) support it. An alternative approach would use a 
// reinterpret_cast<>, but this likewise has aliasing issues.
//
// NOTE: Use like:
//         ...
//         uint32_t in = 123;
//         float out = Ygor_Pack_As<uint32_t>(in);
//         ...
//
//       So you invoke with the 'ToType' as a template argument.
//  
/*
template<typename To, typename From>
struct ygor_alias_cast_t {
    union {
        From raw;
        To   data;
    };
};
*/
template<typename ToType, typename FromType> 
inline 
ToType 
Ygor_Pack_As(FromType in){
    static_assert(sizeof(ToType) == sizeof(FromType), 
                  "Cannot Ygor_Alias_Cast<> these types. They do not match in size");
    union {
        FromType from;
        ToType   to;
    } U;
    U.from = in; //Initializes the first member.
    return U.to; //<--- (Technically the point of undefined behaviour.)
}


//This routine changes the type of the input and also scales it to sit in the same place in the range of
// the destination type as it did in the origin type. This conversion only makes sense for integral and
// floating point types, but might also be applicable for some other.
//
// The relative position in the type's range is determined by taking the position between the min/max of
// the origin type and placing the converted value at the same position between the destination min/max.
// 
// NOTE: At the moment, this routine just uses the highest precision floating point type it can to try
//       ignore numerical issues. It will not work well if the input is (1) high precision and (2) near
//       the min or max of the type.
template<typename ToType, typename FromType>
inline
ToType
Ygor_Scale_With_Type_Range(FromType in){

    if(std::is_same<ToType, FromType>::value) return in;

    using intermed_t = long double;
    constexpr auto minFrom = static_cast<intermed_t>(std::numeric_limits<FromType>::min());
    constexpr auto maxFrom = static_cast<intermed_t>(std::numeric_limits<FromType>::max());
    constexpr auto minTo   = static_cast<intermed_t>(std::numeric_limits<ToType>::min());
    constexpr auto maxTo   = static_cast<intermed_t>(std::numeric_limits<ToType>::max());

    const auto alpha_num = (     in - minFrom);
    const auto alpha_den = (maxFrom - minFrom);

    const auto alpha = alpha_num / alpha_den; // Clamped [0,1].

    intermed_t out;
    out = minTo * (static_cast<intermed_t>(1) - alpha);
    out += alpha * maxTo;

    return static_cast<ToType>(out);
}



//------------------------------------------------------------------------------------------------------
//-------------------------------------- Function Declarations -----------------------------------------
//------------------------------------------------------------------------------------------------------


//Execute a given command in a read-only pipe (using popen/pclose.) Return the output.
// Do not use if you do not care about / do not expect output. An empty string should be 
// able to be interpretted as a failure.
inline
std::string 
Execute_Command_In_Pipe(const std::string &cmd){
    std::string out;
    auto pipe = popen(cmd.c_str(), "r");
    if(pipe == nullptr) return out;

    ssize_t nbytes;
    const long int buffsz = 5000;
    char buff[buffsz];

#ifdef EAGAIN
    while( ((nbytes = read(fileno(pipe), buff, buffsz)) != -1)  || (errno == EAGAIN) ){
#else
    while( ((nbytes = read(fileno(pipe), buff, buffsz)) != -1) ){
#endif
        //Check if we have reached the end of the file (ie. "data has run out.")
        if( nbytes == 0 ) break;

        //Otherwise we fill up the buffer to the high-water mark and move on.
        buff[nbytes] = '\0';
        out += std::string(buff,nbytes); //This is done so that in-buffer '\0's don't confuse = operator.
    }
    pclose(pipe);
    return out;
}

#endif
