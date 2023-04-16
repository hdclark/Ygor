//YgorContainers.cc.
#include "YgorMisc.h"
#include "YgorLog.h"
#include <algorithm>    //Needed for std::sort(..).

#include "YgorDefinitions.h"
#include "YgorContainers.h"

//#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
//    #define YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
//#endif

//----------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------ internal template helpers -------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------
//These are helper templates used for type deduction for use *within this file.*
//
//---- Are two types equal? ----   (ala boost::is_same(..)  and adapted from
//  http://stackoverflow.com/questions/1708867/check-type-of-element-in-stl-container-c .
template<class UA, class UB> struct ARE_THESE_TYPES_EQUAL      { static const bool value = false; };
//Specialization for the case when UA == UB.
template<class U>            struct ARE_THESE_TYPES_EQUAL<U,U> { static const bool value = true;  };



//----------------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------- bimap -------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------- Constructors ---------------------------------------------------------

template <class TA, class TB>  bimap<TA,TB>::bimap() { }

#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template bimap<int64_t, std::string>::bimap();
    template bimap<std::string, int64_t>::bimap();

    template bimap<float, std::string>::bimap();
    template bimap<std::string, float>::bimap();
#endif


//--------------------------------------------------- Member functions -------------------------------------------------------
template <class TA, class TB>  TA & bimap<TA,TB>::operator[](const TB &in){
    //First, we have to check if the element exists yet. If it does, we return a reference to the corresponding element.
    for(auto iter = (*this).the_pairs.begin(); iter != (*this).the_pairs.end(); ++iter){
        if((*iter).second == in) return ((*iter).first);
    }
    //If the element did not exist, we create a new element and push out a reference to the corresponding element.
    (*this).the_pairs.push_back( std::pair<TA,TB>( TA(), in ) ); //Default instantiation.
    return ((*(--((*this).the_pairs.end()))).first); //Return a reference to the last element.
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template int64_t &  bimap<int64_t, std::string>::operator[](const std::string &in);
    template std::string &  bimap<std::string, int64_t>::operator[](const int64_t &in);

    template float &  bimap<float, std::string>::operator[](const std::string &in);
    template std::string &  bimap<std::string, float>::operator[](const float &in);
#endif


//This is a mirror reflection of the previous operator. We need to ensure that both functions are not instantiated
// when TA == TB.
template <class TA, class TB>  TB & bimap<TA,TB>::operator[](const TA &in){
    //First, we have to check if the element exists yet. If it does, we return a reference to the corresponding element.
    for(auto iter = (*this).the_pairs.begin(); iter != (*this).the_pairs.end(); ++iter){
        if((*iter).first == in) return ((*iter).second);
    }
    //If the element did not exist, we create a new element and push out a reference to the corresponding element.
    (*this).the_pairs.push_back( std::pair<TA,TB>( in, TB() ) ); //Default instantiation.
    return ((*(--((*this).the_pairs.end()))).second); //Return a reference to the last element.
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template std::string &  bimap<int64_t, std::string>::operator[](const int64_t &in);
    template int64_t &  bimap<std::string, int64_t>::operator[](const std::string &in);

    template std::string &  bimap<float, std::string>::operator[](const float &in);
    template float &  bimap<std::string, float>::operator[](const std::string &in);
#endif


template <class TA, class TB>    bimap<TA,TB> & bimap<TA,TB>::operator=(const std::map<TA,TB> &rhs) {
    this->the_pairs.clear();
    for(auto it = rhs.begin(); it != rhs.end(); ++it){
        (*this)[it->first] = it->second;
    }
    return *this;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template bimap<int64_t, std::string> &  bimap<int64_t, std::string>::operator=(const std::map<int64_t, std::string> &);
    template bimap<std::string, int64_t> &  bimap<std::string, int64_t>::operator=(const std::map<std::string, int64_t> &);

    template bimap<float, std::string> &  bimap<float, std::string>::operator=(const std::map<float, std::string> &);
    template bimap<std::string, float> &  bimap<std::string, float>::operator=(const std::map<std::string, float> &);
#endif

template <class TA, class TB>    bimap<TA,TB> & bimap<TA,TB>::operator=(const std::map<TB,TA> &rhs) {
    this->the_pairs.clear();
    for(auto it = rhs.begin(); it != rhs.end(); ++it){
        (*this)[it->first] = it->second;
    }
    return *this;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template bimap<int64_t, std::string> &  bimap<int64_t, std::string>::operator=(const std::map<std::string, int64_t> &);
    template bimap<std::string, int64_t> &  bimap<std::string, int64_t>::operator=(const std::map<int64_t, std::string> &);

    template bimap<float, std::string> &  bimap<float, std::string>::operator=(const std::map<std::string, float> &);
    template bimap<std::string, float> &  bimap<std::string, float>::operator=(const std::map<float, std::string> &);
#endif


template <class TA, class TB>   typename std::vector<std::pair<TA,TB> >::const_iterator  bimap<TA,TB>::begin(void) const {
    return (*this).the_pairs.begin();
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template  std::vector<std::pair<int64_t,std::string> >::const_iterator   bimap<int64_t,std::string>::begin(void) const;
    template  std::vector<std::pair<std::string,int64_t> >::const_iterator   bimap<std::string,int64_t>::begin(void) const;

    template  std::vector<std::pair<float,std::string> >::const_iterator   bimap<float,std::string>::begin(void) const;
    template  std::vector<std::pair<std::string,float> >::const_iterator   bimap<std::string,float>::begin(void) const;
#endif

template <class TA, class TB>   typename std::vector<std::pair<TA,TB> >::const_reverse_iterator  bimap<TA,TB>::rbegin(void) const {
    return (*this).the_pairs.rbegin();
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template  std::vector<std::pair<int64_t,std::string> >::const_reverse_iterator   bimap<int64_t,std::string>::rbegin(void) const;
    template  std::vector<std::pair<std::string,int64_t> >::const_reverse_iterator   bimap<std::string,int64_t>::rbegin(void) const;

    template  std::vector<std::pair<float,std::string> >::const_reverse_iterator   bimap<float,std::string>::rbegin(void) const;
    template  std::vector<std::pair<std::string,float> >::const_reverse_iterator   bimap<std::string,float>::rbegin(void) const;
#endif


template <class TA, class TB>   typename std::vector<std::pair<TA,TB> >::const_iterator  bimap<TA,TB>::end(void) const {
    return (*this).the_pairs.end();
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template  std::vector<std::pair<int64_t,std::string> >::const_iterator   bimap<int64_t,std::string>::end(void) const;
    template  std::vector<std::pair<std::string,int64_t> >::const_iterator   bimap<std::string,int64_t>::end(void) const;

    template  std::vector<std::pair<float,std::string> >::const_iterator   bimap<float,std::string>::end(void) const;
    template  std::vector<std::pair<std::string,float> >::const_iterator   bimap<std::string,float>::end(void) const;
#endif

template <class TA, class TB>   typename std::vector<std::pair<TA,TB> >::const_reverse_iterator  bimap<TA,TB>::rend(void) const {
    return (*this).the_pairs.rend();
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template  std::vector<std::pair<int64_t,std::string> >::const_reverse_iterator   bimap<int64_t,std::string>::rend(void) const;
    template  std::vector<std::pair<std::string,int64_t> >::const_reverse_iterator   bimap<std::string,int64_t>::rend(void) const;

    template  std::vector<std::pair<float,std::string> >::const_reverse_iterator   bimap<float,std::string>::rend(void) const;
    template  std::vector<std::pair<std::string,float> >::const_reverse_iterator   bimap<std::string,float>::rend(void) const;
#endif

template <class TA, class TB>   typename std::vector< std::pair<TA,TB> >::const_iterator  bimap<TA,TB>::find( const TA &in ) const {
    //We simply run through the elements to see if there is a corresponding element.
    for(auto iter = (*this).the_pairs.begin(); iter != (*this).the_pairs.end(); ++iter){
        if((*iter).first == in) return iter;
    }
    return (*this).the_pairs.end();
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template  std::vector<std::pair<int64_t,std::string> >::const_iterator bimap<int64_t,std::string>::find( const int64_t & ) const;
    template  std::vector<std::pair<std::string,int64_t> >::const_iterator bimap<std::string,int64_t>::find( const std::string & ) const;

    template  std::vector<std::pair<float,std::string> >::const_iterator bimap<float,std::string>::find( const float & ) const;
    template  std::vector<std::pair<std::string,float> >::const_iterator bimap<std::string,float>::find( const std::string & ) const;
#endif


template <class TA, class TB>   typename std::vector< std::pair<TA,TB> >::const_iterator  bimap<TA,TB>::find( const TB &in ) const {
    //We simply run through the elements to see if there is a corresponding element.
    for(auto iter = (*this).the_pairs.begin(); iter != (*this).the_pairs.end(); ++iter){
        if((*iter).second == in) return iter;
    }
    return (*this).the_pairs.end();
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template  std::vector<std::pair<int64_t,std::string> >::const_iterator bimap<int64_t,std::string>::find( const std::string & ) const;
    template  std::vector<std::pair<std::string,int64_t> >::const_iterator bimap<std::string,int64_t>::find( const int64_t & ) const;

    template  std::vector<std::pair<float,std::string> >::const_iterator bimap<float,std::string>::find( const std::string & ) const;
    template  std::vector<std::pair<std::string,float> >::const_iterator bimap<std::string,float>::find( const float & ) const;
#endif



template <class TA, class TB> template <class T>  typename std::vector< std::pair<TA,TB> >::const_iterator  bimap<TA,TB>::after( const T &in ) const {
    //First, we determine if the element passed in exists.
    auto it = (*this).find(in);
    if( it == (*this).end() ) return it; //Return the not-found signal.
    //If the element did exist, we iterate it appropriately.
    if( (++it) == (*this).end() ) return (*this).begin();  //Cyclic treatment!
    return it;
} 
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template std::vector< std::pair<int64_t,std::string> >::const_iterator bimap<int64_t,std::string>::after( const std::string & ) const;
    template std::vector< std::pair<int64_t,std::string> >::const_iterator bimap<int64_t,std::string>::after( const int64_t & ) const;
    template std::vector< std::pair<std::string,int64_t> >::const_iterator bimap<std::string,int64_t>::after( const std::string & ) const;
    template std::vector< std::pair<std::string,int64_t> >::const_iterator bimap<std::string,int64_t>::after( const int64_t & ) const;

    template std::vector< std::pair<float,std::string> >::const_iterator bimap<float,std::string>::after( const std::string & ) const;
    template std::vector< std::pair<float,std::string> >::const_iterator bimap<float,std::string>::after( const float & ) const;
    template std::vector< std::pair<std::string,float> >::const_iterator bimap<std::string,float>::after( const std::string & ) const;
    template std::vector< std::pair<std::string,float> >::const_iterator bimap<std::string,float>::after( const float & ) const;
#endif

 

template <class TA, class TB> template <class T>  typename std::vector< std::pair<TA,TB> >::const_iterator  bimap<TA,TB>::before( const T &in ) const {
    //First, we determine if the element passed in exists.
    auto it = (*this).find(in);
    if( it == (*this).end() ) return it; //Return the not-found signal.
    //If the element did exist, we iterate it appropriately.
    if( it == (*this).begin() ) return --((*this).end());
    return (--it);
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template std::vector< std::pair<int64_t,std::string> >::const_iterator bimap<int64_t,std::string>::before( const std::string & ) const;
    template std::vector< std::pair<int64_t,std::string> >::const_iterator bimap<int64_t,std::string>::before( const int64_t & ) const;
    template std::vector< std::pair<std::string,int64_t> >::const_iterator bimap<std::string,int64_t>::before( const std::string & ) const;
    template std::vector< std::pair<std::string,int64_t> >::const_iterator bimap<std::string,int64_t>::before( const int64_t & ) const;

    template std::vector< std::pair<float,std::string> >::const_iterator bimap<float,std::string>::before( const std::string & ) const;
    template std::vector< std::pair<float,std::string> >::const_iterator bimap<float,std::string>::before( const float & ) const;
    template std::vector< std::pair<std::string,float> >::const_iterator bimap<std::string,float>::before( const std::string & ) const;
    template std::vector< std::pair<std::string,float> >::const_iterator bimap<std::string,float>::before( const float & ) const;
#endif


template <class TA, class TB> template <class T> T bimap<TA,TB>::get_next( const T &in ) const {
    //First, we take the given value and attempt to get an iterator to the element after it (cyclically.)
    auto it = this->after(in);
    if( it == this->end() ) return in;  //Simply return the value given.    Maybe we should throw an exception instead??

    //Now some tricky footwork. To keep this function general, we cannot indicate the .first or .second parts explicitly.
    // Instead, we use type checking. Do not be frightened by the casts - the types are required to be identical!
    if( ARE_THESE_TYPES_EQUAL<TA,T>::value ){
        return *(reinterpret_cast<const T*>( &(it->first) ));
    }else if( ARE_THESE_TYPES_EQUAL<TB,T>::value ){
        return *(reinterpret_cast<const T*>( &(it->second) ));  
    }
    YLOGERR("Attempted to perform impossible action. Was a wrong type passed to this bimap?");
    return in;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template std::string bimap<int64_t,std::string>::get_next( const std::string & ) const;
    template int64_t    bimap<int64_t,std::string>::get_next( const int64_t & ) const;
    template std::string bimap<std::string,int64_t>::get_next( const std::string & ) const;
    template int64_t    bimap<std::string,int64_t>::get_next( const int64_t & ) const;

    template std::string bimap<float,std::string>::get_next( const std::string & ) const;
    template float       bimap<float,std::string>::get_next( const float & ) const;
    template std::string bimap<std::string,float>::get_next( const std::string & ) const;
    template float       bimap<std::string,float>::get_next( const float & ) const;
#endif


template <class TA, class TB> template <class T> T bimap<TA,TB>::get_previous( const T &in ) const {
    //First, we take the given value and attempt to get an iterator to the element after it (cyclically.)
    auto it = this->before(in);
    if( it == this->end() ) return in;  //Simply return the value given.    Maybe we should throw an exception instead??

    //Now some tricky footwork. To keep this function general, we cannot indicate the .first or .second parts explicitly.
    // Instead, we use type checking. Do not be frightened by the casts - the types are required to be identical!
    if( ARE_THESE_TYPES_EQUAL<TA,T>::value ){
        return *(reinterpret_cast<const T*>( &((*it).first) ));
    }else if( ARE_THESE_TYPES_EQUAL<TB,T>::value ){
        return *(reinterpret_cast<const T*>( &((*it).second) ));
    }
    YLOGERR("Attempted to perform impossible action. Was a wrong type passed to this bimap?");
    return in;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template std::string bimap<int64_t,std::string>::get_previous( const std::string & ) const;
    template int64_t bimap<int64_t,std::string>::get_previous( const int64_t & ) const;
    template std::string bimap<std::string,int64_t>::get_previous( const std::string & ) const;
    template int64_t bimap<std::string,int64_t>::get_previous( const int64_t & ) const;

    template std::string bimap<float,std::string>::get_previous( const std::string & ) const;
    template float bimap<float,std::string>::get_previous( const float & ) const;
    template std::string bimap<std::string,float>::get_previous( const std::string & ) const;
    template float bimap<std::string,float>::get_previous( const float & ) const;
#endif


template <class TA, class TB>  void bimap<TA,TB>::order_on_first(void){
    auto lessthan = [](const std::pair<TA,TB> &A, const std::pair<TA,TB> &B) -> bool {
        return A.first < B.first;
    };
    std::sort(this->the_pairs.begin(), this->the_pairs.end(), lessthan);
    return;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template void bimap<int64_t,std::string>::order_on_first(void);
    template void bimap<std::string,int64_t>::order_on_first(void);

    template void bimap<float,std::string>::order_on_first(void);
    template void bimap<std::string,float>::order_on_first(void);
#endif

template <class TA, class TB>  void bimap<TA,TB>::order_on_second(void){
    auto lessthan = [](const std::pair<TA,TB> &A, const std::pair<TA,TB> &B) -> bool {
        return A.second < B.second;
    };
    std::sort(this->the_pairs.begin(), this->the_pairs.end(), lessthan);
    return;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template void bimap<int64_t,std::string>::order_on_second(void);
    template void bimap<std::string,int64_t>::order_on_second(void);

    template void bimap<float,std::string>::order_on_second(void);
    template void bimap<std::string,float>::order_on_second(void);
#endif

//---------------------------------------------------------------------------------------------------------------------
//--------------------------- yspan: a non-owning sequence proxy object supporting stride -----------------------------
//---------------------------------------------------------------------------------------------------------------------

template <class T>
yspan<T>::yspan() : start(nullptr), count(0), stride_bytes(0) {}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template yspan<float   >::yspan();
    template yspan<double  >::yspan();

    template yspan<int8_t  >::yspan();
    template yspan<int16_t >::yspan();
    template yspan<int32_t >::yspan();
    template yspan<int64_t >::yspan();

    template yspan<uint8_t >::yspan();
    template yspan<uint16_t>::yspan();
    template yspan<uint32_t>::yspan();
    template yspan<uint64_t>::yspan();

    template yspan<const float   >::yspan();
    template yspan<const double  >::yspan();
                         
    template yspan<const int8_t  >::yspan();
    template yspan<const int16_t >::yspan();
    template yspan<const int32_t >::yspan();
    template yspan<const int64_t >::yspan();
                         
    template yspan<const uint8_t >::yspan();
    template yspan<const uint16_t>::yspan();
    template yspan<const uint32_t>::yspan();
    template yspan<const uint64_t>::yspan();
#endif

template <class T>
yspan<T>::yspan(T* l_start, int64_t l_count, int64_t l_sb) : start(l_start), count(l_count), stride_bytes(l_sb) {
    if(this->start == nullptr){
        throw std::invalid_argument("Invalid span: first element is null");
    }
    if(this->count < 0L){
        throw std::invalid_argument("Invalid span: count is negative");
    }
    if(this->stride_bytes < 0L){
        throw std::invalid_argument("Invalid span: stride is negative");
    }
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template yspan<float   >::yspan(float*   , int64_t, int64_t);
    template yspan<double  >::yspan(double*  , int64_t, int64_t);

    template yspan<int8_t  >::yspan(int8_t*  , int64_t, int64_t);
    template yspan<int16_t >::yspan(int16_t* , int64_t, int64_t);
    template yspan<int32_t >::yspan(int32_t* , int64_t, int64_t);
    template yspan<int64_t >::yspan(int64_t* , int64_t, int64_t);

    template yspan<uint8_t >::yspan(uint8_t* , int64_t, int64_t);
    template yspan<uint16_t>::yspan(uint16_t*, int64_t, int64_t);
    template yspan<uint32_t>::yspan(uint32_t*, int64_t, int64_t);
    template yspan<uint64_t>::yspan(uint64_t*, int64_t, int64_t);

    template yspan<const float   >::yspan(const float*   , int64_t, int64_t);
    template yspan<const double  >::yspan(const double*  , int64_t, int64_t);
                                                
    template yspan<const int8_t  >::yspan(const int8_t*  , int64_t, int64_t);
    template yspan<const int16_t >::yspan(const int16_t* , int64_t, int64_t);
    template yspan<const int32_t >::yspan(const int32_t* , int64_t, int64_t);
    template yspan<const int64_t >::yspan(const int64_t* , int64_t, int64_t);
                                                
    template yspan<const uint8_t >::yspan(const uint8_t* , int64_t, int64_t);
    template yspan<const uint16_t>::yspan(const uint16_t*, int64_t, int64_t);
    template yspan<const uint32_t>::yspan(const uint32_t*, int64_t, int64_t);
    template yspan<const uint64_t>::yspan(const uint64_t*, int64_t, int64_t);
#endif

template <class T>
yspan<T>::yspan(const yspan &rhs) : start(rhs.start),
                                    count(rhs.count),
                                    stride_bytes(rhs.stride_bytes) { }
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template yspan<float   >::yspan(const yspan<float   > &);
    template yspan<double  >::yspan(const yspan<double  > &);

    template yspan<int8_t  >::yspan(const yspan<int8_t  > &);
    template yspan<int16_t >::yspan(const yspan<int16_t > &);
    template yspan<int32_t >::yspan(const yspan<int32_t > &);
    template yspan<int64_t >::yspan(const yspan<int64_t > &);

    template yspan<uint8_t >::yspan(const yspan<uint8_t > &);
    template yspan<uint16_t>::yspan(const yspan<uint16_t> &);
    template yspan<uint32_t>::yspan(const yspan<uint32_t> &);
    template yspan<uint64_t>::yspan(const yspan<uint64_t> &);

    template yspan<const float   >::yspan(const yspan<const float   > &);
    template yspan<const double  >::yspan(const yspan<const double  > &);

    template yspan<const int8_t  >::yspan(const yspan<const int8_t  > &);
    template yspan<const int16_t >::yspan(const yspan<const int16_t > &);
    template yspan<const int32_t >::yspan(const yspan<const int32_t > &);
    template yspan<const int64_t >::yspan(const yspan<const int64_t > &);

    template yspan<const uint8_t >::yspan(const yspan<const uint8_t > &);
    template yspan<const uint16_t>::yspan(const yspan<const uint16_t> &);
    template yspan<const uint32_t>::yspan(const yspan<const uint32_t> &);
    template yspan<const uint64_t>::yspan(const yspan<const uint64_t> &);
#endif

template <class T>
yspan<T> &
yspan<T>::operator=(const yspan &rhs){
    if(this == &rhs) return *this;
    this->start = rhs.start;
    this->count = rhs.count;
    this->stride_bytes = rhs.stride_bytes;
    return *this;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template yspan<float   >& yspan<float   >::operator=(const yspan<float   > &);
    template yspan<double  >& yspan<double  >::operator=(const yspan<double  > &);
                             
    template yspan<int8_t  >& yspan<int8_t  >::operator=(const yspan<int8_t  > &);
    template yspan<int16_t >& yspan<int16_t >::operator=(const yspan<int16_t > &);
    template yspan<int32_t >& yspan<int32_t >::operator=(const yspan<int32_t > &);
    template yspan<int64_t >& yspan<int64_t >::operator=(const yspan<int64_t > &);
                             
    template yspan<uint8_t >& yspan<uint8_t >::operator=(const yspan<uint8_t > &);
    template yspan<uint16_t>& yspan<uint16_t>::operator=(const yspan<uint16_t> &);
    template yspan<uint32_t>& yspan<uint32_t>::operator=(const yspan<uint32_t> &);
    template yspan<uint64_t>& yspan<uint64_t>::operator=(const yspan<uint64_t> &);

    template yspan<const float   >& yspan<const float   >::operator=(const yspan<const float   > &);
    template yspan<const double  >& yspan<const double  >::operator=(const yspan<const double  > &);
                                                                                       
    template yspan<const int8_t  >& yspan<const int8_t  >::operator=(const yspan<const int8_t  > &);
    template yspan<const int16_t >& yspan<const int16_t >::operator=(const yspan<const int16_t > &);
    template yspan<const int32_t >& yspan<const int32_t >::operator=(const yspan<const int32_t > &);
    template yspan<const int64_t >& yspan<const int64_t >::operator=(const yspan<const int64_t > &);
                                                                                       
    template yspan<const uint8_t >& yspan<const uint8_t >::operator=(const yspan<const uint8_t > &);
    template yspan<const uint16_t>& yspan<const uint16_t>::operator=(const yspan<const uint16_t> &);
    template yspan<const uint32_t>& yspan<const uint32_t>::operator=(const yspan<const uint32_t> &);
    template yspan<const uint64_t>& yspan<const uint64_t>::operator=(const yspan<const uint64_t> &);
#endif

template <class T>
bool
yspan<T>::operator==(const yspan<T> &rhs) const {
    return (this->start == rhs.start)
        && (this->count == rhs.count)
        && (this->stride_bytes == rhs.stride_bytes);
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template bool yspan<float   >::operator==(const yspan<float   > &) const;
    template bool yspan<double  >::operator==(const yspan<double  > &) const;

    template bool yspan<int8_t  >::operator==(const yspan<int8_t  > &) const;
    template bool yspan<int16_t >::operator==(const yspan<int16_t > &) const;
    template bool yspan<int32_t >::operator==(const yspan<int32_t > &) const;
    template bool yspan<int64_t >::operator==(const yspan<int64_t > &) const;

    template bool yspan<uint8_t >::operator==(const yspan<uint8_t > &) const;
    template bool yspan<uint16_t>::operator==(const yspan<uint16_t> &) const;
    template bool yspan<uint32_t>::operator==(const yspan<uint32_t> &) const;
    template bool yspan<uint64_t>::operator==(const yspan<uint64_t> &) const;

    template bool yspan<const float   >::operator==(const yspan<const float   > &) const;
    template bool yspan<const double  >::operator==(const yspan<const double  > &) const;
                                                                      
    template bool yspan<const int8_t  >::operator==(const yspan<const int8_t  > &) const;
    template bool yspan<const int16_t >::operator==(const yspan<const int16_t > &) const;
    template bool yspan<const int32_t >::operator==(const yspan<const int32_t > &) const;
    template bool yspan<const int64_t >::operator==(const yspan<const int64_t > &) const;
                                                                      
    template bool yspan<const uint8_t >::operator==(const yspan<const uint8_t > &) const;
    template bool yspan<const uint16_t>::operator==(const yspan<const uint16_t> &) const;
    template bool yspan<const uint32_t>::operator==(const yspan<const uint32_t> &) const;
    template bool yspan<const uint64_t>::operator==(const yspan<const uint64_t> &) const;
#endif

template <class T>
bool
yspan<T>::operator!=(const yspan<T> &rhs) const {
    return !(*this == rhs);
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template bool yspan<float   >::operator!=(const yspan<float   > &) const;
    template bool yspan<double  >::operator!=(const yspan<double  > &) const;

    template bool yspan<int8_t  >::operator!=(const yspan<int8_t  > &) const;
    template bool yspan<int16_t >::operator!=(const yspan<int16_t > &) const;
    template bool yspan<int32_t >::operator!=(const yspan<int32_t > &) const;
    template bool yspan<int64_t >::operator!=(const yspan<int64_t > &) const;

    template bool yspan<uint8_t >::operator!=(const yspan<uint8_t > &) const;
    template bool yspan<uint16_t>::operator!=(const yspan<uint16_t> &) const;
    template bool yspan<uint32_t>::operator!=(const yspan<uint32_t> &) const;
    template bool yspan<uint64_t>::operator!=(const yspan<uint64_t> &) const;

    template bool yspan<const float   >::operator!=(const yspan<const float   > &) const;
    template bool yspan<const double  >::operator!=(const yspan<const double  > &) const;
                                                                      
    template bool yspan<const int8_t  >::operator!=(const yspan<const int8_t  > &) const;
    template bool yspan<const int16_t >::operator!=(const yspan<const int16_t > &) const;
    template bool yspan<const int32_t >::operator!=(const yspan<const int32_t > &) const;
    template bool yspan<const int64_t >::operator!=(const yspan<const int64_t > &) const;
                                                                      
    template bool yspan<const uint8_t >::operator!=(const yspan<const uint8_t > &) const;
    template bool yspan<const uint16_t>::operator!=(const yspan<const uint16_t> &) const;
    template bool yspan<const uint32_t>::operator!=(const yspan<const uint32_t> &) const;
    template bool yspan<const uint64_t>::operator!=(const yspan<const uint64_t> &) const;
#endif

template <class T>
bool
yspan<T>::operator<(const yspan<T> &rhs) const {
    return std::make_tuple(this->start, this->count, this->stride_bytes)
         < std::make_tuple(rhs.start,   rhs.count,   rhs.stride_bytes);
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template bool yspan<float   >::operator<(const yspan<float   > &) const;
    template bool yspan<double  >::operator<(const yspan<double  > &) const;

    template bool yspan<int8_t  >::operator<(const yspan<int8_t  > &) const;
    template bool yspan<int16_t >::operator<(const yspan<int16_t > &) const;
    template bool yspan<int32_t >::operator<(const yspan<int32_t > &) const;
    template bool yspan<int64_t >::operator<(const yspan<int64_t > &) const;

    template bool yspan<uint8_t >::operator<(const yspan<uint8_t > &) const;
    template bool yspan<uint16_t>::operator<(const yspan<uint16_t> &) const;
    template bool yspan<uint32_t>::operator<(const yspan<uint32_t> &) const;
    template bool yspan<uint64_t>::operator<(const yspan<uint64_t> &) const;

    template bool yspan<const float   >::operator<(const yspan<const float   > &) const;
    template bool yspan<const double  >::operator<(const yspan<const double  > &) const;
                                                                     
    template bool yspan<const int8_t  >::operator<(const yspan<const int8_t  > &) const;
    template bool yspan<const int16_t >::operator<(const yspan<const int16_t > &) const;
    template bool yspan<const int32_t >::operator<(const yspan<const int32_t > &) const;
    template bool yspan<const int64_t >::operator<(const yspan<const int64_t > &) const;
                                                                     
    template bool yspan<const uint8_t >::operator<(const yspan<const uint8_t > &) const;
    template bool yspan<const uint16_t>::operator<(const yspan<const uint16_t> &) const;
    template bool yspan<const uint32_t>::operator<(const yspan<const uint32_t> &) const;
    template bool yspan<const uint64_t>::operator<(const yspan<const uint64_t> &) const;
#endif

template <class T>
T&
yspan<T>::operator[](int64_t n){
    using base_T = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
    return *( reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(const_cast<base_T*>(start)) + n * (sizeof(T) + this->stride_bytes)));
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template float   & yspan<float   >::operator[](int64_t n);
    template double  & yspan<double  >::operator[](int64_t n);

    template int8_t  & yspan<int8_t  >::operator[](int64_t n);
    template int16_t & yspan<int16_t >::operator[](int64_t n);
    template int32_t & yspan<int32_t >::operator[](int64_t n);
    template int64_t & yspan<int64_t >::operator[](int64_t n);

    template uint8_t & yspan<uint8_t >::operator[](int64_t n);
    template uint16_t& yspan<uint16_t>::operator[](int64_t n);
    template uint32_t& yspan<uint32_t>::operator[](int64_t n);
    template uint64_t& yspan<uint64_t>::operator[](int64_t n);

    template const float   & yspan<const float   >::operator[](int64_t n);
    template const double  & yspan<const double  >::operator[](int64_t n);

    template const int8_t  & yspan<const int8_t  >::operator[](int64_t n);
    template const int16_t & yspan<const int16_t >::operator[](int64_t n);
    template const int32_t & yspan<const int32_t >::operator[](int64_t n);
    template const int64_t & yspan<const int64_t >::operator[](int64_t n);

    template const uint8_t & yspan<const uint8_t >::operator[](int64_t n);
    template const uint16_t& yspan<const uint16_t>::operator[](int64_t n);
    template const uint32_t& yspan<const uint32_t>::operator[](int64_t n);
    template const uint64_t& yspan<const uint64_t>::operator[](int64_t n);
#endif

template <class T>
T&
yspan<T>::at(int64_t n){
    if(this->start == nullptr){
        throw std::runtime_error("Invalid yspan element access; yspan is not engaged");
    }
    if((n < 0L) || (this->count <= n)){
        throw std::runtime_error("Invalid yspan element access; element does not exist");
    }
    return (*this)[n];
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template float   & yspan<float   >::at(int64_t n);
    template double  & yspan<double  >::at(int64_t n);

    template int8_t  & yspan<int8_t  >::at(int64_t n);
    template int16_t & yspan<int16_t >::at(int64_t n);
    template int32_t & yspan<int32_t >::at(int64_t n);
    template int64_t & yspan<int64_t >::at(int64_t n);

    template uint8_t & yspan<uint8_t >::at(int64_t n);
    template uint16_t& yspan<uint16_t>::at(int64_t n);
    template uint32_t& yspan<uint32_t>::at(int64_t n);
    template uint64_t& yspan<uint64_t>::at(int64_t n);

    template const float   & yspan<const float   >::at(int64_t n);
    template const double  & yspan<const double  >::at(int64_t n);

    template const int8_t  & yspan<const int8_t  >::at(int64_t n);
    template const int16_t & yspan<const int16_t >::at(int64_t n);
    template const int32_t & yspan<const int32_t >::at(int64_t n);
    template const int64_t & yspan<const int64_t >::at(int64_t n);

    template const uint8_t & yspan<const uint8_t >::at(int64_t n);
    template const uint16_t& yspan<const uint16_t>::at(int64_t n);
    template const uint32_t& yspan<const uint32_t>::at(int64_t n);
    template const uint64_t& yspan<const uint64_t>::at(int64_t n);
#endif

template <class T>
int64_t
yspan<T>::size() const {
    return std::max<int64_t>(0L, this->count);
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template int64_t yspan<float   >::size() const;
    template int64_t yspan<double  >::size() const;

    template int64_t yspan<int8_t  >::size() const;
    template int64_t yspan<int16_t >::size() const;
    template int64_t yspan<int32_t >::size() const;
    template int64_t yspan<int64_t >::size() const;

    template int64_t yspan<uint8_t >::size() const;
    template int64_t yspan<uint16_t>::size() const;
    template int64_t yspan<uint32_t>::size() const;
    template int64_t yspan<uint64_t>::size() const;

    template int64_t yspan<const float   >::size() const;
    template int64_t yspan<const double  >::size() const;

    template int64_t yspan<const int8_t  >::size() const;
    template int64_t yspan<const int16_t >::size() const;
    template int64_t yspan<const int32_t >::size() const;
    template int64_t yspan<const int64_t >::size() const;

    template int64_t yspan<const uint8_t >::size() const;
    template int64_t yspan<const uint16_t>::size() const;
    template int64_t yspan<const uint32_t>::size() const;
    template int64_t yspan<const uint64_t>::size() const;
#endif

template <class T>
int64_t
yspan<T>::stride() const {
    return std::max<int64_t>(0L, this->stride_bytes);
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template int64_t yspan<float   >::stride() const;
    template int64_t yspan<double  >::stride() const;

    template int64_t yspan<int8_t  >::stride() const;
    template int64_t yspan<int16_t >::stride() const;
    template int64_t yspan<int32_t >::stride() const;
    template int64_t yspan<int64_t >::stride() const;

    template int64_t yspan<uint8_t >::stride() const;
    template int64_t yspan<uint16_t>::stride() const;
    template int64_t yspan<uint32_t>::stride() const;
    template int64_t yspan<uint64_t>::stride() const;

    template int64_t yspan<const float   >::stride() const;
    template int64_t yspan<const double  >::stride() const;

    template int64_t yspan<const int8_t  >::stride() const;
    template int64_t yspan<const int16_t >::stride() const;
    template int64_t yspan<const int32_t >::stride() const;
    template int64_t yspan<const int64_t >::stride() const;

    template int64_t yspan<const uint8_t >::stride() const;
    template int64_t yspan<const uint16_t>::stride() const;
    template int64_t yspan<const uint32_t>::stride() const;
    template int64_t yspan<const uint64_t>::stride() const;
#endif

template <class T>
bool
yspan<T>::empty() const {
    return (this->count < 1L);
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template bool yspan<float   >::empty() const;
    template bool yspan<double  >::empty() const;

    template bool yspan<int8_t  >::empty() const;
    template bool yspan<int16_t >::empty() const;
    template bool yspan<int32_t >::empty() const;
    template bool yspan<int64_t >::empty() const;

    template bool yspan<uint8_t >::empty() const;
    template bool yspan<uint16_t>::empty() const;
    template bool yspan<uint32_t>::empty() const;
    template bool yspan<uint64_t>::empty() const;

    template bool yspan<const float   >::empty() const;
    template bool yspan<const double  >::empty() const;

    template bool yspan<const int8_t  >::empty() const;
    template bool yspan<const int16_t >::empty() const;
    template bool yspan<const int32_t >::empty() const;
    template bool yspan<const int64_t >::empty() const;

    template bool yspan<const uint8_t >::empty() const;
    template bool yspan<const uint16_t>::empty() const;
    template bool yspan<const uint32_t>::empty() const;
    template bool yspan<const uint64_t>::empty() const;
#endif

template <class T>
T&
yspan<T>::front(){
    return this->at(0L);
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template float   & yspan<float   >::front();
    template double  & yspan<double  >::front();
                      
    template int8_t  & yspan<int8_t  >::front();
    template int16_t & yspan<int16_t >::front();
    template int32_t & yspan<int32_t >::front();
    template int64_t & yspan<int64_t >::front();
                      
    template uint8_t & yspan<uint8_t >::front();
    template uint16_t& yspan<uint16_t>::front();
    template uint32_t& yspan<uint32_t>::front();
    template uint64_t& yspan<uint64_t>::front();

    template const float   & yspan<const float   >::front();
    template const double  & yspan<const double  >::front();
                                                                                
    template const int8_t  & yspan<const int8_t  >::front();
    template const int16_t & yspan<const int16_t >::front();
    template const int32_t & yspan<const int32_t >::front();
    template const int64_t & yspan<const int64_t >::front();
                                                                                
    template const uint8_t & yspan<const uint8_t >::front();
    template const uint16_t& yspan<const uint16_t>::front();
    template const uint32_t& yspan<const uint32_t>::front();
    template const uint64_t& yspan<const uint64_t>::front();
#endif

template <class T>
T&
yspan<T>::back(){
    return this->at( this->count - 1L );
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template float   & yspan<float   >::back();
    template double  & yspan<double  >::back();
                      
    template int8_t  & yspan<int8_t  >::back();
    template int16_t & yspan<int16_t >::back();
    template int32_t & yspan<int32_t >::back();
    template int64_t & yspan<int64_t >::back();
                      
    template uint8_t & yspan<uint8_t >::back();
    template uint16_t& yspan<uint16_t>::back();
    template uint32_t& yspan<uint32_t>::back();
    template uint64_t& yspan<uint64_t>::back();

    template const float   & yspan<const float   >::back();
    template const double  & yspan<const double  >::back();
                                                                                
    template const int8_t  & yspan<const int8_t  >::back();
    template const int16_t & yspan<const int16_t >::back();
    template const int32_t & yspan<const int32_t >::back();
    template const int64_t & yspan<const int64_t >::back();
                                                                                
    template const uint8_t & yspan<const uint8_t >::back();
    template const uint16_t& yspan<const uint16_t>::back();
    template const uint32_t& yspan<const uint32_t>::back();
    template const uint64_t& yspan<const uint64_t>::back();
#endif

template <class T>
void
yspan<T>::pop_front(){
    T* new_start = &(this->at(1L));
    this->start = new_start;
    this->count--;
    return;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template void yspan<float   >::pop_front();
    template void yspan<double  >::pop_front();

    template void yspan<int8_t  >::pop_front();
    template void yspan<int16_t >::pop_front();
    template void yspan<int32_t >::pop_front();
    template void yspan<int64_t >::pop_front();

    template void yspan<uint8_t >::pop_front();
    template void yspan<uint16_t>::pop_front();
    template void yspan<uint32_t>::pop_front();
    template void yspan<uint64_t>::pop_front();

    template void yspan<const float   >::pop_front();
    template void yspan<const double  >::pop_front();

    template void yspan<const int8_t  >::pop_front();
    template void yspan<const int16_t >::pop_front();
    template void yspan<const int32_t >::pop_front();
    template void yspan<const int64_t >::pop_front();

    template void yspan<const uint8_t >::pop_front();
    template void yspan<const uint16_t>::pop_front();
    template void yspan<const uint32_t>::pop_front();
    template void yspan<const uint64_t>::pop_front();
#endif

template <class T>
void
yspan<T>::pop_back(){
    this->count--;
    return;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template void yspan<float   >::pop_back();
    template void yspan<double  >::pop_back();

    template void yspan<int8_t  >::pop_back();
    template void yspan<int16_t >::pop_back();
    template void yspan<int32_t >::pop_back();
    template void yspan<int64_t >::pop_back();

    template void yspan<uint8_t >::pop_back();
    template void yspan<uint16_t>::pop_back();
    template void yspan<uint32_t>::pop_back();
    template void yspan<uint64_t>::pop_back();

    template void yspan<const float   >::pop_back();
    template void yspan<const double  >::pop_back();

    template void yspan<const int8_t  >::pop_back();
    template void yspan<const int16_t >::pop_back();
    template void yspan<const int32_t >::pop_back();
    template void yspan<const int64_t >::pop_back();

    template void yspan<const uint8_t >::pop_back();
    template void yspan<const uint16_t>::pop_back();
    template void yspan<const uint32_t>::pop_back();
    template void yspan<const uint64_t>::pop_back();
#endif

template <class T>
void
yspan<T>::swap(yspan<T> &rhs){
    std::swap(this->start, rhs.start);
    std::swap(this->count, rhs.count);
    std::swap(this->stride_bytes, rhs.stride_bytes);
    return;
}
#ifndef YGORCONTAINERS_DISABLE_ALL_SPECIALIZATIONS
    template void yspan<float   >::swap(yspan<float   >&);
    template void yspan<double  >::swap(yspan<double  >&);

    template void yspan<int8_t  >::swap(yspan<int8_t  >&);
    template void yspan<int16_t >::swap(yspan<int16_t >&);
    template void yspan<int32_t >::swap(yspan<int32_t >&);
    template void yspan<int64_t >::swap(yspan<int64_t >&);

    template void yspan<uint8_t >::swap(yspan<uint8_t >&);
    template void yspan<uint16_t>::swap(yspan<uint16_t>&);
    template void yspan<uint32_t>::swap(yspan<uint32_t>&);
    template void yspan<uint64_t>::swap(yspan<uint64_t>&);

    template void yspan<const float   >::swap(yspan<const float   >&);
    template void yspan<const double  >::swap(yspan<const double  >&);

    template void yspan<const int8_t  >::swap(yspan<const int8_t  >&);
    template void yspan<const int16_t >::swap(yspan<const int16_t >&);
    template void yspan<const int32_t >::swap(yspan<const int32_t >&);
    template void yspan<const int64_t >::swap(yspan<const int64_t >&);

    template void yspan<const uint8_t >::swap(yspan<const uint8_t >&);
    template void yspan<const uint16_t>::swap(yspan<const uint16_t>&);
    template void yspan<const uint32_t>::swap(yspan<const uint32_t>&);
    template void yspan<const uint64_t>::swap(yspan<const uint64_t>&);
#endif


